//! Helical Spring Reverb Tank (Physical Torsional Dispersion) (Rules 5, 8, 9, 14, 18, 34, 35, 46, 47)
//! Physical model of dual/triple helical spring reverb coils.
//! Accurately reproduces frequency-dependent torsional dispersion (signature "boing / chirp" sound)
//! using a 16-stage cascaded allpass dispersive lattice, magnetic input transducer saturation,
//! reflection terminal delays, and spring metal high-frequency damping.

use crate::common::{clamp, lerp, denormal_guard, fast_tanh, DelayBuffer};
use std::f32::consts::PI;

const NUM_ALLPASS_STAGES: usize = 16;
const MAX_SPRING_DELAY_SAMPLES: usize = 8192; // Strictly bounded

#[derive(Clone, Copy, Default)]
struct Allpass1st {
    z1: f32,
}

impl Allpass1st {
    #[inline(always)]
    fn process(&mut self, x: f32, coeff: f32) -> f32 {
        let y = -coeff * x + self.z1;
        self.z1 = x + coeff * y;
        denormal_guard(y)
    }

    #[inline(always)]
    fn reset(&mut self) {
        self.z1 = 0.0;
    }
}

pub struct RustSpringReverbTank {
    sample_rate: f32,

    // Dispersive allpass cascades for Left and Right spring coils
    allpass_l: [Allpass1st; NUM_ALLPASS_STAGES],
    allpass_r: [Allpass1st; NUM_ALLPASS_STAGES],

    // Terminal reflection delay buffers
    spring_delay_l: DelayBuffer,
    spring_delay_r: DelayBuffer,

    // High frequency damping 1-pole lowpass
    damp_state_l: f32,
    damp_state_r: f32,

    // Energy tracking for silence gating (Rule 38)
    env_tank: f32,

    // Parameters
    tension: f32,          // 0.1 to 1.0 (allpass dispersion pole)
    decay_time: f32,       // 0.5 to 8.0 seconds
    boing_depth: f32,      // 0.0 to 1.0 (dispersive chirp intensity)
    drive: f32,            // 0.5 to 4.0 (magnetic transducer saturation)
    damping_hz: f32,       // 500.0 to 9000.0 Hz
    mix: f32,              // 0.0 to 1.0

    // Smoothed
    smoothed_mix: f32,
}

impl RustSpringReverbTank {
    pub fn new(sample_rate: f32) -> Self {
        let sr = if sample_rate > 1000.0 { sample_rate } else { 44100.0 };
        Self {
            sample_rate: sr,
            allpass_l: [Allpass1st::default(); NUM_ALLPASS_STAGES],
            allpass_r: [Allpass1st::default(); NUM_ALLPASS_STAGES],
            spring_delay_l: DelayBuffer::new(MAX_SPRING_DELAY_SAMPLES),
            spring_delay_r: DelayBuffer::new(MAX_SPRING_DELAY_SAMPLES),
            damp_state_l: 0.0,
            damp_state_r: 0.0,
            env_tank: 0.0,
            tension: 0.65,
            decay_time: 2.8,
            boing_depth: 0.75,
            drive: 1.2,
            damping_hz: 3800.0,
            mix: 0.45,
            smoothed_mix: 0.45,
        }
    }

    pub fn prepare(&mut self, sample_rate: f32) {
        self.sample_rate = if sample_rate > 1000.0 { sample_rate } else { 44100.0 };
        self.reset();
    }

    pub fn reset(&mut self) {
        for ap in &mut self.allpass_l { ap.reset(); }
        for ap in &mut self.allpass_r { ap.reset(); }
        self.spring_delay_l.reset();
        self.spring_delay_r.reset();
        self.damp_state_l = 0.0;
        self.damp_state_r = 0.0;
        self.env_tank = 0.0;
        self.smoothed_mix = self.mix;
    }

    pub fn set_param(&mut self, param_id: u32, value: f32) {
        match param_id {
            1 => self.tension = clamp(value, 0.1, 0.95),
            2 => self.decay_time = clamp(value, 0.3, 8.0),
            3 => self.boing_depth = clamp(value, 0.0, 1.0),
            4 => self.drive = clamp(value, 0.5, 4.0),
            5 => self.damping_hz = clamp(value, 500.0, 10000.0),
            6 => self.mix = clamp(value, 0.0, 1.0),
            _ => {}
        }
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

        // Delay lengths for the spring coil reflections (~35-65ms reflection paths)
        let delay_samples_l = (0.041 * self.sample_rate).clamp(100.0, (MAX_SPRING_DELAY_SAMPLES - 16) as f32);
        let delay_samples_r = (0.053 * self.sample_rate).clamp(100.0, (MAX_SPRING_DELAY_SAMPLES - 16) as f32);

        // Feedback coefficient calculated from desired T60 decay time
        let loop_time = 0.047; // average reflection loop time in seconds
        let fb_gain = (10.0f32.powf(-3.0 * loop_time / self.decay_time)).clamp(0.2, 0.94);

        // Allpass dispersion coefficient: higher tension = higher dispersion
        let ap_coeff = lerp(0.2, 0.85, self.tension);

        // Damping 1-pole filter alpha
        let damp_alpha = (-2.0 * PI * self.damping_hz / self.sample_rate).exp();
        let smooth = (-1.0 / (0.005 * self.sample_rate)).exp();

        for i in 0..num_samples as usize {
            let xl = unsafe { *in_l.add(i) };
            let xr = unsafe { *in_r.add(i) };

            self.smoothed_mix = lerp(self.mix, self.smoothed_mix, smooth);

            let in_mag = xl.abs() + xr.abs();
            self.env_tank = lerp(in_mag + self.damp_state_l.abs() + self.damp_state_r.abs(), self.env_tank, 0.9995);

            // Silence gating (Rule 38)
            if self.env_tank < 1.0e-5 && in_mag < 1.0e-5 {
                unsafe {
                    *out_l.add(i) = 0.0;
                    *out_r.add(i) = 0.0;
                }
                continue;
            }

            // Magnetic drive transducer saturation
            let driven_l = fast_tanh(xl * self.drive);
            let driven_r = fast_tanh(xr * self.drive);

            // Read terminal reflections from spring delay lines
            let ref_l = self.spring_delay_l.read_cubic(delay_samples_l);
            let ref_r = self.spring_delay_r.read_cubic(delay_samples_r);

            // Damping absorption filters on reflections
            self.damp_state_l = lerp(ref_l, self.damp_state_l, damp_alpha);
            self.damp_state_r = lerp(ref_r, self.damp_state_r, damp_alpha);

            // Sum input with recirculating reflections
            let mut node_l = driven_l + self.damp_state_l * fb_gain;
            let mut node_r = driven_r + self.damp_state_r * fb_gain;

            // Cascade through 16 allpass stages to create torsional chirp ("boing") dispersion
            if self.boing_depth > 0.01 {
                let eff_coeff = ap_coeff * self.boing_depth;
                for stage in 0..NUM_ALLPASS_STAGES {
                    node_l = self.allpass_l[stage].process(node_l, eff_coeff);
                    node_r = self.allpass_r[stage].process(node_r, eff_coeff);
                }
            }

            // Write back to spring reflection buffers
            self.spring_delay_l.write(node_l);
            self.spring_delay_r.write(node_r);

            // Wet tank output sum
            let wet_l = (ref_l + node_l * 0.5) * 0.7;
            let wet_r = (ref_r + node_r * 0.5) * 0.7;

            let out_sample_l = denormal_guard(lerp(xl, wet_l, self.smoothed_mix));
            let out_sample_r = denormal_guard(lerp(xr, wet_r, self.smoothed_mix));

            unsafe {
                *out_l.add(i) = out_sample_l;
                *out_r.add(i) = out_sample_r;
            }
        }
    }
}
