use crate::common;

#[derive(Debug)]
pub struct ExternalSidechainCore {
    sample_rate: f32,
    target_gain_db: f32,
    target_mute: f32,
    current_gain: f32,
}

impl ExternalSidechainCore {
    pub fn new() -> Self {
        Self {
            sample_rate: 44100.0,
            target_gain_db: 0.0,
            target_mute: 0.0,
            current_gain: 1.0,
        }
    }

    pub fn prepare(&mut self, sample_rate: f32) {
        self.sample_rate = sample_rate.max(1.0);
        self.current_gain = 1.0;
    }

    pub fn reset(&mut self) {
        self.current_gain = 1.0;
    }

    pub fn set_parameters(&mut self, gain_db: f32, mute: f32) {
        self.target_gain_db = gain_db.clamp(-60.0, 12.0);
        self.target_mute = mute.clamp(0.0, 1.0);
    }

    pub fn process(
        &mut self,
        sc_l: Option<&[f32]>,
        sc_r: Option<&[f32]>,
        out_l: &mut [f32],
        mut out_r: Option<&mut [f32]>,
        num_samples: usize,
    ) {
        let is_muted = self.target_mute > 0.5;
        let target_gain_lin = if is_muted {
            0.0
        } else {
            10.0f32.powf(self.target_gain_db / 20.0)
        };

        let mut curr_gain = self.current_gain;

        const CHUNK: usize = 1024;
        let mut scratch_l = [0.0f32; CHUNK];
        let mut scratch_r = [0.0f32; CHUNK];

        let mut offset = 0;

        while offset < num_samples {
            let chunk_len = (num_samples - offset).min(CHUNK);

            for i in 0..chunk_len {
                curr_gain += 0.01 * (target_gain_lin - curr_gain);
                let gain = common::denormal_guard(curr_gain);

                let sample_l = match sc_l {
                    Some(s) => s[offset + i] * gain,
                    None => 0.0,
                };
                let sample_r = match (sc_r, sc_l) {
                    (Some(sr), _) => sr[offset + i] * gain,
                    (None, Some(sl)) => sl[offset + i] * gain,
                    _ => 0.0,
                };

                scratch_l[i] = common::denormal_guard(sample_l);
                scratch_r[i] = common::denormal_guard(sample_r);
            }

            out_l[offset..offset + chunk_len].copy_from_slice(&scratch_l[..chunk_len]);
            if let Some(ref mut r_slice) = out_r {
                r_slice[offset..offset + chunk_len].copy_from_slice(&scratch_r[..chunk_len]);
            }

            offset += chunk_len;
        }

        self.current_gain = curr_gain;
    }
}
