//! Work-Stealing Lock-Free Graph Scheduler Core
//! Implements 64-byte cacheline aligned Chase-Lev lock-free circular deques,
//! lock-free multi-worker task stealing, DAG dependency tracking, and dynamic step dispatch.
//! Follows Rules 4, 9, 10, 26, 46, 47 (Zero allocations in audio thread, cacheline alignment, lock-free).

use std::sync::atomic::{AtomicI32, AtomicI64, AtomicU32, AtomicU64, AtomicUsize, Ordering};

pub const MAX_STEPS: usize = 64;
pub const MAX_WORKERS: usize = 4;
pub const TOTAL_QUEUES: usize = MAX_WORKERS + 1; // 5 (Worker 0 = Audio thread, 1..4 = Background workers)
pub const MAX_QUEUE_CAPACITY: usize = 256;

/// 64-byte cacheline aligned Chase-Lev lock-free circular deque (Work-Stealing Deque).
/// - Owner thread pushes and pops at bottom (LIFO: maximum L1/L2 cache locality).
/// - Idle stealer threads steal from top (FIFO).
/// - Cacheline aligned to eradicate false sharing between CPU cores.
#[repr(align(64))]
pub struct RustWorkStealingQueue {
    top: AtomicI64,
    bottom: AtomicI64,
    capacity: usize,
    mask: i64,
    buffer: [AtomicU32; MAX_QUEUE_CAPACITY],
}

impl RustWorkStealingQueue {
    pub fn new(capacity: usize) -> Self {
        let cap = capacity.next_power_of_two().clamp(16, MAX_QUEUE_CAPACITY);
        const INIT_ATOMIC: AtomicU32 = AtomicU32::new(0);
        Self {
            top: AtomicI64::new(0),
            bottom: AtomicI64::new(0),
            capacity: cap,
            mask: (cap - 1) as i64,
            buffer: [INIT_ATOMIC; MAX_QUEUE_CAPACITY],
        }
    }

    /// Owner thread push (LIFO)
    pub fn push(&self, item: u32) -> bool {
        let b = self.bottom.load(Ordering::Relaxed);
        let t = self.top.load(Ordering::Acquire);
        if b - t >= (self.capacity as i64 - 1) {
            return false; // Queue full
        }
        self.buffer[(b & self.mask) as usize].store(item, Ordering::Relaxed);
        self.bottom.store(b + 1, Ordering::Release);
        true
    }

    /// Owner thread pop (LIFO)
    pub fn pop(&self) -> (bool, u32) {
        let b = self.bottom.load(Ordering::Relaxed) - 1;
        self.bottom.store(b, Ordering::SeqCst);
        let t = self.top.load(Ordering::SeqCst);

        if t <= b {
            let item = self.buffer[(b & self.mask) as usize].load(Ordering::Relaxed);
            if t == b {
                // Last element: race with concurrent stealers
                let success = self
                    .top
                    .compare_exchange(t, t + 1, Ordering::SeqCst, Ordering::Relaxed)
                    .is_ok();
                self.bottom.store(t + 1, Ordering::Relaxed);
                if success {
                    (true, item)
                } else {
                    (false, 0)
                }
            } else {
                (true, item)
            }
        } else {
            self.bottom.store(t, Ordering::Relaxed);
            (false, 0)
        }
    }

    /// Stealer thread steal (FIFO)
    pub fn steal(&self) -> (bool, u32) {
        let t = self.top.load(Ordering::Acquire);
        let b = self.bottom.load(Ordering::Acquire);
        if t >= b {
            return false_steal();
        }

        let item = self.buffer[(t & self.mask) as usize].load(Ordering::Relaxed);
        if self
            .top
            .compare_exchange(t, t + 1, Ordering::SeqCst, Ordering::Relaxed)
            .is_ok()
        {
            (true, item)
        } else {
            (false, 0)
        }
    }

    #[inline(always)]
    pub fn is_empty(&self) -> bool {
        self.top.load(Ordering::Relaxed) >= self.bottom.load(Ordering::Relaxed)
    }

    pub fn clear(&self) {
        self.top.store(0, Ordering::Relaxed);
        self.bottom.store(0, Ordering::Relaxed);
    }
}

#[inline(always)]
fn false_steal() -> (bool, u32) {
    (false, 0)
}

/// Work-Stealing Multi-Threaded Graph Scheduler Core
#[repr(align(64))]
pub struct RustWorkStealingScheduler {
    pub queues: [RustWorkStealingQueue; TOTAL_QUEUES],
    pub pending_deps: [AtomicI32; MAX_STEPS],
    pub steps_remaining: AtomicI32,
    pub total_steps: AtomicUsize,
    pub num_workers: AtomicU32,
    pub active_epoch: AtomicU64,

    // Successor adjacency table: for each step i, list of dependent successor step indices
    successor_counts: [u8; MAX_STEPS],
    successor_indices: [[u8; MAX_STEPS]; MAX_STEPS],
}

impl RustWorkStealingScheduler {
    pub fn new() -> Self {
        const INIT_ATOMIC_I32: AtomicI32 = AtomicI32::new(0);
        Self {
            queues: [
                RustWorkStealingQueue::new(MAX_QUEUE_CAPACITY),
                RustWorkStealingQueue::new(MAX_QUEUE_CAPACITY),
                RustWorkStealingQueue::new(MAX_QUEUE_CAPACITY),
                RustWorkStealingQueue::new(MAX_QUEUE_CAPACITY),
                RustWorkStealingQueue::new(MAX_QUEUE_CAPACITY),
            ],
            pending_deps: [INIT_ATOMIC_I32; MAX_STEPS],
            steps_remaining: AtomicI32::new(0),
            total_steps: AtomicUsize::new(0),
            num_workers: AtomicU32::new(1),
            active_epoch: AtomicU64::new(0),
            successor_counts: [0; MAX_STEPS],
            successor_indices: [[0; MAX_STEPS]; MAX_STEPS],
        }
    }

    pub fn prepare(&mut self, num_workers: u32) {
        let workers = num_workers.clamp(1, MAX_WORKERS as u32);
        self.num_workers.store(workers, Ordering::Release);
        self.steps_remaining.store(0, Ordering::Relaxed);
        self.total_steps.store(0, Ordering::Relaxed);
        self.clear_all_queues();

        for i in 0..MAX_STEPS {
            self.pending_deps[i].store(0, Ordering::Relaxed);
            self.successor_counts[i] = 0;
        }
    }

    pub fn clear_all_queues(&self) {
        for q in 0..TOTAL_QUEUES {
            self.queues[q].clear();
        }
    }

    pub fn init_step(&mut self, step_idx: usize, dep_count: i32, successors: &[u32]) {
        if step_idx < MAX_STEPS {
            self.pending_deps[step_idx].store(dep_count, Ordering::Relaxed);
            let count = successors.len().min(MAX_STEPS);
            self.successor_counts[step_idx] = count as u8;
            for i in 0..count {
                self.successor_indices[step_idx][i] = (successors[i] as u8).min((MAX_STEPS - 1) as u8);
            }
        }
    }

    pub fn start_execution(&mut self, total_steps: usize) {
        let steps = total_steps.min(MAX_STEPS);
        self.total_steps.store(steps, Ordering::Relaxed);

        // 0. Limpiar cualquier residuo previo en las colas
        self.clear_all_queues();

        // 1. Configurar pasos restantes
        self.steps_remaining.store(steps as i32, Ordering::Release);

        // 2. Empujar todos los nodos raíz (sin dependencias pendientes) a la cola local del audio thread (Worker 0)
        for i in 0..steps {
            if self.pending_deps[i].load(Ordering::Relaxed) == 0 {
                self.queues[0].push(i as u32);
            }
        }
    }

    pub fn pop_or_steal(&self, worker_id: usize) -> Option<u32> {
        let total_q = (self.num_workers.load(Ordering::Relaxed) as usize + 1).min(TOTAL_QUEUES);
        if worker_id >= total_q {
            return None;
        }

        // 1. Pop local de la propia cola (LIFO)
        let (popped, item) = self.queues[worker_id].pop();
        if popped {
            return Some(item);
        }

        // 2. Si la cola local está vacía, intentar robar tareas de otros hilos (Work-Stealing FIFO)
        for i in 1..total_q {
            let victim = (worker_id + i) % total_q;
            let (stolen, item) = self.queues[victim].steal();
            if stolen {
                return Some(item);
            }
        }

        None
    }

    pub fn finish_step(&self, worker_id: usize, step_idx: usize) {
        if step_idx >= MAX_STEPS {
            return;
        }

        // Notificar a los sucesores
        let count = self.successor_counts[step_idx] as usize;
        for i in 0..count {
            let succ = self.successor_indices[step_idx][i] as usize;
            if succ < MAX_STEPS {
                if self.pending_deps[succ].fetch_sub(1, Ordering::AcqRel) == 1 {
                    // Todas las dependencias resueltas: despachar inmediatamente a la cola local
                    let w = worker_id.min(TOTAL_QUEUES - 1);
                    self.queues[w].push(succ as u32);
                }
            }
        }

        // Decrementar el contador global de pasos restantes estrictamente DESPUÉS de notificar
        self.steps_remaining.fetch_sub(1, Ordering::Release);
    }

    #[inline(always)]
    pub fn get_steps_remaining(&self) -> i32 {
        self.steps_remaining.load(Ordering::Acquire)
    }

    #[inline(always)]
    pub fn get_num_workers(&self) -> u32 {
        self.num_workers.load(Ordering::Relaxed)
    }

    #[inline(always)]
    pub fn increment_epoch(&self) -> u64 {
        self.active_epoch.fetch_add(1, Ordering::Release) + 1
    }

    #[inline(always)]
    pub fn get_epoch(&self) -> u64 {
        self.active_epoch.load(Ordering::Acquire)
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_work_stealing_queue_push_pop_steal() {
        let queue = RustWorkStealingQueue::new(64);
        for i in 0..32 {
            assert!(queue.push(i));
        }

        // Pop LIFO by owner
        let (pop_ok, val) = queue.pop();
        assert!(pop_ok && val == 31);

        // Steal FIFO by stealer
        let (steal_ok, stolen) = queue.steal();
        assert!(steal_ok && stolen == 0);

        // Empty check
        assert!(!queue.is_empty());
        queue.clear();
        assert!(queue.is_empty());
    }

    #[test]
    fn test_work_stealing_scheduler_dag() {
        let mut scheduler = RustWorkStealingScheduler::new();
        scheduler.prepare(2);

        // DAG: Root (0) -> Branch1 (1), Branch2 (2) -> Leaf (3)
        scheduler.init_step(0, 0, &[1, 2]);
        scheduler.init_step(1, 1, &[3]);
        scheduler.init_step(2, 1, &[3]);
        scheduler.init_step(3, 2, &[]);

        scheduler.start_execution(4);
        assert_eq!(scheduler.get_steps_remaining(), 4);

        // Worker 0 pops root step 0
        let step0 = scheduler.pop_or_steal(0);
        assert_eq!(step0, Some(0));

        // Worker 0 finishes step 0 -> resolves steps 1 & 2
        scheduler.finish_step(0, 0);
        assert_eq!(scheduler.get_steps_remaining(), 3);

        // Worker 0 pops step 2 (LIFO)
        let step_w0 = scheduler.pop_or_steal(0);
        assert_eq!(step_w0, Some(2));

        // Worker 1 steals step 1 (FIFO)
        let step_w1 = scheduler.pop_or_steal(1);
        assert_eq!(step_w1, Some(1));

        // Worker 0 finishes step 2
        scheduler.finish_step(0, 2);
        assert_eq!(scheduler.get_steps_remaining(), 2);

        // Worker 1 finishes step 1 -> step 3 dependency count drops to 0!
        scheduler.finish_step(1, 1);
        assert_eq!(scheduler.get_steps_remaining(), 1);

        // Worker 1 pops step 3
        let step3 = scheduler.pop_or_steal(1);
        assert_eq!(step3, Some(3));

        // Worker 1 finishes step 3
        scheduler.finish_step(1, 3);
        assert_eq!(scheduler.get_steps_remaining(), 0);
    }
}
