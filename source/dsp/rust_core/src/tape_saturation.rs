//! TapeSaturationNode with Jiles-Atherton Magnetic Hysteresis, Head Bump & Wow/Flutter in Rust.
//! High-performance, zero allocations, SIMD-friendly implementation.

use crate::common::DENORMAL_GUARD;
use std::f32::consts::PI;

const FLUTTER_BUFFER_SIZE: usize = 4096;
const FLUTTER_BUFFER_MASK: usize = FLUTTER_BUFFER_SIZE - 1;

#[repr(C)]
#[derive(Clone, Copy, Default)]
struct BiquadState {
    x1: f32,
    x2: f32,
    y1: f32,
    y2: f32,
}

impl BiquadState {
    #[inline(always)]
    fn process(&mut self, input: f32, b0: f32, b1: f32, b2: f32, a1: f32, a2: f32) -> f32 {
        let out = b0 * input + b1 * self.x1 + b2 * self.x2 - a1 * self.y1 - a2 * self.y2 + DENORMAL_GUARD;
        self.x2 = self.x1;
        self.x1 = input;
        self.y2 = self.y1;
        self.y1 = out;
        out
    }

    fn reset(&mut self) {
        self.x1 = 0.0;
        self.x2 = 0.0;
        self.y1 = 0.0;
        self.y2 = 0.0;
    }
}

#[repr(C)]
#[derive(Clone, Copy, Default)]
struct JilesAthertonState {
    m: f32,
    h_prev: f32,
    man_prev: f32,
}

impl JilesAthertonState {
    fn reset(&mut self) {
        self.m = 0.0;
        self.h_prev = 0.0;
        self.man_prev = 0.0;
    }

    #[inline(always)]
    fn process(
        &mut self,
        input: f32,
        drive: f32,
        k: f32,
        alpha: f32,
        c: f32,
        a: f32,
        ms: f32,
    ) -> f32 {
        let h = input * drive;
        let dh = h - self.h_prev;

        if dh.abs() < 1.0e-7 {
            return self.m;
        }

        let delta = if dh > 0.0 { 1.0 } else { -1.0 };
        let he = h + alpha * self.m;

        let x = he / a;
        let man = ms * (x / (9.0 + x * x).sqrt());

        let dman = man - self.man_prev;
        let dman_dh = dman / (dh + if dh >= 0.0 { 1.0e-9 } else { -1.0e-9 });

        let diff = man - self.m;
        let dm;

        if delta * diff > 0.0 {
            let mut denom = delta * k - alpha * diff;
            let min_denom = 0.15 * k;
            if denom.abs() < min_denom {
                denom = if denom >= 0.0 { min_denom } else { -min_denom };
            }
            let dm_irr = (diff / denom) * (1.0 - c);
            dm = (dm_irr + c * dman_dh) * dh;
        } else {
            dm = c * dman_dh * dh;
        }

        let max_step = 0.5f32;
        let clamped_dm = dm.clamp(-max_step, max_step);
        self.m = (self.m + clamped_dm).clamp(-ms, ms);

        self.h_prev = h;
        self.man_prev = man;

        self.m
    }
}

#[repr(C)]
pub struct RustTapeSaturation {
    sample_rate: f32,

    // Wow / Flutter circular buffers
    flutter_buffer_l: [f32; FLUTTER_BUFFER_SIZE],
    flutter_buffer_r: [f32; FLUTTER_BUFFER_SIZE],
    write_idx: usize,
    flutter_phase1: f32,
    flutter_phase2: f32,

    // Jiles-Atherton hysteresis states
    hysteresis_l: JilesAthertonState,
    hysteresis_r: JilesAthertonState,

    // Filters states (Head bump & Rolloff per channel)
    head_bump_l: BiquadState,
    head_bump_r: BiquadState,
    rolloff_l: BiquadState,
    rolloff_r: BiquadState,

    // Parameters
    param_drive: f32,       // 1: 1.0 .. 10.0
    param_speed: f32,       // 2: 0 (7.5 ips), 1 (15 ips), 2 (30 ips)
    param_wow_flutter: f32, // 3: 0.0 .. 1.0
    param_warmth: f32,      // 4: 0.0 .. 1.0
    param_mix: f32,         // 5: 0.0 .. 1.0
    param_hysteresis: f32,  // 6: 0.0 .. 1.0
}

impl RustTapeSaturation {
    pub fn new(sample_rate: f32) -> Self {
        Self {
            sample_rate: if sample_rate > 0.0 { sample_rate } else { 44100.0 },
            flutter_buffer_l: [0.0; FLUTTER_BUFFER_SIZE],
            flutter_buffer_r: [0.0; FLUTTER_BUFFER_SIZE],
            write_idx: 0,
            flutter_phase1: 0.0,
            flutter_phase2: 0.0,
            hysteresis_l: JilesAthertonState::default(),
            hysteresis_r: JilesAthertonState::default(),
            head_bump_l: BiquadState::default(),
            head_bump_r: BiquadState::default(),
            rolloff_l: BiquadState::default(),
            rolloff_r: BiquadState::default(),
            param_drive: 2.0,
            param_speed: 1.0,
            param_wow_flutter: 0.25,
            param_warmth: 0.5,
            param_mix: 1.0,
            param_hysteresis: 0.5,
        }
    }

    pub fn reset(&mut self) {
        self.flutter_buffer_l.fill(0.0);
        self.flutter_buffer_r.fill(0.0);
        self.write_idx = 0;
        self.flutter_phase1 = 0.0;
        self.flutter_phase2 = 0.0;
        self.hysteresis_l.reset();
        self.hysteresis_r.reset();
        self.head_bump_l.reset();
        self.head_bump_r.reset();
        self.rolloff_l.reset();
        self.rolloff_r.reset();
    }

    pub fn set_sample_rate(&mut self, sample_rate: f32) {
        if sample_rate > 0.0 {
            self.sample_rate = sample_rate;
        }
    }

    pub fn set_parameter(&mut self, param_id: u32, value: f32) {
        match param_id {
            1 => self.param_drive = value.clamp(1.0, 10.0),
            2 => self.param_speed = value.clamp(0.0, 2.0),
            3 => self.param_wow_flutter = value.clamp(0.0, 1.0),
            4 => self.param_warmth = value.clamp(0.0, 1.0),
            5 => self.param_mix = value.clamp(0.0, 1.0),
            6 => self.param_hysteresis = value.clamp(0.0, 1.0),
            _ => {}
        }
    }

    pub fn get_parameter(&self, param_id: u32) -> f32 {
        match param_id {
            1 => self.param_drive,
            2 => self.param_speed,
            3 => self.param_wow_flutter,
            4 => self.param_warmth,
            5 => self.param_mix,
            6 => self.param_hysteresis,
            _ => 0.0,
        }
    }

    #[inline(always)]
    fn read_interpolated(&self, buffer: &[f32; FLUTTER_BUFFER_SIZE], delay_samples: f32) -> f32 {
        let max_d = (FLUTTER_BUFFER_SIZE - 4) as f32;
        let d = delay_samples.clamp(1.0, max_d);
        let int_d = d as usize;
        let frac = d - (int_d as f32);

        let idx0 = (self.write_idx + FLUTTER_BUFFER_SIZE - int_d) & FLUTTER_BUFFER_MASK;
        let idx1 = (idx0 + FLUTTER_BUFFER_SIZE - 1) & FLUTTER_BUFFER_MASK;

        let s0 = buffer[idx0];
        let s1 = buffer[idx1];
        s0 + frac * (s1 - s0)
    }

    pub fn process_block(
        &mut self,
        in_l: &[f32],
        in_r: &[f32],
        out_l: &mut [f32],
        out_r: &mut [f32],
    ) {
        let n_samples = in_l.len().min(out_l.len());
        if n_samples == 0 {
            return;
        }

        let speed_mode = self.param_speed.round() as i32;
        let speed_factor = match speed_mode {
            0 => 0.5f32,
            1 => 1.0f32,
            _ => 1.5f32,
        };

        // Jiles-Atherton parameters
        let drive = self.param_drive.clamp(0.5, 12.0);
        let hyst = self.param_hysteresis.clamp(0.0, 1.0);
        let k = (0.25 + 0.45 * hyst) / speed_factor.max(0.5);
        let alpha = 0.0015 + 0.003 * hyst;
        let c = 0.22 - 0.08 * hyst;
        let a = 0.95f32;
        let ms = 1.0f32;

        // Head bump filter coefficients (peaking filter ~60-85 Hz)
        let bump_freq = match speed_mode {
            0 => 55.0f32,
            1 => 70.0f32,
            _ => 85.0f32,
        };
        let bump_w0 = 2.0 * PI * bump_freq / self.sample_rate;
        let bump_alpha = bump_w0.sin() / (2.0 * 1.4); // Q = 1.4
        let bump_a = (10.0f32.powf(2.2 / 40.0)).clamp(0.5, 2.0); // +2.2 dB boost
        let b_b0 = 1.0 + bump_alpha * bump_a;
        let b_b1 = -2.0 * bump_w0.cos();
        let b_b2 = 1.0 - bump_alpha * bump_a;
        let b_a0 = 1.0 + bump_alpha / bump_a;
        let b_a1 = -2.0 * bump_w0.cos();
        let b_a2 = 1.0 - bump_alpha / bump_a;

        let bump_b0 = b_b0 / b_a0;
        let bump_b1 = b_b1 / b_a0;
        let bump_b2 = b_b2 / b_a0;
        let bump_a1 = b_a1 / b_a0;
        let bump_a2 = b_a2 / b_a0;

        // Gap-loss rolloff filter coefficients (lowpass 11-19 kHz)
        let rolloff_base = match speed_mode {
            0 => 11000.0f32 - self.param_warmth * 3000.0,
            1 => 15000.0f32 - self.param_warmth * 3000.0,
            _ => 19000.0f32 - self.param_warmth * 2000.0,
        };
        let rolloff_freq = rolloff_base.clamp(3000.0, self.sample_rate * 0.45);
        let r_w0 = 2.0 * PI * rolloff_freq / self.sample_rate;
        let r_alpha = r_w0.sin() / (2.0 * 0.707); // Butterworth Q
        let r_cos = r_w0.cos();
        let r_b0 = (1.0 - r_cos) * 0.5;
        let r_b1 = 1.0 - r_cos;
        let r_b2 = (1.0 - r_cos) * 0.5;
        let r_a0 = 1.0 + r_alpha;
        let r_a1 = -2.0 * r_cos;
        let r_a2 = 1.0 - r_alpha;

        let roll_b0 = r_b0 / r_a0;
        let roll_b1 = r_b1 / r_a0;
        let roll_b2 = r_b2 / r_a0;
        let roll_a1 = r_a1 / r_a0;
        let roll_a2 = r_a2 / r_a0;

        let wow_inc = (2.0 * PI * 0.8) / self.sample_rate;
        let flutter_inc = (2.0 * PI * 4.5) / self.sample_rate;
        let flutter_amount = self.param_wow_flutter.clamp(0.0, 1.0);
        let mix = self.param_mix.clamp(0.0, 1.0);
        let dry = 1.0 - mix;

        for s in 0..n_samples {
            let in_s_l = in_l[s];
            let in_s_r = if s < in_r.len() { in_r[s] } else { in_s_l };

            // Wow / Flutter modulation
            let wow = self.flutter_phase1.sin();
            let flutter = self.flutter_phase2.sin();
            let total_mod = (wow * 0.7 + flutter * 0.3) * flutter_amount * 0.0015;
            let delay_samples = ((0.003 + total_mod) * self.sample_rate).max(1.0);

            // Write to ring buffer
            self.flutter_buffer_l[self.write_idx] = in_s_l;
            self.flutter_buffer_r[self.write_idx] = in_s_r;

            // 1. Read fluttered signal
            let mut sig_l = self.read_interpolated(&self.flutter_buffer_l, delay_samples);
            let mut sig_r = self.read_interpolated(&self.flutter_buffer_r, delay_samples);

            // 2. Head bump
            sig_l = self.head_bump_l.process(sig_l, bump_b0, bump_b1, bump_b2, bump_a1, bump_a2);
            sig_r = self.head_bump_r.process(sig_r, bump_b0, bump_b1, bump_b2, bump_a1, bump_a2);

            // 3. Jiles-Atherton magnetic hysteresis
            sig_l = self.hysteresis_l.process(sig_l, drive, k, alpha, c, a, ms);
            sig_r = self.hysteresis_r.process(sig_r, drive, k, alpha, c, a, ms);

            // 4. Gap loss rolloff
            sig_l = self.rolloff_l.process(sig_l, roll_b0, roll_b1, roll_b2, roll_a1, roll_a2);
            sig_r = self.rolloff_r.process(sig_r, roll_b0, roll_b1, roll_b2, roll_a1, roll_a2);

            // 5. Output mix
            out_l[s] = in_s_l * dry + sig_l * mix;
            if s < out_r.len() {
                out_r[s] = in_s_r * dry + sig_r * mix;
            }

            self.write_idx = (self.write_idx + 1) & FLUTTER_BUFFER_MASK;
            self.flutter_phase1 += wow_inc;
            self.flutter_phase2 += flutter_inc;
            if self.flutter_phase1 >= 2.0 * PI {
                self.flutter_phase1 -= 2.0 * PI;
            }
            if self.flutter_phase2 >= 2.0 * PI {
                self.flutter_phase2 -= 2.0 * PI;
            }
        }
    }
}
