//! Sovereign Real-Time Audio Analysis Engine Core (Rules 1, 7, 9, 34, 46, 47)
//! Unifies multichannel downmixing, silence gating (< -70.45 dBFS / 0.0003 RMS),
//! and coordinates TransientDetector, PitchTracker, and SpectralFeatureExtractor
//! into a single high-performance Rust processing pass with zero allocations.

use crate::common::denormal_guard;
use crate::transient_detector::TransientDetectorCore;
use crate::pitch_tracker::PitchTrackerCore;
use crate::spectral_feature_extractor::RustSpectralFeatureExtractor;

#[repr(C)]
#[derive(Clone, Copy, Debug)]
pub struct RustAnalysisSnapshot {
    pub onset_strength: f32,
    pub is_transient: bool,
    pub _pad: [u8; 3],
    pub pitch_hz: f32,
    pub pitch_clarity: f32,
    pub clarity: f32,
    pub pitch_midi: f32,
    pub pitch_normalized: f32,
    pub spectral_centroid: f32,
    pub spectral_flux: f32,
    pub spectral_flatness: f32,
    pub energy: f32,
}

impl Default for RustAnalysisSnapshot {
    fn default() -> Self {
        Self {
            onset_strength: 0.0,
            is_transient: false,
            _pad: [0; 3],
            pitch_hz: 440.0,
            pitch_clarity: 0.0,
            clarity: 0.0,
            pitch_midi: 69.0,
            pitch_normalized: 0.5,
            spectral_centroid: 0.2,
            spectral_flux: 0.0,
            spectral_flatness: 0.0,
            energy: 0.0,
        }
    }
}

pub struct RustAnalysisEngine {
    sample_rate: f64,
    mono_buffer: Box<[f32]>,
    pub transient_detector: TransientDetectorCore,
    pub pitch_tracker: PitchTrackerCore,
    pub spectral_extractor: RustSpectralFeatureExtractor,
    snapshot: RustAnalysisSnapshot,
    was_silent: bool,
}

impl RustAnalysisEngine {
    pub fn new(sample_rate: f64, max_block_size: usize) -> Self {
        let sr = if sample_rate > 0.0 { sample_rate } else { 44100.0 };
        let bs = max_block_size.max(512);

        let mut td = TransientDetectorCore::new(sr as f32);
        td.prepare(sr as f32);

        let mut pt = PitchTrackerCore::new(sr as f32);
        pt.prepare(sr as f32);

        let mut se = RustSpectralFeatureExtractor::new();
        se.prepare(sr, 1024, 256);

        Self {
            sample_rate: sr,
            mono_buffer: vec![0.0f32; bs].into_boxed_slice(),
            transient_detector: td,
            pitch_tracker: pt,
            spectral_extractor: se,
            snapshot: RustAnalysisSnapshot::default(),
            was_silent: true,
        }
    }

    pub fn prepare(&mut self, sample_rate: f64, max_block_size: usize) {
        self.sample_rate = if sample_rate > 0.0 { sample_rate } else { 44100.0 };
        let bs = max_block_size.max(512);

        if self.mono_buffer.len() < bs {
            self.mono_buffer = vec![0.0f32; bs].into_boxed_slice();
        }

        self.transient_detector.prepare(self.sample_rate as f32);
        self.pitch_tracker.prepare(self.sample_rate as f32);
        self.spectral_extractor.prepare(self.sample_rate, 1024, 256);

        self.reset();
    }

    pub fn reset(&mut self) {
        self.mono_buffer.fill(0.0);
        self.transient_detector.reset();
        self.pitch_tracker.reset();
        self.spectral_extractor.reset();
        self.snapshot = RustAnalysisSnapshot::default();
        self.was_silent = true;
    }

    pub fn process(
        &mut self,
        input_channels: *const *const f32,
        num_channels: u32,
        num_samples: u32,
    ) {
        if input_channels.is_null() || num_channels == 0 || num_samples == 0 {
            self.snapshot.is_transient = false;
            self.snapshot.onset_strength = 0.0;
            self.snapshot.pitch_clarity = 0.0;
            self.snapshot.clarity = 0.0;
            self.snapshot.energy = 0.0;
            self.snapshot.spectral_flux = 0.0;
            self.snapshot.spectral_flatness = 0.0;
            self.was_silent = true;
            return;
        }

        let samples_to_process = (num_samples as usize).min(self.mono_buffer.len());
        if samples_to_process == 0 {
            return;
        }

        // 1. Suma mono para análisis neutral y cálculo de energía RMS
        let mut sum_sq = 0.0f32;
        unsafe {
            let ch0 = *input_channels;
            if ch0.is_null() {
                self.was_silent = true;
                return;
            }

            if num_channels == 1 || (*input_channels.add(1)).is_null() {
                let in0 = std::slice::from_raw_parts(ch0, samples_to_process);
                for s in 0..samples_to_process {
                    let val = denormal_guard(in0[s]);
                    self.mono_buffer[s] = val;
                    sum_sq += val * val;
                }
            } else {
                let in0 = std::slice::from_raw_parts(ch0, samples_to_process);
                let in1 = std::slice::from_raw_parts(*input_channels.add(1), samples_to_process);
                for s in 0..samples_to_process {
                    let val = denormal_guard(0.5 * (in0[s] + in1[s]));
                    self.mono_buffer[s] = val;
                    sum_sq += val * val;
                }
            }
        }

        let rms = if samples_to_process > 0 {
            (sum_sq / (samples_to_process as f32)).sqrt()
        } else {
            0.0
        };

        // Silence gating: threshold < -70.45 dBFS (0.0003 RMS) (Rule 47)
        let is_silent = rms < 0.0003;

        if is_silent {
            // Silence Gating: decaimiento suave en transientes y bypass completo de YIN y FFT
            self.transient_detector.process(None, 0);

            self.snapshot.is_transient = false;
            self.snapshot.onset_strength = 0.0;
            self.snapshot.pitch_clarity = 0.0;
            self.snapshot.clarity = 0.0;
            self.snapshot.energy = 0.0;
            self.snapshot.spectral_flux = 0.0;
            self.snapshot.spectral_flatness = 0.0;
            // Preservar pitch_hz, pitch_midi, pitch_normalized y spectral_centroid
            // para evitar saltos o discontinuidades audibles en parámetros modulados (Regla 35)
            self.was_silent = true;
            return;
        }

        // Si el audio pasa de silencio a activo, reiniciar suavemente el detector para evitar falsos transitorios de borde
        if self.was_silent {
            self.transient_detector.reset();
            self.was_silent = false;
        }

        // 2. Ejecutar analizadores en audio activo
        let mono_slice = &self.mono_buffer[..samples_to_process];
        self.transient_detector.process(Some(mono_slice), samples_to_process);
        self.pitch_tracker.process(Some(mono_slice), samples_to_process);
        self.spectral_extractor.process(mono_slice, false);

        // 3. Actualizar snapshot atómico/thread-safe para modulación y GUI
        self.snapshot.onset_strength = self.transient_detector.get_onset_strength();
        self.snapshot.is_transient = self.transient_detector.is_transient_detected();

        self.snapshot.pitch_hz = self.pitch_tracker.get_fundamental_hz();
        self.snapshot.pitch_clarity = self.pitch_tracker.get_clarity();
        self.snapshot.clarity = self.pitch_tracker.get_clarity();
        self.snapshot.pitch_midi = self.pitch_tracker.get_midi_note();
        self.snapshot.pitch_normalized = self.pitch_tracker.get_pitch_normalized();

        self.snapshot.spectral_centroid = self.spectral_extractor.get_spectral_centroid();
        self.snapshot.spectral_flux = self.spectral_extractor.get_spectral_flux();
        self.snapshot.spectral_flatness = self.spectral_extractor.get_spectral_flatness();
        self.snapshot.energy = self.spectral_extractor.get_energy();
    }

    #[inline]
    pub fn get_snapshot(&self) -> RustAnalysisSnapshot {
        self.snapshot
    }
}
