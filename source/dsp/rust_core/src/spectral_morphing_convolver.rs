//! Spectral Morphing Convolver & Dual Acoustic Impulse Morph Engine
//! Continuous real-time morphing between dual parameterized synthetic impulse models (Cathedral vs Crystal Tank).
#[inline(always)]
fn clamp(val: f32, min: f32, max: f32) -> f32 {
    if val < min { min } else if val > max { max } else { val }
}

#[inline(always)]
fn lerp(a: f32, b: f32, t: f32) -> f32 {
    a + t * (b - a)
}

#[derive(Clone, Copy)]
struct AllpassStage {
    buf: [f32; 2048],
    idx: usize,
    len: usize,
}

impl AllpassStage {
    fn new(len: usize) -> Self {
        Self {
            buf: [0.0; 2048],
            idx: 0,
            len: len.clamp(1, 2047),
        }
    }

    fn reset(&mut self) {
        self.buf.fill(0.0);
        self.idx = 0;
    }

    #[inline(always)]
    fn process(&mut self, x: f32, coeff: f32) -> f32 {
        let buf_out = self.buf[self.idx];
        let y = -coeff * x + buf_out;
        self.buf[self.idx] = x + coeff * y;
        self.idx = (self.idx + 1) % self.len;
        y
    }
}

pub struct RustSpectralMorphingConvolver {
    sample_rate: f32,

    // Delay lines for dual spaces
    predelay_buf_l: [f32; 4096],
    predelay_buf_r: [f32; 4096],
    predelay_idx: usize,

    // Space A: Cathedral (4 parallel comb lines + 2 series allpass diffusers)
    comb_buf_a1: [f32; 4096],
    comb_buf_a2: [f32; 4096],
    comb_buf_a3: [f32; 4096],
    comb_buf_a4: [f32; 4096],
    comb_idx_a1: usize,
    comb_idx_a2: usize,
    comb_idx_a3: usize,
    comb_idx_a4: usize,
    comb_damp_a1: f32,
    comb_damp_a2: f32,
    comb_damp_a3: f32,
    comb_damp_a4: f32,
    ap_a1: AllpassStage,
    ap_a2: AllpassStage,

    // Space B: Metallic Crystal Tank (4 parallel comb lines + 3 series allpass diffusers with chirp)
    comb_buf_b1: [f32; 2048],
    comb_buf_b2: [f32; 2048],
    comb_buf_b3: [f32; 2048],
    comb_buf_b4: [f32; 2048],
    comb_idx_b1: usize,
    comb_idx_b2: usize,
    comb_idx_b3: usize,
    comb_idx_b4: usize,
    comb_damp_b1: f32,
    comb_damp_b2: f32,
    comb_damp_b3: f32,
    comb_damp_b4: f32,
    ap_b1: AllpassStage,
    ap_b2: AllpassStage,
    ap_b3: AllpassStage,

    morph: f32,
    decay_time_s: f32,
    damping: f32,
    predelay_ms: f32,
    stereo_spread: f32,
    mix: f32,

    smoothed_morph: f32,
    smoothed_mix: f32,
}

impl RustSpectralMorphingConvolver {
    pub fn new(sample_rate: f32) -> Self {
        let sr = if sample_rate > 8000.0 { sample_rate } else { 44100.0 };
        let mut inst = Self {
            sample_rate: sr,
            predelay_buf_l: [0.0; 4096],
            predelay_buf_r: [0.0; 4096],
            predelay_idx: 0,
            comb_buf_a1: [0.0; 4096],
            comb_buf_a2: [0.0; 4096],
            comb_buf_a3: [0.0; 4096],
            comb_buf_a4: [0.0; 4096],
            comb_idx_a1: 0,
            comb_idx_a2: 0,
            comb_idx_a3: 0,
            comb_idx_a4: 0,
            comb_damp_a1: 0.0,
            comb_damp_a2: 0.0,
            comb_damp_a3: 0.0,
            comb_damp_a4: 0.0,
            ap_a1: AllpassStage::new(556),
            ap_a2: AllpassStage::new(441),
            comb_buf_b1: [0.0; 2048],
            comb_buf_b2: [0.0; 2048],
            comb_buf_b3: [0.0; 2048],
            comb_buf_b4: [0.0; 2048],
            comb_idx_b1: 0,
            comb_idx_b2: 0,
            comb_idx_b3: 0,
            comb_idx_b4: 0,
            comb_damp_b1: 0.0,
            comb_damp_b2: 0.0,
            comb_damp_b3: 0.0,
            comb_damp_b4: 0.0,
            ap_b1: AllpassStage::new(341),
            ap_b2: AllpassStage::new(225),
            ap_b3: AllpassStage::new(173),
            morph: 0.5,
            decay_time_s: 3.5,
            damping: 0.45,
            predelay_ms: 15.0,
            stereo_spread: 0.8,
            mix: 0.65,
            smoothed_morph: 0.5,
            smoothed_mix: 0.65,
        };
        inst.prepare(sr);
        inst
    }

    pub fn prepare(&mut self, sample_rate: f32) {
        self.sample_rate = if sample_rate > 8000.0 { sample_rate } else { 44100.0 };
        let scale = self.sample_rate / 44100.0;
        self.ap_a1 = AllpassStage::new((556.0 * scale) as usize);
        self.ap_a2 = AllpassStage::new((441.0 * scale) as usize);
        self.ap_b1 = AllpassStage::new((341.0 * scale) as usize);
        self.ap_b2 = AllpassStage::new((225.0 * scale) as usize);
        self.ap_b3 = AllpassStage::new((173.0 * scale) as usize);
        self.reset();
    }

    pub fn reset(&mut self) {
        self.predelay_buf_l.fill(0.0);
        self.predelay_buf_r.fill(0.0);
        self.predelay_idx = 0;

        self.comb_buf_a1.fill(0.0);
        self.comb_buf_a2.fill(0.0);
        self.comb_buf_a3.fill(0.0);
        self.comb_buf_a4.fill(0.0);
        self.comb_idx_a1 = 0;
        self.comb_idx_a2 = 0;
        self.comb_idx_a3 = 0;
        self.comb_idx_a4 = 0;
        self.comb_damp_a1 = 0.0;
        self.comb_damp_a2 = 0.0;
        self.comb_damp_a3 = 0.0;
        self.comb_damp_a4 = 0.0;
        self.ap_a1.reset();
        self.ap_a2.reset();

        self.comb_buf_b1.fill(0.0);
        self.comb_buf_b2.fill(0.0);
        self.comb_buf_b3.fill(0.0);
        self.comb_buf_b4.fill(0.0);
        self.comb_idx_b1 = 0;
        self.comb_idx_b2 = 0;
        self.comb_idx_b3 = 0;
        self.comb_idx_b4 = 0;
        self.comb_damp_b1 = 0.0;
        self.comb_damp_b2 = 0.0;
        self.comb_damp_b3 = 0.0;
        self.comb_damp_b4 = 0.0;
        self.ap_b1.reset();
        self.ap_b2.reset();
        self.ap_b3.reset();

        self.smoothed_morph = self.morph;
        self.smoothed_mix = self.mix;
    }

    pub fn set_param(&mut self, param_id: u32, value: f32) {
        match param_id {
            1 => self.morph = clamp(value, 0.0, 1.0),
            2 => self.decay_time_s = clamp(value, 0.3, 10.0),
            3 => self.damping = clamp(value, 0.0, 1.0),
            4 => self.predelay_ms = clamp(value, 0.0, 100.0),
            5 => self.stereo_spread = clamp(value, 0.0, 1.0),
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

        let scale = self.sample_rate / 44100.0;
        // Space A lengths (Cathedral)
        let len_a1 = (2205.0 * scale) as usize;
        let len_a2 = (2691.0 * scale) as usize;
        let len_a3 = (3177.0 * scale) as usize;
        let len_a4 = (3643.0 * scale) as usize;

        // Space B lengths (Crystal Tank)
        let len_b1 = (883.0 * scale) as usize;
        let len_b2 = (1103.0 * scale) as usize;
        let len_b3 = (1323.0 * scale) as usize;
        let len_b4 = (1543.0 * scale) as usize;

        let predelay_len = ((self.predelay_ms * 0.001 * self.sample_rate) as usize).clamp(1, 4095);
        let fb_gain = (10.0f32.powf(-3.0 * 0.075 / self.decay_time_s)).clamp(0.2, 0.94);
        let damp = self.damping * 0.65;

        let smooth = (-1.0 / (0.005 * self.sample_rate)).exp();

        for i in 0..num_samples as usize {
            let xl = unsafe { *in_l.add(i) };
            let xr = unsafe { *in_r.add(i) };

            self.smoothed_morph = lerp(self.morph, self.smoothed_morph, smooth);
            self.smoothed_mix = lerp(self.mix, self.smoothed_mix, smooth);

            // Predelay tap
            let p_idx = self.predelay_idx;
            self.predelay_buf_l[p_idx] = xl;
            self.predelay_buf_r[p_idx] = xr;
            let read_p_idx = (p_idx + 4096 - predelay_len) % 4096;
            let pre_l = self.predelay_buf_l[read_p_idx];
            let pre_r = self.predelay_buf_r[read_p_idx];
            self.predelay_idx = (p_idx + 1) % 4096;

            let in_mono = 0.5 * (pre_l + pre_r);

            // 1. Process Space A: Cathedral
            let c_out_a1 = self.comb_buf_a1[self.comb_idx_a1];
            let c_out_a2 = self.comb_buf_a2[self.comb_idx_a2];
            let c_out_a3 = self.comb_buf_a3[self.comb_idx_a3];
            let c_out_a4 = self.comb_buf_a4[self.comb_idx_a4];

            self.comb_damp_a1 = c_out_a1 * (1.0 - damp) + self.comb_damp_a1 * damp;
            self.comb_damp_a2 = c_out_a2 * (1.0 - damp) + self.comb_damp_a2 * damp;
            self.comb_damp_a3 = c_out_a3 * (1.0 - damp) + self.comb_damp_a3 * damp;
            self.comb_damp_a4 = c_out_a4 * (1.0 - damp) + self.comb_damp_a4 * damp;

            self.comb_buf_a1[self.comb_idx_a1] = in_mono + self.comb_damp_a1 * fb_gain;
            self.comb_buf_a2[self.comb_idx_a2] = in_mono + self.comb_damp_a2 * fb_gain;
            self.comb_buf_a3[self.comb_idx_a3] = in_mono + self.comb_damp_a3 * fb_gain;
            self.comb_buf_a4[self.comb_idx_a4] = in_mono + self.comb_damp_a4 * fb_gain;

            self.comb_idx_a1 = (self.comb_idx_a1 + 1) % len_a1;
            self.comb_idx_a2 = (self.comb_idx_a2 + 1) % len_a2;
            self.comb_idx_a3 = (self.comb_idx_a3 + 1) % len_a3;
            self.comb_idx_a4 = (self.comb_idx_a4 + 1) % len_a4;

            let sum_a_l = c_out_a1 - c_out_a2 + c_out_a3 - c_out_a4;
            let sum_a_r = c_out_a1 + c_out_a2 - c_out_a3 - c_out_a4;

            let diff_a_l = self.ap_a1.process(sum_a_l, 0.5);
            let diff_a_r = self.ap_a2.process(sum_a_r, 0.5);

            // 2. Process Space B: Crystal Tank
            let c_out_b1 = self.comb_buf_b1[self.comb_idx_b1];
            let c_out_b2 = self.comb_buf_b2[self.comb_idx_b2];
            let c_out_b3 = self.comb_buf_b3[self.comb_idx_b3];
            let c_out_b4 = self.comb_buf_b4[self.comb_idx_b4];

            let damp_b = damp * 0.4; // brighter tank
            self.comb_damp_b1 = c_out_b1 * (1.0 - damp_b) + self.comb_damp_b1 * damp_b;
            self.comb_damp_b2 = c_out_b2 * (1.0 - damp_b) + self.comb_damp_b2 * damp_b;
            self.comb_damp_b3 = c_out_b3 * (1.0 - damp_b) + self.comb_damp_b3 * damp_b;
            self.comb_damp_b4 = c_out_b4 * (1.0 - damp_b) + self.comb_damp_b4 * damp_b;

            self.comb_buf_b1[self.comb_idx_b1] = in_mono + self.comb_damp_b1 * fb_gain;
            self.comb_buf_b2[self.comb_idx_b2] = in_mono + self.comb_damp_b2 * fb_gain;
            self.comb_buf_b3[self.comb_idx_b3] = in_mono + self.comb_damp_b3 * fb_gain;
            self.comb_buf_b4[self.comb_idx_b4] = in_mono + self.comb_damp_b4 * fb_gain;

            self.comb_idx_b1 = (self.comb_idx_b1 + 1) % len_b1;
            self.comb_idx_b2 = (self.comb_idx_b2 + 1) % len_b2;
            self.comb_idx_b3 = (self.comb_idx_b3 + 1) % len_b3;
            self.comb_idx_b4 = (self.comb_idx_b4 + 1) % len_b4;

            let sum_b_l = c_out_b1 + c_out_b2 - c_out_b3 - c_out_b4;
            let sum_b_r = c_out_b1 - c_out_b2 + c_out_b3 - c_out_b4;

            let diff_b_1 = self.ap_b1.process(sum_b_l, 0.65);
            let diff_b_l = self.ap_b2.process(diff_b_1, 0.6);
            let diff_b_r = self.ap_b3.process(sum_b_r, 0.65);

            // 3. Spectral Morphing Interpolation
            let m = self.smoothed_morph;
            let morphed_l = lerp(diff_a_l, diff_b_l, m);
            let morphed_r = lerp(diff_a_r, diff_b_r, m);

            // Stereo Width Control
            let mid = 0.5 * (morphed_l + morphed_r);
            let side = 0.5 * (morphed_l - morphed_r) * (1.0 + self.stereo_spread * 0.8);
            let wet_l = (mid + side) * 0.6;
            let wet_r = (mid - side) * 0.6;

            let yl = lerp(xl, wet_l, self.smoothed_mix);
            let yr = lerp(xr, wet_r, self.smoothed_mix);

            unsafe {
                *out_l.add(i) = yl;
                *out_r.add(i) = yr;
            }
        }
    }
}
