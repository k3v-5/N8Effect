use crate::common::{self, DelayBuffer};

pub struct MidiScaleQuantizerCore {
    sample_rate: f32,
    delay_line_l: DelayBuffer,
    delay_line_r: DelayBuffer,
    active_scale_mask: [bool; 12],
    phase: f32,
    smoothed_shift: f32,
    target_root_key: f32,
    target_scale: f32,
    target_transpose: f32,
    target_snap_speed: f32,
    target_mix: f32,
}

impl MidiScaleQuantizerCore {
    pub fn new() -> Self {
        let max_delay = 4410; // 100 ms at 44.1 kHz default
        let mut core = Self {
            sample_rate: 44100.0,
            delay_line_l: DelayBuffer::new(max_delay),
            delay_line_r: DelayBuffer::new(max_delay),
            active_scale_mask: [false; 12],
            phase: 0.0,
            smoothed_shift: 1.0,
            target_root_key: 0.0,
            target_scale: 1.0,
            target_transpose: 0.0,
            target_snap_speed: 20.0,
            target_mix: 1.0,
        };
        core.update_scale_mask();
        core
    }

    pub fn prepare(&mut self, sample_rate: f32) {
        self.sample_rate = sample_rate.max(1.0);
        let max_delay = (self.sample_rate * 0.1) as usize; // 100 ms
        self.delay_line_l = DelayBuffer::new(max_delay);
        self.delay_line_r = DelayBuffer::new(max_delay);
        self.phase = 0.0;
        self.smoothed_shift = 1.0;
        self.update_scale_mask();
    }

    pub fn reset(&mut self) {
        self.delay_line_l.reset();
        self.delay_line_r.reset();
        self.phase = 0.0;
        self.smoothed_shift = 1.0;
    }

    pub fn set_parameter(&mut self, param_id: u32, value: f32) {
        match param_id {
            1 => self.target_root_key = value.clamp(0.0, 11.0),
            2 => self.target_scale = value.clamp(0.0, 10.0),
            3 => self.target_transpose = value.clamp(-24.0, 24.0),
            4 => self.target_snap_speed = value.clamp(5.0, 100.0),
            5 => self.target_mix = value.clamp(0.0, 1.0),
            _ => {}
        }
        self.update_scale_mask();
    }

    pub fn get_parameter(&self, param_id: u32) -> f32 {
        match param_id {
            1 => self.target_root_key,
            2 => self.target_scale,
            3 => self.target_transpose,
            4 => self.target_snap_speed,
            5 => self.target_mix,
            _ => 0.0,
        }
    }

    pub fn set_parameters(
        &mut self,
        root_key: f32,
        scale: f32,
        transpose: f32,
        snap_speed: f32,
        mix: f32,
    ) {
        self.target_root_key = root_key.clamp(0.0, 11.0);
        self.target_scale = scale.clamp(0.0, 10.0);
        self.target_transpose = transpose.clamp(-24.0, 24.0);
        self.target_snap_speed = snap_speed.clamp(5.0, 100.0);
        self.target_mix = mix.clamp(0.0, 1.0);
        self.update_scale_mask();
    }

    pub fn quantize_note(&self, midi_note: i32) -> i32 {
        let transposed = midi_note + self.target_transpose.round() as i32;
        let root = self.target_root_key.round() as i32;

        let mut best_note = transposed;
        let mut min_distance = 999;

        for note in (transposed - 6)..=(transposed + 6) {
            let pitch_class = (((note - root) % 12 + 12) % 12) as usize;
            if self.active_scale_mask[pitch_class] {
                let dist = (note - transposed).abs();
                if dist < min_distance {
                    min_distance = dist;
                    best_note = note;
                }
            }
        }

        best_note
    }

    pub fn update_scale_mask(&mut self) {
        self.active_scale_mask.fill(false);
        let s = self.target_scale.round() as i32;
        match s {
            0 => { // Major [0, 2, 4, 5, 7, 9, 11]
                for &i in &[0, 2, 4, 5, 7, 9, 11] { self.active_scale_mask[i] = true; }
            }
            1 => { // Natural Minor [0, 2, 3, 5, 7, 8, 10]
                for &i in &[0, 2, 3, 5, 7, 8, 10] { self.active_scale_mask[i] = true; }
            }
            2 => { // Harmonic Minor [0, 2, 3, 5, 7, 8, 11]
                for &i in &[0, 2, 3, 5, 7, 8, 11] { self.active_scale_mask[i] = true; }
            }
            3 => { // Dorian [0, 2, 3, 5, 7, 9, 10]
                for &i in &[0, 2, 3, 5, 7, 9, 10] { self.active_scale_mask[i] = true; }
            }
            4 => { // Phrygian [0, 1, 3, 5, 7, 8, 10]
                for &i in &[0, 1, 3, 5, 7, 8, 10] { self.active_scale_mask[i] = true; }
            }
            5 => { // Lydian [0, 2, 4, 6, 7, 9, 11]
                for &i in &[0, 2, 4, 6, 7, 9, 11] { self.active_scale_mask[i] = true; }
            }
            6 => { // Mixolydian [0, 2, 4, 5, 7, 9, 10]
                for &i in &[0, 2, 4, 5, 7, 9, 10] { self.active_scale_mask[i] = true; }
            }
            7 => { // Pentatonic Major [0, 2, 4, 7, 9]
                for &i in &[0, 2, 4, 7, 9] { self.active_scale_mask[i] = true; }
            }
            8 => { // Pentatonic Minor [0, 3, 5, 7, 10]
                for &i in &[0, 3, 5, 7, 10] { self.active_scale_mask[i] = true; }
            }
            9 => { // Blues [0, 3, 5, 6, 7, 10]
                for &i in &[0, 3, 5, 6, 7, 10] { self.active_scale_mask[i] = true; }
            }
            10 => { // Japanese Hirajoshi [0, 2, 3, 7, 8]
                for &i in &[0, 2, 3, 7, 8] { self.active_scale_mask[i] = true; }
            }
            _ => {
                for &i in &[0, 2, 4, 5, 7, 9, 11] { self.active_scale_mask[i] = true; }
            }
        }
    }

    pub fn process(
        &mut self,
        in_l: &[f32],
        in_r: &[f32],
        out_l: &mut [f32],
        mut out_r: Option<&mut [f32]>,
        num_samples: usize,
    ) {
        let grain_size = self.sample_rate * 0.04; // 40 ms grain window
        let inv_grain = 1.0 / grain_size;

        let semitone_shift = self.target_transpose.round() as i32;
        let target_shift_ratio = 2.0f32.powf(semitone_shift as f32 / 12.0);
        let rate = 1.0 - target_shift_ratio;

        let alpha = 1.0 / (self.target_snap_speed * 0.001 * self.sample_rate);
        let mix = self.target_mix;

        const CHUNK: usize = 1024;
        let mut scratch_l = [0.0f32; CHUNK];
        let mut scratch_r = [0.0f32; CHUNK];

        let mut offset = 0;
        let mut phase = self.phase;
        let mut smoothed_shift = self.smoothed_shift;

        while offset < num_samples {
            let chunk_len = (num_samples - offset).min(CHUNK);

            for i in 0..chunk_len {
                let raw_l = in_l[offset + i];
                let raw_r = in_r[offset + i];

                self.delay_line_l.write(raw_l);
                self.delay_line_r.write(raw_r);

                smoothed_shift += alpha * (rate - smoothed_shift);
                smoothed_shift = common::denormal_guard(smoothed_shift);

                phase += smoothed_shift;
                if phase >= grain_size {
                    phase -= grain_size;
                }
                if phase < 0.0 {
                    phase += grain_size;
                }

                let phase2 = (phase + grain_size * 0.5) % grain_size;

                let w1 = 1.0 - (2.0 * phase * inv_grain - 1.0).abs();
                let w2 = 1.0 - (2.0 * phase2 * inv_grain - 1.0).abs();

                let s1_l = self.delay_line_l.read_cubic(phase);
                let s2_l = self.delay_line_l.read_cubic(phase2);
                let shifted_l = s1_l * w1 + s2_l * w2;

                let s1_r = self.delay_line_r.read_cubic(phase);
                let s2_r = self.delay_line_r.read_cubic(phase2);
                let shifted_r = s1_r * w1 + s2_r * w2;

                scratch_l[i] = common::denormal_guard(raw_l * (1.0 - mix) + shifted_l * mix);
                scratch_r[i] = common::denormal_guard(raw_r * (1.0 - mix) + shifted_r * mix);
            }

            out_l[offset..offset + chunk_len].copy_from_slice(&scratch_l[..chunk_len]);
            if let Some(ref mut r_slice) = out_r {
                r_slice[offset..offset + chunk_len].copy_from_slice(&scratch_r[..chunk_len]);
            }

            offset += chunk_len;
        }

        self.phase = phase;
        self.smoothed_shift = smoothed_shift;
    }
}
