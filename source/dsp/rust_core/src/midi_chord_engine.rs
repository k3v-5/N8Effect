use crate::common;

const MAX_VOICES: usize = 5;

#[derive(Clone, Copy, Default)]
struct ChordVoice {
    phase: f32,
    phase_inc: f32,
    env: f32,
    delay_remaining: u32,
    pan_l: f32,
    pan_r: f32,
}

pub struct MidiChordEngineCore {
    sample_rate: f32,
    voices: [ChordVoice; MAX_VOICES],
    current_voice_count: usize,
    target_chord_type: f32,
    target_inversion: f32,
    target_strum_delay: f32,
    target_spread: f32,
    target_root_note: f32,
    target_synth_mix: f32,
}

impl MidiChordEngineCore {
    pub fn new() -> Self {
        let mut core = Self {
            sample_rate: 44100.0,
            voices: [ChordVoice::default(); MAX_VOICES],
            current_voice_count: 4,
            target_chord_type: 7.0,
            target_inversion: 0.0,
            target_strum_delay: 15.0,
            target_spread: 0.5,
            target_root_note: 48.0,
            target_synth_mix: 0.8,
        };
        core.recalculate_chord();
        core.trigger_chord();
        core
    }

    pub fn prepare(&mut self, sample_rate: f32) {
        self.sample_rate = sample_rate.max(1.0);
        self.recalculate_chord();
        self.trigger_chord();
    }

    pub fn reset(&mut self) {
        for v in &mut self.voices {
            v.phase = 0.0;
            v.env = 0.0;
        }
    }

    pub fn set_parameter(&mut self, param_id: u32, value: f32) {
        let mut recompute = false;
        match param_id {
            1 => { self.target_chord_type = value.clamp(0.0, 10.0); recompute = true; }
            2 => { self.target_inversion = value.clamp(0.0, 2.0); recompute = true; }
            3 => self.target_strum_delay = value.clamp(0.0, 80.0),
            4 => self.target_spread = value.clamp(0.0, 1.0),
            5 => { self.target_root_note = value.clamp(36.0, 72.0); recompute = true; }
            6 => self.target_synth_mix = value.clamp(0.0, 1.0),
            _ => {}
        }
        if recompute {
            self.recalculate_chord();
        }
    }

    pub fn get_parameter(&self, param_id: u32) -> f32 {
        match param_id {
            1 => self.target_chord_type,
            2 => self.target_inversion,
            3 => self.target_strum_delay,
            4 => self.target_spread,
            5 => self.target_root_note,
            6 => self.target_synth_mix,
            _ => 0.0,
        }
    }

    pub fn set_parameters(
        &mut self,
        chord_type: f32,
        inversion: f32,
        strum_delay: f32,
        spread: f32,
        root_note: f32,
        synth_mix: f32,
    ) {
        let recompute = (self.target_chord_type != chord_type)
            || (self.target_inversion != inversion)
            || (self.target_root_note != root_note);

        self.target_chord_type = chord_type.clamp(0.0, 10.0);
        self.target_inversion = inversion.clamp(0.0, 2.0);
        self.target_strum_delay = strum_delay.clamp(0.0, 80.0);
        self.target_spread = spread.clamp(0.0, 1.0);
        self.target_root_note = root_note.clamp(36.0, 72.0);
        self.target_synth_mix = synth_mix.clamp(0.0, 1.0);

        if recompute {
            self.recalculate_chord();
        }
    }

    pub fn trigger_chord(&mut self) {
        let sr = self.sample_rate as f64;
        let strum_samples = ((self.target_strum_delay as f64 * 0.001) * sr) as u32;
        let spread = self.target_spread;
        let count = self.current_voice_count;

        for i in 0..count {
            self.voices[i].delay_remaining = (i as u32) * (strum_samples / (count.max(1) as u32));
            self.voices[i].env = 1.0;

            let pan = if count > 1 {
                ((i as f32) / (count - 1) as f32 * 2.0 - 1.0) * spread
            } else {
                0.0
            };

            let angle = (pan + 1.0) * 0.25 * std::f32::consts::PI;
            self.voices[i].pan_l = angle.cos();
            self.voices[i].pan_r = angle.sin();
        }
    }

    pub fn recalculate_chord(&mut self) {
        let chord_type = self.target_chord_type.round() as i32;
        let root = self.target_root_note.round() as i32;
        let inv = self.target_inversion.round() as i32;

        let mut intervals = [0i32; MAX_VOICES];
        let count;

        match chord_type {
            0 => { intervals[0..3].copy_from_slice(&[0, 12, 24]); count = 3; }
            1 => { intervals[0..4].copy_from_slice(&[0, 7, 12, 19]); count = 4; }
            2 => { intervals[0..4].copy_from_slice(&[0, 4, 7, 12]); count = 4; }
            3 => { intervals[0..4].copy_from_slice(&[0, 3, 7, 12]); count = 4; }
            4 => { intervals[0..4].copy_from_slice(&[0, 2, 7, 12]); count = 4; }
            5 => { intervals[0..4].copy_from_slice(&[0, 5, 7, 12]); count = 4; }
            6 => { intervals[0..4].copy_from_slice(&[0, 4, 7, 10]); count = 4; }
            7 => { intervals[0..4].copy_from_slice(&[0, 4, 7, 11]); count = 4; }
            8 => { intervals[0..4].copy_from_slice(&[0, 3, 7, 10]); count = 4; }
            9 => { intervals[0..5].copy_from_slice(&[0, 3, 7, 10, 14]); count = 5; }
            10 => { intervals[0..4].copy_from_slice(&[0, 3, 6, 12]); count = 4; }
            _ => { intervals[0..4].copy_from_slice(&[0, 4, 7, 11]); count = 4; }
        }

        // Apply inversion (rotate and add 12)
        for _ in 0..inv {
            if count > 1 {
                intervals[0] += 12;
                intervals[0..count].rotate_left(1);
            }
        }

        self.current_voice_count = count;
        let sr = self.sample_rate;

        for i in 0..count {
            let note = root + intervals[i];
            let freq = 440.0 * 2.0f32.powf((note as f32 - 69.0) / 12.0);
            self.voices[i].phase_inc = freq / sr;
        }
    }

    pub fn process(
        &mut self,
        in_l: Option<&[f32]>,
        in_r: Option<&[f32]>,
        out_l: &mut [f32],
        mut out_r: Option<&mut [f32]>,
        num_samples: usize,
    ) {
        let mix = self.target_synth_mix;
        let count = self.current_voice_count;

        const CHUNK: usize = 1024;
        let mut scratch_l = [0.0f32; CHUNK];
        let mut scratch_r = [0.0f32; CHUNK];

        let mut offset = 0;

        while offset < num_samples {
            let chunk_len = (num_samples - offset).min(CHUNK);

            for i in 0..chunk_len {
                let mut chord_sample_l = 0.0f32;
                let mut chord_sample_r = 0.0f32;

                for v in 0..count {
                    let voice = &mut self.voices[v];
                    if voice.delay_remaining > 0 {
                        voice.delay_remaining -= 1;
                        continue;
                    }

                    voice.phase += voice.phase_inc;
                    if voice.phase >= 1.0 {
                        voice.phase -= 1.0;
                    }

                    voice.env *= 0.99985;
                    voice.env = common::denormal_guard(voice.env);

                    let two_pi_p = voice.phase * 2.0 * std::f32::consts::PI;
                    let four_pi_p = voice.phase * 4.0 * std::f32::consts::PI;
                    let wave = (two_pi_p.sin() * 0.7 + four_pi_p.sin() * 0.3) * voice.env;

                    chord_sample_l += wave * voice.pan_l;
                    chord_sample_r += wave * voice.pan_r;
                }

                chord_sample_l = (chord_sample_l * 0.6).tanh();
                chord_sample_r = (chord_sample_r * 0.6).tanh();

                chord_sample_l = common::denormal_guard(chord_sample_l);
                chord_sample_r = common::denormal_guard(chord_sample_r);

                let dry_l = match in_l {
                    Some(s) => s[offset + i],
                    None => 0.0,
                };
                let dry_r = match (in_r, in_l) {
                    (Some(sr), _) => sr[offset + i],
                    (None, Some(sl)) => sl[offset + i],
                    _ => 0.0,
                };

                scratch_l[i] = common::denormal_guard(dry_l * (1.0 - mix) + chord_sample_l * mix);
                scratch_r[i] = common::denormal_guard(dry_r * (1.0 - mix) + chord_sample_r * mix);
            }

            out_l[offset..offset + chunk_len].copy_from_slice(&scratch_l[..chunk_len]);
            if let Some(ref mut r_slice) = out_r {
                r_slice[offset..offset + chunk_len].copy_from_slice(&scratch_r[..chunk_len]);
            }

            offset += chunk_len;
        }
    }
}
