//! Common DSP utilities and building blocks for n8_rust_core
//! Zero allocations, denormal guards, and high-performance algorithms.

use std::f32::consts::PI;

pub const DENORMAL_GUARD: f32 = 1.0e-18;

/// Flush subnormals/denormals to zero to prevent CPU penalties.
#[inline(always)]
pub fn denormal_guard(x: f32) -> f32 {
    if x.abs() < 1.0e-15 {
        0.0
    } else {
        x
    }
}

/// Standard scalar clamp helper
#[inline(always)]
pub fn clamp(x: f32, min_val: f32, max_val: f32) -> f32 {
    if x < min_val { min_val } else if x > max_val { max_val } else { x }
}

/// Standard linear interpolation helper
#[inline(always)]
pub fn lerp(a: f32, b: f32, t: f32) -> f32 {
    a + t * (b - a)
}

/// Fast rational approximation to tanh(x) with C^inf continuity and no trascendentals.
#[inline(always)]
pub fn fast_tanh(x: f32) -> f32 {
    let x2 = x * x;
    x * (27.0 + x2) / (27.0 + 9.0 * x2)
}

/// Fast polynomial approximation of sin(x) for x in [-PI, PI].
#[inline(always)]
pub fn fast_sin(x: f32) -> f32 {
    // Parabolic approximation with Taylor correction
    let b = 4.0 / PI;
    let c = -4.0 / (PI * PI);
    let y = b * x + c * x * x.abs();
    // High precision smoothing
    0.225 * (y * y.abs() - y) + y
}

/// Normalized cubic soft clipper with unit slope at origin.
#[inline(always)]
pub fn fast_soft_clip(x: f32) -> f32 {
    if x >= 1.5 {
        1.0
    } else if x <= -1.5 {
        -1.0
    } else {
        x - (4.0 / 27.0) * (x * x * x)
    }
}

/// Fast Padé [2/2] rational approximation of exp(x) for x <= 0.
#[inline(always)]
pub fn fast_exp_negative(x: f32) -> f32 {
    if x <= -16.0 {
        0.0
    } else if x >= 0.0 {
        1.0
    } else {
        let x2 = x * x;
        let num = 12.0 + 6.0 * x + x2;
        let den = 12.0 - 6.0 * x + x2;
        num / den
    }
}

/// Power-of-two circular delay buffer with cubic Hermite interpolation.
/// Zero heap allocations in audio processing thread.
pub struct DelayBuffer {
    buffer: Box<[f32]>,
    mask: usize,
    write_idx: usize,
    max_delay: f32,
}

impl DelayBuffer {
    pub fn new(capacity: usize) -> Self {
        let cap = capacity.next_power_of_two().max(16);
        Self {
            buffer: vec![0.0f32; cap].into_boxed_slice(),
            mask: cap - 1,
            write_idx: 0,
            max_delay: (cap - 4) as f32,
        }
    }

    #[inline(always)]
    pub fn reset(&mut self) {
        self.buffer.fill(0.0);
        self.write_idx = 0;
    }

    #[inline(always)]
    pub fn write(&mut self, sample: f32) {
        self.buffer[self.write_idx] = sample;
        self.write_idx = (self.write_idx + 1) & self.mask;
    }

    #[inline(always)]
    pub fn read_cubic(&self, delay_samples: f32) -> f32 {
        let clamped = delay_samples.clamp(1.0, self.max_delay);
        let read_pos = (self.write_idx as f32) - clamped;
        let i_pos = read_pos.floor() as isize;
        let frac = read_pos - (i_pos as f32);

        let idx0 = ((i_pos.wrapping_sub(1)) as usize) & self.mask;
        let idx1 = (i_pos as usize) & self.mask;
        let idx2 = ((i_pos.wrapping_add(1)) as usize) & self.mask;
        let idx3 = ((i_pos.wrapping_add(2)) as usize) & self.mask;

        let xm1 = self.buffer[idx0];
        let x0 = self.buffer[idx1];
        let x1 = self.buffer[idx2];
        let x2 = self.buffer[idx3];

        let c0 = x0;
        let c1 = 0.5 * (x1 - xm1);
        let c2 = xm1 - 2.5 * x0 + 2.0 * x1 - 0.5 * x2;
        let c3 = 0.5 * (x2 - xm1) + 1.5 * (x0 - x1);

        ((c3 * frac + c2) * frac + c1) * frac + c0
    }
}

#[repr(C)]
#[derive(Clone, Copy)]
pub struct BiquadCoeffs {
    pub b0: f32,
    pub b1: f32,
    pub b2: f32,
    pub a1: f32,
    pub a2: f32,
}

impl Default for BiquadCoeffs {
    fn default() -> Self {
        Self {
            b0: 1.0,
            b1: 0.0,
            b2: 0.0,
            a1: 0.0,
            a2: 0.0,
        }
    }
}

impl BiquadCoeffs {
    pub fn bandpass(sample_rate: f32, freq_hz: f32, q: f32) -> Self {
        let sr = if sample_rate > 0.0 { sample_rate } else { 44100.0 };
        let nyquist = sr * 0.499;
        let fc = freq_hz.clamp(10.0, nyquist);
        let q_val = q.max(0.01);
        let omega = 2.0 * PI * fc / sr;
        let sn = omega.sin();
        let cs = omega.cos();
        let alpha = sn / (2.0 * q_val);

        let a0 = 1.0 + alpha;
        let inv_a0 = 1.0 / a0;
        let b0 = alpha * inv_a0;
        let b1 = 0.0;
        let b2 = -alpha * inv_a0;
        let a1 = (-2.0 * cs) * inv_a0;
        let a2 = (1.0 - alpha) * inv_a0;

        Self { b0, b1, b2, a1, a2 }
    }

    pub fn lowpass(sample_rate: f32, freq_hz: f32, q: f32) -> Self {
        let sr = if sample_rate > 0.0 { sample_rate } else { 44100.0 };
        let nyquist = sr * 0.499;
        let fc = freq_hz.clamp(10.0, nyquist);
        let q_val = q.max(0.01);
        let omega = 2.0 * PI * fc / sr;
        let sn = omega.sin();
        let cs = omega.cos();
        let alpha = sn / (2.0 * q_val);

        let a0 = 1.0 + alpha;
        let inv_a0 = 1.0 / a0;
        let b0 = ((1.0 - cs) * 0.5) * inv_a0;
        let b1 = (1.0 - cs) * inv_a0;
        let b2 = ((1.0 - cs) * 0.5) * inv_a0;
        let a1 = (-2.0 * cs) * inv_a0;
        let a2 = (1.0 - alpha) * inv_a0;

        Self { b0, b1, b2, a1, a2 }
    }

    pub fn highpass(sample_rate: f32, freq_hz: f32, q: f32) -> Self {
        let sr = if sample_rate > 0.0 { sample_rate } else { 44100.0 };
        let nyquist = sr * 0.499;
        let fc = freq_hz.clamp(10.0, nyquist);
        let q_val = q.max(0.01);
        let omega = 2.0 * PI * fc / sr;
        let sn = omega.sin();
        let cs = omega.cos();
        let alpha = sn / (2.0 * q_val);

        let a0 = 1.0 + alpha;
        let inv_a0 = 1.0 / a0;
        let b0 = ((1.0 + cs) * 0.5) * inv_a0;
        let b1 = (-(1.0 + cs)) * inv_a0;
        let b2 = ((1.0 + cs) * 0.5) * inv_a0;
        let a1 = (-2.0 * cs) * inv_a0;
        let a2 = (1.0 - alpha) * inv_a0;

        Self { b0, b1, b2, a1, a2 }
    }

    pub fn allpass(sample_rate: f32, freq_hz: f32, q: f32) -> Self {
        let sr = if sample_rate > 0.0 { sample_rate } else { 44100.0 };
        let nyquist = sr * 0.499;
        let fc = freq_hz.clamp(10.0, nyquist);
        let q_val = q.max(0.01);
        let omega = 2.0 * PI * fc / sr;
        let sn = omega.sin();
        let cs = omega.cos();
        let alpha = sn / (2.0 * q_val);

        let a0 = 1.0 + alpha;
        let inv_a0 = 1.0 / a0;
        let b0 = (1.0 - alpha) * inv_a0;
        let b1 = (-2.0 * cs) * inv_a0;
        let b2 = (1.0 + alpha) * inv_a0;
        let a1 = (-2.0 * cs) * inv_a0;
        let a2 = (1.0 - alpha) * inv_a0;

        Self { b0, b1, b2, a1, a2 }
    }

    pub fn peak(sample_rate: f32, freq_hz: f32, q: f32, gain_db: f32) -> Self {
        let sr = if sample_rate > 0.0 { sample_rate } else { 44100.0 };
        let nyquist = sr * 0.499;
        let fc = freq_hz.clamp(10.0, nyquist);
        let q_val = q.max(0.01);
        let omega = 2.0 * PI * fc / sr;
        let sn = omega.sin();
        let cs = omega.cos();
        let alpha = sn / (2.0 * q_val);
        let a_val = 10.0f32.powf(gain_db / 40.0);

        let a0 = 1.0 + alpha / a_val;
        let inv_a0 = 1.0 / a0;
        let b0 = (1.0 + alpha * a_val) * inv_a0;
        let b1 = (-2.0 * cs) * inv_a0;
        let b2 = (1.0 - alpha * a_val) * inv_a0;
        let a1 = (-2.0 * cs) * inv_a0;
        let a2 = (1.0 - alpha / a_val) * inv_a0;

        Self { b0, b1, b2, a1, a2 }
    }
}

#[repr(C)]
#[derive(Clone, Copy, Default)]
pub struct BiquadState {
    pub z1: f32,
    pub z2: f32,
}

impl BiquadState {
    #[inline(always)]
    pub fn process(&mut self, in_val: f32, coeffs: &BiquadCoeffs) -> f32 {
        let out = coeffs.b0 * in_val + self.z1 + DENORMAL_GUARD;
        self.z1 = coeffs.b1 * in_val - coeffs.a1 * out + self.z2;
        self.z2 = coeffs.b2 * in_val - coeffs.a2 * out;
        out
    }

    #[inline(always)]
    pub fn reset(&mut self) {
        self.z1 = 0.0;
        self.z2 = 0.0;
    }
}

/// 4-Mode Acoustic Body Resonator implemented in pure Rust
#[repr(C)]
pub struct RustBodyResonator {
    coeffs_l: [BiquadCoeffs; 4],
    coeffs_r: [BiquadCoeffs; 4],
    state_l: [BiquadState; 4],
    state_r: [BiquadState; 4],
    sample_rate: f32,
    body_size: f32,
    body_decay: f32,
    body_mix: f32,
}

impl RustBodyResonator {
    pub fn new(sample_rate: f32) -> Self {
        let mut res = Self {
            coeffs_l: [BiquadCoeffs::default(); 4],
            coeffs_r: [BiquadCoeffs::default(); 4],
            state_l: [BiquadState::default(); 4],
            state_r: [BiquadState::default(); 4],
            sample_rate: if sample_rate > 0.0 { sample_rate } else { 44100.0 },
            body_size: 1.0,
            body_decay: 1.0,
            body_mix: 0.4,
        };
        res.update_modes();
        res
    }

    pub fn reset(&mut self) {
        for s in &mut self.state_l {
            s.reset();
        }
        for s in &mut self.state_r {
            s.reset();
        }
    }

    pub fn set_sample_rate(&mut self, sample_rate: f32) {
        if sample_rate > 0.0 {
            self.sample_rate = sample_rate;
            self.update_modes();
        }
    }

    pub fn set_parameters(&mut self, body_size: f32, body_decay: f32, body_mix: f32) {
        let new_size = body_size.clamp(0.5, 2.0);
        let new_decay = body_decay.clamp(0.1, 3.0);
        let new_mix = body_mix.clamp(0.0, 1.0);

        let size_changed = (new_size - self.body_size).abs() > 1e-4;
        let decay_changed = (new_decay - self.body_decay).abs() > 1e-4;

        self.body_size = new_size;
        self.body_decay = new_decay;
        self.body_mix = new_mix;

        if size_changed || decay_changed {
            self.update_modes();
        }
    }

    pub fn update_modes(&mut self) {
        let base_freqs = [104.0f32, 220.0, 460.0, 1250.0];
        let base_qs = [4.5f32, 6.0, 5.5, 7.0];
        let nyquist = self.sample_rate * 0.49;

        for m in 0..4 {
            let freq_l = (base_freqs[m] / self.body_size).clamp(20.0, nyquist);
            let freq_r = (freq_l * 1.015).clamp(20.0, nyquist);
            let q = (base_qs[m] * self.body_decay).clamp(0.5, 30.0);

            self.coeffs_l[m] = BiquadCoeffs::bandpass(self.sample_rate, freq_l, q);
            self.coeffs_r[m] = BiquadCoeffs::bandpass(self.sample_rate, freq_r, q);
        }
    }

    #[inline(always)]
    pub fn process_sample(&mut self, in_l: f32, in_r: f32) -> (f32, f32) {
        if self.body_mix <= 0.0 {
            return (in_l, in_r);
        }

        let mode_sum_l = 0.35 * self.state_l[0].process(in_l, &self.coeffs_l[0])
                       + 0.30 * self.state_l[1].process(in_l, &self.coeffs_l[1])
                       + 0.25 * self.state_l[2].process(in_l, &self.coeffs_l[2])
                       + 0.18 * self.state_l[3].process(in_l, &self.coeffs_l[3]);

        let mode_sum_r = 0.35 * self.state_r[0].process(in_r, &self.coeffs_r[0])
                       + 0.30 * self.state_r[1].process(in_r, &self.coeffs_r[1])
                       + 0.25 * self.state_r[2].process(in_r, &self.coeffs_r[2])
                       + 0.18 * self.state_r[3].process(in_r, &self.coeffs_r[3]);

        let out_l = (1.0 - self.body_mix) * in_l + self.body_mix * mode_sum_l;
        let out_r = (1.0 - self.body_mix) * in_r + self.body_mix * mode_sum_r;
        (out_l, out_r)
    }
}
