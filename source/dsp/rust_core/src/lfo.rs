//! Multi-Shape Host-Synchronizable Low-Frequency Oscillator (Rules 7, 28, 35, 37, 46, 47)
//! Zero allocations in audio thread, exact host PPQ synchronization, and anti-click smoothing.

use std::f32::consts::PI;

#[repr(u8)]
#[derive(Clone, Copy, PartialEq, Eq, Debug)]
pub enum LFOShape {
    Sine = 0,
    Triangle = 1,
    SawUp = 2,
    SawDown = 3,
    Square = 4,
    Pulse = 5,
    SampleAndHold = 6,
    SmoothRandom = 7,
}

#[repr(u8)]
#[derive(Clone, Copy, PartialEq, Eq, Debug)]
pub enum SyncDivision {
    FreeHz = 0,
    Bar1 = 1,
    Half = 2,
    Quarter = 3,
    Eighth = 4,
    Sixteenth = 5,
    ThirtySecond = 6,
    TripletQuarter = 7,
    TripletEighth = 8,
    DottedEighth = 9,
    DottedQuarter = 10,
}

pub struct RustLFO {
    sample_rate: f64,
    shape: LFOShape,
    sync_div: SyncDivision,
    frequency_hz: f32,
    pulse_width: f32,

    phase: f64,
    last_phase: f32,
    current_output: f32,
    sh_hold_value: f32,
    target_random: f32,
    current_random: f32,
    rng_state: u32,
}

impl RustLFO {
    pub fn new() -> Self {
        Self {
            sample_rate: 44100.0,
            shape: LFOShape::Sine,
            sync_div: SyncDivision::FreeHz,
            frequency_hz: 1.0,
            pulse_width: 0.5,
            phase: 0.0,
            last_phase: 0.0,
            current_output: 0.0,
            sh_hold_value: 0.0,
            target_random: 0.0,
            current_random: 0.0,
            rng_state: 123456789,
        }
    }

    pub fn prepare(&mut self, sample_rate: f64) {
        self.sample_rate = if sample_rate > 0.0 { sample_rate } else { 44100.0 };
        self.phase = 0.0;
        self.current_output = 0.0;
    }

    pub fn reset(&mut self) {
        self.phase = 0.0;
        self.last_phase = 0.0;
        self.current_output = 0.0;
        self.sh_hold_value = 0.0;
    }

    #[inline]
    pub fn set_shape(&mut self, shape: LFOShape) {
        self.shape = shape;
    }

    #[inline]
    pub fn set_sync_division(&mut self, div: SyncDivision) {
        self.sync_div = div;
    }

    #[inline]
    pub fn set_frequency_hz(&mut self, freq: f32) {
        self.frequency_hz = freq.clamp(0.001, 100.0);
    }

    #[inline]
    pub fn set_pulse_width(&mut self, pw: f32) {
        self.pulse_width = pw.clamp(0.05, 0.95);
    }

    #[inline]
    pub fn get_current_value(&self) -> f32 {
        self.current_output
    }

    #[inline]
    pub fn process_sample(&mut self, _bpm: f64, ppq_position: f64, is_host_playing: bool) -> f32 {
        // 1. Determinar el incremento de fase por muestra
        if self.sync_div == SyncDivision::FreeHz || !is_host_playing {
            let phase_inc = (self.frequency_hz as f64) / self.sample_rate;
            self.phase += phase_inc;
            if self.phase >= 1.0 {
                self.phase -= 1.0;
            }
        } else {
            // Sincronización rítmica con el DAW (Regla 37)
            let beats_per_cycle = Self::get_beats_for_division(self.sync_div);
            if beats_per_cycle > 0.0 {
                let cycle_pos = ppq_position / beats_per_cycle;
                self.phase = cycle_pos - cycle_pos.floor();
            }
        }

        // 2. Generar la forma de onda
        let p = self.phase as f32;
        let raw_value = match self.shape {
            LFOShape::Sine => (p * 2.0 * PI).sin(),
            LFOShape::Triangle => {
                if p < 0.5 {
                    4.0 * p - 1.0
                } else {
                    3.0 - 4.0 * p
                }
            }
            LFOShape::SawUp => 2.0 * p - 1.0,
            LFOShape::SawDown => 1.0 - 2.0 * p,
            LFOShape::Square => {
                if p < 0.5 {
                    1.0
                } else {
                    -1.0
                }
            }
            LFOShape::Pulse => {
                if p < self.pulse_width {
                    1.0
                } else {
                    -1.0
                }
            }
            LFOShape::SampleAndHold => {
                if p < self.last_phase {
                    self.rng_state = self.rng_state.wrapping_mul(1664525).wrapping_add(1013904223);
                    self.sh_hold_value = (((self.rng_state & 0xFFFF) as f32) / 32767.5) - 1.0;
                }
                self.sh_hold_value
            }
            LFOShape::SmoothRandom => {
                if p < self.last_phase {
                    self.rng_state = self.rng_state.wrapping_mul(1664525).wrapping_add(1013904223);
                    self.target_random = ((self.rng_state as f32) / 2147483648.0) - 1.0;
                }
                self.current_random += 0.01 * (self.target_random - self.current_random);
                self.current_random
            }
        };

        self.last_phase = p;

        // 3. Suavizado anti-click (Regla 35)
        self.current_output += 0.05 * (raw_value - self.current_output);
        self.current_output
    }

    #[inline]
    fn get_beats_for_division(div: SyncDivision) -> f64 {
        match div {
            SyncDivision::Bar1 => 4.0,
            SyncDivision::Half => 2.0,
            SyncDivision::Quarter => 1.0,
            SyncDivision::Eighth => 0.5,
            SyncDivision::Sixteenth => 0.25,
            SyncDivision::ThirtySecond => 0.125,
            SyncDivision::TripletQuarter => 1.0 * (2.0 / 3.0),
            SyncDivision::TripletEighth => 0.5 * (2.0 / 3.0),
            SyncDivision::DottedQuarter => 1.5,
            SyncDivision::DottedEighth => 0.75,
            _ => 1.0,
        }
    }
}
