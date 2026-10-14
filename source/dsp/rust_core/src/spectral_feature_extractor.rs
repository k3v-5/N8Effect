//! Real-Time Acoustic Spectral Feature Extractor (Rules 1, 7, 9, 32, 46, 47)
//! Analyzes incoming audio in frequency domain via Radix-2 FFT with Hann window:
//! - Spectral Centroid (timbral brightness normalized 0.0 to 1.0)
//! - Spectral Flux (rate of spectral change/novelty)
//! - Spectral Flatness (Wiener entropy, tonal vs noisy/inharmonicity)
//! - Energy (RMS spectral energy)
//! Zero allocations in processing loop.

use std::f32::consts::PI;
use crate::common::denormal_guard;
use crate::fft::RustFFT;

pub const DEFAULT_FFT_SIZE: usize = 1024;
pub const DEFAULT_HOP_SIZE: usize = 256;

pub struct RustSpectralFeatureExtractor {
    sample_rate: f64,
    fft_size: usize,
    hop_size: usize,
    hop_counter: usize,

    fft: RustFFT,
    time_buffer: Box<[f32]>,
    window: Box<[f32]>,
    fft_re: Box<[f32]>,
    fft_im: Box<[f32]>,
    current_magnitudes: Box<[f32]>,
    prev_magnitudes: Box<[f32]>,

    centroid: f32,
    target_centroid: f32,
    flux: f32,
    target_flux: f32,
    flatness: f32,
    energy: f32,
}

impl RustSpectralFeatureExtractor {
    pub fn new() -> Self {
        let fft_size = DEFAULT_FFT_SIZE;
        let num_bins = fft_size / 2 + 1;
        let mut window = vec![0.0f32; fft_size].into_boxed_slice();
        for i in 0..fft_size {
            window[i] = 0.5 * (1.0 - (2.0 * PI * i as f32 / ((fft_size - 1) as f32)).cos());
        }

        Self {
            sample_rate: 44100.0,
            fft_size,
            hop_size: DEFAULT_HOP_SIZE,
            hop_counter: 0,
            fft: RustFFT::new(fft_size),
            time_buffer: vec![0.0f32; fft_size].into_boxed_slice(),
            window,
            fft_re: vec![0.0f32; fft_size].into_boxed_slice(),
            fft_im: vec![0.0f32; fft_size].into_boxed_slice(),
            current_magnitudes: vec![0.0f32; num_bins].into_boxed_slice(),
            prev_magnitudes: vec![0.0f32; num_bins].into_boxed_slice(),
            centroid: 0.2,
            target_centroid: 0.2,
            flux: 0.0,
            target_flux: 0.0,
            flatness: 0.0,
            energy: 0.0,
        }
    }

    pub fn prepare(&mut self, sample_rate: f64, fft_size: usize, hop_size: usize) {
        self.sample_rate = if sample_rate > 0.0 { sample_rate } else { 44100.0 };

        let mut actual_fft_size = 1;
        while actual_fft_size < fft_size {
            actual_fft_size <<= 1;
        }
        if actual_fft_size < 64 {
            actual_fft_size = 64;
        }

        self.fft_size = actual_fft_size;
        self.hop_size = if hop_size > 0 { hop_size } else { self.fft_size / 4 };

        self.fft = RustFFT::new(self.fft_size);
        let num_bins = self.fft_size / 2 + 1;

        self.time_buffer = vec![0.0f32; self.fft_size].into_boxed_slice();
        self.fft_re = vec![0.0f32; self.fft_size].into_boxed_slice();
        self.fft_im = vec![0.0f32; self.fft_size].into_boxed_slice();
        self.current_magnitudes = vec![0.0f32; num_bins].into_boxed_slice();
        self.prev_magnitudes = vec![0.0f32; num_bins].into_boxed_slice();

        let mut window = vec![0.0f32; self.fft_size].into_boxed_slice();
        for i in 0..self.fft_size {
            window[i] = 0.5 * (1.0 - (2.0 * PI * i as f32 / ((self.fft_size - 1) as f32)).cos());
        }
        self.window = window;

        self.hop_counter = 0;
        self.reset();
    }

    pub fn reset(&mut self) {
        self.current_magnitudes.fill(0.0);
        self.prev_magnitudes.fill(0.0);
        self.time_buffer.fill(0.0);
        self.hop_counter = 0;
        self.centroid = 0.2;
        self.target_centroid = 0.2;
        self.flux = 0.0;
        self.target_flux = 0.0;
        self.flatness = 0.0;
        self.energy = 0.0;
    }

    #[inline]
    pub fn process(&mut self, input: &[f32], is_silent: bool) {
        if input.is_empty() {
            return;
        }

        if is_silent {
            // Bypass FFT and descriptors on silence (Rules 9 and 47)
            let alpha = 0.15f32;
            self.target_flux = 0.0;
            self.flux += alpha * (0.0 - self.flux);
            self.energy = 0.0;
            self.flatness = 0.0;
            // Preservar centroid y target_centroid para estabilidad de modulación
            return;
        }

        for &sample in input {
            let s = denormal_guard(sample);
            self.time_buffer[self.hop_counter] = s;
            self.hop_counter += 1;

            if self.hop_counter >= self.fft_size {
                self.hop_counter = 0;
                self.compute_spectral_descriptors();
            }
        }

        // Anti-click smoothing of spectral descriptors (Rule 35)
        let alpha = 0.15f32;
        self.centroid += alpha * (self.target_centroid - self.centroid);
        self.flux += alpha * (self.target_flux - self.flux);
    }

    #[inline]
    pub fn get_spectral_centroid(&self) -> f32 {
        self.centroid
    }

    #[inline]
    pub fn get_spectral_flux(&self) -> f32 {
        self.flux
    }

    #[inline]
    pub fn get_spectral_flatness(&self) -> f32 {
        self.flatness
    }

    #[inline]
    pub fn get_energy(&self) -> f32 {
        self.energy
    }

    fn compute_spectral_descriptors(&mut self) {
        let num_bins = self.fft_size / 2 + 1;

        // Comprobación de silencio en el frame temporal (< -70 dBFS / 0.0003 RMS)
        let mut frame_energy = 0.0f32;
        for i in 0..self.fft_size {
            let val = self.time_buffer[i];
            frame_energy += val * val;
        }
        if frame_energy < (self.fft_size as f32) * (0.0003 * 0.0003) {
            self.target_flux = 0.0;
            self.flux = 0.0;
            self.energy = 0.0;
            self.flatness = 0.0;
            // Mantener centroid y target_centroid
            return;
        }

        // 1. Aplicar ventana Hann y llenar buffer
        for i in 0..self.fft_size {
            self.fft_re[i] = self.time_buffer[i] * self.window[i];
            self.fft_im[i] = 0.0;
        }

        // 2. Ejecutar FFT Radix-2 rápida
        self.fft.compute(&mut self.fft_re, &mut self.fft_im, false);

        // 3. Extraer espectro de magnitud
        let mut sum_magnitude = 0.0f32;
        let mut weighted_sum = 0.0f32;
        let mut flux_diff_sum = 0.0f32;
        let mut log_power_sum = 0.0f32;
        let mut power_sum = 0.0f32;

        for k in 0..num_bins {
            let real = self.fft_re[k];
            let imag = self.fft_im[k];
            let mag = (real * real + imag * imag).sqrt();
            self.current_magnitudes[k] = mag;

            sum_magnitude += mag;
            weighted_sum += k as f32 * mag;

            // Flujo espectral (rectified spectral difference)
            let diff = (mag - self.prev_magnitudes[k]).max(0.0);
            flux_diff_sum += diff * diff;

            // Planitud espectral (Power spectrum)
            let power = mag * mag + 1e-12;
            log_power_sum += power.ln();
            power_sum += power;
        }

        // 4. Centroide espectral normalizado (0.0 = DC, 1.0 = Nyquist)
        if sum_magnitude > 1e-5 {
            self.target_centroid = ((weighted_sum / sum_magnitude) / ((num_bins - 1) as f32)).clamp(0.0, 1.0);
        }

        // 5. Flujo espectral normalizado
        let raw_flux = flux_diff_sum.sqrt();
        self.target_flux = (raw_flux * 0.1).clamp(0.0, 1.0);

        // 6. Planitud espectral (Wiener entropy)
        let geom_mean = (log_power_sum / (num_bins as f32)).exp();
        let arith_mean = power_sum / (num_bins as f32);
        self.flatness = (geom_mean / (arith_mean + 1e-9)).clamp(0.0, 1.0);

        // Energía total RMS del frame
        self.energy = ((power_sum / (num_bins as f32)).sqrt()).clamp(0.0, 1.0);

        // Guardar para la siguiente iteración
        self.prev_magnitudes.copy_from_slice(&self.current_magnitudes);
    }
}
