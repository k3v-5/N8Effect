//! KarplusStrong physical plucked string resonator in Rust
//! Direct replacement for KarplusStrongNode C++ DSP implementation.

use crate::common::{fast_tanh, RustBodyResonator, DENORMAL_GUARD};

pub const KARPLUS_BUFFER_SIZE: usize = 16384;
pub const KARPLUS_BUFFER_MASK: usize = KARPLUS_BUFFER_SIZE - 1;

#[repr(C)]
pub struct RustKarplusStrong {
    sample_rate: f32,

    // Bounded static delay buffers
    buffer_l: [f32; KARPLUS_BUFFER_SIZE],
    buffer_r: [f32; KARPLUS_BUFFER_SIZE],
    write_idx: usize,

    // Damping and feedback states
    prev_feedback_l: f32,
    prev_feedback_r: f32,

    // PRNG and transient trigger state
    rng_state: u32,
    env_follower: f32,
    burst_counter: i32,

    // Parameters
    param_pitch: f32,         // 1: 25.0 .. 2000.0 Hz
    param_damping: f32,       // 2: 0.01 .. 0.99
    param_decay: f32,         // 3: 0.1 .. 5.0 s
    param_pick_pos: f32,      // 4: 0.05 .. 0.95
    param_audio_trigger: f32, // 5: 0.0 (Continuous) or 1.0 (Trigger Mode)
    param_mix: f32,           // 6: 0.0 .. 1.0
    param_body_size: f32,     // 7: 0.5 .. 2.0
    param_body_decay: f32,    // 8: 0.1 .. 3.0
    param_body_mix: f32,      // 9: 0.0 .. 1.0

    // Acoustic body resonator
    body_resonator: RustBodyResonator,
}

impl RustKarplusStrong {
    pub fn new(sample_rate: f32) -> Self {
        let sr = if sample_rate > 0.0 { sample_rate } else { 44100.0 };
        let mut node = Self {
            sample_rate: sr,
            buffer_l: [0.0; KARPLUS_BUFFER_SIZE],
            buffer_r: [0.0; KARPLUS_BUFFER_SIZE],
            write_idx: 0,
            prev_feedback_l: 0.0,
            prev_feedback_r: 0.0,
            rng_state: 0x98765432,
            env_follower: 0.0,
            burst_counter: 0,
            param_pitch: 220.0,
            param_damping: 0.5,
            param_decay: 1.5,
            param_pick_pos: 0.25,
            param_audio_trigger: 1.0,
            param_mix: 1.0,
            param_body_size: 1.0,
            param_body_decay: 1.0,
            param_body_mix: 0.4,
            body_resonator: RustBodyResonator::new(sr),
        };
        node.update_body_parameters();
        node
    }

    pub fn reset(&mut self) {
        self.buffer_l.fill(0.0);
        self.buffer_r.fill(0.0);
        self.write_idx = 0;
        self.prev_feedback_l = 0.0;
        self.prev_feedback_r = 0.0;
        self.env_follower = 0.0;
        self.burst_counter = 0;
        self.body_resonator.reset();
    }

    pub fn set_sample_rate(&mut self, sample_rate: f32) {
        if sample_rate > 0.0 {
            self.sample_rate = sample_rate;
            self.body_resonator.set_sample_rate(sample_rate);
        }
    }

    fn update_body_parameters(&mut self) {
        self.body_resonator.set_parameters(
            self.param_body_size,
            self.param_body_decay,
            self.param_body_mix,
        );
    }

    pub fn set_parameter(&mut self, param_id: u32, value: f32) {
        match param_id {
            1 => self.param_pitch = value.clamp(25.0, 2000.0),
            2 => self.param_damping = value.clamp(0.01, 0.99),
            3 => self.param_decay = value.clamp(0.1, 5.0),
            4 => self.param_pick_pos = value.clamp(0.05, 0.95),
            5 => self.param_audio_trigger = if value >= 0.5 { 1.0 } else { 0.0 },
            6 => self.param_mix = value.clamp(0.0, 1.0),
            7 => {
                self.param_body_size = value.clamp(0.5, 2.0);
                self.update_body_parameters();
            }
            8 => {
                self.param_body_decay = value.clamp(0.1, 3.0);
                self.update_body_parameters();
            }
            9 => {
                self.param_body_mix = value.clamp(0.0, 1.0);
                self.update_body_parameters();
            }
            _ => {}
        }
    }

    pub fn get_parameter(&self, param_id: u32) -> f32 {
        match param_id {
            1 => self.param_pitch,
            2 => self.param_damping,
            3 => self.param_decay,
            4 => self.param_pick_pos,
            5 => self.param_audio_trigger,
            6 => self.param_mix,
            7 => self.param_body_size,
            8 => self.param_body_decay,
            9 => self.param_body_mix,
            _ => 0.0,
        }
    }

    #[inline(always)]
    fn next_noise(&mut self) -> f32 {
        self.rng_state ^= self.rng_state << 13;
        self.rng_state ^= self.rng_state >> 17;
        self.rng_state ^= self.rng_state << 5;
        ((self.rng_state & 0x7FFFFFFF) as f32 / 1073741824.0) - 1.0
    }

    #[inline(always)]
    fn read_hermite(buffer: &[f32; KARPLUS_BUFFER_SIZE], write_pos: usize, delay_samples: f64) -> f32 {
        let read_pos = (write_pos as f64) - delay_samples;
        let mut wrapped = read_pos % (KARPLUS_BUFFER_SIZE as f64);
        if wrapped < 0.0 {
            wrapped += KARPLUS_BUFFER_SIZE as f64;
        }

        let i1 = (wrapped as usize) & KARPLUS_BUFFER_MASK;
        let i0 = (i1 + KARPLUS_BUFFER_SIZE - 1) & KARPLUS_BUFFER_MASK;
        let i2 = (i1 + 1) & KARPLUS_BUFFER_MASK;
        let i3 = (i1 + 2) & KARPLUS_BUFFER_MASK;

        let frac = (wrapped - (i1 as f64)) as f32;

        let y0 = buffer[i0];
        let y1 = buffer[i1];
        let y2 = buffer[i2];
        let y3 = buffer[i3];

        let c0 = y1;
        let c1 = 0.5 * (y2 - y0);
        let c2 = y0 - 2.5 * y1 + 2.0 * y2 - 0.5 * y3;
        let c3 = 0.5 * (y3 - y0) + 1.5 * (y1 - y2);

        ((c3 * frac + c2) * frac + c1) * frac + c0
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

        let sr = if self.sample_rate > 0.0 { self.sample_rate as f64 } else { 44100.0 };
        let fundamental = (self.param_pitch as f64).clamp(25.0, sr * 0.45);
        let delay_length = ((sr / fundamental) - 0.5).clamp(2.0, (KARPLUS_BUFFER_SIZE - 8) as f64);

        let loop_gain = 0.001f64.powf(1.0 / (self.param_decay as f64 * fundamental));
        let feedback_gain = loop_gain.clamp(0.85, 0.9995) as f32;

        let damping = self.param_damping;
        let pick_pos = self.param_pick_pos as f64;
        let trigger_mode = self.param_audio_trigger >= 0.5;
        let mix = self.param_mix;
        let pick_delay = delay_length * pick_pos;

        for i in 0..n_samples {
            let raw_in_l = in_l[i];
            let raw_in_r = if i < in_r.len() { in_r[i] } else { raw_in_l };
            let in_mono = 0.5 * (raw_in_l + raw_in_r);

            let mut excitation = 0.0f32;

            if trigger_mode {
                let in_mag = in_mono.abs();
                if in_mag > self.env_follower + 0.12 && self.burst_counter == 0 {
                    self.burst_counter = (delay_length * 0.75) as i32;
                }
                self.env_follower = 0.98 * self.env_follower + 0.02 * in_mag;

                if self.burst_counter > 0 {
                    self.burst_counter -= 1;
                    excitation = self.next_noise() * 0.8;
                }
            } else {
                excitation = in_mono * 0.6;
            }

            // Pick Position Comb Filter
            let shaped_excitation_l = excitation - 0.9 * Self::read_hermite(&self.buffer_l, self.write_idx, pick_delay);
            let shaped_excitation_r = excitation - 0.9 * Self::read_hermite(&self.buffer_r, self.write_idx, pick_delay);

            // Read end of string
            let delay_out_l = Self::read_hermite(&self.buffer_l, self.write_idx, delay_length);
            let delay_out_r = Self::read_hermite(&self.buffer_r, self.write_idx, delay_length);

            // One-pole lowpass damping filter with denormal guard
            let filtered_l = (1.0 - damping) * delay_out_l + damping * self.prev_feedback_l + DENORMAL_GUARD;
            let filtered_r = (1.0 - damping) * delay_out_r + damping * self.prev_feedback_r + DENORMAL_GUARD;
            self.prev_feedback_l = delay_out_l;
            self.prev_feedback_r = delay_out_r;

            // Non-linear feedback loop with soft saturation
            let next_loop_l = fast_tanh(shaped_excitation_l + filtered_l * feedback_gain);
            let next_loop_r = fast_tanh(shaped_excitation_r + filtered_r * feedback_gain);

            self.buffer_l[self.write_idx] = next_loop_l;
            self.buffer_r[self.write_idx] = next_loop_r;
            self.write_idx = (self.write_idx + 1) & KARPLUS_BUFFER_MASK;

            let raw_wet_l = delay_out_l * 1.4;
            let raw_wet_r = delay_out_r * 1.4;

            let (wet_l, wet_r) = self.body_resonator.process_sample(raw_wet_l, raw_wet_r);

            out_l[i] = (1.0 - mix) * raw_in_l + mix * wet_l;
            if i < out_r.len() {
                out_r[i] = (1.0 - mix) * raw_in_r + mix * wet_r;
            }
        }
    }
}
