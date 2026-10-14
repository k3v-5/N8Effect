//! High-Precision ADSR Envelope Generator for Universal Modulation (Rules 7, 46, 47)
//! Zero allocations in audio thread, deterministic state machine, sample rate aware.

#[repr(u8)]
#[derive(Clone, Copy, PartialEq, Eq, Debug)]
pub enum EnvelopeState {
    Idle = 0,
    Attack = 1,
    Decay = 2,
    Sustain = 3,
    Release = 4,
}

pub struct RustEnvelopeGenerator {
    sample_rate: f64,
    attack_ms: f32,
    decay_ms: f32,
    sustain_level: f32,
    release_ms: f32,

    attack_coeff: f32,
    decay_coeff: f32,
    release_coeff: f32,

    state: EnvelopeState,
    current_level: f32,
}

impl RustEnvelopeGenerator {
    pub fn new() -> Self {
        let mut env = Self {
            sample_rate: 44100.0,
            attack_ms: 10.0,
            decay_ms: 100.0,
            sustain_level: 0.7,
            release_ms: 200.0,
            attack_coeff: 0.001,
            decay_coeff: 0.001,
            release_coeff: 0.001,
            state: EnvelopeState::Idle,
            current_level: 0.0,
        };
        env.update_coefficients();
        env
    }

    pub fn prepare(&mut self, sample_rate: f64) {
        self.sample_rate = if sample_rate > 0.0 { sample_rate } else { 44100.0 };
        self.update_coefficients();
        self.reset();
    }

    pub fn reset(&mut self) {
        self.state = EnvelopeState::Idle;
        self.current_level = 0.0;
    }

    #[inline]
    pub fn set_attack_ms(&mut self, ms: f32) {
        self.attack_ms = ms.max(0.5);
        self.update_coefficients();
    }

    #[inline]
    pub fn set_decay_ms(&mut self, ms: f32) {
        self.decay_ms = ms.max(0.5);
        self.update_coefficients();
    }

    #[inline]
    pub fn set_sustain_level(&mut self, lvl: f32) {
        self.sustain_level = lvl.clamp(0.0, 1.0);
    }

    #[inline]
    pub fn set_release_ms(&mut self, ms: f32) {
        self.release_ms = ms.max(0.5);
        self.update_coefficients();
    }

    #[inline]
    pub fn trigger_gate(&mut self, gate_on: bool) {
        if gate_on {
            self.state = EnvelopeState::Attack;
        } else if self.state != EnvelopeState::Idle {
            self.state = EnvelopeState::Release;
        }
    }

    #[inline]
    pub fn process_sample(&mut self) -> f32 {
        match self.state {
            EnvelopeState::Idle => {
                self.current_level = 0.0;
            }
            EnvelopeState::Attack => {
                self.current_level += self.attack_coeff;
                if self.current_level >= 1.0 {
                    self.current_level = 1.0;
                    self.state = EnvelopeState::Decay;
                }
            }
            EnvelopeState::Decay => {
                self.current_level -= self.decay_coeff * (self.current_level - self.sustain_level);
                if self.current_level <= self.sustain_level + 0.001 {
                    self.current_level = self.sustain_level;
                    self.state = EnvelopeState::Sustain;
                }
            }
            EnvelopeState::Sustain => {
                self.current_level = self.sustain_level;
            }
            EnvelopeState::Release => {
                self.current_level -= self.release_coeff * self.current_level;
                if self.current_level <= 0.0005 {
                    self.current_level = 0.0;
                    self.state = EnvelopeState::Idle;
                }
            }
        }
        self.current_level
    }

    #[inline]
    pub fn get_current_value(&self) -> f32 {
        self.current_level
    }

    #[inline]
    pub fn get_state(&self) -> EnvelopeState {
        self.state
    }

    #[inline]
    pub fn is_active(&self) -> bool {
        self.state != EnvelopeState::Idle
    }

    fn update_coefficients(&mut self) {
        let sr = self.sample_rate as f32;
        let attack_samples = (self.attack_ms * 0.001) * sr;
        let decay_samples = (self.decay_ms * 0.001) * sr;
        let release_samples = (self.release_ms * 0.001) * sr;

        self.attack_coeff = 1.0 / attack_samples.max(1.0);
        self.decay_coeff = 1.0 / decay_samples.max(1.0);
        self.release_coeff = 1.0 / release_samples.max(1.0);
    }
}
