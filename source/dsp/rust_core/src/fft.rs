//! Reusable High-Performance Radix-2 Cooley-Tukey FFT Kernel (Reglas 13, 19, 32, 46, 47)
//! Zero allocations in transform cycle, split real/imaginary vectors for maximum cache locality,
//! precomputed bit-reversal and twiddle factor LUTs, forward and inverse transforms.

use std::f32::consts::TAU;

pub struct RustFFT {
    size: usize,
    bit_rev: Box<[usize]>,
    twiddle_fwd_re: Box<[f32]>,
    twiddle_fwd_im: Box<[f32]>,
    twiddle_inv_re: Box<[f32]>,
    twiddle_inv_im: Box<[f32]>,
}

impl RustFFT {
    pub fn new(size: usize) -> Self {
        let n = size.next_power_of_two().max(2);
        let mut bit_rev = vec![0usize; n].into_boxed_slice();
        for i in 0..n {
            let mut rev = 0;
            let mut temp = i;
            let mut bit = 1;
            while bit < n {
                rev = (rev << 1) | (temp & 1);
                temp >>= 1;
                bit <<= 1;
            }
            bit_rev[i] = rev;
        }

        let half = n / 2;
        let mut twiddle_fwd_re = vec![0.0f32; half].into_boxed_slice();
        let mut twiddle_fwd_im = vec![0.0f32; half].into_boxed_slice();
        let mut twiddle_inv_re = vec![0.0f32; half].into_boxed_slice();
        let mut twiddle_inv_im = vec![0.0f32; half].into_boxed_slice();

        for k in 0..half {
            let angle = TAU * (k as f32) / (n as f32);
            let cos_val = angle.cos();
            let sin_val = angle.sin();
            twiddle_fwd_re[k] = cos_val;
            twiddle_fwd_im[k] = -sin_val;
            twiddle_inv_re[k] = cos_val;
            twiddle_inv_im[k] = sin_val;
        }

        Self {
            size: n,
            bit_rev,
            twiddle_fwd_re,
            twiddle_fwd_im,
            twiddle_inv_re,
            twiddle_inv_im,
        }
    }

    #[inline(always)]
    pub fn size(&self) -> usize {
        self.size
    }

    /// Computes the in-place Radix-2 FFT.
    /// `re` and `im` must have length at least `self.size`.
    #[inline]
    pub fn compute(&self, re: &mut [f32], im: &mut [f32], inverse: bool) {
        let n = self.size;
        debug_assert!(re.len() >= n && im.len() >= n);

        // 1. Bit-reversal permutation via LUT O(N)
        for i in 0..n {
            let j = self.bit_rev[i];
            if i < j {
                re.swap(i, j);
                im.swap(i, j);
            }
        }

        let tw_re = if inverse { &self.twiddle_inv_re } else { &self.twiddle_fwd_re };
        let tw_im = if inverse { &self.twiddle_inv_im } else { &self.twiddle_fwd_im };

        // 2. Butterfly stages
        let mut len = 2;
        while len <= n {
            let half_len = len / 2;
            let stride = n / len;
            let mut i = 0;
            while i < n {
                for j in 0..half_len {
                    let wr = tw_re[j * stride];
                    let wi = tw_im[j * stride];

                    let ur = re[i + j];
                    let ui = im[i + j];

                    let vr_raw = re[i + j + half_len];
                    let vi_raw = im[i + j + half_len];

                    let vr = vr_raw * wr - vi_raw * wi;
                    let vi = vr_raw * wi + vi_raw * wr;

                    re[i + j] = ur + vr;
                    im[i + j] = ui + vi;
                    re[i + j + half_len] = ur - vr;
                    im[i + j + half_len] = ui - vi;
                }
                i += len;
            }
            len <<= 1;
        }

        // 3. Normalization 1/N for IFFT
        if inverse {
            let inv_n = 1.0 / (n as f32);
            for i in 0..n {
                re[i] *= inv_n;
                im[i] *= inv_n;
            }
        }
    }
}
