//! Event Capture Ring Buffer (Roadmap Phase 2, Rules 9, 10, 47)
//! Circular buffer for audio capturing and fractional-delay slice reading with zero allocations in audio loop.

pub struct RustEventCaptureBuffer {
    channels: [Vec<f32>; 2],
    capacity_samples: usize,
    write_pos: usize,
    num_channels: usize,
}

impl RustEventCaptureBuffer {
    pub fn new() -> Self {
        Self {
            channels: [Vec::new(), Vec::new()],
            capacity_samples: 0,
            write_pos: 0,
            num_channels: 2,
        }
    }

    pub fn prepare(&mut self, sample_rate: f64, max_history_seconds: f64, num_channels: usize) {
        let sr = if sample_rate > 0.0 { sample_rate } else { 44100.0 };
        let history_sec = if max_history_seconds > 0.0 { max_history_seconds } else { 5.0 };
        let capacity = (sr * history_sec).ceil() as usize;
        let safe_capacity = capacity.max(256);

        self.num_channels = num_channels.clamp(1, 2);
        self.capacity_samples = safe_capacity;

        for ch in 0..self.num_channels {
            self.channels[ch].clear();
            self.channels[ch].resize(safe_capacity, 0.0);
        }
        for ch in self.num_channels..2 {
            self.channels[ch].clear();
        }

        self.write_pos = 0;
    }

    pub fn reset(&mut self) {
        for ch in 0..self.num_channels {
            self.channels[ch].fill(0.0);
        }
        self.write_pos = 0;
    }

    /// Writes block of input samples into circular buffer without heap allocations.
    pub fn write_channels(&mut self, in_l: &[f32], in_r: &[f32], num_samples: usize) {
        if self.capacity_samples == 0 || num_samples == 0 {
            return;
        }

        let cap = self.capacity_samples;
        let to_process = num_samples.min(in_l.len());

        if to_process >= cap {
            let skip = to_process - cap;
            if self.num_channels > 0 && !self.channels[0].is_empty() {
                self.channels[0].copy_from_slice(&in_l[skip..skip + cap]);
            }
            if self.num_channels > 1 && !self.channels[1].is_empty() {
                let r_slice = if in_r.len() >= skip + cap { &in_r[skip..skip + cap] } else { &in_l[skip..skip + cap] };
                self.channels[1].copy_from_slice(r_slice);
            }
            self.write_pos = 0;
            return;
        }

        let first_chunk = to_process.min(cap - self.write_pos);
        let second_chunk = to_process - first_chunk;

        if self.num_channels > 0 && !self.channels[0].is_empty() {
            self.channels[0][self.write_pos..self.write_pos + first_chunk]
                .copy_from_slice(&in_l[..first_chunk]);
            if second_chunk > 0 {
                self.channels[0][..second_chunk]
                    .copy_from_slice(&in_l[first_chunk..first_chunk + second_chunk]);
            }
        }

        if self.num_channels > 1 && !self.channels[1].is_empty() {
            let r_source = if in_r.len() >= to_process { in_r } else { in_l };
            self.channels[1][self.write_pos..self.write_pos + first_chunk]
                .copy_from_slice(&r_source[..first_chunk]);
            if second_chunk > 0 {
                self.channels[1][..second_chunk]
                    .copy_from_slice(&r_source[first_chunk..first_chunk + second_chunk]);
            }
        }

        self.write_pos = (self.write_pos + to_process) % cap;
    }

    /// Reads single sample with linear interpolation at offset relative to current write position.
    #[inline]
    pub fn read_sample(&self, channel: usize, sample_offset: f64) -> f32 {
        if self.capacity_samples == 0 || channel >= self.num_channels || self.channels[channel].is_empty() {
            return 0.0;
        }

        let cap = self.capacity_samples;
        let cap_f = cap as f64;
        let mut read_index = (self.write_pos as f64) - sample_offset;

        while read_index < 0.0 {
            read_index += cap_f;
        }
        while read_index >= cap_f {
            read_index -= cap_f;
        }

        let idx0 = (read_index as usize) % cap;
        let idx1 = if idx0 + 1 < cap { idx0 + 1 } else { 0 };
        let frac = (read_index - read_index.floor()) as f32;

        let buf = &self.channels[channel];
        let s0 = buf[idx0];
        let s1 = buf[idx1];

        s0 + frac * (s1 - s0)
    }

    #[inline]
    pub fn capacity_samples(&self) -> usize {
        self.capacity_samples
    }

    #[inline]
    pub fn write_position(&self) -> usize {
        self.write_pos
    }

    #[inline]
    pub fn num_channels(&self) -> usize {
        self.num_channels
    }
}
