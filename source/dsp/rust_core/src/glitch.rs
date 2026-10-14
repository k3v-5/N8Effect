//! Glitch, Stutter & Slicing DSP Kernel with Anti-Click Crossfade (DSP #11)
//! Rules 5, 8, 9, 13, 14, 16, 35, 37, 46, 47: Zero allocations in audio thread,
//! preallocated ring buffers, rhythmic slice repetition, reverse stutter,
//! gate envelopes with 32-sample anti-click fades, and denormal protection.

use crate::common::denormal_guard;

const MAX_BUFFER_SIZE: usize = 192000;
const DIVISION_MULTIPLIERS: [f32; 5] = [0.5, 0.25, 0.125, 0.0625, 0.03125];
const FADE_LEN: usize = 32;

pub struct RustGlitch {
    sample_rate: f32,
    buffer_capacity: usize,
    buffer_l: Box<[f32]>,
    buffer_r: Box<[f32]>,
    write_pos: usize,
    slice_start_pos: usize,
    slice_sample_counter: usize,
    is_glitching: bool,
    is_reverse: bool,
    rng_state: u32,

    target_division: f32,
    target_repeat_prob: f32,
    target_reverse_prob: f32,
    target_stutter_gate: f32,
    target_mix: f32,
}

impl RustGlitch {
    pub fn new(sample_rate: f32) -> Self {
        let sr = if sample_rate > 0.0 { sample_rate } else { 44100.0 };
        let buffer_capacity = ((sr * 2.0) as usize).min(MAX_BUFFER_SIZE).max(1024);

        Self {
            sample_rate: sr,
            buffer_capacity,
            buffer_l: vec![0.0f32; buffer_capacity].into_boxed_slice(),
            buffer_r: vec![0.0f32; buffer_capacity].into_boxed_slice(),
            write_pos: 0,
            slice_start_pos: 0,
            slice_sample_counter: 0,
            is_glitching: false,
            is_reverse: false,
            rng_state: 543210987,
            target_division: 2.0,
            target_repeat_prob: 0.5,
            target_reverse_prob: 0.2,
            target_stutter_gate: 1.0,
            target_mix: 0.5,
        }
    }

    pub fn prepare(&mut self, sample_rate: f32) {
        let sr = if sample_rate > 0.0 { sample_rate } else { 44100.0 };
        self.sample_rate = sr;
        let cap = ((sr * 2.0) as usize).min(MAX_BUFFER_SIZE).max(1024);
        if cap != self.buffer_capacity {
            self.buffer_capacity = cap;
            self.buffer_l = vec![0.0f32; cap].into_boxed_slice();
            self.buffer_r = vec![0.0f32; cap].into_boxed_slice();
        }
        self.reset();
    }

    pub fn reset(&mut self) {
        self.buffer_l.fill(0.0);
        self.buffer_r.fill(0.0);
        self.write_pos = 0;
        self.slice_start_pos = 0;
        self.slice_sample_counter = 0;
        self.is_glitching = false;
        self.is_reverse = false;
    }

    #[inline(always)]
    fn next_random_float(&mut self) -> f32 {
        self.rng_state = self.rng_state.wrapping_mul(1664525).wrapping_add(1013904223);
        (self.rng_state & 0x00FFFFFF) as f32 / 16777216.0
    }

    pub fn set_parameter(&mut self, id: u32, value: f32) {
        match id {
            1 => self.target_division = value.clamp(0.0, 4.0),
            2 => self.target_repeat_prob = value.clamp(0.0, 1.0),
            3 => self.target_reverse_prob = value.clamp(0.0, 1.0),
            4 => self.target_stutter_gate = value.clamp(0.05, 1.0),
            5 => self.target_mix = value.clamp(0.0, 1.0),
            _ => {}
        }
    }

    pub fn get_parameter(&self, id: u32) -> f32 {
        match id {
            1 => self.target_division,
            2 => self.target_repeat_prob,
            3 => self.target_reverse_prob,
            4 => self.target_stutter_gate,
            5 => self.target_mix,
            _ => 0.0,
        }
    }

    #[inline]
    pub fn process_block(
        &mut self,
        in_l: &[f32],
        in_r: &[f32],
        out_l: &mut [f32],
        out_r: &mut [f32],
    ) {
        let num_samples = in_l.len().min(out_l.len());
        if num_samples == 0 || self.buffer_capacity == 0 {
            return;
        }

        let div_index = (self.target_division as usize).min(4);
        let slice_len = ((self.sample_rate * DIVISION_MULTIPLIERS[div_index]) as usize).max(64);
        let active_gate_len = ((slice_len as f32) * self.target_stutter_gate) as usize;
        let repeat_prob = self.target_repeat_prob;
        let rev_prob = self.target_reverse_prob;
        let dry_wet = self.target_mix;
        let cap = self.buffer_capacity;

        for s in 0..num_samples {
            let dry_l = in_l[s];
            let dry_r = if s < in_r.len() { in_r[s] } else { dry_l };

            self.buffer_l[self.write_pos] = dry_l;
            self.buffer_r[self.write_pos] = dry_r;
            let current_write = self.write_pos;
            self.write_pos = if self.write_pos + 1 < cap { self.write_pos + 1 } else { 0 };

            if self.slice_sample_counter >= slice_len {
                self.slice_sample_counter = 0;
                let r_rep = self.next_random_float();
                if r_rep < repeat_prob {
                    self.is_glitching = true;
                    self.slice_start_pos = if current_write >= slice_len {
                        current_write - slice_len
                    } else {
                        cap + current_write - slice_len
                    };
                    self.is_reverse = self.next_random_float() < rev_prob;
                } else {
                    self.is_glitching = false;
                }
            }

            let mut wet_l = dry_l;
            let mut wet_r = dry_r;

            if self.is_glitching {
                if self.slice_sample_counter < active_gate_len {
                    let read_offset = if self.is_reverse {
                        slice_len - 1 - self.slice_sample_counter
                    } else {
                        self.slice_sample_counter
                    };
                    let mut read_idx = self.slice_start_pos + read_offset;
                    if read_idx >= cap {
                        read_idx %= cap;
                    }

                    let env = if self.slice_sample_counter < FADE_LEN {
                        self.slice_sample_counter as f32 / FADE_LEN as f32
                    } else if self.slice_sample_counter + FADE_LEN >= active_gate_len {
                        ((active_gate_len - self.slice_sample_counter) as f32 / FADE_LEN as f32).max(0.0)
                    } else {
                        1.0
                    };

                    // Crossfade limpio entre señal directa y glitch buffer sin pops de discontinuidad (Reglas 34 y 35)
                    wet_l = self.buffer_l[read_idx] * env + dry_l * (1.0 - env);
                    wet_r = self.buffer_r[read_idx] * env + dry_r * (1.0 - env);
                }
            }

            self.slice_sample_counter += 1;

            out_l[s] = denormal_guard(dry_l * (1.0 - dry_wet) + wet_l * dry_wet);
            if s < out_r.len() {
                out_r[s] = denormal_guard(dry_r * (1.0 - dry_wet) + wet_r * dry_wet);
            }
        }
    }
}
