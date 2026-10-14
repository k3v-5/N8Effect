//! Step Sequencer with Glide and Probability Modulation (Rules 7, 29, 37, 46, 47)
//! 32-step preallocated array, deterministic PRNG, sample-accurate PPQ synchronization.

pub const MAX_STEPS: usize = 32;

#[repr(C)]
#[derive(Clone, Copy, Debug)]
pub struct SequencerStep {
    pub value: f32,
    pub probability: f32,
    pub glide: f32,
    pub active: bool,
}

impl Default for SequencerStep {
    fn default() -> Self {
        Self {
            value: 0.0,
            probability: 1.0,
            glide: 0.0,
            active: true,
        }
    }
}

pub struct RustStepSequencer {
    sample_rate: f64,
    num_steps: u32,
    current_step: u32,
    last_ppq_step: i64,
    steps: [SequencerStep; MAX_STEPS],
    current_output: f32,
    target_output: f32,
    rng_state: u32,
}

impl RustStepSequencer {
    pub fn new() -> Self {
        let mut steps = [SequencerStep::default(); MAX_STEPS];
        for i in 0..MAX_STEPS {
            steps[i] = SequencerStep {
                value: ((i % 8) as f32 / 7.0) * 2.0 - 1.0,
                probability: 1.0,
                glide: 0.0,
                active: true,
            };
        }

        Self {
            sample_rate: 44100.0,
            num_steps: 16,
            current_step: 0,
            last_ppq_step: -1,
            steps,
            current_output: 0.0,
            target_output: 0.0,
            rng_state: 987654321,
        }
    }

    pub fn prepare(&mut self, sample_rate: f64) {
        self.sample_rate = if sample_rate > 0.0 { sample_rate } else { 44100.0 };
        self.current_step = 0;
        self.current_output = 0.0;
        self.target_output = 0.0;
    }

    pub fn reset(&mut self) {
        self.current_step = 0;
        self.current_output = 0.0;
        self.target_output = 0.0;
        self.last_ppq_step = -1;
    }

    #[inline]
    pub fn set_num_steps(&mut self, count: u32) {
        self.num_steps = count.clamp(1, MAX_STEPS as u32);
    }

    #[inline]
    pub fn set_step(&mut self, index: u32, value: f32, probability: f32, glide: f32, active: bool) {
        let idx = index as usize;
        if idx < MAX_STEPS {
            self.steps[idx] = SequencerStep {
                value: value.clamp(-1.0, 1.0),
                probability: probability.clamp(0.0, 1.0),
                glide: glide.clamp(0.0, 1.0),
                active,
            };
        }
    }

    #[inline]
    pub fn get_step(&self, index: u32) -> SequencerStep {
        let idx = (index as usize).min(MAX_STEPS - 1);
        self.steps[idx]
    }

    #[inline]
    pub fn process_sample(&mut self, ppq_position: f64, is_host_playing: bool) -> f32 {
        if is_host_playing {
            // Calcular paso correspondiente según el PPQ actual (1/16th notes = 0.25 beats)
            let beats_per_step = 0.25;
            let step_idx = (ppq_position / beats_per_step).floor() as i64;

            if step_idx != self.last_ppq_step {
                self.last_ppq_step = step_idx;
                self.advance_step();
            }
        }

        // Aplicar interpolación Glide (Regla 29)
        let cur_idx = (self.current_step as usize).min(MAX_STEPS - 1);
        let glide_coeff = (1.0 - self.steps[cur_idx].glide * 0.98).clamp(0.001, 1.0);
        self.current_output += glide_coeff * (self.target_output - self.current_output);

        self.current_output
    }

    #[inline]
    pub fn get_current_value(&self) -> f32 {
        self.current_output
    }

    #[inline]
    pub fn get_current_step(&self) -> u32 {
        self.current_step
    }

    fn advance_step(&mut self) {
        if self.num_steps == 0 {
            return;
        }
        self.current_step = (self.current_step + 1) % self.num_steps;
        let cur_idx = (self.current_step as usize).min(MAX_STEPS - 1);
        let step = self.steps[cur_idx];

        if step.active {
            self.rng_state = self.rng_state.wrapping_mul(1664525).wrapping_add(1013904223);
            let roll = ((self.rng_state & 0xFFFF) as f32) / 65535.0;

            if roll <= step.probability {
                self.target_output = step.value;
            }
        }
    }
}
