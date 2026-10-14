//! n8_rust_core - Buffer Audio Slicer & Frozen Granular Playground (DSP #49)
//! Circular buffer recording (3 seconds), rhythmic slicing, slice looping,
//! scrubbing, reverse playback, frozen granular synthesis with 8 concurrent voices,
//! Hann windowing, and biquad lowpass filtering (Rules 5, 8, 9, 14, 34, 35, 46, 47).

use crate::common::{self, BiquadCoeffs, BiquadState};

pub const MAX_GRAINS: usize = 8;
pub const MAX_SLICER_BUFFER_CAPACITY: usize = 576000; // 3 seconds at 192 kHz

pub const PARAM_FREEZE: u32 = 1;
pub const PARAM_SLICE_COUNT: u32 = 2;
pub const PARAM_ACTIVE_SLICE: u32 = 3;
pub const PARAM_PLAYBACK_SPEED: u32 = 4;
pub const PARAM_POSITION_SCRUB: u32 = 5;
pub const PARAM_GRAIN_SIZE_MS: u32 = 6;
pub const PARAM_GRAIN_DENSITY: u32 = 7;
pub const PARAM_JITTER: u32 = 8;
pub const PARAM_PITCH_SHIFT_SEMITONES: u32 = 9;
pub const PARAM_FILTER_CUTOFF: u32 = 10;
pub const PARAM_MIX: u32 = 11;

#[derive(Clone, Copy, Default)]
pub struct SlicerGrainVoice {
    pub active: bool,
    pub buffer_pos: f64,
    pub playback_rate: f64,
    pub current_sample: usize,
    pub total_samples: usize,
}

pub struct RustAudioSlicer {
    sample_rate: f32,
    buffer_capacity: usize,
    ring_buffer_l: Vec<f32>,
    ring_buffer_r: Vec<f32>,
    write_index: usize,
    playhead_pos: f64,
    grain_timer: usize,

    grains: [SlicerGrainVoice; MAX_GRAINS],

    filter_coeffs: BiquadCoeffs,
    filter_l: BiquadState,
    filter_r: BiquadState,

    // Parameters
    target_freeze: f32,
    target_slice_count: f32,
    target_active_slice: f32,
    target_speed: f32,
    target_position: f32,
    target_grain_size: f32,
    target_density: f32,
    target_jitter: f32,
    target_pitch: f32,
    target_cutoff: f32,
    target_mix: f32,
}

impl RustAudioSlicer {
    #[inline(always)]
    fn pseudo_random(mut seed: u32) -> f32 {
        seed = (seed ^ 61) ^ (seed >> 16);
        seed = seed.wrapping_add(seed << 3);
        seed ^= seed >> 4;
        seed = seed.wrapping_mul(0x27d4eb2d);
        seed ^= seed >> 15;
        (seed & 0xffff) as f32 / 65535.0
    }

    pub fn new(sample_rate: f64) -> Self {
        let sr = if sample_rate > 0.0 { sample_rate as f32 } else { 48000.0 };
        let mut capacity = (sr * 3.0) as usize;
        if capacity < 1024 {
            capacity = 1024;
        }
        if capacity > MAX_SLICER_BUFFER_CAPACITY {
            capacity = MAX_SLICER_BUFFER_CAPACITY;
        }

        let filter_coeffs = BiquadCoeffs::lowpass(sr, 18000.0, 0.707);

        Self {
            sample_rate: sr,
            buffer_capacity: capacity,
            ring_buffer_l: vec![0.0f32; MAX_SLICER_BUFFER_CAPACITY],
            ring_buffer_r: vec![0.0f32; MAX_SLICER_BUFFER_CAPACITY],
            write_index: 0,
            playhead_pos: 0.0,
            grain_timer: 0,
            grains: [SlicerGrainVoice::default(); MAX_GRAINS],
            filter_coeffs,
            filter_l: BiquadState::default(),
            filter_r: BiquadState::default(),

            target_freeze: 0.0,
            target_slice_count: 8.0,
            target_active_slice: -1.0,
            target_speed: 1.0,
            target_position: 0.0,
            target_grain_size: 80.0,
            target_density: 4.0,
            target_jitter: 0.15,
            target_pitch: 0.0,
            target_cutoff: 18000.0,
            target_mix: 0.85,
        }
    }

    pub fn prepare(&mut self, sample_rate: f64) {
        let sr = if sample_rate > 0.0 { sample_rate as f32 } else { 48000.0 };
        self.sample_rate = sr;
        let mut capacity = (sr * 3.0) as usize;
        if capacity < 1024 {
            capacity = 1024;
        }
        if capacity > MAX_SLICER_BUFFER_CAPACITY {
            capacity = MAX_SLICER_BUFFER_CAPACITY;
        }
        self.buffer_capacity = capacity;

        self.ring_buffer_l[..self.buffer_capacity].fill(0.0);
        self.ring_buffer_r[..self.buffer_capacity].fill(0.0);
        self.write_index = 0;
        self.playhead_pos = 0.0;
        self.grain_timer = 0;

        for g in &mut self.grains {
            g.active = false;
        }

        self.filter_coeffs = BiquadCoeffs::lowpass(sr, self.target_cutoff.clamp(20.0, 20000.0), 0.707);
        self.filter_l.reset();
        self.filter_r.reset();
    }

    pub fn reset(&mut self) {
        self.ring_buffer_l[..self.buffer_capacity].fill(0.0);
        self.ring_buffer_r[..self.buffer_capacity].fill(0.0);
        self.write_index = 0;
        self.playhead_pos = 0.0;
        self.grain_timer = 0;

        for g in &mut self.grains {
            g.active = false;
        }

        self.filter_l.reset();
        self.filter_r.reset();
    }

    pub fn set_parameter(&mut self, param_id: u32, value: f32) {
        match param_id {
            PARAM_FREEZE => self.target_freeze = value,
            PARAM_SLICE_COUNT => self.target_slice_count = value,
            PARAM_ACTIVE_SLICE => self.target_active_slice = value,
            PARAM_PLAYBACK_SPEED => self.target_speed = value,
            PARAM_POSITION_SCRUB => self.target_position = value,
            PARAM_GRAIN_SIZE_MS => self.target_grain_size = value,
            PARAM_GRAIN_DENSITY => self.target_density = value,
            PARAM_JITTER => self.target_jitter = value,
            PARAM_PITCH_SHIFT_SEMITONES => self.target_pitch = value,
            PARAM_FILTER_CUTOFF => self.target_cutoff = value,
            PARAM_MIX => self.target_mix = value,
            _ => {}
        }
    }

    pub fn get_parameter(&self, param_id: u32) -> f32 {
        match param_id {
            PARAM_FREEZE => self.target_freeze,
            PARAM_SLICE_COUNT => self.target_slice_count,
            PARAM_ACTIVE_SLICE => self.target_active_slice,
            PARAM_PLAYBACK_SPEED => self.target_speed,
            PARAM_POSITION_SCRUB => self.target_position,
            PARAM_GRAIN_SIZE_MS => self.target_grain_size,
            PARAM_GRAIN_DENSITY => self.target_density,
            PARAM_JITTER => self.target_jitter,
            PARAM_PITCH_SHIFT_SEMITONES => self.target_pitch,
            PARAM_FILTER_CUTOFF => self.target_cutoff,
            PARAM_MIX => self.target_mix,
            _ => 0.0,
        }
    }

    pub fn copy_waveform_thumbnail(&self, out_points: &mut [f32]) -> usize {
        let num_points = out_points.len();
        if self.buffer_capacity == 0 || num_points == 0 {
            return 0;
        }
        let step = self.buffer_capacity / num_points;
        if step == 0 {
            return 0;
        }
        for p in 0..num_points {
            let mut peak = 0.0f32;
            let start = p * step;
            let end = (start + step).min(self.buffer_capacity);
            for i in start..end {
                let val = self.ring_buffer_l[i].abs();
                if val > peak {
                    peak = val;
                }
            }
            out_points[p] = peak.clamp(0.0, 1.0);
        }
        num_points
    }

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

        let is_frozen = self.target_freeze > 0.5;
        let mix = self.target_mix.clamp(0.0, 1.0);
        let num_slices = (self.target_slice_count as i32).clamp(2, 32);
        let slice_len = self.buffer_capacity as f32 / num_slices as f32;

        self.filter_coeffs = BiquadCoeffs::lowpass(
            self.sample_rate,
            self.target_cutoff.clamp(20.0, 20000.0),
            0.707,
        );

        let pitch_ratio = 2.0f32.powf(self.target_pitch / 12.0);
        let effective_speed = (self.target_speed * pitch_ratio) as f64;

        let max_half_buf = (self.buffer_capacity as f32 * 0.5).max(128.0);
        let grain_length_samples = (self.target_grain_size * 0.001 * self.sample_rate)
            .clamp(128.0, max_half_buf) as usize;
        let max_active_grains = (self.target_density as usize).clamp(1, MAX_GRAINS);

        let cap = self.buffer_capacity;
        let cap_f64 = cap as f64;

        for s in 0..num_samples {
            let dry_l = in_l[s];
            let dry_r = if s < in_r.len() { in_r[s] } else { dry_l };

            // 1. Escritura en ring buffer si NO está congelado
            if !is_frozen {
                self.ring_buffer_l[self.write_index] = dry_l;
                self.ring_buffer_r[self.write_index] = dry_r;
                self.write_index = (self.write_index + 1) % cap;
            }

            let mut wet_l = 0.0f32;
            let mut wet_r = 0.0f32;

            // 2. Modo Granular Frozen Playground vs Modo Slice Player
            if is_frozen {
                self.grain_timer += 1;
                let spawn_threshold = grain_length_samples / (max_active_grains * 2 + 1);
                if self.grain_timer >= spawn_threshold {
                    self.grain_timer = 0;
                    for g in 0..max_active_grains {
                        if !self.grains[g].active {
                            self.grains[g].active = true;
                            self.grains[g].current_sample = 0;
                            self.grains[g].total_samples = grain_length_samples;
                            self.grains[g].playback_rate = effective_speed;

                            let base_pos = self.target_position as f64 * cap_f64;
                            let jitter_offset = (Self::pseudo_random(s as u32 + g as u32) as f64 - 0.5)
                                * self.target_jitter as f64
                                * (cap_f64 * 0.25);
                            let start_pos = (base_pos + jitter_offset + cap_f64) % cap_f64;
                            self.grains[g].buffer_pos = start_pos;
                            break;
                        }
                    }
                }

                let mut grain_sum_l = 0.0f32;
                let mut grain_sum_r = 0.0f32;
                let mut active_count = 0usize;

                for g in 0..max_active_grains {
                    if !self.grains[g].active {
                        continue;
                    }
                    let gr = &mut self.grains[g];
                    let win_phase = gr.current_sample as f32 / gr.total_samples as f32;
                    let win = 0.5 * (1.0 - (std::f32::consts::PI * 2.0 * win_phase).cos());

                    let idx0 = (gr.buffer_pos as usize) % cap;
                    let idx1 = (idx0 + 1) % cap;
                    let frac = (gr.buffer_pos - gr.buffer_pos.floor()) as f32;

                    let s_l = self.ring_buffer_l[idx0]
                        + frac * (self.ring_buffer_l[idx1] - self.ring_buffer_l[idx0]);
                    let s_r = self.ring_buffer_r[idx0]
                        + frac * (self.ring_buffer_r[idx1] - self.ring_buffer_r[idx0]);

                    grain_sum_l += s_l * win;
                    grain_sum_r += s_r * win;
                    active_count += 1;

                    gr.buffer_pos = (gr.buffer_pos + gr.playback_rate + cap_f64) % cap_f64;
                    gr.current_sample += 1;
                    if gr.current_sample >= gr.total_samples {
                        gr.active = false;
                    }
                }

                if active_count > 0 {
                    let norm_gain = 1.0 / (active_count as f32).sqrt();
                    wet_l = grain_sum_l * norm_gain;
                    wet_r = grain_sum_r * norm_gain;
                }
            } else {
                let slice_idx = self.target_active_slice as i32;
                if slice_idx < 0 {
                    self.playhead_pos = (self.playhead_pos + effective_speed + cap_f64) % cap_f64;
                } else {
                    let slice_start = slice_idx as f64 * slice_len as f64;
                    self.playhead_pos = slice_start
                        + ((self.playhead_pos - slice_start + effective_speed + slice_len as f64)
                            % slice_len as f64);
                }

                let p0 = (self.playhead_pos as usize) % cap;
                let p1 = (p0 + 1) % cap;
                let frac = (self.playhead_pos - self.playhead_pos.floor()) as f32;

                wet_l = self.ring_buffer_l[p0] + frac * (self.ring_buffer_l[p1] - self.ring_buffer_l[p0]);
                wet_r = self.ring_buffer_r[p0] + frac * (self.ring_buffer_r[p1] - self.ring_buffer_r[p0]);

                if slice_idx >= 0 && slice_len > 16.0 {
                    let slice_start = slice_idx as f64 * slice_len as f64;
                    let pos_in_slice = self.playhead_pos - slice_start;
                    let edge_fade = 128.0f64.min(slice_len as f64 * 0.05);
                    let mut edge_env = 1.0f32;
                    if pos_in_slice < edge_fade {
                        let x = (pos_in_slice / edge_fade) as f32;
                        edge_env = 0.5 * (1.0 - (x * std::f32::consts::PI).cos());
                    } else if pos_in_slice > (slice_len as f64 - edge_fade) {
                        let x = ((slice_len as f64 - pos_in_slice) / edge_fade) as f32;
                        edge_env = 0.5 * (1.0 - (x * std::f32::consts::PI).cos());
                    }
                    wet_l *= edge_env;
                    wet_r *= edge_env;
                }
            }

            wet_l = self.filter_l.process(wet_l, &self.filter_coeffs);
            wet_r = self.filter_r.process(wet_r, &self.filter_coeffs);

            out_l[s] = common::denormal_guard(dry_l * (1.0 - mix) + wet_l * mix);
            if s < out_r.len() {
                out_r[s] = common::denormal_guard(dry_r * (1.0 - mix) + wet_r * mix);
            }
        }
    }
}
