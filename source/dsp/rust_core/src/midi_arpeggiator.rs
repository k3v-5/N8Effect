use crate::common;

pub struct MidiArpeggiatorCore {
    sample_rate: f32,
    active_notes: [i32; 16],
    num_active_notes: usize,
    current_note: i32,
    last_step: i64,
    osc_phase: f32,
    env_level: f32,
    target_rate: f32,
    target_pattern: f32,
    target_octaves: f32,
    target_gate: f32,
    target_swing: f32,
    target_synth_mix: f32,
}

impl MidiArpeggiatorCore {
    pub fn new() -> Self {
        let mut notes = [0i32; 16];
        notes[0] = 48;
        notes[1] = 51;
        notes[2] = 55;
        notes[3] = 58;
        notes[4] = 62;
        Self {
            sample_rate: 44100.0,
            active_notes: notes,
            num_active_notes: 5,
            current_note: 48,
            last_step: -1,
            osc_phase: 0.0,
            env_level: 0.0,
            target_rate: 1.0,
            target_pattern: 0.0,
            target_octaves: 2.0,
            target_gate: 75.0,
            target_swing: 0.0,
            target_synth_mix: 0.8,
        }
    }

    pub fn prepare(&mut self, sample_rate: f32) {
        self.sample_rate = sample_rate.max(1.0);
        self.osc_phase = 0.0;
        self.env_level = 0.0;
        self.last_step = -1;
        self.current_note = self.active_notes[0];
    }

    pub fn reset(&mut self) {
        self.osc_phase = 0.0;
        self.env_level = 0.0;
        self.last_step = -1;
    }

    pub fn set_parameter(&mut self, param_id: u32, value: f32) {
        match param_id {
            1 => self.target_rate = value,
            2 => self.target_pattern = value,
            3 => self.target_octaves = value,
            4 => self.target_gate = value,
            5 => self.target_swing = value,
            6 => self.target_synth_mix = value,
            _ => {}
        }
    }

    pub fn get_parameter(&self, param_id: u32) -> f32 {
        match param_id {
            1 => self.target_rate,
            2 => self.target_pattern,
            3 => self.target_octaves,
            4 => self.target_gate,
            5 => self.target_swing,
            6 => self.target_synth_mix,
            _ => 0.0,
        }
    }

    pub fn set_notes(&mut self, notes: &[i32]) {
        self.num_active_notes = notes.len().min(16);
        for i in 0..self.num_active_notes {
            self.active_notes[i] = notes[i];
        }
        if self.num_active_notes == 0 {
            self.active_notes[0] = 48;
            self.num_active_notes = 1;
        }
    }

    pub fn set_parameters(
        &mut self,
        rate: f32,
        pattern: f32,
        octaves: f32,
        gate: f32,
        swing: f32,
        synth_mix: f32,
    ) {
        self.target_rate = rate.clamp(0.0, 6.0);
        self.target_pattern = pattern.clamp(0.0, 4.0);
        self.target_octaves = octaves.clamp(1.0, 4.0);
        self.target_gate = gate.clamp(10.0, 100.0);
        self.target_swing = swing.clamp(0.0, 75.0);
        self.target_synth_mix = synth_mix.clamp(0.0, 1.0);
    }

    fn get_beats_per_step(&self) -> f64 {
        let r = self.target_rate.round() as i32;
        match r {
            0 => 0.125,
            1 => 0.25,
            2 => 0.5,
            3 => 1.0,
            4 => 2.0,
            5 => 0.5 * (2.0 / 3.0),
            6 => 0.25 * 1.5,
            _ => 0.25,
        }
    }

    fn compute_arp_note(&self, step_idx: i64, total_octaves: usize) -> i32 {
        if self.num_active_notes == 0 {
            return 60;
        }

        let total_notes = self.num_active_notes * total_octaves;
        if total_notes == 0 {
            return 60;
        }
        let pattern = self.target_pattern.round() as i32;
        let tn_i64 = total_notes as i64;

        let note_idx = match pattern {
            0 => { // Up
                ((step_idx % tn_i64 + tn_i64) % tn_i64) as usize
            }
            1 => { // Down
                let pos = ((step_idx % tn_i64 + tn_i64) % tn_i64) as usize;
                total_notes - 1 - pos
            }
            2 => { // UpDown
                let cycle = if total_notes > 1 { total_notes * 2 - 2 } else { 1 };
                let cycle_i64 = cycle as i64;
                let pos = ((step_idx % cycle_i64 + cycle_i64) % cycle_i64) as usize;
                if pos < total_notes {
                    pos
                } else {
                    cycle - pos
                }
            }
            3 => { // Random
                let r = (step_idx as u32).wrapping_mul(1103515245).wrapping_add(12345);
                ((r >> 16) as usize) % total_notes
            }
            _ => { // Chord root / As-Played
                let num_i64 = self.num_active_notes as i64;
                ((step_idx % num_i64 + num_i64) % num_i64) as usize
            }
        };

        let base_note_idx = note_idx % self.num_active_notes;
        let octave_offset = (note_idx / self.num_active_notes) as i32 * 12;
        self.active_notes[base_note_idx] + octave_offset
    }

    pub fn process(
        &mut self,
        in_l: Option<&[f32]>,
        in_r: Option<&[f32]>,
        out_l: &mut [f32],
        mut out_r: Option<&mut [f32]>,
        num_samples: usize,
        ppq_position: f64,
        bpm: f64,
    ) {
        let sample_rate = self.sample_rate as f64;
        let beats_per_step = self.get_beats_per_step();
        let total_octaves = (self.target_octaves.round() as usize).clamp(1, 4);
        let gate_ratio = (self.target_gate * 0.01).clamp(0.05, 0.99) as f64;
        let mix = self.target_synth_mix;

        let effective_bpm = if bpm > 0.0 { bpm } else { 120.0 };
        let ppq_per_sample = (effective_bpm / 60.0) / sample_rate;
        let mut current_ppq = ppq_position;

        let mut osc_phase = self.osc_phase;
        let mut env_level = self.env_level;
        let mut last_step = self.last_step;
        let mut current_note = self.current_note;

        const CHUNK: usize = 1024;
        let mut scratch_l = [0.0f32; CHUNK];
        let mut scratch_r = [0.0f32; CHUNK];

        let mut offset = 0;

        while offset < num_samples {
            let chunk_len = (num_samples - offset).min(CHUNK);

            for i in 0..chunk_len {
                let pos_in_step = (current_ppq % beats_per_step + beats_per_step) % beats_per_step / beats_per_step;
                let step_idx = (current_ppq / beats_per_step).floor() as i64;

                if step_idx != last_step {
                    last_step = step_idx;
                    current_note = self.compute_arp_note(step_idx, total_octaves);
                    env_level = 1.0;
                }

                if pos_in_step > gate_ratio {
                    env_level *= 0.992;
                } else {
                    env_level *= 0.9995;
                }
                env_level = common::denormal_guard(env_level);

                let freq = 440.0 * 2.0f32.powf((current_note as f32 - 69.0) / 12.0);
                let phase_inc = freq / self.sample_rate;

                osc_phase += phase_inc;
                if osc_phase >= 1.0 {
                    osc_phase -= 1.0;
                }

                let mut synth_sample = (2.0 * osc_phase - 1.0) * env_level;
                synth_sample = (synth_sample * 1.5).tanh();
                synth_sample = common::denormal_guard(synth_sample);

                let dry_l = match in_l {
                    Some(s) => s[offset + i],
                    None => 0.0,
                };
                let dry_r = match (in_r, in_l) {
                    (Some(sr), _) => sr[offset + i],
                    (None, Some(sl)) => sl[offset + i],
                    _ => 0.0,
                };

                scratch_l[i] = common::denormal_guard(dry_l * (1.0 - mix) + synth_sample * mix);
                scratch_r[i] = common::denormal_guard(dry_r * (1.0 - mix) + synth_sample * mix);

                current_ppq += ppq_per_sample;
            }

            out_l[offset..offset + chunk_len].copy_from_slice(&scratch_l[..chunk_len]);
            if let Some(ref mut r_slice) = out_r {
                r_slice[offset..offset + chunk_len].copy_from_slice(&scratch_r[..chunk_len]);
            }

            offset += chunk_len;
        }

        self.osc_phase = osc_phase;
        self.env_level = env_level;
        self.last_step = last_step;
        self.current_note = current_note;
    }
}
