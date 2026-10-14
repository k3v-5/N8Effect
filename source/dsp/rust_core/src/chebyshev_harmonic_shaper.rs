//! Chebyshev Polynomial Harmonic Shaper (Rules 5, 8, 9, 14, 34, 35, 46, 47)
//! Generates pure, isolated harmonic overtones (H2 to H8) using Chebyshev polynomials of the first kind:
//! T0(x)=1, T1(x)=x, T2(x)=2x^2-1, T3(x)=4x^3-3x, T4(x)=8x^4-8x^2+1, T5(x)=16x^5-20x^3+5x,
//! T6(x)=32x^6-48x^4+18x^2-1, T7(x)=64x^7-112x^5+56x^3-7x, T8(x)=128x^8-256x^6+160x^4-32x^2+1.
//! Zero intermodulation mud, discrete harmonic control, DC-blocker filter, and envelope gating.

use crate::common::{clamp, lerp, denormal_guard};

#[repr(C)]
#[derive(Clone, Copy, Default)]
struct DcBlocker {
    x1: f32,
    y1: f32,
}

impl DcBlocker {
    #[inline(always)]
    fn process(&mut self, x: f32, r: f32) -> f32 {
        let y = x - self.x1 + r * self.y1;
        self.x1 = x;
        self.y1 = denormal_guard(y);
        y
    }

    #[inline(always)]
    fn reset(&mut self) {
        self.x1 = 0.0;
        self.y1 = 0.0;
    }
}

pub struct RustChebyshevHarmonicShaper {
    sample_rate: f32,
    dc_r: f32,
    dc_l: DcBlocker,
    dc_r_state: DcBlocker,

    // Input envelope follower for zero idle noise gating (Rule 38)
    env_in: f32,

    // Parameters
    drive: f32,
    h2: f32, // 2nd harmonic (octave)
    h3: f32, // 3rd harmonic (octave + 5th)
    h4: f32, // 4th harmonic (2 octaves)
    h5: f32, // 5th harmonic
    h6: f32, // 6th harmonic
    h7: f32, // 7th harmonic
    h8: f32, // 8th harmonic
    mix: f32,

    // Smoothed
    smoothed_drive: f32,
    smoothed_mix: f32,
}

impl RustChebyshevHarmonicShaper {
    pub fn new(sample_rate: f32) -> Self {
        let sr = if sample_rate > 1000.0 { sample_rate } else { 44100.0 };
        // DC blocker pole radius corresponding to ~10 Hz cutoff
        let r = (-2.0 * std::f32::consts::PI * 10.0 / sr).exp();
        Self {
            sample_rate: sr,
            dc_r: r,
            dc_l: DcBlocker::default(),
            dc_r_state: DcBlocker::default(),
            env_in: 0.0,
            drive: 1.0,
            h2: 0.3,
            h3: 0.2,
            h4: 0.1,
            h5: 0.0,
            h6: 0.0,
            h7: 0.0,
            h8: 0.0,
            mix: 1.0,
            smoothed_drive: 1.0,
            smoothed_mix: 1.0,
        }
    }

    pub fn prepare(&mut self, sample_rate: f32) {
        self.sample_rate = if sample_rate > 1000.0 { sample_rate } else { 44100.0 };
        self.dc_r = (-2.0 * std::f32::consts::PI * 10.0 / self.sample_rate).exp();
        self.reset();
    }

    pub fn reset(&mut self) {
        self.dc_l.reset();
        self.dc_r_state.reset();
        self.env_in = 0.0;
        self.smoothed_drive = self.drive;
        self.smoothed_mix = self.mix;
    }

    pub fn set_param(&mut self, param_id: u32, value: f32) {
        match param_id {
            1 => self.drive = clamp(value, 0.1, 10.0),
            2 => self.h2 = clamp(value, 0.0, 1.0),
            3 => self.h3 = clamp(value, 0.0, 1.0),
            4 => self.h4 = clamp(value, 0.0, 1.0),
            5 => self.h5 = clamp(value, 0.0, 1.0),
            6 => self.h6 = clamp(value, 0.0, 1.0),
            7 => self.h7 = clamp(value, 0.0, 1.0),
            8 => self.h8 = clamp(value, 0.0, 1.0),
            9 => self.mix = clamp(value, 0.0, 1.0),
            _ => {}
        }
    }

    /// Evaluates linear combination of Chebyshev polynomials
    #[inline(always)]
    fn evaluate_chebyshev(&self, x: f32) -> f32 {
        let x_norm = clamp(x, -1.0, 1.0);
        let x2 = x_norm * x_norm;
        let x3 = x2 * x_norm;
        let x4 = x2 * x2;
        let x5 = x4 * x_norm;
        let x6 = x4 * x2;
        let x7 = x6 * x_norm;
        let x8 = x4 * x4;

        // T1(x) = x (fundamental)
        let mut sum = x_norm;

        // Even polynomials with zero-bias offset subtraction to eliminate DC prior to DC blocker
        if self.h2 > 0.001 {
            // T2(x) = 2*x^2 - 1 -> zero-centered: 2*x^2
            sum += self.h2 * (2.0 * x2 - 1.0);
        }
        if self.h3 > 0.001 {
            // T3(x) = 4*x^3 - 3*x
            sum += self.h3 * (4.0 * x3 - 3.0 * x_norm);
        }
        if self.h4 > 0.001 {
            // T4(x) = 8*x^4 - 8*x^2 + 1
            sum += self.h4 * (8.0 * x4 - 8.0 * x2 + 1.0);
        }
        if self.h5 > 0.001 {
            // T5(x) = 16*x^5 - 20*x^3 + 5*x
            sum += self.h5 * (16.0 * x5 - 20.0 * x3 + 5.0 * x_norm);
        }
        if self.h6 > 0.001 {
            // T6(x) = 32*x^6 - 48*x^4 + 18*x^2 - 1
            sum += self.h6 * (32.0 * x6 - 48.0 * x4 + 18.0 * x2 - 1.0);
        }
        if self.h7 > 0.001 {
            // T7(x) = 64*x^7 - 112*x^5 + 56*x^3 - 7*x
            sum += self.h7 * (64.0 * x7 - 112.0 * x5 + 56.0 * x3 - 7.0 * x_norm);
        }
        if self.h8 > 0.001 {
            // T8(x) = 128*x^8 - 256*x^6 + 160*x^4 - 32*x^2 + 1
            sum += self.h8 * (128.0 * x8 - 256.0 * x6 + 160.0 * x4 - 32.0 * x2 + 1.0);
        }

        sum
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

        let smooth = (-1.0 / (0.005 * self.sample_rate)).exp();
        let env_alpha = (-1.0 / (0.010 * self.sample_rate)).exp();

        for i in 0..num_samples as usize {
            let xl = unsafe { *in_l.add(i) };
            let xr = unsafe { *in_r.add(i) };

            self.smoothed_drive = lerp(self.drive, self.smoothed_drive, smooth);
            self.smoothed_mix = lerp(self.mix, self.smoothed_mix, smooth);

            let in_mag = 0.5 * (xl.abs() + xr.abs());
            self.env_in = lerp(in_mag, self.env_in, env_alpha);

            // Silence gating (Rule 38): if signal is zero/subnormal, clear state & output zero
            if self.env_in < 1.0e-5 && in_mag < 1.0e-5 {
                self.dc_l.reset();
                self.dc_r_state.reset();
                unsafe {
                    *out_l.add(i) = 0.0;
                    *out_r.add(i) = 0.0;
                }
                continue;
            }

            let driven_l = xl * self.smoothed_drive;
            let driven_r = xr * self.smoothed_drive;

            let shaped_l = self.evaluate_chebyshev(driven_l);
            let shaped_r = self.evaluate_chebyshev(driven_r);

            // Remove DC bias generated by even harmonics
            let ac_l = self.dc_l.process(shaped_l, self.dc_r);
            let ac_r = self.dc_r_state.process(shaped_r, self.dc_r);

            let out_sample_l = denormal_guard(lerp(xl, ac_l, self.smoothed_mix));
            let out_sample_r = denormal_guard(lerp(xr, ac_r, self.smoothed_mix));

            unsafe {
                *out_l.add(i) = out_sample_l;
                *out_r.add(i) = out_sample_r;
            }
        }
    }
}
