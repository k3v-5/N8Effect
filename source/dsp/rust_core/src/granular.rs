//! n8_rust_core - Granular Synthesis & Cloud Processing Engine
//! High-performance real-time granular processor with 128-grain pool,
//! 3 spawning modes (Clock, Stochastic, Spectral/Onset), pitch spray,
//! scrub position, freeze, and stereo pan spray (Rules 5, 8, 9, 10, 11, 18, 35, 46, 47).

use crate::common;

pub const MAX_GRAINS: usize = 128;
pub const MAX_RING_BUFFER_SIZE: usize = 192000;

pub const PARAM_GRAIN_SIZE_MS: u32 = 1;
pub const PARAM_DENSITY: u32 = 2;
pub const PARAM_POSITION_SPRAY_MS: u32 = 3;
pub const PARAM_PITCH_SEMITONES: u32 = 4;
pub const PARAM_PITCH_SPRAY: u32 = 5;
pub const PARAM_PAN_SPRAY: u32 = 6;
pub const PARAM_DRY_WET: u32 = 7;
pub const PARAM_SCRUB_POSITION: u32 = 8;
pub const PARAM_FREEZE: u32 = 9;
pub const PARAM_SPAWN_MODE: u32 = 10;
pub const PARAM_SPECTRAL_SENSITIVITY: u32 = 11;

#[repr(C)]
#[derive(Clone, Copy, Default)]
pub struct RustGrainCloudPoint {
    pub norm_position: f32,
    pub pitch_ratio: f32,
    pub pan: f32,
    pub envelope: f32,
    pub active: bool,
}

#[derive(Clone, Copy)]
pub struct RustGrain {
    pub active: bool,
    pub source_start_pos: f32,
    pub current_playhead: f32,
    pub playback_rate: f32,
    pub duration_samples: i32,
    pub age_samples: i32,
    pub amplitude: f32,
    pub pan_l: f32,
    pub pan_r: f32,
}

impl RustGrain {
    pub const fn new() -> Self {
        Self {
            active: false,
            source_start_pos: 0.0,
            current_playhead: 0.0,
            playback_rate: 1.0,
            duration_samples: 0,
            age_samples: 0,
            amplitude: 1.0,
            pan_l: 0.7071,
            pan_r: 0.7071,
        }
    }

    #[inline(always)]
    pub fn reset(&mut self) {
        self.active = false;
        self.source_start_pos = 0.0;
        self.current_playhead = 0.0;
        self.playback_rate = 1.0;
        self.duration_samples = 0;
        self.age_samples = 0;
        self.amplitude = 1.0;
        self.pan_l = 0.7071;
        self.pan_r = 0.7071;
    }

    #[inline(always)]
    pub fn get_window_envelope(&self) -> f32 {
        if self.duration_samples <= 0 {
            return 0.0;
        }
        let norm = self.age_samples as f32 / self.duration_samples as f32;
        0.5 * (1.0 - (std::f32::consts::PI * 2.0 * norm).cos())
    }
}

pub struct RustGranular {
    sample_rate: f32,
    buffer_capacity: usize,
    ring_buffer_l: Vec<f32>,
    ring_buffer_r: Vec<f32>,
    write_pos: usize,
    total_samples_recorded: usize,
    samples_until_next_grain: f32,
    rng_state: u32,
    prev_sample_l: f32,
    prev_sample_r: f32,

    // Parameters
    target_grain_size_ms: f32,
    target_density: f32,
    target_position_spray_ms: f32,
    target_pitch_semitones: f32,
    target_pitch_spray: f32,
    target_pan_spray: f32,
    target_mix: f32,
    target_scrub_pos: f32,
    target_freeze: f32,
    target_spawn_mode: f32,
    target_spectral_sensitivity: f32,

    // Preallocated grain pool
    grains: [RustGrain; MAX_GRAINS],
    free_indices: [usize; MAX_GRAINS],
    free_count: usize,
    active_count: usize,
}

impl RustGranular {
    pub fn new(sample_rate: f64) -> Self {
        let sr = if sample_rate > 0.0 { sample_rate as f32 } else { 44100.0 };
        let mut capacity = (sr * 2.0) as usize;
        if capacity > MAX_RING_BUFFER_SIZE {
            capacity = MAX_RING_BUFFER_SIZE;
        }
        if capacity == 0 {
            capacity = 88200;
        }

        let mut free_indices = [0usize; MAX_GRAINS];
        for i in 0..MAX_GRAINS {
            free_indices[i] = i;
        }

        let mut instance = Self {
            sample_rate: sr,
            buffer_capacity: capacity,
            ring_buffer_l: vec![0.0f32; MAX_RING_BUFFER_SIZE],
            ring_buffer_r: vec![0.0f32; MAX_RING_BUFFER_SIZE],
            write_pos: 0,
            total_samples_recorded: 0,
            samples_until_next_grain: 0.0,
            rng_state: 123456789,
            prev_sample_l: 0.0,
            prev_sample_r: 0.0,

            target_grain_size_ms: 80.0,
            target_density: 15.0,
            target_position_spray_ms: 20.0,
            target_pitch_semitones: 0.0,
            target_pitch_spray: 0.0,
            target_pan_spray: 0.5,
            target_mix: 0.5,
            target_scrub_pos: 0.0,
            target_freeze: 0.0,
            target_spawn_mode: 0.0,
            target_spectral_sensitivity: 0.5,

            grains: [RustGrain::new(); MAX_GRAINS],
            free_indices,
            free_count: MAX_GRAINS,
            active_count: 0,
        };

        instance.reset_pool();
        instance
    }

    pub fn prepare(&mut self, sample_rate: f64) {
        let sr = if sample_rate > 0.0 { sample_rate as f32 } else { 44100.0 };
        self.sample_rate = sr;
        let mut capacity = (sr * 2.0) as usize;
        if capacity > MAX_RING_BUFFER_SIZE {
            capacity = MAX_RING_BUFFER_SIZE;
        }
        if capacity == 0 {
            capacity = 88200;
        }
        self.buffer_capacity = capacity;

        self.reset_pool();
        self.ring_buffer_l[..self.buffer_capacity].fill(0.0);
        self.ring_buffer_r[..self.buffer_capacity].fill(0.0);
        self.write_pos = 0;
        self.samples_until_next_grain = 0.0;
        self.total_samples_recorded = 0;
        self.prev_sample_l = 0.0;
        self.prev_sample_r = 0.0;
    }

    pub fn reset(&mut self) {
        self.reset_pool();
        self.write_pos = 0;
        self.samples_until_next_grain = 0.0;
        self.total_samples_recorded = 0;
        self.prev_sample_l = 0.0;
        self.prev_sample_r = 0.0;
        self.ring_buffer_l[..self.buffer_capacity].fill(0.0);
        self.ring_buffer_r[..self.buffer_capacity].fill(0.0);
    }

    #[inline]
    fn reset_pool(&mut self) {
        self.free_count = MAX_GRAINS;
        self.active_count = 0;
        for i in 0..MAX_GRAINS {
            self.grains[i].reset();
            self.free_indices[i] = i;
        }
    }

    #[inline(always)]
    fn acquire_grain(&mut self) -> Option<usize> {
        if self.free_count == 0 {
            return None;
        }
        self.free_count -= 1;
        let idx = self.free_indices[self.free_count];
        self.active_count += 1;
        self.grains[idx].reset();
        self.grains[idx].active = true;
        Some(idx)
    }

    #[inline(always)]
    fn release_grain(&mut self, idx: usize) {
        if idx >= MAX_GRAINS || !self.grains[idx].active {
            return;
        }
        self.grains[idx].active = false;
        if self.free_count < MAX_GRAINS {
            self.free_indices[self.free_count] = idx;
            self.free_count += 1;
        }
        if self.active_count > 0 {
            self.active_count -= 1;
        }
    }

    #[inline(always)]
    fn next_random_float(&mut self) -> f32 {
        self.rng_state = self.rng_state.wrapping_mul(1664525).wrapping_add(1013904223);
        (self.rng_state & 0x00FFFFFF) as f32 / 16777216.0
    }

    pub fn set_parameter(&mut self, param_id: u32, value: f32) {
        match param_id {
            PARAM_GRAIN_SIZE_MS => self.target_grain_size_ms = value.clamp(10.0, 500.0),
            PARAM_DENSITY => self.target_density = value.clamp(1.0, 60.0),
            PARAM_POSITION_SPRAY_MS => self.target_position_spray_ms = value.clamp(0.0, 200.0),
            PARAM_PITCH_SEMITONES => self.target_pitch_semitones = value.clamp(-24.0, 24.0),
            PARAM_PITCH_SPRAY => self.target_pitch_spray = value.clamp(0.0, 12.0),
            PARAM_PAN_SPRAY => self.target_pan_spray = value.clamp(0.0, 1.0),
            PARAM_DRY_WET => self.target_mix = value.clamp(0.0, 1.0),
            PARAM_SCRUB_POSITION => self.target_scrub_pos = value.clamp(0.0, 1.0),
            PARAM_FREEZE => self.target_freeze = if value > 0.5 { 1.0 } else { 0.0 },
            PARAM_SPAWN_MODE => self.target_spawn_mode = value.clamp(0.0, 2.0),
            PARAM_SPECTRAL_SENSITIVITY => self.target_spectral_sensitivity = value.clamp(0.0, 1.0),
            _ => {}
        }
    }

    pub fn get_parameter(&self, param_id: u32) -> f32 {
        match param_id {
            PARAM_GRAIN_SIZE_MS => self.target_grain_size_ms,
            PARAM_DENSITY => self.target_density,
            PARAM_POSITION_SPRAY_MS => self.target_position_spray_ms,
            PARAM_PITCH_SEMITONES => self.target_pitch_semitones,
            PARAM_PITCH_SPRAY => self.target_pitch_spray,
            PARAM_PAN_SPRAY => self.target_pan_spray,
            PARAM_DRY_WET => self.target_mix,
            PARAM_SCRUB_POSITION => self.target_scrub_pos,
            PARAM_FREEZE => self.target_freeze,
            PARAM_SPAWN_MODE => self.target_spawn_mode,
            PARAM_SPECTRAL_SENSITIVITY => self.target_spectral_sensitivity,
            _ => 0.0,
        }
    }

    pub fn copy_waveform_overview(&self, out_min_max: &mut [f32], num_pairs: usize) -> usize {
        if self.buffer_capacity == 0 || out_min_max.len() < num_pairs * 2 {
            return 0;
        }
        let step = (self.buffer_capacity / num_pairs).max(1);
        for p in 0..num_pairs {
            let mut min_val = 0.0f32;
            let mut max_val = 0.0f32;
            let start_idx = p * step;
            let end_idx = (start_idx + step).min(self.buffer_capacity);
            for i in start_idx..end_idx {
                let s = 0.5 * (self.ring_buffer_l[i] + self.ring_buffer_r[i]);
                if s < min_val {
                    min_val = s;
                }
                if s > max_val {
                    max_val = s;
                }
            }
            out_min_max[p * 2] = min_val;
            out_min_max[p * 2 + 1] = max_val;
        }
        num_pairs
    }

    pub fn copy_active_grains_snapshot(&self, out_points: &mut [RustGrainCloudPoint]) -> usize {
        if out_points.is_empty() || self.buffer_capacity == 0 {
            return 0;
        }
        let cap_f = self.buffer_capacity as f32;
        let mut written = 0;
        for g in 0..MAX_GRAINS {
            if written >= out_points.len() {
                break;
            }
            let grain = &self.grains[g];
            if !grain.active {
                continue;
            }
            let pt = &mut out_points[written];
            pt.norm_position = (grain.current_playhead / cap_f).clamp(0.0, 1.0);
            pt.pitch_ratio = grain.playback_rate;
            pt.pan = (grain.pan_r.atan2(grain.pan_l) / std::f32::consts::FRAC_PI_2).clamp(0.0, 1.0);
            pt.envelope = grain.get_window_envelope();
            pt.active = true;
            written += 1;
        }
        written
    }

    pub fn get_write_position_normalized(&self) -> f32 {
        if self.buffer_capacity > 0 {
            self.write_pos as f32 / self.buffer_capacity as f32
        } else {
            0.0
        }
    }

    pub fn process_block(
        &mut self,
        in_l: &[f32],
        in_r: &[f32],
        out_l: &mut [f32],
        out_r: &mut [f32],
    ) {
        let num_samples = in_l.len().min(out_l.len());
        if self.buffer_capacity == 0 || num_samples == 0 {
            return;
        }

        let grain_size_ms = self.target_grain_size_ms;
        let density = self.target_density.clamp(1.0, 60.0);
        let pos_spray_ms = self.target_position_spray_ms.max(0.0);
        let pitch_semi = self.target_pitch_semitones;
        let pitch_spray = self.target_pitch_spray.max(0.0);
        let pan_spray = self.target_pan_spray.clamp(0.0, 1.0);
        let dry_wet = self.target_mix.clamp(0.0, 1.0);

        let spawn_interval_samples = self.sample_rate / density;
        let base_grain_duration = 16i32.max((grain_size_ms * 0.001 * self.sample_rate) as i32);
        let mode = (self.target_spawn_mode + 0.5) as i32;

        let cap = self.buffer_capacity;
        let cap_f = cap as f32;

        for s in 0..num_samples {
            let dry_sample_l = in_l[s];
            let dry_sample_r = if s < in_r.len() { in_r[s] } else { dry_sample_l };

            if self.target_freeze <= 0.5 {
                self.ring_buffer_l[self.write_pos] = dry_sample_l;
                self.ring_buffer_r[self.write_pos] = dry_sample_r;
                self.write_pos = if self.write_pos + 1 < cap {
                    self.write_pos + 1
                } else {
                    0
                };
                self.total_samples_recorded = self.total_samples_recorded.saturating_add(1);
            }
            let current_write = self.write_pos;

            // Detección de novedad / flujo espectral para SpawnMode == 2 (Spectral / Onset)
            let diff_l = dry_sample_l - self.prev_sample_l;
            let diff_r = dry_sample_r - self.prev_sample_r;
            self.prev_sample_l = dry_sample_l;
            self.prev_sample_r = dry_sample_r;
            let sample_flux = (0.5 * (diff_l * diff_l + diff_r * diff_r)).sqrt();

            let mut should_spawn = false;

            if mode == 1 {
                // Modo 1: Estocástico (Poisson)
                let prob = density / self.sample_rate;
                should_spawn = self.next_random_float() < prob;
            } else if mode == 2 {
                // Modo 2: Espectral / Onset
                self.samples_until_next_grain -= 1.0;
                let flux_threshold = (1.05 - self.target_spectral_sensitivity) * 0.12;
                if sample_flux > flux_threshold && self.samples_until_next_grain <= 0.0 {
                    should_spawn = true;
                    self.samples_until_next_grain = 64.0f32.max(spawn_interval_samples * 0.25);
                } else if self.samples_until_next_grain <= -spawn_interval_samples * 2.0 {
                    should_spawn = true;
                    self.samples_until_next_grain = spawn_interval_samples;
                }
            } else {
                // Modo 0: Clock isócrono regular
                self.samples_until_next_grain -= 1.0;
                if self.samples_until_next_grain <= 0.0 {
                    should_spawn = true;
                    self.samples_until_next_grain += spawn_interval_samples;
                }
            }

            if should_spawn {
                if let Some(grain_idx) = self.acquire_grain() {
                    let r_pos = self.next_random_float();
                    let r_pitch = self.next_random_float() * 2.0 - 1.0;
                    let r_pan = self.next_random_float() * 2.0 - 1.0;

                    let offset_samples = pos_spray_ms * 0.001 * self.sample_rate * r_pos;
                    let mut start_pos: f32;

                    if self.target_scrub_pos > 0.0001 {
                        let center_pos = self.target_scrub_pos * cap_f;
                        start_pos = center_pos - offset_samples;
                        while start_pos < 0.0 {
                            start_pos += cap_f;
                        }
                        while start_pos >= cap_f {
                            start_pos -= cap_f;
                        }
                    } else if self.total_samples_recorded < cap {
                        let max_back = current_write as f32;
                        start_pos = (max_back - offset_samples - base_grain_duration as f32 * 0.5).max(0.0);
                    } else {
                        start_pos = current_write as f32 - offset_samples - base_grain_duration as f32;
                        while start_pos < 0.0 {
                            start_pos += cap_f;
                        }
                    }

                    let effective_pitch_spray = if mode == 2 {
                        pitch_spray * (1.0 + sample_flux * 2.0)
                    } else {
                        pitch_spray
                    };
                    let final_pitch_semitones = pitch_semi + (effective_pitch_spray * r_pitch);
                    let rate = 2.0f32.powf(final_pitch_semitones / 12.0);

                    let pan = (0.5 + 0.5 * (r_pan * pan_spray)).clamp(0.0, 1.0);

                    let grain = &mut self.grains[grain_idx];
                    grain.source_start_pos = start_pos;
                    grain.current_playhead = start_pos;
                    grain.playback_rate = rate.clamp(0.25, 4.0);
                    grain.duration_samples = base_grain_duration;
                    grain.age_samples = 0;
                    grain.pan_l = (pan * std::f32::consts::FRAC_PI_2).cos();
                    grain.pan_r = (pan * std::f32::consts::FRAC_PI_2).sin();
                    grain.amplitude = 1.0;
                }
            }

            let mut grain_sum_l = 0.0f32;
            let mut grain_sum_r = 0.0f32;

            for g in 0..MAX_GRAINS {
                if !self.grains[g].active {
                    continue;
                }
                let env = self.grains[g].get_window_envelope();

                let ph = self.grains[g].current_playhead;
                let idx0 = (ph as usize) % cap;
                let idx1 = if idx0 + 1 < cap { idx0 + 1 } else { 0 };
                let frac = ph - ph.floor();

                let g_samp_l = self.ring_buffer_l[idx0]
                    + frac * (self.ring_buffer_l[idx1] - self.ring_buffer_l[idx0]);
                let g_samp_r = self.ring_buffer_r[idx0]
                    + frac * (self.ring_buffer_r[idx1] - self.ring_buffer_r[idx0]);

                let win_samp_l = g_samp_l * env;
                let win_samp_r = g_samp_r * env;

                grain_sum_l += win_samp_l * self.grains[g].pan_l;
                grain_sum_r += win_samp_r * self.grains[g].pan_r;

                self.grains[g].current_playhead += self.grains[g].playback_rate;
                if self.grains[g].current_playhead >= cap_f {
                    self.grains[g].current_playhead -= cap_f;
                }
                self.grains[g].age_samples += 1;

                if self.grains[g].age_samples >= self.grains[g].duration_samples {
                    self.release_grain(g);
                }
            }

            const GRAIN_GAIN: f32 = 0.707;
            let wet_l = grain_sum_l * GRAIN_GAIN;
            let wet_r = grain_sum_r * GRAIN_GAIN;

            out_l[s] = common::denormal_guard(dry_sample_l * (1.0 - dry_wet) + wet_l * dry_wet);
            if s < out_r.len() {
                out_r[s] = common::denormal_guard(dry_sample_r * (1.0 - dry_wet) + wet_r * dry_wet);
            }
        }
    }
}
