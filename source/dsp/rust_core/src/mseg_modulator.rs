//! Multi-Segment Envelope Generator (MSEG) with Configurable Curve Tension (Rules 7, 8, 25, 37, 46, 47)
//! 16-point bounded capacity, non-linear Bézier/tension curvature, host sync & free frequency.

use crate::lfo::SyncDivision;

pub const MAX_MSEG_POINTS: usize = 16;

#[repr(u8)]
#[derive(Clone, Copy, PartialEq, Eq, Debug)]
pub enum MSEGPlayMode {
    Loop = 0,
    OneShot = 1,
    PingPong = 2,
}

#[repr(C)]
#[derive(Clone, Copy, Debug)]
pub struct MSEGPoint {
    pub time: f32,
    pub level: f32,
    pub curve: f32,
}

impl Default for MSEGPoint {
    fn default() -> Self {
        Self {
            time: 0.0,
            level: 0.0,
            curve: 0.0,
        }
    }
}

pub struct RustMSEGModulator {
    sample_rate: f64,
    points: [MSEGPoint; MAX_MSEG_POINTS],
    num_points: usize,

    mode: MSEGPlayMode,
    sync_div: SyncDivision,
    free_hz: f32,

    phase: f64,
    current_value: f32,
    ping_pong_forward: bool,
    finished: bool,
}

impl RustMSEGModulator {
    pub fn new() -> Self {
        let mut mseg = Self {
            sample_rate: 44100.0,
            points: [MSEGPoint::default(); MAX_MSEG_POINTS],
            num_points: 4,
            mode: MSEGPlayMode::Loop,
            sync_div: SyncDivision::Bar1,
            free_hz: 1.0,
            phase: 0.0,
            current_value: 0.0,
            ping_pong_forward: true,
            finished: false,
        };
        mseg.set_default_envelope();
        mseg
    }

    pub fn prepare(&mut self, sample_rate: f64) {
        self.sample_rate = if sample_rate > 0.0 { sample_rate } else { 44100.0 };
        self.reset();
    }

    pub fn reset(&mut self) {
        self.phase = 0.0;
        self.current_value = self.points[0].level;
        self.ping_pong_forward = true;
        self.finished = false;
    }

    pub fn set_default_envelope(&mut self) {
        self.num_points = 4;
        self.points[0] = MSEGPoint { time: 0.0,  level: 0.0,  curve: 0.0 };
        self.points[1] = MSEGPoint { time: 0.25, level: 1.0,  curve: 0.25 };
        self.points[2] = MSEGPoint { time: 0.75, level: 0.4,  curve: -0.2 };
        self.points[3] = MSEGPoint { time: 1.0,  level: 0.0,  curve: -0.4 };
    }

    #[inline]
    pub fn set_point(&mut self, index: usize, time: f32, level: f32, curve: f32) {
        if index < MAX_MSEG_POINTS {
            self.points[index] = MSEGPoint {
                time: time.clamp(0.0, 1.0),
                level: level.clamp(-1.0, 1.0),
                curve: curve.clamp(-1.0, 1.0),
            };
        }
    }

    #[inline]
    pub fn set_num_points(&mut self, count: usize) {
        self.num_points = count.clamp(2, MAX_MSEG_POINTS);
    }

    #[inline]
    pub fn set_mode(&mut self, mode: MSEGPlayMode) {
        self.mode = mode;
    }

    #[inline]
    pub fn set_sync(&mut self, div: SyncDivision, free_hz: f32) {
        self.sync_div = div;
        self.free_hz = free_hz.clamp(0.01, 40.0);
    }

    #[inline]
    pub fn trigger(&mut self) {
        self.phase = 0.0;
        self.ping_pong_forward = true;
        self.finished = false;
    }

    #[inline]
    pub fn process_sample(&mut self, bpm: f64, ppq: f64, is_playing: bool) {
        if self.finished {
            return;
        }

        // 1. Cálculo de incremento de fase
        if self.sync_div != SyncDivision::FreeHz && is_playing && bpm > 1.0 {
            let beats_per_cycle = match self.sync_div {
                SyncDivision::Bar1 => 4.0,
                SyncDivision::Half => 2.0,
                SyncDivision::Quarter => 1.0,
                SyncDivision::Eighth => 0.5,
                SyncDivision::Sixteenth => 0.25,
                SyncDivision::ThirtySecond => 0.125,
                SyncDivision::TripletQuarter => 4.0 / 3.0,
                SyncDivision::TripletEighth => 2.0 / 3.0,
                _ => 1.0,
            };

            let mut normalized_pos = (ppq % beats_per_cycle) / beats_per_cycle;
            if normalized_pos < 0.0 {
                normalized_pos += 1.0;
            }
            self.phase = normalized_pos;
        } else {
            let phase_increment = (self.free_hz as f64) / self.sample_rate;
            if self.mode == MSEGPlayMode::PingPong {
                if self.ping_pong_forward {
                    self.phase += phase_increment;
                    if self.phase >= 1.0 {
                        self.phase = 1.0;
                        self.ping_pong_forward = false;
                    }
                } else {
                    self.phase -= phase_increment;
                    if self.phase <= 0.0 {
                        self.phase = 0.0;
                        self.ping_pong_forward = true;
                    }
                }
            } else {
                self.phase += phase_increment;
                if self.phase >= 1.0 {
                    if self.mode == MSEGPlayMode::Loop {
                        self.phase %= 1.0;
                    } else {
                        self.phase = 1.0;
                        self.finished = true;
                    }
                }
            }
        }

        // 2. Evaluar valor del MSEG en la fase actual
        self.current_value = self.evaluate_at(self.phase as f32);
    }

    #[inline]
    pub fn get_current_value(&self) -> f32 {
        self.current_value
    }

    fn evaluate_at(&self, t: f32) -> f32 {
        if self.num_points < 2 {
            return 0.0;
        }

        if t <= self.points[0].time {
            return self.points[0].level;
        }
        if t >= self.points[self.num_points - 1].time {
            return self.points[self.num_points - 1].level;
        }

        let mut seg_idx = 0;
        for i in 0..self.num_points - 1 {
            if t >= self.points[i].time && t <= self.points[i + 1].time {
                seg_idx = i;
                break;
            }
        }

        let p0 = self.points[seg_idx];
        let p1 = self.points[seg_idx + 1];

        let seg_duration = p1.time - p0.time;
        if seg_duration <= 1e-6 {
            return p0.level;
        }

        let local_t = ((t - p0.time) / seg_duration).clamp(0.0, 1.0);

        let mut curved_t = local_t;
        let c = p0.curve;
        if c.abs() > 0.001 {
            if c > 0.0 {
                let exponent = 1.0 + 3.0 * c;
                curved_t = 1.0 - (1.0 - local_t).powf(exponent);
            } else {
                let exponent = 1.0 - 3.0 * c;
                curved_t = local_t.powf(exponent);
            }
        }

        (1.0 - curved_t) * p0.level + curved_t * p1.level
    }
}
