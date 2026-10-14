//! Granular Buffer Shuffler & Reverse Memory Looper (Rules 5, 8, 9, 14, 34, 35, 46, 47)
//! Captures audio into a bounded circular history memory, spawning asynchronous grains
//! with probabilistic direction reversal, pitch-jumping (octaves/fifths), stereo scattering,
//! and raised-cosine envelope smoothing. Zero dynamic allocations in audio thread.

use crate::common::{clamp, lerp, denormal_guard};
use std::f32::consts::PI;

const MAX_BUFFER_SAMPLES: usize = 131072; // ~2.73s at 48kHz, strictly bounded
const BUFFER_MASK: usize = MAX_BUFFER_SAMPLES - 1;
const MAX_GRAINS: usize = 8;

#[derive(Clone, Copy)]
struct Grain {
    active: bool,
    pos: f32,          // Position in circular buffer
    speed: f32,        // Playback speed (+1.0, -1.0, +2.0, -0.5, etc.)
    length_samples: f32,
    age_samples: f32,
    pan_l: f32,
    pan_r: f32,
}

impl Grain {
    const fn new() -> Self {
        Self {
            active: false,
            pos: 0.0,
            speed: 1.0,
            length_samples: 4410.0,
            age_samples: 0.0,
            pan_l: 0.707,
            pan_r: 0.707,
        }
    }
}

pub struct RustGranularBufferShuffler {
    sample_rate: f32,

    buf_l: Box<[f32]>,
    buf_r: Box<[f32]>,
    write_idx: usize,

    grains: [Grain; MAX_GRAINS],
    spawn_timer: f32,

    // PRNG state (Xorshift32)
    rng_state: u32,

    // Parameters
    buffer_length_ms: f32,  // 200.0 to 2500.0 ms
    grain_size_ms: f32,     // 20.0 to 400.0 ms
    density: f32,           // 1.0 to 30.0 grains/sec
    reverse_prob: f32,      // 0.0 to 1.0
    pitch_mode: u32,        // 0: Normal, 1: Octave Up (+12), 2: Octave Down (-12), 3: Fifth (+7), 4: Random Octaves
    scatter_pan: f32,       // 0.0 to 1.0
    freeze: f32,            // > 0.5 = frozen buffer
    mix: f32,               // 0.0 to 1.0

    // Smoothed
    smoothed_mix: f32,
}

impl RustGranularBufferShuffler {
    pub fn new(sample_rate: f32) -> Self {
        let sr = if sample_rate > 1000.0 { sample_rate } else { 44100.0 };
        Self {
            sample_rate: sr,
            buf_l: vec![0.0f32; MAX_BUFFER_SAMPLES].into_boxed_slice(),
            buf_r: vec![0.0f32; MAX_BUFFER_SAMPLES].into_boxed_slice(),
            write_idx: 0,
            grains: [Grain::new(); MAX_GRAINS],
            spawn_timer: 0.0,
            rng_state: 0x92418a23,
            buffer_length_ms: 1500.0,
            grain_size_ms: 120.0,
            density: 8.0,
            reverse_prob: 0.4,
            pitch_mode: 0,
            scatter_pan: 0.7,
            freeze: 0.0,
            mix: 0.8,
            smoothed_mix: 0.8,
        }
    }

    pub fn prepare(&mut self, sample_rate: f32) {
        self.sample_rate = if sample_rate > 1000.0 { sample_rate } else { 44100.0 };
        self.reset();
    }

    pub fn reset(&mut self) {
        self.buf_l.fill(0.0);
        self.buf_r.fill(0.0);
        self.write_idx = 0;
        for g in &mut self.grains {
            g.active = false;
        }
        self.spawn_timer = 0.0;
        self.smoothed_mix = self.mix;
    }

    #[inline(always)]
    fn next_random(&mut self) -> f32 {
        let mut x = self.rng_state;
        x ^= x << 13;
        x ^= x >> 17;
        x ^= x << 5;
        self.rng_state = x;
        (x as f32) / (u32::MAX as f32)
    }

    pub fn set_param(&mut self, param_id: u32, value: f32) {
        match param_id {
            1 => self.buffer_length_ms = clamp(value, 200.0, 2500.0),
            2 => self.grain_size_ms = clamp(value, 20.0, 400.0),
            3 => self.density = clamp(value, 1.0, 30.0),
            4 => self.reverse_prob = clamp(value, 0.0, 1.0),
            5 => self.pitch_mode = clamp(value, 0.0, 4.0) as u32,
            6 => self.scatter_pan = clamp(value, 0.0, 1.0),
            7 => self.freeze = clamp(value, 0.0, 1.0),
            8 => self.mix = clamp(value, 0.0, 1.0),
            _ => {}
        }
    }

    #[inline(always)]
    fn read_sample_cubic(buf: &[f32], read_pos: f32) -> f32 {
        let i_pos = read_pos.floor() as isize;
        let frac = read_pos - (i_pos as f32);

        let idx0 = (i_pos.wrapping_sub(1) as usize) & BUFFER_MASK;
        let idx1 = (i_pos as usize) & BUFFER_MASK;
        let idx2 = (i_pos.wrapping_add(1) as usize) & BUFFER_MASK;
        let idx3 = (i_pos.wrapping_add(2) as usize) & BUFFER_MASK;

        let xm1 = buf[idx0];
        let x0 = buf[idx1];
        let x1 = buf[idx2];
        let x2 = buf[idx3];

        let c0 = x0;
        let c1 = 0.5 * (x1 - xm1);
        let c2 = xm1 - 2.5 * x0 + 2.0 * x1 - 0.5 * x2;
        let c3 = 0.5 * (x2 - xm1) + 1.5 * (x0 - x1);

        ((c3 * frac + c2) * frac + c1) * frac + c0
    }

    fn try_spawn_grain(&mut self) {
        let slot_idx = match self.grains.iter().position(|g| !g.active) {
            Some(i) => i,
            None => return,
        };

        let r1 = self.next_random();
        let r2 = self.next_random();
        let r3 = self.next_random();
        let r4 = self.next_random();

        let max_history_samples = (self.buffer_length_ms * 0.001 * self.sample_rate).clamp(100.0, (MAX_BUFFER_SAMPLES - 16) as f32);
        let history_offset = r1 * max_history_samples;
        let start_pos = ((self.write_idx as f32) - history_offset + (MAX_BUFFER_SAMPLES as f32)) % (MAX_BUFFER_SAMPLES as f32);

        let is_reversed = r2 < self.reverse_prob;
        let speed_mag = match self.pitch_mode {
            0 => 1.0,
            1 => 2.0,                      // +1 Octave
            2 => 0.5,                      // -1 Octave
            3 => 1.4983,                   // +1 Fifth (2^(7/12))
            4 => {
                if r3 < 0.33 { 0.5 } else if r3 < 0.66 { 1.0 } else { 2.0 }
            }
            _ => 1.0,
        };
        let speed = if is_reversed { -speed_mag } else { speed_mag };

        let len_samples = (self.grain_size_ms * 0.001 * self.sample_rate).clamp(100.0, 44100.0);

        let pan = lerp(0.5, r4, self.scatter_pan);
        let pan_angle = pan * (PI * 0.5);

        let g = &mut self.grains[slot_idx];
        g.active = true;
        g.pos = start_pos;
        g.speed = speed;
        g.length_samples = len_samples;
        g.age_samples = 0.0;
        g.pan_l = pan_angle.cos();
        g.pan_r = pan_angle.sin();
    }

    pub fn process(
        &mut self,
        in_l: *const f32,
        in_r: *const f32,
        out_l: *mut f32,
        out_r: *mut f32,
        num_samples: u32,
    ) {
        if in_l.is_null() || in_r.is_null() || out_l.is_null() || out_r.is_null() || num_samples == 0 {
            return;
        }

        let is_frozen = self.freeze > 0.5;
        let spawn_interval = (self.sample_rate / self.density.max(0.5)).max(16.0);
        let smooth = (-1.0 / (0.005 * self.sample_rate)).exp();

        for i in 0..num_samples as usize {
            let xl = unsafe { *in_l.add(i) };
            let xr = unsafe { *in_r.add(i) };

            self.smoothed_mix = lerp(self.mix, self.smoothed_mix, smooth);

            // Record to circular buffer if not frozen
            if !is_frozen {
                self.buf_l[self.write_idx] = xl;
                self.buf_r[self.write_idx] = xr;
                self.write_idx = (self.write_idx + 1) & BUFFER_MASK;
            }

            // Spawn timer
            self.spawn_timer += 1.0;
            if self.spawn_timer >= spawn_interval {
                self.spawn_timer = 0.0;
                self.try_spawn_grain();
            }

            // Accumulate active grains
            let mut wet_l = 0.0f32;
            let mut wet_r = 0.0f32;

            for g in &mut self.grains {
                if g.active {
                    let env_phase = g.age_samples / g.length_samples;
                    if env_phase >= 1.0 {
                        g.active = false;
                        continue;
                    }

                    // Hann raised-cosine window: 0.5 * (1 - cos(2*pi*t))
                    let win = 0.5 * (1.0 - (2.0 * PI * env_phase).cos());

                    let gl = Self::read_sample_cubic(&self.buf_l, g.pos) * win;
                    let gr = Self::read_sample_cubic(&self.buf_r, g.pos) * win;

                    wet_l += gl * g.pan_l;
                    wet_r += gr * g.pan_r;

                    g.pos = (g.pos + g.speed + (MAX_BUFFER_SAMPLES as f32)) % (MAX_BUFFER_SAMPLES as f32);
                    g.age_samples += 1.0;
                }
            }

            let out_sample_l = denormal_guard(lerp(xl, wet_l, self.smoothed_mix));
            let out_sample_r = denormal_guard(lerp(xr, wet_r, self.smoothed_mix));

            unsafe {
                *out_l.add(i) = out_sample_l;
                *out_r.add(i) = out_sample_r;
            }
        }
    }
}
