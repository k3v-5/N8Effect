//! Fractional Circular Delay Line with Power-of-Two Bitmask & Hermite Interpolation
//! Implements O(1) bitmasked indexing (index & mask), linear and 4-point cubic Hermite interpolation,
//! zero allocations during audio processing, and SIMD cacheline friendly storage.
//! Follows Rules 11, 12, 32, 34, 46, 47.

#[repr(C)]
pub struct RustDelayLine {
    max_delay_samples: usize,
    capacity: usize,
    mask: usize,
    write_index: usize,
    buffer: Vec<f32>,
}

impl RustDelayLine {
    pub fn new() -> Self {
        Self {
            max_delay_samples: 0,
            capacity: 0,
            mask: 0,
            write_index: 0,
            buffer: Vec::new(),
        }
    }

    pub fn prepare(&mut self, max_delay_samples: usize) {
        self.max_delay_samples = max_delay_samples.max(16);
        let mut cap = 16usize;
        while cap < self.max_delay_samples + 4 {
            cap <<= 1;
        }
        self.capacity = cap;
        self.mask = self.capacity - 1;
        self.buffer.resize(self.capacity, 0.0);
        self.buffer.fill(0.0);
        self.write_index = 0;
    }

    pub fn reset(&mut self) {
        self.buffer.fill(0.0);
        self.write_index = 0;
    }

    #[inline(always)]
    pub fn write(&mut self, sample: f32) {
        if self.capacity == 0 {
            return;
        }
        self.buffer[self.write_index] = sample;
        self.write_index = (self.write_index + 1) & self.mask;
    }

    #[inline(always)]
    pub fn read_linear(&self, delay_samples: f32) -> f32 {
        if self.capacity == 0 {
            return 0.0;
        }

        let clamped_delay = delay_samples.clamp(0.0, self.max_delay_samples as f32);
        let read_pos = self.write_index as f32 - clamped_delay;
        let i_pos = read_pos.floor() as isize;
        let frac = read_pos - i_pos as f32;

        let idx0 = (i_pos as usize) & self.mask;
        let idx1 = (idx0 + 1) & self.mask;

        self.buffer[idx0] + frac * (self.buffer[idx1] - self.buffer[idx0])
    }

    #[inline(always)]
    pub fn read_cubic(&self, delay_samples: f32) -> f32 {
        if self.capacity == 0 {
            return 0.0;
        }
        if self.capacity < 4 {
            return self.read_linear(delay_samples);
        }

        let clamped_delay = delay_samples.clamp(1.0, self.max_delay_samples as f32);
        let read_pos = self.write_index as f32 - clamped_delay;
        let i_pos = read_pos.floor() as isize;
        let frac = read_pos - i_pos as f32;

        let idx0 = ((i_pos - 1) as usize) & self.mask;
        let idx1 = (i_pos as usize) & self.mask;
        let idx2 = (idx1 + 1) & self.mask;
        let idx3 = (idx1 + 2) & self.mask;

        let xm1 = self.buffer[idx0];
        let x0  = self.buffer[idx1];
        let x1  = self.buffer[idx2];
        let x2  = self.buffer[idx3];

        let c0 = x0;
        let c1 = 0.5 * (x1 - xm1);
        let c2 = xm1 - 2.5 * x0 + 2.0 * x1 - 0.5 * x2;
        let c3 = 0.5 * (x2 - xm1) + 1.5 * (x0 - x1);

        ((c3 * frac + c2) * frac + c1) * frac + c0
    }

    #[inline(always)]
    pub fn get_max_delay_samples(&self) -> usize {
        self.max_delay_samples
    }

    #[inline(always)]
    pub fn get_capacity(&self) -> usize {
        self.capacity
    }

    #[inline(always)]
    pub fn get_mask(&self) -> usize {
        self.mask
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_delay_line_bitmask_and_interpolation() {
        let mut delay = RustDelayLine::new();
        delay.prepare(100);

        assert_eq!(delay.get_capacity(), 128);
        assert_eq!(delay.get_capacity() & (delay.get_capacity() - 1), 0);
        assert_eq!(delay.get_mask(), 127);

        delay.write(1.0);
        for _ in 0..19 {
            delay.write(0.0);
        }

        let r_linear = delay.read_linear(20.0);
        assert!((r_linear - 1.0).abs() < 1e-4);

        let r_cubic = delay.read_cubic(20.0);
        assert!((r_cubic - 1.0).abs() < 1e-3);
    }
}
