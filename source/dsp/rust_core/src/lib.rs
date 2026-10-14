//! n8_rust_core - High-Performance Acoustic Physical Waveguide & Non-Linear Resonator
//! Developed for the N8Effect Hybrid C++ / Rust Audio Engine Architecture (Rules 5, 8, 9, 14, 46, 47).
//! Zero allocations in audio thread, cacheline alignment, and aggressive AVX2 vectorization.

pub mod common;
pub mod karplus_strong;
pub mod modal_resonator;
pub mod tape_saturation;
pub mod shimmer_reverb;
pub mod refraction;
pub mod dynamic_eq;
pub mod transient_shaper;
pub mod granular;
pub mod audio_slicer;
pub mod vocoder;
pub mod rotary_speaker;
pub mod harmonic_exciter;
pub mod brickwall_limiter;
pub mod multiband_dynamics;
pub mod resonator_bank;
pub mod bitcrusher;
pub mod noise_gate;
pub mod de_esser;
pub mod auto_wah;
pub mod compressor;
pub mod parametric_eq;
pub mod stereo_imager;
pub mod phaser;
pub mod chorus;
pub mod flanger;
pub mod ring_modulator;
pub mod frequency_shifter;
pub mod distortion;
pub mod formant_filter;
pub mod noise_texture;
pub mod spatial_panner;
pub mod tape_stop;
pub mod lofi_cassette;
pub mod advanced_delay;
pub mod reverb;
pub mod reverse_reverb;
pub mod glitch;
pub mod mid_side_encoder;
pub mod mid_side_decoder;
pub mod fft;
pub mod spectral_processor;
pub mod spectral_freeze;
pub mod spectral_smear;
pub mod pitch_shifter;
pub mod sample_player;
pub mod convolution;
pub mod oversampler;
pub mod simple_filter;
pub mod simple_delay;
pub mod passthrough;
pub mod external_sidechain;
pub mod midi_scale_quantizer;
pub mod midi_arpeggiator;
pub mod midi_chord_engine;
pub mod feedback_container;
pub mod transient_detector;
pub mod pitch_tracker;
pub mod spectral_feature_extractor;
pub mod analysis_engine;
pub mod lfo;
pub mod envelope_generator;
pub mod audio_follower;
pub mod chaos_modulator;
pub mod step_sequencer;
pub mod euclidean_modulator;
pub mod mseg_modulator;
pub mod modulation_matrix;
pub mod modulation_engine;
pub mod event_types;
pub mod event_capture_buffer;
pub mod audio_fragment;
pub mod event;
pub mod event_pool;
pub mod event_telemetry;
pub mod event_manager;
pub mod event_container;
pub mod graph_engine;
pub mod scene_morph;
pub mod optimization;
pub mod dimension_chorus;
pub mod dynamic_tremolo;
pub mod multiband_saturator;
pub mod comb_resonator;
pub mod pitch_delay;
pub mod phase_dispersion;
pub mod morphing_svf;
pub mod fractal_resonator;
pub mod vactrol_lpg;
pub mod barberpole_filter;
pub mod sidechain_pumper;
pub mod frequency_split_panner;
pub mod chebyshev_harmonic_shaper;
pub mod subharmonic_synthesizer;
pub mod transaural_crosstalk_canceller;
pub mod doppler_orbital_spatializer;
pub mod granular_buffer_shuffler;
pub mod disintegration_looper;
pub mod spring_reverb_tank;
pub mod dual_bode_frequency_shifter;
pub mod microtonal_harmonizer;
pub mod formant_pitch_shifter;
pub mod spectral_resonance_tamer;
pub mod spectral_morphing_convolver;
pub mod graph_undo_manager;
pub mod user_preset_bank;
pub mod preset_packager;
pub mod node_automation_sequencer;
pub mod midi_mapping_manager;
pub mod mpe_manager;
pub mod preset_manager;
pub mod graph_serializer;
pub mod factory_preset_catalog;
pub mod work_stealing_scheduler;
pub mod adaptive_noise_floor_estimator;
pub mod delay_line;
pub mod linkwitz_riley_crossover;
pub mod container_node;

pub use container_node::RustContainerCore;
pub use dimension_chorus::RustDimensionChorus;
pub use dynamic_tremolo::RustDynamicTremolo;
pub use multiband_saturator::RustMultibandSaturator;
pub use comb_resonator::RustCombResonator;
pub use pitch_delay::RustPitchDelay;
pub use phase_dispersion::RustPhaseDispersion;
pub use morphing_svf::RustMorphingSvf;
pub use fractal_resonator::RustFractalResonator;
pub use vactrol_lpg::RustVactrolLPG;
pub use barberpole_filter::RustBarberpoleFilter;
pub use sidechain_pumper::RustSidechainPumper;
pub use frequency_split_panner::RustFrequencySplitPanner;
pub use chebyshev_harmonic_shaper::RustChebyshevHarmonicShaper;
pub use subharmonic_synthesizer::RustSubharmonicSynthesizer;
pub use transaural_crosstalk_canceller::RustTransauralCrosstalkCanceller;
pub use doppler_orbital_spatializer::RustDopplerOrbitalSpatializer;
pub use granular_buffer_shuffler::RustGranularBufferShuffler;
pub use disintegration_looper::RustDisintegrationLooper;
pub use spring_reverb_tank::RustSpringReverbTank;
pub use dual_bode_frequency_shifter::RustDualBodeFrequencyShifter;
pub use microtonal_harmonizer::RustMicrotonalHarmonizer;
pub use formant_pitch_shifter::RustFormantPitchShifter;
pub use spectral_resonance_tamer::RustSpectralResonanceTamer;
pub use spectral_morphing_convolver::RustSpectralMorphingConvolver;
pub use graph_undo_manager::{RustGraphUndoManager, RustUndoCheckpoint, RustActionCategory};
pub use user_preset_bank::{RustUserPresetBank, RustUserPresetRecord};
pub use preset_packager::{RustPackManifest, RustPackPresetEntry, RustPresetPackager, RustPackBuilder, RustPackExtractor};
pub use node_automation_sequencer::{RustNodeAutomationBank, RustAutomationLane};
pub use midi_mapping_manager::{RustMidiCurve, RustMidiMapping, RustMidiDispatchItem, RustMidiMappingManager};
pub use mpe_manager::{RustZoneMode, RustMpeVoice, RustMpeActionType, RustMpeAction, RustMpeManager};
pub use preset_manager::{RustPresetSlot, RustPresetSnapshot, RustPresetManager};
pub use graph_serializer::{RustPresetMetadata, RustGraphSerializer};
pub use factory_preset_catalog::{RustFactoryPresetEntry, RustFactoryPresetCatalog};
pub use work_stealing_scheduler::{RustWorkStealingQueue, RustWorkStealingScheduler};
pub use adaptive_noise_floor_estimator::RustAdaptiveNoiseFloorEstimator;
pub use delay_line::RustDelayLine;
pub use linkwitz_riley_crossover::RustLinkwitzRileyCrossover3Way;

use std::ffi::{CStr, c_char};




pub use event_types::{EventType, EventLifecycle, SourceDisappearanceMode, RustEventAttributes, RustEventTelemetryItem};
pub use event_capture_buffer::RustEventCaptureBuffer;
pub use audio_fragment::RustAudioFragment;
pub use event::RustEvent;
pub use event_pool::RustEventPool;
pub use event_telemetry::RustEventTelemetryBuffer;
pub use event_manager::RustEventManager;
pub use graph_engine::{
    RustGraphConnection, RustNodeLayout, RustExecutionStepDesc, RustGraphPlan, RustGraphPlanExchanger,
    rust_would_create_cycle, rust_validate_and_topological_sort,
    rust_compute_topological_depths, rust_compute_auto_layout,
};
pub use scene_morph::{
    RustParamEntry, RustNodeParamEntry, RustSceneSnapshot, RustSceneMorphEngine, RustSmartRandomizer,
};
pub use optimization::{
    RustStepSlotAssignment, rust_enable_hardware_ftz_daz, rust_restore_hardware_mxcsr, rust_is_ftz_daz_active,
    fast_pade_tanh, fast_soft_clip, fast_exp_negative, fast_db_to_gain,
    vec_pade_tanh, vec_multiply_add, vec_apply_gain, vec_mix, vec_flush_denormals, vec_soft_clip,
    rust_compute_optimal_buffer_slots,
};

pub use external_sidechain::ExternalSidechainCore;
pub use midi_scale_quantizer::MidiScaleQuantizerCore;
pub use midi_arpeggiator::MidiArpeggiatorCore;
pub use midi_chord_engine::MidiChordEngineCore;
pub use feedback_container::FeedbackContainerCore;
pub use event_container::EventContainerCore;
pub use transient_detector::TransientDetectorCore;
pub use pitch_tracker::PitchTrackerCore;
pub use spectral_feature_extractor::RustSpectralFeatureExtractor;
pub use analysis_engine::{RustAnalysisEngine, RustAnalysisSnapshot};
pub use lfo::{RustLFO, LFOShape, SyncDivision};
pub use envelope_generator::{RustEnvelopeGenerator, EnvelopeState};
pub use audio_follower::RustAudioFollower;
pub use chaos_modulator::RustChaosModulator;
pub use step_sequencer::{RustStepSequencer, SequencerStep};
pub use euclidean_modulator::RustEuclideanModulator;
pub use mseg_modulator::{RustMSEGModulator, MSEGPoint, MSEGPlayMode};
pub use modulation_matrix::{RustMacroManager, RustModulationMatrix, RustModulationRoute};
pub use modulation_engine::RustModulationEngine;
pub use karplus_strong::RustKarplusStrong;
pub use modal_resonator::RustModalResonator;
pub use tape_saturation::RustTapeSaturation;
pub use shimmer_reverb::RustShimmerReverb;
pub use refraction::RustRefraction;
pub use dynamic_eq::RustDynamicEq;
pub use transient_shaper::RustTransientShaper;
pub use granular::{RustGranular, RustGrainCloudPoint};
pub use audio_slicer::RustAudioSlicer;
pub use vocoder::RustVocoder;
pub use rotary_speaker::RustRotarySpeaker;
pub use harmonic_exciter::RustHarmonicExciter;
pub use brickwall_limiter::RustBrickwallLimiter;
pub use multiband_dynamics::RustMultibandDynamics;
pub use resonator_bank::RustResonatorBank;
pub use bitcrusher::RustBitcrusher;
pub use noise_gate::RustNoiseGate;
pub use de_esser::RustDeEsser;
pub use auto_wah::RustAutoWah;
pub use compressor::RustCompressor;
pub use parametric_eq::RustParametricEq;
pub use stereo_imager::RustStereoImager;
pub use phaser::RustPhaser;
pub use chorus::RustChorus;
pub use flanger::RustFlanger;
pub use ring_modulator::RustRingModulator;
pub use frequency_shifter::RustFrequencyShifter;
pub use distortion::RustDistortion;
pub use formant_filter::RustFormantFilter;
pub use noise_texture::RustNoiseTexture;
pub use spatial_panner::RustSpatialPanner;
pub use tape_stop::RustTapeStop;
pub use lofi_cassette::RustLoFiCassette;
pub use advanced_delay::RustAdvancedDelay;
pub use reverb::RustReverb;
pub use reverse_reverb::RustReverseReverb;
pub use glitch::RustGlitch;
pub use mid_side_encoder::RustMidSideEncoder;
pub use mid_side_decoder::RustMidSideDecoder;
pub use spectral_processor::RustSpectralProcessor;
pub use spectral_freeze::RustSpectralFreeze;
pub use spectral_smear::RustSpectralSmear;
pub use pitch_shifter::RustPitchShifter;
pub use sample_player::RustSamplePlayer;
pub use convolution::RustConvolution;
pub use oversampler::RustOversampler;
pub use simple_filter::RustSimpleFilter;
pub use simple_delay::RustSimpleDelay;
pub use passthrough::RustPassthrough;

pub const BUFFER_SIZE: usize = 16384;
pub const BUFFER_MASK: usize = BUFFER_SIZE - 1;

#[repr(C)]
pub struct RustWaveguide {
    sample_rate: f32,

    // Bounded static circular delay buffers
    buffer_l: [f32; BUFFER_SIZE],
    buffer_r: [f32; BUFFER_SIZE],
    write_pos: usize,

    // Filter states (one-pole damping loop filters)
    filter_state_l: f32,
    filter_state_r: f32,

    // Parameters
    param_freq: f32,         // 1: 20 Hz .. 2000 Hz
    param_decay: f32,        // 2: 0.1 .. 0.999
    param_brightness: f32,   // 3: 0.0 .. 1.0 (filter cutoff)
    param_non_linearity: f32,// 4: 0.0 .. 1.0 (waveshaper drive)
    param_coupling: f32,     // 5: 0.0 .. 1.0 (cross-talk between strings)
    param_mix: f32,          // 6: 0.0 .. 1.0 (dry/wet mix)
}

impl RustWaveguide {
    pub fn new(sample_rate: f32) -> Self {
        let sr = if sample_rate > 0.0 { sample_rate } else { 48000.0 };
        Self {
            sample_rate: sr,
            buffer_l: [0.0; BUFFER_SIZE],
            buffer_r: [0.0; BUFFER_SIZE],
            write_pos: 0,
            filter_state_l: 0.0,
            filter_state_r: 0.0,
            param_freq: 220.0,
            param_decay: 0.92,
            param_brightness: 0.65,
            param_non_linearity: 0.25,
            param_coupling: 0.30,
            param_mix: 0.70,
        }
    }

    #[inline(always)]
    pub fn reset(&mut self) {
        self.buffer_l.fill(0.0);
        self.buffer_r.fill(0.0);
        self.write_pos = 0;
        self.filter_state_l = 0.0;
        self.filter_state_r = 0.0;
    }

    #[inline(always)]
    pub fn set_sample_rate(&mut self, sample_rate: f32) {
        if sample_rate > 0.0 {
            self.sample_rate = sample_rate;
        }
    }

    #[inline(always)]
    fn fast_tanh(x: f32) -> f32 {
        let x2 = x * x;
        x * (27.0 + x2) / (27.0 + 9.0 * x2)
    }

    #[inline(always)]
    fn read_interpolated(&self, buffer: &[f32; BUFFER_SIZE], delay_samples: f32) -> f32 {
        let d = delay_samples.clamp(1.0, (BUFFER_SIZE - 4) as f32);
        let int_delay = d as usize;
        let frac = d - (int_delay as f32);

        let idx0 = (self.write_pos + BUFFER_SIZE - int_delay) & BUFFER_MASK;
        let idx1 = (idx0 + BUFFER_SIZE - 1) & BUFFER_MASK;

        // Linear interpolation for pure efficiency
        let s0 = buffer[idx0];
        let s1 = buffer[idx1];
        s0 + frac * (s1 - s0)
    }

    pub fn process_block(
        &mut self,
        in_l: &[f32],
        in_r: &[f32],
        out_l: &mut [f32],
        out_r: &mut [f32],
    ) {
        let n_samples = in_l.len().min(out_l.len());
        if n_samples == 0 {
            return;
        }

        let delay_len = (self.sample_rate / self.param_freq.clamp(20.0, 2000.0)).clamp(2.0, (BUFFER_SIZE - 4) as f32);
        let feedback = self.param_decay.clamp(0.01, 0.998);
        let damp_cutoff = self.param_brightness.clamp(0.05, 0.95);
        let drive = 1.0 + self.param_non_linearity.clamp(0.0, 1.0) * 3.0;
        let coupling = self.param_coupling.clamp(0.0, 0.8) * 0.5;
        let mix = self.param_mix.clamp(0.0, 1.0);
        let dry = 1.0 - mix;

        // Anti-denormal constant
        const DENORMAL_GUARD: f32 = 1.0e-18;

        for i in 0..n_samples {
            let x_l = in_l[i];
            let x_r = if i < in_r.len() { in_r[i] } else { x_l };

            // 1. Read delayed signal from waveguides
            let del_l = self.read_interpolated(&self.buffer_l, delay_len);
            let del_r = self.read_interpolated(&self.buffer_r, delay_len * 1.004); // subtle detune between strings

            // 2. Coupling scattering junction
            let coupled_l = del_l * (1.0 - coupling) + del_r * coupling;
            let coupled_r = del_r * (1.0 - coupling) + del_l * coupling;

            // 3. One-pole lowpass damping filter
            self.filter_state_l += damp_cutoff * (coupled_l - self.filter_state_l) + DENORMAL_GUARD;
            self.filter_state_r += damp_cutoff * (coupled_r - self.filter_state_r) + DENORMAL_GUARD;

            // 4. Non-linear saturation waveshaping
            let sat_l = Self::fast_tanh(self.filter_state_l * drive) / drive;
            let sat_r = Self::fast_tanh(self.filter_state_r * drive) / drive;

            // 5. Write back to waveguides (feedback loop + input excitation)
            let next_l = x_l + sat_l * feedback;
            let next_r = x_r + sat_r * feedback;

            self.buffer_l[self.write_pos] = next_l;
            self.buffer_r[self.write_pos] = next_r;
            self.write_pos = (self.write_pos + 1) & BUFFER_MASK;

            // 6. Output Dry/Wet mix
            out_l[i] = x_l * dry + sat_l * mix;
            if i < out_r.len() {
                out_r[i] = x_r * dry + sat_r * mix;
            }
        }
    }
}

// ==============================================================================
// Safe C-ABI Exports (Reglas 9, 13, 46)
// ==============================================================================

// --- RustWaveguide ---

#[no_mangle]
pub unsafe extern "C" fn n8_rust_waveguide_create(sample_rate: f64) -> *mut RustWaveguide {
    let instance = Box::new(RustWaveguide::new(sample_rate as f32));
    Box::into_raw(instance)
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_waveguide_destroy(ptr: *mut RustWaveguide) {
    if !ptr.is_null() {
        drop(Box::from_raw(ptr));
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_waveguide_reset(ptr: *mut RustWaveguide) {
    if let Some(w) = ptr.as_mut() {
        w.reset();
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_waveguide_set_sample_rate(ptr: *mut RustWaveguide, sample_rate: f64) {
    if let Some(w) = ptr.as_mut() {
        w.set_sample_rate(sample_rate as f32);
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_waveguide_set_parameter(ptr: *mut RustWaveguide, param_id: u32, value: f32) {
    if let Some(w) = ptr.as_mut() {
        match param_id {
            1 => w.param_freq = value.clamp(20.0, 2000.0),
            2 => w.param_decay = value.clamp(0.01, 0.999),
            3 => w.param_brightness = value.clamp(0.0, 1.0),
            4 => w.param_non_linearity = value.clamp(0.0, 1.0),
            5 => w.param_coupling = value.clamp(0.0, 1.0),
            6 => w.param_mix = value.clamp(0.0, 1.0),
            _ => {}
        }
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_waveguide_get_parameter(ptr: *const RustWaveguide, param_id: u32) -> f32 {
    if let Some(w) = ptr.as_ref() {
        match param_id {
            1 => w.param_freq,
            2 => w.param_decay,
            3 => w.param_brightness,
            4 => w.param_non_linearity,
            5 => w.param_coupling,
            6 => w.param_mix,
            _ => 0.0,
        }
    } else {
        0.0
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_waveguide_process(
    ptr: *mut RustWaveguide,
    in_l: *const f32,
    in_r: *const f32,
    out_l: *mut f32,
    out_r: *mut f32,
    num_samples: u32,
) {
    if ptr.is_null() || in_l.is_null() || out_l.is_null() || num_samples == 0 {
        return;
    }

    let w = &mut *ptr;
    let n = num_samples as usize;

    let is_in_place = std::ptr::eq(in_l, out_l as *const f32)
        || (!in_r.is_null() && !out_r.is_null() && std::ptr::eq(in_r, out_r as *const f32));

    if is_in_place {
        let mut scratch_in_l = [0.0f32; 1024];
        let mut scratch_in_r = [0.0f32; 1024];
        let mut scratch_out_r = [0.0f32; 1024];
        let is_mono_out = out_r.is_null() || std::ptr::eq(out_l, out_r);

        let mut offset = 0;
        while offset < n {
            let chunk_size = (n - offset).min(1024);
            std::ptr::copy_nonoverlapping(in_l.add(offset), scratch_in_l.as_mut_ptr(), chunk_size);
            if !in_r.is_null() {
                std::ptr::copy_nonoverlapping(in_r.add(offset), scratch_in_r.as_mut_ptr(), chunk_size);
            } else {
                scratch_in_r[..chunk_size].copy_from_slice(&scratch_in_l[..chunk_size]);
            }

            let chunk_out_l = std::slice::from_raw_parts_mut(out_l.add(offset), chunk_size);
            if !is_mono_out {
                let chunk_out_r = std::slice::from_raw_parts_mut(out_r.add(offset), chunk_size);
                w.process_block(
                    &scratch_in_l[..chunk_size],
                    &scratch_in_r[..chunk_size],
                    chunk_out_l,
                    chunk_out_r,
                );
            } else {
                w.process_block(
                    &scratch_in_l[..chunk_size],
                    &scratch_in_r[..chunk_size],
                    chunk_out_l,
                    &mut scratch_out_r[..chunk_size],
                );
            }
            offset += chunk_size;
        }
        return;
    }

    let in_l_slice = std::slice::from_raw_parts(in_l, n);
    let in_r_slice = if !in_r.is_null() {
        std::slice::from_raw_parts(in_r, n)
    } else {
        in_l_slice
    };

    if !out_r.is_null() && !std::ptr::eq(out_l, out_r) {
        let out_l_slice = std::slice::from_raw_parts_mut(out_l, n);
        let out_r_slice = std::slice::from_raw_parts_mut(out_r, n);
        w.process_block(in_l_slice, in_r_slice, out_l_slice, out_r_slice);
    } else {
        let mut mono_scratch = [0.0f32; 1024];
        let mut offset = 0;
        let out_l_slice = std::slice::from_raw_parts_mut(out_l, n);
        while offset < n {
            let chunk_size = (n - offset).min(1024);
            let chunk_in_l = &in_l_slice[offset..offset + chunk_size];
            let chunk_in_r = &in_r_slice[offset..offset + chunk_size];
            let chunk_out_l = &mut out_l_slice[offset..offset + chunk_size];
            w.process_block(chunk_in_l, chunk_in_r, chunk_out_l, &mut mono_scratch[..chunk_size]);
            offset += chunk_size;
        }
    }
}

// --- RustKarplusStrong ---

#[no_mangle]
pub unsafe extern "C" fn n8_rust_karplus_create(sample_rate: f64) -> *mut RustKarplusStrong {
    let instance = Box::new(RustKarplusStrong::new(sample_rate as f32));
    Box::into_raw(instance)
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_karplus_destroy(ptr: *mut RustKarplusStrong) {
    if !ptr.is_null() {
        drop(Box::from_raw(ptr));
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_karplus_reset(ptr: *mut RustKarplusStrong) {
    if let Some(k) = ptr.as_mut() {
        k.reset();
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_karplus_set_sample_rate(ptr: *mut RustKarplusStrong, sample_rate: f64) {
    if let Some(k) = ptr.as_mut() {
        k.set_sample_rate(sample_rate as f32);
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_karplus_set_parameter(ptr: *mut RustKarplusStrong, param_id: u32, value: f32) {
    if let Some(k) = ptr.as_mut() {
        k.set_parameter(param_id, value);
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_karplus_get_parameter(ptr: *const RustKarplusStrong, param_id: u32) -> f32 {
    if let Some(k) = ptr.as_ref() {
        k.get_parameter(param_id)
    } else {
        0.0
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_karplus_process(
    ptr: *mut RustKarplusStrong,
    in_l: *const f32,
    in_r: *const f32,
    out_l: *mut f32,
    out_r: *mut f32,
    num_samples: u32,
) {
    if ptr.is_null() || in_l.is_null() || out_l.is_null() || num_samples == 0 {
        return;
    }

    let k = &mut *ptr;
    let n = num_samples as usize;

    let is_in_place = std::ptr::eq(in_l, out_l as *const f32)
        || (!in_r.is_null() && !out_r.is_null() && std::ptr::eq(in_r, out_r as *const f32));

    if is_in_place {
        let mut scratch_in_l = [0.0f32; 1024];
        let mut scratch_in_r = [0.0f32; 1024];
        let mut scratch_out_r = [0.0f32; 1024];
        let is_mono_out = out_r.is_null() || std::ptr::eq(out_l, out_r);

        let mut offset = 0;
        while offset < n {
            let chunk_size = (n - offset).min(1024);
            std::ptr::copy_nonoverlapping(in_l.add(offset), scratch_in_l.as_mut_ptr(), chunk_size);
            if !in_r.is_null() {
                std::ptr::copy_nonoverlapping(in_r.add(offset), scratch_in_r.as_mut_ptr(), chunk_size);
            } else {
                scratch_in_r[..chunk_size].copy_from_slice(&scratch_in_l[..chunk_size]);
            }

            let chunk_out_l = std::slice::from_raw_parts_mut(out_l.add(offset), chunk_size);
            if !is_mono_out {
                let chunk_out_r = std::slice::from_raw_parts_mut(out_r.add(offset), chunk_size);
                k.process_block(
                    &scratch_in_l[..chunk_size],
                    &scratch_in_r[..chunk_size],
                    chunk_out_l,
                    chunk_out_r,
                );
            } else {
                k.process_block(
                    &scratch_in_l[..chunk_size],
                    &scratch_in_r[..chunk_size],
                    chunk_out_l,
                    &mut scratch_out_r[..chunk_size],
                );
            }
            offset += chunk_size;
        }
        return;
    }

    let in_l_slice = std::slice::from_raw_parts(in_l, n);
    let in_r_slice = if !in_r.is_null() {
        std::slice::from_raw_parts(in_r, n)
    } else {
        in_l_slice
    };

    if !out_r.is_null() && !std::ptr::eq(out_l, out_r) {
        let out_l_slice = std::slice::from_raw_parts_mut(out_l, n);
        let out_r_slice = std::slice::from_raw_parts_mut(out_r, n);
        k.process_block(in_l_slice, in_r_slice, out_l_slice, out_r_slice);
    } else {
        let mut mono_scratch = [0.0f32; 1024];
        let mut offset = 0;
        let out_l_slice = std::slice::from_raw_parts_mut(out_l, n);
        while offset < n {
            let chunk_size = (n - offset).min(1024);
            let chunk_in_l = &in_l_slice[offset..offset + chunk_size];
            let chunk_in_r = &in_r_slice[offset..offset + chunk_size];
            let chunk_out_l = &mut out_l_slice[offset..offset + chunk_size];
            k.process_block(chunk_in_l, chunk_in_r, chunk_out_l, &mut mono_scratch[..chunk_size]);
            offset += chunk_size;
        }
    }
}

// --- RustModalResonator ---

#[no_mangle]
pub unsafe extern "C" fn n8_rust_modal_resonator_create(sample_rate: f64) -> *mut RustModalResonator {
    let instance = Box::new(RustModalResonator::new(sample_rate as f32));
    Box::into_raw(instance)
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_modal_resonator_destroy(ptr: *mut RustModalResonator) {
    if !ptr.is_null() {
        drop(Box::from_raw(ptr));
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_modal_resonator_reset(ptr: *mut RustModalResonator) {
    if let Some(m) = ptr.as_mut() {
        m.reset();
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_modal_resonator_set_sample_rate(ptr: *mut RustModalResonator, sample_rate: f64) {
    if let Some(m) = ptr.as_mut() {
        m.set_sample_rate(sample_rate as f32);
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_modal_resonator_set_parameter(ptr: *mut RustModalResonator, param_id: u32, value: f32) {
    if let Some(m) = ptr.as_mut() {
        m.set_parameter(param_id, value);
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_modal_resonator_get_parameter(ptr: *const RustModalResonator, param_id: u32) -> f32 {
    if let Some(m) = ptr.as_ref() {
        m.get_parameter(param_id)
    } else {
        0.0
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_modal_resonator_process(
    ptr: *mut RustModalResonator,
    in_l: *const f32,
    in_r: *const f32,
    out_l: *mut f32,
    out_r: *mut f32,
    num_samples: u32,
) {
    if ptr.is_null() || in_l.is_null() || out_l.is_null() || num_samples == 0 {
        return;
    }

    let m = &mut *ptr;
    let n = num_samples as usize;

    let is_in_place = std::ptr::eq(in_l, out_l as *const f32)
        || (!in_r.is_null() && !out_r.is_null() && std::ptr::eq(in_r, out_r as *const f32));

    if is_in_place {
        let mut scratch_in_l = [0.0f32; 1024];
        let mut scratch_in_r = [0.0f32; 1024];
        let mut scratch_out_r = [0.0f32; 1024];
        let is_mono_out = out_r.is_null() || std::ptr::eq(out_l, out_r);

        let mut offset = 0;
        while offset < n {
            let chunk_size = (n - offset).min(1024);
            std::ptr::copy_nonoverlapping(in_l.add(offset), scratch_in_l.as_mut_ptr(), chunk_size);
            if !in_r.is_null() {
                std::ptr::copy_nonoverlapping(in_r.add(offset), scratch_in_r.as_mut_ptr(), chunk_size);
            } else {
                scratch_in_r[..chunk_size].copy_from_slice(&scratch_in_l[..chunk_size]);
            }

            let chunk_out_l = std::slice::from_raw_parts_mut(out_l.add(offset), chunk_size);
            if !is_mono_out {
                let chunk_out_r = std::slice::from_raw_parts_mut(out_r.add(offset), chunk_size);
                m.process_block(
                    &scratch_in_l[..chunk_size],
                    &scratch_in_r[..chunk_size],
                    chunk_out_l,
                    chunk_out_r,
                );
            } else {
                m.process_block(
                    &scratch_in_l[..chunk_size],
                    &scratch_in_r[..chunk_size],
                    chunk_out_l,
                    &mut scratch_out_r[..chunk_size],
                );
            }
            offset += chunk_size;
        }
        return;
    }

    let in_l_slice = std::slice::from_raw_parts(in_l, n);
    let in_r_slice = if !in_r.is_null() {
        std::slice::from_raw_parts(in_r, n)
    } else {
        in_l_slice
    };

    if !out_r.is_null() && !std::ptr::eq(out_l, out_r) {
        let out_l_slice = std::slice::from_raw_parts_mut(out_l, n);
        let out_r_slice = std::slice::from_raw_parts_mut(out_r, n);
        m.process_block(in_l_slice, in_r_slice, out_l_slice, out_r_slice);
    } else {
        let mut mono_scratch = [0.0f32; 1024];
        let mut offset = 0;
        let out_l_slice = std::slice::from_raw_parts_mut(out_l, n);
        while offset < n {
            let chunk_size = (n - offset).min(1024);
            let chunk_in_l = &in_l_slice[offset..offset + chunk_size];
            let chunk_in_r = &in_r_slice[offset..offset + chunk_size];
            let chunk_out_l = &mut out_l_slice[offset..offset + chunk_size];
            m.process_block(chunk_in_l, chunk_in_r, chunk_out_l, &mut mono_scratch[..chunk_size]);
            offset += chunk_size;
        }
    }
}

// --- RustTapeSaturation ---

#[no_mangle]
pub unsafe extern "C" fn n8_rust_tape_saturation_create(sample_rate: f64) -> *mut RustTapeSaturation {
    let instance = Box::new(RustTapeSaturation::new(sample_rate as f32));
    Box::into_raw(instance)
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_tape_saturation_destroy(ptr: *mut RustTapeSaturation) {
    if !ptr.is_null() {
        drop(Box::from_raw(ptr));
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_tape_saturation_reset(ptr: *mut RustTapeSaturation) {
    if let Some(t) = ptr.as_mut() {
        t.reset();
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_tape_saturation_set_sample_rate(ptr: *mut RustTapeSaturation, sample_rate: f64) {
    if let Some(t) = ptr.as_mut() {
        t.set_sample_rate(sample_rate as f32);
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_tape_saturation_set_parameter(ptr: *mut RustTapeSaturation, param_id: u32, value: f32) {
    if let Some(t) = ptr.as_mut() {
        t.set_parameter(param_id, value);
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_tape_saturation_get_parameter(ptr: *const RustTapeSaturation, param_id: u32) -> f32 {
    if let Some(t) = ptr.as_ref() {
        t.get_parameter(param_id)
    } else {
        0.0
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_tape_saturation_process(
    ptr: *mut RustTapeSaturation,
    in_l: *const f32,
    in_r: *const f32,
    out_l: *mut f32,
    out_r: *mut f32,
    num_samples: u32,
) {
    if ptr.is_null() || in_l.is_null() || out_l.is_null() || num_samples == 0 {
        return;
    }

    let t = &mut *ptr;
    let n = num_samples as usize;

    let is_in_place = std::ptr::eq(in_l, out_l as *const f32)
        || (!in_r.is_null() && !out_r.is_null() && std::ptr::eq(in_r, out_r as *const f32));

    if is_in_place {
        let mut scratch_in_l = [0.0f32; 1024];
        let mut scratch_in_r = [0.0f32; 1024];
        let mut scratch_out_r = [0.0f32; 1024];
        let is_mono_out = out_r.is_null() || std::ptr::eq(out_l, out_r);

        let mut offset = 0;
        while offset < n {
            let chunk_size = (n - offset).min(1024);
            std::ptr::copy_nonoverlapping(in_l.add(offset), scratch_in_l.as_mut_ptr(), chunk_size);
            if !in_r.is_null() {
                std::ptr::copy_nonoverlapping(in_r.add(offset), scratch_in_r.as_mut_ptr(), chunk_size);
            } else {
                scratch_in_r[..chunk_size].copy_from_slice(&scratch_in_l[..chunk_size]);
            }

            let chunk_out_l = std::slice::from_raw_parts_mut(out_l.add(offset), chunk_size);
            if !is_mono_out {
                let chunk_out_r = std::slice::from_raw_parts_mut(out_r.add(offset), chunk_size);
                t.process_block(
                    &scratch_in_l[..chunk_size],
                    &scratch_in_r[..chunk_size],
                    chunk_out_l,
                    chunk_out_r,
                );
            } else {
                t.process_block(
                    &scratch_in_l[..chunk_size],
                    &scratch_in_r[..chunk_size],
                    chunk_out_l,
                    &mut scratch_out_r[..chunk_size],
                );
            }
            offset += chunk_size;
        }
        return;
    }

    let in_l_slice = std::slice::from_raw_parts(in_l, n);
    let in_r_slice = if !in_r.is_null() {
        std::slice::from_raw_parts(in_r, n)
    } else {
        in_l_slice
    };

    if !out_r.is_null() && !std::ptr::eq(out_l, out_r) {
        let out_l_slice = std::slice::from_raw_parts_mut(out_l, n);
        let out_r_slice = std::slice::from_raw_parts_mut(out_r, n);
        t.process_block(in_l_slice, in_r_slice, out_l_slice, out_r_slice);
    } else {
        let mut mono_scratch = [0.0f32; 1024];
        let mut offset = 0;
        let out_l_slice = std::slice::from_raw_parts_mut(out_l, n);
        while offset < n {
            let chunk_size = (n - offset).min(1024);
            let chunk_in_l = &in_l_slice[offset..offset + chunk_size];
            let chunk_in_r = &in_r_slice[offset..offset + chunk_size];
            let chunk_out_l = &mut out_l_slice[offset..offset + chunk_size];
            t.process_block(chunk_in_l, chunk_in_r, chunk_out_l, &mut mono_scratch[..chunk_size]);
            offset += chunk_size;
        }
    }
}

// --- RustShimmerReverb ---

#[no_mangle]
pub unsafe extern "C" fn n8_rust_shimmer_reverb_create(sample_rate: f64) -> *mut RustShimmerReverb {
    let instance = Box::new(RustShimmerReverb::new(sample_rate as f32));
    Box::into_raw(instance)
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_shimmer_reverb_destroy(ptr: *mut RustShimmerReverb) {
    if !ptr.is_null() {
        drop(Box::from_raw(ptr));
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_shimmer_reverb_reset(ptr: *mut RustShimmerReverb) {
    if let Some(r) = ptr.as_mut() {
        r.reset();
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_shimmer_reverb_set_sample_rate(ptr: *mut RustShimmerReverb, sample_rate: f64) {
    if let Some(r) = ptr.as_mut() {
        r.set_sample_rate(sample_rate as f32);
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_shimmer_reverb_set_parameter(ptr: *mut RustShimmerReverb, param_id: u32, value: f32) {
    if let Some(r) = ptr.as_mut() {
        r.set_parameter(param_id, value);
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_shimmer_reverb_get_parameter(ptr: *const RustShimmerReverb, param_id: u32) -> f32 {
    if let Some(r) = ptr.as_ref() {
        r.get_parameter(param_id)
    } else {
        0.0
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_shimmer_reverb_process(
    ptr: *mut RustShimmerReverb,
    in_l: *const f32,
    in_r: *const f32,
    out_l: *mut f32,
    out_r: *mut f32,
    num_samples: u32,
) {
    if ptr.is_null() || in_l.is_null() || out_l.is_null() || num_samples == 0 {
        return;
    }

    let r = &mut *ptr;
    let n = num_samples as usize;

    let is_in_place = std::ptr::eq(in_l, out_l as *const f32)
        || (!in_r.is_null() && !out_r.is_null() && std::ptr::eq(in_r, out_r as *const f32));

    if is_in_place {
        let mut scratch_in_l = [0.0f32; 1024];
        let mut scratch_in_r = [0.0f32; 1024];
        let mut scratch_out_r = [0.0f32; 1024];
        let is_mono_out = out_r.is_null() || std::ptr::eq(out_l, out_r);

        let mut offset = 0;
        while offset < n {
            let chunk_size = (n - offset).min(1024);
            std::ptr::copy_nonoverlapping(in_l.add(offset), scratch_in_l.as_mut_ptr(), chunk_size);
            if !in_r.is_null() {
                std::ptr::copy_nonoverlapping(in_r.add(offset), scratch_in_r.as_mut_ptr(), chunk_size);
            } else {
                scratch_in_r[..chunk_size].copy_from_slice(&scratch_in_l[..chunk_size]);
            }

            let chunk_out_l = std::slice::from_raw_parts_mut(out_l.add(offset), chunk_size);
            if !is_mono_out {
                let chunk_out_r = std::slice::from_raw_parts_mut(out_r.add(offset), chunk_size);
                r.process_block(
                    &scratch_in_l[..chunk_size],
                    &scratch_in_r[..chunk_size],
                    chunk_out_l,
                    chunk_out_r,
                );
            } else {
                r.process_block(
                    &scratch_in_l[..chunk_size],
                    &scratch_in_r[..chunk_size],
                    chunk_out_l,
                    &mut scratch_out_r[..chunk_size],
                );
            }
            offset += chunk_size;
        }
        return;
    }

    let in_l_slice = std::slice::from_raw_parts(in_l, n);
    let in_r_slice = if !in_r.is_null() {
        std::slice::from_raw_parts(in_r, n)
    } else {
        in_l_slice
    };

    if !out_r.is_null() && !std::ptr::eq(out_l, out_r) {
        let out_l_slice = std::slice::from_raw_parts_mut(out_l, n);
        let out_r_slice = std::slice::from_raw_parts_mut(out_r, n);
        r.process_block(in_l_slice, in_r_slice, out_l_slice, out_r_slice);
    } else {
        let mut mono_scratch = [0.0f32; 1024];
        let mut offset = 0;
        let out_l_slice = std::slice::from_raw_parts_mut(out_l, n);
        while offset < n {
            let chunk_size = (n - offset).min(1024);
            let chunk_in_l = &in_l_slice[offset..offset + chunk_size];
            let chunk_in_r = &in_r_slice[offset..offset + chunk_size];
            let chunk_out_l = &mut out_l_slice[offset..offset + chunk_size];
            r.process_block(chunk_in_l, chunk_in_r, chunk_out_l, &mut mono_scratch[..chunk_size]);
            offset += chunk_size;
        }
    }
}

// --- RustRefraction ---

#[no_mangle]
pub unsafe extern "C" fn n8_rust_refraction_create(sample_rate: f64) -> *mut RustRefraction {
    let instance = Box::new(RustRefraction::new(sample_rate as f32));
    Box::into_raw(instance)
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_refraction_destroy(ptr: *mut RustRefraction) {
    if !ptr.is_null() {
        drop(Box::from_raw(ptr));
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_refraction_reset(ptr: *mut RustRefraction) {
    if let Some(r) = ptr.as_mut() {
        r.reset();
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_refraction_set_sample_rate(ptr: *mut RustRefraction, sample_rate: f64) {
    if let Some(r) = ptr.as_mut() {
        r.set_sample_rate(sample_rate as f32);
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_refraction_set_parameter(ptr: *mut RustRefraction, param_id: u32, value: f32) {
    if let Some(r) = ptr.as_mut() {
        r.set_parameter(param_id, value);
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_refraction_get_parameter(ptr: *const RustRefraction, param_id: u32) -> f32 {
    if let Some(r) = ptr.as_ref() {
        r.get_parameter(param_id)
    } else {
        0.0
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_refraction_process(
    ptr: *mut RustRefraction,
    in_l: *const f32,
    in_r: *const f32,
    out_l: *mut f32,
    out_r: *mut f32,
    num_samples: u32,
) {
    if ptr.is_null() || in_l.is_null() || out_l.is_null() || num_samples == 0 {
        return;
    }

    let r = &mut *ptr;
    let n = num_samples as usize;

    let is_in_place = std::ptr::eq(in_l, out_l as *const f32)
        || (!in_r.is_null() && !out_r.is_null() && std::ptr::eq(in_r, out_r as *const f32));

    if is_in_place {
        let mut scratch_in_l = [0.0f32; 1024];
        let mut scratch_in_r = [0.0f32; 1024];
        let mut scratch_out_r = [0.0f32; 1024];
        let is_mono_out = out_r.is_null() || std::ptr::eq(out_l, out_r);

        let mut offset = 0;
        while offset < n {
            let chunk_size = (n - offset).min(1024);
            std::ptr::copy_nonoverlapping(in_l.add(offset), scratch_in_l.as_mut_ptr(), chunk_size);
            if !in_r.is_null() {
                std::ptr::copy_nonoverlapping(in_r.add(offset), scratch_in_r.as_mut_ptr(), chunk_size);
            } else {
                scratch_in_r[..chunk_size].copy_from_slice(&scratch_in_l[..chunk_size]);
            }

            let chunk_out_l = std::slice::from_raw_parts_mut(out_l.add(offset), chunk_size);
            if !is_mono_out {
                let chunk_out_r = std::slice::from_raw_parts_mut(out_r.add(offset), chunk_size);
                r.process_block(
                    &scratch_in_l[..chunk_size],
                    &scratch_in_r[..chunk_size],
                    chunk_out_l,
                    chunk_out_r,
                );
            } else {
                r.process_block(
                    &scratch_in_l[..chunk_size],
                    &scratch_in_r[..chunk_size],
                    chunk_out_l,
                    &mut scratch_out_r[..chunk_size],
                );
            }
            offset += chunk_size;
        }
        return;
    }

    let in_l_slice = std::slice::from_raw_parts(in_l, n);
    let in_r_slice = if !in_r.is_null() {
        std::slice::from_raw_parts(in_r, n)
    } else {
        in_l_slice
    };

    if !out_r.is_null() && !std::ptr::eq(out_l, out_r) {
        let out_l_slice = std::slice::from_raw_parts_mut(out_l, n);
        let out_r_slice = std::slice::from_raw_parts_mut(out_r, n);
        r.process_block(in_l_slice, in_r_slice, out_l_slice, out_r_slice);
    } else {
        let mut mono_scratch = [0.0f32; 1024];
        let mut offset = 0;
        let out_l_slice = std::slice::from_raw_parts_mut(out_l, n);
        while offset < n {
            let chunk_size = (n - offset).min(1024);
            let chunk_in_l = &in_l_slice[offset..offset + chunk_size];
            let chunk_in_r = &in_r_slice[offset..offset + chunk_size];
            let chunk_out_l = &mut out_l_slice[offset..offset + chunk_size];
            r.process_block(chunk_in_l, chunk_in_r, chunk_out_l, &mut mono_scratch[..chunk_size]);
            offset += chunk_size;
        }
    }
}

// --- RustDynamicEq ---

#[no_mangle]
pub unsafe extern "C" fn n8_rust_dynamic_eq_create(sample_rate: f64) -> *mut RustDynamicEq {
    let instance = Box::new(RustDynamicEq::new(sample_rate as f32));
    Box::into_raw(instance)
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_dynamic_eq_destroy(ptr: *mut RustDynamicEq) {
    if !ptr.is_null() {
        drop(Box::from_raw(ptr));
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_dynamic_eq_reset(ptr: *mut RustDynamicEq) {
    if let Some(eq) = ptr.as_mut() {
        eq.reset();
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_dynamic_eq_set_sample_rate(ptr: *mut RustDynamicEq, sample_rate: f64) {
    if let Some(eq) = ptr.as_mut() {
        eq.set_sample_rate(sample_rate as f32);
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_dynamic_eq_set_parameter(ptr: *mut RustDynamicEq, param_id: u32, value: f32) {
    if let Some(eq) = ptr.as_mut() {
        eq.set_parameter(param_id, value);
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_dynamic_eq_get_parameter(ptr: *const RustDynamicEq, param_id: u32) -> f32 {
    if let Some(eq) = ptr.as_ref() {
        eq.get_parameter(param_id)
    } else {
        0.0
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_dynamic_eq_process(
    ptr: *mut RustDynamicEq,
    in_l: *const f32,
    in_r: *const f32,
    out_l: *mut f32,
    out_r: *mut f32,
    num_samples: u32,
) {
    if ptr.is_null() || in_l.is_null() || out_l.is_null() || num_samples == 0 {
        return;
    }

    let eq = &mut *ptr;
    let n = num_samples as usize;

    let is_in_place = std::ptr::eq(in_l, out_l as *const f32)
        || (!in_r.is_null() && !out_r.is_null() && std::ptr::eq(in_r, out_r as *const f32));

    if is_in_place {
        let mut scratch_in_l = [0.0f32; 1024];
        let mut scratch_in_r = [0.0f32; 1024];
        let mut scratch_out_r = [0.0f32; 1024];
        let is_mono_out = out_r.is_null() || std::ptr::eq(out_l, out_r);

        let mut offset = 0;
        while offset < n {
            let chunk_size = (n - offset).min(1024);
            std::ptr::copy_nonoverlapping(in_l.add(offset), scratch_in_l.as_mut_ptr(), chunk_size);
            if !in_r.is_null() {
                std::ptr::copy_nonoverlapping(in_r.add(offset), scratch_in_r.as_mut_ptr(), chunk_size);
            } else {
                scratch_in_r[..chunk_size].copy_from_slice(&scratch_in_l[..chunk_size]);
            }

            let chunk_out_l = std::slice::from_raw_parts_mut(out_l.add(offset), chunk_size);
            if !is_mono_out {
                let chunk_out_r = std::slice::from_raw_parts_mut(out_r.add(offset), chunk_size);
                eq.process_block(
                    &scratch_in_l[..chunk_size],
                    &scratch_in_r[..chunk_size],
                    chunk_out_l,
                    chunk_out_r,
                );
            } else {
                eq.process_block(
                    &scratch_in_l[..chunk_size],
                    &scratch_in_r[..chunk_size],
                    chunk_out_l,
                    &mut scratch_out_r[..chunk_size],
                );
            }
            offset += chunk_size;
        }
        return;
    }

    let in_l_slice = std::slice::from_raw_parts(in_l, n);
    let in_r_slice = if !in_r.is_null() {
        std::slice::from_raw_parts(in_r, n)
    } else {
        in_l_slice
    };

    if !out_r.is_null() && !std::ptr::eq(out_l, out_r) {
        let out_l_slice = std::slice::from_raw_parts_mut(out_l, n);
        let out_r_slice = std::slice::from_raw_parts_mut(out_r, n);
        eq.process_block(in_l_slice, in_r_slice, out_l_slice, out_r_slice);
    } else {
        let mut mono_scratch = [0.0f32; 1024];
        let mut offset = 0;
        let out_l_slice = std::slice::from_raw_parts_mut(out_l, n);
        while offset < n {
            let chunk_size = (n - offset).min(1024);
            let chunk_in_l = &in_l_slice[offset..offset + chunk_size];
            let chunk_in_r = &in_r_slice[offset..offset + chunk_size];
            let chunk_out_l = &mut out_l_slice[offset..offset + chunk_size];
            eq.process_block(chunk_in_l, chunk_in_r, chunk_out_l, &mut mono_scratch[..chunk_size]);
            offset += chunk_size;
        }
    }
}

// --- RustTransientShaper ---

#[no_mangle]
pub unsafe extern "C" fn n8_rust_transient_shaper_create(sample_rate: f64) -> *mut RustTransientShaper {
    let instance = Box::new(RustTransientShaper::new(sample_rate as f32));
    Box::into_raw(instance)
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_transient_shaper_destroy(ptr: *mut RustTransientShaper) {
    if !ptr.is_null() {
        drop(Box::from_raw(ptr));
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_transient_shaper_reset(ptr: *mut RustTransientShaper) {
    if let Some(ts) = ptr.as_mut() {
        ts.reset();
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_transient_shaper_set_sample_rate(ptr: *mut RustTransientShaper, sample_rate: f64) {
    if let Some(ts) = ptr.as_mut() {
        ts.set_sample_rate(sample_rate as f32);
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_transient_shaper_set_parameter(ptr: *mut RustTransientShaper, param_id: u32, value: f32) {
    if let Some(ts) = ptr.as_mut() {
        ts.set_parameter(param_id, value);
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_transient_shaper_get_parameter(ptr: *const RustTransientShaper, param_id: u32) -> f32 {
    if let Some(ts) = ptr.as_ref() {
        ts.get_parameter(param_id)
    } else {
        0.0
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_transient_shaper_process(
    ptr: *mut RustTransientShaper,
    in_l: *const f32,
    in_r: *const f32,
    out_l: *mut f32,
    out_r: *mut f32,
    num_samples: u32,
) {
    if ptr.is_null() || in_l.is_null() || out_l.is_null() || num_samples == 0 {
        return;
    }

    let ts = &mut *ptr;
    let n = num_samples as usize;

    let is_in_place = std::ptr::eq(in_l, out_l as *const f32)
        || (!in_r.is_null() && !out_r.is_null() && std::ptr::eq(in_r, out_r as *const f32));

    if is_in_place {
        let mut scratch_in_l = [0.0f32; 1024];
        let mut scratch_in_r = [0.0f32; 1024];
        let mut scratch_out_r = [0.0f32; 1024];
        let is_mono_out = out_r.is_null() || std::ptr::eq(out_l, out_r);

        let mut offset = 0;
        while offset < n {
            let chunk_size = (n - offset).min(1024);
            std::ptr::copy_nonoverlapping(in_l.add(offset), scratch_in_l.as_mut_ptr(), chunk_size);
            if !in_r.is_null() {
                std::ptr::copy_nonoverlapping(in_r.add(offset), scratch_in_r.as_mut_ptr(), chunk_size);
            } else {
                scratch_in_r[..chunk_size].copy_from_slice(&scratch_in_l[..chunk_size]);
            }

            let chunk_out_l = std::slice::from_raw_parts_mut(out_l.add(offset), chunk_size);
            if !is_mono_out {
                let chunk_out_r = std::slice::from_raw_parts_mut(out_r.add(offset), chunk_size);
                ts.process_block(
                    &scratch_in_l[..chunk_size],
                    &scratch_in_r[..chunk_size],
                    chunk_out_l,
                    chunk_out_r,
                );
            } else {
                ts.process_block(
                    &scratch_in_l[..chunk_size],
                    &scratch_in_r[..chunk_size],
                    chunk_out_l,
                    &mut scratch_out_r[..chunk_size],
                );
            }
            offset += chunk_size;
        }
        return;
    }

    let in_l_slice = std::slice::from_raw_parts(in_l, n);
    let in_r_slice = if !in_r.is_null() {
        std::slice::from_raw_parts(in_r, n)
    } else {
        in_l_slice
    };

    if !out_r.is_null() && !std::ptr::eq(out_l, out_r) {
        let out_l_slice = std::slice::from_raw_parts_mut(out_l, n);
        let out_r_slice = std::slice::from_raw_parts_mut(out_r, n);
        ts.process_block(in_l_slice, in_r_slice, out_l_slice, out_r_slice);
    } else {
        let mut mono_scratch = [0.0f32; 1024];
        let mut offset = 0;
        let out_l_slice = std::slice::from_raw_parts_mut(out_l, n);
        while offset < n {
            let chunk_size = (n - offset).min(1024);
            let chunk_in_l = &in_l_slice[offset..offset + chunk_size];
            let chunk_in_r = &in_r_slice[offset..offset + chunk_size];
            let chunk_out_l = &mut out_l_slice[offset..offset + chunk_size];
            ts.process_block(chunk_in_l, chunk_in_r, chunk_out_l, &mut mono_scratch[..chunk_size]);
            offset += chunk_size;
        }
    }
}

// ============================================================================
// Granular Cloud Engine C-ABI FFI Exports (DSP #7)
// ============================================================================

#[no_mangle]
pub unsafe extern "C" fn n8_rust_granular_create(sample_rate: f64) -> *mut RustGranular {
    Box::into_raw(Box::new(RustGranular::new(sample_rate)))
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_granular_destroy(node: *mut RustGranular) {
    if !node.is_null() {
        drop(Box::from_raw(node));
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_granular_prepare(node: *mut RustGranular, sample_rate: f64) {
    if let Some(gran) = node.as_mut() {
        gran.prepare(sample_rate);
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_granular_reset(node: *mut RustGranular) {
    if let Some(gran) = node.as_mut() {
        gran.reset();
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_granular_set_parameter(node: *mut RustGranular, param_id: u32, value: f32) {
    if let Some(gran) = node.as_mut() {
        gran.set_parameter(param_id, value);
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_granular_get_parameter(node: *const RustGranular, param_id: u32) -> f32 {
    if let Some(gran) = node.as_ref() {
        gran.get_parameter(param_id)
    } else {
        0.0
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_granular_copy_waveform_overview(
    node: *const RustGranular,
    out_min_max: *mut f32,
    num_pairs: usize,
) -> usize {
    if let Some(gran) = node.as_ref() {
        if !out_min_max.is_null() && num_pairs > 0 {
            let slice = std::slice::from_raw_parts_mut(out_min_max, num_pairs * 2);
            return gran.copy_waveform_overview(slice, num_pairs);
        }
    }
    0
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_granular_copy_active_grains_snapshot(
    node: *const RustGranular,
    out_points: *mut RustGrainCloudPoint,
    max_points: usize,
) -> usize {
    if let Some(gran) = node.as_ref() {
        if !out_points.is_null() && max_points > 0 {
            let slice = std::slice::from_raw_parts_mut(out_points, max_points);
            return gran.copy_active_grains_snapshot(slice);
        }
    }
    0
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_granular_get_write_position_normalized(node: *const RustGranular) -> f32 {
    if let Some(gran) = node.as_ref() {
        gran.get_write_position_normalized()
    } else {
        0.0
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_granular_process(
    node: *mut RustGranular,
    in_l: *const f32,
    in_r: *const f32,
    out_l: *mut f32,
    out_r: *mut f32,
    num_samples: u32,
) {
    if node.is_null() || in_l.is_null() || out_l.is_null() || num_samples == 0 {
        return;
    }
    let gran = &mut *node;
    let n = num_samples as usize;

    let is_in_place = std::ptr::eq(in_l, out_l as *const f32)
        || (!in_r.is_null() && !out_r.is_null() && std::ptr::eq(in_r, out_r as *const f32));

    if is_in_place {
        let mut scratch_in_l = [0.0f32; 1024];
        let mut scratch_in_r = [0.0f32; 1024];
        let mut scratch_out_r = [0.0f32; 1024];
        let is_mono_out = out_r.is_null() || std::ptr::eq(out_l, out_r);

        let mut offset = 0;
        while offset < n {
            let chunk_size = (n - offset).min(1024);
            std::ptr::copy_nonoverlapping(in_l.add(offset), scratch_in_l.as_mut_ptr(), chunk_size);
            if !in_r.is_null() {
                std::ptr::copy_nonoverlapping(in_r.add(offset), scratch_in_r.as_mut_ptr(), chunk_size);
            } else {
                scratch_in_r[..chunk_size].copy_from_slice(&scratch_in_l[..chunk_size]);
            }

            let chunk_out_l = std::slice::from_raw_parts_mut(out_l.add(offset), chunk_size);
            if !is_mono_out {
                let chunk_out_r = std::slice::from_raw_parts_mut(out_r.add(offset), chunk_size);
                gran.process_block(
                    &scratch_in_l[..chunk_size],
                    &scratch_in_r[..chunk_size],
                    chunk_out_l,
                    chunk_out_r,
                );
            } else {
                gran.process_block(
                    &scratch_in_l[..chunk_size],
                    &scratch_in_r[..chunk_size],
                    chunk_out_l,
                    &mut scratch_out_r[..chunk_size],
                );
            }
            offset += chunk_size;
        }
        return;
    }

    let in_l_slice = std::slice::from_raw_parts(in_l, n);
    let in_r_slice = if !in_r.is_null() {
        std::slice::from_raw_parts(in_r, n)
    } else {
        in_l_slice
    };

    if !out_r.is_null() && !std::ptr::eq(out_l, out_r) {
        let out_l_slice = std::slice::from_raw_parts_mut(out_l, n);
        let out_r_slice = std::slice::from_raw_parts_mut(out_r, n);
        gran.process_block(in_l_slice, in_r_slice, out_l_slice, out_r_slice);
    } else {
        let mut mono_scratch = [0.0f32; 1024];
        let mut offset = 0;
        let out_l_slice = std::slice::from_raw_parts_mut(out_l, n);
        while offset < n {
            let chunk_size = (n - offset).min(1024);
            let chunk_in_l = &in_l_slice[offset..offset + chunk_size];
            let chunk_in_r = &in_r_slice[offset..offset + chunk_size];
            let chunk_out_l = &mut out_l_slice[offset..offset + chunk_size];
            gran.process_block(chunk_in_l, chunk_in_r, chunk_out_l, &mut mono_scratch[..chunk_size]);
            offset += chunk_size;
        }
    }
}

// ============================================================================
// Audio Slicer & Frozen Granular Playground C-ABI FFI Exports (DSP #49)
// ============================================================================

#[no_mangle]
pub unsafe extern "C" fn n8_rust_audio_slicer_create(sample_rate: f64) -> *mut RustAudioSlicer {
    Box::into_raw(Box::new(RustAudioSlicer::new(sample_rate)))
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_audio_slicer_destroy(node: *mut RustAudioSlicer) {
    if !node.is_null() {
        drop(Box::from_raw(node));
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_audio_slicer_prepare(node: *mut RustAudioSlicer, sample_rate: f64) {
    if let Some(slicer) = node.as_mut() {
        slicer.prepare(sample_rate);
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_audio_slicer_reset(node: *mut RustAudioSlicer) {
    if let Some(slicer) = node.as_mut() {
        slicer.reset();
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_audio_slicer_set_parameter(node: *mut RustAudioSlicer, param_id: u32, value: f32) {
    if let Some(slicer) = node.as_mut() {
        slicer.set_parameter(param_id, value);
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_audio_slicer_get_parameter(node: *const RustAudioSlicer, param_id: u32) -> f32 {
    if let Some(slicer) = node.as_ref() {
        slicer.get_parameter(param_id)
    } else {
        0.0
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_audio_slicer_copy_waveform_thumbnail(
    node: *const RustAudioSlicer,
    out_points: *mut f32,
    num_points: usize,
) -> usize {
    if let Some(slicer) = node.as_ref() {
        if !out_points.is_null() && num_points > 0 {
            let slice = std::slice::from_raw_parts_mut(out_points, num_points);
            return slicer.copy_waveform_thumbnail(slice);
        }
    }
    0
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_audio_slicer_process(
    node: *mut RustAudioSlicer,
    in_l: *const f32,
    in_r: *const f32,
    out_l: *mut f32,
    out_r: *mut f32,
    num_samples: u32,
) {
    if node.is_null() || in_l.is_null() || out_l.is_null() || num_samples == 0 {
        return;
    }
    let slicer = &mut *node;
    let n = num_samples as usize;

    let is_in_place = std::ptr::eq(in_l, out_l as *const f32)
        || (!in_r.is_null() && !out_r.is_null() && std::ptr::eq(in_r, out_r as *const f32));

    if is_in_place {
        let mut scratch_in_l = [0.0f32; 1024];
        let mut scratch_in_r = [0.0f32; 1024];
        let mut scratch_out_r = [0.0f32; 1024];
        let is_mono_out = out_r.is_null() || std::ptr::eq(out_l, out_r);

        let mut offset = 0;
        while offset < n {
            let chunk_size = (n - offset).min(1024);
            std::ptr::copy_nonoverlapping(in_l.add(offset), scratch_in_l.as_mut_ptr(), chunk_size);
            if !in_r.is_null() {
                std::ptr::copy_nonoverlapping(in_r.add(offset), scratch_in_r.as_mut_ptr(), chunk_size);
            } else {
                scratch_in_r[..chunk_size].copy_from_slice(&scratch_in_l[..chunk_size]);
            }

            let chunk_out_l = std::slice::from_raw_parts_mut(out_l.add(offset), chunk_size);
            if !is_mono_out {
                let chunk_out_r = std::slice::from_raw_parts_mut(out_r.add(offset), chunk_size);
                slicer.process_block(
                    &scratch_in_l[..chunk_size],
                    &scratch_in_r[..chunk_size],
                    chunk_out_l,
                    chunk_out_r,
                );
            } else {
                slicer.process_block(
                    &scratch_in_l[..chunk_size],
                    &scratch_in_r[..chunk_size],
                    chunk_out_l,
                    &mut scratch_out_r[..chunk_size],
                );
            }
            offset += chunk_size;
        }
        return;
    }

    let in_l_slice = std::slice::from_raw_parts(in_l, n);
    let in_r_slice = if !in_r.is_null() {
        std::slice::from_raw_parts(in_r, n)
    } else {
        in_l_slice
    };

    if !out_r.is_null() && !std::ptr::eq(out_l, out_r) {
        let out_l_slice = std::slice::from_raw_parts_mut(out_l, n);
        let out_r_slice = std::slice::from_raw_parts_mut(out_r, n);
        slicer.process_block(in_l_slice, in_r_slice, out_l_slice, out_r_slice);
    } else {
        let mut mono_scratch = [0.0f32; 1024];
        let mut offset = 0;
        let out_l_slice = std::slice::from_raw_parts_mut(out_l, n);
        while offset < n {
            let chunk_size = (n - offset).min(1024);
            let chunk_in_l = &in_l_slice[offset..offset + chunk_size];
            let chunk_in_r = &in_r_slice[offset..offset + chunk_size];
            let chunk_out_l = &mut out_l_slice[offset..offset + chunk_size];
            slicer.process_block(chunk_in_l, chunk_in_r, chunk_out_l, &mut mono_scratch[..chunk_size]);
            offset += chunk_size;
        }
    }
}

// ============================================================================
// Channel Vocoder C-ABI FFI Exports (DSP #45)
// ============================================================================

#[no_mangle]
pub unsafe extern "C" fn n8_rust_vocoder_create(sample_rate: f64) -> *mut RustVocoder {
    Box::into_raw(Box::new(RustVocoder::new(sample_rate)))
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_vocoder_destroy(node: *mut RustVocoder) {
    if !node.is_null() {
        drop(Box::from_raw(node));
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_vocoder_prepare(node: *mut RustVocoder, sample_rate: f64) {
    if let Some(voc) = node.as_mut() {
        voc.prepare(sample_rate);
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_vocoder_reset(node: *mut RustVocoder) {
    if let Some(voc) = node.as_mut() {
        voc.reset();
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_vocoder_set_parameter(node: *mut RustVocoder, param_id: u32, value: f32) {
    if let Some(voc) = node.as_mut() {
        voc.set_parameter(param_id, value);
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_vocoder_get_parameter(node: *const RustVocoder, param_id: u32) -> f32 {
    if let Some(voc) = node.as_ref() {
        voc.get_parameter(param_id)
    } else {
        0.0
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_vocoder_process(
    node: *mut RustVocoder,
    in_l: *const f32,
    in_r: *const f32,
    sc_l: *const f32,
    sc_r: *const f32,
    out_l: *mut f32,
    out_r: *mut f32,
    num_samples: u32,
) {
    if node.is_null() || in_l.is_null() || out_l.is_null() || num_samples == 0 {
        return;
    }
    let voc = &mut *node;
    let n = num_samples as usize;

    let is_in_place = std::ptr::eq(in_l, out_l as *const f32)
        || (!in_r.is_null() && !out_r.is_null() && std::ptr::eq(in_r, out_r as *const f32));

    if is_in_place {
        let mut scratch_in_l = [0.0f32; 1024];
        let mut scratch_in_r = [0.0f32; 1024];
        let mut scratch_out_r = [0.0f32; 1024];
        let is_mono_out = out_r.is_null() || std::ptr::eq(out_l, out_r);

        let mut offset = 0;
        while offset < n {
            let chunk_size = (n - offset).min(1024);
            std::ptr::copy_nonoverlapping(in_l.add(offset), scratch_in_l.as_mut_ptr(), chunk_size);
            if !in_r.is_null() {
                std::ptr::copy_nonoverlapping(in_r.add(offset), scratch_in_r.as_mut_ptr(), chunk_size);
            } else {
                scratch_in_r[..chunk_size].copy_from_slice(&scratch_in_l[..chunk_size]);
            }

            let sc_l_slice = if !sc_l.is_null() {
                Some(std::slice::from_raw_parts(sc_l.add(offset), chunk_size))
            } else {
                None
            };
            let sc_r_slice = if !sc_r.is_null() {
                Some(std::slice::from_raw_parts(sc_r.add(offset), chunk_size))
            } else {
                sc_l_slice
            };

            let chunk_out_l = std::slice::from_raw_parts_mut(out_l.add(offset), chunk_size);
            if !is_mono_out {
                let chunk_out_r = std::slice::from_raw_parts_mut(out_r.add(offset), chunk_size);
                voc.process_block(
                    &scratch_in_l[..chunk_size],
                    &scratch_in_r[..chunk_size],
                    sc_l_slice,
                    sc_r_slice,
                    chunk_out_l,
                    chunk_out_r,
                );
            } else {
                voc.process_block(
                    &scratch_in_l[..chunk_size],
                    &scratch_in_r[..chunk_size],
                    sc_l_slice,
                    sc_r_slice,
                    chunk_out_l,
                    &mut scratch_out_r[..chunk_size],
                );
            }
            offset += chunk_size;
        }
        return;
    }

    let in_l_slice = std::slice::from_raw_parts(in_l, n);
    let in_r_slice = if !in_r.is_null() {
        std::slice::from_raw_parts(in_r, n)
    } else {
        in_l_slice
    };

    let sc_l_slice = if !sc_l.is_null() {
        Some(std::slice::from_raw_parts(sc_l, n))
    } else {
        None
    };
    let sc_r_slice = if !sc_r.is_null() {
        Some(std::slice::from_raw_parts(sc_r, n))
    } else {
        sc_l_slice
    };

    if !out_r.is_null() && !std::ptr::eq(out_l, out_r) {
        let out_l_slice = std::slice::from_raw_parts_mut(out_l, n);
        let out_r_slice = std::slice::from_raw_parts_mut(out_r, n);
        voc.process_block(in_l_slice, in_r_slice, sc_l_slice, sc_r_slice, out_l_slice, out_r_slice);
    } else {
        let mut mono_scratch = [0.0f32; 1024];
        let mut offset = 0;
        let out_l_slice = std::slice::from_raw_parts_mut(out_l, n);
        while offset < n {
            let chunk_size = (n - offset).min(1024);
            let chunk_in_l = &in_l_slice[offset..offset + chunk_size];
            let chunk_in_r = &in_r_slice[offset..offset + chunk_size];
            let chunk_sc_l = sc_l_slice.map(|s| &s[offset..offset + chunk_size]);
            let chunk_sc_r = sc_r_slice.map(|s| &s[offset..offset + chunk_size]);
            let chunk_out_l = &mut out_l_slice[offset..offset + chunk_size];
            voc.process_block(
                chunk_in_l,
                chunk_in_r,
                chunk_sc_l,
                chunk_sc_r,
                chunk_out_l,
                &mut mono_scratch[..chunk_size],
            );
            offset += chunk_size;
        }
    }
}

// ============================================================================
// Rotary Speaker / Leslie Cabinet C-ABI FFI Exports (DSP #43)
// ============================================================================

#[no_mangle]
pub unsafe extern "C" fn n8_rust_rotary_speaker_create(sample_rate: f64) -> *mut RustRotarySpeaker {
    Box::into_raw(Box::new(RustRotarySpeaker::new(sample_rate)))
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_rotary_speaker_destroy(node: *mut RustRotarySpeaker) {
    if !node.is_null() {
        drop(Box::from_raw(node));
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_rotary_speaker_prepare(node: *mut RustRotarySpeaker, sample_rate: f64) {
    if let Some(rotary) = node.as_mut() {
        rotary.prepare(sample_rate);
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_rotary_speaker_reset(node: *mut RustRotarySpeaker) {
    if let Some(rotary) = node.as_mut() {
        rotary.reset();
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_rotary_speaker_set_parameter(node: *mut RustRotarySpeaker, param_id: u32, value: f32) {
    if let Some(rotary) = node.as_mut() {
        rotary.set_parameter(param_id, value);
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_rotary_speaker_get_parameter(node: *const RustRotarySpeaker, param_id: u32) -> f32 {
    if let Some(rotary) = node.as_ref() {
        rotary.get_parameter(param_id)
    } else {
        0.0
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_rotary_speaker_process(
    node: *mut RustRotarySpeaker,
    in_l: *const f32,
    in_r: *const f32,
    out_l: *mut f32,
    out_r: *mut f32,
    num_samples: u32,
) {
    if node.is_null() || in_l.is_null() || out_l.is_null() || num_samples == 0 {
        return;
    }
    let rotary = &mut *node;
    let n = num_samples as usize;

    let is_in_place = std::ptr::eq(in_l, out_l as *const f32)
        || (!in_r.is_null() && !out_r.is_null() && std::ptr::eq(in_r, out_r as *const f32));

    if is_in_place {
        let mut scratch_in_l = [0.0f32; 1024];
        let mut scratch_in_r = [0.0f32; 1024];
        let mut scratch_out_r = [0.0f32; 1024];
        let is_mono_out = out_r.is_null() || std::ptr::eq(out_l, out_r);

        let mut offset = 0;
        while offset < n {
            let chunk_size = (n - offset).min(1024);
            std::ptr::copy_nonoverlapping(in_l.add(offset), scratch_in_l.as_mut_ptr(), chunk_size);
            if !in_r.is_null() {
                std::ptr::copy_nonoverlapping(in_r.add(offset), scratch_in_r.as_mut_ptr(), chunk_size);
            } else {
                scratch_in_r[..chunk_size].copy_from_slice(&scratch_in_l[..chunk_size]);
            }

            let chunk_out_l = std::slice::from_raw_parts_mut(out_l.add(offset), chunk_size);
            if !is_mono_out {
                let chunk_out_r = std::slice::from_raw_parts_mut(out_r.add(offset), chunk_size);
                rotary.process_block(
                    &scratch_in_l[..chunk_size],
                    &scratch_in_r[..chunk_size],
                    chunk_out_l,
                    chunk_out_r,
                );
            } else {
                rotary.process_block(
                    &scratch_in_l[..chunk_size],
                    &scratch_in_r[..chunk_size],
                    chunk_out_l,
                    &mut scratch_out_r[..chunk_size],
                );
            }
            offset += chunk_size;
        }
        return;
    }

    let in_l_slice = std::slice::from_raw_parts(in_l, n);
    let in_r_slice = if !in_r.is_null() {
        std::slice::from_raw_parts(in_r, n)
    } else {
        in_l_slice
    };

    if !out_r.is_null() && !std::ptr::eq(out_l, out_r) {
        let out_l_slice = std::slice::from_raw_parts_mut(out_l, n);
        let out_r_slice = std::slice::from_raw_parts_mut(out_r, n);
        rotary.process_block(in_l_slice, in_r_slice, out_l_slice, out_r_slice);
    } else {
        let mut mono_scratch = [0.0f32; 1024];
        let mut offset = 0;
        let out_l_slice = std::slice::from_raw_parts_mut(out_l, n);
        while offset < n {
            let chunk_size = (n - offset).min(1024);
            let chunk_in_l = &in_l_slice[offset..offset + chunk_size];
            let chunk_in_r = &in_r_slice[offset..offset + chunk_size];
            let chunk_out_l = &mut out_l_slice[offset..offset + chunk_size];
            rotary.process_block(chunk_in_l, chunk_in_r, chunk_out_l, &mut mono_scratch[..chunk_size]);
            offset += chunk_size;
        }
    }
}

// ============================================================================
// Harmonic Exciter & Sub-Harmonic Bass Synthesizer C-ABI FFI Exports (DSP #44)
// ============================================================================

#[no_mangle]
pub unsafe extern "C" fn n8_rust_harmonic_exciter_create(sample_rate: f64) -> *mut RustHarmonicExciter {
    Box::into_raw(Box::new(RustHarmonicExciter::new(sample_rate)))
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_harmonic_exciter_destroy(node: *mut RustHarmonicExciter) {
    if !node.is_null() {
        drop(Box::from_raw(node));
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_harmonic_exciter_prepare(node: *mut RustHarmonicExciter, sample_rate: f64) {
    if let Some(exciter) = node.as_mut() {
        exciter.prepare(sample_rate);
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_harmonic_exciter_reset(node: *mut RustHarmonicExciter) {
    if let Some(exciter) = node.as_mut() {
        exciter.reset();
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_harmonic_exciter_set_parameter(node: *mut RustHarmonicExciter, param_id: u32, value: f32) {
    if let Some(exciter) = node.as_mut() {
        exciter.set_parameter(param_id, value);
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_harmonic_exciter_get_parameter(node: *const RustHarmonicExciter, param_id: u32) -> f32 {
    if let Some(exciter) = node.as_ref() {
        exciter.get_parameter(param_id)
    } else {
        0.0
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_harmonic_exciter_process(
    node: *mut RustHarmonicExciter,
    in_l: *const f32,
    in_r: *const f32,
    out_l: *mut f32,
    out_r: *mut f32,
    num_samples: u32,
) {
    if node.is_null() || in_l.is_null() || out_l.is_null() || num_samples == 0 {
        return;
    }
    let exciter = &mut *node;
    let n = num_samples as usize;

    let is_in_place = std::ptr::eq(in_l, out_l as *const f32)
        || (!in_r.is_null() && !out_r.is_null() && std::ptr::eq(in_r, out_r as *const f32));

    if is_in_place {
        let mut scratch_in_l = [0.0f32; 1024];
        let mut scratch_in_r = [0.0f32; 1024];
        let mut scratch_out_r = [0.0f32; 1024];
        let is_mono_out = out_r.is_null() || std::ptr::eq(out_l, out_r);

        let mut offset = 0;
        while offset < n {
            let chunk_size = (n - offset).min(1024);
            std::ptr::copy_nonoverlapping(in_l.add(offset), scratch_in_l.as_mut_ptr(), chunk_size);
            if !in_r.is_null() {
                std::ptr::copy_nonoverlapping(in_r.add(offset), scratch_in_r.as_mut_ptr(), chunk_size);
            } else {
                scratch_in_r[..chunk_size].copy_from_slice(&scratch_in_l[..chunk_size]);
            }

            let chunk_out_l = std::slice::from_raw_parts_mut(out_l.add(offset), chunk_size);
            if !is_mono_out {
                let chunk_out_r = std::slice::from_raw_parts_mut(out_r.add(offset), chunk_size);
                exciter.process_block(
                    &scratch_in_l[..chunk_size],
                    &scratch_in_r[..chunk_size],
                    chunk_out_l,
                    chunk_out_r,
                );
            } else {
                exciter.process_block(
                    &scratch_in_l[..chunk_size],
                    &scratch_in_r[..chunk_size],
                    chunk_out_l,
                    &mut scratch_out_r[..chunk_size],
                );
            }
            offset += chunk_size;
        }
        return;
    }

    let in_l_slice = std::slice::from_raw_parts(in_l, n);
    let in_r_slice = if !in_r.is_null() {
        std::slice::from_raw_parts(in_r, n)
    } else {
        in_l_slice
    };

    if !out_r.is_null() && !std::ptr::eq(out_l, out_r) {
        let out_l_slice = std::slice::from_raw_parts_mut(out_l, n);
        let out_r_slice = std::slice::from_raw_parts_mut(out_r, n);
        exciter.process_block(in_l_slice, in_r_slice, out_l_slice, out_r_slice);
    } else {
        let mut mono_scratch = [0.0f32; 1024];
        let mut offset = 0;
        let out_l_slice = std::slice::from_raw_parts_mut(out_l, n);
        while offset < n {
            let chunk_size = (n - offset).min(1024);
            let chunk_in_l = &in_l_slice[offset..offset + chunk_size];
            let chunk_in_r = &in_r_slice[offset..offset + chunk_size];
            let chunk_out_l = &mut out_l_slice[offset..offset + chunk_size];
            exciter.process_block(chunk_in_l, chunk_in_r, chunk_out_l, &mut mono_scratch[..chunk_size]);
            offset += chunk_size;
        }
    }
}

// ============================================================================
// Brickwall Limiter C-ABI (Rules 5, 8, 9, 14, 34, 46, 47)
// ============================================================================

#[no_mangle]
pub unsafe extern "C" fn n8_rust_brickwall_limiter_create(sample_rate: f32) -> *mut RustBrickwallLimiter {
    Box::into_raw(Box::new(RustBrickwallLimiter::new(sample_rate)))
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_brickwall_limiter_destroy(node: *mut RustBrickwallLimiter) {
    if !node.is_null() {
        drop(Box::from_raw(node));
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_brickwall_limiter_prepare(node: *mut RustBrickwallLimiter, sample_rate: f32) {
    if let Some(limiter) = node.as_mut() {
        limiter.prepare(sample_rate);
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_brickwall_limiter_reset(node: *mut RustBrickwallLimiter) {
    if let Some(limiter) = node.as_mut() {
        limiter.reset();
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_brickwall_limiter_set_parameter(
    node: *mut RustBrickwallLimiter,
    param_id: u32,
    value: f32,
) {
    if let Some(limiter) = node.as_mut() {
        limiter.set_parameter(param_id, value);
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_brickwall_limiter_get_parameter(
    node: *const RustBrickwallLimiter,
    param_id: u32,
) -> f32 {
    if let Some(limiter) = node.as_ref() {
        limiter.get_parameter(param_id)
    } else {
        0.0
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_brickwall_limiter_process(
    node: *mut RustBrickwallLimiter,
    in_l: *const f32,
    in_r: *const f32,
    out_l: *mut f32,
    out_r: *mut f32,
    num_samples: u32,
) {
    if node.is_null() || in_l.is_null() || out_l.is_null() || num_samples == 0 {
        return;
    }
    let limiter = &mut *node;
    let n = num_samples as usize;

    let is_in_place = std::ptr::eq(in_l, out_l as *const f32)
        || (!in_r.is_null() && !out_r.is_null() && std::ptr::eq(in_r, out_r as *const f32));

    if is_in_place {
        let mut scratch_in_l = [0.0f32; 1024];
        let mut scratch_in_r = [0.0f32; 1024];
        let mut scratch_out_r = [0.0f32; 1024];
        let is_mono_out = out_r.is_null() || std::ptr::eq(out_l, out_r);

        let mut offset = 0;
        while offset < n {
            let chunk_size = (n - offset).min(1024);
            std::ptr::copy_nonoverlapping(in_l.add(offset), scratch_in_l.as_mut_ptr(), chunk_size);
            if !in_r.is_null() {
                std::ptr::copy_nonoverlapping(in_r.add(offset), scratch_in_r.as_mut_ptr(), chunk_size);
            } else {
                scratch_in_r[..chunk_size].copy_from_slice(&scratch_in_l[..chunk_size]);
            }

            let chunk_out_l = std::slice::from_raw_parts_mut(out_l.add(offset), chunk_size);
            if !is_mono_out {
                let chunk_out_r = std::slice::from_raw_parts_mut(out_r.add(offset), chunk_size);
                limiter.process_block(
                    &scratch_in_l[..chunk_size],
                    &scratch_in_r[..chunk_size],
                    chunk_out_l,
                    chunk_out_r,
                );
            } else {
                limiter.process_block(
                    &scratch_in_l[..chunk_size],
                    &scratch_in_r[..chunk_size],
                    chunk_out_l,
                    &mut scratch_out_r[..chunk_size],
                );
            }
            offset += chunk_size;
        }
        return;
    }

    let in_l_slice = std::slice::from_raw_parts(in_l, n);
    let in_r_slice = if !in_r.is_null() {
        std::slice::from_raw_parts(in_r, n)
    } else {
        in_l_slice
    };

    if !out_r.is_null() && !std::ptr::eq(out_l, out_r) {
        let out_l_slice = std::slice::from_raw_parts_mut(out_l, n);
        let out_r_slice = std::slice::from_raw_parts_mut(out_r, n);
        limiter.process_block(in_l_slice, in_r_slice, out_l_slice, out_r_slice);
    } else {
        let mut mono_scratch = [0.0f32; 1024];
        let mut offset = 0;
        let out_l_slice = std::slice::from_raw_parts_mut(out_l, n);
        while offset < n {
            let chunk_size = (n - offset).min(1024);
            let chunk_in_l = &in_l_slice[offset..offset + chunk_size];
            let chunk_in_r = &in_r_slice[offset..offset + chunk_size];
            let chunk_out_l = &mut out_l_slice[offset..offset + chunk_size];
            limiter.process_block(chunk_in_l, chunk_in_r, chunk_out_l, &mut mono_scratch[..chunk_size]);
            offset += chunk_size;
        }
    }
}

// ============================================================================
// Multiband Dynamics C-ABI (Rules 5, 8, 9, 14, 32, 33, 34, 46, 47)
// ============================================================================

#[no_mangle]
pub unsafe extern "C" fn n8_rust_multiband_dynamics_create(sample_rate: f32) -> *mut RustMultibandDynamics {
    Box::into_raw(Box::new(RustMultibandDynamics::new(sample_rate)))
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_multiband_dynamics_destroy(node: *mut RustMultibandDynamics) {
    if !node.is_null() {
        drop(Box::from_raw(node));
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_multiband_dynamics_prepare(node: *mut RustMultibandDynamics, sample_rate: f32) {
    if let Some(mb) = node.as_mut() {
        mb.prepare(sample_rate);
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_multiband_dynamics_reset(node: *mut RustMultibandDynamics) {
    if let Some(mb) = node.as_mut() {
        mb.reset();
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_multiband_dynamics_set_parameter(
    node: *mut RustMultibandDynamics,
    param_id: u32,
    value: f32,
) {
    if let Some(mb) = node.as_mut() {
        mb.set_parameter(param_id, value);
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_multiband_dynamics_get_parameter(
    node: *const RustMultibandDynamics,
    param_id: u32,
) -> f32 {
    if let Some(mb) = node.as_ref() {
        mb.get_parameter(param_id)
    } else {
        0.0
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_multiband_dynamics_process(
    node: *mut RustMultibandDynamics,
    in_l: *const f32,
    in_r: *const f32,
    out_l: *mut f32,
    out_r: *mut f32,
    num_samples: u32,
) {
    if node.is_null() || in_l.is_null() || out_l.is_null() || num_samples == 0 {
        return;
    }
    let mb = &mut *node;
    let n = num_samples as usize;

    let is_in_place = std::ptr::eq(in_l, out_l as *const f32)
        || (!in_r.is_null() && !out_r.is_null() && std::ptr::eq(in_r, out_r as *const f32));

    if is_in_place {
        let mut scratch_in_l = [0.0f32; 1024];
        let mut scratch_in_r = [0.0f32; 1024];
        let mut scratch_out_r = [0.0f32; 1024];
        let is_mono_out = out_r.is_null() || std::ptr::eq(out_l, out_r);

        let mut offset = 0;
        while offset < n {
            let chunk_size = (n - offset).min(1024);
            std::ptr::copy_nonoverlapping(in_l.add(offset), scratch_in_l.as_mut_ptr(), chunk_size);
            if !in_r.is_null() {
                std::ptr::copy_nonoverlapping(in_r.add(offset), scratch_in_r.as_mut_ptr(), chunk_size);
            } else {
                scratch_in_r[..chunk_size].copy_from_slice(&scratch_in_l[..chunk_size]);
            }

            let chunk_out_l = std::slice::from_raw_parts_mut(out_l.add(offset), chunk_size);
            if !is_mono_out {
                let chunk_out_r = std::slice::from_raw_parts_mut(out_r.add(offset), chunk_size);
                mb.process_block(
                    &scratch_in_l[..chunk_size],
                    &scratch_in_r[..chunk_size],
                    chunk_out_l,
                    chunk_out_r,
                );
            } else {
                mb.process_block(
                    &scratch_in_l[..chunk_size],
                    &scratch_in_r[..chunk_size],
                    chunk_out_l,
                    &mut scratch_out_r[..chunk_size],
                );
            }
            offset += chunk_size;
        }
        return;
    }

    let in_l_slice = std::slice::from_raw_parts(in_l, n);
    let in_r_slice = if !in_r.is_null() {
        std::slice::from_raw_parts(in_r, n)
    } else {
        in_l_slice
    };

    if !out_r.is_null() && !std::ptr::eq(out_l, out_r) {
        let out_l_slice = std::slice::from_raw_parts_mut(out_l, n);
        let out_r_slice = std::slice::from_raw_parts_mut(out_r, n);
        mb.process_block(in_l_slice, in_r_slice, out_l_slice, out_r_slice);
    } else {
        let mut mono_scratch = [0.0f32; 1024];
        let mut offset = 0;
        let out_l_slice = std::slice::from_raw_parts_mut(out_l, n);
        while offset < n {
            let chunk_size = (n - offset).min(1024);
            let chunk_in_l = &in_l_slice[offset..offset + chunk_size];
            let chunk_in_r = &in_r_slice[offset..offset + chunk_size];
            let chunk_out_l = &mut out_l_slice[offset..offset + chunk_size];
            mb.process_block(chunk_in_l, chunk_in_r, chunk_out_l, &mut mono_scratch[..chunk_size]);
            offset += chunk_size;
        }
    }
}

// ============================================================================
// Resonator Bank C-ABI (Rules 5, 8, 9, 12, 14, 18, 32, 46, 47)
// ============================================================================

#[no_mangle]
pub unsafe extern "C" fn n8_rust_resonator_bank_create(sample_rate: f32) -> *mut RustResonatorBank {
    Box::into_raw(Box::new(RustResonatorBank::new(sample_rate)))
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_resonator_bank_destroy(node: *mut RustResonatorBank) {
    if !node.is_null() {
        drop(Box::from_raw(node));
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_resonator_bank_prepare(node: *mut RustResonatorBank, sample_rate: f32) {
    if let Some(bank) = node.as_mut() {
        bank.prepare(sample_rate);
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_resonator_bank_reset(node: *mut RustResonatorBank) {
    if let Some(bank) = node.as_mut() {
        bank.reset();
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_resonator_bank_set_parameter(
    node: *mut RustResonatorBank,
    param_id: u32,
    value: f32,
) {
    if let Some(bank) = node.as_mut() {
        bank.set_parameter(param_id, value);
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_resonator_bank_get_parameter(
    node: *const RustResonatorBank,
    param_id: u32,
) -> f32 {
    if let Some(bank) = node.as_ref() {
        bank.get_parameter(param_id)
    } else {
        0.0
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_resonator_bank_get_tail_samples(node: *const RustResonatorBank) -> u32 {
    if let Some(bank) = node.as_ref() {
        bank.get_tail_samples()
    } else {
        0
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_resonator_bank_process(
    node: *mut RustResonatorBank,
    in_l: *const f32,
    in_r: *const f32,
    out_l: *mut f32,
    out_r: *mut f32,
    num_samples: u32,
) {
    if node.is_null() || in_l.is_null() || out_l.is_null() || num_samples == 0 {
        return;
    }
    let bank = &mut *node;
    let n = num_samples as usize;

    let is_in_place = std::ptr::eq(in_l, out_l as *const f32)
        || (!in_r.is_null() && !out_r.is_null() && std::ptr::eq(in_r, out_r as *const f32));

    if is_in_place {
        let mut scratch_in_l = [0.0f32; 1024];
        let mut scratch_in_r = [0.0f32; 1024];
        let mut scratch_out_r = [0.0f32; 1024];
        let is_mono_out = out_r.is_null() || std::ptr::eq(out_l, out_r);

        let mut offset = 0;
        while offset < n {
            let chunk_size = (n - offset).min(1024);
            std::ptr::copy_nonoverlapping(in_l.add(offset), scratch_in_l.as_mut_ptr(), chunk_size);
            if !in_r.is_null() {
                std::ptr::copy_nonoverlapping(in_r.add(offset), scratch_in_r.as_mut_ptr(), chunk_size);
            } else {
                scratch_in_r[..chunk_size].copy_from_slice(&scratch_in_l[..chunk_size]);
            }

            let chunk_out_l = std::slice::from_raw_parts_mut(out_l.add(offset), chunk_size);
            if !is_mono_out {
                let chunk_out_r = std::slice::from_raw_parts_mut(out_r.add(offset), chunk_size);
                bank.process_block(
                    &scratch_in_l[..chunk_size],
                    &scratch_in_r[..chunk_size],
                    chunk_out_l,
                    chunk_out_r,
                );
            } else {
                bank.process_block(
                    &scratch_in_l[..chunk_size],
                    &scratch_in_r[..chunk_size],
                    chunk_out_l,
                    &mut scratch_out_r[..chunk_size],
                );
            }
            offset += chunk_size;
        }
        return;
    }

    let in_l_slice = std::slice::from_raw_parts(in_l, n);
    let in_r_slice = if !in_r.is_null() {
        std::slice::from_raw_parts(in_r, n)
    } else {
        in_l_slice
    };

    if !out_r.is_null() && !std::ptr::eq(out_l, out_r) {
        let out_l_slice = std::slice::from_raw_parts_mut(out_l, n);
        let out_r_slice = std::slice::from_raw_parts_mut(out_r, n);
        bank.process_block(in_l_slice, in_r_slice, out_l_slice, out_r_slice);
    } else {
        let mut mono_scratch = [0.0f32; 1024];
        let mut offset = 0;
        let out_l_slice = std::slice::from_raw_parts_mut(out_l, n);
        while offset < n {
            let chunk_size = (n - offset).min(1024);
            let chunk_in_l = &in_l_slice[offset..offset + chunk_size];
            let chunk_in_r = &in_r_slice[offset..offset + chunk_size];
            let chunk_out_l = &mut out_l_slice[offset..offset + chunk_size];
            bank.process_block(chunk_in_l, chunk_in_r, chunk_out_l, &mut mono_scratch[..chunk_size]);
            offset += chunk_size;
        }
    }
}

// ============================================================================
// Bitcrusher C-ABI (Rules 5, 8, 9, 14, 34, 46, 47)
// ============================================================================

#[no_mangle]
pub unsafe extern "C" fn n8_rust_bitcrusher_create(sample_rate: f32) -> *mut RustBitcrusher {
    Box::into_raw(Box::new(RustBitcrusher::new(sample_rate)))
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_bitcrusher_destroy(node: *mut RustBitcrusher) {
    if !node.is_null() {
        drop(Box::from_raw(node));
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_bitcrusher_prepare(node: *mut RustBitcrusher, sample_rate: f32) {
    if let Some(bc) = node.as_mut() {
        bc.prepare(sample_rate);
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_bitcrusher_reset(node: *mut RustBitcrusher) {
    if let Some(bc) = node.as_mut() {
        bc.reset();
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_bitcrusher_set_parameter(
    node: *mut RustBitcrusher,
    param_id: u32,
    value: f32,
) {
    if let Some(bc) = node.as_mut() {
        bc.set_parameter(param_id, value);
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_bitcrusher_get_parameter(
    node: *const RustBitcrusher,
    param_id: u32,
) -> f32 {
    if let Some(bc) = node.as_ref() {
        bc.get_parameter(param_id)
    } else {
        0.0
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_bitcrusher_process(
    node: *mut RustBitcrusher,
    in_l: *const f32,
    in_r: *const f32,
    out_l: *mut f32,
    out_r: *mut f32,
    num_samples: u32,
) {
    if node.is_null() || in_l.is_null() || out_l.is_null() || num_samples == 0 {
        return;
    }
    let bc = &mut *node;
    let n = num_samples as usize;

    let is_in_place = std::ptr::eq(in_l, out_l as *const f32)
        || (!in_r.is_null() && !out_r.is_null() && std::ptr::eq(in_r, out_r as *const f32));

    if is_in_place {
        let mut scratch_in_l = [0.0f32; 1024];
        let mut scratch_in_r = [0.0f32; 1024];
        let mut scratch_out_r = [0.0f32; 1024];
        let is_mono_out = out_r.is_null() || std::ptr::eq(out_l, out_r);

        let mut offset = 0;
        while offset < n {
            let chunk_size = (n - offset).min(1024);
            std::ptr::copy_nonoverlapping(in_l.add(offset), scratch_in_l.as_mut_ptr(), chunk_size);
            if !in_r.is_null() {
                std::ptr::copy_nonoverlapping(in_r.add(offset), scratch_in_r.as_mut_ptr(), chunk_size);
            } else {
                scratch_in_r[..chunk_size].copy_from_slice(&scratch_in_l[..chunk_size]);
            }

            let chunk_out_l = std::slice::from_raw_parts_mut(out_l.add(offset), chunk_size);
            if !is_mono_out {
                let chunk_out_r = std::slice::from_raw_parts_mut(out_r.add(offset), chunk_size);
                bc.process_block(
                    &scratch_in_l[..chunk_size],
                    &scratch_in_r[..chunk_size],
                    chunk_out_l,
                    chunk_out_r,
                );
            } else {
                bc.process_block(
                    &scratch_in_l[..chunk_size],
                    &scratch_in_r[..chunk_size],
                    chunk_out_l,
                    &mut scratch_out_r[..chunk_size],
                );
            }
            offset += chunk_size;
        }
        return;
    }

    let in_l_slice = std::slice::from_raw_parts(in_l, n);
    let in_r_slice = if !in_r.is_null() {
        std::slice::from_raw_parts(in_r, n)
    } else {
        in_l_slice
    };

    if !out_r.is_null() && !std::ptr::eq(out_l, out_r) {
        let out_l_slice = std::slice::from_raw_parts_mut(out_l, n);
        let out_r_slice = std::slice::from_raw_parts_mut(out_r, n);
        bc.process_block(in_l_slice, in_r_slice, out_l_slice, out_r_slice);
    } else {
        let mut mono_scratch = [0.0f32; 1024];
        let mut offset = 0;
        let out_l_slice = std::slice::from_raw_parts_mut(out_l, n);
        while offset < n {
            let chunk_size = (n - offset).min(1024);
            let chunk_in_l = &in_l_slice[offset..offset + chunk_size];
            let chunk_in_r = &in_r_slice[offset..offset + chunk_size];
            let chunk_out_l = &mut out_l_slice[offset..offset + chunk_size];
            bc.process_block(chunk_in_l, chunk_in_r, chunk_out_l, &mut mono_scratch[..chunk_size]);
            offset += chunk_size;
        }
    }
}

// ============================================================================
// Noise Gate C-ABI (Rules 5, 8, 9, 14, 34, 46, 47)
// ============================================================================

#[no_mangle]
pub unsafe extern "C" fn n8_rust_noise_gate_create(sample_rate: f32) -> *mut RustNoiseGate {
    Box::into_raw(Box::new(RustNoiseGate::new(sample_rate)))
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_noise_gate_destroy(node: *mut RustNoiseGate) {
    if !node.is_null() {
        drop(Box::from_raw(node));
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_noise_gate_prepare(node: *mut RustNoiseGate, sample_rate: f32) {
    if let Some(gate) = node.as_mut() {
        gate.prepare(sample_rate);
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_noise_gate_reset(node: *mut RustNoiseGate) {
    if let Some(gate) = node.as_mut() {
        gate.reset();
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_noise_gate_set_parameter(
    node: *mut RustNoiseGate,
    param_id: u32,
    value: f32,
) {
    if let Some(gate) = node.as_mut() {
        gate.set_parameter(param_id, value);
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_noise_gate_get_parameter(
    node: *const RustNoiseGate,
    param_id: u32,
) -> f32 {
    if let Some(gate) = node.as_ref() {
        gate.get_parameter(param_id)
    } else {
        0.0
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_noise_gate_process(
    node: *mut RustNoiseGate,
    in_l: *const f32,
    in_r: *const f32,
    sc_l: *const f32,
    sc_r: *const f32,
    out_l: *mut f32,
    out_r: *mut f32,
    num_samples: u32,
) {
    if node.is_null() || in_l.is_null() || out_l.is_null() || num_samples == 0 {
        return;
    }
    let gate = &mut *node;
    let n = num_samples as usize;

    let is_in_place = std::ptr::eq(in_l, out_l as *const f32)
        || (!in_r.is_null() && !out_r.is_null() && std::ptr::eq(in_r, out_r as *const f32));

    let sc_l_slice = if !sc_l.is_null() {
        Some(std::slice::from_raw_parts(sc_l, n))
    } else {
        None
    };
    let sc_r_slice = if !sc_r.is_null() {
        Some(std::slice::from_raw_parts(sc_r, n))
    } else {
        sc_l_slice
    };

    if is_in_place {
        let mut scratch_in_l = [0.0f32; 1024];
        let mut scratch_in_r = [0.0f32; 1024];
        let mut scratch_out_r = [0.0f32; 1024];
        let is_mono_out = out_r.is_null() || std::ptr::eq(out_l, out_r);

        let mut offset = 0;
        while offset < n {
            let chunk_size = (n - offset).min(1024);
            std::ptr::copy_nonoverlapping(in_l.add(offset), scratch_in_l.as_mut_ptr(), chunk_size);
            if !in_r.is_null() {
                std::ptr::copy_nonoverlapping(in_r.add(offset), scratch_in_r.as_mut_ptr(), chunk_size);
            } else {
                scratch_in_r[..chunk_size].copy_from_slice(&scratch_in_l[..chunk_size]);
            }

            let chunk_sc_l = sc_l_slice.map(|s| &s[offset..offset + chunk_size]);
            let chunk_sc_r = sc_r_slice.map(|s| &s[offset..offset + chunk_size]);

            let chunk_out_l = std::slice::from_raw_parts_mut(out_l.add(offset), chunk_size);
            if !is_mono_out {
                let chunk_out_r = std::slice::from_raw_parts_mut(out_r.add(offset), chunk_size);
                gate.process_block(
                    &scratch_in_l[..chunk_size],
                    &scratch_in_r[..chunk_size],
                    chunk_sc_l,
                    chunk_sc_r,
                    chunk_out_l,
                    chunk_out_r,
                );
            } else {
                gate.process_block(
                    &scratch_in_l[..chunk_size],
                    &scratch_in_r[..chunk_size],
                    chunk_sc_l,
                    chunk_sc_r,
                    chunk_out_l,
                    &mut scratch_out_r[..chunk_size],
                );
            }
            offset += chunk_size;
        }
        return;
    }

    let in_l_slice = std::slice::from_raw_parts(in_l, n);
    let in_r_slice = if !in_r.is_null() {
        std::slice::from_raw_parts(in_r, n)
    } else {
        in_l_slice
    };

    if !out_r.is_null() && !std::ptr::eq(out_l, out_r) {
        let out_l_slice = std::slice::from_raw_parts_mut(out_l, n);
        let out_r_slice = std::slice::from_raw_parts_mut(out_r, n);
        gate.process_block(in_l_slice, in_r_slice, sc_l_slice, sc_r_slice, out_l_slice, out_r_slice);
    } else {
        let mut mono_scratch = [0.0f32; 1024];
        let mut offset = 0;
        let out_l_slice = std::slice::from_raw_parts_mut(out_l, n);
        while offset < n {
            let chunk_size = (n - offset).min(1024);
            let chunk_in_l = &in_l_slice[offset..offset + chunk_size];
            let chunk_in_r = &in_r_slice[offset..offset + chunk_size];
            let chunk_sc_l = sc_l_slice.map(|s| &s[offset..offset + chunk_size]);
            let chunk_sc_r = sc_r_slice.map(|s| &s[offset..offset + chunk_size]);
            let chunk_out_l = &mut out_l_slice[offset..offset + chunk_size];
            gate.process_block(chunk_in_l, chunk_in_r, chunk_sc_l, chunk_sc_r, chunk_out_l, &mut mono_scratch[..chunk_size]);
            offset += chunk_size;
        }
    }
}

// ============================================================================
// De-Esser C-ABI (Rules 5, 8, 9, 14, 34, 46, 47)
// ============================================================================

#[no_mangle]
pub unsafe extern "C" fn n8_rust_de_esser_create(sample_rate: f32) -> *mut RustDeEsser {
    Box::into_raw(Box::new(RustDeEsser::new(sample_rate)))
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_de_esser_destroy(node: *mut RustDeEsser) {
    if !node.is_null() {
        drop(Box::from_raw(node));
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_de_esser_prepare(node: *mut RustDeEsser, sample_rate: f32) {
    if let Some(deesser) = node.as_mut() {
        deesser.prepare(sample_rate);
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_de_esser_reset(node: *mut RustDeEsser) {
    if let Some(deesser) = node.as_mut() {
        deesser.reset();
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_de_esser_set_parameter(
    node: *mut RustDeEsser,
    param_id: u32,
    value: f32,
) {
    if let Some(deesser) = node.as_mut() {
        deesser.set_parameter(param_id, value);
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_de_esser_get_parameter(
    node: *const RustDeEsser,
    param_id: u32,
) -> f32 {
    if let Some(deesser) = node.as_ref() {
        deesser.get_parameter(param_id)
    } else {
        0.0
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_de_esser_process(
    node: *mut RustDeEsser,
    in_l: *const f32,
    in_r: *const f32,
    out_l: *mut f32,
    out_r: *mut f32,
    num_samples: u32,
) {
    if node.is_null() || in_l.is_null() || out_l.is_null() || num_samples == 0 {
        return;
    }
    let deesser = &mut *node;
    let n = num_samples as usize;

    let is_in_place = std::ptr::eq(in_l, out_l as *const f32)
        || (!in_r.is_null() && !out_r.is_null() && std::ptr::eq(in_r, out_r as *const f32));

    if is_in_place {
        let mut scratch_in_l = [0.0f32; 1024];
        let mut scratch_in_r = [0.0f32; 1024];
        let mut scratch_out_r = [0.0f32; 1024];
        let is_mono_out = out_r.is_null() || std::ptr::eq(out_l, out_r);

        let mut offset = 0;
        while offset < n {
            let chunk_size = (n - offset).min(1024);
            std::ptr::copy_nonoverlapping(in_l.add(offset), scratch_in_l.as_mut_ptr(), chunk_size);
            if !in_r.is_null() {
                std::ptr::copy_nonoverlapping(in_r.add(offset), scratch_in_r.as_mut_ptr(), chunk_size);
            } else {
                scratch_in_r[..chunk_size].copy_from_slice(&scratch_in_l[..chunk_size]);
            }

            let chunk_out_l = std::slice::from_raw_parts_mut(out_l.add(offset), chunk_size);
            if !is_mono_out {
                let chunk_out_r = std::slice::from_raw_parts_mut(out_r.add(offset), chunk_size);
                deesser.process_block(
                    &scratch_in_l[..chunk_size],
                    &scratch_in_r[..chunk_size],
                    chunk_out_l,
                    chunk_out_r,
                );
            } else {
                deesser.process_block(
                    &scratch_in_l[..chunk_size],
                    &scratch_in_r[..chunk_size],
                    chunk_out_l,
                    &mut scratch_out_r[..chunk_size],
                );
            }
            offset += chunk_size;
        }
        return;
    }

    let in_l_slice = std::slice::from_raw_parts(in_l, n);
    let in_r_slice = if !in_r.is_null() {
        std::slice::from_raw_parts(in_r, n)
    } else {
        in_l_slice
    };

    if !out_r.is_null() && !std::ptr::eq(out_l, out_r) {
        let out_l_slice = std::slice::from_raw_parts_mut(out_l, n);
        let out_r_slice = std::slice::from_raw_parts_mut(out_r, n);
        deesser.process_block(in_l_slice, in_r_slice, out_l_slice, out_r_slice);
    } else {
        let mut mono_scratch = [0.0f32; 1024];
        let mut offset = 0;
        let out_l_slice = std::slice::from_raw_parts_mut(out_l, n);
        while offset < n {
            let chunk_size = (n - offset).min(1024);
            let chunk_in_l = &in_l_slice[offset..offset + chunk_size];
            let chunk_in_r = &in_r_slice[offset..offset + chunk_size];
            let chunk_out_l = &mut out_l_slice[offset..offset + chunk_size];
            deesser.process_block(chunk_in_l, chunk_in_r, chunk_out_l, &mut mono_scratch[..chunk_size]);
            offset += chunk_size;
        }
    }
}

// ============================================================================
// Auto-Wah Filter C-ABI (Rules 5, 8, 9, 14, 34, 46, 47)
// ============================================================================

#[no_mangle]
pub unsafe extern "C" fn n8_rust_auto_wah_create(sample_rate: f32) -> *mut RustAutoWah {
    Box::into_raw(Box::new(RustAutoWah::new(sample_rate)))
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_auto_wah_destroy(node: *mut RustAutoWah) {
    if !node.is_null() {
        drop(Box::from_raw(node));
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_auto_wah_prepare(node: *mut RustAutoWah, sample_rate: f32) {
    if let Some(aw) = node.as_mut() {
        aw.prepare(sample_rate);
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_auto_wah_reset(node: *mut RustAutoWah) {
    if let Some(aw) = node.as_mut() {
        aw.reset();
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_auto_wah_set_parameter(
    node: *mut RustAutoWah,
    param_id: u32,
    value: f32,
) {
    if let Some(aw) = node.as_mut() {
        aw.set_parameter(param_id, value);
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_auto_wah_get_parameter(
    node: *const RustAutoWah,
    param_id: u32,
) -> f32 {
    if let Some(aw) = node.as_ref() {
        aw.get_parameter(param_id)
    } else {
        0.0
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_auto_wah_process(
    node: *mut RustAutoWah,
    in_l: *const f32,
    in_r: *const f32,
    sc_l: *const f32,
    sc_r: *const f32,
    out_l: *mut f32,
    out_r: *mut f32,
    num_samples: u32,
) {
    let _ = sc_r; // Sidechain detector uses sc_l
    if node.is_null() || in_l.is_null() || out_l.is_null() || num_samples == 0 {
        return;
    }
    let aw = &mut *node;
    let n = num_samples as usize;

    let sc_l_slice = if !sc_l.is_null() {
        Some(std::slice::from_raw_parts(sc_l, n))
    } else {
        None
    };

    let is_in_place = std::ptr::eq(in_l, out_l as *const f32)
        || (!in_r.is_null() && !out_r.is_null() && std::ptr::eq(in_r, out_r as *const f32));

    if is_in_place {
        let mut scratch_in_l = [0.0f32; 1024];
        let mut scratch_in_r = [0.0f32; 1024];
        let mut scratch_out_r = [0.0f32; 1024];
        let is_mono_out = out_r.is_null() || std::ptr::eq(out_l, out_r);

        let mut offset = 0;
        while offset < n {
            let chunk_size = (n - offset).min(1024);
            std::ptr::copy_nonoverlapping(in_l.add(offset), scratch_in_l.as_mut_ptr(), chunk_size);
            if !in_r.is_null() {
                std::ptr::copy_nonoverlapping(in_r.add(offset), scratch_in_r.as_mut_ptr(), chunk_size);
            } else {
                scratch_in_r[..chunk_size].copy_from_slice(&scratch_in_l[..chunk_size]);
            }

            let chunk_sc_l = sc_l_slice.map(|s| &s[offset..offset + chunk_size]);
            let chunk_out_l = std::slice::from_raw_parts_mut(out_l.add(offset), chunk_size);
            if !is_mono_out {
                let chunk_out_r = std::slice::from_raw_parts_mut(out_r.add(offset), chunk_size);
                aw.process_block(
                    &scratch_in_l[..chunk_size],
                    &scratch_in_r[..chunk_size],
                    chunk_sc_l,
                    chunk_out_l,
                    chunk_out_r,
                );
            } else {
                aw.process_block(
                    &scratch_in_l[..chunk_size],
                    &scratch_in_r[..chunk_size],
                    chunk_sc_l,
                    chunk_out_l,
                    &mut scratch_out_r[..chunk_size],
                );
            }
            offset += chunk_size;
        }
        return;
    }

    let in_l_slice = std::slice::from_raw_parts(in_l, n);
    let in_r_slice = if !in_r.is_null() {
        std::slice::from_raw_parts(in_r, n)
    } else {
        in_l_slice
    };

    if !out_r.is_null() && !std::ptr::eq(out_l, out_r) {
        let out_l_slice = std::slice::from_raw_parts_mut(out_l, n);
        let out_r_slice = std::slice::from_raw_parts_mut(out_r, n);
        aw.process_block(in_l_slice, in_r_slice, sc_l_slice, out_l_slice, out_r_slice);
    } else {
        let mut mono_scratch = [0.0f32; 1024];
        let mut offset = 0;
        let out_l_slice = std::slice::from_raw_parts_mut(out_l, n);
        while offset < n {
            let chunk_size = (n - offset).min(1024);
            let chunk_in_l = &in_l_slice[offset..offset + chunk_size];
            let chunk_in_r = &in_r_slice[offset..offset + chunk_size];
            let chunk_sc_l = sc_l_slice.map(|s| &s[offset..offset + chunk_size]);
            let chunk_out_l = &mut out_l_slice[offset..offset + chunk_size];
            aw.process_block(chunk_in_l, chunk_in_r, chunk_sc_l, chunk_out_l, &mut mono_scratch[..chunk_size]);
            offset += chunk_size;
        }
    }
}

// ============================================================================
// VCA Compressor C-ABI (Rules 5, 8, 9, 14, 34, 46, 47)
// ============================================================================

#[no_mangle]
pub unsafe extern "C" fn n8_rust_compressor_create(sample_rate: f32) -> *mut RustCompressor {
    Box::into_raw(Box::new(RustCompressor::new(sample_rate)))
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_compressor_destroy(node: *mut RustCompressor) {
    if !node.is_null() {
        drop(Box::from_raw(node));
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_compressor_prepare(node: *mut RustCompressor, sample_rate: f32) {
    if let Some(comp) = node.as_mut() {
        comp.prepare(sample_rate);
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_compressor_reset(node: *mut RustCompressor) {
    if let Some(comp) = node.as_mut() {
        comp.reset();
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_compressor_set_parameter(
    node: *mut RustCompressor,
    param_id: u32,
    value: f32,
) {
    if let Some(comp) = node.as_mut() {
        comp.set_parameter(param_id, value);
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_compressor_get_parameter(
    node: *const RustCompressor,
    param_id: u32,
) -> f32 {
    if let Some(comp) = node.as_ref() {
        comp.get_parameter(param_id)
    } else {
        0.0
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_compressor_process(
    node: *mut RustCompressor,
    in_l: *const f32,
    in_r: *const f32,
    sc_l: *const f32,
    sc_r: *const f32,
    out_l: *mut f32,
    out_r: *mut f32,
    num_samples: u32,
) {
    if node.is_null() || in_l.is_null() || out_l.is_null() || num_samples == 0 {
        return;
    }
    let comp = &mut *node;
    let n = num_samples as usize;

    let sc_l_slice = if !sc_l.is_null() {
        Some(std::slice::from_raw_parts(sc_l, n))
    } else {
        None
    };
    let sc_r_slice = if !sc_r.is_null() {
        Some(std::slice::from_raw_parts(sc_r, n))
    } else {
        None
    };

    let is_in_place = std::ptr::eq(in_l, out_l as *const f32)
        || (!in_r.is_null() && !out_r.is_null() && std::ptr::eq(in_r, out_r as *const f32));

    if is_in_place {
        let mut scratch_in_l = [0.0f32; 1024];
        let mut scratch_in_r = [0.0f32; 1024];
        let mut scratch_out_r = [0.0f32; 1024];
        let is_mono_out = out_r.is_null() || std::ptr::eq(out_l, out_r);

        let mut offset = 0;
        while offset < n {
            let chunk_size = (n - offset).min(1024);
            std::ptr::copy_nonoverlapping(in_l.add(offset), scratch_in_l.as_mut_ptr(), chunk_size);
            if !in_r.is_null() {
                std::ptr::copy_nonoverlapping(in_r.add(offset), scratch_in_r.as_mut_ptr(), chunk_size);
            } else {
                scratch_in_r[..chunk_size].copy_from_slice(&scratch_in_l[..chunk_size]);
            }

            let chunk_sc_l = sc_l_slice.map(|s| &s[offset..offset + chunk_size]);
            let chunk_sc_r = sc_r_slice.map(|s| &s[offset..offset + chunk_size]);
            let chunk_out_l = std::slice::from_raw_parts_mut(out_l.add(offset), chunk_size);

            if !is_mono_out {
                let chunk_out_r = std::slice::from_raw_parts_mut(out_r.add(offset), chunk_size);
                comp.process_block(
                    &scratch_in_l[..chunk_size],
                    &scratch_in_r[..chunk_size],
                    chunk_sc_l,
                    chunk_sc_r,
                    chunk_out_l,
                    chunk_out_r,
                );
            } else {
                comp.process_block(
                    &scratch_in_l[..chunk_size],
                    &scratch_in_r[..chunk_size],
                    chunk_sc_l,
                    chunk_sc_r,
                    chunk_out_l,
                    &mut scratch_out_r[..chunk_size],
                );
            }
            offset += chunk_size;
        }
        return;
    }

    let in_l_slice = std::slice::from_raw_parts(in_l, n);
    let in_r_slice = if !in_r.is_null() {
        std::slice::from_raw_parts(in_r, n)
    } else {
        in_l_slice
    };

    if !out_r.is_null() && !std::ptr::eq(out_l, out_r) {
        let out_l_slice = std::slice::from_raw_parts_mut(out_l, n);
        let out_r_slice = std::slice::from_raw_parts_mut(out_r, n);
        comp.process_block(in_l_slice, in_r_slice, sc_l_slice, sc_r_slice, out_l_slice, out_r_slice);
    } else {
        let mut mono_scratch = [0.0f32; 1024];
        let mut offset = 0;
        let out_l_slice = std::slice::from_raw_parts_mut(out_l, n);
        while offset < n {
            let chunk_size = (n - offset).min(1024);
            let chunk_in_l = &in_l_slice[offset..offset + chunk_size];
            let chunk_in_r = &in_r_slice[offset..offset + chunk_size];
            let chunk_sc_l = sc_l_slice.map(|s| &s[offset..offset + chunk_size]);
            let chunk_sc_r = sc_r_slice.map(|s| &s[offset..offset + chunk_size]);
            let chunk_out_l = &mut out_l_slice[offset..offset + chunk_size];
            comp.process_block(chunk_in_l, chunk_in_r, chunk_sc_l, chunk_sc_r, chunk_out_l, &mut mono_scratch[..chunk_size]);
            offset += chunk_size;
        }
    }
}

// ============================================================================
// Parametric EQ C-ABI (Rules 5, 8, 9, 14, 34, 46, 47)
// ============================================================================

#[no_mangle]
pub unsafe extern "C" fn n8_rust_parametric_eq_create(sample_rate: f32) -> *mut RustParametricEq {
    Box::into_raw(Box::new(RustParametricEq::new(sample_rate)))
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_parametric_eq_destroy(node: *mut RustParametricEq) {
    if !node.is_null() {
        drop(Box::from_raw(node));
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_parametric_eq_prepare(node: *mut RustParametricEq, sample_rate: f32) {
    if let Some(eq) = node.as_mut() {
        eq.prepare(sample_rate);
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_parametric_eq_reset(node: *mut RustParametricEq) {
    if let Some(eq) = node.as_mut() {
        eq.reset();
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_parametric_eq_set_parameter(
    node: *mut RustParametricEq,
    param_id: u32,
    value: f32,
) {
    if let Some(eq) = node.as_mut() {
        eq.set_parameter(param_id, value);
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_parametric_eq_get_parameter(
    node: *const RustParametricEq,
    param_id: u32,
) -> f32 {
    if let Some(eq) = node.as_ref() {
        eq.get_parameter(param_id)
    } else {
        0.0
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_parametric_eq_process(
    node: *mut RustParametricEq,
    in_l: *const f32,
    in_r: *const f32,
    out_l: *mut f32,
    out_r: *mut f32,
    num_samples: u32,
) {
    if node.is_null() || in_l.is_null() || out_l.is_null() || num_samples == 0 {
        return;
    }
    let eq = &mut *node;
    let n = num_samples as usize;

    let is_in_place = std::ptr::eq(in_l, out_l as *const f32)
        || (!in_r.is_null() && !out_r.is_null() && std::ptr::eq(in_r, out_r as *const f32));

    if is_in_place {
        let mut scratch_in_l = [0.0f32; 1024];
        let mut scratch_in_r = [0.0f32; 1024];
        let mut scratch_out_r = [0.0f32; 1024];
        let is_mono_out = out_r.is_null() || std::ptr::eq(out_l, out_r);

        let mut offset = 0;
        while offset < n {
            let chunk_size = (n - offset).min(1024);
            std::ptr::copy_nonoverlapping(in_l.add(offset), scratch_in_l.as_mut_ptr(), chunk_size);
            if !in_r.is_null() {
                std::ptr::copy_nonoverlapping(in_r.add(offset), scratch_in_r.as_mut_ptr(), chunk_size);
            } else {
                scratch_in_r[..chunk_size].copy_from_slice(&scratch_in_l[..chunk_size]);
            }

            let chunk_out_l = std::slice::from_raw_parts_mut(out_l.add(offset), chunk_size);

            if !is_mono_out {
                let chunk_out_r = std::slice::from_raw_parts_mut(out_r.add(offset), chunk_size);
                eq.process_block(
                    &scratch_in_l[..chunk_size],
                    &scratch_in_r[..chunk_size],
                    chunk_out_l,
                    chunk_out_r,
                );
            } else {
                eq.process_block(
                    &scratch_in_l[..chunk_size],
                    &scratch_in_r[..chunk_size],
                    chunk_out_l,
                    &mut scratch_out_r[..chunk_size],
                );
            }
            offset += chunk_size;
        }
        return;
    }

    let in_l_slice = std::slice::from_raw_parts(in_l, n);
    let in_r_slice = if !in_r.is_null() {
        std::slice::from_raw_parts(in_r, n)
    } else {
        in_l_slice
    };

    if !out_r.is_null() && !std::ptr::eq(out_l, out_r) {
        let out_l_slice = std::slice::from_raw_parts_mut(out_l, n);
        let out_r_slice = std::slice::from_raw_parts_mut(out_r, n);
        eq.process_block(in_l_slice, in_r_slice, out_l_slice, out_r_slice);
    } else {
        let mut mono_scratch = [0.0f32; 1024];
        let mut offset = 0;
        let out_l_slice = std::slice::from_raw_parts_mut(out_l, n);
        while offset < n {
            let chunk_size = (n - offset).min(1024);
            let chunk_in_l = &in_l_slice[offset..offset + chunk_size];
            let chunk_in_r = &in_r_slice[offset..offset + chunk_size];
            let chunk_out_l = &mut out_l_slice[offset..offset + chunk_size];
            eq.process_block(chunk_in_l, chunk_in_r, chunk_out_l, &mut mono_scratch[..chunk_size]);
            offset += chunk_size;
        }
    }
}

// ============================================================================
// Stereo Imager C-ABI (Rules 5, 8, 9, 14, 16, 34, 46, 47)
// ============================================================================

#[no_mangle]
pub unsafe extern "C" fn n8_rust_stereo_imager_create(sample_rate: f32) -> *mut RustStereoImager {
    Box::into_raw(Box::new(RustStereoImager::new(sample_rate)))
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_stereo_imager_destroy(node: *mut RustStereoImager) {
    if !node.is_null() {
        drop(Box::from_raw(node));
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_stereo_imager_prepare(node: *mut RustStereoImager, sample_rate: f32) {
    if let Some(imager) = node.as_mut() {
        imager.prepare(sample_rate);
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_stereo_imager_reset(node: *mut RustStereoImager) {
    if let Some(imager) = node.as_mut() {
        imager.reset();
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_stereo_imager_set_parameter(
    node: *mut RustStereoImager,
    param_id: u32,
    value: f32,
) {
    if let Some(imager) = node.as_mut() {
        imager.set_parameter(param_id, value);
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_stereo_imager_get_parameter(
    node: *const RustStereoImager,
    param_id: u32,
) -> f32 {
    if let Some(imager) = node.as_ref() {
        imager.get_parameter(param_id)
    } else {
        0.0
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_stereo_imager_get_phase_correlation(
    node: *const RustStereoImager,
) -> f32 {
    if let Some(imager) = node.as_ref() {
        imager.get_phase_correlation()
    } else {
        1.0
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_stereo_imager_process(
    node: *mut RustStereoImager,
    in_l: *const f32,
    in_r: *const f32,
    out_l: *mut f32,
    out_r: *mut f32,
    num_samples: u32,
) {
    if node.is_null() || in_l.is_null() || out_l.is_null() || num_samples == 0 {
        return;
    }
    let imager = &mut *node;
    let n = num_samples as usize;

    let is_in_place = std::ptr::eq(in_l, out_l as *const f32)
        || (!in_r.is_null() && !out_r.is_null() && std::ptr::eq(in_r, out_r as *const f32));

    if is_in_place {
        let mut scratch_in_l = [0.0f32; 1024];
        let mut scratch_in_r = [0.0f32; 1024];
        let mut scratch_out_r = [0.0f32; 1024];
        let is_mono_out = out_r.is_null() || std::ptr::eq(out_l, out_r);

        let mut offset = 0;
        while offset < n {
            let chunk_size = (n - offset).min(1024);
            std::ptr::copy_nonoverlapping(in_l.add(offset), scratch_in_l.as_mut_ptr(), chunk_size);
            if !in_r.is_null() {
                std::ptr::copy_nonoverlapping(in_r.add(offset), scratch_in_r.as_mut_ptr(), chunk_size);
            } else {
                scratch_in_r[..chunk_size].copy_from_slice(&scratch_in_l[..chunk_size]);
            }

            let chunk_out_l = std::slice::from_raw_parts_mut(out_l.add(offset), chunk_size);

            if !is_mono_out {
                let chunk_out_r = std::slice::from_raw_parts_mut(out_r.add(offset), chunk_size);
                imager.process_block(
                    &scratch_in_l[..chunk_size],
                    &scratch_in_r[..chunk_size],
                    chunk_out_l,
                    chunk_out_r,
                );
            } else {
                imager.process_block(
                    &scratch_in_l[..chunk_size],
                    &scratch_in_r[..chunk_size],
                    chunk_out_l,
                    &mut scratch_out_r[..chunk_size],
                );
            }
            offset += chunk_size;
        }
        return;
    }

    let in_l_slice = std::slice::from_raw_parts(in_l, n);
    let in_r_slice = if !in_r.is_null() {
        std::slice::from_raw_parts(in_r, n)
    } else {
        in_l_slice
    };

    if !out_r.is_null() && !std::ptr::eq(out_l, out_r) {
        let out_l_slice = std::slice::from_raw_parts_mut(out_l, n);
        let out_r_slice = std::slice::from_raw_parts_mut(out_r, n);
        imager.process_block(in_l_slice, in_r_slice, out_l_slice, out_r_slice);
    } else {
        let mut mono_scratch = [0.0f32; 1024];
        let mut offset = 0;
        let out_l_slice = std::slice::from_raw_parts_mut(out_l, n);
        while offset < n {
            let chunk_size = (n - offset).min(1024);
            let chunk_in_l = &in_l_slice[offset..offset + chunk_size];
            let chunk_in_r = &in_r_slice[offset..offset + chunk_size];
            let chunk_out_l = &mut out_l_slice[offset..offset + chunk_size];
            imager.process_block(chunk_in_l, chunk_in_r, chunk_out_l, &mut mono_scratch[..chunk_size]);
            offset += chunk_size;
        }
    }
}

// ============================================================================
// Phaser C-ABI (Rules 5, 8, 9, 14, 34, 46, 47)
// ============================================================================

#[no_mangle]
pub unsafe extern "C" fn n8_rust_phaser_create(sample_rate: f32) -> *mut RustPhaser {
    Box::into_raw(Box::new(RustPhaser::new(sample_rate)))
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_phaser_destroy(node: *mut RustPhaser) {
    if !node.is_null() {
        drop(Box::from_raw(node));
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_phaser_prepare(node: *mut RustPhaser, sample_rate: f32) {
    if let Some(phaser) = node.as_mut() {
        phaser.prepare(sample_rate);
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_phaser_reset(node: *mut RustPhaser) {
    if let Some(phaser) = node.as_mut() {
        phaser.reset();
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_phaser_set_parameter(
    node: *mut RustPhaser,
    param_id: u32,
    value: f32,
) {
    if let Some(phaser) = node.as_mut() {
        phaser.set_parameter(param_id, value);
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_phaser_get_parameter(
    node: *const RustPhaser,
    param_id: u32,
) -> f32 {
    if let Some(phaser) = node.as_ref() {
        phaser.get_parameter(param_id)
    } else {
        0.0
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_phaser_process(
    node: *mut RustPhaser,
    in_l: *const f32,
    in_r: *const f32,
    out_l: *mut f32,
    out_r: *mut f32,
    num_samples: u32,
) {
    if node.is_null() || in_l.is_null() || out_l.is_null() || num_samples == 0 {
        return;
    }
    let phaser = &mut *node;
    let n = num_samples as usize;

    let is_in_place = std::ptr::eq(in_l, out_l as *const f32)
        || (!in_r.is_null() && !out_r.is_null() && std::ptr::eq(in_r, out_r as *const f32));

    if is_in_place {
        let mut scratch_in_l = [0.0f32; 1024];
        let mut scratch_in_r = [0.0f32; 1024];
        let mut scratch_out_r = [0.0f32; 1024];
        let is_mono_out = out_r.is_null() || std::ptr::eq(out_l, out_r);

        let mut offset = 0;
        while offset < n {
            let chunk_size = (n - offset).min(1024);
            std::ptr::copy_nonoverlapping(in_l.add(offset), scratch_in_l.as_mut_ptr(), chunk_size);
            if !in_r.is_null() {
                std::ptr::copy_nonoverlapping(in_r.add(offset), scratch_in_r.as_mut_ptr(), chunk_size);
            } else {
                scratch_in_r[..chunk_size].copy_from_slice(&scratch_in_l[..chunk_size]);
            }

            let chunk_out_l = std::slice::from_raw_parts_mut(out_l.add(offset), chunk_size);

            if !is_mono_out {
                let chunk_out_r = std::slice::from_raw_parts_mut(out_r.add(offset), chunk_size);
                phaser.process_block(
                    &scratch_in_l[..chunk_size],
                    &scratch_in_r[..chunk_size],
                    chunk_out_l,
                    chunk_out_r,
                );
            } else {
                phaser.process_block(
                    &scratch_in_l[..chunk_size],
                    &scratch_in_r[..chunk_size],
                    chunk_out_l,
                    &mut scratch_out_r[..chunk_size],
                );
            }
            offset += chunk_size;
        }
        return;
    }

    let in_l_slice = std::slice::from_raw_parts(in_l, n);
    let in_r_slice = if !in_r.is_null() {
        std::slice::from_raw_parts(in_r, n)
    } else {
        in_l_slice
    };

    if !out_r.is_null() && !std::ptr::eq(out_l, out_r) {
        let out_l_slice = std::slice::from_raw_parts_mut(out_l, n);
        let out_r_slice = std::slice::from_raw_parts_mut(out_r, n);
        phaser.process_block(in_l_slice, in_r_slice, out_l_slice, out_r_slice);
    } else {
        let mut mono_scratch = [0.0f32; 1024];
        let mut offset = 0;
        let out_l_slice = std::slice::from_raw_parts_mut(out_l, n);
        while offset < n {
            let chunk_size = (n - offset).min(1024);
            let chunk_in_l = &in_l_slice[offset..offset + chunk_size];
            let chunk_in_r = &in_r_slice[offset..offset + chunk_size];
            let chunk_out_l = &mut out_l_slice[offset..offset + chunk_size];
            phaser.process_block(chunk_in_l, chunk_in_r, chunk_out_l, &mut mono_scratch[..chunk_size]);
            offset += chunk_size;
        }
    }
}

// ============================================================================
// Chorus C-ABI (Rules 5, 8, 9, 14, 34, 46, 47)
// ============================================================================

#[no_mangle]
pub unsafe extern "C" fn n8_rust_chorus_create(sample_rate: f32) -> *mut RustChorus {
    Box::into_raw(Box::new(RustChorus::new(sample_rate)))
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_chorus_destroy(node: *mut RustChorus) {
    if !node.is_null() {
        drop(Box::from_raw(node));
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_chorus_prepare(node: *mut RustChorus, sample_rate: f32) {
    if let Some(chorus) = node.as_mut() {
        chorus.prepare(sample_rate);
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_chorus_reset(node: *mut RustChorus) {
    if let Some(chorus) = node.as_mut() {
        chorus.reset();
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_chorus_set_parameter(
    node: *mut RustChorus,
    param_id: u32,
    value: f32,
) {
    if let Some(chorus) = node.as_mut() {
        chorus.set_parameter(param_id, value);
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_chorus_get_parameter(
    node: *const RustChorus,
    param_id: u32,
) -> f32 {
    if let Some(chorus) = node.as_ref() {
        chorus.get_parameter(param_id)
    } else {
        0.0
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_chorus_process(
    node: *mut RustChorus,
    in_l: *const f32,
    in_r: *const f32,
    out_l: *mut f32,
    out_r: *mut f32,
    num_samples: u32,
) {
    if node.is_null() || in_l.is_null() || out_l.is_null() || num_samples == 0 {
        return;
    }
    let chorus = &mut *node;
    let n = num_samples as usize;

    let is_in_place = std::ptr::eq(in_l, out_l as *const f32)
        || (!in_r.is_null() && !out_r.is_null() && std::ptr::eq(in_r, out_r as *const f32));

    if is_in_place {
        let mut scratch_in_l = [0.0f32; 1024];
        let mut scratch_in_r = [0.0f32; 1024];
        let mut scratch_out_r = [0.0f32; 1024];
        let is_mono_out = out_r.is_null() || std::ptr::eq(out_l, out_r);

        let mut offset = 0;
        while offset < n {
            let chunk_size = (n - offset).min(1024);
            std::ptr::copy_nonoverlapping(in_l.add(offset), scratch_in_l.as_mut_ptr(), chunk_size);
            if !in_r.is_null() {
                std::ptr::copy_nonoverlapping(in_r.add(offset), scratch_in_r.as_mut_ptr(), chunk_size);
            } else {
                scratch_in_r[..chunk_size].copy_from_slice(&scratch_in_l[..chunk_size]);
            }

            let chunk_out_l = std::slice::from_raw_parts_mut(out_l.add(offset), chunk_size);

            if !is_mono_out {
                let chunk_out_r = std::slice::from_raw_parts_mut(out_r.add(offset), chunk_size);
                chorus.process_block(
                    &scratch_in_l[..chunk_size],
                    &scratch_in_r[..chunk_size],
                    chunk_out_l,
                    chunk_out_r,
                );
            } else {
                chorus.process_block(
                    &scratch_in_l[..chunk_size],
                    &scratch_in_r[..chunk_size],
                    chunk_out_l,
                    &mut scratch_out_r[..chunk_size],
                );
            }
            offset += chunk_size;
        }
        return;
    }

    let in_l_slice = std::slice::from_raw_parts(in_l, n);
    let in_r_slice = if !in_r.is_null() {
        std::slice::from_raw_parts(in_r, n)
    } else {
        in_l_slice
    };

    if !out_r.is_null() && !std::ptr::eq(out_l, out_r) {
        let out_l_slice = std::slice::from_raw_parts_mut(out_l, n);
        let out_r_slice = std::slice::from_raw_parts_mut(out_r, n);
        chorus.process_block(in_l_slice, in_r_slice, out_l_slice, out_r_slice);
    } else {
        let mut mono_scratch = [0.0f32; 1024];
        let mut offset = 0;
        let out_l_slice = std::slice::from_raw_parts_mut(out_l, n);
        while offset < n {
            let chunk_size = (n - offset).min(1024);
            let chunk_in_l = &in_l_slice[offset..offset + chunk_size];
            let chunk_in_r = &in_r_slice[offset..offset + chunk_size];
            let chunk_out_l = &mut out_l_slice[offset..offset + chunk_size];
            chorus.process_block(chunk_in_l, chunk_in_r, chunk_out_l, &mut mono_scratch[..chunk_size]);
            offset += chunk_size;
        }
    }
}

// ============================================================================
// Flanger C-ABI (Rules 5, 8, 9, 14, 34, 46, 47)
// ============================================================================

#[no_mangle]
pub unsafe extern "C" fn n8_rust_flanger_create(sample_rate: f32) -> *mut RustFlanger {
    Box::into_raw(Box::new(RustFlanger::new(sample_rate)))
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_flanger_destroy(node: *mut RustFlanger) {
    if !node.is_null() {
        drop(Box::from_raw(node));
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_flanger_prepare(node: *mut RustFlanger, sample_rate: f32) {
    if let Some(flanger) = node.as_mut() {
        flanger.prepare(sample_rate);
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_flanger_reset(node: *mut RustFlanger) {
    if let Some(flanger) = node.as_mut() {
        flanger.reset();
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_flanger_set_parameter(
    node: *mut RustFlanger,
    param_id: u32,
    value: f32,
) {
    if let Some(flanger) = node.as_mut() {
        flanger.set_parameter(param_id, value);
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_flanger_get_parameter(
    node: *const RustFlanger,
    param_id: u32,
) -> f32 {
    if let Some(flanger) = node.as_ref() {
        flanger.get_parameter(param_id)
    } else {
        0.0
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_flanger_process(
    node: *mut RustFlanger,
    in_l: *const f32,
    in_r: *const f32,
    out_l: *mut f32,
    out_r: *mut f32,
    num_samples: u32,
) {
    if node.is_null() || in_l.is_null() || out_l.is_null() || num_samples == 0 {
        return;
    }
    let flanger = &mut *node;
    let n = num_samples as usize;

    let is_in_place = std::ptr::eq(in_l, out_l as *const f32)
        || (!in_r.is_null() && !out_r.is_null() && std::ptr::eq(in_r, out_r as *const f32));

    if is_in_place {
        let mut scratch_in_l = [0.0f32; 1024];
        let mut scratch_in_r = [0.0f32; 1024];
        let mut scratch_out_r = [0.0f32; 1024];
        let is_mono_out = out_r.is_null() || std::ptr::eq(out_l, out_r);

        let mut offset = 0;
        while offset < n {
            let chunk_size = (n - offset).min(1024);
            std::ptr::copy_nonoverlapping(in_l.add(offset), scratch_in_l.as_mut_ptr(), chunk_size);
            if !in_r.is_null() {
                std::ptr::copy_nonoverlapping(in_r.add(offset), scratch_in_r.as_mut_ptr(), chunk_size);
            } else {
                scratch_in_r[..chunk_size].copy_from_slice(&scratch_in_l[..chunk_size]);
            }

            let chunk_out_l = std::slice::from_raw_parts_mut(out_l.add(offset), chunk_size);

            if !is_mono_out {
                let chunk_out_r = std::slice::from_raw_parts_mut(out_r.add(offset), chunk_size);
                flanger.process_block(
                    &scratch_in_l[..chunk_size],
                    &scratch_in_r[..chunk_size],
                    chunk_out_l,
                    chunk_out_r,
                );
            } else {
                flanger.process_block(
                    &scratch_in_l[..chunk_size],
                    &scratch_in_r[..chunk_size],
                    chunk_out_l,
                    &mut scratch_out_r[..chunk_size],
                );
            }
            offset += chunk_size;
        }
        return;
    }

    let in_l_slice = std::slice::from_raw_parts(in_l, n);
    let in_r_slice = if !in_r.is_null() {
        std::slice::from_raw_parts(in_r, n)
    } else {
        in_l_slice
    };

    if !out_r.is_null() && !std::ptr::eq(out_l, out_r) {
        let out_l_slice = std::slice::from_raw_parts_mut(out_l, n);
        let out_r_slice = std::slice::from_raw_parts_mut(out_r, n);
        flanger.process_block(in_l_slice, in_r_slice, out_l_slice, out_r_slice);
    } else {
        let mut mono_scratch = [0.0f32; 1024];
        let mut offset = 0;
        let out_l_slice = std::slice::from_raw_parts_mut(out_l, n);
        while offset < n {
            let chunk_size = (n - offset).min(1024);
            let chunk_in_l = &in_l_slice[offset..offset + chunk_size];
            let chunk_in_r = &in_r_slice[offset..offset + chunk_size];
            let chunk_out_l = &mut out_l_slice[offset..offset + chunk_size];
            flanger.process_block(chunk_in_l, chunk_in_r, chunk_out_l, &mut mono_scratch[..chunk_size]);
            offset += chunk_size;
        }
    }
}

// ============================================================================
// Ring Modulator C-ABI (Rules 5, 8, 9, 14, 34, 46, 47)
// ============================================================================

#[no_mangle]
pub unsafe extern "C" fn n8_rust_ring_modulator_create(sample_rate: f32) -> *mut RustRingModulator {
    Box::into_raw(Box::new(RustRingModulator::new(sample_rate)))
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_ring_modulator_destroy(node: *mut RustRingModulator) {
    if !node.is_null() {
        drop(Box::from_raw(node));
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_ring_modulator_prepare(node: *mut RustRingModulator, sample_rate: f32) {
    if let Some(ringmod) = node.as_mut() {
        ringmod.prepare(sample_rate);
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_ring_modulator_reset(node: *mut RustRingModulator) {
    if let Some(ringmod) = node.as_mut() {
        ringmod.reset();
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_ring_modulator_set_parameter(
    node: *mut RustRingModulator,
    param_id: u32,
    value: f32,
) {
    if let Some(ringmod) = node.as_mut() {
        ringmod.set_parameter(param_id, value);
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_ring_modulator_get_parameter(
    node: *const RustRingModulator,
    param_id: u32,
) -> f32 {
    if let Some(ringmod) = node.as_ref() {
        ringmod.get_parameter(param_id)
    } else {
        0.0
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_ring_modulator_process(
    node: *mut RustRingModulator,
    in_l: *const f32,
    in_r: *const f32,
    out_l: *mut f32,
    out_r: *mut f32,
    num_samples: u32,
) {
    if node.is_null() || in_l.is_null() || out_l.is_null() || num_samples == 0 {
        return;
    }
    let ringmod = &mut *node;
    let n = num_samples as usize;

    let is_in_place = std::ptr::eq(in_l, out_l as *const f32)
        || (!in_r.is_null() && !out_r.is_null() && std::ptr::eq(in_r, out_r as *const f32));

    if is_in_place {
        let mut scratch_in_l = [0.0f32; 1024];
        let mut scratch_in_r = [0.0f32; 1024];
        let mut scratch_out_r = [0.0f32; 1024];
        let is_mono_out = out_r.is_null() || std::ptr::eq(out_l, out_r);

        let mut offset = 0;
        while offset < n {
            let chunk_size = (n - offset).min(1024);
            std::ptr::copy_nonoverlapping(in_l.add(offset), scratch_in_l.as_mut_ptr(), chunk_size);
            if !in_r.is_null() {
                std::ptr::copy_nonoverlapping(in_r.add(offset), scratch_in_r.as_mut_ptr(), chunk_size);
            } else {
                scratch_in_r[..chunk_size].copy_from_slice(&scratch_in_l[..chunk_size]);
            }

            let chunk_out_l = std::slice::from_raw_parts_mut(out_l.add(offset), chunk_size);

            if !is_mono_out {
                let chunk_out_r = std::slice::from_raw_parts_mut(out_r.add(offset), chunk_size);
                ringmod.process_block(
                    &scratch_in_l[..chunk_size],
                    &scratch_in_r[..chunk_size],
                    chunk_out_l,
                    chunk_out_r,
                );
            } else {
                ringmod.process_block(
                    &scratch_in_l[..chunk_size],
                    &scratch_in_r[..chunk_size],
                    chunk_out_l,
                    &mut scratch_out_r[..chunk_size],
                );
            }
            offset += chunk_size;
        }
        return;
    }

    let in_l_slice = std::slice::from_raw_parts(in_l, n);
    let in_r_slice = if !in_r.is_null() {
        std::slice::from_raw_parts(in_r, n)
    } else {
        in_l_slice
    };

    if !out_r.is_null() && !std::ptr::eq(out_l, out_r) {
        let out_l_slice = std::slice::from_raw_parts_mut(out_l, n);
        let out_r_slice = std::slice::from_raw_parts_mut(out_r, n);
        ringmod.process_block(in_l_slice, in_r_slice, out_l_slice, out_r_slice);
    } else {
        let mut mono_scratch = [0.0f32; 1024];
        let mut offset = 0;
        let out_l_slice = std::slice::from_raw_parts_mut(out_l, n);
        while offset < n {
            let chunk_size = (n - offset).min(1024);
            let chunk_in_l = &in_l_slice[offset..offset + chunk_size];
            let chunk_in_r = &in_r_slice[offset..offset + chunk_size];
            let chunk_out_l = &mut out_l_slice[offset..offset + chunk_size];
            ringmod.process_block(chunk_in_l, chunk_in_r, chunk_out_l, &mut mono_scratch[..chunk_size]);
            offset += chunk_size;
        }
    }
}

// ============================================================================
// Frequency Shifter C-ABI (Rules 5, 8, 9, 14, 34, 46, 47)
// ============================================================================

#[no_mangle]
pub unsafe extern "C" fn n8_rust_frequency_shifter_create(sample_rate: f32) -> *mut RustFrequencyShifter {
    Box::into_raw(Box::new(RustFrequencyShifter::new(sample_rate)))
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_frequency_shifter_destroy(node: *mut RustFrequencyShifter) {
    if !node.is_null() {
        drop(Box::from_raw(node));
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_frequency_shifter_prepare(node: *mut RustFrequencyShifter, sample_rate: f32) {
    if let Some(shifter) = node.as_mut() {
        shifter.prepare(sample_rate);
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_frequency_shifter_reset(node: *mut RustFrequencyShifter) {
    if let Some(shifter) = node.as_mut() {
        shifter.reset();
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_frequency_shifter_set_parameter(
    node: *mut RustFrequencyShifter,
    param_id: u32,
    value: f32,
) {
    if let Some(shifter) = node.as_mut() {
        shifter.set_parameter(param_id, value);
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_frequency_shifter_get_parameter(
    node: *const RustFrequencyShifter,
    param_id: u32,
) -> f32 {
    if let Some(shifter) = node.as_ref() {
        shifter.get_parameter(param_id)
    } else {
        0.0
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_frequency_shifter_process(
    node: *mut RustFrequencyShifter,
    in_l: *const f32,
    in_r: *const f32,
    out_l: *mut f32,
    out_r: *mut f32,
    num_samples: u32,
) {
    if node.is_null() || in_l.is_null() || out_l.is_null() || num_samples == 0 {
        return;
    }
    let shifter = &mut *node;
    let n = num_samples as usize;

    let is_in_place = std::ptr::eq(in_l, out_l as *const f32)
        || (!in_r.is_null() && !out_r.is_null() && std::ptr::eq(in_r, out_r as *const f32));

    if is_in_place {
        let mut scratch_in_l = [0.0f32; 1024];
        let mut scratch_in_r = [0.0f32; 1024];
        let mut scratch_out_r = [0.0f32; 1024];
        let is_mono_out = out_r.is_null() || std::ptr::eq(out_l, out_r);

        let mut offset = 0;
        while offset < n {
            let chunk_size = (n - offset).min(1024);
            std::ptr::copy_nonoverlapping(in_l.add(offset), scratch_in_l.as_mut_ptr(), chunk_size);
            if !in_r.is_null() {
                std::ptr::copy_nonoverlapping(in_r.add(offset), scratch_in_r.as_mut_ptr(), chunk_size);
            } else {
                scratch_in_r[..chunk_size].copy_from_slice(&scratch_in_l[..chunk_size]);
            }

            let chunk_out_l = std::slice::from_raw_parts_mut(out_l.add(offset), chunk_size);

            if !is_mono_out {
                let chunk_out_r = std::slice::from_raw_parts_mut(out_r.add(offset), chunk_size);
                shifter.process_block(
                    &scratch_in_l[..chunk_size],
                    &scratch_in_r[..chunk_size],
                    chunk_out_l,
                    chunk_out_r,
                );
            } else {
                shifter.process_block(
                    &scratch_in_l[..chunk_size],
                    &scratch_in_r[..chunk_size],
                    chunk_out_l,
                    &mut scratch_out_r[..chunk_size],
                );
            }
            offset += chunk_size;
        }
        return;
    }

    let in_l_slice = std::slice::from_raw_parts(in_l, n);
    let in_r_slice = if !in_r.is_null() {
        std::slice::from_raw_parts(in_r, n)
    } else {
        in_l_slice
    };

    if !out_r.is_null() && !std::ptr::eq(out_l, out_r) {
        let out_l_slice = std::slice::from_raw_parts_mut(out_l, n);
        let out_r_slice = std::slice::from_raw_parts_mut(out_r, n);
        shifter.process_block(in_l_slice, in_r_slice, out_l_slice, out_r_slice);
    } else {
        let mut mono_scratch = [0.0f32; 1024];
        let mut offset = 0;
        let out_l_slice = std::slice::from_raw_parts_mut(out_l, n);
        while offset < n {
            let chunk_size = (n - offset).min(1024);
            let chunk_in_l = &in_l_slice[offset..offset + chunk_size];
            let chunk_in_r = &in_r_slice[offset..offset + chunk_size];
            let chunk_out_l = &mut out_l_slice[offset..offset + chunk_size];
            shifter.process_block(chunk_in_l, chunk_in_r, chunk_out_l, &mut mono_scratch[..chunk_size]);
            offset += chunk_size;
        }
    }
}

// ============================================================================
// Distortion C-ABI (Rules 5, 8, 9, 13, 34, 35, 46, 47)
// ============================================================================

#[no_mangle]
pub unsafe extern "C" fn n8_rust_distortion_create(sample_rate: f32) -> *mut RustDistortion {
    Box::into_raw(Box::new(RustDistortion::new(sample_rate)))
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_distortion_destroy(node: *mut RustDistortion) {
    if !node.is_null() {
        drop(Box::from_raw(node));
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_distortion_prepare(node: *mut RustDistortion, sample_rate: f32) {
    if let Some(dist) = node.as_mut() {
        dist.prepare(sample_rate);
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_distortion_reset(node: *mut RustDistortion) {
    if let Some(dist) = node.as_mut() {
        dist.reset();
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_distortion_set_parameter(
    node: *mut RustDistortion,
    param_id: u32,
    value: f32,
) {
    if let Some(dist) = node.as_mut() {
        dist.set_parameter(param_id, value);
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_distortion_get_parameter(
    node: *const RustDistortion,
    param_id: u32,
) -> f32 {
    if let Some(dist) = node.as_ref() {
        dist.get_parameter(param_id)
    } else {
        0.0
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_distortion_process(
    node: *mut RustDistortion,
    in_l: *const f32,
    in_r: *const f32,
    out_l: *mut f32,
    out_r: *mut f32,
    num_samples: u32,
) {
    if node.is_null() || in_l.is_null() || out_l.is_null() || num_samples == 0 {
        return;
    }
    let dist = &mut *node;
    let n = num_samples as usize;

    let is_in_place = std::ptr::eq(in_l, out_l as *const f32)
        || (!in_r.is_null() && !out_r.is_null() && std::ptr::eq(in_r, out_r as *const f32));

    if is_in_place {
        let mut scratch_in_l = [0.0f32; 1024];
        let mut scratch_in_r = [0.0f32; 1024];
        let mut scratch_out_r = [0.0f32; 1024];
        let is_mono_out = out_r.is_null() || std::ptr::eq(out_l, out_r);

        let mut offset = 0;
        while offset < n {
            let chunk_size = (n - offset).min(1024);
            std::ptr::copy_nonoverlapping(in_l.add(offset), scratch_in_l.as_mut_ptr(), chunk_size);
            if !in_r.is_null() {
                std::ptr::copy_nonoverlapping(in_r.add(offset), scratch_in_r.as_mut_ptr(), chunk_size);
            } else {
                scratch_in_r[..chunk_size].copy_from_slice(&scratch_in_l[..chunk_size]);
            }

            let chunk_out_l = std::slice::from_raw_parts_mut(out_l.add(offset), chunk_size);

            if !is_mono_out {
                let chunk_out_r = std::slice::from_raw_parts_mut(out_r.add(offset), chunk_size);
                dist.process_block(
                    &scratch_in_l[..chunk_size],
                    &scratch_in_r[..chunk_size],
                    chunk_out_l,
                    chunk_out_r,
                );
            } else {
                dist.process_block(
                    &scratch_in_l[..chunk_size],
                    &scratch_in_r[..chunk_size],
                    chunk_out_l,
                    &mut scratch_out_r[..chunk_size],
                );
            }
            offset += chunk_size;
        }
        return;
    }

    let in_l_slice = std::slice::from_raw_parts(in_l, n);
    let in_r_slice = if !in_r.is_null() {
        std::slice::from_raw_parts(in_r, n)
    } else {
        in_l_slice
    };

    if !out_r.is_null() && !std::ptr::eq(out_l, out_r) {
        let out_l_slice = std::slice::from_raw_parts_mut(out_l, n);
        let out_r_slice = std::slice::from_raw_parts_mut(out_r, n);
        dist.process_block(in_l_slice, in_r_slice, out_l_slice, out_r_slice);
    } else {
        let mut mono_scratch = [0.0f32; 1024];
        let mut offset = 0;
        let out_l_slice = std::slice::from_raw_parts_mut(out_l, n);
        while offset < n {
            let chunk_size = (n - offset).min(1024);
            let chunk_in_l = &in_l_slice[offset..offset + chunk_size];
            let chunk_in_r = &in_r_slice[offset..offset + chunk_size];
            let chunk_out_l = &mut out_l_slice[offset..offset + chunk_size];
            dist.process_block(chunk_in_l, chunk_in_r, chunk_out_l, &mut mono_scratch[..chunk_size]);
            offset += chunk_size;
        }
    }
}

// ============================================================================
// Formant Filter C-ABI (Rules 5, 8, 9, 14, 34, 46, 47)
// ============================================================================

#[no_mangle]
pub unsafe extern "C" fn n8_rust_formant_filter_create(sample_rate: f32) -> *mut RustFormantFilter {
    Box::into_raw(Box::new(RustFormantFilter::new(sample_rate)))
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_formant_filter_destroy(node: *mut RustFormantFilter) {
    if !node.is_null() {
        drop(Box::from_raw(node));
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_formant_filter_prepare(node: *mut RustFormantFilter, sample_rate: f32) {
    if let Some(formant) = node.as_mut() {
        formant.prepare(sample_rate);
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_formant_filter_reset(node: *mut RustFormantFilter) {
    if let Some(formant) = node.as_mut() {
        formant.reset();
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_formant_filter_set_parameter(
    node: *mut RustFormantFilter,
    param_id: u32,
    value: f32,
) {
    if let Some(formant) = node.as_mut() {
        formant.set_parameter(param_id, value);
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_formant_filter_get_parameter(
    node: *const RustFormantFilter,
    param_id: u32,
) -> f32 {
    if let Some(formant) = node.as_ref() {
        formant.get_parameter(param_id)
    } else {
        0.0
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_formant_filter_process(
    node: *mut RustFormantFilter,
    in_l: *const f32,
    in_r: *const f32,
    out_l: *mut f32,
    out_r: *mut f32,
    num_samples: u32,
) {
    if node.is_null() || in_l.is_null() || out_l.is_null() || num_samples == 0 {
        return;
    }
    let formant = &mut *node;
    let n = num_samples as usize;

    let is_in_place = std::ptr::eq(in_l, out_l as *const f32)
        || (!in_r.is_null() && !out_r.is_null() && std::ptr::eq(in_r, out_r as *const f32));

    if is_in_place {
        let mut scratch_in_l = [0.0f32; 1024];
        let mut scratch_in_r = [0.0f32; 1024];
        let mut scratch_out_r = [0.0f32; 1024];
        let is_mono_out = out_r.is_null() || std::ptr::eq(out_l, out_r);

        let mut offset = 0;
        while offset < n {
            let chunk_size = (n - offset).min(1024);
            std::ptr::copy_nonoverlapping(in_l.add(offset), scratch_in_l.as_mut_ptr(), chunk_size);
            if !in_r.is_null() {
                std::ptr::copy_nonoverlapping(in_r.add(offset), scratch_in_r.as_mut_ptr(), chunk_size);
            } else {
                scratch_in_r[..chunk_size].copy_from_slice(&scratch_in_l[..chunk_size]);
            }

            let chunk_out_l = std::slice::from_raw_parts_mut(out_l.add(offset), chunk_size);

            if !is_mono_out {
                let chunk_out_r = std::slice::from_raw_parts_mut(out_r.add(offset), chunk_size);
                formant.process_block(
                    &scratch_in_l[..chunk_size],
                    &scratch_in_r[..chunk_size],
                    chunk_out_l,
                    chunk_out_r,
                );
            } else {
                formant.process_block(
                    &scratch_in_l[..chunk_size],
                    &scratch_in_r[..chunk_size],
                    chunk_out_l,
                    &mut scratch_out_r[..chunk_size],
                );
            }
            offset += chunk_size;
        }
        return;
    }

    let in_l_slice = std::slice::from_raw_parts(in_l, n);
    let in_r_slice = if !in_r.is_null() {
        std::slice::from_raw_parts(in_r, n)
    } else {
        in_l_slice
    };

    if !out_r.is_null() && !std::ptr::eq(out_l, out_r) {
        let out_l_slice = std::slice::from_raw_parts_mut(out_l, n);
        let out_r_slice = std::slice::from_raw_parts_mut(out_r, n);
        formant.process_block(in_l_slice, in_r_slice, out_l_slice, out_r_slice);
    } else {
        let mut mono_scratch = [0.0f32; 1024];
        let mut offset = 0;
        let out_l_slice = std::slice::from_raw_parts_mut(out_l, n);
        while offset < n {
            let chunk_size = (n - offset).min(1024);
            let chunk_in_l = &in_l_slice[offset..offset + chunk_size];
            let chunk_in_r = &in_r_slice[offset..offset + chunk_size];
            let chunk_out_l = &mut out_l_slice[offset..offset + chunk_size];
            formant.process_block(chunk_in_l, chunk_in_r, chunk_out_l, &mut mono_scratch[..chunk_size]);
            offset += chunk_size;
        }
    }
}

// ============================================================================
// Noise Texture C-ABI (Rules 5, 8, 9, 14, 34, 46, 47)
// ============================================================================

#[no_mangle]
pub unsafe extern "C" fn n8_rust_noise_texture_create(sample_rate: f32) -> *mut RustNoiseTexture {
    Box::into_raw(Box::new(RustNoiseTexture::new(sample_rate)))
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_noise_texture_destroy(node: *mut RustNoiseTexture) {
    if !node.is_null() {
        drop(Box::from_raw(node));
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_noise_texture_prepare(node: *mut RustNoiseTexture, sample_rate: f32) {
    if let Some(noise) = node.as_mut() {
        noise.prepare(sample_rate);
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_noise_texture_reset(node: *mut RustNoiseTexture) {
    if let Some(noise) = node.as_mut() {
        noise.reset();
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_noise_texture_set_parameter(
    node: *mut RustNoiseTexture,
    param_id: u32,
    value: f32,
) {
    if let Some(noise) = node.as_mut() {
        noise.set_parameter(param_id, value);
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_noise_texture_get_parameter(
    node: *const RustNoiseTexture,
    param_id: u32,
) -> f32 {
    if let Some(noise) = node.as_ref() {
        noise.get_parameter(param_id)
    } else {
        0.0
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_noise_texture_process(
    node: *mut RustNoiseTexture,
    in_l: *const f32,
    in_r: *const f32,
    out_l: *mut f32,
    out_r: *mut f32,
    num_samples: u32,
) {
    if node.is_null() || out_l.is_null() || num_samples == 0 {
        return;
    }
    let noise = &mut *node;
    let n = num_samples as usize;

    let is_in_place = (!in_l.is_null() && std::ptr::eq(in_l, out_l as *const f32))
        || (!in_r.is_null() && !out_r.is_null() && std::ptr::eq(in_r, out_r as *const f32));

    if is_in_place {
        let mut scratch_in_l = [0.0f32; 1024];
        let mut scratch_in_r = [0.0f32; 1024];
        let mut scratch_out_r = [0.0f32; 1024];
        let is_mono_out = out_r.is_null() || std::ptr::eq(out_l, out_r);

        let mut offset = 0;
        while offset < n {
            let chunk_size = (n - offset).min(1024);
            if !in_l.is_null() {
                std::ptr::copy_nonoverlapping(in_l.add(offset), scratch_in_l.as_mut_ptr(), chunk_size);
            } else {
                scratch_in_l[..chunk_size].fill(0.0);
            }
            if !in_r.is_null() {
                std::ptr::copy_nonoverlapping(in_r.add(offset), scratch_in_r.as_mut_ptr(), chunk_size);
            } else {
                scratch_in_r[..chunk_size].copy_from_slice(&scratch_in_l[..chunk_size]);
            }

            let chunk_out_l = std::slice::from_raw_parts_mut(out_l.add(offset), chunk_size);

            if !is_mono_out {
                let chunk_out_r = std::slice::from_raw_parts_mut(out_r.add(offset), chunk_size);
                noise.process_block(
                    &scratch_in_l[..chunk_size],
                    &scratch_in_r[..chunk_size],
                    chunk_out_l,
                    chunk_out_r,
                );
            } else {
                noise.process_block(
                    &scratch_in_l[..chunk_size],
                    &scratch_in_r[..chunk_size],
                    chunk_out_l,
                    &mut scratch_out_r[..chunk_size],
                );
            }
            offset += chunk_size;
        }
        return;
    }

    let zero_buf = [0.0f32; 1024];
    let in_l_slice = if !in_l.is_null() {
        std::slice::from_raw_parts(in_l, n)
    } else {
        &zero_buf[..n.min(1024)]
    };
    let in_r_slice = if !in_r.is_null() {
        std::slice::from_raw_parts(in_r, n)
    } else {
        in_l_slice
    };

    if !out_r.is_null() && !std::ptr::eq(out_l, out_r) {
        let out_l_slice = std::slice::from_raw_parts_mut(out_l, n);
        let out_r_slice = std::slice::from_raw_parts_mut(out_r, n);
        noise.process_block(in_l_slice, in_r_slice, out_l_slice, out_r_slice);
    } else {
        let mut mono_scratch = [0.0f32; 1024];
        let mut offset = 0;
        let out_l_slice = std::slice::from_raw_parts_mut(out_l, n);
        while offset < n {
            let chunk_size = (n - offset).min(1024);
            let chunk_in_l = if !in_l.is_null() {
                &std::slice::from_raw_parts(in_l.add(offset), chunk_size)[..chunk_size]
            } else {
                &zero_buf[..chunk_size]
            };
            let chunk_in_r = if !in_r.is_null() {
                &std::slice::from_raw_parts(in_r.add(offset), chunk_size)[..chunk_size]
            } else {
                chunk_in_l
            };
            let chunk_out_l = &mut out_l_slice[offset..offset + chunk_size];
            noise.process_block(chunk_in_l, chunk_in_r, chunk_out_l, &mut mono_scratch[..chunk_size]);
            offset += chunk_size;
        }
    }
}

// ============================================================================
// Spatial Panner C-ABI (Rules 5, 8, 9, 13, 16, 32, 34, 46, 47)
// ============================================================================

#[no_mangle]
pub unsafe extern "C" fn n8_rust_spatial_panner_create(sample_rate: f32) -> *mut RustSpatialPanner {
    Box::into_raw(Box::new(RustSpatialPanner::new(sample_rate)))
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_spatial_panner_destroy(node: *mut RustSpatialPanner) {
    if !node.is_null() {
        drop(Box::from_raw(node));
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_spatial_panner_prepare(node: *mut RustSpatialPanner, sample_rate: f32) {
    if let Some(panner) = node.as_mut() {
        panner.prepare(sample_rate);
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_spatial_panner_reset(node: *mut RustSpatialPanner) {
    if let Some(panner) = node.as_mut() {
        panner.reset();
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_spatial_panner_set_parameter(
    node: *mut RustSpatialPanner,
    param_id: u32,
    value: f32,
) {
    if let Some(panner) = node.as_mut() {
        panner.set_parameter(param_id, value);
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_spatial_panner_get_parameter(
    node: *const RustSpatialPanner,
    param_id: u32,
) -> f32 {
    if let Some(panner) = node.as_ref() {
        panner.get_parameter(param_id)
    } else {
        0.0
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_spatial_panner_process(
    node: *mut RustSpatialPanner,
    in_l: *const f32,
    in_r: *const f32,
    out_l: *mut f32,
    out_r: *mut f32,
    num_samples: u32,
) {
    if node.is_null() || in_l.is_null() || out_l.is_null() || num_samples == 0 {
        return;
    }
    let panner = &mut *node;
    let n = num_samples as usize;

    let is_in_place = std::ptr::eq(in_l, out_l as *const f32)
        || (!in_r.is_null() && !out_r.is_null() && std::ptr::eq(in_r, out_r as *const f32));

    if is_in_place {
        let mut scratch_in_l = [0.0f32; 1024];
        let mut scratch_in_r = [0.0f32; 1024];
        let mut scratch_out_r = [0.0f32; 1024];
        let is_mono_out = out_r.is_null() || std::ptr::eq(out_l, out_r);

        let mut offset = 0;
        while offset < n {
            let chunk_size = (n - offset).min(1024);
            std::ptr::copy_nonoverlapping(in_l.add(offset), scratch_in_l.as_mut_ptr(), chunk_size);
            if !in_r.is_null() {
                std::ptr::copy_nonoverlapping(in_r.add(offset), scratch_in_r.as_mut_ptr(), chunk_size);
            } else {
                scratch_in_r[..chunk_size].copy_from_slice(&scratch_in_l[..chunk_size]);
            }

            let chunk_out_l = std::slice::from_raw_parts_mut(out_l.add(offset), chunk_size);

            if !is_mono_out {
                let chunk_out_r = std::slice::from_raw_parts_mut(out_r.add(offset), chunk_size);
                panner.process_block(
                    &scratch_in_l[..chunk_size],
                    &scratch_in_r[..chunk_size],
                    chunk_out_l,
                    chunk_out_r,
                );
            } else {
                panner.process_block(
                    &scratch_in_l[..chunk_size],
                    &scratch_in_r[..chunk_size],
                    chunk_out_l,
                    &mut scratch_out_r[..chunk_size],
                );
            }
            offset += chunk_size;
        }
        return;
    }

    let in_l_slice = std::slice::from_raw_parts(in_l, n);
    let in_r_slice = if !in_r.is_null() {
        std::slice::from_raw_parts(in_r, n)
    } else {
        in_l_slice
    };

    if !out_r.is_null() && !std::ptr::eq(out_l, out_r) {
        let out_l_slice = std::slice::from_raw_parts_mut(out_l, n);
        let out_r_slice = std::slice::from_raw_parts_mut(out_r, n);
        panner.process_block(in_l_slice, in_r_slice, out_l_slice, out_r_slice);
    } else {
        let mut mono_scratch = [0.0f32; 1024];
        let mut offset = 0;
        let out_l_slice = std::slice::from_raw_parts_mut(out_l, n);
        while offset < n {
            let chunk_size = (n - offset).min(1024);
            let chunk_in_l = &in_l_slice[offset..offset + chunk_size];
            let chunk_in_r = &in_r_slice[offset..offset + chunk_size];
            let chunk_out_l = &mut out_l_slice[offset..offset + chunk_size];
            panner.process_block(chunk_in_l, chunk_in_r, chunk_out_l, &mut mono_scratch[..chunk_size]);
            offset += chunk_size;
        }
    }
}

// ============================================================================
// Tape Stop C-ABI (Rules 5, 8, 9, 13, 16, 32, 34, 46, 47)
// ============================================================================

#[no_mangle]
pub unsafe extern "C" fn n8_rust_tape_stop_create(sample_rate: f32) -> *mut RustTapeStop {
    Box::into_raw(Box::new(RustTapeStop::new(sample_rate)))
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_tape_stop_destroy(node: *mut RustTapeStop) {
    if !node.is_null() {
        drop(Box::from_raw(node));
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_tape_stop_prepare(node: *mut RustTapeStop, sample_rate: f32) {
    if let Some(stop) = node.as_mut() {
        stop.prepare(sample_rate);
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_tape_stop_reset(node: *mut RustTapeStop) {
    if let Some(stop) = node.as_mut() {
        stop.reset();
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_tape_stop_set_parameter(
    node: *mut RustTapeStop,
    param_id: u32,
    value: f32,
) {
    if let Some(stop) = node.as_mut() {
        stop.set_parameter(param_id, value);
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_tape_stop_get_parameter(
    node: *const RustTapeStop,
    param_id: u32,
) -> f32 {
    if let Some(stop) = node.as_ref() {
        stop.get_parameter(param_id)
    } else {
        0.0
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_tape_stop_process(
    node: *mut RustTapeStop,
    in_l: *const f32,
    in_r: *const f32,
    out_l: *mut f32,
    out_r: *mut f32,
    num_samples: u32,
) {
    if node.is_null() || in_l.is_null() || out_l.is_null() || num_samples == 0 {
        return;
    }
    let stop = &mut *node;
    let n = num_samples as usize;

    let is_in_place = std::ptr::eq(in_l, out_l as *const f32)
        || (!in_r.is_null() && !out_r.is_null() && std::ptr::eq(in_r, out_r as *const f32));

    if is_in_place {
        let mut scratch_in_l = [0.0f32; 1024];
        let mut scratch_in_r = [0.0f32; 1024];
        let mut scratch_out_r = [0.0f32; 1024];
        let is_mono_out = out_r.is_null() || std::ptr::eq(out_l, out_r);

        let mut offset = 0;
        while offset < n {
            let chunk_size = (n - offset).min(1024);
            std::ptr::copy_nonoverlapping(in_l.add(offset), scratch_in_l.as_mut_ptr(), chunk_size);
            if !in_r.is_null() {
                std::ptr::copy_nonoverlapping(in_r.add(offset), scratch_in_r.as_mut_ptr(), chunk_size);
            } else {
                scratch_in_r[..chunk_size].copy_from_slice(&scratch_in_l[..chunk_size]);
            }

            let chunk_out_l = std::slice::from_raw_parts_mut(out_l.add(offset), chunk_size);

            if !is_mono_out {
                let chunk_out_r = std::slice::from_raw_parts_mut(out_r.add(offset), chunk_size);
                stop.process_block(
                    &scratch_in_l[..chunk_size],
                    &scratch_in_r[..chunk_size],
                    chunk_out_l,
                    chunk_out_r,
                );
            } else {
                stop.process_block(
                    &scratch_in_l[..chunk_size],
                    &scratch_in_r[..chunk_size],
                    chunk_out_l,
                    &mut scratch_out_r[..chunk_size],
                );
            }
            offset += chunk_size;
        }
        return;
    }

    let in_l_slice = std::slice::from_raw_parts(in_l, n);
    let in_r_slice = if !in_r.is_null() {
        std::slice::from_raw_parts(in_r, n)
    } else {
        in_l_slice
    };

    if !out_r.is_null() && !std::ptr::eq(out_l, out_r) {
        let out_l_slice = std::slice::from_raw_parts_mut(out_l, n);
        let out_r_slice = std::slice::from_raw_parts_mut(out_r, n);
        stop.process_block(in_l_slice, in_r_slice, out_l_slice, out_r_slice);
    } else {
        let mut mono_scratch = [0.0f32; 1024];
        let mut offset = 0;
        let out_l_slice = std::slice::from_raw_parts_mut(out_l, n);
        while offset < n {
            let chunk_size = (n - offset).min(1024);
            let chunk_in_l = &in_l_slice[offset..offset + chunk_size];
            let chunk_in_r = &in_r_slice[offset..offset + chunk_size];
            let chunk_out_l = &mut out_l_slice[offset..offset + chunk_size];
            stop.process_block(chunk_in_l, chunk_in_r, chunk_out_l, &mut mono_scratch[..chunk_size]);
            offset += chunk_size;
        }
    }
}

// ============================================================================
// Lo-Fi Cassette C-ABI (Rules 5, 8, 9, 13, 16, 32, 34, 46, 47)
// ============================================================================

#[no_mangle]
pub unsafe extern "C" fn n8_rust_lofi_cassette_create(sample_rate: f32) -> *mut RustLoFiCassette {
    Box::into_raw(Box::new(RustLoFiCassette::new(sample_rate)))
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_lofi_cassette_destroy(node: *mut RustLoFiCassette) {
    if !node.is_null() {
        drop(Box::from_raw(node));
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_lofi_cassette_prepare(node: *mut RustLoFiCassette, sample_rate: f32) {
    if let Some(cassette) = node.as_mut() {
        cassette.prepare(sample_rate);
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_lofi_cassette_reset(node: *mut RustLoFiCassette) {
    if let Some(cassette) = node.as_mut() {
        cassette.reset();
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_lofi_cassette_set_parameter(
    node: *mut RustLoFiCassette,
    param_id: u32,
    value: f32,
) {
    if let Some(cassette) = node.as_mut() {
        cassette.set_parameter(param_id, value);
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_lofi_cassette_get_parameter(
    node: *const RustLoFiCassette,
    param_id: u32,
) -> f32 {
    if let Some(cassette) = node.as_ref() {
        cassette.get_parameter(param_id)
    } else {
        0.0
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_lofi_cassette_process(
    node: *mut RustLoFiCassette,
    in_l: *const f32,
    in_r: *const f32,
    out_l: *mut f32,
    out_r: *mut f32,
    num_samples: u32,
) {
    if node.is_null() || in_l.is_null() || out_l.is_null() || num_samples == 0 {
        return;
    }
    let cassette = &mut *node;
    let n = num_samples as usize;

    let is_in_place = std::ptr::eq(in_l, out_l as *const f32)
        || (!in_r.is_null() && !out_r.is_null() && std::ptr::eq(in_r, out_r as *const f32));

    if is_in_place {
        let mut scratch_in_l = [0.0f32; 1024];
        let mut scratch_in_r = [0.0f32; 1024];
        let mut scratch_out_r = [0.0f32; 1024];
        let is_mono_out = out_r.is_null() || std::ptr::eq(out_l, out_r);

        let mut offset = 0;
        while offset < n {
            let chunk_size = (n - offset).min(1024);
            std::ptr::copy_nonoverlapping(in_l.add(offset), scratch_in_l.as_mut_ptr(), chunk_size);
            if !in_r.is_null() {
                std::ptr::copy_nonoverlapping(in_r.add(offset), scratch_in_r.as_mut_ptr(), chunk_size);
            } else {
                scratch_in_r[..chunk_size].copy_from_slice(&scratch_in_l[..chunk_size]);
            }

            let chunk_out_l = std::slice::from_raw_parts_mut(out_l.add(offset), chunk_size);

            if !is_mono_out {
                let chunk_out_r = std::slice::from_raw_parts_mut(out_r.add(offset), chunk_size);
                cassette.process_block(
                    &scratch_in_l[..chunk_size],
                    &scratch_in_r[..chunk_size],
                    chunk_out_l,
                    chunk_out_r,
                );
            } else {
                cassette.process_block(
                    &scratch_in_l[..chunk_size],
                    &scratch_in_r[..chunk_size],
                    chunk_out_l,
                    &mut scratch_out_r[..chunk_size],
                );
            }
            offset += chunk_size;
        }
        return;
    }

    let in_l_slice = std::slice::from_raw_parts(in_l, n);
    let in_r_slice = if !in_r.is_null() {
        std::slice::from_raw_parts(in_r, n)
    } else {
        in_l_slice
    };

    if !out_r.is_null() && !std::ptr::eq(out_l, out_r) {
        let out_l_slice = std::slice::from_raw_parts_mut(out_l, n);
        let out_r_slice = std::slice::from_raw_parts_mut(out_r, n);
        cassette.process_block(in_l_slice, in_r_slice, out_l_slice, out_r_slice);
    } else {
        let mut mono_scratch = [0.0f32; 1024];
        let mut offset = 0;
        let out_l_slice = std::slice::from_raw_parts_mut(out_l, n);
        while offset < n {
            let chunk_size = (n - offset).min(1024);
            let chunk_in_l = &in_l_slice[offset..offset + chunk_size];
            let chunk_in_r = &in_r_slice[offset..offset + chunk_size];
            let chunk_out_l = &mut out_l_slice[offset..offset + chunk_size];
            cassette.process_block(chunk_in_l, chunk_in_r, chunk_out_l, &mut mono_scratch[..chunk_size]);
            offset += chunk_size;
        }
    }
}

// ============================================================================
// Advanced Delay C-ABI (Rules 5, 8, 9, 12, 13, 14, 16, 18, 32, 34, 46, 47)
// ============================================================================

#[no_mangle]
pub unsafe extern "C" fn n8_rust_advanced_delay_create(sample_rate: f32) -> *mut RustAdvancedDelay {
    Box::into_raw(Box::new(RustAdvancedDelay::new(sample_rate)))
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_advanced_delay_destroy(node: *mut RustAdvancedDelay) {
    if !node.is_null() {
        drop(Box::from_raw(node));
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_advanced_delay_prepare(node: *mut RustAdvancedDelay, sample_rate: f32) {
    if let Some(delay) = node.as_mut() {
        delay.prepare(sample_rate);
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_advanced_delay_reset(node: *mut RustAdvancedDelay) {
    if let Some(delay) = node.as_mut() {
        delay.reset();
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_advanced_delay_set_parameter(
    node: *mut RustAdvancedDelay,
    param_id: u32,
    value: f32,
) {
    if let Some(delay) = node.as_mut() {
        delay.set_parameter(param_id, value);
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_advanced_delay_get_parameter(
    node: *const RustAdvancedDelay,
    param_id: u32,
) -> f32 {
    if let Some(delay) = node.as_ref() {
        delay.get_parameter(param_id)
    } else {
        0.0
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_advanced_delay_process(
    node: *mut RustAdvancedDelay,
    in_l: *const f32,
    in_r: *const f32,
    out_l: *mut f32,
    out_r: *mut f32,
    num_samples: u32,
) {
    if node.is_null() || in_l.is_null() || out_l.is_null() || num_samples == 0 {
        return;
    }
    let delay = &mut *node;
    let n = num_samples as usize;

    let is_in_place = std::ptr::eq(in_l, out_l as *const f32)
        || (!in_r.is_null() && !out_r.is_null() && std::ptr::eq(in_r, out_r as *const f32));

    if is_in_place {
        let mut scratch_in_l = [0.0f32; 1024];
        let mut scratch_in_r = [0.0f32; 1024];
        let mut scratch_out_r = [0.0f32; 1024];
        let is_mono_out = out_r.is_null() || std::ptr::eq(out_l, out_r);

        let mut offset = 0;
        while offset < n {
            let chunk_size = (n - offset).min(1024);
            std::ptr::copy_nonoverlapping(in_l.add(offset), scratch_in_l.as_mut_ptr(), chunk_size);
            if !in_r.is_null() {
                std::ptr::copy_nonoverlapping(in_r.add(offset), scratch_in_r.as_mut_ptr(), chunk_size);
            } else {
                scratch_in_r[..chunk_size].copy_from_slice(&scratch_in_l[..chunk_size]);
            }

            let chunk_out_l = std::slice::from_raw_parts_mut(out_l.add(offset), chunk_size);

            if !is_mono_out {
                let chunk_out_r = std::slice::from_raw_parts_mut(out_r.add(offset), chunk_size);
                delay.process_block(
                    &scratch_in_l[..chunk_size],
                    &scratch_in_r[..chunk_size],
                    chunk_out_l,
                    chunk_out_r,
                );
            } else {
                delay.process_block(
                    &scratch_in_l[..chunk_size],
                    &scratch_in_r[..chunk_size],
                    chunk_out_l,
                    &mut scratch_out_r[..chunk_size],
                );
            }
            offset += chunk_size;
        }
        return;
    }

    let in_l_slice = std::slice::from_raw_parts(in_l, n);
    let in_r_slice = if !in_r.is_null() {
        std::slice::from_raw_parts(in_r, n)
    } else {
        in_l_slice
    };

    if !out_r.is_null() && !std::ptr::eq(out_l, out_r) {
        let out_l_slice = std::slice::from_raw_parts_mut(out_l, n);
        let out_r_slice = std::slice::from_raw_parts_mut(out_r, n);
        delay.process_block(in_l_slice, in_r_slice, out_l_slice, out_r_slice);
    } else {
        let mut mono_scratch = [0.0f32; 1024];
        let mut offset = 0;
        let out_l_slice = std::slice::from_raw_parts_mut(out_l, n);
        while offset < n {
            let chunk_size = (n - offset).min(1024);
            let chunk_in_l = &in_l_slice[offset..offset + chunk_size];
            let chunk_in_r = &in_r_slice[offset..offset + chunk_size];
            let chunk_out_l = &mut out_l_slice[offset..offset + chunk_size];
            delay.process_block(chunk_in_l, chunk_in_r, chunk_out_l, &mut mono_scratch[..chunk_size]);
            offset += chunk_size;
        }
    }
}

// ============================================================================
// Algorithmic FDN Reverb C-ABI (Rules 5, 8, 9, 13, 14, 16, 18, 32, 34, 46, 47)
// ============================================================================

#[no_mangle]
pub unsafe extern "C" fn n8_rust_reverb_create(sample_rate: f32) -> *mut RustReverb {
    Box::into_raw(Box::new(RustReverb::new(sample_rate)))
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_reverb_destroy(node: *mut RustReverb) {
    if !node.is_null() {
        drop(Box::from_raw(node));
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_reverb_prepare(node: *mut RustReverb, sample_rate: f32) {
    if let Some(reverb) = node.as_mut() {
        reverb.prepare(sample_rate);
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_reverb_reset(node: *mut RustReverb) {
    if let Some(reverb) = node.as_mut() {
        reverb.reset();
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_reverb_set_parameter(
    node: *mut RustReverb,
    param_id: u32,
    value: f32,
) {
    if let Some(reverb) = node.as_mut() {
        reverb.set_parameter(param_id, value);
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_reverb_get_parameter(
    node: *const RustReverb,
    param_id: u32,
) -> f32 {
    if let Some(reverb) = node.as_ref() {
        reverb.get_parameter(param_id)
    } else {
        0.0
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_reverb_process(
    node: *mut RustReverb,
    in_l: *const f32,
    in_r: *const f32,
    out_l: *mut f32,
    out_r: *mut f32,
    num_samples: u32,
) {
    if node.is_null() || in_l.is_null() || out_l.is_null() || num_samples == 0 {
        return;
    }
    let reverb = &mut *node;
    let n = num_samples as usize;

    let is_in_place = std::ptr::eq(in_l, out_l as *const f32)
        || (!in_r.is_null() && !out_r.is_null() && std::ptr::eq(in_r, out_r as *const f32));

    if is_in_place {
        let mut scratch_in_l = [0.0f32; 1024];
        let mut scratch_in_r = [0.0f32; 1024];
        let mut scratch_out_r = [0.0f32; 1024];
        let is_mono_out = out_r.is_null() || std::ptr::eq(out_l, out_r);

        let mut offset = 0;
        while offset < n {
            let chunk_size = (n - offset).min(1024);
            std::ptr::copy_nonoverlapping(in_l.add(offset), scratch_in_l.as_mut_ptr(), chunk_size);
            if !in_r.is_null() {
                std::ptr::copy_nonoverlapping(in_r.add(offset), scratch_in_r.as_mut_ptr(), chunk_size);
            } else {
                scratch_in_r[..chunk_size].copy_from_slice(&scratch_in_l[..chunk_size]);
            }

            let chunk_out_l = std::slice::from_raw_parts_mut(out_l.add(offset), chunk_size);

            if !is_mono_out {
                let chunk_out_r = std::slice::from_raw_parts_mut(out_r.add(offset), chunk_size);
                reverb.process_block(
                    &scratch_in_l[..chunk_size],
                    &scratch_in_r[..chunk_size],
                    chunk_out_l,
                    chunk_out_r,
                );
            } else {
                reverb.process_block(
                    &scratch_in_l[..chunk_size],
                    &scratch_in_r[..chunk_size],
                    chunk_out_l,
                    &mut scratch_out_r[..chunk_size],
                );
            }
            offset += chunk_size;
        }
        return;
    }

    let in_l_slice = std::slice::from_raw_parts(in_l, n);
    let in_r_slice = if !in_r.is_null() {
        std::slice::from_raw_parts(in_r, n)
    } else {
        in_l_slice
    };

    if !out_r.is_null() && !std::ptr::eq(out_l, out_r) {
        let out_l_slice = std::slice::from_raw_parts_mut(out_l, n);
        let out_r_slice = std::slice::from_raw_parts_mut(out_r, n);
        reverb.process_block(in_l_slice, in_r_slice, out_l_slice, out_r_slice);
    } else {
        let mut mono_scratch = [0.0f32; 1024];
        let mut offset = 0;
        let out_l_slice = std::slice::from_raw_parts_mut(out_l, n);
        while offset < n {
            let chunk_size = (n - offset).min(1024);
            let chunk_in_l = &in_l_slice[offset..offset + chunk_size];
            let chunk_in_r = &in_r_slice[offset..offset + chunk_size];
            let chunk_out_l = &mut out_l_slice[offset..offset + chunk_size];
            reverb.process_block(chunk_in_l, chunk_in_r, chunk_out_l, &mut mono_scratch[..chunk_size]);
            offset += chunk_size;
        }
    }
}

// ============================================================================
// Reverse Reverb (DSP #39) C-ABI Wrappers
// ============================================================================

#[no_mangle]
pub unsafe extern "C" fn n8_rust_reverse_reverb_create(sample_rate: f32) -> *mut RustReverseReverb {
    Box::into_raw(Box::new(RustReverseReverb::new(sample_rate)))
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_reverse_reverb_destroy(node: *mut RustReverseReverb) {
    if !node.is_null() {
        drop(Box::from_raw(node));
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_reverse_reverb_reset(node: *mut RustReverseReverb) {
    if let Some(rev) = node.as_mut() {
        rev.reset();
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_reverse_reverb_prepare(node: *mut RustReverseReverb, sample_rate: f32) {
    if let Some(rev) = node.as_mut() {
        rev.prepare(sample_rate);
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_reverse_reverb_set_parameter(
    node: *mut RustReverseReverb,
    param_id: u32,
    value: f32,
) {
    if let Some(rev) = node.as_mut() {
        rev.set_parameter(param_id, value);
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_reverse_reverb_get_parameter(
    node: *const RustReverseReverb,
    param_id: u32,
) -> f32 {
    if let Some(rev) = node.as_ref() {
        rev.get_parameter(param_id)
    } else {
        0.0
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_reverse_reverb_process(
    node: *mut RustReverseReverb,
    in_l: *const f32,
    in_r: *const f32,
    out_l: *mut f32,
    out_r: *mut f32,
    num_samples: u32,
) {
    if node.is_null() || in_l.is_null() || out_l.is_null() || num_samples == 0 {
        return;
    }
    let rev = &mut *node;
    let n = num_samples as usize;

    let is_in_place = std::ptr::eq(in_l, out_l as *const f32)
        || (!in_r.is_null() && !out_r.is_null() && std::ptr::eq(in_r, out_r as *const f32));

    if is_in_place {
        let mut scratch_in_l = [0.0f32; 1024];
        let mut scratch_in_r = [0.0f32; 1024];
        let mut scratch_out_r = [0.0f32; 1024];
        let is_mono_out = out_r.is_null() || std::ptr::eq(out_l, out_r);

        let mut offset = 0;
        while offset < n {
            let chunk_size = (n - offset).min(1024);
            std::ptr::copy_nonoverlapping(in_l.add(offset), scratch_in_l.as_mut_ptr(), chunk_size);
            if !in_r.is_null() {
                std::ptr::copy_nonoverlapping(in_r.add(offset), scratch_in_r.as_mut_ptr(), chunk_size);
            } else {
                scratch_in_r[..chunk_size].copy_from_slice(&scratch_in_l[..chunk_size]);
            }

            let chunk_out_l = std::slice::from_raw_parts_mut(out_l.add(offset), chunk_size);

            if !is_mono_out {
                let chunk_out_r = std::slice::from_raw_parts_mut(out_r.add(offset), chunk_size);
                rev.process_block(
                    &scratch_in_l[..chunk_size],
                    &scratch_in_r[..chunk_size],
                    chunk_out_l,
                    chunk_out_r,
                );
            } else {
                rev.process_block(
                    &scratch_in_l[..chunk_size],
                    &scratch_in_r[..chunk_size],
                    chunk_out_l,
                    &mut scratch_out_r[..chunk_size],
                );
            }
            offset += chunk_size;
        }
        return;
    }

    let in_l_slice = std::slice::from_raw_parts(in_l, n);
    let in_r_slice = if !in_r.is_null() {
        std::slice::from_raw_parts(in_r, n)
    } else {
        in_l_slice
    };

    if !out_r.is_null() && !std::ptr::eq(out_l, out_r) {
        let out_l_slice = std::slice::from_raw_parts_mut(out_l, n);
        let out_r_slice = std::slice::from_raw_parts_mut(out_r, n);
        rev.process_block(in_l_slice, in_r_slice, out_l_slice, out_r_slice);
    } else {
        let mut mono_scratch = [0.0f32; 1024];
        let mut offset = 0;
        let out_l_slice = std::slice::from_raw_parts_mut(out_l, n);
        while offset < n {
            let chunk_size = (n - offset).min(1024);
            let chunk_in_l = &in_l_slice[offset..offset + chunk_size];
            let chunk_in_r = &in_r_slice[offset..offset + chunk_size];
            let chunk_out_l = &mut out_l_slice[offset..offset + chunk_size];
            rev.process_block(chunk_in_l, chunk_in_r, chunk_out_l, &mut mono_scratch[..chunk_size]);
            offset += chunk_size;
        }
    }
}

// ============================================================================
// Glitch (DSP #11) C-ABI Wrappers
// ============================================================================

#[no_mangle]
pub unsafe extern "C" fn n8_rust_glitch_create(sample_rate: f32) -> *mut RustGlitch {
    Box::into_raw(Box::new(RustGlitch::new(sample_rate)))
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_glitch_destroy(node: *mut RustGlitch) {
    if !node.is_null() {
        drop(Box::from_raw(node));
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_glitch_reset(node: *mut RustGlitch) {
    if let Some(g) = node.as_mut() {
        g.reset();
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_glitch_prepare(node: *mut RustGlitch, sample_rate: f32) {
    if let Some(g) = node.as_mut() {
        g.prepare(sample_rate);
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_glitch_set_parameter(
    node: *mut RustGlitch,
    param_id: u32,
    value: f32,
) {
    if let Some(g) = node.as_mut() {
        g.set_parameter(param_id, value);
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_glitch_get_parameter(
    node: *const RustGlitch,
    param_id: u32,
) -> f32 {
    if let Some(g) = node.as_ref() {
        g.get_parameter(param_id)
    } else {
        0.0
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_glitch_process(
    node: *mut RustGlitch,
    in_l: *const f32,
    in_r: *const f32,
    out_l: *mut f32,
    out_r: *mut f32,
    num_samples: u32,
) {
    if node.is_null() || in_l.is_null() || out_l.is_null() || num_samples == 0 {
        return;
    }
    let g = &mut *node;
    let n = num_samples as usize;

    let is_in_place = std::ptr::eq(in_l, out_l as *const f32)
        || (!in_r.is_null() && !out_r.is_null() && std::ptr::eq(in_r, out_r as *const f32));

    if is_in_place {
        let mut scratch_in_l = [0.0f32; 1024];
        let mut scratch_in_r = [0.0f32; 1024];
        let mut scratch_out_r = [0.0f32; 1024];
        let is_mono_out = out_r.is_null() || std::ptr::eq(out_l, out_r);

        let mut offset = 0;
        while offset < n {
            let chunk_size = (n - offset).min(1024);
            std::ptr::copy_nonoverlapping(in_l.add(offset), scratch_in_l.as_mut_ptr(), chunk_size);
            if !in_r.is_null() {
                std::ptr::copy_nonoverlapping(in_r.add(offset), scratch_in_r.as_mut_ptr(), chunk_size);
            } else {
                scratch_in_r[..chunk_size].copy_from_slice(&scratch_in_l[..chunk_size]);
            }

            let chunk_out_l = std::slice::from_raw_parts_mut(out_l.add(offset), chunk_size);

            if !is_mono_out {
                let chunk_out_r = std::slice::from_raw_parts_mut(out_r.add(offset), chunk_size);
                g.process_block(
                    &scratch_in_l[..chunk_size],
                    &scratch_in_r[..chunk_size],
                    chunk_out_l,
                    chunk_out_r,
                );
            } else {
                g.process_block(
                    &scratch_in_l[..chunk_size],
                    &scratch_in_r[..chunk_size],
                    chunk_out_l,
                    &mut scratch_out_r[..chunk_size],
                );
            }
            offset += chunk_size;
        }
        return;
    }

    let in_l_slice = std::slice::from_raw_parts(in_l, n);
    let in_r_slice = if !in_r.is_null() {
        std::slice::from_raw_parts(in_r, n)
    } else {
        in_l_slice
    };

    if !out_r.is_null() && !std::ptr::eq(out_l, out_r) {
        let out_l_slice = std::slice::from_raw_parts_mut(out_l, n);
        let out_r_slice = std::slice::from_raw_parts_mut(out_r, n);
        g.process_block(in_l_slice, in_r_slice, out_l_slice, out_r_slice);
    } else {
        let mut mono_scratch = [0.0f32; 1024];
        let mut offset = 0;
        let out_l_slice = std::slice::from_raw_parts_mut(out_l, n);
        while offset < n {
            let chunk_size = (n - offset).min(1024);
            let chunk_in_l = &in_l_slice[offset..offset + chunk_size];
            let chunk_in_r = &in_r_slice[offset..offset + chunk_size];
            let chunk_out_l = &mut out_l_slice[offset..offset + chunk_size];
            g.process_block(chunk_in_l, chunk_in_r, chunk_out_l, &mut mono_scratch[..chunk_size]);
            offset += chunk_size;
        }
    }
}

// ============================================================================
// Mid/Side Encoder (DSP #24) C-ABI Wrappers
// ============================================================================

#[no_mangle]
pub unsafe extern "C" fn n8_rust_mid_side_encoder_create() -> *mut RustMidSideEncoder {
    Box::into_raw(Box::new(RustMidSideEncoder::new()))
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_mid_side_encoder_destroy(node: *mut RustMidSideEncoder) {
    if !node.is_null() {
        drop(Box::from_raw(node));
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_mid_side_encoder_reset(node: *mut RustMidSideEncoder) {
    if let Some(enc) = node.as_mut() {
        enc.reset();
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_mid_side_encoder_prepare(node: *mut RustMidSideEncoder, sample_rate: f32) {
    if let Some(enc) = node.as_mut() {
        enc.prepare(sample_rate);
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_mid_side_encoder_set_parameter(
    node: *mut RustMidSideEncoder,
    param_id: u32,
    value: f32,
) {
    if let Some(enc) = node.as_mut() {
        enc.set_parameter(param_id, value);
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_mid_side_encoder_get_parameter(
    node: *const RustMidSideEncoder,
    param_id: u32,
) -> f32 {
    if let Some(enc) = node.as_ref() {
        enc.get_parameter(param_id)
    } else {
        0.0
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_mid_side_encoder_process(
    node: *mut RustMidSideEncoder,
    in_l: *const f32,
    in_r: *const f32,
    out_mid: *mut f32,
    out_side: *mut f32,
    num_samples: u32,
) {
    if node.is_null() || in_l.is_null() || out_mid.is_null() || num_samples == 0 {
        return;
    }
    let enc = &mut *node;
    let n = num_samples as usize;

    let is_in_place = std::ptr::eq(in_l, out_mid as *const f32)
        || (!in_r.is_null() && !out_side.is_null() && std::ptr::eq(in_r, out_side as *const f32));

    if is_in_place {
        let mut scratch_in_l = [0.0f32; 1024];
        let mut scratch_in_r = [0.0f32; 1024];
        let mut scratch_out_side = [0.0f32; 1024];
        let is_mono_out = out_side.is_null() || std::ptr::eq(out_mid, out_side);

        let mut offset = 0;
        while offset < n {
            let chunk_size = (n - offset).min(1024);
            std::ptr::copy_nonoverlapping(in_l.add(offset), scratch_in_l.as_mut_ptr(), chunk_size);
            if !in_r.is_null() {
                std::ptr::copy_nonoverlapping(in_r.add(offset), scratch_in_r.as_mut_ptr(), chunk_size);
            } else {
                scratch_in_r[..chunk_size].copy_from_slice(&scratch_in_l[..chunk_size]);
            }

            let chunk_out_mid = std::slice::from_raw_parts_mut(out_mid.add(offset), chunk_size);

            if !is_mono_out {
                let chunk_out_side = std::slice::from_raw_parts_mut(out_side.add(offset), chunk_size);
                enc.process_block(
                    &scratch_in_l[..chunk_size],
                    &scratch_in_r[..chunk_size],
                    chunk_out_mid,
                    chunk_out_side,
                );
            } else {
                enc.process_block(
                    &scratch_in_l[..chunk_size],
                    &scratch_in_r[..chunk_size],
                    chunk_out_mid,
                    &mut scratch_out_side[..chunk_size],
                );
            }
            offset += chunk_size;
        }
        return;
    }

    let in_l_slice = std::slice::from_raw_parts(in_l, n);
    let in_r_slice = if !in_r.is_null() {
        std::slice::from_raw_parts(in_r, n)
    } else {
        in_l_slice
    };

    if !out_side.is_null() && !std::ptr::eq(out_mid, out_side) {
        let out_mid_slice = std::slice::from_raw_parts_mut(out_mid, n);
        let out_side_slice = std::slice::from_raw_parts_mut(out_side, n);
        enc.process_block(in_l_slice, in_r_slice, out_mid_slice, out_side_slice);
    } else {
        let mut mono_scratch = [0.0f32; 1024];
        let mut offset = 0;
        let out_mid_slice = std::slice::from_raw_parts_mut(out_mid, n);
        while offset < n {
            let chunk_size = (n - offset).min(1024);
            let chunk_in_l = &in_l_slice[offset..offset + chunk_size];
            let chunk_in_r = &in_r_slice[offset..offset + chunk_size];
            let chunk_out_mid = &mut out_mid_slice[offset..offset + chunk_size];
            enc.process_block(chunk_in_l, chunk_in_r, chunk_out_mid, &mut mono_scratch[..chunk_size]);
            offset += chunk_size;
        }
    }
}

// ============================================================================
// Mid/Side Decoder (DSP #25) C-ABI Wrappers
// ============================================================================

#[no_mangle]
pub unsafe extern "C" fn n8_rust_mid_side_decoder_create(sample_rate: f32) -> *mut RustMidSideDecoder {
    Box::into_raw(Box::new(RustMidSideDecoder::new(sample_rate)))
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_mid_side_decoder_destroy(node: *mut RustMidSideDecoder) {
    if !node.is_null() {
        drop(Box::from_raw(node));
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_mid_side_decoder_reset(node: *mut RustMidSideDecoder) {
    if let Some(dec) = node.as_mut() {
        dec.reset();
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_mid_side_decoder_prepare(node: *mut RustMidSideDecoder, sample_rate: f32) {
    if let Some(dec) = node.as_mut() {
        dec.prepare(sample_rate);
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_mid_side_decoder_set_parameter(
    node: *mut RustMidSideDecoder,
    param_id: u32,
    value: f32,
) {
    if let Some(dec) = node.as_mut() {
        dec.set_parameter(param_id, value);
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_mid_side_decoder_get_parameter(
    node: *const RustMidSideDecoder,
    param_id: u32,
) -> f32 {
    if let Some(dec) = node.as_ref() {
        dec.get_parameter(param_id)
    } else {
        0.0
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_mid_side_decoder_process(
    node: *mut RustMidSideDecoder,
    in_mid: *const f32,
    in_side: *const f32,
    out_l: *mut f32,
    out_r: *mut f32,
    num_samples: u32,
) {
    if node.is_null() || in_mid.is_null() || out_l.is_null() || num_samples == 0 {
        return;
    }
    let dec = &mut *node;
    let n = num_samples as usize;

    let is_in_place = std::ptr::eq(in_mid, out_l as *const f32)
        || (!in_side.is_null() && !out_r.is_null() && std::ptr::eq(in_side, out_r as *const f32));

    if is_in_place {
        let mut scratch_in_mid = [0.0f32; 1024];
        let mut scratch_in_side = [0.0f32; 1024];
        let mut scratch_out_r = [0.0f32; 1024];
        let is_mono_out = out_r.is_null() || std::ptr::eq(out_l, out_r);

        let mut offset = 0;
        while offset < n {
            let chunk_size = (n - offset).min(1024);
            std::ptr::copy_nonoverlapping(in_mid.add(offset), scratch_in_mid.as_mut_ptr(), chunk_size);
            if !in_side.is_null() {
                std::ptr::copy_nonoverlapping(in_side.add(offset), scratch_in_side.as_mut_ptr(), chunk_size);
            } else {
                scratch_in_side[..chunk_size].fill(0.0);
            }

            let chunk_out_l = std::slice::from_raw_parts_mut(out_l.add(offset), chunk_size);

            if !is_mono_out {
                let chunk_out_r = std::slice::from_raw_parts_mut(out_r.add(offset), chunk_size);
                dec.process_block(
                    &scratch_in_mid[..chunk_size],
                    &scratch_in_side[..chunk_size],
                    chunk_out_l,
                    chunk_out_r,
                );
            } else {
                dec.process_block(
                    &scratch_in_mid[..chunk_size],
                    &scratch_in_side[..chunk_size],
                    chunk_out_l,
                    &mut scratch_out_r[..chunk_size],
                );
            }
            offset += chunk_size;
        }
        return;
    }

    let in_mid_slice = std::slice::from_raw_parts(in_mid, n);
    let in_side_slice = if !in_side.is_null() {
        std::slice::from_raw_parts(in_side, n)
    } else {
        &[0.0f32; 0]
    };

    if !out_r.is_null() && !std::ptr::eq(out_l, out_r) {
        let out_l_slice = std::slice::from_raw_parts_mut(out_l, n);
        let out_r_slice = std::slice::from_raw_parts_mut(out_r, n);
        dec.process_block(in_mid_slice, in_side_slice, out_l_slice, out_r_slice);
    } else {
        let mut mono_scratch = [0.0f32; 1024];
        let mut offset = 0;
        let out_l_slice = std::slice::from_raw_parts_mut(out_l, n);
        while offset < n {
            let chunk_size = (n - offset).min(1024);
            let chunk_in_mid = &in_mid_slice[offset..offset + chunk_size];
            let chunk_in_side = if !in_side.is_null() {
                &in_side_slice[offset..offset + chunk_size]
            } else {
                &[0.0f32; 0]
            };
            let chunk_out_l = &mut out_l_slice[offset..offset + chunk_size];
            dec.process_block(chunk_in_mid, chunk_in_side, chunk_out_l, &mut mono_scratch[..chunk_size]);
            offset += chunk_size;
        }
    }
}

// ============================================================================
// Spectral Processor (DSP #29) C-ABI Wrappers
// ============================================================================

#[no_mangle]
pub unsafe extern "C" fn n8_rust_spectral_processor_create() -> *mut RustSpectralProcessor {
    Box::into_raw(Box::new(RustSpectralProcessor::new()))
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_spectral_processor_destroy(node: *mut RustSpectralProcessor) {
    if !node.is_null() {
        drop(Box::from_raw(node));
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_spectral_processor_reset(node: *mut RustSpectralProcessor) {
    if let Some(p) = node.as_mut() {
        p.reset();
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_spectral_processor_prepare(node: *mut RustSpectralProcessor, sample_rate: f32) {
    if let Some(p) = node.as_mut() {
        p.prepare(sample_rate);
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_spectral_processor_set_parameter(
    node: *mut RustSpectralProcessor,
    param_id: u32,
    value: f32,
) {
    if let Some(p) = node.as_mut() {
        p.set_parameter(param_id, value);
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_spectral_processor_get_parameter(
    node: *const RustSpectralProcessor,
    param_id: u32,
) -> f32 {
    if let Some(p) = node.as_ref() {
        p.get_parameter(param_id)
    } else {
        0.0
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_spectral_processor_process(
    node: *mut RustSpectralProcessor,
    in_l: *const f32,
    in_r: *const f32,
    out_l: *mut f32,
    out_r: *mut f32,
    num_samples: u32,
) {
    if node.is_null() || in_l.is_null() || out_l.is_null() || num_samples == 0 {
        return;
    }
    let p = &mut *node;
    let n = num_samples as usize;

    let is_in_place = std::ptr::eq(in_l, out_l as *const f32)
        || (!in_r.is_null() && !out_r.is_null() && std::ptr::eq(in_r, out_r as *const f32));

    if is_in_place {
        let mut scratch_in_l = [0.0f32; 1024];
        let mut scratch_in_r = [0.0f32; 1024];
        let mut scratch_out_r = [0.0f32; 1024];
        let is_mono_out = out_r.is_null() || std::ptr::eq(out_l, out_r);

        let mut offset = 0;
        while offset < n {
            let chunk_size = (n - offset).min(1024);
            std::ptr::copy_nonoverlapping(in_l.add(offset), scratch_in_l.as_mut_ptr(), chunk_size);
            if !in_r.is_null() {
                std::ptr::copy_nonoverlapping(in_r.add(offset), scratch_in_r.as_mut_ptr(), chunk_size);
            } else {
                scratch_in_r[..chunk_size].copy_from_slice(&scratch_in_l[..chunk_size]);
            }

            let chunk_out_l = std::slice::from_raw_parts_mut(out_l.add(offset), chunk_size);

            if !is_mono_out {
                let chunk_out_r = std::slice::from_raw_parts_mut(out_r.add(offset), chunk_size);
                p.process_block(
                    &scratch_in_l[..chunk_size],
                    &scratch_in_r[..chunk_size],
                    chunk_out_l,
                    chunk_out_r,
                );
            } else {
                p.process_block(
                    &scratch_in_l[..chunk_size],
                    &scratch_in_r[..chunk_size],
                    chunk_out_l,
                    &mut scratch_out_r[..chunk_size],
                );
            }
            offset += chunk_size;
        }
        return;
    }

    let in_l_slice = std::slice::from_raw_parts(in_l, n);
    let in_r_slice = if !in_r.is_null() {
        std::slice::from_raw_parts(in_r, n)
    } else {
        in_l_slice
    };

    if !out_r.is_null() && !std::ptr::eq(out_l, out_r) {
        let out_l_slice = std::slice::from_raw_parts_mut(out_l, n);
        let out_r_slice = std::slice::from_raw_parts_mut(out_r, n);
        p.process_block(in_l_slice, in_r_slice, out_l_slice, out_r_slice);
    } else {
        let mut mono_scratch = [0.0f32; 1024];
        let mut offset = 0;
        let out_l_slice = std::slice::from_raw_parts_mut(out_l, n);
        while offset < n {
            let chunk_size = (n - offset).min(1024);
            let chunk_in_l = &in_l_slice[offset..offset + chunk_size];
            let chunk_in_r = &in_r_slice[offset..offset + chunk_size];
            let chunk_out_l = &mut out_l_slice[offset..offset + chunk_size];
            p.process_block(chunk_in_l, chunk_in_r, chunk_out_l, &mut mono_scratch[..chunk_size]);
            offset += chunk_size;
        }
    }
}

// ============================================================================
// Spectral Freeze (DSP #12)
// ============================================================================

#[no_mangle]
pub unsafe extern "C" fn n8_rust_spectral_freeze_create() -> *mut RustSpectralFreeze {
    Box::into_raw(Box::new(RustSpectralFreeze::new()))
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_spectral_freeze_destroy(node: *mut RustSpectralFreeze) {
    if !node.is_null() {
        drop(Box::from_raw(node));
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_spectral_freeze_reset(node: *mut RustSpectralFreeze) {
    if let Some(p) = node.as_mut() {
        p.reset();
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_spectral_freeze_prepare(node: *mut RustSpectralFreeze, sample_rate: f32) {
    if let Some(p) = node.as_mut() {
        p.prepare(sample_rate);
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_spectral_freeze_set_parameter(
    node: *mut RustSpectralFreeze,
    param_id: u32,
    value: f32,
) {
    if let Some(p) = node.as_mut() {
        p.set_parameter(param_id, value);
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_spectral_freeze_get_parameter(
    node: *const RustSpectralFreeze,
    param_id: u32,
) -> f32 {
    if let Some(p) = node.as_ref() {
        p.get_parameter(param_id)
    } else {
        0.0
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_spectral_freeze_process(
    node: *mut RustSpectralFreeze,
    in_l: *const f32,
    in_r: *const f32,
    out_l: *mut f32,
    out_r: *mut f32,
    num_samples: u32,
) {
    if node.is_null() || in_l.is_null() || out_l.is_null() || num_samples == 0 {
        return;
    }
    let p = &mut *node;
    let n = num_samples as usize;

    let is_in_place = std::ptr::eq(in_l, out_l as *const f32)
        || (!in_r.is_null() && !out_r.is_null() && std::ptr::eq(in_r, out_r as *const f32));

    if is_in_place {
        let mut scratch_in_l = [0.0f32; 1024];
        let mut scratch_in_r = [0.0f32; 1024];
        let mut scratch_out_r = [0.0f32; 1024];
        let is_mono_out = out_r.is_null() || std::ptr::eq(out_l, out_r);

        let mut offset = 0;
        while offset < n {
            let chunk_size = (n - offset).min(1024);
            std::ptr::copy_nonoverlapping(in_l.add(offset), scratch_in_l.as_mut_ptr(), chunk_size);
            if !in_r.is_null() {
                std::ptr::copy_nonoverlapping(in_r.add(offset), scratch_in_r.as_mut_ptr(), chunk_size);
            } else {
                scratch_in_r[..chunk_size].copy_from_slice(&scratch_in_l[..chunk_size]);
            }

            let chunk_out_l = std::slice::from_raw_parts_mut(out_l.add(offset), chunk_size);

            if !is_mono_out {
                let chunk_out_r = std::slice::from_raw_parts_mut(out_r.add(offset), chunk_size);
                p.process(
                    &scratch_in_l[..chunk_size],
                    &scratch_in_r[..chunk_size],
                    chunk_out_l,
                    chunk_out_r,
                );
            } else {
                p.process(
                    &scratch_in_l[..chunk_size],
                    &scratch_in_r[..chunk_size],
                    chunk_out_l,
                    &mut scratch_out_r[..chunk_size],
                );
            }
            offset += chunk_size;
        }
        return;
    }

    let in_l_slice = std::slice::from_raw_parts(in_l, n);
    let in_r_slice = if !in_r.is_null() {
        std::slice::from_raw_parts(in_r, n)
    } else {
        in_l_slice
    };

    if !out_r.is_null() && !std::ptr::eq(out_l, out_r) {
        let out_l_slice = std::slice::from_raw_parts_mut(out_l, n);
        let out_r_slice = std::slice::from_raw_parts_mut(out_r, n);
        p.process(in_l_slice, in_r_slice, out_l_slice, out_r_slice);
    } else {
        let mut mono_scratch = [0.0f32; 1024];
        let mut offset = 0;
        let out_l_slice = std::slice::from_raw_parts_mut(out_l, n);
        while offset < n {
            let chunk_size = (n - offset).min(1024);
            let chunk_in_l = &in_l_slice[offset..offset + chunk_size];
            let chunk_in_r = &in_r_slice[offset..offset + chunk_size];
            let chunk_out_l = &mut out_l_slice[offset..offset + chunk_size];
            p.process(chunk_in_l, chunk_in_r, chunk_out_l, &mut mono_scratch[..chunk_size]);
            offset += chunk_size;
        }
    }
}

// ============================================================================
// Spectral Smear (DSP #52)
// ============================================================================

#[no_mangle]
pub unsafe extern "C" fn n8_rust_spectral_smear_create() -> *mut RustSpectralSmear {
    Box::into_raw(Box::new(RustSpectralSmear::new()))
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_spectral_smear_destroy(node: *mut RustSpectralSmear) {
    if !node.is_null() {
        drop(Box::from_raw(node));
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_spectral_smear_reset(node: *mut RustSpectralSmear) {
    if let Some(p) = node.as_mut() {
        p.reset();
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_spectral_smear_prepare(node: *mut RustSpectralSmear, sample_rate: f32) {
    if let Some(p) = node.as_mut() {
        p.prepare(sample_rate);
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_spectral_smear_set_parameter(
    node: *mut RustSpectralSmear,
    param_id: u32,
    value: f32,
) {
    if let Some(p) = node.as_mut() {
        p.set_parameter(param_id, value);
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_spectral_smear_get_parameter(
    node: *const RustSpectralSmear,
    param_id: u32,
) -> f32 {
    if let Some(p) = node.as_ref() {
        p.get_parameter(param_id)
    } else {
        0.0
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_spectral_smear_process(
    node: *mut RustSpectralSmear,
    in_l: *const f32,
    in_r: *const f32,
    out_l: *mut f32,
    out_r: *mut f32,
    num_samples: u32,
) {
    if node.is_null() || in_l.is_null() || out_l.is_null() || num_samples == 0 {
        return;
    }
    let p = &mut *node;
    let n = num_samples as usize;

    let is_in_place = std::ptr::eq(in_l, out_l as *const f32)
        || (!in_r.is_null() && !out_r.is_null() && std::ptr::eq(in_r, out_r as *const f32));

    if is_in_place {
        let mut scratch_in_l = [0.0f32; 1024];
        let mut scratch_in_r = [0.0f32; 1024];
        let mut scratch_out_r = [0.0f32; 1024];
        let is_mono_out = out_r.is_null() || std::ptr::eq(out_l, out_r);

        let mut offset = 0;
        while offset < n {
            let chunk_size = (n - offset).min(1024);
            std::ptr::copy_nonoverlapping(in_l.add(offset), scratch_in_l.as_mut_ptr(), chunk_size);
            if !in_r.is_null() {
                std::ptr::copy_nonoverlapping(in_r.add(offset), scratch_in_r.as_mut_ptr(), chunk_size);
            } else {
                scratch_in_r[..chunk_size].copy_from_slice(&scratch_in_l[..chunk_size]);
            }

            let chunk_out_l = std::slice::from_raw_parts_mut(out_l.add(offset), chunk_size);

            if !is_mono_out {
                let chunk_out_r = std::slice::from_raw_parts_mut(out_r.add(offset), chunk_size);
                p.process(
                    &scratch_in_l[..chunk_size],
                    &scratch_in_r[..chunk_size],
                    chunk_out_l,
                    chunk_out_r,
                );
            } else {
                p.process(
                    &scratch_in_l[..chunk_size],
                    &scratch_in_r[..chunk_size],
                    chunk_out_l,
                    &mut scratch_out_r[..chunk_size],
                );
            }
            offset += chunk_size;
        }
        return;
    }

    let in_l_slice = std::slice::from_raw_parts(in_l, n);
    let in_r_slice = if !in_r.is_null() {
        std::slice::from_raw_parts(in_r, n)
    } else {
        in_l_slice
    };

    if !out_r.is_null() && !std::ptr::eq(out_l, out_r) {
        let out_l_slice = std::slice::from_raw_parts_mut(out_l, n);
        let out_r_slice = std::slice::from_raw_parts_mut(out_r, n);
        p.process(in_l_slice, in_r_slice, out_l_slice, out_r_slice);
    } else {
        let mut mono_scratch = [0.0f32; 1024];
        let mut offset = 0;
        let out_l_slice = std::slice::from_raw_parts_mut(out_l, n);
        while offset < n {
            let chunk_size = (n - offset).min(1024);
            let chunk_in_l = &in_l_slice[offset..offset + chunk_size];
            let chunk_in_r = &in_r_slice[offset..offset + chunk_size];
            let chunk_out_l = &mut out_l_slice[offset..offset + chunk_size];
            p.process(chunk_in_l, chunk_in_r, chunk_out_l, &mut mono_scratch[..chunk_size]);
            offset += chunk_size;
        }
    }
}

// ============================================================================
// Pitch Shifter (DSP #27)
// ============================================================================

#[no_mangle]
pub unsafe extern "C" fn n8_rust_pitch_shifter_create() -> *mut RustPitchShifter {
    Box::into_raw(Box::new(RustPitchShifter::new()))
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_pitch_shifter_destroy(node: *mut RustPitchShifter) {
    if !node.is_null() {
        drop(Box::from_raw(node));
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_pitch_shifter_reset(node: *mut RustPitchShifter) {
    if let Some(p) = node.as_mut() {
        p.reset();
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_pitch_shifter_prepare(node: *mut RustPitchShifter, sample_rate: f32) {
    if let Some(p) = node.as_mut() {
        p.prepare(sample_rate);
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_pitch_shifter_set_parameter(
    node: *mut RustPitchShifter,
    param_id: u32,
    value: f32,
) {
    if let Some(p) = node.as_mut() {
        p.set_parameter(param_id, value);
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_pitch_shifter_get_parameter(
    node: *const RustPitchShifter,
    param_id: u32,
) -> f32 {
    if let Some(p) = node.as_ref() {
        p.get_parameter(param_id)
    } else {
        0.0
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_pitch_shifter_process(
    node: *mut RustPitchShifter,
    in_l: *const f32,
    in_r: *const f32,
    out_l: *mut f32,
    out_r: *mut f32,
    num_samples: u32,
) {
    if node.is_null() || in_l.is_null() || out_l.is_null() || num_samples == 0 {
        return;
    }
    let p = &mut *node;
    let n = num_samples as usize;

    let is_in_place = std::ptr::eq(in_l, out_l as *const f32)
        || (!in_r.is_null() && !out_r.is_null() && std::ptr::eq(in_r, out_r as *const f32));

    if is_in_place {
        let mut scratch_in_l = [0.0f32; 1024];
        let mut scratch_in_r = [0.0f32; 1024];
        let mut scratch_out_r = [0.0f32; 1024];
        let is_mono_out = out_r.is_null() || std::ptr::eq(out_l, out_r);

        let mut offset = 0;
        while offset < n {
            let chunk_size = (n - offset).min(1024);
            std::ptr::copy_nonoverlapping(in_l.add(offset), scratch_in_l.as_mut_ptr(), chunk_size);
            if !in_r.is_null() {
                std::ptr::copy_nonoverlapping(in_r.add(offset), scratch_in_r.as_mut_ptr(), chunk_size);
            } else {
                scratch_in_r[..chunk_size].copy_from_slice(&scratch_in_l[..chunk_size]);
            }

            let chunk_out_l = std::slice::from_raw_parts_mut(out_l.add(offset), chunk_size);

            if !is_mono_out {
                let chunk_out_r = std::slice::from_raw_parts_mut(out_r.add(offset), chunk_size);
                p.process(
                    &scratch_in_l[..chunk_size],
                    &scratch_in_r[..chunk_size],
                    chunk_out_l,
                    chunk_out_r,
                );
            } else {
                p.process(
                    &scratch_in_l[..chunk_size],
                    &scratch_in_r[..chunk_size],
                    chunk_out_l,
                    &mut scratch_out_r[..chunk_size],
                );
            }
            offset += chunk_size;
        }
        return;
    }

    let in_l_slice = std::slice::from_raw_parts(in_l, n);
    let in_r_slice = if !in_r.is_null() {
        std::slice::from_raw_parts(in_r, n)
    } else {
        in_l_slice
    };

    if !out_r.is_null() && !std::ptr::eq(out_l, out_r) {
        let out_l_slice = std::slice::from_raw_parts_mut(out_l, n);
        let out_r_slice = std::slice::from_raw_parts_mut(out_r, n);
        p.process(in_l_slice, in_r_slice, out_l_slice, out_r_slice);
    } else {
        let mut mono_scratch = [0.0f32; 1024];
        let mut offset = 0;
        let out_l_slice = std::slice::from_raw_parts_mut(out_l, n);
        while offset < n {
            let chunk_size = (n - offset).min(1024);
            let chunk_in_l = &in_l_slice[offset..offset + chunk_size];
            let chunk_in_r = &in_r_slice[offset..offset + chunk_size];
            let chunk_out_l = &mut out_l_slice[offset..offset + chunk_size];
            p.process(chunk_in_l, chunk_in_r, chunk_out_l, &mut mono_scratch[..chunk_size]);
            offset += chunk_size;
        }
    }
}

// ============================================================================
// Sample Player (DSP #53)
// ============================================================================

#[no_mangle]
pub unsafe extern "C" fn n8_rust_sample_player_create() -> *mut RustSamplePlayer {
    Box::into_raw(Box::new(RustSamplePlayer::new()))
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_sample_player_destroy(node: *mut RustSamplePlayer) {
    if !node.is_null() {
        drop(Box::from_raw(node));
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_sample_player_reset(node: *mut RustSamplePlayer) {
    if let Some(p) = node.as_mut() {
        p.reset();
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_sample_player_prepare(node: *mut RustSamplePlayer, sample_rate: f64) {
    if let Some(p) = node.as_mut() {
        p.prepare(sample_rate);
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_sample_player_set_parameter(
    node: *mut RustSamplePlayer,
    param_id: u32,
    value: f32,
) {
    if let Some(p) = node.as_mut() {
        p.set_parameter(param_id, value);
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_sample_player_get_parameter(
    node: *const RustSamplePlayer,
    param_id: u32,
) -> f32 {
    if let Some(p) = node.as_ref() {
        p.get_parameter(param_id)
    } else {
        0.0
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_sample_player_trigger(node: *mut RustSamplePlayer) {
    if let Some(p) = node.as_mut() {
        p.trigger_playback();
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_sample_player_get_thumbnail(
    node: *const RustSamplePlayer,
    out_64: *mut f32,
) {
    if let Some(p) = node.as_ref() {
        if !out_64.is_null() {
            let out_slice = std::slice::from_raw_parts_mut(out_64, 64);
            let mut arr = [0.0f32; 64];
            p.get_thumbnail(&mut arr);
            out_slice.copy_from_slice(&arr);
        }
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_sample_player_load_sample(
    node: *mut RustSamplePlayer,
    l_data: *const f32,
    r_data: *const f32,
    num_samples: usize,
    sample_rate: f64,
) {
    if let Some(p) = node.as_mut() {
        if !l_data.is_null() && num_samples > 0 {
            let l_slice = std::slice::from_raw_parts(l_data, num_samples);
            let r_slice = if !r_data.is_null() {
                Some(std::slice::from_raw_parts(r_data, num_samples))
            } else {
                None
            };
            p.load_sample(l_slice, r_slice, sample_rate);
        }
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_sample_player_process(
    node: *mut RustSamplePlayer,
    in_l: *const f32,
    in_r: *const f32,
    trig: *const f32,
    out_l: *mut f32,
    out_r: *mut f32,
    num_samples: u32,
) {
    if node.is_null() || out_l.is_null() || num_samples == 0 {
        return;
    }
    let p = &mut *node;
    let n = num_samples as usize;

    let is_in_place = !in_l.is_null() && (std::ptr::eq(in_l, out_l as *const f32)
        || (!in_r.is_null() && !out_r.is_null() && std::ptr::eq(in_r, out_r as *const f32)));

    let is_mono_out = out_r.is_null() || std::ptr::eq(out_l, out_r);

    if is_in_place || in_l.is_null() {
        let mut scratch_in_l = [0.0f32; 1024];
        let mut scratch_in_r = [0.0f32; 1024];
        let mut scratch_trig = [0.0f32; 1024];
        let mut scratch_out_r = [0.0f32; 1024];

        let mut offset = 0;
        while offset < n {
            let chunk_size = (n - offset).min(1024);
            if !in_l.is_null() {
                std::ptr::copy_nonoverlapping(in_l.add(offset), scratch_in_l.as_mut_ptr(), chunk_size);
                if !in_r.is_null() {
                    std::ptr::copy_nonoverlapping(in_r.add(offset), scratch_in_r.as_mut_ptr(), chunk_size);
                } else {
                    scratch_in_r[..chunk_size].copy_from_slice(&scratch_in_l[..chunk_size]);
                }
            } else {
                scratch_in_l[..chunk_size].fill(0.0);
                scratch_in_r[..chunk_size].fill(0.0);
            }
            let trig_slice = if !trig.is_null() {
                std::ptr::copy_nonoverlapping(trig.add(offset), scratch_trig.as_mut_ptr(), chunk_size);
                Some(&scratch_trig[..chunk_size])
            } else {
                None
            };

            let chunk_out_l = std::slice::from_raw_parts_mut(out_l.add(offset), chunk_size);

            if !is_mono_out {
                let chunk_out_r = std::slice::from_raw_parts_mut(out_r.add(offset), chunk_size);
                p.process(
                    &scratch_in_l[..chunk_size],
                    &scratch_in_r[..chunk_size],
                    trig_slice,
                    chunk_out_l,
                    chunk_out_r,
                );
            } else {
                p.process(
                    &scratch_in_l[..chunk_size],
                    &scratch_in_r[..chunk_size],
                    trig_slice,
                    chunk_out_l,
                    &mut scratch_out_r[..chunk_size],
                );
            }
            offset += chunk_size;
        }
        return;
    }

    let in_l_slice = std::slice::from_raw_parts(in_l, n);
    let in_r_slice = if !in_r.is_null() {
        std::slice::from_raw_parts(in_r, n)
    } else {
        in_l_slice
    };
    let trig_slice = if !trig.is_null() {
        Some(std::slice::from_raw_parts(trig, n))
    } else {
        None
    };

    if !out_r.is_null() && !std::ptr::eq(out_l, out_r) {
        let out_l_slice = std::slice::from_raw_parts_mut(out_l, n);
        let out_r_slice = std::slice::from_raw_parts_mut(out_r, n);
        p.process(in_l_slice, in_r_slice, trig_slice, out_l_slice, out_r_slice);
    } else {
        let mut mono_scratch = [0.0f32; 1024];
        let mut offset = 0;
        let out_l_slice = std::slice::from_raw_parts_mut(out_l, n);
        while offset < n {
            let chunk_size = (n - offset).min(1024);
            let chunk_in_l = &in_l_slice[offset..offset + chunk_size];
            let chunk_in_r = &in_r_slice[offset..offset + chunk_size];
            let chunk_trig = trig_slice.map(|ts| &ts[offset..offset + chunk_size]);
            let chunk_out_l = &mut out_l_slice[offset..offset + chunk_size];
            p.process(chunk_in_l, chunk_in_r, chunk_trig, chunk_out_l, &mut mono_scratch[..chunk_size]);
            offset += chunk_size;
        }
    }
}

// ============================================================================
// Zero-Latency Partitioned Convolution Engine (DSP #48)
// ============================================================================

#[no_mangle]
pub unsafe extern "C" fn n8_rust_convolution_create() -> *mut RustConvolution {
    Box::into_raw(Box::new(RustConvolution::new()))
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_convolution_destroy(node: *mut RustConvolution) {
    if !node.is_null() {
        drop(Box::from_raw(node));
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_convolution_reset(node: *mut RustConvolution) {
    if let Some(p) = node.as_mut() {
        p.reset();
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_convolution_prepare(node: *mut RustConvolution, sample_rate: f64) {
    if let Some(p) = node.as_mut() {
        p.prepare(sample_rate);
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_convolution_set_parameter(
    node: *mut RustConvolution,
    param_id: u32,
    value: f32,
) {
    if let Some(p) = node.as_mut() {
        p.set_parameter(param_id, value);
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_convolution_get_parameter(
    node: *const RustConvolution,
    param_id: u32,
) -> f32 {
    if let Some(p) = node.as_ref() {
        p.get_parameter(param_id)
    } else {
        0.0
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_convolution_get_thumbnail(
    node: *const RustConvolution,
    out_64: *mut f32,
) {
    if let Some(p) = node.as_ref() {
        if !out_64.is_null() {
            let out_slice = std::slice::from_raw_parts_mut(out_64, 64);
            let mut arr = [0.0f32; 64];
            p.get_thumbnail(&mut arr);
            out_slice.copy_from_slice(&arr);
        }
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_convolution_set_custom_ir(
    node: *mut RustConvolution,
    l_data: *const f32,
    r_data: *const f32,
    num_samples: usize,
) {
    if let Some(p) = node.as_mut() {
        if !l_data.is_null() && num_samples > 0 {
            let l_slice = std::slice::from_raw_parts(l_data, num_samples);
            let r_slice = if !r_data.is_null() {
                Some(std::slice::from_raw_parts(r_data, num_samples))
            } else {
                None
            };
            p.set_custom_impulse_response(l_slice, r_slice);
        }
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_convolution_process(
    node: *mut RustConvolution,
    in_l: *const f32,
    in_r: *const f32,
    out_l: *mut f32,
    out_r: *mut f32,
    num_samples: u32,
) {
    if node.is_null() || out_l.is_null() || num_samples == 0 {
        return;
    }
    let p = &mut *node;
    let n = num_samples as usize;

    let is_in_place = !in_l.is_null() && (std::ptr::eq(in_l, out_l as *const f32)
        || (!in_r.is_null() && !out_r.is_null() && std::ptr::eq(in_r, out_r as *const f32)));

    let is_mono_out = out_r.is_null() || std::ptr::eq(out_l, out_r);

    if is_in_place || in_l.is_null() {
        let mut scratch_in_l = [0.0f32; 1024];
        let mut scratch_in_r = [0.0f32; 1024];
        let mut scratch_out_r = [0.0f32; 1024];

        let mut offset = 0;
        while offset < n {
            let chunk_size = (n - offset).min(1024);
            if !in_l.is_null() {
                std::ptr::copy_nonoverlapping(in_l.add(offset), scratch_in_l.as_mut_ptr(), chunk_size);
                if !in_r.is_null() {
                    std::ptr::copy_nonoverlapping(in_r.add(offset), scratch_in_r.as_mut_ptr(), chunk_size);
                } else {
                    scratch_in_r[..chunk_size].copy_from_slice(&scratch_in_l[..chunk_size]);
                }
            } else {
                scratch_in_l[..chunk_size].fill(0.0);
                scratch_in_r[..chunk_size].fill(0.0);
            }

            let chunk_out_l = std::slice::from_raw_parts_mut(out_l.add(offset), chunk_size);

            if !is_mono_out {
                let chunk_out_r = std::slice::from_raw_parts_mut(out_r.add(offset), chunk_size);
                p.process(
                    &scratch_in_l[..chunk_size],
                    &scratch_in_r[..chunk_size],
                    chunk_out_l,
                    chunk_out_r,
                );
            } else {
                p.process(
                    &scratch_in_l[..chunk_size],
                    &scratch_in_r[..chunk_size],
                    chunk_out_l,
                    &mut scratch_out_r[..chunk_size],
                );
            }
            offset += chunk_size;
        }
        return;
    }

    let in_l_slice = std::slice::from_raw_parts(in_l, n);
    let in_r_slice = if !in_r.is_null() {
        std::slice::from_raw_parts(in_r, n)
    } else {
        in_l_slice
    };

    if !out_r.is_null() && !std::ptr::eq(out_l, out_r) {
        let out_l_slice = std::slice::from_raw_parts_mut(out_l, n);
        let out_r_slice = std::slice::from_raw_parts_mut(out_r, n);
        p.process(in_l_slice, in_r_slice, out_l_slice, out_r_slice);
    } else {
        let mut mono_scratch = [0.0f32; 1024];
        let mut offset = 0;
        let out_l_slice = std::slice::from_raw_parts_mut(out_l, n);
        while offset < n {
            let chunk_size = (n - offset).min(1024);
            let chunk_in_l = &in_l_slice[offset..offset + chunk_size];
            let chunk_in_r = &in_r_slice[offset..offset + chunk_size];
            let chunk_out_l = &mut out_l_slice[offset..offset + chunk_size];
            p.process(chunk_in_l, chunk_in_r, chunk_out_l, &mut mono_scratch[..chunk_size]);
            offset += chunk_size;
        }
    }
}

// ============================================================================
// Polyphase Oversampling & Saturation Engine (DSP #48)
// ============================================================================

#[no_mangle]
pub unsafe extern "C" fn n8_rust_oversampler_create() -> *mut RustOversampler {
    Box::into_raw(Box::new(RustOversampler::new()))
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_oversampler_destroy(node: *mut RustOversampler) {
    if !node.is_null() {
        drop(Box::from_raw(node));
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_oversampler_reset(node: *mut RustOversampler) {
    if let Some(p) = node.as_mut() {
        p.reset();
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_oversampler_prepare(
    node: *mut RustOversampler,
    sample_rate: f64,
    max_block_size: usize,
) {
    if let Some(p) = node.as_mut() {
        p.prepare(sample_rate, max_block_size);
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_oversampler_set_parameter(
    node: *mut RustOversampler,
    param_id: u32,
    value: f32,
) {
    if let Some(p) = node.as_mut() {
        p.set_parameter(param_id, value);
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_oversampler_get_parameter(
    node: *const RustOversampler,
    param_id: u32,
) -> f32 {
    if let Some(p) = node.as_ref() {
        p.get_parameter(param_id)
    } else {
        0.0
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_oversampler_get_latency(node: *const RustOversampler) -> usize {
    if let Some(p) = node.as_ref() {
        p.get_latency_in_samples()
    } else {
        0
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_oversampler_process(
    node: *mut RustOversampler,
    in_l: *const f32,
    in_r: *const f32,
    out_l: *mut f32,
    out_r: *mut f32,
    num_samples: u32,
) {
    if node.is_null() || out_l.is_null() || num_samples == 0 {
        return;
    }
    let p = &mut *node;
    let n = num_samples as usize;

    let is_in_place = !in_l.is_null() && (std::ptr::eq(in_l, out_l as *const f32)
        || (!in_r.is_null() && !out_r.is_null() && std::ptr::eq(in_r, out_r as *const f32)));

    let is_mono_out = out_r.is_null() || std::ptr::eq(out_l, out_r);

    if is_in_place || in_l.is_null() {
        let mut scratch_in_l = [0.0f32; 1024];
        let mut scratch_in_r = [0.0f32; 1024];
        let mut scratch_out_r = [0.0f32; 1024];

        let mut offset = 0;
        while offset < n {
            let chunk_size = (n - offset).min(1024);
            if !in_l.is_null() {
                std::ptr::copy_nonoverlapping(in_l.add(offset), scratch_in_l.as_mut_ptr(), chunk_size);
                if !in_r.is_null() {
                    std::ptr::copy_nonoverlapping(in_r.add(offset), scratch_in_r.as_mut_ptr(), chunk_size);
                } else {
                    scratch_in_r[..chunk_size].copy_from_slice(&scratch_in_l[..chunk_size]);
                }
            } else {
                scratch_in_l[..chunk_size].fill(0.0);
                scratch_in_r[..chunk_size].fill(0.0);
            }

            let chunk_out_l = std::slice::from_raw_parts_mut(out_l.add(offset), chunk_size);

            if !is_mono_out {
                let chunk_out_r = std::slice::from_raw_parts_mut(out_r.add(offset), chunk_size);
                p.process(
                    &scratch_in_l[..chunk_size],
                    &scratch_in_r[..chunk_size],
                    chunk_out_l,
                    chunk_out_r,
                );
            } else {
                p.process(
                    &scratch_in_l[..chunk_size],
                    &scratch_in_r[..chunk_size],
                    chunk_out_l,
                    &mut scratch_out_r[..chunk_size],
                );
            }
            offset += chunk_size;
        }
        return;
    }

    let in_l_slice = std::slice::from_raw_parts(in_l, n);
    let in_r_slice = if !in_r.is_null() {
        std::slice::from_raw_parts(in_r, n)
    } else {
        in_l_slice
    };

    if !out_r.is_null() && !std::ptr::eq(out_l, out_r) {
        let out_l_slice = std::slice::from_raw_parts_mut(out_l, n);
        let out_r_slice = std::slice::from_raw_parts_mut(out_r, n);
        p.process(in_l_slice, in_r_slice, out_l_slice, out_r_slice);
    } else {
        let mut mono_scratch = [0.0f32; 1024];
        let mut offset = 0;
        let out_l_slice = std::slice::from_raw_parts_mut(out_l, n);
        while offset < n {
            let chunk_size = (n - offset).min(1024);
            let chunk_in_l = &in_l_slice[offset..offset + chunk_size];
            let chunk_in_r = &in_r_slice[offset..offset + chunk_size];
            let chunk_out_l = &mut out_l_slice[offset..offset + chunk_size];
            p.process(chunk_in_l, chunk_in_r, chunk_out_l, &mut mono_scratch[..chunk_size]);
            offset += chunk_size;
        }
    }
}

// ============================================================================
// Topology-Preserving Transform (TPT) State Variable Filter C-ABI FFI Exports (DSP #4)
// ============================================================================

#[no_mangle]
pub unsafe extern "C" fn n8_rust_simple_filter_create() -> *mut RustSimpleFilter {
    Box::into_raw(Box::new(RustSimpleFilter::new()))
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_simple_filter_destroy(node: *mut RustSimpleFilter) {
    if !node.is_null() {
        drop(Box::from_raw(node));
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_simple_filter_reset(node: *mut RustSimpleFilter) {
    if let Some(p) = node.as_mut() {
        p.reset();
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_simple_filter_prepare(node: *mut RustSimpleFilter, sample_rate: f64) {
    if let Some(p) = node.as_mut() {
        p.prepare(sample_rate);
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_simple_filter_set_parameter(node: *mut RustSimpleFilter, param_id: u32, value: f32) {
    if let Some(p) = node.as_mut() {
        p.set_parameter(param_id, value);
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_simple_filter_get_parameter(node: *const RustSimpleFilter, param_id: u32) -> f32 {
    if let Some(p) = node.as_ref() {
        p.get_parameter(param_id)
    } else {
        0.0
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_simple_filter_process(
    node: *mut RustSimpleFilter,
    in_l: *const f32,
    in_r: *const f32,
    fm_mod: *const f32,
    out_l: *mut f32,
    out_r: *mut f32,
    num_samples: u32,
) {
    if node.is_null() || out_l.is_null() || num_samples == 0 {
        return;
    }
    let p = &mut *node;
    let n = num_samples as usize;

    let is_in_place = !in_l.is_null() && (std::ptr::eq(in_l, out_l as *const f32)
        || (!in_r.is_null() && !out_r.is_null() && std::ptr::eq(in_r, out_r as *const f32)));

    let is_mono_out = out_r.is_null() || std::ptr::eq(out_l, out_r);

    if is_in_place || in_l.is_null() {
        let mut scratch_in_l = [0.0f32; 1024];
        let mut scratch_in_r = [0.0f32; 1024];
        let mut scratch_out_r = [0.0f32; 1024];
        let mut scratch_fm = [0.0f32; 1024];

        let mut offset = 0;
        while offset < n {
            let chunk_size = (n - offset).min(1024);
            if !in_l.is_null() {
                std::ptr::copy_nonoverlapping(in_l.add(offset), scratch_in_l.as_mut_ptr(), chunk_size);
                if !in_r.is_null() {
                    std::ptr::copy_nonoverlapping(in_r.add(offset), scratch_in_r.as_mut_ptr(), chunk_size);
                } else {
                    scratch_in_r[..chunk_size].copy_from_slice(&scratch_in_l[..chunk_size]);
                }
            } else {
                scratch_in_l[..chunk_size].fill(0.0);
                scratch_in_r[..chunk_size].fill(0.0);
            }

            let fm_slice = if !fm_mod.is_null() {
                std::ptr::copy_nonoverlapping(fm_mod.add(offset), scratch_fm.as_mut_ptr(), chunk_size);
                Some(&scratch_fm[..chunk_size])
            } else {
                None
            };

            let chunk_out_l = std::slice::from_raw_parts_mut(out_l.add(offset), chunk_size);

            if !is_mono_out {
                let chunk_out_r = std::slice::from_raw_parts_mut(out_r.add(offset), chunk_size);
                p.process(
                    &scratch_in_l[..chunk_size],
                    &scratch_in_r[..chunk_size],
                    fm_slice,
                    chunk_out_l,
                    chunk_out_r,
                );
            } else {
                p.process(
                    &scratch_in_l[..chunk_size],
                    &scratch_in_r[..chunk_size],
                    fm_slice,
                    chunk_out_l,
                    &mut scratch_out_r[..chunk_size],
                );
            }
            offset += chunk_size;
        }
        return;
    }

    let in_l_slice = std::slice::from_raw_parts(in_l, n);
    let in_r_slice = if !in_r.is_null() {
        std::slice::from_raw_parts(in_r, n)
    } else {
        in_l_slice
    };
    let fm_opt = if !fm_mod.is_null() {
        Some(std::slice::from_raw_parts(fm_mod, n))
    } else {
        None
    };

    if !out_r.is_null() && !std::ptr::eq(out_l, out_r) {
        let out_l_slice = std::slice::from_raw_parts_mut(out_l, n);
        let out_r_slice = std::slice::from_raw_parts_mut(out_r, n);
        p.process(in_l_slice, in_r_slice, fm_opt, out_l_slice, out_r_slice);
    } else {
        let mut mono_scratch = [0.0f32; 1024];
        let mut offset = 0;
        let out_l_slice = std::slice::from_raw_parts_mut(out_l, n);
        while offset < n {
            let chunk_size = (n - offset).min(1024);
            let chunk_in_l = &in_l_slice[offset..offset + chunk_size];
            let chunk_in_r = &in_r_slice[offset..offset + chunk_size];
            let chunk_fm = fm_opt.map(|f| &f[offset..offset + chunk_size]);
            let chunk_out_l = &mut out_l_slice[offset..offset + chunk_size];
            p.process(chunk_in_l, chunk_in_r, chunk_fm, chunk_out_l, &mut mono_scratch[..chunk_size]);
            offset += chunk_size;
        }
    }
}

// ============================================================================
// Stereo Delay C-ABI FFI Exports (DSP #5)
// ============================================================================

#[no_mangle]
pub unsafe extern "C" fn n8_rust_simple_delay_create() -> *mut RustSimpleDelay {
    Box::into_raw(Box::new(RustSimpleDelay::new()))
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_simple_delay_destroy(node: *mut RustSimpleDelay) {
    if !node.is_null() {
        drop(Box::from_raw(node));
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_simple_delay_reset(node: *mut RustSimpleDelay) {
    if let Some(p) = node.as_mut() {
        p.reset();
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_simple_delay_prepare(node: *mut RustSimpleDelay, sample_rate: f64) {
    if let Some(p) = node.as_mut() {
        p.prepare(sample_rate);
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_simple_delay_set_parameter(node: *mut RustSimpleDelay, param_id: u32, value: f32) {
    if let Some(p) = node.as_mut() {
        p.set_parameter(param_id, value);
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_simple_delay_get_parameter(node: *const RustSimpleDelay, param_id: u32) -> f32 {
    if let Some(p) = node.as_ref() {
        p.get_parameter(param_id)
    } else {
        0.0
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_simple_delay_process(
    node: *mut RustSimpleDelay,
    in_l: *const f32,
    in_r: *const f32,
    out_l: *mut f32,
    out_r: *mut f32,
    num_samples: u32,
) {
    if node.is_null() || out_l.is_null() || num_samples == 0 {
        return;
    }
    let p = &mut *node;
    let n = num_samples as usize;

    let is_in_place = !in_l.is_null() && (std::ptr::eq(in_l, out_l as *const f32)
        || (!in_r.is_null() && !out_r.is_null() && std::ptr::eq(in_r, out_r as *const f32)));

    let is_mono_out = out_r.is_null() || std::ptr::eq(out_l, out_r);

    if is_in_place || in_l.is_null() {
        let mut scratch_in_l = [0.0f32; 1024];
        let mut scratch_in_r = [0.0f32; 1024];
        let mut scratch_out_r = [0.0f32; 1024];

        let mut offset = 0;
        while offset < n {
            let chunk_size = (n - offset).min(1024);
            if !in_l.is_null() {
                std::ptr::copy_nonoverlapping(in_l.add(offset), scratch_in_l.as_mut_ptr(), chunk_size);
                if !in_r.is_null() {
                    std::ptr::copy_nonoverlapping(in_r.add(offset), scratch_in_r.as_mut_ptr(), chunk_size);
                } else {
                    scratch_in_r[..chunk_size].copy_from_slice(&scratch_in_l[..chunk_size]);
                }
            } else {
                scratch_in_l[..chunk_size].fill(0.0);
                scratch_in_r[..chunk_size].fill(0.0);
            }

            let chunk_out_l = std::slice::from_raw_parts_mut(out_l.add(offset), chunk_size);

            if !is_mono_out {
                let chunk_out_r = std::slice::from_raw_parts_mut(out_r.add(offset), chunk_size);
                p.process(
                    &scratch_in_l[..chunk_size],
                    &scratch_in_r[..chunk_size],
                    chunk_out_l,
                    chunk_out_r,
                );
            } else {
                p.process(
                    &scratch_in_l[..chunk_size],
                    &scratch_in_r[..chunk_size],
                    chunk_out_l,
                    &mut scratch_out_r[..chunk_size],
                );
            }
            offset += chunk_size;
        }
        return;
    }

    let in_l_slice = std::slice::from_raw_parts(in_l, n);
    let in_r_slice = if !in_r.is_null() {
        std::slice::from_raw_parts(in_r, n)
    } else {
        in_l_slice
    };

    if !out_r.is_null() && !std::ptr::eq(out_l, out_r) {
        let out_l_slice = std::slice::from_raw_parts_mut(out_l, n);
        let out_r_slice = std::slice::from_raw_parts_mut(out_r, n);
        p.process(in_l_slice, in_r_slice, out_l_slice, out_r_slice);
    } else {
        let mut mono_scratch = [0.0f32; 1024];
        let mut offset = 0;
        let out_l_slice = std::slice::from_raw_parts_mut(out_l, n);
        while offset < n {
            let chunk_size = (n - offset).min(1024);
            let chunk_in_l = &in_l_slice[offset..offset + chunk_size];
            let chunk_in_r = &in_r_slice[offset..offset + chunk_size];
            let chunk_out_l = &mut out_l_slice[offset..offset + chunk_size];
            p.process(chunk_in_l, chunk_in_r, chunk_out_l, &mut mono_scratch[..chunk_size]);
            offset += chunk_size;
        }
    }
}

// ============================================================================
// Passthrough & Gain Utility C-ABI FFI Exports (DSP #3)
// ============================================================================

#[no_mangle]
pub unsafe extern "C" fn n8_rust_passthrough_create() -> *mut RustPassthrough {
    Box::into_raw(Box::new(RustPassthrough::new()))
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_passthrough_destroy(node: *mut RustPassthrough) {
    if !node.is_null() {
        drop(Box::from_raw(node));
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_passthrough_reset(node: *mut RustPassthrough) {
    if let Some(p) = node.as_mut() {
        p.reset();
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_passthrough_prepare(node: *mut RustPassthrough) {
    if let Some(p) = node.as_mut() {
        p.prepare();
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_passthrough_set_parameter(node: *mut RustPassthrough, param_id: u32, value: f32) {
    if let Some(p) = node.as_mut() {
        p.set_parameter(param_id, value);
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_passthrough_get_parameter(node: *const RustPassthrough, param_id: u32) -> f32 {
    if let Some(p) = node.as_ref() {
        p.get_parameter(param_id)
    } else {
        0.0
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_passthrough_process(
    node: *mut RustPassthrough,
    in_l: *const f32,
    in_r: *const f32,
    out_l: *mut f32,
    out_r: *mut f32,
    num_samples: u32,
) {
    if node.is_null() || out_l.is_null() || num_samples == 0 {
        return;
    }
    let p = &mut *node;
    let n = num_samples as usize;

    let is_in_place = !in_l.is_null() && (std::ptr::eq(in_l, out_l as *const f32)
        || (!in_r.is_null() && !out_r.is_null() && std::ptr::eq(in_r, out_r as *const f32)));

    let is_mono_out = out_r.is_null() || std::ptr::eq(out_l, out_r);

    if is_in_place || in_l.is_null() {
        let mut scratch_in_l = [0.0f32; 1024];
        let mut scratch_in_r = [0.0f32; 1024];
        let mut scratch_out_r = [0.0f32; 1024];

        let mut offset = 0;
        while offset < n {
            let chunk_size = (n - offset).min(1024);
            if !in_l.is_null() {
                std::ptr::copy_nonoverlapping(in_l.add(offset), scratch_in_l.as_mut_ptr(), chunk_size);
                if !in_r.is_null() {
                    std::ptr::copy_nonoverlapping(in_r.add(offset), scratch_in_r.as_mut_ptr(), chunk_size);
                } else {
                    scratch_in_r[..chunk_size].copy_from_slice(&scratch_in_l[..chunk_size]);
                }
            } else {
                scratch_in_l[..chunk_size].fill(0.0);
                scratch_in_r[..chunk_size].fill(0.0);
            }

            let chunk_out_l = std::slice::from_raw_parts_mut(out_l.add(offset), chunk_size);

            if !is_mono_out {
                let chunk_out_r = std::slice::from_raw_parts_mut(out_r.add(offset), chunk_size);
                p.process(
                    &scratch_in_l[..chunk_size],
                    &scratch_in_r[..chunk_size],
                    chunk_out_l,
                    chunk_out_r,
                );
            } else {
                p.process(
                    &scratch_in_l[..chunk_size],
                    &scratch_in_r[..chunk_size],
                    chunk_out_l,
                    &mut scratch_out_r[..chunk_size],
                );
            }
            offset += chunk_size;
        }
        return;
    }

    let in_l_slice = std::slice::from_raw_parts(in_l, n);
    let in_r_slice = if !in_r.is_null() {
        std::slice::from_raw_parts(in_r, n)
    } else {
        in_l_slice
    };

    if !out_r.is_null() && !std::ptr::eq(out_l, out_r) {
        let out_l_slice = std::slice::from_raw_parts_mut(out_l, n);
        let out_r_slice = std::slice::from_raw_parts_mut(out_r, n);
        p.process(in_l_slice, in_r_slice, out_l_slice, out_r_slice);
    } else {
        let mut mono_scratch = [0.0f32; 1024];
        let mut offset = 0;
        let out_l_slice = std::slice::from_raw_parts_mut(out_l, n);
        while offset < n {
            let chunk_size = (n - offset).min(1024);
            let chunk_in_l = &in_l_slice[offset..offset + chunk_size];
            let chunk_in_r = &in_r_slice[offset..offset + chunk_size];
            let chunk_out_l = &mut out_l_slice[offset..offset + chunk_size];
            p.process(chunk_in_l, chunk_in_r, chunk_out_l, &mut mono_scratch[..chunk_size]);
            offset += chunk_size;
        }
    }
}

// ============================================================================
// ExternalSidechainNode C-ABI
// ============================================================================

#[no_mangle]
pub unsafe extern "C" fn n8_rust_external_sidechain_create() -> *mut ExternalSidechainCore {
    Box::into_raw(Box::new(ExternalSidechainCore::new()))
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_external_sidechain_free(ptr: *mut ExternalSidechainCore) {
    if !ptr.is_null() {
        drop(Box::from_raw(ptr));
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_external_sidechain_prepare(ptr: *mut ExternalSidechainCore, sample_rate: f32) {
    if let Some(core) = ptr.as_mut() {
        core.prepare(sample_rate);
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_external_sidechain_reset(ptr: *mut ExternalSidechainCore) {
    if let Some(core) = ptr.as_mut() {
        core.reset();
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_external_sidechain_set_parameters(
    ptr: *mut ExternalSidechainCore,
    gain_db: f32,
    mute: f32,
) {
    if let Some(core) = ptr.as_mut() {
        core.set_parameters(gain_db, mute);
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_external_sidechain_process(
    ptr: *mut ExternalSidechainCore,
    sc_l: *const f32,
    sc_r: *const f32,
    out_l: *mut f32,
    out_r: *mut f32,
    num_samples: u32,
) {
    if let Some(core) = ptr.as_mut() {
        let n = num_samples as usize;
        let sc_l_slice = if !sc_l.is_null() {
            Some(core::slice::from_raw_parts(sc_l, n))
        } else {
            None
        };
        let sc_r_slice = if !sc_r.is_null() {
            Some(core::slice::from_raw_parts(sc_r, n))
        } else {
            None
        };
        if !out_l.is_null() {
            let out_l_slice = core::slice::from_raw_parts_mut(out_l, n);
            if !out_r.is_null() && !std::ptr::eq(out_l, out_r) {
                let out_r_slice = core::slice::from_raw_parts_mut(out_r, n);
                core.process(sc_l_slice, sc_r_slice, out_l_slice, Some(out_r_slice), n);
            } else {
                core.process(sc_l_slice, sc_r_slice, out_l_slice, None, n);
            }
        }
    }
}

// ============================================================================
// MidiScaleQuantizerNode C-ABI
// ============================================================================

#[no_mangle]
pub unsafe extern "C" fn n8_rust_midi_scale_quantizer_create() -> *mut MidiScaleQuantizerCore {
    Box::into_raw(Box::new(MidiScaleQuantizerCore::new()))
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_midi_scale_quantizer_free(ptr: *mut MidiScaleQuantizerCore) {
    if !ptr.is_null() {
        drop(Box::from_raw(ptr));
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_midi_scale_quantizer_prepare(ptr: *mut MidiScaleQuantizerCore, sample_rate: f32) {
    if let Some(core) = ptr.as_mut() {
        core.prepare(sample_rate);
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_midi_scale_quantizer_reset(ptr: *mut MidiScaleQuantizerCore) {
    if let Some(core) = ptr.as_mut() {
        core.reset();
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_midi_scale_quantizer_set_parameters(
    ptr: *mut MidiScaleQuantizerCore,
    root_key: f32,
    scale: f32,
    transpose: f32,
    snap_speed: f32,
    mix: f32,
) {
    if let Some(core) = ptr.as_mut() {
        core.set_parameters(root_key, scale, transpose, snap_speed, mix);
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_midi_scale_quantizer_quantize_note(
    ptr: *const MidiScaleQuantizerCore,
    midi_note: i32,
) -> i32 {
    if let Some(core) = ptr.as_ref() {
        core.quantize_note(midi_note)
    } else {
        midi_note
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_midi_scale_quantizer_process(
    ptr: *mut MidiScaleQuantizerCore,
    in_l: *const f32,
    in_r: *const f32,
    out_l: *mut f32,
    out_r: *mut f32,
    num_samples: u32,
) {
    if let Some(core) = ptr.as_mut() {
        let n = num_samples as usize;
        if in_l.is_null() || out_l.is_null() || n == 0 {
            return;
        }
        let in_l_slice = core::slice::from_raw_parts(in_l, n);
        let in_r_slice = if !in_r.is_null() {
            core::slice::from_raw_parts(in_r, n)
        } else {
            in_l_slice
        };
        let out_l_slice = core::slice::from_raw_parts_mut(out_l, n);
        if !out_r.is_null() && !std::ptr::eq(out_l, out_r) {
            let out_r_slice = core::slice::from_raw_parts_mut(out_r, n);
            core.process(in_l_slice, in_r_slice, out_l_slice, Some(out_r_slice), n);
        } else {
            core.process(in_l_slice, in_r_slice, out_l_slice, None, n);
        }
    }
}

// ============================================================================
// MidiArpeggiatorNode C-ABI
// ============================================================================

#[no_mangle]
pub unsafe extern "C" fn n8_rust_midi_arpeggiator_create() -> *mut MidiArpeggiatorCore {
    Box::into_raw(Box::new(MidiArpeggiatorCore::new()))
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_midi_arpeggiator_free(ptr: *mut MidiArpeggiatorCore) {
    if !ptr.is_null() {
        drop(Box::from_raw(ptr));
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_midi_arpeggiator_prepare(ptr: *mut MidiArpeggiatorCore, sample_rate: f32) {
    if let Some(core) = ptr.as_mut() {
        core.prepare(sample_rate);
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_midi_arpeggiator_reset(ptr: *mut MidiArpeggiatorCore) {
    if let Some(core) = ptr.as_mut() {
        core.reset();
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_midi_arpeggiator_set_notes(
    ptr: *mut MidiArpeggiatorCore,
    notes: *const i32,
    count: usize,
) {
    if let Some(core) = ptr.as_mut() {
        if !notes.is_null() && count > 0 {
            let slice = core::slice::from_raw_parts(notes, count);
            core.set_notes(slice);
        } else {
            core.set_notes(&[]);
        }
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_midi_arpeggiator_set_parameters(
    ptr: *mut MidiArpeggiatorCore,
    rate: f32,
    pattern: f32,
    octaves: f32,
    gate: f32,
    swing: f32,
    synth_mix: f32,
) {
    if let Some(core) = ptr.as_mut() {
        core.set_parameters(rate, pattern, octaves, gate, swing, synth_mix);
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_midi_arpeggiator_process(
    ptr: *mut MidiArpeggiatorCore,
    in_l: *const f32,
    in_r: *const f32,
    out_l: *mut f32,
    out_r: *mut f32,
    num_samples: u32,
    ppq_position: f64,
    bpm: f64,
) {
    if let Some(core) = ptr.as_mut() {
        let n = num_samples as usize;
        if out_l.is_null() || n == 0 {
            return;
        }
        let in_l_slice = if !in_l.is_null() {
            Some(core::slice::from_raw_parts(in_l, n))
        } else {
            None
        };
        let in_r_slice = if !in_r.is_null() {
            Some(core::slice::from_raw_parts(in_r, n))
        } else {
            None
        };
        let out_l_slice = core::slice::from_raw_parts_mut(out_l, n);
        if !out_r.is_null() && !std::ptr::eq(out_l, out_r) {
            let out_r_slice = core::slice::from_raw_parts_mut(out_r, n);
            core.process(in_l_slice, in_r_slice, out_l_slice, Some(out_r_slice), n, ppq_position, bpm);
        } else {
            core.process(in_l_slice, in_r_slice, out_l_slice, None, n, ppq_position, bpm);
        }
    }
}

// ============================================================================
// MidiChordEngineNode C-ABI
// ============================================================================

#[no_mangle]
pub unsafe extern "C" fn n8_rust_midi_chord_engine_create() -> *mut MidiChordEngineCore {
    Box::into_raw(Box::new(MidiChordEngineCore::new()))
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_midi_chord_engine_free(ptr: *mut MidiChordEngineCore) {
    if !ptr.is_null() {
        drop(Box::from_raw(ptr));
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_midi_chord_engine_prepare(ptr: *mut MidiChordEngineCore, sample_rate: f32) {
    if let Some(core) = ptr.as_mut() {
        core.prepare(sample_rate);
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_midi_chord_engine_reset(ptr: *mut MidiChordEngineCore) {
    if let Some(core) = ptr.as_mut() {
        core.reset();
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_midi_chord_engine_trigger_chord(ptr: *mut MidiChordEngineCore) {
    if let Some(core) = ptr.as_mut() {
        core.trigger_chord();
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_midi_chord_engine_set_parameters(
    ptr: *mut MidiChordEngineCore,
    chord_type: f32,
    inversion: f32,
    strum_delay: f32,
    spread: f32,
    root_note: f32,
    synth_mix: f32,
) {
    if let Some(core) = ptr.as_mut() {
        core.set_parameters(chord_type, inversion, strum_delay, spread, root_note, synth_mix);
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_midi_chord_engine_process(
    ptr: *mut MidiChordEngineCore,
    in_l: *const f32,
    in_r: *const f32,
    out_l: *mut f32,
    out_r: *mut f32,
    num_samples: u32,
) {
    if let Some(core) = ptr.as_mut() {
        let n = num_samples as usize;
        if out_l.is_null() || n == 0 {
            return;
        }
        let in_l_slice = if !in_l.is_null() {
            Some(core::slice::from_raw_parts(in_l, n))
        } else {
            None
        };
        let in_r_slice = if !in_r.is_null() {
            Some(core::slice::from_raw_parts(in_r, n))
        } else {
            None
        };
        let out_l_slice = core::slice::from_raw_parts_mut(out_l, n);
        if !out_r.is_null() && !std::ptr::eq(out_l, out_r) {
            let out_r_slice = core::slice::from_raw_parts_mut(out_r, n);
            core.process(in_l_slice, in_r_slice, out_l_slice, Some(out_r_slice), n);
        } else {
            core.process(in_l_slice, in_r_slice, out_l_slice, None, n);
        }
    }
}

// ============================================================================
// C-ABI EXPORTS: FEEDBACK CONTAINER (DSP #53 / NodeType::Feedback)
// ============================================================================

#[no_mangle]
pub unsafe extern "C" fn n8_rust_feedback_container_create(sample_rate: f32) -> *mut FeedbackContainerCore {
    Box::into_raw(Box::new(FeedbackContainerCore::new(sample_rate)))
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_feedback_container_free(ptr: *mut FeedbackContainerCore) {
    if !ptr.is_null() {
        drop(Box::from_raw(ptr));
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_feedback_container_prepare(ptr: *mut FeedbackContainerCore, sample_rate: f32) {
    if let Some(core) = ptr.as_mut() {
        core.prepare(sample_rate);
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_feedback_container_reset(ptr: *mut FeedbackContainerCore) {
    if let Some(core) = ptr.as_mut() {
        core.reset();
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_feedback_container_set_parameter(ptr: *mut FeedbackContainerCore, param_id: u32, value: f32) {
    if let Some(core) = ptr.as_mut() {
        core.set_parameter(param_id, value);
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_feedback_container_get_parameter(ptr: *const FeedbackContainerCore, param_id: u32) -> f32 {
    if let Some(core) = ptr.as_ref() {
        core.get_parameter(param_id)
    } else {
        0.0
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_feedback_container_inject_forward(
    ptr: *mut FeedbackContainerCore,
    in_l: *const f32,
    in_r: *const f32,
    forward_l: *mut f32,
    forward_r: *mut f32,
    num_samples: u32,
) {
    if let Some(core) = ptr.as_mut() {
        let n = num_samples as usize;
        if n == 0 || forward_l.is_null() || forward_r.is_null() {
            return;
        }
        let in_l_slice = if !in_l.is_null() {
            Some(core::slice::from_raw_parts(in_l, n))
        } else {
            None
        };
        let in_r_slice = if !in_r.is_null() {
            Some(core::slice::from_raw_parts(in_r, n))
        } else {
            None
        };
        let fwd_l_slice = core::slice::from_raw_parts_mut(forward_l, n);
        let fwd_r_slice = core::slice::from_raw_parts_mut(forward_r, n);
        core.inject_forward(in_l_slice, in_r_slice, fwd_l_slice, fwd_r_slice, n);
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_feedback_container_process_return(
    ptr: *mut FeedbackContainerCore,
    in_l: *const f32,
    in_r: *const f32,
    wet_l: *const f32,
    wet_r: *const f32,
    out_l: *mut f32,
    out_r: *mut f32,
    num_samples: u32,
) {
    if let Some(core) = ptr.as_mut() {
        let n = num_samples as usize;
        if n == 0 || out_l.is_null() || wet_l.is_null() || wet_r.is_null() {
            return;
        }
        let in_l_slice = if !in_l.is_null() {
            Some(core::slice::from_raw_parts(in_l, n))
        } else {
            None
        };
        let in_r_slice = if !in_r.is_null() {
            Some(core::slice::from_raw_parts(in_r, n))
        } else {
            None
        };
        let wet_l_slice = core::slice::from_raw_parts(wet_l, n);
        let wet_r_slice = core::slice::from_raw_parts(wet_r, n);
        let out_l_slice = core::slice::from_raw_parts_mut(out_l, n);
        if !out_r.is_null() && !std::ptr::eq(out_l, out_r) {
            let out_r_slice = core::slice::from_raw_parts_mut(out_r, n);
            core.process_return(in_l_slice, in_r_slice, wet_l_slice, wet_r_slice, out_l_slice, Some(out_r_slice), n);
        } else {
            core.process_return(in_l_slice, in_r_slice, wet_l_slice, wet_r_slice, out_l_slice, None, n);
        }
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_feedback_container_process(
    ptr: *mut FeedbackContainerCore,
    in_l: *const f32,
    in_r: *const f32,
    out_l: *mut f32,
    out_r: *mut f32,
    num_samples: u32,
) {
    if let Some(core) = ptr.as_mut() {
        let n = num_samples as usize;
        if n == 0 || out_l.is_null() {
            return;
        }
        let in_l_slice = if !in_l.is_null() {
            Some(core::slice::from_raw_parts(in_l, n))
        } else {
            None
        };
        let in_r_slice = if !in_r.is_null() {
            Some(core::slice::from_raw_parts(in_r, n))
        } else {
            None
        };
        let out_l_slice = core::slice::from_raw_parts_mut(out_l, n);
        if !out_r.is_null() && !std::ptr::eq(out_l, out_r) {
            let out_r_slice = core::slice::from_raw_parts_mut(out_r, n);
            core.process(in_l_slice, in_r_slice, out_l_slice, Some(out_r_slice), n);
        } else {
            core.process(in_l_slice, in_r_slice, out_l_slice, None, n);
        }
    }
}

// ============================================================================
// C-ABI EXPORTS: TRANSIENT DETECTOR (Analysis Core)
// ============================================================================

#[no_mangle]
pub unsafe extern "C" fn n8_rust_transient_detector_create(sample_rate: f32) -> *mut TransientDetectorCore {
    Box::into_raw(Box::new(TransientDetectorCore::new(sample_rate)))
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_transient_detector_free(ptr: *mut TransientDetectorCore) {
    if !ptr.is_null() {
        drop(Box::from_raw(ptr));
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_transient_detector_prepare(ptr: *mut TransientDetectorCore, sample_rate: f32) {
    if let Some(core) = ptr.as_mut() {
        core.prepare(sample_rate);
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_transient_detector_reset(ptr: *mut TransientDetectorCore) {
    if let Some(core) = ptr.as_mut() {
        core.reset();
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_transient_detector_set_sensitivity(ptr: *mut TransientDetectorCore, sensitivity: f32) {
    if let Some(core) = ptr.as_mut() {
        core.set_sensitivity(sensitivity);
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_transient_detector_get_sensitivity(ptr: *const TransientDetectorCore) -> f32 {
    if let Some(core) = ptr.as_ref() {
        core.get_sensitivity()
    } else {
        0.0
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_transient_detector_is_transient_detected(ptr: *const TransientDetectorCore) -> bool {
    if let Some(core) = ptr.as_ref() {
        core.is_transient_detected()
    } else {
        false
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_transient_detector_get_onset_strength(ptr: *const TransientDetectorCore) -> f32 {
    if let Some(core) = ptr.as_ref() {
        core.get_onset_strength()
    } else {
        0.0
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_transient_detector_process(
    ptr: *mut TransientDetectorCore,
    input: *const f32,
    num_samples: u32,
) {
    if let Some(core) = ptr.as_mut() {
        let n = num_samples as usize;
        let input_slice = if !input.is_null() && n > 0 {
            Some(core::slice::from_raw_parts(input, n))
        } else {
            None
        };
        core.process(input_slice, n);
    }
}

// ============================================================================
// C-ABI EXPORTS: PITCH TRACKER (Analysis Core)
// ============================================================================

#[no_mangle]
pub unsafe extern "C" fn n8_rust_pitch_tracker_create(sample_rate: f32) -> *mut PitchTrackerCore {
    Box::into_raw(Box::new(PitchTrackerCore::new(sample_rate)))
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_pitch_tracker_free(ptr: *mut PitchTrackerCore) {
    if !ptr.is_null() {
        drop(Box::from_raw(ptr));
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_pitch_tracker_prepare(ptr: *mut PitchTrackerCore, sample_rate: f32) {
    if let Some(core) = ptr.as_mut() {
        core.prepare(sample_rate);
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_pitch_tracker_reset(ptr: *mut PitchTrackerCore) {
    if let Some(core) = ptr.as_mut() {
        core.reset();
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_pitch_tracker_get_fundamental_hz(ptr: *const PitchTrackerCore) -> f32 {
    if let Some(core) = ptr.as_ref() {
        core.get_fundamental_hz()
    } else {
        440.0
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_pitch_tracker_get_clarity(ptr: *const PitchTrackerCore) -> f32 {
    if let Some(core) = ptr.as_ref() {
        core.get_clarity()
    } else {
        0.0
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_pitch_tracker_get_midi_note(ptr: *const PitchTrackerCore) -> f32 {
    if let Some(core) = ptr.as_ref() {
        core.get_midi_note()
    } else {
        69.0
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_pitch_tracker_get_pitch_normalized(ptr: *const PitchTrackerCore) -> f32 {
    if let Some(core) = ptr.as_ref() {
        core.get_pitch_normalized()
    } else {
        0.5
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_pitch_tracker_process(
    ptr: *mut PitchTrackerCore,
    input: *const f32,
    num_samples: u32,
) {
    if let Some(core) = ptr.as_mut() {
        let n = num_samples as usize;
        let input_slice = if !input.is_null() && n > 0 {
            Some(core::slice::from_raw_parts(input, n))
        } else {
            None
        };
        core.process(input_slice, n);
    }
}

// =========================================================================
// SpectralFeatureExtractor C-ABI
// =========================================================================

#[no_mangle]
pub unsafe extern "C" fn n8_rust_spectral_feature_extractor_create() -> *mut RustSpectralFeatureExtractor {
    Box::into_raw(Box::new(RustSpectralFeatureExtractor::new()))
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_spectral_feature_extractor_destroy(ptr: *mut RustSpectralFeatureExtractor) {
    if !ptr.is_null() {
        drop(Box::from_raw(ptr));
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_spectral_feature_extractor_prepare(
    ptr: *mut RustSpectralFeatureExtractor,
    sample_rate: f64,
    fft_size: usize,
    hop_size: usize,
) {
    if let Some(ext) = ptr.as_mut() {
        ext.prepare(sample_rate, fft_size, hop_size);
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_spectral_feature_extractor_reset(ptr: *mut RustSpectralFeatureExtractor) {
    if let Some(ext) = ptr.as_mut() {
        ext.reset();
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_spectral_feature_extractor_process(
    ptr: *mut RustSpectralFeatureExtractor,
    input: *const f32,
    num_samples: u32,
    is_silent: bool,
) {
    if let Some(ext) = ptr.as_mut() {
        let n = num_samples as usize;
        if !input.is_null() && n > 0 {
            let slice = core::slice::from_raw_parts(input, n);
            ext.process(slice, is_silent);
        } else {
            ext.process(&[], is_silent);
        }
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_spectral_feature_extractor_get_centroid(ptr: *const RustSpectralFeatureExtractor) -> f32 {
    if let Some(ext) = ptr.as_ref() {
        ext.get_spectral_centroid()
    } else {
        0.2
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_spectral_feature_extractor_get_flux(ptr: *const RustSpectralFeatureExtractor) -> f32 {
    if let Some(ext) = ptr.as_ref() {
        ext.get_spectral_flux()
    } else {
        0.0
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_spectral_feature_extractor_get_flatness(ptr: *const RustSpectralFeatureExtractor) -> f32 {
    if let Some(ext) = ptr.as_ref() {
        ext.get_spectral_flatness()
    } else {
        0.0
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_spectral_feature_extractor_get_energy(ptr: *const RustSpectralFeatureExtractor) -> f32 {
    if let Some(ext) = ptr.as_ref() {
        ext.get_energy()
    } else {
        0.0
    }
}

// =========================================================================
// AnalysisEngine C-ABI
// =========================================================================

#[no_mangle]
pub unsafe extern "C" fn n8_rust_analysis_engine_create(sample_rate: f64, max_block_size: usize) -> *mut RustAnalysisEngine {
    Box::into_raw(Box::new(RustAnalysisEngine::new(sample_rate, max_block_size)))
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_analysis_engine_destroy(ptr: *mut RustAnalysisEngine) {
    if !ptr.is_null() {
        drop(Box::from_raw(ptr));
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_analysis_engine_prepare(
    ptr: *mut RustAnalysisEngine,
    sample_rate: f64,
    max_block_size: usize,
) {
    if let Some(engine) = ptr.as_mut() {
        engine.prepare(sample_rate, max_block_size);
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_analysis_engine_reset(ptr: *mut RustAnalysisEngine) {
    if let Some(engine) = ptr.as_mut() {
        engine.reset();
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_analysis_engine_process(
    ptr: *mut RustAnalysisEngine,
    input_channels: *const *const f32,
    num_channels: u32,
    num_samples: u32,
) {
    if let Some(engine) = ptr.as_mut() {
        engine.process(input_channels, num_channels, num_samples);
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_analysis_engine_get_snapshot(
    ptr: *const RustAnalysisEngine,
    out: *mut RustAnalysisSnapshot,
) {
    if let (Some(engine), Some(dst)) = (ptr.as_ref(), out.as_mut()) {
        *dst = engine.get_snapshot();
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_analysis_engine_get_transient_detector(
    ptr: *mut RustAnalysisEngine,
) -> *mut TransientDetectorCore {
    if let Some(engine) = ptr.as_mut() {
        &mut engine.transient_detector as *mut TransientDetectorCore
    } else {
        std::ptr::null_mut()
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_analysis_engine_get_pitch_tracker(
    ptr: *mut RustAnalysisEngine,
) -> *mut PitchTrackerCore {
    if let Some(engine) = ptr.as_mut() {
        &mut engine.pitch_tracker as *mut PitchTrackerCore
    } else {
        std::ptr::null_mut()
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_analysis_engine_get_spectral_extractor(
    ptr: *mut RustAnalysisEngine,
) -> *mut RustSpectralFeatureExtractor {
    if let Some(engine) = ptr.as_mut() {
        &mut engine.spectral_extractor as *mut RustSpectralFeatureExtractor
    } else {
        std::ptr::null_mut()
    }
}

// =========================================================================
// Modulation Core C-ABI (Rules 7, 8, 25, 37, 46, 47)
// =========================================================================

// --- LFO ---
#[no_mangle]
pub unsafe extern "C" fn n8_rust_lfo_create() -> *mut RustLFO {
    Box::into_raw(Box::new(RustLFO::new()))
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_lfo_destroy(ptr: *mut RustLFO) {
    if !ptr.is_null() {
        drop(Box::from_raw(ptr));
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_lfo_prepare(ptr: *mut RustLFO, sample_rate: f64) {
    if let Some(lfo) = ptr.as_mut() {
        lfo.prepare(sample_rate);
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_lfo_reset(ptr: *mut RustLFO) {
    if let Some(lfo) = ptr.as_mut() {
        lfo.reset();
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_lfo_set_shape(ptr: *mut RustLFO, shape: u8) {
    if let Some(lfo) = ptr.as_mut() {
        let s = match shape {
            0 => LFOShape::Sine,
            1 => LFOShape::Triangle,
            2 => LFOShape::SawUp,
            3 => LFOShape::SawDown,
            4 => LFOShape::Square,
            5 => LFOShape::Pulse,
            6 => LFOShape::SampleAndHold,
            7 => LFOShape::SmoothRandom,
            _ => LFOShape::Sine,
        };
        lfo.set_shape(s);
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_lfo_set_sync_division(ptr: *mut RustLFO, div: u8) {
    if let Some(lfo) = ptr.as_mut() {
        let d = match div {
            0 => SyncDivision::FreeHz,
            1 => SyncDivision::Bar1,
            2 => SyncDivision::Half,
            3 => SyncDivision::Quarter,
            4 => SyncDivision::Eighth,
            5 => SyncDivision::Sixteenth,
            6 => SyncDivision::ThirtySecond,
            7 => SyncDivision::TripletQuarter,
            8 => SyncDivision::TripletEighth,
            9 => SyncDivision::DottedEighth,
            10 => SyncDivision::DottedQuarter,
            _ => SyncDivision::FreeHz,
        };
        lfo.set_sync_division(d);
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_lfo_set_frequency_hz(ptr: *mut RustLFO, freq: f32) {
    if let Some(lfo) = ptr.as_mut() {
        lfo.set_frequency_hz(freq);
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_lfo_set_pulse_width(ptr: *mut RustLFO, pw: f32) {
    if let Some(lfo) = ptr.as_mut() {
        lfo.set_pulse_width(pw);
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_lfo_process_sample(
    ptr: *mut RustLFO,
    bpm: f64,
    ppq: f64,
    is_playing: bool,
) -> f32 {
    if let Some(lfo) = ptr.as_mut() {
        lfo.process_sample(bpm, ppq, is_playing)
    } else {
        0.0
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_lfo_get_current_value(ptr: *const RustLFO) -> f32 {
    if let Some(lfo) = ptr.as_ref() {
        lfo.get_current_value()
    } else {
        0.0
    }
}

// --- EnvelopeGenerator ---
#[no_mangle]
pub unsafe extern "C" fn n8_rust_envelope_create() -> *mut RustEnvelopeGenerator {
    Box::into_raw(Box::new(RustEnvelopeGenerator::new()))
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_envelope_destroy(ptr: *mut RustEnvelopeGenerator) {
    if !ptr.is_null() {
        drop(Box::from_raw(ptr));
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_envelope_prepare(ptr: *mut RustEnvelopeGenerator, sample_rate: f64) {
    if let Some(env) = ptr.as_mut() {
        env.prepare(sample_rate);
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_envelope_reset(ptr: *mut RustEnvelopeGenerator) {
    if let Some(env) = ptr.as_mut() {
        env.reset();
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_envelope_set_attack_ms(ptr: *mut RustEnvelopeGenerator, ms: f32) {
    if let Some(env) = ptr.as_mut() {
        env.set_attack_ms(ms);
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_envelope_set_decay_ms(ptr: *mut RustEnvelopeGenerator, ms: f32) {
    if let Some(env) = ptr.as_mut() {
        env.set_decay_ms(ms);
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_envelope_set_sustain_level(ptr: *mut RustEnvelopeGenerator, lvl: f32) {
    if let Some(env) = ptr.as_mut() {
        env.set_sustain_level(lvl);
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_envelope_set_release_ms(ptr: *mut RustEnvelopeGenerator, ms: f32) {
    if let Some(env) = ptr.as_mut() {
        env.set_release_ms(ms);
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_envelope_trigger_gate(ptr: *mut RustEnvelopeGenerator, gate_on: bool) {
    if let Some(env) = ptr.as_mut() {
        env.trigger_gate(gate_on);
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_envelope_process_sample(ptr: *mut RustEnvelopeGenerator) -> f32 {
    if let Some(env) = ptr.as_mut() {
        env.process_sample()
    } else {
        0.0
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_envelope_get_current_value(ptr: *const RustEnvelopeGenerator) -> f32 {
    if let Some(env) = ptr.as_ref() {
        env.get_current_value()
    } else {
        0.0
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_envelope_get_state(ptr: *const RustEnvelopeGenerator) -> u8 {
    if let Some(env) = ptr.as_ref() {
        env.get_state() as u8
    } else {
        0
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_envelope_is_active(ptr: *const RustEnvelopeGenerator) -> bool {
    if let Some(env) = ptr.as_ref() {
        env.is_active()
    } else {
        false
    }
}

// --- AudioFollower ---
#[no_mangle]
pub unsafe extern "C" fn n8_rust_audio_follower_create() -> *mut RustAudioFollower {
    Box::into_raw(Box::new(RustAudioFollower::new()))
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_audio_follower_destroy(ptr: *mut RustAudioFollower) {
    if !ptr.is_null() {
        drop(Box::from_raw(ptr));
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_audio_follower_prepare(ptr: *mut RustAudioFollower, sample_rate: f64) {
    if let Some(fol) = ptr.as_mut() {
        fol.prepare(sample_rate);
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_audio_follower_reset(ptr: *mut RustAudioFollower) {
    if let Some(fol) = ptr.as_mut() {
        fol.reset();
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_audio_follower_set_attack_ms(ptr: *mut RustAudioFollower, ms: f32) {
    if let Some(fol) = ptr.as_mut() {
        fol.set_attack_ms(ms);
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_audio_follower_set_release_ms(ptr: *mut RustAudioFollower, ms: f32) {
    if let Some(fol) = ptr.as_mut() {
        fol.set_release_ms(ms);
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_audio_follower_process_block(
    ptr: *mut RustAudioFollower,
    input_channels: *const *const f32,
    num_channels: u32,
    num_samples: u32,
) -> f32 {
    if let Some(fol) = ptr.as_mut() {
        fol.process_block(input_channels, num_channels, num_samples)
    } else {
        0.0
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_audio_follower_get_current_value(ptr: *const RustAudioFollower) -> f32 {
    if let Some(fol) = ptr.as_ref() {
        fol.get_current_value()
    } else {
        0.0
    }
}

// --- ChaosModulator ---
#[no_mangle]
pub unsafe extern "C" fn n8_rust_chaos_modulator_create() -> *mut RustChaosModulator {
    Box::into_raw(Box::new(RustChaosModulator::new()))
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_chaos_modulator_destroy(ptr: *mut RustChaosModulator) {
    if !ptr.is_null() {
        drop(Box::from_raw(ptr));
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_chaos_modulator_prepare(ptr: *mut RustChaosModulator, sample_rate: f64) {
    if let Some(chaos) = ptr.as_mut() {
        chaos.prepare(sample_rate);
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_chaos_modulator_reset(ptr: *mut RustChaosModulator) {
    if let Some(chaos) = ptr.as_mut() {
        chaos.reset();
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_chaos_modulator_set_speed(ptr: *mut RustChaosModulator, speed: f32) {
    if let Some(chaos) = ptr.as_mut() {
        chaos.set_speed(speed);
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_chaos_modulator_process_sample(ptr: *mut RustChaosModulator) {
    if let Some(chaos) = ptr.as_mut() {
        chaos.process_sample();
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_chaos_modulator_get_normalized_x(ptr: *const RustChaosModulator) -> f32 {
    if let Some(chaos) = ptr.as_ref() {
        chaos.get_normalized_x()
    } else {
        0.0
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_chaos_modulator_get_normalized_y(ptr: *const RustChaosModulator) -> f32 {
    if let Some(chaos) = ptr.as_ref() {
        chaos.get_normalized_y()
    } else {
        0.0
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_chaos_modulator_get_normalized_z(ptr: *const RustChaosModulator) -> f32 {
    if let Some(chaos) = ptr.as_ref() {
        chaos.get_normalized_z()
    } else {
        0.0
    }
}

// --- StepSequencer ---
#[no_mangle]
pub unsafe extern "C" fn n8_rust_step_sequencer_create() -> *mut RustStepSequencer {
    Box::into_raw(Box::new(RustStepSequencer::new()))
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_step_sequencer_destroy(ptr: *mut RustStepSequencer) {
    if !ptr.is_null() {
        drop(Box::from_raw(ptr));
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_step_sequencer_prepare(ptr: *mut RustStepSequencer, sample_rate: f64) {
    if let Some(seq) = ptr.as_mut() {
        seq.prepare(sample_rate);
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_step_sequencer_reset(ptr: *mut RustStepSequencer) {
    if let Some(seq) = ptr.as_mut() {
        seq.reset();
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_step_sequencer_set_num_steps(ptr: *mut RustStepSequencer, count: u32) {
    if let Some(seq) = ptr.as_mut() {
        seq.set_num_steps(count);
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_step_sequencer_set_step(
    ptr: *mut RustStepSequencer,
    index: u32,
    value: f32,
    probability: f32,
    glide: f32,
    active: bool,
) {
    if let Some(seq) = ptr.as_mut() {
        seq.set_step(index, value, probability, glide, active);
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_step_sequencer_get_step(
    ptr: *const RustStepSequencer,
    index: u32,
    out: *mut SequencerStep,
) {
    if let (Some(seq), Some(dst)) = (ptr.as_ref(), out.as_mut()) {
        *dst = seq.get_step(index);
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_step_sequencer_process_sample(
    ptr: *mut RustStepSequencer,
    ppq: f64,
    is_playing: bool,
) -> f32 {
    if let Some(seq) = ptr.as_mut() {
        seq.process_sample(ppq, is_playing)
    } else {
        0.0
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_step_sequencer_get_current_value(ptr: *const RustStepSequencer) -> f32 {
    if let Some(seq) = ptr.as_ref() {
        seq.get_current_value()
    } else {
        0.0
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_step_sequencer_get_current_step(ptr: *const RustStepSequencer) -> u32 {
    if let Some(seq) = ptr.as_ref() {
        seq.get_current_step()
    } else {
        0
    }
}

// --- EuclideanModulator ---
#[no_mangle]
pub unsafe extern "C" fn n8_rust_euclidean_modulator_create() -> *mut RustEuclideanModulator {
    Box::into_raw(Box::new(RustEuclideanModulator::new()))
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_euclidean_modulator_destroy(ptr: *mut RustEuclideanModulator) {
    if !ptr.is_null() {
        drop(Box::from_raw(ptr));
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_euclidean_modulator_prepare(ptr: *mut RustEuclideanModulator, sample_rate: f64) {
    if let Some(euc) = ptr.as_mut() {
        euc.prepare(sample_rate);
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_euclidean_modulator_reset(ptr: *mut RustEuclideanModulator) {
    if let Some(euc) = ptr.as_mut() {
        euc.reset();
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_euclidean_modulator_set_steps(ptr: *mut RustEuclideanModulator, steps: i32) {
    if let Some(euc) = ptr.as_mut() {
        euc.set_steps(steps);
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_euclidean_modulator_set_pulses(ptr: *mut RustEuclideanModulator, pulses: i32) {
    if let Some(euc) = ptr.as_mut() {
        euc.set_pulses(pulses);
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_euclidean_modulator_set_rotation(ptr: *mut RustEuclideanModulator, rotation: i32) {
    if let Some(euc) = ptr.as_mut() {
        euc.set_rotation(rotation);
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_euclidean_modulator_set_decay(ptr: *mut RustEuclideanModulator, seconds: f32) {
    if let Some(euc) = ptr.as_mut() {
        euc.set_decay(seconds);
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_euclidean_modulator_set_sync(ptr: *mut RustEuclideanModulator, div: u8, free_bpm: f32) {
    if let Some(euc) = ptr.as_mut() {
        let d = match div {
            0 => SyncDivision::FreeHz,
            3 => SyncDivision::Quarter,
            4 => SyncDivision::Eighth,
            5 => SyncDivision::Sixteenth,
            6 => SyncDivision::ThirtySecond,
            8 => SyncDivision::TripletEighth,
            _ => SyncDivision::Sixteenth,
        };
        euc.set_sync(d, free_bpm);
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_euclidean_modulator_has_pulse_at_step(ptr: *const RustEuclideanModulator, step: i32) -> bool {
    if let Some(euc) = ptr.as_ref() {
        euc.has_pulse_at_step(step)
    } else {
        false
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_euclidean_modulator_process_sample(
    ptr: *mut RustEuclideanModulator,
    bpm: f64,
    ppq: f64,
    is_playing: bool,
) {
    if let Some(euc) = ptr.as_mut() {
        euc.process_sample(bpm, ppq, is_playing);
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_euclidean_modulator_get_current_value(ptr: *const RustEuclideanModulator) -> f32 {
    if let Some(euc) = ptr.as_ref() {
        euc.get_current_value()
    } else {
        0.0
    }
}

// --- MSEGModulator ---
#[no_mangle]
pub unsafe extern "C" fn n8_rust_mseg_modulator_create() -> *mut RustMSEGModulator {
    Box::into_raw(Box::new(RustMSEGModulator::new()))
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_mseg_modulator_destroy(ptr: *mut RustMSEGModulator) {
    if !ptr.is_null() {
        drop(Box::from_raw(ptr));
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_mseg_modulator_prepare(ptr: *mut RustMSEGModulator, sample_rate: f64) {
    if let Some(mseg) = ptr.as_mut() {
        mseg.prepare(sample_rate);
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_mseg_modulator_reset(ptr: *mut RustMSEGModulator) {
    if let Some(mseg) = ptr.as_mut() {
        mseg.reset();
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_mseg_modulator_set_default_envelope(ptr: *mut RustMSEGModulator) {
    if let Some(mseg) = ptr.as_mut() {
        mseg.set_default_envelope();
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_mseg_modulator_set_point(
    ptr: *mut RustMSEGModulator,
    index: usize,
    time: f32,
    level: f32,
    curve: f32,
) {
    if let Some(mseg) = ptr.as_mut() {
        mseg.set_point(index, time, level, curve);
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_mseg_modulator_set_num_points(ptr: *mut RustMSEGModulator, count: usize) {
    if let Some(mseg) = ptr.as_mut() {
        mseg.set_num_points(count);
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_mseg_modulator_set_mode(ptr: *mut RustMSEGModulator, mode: u8) {
    if let Some(mseg) = ptr.as_mut() {
        let m = match mode {
            0 => MSEGPlayMode::Loop,
            1 => MSEGPlayMode::OneShot,
            2 => MSEGPlayMode::PingPong,
            _ => MSEGPlayMode::Loop,
        };
        mseg.set_mode(m);
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_mseg_modulator_set_sync(ptr: *mut RustMSEGModulator, div: u8, free_hz: f32) {
    if let Some(mseg) = ptr.as_mut() {
        let d = match div {
            0 => SyncDivision::FreeHz,
            1 => SyncDivision::Bar1,
            2 => SyncDivision::Half,
            3 => SyncDivision::Quarter,
            4 => SyncDivision::Eighth,
            5 => SyncDivision::Sixteenth,
            6 => SyncDivision::ThirtySecond,
            7 => SyncDivision::TripletQuarter,
            8 => SyncDivision::TripletEighth,
            _ => SyncDivision::Bar1,
        };
        mseg.set_sync(d, free_hz);
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_mseg_modulator_trigger(ptr: *mut RustMSEGModulator) {
    if let Some(mseg) = ptr.as_mut() {
        mseg.trigger();
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_mseg_modulator_process_sample(
    ptr: *mut RustMSEGModulator,
    bpm: f64,
    ppq: f64,
    is_playing: bool,
) {
    if let Some(mseg) = ptr.as_mut() {
        mseg.process_sample(bpm, ppq, is_playing);
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_mseg_modulator_get_current_value(ptr: *const RustMSEGModulator) -> f32 {
    if let Some(mseg) = ptr.as_ref() {
        mseg.get_current_value()
    } else {
        0.0
    }
}

// --- MacroManager ---
#[no_mangle]
pub unsafe extern "C" fn n8_rust_macro_manager_create() -> *mut RustMacroManager {
    Box::into_raw(Box::new(RustMacroManager::new()))
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_macro_manager_destroy(ptr: *mut RustMacroManager) {
    if !ptr.is_null() {
        drop(Box::from_raw(ptr));
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_macro_manager_reset(ptr: *mut RustMacroManager) {
    if let Some(mm) = ptr.as_mut() {
        mm.reset();
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_macro_manager_set_macro(ptr: *mut RustMacroManager, idx: usize, val: f32) {
    if let Some(mm) = ptr.as_mut() {
        mm.set_macro(idx, val);
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_macro_manager_get_macro(ptr: *const RustMacroManager, idx: usize) -> f32 {
    if let Some(mm) = ptr.as_ref() {
        mm.get_macro(idx)
    } else {
        0.0
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_macro_manager_get_smoothed_macro(ptr: *const RustMacroManager, idx: usize) -> f32 {
    if let Some(mm) = ptr.as_ref() {
        mm.get_smoothed_macro(idx)
    } else {
        0.0
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_macro_manager_set_pad(ptr: *mut RustMacroManager, x: f32, y: f32) {
    if let Some(mm) = ptr.as_mut() {
        mm.set_pad(x, y);
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_macro_manager_get_pad_x(ptr: *const RustMacroManager) -> f32 {
    if let Some(mm) = ptr.as_ref() {
        mm.get_pad_x()
    } else {
        0.0
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_macro_manager_get_pad_y(ptr: *const RustMacroManager) -> f32 {
    if let Some(mm) = ptr.as_ref() {
        mm.get_pad_y()
    } else {
        0.0
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_macro_manager_get_smoothed_pad_x(ptr: *const RustMacroManager) -> f32 {
    if let Some(mm) = ptr.as_ref() {
        mm.get_smoothed_pad_x()
    } else {
        0.0
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_macro_manager_get_smoothed_pad_y(ptr: *const RustMacroManager) -> f32 {
    if let Some(mm) = ptr.as_ref() {
        mm.get_smoothed_pad_y()
    } else {
        0.0
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_macro_manager_process_smoothing(ptr: *mut RustMacroManager) {
    if let Some(mm) = ptr.as_mut() {
        mm.process_smoothing();
    }
}

// --- ModulationMatrix ---
#[no_mangle]
pub unsafe extern "C" fn n8_rust_modulation_matrix_create() -> *mut RustModulationMatrix {
    Box::into_raw(Box::new(RustModulationMatrix::new()))
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_modulation_matrix_destroy(ptr: *mut RustModulationMatrix) {
    if !ptr.is_null() {
        drop(Box::from_raw(ptr));
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_modulation_matrix_reset(ptr: *mut RustModulationMatrix) {
    if let Some(mat) = ptr.as_mut() {
        mat.reset();
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_modulation_matrix_add_route(
    ptr: *mut RustModulationMatrix,
    source: u16,
    target_node: u32,
    target_param: u32,
    amount: f32,
    bipolar: bool,
) -> i32 {
    if let Some(mat) = ptr.as_mut() {
        mat.add_route(source, target_node, target_param, amount, bipolar)
    } else {
        -1
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_modulation_matrix_remove_route(ptr: *mut RustModulationMatrix, index: usize) {
    if let Some(mat) = ptr.as_mut() {
        mat.remove_route(index);
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_modulation_matrix_set_route_amount(
    ptr: *mut RustModulationMatrix,
    index: usize,
    amount: f32,
) {
    if let Some(mat) = ptr.as_mut() {
        mat.set_route_amount(index, amount);
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_modulation_matrix_get_active_route_count(ptr: *const RustModulationMatrix) -> usize {
    if let Some(mat) = ptr.as_ref() {
        mat.get_active_route_count()
    } else {
        0
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_modulation_matrix_get_route(
    ptr: *const RustModulationMatrix,
    index: usize,
    out: *mut RustModulationRoute,
) {
    if let (Some(mat), Some(dst)) = (ptr.as_ref(), out.as_mut()) {
        *dst = mat.get_route(index);
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_modulation_matrix_calculate_offset(
    ptr: *const RustModulationMatrix,
    target_node: u32,
    target_param: u32,
    source_values: *const f32,
    num_sources: usize,
) -> f32 {
    if let Some(mat) = ptr.as_ref() {
        if !source_values.is_null() && num_sources > 0 {
            let slice = std::slice::from_raw_parts(source_values, num_sources);
            mat.calculate_modulation_offset(target_node, target_param, slice)
        } else {
            0.0
        }
    } else {
        0.0
    }
}

// --- ModulationEngine ---
#[no_mangle]
pub unsafe extern "C" fn n8_rust_modulation_engine_create() -> *mut RustModulationEngine {
    Box::into_raw(Box::new(RustModulationEngine::new()))
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_modulation_engine_destroy(ptr: *mut RustModulationEngine) {
    if !ptr.is_null() {
        drop(Box::from_raw(ptr));
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_modulation_engine_prepare(ptr: *mut RustModulationEngine, sample_rate: f64) {
    if let Some(eng) = ptr.as_mut() {
        eng.prepare(sample_rate);
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_modulation_engine_reset(ptr: *mut RustModulationEngine) {
    if let Some(eng) = ptr.as_mut() {
        eng.reset();
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_modulation_engine_process_sources(
    ptr: *mut RustModulationEngine,
    bpm: f64,
    ppq: f64,
    is_playing: bool,
    input_channels: *const *const f32,
    num_channels: u32,
    num_samples: u32,
) {
    if let Some(eng) = ptr.as_mut() {
        eng.process_sources(bpm, ppq, is_playing, input_channels, num_channels, num_samples);
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_modulation_engine_calculate_offset(
    ptr: *const RustModulationEngine,
    target_node: u32,
    target_param: u32,
) -> f32 {
    if let Some(eng) = ptr.as_ref() {
        eng.calculate_modulation_offset(target_node, target_param)
    } else {
        0.0
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_modulation_engine_update_analysis_sources(
    ptr: *mut RustModulationEngine,
    transient: f32,
    pitch_norm: f32,
    centroid: f32,
    flux: f32,
) {
    if let Some(eng) = ptr.as_mut() {
        eng.update_analysis_sources(transient, pitch_norm, centroid, flux);
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_modulation_engine_get_source_value(
    ptr: *const RustModulationEngine,
    source_type: u16,
) -> f32 {
    if let Some(eng) = ptr.as_ref() {
        eng.get_source_value(source_type)
    } else {
        0.0
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_modulation_engine_get_lfo(
    ptr: *mut RustModulationEngine,
    index: usize,
) -> *mut RustLFO {
    if let Some(eng) = ptr.as_mut() {
        let idx = index.min(3);
        &mut eng.lfos[idx] as *mut RustLFO
    } else {
        std::ptr::null_mut()
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_modulation_engine_get_envelope(
    ptr: *mut RustModulationEngine,
    index: usize,
) -> *mut RustEnvelopeGenerator {
    if let Some(eng) = ptr.as_mut() {
        let idx = index.min(1);
        &mut eng.envelopes[idx] as *mut RustEnvelopeGenerator
    } else {
        std::ptr::null_mut()
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_modulation_engine_get_step_sequencer(
    ptr: *mut RustModulationEngine,
) -> *mut RustStepSequencer {
    if let Some(eng) = ptr.as_mut() {
        &mut eng.step_seq as *mut RustStepSequencer
    } else {
        std::ptr::null_mut()
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_modulation_engine_get_audio_follower(
    ptr: *mut RustModulationEngine,
) -> *mut RustAudioFollower {
    if let Some(eng) = ptr.as_mut() {
        &mut eng.audio_follower as *mut RustAudioFollower
    } else {
        std::ptr::null_mut()
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_modulation_engine_get_macro_manager(
    ptr: *mut RustModulationEngine,
) -> *mut RustMacroManager {
    if let Some(eng) = ptr.as_mut() {
        &mut eng.macro_manager as *mut RustMacroManager
    } else {
        std::ptr::null_mut()
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_modulation_engine_get_matrix(
    ptr: *mut RustModulationEngine,
) -> *mut RustModulationMatrix {
    if let Some(eng) = ptr.as_mut() {
        &mut eng.matrix as *mut RustModulationMatrix
    } else {
        std::ptr::null_mut()
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_modulation_engine_get_mseg(
    ptr: *mut RustModulationEngine,
    index: usize,
) -> *mut RustMSEGModulator {
    if let Some(eng) = ptr.as_mut() {
        let idx = index.min(1);
        &mut eng.msegs[idx] as *mut RustMSEGModulator
    } else {
        std::ptr::null_mut()
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_modulation_engine_get_euclidean(
    ptr: *mut RustModulationEngine,
    index: usize,
) -> *mut RustEuclideanModulator {
    if let Some(eng) = ptr.as_mut() {
        let idx = index.min(1);
        &mut eng.euclideans[idx] as *mut RustEuclideanModulator
    } else {
        std::ptr::null_mut()
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_modulation_engine_get_chaos(
    ptr: *mut RustModulationEngine,
) -> *mut RustChaosModulator {
    if let Some(eng) = ptr.as_mut() {
        &mut eng.chaos as *mut RustChaosModulator
    } else {
        std::ptr::null_mut()
    }
}

// ============================================================================
// C-ABI: EVENT ENGINE (Roadmap Phase 2)
// ============================================================================

// ----------------------------------------------------------------------------
// RustEventCaptureBuffer
// ----------------------------------------------------------------------------
#[no_mangle]
pub unsafe extern "C" fn n8_rust_event_capture_buffer_create() -> *mut RustEventCaptureBuffer {
    Box::into_raw(Box::new(RustEventCaptureBuffer::new()))
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_event_capture_buffer_destroy(ptr: *mut RustEventCaptureBuffer) {
    if !ptr.is_null() {
        drop(Box::from_raw(ptr));
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_event_capture_buffer_prepare(
    ptr: *mut RustEventCaptureBuffer,
    sample_rate: f64,
    max_history_seconds: f64,
    num_channels: usize,
) {
    if let Some(buf) = ptr.as_mut() {
        buf.prepare(sample_rate, max_history_seconds, num_channels);
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_event_capture_buffer_reset(ptr: *mut RustEventCaptureBuffer) {
    if let Some(buf) = ptr.as_mut() {
        buf.reset();
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_event_capture_buffer_write(
    ptr: *mut RustEventCaptureBuffer,
    in_l: *const f32,
    in_r: *const f32,
    num_samples: usize,
) {
    if let Some(buf) = ptr.as_mut() {
        if !in_l.is_null() && num_samples > 0 {
            let slice_l = std::slice::from_raw_parts(in_l, num_samples);
            let slice_r = if !in_r.is_null() {
                std::slice::from_raw_parts(in_r, num_samples)
            } else {
                slice_l
            };
            buf.write_channels(slice_l, slice_r, num_samples);
        }
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_event_capture_buffer_read_sample(
    ptr: *const RustEventCaptureBuffer,
    channel: usize,
    sample_offset: f64,
) -> f32 {
    if let Some(buf) = ptr.as_ref() {
        buf.read_sample(channel, sample_offset)
    } else {
        0.0
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_event_capture_buffer_get_capacity(
    ptr: *const RustEventCaptureBuffer,
) -> usize {
    if let Some(buf) = ptr.as_ref() {
        buf.capacity_samples()
    } else {
        0
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_event_capture_buffer_get_write_pos(
    ptr: *const RustEventCaptureBuffer,
) -> usize {
    if let Some(buf) = ptr.as_ref() {
        buf.write_position()
    } else {
        0
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_event_capture_buffer_get_num_channels(
    ptr: *const RustEventCaptureBuffer,
) -> usize {
    if let Some(buf) = ptr.as_ref() {
        buf.num_channels()
    } else {
        0
    }
}

// ----------------------------------------------------------------------------
// RustAudioFragment
// ----------------------------------------------------------------------------
#[no_mangle]
pub unsafe extern "C" fn n8_rust_audio_fragment_create() -> *mut RustAudioFragment {
    Box::into_raw(Box::new(RustAudioFragment::new()))
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_audio_fragment_destroy(ptr: *mut RustAudioFragment) {
    if !ptr.is_null() {
        drop(Box::from_raw(ptr));
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_audio_fragment_configure(
    ptr: *mut RustAudioFragment,
    capture_offset_samples: f64,
    duration_samples: u64,
    pitch_ratio: f32,
    reverse: bool,
) {
    if let Some(frag) = ptr.as_mut() {
        frag.configure(capture_offset_samples, duration_samples, pitch_ratio, reverse);
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_audio_fragment_reset(ptr: *mut RustAudioFragment) {
    if let Some(frag) = ptr.as_mut() {
        frag.reset();
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_audio_fragment_get_next_sample(
    ptr: *mut RustAudioFragment,
    capture_ptr: *const RustEventCaptureBuffer,
    envelope_gain: f32,
    out_l: *mut f32,
    out_r: *mut f32,
) -> bool {
    if let (Some(frag), Some(capture)) = (ptr.as_mut(), capture_ptr.as_ref()) {
        let (has_more, l, r) = frag.get_next_sample(capture, envelope_gain);
        if !out_l.is_null() { *out_l = l; }
        if !out_r.is_null() { *out_r = r; }
        has_more
    } else {
        if !out_l.is_null() { *out_l = 0.0; }
        if !out_r.is_null() { *out_r = 0.0; }
        false
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_audio_fragment_get_current_phase(
    ptr: *const RustAudioFragment,
) -> f64 {
    if let Some(frag) = ptr.as_ref() {
        frag.current_phase()
    } else {
        0.0
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_audio_fragment_get_duration_samples(
    ptr: *const RustAudioFragment,
) -> u64 {
    if let Some(frag) = ptr.as_ref() {
        frag.duration_samples()
    } else {
        0
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_audio_fragment_is_finished(
    ptr: *const RustAudioFragment,
) -> bool {
    if let Some(frag) = ptr.as_ref() {
        frag.is_finished()
    } else {
        true
    }
}

// ----------------------------------------------------------------------------
// RustEvent
// ----------------------------------------------------------------------------
#[no_mangle]
pub unsafe extern "C" fn n8_rust_event_create() -> *mut RustEvent {
    Box::into_raw(Box::new(RustEvent::new()))
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_event_destroy(ptr: *mut RustEvent) {
    if !ptr.is_null() {
        drop(Box::from_raw(ptr));
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_event_initialize(
    ptr: *mut RustEvent,
    attrs: *const RustEventAttributes,
    capture_offset: f64,
    duration_samples: u64,
    reverse: bool,
) {
    if let (Some(ev), Some(at)) = (ptr.as_mut(), attrs.as_ref()) {
        ev.initialize(*at, capture_offset, duration_samples, reverse);
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_event_start(ptr: *mut RustEvent) {
    if let Some(ev) = ptr.as_mut() {
        ev.start();
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_event_kill(ptr: *mut RustEvent) {
    if let Some(ev) = ptr.as_mut() {
        ev.kill();
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_event_release(ptr: *mut RustEvent) {
    if let Some(ev) = ptr.as_mut() {
        ev.release();
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_event_render(
    ptr: *mut RustEvent,
    capture_ptr: *const RustEventCaptureBuffer,
    out_l: *mut f32,
    out_r: *mut f32,
    num_samples: usize,
    source_level: f32,
    source_silence_override: i32,
) {
    if let (Some(ev), Some(capture)) = (ptr.as_mut(), capture_ptr.as_ref()) {
        if !out_l.is_null() && !out_r.is_null() && num_samples > 0 {
            let slice_l = std::slice::from_raw_parts_mut(out_l, num_samples);
            let slice_r = std::slice::from_raw_parts_mut(out_r, num_samples);
            ev.render(capture, slice_l, slice_r, num_samples, source_level, source_silence_override);
        }
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_event_is_active(ptr: *const RustEvent) -> bool {
    if let Some(ev) = ptr.as_ref() {
        ev.is_active()
    } else {
        false
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_event_get_state(ptr: *const RustEvent) -> u8 {
    if let Some(ev) = ptr.as_ref() {
        ev.get_state() as u8
    } else {
        EventLifecycle::Destroyed as u8
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_event_set_state(ptr: *mut RustEvent, state: u8) {
    if let Some(ev) = ptr.as_mut() {
        ev.set_state(EventLifecycle::from(state));
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_event_get_attributes(
    ptr: *const RustEvent,
    out_attrs: *mut RustEventAttributes,
) {
    if let (Some(ev), Some(out)) = (ptr.as_ref(), out_attrs.as_mut()) {
        *out = *ev.get_attributes();
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_event_set_attributes(
    ptr: *mut RustEvent,
    in_attrs: *const RustEventAttributes,
) {
    if let (Some(ev), Some(inp)) = (ptr.as_mut(), in_attrs.as_ref()) {
        *ev.get_attributes_mut() = *inp;
    }
}

// ----------------------------------------------------------------------------
// RustEventPool
// ----------------------------------------------------------------------------
#[no_mangle]
pub unsafe extern "C" fn n8_rust_event_pool_create() -> *mut RustEventPool {
    Box::into_raw(Box::new(RustEventPool::new()))
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_event_pool_destroy(ptr: *mut RustEventPool) {
    if !ptr.is_null() {
        drop(Box::from_raw(ptr));
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_event_pool_prepare(ptr: *mut RustEventPool, max_events: usize) {
    if let Some(pool) = ptr.as_mut() {
        pool.prepare(max_events);
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_event_pool_reset(ptr: *mut RustEventPool) {
    if let Some(pool) = ptr.as_mut() {
        pool.reset();
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_event_pool_acquire(ptr: *mut RustEventPool) -> *mut RustEvent {
    if let Some(pool) = ptr.as_mut() {
        pool.acquire()
    } else {
        std::ptr::null_mut()
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_event_pool_release(ptr: *mut RustEventPool, ev: *mut RustEvent) {
    if let Some(pool) = ptr.as_mut() {
        pool.release(ev);
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_event_pool_get_capacity(ptr: *const RustEventPool) -> usize {
    if let Some(pool) = ptr.as_ref() {
        pool.capacity()
    } else {
        0
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_event_pool_get_active_count(ptr: *const RustEventPool) -> usize {
    if let Some(pool) = ptr.as_ref() {
        pool.active_count()
    } else {
        0
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_event_pool_get_available_count(ptr: *const RustEventPool) -> usize {
    if let Some(pool) = ptr.as_ref() {
        pool.available_count()
    } else {
        0
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_event_pool_get_event_at(
    ptr: *mut RustEventPool,
    index: usize,
) -> *mut RustEvent {
    if let Some(pool) = ptr.as_mut() {
        pool.get_event_at(index)
    } else {
        std::ptr::null_mut()
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_event_pool_get_event_index(
    ptr: *const RustEventPool,
    ev: *const RustEvent,
) -> usize {
    if let Some(pool) = ptr.as_ref() {
        pool.get_event_index(ev)
    } else {
        usize::MAX
    }
}

// ----------------------------------------------------------------------------
// RustEventTelemetryBuffer
// ----------------------------------------------------------------------------
#[no_mangle]
pub unsafe extern "C" fn n8_rust_event_telemetry_create() -> *mut RustEventTelemetryBuffer {
    Box::into_raw(Box::new(RustEventTelemetryBuffer::new()))
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_event_telemetry_destroy(ptr: *mut RustEventTelemetryBuffer) {
    if !ptr.is_null() {
        drop(Box::from_raw(ptr));
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_event_telemetry_reset(ptr: *const RustEventTelemetryBuffer) {
    if let Some(buf) = ptr.as_ref() {
        buf.reset();
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_event_telemetry_push(
    ptr: *const RustEventTelemetryBuffer,
    item: *const RustEventTelemetryItem,
) -> bool {
    if let (Some(buf), Some(it)) = (ptr.as_ref(), item.as_ref()) {
        buf.push(*it)
    } else {
        false
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_event_telemetry_pop(
    ptr: *const RustEventTelemetryBuffer,
    out_item: *mut RustEventTelemetryItem,
) -> bool {
    if let (Some(buf), Some(out)) = (ptr.as_ref(), out_item.as_mut()) {
        buf.pop(out)
    } else {
        false
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_event_telemetry_size(
    ptr: *const RustEventTelemetryBuffer,
) -> usize {
    if let Some(buf) = ptr.as_ref() {
        buf.size()
    } else {
        0
    }
}

// ----------------------------------------------------------------------------
// RustEventManager
// ----------------------------------------------------------------------------
#[no_mangle]
pub unsafe extern "C" fn n8_rust_event_manager_create() -> *mut RustEventManager {
    Box::into_raw(Box::new(RustEventManager::new()))
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_event_manager_destroy(ptr: *mut RustEventManager) {
    if !ptr.is_null() {
        drop(Box::from_raw(ptr));
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_event_manager_prepare(
    ptr: *mut RustEventManager,
    sample_rate: f64,
    max_block_size: usize,
    pool_capacity: usize,
) {
    if let Some(mgr) = ptr.as_mut() {
        mgr.prepare(sample_rate, max_block_size, pool_capacity);
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_event_manager_reset(ptr: *mut RustEventManager) {
    if let Some(mgr) = ptr.as_mut() {
        mgr.reset();
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_event_manager_capture_input(
    ptr: *mut RustEventManager,
    in_l: *const f32,
    in_r: *const f32,
    num_samples: usize,
) {
    if let Some(mgr) = ptr.as_mut() {
        if !in_l.is_null() && num_samples > 0 {
            let slice_l = std::slice::from_raw_parts(in_l, num_samples);
            let slice_r = if !in_r.is_null() {
                std::slice::from_raw_parts(in_r, num_samples)
            } else {
                slice_l
            };
            mgr.capture_input_audio(slice_l, slice_r, num_samples);
        }
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_event_manager_spawn_event(
    ptr: *mut RustEventManager,
    event_type: u16,
    capture_offset_samples: f64,
    duration_samples: u64,
    pitch_ratio: f32,
    gain: f32,
    pan: f32,
    source_follow: f32,
    disappearance_mode: u8,
    reverse: bool,
) -> *mut RustEvent {
    if let Some(mgr) = ptr.as_mut() {
        mgr.spawn_event(
            event_type,
            capture_offset_samples,
            duration_samples,
            pitch_ratio,
            gain,
            pan,
            source_follow,
            disappearance_mode,
            reverse,
        )
    } else {
        std::ptr::null_mut()
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_event_manager_spawn_child_event(
    ptr: *mut RustEventManager,
    parent_ptr: *const RustEvent,
    offset_shift_samples: f64,
    pitch_multiplier: f32,
    energy_decay: f32,
) -> *mut RustEvent {
    if let (Some(mgr), Some(parent)) = (ptr.as_mut(), parent_ptr.as_ref()) {
        mgr.spawn_child_event(parent, offset_shift_samples, pitch_multiplier, energy_decay)
    } else {
        std::ptr::null_mut()
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_event_manager_render(
    ptr: *mut RustEventManager,
    out_l: *mut f32,
    out_r: *mut f32,
    num_samples: usize,
    source_level: f32,
    source_silence_override: i32,
) {
    if let Some(mgr) = ptr.as_mut() {
        if !out_l.is_null() && !out_r.is_null() && num_samples > 0 {
            let slice_l = std::slice::from_raw_parts_mut(out_l, num_samples);
            let slice_r = std::slice::from_raw_parts_mut(out_r, num_samples);
            mgr.render(slice_l, slice_r, num_samples, source_level, source_silence_override);
        }
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_event_manager_set_editor_active(
    ptr: *const RustEventManager,
    active: bool,
) {
    if let Some(mgr) = ptr.as_ref() {
        mgr.set_editor_active(active);
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_event_manager_is_editor_active(
    ptr: *const RustEventManager,
) -> bool {
    if let Some(mgr) = ptr.as_ref() {
        mgr.is_editor_active()
    } else {
        false
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_event_manager_get_active_count(
    ptr: *const RustEventManager,
) -> usize {
    if let Some(mgr) = ptr.as_ref() {
        mgr.get_active_event_count()
    } else {
        0
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_event_manager_get_pool_capacity(
    ptr: *const RustEventManager,
) -> usize {
    if let Some(mgr) = ptr.as_ref() {
        mgr.get_pool_capacity()
    } else {
        0
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_event_manager_get_capture_buffer(
    ptr: *mut RustEventManager,
) -> *mut RustEventCaptureBuffer {
    if let Some(mgr) = ptr.as_mut() {
        mgr.get_capture_buffer_mut() as *mut RustEventCaptureBuffer
    } else {
        std::ptr::null_mut()
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_event_manager_get_telemetry_buffer(
    ptr: *mut RustEventManager,
) -> *mut RustEventTelemetryBuffer {
    if let Some(mgr) = ptr.as_mut() {
        mgr.get_telemetry_buffer() as *const RustEventTelemetryBuffer as *mut RustEventTelemetryBuffer
    } else {
        std::ptr::null_mut()
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_event_manager_get_event_pool(
    ptr: *mut RustEventManager,
) -> *mut RustEventPool {
    if let Some(mgr) = ptr.as_mut() {
        mgr.get_event_pool_mut() as *mut RustEventPool
    } else {
        std::ptr::null_mut()
    }
}

// ==============================================================================
// Safe C-ABI Exports: Graph Engine & Topological Compiler (Phase 6, Rules 4, 28, 29, 30)
// ==============================================================================

#[no_mangle]
pub unsafe extern "C" fn n8_rust_graph_would_create_cycle(
    src_node: u32,
    dest_node: u32,
    node_ids: *const u32,
    num_nodes: u32,
    connections: *const RustGraphConnection,
    num_connections: u32,
) -> bool {
    if node_ids.is_null() || num_nodes == 0 {
        return true;
    }
    let nodes = std::slice::from_raw_parts(node_ids, num_nodes as usize);
    let conns = if !connections.is_null() && num_connections > 0 {
        std::slice::from_raw_parts(connections, num_connections as usize)
    } else {
        &[]
    };
    rust_would_create_cycle(src_node, dest_node, nodes, conns)
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_graph_validate_and_topological_sort(
    node_ids: *const u32,
    num_nodes: u32,
    connections: *const RustGraphConnection,
    num_connections: u32,
    out_sorted: *mut u32,
) -> bool {
    if node_ids.is_null() || out_sorted.is_null() {
        return false;
    }
    if num_nodes == 0 {
        return true;
    }
    let nodes = std::slice::from_raw_parts(node_ids, num_nodes as usize);
    let conns = if !connections.is_null() && num_connections > 0 {
        std::slice::from_raw_parts(connections, num_connections as usize)
    } else {
        &[]
    };
    let sorted = std::slice::from_raw_parts_mut(out_sorted, num_nodes as usize);
    rust_validate_and_topological_sort(nodes, conns, sorted)
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_graph_compute_topological_depths(
    node_ids: *const u32,
    num_nodes: u32,
    connections: *const RustGraphConnection,
    num_connections: u32,
    out_depths: *mut u32,
) {
    if node_ids.is_null() || out_depths.is_null() || num_nodes == 0 {
        return;
    }
    let nodes = std::slice::from_raw_parts(node_ids, num_nodes as usize);
    let conns = if !connections.is_null() && num_connections > 0 {
        std::slice::from_raw_parts(connections, num_connections as usize)
    } else {
        &[]
    };
    let depths = std::slice::from_raw_parts_mut(out_depths, num_nodes as usize);
    rust_compute_topological_depths(nodes, conns, depths);
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_graph_compute_auto_layout(
    node_ids: *const u32,
    num_nodes: u32,
    connections: *const RustGraphConnection,
    num_connections: u32,
    start_x: f32,
    start_y: f32,
    step_x: f32,
    step_y: f32,
    out_layouts: *mut RustNodeLayout,
) {
    if node_ids.is_null() || out_layouts.is_null() || num_nodes == 0 {
        return;
    }
    let nodes = std::slice::from_raw_parts(node_ids, num_nodes as usize);
    let conns = if !connections.is_null() && num_connections > 0 {
        std::slice::from_raw_parts(connections, num_connections as usize)
    } else {
        &[]
    };
    let layouts = std::slice::from_raw_parts_mut(out_layouts, num_nodes as usize);
    rust_compute_auto_layout(nodes, conns, start_x, start_y, step_x, step_y, layouts);
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_graph_exchanger_create() -> *mut RustGraphPlanExchanger {
    let ex = Box::new(RustGraphPlanExchanger::new());
    Box::into_raw(ex)
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_graph_exchanger_destroy(ptr: *mut RustGraphPlanExchanger) {
    if !ptr.is_null() {
        drop(Box::from_raw(ptr));
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_graph_exchanger_compile_standby(
    ptr: *mut RustGraphPlanExchanger,
    topological_order: *const u32,
    num_nodes: u32,
    connections: *const RustGraphConnection,
    num_connections: u32,
    bypassed_flags: *const bool,
    num_flags: u32,
) {
    if let Some(ex) = ptr.as_mut() {
        let order = if !topological_order.is_null() && num_nodes > 0 {
            std::slice::from_raw_parts(topological_order, num_nodes as usize)
        } else {
            &[]
        };
        let conns = if !connections.is_null() && num_connections > 0 {
            std::slice::from_raw_parts(connections, num_connections as usize)
        } else {
            &[]
        };
        let flags = if !bypassed_flags.is_null() && num_flags > 0 {
            std::slice::from_raw_parts(bypassed_flags, num_flags as usize)
        } else {
            &[]
        };
        ex.compile_standby_plan(order, conns, flags);
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_graph_exchanger_commit(ptr: *const RustGraphPlanExchanger) {
    if let Some(ex) = ptr.as_ref() {
        ex.commit_standby_plan();
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_graph_exchanger_get_active_step_count(
    ptr: *const RustGraphPlanExchanger,
) -> usize {
    if let Some(ex) = ptr.as_ref() {
        ex.get_active_step_count()
    } else {
        0
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_graph_exchanger_get_active_step(
    ptr: *const RustGraphPlanExchanger,
    index: usize,
    out_step: *mut RustExecutionStepDesc,
) -> bool {
    if let (Some(ex), false) = (ptr.as_ref(), out_step.is_null()) {
        let plan = ex.get_active_plan();
        if index < plan.step_count {
            *out_step = plan.steps[index];
            return true;
        }
    }
    false
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_graph_exchanger_active_has_explicit_connections(
    ptr: *const RustGraphPlanExchanger,
) -> bool {
    if let Some(ex) = ptr.as_ref() {
        ex.get_active_plan().has_explicit_connections
    } else {
        false
    }
}

// ==============================================================================
// Safe C-ABI Exports: Scene Snapshot, Morphing & Smart Randomizer (Phase 7, Rules 7, 21, 35)
// ==============================================================================

#[no_mangle]
pub unsafe extern "C" fn n8_rust_scene_morph_create() -> *mut RustSceneMorphEngine {
    let engine = Box::new(RustSceneMorphEngine::new());
    Box::into_raw(engine)
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_scene_morph_destroy(ptr: *mut RustSceneMorphEngine) {
    if !ptr.is_null() {
        drop(Box::from_raw(ptr));
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_scene_morph_clear_scene(
    ptr: *mut RustSceneMorphEngine,
    scene_idx: u32,
) {
    if let Some(eng) = ptr.as_mut() {
        match scene_idx {
            0 => eng.scene_a.clear(),
            1 => eng.scene_b.clear(),
            2 => eng.scene_c.clear(),
            3 => eng.scene_d.clear(),
            _ => {}
        }
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_scene_morph_set_node_params(
    ptr: *mut RustSceneMorphEngine,
    scene_idx: u32,
    node_id: u32,
    params: *const RustParamEntry,
    num_params: u32,
) {
    if let (Some(eng), false) = (ptr.as_mut(), params.is_null()) {
        let p_slice = std::slice::from_raw_parts(params, num_params as usize);
        let scene = match scene_idx {
            0 => &mut eng.scene_a,
            1 => &mut eng.scene_b,
            2 => &mut eng.scene_c,
            3 => &mut eng.scene_d,
            _ => return,
        };
        scene.set_node_params(node_id, p_slice);
        scene.is_captured = true;
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_scene_morph_set_macros(
    ptr: *mut RustSceneMorphEngine,
    scene_idx: u32,
    macros: *const f32,
) {
    if let (Some(eng), false) = (ptr.as_mut(), macros.is_null()) {
        let m_slice = std::slice::from_raw_parts(macros, scene_morph::NUM_MACROS);
        let scene = match scene_idx {
            0 => &mut eng.scene_a,
            1 => &mut eng.scene_b,
            2 => &mut eng.scene_c,
            3 => &mut eng.scene_d,
            _ => return,
        };
        scene.macros.copy_from_slice(m_slice);
        scene.is_captured = true;
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_scene_morph_has_scene(
    ptr: *const RustSceneMorphEngine,
    scene_idx: u32,
) -> bool {
    if let Some(eng) = ptr.as_ref() {
        match scene_idx {
            0 => eng.scene_a.is_captured,
            1 => eng.scene_b.is_captured,
            2 => eng.scene_c.is_captured,
            3 => eng.scene_d.is_captured,
            _ => false,
        }
    } else {
        false
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_scene_morph_set_factor_1d(
    ptr: *mut RustSceneMorphEngine,
    t: f32,
) {
    if let Some(eng) = ptr.as_mut() {
        eng.set_morph_factor_1d(t);
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_scene_morph_get_target_factor_1d(
    ptr: *const RustSceneMorphEngine,
) -> f32 {
    if let Some(eng) = ptr.as_ref() {
        eng.get_target_morph_1d()
    } else {
        0.0
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_scene_morph_get_current_factor_1d(
    ptr: *const RustSceneMorphEngine,
) -> f32 {
    if let Some(eng) = ptr.as_ref() {
        eng.get_current_morph_1d()
    } else {
        0.0
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_scene_morph_step_smoothing_1d(
    ptr: *mut RustSceneMorphEngine,
    smoothing_speed: f32,
) -> f32 {
    if let Some(eng) = ptr.as_mut() {
        eng.step_smoothing_1d(smoothing_speed)
    } else {
        0.0
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_scene_morph_interpolate_param_1d(
    ptr: *const RustSceneMorphEngine,
    node_id: u32,
    param_id: u32,
    fallback_val: f32,
) -> f32 {
    if let Some(eng) = ptr.as_ref() {
        eng.interpolate_param_1d(node_id, param_id, fallback_val)
    } else {
        fallback_val
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_scene_morph_interpolate_macros_1d(
    ptr: *const RustSceneMorphEngine,
    out_macros: *mut f32,
) {
    if let (Some(eng), false) = (ptr.as_ref(), out_macros.is_null()) {
        let mut m = [0.0f32; scene_morph::NUM_MACROS];
        eng.interpolate_macros_1d(&mut m);
        let out_slice = std::slice::from_raw_parts_mut(out_macros, scene_morph::NUM_MACROS);
        out_slice.copy_from_slice(&m);
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_scene_morph_set_xy(
    ptr: *mut RustSceneMorphEngine,
    x: f32,
    y: f32,
) {
    if let Some(eng) = ptr.as_mut() {
        eng.set_morph_xy(x, y);
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_scene_morph_get_target_x(
    ptr: *const RustSceneMorphEngine,
) -> f32 {
    if let Some(eng) = ptr.as_ref() {
        eng.get_target_morph_x()
    } else {
        0.0
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_scene_morph_get_target_y(
    ptr: *const RustSceneMorphEngine,
) -> f32 {
    if let Some(eng) = ptr.as_ref() {
        eng.get_target_morph_y()
    } else {
        0.0
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_scene_morph_get_current_x(
    ptr: *const RustSceneMorphEngine,
) -> f32 {
    if let Some(eng) = ptr.as_ref() {
        eng.get_current_morph_x()
    } else {
        0.0
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_scene_morph_get_current_y(
    ptr: *const RustSceneMorphEngine,
) -> f32 {
    if let Some(eng) = ptr.as_ref() {
        eng.get_current_morph_y()
    } else {
        0.0
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_scene_morph_step_smoothing_xy(
    ptr: *mut RustSceneMorphEngine,
    smoothing_speed: f32,
    out_x: *mut f32,
    out_y: *mut f32,
) {
    if let Some(eng) = ptr.as_mut() {
        let (cx, cy) = eng.step_smoothing_xy(smoothing_speed);
        if !out_x.is_null() { *out_x = cx; }
        if !out_y.is_null() { *out_y = cy; }
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_scene_morph_interpolate_param_xy(
    ptr: *const RustSceneMorphEngine,
    node_id: u32,
    param_id: u32,
    fallback_val: f32,
) -> f32 {
    if let Some(eng) = ptr.as_ref() {
        eng.interpolate_param_xy(node_id, param_id, fallback_val)
    } else {
        fallback_val
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_scene_morph_interpolate_macros_xy(
    ptr: *const RustSceneMorphEngine,
    out_macros: *mut f32,
) {
    if let (Some(eng), false) = (ptr.as_ref(), out_macros.is_null()) {
        let mut m = [0.0f32; scene_morph::NUM_MACROS];
        eng.interpolate_macros_xy(&mut m);
        let out_slice = std::slice::from_raw_parts_mut(out_macros, scene_morph::NUM_MACROS);
        out_slice.copy_from_slice(&m);
    }
}

// --- RustSmartRandomizer C-ABI ---

#[no_mangle]
pub unsafe extern "C" fn n8_rust_smart_randomizer_create(seed: u32) -> *mut RustSmartRandomizer {
    let r = Box::new(RustSmartRandomizer::new(seed));
    Box::into_raw(r)
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_smart_randomizer_destroy(ptr: *mut RustSmartRandomizer) {
    if !ptr.is_null() {
        drop(Box::from_raw(ptr));
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_smart_randomizer_perturb_param(
    ptr: *mut RustSmartRandomizer,
    current_val: f32,
    min_val: f32,
    max_val: f32,
    intensity: f32,
    is_feedback: bool,
    is_resonance: bool,
    is_drive: bool,
) -> f32 {
    if let Some(r) = ptr.as_mut() {
        r.perturb_parameter(current_val, min_val, max_val, intensity, is_feedback, is_resonance, is_drive)
    } else {
        current_val
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_smart_randomizer_mutate_macros(
    ptr: *mut RustSmartRandomizer,
    macros: *mut f32,
    intensity: f32,
) {
    if let (Some(r), false) = (ptr.as_mut(), macros.is_null()) {
        let m_slice = std::slice::from_raw_parts_mut(macros, scene_morph::NUM_MACROS);
        let mut m_arr = [0.0f32; scene_morph::NUM_MACROS];
        m_arr.copy_from_slice(m_slice);
        r.mutate_macros(&mut m_arr, intensity);
        m_slice.copy_from_slice(&m_arr);
    }
}

// ==============================================================================
// Safe C-ABI Exports: Optimization Engine & SIMD Kernels (Phase 8, Rules 9, 34, 47)
// ==============================================================================

#[no_mangle]
pub unsafe extern "C" fn n8_rust_opt_enable_ftz_daz() -> u32 {
    rust_enable_hardware_ftz_daz()
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_opt_restore_mxcsr(old_csr: u32) {
    rust_restore_hardware_mxcsr(old_csr);
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_opt_is_ftz_daz_active() -> bool {
    rust_is_ftz_daz_active()
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_opt_fast_tanh(x: f32) -> f32 {
    fast_pade_tanh(x)
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_opt_fast_soft_clip(x: f32) -> f32 {
    fast_soft_clip(x)
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_opt_fast_exp_negative(x: f32) -> f32 {
    fast_exp_negative(x)
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_opt_fast_db_to_gain(db: f32) -> f32 {
    fast_db_to_gain(db)
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_opt_vec_pade_tanh(
    input: *const f32,
    output: *mut f32,
    num_samples: u32,
) {
    if !input.is_null() && !output.is_null() && num_samples > 0 {
        let n = num_samples as usize;
        let in_slice = std::slice::from_raw_parts(input, n);
        let out_slice = std::slice::from_raw_parts_mut(output, n);
        vec_pade_tanh(in_slice, out_slice);
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_opt_vec_multiply_add(
    a: *const f32,
    b: *const f32,
    c: *const f32,
    out: *mut f32,
    num_samples: u32,
) {
    if !a.is_null() && !b.is_null() && !c.is_null() && !out.is_null() && num_samples > 0 {
        let n = num_samples as usize;
        let a_slice = std::slice::from_raw_parts(a, n);
        let b_slice = std::slice::from_raw_parts(b, n);
        let c_slice = std::slice::from_raw_parts(c, n);
        let out_slice = std::slice::from_raw_parts_mut(out, n);
        vec_multiply_add(a_slice, b_slice, c_slice, out_slice);
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_opt_vec_apply_gain(
    input: *const f32,
    output: *mut f32,
    gain: f32,
    num_samples: u32,
) {
    if !input.is_null() && !output.is_null() && num_samples > 0 {
        let n = num_samples as usize;
        let in_slice = std::slice::from_raw_parts(input, n);
        let out_slice = std::slice::from_raw_parts_mut(output, n);
        vec_apply_gain(in_slice, out_slice, gain);
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_opt_vec_mix(
    dry: *const f32,
    wet: *const f32,
    out: *mut f32,
    mix: f32,
    num_samples: u32,
) {
    if !dry.is_null() && !wet.is_null() && !out.is_null() && num_samples > 0 {
        let n = num_samples as usize;
        let dry_slice = std::slice::from_raw_parts(dry, n);
        let wet_slice = std::slice::from_raw_parts(wet, n);
        let out_slice = std::slice::from_raw_parts_mut(out, n);
        vec_mix(dry_slice, wet_slice, out_slice, mix);
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_opt_vec_flush_denormals(
    buffer: *mut f32,
    num_samples: u32,
) {
    if !buffer.is_null() && num_samples > 0 {
        let n = num_samples as usize;
        let buf_slice = std::slice::from_raw_parts_mut(buffer, n);
        vec_flush_denormals(buf_slice);
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_opt_vec_soft_clip(
    input: *const f32,
    output: *mut f32,
    num_samples: u32,
) {
    if !input.is_null() && !output.is_null() && num_samples > 0 {
        let n = num_samples as usize;
        let in_slice = std::slice::from_raw_parts(input, n);
        let out_slice = std::slice::from_raw_parts_mut(output, n);
        vec_soft_clip(in_slice, out_slice);
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_opt_compute_optimal_buffer_slots(
    step_predecessors: *const u32,
    total_predecessors: u32,
    step_pred_offsets: *const u32,
    step_pred_counts: *const u32,
    num_steps: u32,
    out_assignments: *mut RustStepSlotAssignment,
) -> u32 {
    if step_pred_offsets.is_null() || step_pred_counts.is_null() || out_assignments.is_null() || num_steps == 0 {
        return 0;
    }
    let preds = if !step_predecessors.is_null() && total_predecessors > 0 {
        std::slice::from_raw_parts(step_predecessors, total_predecessors as usize)
    } else {
        &[]
    };
    let offsets = std::slice::from_raw_parts(step_pred_offsets, num_steps as usize);
    let counts = std::slice::from_raw_parts(step_pred_counts, num_steps as usize);
    let assigns = std::slice::from_raw_parts_mut(out_assignments, num_steps as usize);
    rust_compute_optimal_buffer_slots(preds, offsets, counts, num_steps, assigns)
}

// ==============================================================================
// Safe C-ABI Exports: Phase 9 Large Effect Library (Rules 5, 8, 9, 34, 46, 47)
// ==============================================================================

// --- 1. Dimension Chorus ---
#[no_mangle]
pub unsafe extern "C" fn n8_rust_dimension_chorus_create(sample_rate: f32) -> *mut RustDimensionChorus {
    Box::into_raw(Box::new(RustDimensionChorus::new(sample_rate)))
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_dimension_chorus_destroy(ptr: *mut RustDimensionChorus) {
    if !ptr.is_null() {
        drop(Box::from_raw(ptr));
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_dimension_chorus_prepare(ptr: *mut RustDimensionChorus, sample_rate: f32) {
    if let Some(c) = ptr.as_mut() {
        c.prepare(sample_rate);
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_dimension_chorus_reset(ptr: *mut RustDimensionChorus) {
    if let Some(c) = ptr.as_mut() {
        c.reset();
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_dimension_chorus_set_param(ptr: *mut RustDimensionChorus, param_id: u32, value: f32) {
    if let Some(c) = ptr.as_mut() {
        match param_id {
            1 => c.set_mode(value as u32),
            2 => c.set_rate(value),
            3 => c.set_depth(value),
            4 => c.set_stereo_spread(value),
            5 => c.set_drive(value),
            6 => c.set_mix(value),
            _ => {}
        }
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_dimension_chorus_process(
    ptr: *mut RustDimensionChorus,
    in_l: *const f32,
    in_r: *const f32,
    out_l: *mut f32,
    out_r: *mut f32,
    num_samples: u32,
) {
    if ptr.is_null() || in_l.is_null() || out_l.is_null() || num_samples == 0 {
        return;
    }
    let c = &mut *ptr;
    let n = num_samples as usize;
    let in_l_slice = std::slice::from_raw_parts(in_l, n);
    let in_r_slice = if !in_r.is_null() { std::slice::from_raw_parts(in_r, n) } else { in_l_slice };
    let out_l_slice = std::slice::from_raw_parts_mut(out_l, n);
    if !out_r.is_null() && !std::ptr::eq(out_l, out_r) {
        let out_r_slice = std::slice::from_raw_parts_mut(out_r, n);
        c.process_block(in_l_slice, in_r_slice, out_l_slice, out_r_slice);
    } else {
        let mut scratch_r = [0.0f32; 1024];
        let mut offset = 0;
        while offset < n {
            let chunk = (n - offset).min(1024);
            c.process_block(
                &in_l_slice[offset..offset + chunk],
                &in_r_slice[offset..offset + chunk],
                &mut out_l_slice[offset..offset + chunk],
                &mut scratch_r[..chunk],
            );
            offset += chunk;
        }
    }
}

// --- 2. Dynamic Tremolo ---
#[no_mangle]
pub unsafe extern "C" fn n8_rust_dynamic_tremolo_create(sample_rate: f32) -> *mut RustDynamicTremolo {
    Box::into_raw(Box::new(RustDynamicTremolo::new(sample_rate)))
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_dynamic_tremolo_destroy(ptr: *mut RustDynamicTremolo) {
    if !ptr.is_null() {
        drop(Box::from_raw(ptr));
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_dynamic_tremolo_prepare(ptr: *mut RustDynamicTremolo, sample_rate: f32) {
    if let Some(t) = ptr.as_mut() {
        t.prepare(sample_rate);
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_dynamic_tremolo_reset(ptr: *mut RustDynamicTremolo) {
    if let Some(t) = ptr.as_mut() {
        t.reset();
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_dynamic_tremolo_set_param(ptr: *mut RustDynamicTremolo, param_id: u32, value: f32) {
    if let Some(t) = ptr.as_mut() {
        match param_id {
            1 => t.set_rate(value),
            2 => t.set_depth(value),
            3 => t.set_shape(value as u32),
            4 => t.set_stereo_phase(value),
            5 => t.set_dynamics(value),
            6 => t.set_mix(value),
            _ => {}
        }
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_dynamic_tremolo_process(
    ptr: *mut RustDynamicTremolo,
    in_l: *const f32,
    in_r: *const f32,
    out_l: *mut f32,
    out_r: *mut f32,
    num_samples: u32,
) {
    if ptr.is_null() || in_l.is_null() || out_l.is_null() || num_samples == 0 {
        return;
    }
    let t = &mut *ptr;
    let n = num_samples as usize;
    let in_l_slice = std::slice::from_raw_parts(in_l, n);
    let in_r_slice = if !in_r.is_null() { std::slice::from_raw_parts(in_r, n) } else { in_l_slice };
    let out_l_slice = std::slice::from_raw_parts_mut(out_l, n);
    if !out_r.is_null() && !std::ptr::eq(out_l, out_r) {
        let out_r_slice = std::slice::from_raw_parts_mut(out_r, n);
        t.process_block(in_l_slice, in_r_slice, out_l_slice, out_r_slice);
    } else {
        let mut scratch_r = [0.0f32; 1024];
        let mut offset = 0;
        while offset < n {
            let chunk = (n - offset).min(1024);
            t.process_block(
                &in_l_slice[offset..offset + chunk],
                &in_r_slice[offset..offset + chunk],
                &mut out_l_slice[offset..offset + chunk],
                &mut scratch_r[..chunk],
            );
            offset += chunk;
        }
    }
}

// --- 3. Multiband Saturator ---
#[no_mangle]
pub unsafe extern "C" fn n8_rust_multiband_saturator_create(sample_rate: f32) -> *mut RustMultibandSaturator {
    Box::into_raw(Box::new(RustMultibandSaturator::new(sample_rate)))
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_multiband_saturator_destroy(ptr: *mut RustMultibandSaturator) {
    if !ptr.is_null() {
        drop(Box::from_raw(ptr));
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_multiband_saturator_prepare(ptr: *mut RustMultibandSaturator, sample_rate: f32) {
    if let Some(s) = ptr.as_mut() {
        s.prepare(sample_rate);
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_multiband_saturator_reset(ptr: *mut RustMultibandSaturator) {
    if let Some(s) = ptr.as_mut() {
        s.reset();
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_multiband_saturator_set_param(ptr: *mut RustMultibandSaturator, param_id: u32, value: f32) {
    if let Some(s) = ptr.as_mut() {
        match param_id {
            1 => s.set_crossover_low(value),
            2 => s.set_crossover_high(value),
            3 => s.set_drive_low(value),
            4 => s.set_drive_mid(value),
            5 => s.set_drive_high(value),
            6 => s.set_model_low(value as u32),
            7 => s.set_model_mid(value as u32),
            8 => s.set_model_high(value as u32),
            9 => s.set_mix(value),
            _ => {}
        }
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_multiband_saturator_process(
    ptr: *mut RustMultibandSaturator,
    in_l: *const f32,
    in_r: *const f32,
    out_l: *mut f32,
    out_r: *mut f32,
    num_samples: u32,
) {
    if ptr.is_null() || in_l.is_null() || out_l.is_null() || num_samples == 0 {
        return;
    }
    let s = &mut *ptr;
    let n = num_samples as usize;
    let in_l_slice = std::slice::from_raw_parts(in_l, n);
    let in_r_slice = if !in_r.is_null() { std::slice::from_raw_parts(in_r, n) } else { in_l_slice };
    let out_l_slice = std::slice::from_raw_parts_mut(out_l, n);
    if !out_r.is_null() && !std::ptr::eq(out_l, out_r) {
        let out_r_slice = std::slice::from_raw_parts_mut(out_r, n);
        s.process_block(in_l_slice, in_r_slice, out_l_slice, out_r_slice);
    } else {
        let mut scratch_r = [0.0f32; 1024];
        let mut offset = 0;
        while offset < n {
            let chunk = (n - offset).min(1024);
            s.process_block(
                &in_l_slice[offset..offset + chunk],
                &in_r_slice[offset..offset + chunk],
                &mut out_l_slice[offset..offset + chunk],
                &mut scratch_r[..chunk],
            );
            offset += chunk;
        }
    }
}

// --- 4. Comb Resonator ---
#[no_mangle]
pub unsafe extern "C" fn n8_rust_comb_resonator_create(sample_rate: f32) -> *mut RustCombResonator {
    Box::into_raw(Box::new(RustCombResonator::new(sample_rate)))
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_comb_resonator_destroy(ptr: *mut RustCombResonator) {
    if !ptr.is_null() {
        drop(Box::from_raw(ptr));
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_comb_resonator_prepare(ptr: *mut RustCombResonator, sample_rate: f32) {
    if let Some(c) = ptr.as_mut() {
        c.prepare(sample_rate);
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_comb_resonator_reset(ptr: *mut RustCombResonator) {
    if let Some(c) = ptr.as_mut() {
        c.reset();
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_comb_resonator_set_param(ptr: *mut RustCombResonator, param_id: u32, value: f32) {
    if let Some(c) = ptr.as_mut() {
        match param_id {
            1 => c.set_freq(value),
            2 => c.set_feedback(value),
            3 => c.set_damping(value),
            4 => c.set_stereo_detune(value),
            5 => c.set_mix(value),
            _ => {}
        }
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_comb_resonator_process(
    ptr: *mut RustCombResonator,
    in_l: *const f32,
    in_r: *const f32,
    out_l: *mut f32,
    out_r: *mut f32,
    num_samples: u32,
) {
    if ptr.is_null() || in_l.is_null() || out_l.is_null() || num_samples == 0 {
        return;
    }
    let c = &mut *ptr;
    let n = num_samples as usize;
    let in_l_slice = std::slice::from_raw_parts(in_l, n);
    let in_r_slice = if !in_r.is_null() { std::slice::from_raw_parts(in_r, n) } else { in_l_slice };
    let out_l_slice = std::slice::from_raw_parts_mut(out_l, n);
    if !out_r.is_null() && !std::ptr::eq(out_l, out_r) {
        let out_r_slice = std::slice::from_raw_parts_mut(out_r, n);
        c.process_block(in_l_slice, in_r_slice, out_l_slice, out_r_slice);
    } else {
        let mut scratch_r = [0.0f32; 1024];
        let mut offset = 0;
        while offset < n {
            let chunk = (n - offset).min(1024);
            c.process_block(
                &in_l_slice[offset..offset + chunk],
                &in_r_slice[offset..offset + chunk],
                &mut out_l_slice[offset..offset + chunk],
                &mut scratch_r[..chunk],
            );
            offset += chunk;
        }
    }
}

// --- 5. Pitch Delay ---
#[no_mangle]
pub unsafe extern "C" fn n8_rust_pitch_delay_create(sample_rate: f32) -> *mut RustPitchDelay {
    Box::into_raw(Box::new(RustPitchDelay::new(sample_rate)))
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_pitch_delay_destroy(ptr: *mut RustPitchDelay) {
    if !ptr.is_null() {
        drop(Box::from_raw(ptr));
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_pitch_delay_prepare(ptr: *mut RustPitchDelay, sample_rate: f32) {
    if let Some(p) = ptr.as_mut() {
        p.prepare(sample_rate);
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_pitch_delay_reset(ptr: *mut RustPitchDelay) {
    if let Some(p) = ptr.as_mut() {
        p.reset();
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_pitch_delay_set_param(ptr: *mut RustPitchDelay, param_id: u32, value: f32) {
    if let Some(p) = ptr.as_mut() {
        match param_id {
            1 => p.set_delay_l(value),
            2 => p.set_delay_r(value),
            3 => p.set_pitch(value),
            4 => p.set_detune(value),
            5 => p.set_feedback(value),
            6 => p.set_reverse(value >= 0.5),
            7 => p.set_damping(value),
            8 => p.set_mix(value),
            _ => {}
        }
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_pitch_delay_process(
    ptr: *mut RustPitchDelay,
    in_l: *const f32,
    in_r: *const f32,
    out_l: *mut f32,
    out_r: *mut f32,
    num_samples: u32,
) {
    if ptr.is_null() || in_l.is_null() || out_l.is_null() || num_samples == 0 {
        return;
    }
    let p = &mut *ptr;
    let n = num_samples as usize;
    let in_l_slice = std::slice::from_raw_parts(in_l, n);
    let in_r_slice = if !in_r.is_null() { std::slice::from_raw_parts(in_r, n) } else { in_l_slice };
    let out_l_slice = std::slice::from_raw_parts_mut(out_l, n);
    if !out_r.is_null() && !std::ptr::eq(out_l, out_r) {
        let out_r_slice = std::slice::from_raw_parts_mut(out_r, n);
        p.process_block(in_l_slice, in_r_slice, out_l_slice, out_r_slice);
    } else {
        let mut scratch_r = [0.0f32; 1024];
        let mut offset = 0;
        while offset < n {
            let chunk = (n - offset).min(1024);
            p.process_block(
                &in_l_slice[offset..offset + chunk],
                &in_r_slice[offset..offset + chunk],
                &mut out_l_slice[offset..offset + chunk],
                &mut scratch_r[..chunk],
            );
            offset += chunk;
        }
    }
}

// --- 6. Phase Dispersion ---
#[no_mangle]
pub unsafe extern "C" fn n8_rust_phase_dispersion_create(sample_rate: f32) -> *mut RustPhaseDispersion {
    Box::into_raw(Box::new(RustPhaseDispersion::new(sample_rate)))
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_phase_dispersion_destroy(ptr: *mut RustPhaseDispersion) {
    if !ptr.is_null() {
        drop(Box::from_raw(ptr));
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_phase_dispersion_prepare(ptr: *mut RustPhaseDispersion, sample_rate: f32) {
    if let Some(p) = ptr.as_mut() {
        p.prepare(sample_rate);
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_phase_dispersion_reset(ptr: *mut RustPhaseDispersion) {
    if let Some(p) = ptr.as_mut() {
        p.reset();
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_phase_dispersion_set_param(ptr: *mut RustPhaseDispersion, param_id: u32, value: f32) {
    if let Some(p) = ptr.as_mut() {
        match param_id {
            1 => p.set_stage_count(value),
            2 => p.set_dispersion_freq(value),
            3 => p.set_spread(value),
            4 => p.set_damping(value),
            5 => p.set_feedback(value),
            6 => p.set_mix(value),
            _ => {}
        }
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_phase_dispersion_process(
    ptr: *mut RustPhaseDispersion,
    in_l: *const f32,
    in_r: *const f32,
    out_l: *mut f32,
    out_r: *mut f32,
    num_samples: u32,
) {
    if ptr.is_null() || in_l.is_null() || out_l.is_null() || num_samples == 0 {
        return;
    }
    let p = &mut *ptr;
    let n = num_samples as usize;
    let in_l_slice = std::slice::from_raw_parts(in_l, n);
    let in_r_slice = if !in_r.is_null() { std::slice::from_raw_parts(in_r, n) } else { in_l_slice };
    let out_l_slice = std::slice::from_raw_parts_mut(out_l, n);
    if !out_r.is_null() && !std::ptr::eq(out_l, out_r) {
        let out_r_slice = std::slice::from_raw_parts_mut(out_r, n);
        p.process_block(in_l_slice, in_r_slice, out_l_slice, out_r_slice, n);
    } else {
        let mut scratch_r = [0.0f32; 1024];
        let mut offset = 0;
        while offset < n {
            let chunk = (n - offset).min(1024);
            p.process_block(
                &in_l_slice[offset..offset + chunk],
                &in_r_slice[offset..offset + chunk],
                &mut out_l_slice[offset..offset + chunk],
                &mut scratch_r[..chunk],
                chunk,
            );
            offset += chunk;
        }
    }
}

// --- 7. Morphing SVF ---
#[no_mangle]
pub unsafe extern "C" fn n8_rust_morphing_svf_create(sample_rate: f32) -> *mut RustMorphingSvf {
    Box::into_raw(Box::new(RustMorphingSvf::new(sample_rate)))
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_morphing_svf_destroy(ptr: *mut RustMorphingSvf) {
    if !ptr.is_null() {
        drop(Box::from_raw(ptr));
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_morphing_svf_prepare(ptr: *mut RustMorphingSvf, sample_rate: f32) {
    if let Some(p) = ptr.as_mut() {
        p.prepare(sample_rate);
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_morphing_svf_reset(ptr: *mut RustMorphingSvf) {
    if let Some(p) = ptr.as_mut() {
        p.reset();
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_morphing_svf_set_param(ptr: *mut RustMorphingSvf, param_id: u32, value: f32) {
    if let Some(p) = ptr.as_mut() {
        match param_id {
            1 => p.set_cutoff(value),
            2 => p.set_resonance(value),
            3 => p.set_morph(value),
            4 => p.set_drive(value),
            5 => p.set_fm_depth(value),
            _ => {}
        }
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_morphing_svf_process(
    ptr: *mut RustMorphingSvf,
    in_l: *const f32,
    in_r: *const f32,
    fm_in: *const f32,
    out_l: *mut f32,
    out_r: *mut f32,
    num_samples: u32,
) {
    if ptr.is_null() || in_l.is_null() || out_l.is_null() || num_samples == 0 {
        return;
    }
    let p = &mut *ptr;
    let n = num_samples as usize;
    let in_l_slice = std::slice::from_raw_parts(in_l, n);
    let in_r_slice = if !in_r.is_null() { std::slice::from_raw_parts(in_r, n) } else { in_l_slice };
    let fm_slice = if !fm_in.is_null() { Some(std::slice::from_raw_parts(fm_in, n)) } else { None };
    let out_l_slice = std::slice::from_raw_parts_mut(out_l, n);
    if !out_r.is_null() && !std::ptr::eq(out_l, out_r) {
        let out_r_slice = std::slice::from_raw_parts_mut(out_r, n);
        p.process_block(in_l_slice, in_r_slice, fm_slice, out_l_slice, out_r_slice, n);
    } else {
        let mut scratch_r = [0.0f32; 1024];
        let mut offset = 0;
        while offset < n {
            let chunk = (n - offset).min(1024);
            let sub_fm = fm_slice.map(|s| &s[offset..offset + chunk]);
            p.process_block(
                &in_l_slice[offset..offset + chunk],
                &in_r_slice[offset..offset + chunk],
                sub_fm,
                &mut out_l_slice[offset..offset + chunk],
                &mut scratch_r[..chunk],
                chunk,
            );
            offset += chunk;
        }
    }
}

// --- 8. Fractal Resonator ---
#[no_mangle]
pub unsafe extern "C" fn n8_rust_fractal_resonator_create(sample_rate: f32) -> *mut RustFractalResonator {
    Box::into_raw(Box::new(RustFractalResonator::new(sample_rate)))
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_fractal_resonator_destroy(ptr: *mut RustFractalResonator) {
    if !ptr.is_null() {
        drop(Box::from_raw(ptr));
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_fractal_resonator_prepare(ptr: *mut RustFractalResonator, sample_rate: f32) {
    if let Some(p) = ptr.as_mut() {
        p.prepare(sample_rate);
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_fractal_resonator_reset(ptr: *mut RustFractalResonator) {
    if let Some(p) = ptr.as_mut() {
        p.reset();
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_fractal_resonator_set_param(ptr: *mut RustFractalResonator, param_id: u32, value: f32) {
    if let Some(p) = ptr.as_mut() {
        match param_id {
            1 => p.set_base_freq(value),
            2 => p.set_fractal_ratio(value),
            3 => p.set_decay(value),
            4 => p.set_damping(value),
            5 => p.set_stereo_width(value),
            6 => p.set_mix(value),
            _ => {}
        }
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_fractal_resonator_process(
    ptr: *mut RustFractalResonator,
    in_l: *const f32,
    in_r: *const f32,
    out_l: *mut f32,
    out_r: *mut f32,
    num_samples: u32,
) {
    if ptr.is_null() || in_l.is_null() || out_l.is_null() || num_samples == 0 {
        return;
    }
    let p = &mut *ptr;
    let n = num_samples as usize;
    let in_l_slice = std::slice::from_raw_parts(in_l, n);
    let in_r_slice = if !in_r.is_null() { std::slice::from_raw_parts(in_r, n) } else { in_l_slice };
    let out_l_slice = std::slice::from_raw_parts_mut(out_l, n);
    if !out_r.is_null() && !std::ptr::eq(out_l, out_r) {
        let out_r_slice = std::slice::from_raw_parts_mut(out_r, n);
        p.process_block(in_l_slice, in_r_slice, out_l_slice, out_r_slice, n);
    } else {
        let mut scratch_r = [0.0f32; 1024];
        let mut offset = 0;
        while offset < n {
            let chunk = (n - offset).min(1024);
            p.process_block(
                &in_l_slice[offset..offset + chunk],
                &in_r_slice[offset..offset + chunk],
                &mut out_l_slice[offset..offset + chunk],
                &mut scratch_r[..chunk],
                chunk,
            );
            offset += chunk;
        }
    }
}

// --- 9. Buchla Vactrol Low-Pass Gate & Resonant Strike ---
#[no_mangle]
pub unsafe extern "C" fn n8_rust_vactrol_lpg_create(sample_rate: f32) -> *mut RustVactrolLPG {
    Box::into_raw(Box::new(RustVactrolLPG::new(sample_rate)))
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_vactrol_lpg_destroy(ptr: *mut RustVactrolLPG) {
    if !ptr.is_null() {
        drop(Box::from_raw(ptr));
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_vactrol_lpg_prepare(ptr: *mut RustVactrolLPG, sample_rate: f32) {
    if let Some(p) = ptr.as_mut() {
        p.prepare(sample_rate);
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_vactrol_lpg_reset(ptr: *mut RustVactrolLPG) {
    if let Some(p) = ptr.as_mut() {
        p.reset();
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_vactrol_lpg_set_param(ptr: *mut RustVactrolLPG, param_id: u32, value: f32) {
    if let Some(p) = ptr.as_mut() {
        match param_id {
            1 => p.set_cutoff(value),
            2 => p.set_resonance(value),
            3 => p.set_decay_ms(value),
            4 => p.set_strike(value),
            5 => p.set_mode_balance(value),
            6 => p.set_drive(value),
            7 => p.set_mix(value),
            _ => {}
        }
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_vactrol_lpg_trigger_strike(ptr: *mut RustVactrolLPG) {
    if let Some(p) = ptr.as_mut() {
        p.trigger_strike();
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_vactrol_lpg_process(
    ptr: *mut RustVactrolLPG,
    in_l: *const f32,
    in_r: *const f32,
    strike_in: *const f32,
    out_l: *mut f32,
    out_r: *mut f32,
    num_samples: u32,
) {
    if ptr.is_null() || in_l.is_null() || out_l.is_null() || num_samples == 0 {
        return;
    }
    let p = &mut *ptr;
    let n = num_samples as usize;
    let in_l_slice = std::slice::from_raw_parts(in_l, n);
    let in_r_slice = if !in_r.is_null() { std::slice::from_raw_parts(in_r, n) } else { in_l_slice };
    let strike_slice = if !strike_in.is_null() { Some(std::slice::from_raw_parts(strike_in, n)) } else { None };
    let out_l_slice = std::slice::from_raw_parts_mut(out_l, n);
    if !out_r.is_null() && !std::ptr::eq(out_l, out_r) {
        let out_r_slice = std::slice::from_raw_parts_mut(out_r, n);
        p.process_block(in_l_slice, in_r_slice, strike_slice, out_l_slice, out_r_slice, n);
    } else {
        let mut scratch_r = [0.0f32; 1024];
        let mut offset = 0;
        while offset < n {
            let chunk = (n - offset).min(1024);
            let sub_strike = strike_slice.map(|s| &s[offset..offset + chunk]);
            p.process_block(
                &in_l_slice[offset..offset + chunk],
                &in_r_slice[offset..offset + chunk],
                sub_strike,
                &mut out_l_slice[offset..offset + chunk],
                &mut scratch_r[..chunk],
                chunk,
            );
            offset += chunk;
        }
    }
}

// --- 10. Shepard-Risset Barberpole Filter ---
#[no_mangle]
pub unsafe extern "C" fn n8_rust_barberpole_filter_create(sample_rate: f32) -> *mut RustBarberpoleFilter {
    Box::into_raw(Box::new(RustBarberpoleFilter::new(sample_rate)))
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_barberpole_filter_destroy(ptr: *mut RustBarberpoleFilter) {
    if !ptr.is_null() {
        drop(Box::from_raw(ptr));
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_barberpole_filter_prepare(ptr: *mut RustBarberpoleFilter, sample_rate: f32) {
    if let Some(p) = ptr.as_mut() {
        p.prepare(sample_rate);
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_barberpole_filter_reset(ptr: *mut RustBarberpoleFilter) {
    if let Some(p) = ptr.as_mut() {
        p.reset();
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_barberpole_filter_set_param(ptr: *mut RustBarberpoleFilter, param_id: u32, value: f32) {
    if let Some(p) = ptr.as_mut() {
        match param_id {
            1 => p.set_rate(value),
            2 => p.set_feedback(value),
            3 => p.set_stereo_spread(value),
            4 => p.set_f_min(value),
            5 => p.set_f_max(value),
            6 => p.set_mix(value),
            _ => {}
        }
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_barberpole_filter_process(
    ptr: *mut RustBarberpoleFilter,
    in_l: *const f32,
    in_r: *const f32,
    out_l: *mut f32,
    out_r: *mut f32,
    num_samples: u32,
) {
    if ptr.is_null() || in_l.is_null() || out_l.is_null() || num_samples == 0 {
        return;
    }
    let p = &mut *ptr;
    let n = num_samples as usize;
    let in_l_slice = std::slice::from_raw_parts(in_l, n);
    let in_r_slice = if !in_r.is_null() { std::slice::from_raw_parts(in_r, n) } else { in_l_slice };
    let out_l_slice = std::slice::from_raw_parts_mut(out_l, n);
    if !out_r.is_null() && !std::ptr::eq(out_l, out_r) {
        let out_r_slice = std::slice::from_raw_parts_mut(out_r, n);
        p.process_block(in_l_slice, in_r_slice, out_l_slice, out_r_slice, n);
    } else {
        let mut scratch_r = [0.0f32; 1024];
        let mut offset = 0;
        while offset < n {
            let chunk = (n - offset).min(1024);
            p.process_block(
                &in_l_slice[offset..offset + chunk],
                &in_r_slice[offset..offset + chunk],
                &mut out_l_slice[offset..offset + chunk],
                &mut scratch_r[..chunk],
                chunk,
            );
            offset += chunk;
        }
    }
}

// --- 11. Sidechain Pumper (Kickstart-Style Ducking & Multiband Pump) ---
#[no_mangle]
pub unsafe extern "C" fn n8_rust_sidechain_pumper_create(sample_rate: f32) -> *mut RustSidechainPumper {
    Box::into_raw(Box::new(RustSidechainPumper::new(sample_rate)))
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_sidechain_pumper_destroy(ptr: *mut RustSidechainPumper) {
    if !ptr.is_null() {
        drop(Box::from_raw(ptr));
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_sidechain_pumper_prepare(ptr: *mut RustSidechainPumper, sample_rate: f32) {
    if let Some(p) = ptr.as_mut() {
        p.prepare(sample_rate);
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_sidechain_pumper_reset(ptr: *mut RustSidechainPumper) {
    if let Some(p) = ptr.as_mut() {
        p.reset();
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_sidechain_pumper_set_param(ptr: *mut RustSidechainPumper, param_id: u32, value: f32) {
    if let Some(p) = ptr.as_mut() {
        p.set_param(param_id, value);
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_sidechain_pumper_set_host_sync(ptr: *mut RustSidechainPumper, bpm: f32, ppq: f64, is_playing: bool) {
    if let Some(p) = ptr.as_mut() {
        p.set_host_sync(bpm, ppq, is_playing);
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_sidechain_pumper_retrigger(ptr: *mut RustSidechainPumper) {
    if let Some(p) = ptr.as_mut() {
        p.retrigger();
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_sidechain_pumper_process(
    ptr: *mut RustSidechainPumper,
    in_l: *const f32,
    in_r: *const f32,
    out_l: *mut f32,
    out_r: *mut f32,
    sc_l: *const f32,
    sc_r: *const f32,
    num_samples: u32,
) {
    if let Some(p) = ptr.as_mut() {
        p.process(in_l, in_r, out_l, out_r, sc_l, sc_r, num_samples);
    }
}

// --- 12. Frequency-Split Spectral Dynamic Spatial Panner ---
#[no_mangle]
pub unsafe extern "C" fn n8_rust_frequency_split_panner_create(sample_rate: f32) -> *mut RustFrequencySplitPanner {
    Box::into_raw(Box::new(RustFrequencySplitPanner::new(sample_rate)))
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_frequency_split_panner_destroy(ptr: *mut RustFrequencySplitPanner) {
    if !ptr.is_null() {
        drop(Box::from_raw(ptr));
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_frequency_split_panner_prepare(ptr: *mut RustFrequencySplitPanner, sample_rate: f32) {
    if let Some(p) = ptr.as_mut() {
        p.prepare(sample_rate);
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_frequency_split_panner_reset(ptr: *mut RustFrequencySplitPanner) {
    if let Some(p) = ptr.as_mut() {
        p.reset();
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_frequency_split_panner_set_param(ptr: *mut RustFrequencySplitPanner, param_id: u32, value: f32) {
    if let Some(p) = ptr.as_mut() {
        p.set_param(param_id, value);
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_frequency_split_panner_process(
    ptr: *mut RustFrequencySplitPanner,
    in_l: *const f32,
    in_r: *const f32,
    out_l: *mut f32,
    out_r: *mut f32,
    num_samples: u32,
) {
    if let Some(p) = ptr.as_mut() {
        p.process(in_l, in_r, out_l, out_r, num_samples);
    }
}

// --- 13. Chebyshev Polynomial Harmonic Shaper ---
#[no_mangle]
pub unsafe extern "C" fn n8_rust_chebyshev_harmonic_shaper_create(sample_rate: f32) -> *mut RustChebyshevHarmonicShaper {
    Box::into_raw(Box::new(RustChebyshevHarmonicShaper::new(sample_rate)))
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_chebyshev_harmonic_shaper_destroy(ptr: *mut RustChebyshevHarmonicShaper) {
    if !ptr.is_null() {
        drop(Box::from_raw(ptr));
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_chebyshev_harmonic_shaper_prepare(ptr: *mut RustChebyshevHarmonicShaper, sample_rate: f32) {
    if let Some(p) = ptr.as_mut() {
        p.prepare(sample_rate);
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_chebyshev_harmonic_shaper_reset(ptr: *mut RustChebyshevHarmonicShaper) {
    if let Some(p) = ptr.as_mut() {
        p.reset();
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_chebyshev_harmonic_shaper_set_param(ptr: *mut RustChebyshevHarmonicShaper, param_id: u32, value: f32) {
    if let Some(p) = ptr.as_mut() {
        p.set_param(param_id, value);
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_chebyshev_harmonic_shaper_process(
    ptr: *mut RustChebyshevHarmonicShaper,
    in_l: *const f32,
    in_r: *const f32,
    out_l: *mut f32,
    out_r: *mut f32,
    num_samples: u32,
) {
    if let Some(p) = ptr.as_mut() {
        p.process(in_l, in_r, out_l, out_r, num_samples);
    }
}

// --- 14. Subharmonic Synthesizer & Bass Extender ---
#[no_mangle]
pub unsafe extern "C" fn n8_rust_subharmonic_synthesizer_create(sample_rate: f32) -> *mut RustSubharmonicSynthesizer {
    Box::into_raw(Box::new(RustSubharmonicSynthesizer::new(sample_rate)))
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_subharmonic_synthesizer_destroy(ptr: *mut RustSubharmonicSynthesizer) {
    if !ptr.is_null() {
        drop(Box::from_raw(ptr));
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_subharmonic_synthesizer_prepare(ptr: *mut RustSubharmonicSynthesizer, sample_rate: f32) {
    if let Some(p) = ptr.as_mut() {
        p.prepare(sample_rate);
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_subharmonic_synthesizer_reset(ptr: *mut RustSubharmonicSynthesizer) {
    if let Some(p) = ptr.as_mut() {
        p.reset();
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_subharmonic_synthesizer_set_param(ptr: *mut RustSubharmonicSynthesizer, param_id: u32, value: f32) {
    if let Some(p) = ptr.as_mut() {
        p.set_param(param_id, value);
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_subharmonic_synthesizer_process(
    ptr: *mut RustSubharmonicSynthesizer,
    in_l: *const f32,
    in_r: *const f32,
    out_l: *mut f32,
    out_r: *mut f32,
    num_samples: u32,
) {
    if let Some(p) = ptr.as_mut() {
        p.process(in_l, in_r, out_l, out_r, num_samples);
    }
}

// --- 15. Transaural Crosstalk Canceller ---
#[no_mangle]
pub unsafe extern "C" fn n8_rust_transaural_crosstalk_canceller_create(sample_rate: f32) -> *mut RustTransauralCrosstalkCanceller {
    Box::into_raw(Box::new(RustTransauralCrosstalkCanceller::new(sample_rate)))
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_transaural_crosstalk_canceller_destroy(ptr: *mut RustTransauralCrosstalkCanceller) {
    if !ptr.is_null() {
        drop(Box::from_raw(ptr));
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_transaural_crosstalk_canceller_prepare(ptr: *mut RustTransauralCrosstalkCanceller, sample_rate: f32) {
    if let Some(p) = ptr.as_mut() {
        p.prepare(sample_rate);
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_transaural_crosstalk_canceller_reset(ptr: *mut RustTransauralCrosstalkCanceller) {
    if let Some(p) = ptr.as_mut() {
        p.reset();
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_transaural_crosstalk_canceller_set_param(ptr: *mut RustTransauralCrosstalkCanceller, param_id: u32, value: f32) {
    if let Some(p) = ptr.as_mut() {
        p.set_param(param_id, value);
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_transaural_crosstalk_canceller_process(
    ptr: *mut RustTransauralCrosstalkCanceller,
    in_l: *const f32,
    in_r: *const f32,
    out_l: *mut f32,
    out_r: *mut f32,
    num_samples: u32,
) {
    if let Some(p) = ptr.as_mut() {
        p.process(in_l, in_r, out_l, out_r, num_samples);
    }
}

// --- 16. Doppler 3D Orbital Spatializer ---
#[no_mangle]
pub unsafe extern "C" fn n8_rust_doppler_orbital_spatializer_create(sample_rate: f32) -> *mut RustDopplerOrbitalSpatializer {
    Box::into_raw(Box::new(RustDopplerOrbitalSpatializer::new(sample_rate)))
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_doppler_orbital_spatializer_destroy(ptr: *mut RustDopplerOrbitalSpatializer) {
    if !ptr.is_null() {
        drop(Box::from_raw(ptr));
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_doppler_orbital_spatializer_prepare(ptr: *mut RustDopplerOrbitalSpatializer, sample_rate: f32) {
    if let Some(p) = ptr.as_mut() {
        p.prepare(sample_rate);
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_doppler_orbital_spatializer_reset(ptr: *mut RustDopplerOrbitalSpatializer) {
    if let Some(p) = ptr.as_mut() {
        p.reset();
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_doppler_orbital_spatializer_set_param(ptr: *mut RustDopplerOrbitalSpatializer, param_id: u32, value: f32) {
    if let Some(p) = ptr.as_mut() {
        p.set_param(param_id, value);
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_doppler_orbital_spatializer_process(
    ptr: *mut RustDopplerOrbitalSpatializer,
    in_l: *const f32,
    in_r: *const f32,
    out_l: *mut f32,
    out_r: *mut f32,
    num_samples: u32,
) {
    if let Some(p) = ptr.as_mut() {
        p.process(in_l, in_r, out_l, out_r, num_samples);
    }
}

// --- 17. Granular Buffer Shuffler & Reverse Memory ---
#[no_mangle]
pub unsafe extern "C" fn n8_rust_granular_buffer_shuffler_create(sample_rate: f32) -> *mut RustGranularBufferShuffler {
    Box::into_raw(Box::new(RustGranularBufferShuffler::new(sample_rate)))
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_granular_buffer_shuffler_destroy(ptr: *mut RustGranularBufferShuffler) {
    if !ptr.is_null() {
        drop(Box::from_raw(ptr));
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_granular_buffer_shuffler_prepare(ptr: *mut RustGranularBufferShuffler, sample_rate: f32) {
    if let Some(p) = ptr.as_mut() {
        p.prepare(sample_rate);
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_granular_buffer_shuffler_reset(ptr: *mut RustGranularBufferShuffler) {
    if let Some(p) = ptr.as_mut() {
        p.reset();
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_granular_buffer_shuffler_set_param(ptr: *mut RustGranularBufferShuffler, param_id: u32, value: f32) {
    if let Some(p) = ptr.as_mut() {
        p.set_param(param_id, value);
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_granular_buffer_shuffler_process(
    ptr: *mut RustGranularBufferShuffler,
    in_l: *const f32,
    in_r: *const f32,
    out_l: *mut f32,
    out_r: *mut f32,
    num_samples: u32,
) {
    if let Some(p) = ptr.as_mut() {
        p.process(in_l, in_r, out_l, out_r, num_samples);
    }
}

// --- 18. Disintegration Tape Looper ---
#[no_mangle]
pub unsafe extern "C" fn n8_rust_disintegration_looper_create(sample_rate: f32) -> *mut RustDisintegrationLooper {
    Box::into_raw(Box::new(RustDisintegrationLooper::new(sample_rate)))
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_disintegration_looper_destroy(ptr: *mut RustDisintegrationLooper) {
    if !ptr.is_null() {
        drop(Box::from_raw(ptr));
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_disintegration_looper_prepare(ptr: *mut RustDisintegrationLooper, sample_rate: f32) {
    if let Some(p) = ptr.as_mut() {
        p.prepare(sample_rate);
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_disintegration_looper_reset(ptr: *mut RustDisintegrationLooper) {
    if let Some(p) = ptr.as_mut() {
        p.reset();
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_disintegration_looper_set_param(ptr: *mut RustDisintegrationLooper, param_id: u32, value: f32) {
    if let Some(p) = ptr.as_mut() {
        p.set_param(param_id, value);
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_disintegration_looper_process(
    ptr: *mut RustDisintegrationLooper,
    in_l: *const f32,
    in_r: *const f32,
    out_l: *mut f32,
    out_r: *mut f32,
    num_samples: u32,
) {
    if let Some(p) = ptr.as_mut() {
        p.process(in_l, in_r, out_l, out_r, num_samples);
    }
}

// --- 19. Helical Spring Reverb Tank ---
#[no_mangle]
pub unsafe extern "C" fn n8_rust_spring_reverb_tank_create(sample_rate: f32) -> *mut RustSpringReverbTank {
    Box::into_raw(Box::new(RustSpringReverbTank::new(sample_rate)))
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_spring_reverb_tank_destroy(ptr: *mut RustSpringReverbTank) {
    if !ptr.is_null() {
        drop(Box::from_raw(ptr));
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_spring_reverb_tank_prepare(ptr: *mut RustSpringReverbTank, sample_rate: f32) {
    if let Some(p) = ptr.as_mut() {
        p.prepare(sample_rate);
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_spring_reverb_tank_reset(ptr: *mut RustSpringReverbTank) {
    if let Some(p) = ptr.as_mut() {
        p.reset();
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_spring_reverb_tank_set_param(ptr: *mut RustSpringReverbTank, param_id: u32, value: f32) {
    if let Some(p) = ptr.as_mut() {
        p.set_param(param_id, value);
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_spring_reverb_tank_process(
    ptr: *mut RustSpringReverbTank,
    in_l: *const f32,
    in_r: *const f32,
    out_l: *mut f32,
    out_r: *mut f32,
    num_samples: u32,
) {
    if let Some(p) = ptr.as_mut() {
        p.process(in_l, in_r, out_l, out_r, num_samples);
    }
}

// --- 20. Dual Bode Frequency Shifter ---
#[no_mangle]
pub unsafe extern "C" fn n8_rust_dual_bode_frequency_shifter_create(sample_rate: f32) -> *mut RustDualBodeFrequencyShifter {
    Box::into_raw(Box::new(RustDualBodeFrequencyShifter::new(sample_rate)))
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_dual_bode_frequency_shifter_destroy(ptr: *mut RustDualBodeFrequencyShifter) {
    if !ptr.is_null() {
        drop(Box::from_raw(ptr));
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_dual_bode_frequency_shifter_prepare(ptr: *mut RustDualBodeFrequencyShifter, sample_rate: f32) {
    if let Some(p) = ptr.as_mut() {
        p.prepare(sample_rate);
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_dual_bode_frequency_shifter_reset(ptr: *mut RustDualBodeFrequencyShifter) {
    if let Some(p) = ptr.as_mut() {
        p.reset();
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_dual_bode_frequency_shifter_set_param(ptr: *mut RustDualBodeFrequencyShifter, param_id: u32, value: f32) {
    if let Some(p) = ptr.as_mut() {
        p.set_param(param_id, value);
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_dual_bode_frequency_shifter_process(
    ptr: *mut RustDualBodeFrequencyShifter,
    in_l: *const f32,
    in_r: *const f32,
    out_l: *mut f32,
    out_r: *mut f32,
    num_samples: u32,
) {
    if let Some(p) = ptr.as_mut() {
        p.process(in_l, in_r, out_l, out_r, num_samples);
    }
}

// --- 21. Microtonal Harmonizer ---
#[no_mangle]
pub unsafe extern "C" fn n8_rust_microtonal_harmonizer_create(sample_rate: f32) -> *mut RustMicrotonalHarmonizer {
    Box::into_raw(Box::new(RustMicrotonalHarmonizer::new(sample_rate)))
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_microtonal_harmonizer_destroy(ptr: *mut RustMicrotonalHarmonizer) {
    if !ptr.is_null() {
        drop(Box::from_raw(ptr));
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_microtonal_harmonizer_prepare(ptr: *mut RustMicrotonalHarmonizer, sample_rate: f32) {
    if let Some(p) = ptr.as_mut() {
        p.prepare(sample_rate);
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_microtonal_harmonizer_reset(ptr: *mut RustMicrotonalHarmonizer) {
    if let Some(p) = ptr.as_mut() {
        p.reset();
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_microtonal_harmonizer_set_param(ptr: *mut RustMicrotonalHarmonizer, param_id: u32, value: f32) {
    if let Some(p) = ptr.as_mut() {
        p.set_param(param_id, value);
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_microtonal_harmonizer_process(
    ptr: *mut RustMicrotonalHarmonizer,
    in_l: *const f32,
    in_r: *const f32,
    out_l: *mut f32,
    out_r: *mut f32,
    num_samples: u32,
) {
    if let Some(p) = ptr.as_mut() {
        p.process(in_l, in_r, out_l, out_r, num_samples);
    }
}

// --- 22. Formant Pitch Shifter ---
#[no_mangle]
pub unsafe extern "C" fn n8_rust_formant_pitch_shifter_create(sample_rate: f32) -> *mut RustFormantPitchShifter {
    Box::into_raw(Box::new(RustFormantPitchShifter::new(sample_rate)))
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_formant_pitch_shifter_destroy(ptr: *mut RustFormantPitchShifter) {
    if !ptr.is_null() {
        drop(Box::from_raw(ptr));
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_formant_pitch_shifter_prepare(ptr: *mut RustFormantPitchShifter, sample_rate: f32) {
    if let Some(p) = ptr.as_mut() {
        p.prepare(sample_rate);
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_formant_pitch_shifter_reset(ptr: *mut RustFormantPitchShifter) {
    if let Some(p) = ptr.as_mut() {
        p.reset();
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_formant_pitch_shifter_set_param(ptr: *mut RustFormantPitchShifter, param_id: u32, value: f32) {
    if let Some(p) = ptr.as_mut() {
        p.set_param(param_id, value);
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_formant_pitch_shifter_process(
    ptr: *mut RustFormantPitchShifter,
    in_l: *const f32,
    in_r: *const f32,
    out_l: *mut f32,
    out_r: *mut f32,
    num_samples: u32,
) {
    if let Some(p) = ptr.as_mut() {
        p.process(in_l, in_r, out_l, out_r, num_samples);
    }
}

// --- 23. Spectral Resonance Tamer ---
#[no_mangle]
pub unsafe extern "C" fn n8_rust_spectral_resonance_tamer_create(sample_rate: f32) -> *mut RustSpectralResonanceTamer {
    Box::into_raw(Box::new(RustSpectralResonanceTamer::new(sample_rate)))
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_spectral_resonance_tamer_destroy(ptr: *mut RustSpectralResonanceTamer) {
    if !ptr.is_null() {
        drop(Box::from_raw(ptr));
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_spectral_resonance_tamer_prepare(ptr: *mut RustSpectralResonanceTamer, sample_rate: f32) {
    if let Some(p) = ptr.as_mut() {
        p.prepare(sample_rate);
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_spectral_resonance_tamer_reset(ptr: *mut RustSpectralResonanceTamer) {
    if let Some(p) = ptr.as_mut() {
        p.reset();
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_spectral_resonance_tamer_set_param(ptr: *mut RustSpectralResonanceTamer, param_id: u32, value: f32) {
    if let Some(p) = ptr.as_mut() {
        p.set_param(param_id, value);
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_spectral_resonance_tamer_process(
    ptr: *mut RustSpectralResonanceTamer,
    in_l: *const f32,
    in_r: *const f32,
    out_l: *mut f32,
    out_r: *mut f32,
    num_samples: u32,
) {
    if let Some(p) = ptr.as_mut() {
        p.process(in_l, in_r, out_l, out_r, num_samples);
    }
}

// --- 24. Spectral Morphing Convolver ---
#[no_mangle]
pub unsafe extern "C" fn n8_rust_spectral_morphing_convolver_create(sample_rate: f32) -> *mut RustSpectralMorphingConvolver {
    Box::into_raw(Box::new(RustSpectralMorphingConvolver::new(sample_rate)))
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_spectral_morphing_convolver_destroy(ptr: *mut RustSpectralMorphingConvolver) {
    if !ptr.is_null() {
        drop(Box::from_raw(ptr));
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_spectral_morphing_convolver_prepare(ptr: *mut RustSpectralMorphingConvolver, sample_rate: f32) {
    if let Some(p) = ptr.as_mut() {
        p.prepare(sample_rate);
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_spectral_morphing_convolver_reset(ptr: *mut RustSpectralMorphingConvolver) {
    if let Some(p) = ptr.as_mut() {
        p.reset();
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_spectral_morphing_convolver_set_param(ptr: *mut RustSpectralMorphingConvolver, param_id: u32, value: f32) {
    if let Some(p) = ptr.as_mut() {
        p.set_param(param_id, value);
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_spectral_morphing_convolver_process(
    ptr: *mut RustSpectralMorphingConvolver,
    in_l: *const f32,
    in_r: *const f32,
    out_l: *mut f32,
    out_r: *mut f32,
    num_samples: u32,
) {
    if let Some(p) = ptr.as_mut() {
        p.process(in_l, in_r, out_l, out_r, num_samples);
    }
}

// ============================================================================
// C-ABI EXPORTS: EVENT CONTAINER (DSP #47 / NodeType::EventContainer)
// ============================================================================

#[no_mangle]
pub unsafe extern "C" fn n8_rust_event_container_create(sample_rate: f32) -> *mut EventContainerCore {
    Box::into_raw(Box::new(EventContainerCore::new(sample_rate)))
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_event_container_free(ptr: *mut EventContainerCore) {
    if !ptr.is_null() {
        drop(Box::from_raw(ptr));
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_event_container_prepare(
    ptr: *mut EventContainerCore,
    sample_rate: f32,
    max_block_size: u32,
    pool_capacity: u32,
) {
    if let Some(core) = ptr.as_mut() {
        core.prepare(sample_rate, max_block_size as usize, pool_capacity as usize);
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_event_container_reset(ptr: *mut EventContainerCore) {
    if let Some(core) = ptr.as_mut() {
        core.reset();
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_event_container_set_parameter(
    ptr: *mut EventContainerCore,
    param_id: u32,
    value: f32,
) {
    if let Some(core) = ptr.as_mut() {
        core.set_parameter(param_id, value);
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_event_container_get_parameter(
    ptr: *const EventContainerCore,
    param_id: u32,
) -> f32 {
    if let Some(core) = ptr.as_ref() {
        core.get_parameter(param_id)
    } else {
        0.0
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_event_container_inject_forward(
    ptr: *mut EventContainerCore,
    in_l: *const f32,
    in_r: *const f32,
    forward_l: *mut f32,
    forward_r: *mut f32,
    num_samples: u32,
) {
    if let Some(core) = ptr.as_mut() {
        let n = num_samples as usize;
        if n == 0 || forward_l.is_null() || forward_r.is_null() {
            return;
        }
        let in_l_slice = if !in_l.is_null() {
            Some(core::slice::from_raw_parts(in_l, n))
        } else {
            None
        };
        let in_r_slice = if !in_r.is_null() {
            Some(core::slice::from_raw_parts(in_r, n))
        } else {
            None
        };
        let fwd_l_slice = core::slice::from_raw_parts_mut(forward_l, n);
        let fwd_r_slice = core::slice::from_raw_parts_mut(forward_r, n);
        core.inject_forward(in_l_slice, in_r_slice, fwd_l_slice, fwd_r_slice, n);
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_event_container_process_return(
    ptr: *mut EventContainerCore,
    in_l: *const f32,
    in_r: *const f32,
    wet_l: *const f32,
    wet_r: *const f32,
    out_l: *mut f32,
    out_r: *mut f32,
    num_samples: u32,
) {
    if let Some(core) = ptr.as_mut() {
        let n = num_samples as usize;
        if n == 0 || out_l.is_null() || wet_l.is_null() || wet_r.is_null() {
            return;
        }
        let in_l_slice = if !in_l.is_null() {
            Some(core::slice::from_raw_parts(in_l, n))
        } else {
            None
        };
        let in_r_slice = if !in_r.is_null() {
            Some(core::slice::from_raw_parts(in_r, n))
        } else {
            None
        };
        let wet_l_slice = core::slice::from_raw_parts(wet_l, n);
        let wet_r_slice = core::slice::from_raw_parts(wet_r, n);
        let out_l_slice = core::slice::from_raw_parts_mut(out_l, n);
        if !out_r.is_null() && !std::ptr::eq(out_l, out_r) {
            let out_r_slice = core::slice::from_raw_parts_mut(out_r, n);
            core.process_return(in_l_slice, in_r_slice, wet_l_slice, wet_r_slice, out_l_slice, Some(out_r_slice), n);
        } else {
            core.process_return(in_l_slice, in_r_slice, wet_l_slice, wet_r_slice, out_l_slice, None, n);
        }
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_event_container_process(
    ptr: *mut EventContainerCore,
    in_l: *const f32,
    in_r: *const f32,
    out_l: *mut f32,
    out_r: *mut f32,
    num_samples: u32,
) {
    if let Some(core) = ptr.as_mut() {
        let n = num_samples as usize;
        if n == 0 || out_l.is_null() {
            return;
        }
        let in_l_slice = if !in_l.is_null() {
            Some(core::slice::from_raw_parts(in_l, n))
        } else {
            None
        };
        let in_r_slice = if !in_r.is_null() {
            Some(core::slice::from_raw_parts(in_r, n))
        } else {
            None
        };
        let out_l_slice = core::slice::from_raw_parts_mut(out_l, n);
        if !out_r.is_null() && !std::ptr::eq(out_l, out_r) {
            let out_r_slice = core::slice::from_raw_parts_mut(out_r, n);
            core.process_direct(in_l_slice, in_r_slice, out_l_slice, Some(out_r_slice), n);
        } else {
            core.process_direct(in_l_slice, in_r_slice, out_l_slice, None, n);
        }
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_event_container_get_event_manager(
    ptr: *mut EventContainerCore,
) -> *mut RustEventManager {
    if let Some(core) = ptr.as_mut() {
        core.get_event_manager_mut()
    } else {
        std::ptr::null_mut()
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_event_container_get_active_count(
    ptr: *const EventContainerCore,
) -> u32 {
    if let Some(core) = ptr.as_ref() {
        core.get_active_event_count() as u32
    } else {
        0
    }
}

// =========================================================================
// C-ABI String Helpers (Rules 21, 22, 27, 47)
// =========================================================================
unsafe fn c_char_to_str<'a>(ptr: *const std::os::raw::c_char) -> &'a str {
    if ptr.is_null() {
        ""
    } else {
        std::ffi::CStr::from_ptr(ptr).to_str().unwrap_or("")
    }
}

unsafe fn copy_str_to_c_buf(src: &str, out_buf: *mut std::os::raw::c_char, max_buf_len: usize) -> usize {
    if out_buf.is_null() || max_buf_len == 0 {
        return src.len();
    }
    let bytes = src.as_bytes();
    let copy_len = bytes.len().min(max_buf_len - 1);
    std::ptr::copy_nonoverlapping(bytes.as_ptr(), out_buf as *mut u8, copy_len);
    *out_buf.add(copy_len) = 0;
    copy_len
}

// =========================================================================
// Graph Undo / Redo Manager C-ABI (Rules 21, 22, 47)
// =========================================================================
#[no_mangle]
pub unsafe extern "C" fn n8_rust_graph_undo_manager_create(
    max_history_steps: usize,
) -> *mut RustGraphUndoManager {
    let mgr = Box::new(RustGraphUndoManager::new(max_history_steps));
    Box::into_raw(mgr)
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_graph_undo_manager_destroy(
    ptr: *mut RustGraphUndoManager,
) {
    if !ptr.is_null() {
        drop(Box::from_raw(ptr));
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_graph_undo_manager_push_action(
    ptr: *mut RustGraphUndoManager,
    desc: *const std::os::raw::c_char,
    category: u8,
    serialized_state: *const std::os::raw::c_char,
    timestamp_ms: i64,
) {
    if let Some(mgr) = ptr.as_mut() {
        let d = c_char_to_str(desc);
        let s = c_char_to_str(serialized_state);
        mgr.push_action(d, category, s, timestamp_ms);
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_graph_undo_manager_can_undo(
    ptr: *const RustGraphUndoManager,
) -> bool {
    if let Some(mgr) = ptr.as_ref() {
        mgr.can_undo()
    } else {
        false
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_graph_undo_manager_can_redo(
    ptr: *const RustGraphUndoManager,
) -> bool {
    if let Some(mgr) = ptr.as_ref() {
        mgr.can_redo()
    } else {
        false
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_graph_undo_manager_undo(
    ptr: *mut RustGraphUndoManager,
    out_buf: *mut std::os::raw::c_char,
    max_buf_len: usize,
) -> bool {
    if let Some(mgr) = ptr.as_mut() {
        if let Some(state) = mgr.undo() {
            copy_str_to_c_buf(state, out_buf, max_buf_len);
            return true;
        }
    }
    false
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_graph_undo_manager_redo(
    ptr: *mut RustGraphUndoManager,
    out_buf: *mut std::os::raw::c_char,
    max_buf_len: usize,
) -> bool {
    if let Some(mgr) = ptr.as_mut() {
        if let Some(state) = mgr.redo() {
            copy_str_to_c_buf(state, out_buf, max_buf_len);
            return true;
        }
    }
    false
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_graph_undo_manager_jump_to_step(
    ptr: *mut RustGraphUndoManager,
    target_index: usize,
    out_buf: *mut std::os::raw::c_char,
    max_buf_len: usize,
) -> bool {
    if let Some(mgr) = ptr.as_mut() {
        if let Some(state) = mgr.jump_to_step(target_index) {
            copy_str_to_c_buf(state, out_buf, max_buf_len);
            return true;
        }
    }
    false
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_graph_undo_manager_get_current_index(
    ptr: *const RustGraphUndoManager,
) -> usize {
    if let Some(mgr) = ptr.as_ref() {
        mgr.get_current_index()
    } else {
        0
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_graph_undo_manager_get_history_size(
    ptr: *const RustGraphUndoManager,
) -> usize {
    if let Some(mgr) = ptr.as_ref() {
        mgr.get_history_size()
    } else {
        0
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_graph_undo_manager_get_state_len(
    ptr: *const RustGraphUndoManager,
    index: usize,
) -> usize {
    if let Some(mgr) = ptr.as_ref() {
        mgr.get_state_len(index)
    } else {
        0
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_graph_undo_manager_get_current_state_len(
    ptr: *const RustGraphUndoManager,
) -> usize {
    if let Some(mgr) = ptr.as_ref() {
        mgr.get_current_state_len()
    } else {
        0
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_graph_undo_manager_get_checkpoint_info(
    ptr: *const RustGraphUndoManager,
    index: usize,
    out_desc: *mut std::os::raw::c_char,
    desc_len: usize,
    out_category: *mut u8,
    out_timestamp: *mut i64,
    out_state_len: *mut usize,
) -> bool {
    if let Some(mgr) = ptr.as_ref() {
        if let Some(cp) = mgr.get_checkpoint(index) {
            copy_str_to_c_buf(&cp.description, out_desc, desc_len);
            if !out_category.is_null() {
                *out_category = cp.category as u8;
            }
            if !out_timestamp.is_null() {
                *out_timestamp = cp.timestamp_ms;
            }
            if !out_state_len.is_null() {
                *out_state_len = cp.serialized_state.len();
            }
            return true;
        }
    }
    false
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_graph_undo_manager_get_checkpoint_state(
    ptr: *const RustGraphUndoManager,
    index: usize,
    out_buf: *mut std::os::raw::c_char,
    max_buf_len: usize,
) -> bool {
    if let Some(mgr) = ptr.as_ref() {
        if let Some(cp) = mgr.get_checkpoint(index) {
            copy_str_to_c_buf(&cp.serialized_state, out_buf, max_buf_len);
            return true;
        }
    }
    false
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_graph_undo_manager_get_next_undo_description(
    ptr: *const RustGraphUndoManager,
    out_buf: *mut std::os::raw::c_char,
    max_buf_len: usize,
) {
    if let Some(mgr) = ptr.as_ref() {
        let desc = mgr.get_next_undo_description();
        copy_str_to_c_buf(desc, out_buf, max_buf_len);
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_graph_undo_manager_get_next_redo_description(
    ptr: *const RustGraphUndoManager,
    out_buf: *mut std::os::raw::c_char,
    max_buf_len: usize,
) {
    if let Some(mgr) = ptr.as_ref() {
        let desc = mgr.get_next_redo_description();
        copy_str_to_c_buf(desc, out_buf, max_buf_len);
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_graph_undo_manager_clear(
    ptr: *mut RustGraphUndoManager,
) {
    if let Some(mgr) = ptr.as_mut() {
        mgr.clear();
    }
}

// =========================================================================
// User Preset Bank C-ABI (Rules 21, 27, 47)
// =========================================================================
#[no_mangle]
pub unsafe extern "C" fn n8_rust_user_preset_bank_create() -> *mut RustUserPresetBank {
    let bank = Box::new(RustUserPresetBank::new());
    Box::into_raw(bank)
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_user_preset_bank_destroy(ptr: *mut RustUserPresetBank) {
    if !ptr.is_null() {
        drop(Box::from_raw(ptr));
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_user_preset_bank_clear(ptr: *mut RustUserPresetBank) {
    if let Some(bank) = ptr.as_mut() {
        bank.clear();
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_user_preset_bank_set_directory(
    ptr: *mut RustUserPresetBank,
    dir_path: *const std::os::raw::c_char,
) {
    if let Some(bank) = ptr.as_mut() {
        bank.set_directory(c_char_to_str(dir_path));
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_user_preset_bank_add_preset(
    ptr: *mut RustUserPresetBank,
    file_path: *const std::os::raw::c_char,
    name: *const std::os::raw::c_char,
    category: *const std::os::raw::c_char,
    author: *const std::os::raw::c_char,
    desc: *const std::os::raw::c_char,
    version: *const std::os::raw::c_char,
    tags_csv: *const std::os::raw::c_char,
    favorite: bool,
    json_content: *const std::os::raw::c_char,
) {
    if let Some(bank) = ptr.as_mut() {
        bank.add_preset(
            c_char_to_str(file_path),
            c_char_to_str(name),
            c_char_to_str(category),
            c_char_to_str(author),
            c_char_to_str(desc),
            c_char_to_str(version),
            c_char_to_str(tags_csv),
            favorite,
            c_char_to_str(json_content),
        );
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_user_preset_bank_sort(ptr: *mut RustUserPresetBank) {
    if let Some(bank) = ptr.as_mut() {
        bank.sort_alphabetical();
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_user_preset_bank_get_count(ptr: *const RustUserPresetBank) -> usize {
    if let Some(bank) = ptr.as_ref() {
        bank.get_preset_count()
    } else {
        0
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_user_preset_bank_delete(ptr: *mut RustUserPresetBank, index: usize) -> bool {
    if let Some(bank) = ptr.as_mut() {
        bank.delete_preset(index)
    } else {
        false
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_user_preset_bank_toggle_favorite(ptr: *mut RustUserPresetBank, index: usize) -> bool {
    if let Some(bank) = ptr.as_mut() {
        bank.toggle_favorite(index)
    } else {
        false
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_user_preset_bank_get_preset_info(
    ptr: *const RustUserPresetBank,
    index: usize,
    out_path: *mut std::os::raw::c_char,
    path_len: usize,
    out_name: *mut std::os::raw::c_char,
    name_len: usize,
    out_cat: *mut std::os::raw::c_char,
    cat_len: usize,
    out_author: *mut std::os::raw::c_char,
    author_len: usize,
    out_desc: *mut std::os::raw::c_char,
    desc_len: usize,
    out_tags: *mut std::os::raw::c_char,
    tags_len: usize,
    out_fav: *mut bool,
    out_json_len: *mut usize,
) -> bool {
    if let Some(bank) = ptr.as_ref() {
        if let Some(p) = bank.get_preset(index) {
            copy_str_to_c_buf(&p.file_path, out_path, path_len);
            copy_str_to_c_buf(&p.name, out_name, name_len);
            copy_str_to_c_buf(&p.category, out_cat, cat_len);
            copy_str_to_c_buf(&p.author, out_author, author_len);
            copy_str_to_c_buf(&p.description, out_desc, desc_len);
            let tags_csv = p.tags.join(",");
            copy_str_to_c_buf(&tags_csv, out_tags, tags_len);
            if !out_fav.is_null() {
                *out_fav = p.favorite;
            }
            if !out_json_len.is_null() {
                *out_json_len = p.json_content.len();
            }
            return true;
        }
    }
    false
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_user_preset_bank_get_preset_json(
    ptr: *const RustUserPresetBank,
    index: usize,
    out_json: *mut std::os::raw::c_char,
    max_len: usize,
) -> bool {
    if let Some(bank) = ptr.as_ref() {
        if let Some(p) = bank.get_preset(index) {
            copy_str_to_c_buf(&p.json_content, out_json, max_len);
            return true;
        }
    }
    false
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_user_preset_bank_update_preset_json(
    ptr: *mut RustUserPresetBank,
    index: usize,
    json_content: *const std::os::raw::c_char,
) -> bool {
    if let Some(bank) = ptr.as_mut() {
        bank.update_preset_json(index, c_char_to_str(json_content))
    } else {
        false
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_user_preset_bank_filter(
    ptr: *const RustUserPresetBank,
    query: *const std::os::raw::c_char,
    category: *const std::os::raw::c_char,
    tags_csv: *const std::os::raw::c_char,
    favorites_only: bool,
    out_indices: *mut usize,
    max_indices: usize,
) -> usize {
    if let Some(bank) = ptr.as_ref() {
        let matches = bank.filter(
            c_char_to_str(query),
            c_char_to_str(category),
            c_char_to_str(tags_csv),
            favorites_only,
        );
        let count = matches.len().min(max_indices);
        if !out_indices.is_null() && count > 0 {
            std::ptr::copy_nonoverlapping(matches.as_ptr(), out_indices, count);
        }
        matches.len()
    } else {
        0
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_user_preset_bank_get_tags_csv(
    ptr: *const RustUserPresetBank,
    out_buf: *mut std::os::raw::c_char,
    max_len: usize,
) -> usize {
    if let Some(bank) = ptr.as_ref() {
        let s = bank.get_all_tags_csv();
        copy_str_to_c_buf(&s, out_buf, max_len);
        s.len()
    } else {
        0
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_user_preset_bank_get_categories_csv(
    ptr: *const RustUserPresetBank,
    out_buf: *mut std::os::raw::c_char,
    max_len: usize,
) -> usize {
    if let Some(bank) = ptr.as_ref() {
        let s = bank.get_all_categories_csv();
        copy_str_to_c_buf(&s, out_buf, max_len);
        s.len()
    } else {
        0
    }
}

// =========================================================================
// Preset Packager C-ABI (Rules 21, 27, 47)
// =========================================================================
#[no_mangle]
pub unsafe extern "C" fn n8_rust_pack_builder_create(
    format: *const std::os::raw::c_char,
    pack_name: *const std::os::raw::c_char,
    author: *const std::os::raw::c_char,
    description: *const std::os::raw::c_char,
    version: *const std::os::raw::c_char,
    created_at: *const std::os::raw::c_char,
    tags_csv: *const std::os::raw::c_char,
) -> *mut RustPackBuilder {
    let builder = Box::new(RustPackBuilder::new(
        c_char_to_str(format),
        c_char_to_str(pack_name),
        c_char_to_str(author),
        c_char_to_str(description),
        c_char_to_str(version),
        c_char_to_str(created_at),
        c_char_to_str(tags_csv),
    ));
    Box::into_raw(builder)
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_pack_builder_destroy(ptr: *mut RustPackBuilder) {
    if !ptr.is_null() {
        drop(Box::from_raw(ptr));
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_pack_builder_add_preset(
    ptr: *mut RustPackBuilder,
    name: *const std::os::raw::c_char,
    category: *const std::os::raw::c_char,
    author: *const std::os::raw::c_char,
    content: *const std::os::raw::c_char,
) {
    if let Some(builder) = ptr.as_mut() {
        builder.add_preset(
            c_char_to_str(name),
            c_char_to_str(category),
            c_char_to_str(author),
            c_char_to_str(content),
        );
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_pack_builder_serialize(
    ptr: *const RustPackBuilder,
    out_buf: *mut std::os::raw::c_char,
    max_buf_len: usize,
) -> usize {
    if let Some(builder) = ptr.as_ref() {
        let json = builder.serialize();
        copy_str_to_c_buf(&json, out_buf, max_buf_len);
        json.len()
    } else {
        0
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_preset_packager_inspect_manifest(
    content: *const std::os::raw::c_char,
    out_format: *mut std::os::raw::c_char,
    format_len: usize,
    out_pack_version: *mut u32,
    out_name: *mut std::os::raw::c_char,
    name_len: usize,
    out_author: *mut std::os::raw::c_char,
    author_len: usize,
    out_desc: *mut std::os::raw::c_char,
    desc_len: usize,
    out_ver: *mut std::os::raw::c_char,
    ver_len: usize,
    out_created: *mut std::os::raw::c_char,
    created_len: usize,
    out_count: *mut usize,
    out_tags: *mut std::os::raw::c_char,
    tags_len: usize,
    out_err: *mut std::os::raw::c_char,
    err_len: usize,
) -> bool {
    let s = c_char_to_str(content);
    match RustPresetPackager::inspect_pack_manifest(s) {
        Ok(m) => {
            copy_str_to_c_buf(&m.format, out_format, format_len);
            if !out_pack_version.is_null() {
                *out_pack_version = m.pack_version;
            }
            copy_str_to_c_buf(&m.pack_name, out_name, name_len);
            copy_str_to_c_buf(&m.author, out_author, author_len);
            copy_str_to_c_buf(&m.description, out_desc, desc_len);
            copy_str_to_c_buf(&m.version, out_ver, ver_len);
            copy_str_to_c_buf(&m.created_at, out_created, created_len);
            if !out_count.is_null() {
                *out_count = m.preset_count;
            }
            let tags_csv = m.tags.join(",");
            copy_str_to_c_buf(&tags_csv, out_tags, tags_len);
            true
        }
        Err(e) => {
            copy_str_to_c_buf(&e, out_err, err_len);
            false
        }
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_pack_extractor_create(
    content: *const std::os::raw::c_char,
) -> *mut RustPackExtractor {
    let s = c_char_to_str(content);
    let extractor = Box::new(RustPackExtractor::new(s));
    Box::into_raw(extractor)
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_pack_extractor_destroy(ptr: *mut RustPackExtractor) {
    if !ptr.is_null() {
        drop(Box::from_raw(ptr));
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_pack_extractor_get_count(ptr: *const RustPackExtractor) -> usize {
    if let Some(ext) = ptr.as_ref() {
        ext.presets.len()
    } else {
        0
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_pack_extractor_get_preset_info(
    ptr: *const RustPackExtractor,
    index: usize,
    out_name: *mut std::os::raw::c_char,
    name_len: usize,
    out_cat: *mut std::os::raw::c_char,
    cat_len: usize,
    out_author: *mut std::os::raw::c_char,
    author_len: usize,
    out_content_len: *mut usize,
) -> bool {
    if let Some(ext) = ptr.as_ref() {
        if let Some(p) = ext.presets.get(index) {
            copy_str_to_c_buf(&p.name, out_name, name_len);
            copy_str_to_c_buf(&p.category, out_cat, cat_len);
            copy_str_to_c_buf(&p.author, out_author, author_len);
            if !out_content_len.is_null() {
                *out_content_len = p.content.len();
            }
            return true;
        }
    }
    false
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_pack_extractor_get_preset_content(
    ptr: *const RustPackExtractor,
    index: usize,
    out_buf: *mut std::os::raw::c_char,
    max_len: usize,
) -> bool {
    if let Some(ext) = ptr.as_ref() {
        if let Some(p) = ext.presets.get(index) {
            copy_str_to_c_buf(&p.content, out_buf, max_len);
            return true;
        }
    }
    false
}

// =========================================================================
// Node Automation Sequencer C-ABI (Rules 7, 8, 9, 25, 37, 46, 47)
// =========================================================================
#[no_mangle]
pub unsafe extern "C" fn n8_rust_node_automation_bank_create() -> *mut RustNodeAutomationBank {
    let bank = Box::new(RustNodeAutomationBank::new());
    Box::into_raw(bank)
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_node_automation_bank_destroy(ptr: *mut RustNodeAutomationBank) {
    if !ptr.is_null() {
        drop(Box::from_raw(ptr));
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_node_automation_bank_prepare(
    ptr: *mut RustNodeAutomationBank,
    sample_rate: f64,
) {
    if let Some(bank) = ptr.as_mut() {
        bank.prepare(sample_rate);
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_node_automation_bank_reset(ptr: *mut RustNodeAutomationBank) {
    if let Some(bank) = ptr.as_mut() {
        bank.reset();
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_node_automation_bank_apply_shape(
    ptr: *mut RustNodeAutomationBank,
    lane_idx: usize,
    shape: u8,
) {
    if let Some(bank) = ptr.as_mut() {
        bank.apply_shape(lane_idx, shape);
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_node_automation_bank_set_lane_config(
    ptr: *mut RustNodeAutomationBank,
    lane_idx: usize,
    active: bool,
    target_param_id: u32,
    num_steps: u32,
    rate: u8,
    glide: f32,
    amount: f32,
) {
    if let Some(bank) = ptr.as_mut() {
        if lane_idx < 4 {
            let lane = &mut bank.lanes[lane_idx];
            lane.active = active;
            lane.target_param_id = target_param_id;
            lane.num_steps = num_steps.clamp(1, 32);
            lane.rate = rate;
            lane.glide = glide.clamp(0.0, 1.0);
            lane.amount = amount.clamp(0.0, 1.0);
        }
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_node_automation_bank_get_lane_config(
    ptr: *const RustNodeAutomationBank,
    lane_idx: usize,
    out_active: *mut bool,
    out_target_param_id: *mut u32,
    out_num_steps: *mut u32,
    out_rate: *mut u8,
    out_glide: *mut f32,
    out_amount: *mut f32,
    out_current_step: *mut u32,
    out_live_val: *mut f32,
) {
    if let Some(bank) = ptr.as_ref() {
        if lane_idx < 4 {
            let lane = &bank.lanes[lane_idx];
            if !out_active.is_null() { *out_active = lane.active; }
            if !out_target_param_id.is_null() { *out_target_param_id = lane.target_param_id; }
            if !out_num_steps.is_null() { *out_num_steps = lane.num_steps; }
            if !out_rate.is_null() { *out_rate = lane.rate; }
            if !out_glide.is_null() { *out_glide = lane.glide; }
            if !out_amount.is_null() { *out_amount = lane.amount; }
            if !out_current_step.is_null() { *out_current_step = lane.current_step; }
            if !out_live_val.is_null() { *out_live_val = lane.live_modulated_val; }
        }
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_node_automation_bank_set_step(
    ptr: *mut RustNodeAutomationBank,
    lane_idx: usize,
    step_idx: usize,
    val: f32,
) {
    if let Some(bank) = ptr.as_mut() {
        if lane_idx < 4 && step_idx < 32 {
            bank.lanes[lane_idx].steps[step_idx] = val.clamp(0.0, 1.0);
        }
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_node_automation_bank_get_step(
    ptr: *const RustNodeAutomationBank,
    lane_idx: usize,
    step_idx: usize,
) -> f32 {
    if let Some(bank) = ptr.as_ref() {
        if lane_idx < 4 && step_idx < 32 {
            return bank.lanes[lane_idx].steps[step_idx];
        }
    }
    0.5
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_node_automation_bank_process_step(
    ptr: *mut RustNodeAutomationBank,
    lane_idx: usize,
    is_playing: bool,
    ppq_position: f64,
    base_val: f32,
    min_val: f32,
    max_val: f32,
    out_mod_val: *mut f32,
) -> bool {
    if let Some(bank) = ptr.as_mut() {
        let (val, active) = bank.process_step(lane_idx, is_playing, ppq_position, base_val, min_val, max_val);
        if !out_mod_val.is_null() {
            *out_mod_val = val;
        }
        return active;
    }
    false
}

// =========================================================================
// Midi Mapping Manager C-ABI (Rules 8, 9, 21, 46, 47)
// =========================================================================

#[no_mangle]
pub unsafe extern "C" fn n8_rust_midi_mapping_manager_create() -> *mut RustMidiMappingManager {
    Box::into_raw(Box::new(RustMidiMappingManager::new()))
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_midi_mapping_manager_destroy(ptr: *mut RustMidiMappingManager) {
    if !ptr.is_null() {
        drop(Box::from_raw(ptr));
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_midi_mapping_manager_reset(ptr: *mut RustMidiMappingManager) {
    if let Some(mgr) = ptr.as_mut() {
        mgr.reset();
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_midi_mapping_manager_start_learning(
    ptr: *mut RustMidiMappingManager,
    param_id: u32,
    target_node_id: u32,
    min_val: f32,
    max_val: f32,
    curve: u8,
    inverted: bool,
) {
    if let Some(mgr) = ptr.as_mut() {
        mgr.start_learning(param_id, target_node_id, min_val, max_val, curve, inverted);
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_midi_mapping_manager_cancel_learning(ptr: *mut RustMidiMappingManager) {
    if let Some(mgr) = ptr.as_mut() {
        mgr.cancel_learning();
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_midi_mapping_manager_is_learning(ptr: *const RustMidiMappingManager) -> bool {
    if let Some(mgr) = ptr.as_ref() {
        mgr.is_learning()
    } else {
        false
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_midi_mapping_manager_get_learn_target(
    ptr: *const RustMidiMappingManager,
    out_param_id: *mut u32,
    out_target_node_id: *mut u32,
    out_min_val: *mut f32,
    out_max_val: *mut f32,
    out_curve: *mut u8,
    out_inverted: *mut bool,
) {
    if let Some(mgr) = ptr.as_ref() {
        if !out_param_id.is_null() { *out_param_id = mgr.pending_target.param_id; }
        if !out_target_node_id.is_null() { *out_target_node_id = mgr.pending_target.target_node_id; }
        if !out_min_val.is_null() { *out_min_val = mgr.pending_target.min_val; }
        if !out_max_val.is_null() { *out_max_val = mgr.pending_target.max_val; }
        if !out_curve.is_null() { *out_curve = mgr.pending_target.curve as u8; }
        if !out_inverted.is_null() { *out_inverted = mgr.pending_target.inverted; }
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_midi_mapping_manager_add_mapping(
    ptr: *mut RustMidiMappingManager,
    cc_number: u8,
    channel: u8,
    param_id: u32,
    target_node_id: u32,
    min_val: f32,
    max_val: f32,
    inverted: bool,
    curve: u8,
    custom_label: *const c_char,
) -> bool {
    if let Some(mgr) = ptr.as_mut() {
        let label = if !custom_label.is_null() {
            CStr::from_ptr(custom_label).to_string_lossy().into_owned()
        } else {
            String::new()
        };
        let c = match curve {
            1 => RustMidiCurve::Exponential,
            2 => RustMidiCurve::Logarithmic,
            3 => RustMidiCurve::SShaped,
            _ => RustMidiCurve::Linear,
        };
        mgr.add_mapping(RustMidiMapping {
            cc_number,
            channel,
            param_id,
            target_node_id,
            min_val,
            max_val,
            inverted,
            curve: c,
            custom_label: label,
        })
    } else {
        false
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_midi_mapping_manager_remove_mapping(
    ptr: *mut RustMidiMappingManager,
    param_id: u32,
    target_node_id: u32,
) -> bool {
    if let Some(mgr) = ptr.as_mut() {
        mgr.remove_mapping(param_id, target_node_id)
    } else {
        false
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_midi_mapping_manager_remove_mapping_by_cc(
    ptr: *mut RustMidiMappingManager,
    cc_number: u8,
    channel: u8,
) {
    if let Some(mgr) = ptr.as_mut() {
        mgr.remove_mapping_by_cc(cc_number, channel);
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_midi_mapping_manager_get_count(ptr: *const RustMidiMappingManager) -> usize {
    if let Some(mgr) = ptr.as_ref() {
        mgr.mappings.len()
    } else {
        0
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_midi_mapping_manager_get_mapping(
    ptr: *const RustMidiMappingManager,
    index: usize,
    out_cc: *mut u8,
    out_channel: *mut u8,
    out_param_id: *mut u32,
    out_target_node_id: *mut u32,
    out_min_val: *mut f32,
    out_max_val: *mut f32,
    out_inverted: *mut bool,
    out_curve: *mut u8,
    out_label: *mut c_char,
    label_max_len: usize,
) -> bool {
    if let Some(mgr) = ptr.as_ref() {
        if index < mgr.mappings.len() {
            let m = &mgr.mappings[index];
            if !out_cc.is_null() { *out_cc = m.cc_number; }
            if !out_channel.is_null() { *out_channel = m.channel; }
            if !out_param_id.is_null() { *out_param_id = m.param_id; }
            if !out_target_node_id.is_null() { *out_target_node_id = m.target_node_id; }
            if !out_min_val.is_null() { *out_min_val = m.min_val; }
            if !out_max_val.is_null() { *out_max_val = m.max_val; }
            if !out_inverted.is_null() { *out_inverted = m.inverted; }
            if !out_curve.is_null() { *out_curve = m.curve as u8; }
            if !out_label.is_null() && label_max_len > 0 {
                let bytes = m.custom_label.as_bytes();
                let len = bytes.len().min(label_max_len - 1);
                std::ptr::copy_nonoverlapping(bytes.as_ptr(), out_label as *mut u8, len);
                *out_label.add(len) = 0;
            }
            return true;
        }
    }
    false
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_midi_mapping_manager_calculate_mapped_value(
    min_val: f32,
    max_val: f32,
    inverted: bool,
    curve: u8,
    cc_value: u8,
) -> f32 {
    let c = match curve {
        1 => RustMidiCurve::Exponential,
        2 => RustMidiCurve::Logarithmic,
        3 => RustMidiCurve::SShaped,
        _ => RustMidiCurve::Linear,
    };
    let map = RustMidiMapping {
        cc_number: 0,
        channel: 0,
        param_id: 0,
        target_node_id: 0,
        min_val,
        max_val,
        inverted,
        curve: c,
        custom_label: String::new(),
    };
    RustMidiMappingManager::calculate_mapped_value(&map, cc_value)
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_midi_mapping_manager_process_control_change(
    ptr: *mut RustMidiMappingManager,
    channel: u8,
    cc_number: u8,
    cc_value: u8,
    out_targets: *mut u32,
    out_param_ids: *mut u32,
    out_values: *mut f32,
    max_dispatches: usize,
) -> usize {
    if let Some(mgr) = ptr.as_mut() {
        let mut dispatches = [RustMidiDispatchItem {
            target_node_id: u32::MAX,
            param_id: 0,
            mapped_value: 0.0,
        }; 64];
        let n = mgr.process_control_change(channel, cc_number, cc_value, &mut dispatches);
        let write_n = n.min(max_dispatches);
        for i in 0..write_n {
            if !out_targets.is_null() {
                *out_targets.add(i) = dispatches[i].target_node_id;
            }
            if !out_param_ids.is_null() {
                *out_param_ids.add(i) = dispatches[i].param_id;
            }
            if !out_values.is_null() {
                *out_values.add(i) = dispatches[i].mapped_value;
            }
        }
        write_n
    } else {
        0
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_midi_mapping_manager_load_factory_profile(
    ptr: *mut RustMidiMappingManager,
    profile_name: *const c_char,
) {
    if let Some(mgr) = ptr.as_mut() {
        if !profile_name.is_null() {
            if let Ok(name) = CStr::from_ptr(profile_name).to_str() {
                mgr.load_factory_profile(name);
            }
        }
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_midi_mapping_manager_export_json(
    ptr: *const RustMidiMappingManager,
    profile_name: *const c_char,
    out_buf: *mut c_char,
    max_len: usize,
) -> usize {
    if let Some(mgr) = ptr.as_ref() {
        let name = if !profile_name.is_null() {
            CStr::from_ptr(profile_name).to_str().unwrap_or("Custom Hardware Profile")
        } else {
            "Custom Hardware Profile"
        };
        let json = mgr.export_json(name);
        let bytes = json.as_bytes();
        let len = bytes.len().min(if max_len > 0 { max_len - 1 } else { 0 });
        if !out_buf.is_null() && max_len > 0 {
            std::ptr::copy_nonoverlapping(bytes.as_ptr(), out_buf as *mut u8, len);
            *out_buf.add(len) = 0;
        }
        bytes.len()
    } else {
        0
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_midi_mapping_manager_import_json(
    ptr: *mut RustMidiMappingManager,
    json_str: *const c_char,
) -> bool {
    if let Some(mgr) = ptr.as_mut() {
        if !json_str.is_null() {
            if let Ok(s) = CStr::from_ptr(json_str).to_str() {
                return mgr.import_json(s);
            }
        }
    }
    false
}

// =========================================================================
// MPE Manager C-ABI (Rules 2, 3, 5, 8, 9, 14, 34, 46, 47)
// =========================================================================

#[no_mangle]
pub unsafe extern "C" fn n8_rust_mpe_manager_create() -> *mut RustMpeManager {
    Box::into_raw(Box::new(RustMpeManager::new()))
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_mpe_manager_destroy(ptr: *mut RustMpeManager) {
    if !ptr.is_null() {
        drop(Box::from_raw(ptr));
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_mpe_manager_prepare(ptr: *mut RustMpeManager, sample_rate: f64) {
    if let Some(mpe) = ptr.as_mut() {
        mpe.prepare(sample_rate);
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_mpe_manager_reset(ptr: *mut RustMpeManager) {
    if let Some(mpe) = ptr.as_mut() {
        mpe.reset();
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_mpe_manager_set_zone_mode(ptr: *mut RustMpeManager, mode: u8) {
    if let Some(mpe) = ptr.as_mut() {
        mpe.set_zone_mode(mode);
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_mpe_manager_get_zone_mode(ptr: *const RustMpeManager) -> u8 {
    if let Some(mpe) = ptr.as_ref() {
        mpe.get_zone_mode()
    } else {
        0
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_mpe_manager_set_per_note_pitch_bend_range(ptr: *mut RustMpeManager, semitones: f32) {
    if let Some(mpe) = ptr.as_mut() {
        mpe.set_per_note_pitch_bend_range(semitones);
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_mpe_manager_get_per_note_pitch_bend_range(ptr: *const RustMpeManager) -> f32 {
    if let Some(mpe) = ptr.as_ref() {
        mpe.get_per_note_pitch_bend_range()
    } else {
        48.0
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_mpe_manager_set_master_pitch_bend_range(ptr: *mut RustMpeManager, semitones: f32) {
    if let Some(mpe) = ptr.as_mut() {
        mpe.set_master_pitch_bend_range(semitones);
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_mpe_manager_get_master_pitch_bend_range(ptr: *const RustMpeManager) -> f32 {
    if let Some(mpe) = ptr.as_ref() {
        mpe.get_master_pitch_bend_range()
    } else {
        2.0
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_mpe_manager_get_active_voice_count(ptr: *const RustMpeManager) -> usize {
    if let Some(mpe) = ptr.as_ref() {
        mpe.get_active_voice_count()
    } else {
        0
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_mpe_manager_get_voice_info(
    ptr: *const RustMpeManager,
    channel: usize,
    out_is_active: *mut bool,
    out_note_number: *mut u8,
    out_strike_velocity: *mut f32,
    out_press: *mut f32,
    out_per_note_glide: *mut f32,
    out_master_glide: *mut f32,
    out_slide: *mut f32,
    out_lift_velocity: *mut f32,
    out_base_freq: *mut f32,
    out_current_freq: *mut f32,
    out_pitch_ratio: *mut f32,
    out_associated_event_id: *mut u32,
) -> bool {
    if let Some(mpe) = ptr.as_ref() {
        if channel < mpe_manager::MAX_MPE_CHANNELS {
            let v = &mpe.voices[channel];
            if !out_is_active.is_null() { *out_is_active = v.is_active; }
            if !out_note_number.is_null() { *out_note_number = v.note_number; }
            if !out_strike_velocity.is_null() { *out_strike_velocity = v.strike_velocity; }
            if !out_press.is_null() { *out_press = v.press; }
            if !out_per_note_glide.is_null() { *out_per_note_glide = v.per_note_glide_semitones; }
            if !out_master_glide.is_null() { *out_master_glide = v.master_glide_semitones; }
            if !out_slide.is_null() { *out_slide = v.slide; }
            if !out_lift_velocity.is_null() { *out_lift_velocity = v.lift_velocity; }
            if !out_base_freq.is_null() { *out_base_freq = v.base_frequency; }
            if !out_current_freq.is_null() { *out_current_freq = v.current_frequency; }
            if !out_pitch_ratio.is_null() { *out_pitch_ratio = v.pitch_ratio; }
            if !out_associated_event_id.is_null() { *out_associated_event_id = v.associated_event_id; }
            return true;
        }
    }
    false
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_mpe_manager_set_associated_event_id(
    ptr: *mut RustMpeManager,
    channel: usize,
    event_id: u32,
) {
    if let Some(mpe) = ptr.as_mut() {
        mpe.set_associated_event_id(channel, event_id);
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_mpe_manager_process_midi_message(
    ptr: *mut RustMpeManager,
    status: u8,
    data1: u8,
    data2: u8,
    out_action_type: *mut u8,
    out_channel: *mut u8,
    out_note: *mut u8,
    out_pitch_ratio: *mut f32,
    out_strike_velocity: *mut f32,
    out_pan: *mut f32,
    out_max_duration: *mut u64,
) -> bool {
    if let Some(mpe) = ptr.as_mut() {
        let action = mpe.process_midi_message(status, data1, data2);
        if !out_action_type.is_null() { *out_action_type = action.action_type as u8; }
        if !out_channel.is_null() { *out_channel = action.channel; }
        if !out_note.is_null() { *out_note = action.note; }
        if !out_pitch_ratio.is_null() { *out_pitch_ratio = action.pitch_ratio; }
        if !out_strike_velocity.is_null() { *out_strike_velocity = action.strike_velocity; }
        if !out_pan.is_null() { *out_pan = action.pan; }
        if !out_max_duration.is_null() { *out_max_duration = action.max_duration_samples; }
        return action.action_type != RustMpeActionType::None;
    }
    false
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_mpe_manager_midi_note_to_frequency(note: u8) -> f32 {
    RustMpeManager::midi_note_to_frequency(note)
}

// =========================================================================
// Preset Manager C-ABI (Rules 21, 22, 27, 46, 47)
// =========================================================================

#[no_mangle]
pub unsafe extern "C" fn n8_rust_preset_manager_create() -> *mut RustPresetManager {
    Box::into_raw(Box::new(RustPresetManager::new()))
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_preset_manager_destroy(ptr: *mut RustPresetManager) {
    if !ptr.is_null() {
        drop(Box::from_raw(ptr));
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_preset_manager_reset(ptr: *mut RustPresetManager) {
    if let Some(pm) = ptr.as_mut() {
        pm.reset();
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_preset_manager_store_slot_a(
    ptr: *mut RustPresetManager,
    name: *const c_char,
    json: *const c_char,
    macros: *const f32,
    timestamp: u64,
) {
    if let Some(pm) = ptr.as_mut() {
        let n = if !name.is_null() { CStr::from_ptr(name).to_str().unwrap_or("") } else { "" };
        let j = if !json.is_null() { CStr::from_ptr(json).to_str().unwrap_or("") } else { "" };
        let mut macs = [0.0; preset_manager::MACRO_COUNT];
        if !macros.is_null() {
            for i in 0..preset_manager::MACRO_COUNT {
                macs[i] = *macros.add(i);
            }
        }
        pm.store_slot_a(n, j, &macs, timestamp);
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_preset_manager_store_slot_b(
    ptr: *mut RustPresetManager,
    name: *const c_char,
    json: *const c_char,
    macros: *const f32,
    timestamp: u64,
) {
    if let Some(pm) = ptr.as_mut() {
        let n = if !name.is_null() { CStr::from_ptr(name).to_str().unwrap_or("") } else { "" };
        let j = if !json.is_null() { CStr::from_ptr(json).to_str().unwrap_or("") } else { "" };
        let mut macs = [0.0; preset_manager::MACRO_COUNT];
        if !macros.is_null() {
            for i in 0..preset_manager::MACRO_COUNT {
                macs[i] = *macros.add(i);
            }
        }
        pm.store_slot_b(n, j, &macs, timestamp);
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_preset_manager_get_slot_a(
    ptr: *const RustPresetManager,
    out_name: *mut c_char,
    name_max_len: usize,
    out_json: *mut c_char,
    json_max_len: usize,
    out_macros: *mut f32,
) -> bool {
    if let Some(pm) = ptr.as_ref() {
        if !pm.slot_a.name.is_empty() || !pm.slot_a.json_content.is_empty() {
            if !out_name.is_null() && name_max_len > 0 {
                let bytes = pm.slot_a.name.as_bytes();
                let len = bytes.len().min(name_max_len - 1);
                std::ptr::copy_nonoverlapping(bytes.as_ptr(), out_name as *mut u8, len);
                *out_name.add(len) = 0;
            }
            if !out_json.is_null() && json_max_len > 0 {
                let bytes = pm.slot_a.json_content.as_bytes();
                let len = bytes.len().min(json_max_len - 1);
                std::ptr::copy_nonoverlapping(bytes.as_ptr(), out_json as *mut u8, len);
                *out_json.add(len) = 0;
            }
            if !out_macros.is_null() {
                for i in 0..preset_manager::MACRO_COUNT {
                    *out_macros.add(i) = pm.slot_a.macros[i];
                }
            }
            return true;
        }
    }
    false
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_preset_manager_get_slot_b(
    ptr: *const RustPresetManager,
    out_name: *mut c_char,
    name_max_len: usize,
    out_json: *mut c_char,
    json_max_len: usize,
    out_macros: *mut f32,
) -> bool {
    if let Some(pm) = ptr.as_ref() {
        if !pm.slot_b.name.is_empty() || !pm.slot_b.json_content.is_empty() {
            if !out_name.is_null() && name_max_len > 0 {
                let bytes = pm.slot_b.name.as_bytes();
                let len = bytes.len().min(name_max_len - 1);
                std::ptr::copy_nonoverlapping(bytes.as_ptr(), out_name as *mut u8, len);
                *out_name.add(len) = 0;
            }
            if !out_json.is_null() && json_max_len > 0 {
                let bytes = pm.slot_b.json_content.as_bytes();
                let len = bytes.len().min(json_max_len - 1);
                std::ptr::copy_nonoverlapping(bytes.as_ptr(), out_json as *mut u8, len);
                *out_json.add(len) = 0;
            }
            if !out_macros.is_null() {
                for i in 0..preset_manager::MACRO_COUNT {
                    *out_macros.add(i) = pm.slot_b.macros[i];
                }
            }
            return true;
        }
    }
    false
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_preset_manager_set_active_slot(ptr: *mut RustPresetManager, slot: u8) {
    if let Some(pm) = ptr.as_mut() {
        pm.set_active_slot(if slot == 1 { RustPresetSlot::SlotB } else { RustPresetSlot::SlotA });
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_preset_manager_get_active_slot(ptr: *const RustPresetManager) -> u8 {
    if let Some(pm) = ptr.as_ref() {
        pm.get_active_slot() as u8
    } else {
        0
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_preset_manager_copy_a_to_b(ptr: *mut RustPresetManager) {
    if let Some(pm) = ptr.as_mut() {
        pm.copy_a_to_b();
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_preset_manager_copy_b_to_a(ptr: *mut RustPresetManager) {
    if let Some(pm) = ptr.as_mut() {
        pm.copy_b_to_a();
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_preset_manager_swap_slots(ptr: *mut RustPresetManager) {
    if let Some(pm) = ptr.as_mut() {
        pm.swap_slots();
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_preset_manager_set_morph_factor(ptr: *mut RustPresetManager, factor: f32) {
    if let Some(pm) = ptr.as_mut() {
        pm.set_morph_factor(factor);
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_preset_manager_get_morph_factor(ptr: *const RustPresetManager) -> f32 {
    if let Some(pm) = ptr.as_ref() {
        pm.get_morph_factor()
    } else {
        0.0
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_preset_manager_get_interpolated_macros(
    ptr: *const RustPresetManager,
    out_macros: *mut f32,
) {
    if let Some(pm) = ptr.as_ref() {
        if !out_macros.is_null() {
            let mut macs = [0.0; preset_manager::MACRO_COUNT];
            pm.get_interpolated_macros(&mut macs);
            for i in 0..preset_manager::MACRO_COUNT {
                *out_macros.add(i) = macs[i];
            }
        }
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_preset_manager_take_snapshot(
    ptr: *mut RustPresetManager,
    name: *const c_char,
    json: *const c_char,
    macros: *const f32,
    timestamp: u64,
) {
    if let Some(pm) = ptr.as_mut() {
        let n = if !name.is_null() { CStr::from_ptr(name).to_str().unwrap_or("") } else { "" };
        let j = if !json.is_null() { CStr::from_ptr(json).to_str().unwrap_or("") } else { "" };
        let mut macs = [0.0; preset_manager::MACRO_COUNT];
        if !macros.is_null() {
            for i in 0..preset_manager::MACRO_COUNT {
                macs[i] = *macros.add(i);
            }
        }
        pm.take_snapshot(n, j, &macs, timestamp);
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_preset_manager_get_snapshot_count(ptr: *const RustPresetManager) -> usize {
    if let Some(pm) = ptr.as_ref() {
        pm.get_snapshot_count()
    } else {
        0
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_preset_manager_get_snapshot(
    ptr: *const RustPresetManager,
    index: usize,
    out_name: *mut c_char,
    name_max_len: usize,
    out_json: *mut c_char,
    json_max_len: usize,
    out_macros: *mut f32,
) -> bool {
    if let Some(pm) = ptr.as_ref() {
        if let Some(s) = pm.get_snapshot(index) {
            if !out_name.is_null() && name_max_len > 0 {
                let bytes = s.name.as_bytes();
                let len = bytes.len().min(name_max_len - 1);
                std::ptr::copy_nonoverlapping(bytes.as_ptr(), out_name as *mut u8, len);
                *out_name.add(len) = 0;
            }
            if !out_json.is_null() && json_max_len > 0 {
                let bytes = s.json_content.as_bytes();
                let len = bytes.len().min(json_max_len - 1);
                std::ptr::copy_nonoverlapping(bytes.as_ptr(), out_json as *mut u8, len);
                *out_json.add(len) = 0;
            }
            if !out_macros.is_null() {
                for i in 0..preset_manager::MACRO_COUNT {
                    *out_macros.add(i) = s.macros[i];
                }
            }
            return true;
        }
    }
    false
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_preset_manager_save_user_preset(
    ptr: *mut RustPresetManager,
    name: *const c_char,
    json: *const c_char,
    macros: *const f32,
) {
    if let Some(pm) = ptr.as_mut() {
        let n = if !name.is_null() { CStr::from_ptr(name).to_str().unwrap_or("") } else { "" };
        let j = if !json.is_null() { CStr::from_ptr(json).to_str().unwrap_or("") } else { "" };
        let mut macs = [0.0; preset_manager::MACRO_COUNT];
        if !macros.is_null() {
            for i in 0..preset_manager::MACRO_COUNT {
                macs[i] = *macros.add(i);
            }
        }
        pm.save_user_preset(n, j, &macs);
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_preset_manager_load_user_preset(
    ptr: *const RustPresetManager,
    name: *const c_char,
    out_json: *mut c_char,
    json_max_len: usize,
    out_macros: *mut f32,
) -> bool {
    if let Some(pm) = ptr.as_ref() {
        if !name.is_null() {
            if let Ok(n) = CStr::from_ptr(name).to_str() {
                if let Some(s) = pm.load_user_preset(n) {
                    if !out_json.is_null() && json_max_len > 0 {
                        let bytes = s.json_content.as_bytes();
                        let len = bytes.len().min(json_max_len - 1);
                        std::ptr::copy_nonoverlapping(bytes.as_ptr(), out_json as *mut u8, len);
                        *out_json.add(len) = 0;
                    }
                    if !out_macros.is_null() {
                        for i in 0..preset_manager::MACRO_COUNT {
                            *out_macros.add(i) = s.macros[i];
                        }
                    }
                    return true;
                }
            }
        }
    }
    false
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_preset_manager_get_user_preset_count(ptr: *const RustPresetManager) -> usize {
    if let Some(pm) = ptr.as_ref() {
        pm.get_user_preset_count()
    } else {
        0
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_preset_manager_get_user_preset_name(
    ptr: *const RustPresetManager,
    index: usize,
    out_name: *mut c_char,
    name_max_len: usize,
) -> bool {
    if let Some(pm) = ptr.as_ref() {
        if let Some(p) = pm.get_user_preset_by_index(index) {
            if !out_name.is_null() && name_max_len > 0 {
                let bytes = p.name.as_bytes();
                let len = bytes.len().min(name_max_len - 1);
                std::ptr::copy_nonoverlapping(bytes.as_ptr(), out_name as *mut u8, len);
                *out_name.add(len) = 0;
            }
            return true;
        }
    }
    false
}

// =========================================================================
// Graph Serializer C-ABI (Rules 21, 22, 28, 46, 47)
// =========================================================================

#[no_mangle]
pub unsafe extern "C" fn n8_rust_graph_serializer_escape(
    src: *const c_char,
    out_buf: *mut c_char,
    max_len: usize,
) -> usize {
    if !src.is_null() {
        if let Ok(s) = CStr::from_ptr(src).to_str() {
            let esc = RustGraphSerializer::escape_json(s);
            let bytes = esc.as_bytes();
            let len = bytes.len().min(if max_len > 0 { max_len - 1 } else { 0 });
            if !out_buf.is_null() && max_len > 0 {
                std::ptr::copy_nonoverlapping(bytes.as_ptr(), out_buf as *mut u8, len);
                *out_buf.add(len) = 0;
            }
            return bytes.len();
        }
    }
    0
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_graph_serializer_extract_string(
    json: *const c_char,
    key: *const c_char,
    def_val: *const c_char,
    out_buf: *mut c_char,
    max_len: usize,
) -> usize {
    if !json.is_null() && !key.is_null() {
        let j = CStr::from_ptr(json).to_str().unwrap_or("");
        let k = CStr::from_ptr(key).to_str().unwrap_or("");
        let def = if !def_val.is_null() { CStr::from_ptr(def_val).to_str().unwrap_or("") } else { "" };
        let s = RustGraphSerializer::extract_string(j, k, def);
        let bytes = s.as_bytes();
        let len = bytes.len().min(if max_len > 0 { max_len - 1 } else { 0 });
        if !out_buf.is_null() && max_len > 0 {
            std::ptr::copy_nonoverlapping(bytes.as_ptr(), out_buf as *mut u8, len);
            *out_buf.add(len) = 0;
        }
        return bytes.len();
    }
    0
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_graph_serializer_extract_uint(
    json: *const c_char,
    key: *const c_char,
    def_val: u32,
) -> u32 {
    if !json.is_null() && !key.is_null() {
        let j = CStr::from_ptr(json).to_str().unwrap_or("");
        let k = CStr::from_ptr(key).to_str().unwrap_or("");
        return RustGraphSerializer::extract_uint(j, k, def_val);
    }
    def_val
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_graph_serializer_extract_float(
    json: *const c_char,
    key: *const c_char,
    def_val: f32,
) -> f32 {
    if !json.is_null() && !key.is_null() {
        let j = CStr::from_ptr(json).to_str().unwrap_or("");
        let k = CStr::from_ptr(key).to_str().unwrap_or("");
        return RustGraphSerializer::extract_float(j, k, def_val);
    }
    def_val
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_graph_serializer_extract_array_section(
    json: *const c_char,
    key: *const c_char,
    out_buf: *mut c_char,
    max_len: usize,
) -> usize {
    if !json.is_null() && !key.is_null() {
        let j = CStr::from_ptr(json).to_str().unwrap_or("");
        let k = CStr::from_ptr(key).to_str().unwrap_or("");
        if let Some(sec) = RustGraphSerializer::extract_array_section(j, k) {
            let bytes = sec.as_bytes();
            let len = bytes.len().min(if max_len > 0 { max_len - 1 } else { 0 });
            if !out_buf.is_null() && max_len > 0 {
                std::ptr::copy_nonoverlapping(bytes.as_ptr(), out_buf as *mut u8, len);
                *out_buf.add(len) = 0;
            }
            return bytes.len();
        }
    }
    0
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_graph_serializer_extract_object_section(
    json: *const c_char,
    key: *const c_char,
    out_buf: *mut c_char,
    max_len: usize,
) -> usize {
    if !json.is_null() && !key.is_null() {
        let j = CStr::from_ptr(json).to_str().unwrap_or("");
        let k = CStr::from_ptr(key).to_str().unwrap_or("");
        if let Some(sec) = RustGraphSerializer::extract_object_section(j, k) {
            let bytes = sec.as_bytes();
            let len = bytes.len().min(if max_len > 0 { max_len - 1 } else { 0 });
            if !out_buf.is_null() && max_len > 0 {
                std::ptr::copy_nonoverlapping(bytes.as_ptr(), out_buf as *mut u8, len);
                *out_buf.add(len) = 0;
            }
            return bytes.len();
        }
    }
    0
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_graph_serializer_parse_number_list(
    text: *const c_char,
    out_buf: *mut f32,
    max_len: usize,
) -> usize {
    if !text.is_null() {
        if let Ok(t) = CStr::from_ptr(text).to_str() {
            let nums = RustGraphSerializer::parse_number_list(t);
            let count = nums.len().min(max_len);
            if !out_buf.is_null() {
                for i in 0..count {
                    *out_buf.add(i) = nums[i];
                }
            }
            return nums.len();
        }
    }
    0
}

// =========================================================================
// Factory Preset Catalog C-ABI (Rules 4, 19, 20, 21, 22, 46)
// =========================================================================

#[inline]
fn catalog_buf_len(m: usize) -> usize {
    if m > 0 { m - 1 } else { 0 }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_factory_preset_catalog_create() -> *mut RustFactoryPresetCatalog {
    Box::into_raw(Box::new(RustFactoryPresetCatalog::new()))
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_factory_preset_catalog_destroy(ptr: *mut RustFactoryPresetCatalog) {
    if !ptr.is_null() {
        drop(Box::from_raw(ptr));
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_factory_preset_catalog_reset(ptr: *mut RustFactoryPresetCatalog) {
    if let Some(cat) = ptr.as_mut() {
        cat.reset();
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_factory_preset_catalog_register_preset(
    ptr: *mut RustFactoryPresetCatalog,
    name: *const c_char,
    category: *const c_char,
    json: *const c_char,
) {
    if let Some(cat) = ptr.as_mut() {
        let n = if !name.is_null() { CStr::from_ptr(name).to_str().unwrap_or("") } else { "" };
        let c = if !category.is_null() { CStr::from_ptr(category).to_str().unwrap_or("") } else { "" };
        let j = if !json.is_null() { CStr::from_ptr(json).to_str().unwrap_or("") } else { "" };
        cat.register_preset(n, c, j);
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_factory_preset_catalog_get_count(ptr: *const RustFactoryPresetCatalog) -> usize {
    if let Some(cat) = ptr.as_ref() {
        cat.get_preset_count()
    } else {
        0
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_factory_preset_catalog_get_preset_name(
    ptr: *const RustFactoryPresetCatalog,
    index: usize,
    out_buf: *mut c_char,
    max_len: usize,
) -> bool {
    if let Some(cat) = ptr.as_ref() {
        if let Some(p) = cat.get_preset(index) {
            if !out_buf.is_null() && max_len > 0 {
                let bytes = p.name.as_bytes();
                let len = bytes.len().min(catalog_buf_len(max_len));
                std::ptr::copy_nonoverlapping(bytes.as_ptr(), out_buf as *mut u8, len);
                *out_buf.add(len) = 0;
            }
            return true;
        }
    }
    false
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_factory_preset_catalog_get_preset_category(
    ptr: *const RustFactoryPresetCatalog,
    index: usize,
    out_buf: *mut c_char,
    max_len: usize,
) -> bool {
    if let Some(cat) = ptr.as_ref() {
        if let Some(p) = cat.get_preset(index) {
            if !out_buf.is_null() && max_len > 0 {
                let bytes = p.category.as_bytes();
                let len = bytes.len().min(catalog_buf_len(max_len));
                std::ptr::copy_nonoverlapping(bytes.as_ptr(), out_buf as *mut u8, len);
                *out_buf.add(len) = 0;
            }
            return true;
        }
    }
    false
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_factory_preset_catalog_get_preset_json(
    ptr: *const RustFactoryPresetCatalog,
    index: usize,
    out_buf: *mut c_char,
    max_len: usize,
) -> usize {
    if let Some(cat) = ptr.as_ref() {
        if let Some(p) = cat.get_preset(index) {
            let bytes = p.json_content.as_bytes();
            let len = bytes.len().min(catalog_buf_len(max_len));
            if !out_buf.is_null() && max_len > 0 {
                std::ptr::copy_nonoverlapping(bytes.as_ptr(), out_buf as *mut u8, len);
                *out_buf.add(len) = 0;
            }
            return bytes.len();
        }
    }
    0
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_factory_preset_catalog_get_category_count(ptr: *const RustFactoryPresetCatalog) -> usize {
    if let Some(cat) = ptr.as_ref() {
        cat.get_category_count()
    } else {
        0
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_factory_preset_catalog_get_category_name(
    ptr: *const RustFactoryPresetCatalog,
    index: usize,
    out_buf: *mut c_char,
    max_len: usize,
) -> bool {
    if let Some(cat) = ptr.as_ref() {
        if let Some(c) = cat.get_category(index) {
            if !out_buf.is_null() && max_len > 0 {
                let bytes = c.as_bytes();
                let len = bytes.len().min(catalog_buf_len(max_len));
                std::ptr::copy_nonoverlapping(bytes.as_ptr(), out_buf as *mut u8, len);
                *out_buf.add(len) = 0;
            }
            return true;
        }
    }
    false
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_factory_preset_catalog_search(
    ptr: *const RustFactoryPresetCatalog,
    query: *const c_char,
    out_indices: *mut usize,
    max_results: usize,
) -> usize {
    if let Some(cat) = ptr.as_ref() {
        let q = if !query.is_null() { CStr::from_ptr(query).to_str().unwrap_or("") } else { "" };
        let results = cat.search(q);
        let count = results.len().min(max_results);
        if !out_indices.is_null() {
            for i in 0..count {
                *out_indices.add(i) = results[i];
            }
        }
        return results.len();
    }
    0
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_factory_preset_catalog_filter_by_category(
    ptr: *const RustFactoryPresetCatalog,
    category: *const c_char,
    out_indices: *mut usize,
    max_results: usize,
) -> usize {
    if let Some(cat) = ptr.as_ref() {
        let c = if !category.is_null() { CStr::from_ptr(category).to_str().unwrap_or("") } else { "" };
        let results = cat.filter_by_category(c);
        let count = results.len().min(max_results);
        if !out_indices.is_null() {
            for i in 0..count {
                *out_indices.add(i) = results[i];
            }
        }
        return results.len();
    }
    0
}

// =========================================================================
// Work-Stealing Queue C-ABI (Rules 4, 9, 10, 26, 47)
// =========================================================================

#[no_mangle]
pub unsafe extern "C" fn n8_rust_work_stealing_queue_create(capacity: u32) -> *mut RustWorkStealingQueue {
    Box::into_raw(Box::new(RustWorkStealingQueue::new(capacity as usize)))
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_work_stealing_queue_destroy(ptr: *mut RustWorkStealingQueue) {
    if !ptr.is_null() {
        drop(Box::from_raw(ptr));
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_work_stealing_queue_push(ptr: *mut RustWorkStealingQueue, item: u32) -> bool {
    if let Some(q) = ptr.as_ref() {
        q.push(item)
    } else {
        false
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_work_stealing_queue_pop(ptr: *mut RustWorkStealingQueue, out_item: *mut u32) -> bool {
    if let Some(q) = ptr.as_ref() {
        let (popped, val) = q.pop();
        if popped {
            if !out_item.is_null() {
                *out_item = val;
            }
            return true;
        }
    }
    false
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_work_stealing_queue_steal(ptr: *mut RustWorkStealingQueue, out_item: *mut u32) -> bool {
    if let Some(q) = ptr.as_ref() {
        let (stolen, val) = q.steal();
        if stolen {
            if !out_item.is_null() {
                *out_item = val;
            }
            return true;
        }
    }
    false
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_work_stealing_queue_empty(ptr: *const RustWorkStealingQueue) -> bool {
    if let Some(q) = ptr.as_ref() {
        q.is_empty()
    } else {
        true
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_work_stealing_queue_clear(ptr: *mut RustWorkStealingQueue) {
    if let Some(q) = ptr.as_ref() {
        q.clear();
    }
}

// =========================================================================
// Work-Stealing Graph Scheduler C-ABI (Rules 4, 9, 10, 26, 47)
// =========================================================================

#[no_mangle]
pub unsafe extern "C" fn n8_rust_work_stealing_scheduler_create() -> *mut RustWorkStealingScheduler {
    Box::into_raw(Box::new(RustWorkStealingScheduler::new()))
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_work_stealing_scheduler_destroy(ptr: *mut RustWorkStealingScheduler) {
    if !ptr.is_null() {
        drop(Box::from_raw(ptr));
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_work_stealing_scheduler_prepare(ptr: *mut RustWorkStealingScheduler, num_workers: u32) {
    if let Some(s) = ptr.as_mut() {
        s.prepare(num_workers);
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_work_stealing_scheduler_clear_queues(ptr: *mut RustWorkStealingScheduler) {
    if let Some(s) = ptr.as_ref() {
        s.clear_all_queues();
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_work_stealing_scheduler_init_step(
    ptr: *mut RustWorkStealingScheduler,
    step_idx: usize,
    dep_count: i32,
    successors: *const u32,
    num_successors: usize,
) {
    if let Some(s) = ptr.as_mut() {
        let succs = if !successors.is_null() && num_successors > 0 {
            std::slice::from_raw_parts(successors, num_successors)
        } else {
            &[]
        };
        s.init_step(step_idx, dep_count, succs);
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_work_stealing_scheduler_start_execution(
    ptr: *mut RustWorkStealingScheduler,
    total_steps: usize,
) {
    if let Some(s) = ptr.as_mut() {
        s.start_execution(total_steps);
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_work_stealing_scheduler_pop_or_steal(
    ptr: *mut RustWorkStealingScheduler,
    worker_id: u32,
    out_step_idx: *mut u32,
) -> bool {
    if let Some(s) = ptr.as_ref() {
        if let Some(step) = s.pop_or_steal(worker_id as usize) {
            if !out_step_idx.is_null() {
                *out_step_idx = step;
            }
            return true;
        }
    }
    false
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_work_stealing_scheduler_finish_step(
    ptr: *mut RustWorkStealingScheduler,
    worker_id: u32,
    step_idx: u32,
) {
    if let Some(s) = ptr.as_ref() {
        s.finish_step(worker_id as usize, step_idx as usize);
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_work_stealing_scheduler_get_steps_remaining(
    ptr: *const RustWorkStealingScheduler,
) -> i32 {
    if let Some(s) = ptr.as_ref() {
        s.get_steps_remaining()
    } else {
        0
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_work_stealing_scheduler_get_num_workers(
    ptr: *const RustWorkStealingScheduler,
) -> u32 {
    if let Some(s) = ptr.as_ref() {
        s.get_num_workers()
    } else {
        0
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_work_stealing_scheduler_increment_epoch(
    ptr: *mut RustWorkStealingScheduler,
) -> u64 {
    if let Some(s) = ptr.as_ref() {
        s.increment_epoch()
    } else {
        0
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_work_stealing_scheduler_get_epoch(
    ptr: *const RustWorkStealingScheduler,
) -> u64 {
    if let Some(s) = ptr.as_ref() {
        s.get_epoch()
    } else {
        0
    }
}

// =========================================================================
// Adaptive Noise Floor Estimator C-ABI (Rules 1, 3, 9, 27, 47)
// =========================================================================

#[no_mangle]
pub unsafe extern "C" fn n8_rust_adaptive_noise_floor_estimator_create() -> *mut RustAdaptiveNoiseFloorEstimator {
    Box::into_raw(Box::new(RustAdaptiveNoiseFloorEstimator::new()))
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_adaptive_noise_floor_estimator_destroy(ptr: *mut RustAdaptiveNoiseFloorEstimator) {
    if !ptr.is_null() {
        drop(Box::from_raw(ptr));
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_adaptive_noise_floor_estimator_prepare(
    ptr: *mut RustAdaptiveNoiseFloorEstimator,
    sample_rate: f64,
) {
    if let Some(est) = ptr.as_mut() {
        est.prepare(sample_rate);
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_adaptive_noise_floor_estimator_reset(ptr: *mut RustAdaptiveNoiseFloorEstimator) {
    if let Some(est) = ptr.as_mut() {
        est.reset();
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_adaptive_noise_floor_estimator_update_block(
    ptr: *mut RustAdaptiveNoiseFloorEstimator,
    block_rms: f32,
    num_samples: u32,
) {
    if let Some(est) = ptr.as_mut() {
        est.update_block(block_rms, num_samples);
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_adaptive_noise_floor_estimator_is_source_active(
    ptr: *const RustAdaptiveNoiseFloorEstimator,
) -> bool {
    if let Some(est) = ptr.as_ref() {
        est.is_source_active()
    } else {
        false
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_adaptive_noise_floor_estimator_is_source_silent(
    ptr: *const RustAdaptiveNoiseFloorEstimator,
) -> bool {
    if let Some(est) = ptr.as_ref() {
        est.is_source_silent()
    } else {
        true
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_adaptive_noise_floor_estimator_get_noise_floor(
    ptr: *const RustAdaptiveNoiseFloorEstimator,
) -> f32 {
    if let Some(est) = ptr.as_ref() {
        est.get_noise_floor()
    } else {
        adaptive_noise_floor_estimator::DEFAULT_FLOOR
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_adaptive_noise_floor_estimator_get_smoothed_level(
    ptr: *const RustAdaptiveNoiseFloorEstimator,
) -> f32 {
    if let Some(est) = ptr.as_ref() {
        est.get_smoothed_level()
    } else {
        0.0
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_adaptive_noise_floor_estimator_set_debounce_blocks(
    ptr: *mut RustAdaptiveNoiseFloorEstimator,
    blocks: u32,
) {
    if let Some(est) = ptr.as_mut() {
        est.set_debounce_blocks(blocks);
    }
}

// =========================================================================
// Delay Line C-ABI (Rules 11, 12, 32, 34, 46, 47)
// =========================================================================

#[no_mangle]
pub unsafe extern "C" fn n8_rust_delay_line_create() -> *mut RustDelayLine {
    Box::into_raw(Box::new(RustDelayLine::new()))
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_delay_line_destroy(ptr: *mut RustDelayLine) {
    if !ptr.is_null() {
        drop(Box::from_raw(ptr));
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_delay_line_prepare(ptr: *mut RustDelayLine, max_delay: usize) {
    if let Some(d) = ptr.as_mut() {
        d.prepare(max_delay);
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_delay_line_reset(ptr: *mut RustDelayLine) {
    if let Some(d) = ptr.as_mut() {
        d.reset();
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_delay_line_write(ptr: *mut RustDelayLine, sample: f32) {
    if let Some(d) = ptr.as_mut() {
        d.write(sample);
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_delay_line_read_linear(ptr: *const RustDelayLine, delay: f32) -> f32 {
    if let Some(d) = ptr.as_ref() {
        d.read_linear(delay)
    } else {
        0.0
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_delay_line_read_cubic(ptr: *const RustDelayLine, delay: f32) -> f32 {
    if let Some(d) = ptr.as_ref() {
        d.read_cubic(delay)
    } else {
        0.0
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_delay_line_get_capacity(ptr: *const RustDelayLine) -> usize {
    if let Some(d) = ptr.as_ref() {
        d.get_capacity()
    } else {
        0
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_delay_line_get_mask(ptr: *const RustDelayLine) -> usize {
    if let Some(d) = ptr.as_ref() {
        d.get_mask()
    } else {
        0
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_delay_line_get_max_delay_samples(ptr: *const RustDelayLine) -> usize {
    if let Some(d) = ptr.as_ref() {
        d.get_max_delay_samples()
    } else {
        0
    }
}

// =========================================================================
// Linkwitz-Riley LR4 3-Way Crossover C-ABI (Rules 32, 33, 34, 46, 47)
// =========================================================================

#[no_mangle]
pub unsafe extern "C" fn n8_rust_lr_crossover_create() -> *mut RustLinkwitzRileyCrossover3Way {
    Box::into_raw(Box::new(RustLinkwitzRileyCrossover3Way::new()))
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_lr_crossover_destroy(ptr: *mut RustLinkwitzRileyCrossover3Way) {
    if !ptr.is_null() {
        drop(Box::from_raw(ptr));
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_lr_crossover_prepare(ptr: *mut RustLinkwitzRileyCrossover3Way, sample_rate: f64) {
    if let Some(lr) = ptr.as_mut() {
        lr.prepare(sample_rate);
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_lr_crossover_reset(ptr: *mut RustLinkwitzRileyCrossover3Way) {
    if let Some(lr) = ptr.as_mut() {
        lr.reset();
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_lr_crossover_set_crossover_frequencies(
    ptr: *mut RustLinkwitzRileyCrossover3Way,
    f_low_mid: f32,
    f_mid_high: f32,
) {
    if let Some(lr) = ptr.as_mut() {
        lr.set_crossover_frequencies(f_low_mid, f_mid_high);
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_lr_crossover_process_sample(
    ptr: *mut RustLinkwitzRileyCrossover3Way,
    ch: usize,
    in_sample: f32,
    out_low: *mut f32,
    out_mid: *mut f32,
    out_high: *mut f32,
) {
    if let Some(lr) = ptr.as_mut() {
        let (l, m, h) = lr.process_sample(ch, in_sample);
        if !out_low.is_null() { *out_low = l; }
        if !out_mid.is_null() { *out_mid = m; }
        if !out_high.is_null() { *out_high = h; }
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_lr_crossover_process_block(
    ptr: *mut RustLinkwitzRileyCrossover3Way,
    in_l: *const f32,
    in_r: *const f32,
    low_l: *mut f32,
    low_r: *mut f32,
    mid_l: *mut f32,
    mid_r: *mut f32,
    high_l: *mut f32,
    high_r: *mut f32,
    num_samples: usize,
) {
    if let Some(lr) = ptr.as_mut() {
        lr.process_block(in_l, in_r, low_l, low_r, mid_l, mid_r, high_l, high_r, num_samples);
    }
}

// =========================================================================
// ContainerNode Core C-ABI (Rules 5, 6, 9, 28, 46, 47)
// =========================================================================

#[no_mangle]
pub unsafe extern "C" fn n8_rust_container_create(sample_rate: f32) -> *mut RustContainerCore {
    Box::into_raw(Box::new(RustContainerCore::new(sample_rate)))
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_container_free(ptr: *mut RustContainerCore) {
    if !ptr.is_null() {
        drop(Box::from_raw(ptr));
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_container_prepare(
    ptr: *mut RustContainerCore,
    sample_rate: f32,
    max_block_size: usize,
) {
    if let Some(c) = ptr.as_mut() {
        c.prepare(sample_rate, max_block_size);
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_container_reset(ptr: *mut RustContainerCore) {
    if let Some(c) = ptr.as_mut() {
        c.reset();
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_container_set_parameter(
    ptr: *mut RustContainerCore,
    param_id: u32,
    value: f32,
) {
    if let Some(c) = ptr.as_mut() {
        c.set_parameter(param_id, value);
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_container_get_parameter(
    ptr: *const RustContainerCore,
    param_id: u32,
) -> f32 {
    if let Some(c) = ptr.as_ref() {
        c.get_parameter(param_id)
    } else {
        0.0
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_container_process_passthrough(
    ptr: *mut RustContainerCore,
    in_l: *const f32,
    in_r: *const f32,
    out_l: *mut f32,
    out_r: *mut f32,
    num_samples: usize,
) {
    if let Some(c) = ptr.as_mut() {
        c.process_passthrough(in_l, in_r, out_l, out_r, num_samples);
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_container_process_mix(
    ptr: *mut RustContainerCore,
    dry_l: *const f32,
    dry_r: *const f32,
    wet_l: *const f32,
    wet_r: *const f32,
    out_l: *mut f32,
    out_r: *mut f32,
    num_samples: usize,
) {
    if let Some(c) = ptr.as_mut() {
        c.process_mix(dry_l, dry_r, wet_l, wet_r, out_l, out_r, num_samples);
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_container_process_channels_passthrough(
    ptr: *mut RustContainerCore,
    in_ptrs: *const *const f32,
    num_in: usize,
    out_ptrs: *mut *mut f32,
    num_out: usize,
    num_samples: usize,
) {
    if let Some(c) = ptr.as_mut() {
        c.process_channels_passthrough(in_ptrs, num_in, out_ptrs, num_out, num_samples);
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_container_process_channels_mix(
    ptr: *mut RustContainerCore,
    dry_ptrs: *const *const f32,
    num_dry: usize,
    wet_ptrs: *const *const f32,
    num_wet: usize,
    out_ptrs: *mut *mut f32,
    num_out: usize,
    num_samples: usize,
) {
    if let Some(c) = ptr.as_mut() {
        c.process_channels_mix(dry_ptrs, num_dry, wet_ptrs, num_wet, out_ptrs, num_out, num_samples);
    }
}

#[no_mangle]
pub unsafe extern "C" fn n8_rust_container_validate_subgraph(
    node_ids: *const u32,
    num_nodes: usize,
    connections: *const crate::graph_engine::RustGraphConnection,
    num_connections: usize,
) -> bool {
    if node_ids.is_null() || num_nodes == 0 {
        return true;
    }
    let nodes_slice = std::slice::from_raw_parts(node_ids, num_nodes);
    let conns_slice = if connections.is_null() || num_connections == 0 {
        &[]
    } else {
        std::slice::from_raw_parts(connections, num_connections)
    };
    RustContainerCore::validate_subgraph(nodes_slice, conns_slice)
}


