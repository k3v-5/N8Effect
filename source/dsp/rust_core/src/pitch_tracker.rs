//! PitchTrackerCore - Real-Time Fundamental Frequency & Clarity Tracker (YIN Algorithm)
//! Implemented for the N8Effect Hybrid C++ / Rust Audio Engine Architecture (Rules 1, 7, 9, 34, 46, 47).
//! Uses Cumulative Mean Normalized Difference Function (CMNDF), early threshold exit,
//! valley descent tracking, and sub-sample parabolic interpolation.
//! Zero heap allocations in audio thread, AVX2 SIMD friendly, denormal safe.

pub const WINDOW_SIZE: usize = 2048;

pub struct PitchTrackerCore {
    sample_rate: f32,
    buffer: Box<[f32]>,
    yin_buffer: Box<[f32]>,
    write_index: usize,
    buffered_samples: usize,
    min_lag: usize,
    max_lag: usize,
    current_pitch_hz: f32,
    target_pitch_hz: f32,
    clarity: f32,
}

impl Default for PitchTrackerCore {
    fn default() -> Self {
        Self::new(44100.0)
    }
}

impl PitchTrackerCore {
    pub fn new(sample_rate: f32) -> Self {
        let sr = if sample_rate > 0.0 { sample_rate } else { 44100.0 };
        let mut tracker = Self {
            sample_rate: sr,
            buffer: vec![0.0f32; WINDOW_SIZE * 3].into_boxed_slice(),
            yin_buffer: vec![0.0f32; WINDOW_SIZE].into_boxed_slice(),
            write_index: 0,
            buffered_samples: 0,
            min_lag: 22,
            max_lag: 1102,
            current_pitch_hz: 440.0,
            target_pitch_hz: 440.0,
            clarity: 0.0,
        };
        tracker.prepare(sr);
        tracker
    }

    pub fn prepare(&mut self, sample_rate: f32) {
        self.sample_rate = if sample_rate > 0.0 { sample_rate } else { 44100.0 };
        self.min_lag = ((self.sample_rate / 2000.0) as usize).max(2); // ~2000 Hz
        self.max_lag = (WINDOW_SIZE - 1).min((self.sample_rate / 40.0) as usize); // ~40 Hz
        self.reset();
    }

    pub fn reset(&mut self) {
        self.buffer.fill(0.0);
        self.yin_buffer.fill(0.0);
        self.write_index = 0;
        self.buffered_samples = 0;
        self.current_pitch_hz = 440.0;
        self.target_pitch_hz = 440.0;
        self.clarity = 0.0;
    }

    #[inline(always)]
    pub fn get_fundamental_hz(&self) -> f32 {
        self.current_pitch_hz
    }

    #[inline(always)]
    pub fn get_clarity(&self) -> f32 {
        self.clarity
    }

    #[inline(always)]
    pub fn get_midi_note(&self) -> f32 {
        if self.current_pitch_hz <= 10.0 {
            0.0
        } else {
            69.0 + 12.0 * (self.current_pitch_hz / 440.0).log2()
        }
    }

    #[inline(always)]
    pub fn get_pitch_normalized(&self) -> f32 {
        let midi = self.get_midi_note();
        ((midi - 21.0) / (108.0 - 21.0)).clamp(0.0, 1.0)
    }

    pub fn process(&mut self, input: Option<&[f32]>, num_samples: usize) {
        if input.is_none() || num_samples == 0 {
            return;
        }

        let slice = input.unwrap();
        // Triple mirror buffer write for contiguous lookahead reads
        for s in 0..num_samples {
            let val = slice[s];
            self.buffer[self.write_index] = val;
            self.buffer[self.write_index + WINDOW_SIZE] = val;
            self.buffer[self.write_index + WINDOW_SIZE * 2] = val;
            self.write_index = (self.write_index + 1) % WINDOW_SIZE;
        }

        self.buffered_samples += num_samples;
        // Analyze periodically every ~256 samples for CPU efficiency (Rule 47)
        if self.buffered_samples >= 256 {
            self.buffered_samples = 0;
            self.estimate_pitch();
        }

        // Anti-click smoothing of estimated fundamental frequency (Rule 35)
        let alpha = 0.1f32;
        self.current_pitch_hz += alpha * (self.target_pitch_hz - self.current_pitch_hz);
    }

    #[inline(always)]
    fn compute_difference_for_lag(window: &[f32], tau: usize, w: usize) -> f32 {
        let p0 = &window[..w];
        let p_tau = &window[tau..tau + w];
        let mut sum = 0.0f32;
        for j in 0..w {
            let d = p0[j] - p_tau[j];
            sum += d * d;
        }
        sum
    }

    fn estimate_pitch(&mut self) {
        let w = WINDOW_SIZE / 2; // 1024
        let window = &self.buffer[self.write_index..self.write_index + WINDOW_SIZE * 2];

        // Silence check on analysis window (< -70 dBFS / 0.0003 RMS)
        let mut window_energy = 0.0f32;
        for j in 0..w {
            let s = window[j];
            window_energy += s * s;
        }
        if window_energy < (w as f32) * (0.0003 * 0.0003) {
            self.clarity = 0.0;
            return;
        }

        let threshold = 0.15f32;
        let mut best_tau = 0usize;
        let mut min_val = 100.0f32;
        let mut min_val_tau = 0usize;
        let mut running_sum = 0.0f32;

        self.yin_buffer[0] = 1.0;

        // Progressive YIN search with CMNDF and early exit
        let mut tau = self.min_lag;
        while tau <= self.max_lag {
            let diff_sum = Self::compute_difference_for_lag(window, tau, w);
            running_sum += diff_sum;
            let cmndf = if running_sum > 1e-6 {
                diff_sum * (tau as f32) / running_sum
            } else {
                1.0
            };
            self.yin_buffer[tau] = cmndf;

            if cmndf < min_val {
                min_val = cmndf;
                min_val_tau = tau;
            }

            // Early exit if CMNDF falls below threshold (0.15)
            if cmndf < threshold {
                while tau + 1 <= self.max_lag {
                    let next_tau = tau + 1;
                    let next_diff = Self::compute_difference_for_lag(window, next_tau, w);
                    running_sum += next_diff;
                    let next_cmndf = if running_sum > 1e-6 {
                        next_diff * (next_tau as f32) / running_sum
                    } else {
                        1.0
                    };
                    self.yin_buffer[next_tau] = next_cmndf;

                    if next_cmndf < min_val {
                        min_val = next_cmndf;
                        min_val_tau = next_tau;
                    }

                    if next_cmndf < self.yin_buffer[tau] {
                        tau = next_tau;
                    } else {
                        // Valley began ascending: local minimum is at tau
                        break;
                    }
                }
                best_tau = tau;
                break;
            }

            tau += 1;
        }

        if best_tau == 0 && min_val_tau >= self.min_lag {
            best_tau = min_val_tau;
        }

        if best_tau >= self.min_lag && best_tau <= self.max_lag {
            // Sub-sample parabolic interpolation for maximum precision (Rule 34)
            let mut refined_tau = best_tau as f32;
            if best_tau > self.min_lag && best_tau < self.max_lag {
                let y0 = self.yin_buffer[best_tau - 1];
                let y1 = self.yin_buffer[best_tau];
                let y2 = self.yin_buffer[best_tau + 1];
                let denom = 2.0 * (y0 - 2.0 * y1 + y2);
                if denom.abs() > 1e-6 {
                    refined_tau += (y0 - y2) / denom;
                }
            }

            if refined_tau > 0.0 {
                let estimated_hz = self.sample_rate / refined_tau;
                let current_val = self.yin_buffer[best_tau];
                self.clarity = (1.0 - current_val).clamp(0.0, 1.0);

                if self.clarity > 0.25 && estimated_hz >= 40.0 && estimated_hz <= 2200.0 {
                    self.target_pitch_hz = estimated_hz;
                }
            }
        }
    }
}
