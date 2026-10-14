//! High-Performance Sample Player DSP Kernel (DSP #53)
//! Rules 5, 8, 9, 14, 34, 46, 47: Zero allocations in audio processing thread,
//! double-buffered atomic sample exchange, sub-sample linear interpolation,
//! multi-mode looping / one-shot playback, attack/release envelope, anti-click smoothing,
//! 64-point normalized thumbnail extraction, and denormal protection.

use std::f32::consts::PI;
use std::sync::atomic::{AtomicUsize, Ordering};
use crate::common::denormal_guard;

struct SampleSlot {
    sample_l: Box<[f32]>,
    sample_r: Box<[f32]>,
    num_samples: usize,
    sample_rate: f64,
}

impl SampleSlot {
    fn empty() -> Self {
        Self {
            sample_l: vec![].into_boxed_slice(),
            sample_r: vec![].into_boxed_slice(),
            num_samples: 0,
            sample_rate: 48000.0,
        }
    }
}

pub struct RustSamplePlayer {
    slots: [SampleSlot; 2],
    active_slot_index: AtomicUsize,
    playhead_pos: f64,
    is_playing: bool,
    env_level: f32,
    prev_trigger: f32,
    sample_rate: f64,

    thumbnail: [f32; 64],

    target_pitch: f32,
    target_root_note: f32,
    target_speed: f32,
    target_loop: f32,
    target_attack: f32,
    target_release: f32,
    target_gain: f32,
    target_mix: f32,
}

impl RustSamplePlayer {
    pub fn new() -> Self {
        let mut player = Self {
            slots: [SampleSlot::empty(), SampleSlot::empty()],
            active_slot_index: AtomicUsize::new(0),
            playhead_pos: 0.0,
            is_playing: true,
            env_level: 0.0,
            prev_trigger: 0.0,
            sample_rate: 48000.0,
            thumbnail: [0.0; 64],
            target_pitch: 0.0,
            target_root_note: 60.0,
            target_speed: 1.0,
            target_loop: 1.0,
            target_attack: 5.0,
            target_release: 50.0,
            target_gain: 1.0,
            target_mix: 1.0,
        };
        player.generate_default_sample(48000.0);
        player
    }

    pub fn prepare(&mut self, sample_rate: f64) {
        self.sample_rate = if sample_rate > 0.0 { sample_rate } else { 48000.0 };
        self.reset();

        let active_idx = self.active_slot_index.load(Ordering::Relaxed);
        if self.slots[active_idx].num_samples == 0 {
            self.generate_default_sample(self.sample_rate);
        }
    }

    pub fn reset(&mut self) {
        self.playhead_pos = 0.0;
        self.is_playing = true;
        self.env_level = 0.0;
        self.prev_trigger = 0.0;
    }

    pub fn set_parameter(&mut self, id: u32, value: f32) {
        match id {
            1 => self.target_pitch = value.clamp(-24.0, 24.0),
            2 => self.target_root_note = value.clamp(0.0, 127.0),
            3 => self.target_speed = value.clamp(0.1, 4.0),
            4 => self.target_loop = value.clamp(0.0, 1.0),
            5 => self.target_attack = value.clamp(1.0, 1000.0),
            6 => self.target_release = value.clamp(5.0, 2000.0),
            7 => self.target_gain = value.clamp(0.0, 2.0),
            8 => self.target_mix = value.clamp(0.0, 1.0),
            _ => {}
        }
    }

    pub fn get_parameter(&self, id: u32) -> f32 {
        match id {
            1 => self.target_pitch,
            2 => self.target_root_note,
            3 => self.target_speed,
            4 => self.target_loop,
            5 => self.target_attack,
            6 => self.target_release,
            7 => self.target_gain,
            8 => self.target_mix,
            _ => 0.0,
        }
    }

    pub fn trigger_playback(&mut self) {
        self.playhead_pos = 0.0;
        self.is_playing = true;
        self.env_level = 0.0;
    }

    pub fn get_thumbnail(&self, out: &mut [f32; 64]) {
        out.copy_from_slice(&self.thumbnail);
    }

    pub fn load_sample(&mut self, l_data: &[f32], r_data: Option<&[f32]>, source_sample_rate: f64) {
        if l_data.is_empty() {
            return;
        }

        let num_samples = l_data.len();
        let active_idx = self.active_slot_index.load(Ordering::Relaxed);
        let inactive_idx = 1 - active_idx;

        let mut sample_l = vec![0.0f32; num_samples].into_boxed_slice();
        sample_l.copy_from_slice(l_data);

        let mut sample_r = vec![0.0f32; num_samples].into_boxed_slice();
        if let Some(r) = r_data {
            sample_r.copy_from_slice(r);
        } else {
            sample_r.copy_from_slice(l_data);
        }

        self.update_thumbnail(&sample_l, &sample_r);

        self.slots[inactive_idx] = SampleSlot {
            sample_l,
            sample_r,
            num_samples,
            sample_rate: if source_sample_rate > 0.0 { source_sample_rate } else { 44100.0 },
        };

        self.active_slot_index.store(inactive_idx, Ordering::Release);
        self.playhead_pos = 0.0;
        self.is_playing = true;
    }

    fn generate_default_sample(&mut self, sample_rate: f64) {
        let total_samples = (sample_rate * 1.5) as usize;
        let mut def_l = vec![0.0f32; total_samples].into_boxed_slice();
        let mut def_r = vec![0.0f32; total_samples].into_boxed_slice();

        let fundamental = 261.63f32; // C4
        let omega = 2.0 * PI * fundamental / (sample_rate as f32);

        for n in 0..total_samples {
            let t = n as f32;
            let decay1 = (-3.5 * t / (sample_rate as f32)).exp();
            let decay2 = (-6.0 * t / (sample_rate as f32)).exp();
            let decay3 = (-9.0 * t / (sample_rate as f32)).exp();

            let s1 = (omega * t).sin() * decay1;
            let s2 = (omega * 2.0 * t).sin() * 0.4 * decay2;
            let s3 = (omega * 3.0 * t).sin() * 0.15 * decay3;

            def_l[n] = (s1 + s2 + s3) * 0.8;
            def_r[n] = (s1 * 0.95 + s2 * 1.05 + s3) * 0.8;
        }

        self.update_thumbnail(&def_l, &def_r);

        self.slots[0] = SampleSlot {
            sample_l: def_l,
            sample_r: def_r,
            num_samples: total_samples,
            sample_rate,
        };
        self.active_slot_index.store(0, Ordering::Release);
    }

    fn update_thumbnail(&mut self, l_data: &[f32], r_data: &[f32]) {
        let num_samples = l_data.len();
        if num_samples == 0 {
            self.thumbnail.fill(0.0);
            return;
        }

        let mut global_max = 0.0f32;
        for pt in 0..64 {
            let start = pt * num_samples / 64;
            let mut end = (pt + 1) * num_samples / 64;
            if end <= start { end = start + 1; }
            if end > num_samples { end = num_samples; }

            let mut peak = 0.0f32;
            for i in start..end {
                let val_l = l_data[i].abs();
                let val_r = r_data[i].abs();
                let val = val_l.max(val_r);
                if val > peak { peak = val; }
            }
            self.thumbnail[pt] = peak;
            if peak > global_max { global_max = peak; }
        }

        if global_max > 1e-5 {
            for v in &mut self.thumbnail {
                *v /= global_max;
            }
        }
    }

    #[inline]
    pub fn process(
        &mut self,
        in_l: &[f32],
        in_r: &[f32],
        trigger_sig: Option<&[f32]>,
        out_l: &mut [f32],
        out_r: &mut [f32],
    ) {
        let num_samples = in_l.len();
        let active_idx = self.active_slot_index.load(Ordering::Acquire);
        let slot = &self.slots[active_idx];

        if slot.num_samples == 0 || (!self.is_playing && self.target_loop < 0.5) {
            for s in 0..num_samples {
                out_l[s] = denormal_guard(in_l[s]);
                out_r[s] = denormal_guard(in_r[s]);
            }
            return;
        }

        let host_sr = self.sample_rate;
        let pitch_factor = 2.0f32.powf(self.target_pitch / 12.0);
        let rate_ratio = (slot.sample_rate / host_sr) * (self.target_speed as f64) * (pitch_factor as f64);

        let mix = self.target_mix.clamp(0.0, 1.0);
        let gain = self.target_gain.clamp(0.0, 2.0);
        let is_loop = self.target_loop > 0.5;
        let total_samples = slot.num_samples;

        let attack_samples = (self.target_attack * 0.001 * (host_sr as f32)).max(1.0);
        let release_samples = (self.target_release * 0.001 * (host_sr as f32)).max(1.0);
        let attack_step = 1.0 / attack_samples;

        for s in 0..num_samples {
            if let Some(trig) = trigger_sig {
                let trig_val = trig[s];
                if trig_val > 0.5 && self.prev_trigger <= 0.5 {
                    self.playhead_pos = 0.0;
                    self.is_playing = true;
                    self.env_level = 0.0;
                }
                self.prev_trigger = trig_val;
            }

            if !self.is_playing {
                out_l[s] = denormal_guard(in_l[s]);
                out_r[s] = denormal_guard(in_r[s]);
                continue;
            }

            let idx0 = self.playhead_pos as usize;
            let idx1 = if idx0 + 1 < total_samples {
                idx0 + 1
            } else if is_loop {
                0
            } else {
                idx0
            };
            let frac = (self.playhead_pos - (idx0 as f64)) as f32;

            let s0_l = slot.sample_l[idx0];
            let s1_l = slot.sample_l[idx1];
            let sample_l = s0_l + frac * (s1_l - s0_l);

            let s0_r = slot.sample_r[idx0];
            let s1_r = slot.sample_r[idx1];
            let sample_r = s0_r + frac * (s1_r - s0_r);

            if self.env_level < 1.0 {
                self.env_level = (self.env_level + attack_step).min(1.0);
            }

            let mut final_env = self.env_level;
            if !is_loop {
                let remaining = (total_samples as f64) - self.playhead_pos;
                if remaining < (release_samples as f64) {
                    final_env *= (remaining / (release_samples as f64)) as f32;
                }
            } else {
                let loop_fade = (128.0f64).min((total_samples as f64) * 0.02);
                if loop_fade > 1.0 {
                    if self.playhead_pos < loop_fade {
                        let x = (self.playhead_pos / loop_fade) as f32;
                        final_env *= 0.5 * (1.0 - (x * PI).cos());
                    } else if self.playhead_pos > ((total_samples as f64) - loop_fade) {
                        let dist_to_end = (total_samples as f64) - self.playhead_pos;
                        let x = (dist_to_end / loop_fade) as f32;
                        final_env *= 0.5 * (1.0 - (x * PI).cos());
                    }
                }
            }

            let wet_l = sample_l * final_env * gain;
            let wet_r = sample_r * final_env * gain;
            let dry_l = in_l[s];
            let dry_r = in_r[s];

            out_l[s] = denormal_guard(dry_l * (1.0 - mix) + wet_l * mix);
            out_r[s] = denormal_guard(dry_r * (1.0 - mix) + wet_r * mix);

            self.playhead_pos += rate_ratio;
            if self.playhead_pos >= (total_samples as f64) {
                if is_loop {
                    self.playhead_pos %= total_samples as f64;
                } else {
                    self.is_playing = false;
                    self.playhead_pos = 0.0;
                }
            }
        }
    }
}
