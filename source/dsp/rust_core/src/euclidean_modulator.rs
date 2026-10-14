//! Bjorklund Euclidean Polyrhythmic Modulator (Rules 7, 8, 37, 46, 47)
//! Direct O(1) Bresenham pulse distribution, analog exponential decay envelope.

use crate::lfo::SyncDivision;

pub struct RustEuclideanModulator {
    sample_rate: f64,
    steps: i32,
    pulses: i32,
    rotation: i32,
    decay_seconds: f32,
    decay_coeff: f32,
    sync_div: SyncDivision,
    free_bpm: f32,

    current_env: f32,
    last_step: i32,
    phase: f64,
    internal_step: i32,
}

impl RustEuclideanModulator {
    pub fn new() -> Self {
        let mut euc = Self {
            sample_rate: 44100.0,
            steps: 16,
            pulses: 4,
            rotation: 0,
            decay_seconds: 0.1,
            decay_coeff: 0.999,
            sync_div: SyncDivision::Sixteenth,
            free_bpm: 120.0,
            current_env: 0.0,
            last_step: -1,
            phase: 0.0,
            internal_step: 0,
        };
        euc.update_decay_coeff();
        euc
    }

    pub fn prepare(&mut self, sample_rate: f64) {
        self.sample_rate = if sample_rate > 0.0 { sample_rate } else { 44100.0 };
        self.update_decay_coeff();
        self.reset();
    }

    pub fn reset(&mut self) {
        self.current_env = 0.0;
        self.last_step = -1;
        self.phase = 0.0;
        self.internal_step = 0;
    }

    #[inline]
    pub fn set_steps(&mut self, steps: i32) {
        self.steps = steps.clamp(1, 32);
    }

    #[inline]
    pub fn set_pulses(&mut self, pulses: i32) {
        self.pulses = pulses.clamp(0, self.steps);
    }

    #[inline]
    pub fn set_rotation(&mut self, rotation: i32) {
        if self.steps > 0 {
            self.rotation = (rotation % self.steps + self.steps) % self.steps;
        }
    }

    #[inline]
    pub fn set_decay(&mut self, seconds: f32) {
        self.decay_seconds = seconds.clamp(0.01, 2.0);
        self.update_decay_coeff();
    }

    #[inline]
    pub fn set_sync(&mut self, div: SyncDivision, free_bpm: f32) {
        self.sync_div = div;
        self.free_bpm = free_bpm.clamp(20.0, 300.0);
    }

    #[inline]
    pub fn has_pulse_at_step(&self, step: i32) -> bool {
        if self.pulses <= 0 {
            return false;
        }
        if self.pulses >= self.steps {
            return true;
        }

        let rotated = (step + self.rotation) % self.steps;
        // Algoritmo matemático directo O(1) de Bjorklund / Bresenham
        ((rotated * self.pulses) % self.steps) < self.pulses
    }

    #[inline]
    pub fn process_sample(&mut self, bpm: f64, ppq: f64, is_playing: bool) {
        let current_step = if self.sync_div != SyncDivision::FreeHz && is_playing && bpm > 1.0 {
            let steps_per_beat = match self.sync_div {
                SyncDivision::Quarter => 1.0,
                SyncDivision::Eighth => 2.0,
                SyncDivision::Sixteenth => 4.0,
                SyncDivision::ThirtySecond => 8.0,
                SyncDivision::TripletEighth => 3.0,
                _ => 4.0,
            };

            let total_steps = ppq * steps_per_beat;
            let mut cs = (total_steps.floor() as i32) % self.steps;
            if cs < 0 {
                cs += self.steps;
            }
            cs
        } else {
            let beat_rate = ((self.free_bpm as f64) / 60.0) * 4.0;
            self.phase += beat_rate / self.sample_rate;
            if self.phase >= 1.0 {
                self.phase = self.phase % 1.0;
                self.internal_step = (self.internal_step + 1) % self.steps;
            }
            self.internal_step
        };

        if current_step != self.last_step {
            self.last_step = current_step;
            if self.has_pulse_at_step(current_step) {
                self.current_env = 1.0;
            }
        }

        self.current_env *= self.decay_coeff;
        if self.current_env < 1e-5 {
            self.current_env = 0.0;
        }
    }

    #[inline]
    pub fn get_current_value(&self) -> f32 {
        self.current_env
    }

    fn update_decay_coeff(&mut self) {
        let sr = self.sample_rate as f32;
        self.decay_coeff = (-1.0 / (self.decay_seconds * sr)).exp();
    }
}
