//! RustProcessorNode & RustProcessorRegistry (Phase 3, Rules 5, 19, 20, 46, 47)
//! High-performance Rust-native processor abstraction, O(1) registry dispatch,
//! zero heap allocations during audio processing thread execution.

use std::ffi::c_void;
use crate::passthrough::RustPassthrough;
use crate::simple_filter::RustSimpleFilter;
use crate::simple_delay::RustSimpleDelay;
use crate::container_node::RustContainerCore;
use crate::distortion::RustDistortion;
use crate::reverb::RustReverb;
use crate::compressor::RustCompressor;
use crate::phaser::RustPhaser;
use crate::chorus::RustChorus;
use crate::flanger::RustFlanger;
use crate::ring_modulator::RustRingModulator;
use crate::frequency_shifter::RustFrequencyShifter;
use crate::tape_saturation::RustTapeSaturation;
use crate::mid_side_encoder::RustMidSideEncoder;
use crate::mid_side_decoder::RustMidSideDecoder;
use crate::spatial_panner::RustSpatialPanner;
use crate::parametric_eq::RustParametricEq;
use crate::advanced_delay::RustAdvancedDelay;
use crate::tape_stop::RustTapeStop;
use crate::granular::RustGranular;
use crate::pitch_shifter::RustPitchShifter;
use crate::spectral_processor::RustSpectralProcessor;
use crate::resonator_bank::RustResonatorBank;
use crate::glitch::RustGlitch;
use crate::multiband_dynamics::RustMultibandDynamics;
use crate::feedback_container::FeedbackContainerCore;
use crate::event_container::EventContainerCore;
use crate::formant_filter::RustFormantFilter;
use crate::noise_texture::RustNoiseTexture;
use crate::transient_shaper::RustTransientShaper;
use crate::rotary_speaker::RustRotarySpeaker;
use crate::harmonic_exciter::RustHarmonicExciter;
use crate::vocoder::RustVocoder;
use crate::karplus_strong::RustKarplusStrong;
use crate::reverse_reverb::RustReverseReverb;
use crate::brickwall_limiter::RustBrickwallLimiter;
use crate::bitcrusher::RustBitcrusher;
use crate::noise_gate::RustNoiseGate;
use crate::de_esser::RustDeEsser;
use crate::midi_arpeggiator::MidiArpeggiatorCore;
use crate::midi_chord_engine::MidiChordEngineCore;
use crate::midi_scale_quantizer::MidiScaleQuantizerCore;
use crate::external_sidechain::ExternalSidechainCore;
use crate::convolution::RustConvolution;
use crate::oversampler::RustOversampler;
use crate::audio_slicer::RustAudioSlicer;
use crate::shimmer_reverb::RustShimmerReverb;
use crate::refraction::RustRefraction;
use crate::spectral_smear::RustSpectralSmear;
use crate::sample_player::RustSamplePlayer;
use crate::dynamic_eq::RustDynamicEq;
use crate::stereo_imager::RustStereoImager;
use crate::lofi_cassette::RustLoFiCassette;
use crate::auto_wah::RustAutoWah;
use crate::modal_resonator::RustModalResonator;
use crate::RustWaveguide;
use crate::dimension_chorus::RustDimensionChorus;
use crate::dynamic_tremolo::RustDynamicTremolo;
use crate::multiband_saturator::RustMultibandSaturator;
use crate::comb_resonator::RustCombResonator;
use crate::pitch_delay::RustPitchDelay;
use crate::phase_dispersion::RustPhaseDispersion;
use crate::morphing_svf::RustMorphingSvf;
use crate::fractal_resonator::RustFractalResonator;
use crate::vactrol_lpg::RustVactrolLPG;
use crate::barberpole_filter::RustBarberpoleFilter;
use crate::sidechain_pumper::RustSidechainPumper;
use crate::frequency_split_panner::RustFrequencySplitPanner;
use crate::chebyshev_harmonic_shaper::RustChebyshevHarmonicShaper;
use crate::subharmonic_synthesizer::RustSubharmonicSynthesizer;
use crate::transaural_crosstalk_canceller::RustTransauralCrosstalkCanceller;
use crate::doppler_orbital_spatializer::RustDopplerOrbitalSpatializer;
use crate::granular_buffer_shuffler::RustGranularBufferShuffler;
use crate::disintegration_looper::RustDisintegrationLooper;
use crate::spring_reverb_tank::RustSpringReverbTank;
use crate::dual_bode_frequency_shifter::RustDualBodeFrequencyShifter;
use crate::microtonal_harmonizer::RustMicrotonalHarmonizer;
use crate::formant_pitch_shifter::RustFormantPitchShifter;
use crate::spectral_resonance_tamer::RustSpectralResonanceTamer;
use crate::spectral_morphing_convolver::RustSpectralMorphingConvolver;


/// Unified processor node interface for all audio effects in Rust (Rule 5).
pub trait RustProcessorNode: Send {
    fn type_id(&self) -> u32;
    fn name(&self) -> &'static str;
    fn prepare(&mut self, sample_rate: f64, max_block_size: usize);
    fn reset(&mut self);
    fn set_parameter(&mut self, param_id: u32, value: f32);
    fn get_parameter(&self, param_id: u32) -> f32;
    fn process(
        &mut self,
        in_l: *const f32,
        in_r: *const f32,
        out_l: *mut f32,
        out_r: *mut f32,
        num_samples: usize,
        sc_l: *const f32,
        sc_r: *const f32,
        arm_l: *const f32,
        arm_r: *const f32,
    );
}

/// Generic Opaque Processor Adapter for any Rust DSP core with C-ABI signatures (Rule 46: SOLID OCP).
pub struct RustOpaqueProcessor {
    ptr: *mut c_void,
    type_id: u32,
    name: &'static str,
    prepare_fn: unsafe fn(*mut c_void, f64),
    reset_fn: unsafe fn(*mut c_void),
    set_param_fn: unsafe fn(*mut c_void, u32, f32),
    get_param_fn: unsafe fn(*const c_void, u32) -> f32,
    process_fn: unsafe fn(
        *mut c_void,
        *const f32,
        *const f32,
        *mut f32,
        *mut f32,
        usize,
        *const f32,
        *const f32,
        *const f32,
        *const f32,
    ),
    destroy_fn: unsafe fn(*mut c_void),
}

unsafe impl Send for RustOpaqueProcessor {}

impl RustOpaqueProcessor {
    pub fn new(
        ptr: *mut c_void,
        type_id: u32,
        name: &'static str,
        prepare_fn: unsafe fn(*mut c_void, f64),
        reset_fn: unsafe fn(*mut c_void),
        set_param_fn: unsafe fn(*mut c_void, u32, f32),
        get_param_fn: unsafe fn(*const c_void, u32) -> f32,
        process_fn: unsafe fn(
            *mut c_void,
            *const f32,
            *const f32,
            *mut f32,
            *mut f32,
            usize,
            *const f32,
            *const f32,
            *const f32,
            *const f32,
        ),
        destroy_fn: unsafe fn(*mut c_void),
    ) -> Self {
        Self {
            ptr,
            type_id,
            name,
            prepare_fn,
            reset_fn,
            set_param_fn,
            get_param_fn,
            process_fn,
            destroy_fn,
        }
    }
}

impl Drop for RustOpaqueProcessor {
    fn drop(&mut self) {
        if !self.ptr.is_null() {
            unsafe {
                (self.destroy_fn)(self.ptr);
            }
            self.ptr = std::ptr::null_mut();
        }
    }
}

impl RustProcessorNode for RustOpaqueProcessor {
    #[inline(always)]
    fn type_id(&self) -> u32 {
        self.type_id
    }

    #[inline(always)]
    fn name(&self) -> &'static str {
        self.name
    }

    #[inline(always)]
    fn prepare(&mut self, sample_rate: f64, _max_block_size: usize) {
        if !self.ptr.is_null() {
            unsafe { (self.prepare_fn)(self.ptr, sample_rate); }
        }
    }

    #[inline(always)]
    fn reset(&mut self) {
        if !self.ptr.is_null() {
            unsafe { (self.reset_fn)(self.ptr); }
        }
    }

    #[inline(always)]
    fn set_parameter(&mut self, param_id: u32, value: f32) {
        if !self.ptr.is_null() {
            unsafe { (self.set_param_fn)(self.ptr, param_id, value); }
        }
    }

    #[inline(always)]
    fn get_parameter(&self, param_id: u32) -> f32 {
        if !self.ptr.is_null() {
            unsafe { (self.get_param_fn)(self.ptr, param_id) }
        } else {
            0.0
        }
    }

    #[inline(always)]
    fn process(
        &mut self,
        in_l: *const f32,
        in_r: *const f32,
        out_l: *mut f32,
        out_r: *mut f32,
        num_samples: usize,
        sc_l: *const f32,
        sc_r: *const f32,
        arm_l: *const f32,
        arm_r: *const f32,
    ) {
        if !self.ptr.is_null() {
            unsafe {
                (self.process_fn)(
                    self.ptr,
                    in_l,
                    in_r,
                    out_l,
                    out_r,
                    num_samples,
                    sc_l,
                    sc_r,
                    arm_l,
                    arm_r,
                );
            }
        }
    }
}

// ---------------------------------------------------------------------------
// Native implementations for foundational processors (Passthrough, Filter, Delay, Container)
// ---------------------------------------------------------------------------

impl RustProcessorNode for RustPassthrough {
    #[inline(always)]
    fn type_id(&self) -> u32 { 3 } // NodeType::Passthrough

    #[inline(always)]
    fn name(&self) -> &'static str { "Passthrough" }

    #[inline(always)]
    fn prepare(&mut self, _sample_rate: f64, _max_block_size: usize) {
        self.prepare();
    }

    #[inline(always)]
    fn reset(&mut self) {
        self.reset();
    }

    #[inline(always)]
    fn set_parameter(&mut self, param_id: u32, value: f32) {
        self.set_parameter(param_id, value);
    }

    #[inline(always)]
    fn get_parameter(&self, param_id: u32) -> f32 {
        self.get_parameter(param_id)
    }

    #[inline(always)]
    fn process(
        &mut self,
        in_l: *const f32,
        in_r: *const f32,
        out_l: *mut f32,
        out_r: *mut f32,
        num_samples: usize,
        _sc_l: *const f32,
        _sc_r: *const f32,
        _arm_l: *const f32,
        _arm_r: *const f32,
    ) {
        if in_l.is_null() || out_l.is_null() || num_samples == 0 {
            return;
        }
        let in_r_ptr = if in_r.is_null() { in_l } else { in_r };
        let out_r_ptr = if out_r.is_null() { out_l } else { out_r };
        unsafe {
            let in_l_slice = std::slice::from_raw_parts(in_l, num_samples);
            let in_r_slice = std::slice::from_raw_parts(in_r_ptr, num_samples);
            let out_l_slice = std::slice::from_raw_parts_mut(out_l, num_samples);
            let out_r_slice = std::slice::from_raw_parts_mut(out_r_ptr, num_samples);
            RustPassthrough::process(self, in_l_slice, in_r_slice, out_l_slice, out_r_slice);
        }
    }
}

impl RustProcessorNode for RustSimpleFilter {
    #[inline(always)]
    fn type_id(&self) -> u32 { 4 } // NodeType::Filter

    #[inline(always)]
    fn name(&self) -> &'static str { "Filter" }

    #[inline(always)]
    fn prepare(&mut self, sample_rate: f64, _max_block_size: usize) {
        self.prepare(sample_rate);
    }

    #[inline(always)]
    fn reset(&mut self) {
        self.reset();
    }

    #[inline(always)]
    fn set_parameter(&mut self, param_id: u32, value: f32) {
        self.set_parameter(param_id, value);
    }

    #[inline(always)]
    fn get_parameter(&self, param_id: u32) -> f32 {
        self.get_parameter(param_id)
    }

    #[inline(always)]
    fn process(
        &mut self,
        in_l: *const f32,
        in_r: *const f32,
        out_l: *mut f32,
        out_r: *mut f32,
        num_samples: usize,
        _sc_l: *const f32,
        _sc_r: *const f32,
        arm_l: *const f32,
        _arm_r: *const f32,
    ) {
        if in_l.is_null() || out_l.is_null() || num_samples == 0 {
            return;
        }
        let in_r_ptr = if in_r.is_null() { in_l } else { in_r };
        let out_r_ptr = if out_r.is_null() { out_l } else { out_r };
        unsafe {
            let in_l_slice = std::slice::from_raw_parts(in_l, num_samples);
            let in_r_slice = std::slice::from_raw_parts(in_r_ptr, num_samples);
            let out_l_slice = std::slice::from_raw_parts_mut(out_l, num_samples);
            let out_r_slice = std::slice::from_raw_parts_mut(out_r_ptr, num_samples);
            let fm_slice = if !arm_l.is_null() {
                Some(std::slice::from_raw_parts(arm_l, num_samples))
            } else {
                None
            };
            RustSimpleFilter::process(self, in_l_slice, in_r_slice, fm_slice, out_l_slice, out_r_slice);
        }
    }
}

impl RustProcessorNode for RustSimpleDelay {
    #[inline(always)]
    fn type_id(&self) -> u32 { 5 } // NodeType::Delay

    #[inline(always)]
    fn name(&self) -> &'static str { "Delay" }

    #[inline(always)]
    fn prepare(&mut self, sample_rate: f64, _max_block_size: usize) {
        self.prepare(sample_rate);
    }

    #[inline(always)]
    fn reset(&mut self) {
        self.reset();
    }

    #[inline(always)]
    fn set_parameter(&mut self, param_id: u32, value: f32) {
        self.set_parameter(param_id, value);
    }

    #[inline(always)]
    fn get_parameter(&self, param_id: u32) -> f32 {
        self.get_parameter(param_id)
    }

    #[inline(always)]
    fn process(
        &mut self,
        in_l: *const f32,
        in_r: *const f32,
        out_l: *mut f32,
        out_r: *mut f32,
        num_samples: usize,
        _sc_l: *const f32,
        _sc_r: *const f32,
        _arm_l: *const f32,
        _arm_r: *const f32,
    ) {
        if in_l.is_null() || out_l.is_null() || num_samples == 0 {
            return;
        }
        let in_r_ptr = if in_r.is_null() { in_l } else { in_r };
        let out_r_ptr = if out_r.is_null() { out_l } else { out_r };
        unsafe {
            let in_l_slice = std::slice::from_raw_parts(in_l, num_samples);
            let in_r_slice = std::slice::from_raw_parts(in_r_ptr, num_samples);
            let out_l_slice = std::slice::from_raw_parts_mut(out_l, num_samples);
            let out_r_slice = std::slice::from_raw_parts_mut(out_r_ptr, num_samples);
            RustSimpleDelay::process(self, in_l_slice, in_r_slice, out_l_slice, out_r_slice);
        }
    }
}

impl RustProcessorNode for RustContainerCore {
    #[inline(always)]
    fn type_id(&self) -> u32 { 10 } // NodeType::Container

    #[inline(always)]
    fn name(&self) -> &'static str { "Container" }

    #[inline(always)]
    fn prepare(&mut self, sample_rate: f64, max_block_size: usize) {
        self.prepare(sample_rate as f32, max_block_size);
    }

    #[inline(always)]
    fn reset(&mut self) {
        self.reset();
    }

    #[inline(always)]
    fn set_parameter(&mut self, param_id: u32, value: f32) {
        self.set_parameter(param_id, value);
    }

    #[inline(always)]
    fn get_parameter(&self, param_id: u32) -> f32 {
        self.get_parameter(param_id)
    }

    #[inline(always)]
    fn process(
        &mut self,
        in_l: *const f32,
        in_r: *const f32,
        out_l: *mut f32,
        out_r: *mut f32,
        num_samples: usize,
        _sc_l: *const f32,
        _sc_r: *const f32,
        _arm_l: *const f32,
        _arm_r: *const f32,
    ) {
        self.process_passthrough(in_l, in_r, out_l, out_r, num_samples);
    }
}

impl RustProcessorNode for RustDistortion {
    #[inline(always)]
    fn type_id(&self) -> u32 { 8 } // NodeType::Distortion

    #[inline(always)]
    fn name(&self) -> &'static str { "Distortion" }

    #[inline(always)]
    fn prepare(&mut self, sample_rate: f64, _max_block_size: usize) {
        self.prepare(sample_rate as f32);
    }

    #[inline(always)]
    fn reset(&mut self) {
        self.reset();
    }

    #[inline(always)]
    fn set_parameter(&mut self, param_id: u32, value: f32) {
        self.set_parameter(param_id, value);
    }

    #[inline(always)]
    fn get_parameter(&self, param_id: u32) -> f32 {
        match param_id {
            1 => 2.0, // default target_drive
            2 => 0.0,
            3 => 10000.0,
            4 => 1.0,
            _ => 0.0,
        }
    }

    #[inline(always)]
    fn process(
        &mut self,
        in_l: *const f32,
        in_r: *const f32,
        out_l: *mut f32,
        out_r: *mut f32,
        num_samples: usize,
        _sc_l: *const f32,
        _sc_r: *const f32,
        _arm_l: *const f32,
        _arm_r: *const f32,
    ) {
        if in_l.is_null() || out_l.is_null() || num_samples == 0 {
            return;
        }
        let in_r_ptr = if in_r.is_null() { in_l } else { in_r };
        let out_r_ptr = if out_r.is_null() { out_l } else { out_r };
        unsafe {
            let in_l_slice = std::slice::from_raw_parts(in_l, num_samples);
            let in_r_slice = std::slice::from_raw_parts(in_r_ptr, num_samples);
            let out_l_slice = std::slice::from_raw_parts_mut(out_l, num_samples);
            let out_r_slice = std::slice::from_raw_parts_mut(out_r_ptr, num_samples);
            self.process_block(in_l_slice, in_r_slice, out_l_slice, out_r_slice);
        }
    }
}

impl RustProcessorNode for RustReverb {
    #[inline(always)]
    fn type_id(&self) -> u32 { 6 } // NodeType::Reverb

    #[inline(always)]
    fn name(&self) -> &'static str { "Reverb" }

    #[inline(always)]
    fn prepare(&mut self, sample_rate: f64, _max_block_size: usize) {
        self.prepare(sample_rate as f32);
    }

    #[inline(always)]
    fn reset(&mut self) {
        self.reset();
    }

    #[inline(always)]
    fn set_parameter(&mut self, param_id: u32, value: f32) {
        self.set_parameter(param_id, value);
    }

    #[inline(always)]
    fn get_parameter(&self, param_id: u32) -> f32 {
        self.get_parameter(param_id)
    }

    #[inline(always)]
    fn process(
        &mut self,
        in_l: *const f32,
        in_r: *const f32,
        out_l: *mut f32,
        out_r: *mut f32,
        num_samples: usize,
        _sc_l: *const f32,
        _sc_r: *const f32,
        _arm_l: *const f32,
        _arm_r: *const f32,
    ) {
        if in_l.is_null() || out_l.is_null() || num_samples == 0 {
            return;
        }
        let in_r_ptr = if in_r.is_null() { in_l } else { in_r };
        let out_r_ptr = if out_r.is_null() { out_l } else { out_r };
        unsafe {
            let in_l_slice = std::slice::from_raw_parts(in_l, num_samples);
            let in_r_slice = std::slice::from_raw_parts(in_r_ptr, num_samples);
            let out_l_slice = std::slice::from_raw_parts_mut(out_l, num_samples);
            let out_r_slice = std::slice::from_raw_parts_mut(out_r_ptr, num_samples);
            self.process_block(in_l_slice, in_r_slice, out_l_slice, out_r_slice);
        }
    }
}

impl RustProcessorNode for RustCompressor {
    #[inline(always)]
    fn type_id(&self) -> u32 { 9 } // NodeType::Compressor

    #[inline(always)]
    fn name(&self) -> &'static str { "Compressor" }

    #[inline(always)]
    fn prepare(&mut self, sample_rate: f64, _max_block_size: usize) {
        self.prepare(sample_rate as f32);
    }

    #[inline(always)]
    fn reset(&mut self) {
        self.reset();
    }

    #[inline(always)]
    fn set_parameter(&mut self, param_id: u32, value: f32) {
        self.set_parameter(param_id, value);
    }

    #[inline(always)]
    fn get_parameter(&self, param_id: u32) -> f32 {
        self.get_parameter(param_id)
    }

    #[inline(always)]
    fn process(
        &mut self,
        in_l: *const f32,
        in_r: *const f32,
        out_l: *mut f32,
        out_r: *mut f32,
        num_samples: usize,
        sc_l: *const f32,
        sc_r: *const f32,
        _arm_l: *const f32,
        _arm_r: *const f32,
    ) {
        if in_l.is_null() || out_l.is_null() || num_samples == 0 {
            return;
        }
        let in_r_ptr = if in_r.is_null() { in_l } else { in_r };
        let out_r_ptr = if out_r.is_null() { out_l } else { out_r };
        unsafe {
            let in_l_slice = std::slice::from_raw_parts(in_l, num_samples);
            let in_r_slice = std::slice::from_raw_parts(in_r_ptr, num_samples);
            let out_l_slice = std::slice::from_raw_parts_mut(out_l, num_samples);
            let out_r_slice = std::slice::from_raw_parts_mut(out_r_ptr, num_samples);
            let sc_l_slice = if !sc_l.is_null() { Some(std::slice::from_raw_parts(sc_l, num_samples)) } else { None };
            let sc_r_slice = if !sc_r.is_null() { Some(std::slice::from_raw_parts(sc_r, num_samples)) } else { None };
            self.process_block(in_l_slice, in_r_slice, sc_l_slice, sc_r_slice, out_l_slice, out_r_slice);
        }
    }
}

impl RustProcessorNode for RustPhaser {
    #[inline(always)]
    fn type_id(&self) -> u32 { 16 }
    #[inline(always)]
    fn name(&self) -> &'static str { "Phaser" }
    #[inline(always)]
    fn prepare(&mut self, sample_rate: f64, _max_block_size: usize) { self.prepare(sample_rate as f32); }
    #[inline(always)]
    fn reset(&mut self) { self.reset(); }
    #[inline(always)]
    fn set_parameter(&mut self, param_id: u32, value: f32) { self.set_parameter(param_id, value); }
    #[inline(always)]
    fn get_parameter(&self, param_id: u32) -> f32 { self.get_parameter(param_id) }
    #[inline(always)]
    fn process(
        &mut self,
        in_l: *const f32, in_r: *const f32,
        out_l: *mut f32, out_r: *mut f32,
        num_samples: usize,
        _sc_l: *const f32, _sc_r: *const f32,
        __arm_l: *const f32, _arm_r: *const f32,
    ) {
        if in_l.is_null() || out_l.is_null() || num_samples == 0 { return; }
        let in_r_ptr = if in_r.is_null() { in_l } else { in_r };
        let out_r_ptr = if out_r.is_null() { out_l } else { out_r };
        unsafe {
            let in_l_slice = std::slice::from_raw_parts(in_l, num_samples);
            let in_r_slice = std::slice::from_raw_parts(in_r_ptr, num_samples);
            let out_l_slice = std::slice::from_raw_parts_mut(out_l, num_samples);
            let out_r_slice = std::slice::from_raw_parts_mut(out_r_ptr, num_samples);
            self.process_block(in_l_slice, in_r_slice, out_l_slice, out_r_slice);
        }
    }
}

impl RustProcessorNode for RustChorus {
    #[inline(always)]
    fn type_id(&self) -> u32 { 17 }
    #[inline(always)]
    fn name(&self) -> &'static str { "Chorus" }
    #[inline(always)]
    fn prepare(&mut self, sample_rate: f64, _max_block_size: usize) { self.prepare(sample_rate as f32); }
    #[inline(always)]
    fn reset(&mut self) { self.reset(); }
    #[inline(always)]
    fn set_parameter(&mut self, param_id: u32, value: f32) { self.set_parameter(param_id, value); }
    #[inline(always)]
    fn get_parameter(&self, param_id: u32) -> f32 { self.get_parameter(param_id) }
    #[inline(always)]
    fn process(
        &mut self,
        in_l: *const f32, in_r: *const f32,
        out_l: *mut f32, out_r: *mut f32,
        num_samples: usize,
        _sc_l: *const f32, _sc_r: *const f32,
        __arm_l: *const f32, _arm_r: *const f32,
    ) {
        if in_l.is_null() || out_l.is_null() || num_samples == 0 { return; }
        let in_r_ptr = if in_r.is_null() { in_l } else { in_r };
        let out_r_ptr = if out_r.is_null() { out_l } else { out_r };
        unsafe {
            let in_l_slice = std::slice::from_raw_parts(in_l, num_samples);
            let in_r_slice = std::slice::from_raw_parts(in_r_ptr, num_samples);
            let out_l_slice = std::slice::from_raw_parts_mut(out_l, num_samples);
            let out_r_slice = std::slice::from_raw_parts_mut(out_r_ptr, num_samples);
            self.process_block(in_l_slice, in_r_slice, out_l_slice, out_r_slice);
        }
    }
}

impl RustProcessorNode for RustFlanger {
    #[inline(always)]
    fn type_id(&self) -> u32 { 18 }
    #[inline(always)]
    fn name(&self) -> &'static str { "Flanger" }
    #[inline(always)]
    fn prepare(&mut self, sample_rate: f64, _max_block_size: usize) { self.prepare(sample_rate as f32); }
    #[inline(always)]
    fn reset(&mut self) { self.reset(); }
    #[inline(always)]
    fn set_parameter(&mut self, param_id: u32, value: f32) { self.set_parameter(param_id, value); }
    #[inline(always)]
    fn get_parameter(&self, param_id: u32) -> f32 { self.get_parameter(param_id) }
    #[inline(always)]
    fn process(
        &mut self,
        in_l: *const f32, in_r: *const f32,
        out_l: *mut f32, out_r: *mut f32,
        num_samples: usize,
        _sc_l: *const f32, _sc_r: *const f32,
        __arm_l: *const f32, _arm_r: *const f32,
    ) {
        if in_l.is_null() || out_l.is_null() || num_samples == 0 { return; }
        let in_r_ptr = if in_r.is_null() { in_l } else { in_r };
        let out_r_ptr = if out_r.is_null() { out_l } else { out_r };
        unsafe {
            let in_l_slice = std::slice::from_raw_parts(in_l, num_samples);
            let in_r_slice = std::slice::from_raw_parts(in_r_ptr, num_samples);
            let out_l_slice = std::slice::from_raw_parts_mut(out_l, num_samples);
            let out_r_slice = std::slice::from_raw_parts_mut(out_r_ptr, num_samples);
            self.process_block(in_l_slice, in_r_slice, out_l_slice, out_r_slice);
        }
    }
}

impl RustProcessorNode for RustRingModulator {
    #[inline(always)]
    fn type_id(&self) -> u32 { 19 }
    #[inline(always)]
    fn name(&self) -> &'static str { "RingModulator" }
    #[inline(always)]
    fn prepare(&mut self, sample_rate: f64, _max_block_size: usize) { self.prepare(sample_rate as f32); }
    #[inline(always)]
    fn reset(&mut self) { self.reset(); }
    #[inline(always)]
    fn set_parameter(&mut self, param_id: u32, value: f32) { self.set_parameter(param_id, value); }
    #[inline(always)]
    fn get_parameter(&self, param_id: u32) -> f32 { self.get_parameter(param_id) }
    #[inline(always)]
    fn process(
        &mut self,
        in_l: *const f32, in_r: *const f32,
        out_l: *mut f32, out_r: *mut f32,
        num_samples: usize,
        _sc_l: *const f32, _sc_r: *const f32,
        __arm_l: *const f32, _arm_r: *const f32,
    ) {
        if in_l.is_null() || out_l.is_null() || num_samples == 0 { return; }
        let in_r_ptr = if in_r.is_null() { in_l } else { in_r };
        let out_r_ptr = if out_r.is_null() { out_l } else { out_r };
        unsafe {
            let in_l_slice = std::slice::from_raw_parts(in_l, num_samples);
            let in_r_slice = std::slice::from_raw_parts(in_r_ptr, num_samples);
            let out_l_slice = std::slice::from_raw_parts_mut(out_l, num_samples);
            let out_r_slice = std::slice::from_raw_parts_mut(out_r_ptr, num_samples);
            self.process_block(in_l_slice, in_r_slice, out_l_slice, out_r_slice);
        }
    }
}

impl RustProcessorNode for RustFrequencyShifter {
    #[inline(always)]
    fn type_id(&self) -> u32 { 20 }
    #[inline(always)]
    fn name(&self) -> &'static str { "FrequencyShifter" }
    #[inline(always)]
    fn prepare(&mut self, sample_rate: f64, _max_block_size: usize) { self.prepare(sample_rate as f32); }
    #[inline(always)]
    fn reset(&mut self) { self.reset(); }
    #[inline(always)]
    fn set_parameter(&mut self, param_id: u32, value: f32) { self.set_parameter(param_id, value); }
    #[inline(always)]
    fn get_parameter(&self, param_id: u32) -> f32 { self.get_parameter(param_id) }
    #[inline(always)]
    fn process(
        &mut self,
        in_l: *const f32, in_r: *const f32,
        out_l: *mut f32, out_r: *mut f32,
        num_samples: usize,
        _sc_l: *const f32, _sc_r: *const f32,
        __arm_l: *const f32, _arm_r: *const f32,
    ) {
        if in_l.is_null() || out_l.is_null() || num_samples == 0 { return; }
        let in_r_ptr = if in_r.is_null() { in_l } else { in_r };
        let out_r_ptr = if out_r.is_null() { out_l } else { out_r };
        unsafe {
            let in_l_slice = std::slice::from_raw_parts(in_l, num_samples);
            let in_r_slice = std::slice::from_raw_parts(in_r_ptr, num_samples);
            let out_l_slice = std::slice::from_raw_parts_mut(out_l, num_samples);
            let out_r_slice = std::slice::from_raw_parts_mut(out_r_ptr, num_samples);
            self.process_block(in_l_slice, in_r_slice, out_l_slice, out_r_slice);
        }
    }
}

impl RustProcessorNode for RustTapeSaturation {
    #[inline(always)]
    fn type_id(&self) -> u32 { 21 }
    #[inline(always)]
    fn name(&self) -> &'static str { "Tape" }
    #[inline(always)]
    fn prepare(&mut self, sample_rate: f64, _max_block_size: usize) {
        self.set_sample_rate(sample_rate as f32);
        self.reset();
    }
    #[inline(always)]
    fn reset(&mut self) { self.reset(); }
    #[inline(always)]
    fn set_parameter(&mut self, param_id: u32, value: f32) { self.set_parameter(param_id, value); }
    #[inline(always)]
    fn get_parameter(&self, param_id: u32) -> f32 { self.get_parameter(param_id) }
    #[inline(always)]
    fn process(
        &mut self,
        in_l: *const f32, in_r: *const f32,
        out_l: *mut f32, out_r: *mut f32,
        num_samples: usize,
        _sc_l: *const f32, _sc_r: *const f32,
        __arm_l: *const f32, _arm_r: *const f32,
    ) {
        if in_l.is_null() || out_l.is_null() || num_samples == 0 { return; }
        let in_r_ptr = if in_r.is_null() { in_l } else { in_r };
        let out_r_ptr = if out_r.is_null() { out_l } else { out_r };
        unsafe {
            let in_l_slice = std::slice::from_raw_parts(in_l, num_samples);
            let in_r_slice = std::slice::from_raw_parts(in_r_ptr, num_samples);
            let out_l_slice = std::slice::from_raw_parts_mut(out_l, num_samples);
            let out_r_slice = std::slice::from_raw_parts_mut(out_r_ptr, num_samples);
            self.process_block(in_l_slice, in_r_slice, out_l_slice, out_r_slice);
        }
    }
}

impl RustProcessorNode for RustMidSideEncoder {
    #[inline(always)]
    fn type_id(&self) -> u32 { 24 }
    #[inline(always)]
    fn name(&self) -> &'static str { "MidSideEncoder" }
    #[inline(always)]
    fn prepare(&mut self, sample_rate: f64, _max_block_size: usize) { self.prepare(sample_rate as f32); }
    #[inline(always)]
    fn reset(&mut self) { self.reset(); }
    #[inline(always)]
    fn set_parameter(&mut self, param_id: u32, value: f32) { self.set_parameter(param_id, value); }
    #[inline(always)]
    fn get_parameter(&self, param_id: u32) -> f32 { self.get_parameter(param_id) }
    #[inline(always)]
    fn process(
        &mut self,
        in_l: *const f32, in_r: *const f32,
        out_l: *mut f32, out_r: *mut f32,
        num_samples: usize,
        _sc_l: *const f32, _sc_r: *const f32,
        __arm_l: *const f32, _arm_r: *const f32,
    ) {
        if in_l.is_null() || out_l.is_null() || num_samples == 0 { return; }
        let in_r_ptr = if in_r.is_null() { in_l } else { in_r };
        let out_r_ptr = if out_r.is_null() { out_l } else { out_r };
        unsafe {
            let in_l_slice = std::slice::from_raw_parts(in_l, num_samples);
            let in_r_slice = std::slice::from_raw_parts(in_r_ptr, num_samples);
            let out_l_slice = std::slice::from_raw_parts_mut(out_l, num_samples);
            let out_r_slice = std::slice::from_raw_parts_mut(out_r_ptr, num_samples);
            self.process_block(in_l_slice, in_r_slice, out_l_slice, out_r_slice);
        }
    }
}

impl RustProcessorNode for RustMidSideDecoder {
    #[inline(always)]
    fn type_id(&self) -> u32 { 25 }
    #[inline(always)]
    fn name(&self) -> &'static str { "MidSideDecoder" }
    #[inline(always)]
    fn prepare(&mut self, sample_rate: f64, _max_block_size: usize) { self.prepare(sample_rate as f32); }
    #[inline(always)]
    fn reset(&mut self) { self.reset(); }
    #[inline(always)]
    fn set_parameter(&mut self, param_id: u32, value: f32) { self.set_parameter(param_id, value); }
    #[inline(always)]
    fn get_parameter(&self, param_id: u32) -> f32 { self.get_parameter(param_id) }
    #[inline(always)]
    fn process(
        &mut self,
        in_l: *const f32, in_r: *const f32,
        out_l: *mut f32, out_r: *mut f32,
        num_samples: usize,
        _sc_l: *const f32, _sc_r: *const f32,
        __arm_l: *const f32, _arm_r: *const f32,
    ) {
        if in_l.is_null() || out_l.is_null() || num_samples == 0 { return; }
        let in_r_ptr = if in_r.is_null() { in_l } else { in_r };
        let out_r_ptr = if out_r.is_null() { out_l } else { out_r };
        unsafe {
            let in_l_slice = std::slice::from_raw_parts(in_l, num_samples);
            let in_r_slice = std::slice::from_raw_parts(in_r_ptr, num_samples);
            let out_l_slice = std::slice::from_raw_parts_mut(out_l, num_samples);
            let out_r_slice = std::slice::from_raw_parts_mut(out_r_ptr, num_samples);
            self.process_block(in_l_slice, in_r_slice, out_l_slice, out_r_slice);
        }
    }
}

impl RustProcessorNode for RustSpatialPanner {
    #[inline(always)]
    fn type_id(&self) -> u32 { 26 }
    #[inline(always)]
    fn name(&self) -> &'static str { "SpatialPanner" }
    #[inline(always)]
    fn prepare(&mut self, sample_rate: f64, _max_block_size: usize) { self.prepare(sample_rate as f32); }
    #[inline(always)]
    fn reset(&mut self) { self.reset(); }
    #[inline(always)]
    fn set_parameter(&mut self, param_id: u32, value: f32) { self.set_parameter(param_id, value); }
    #[inline(always)]
    fn get_parameter(&self, param_id: u32) -> f32 { self.get_parameter(param_id) }
    #[inline(always)]
    fn process(
        &mut self,
        in_l: *const f32, in_r: *const f32,
        out_l: *mut f32, out_r: *mut f32,
        num_samples: usize,
        _sc_l: *const f32, _sc_r: *const f32,
        __arm_l: *const f32, _arm_r: *const f32,
    ) {
        if in_l.is_null() || out_l.is_null() || num_samples == 0 { return; }
        let in_r_ptr = if in_r.is_null() { in_l } else { in_r };
        let out_r_ptr = if out_r.is_null() { out_l } else { out_r };
        unsafe {
            let in_l_slice = std::slice::from_raw_parts(in_l, num_samples);
            let in_r_slice = std::slice::from_raw_parts(in_r_ptr, num_samples);
            let out_l_slice = std::slice::from_raw_parts_mut(out_l, num_samples);
            let out_r_slice = std::slice::from_raw_parts_mut(out_r_ptr, num_samples);
            self.process_block(in_l_slice, in_r_slice, out_l_slice, out_r_slice);
        }
    }
}

impl RustProcessorNode for RustParametricEq {
    #[inline(always)]
    fn type_id(&self) -> u32 { 27 }
    #[inline(always)]
    fn name(&self) -> &'static str { "ParametricEQ" }
    #[inline(always)]
    fn prepare(&mut self, sample_rate: f64, _max_block_size: usize) { self.prepare(sample_rate as f32); }
    #[inline(always)]
    fn reset(&mut self) { self.reset(); }
    #[inline(always)]
    fn set_parameter(&mut self, param_id: u32, value: f32) { self.set_parameter(param_id, value); }
    #[inline(always)]
    fn get_parameter(&self, param_id: u32) -> f32 { self.get_parameter(param_id) }
    #[inline(always)]
    fn process(
        &mut self,
        in_l: *const f32, in_r: *const f32,
        out_l: *mut f32, out_r: *mut f32,
        num_samples: usize,
        _sc_l: *const f32, _sc_r: *const f32,
        __arm_l: *const f32, _arm_r: *const f32,
    ) {
        if in_l.is_null() || out_l.is_null() || num_samples == 0 { return; }
        let in_r_ptr = if in_r.is_null() { in_l } else { in_r };
        let out_r_ptr = if out_r.is_null() { out_l } else { out_r };
        unsafe {
            let in_l_slice = std::slice::from_raw_parts(in_l, num_samples);
            let in_r_slice = std::slice::from_raw_parts(in_r_ptr, num_samples);
            let out_l_slice = std::slice::from_raw_parts_mut(out_l, num_samples);
            let out_r_slice = std::slice::from_raw_parts_mut(out_r_ptr, num_samples);
            self.process_block(in_l_slice, in_r_slice, out_l_slice, out_r_slice);
        }
    }
}

impl RustProcessorNode for RustAdvancedDelay {
    #[inline(always)]
    fn type_id(&self) -> u32 { 28 }
    #[inline(always)]
    fn name(&self) -> &'static str { "AdvancedDelay" }
    #[inline(always)]
    fn prepare(&mut self, sample_rate: f64, _max_block_size: usize) { self.prepare(sample_rate as f32); }
    #[inline(always)]
    fn reset(&mut self) { self.reset(); }
    #[inline(always)]
    fn set_parameter(&mut self, param_id: u32, value: f32) { self.set_parameter(param_id, value); }
    #[inline(always)]
    fn get_parameter(&self, param_id: u32) -> f32 { self.get_parameter(param_id) }
    #[inline(always)]
    fn process(
        &mut self,
        in_l: *const f32, in_r: *const f32,
        out_l: *mut f32, out_r: *mut f32,
        num_samples: usize,
        _sc_l: *const f32, _sc_r: *const f32,
        __arm_l: *const f32, _arm_r: *const f32,
    ) {
        if in_l.is_null() || out_l.is_null() || num_samples == 0 { return; }
        let in_r_ptr = if in_r.is_null() { in_l } else { in_r };
        let out_r_ptr = if out_r.is_null() { out_l } else { out_r };
        unsafe {
            let in_l_slice = std::slice::from_raw_parts(in_l, num_samples);
            let in_r_slice = std::slice::from_raw_parts(in_r_ptr, num_samples);
            let out_l_slice = std::slice::from_raw_parts_mut(out_l, num_samples);
            let out_r_slice = std::slice::from_raw_parts_mut(out_r_ptr, num_samples);
            self.process_block(in_l_slice, in_r_slice, out_l_slice, out_r_slice);
        }
    }
}

impl RustProcessorNode for RustTapeStop {
    #[inline(always)]
    fn type_id(&self) -> u32 { 30 }
    #[inline(always)]
    fn name(&self) -> &'static str { "TapeStop" }
    #[inline(always)]
    fn prepare(&mut self, sample_rate: f64, _max_block_size: usize) { self.prepare(sample_rate as f32); }
    #[inline(always)]
    fn reset(&mut self) { self.reset(); }
    #[inline(always)]
    fn set_parameter(&mut self, param_id: u32, value: f32) { self.set_parameter(param_id, value); }
    #[inline(always)]
    fn get_parameter(&self, param_id: u32) -> f32 { self.get_parameter(param_id) }
    #[inline(always)]
    fn process(
        &mut self,
        in_l: *const f32, in_r: *const f32,
        out_l: *mut f32, out_r: *mut f32,
        num_samples: usize,
        _sc_l: *const f32, _sc_r: *const f32,
        __arm_l: *const f32, _arm_r: *const f32,
    ) {
        if in_l.is_null() || out_l.is_null() || num_samples == 0 { return; }
        let in_r_ptr = if in_r.is_null() { in_l } else { in_r };
        let out_r_ptr = if out_r.is_null() { out_l } else { out_r };
        unsafe {
            let in_l_slice = std::slice::from_raw_parts(in_l, num_samples);
            let in_r_slice = std::slice::from_raw_parts(in_r_ptr, num_samples);
            let out_l_slice = std::slice::from_raw_parts_mut(out_l, num_samples);
            let out_r_slice = std::slice::from_raw_parts_mut(out_r_ptr, num_samples);
            self.process_block(in_l_slice, in_r_slice, out_l_slice, out_r_slice);
        }
    }
}


// ---------------------------------------------------------------------------
// Native implementations for extended processor catalog (Phase 9 & 14, Rules 5, 19, 20, 46, 47)
// ---------------------------------------------------------------------------

// --- 7. Granular ---
impl RustProcessorNode for RustGranular {
    #[inline(always)]
    fn type_id(&self) -> u32 { 7 }
    #[inline(always)]
    fn name(&self) -> &'static str { "Granular" }
    #[inline(always)]
    fn prepare(&mut self, sample_rate: f64, _max_block_size: usize) { self.prepare(sample_rate); }
    #[inline(always)]
    fn reset(&mut self) { self.reset(); }
    #[inline(always)]
    fn set_parameter(&mut self, param_id: u32, value: f32) { self.set_parameter(param_id, value); }
    #[inline(always)]
    fn get_parameter(&self, param_id: u32) -> f32 { self.get_parameter(param_id) }
    #[inline(always)]
    fn process(
        &mut self,
        in_l: *const f32, in_r: *const f32,
        out_l: *mut f32, out_r: *mut f32,
        num_samples: usize,
        _sc_l: *const f32, _sc_r: *const f32,
        __arm_l: *const f32, _arm_r: *const f32,
    ) {
        if in_l.is_null() || out_l.is_null() || num_samples == 0 { return; }
        let in_r_ptr = if in_r.is_null() { in_l } else { in_r };
        let out_r_ptr = if out_r.is_null() { out_l } else { out_r };
        unsafe {
            let in_l_slice = std::slice::from_raw_parts(in_l, num_samples);
            let in_r_slice = std::slice::from_raw_parts(in_r_ptr, num_samples);
            let out_l_slice = std::slice::from_raw_parts_mut(out_l, num_samples);
            let out_r_slice = std::slice::from_raw_parts_mut(out_r_ptr, num_samples);
            self.process_block(in_l_slice, in_r_slice, out_l_slice, out_r_slice);
        }
    }
}

// --- 11. PitchShifter ---
impl RustProcessorNode for RustPitchShifter {
    #[inline(always)]
    fn type_id(&self) -> u32 { 11 }
    #[inline(always)]
    fn name(&self) -> &'static str { "PitchShifter" }
    #[inline(always)]
    fn prepare(&mut self, sample_rate: f64, _max_block_size: usize) { self.prepare(sample_rate as f32); }
    #[inline(always)]
    fn reset(&mut self) { self.reset(); }
    #[inline(always)]
    fn set_parameter(&mut self, param_id: u32, value: f32) { self.set_parameter(param_id, value); }
    #[inline(always)]
    fn get_parameter(&self, param_id: u32) -> f32 { self.get_parameter(param_id) }
    #[inline(always)]
    fn process(
        &mut self,
        in_l: *const f32, in_r: *const f32,
        out_l: *mut f32, out_r: *mut f32,
        num_samples: usize,
        _sc_l: *const f32, _sc_r: *const f32,
        __arm_l: *const f32, _arm_r: *const f32,
    ) {
        if in_l.is_null() || out_l.is_null() || num_samples == 0 { return; }
        let in_r_ptr = if in_r.is_null() { in_l } else { in_r };
        let out_r_ptr = if out_r.is_null() { out_l } else { out_r };
        unsafe {
            let in_l_slice = std::slice::from_raw_parts(in_l, num_samples);
            let in_r_slice = std::slice::from_raw_parts(in_r_ptr, num_samples);
            let out_l_slice = std::slice::from_raw_parts_mut(out_l, num_samples);
            let out_r_slice = std::slice::from_raw_parts_mut(out_r_ptr, num_samples);
            self.process(in_l_slice, in_r_slice, out_l_slice, out_r_slice);
        }
    }
}

// --- 12 & 29. SpectralProcessor ---
impl RustProcessorNode for RustSpectralProcessor {
    #[inline(always)]
    fn type_id(&self) -> u32 { 12 }
    #[inline(always)]
    fn name(&self) -> &'static str { "SpectralProcessor" }
    #[inline(always)]
    fn prepare(&mut self, sample_rate: f64, _max_block_size: usize) { self.prepare(sample_rate as f32); }
    #[inline(always)]
    fn reset(&mut self) { self.reset(); }
    #[inline(always)]
    fn set_parameter(&mut self, param_id: u32, value: f32) { self.set_parameter(param_id, value); }
    #[inline(always)]
    fn get_parameter(&self, param_id: u32) -> f32 { self.get_parameter(param_id) }
    #[inline(always)]
    fn process(
        &mut self,
        in_l: *const f32, in_r: *const f32,
        out_l: *mut f32, out_r: *mut f32,
        num_samples: usize,
        _sc_l: *const f32, _sc_r: *const f32,
        __arm_l: *const f32, _arm_r: *const f32,
    ) {
        if in_l.is_null() || out_l.is_null() || num_samples == 0 { return; }
        let in_r_ptr = if in_r.is_null() { in_l } else { in_r };
        let out_r_ptr = if out_r.is_null() { out_l } else { out_r };
        unsafe {
            let in_l_slice = std::slice::from_raw_parts(in_l, num_samples);
            let in_r_slice = std::slice::from_raw_parts(in_r_ptr, num_samples);
            let out_l_slice = std::slice::from_raw_parts_mut(out_l, num_samples);
            let out_r_slice = std::slice::from_raw_parts_mut(out_r_ptr, num_samples);
            self.process_block(in_l_slice, in_r_slice, out_l_slice, out_r_slice);
        }
    }
}

// --- 13. ResonatorBank ---
impl RustProcessorNode for RustResonatorBank {
    #[inline(always)]
    fn type_id(&self) -> u32 { 13 }
    #[inline(always)]
    fn name(&self) -> &'static str { "Resonator" }
    #[inline(always)]
    fn prepare(&mut self, sample_rate: f64, _max_block_size: usize) { self.prepare(sample_rate as f32); }
    #[inline(always)]
    fn reset(&mut self) { self.reset(); }
    #[inline(always)]
    fn set_parameter(&mut self, param_id: u32, value: f32) { self.set_parameter(param_id, value); }
    #[inline(always)]
    fn get_parameter(&self, param_id: u32) -> f32 { self.get_parameter(param_id) }
    #[inline(always)]
    fn process(
        &mut self,
        in_l: *const f32, in_r: *const f32,
        out_l: *mut f32, out_r: *mut f32,
        num_samples: usize,
        _sc_l: *const f32, _sc_r: *const f32,
        __arm_l: *const f32, _arm_r: *const f32,
    ) {
        if in_l.is_null() || out_l.is_null() || num_samples == 0 { return; }
        let in_r_ptr = if in_r.is_null() { in_l } else { in_r };
        let out_r_ptr = if out_r.is_null() { out_l } else { out_r };
        unsafe {
            let in_l_slice = std::slice::from_raw_parts(in_l, num_samples);
            let in_r_slice = std::slice::from_raw_parts(in_r_ptr, num_samples);
            let out_l_slice = std::slice::from_raw_parts_mut(out_l, num_samples);
            let out_r_slice = std::slice::from_raw_parts_mut(out_r_ptr, num_samples);
            self.process_block(in_l_slice, in_r_slice, out_l_slice, out_r_slice);
        }
    }
}

// --- 14. Glitch ---
impl RustProcessorNode for RustGlitch {
    #[inline(always)]
    fn type_id(&self) -> u32 { 14 }
    #[inline(always)]
    fn name(&self) -> &'static str { "Glitch" }
    #[inline(always)]
    fn prepare(&mut self, sample_rate: f64, _max_block_size: usize) { self.prepare(sample_rate as f32); }
    #[inline(always)]
    fn reset(&mut self) { self.reset(); }
    #[inline(always)]
    fn set_parameter(&mut self, param_id: u32, value: f32) { self.set_parameter(param_id, value); }
    #[inline(always)]
    fn get_parameter(&self, param_id: u32) -> f32 { self.get_parameter(param_id) }
    #[inline(always)]
    fn process(
        &mut self,
        in_l: *const f32, in_r: *const f32,
        out_l: *mut f32, out_r: *mut f32,
        num_samples: usize,
        _sc_l: *const f32, _sc_r: *const f32,
        __arm_l: *const f32, _arm_r: *const f32,
    ) {
        if in_l.is_null() || out_l.is_null() || num_samples == 0 { return; }
        let in_r_ptr = if in_r.is_null() { in_l } else { in_r };
        let out_r_ptr = if out_r.is_null() { out_l } else { out_r };
        unsafe {
            let in_l_slice = std::slice::from_raw_parts(in_l, num_samples);
            let in_r_slice = std::slice::from_raw_parts(in_r_ptr, num_samples);
            let out_l_slice = std::slice::from_raw_parts_mut(out_l, num_samples);
            let out_r_slice = std::slice::from_raw_parts_mut(out_r_ptr, num_samples);
            self.process_block(in_l_slice, in_r_slice, out_l_slice, out_r_slice);
        }
    }
}

// --- 15. MultibandDynamics ---
impl RustProcessorNode for RustMultibandDynamics {
    #[inline(always)]
    fn type_id(&self) -> u32 { 15 }
    #[inline(always)]
    fn name(&self) -> &'static str { "Multiband" }
    #[inline(always)]
    fn prepare(&mut self, sample_rate: f64, _max_block_size: usize) { self.prepare(sample_rate as f32); }
    #[inline(always)]
    fn reset(&mut self) { self.reset(); }
    #[inline(always)]
    fn set_parameter(&mut self, param_id: u32, value: f32) { self.set_parameter(param_id, value); }
    #[inline(always)]
    fn get_parameter(&self, param_id: u32) -> f32 { self.get_parameter(param_id) }
    #[inline(always)]
    fn process(
        &mut self,
        in_l: *const f32, in_r: *const f32,
        out_l: *mut f32, out_r: *mut f32,
        num_samples: usize,
        _sc_l: *const f32, _sc_r: *const f32,
        __arm_l: *const f32, _arm_r: *const f32,
    ) {
        if in_l.is_null() || out_l.is_null() || num_samples == 0 { return; }
        let in_r_ptr = if in_r.is_null() { in_l } else { in_r };
        let out_r_ptr = if out_r.is_null() { out_l } else { out_r };
        unsafe {
            let in_l_slice = std::slice::from_raw_parts(in_l, num_samples);
            let in_r_slice = std::slice::from_raw_parts(in_r_ptr, num_samples);
            let out_l_slice = std::slice::from_raw_parts_mut(out_l, num_samples);
            let out_r_slice = std::slice::from_raw_parts_mut(out_r_ptr, num_samples);
            self.process_block(in_l_slice, in_r_slice, out_l_slice, out_r_slice);
        }
    }
}

// --- 22. FeedbackContainerCore ---
impl RustProcessorNode for FeedbackContainerCore {
    #[inline(always)]
    fn type_id(&self) -> u32 { 22 }
    #[inline(always)]
    fn name(&self) -> &'static str { "Feedback" }
    #[inline(always)]
    fn prepare(&mut self, sample_rate: f64, _max_block_size: usize) { self.prepare(sample_rate as f32); }
    #[inline(always)]
    fn reset(&mut self) { self.reset(); }
    #[inline(always)]
    fn set_parameter(&mut self, param_id: u32, value: f32) { self.set_parameter(param_id, value); }
    #[inline(always)]
    fn get_parameter(&self, param_id: u32) -> f32 { self.get_parameter(param_id) }
    #[inline(always)]
    fn process(
        &mut self,
        in_l: *const f32, in_r: *const f32,
        out_l: *mut f32, out_r: *mut f32,
        num_samples: usize,
        _sc_l: *const f32, _sc_r: *const f32,
        __arm_l: *const f32, _arm_r: *const f32,
    ) {
        if in_l.is_null() || out_l.is_null() || num_samples == 0 { return; }
        let in_r_ptr = if in_r.is_null() { in_l } else { in_r };
        let out_r_ptr = if out_r.is_null() { out_l } else { out_r };
        unsafe {
            let in_l_slice = std::slice::from_raw_parts(in_l, num_samples);
            let in_r_slice = std::slice::from_raw_parts(in_r_ptr, num_samples);
            let out_l_slice = std::slice::from_raw_parts_mut(out_l, num_samples);
            let out_r_slice = std::slice::from_raw_parts_mut(out_r_ptr, num_samples);
            self.process(Some(in_l_slice), Some(in_r_slice), out_l_slice, Some(out_r_slice), num_samples);
        }
    }
}

// --- 23. EventContainerCore ---
impl RustProcessorNode for EventContainerCore {
    #[inline(always)]
    fn type_id(&self) -> u32 { 23 }
    #[inline(always)]
    fn name(&self) -> &'static str { "EventContainer" }
    #[inline(always)]
    fn prepare(&mut self, sample_rate: f64, max_block_size: usize) { self.prepare(sample_rate as f32, max_block_size, 64); }
    #[inline(always)]
    fn reset(&mut self) { self.reset(); }
    #[inline(always)]
    fn set_parameter(&mut self, param_id: u32, value: f32) { self.set_parameter(param_id, value); }
    #[inline(always)]
    fn get_parameter(&self, param_id: u32) -> f32 { self.get_parameter(param_id) }
    #[inline(always)]
    fn process(
        &mut self,
        in_l: *const f32, in_r: *const f32,
        out_l: *mut f32, out_r: *mut f32,
        num_samples: usize,
        _sc_l: *const f32, _sc_r: *const f32,
        __arm_l: *const f32, _arm_r: *const f32,
    ) {
        if in_l.is_null() || out_l.is_null() || num_samples == 0 { return; }
        let in_r_ptr = if in_r.is_null() { in_l } else { in_r };
        let out_r_ptr = if out_r.is_null() { out_l } else { out_r };
        unsafe {
            let in_l_slice = std::slice::from_raw_parts(in_l, num_samples);
            let in_r_slice = std::slice::from_raw_parts(in_r_ptr, num_samples);
            let out_l_slice = std::slice::from_raw_parts_mut(out_l, num_samples);
            let out_r_slice = std::slice::from_raw_parts_mut(out_r_ptr, num_samples);
            self.process_direct(Some(in_l_slice), Some(in_r_slice), out_l_slice, Some(out_r_slice), num_samples);
        }
    }
}

// --- 31. FormantFilter ---
impl RustProcessorNode for RustFormantFilter {
    #[inline(always)]
    fn type_id(&self) -> u32 { 31 }
    #[inline(always)]
    fn name(&self) -> &'static str { "FormantFilter" }
    #[inline(always)]
    fn prepare(&mut self, sample_rate: f64, _max_block_size: usize) { self.prepare(sample_rate as f32); }
    #[inline(always)]
    fn reset(&mut self) { self.reset(); }
    #[inline(always)]
    fn set_parameter(&mut self, param_id: u32, value: f32) { self.set_parameter(param_id, value); }
    #[inline(always)]
    fn get_parameter(&self, param_id: u32) -> f32 { self.get_parameter(param_id) }
    #[inline(always)]
    fn process(
        &mut self,
        in_l: *const f32, in_r: *const f32,
        out_l: *mut f32, out_r: *mut f32,
        num_samples: usize,
        _sc_l: *const f32, _sc_r: *const f32,
        __arm_l: *const f32, _arm_r: *const f32,
    ) {
        if in_l.is_null() || out_l.is_null() || num_samples == 0 { return; }
        let in_r_ptr = if in_r.is_null() { in_l } else { in_r };
        let out_r_ptr = if out_r.is_null() { out_l } else { out_r };
        unsafe {
            let in_l_slice = std::slice::from_raw_parts(in_l, num_samples);
            let in_r_slice = std::slice::from_raw_parts(in_r_ptr, num_samples);
            let out_l_slice = std::slice::from_raw_parts_mut(out_l, num_samples);
            let out_r_slice = std::slice::from_raw_parts_mut(out_r_ptr, num_samples);
            self.process_block(in_l_slice, in_r_slice, out_l_slice, out_r_slice);
        }
    }
}

// --- 32. NoiseTexture ---
impl RustProcessorNode for RustNoiseTexture {
    #[inline(always)]
    fn type_id(&self) -> u32 { 32 }
    #[inline(always)]
    fn name(&self) -> &'static str { "NoiseTexture" }
    #[inline(always)]
    fn prepare(&mut self, sample_rate: f64, _max_block_size: usize) { self.prepare(sample_rate as f32); }
    #[inline(always)]
    fn reset(&mut self) { self.reset(); }
    #[inline(always)]
    fn set_parameter(&mut self, param_id: u32, value: f32) { self.set_parameter(param_id, value); }
    #[inline(always)]
    fn get_parameter(&self, param_id: u32) -> f32 { self.get_parameter(param_id) }
    #[inline(always)]
    fn process(
        &mut self,
        in_l: *const f32, in_r: *const f32,
        out_l: *mut f32, out_r: *mut f32,
        num_samples: usize,
        _sc_l: *const f32, _sc_r: *const f32,
        __arm_l: *const f32, _arm_r: *const f32,
    ) {
        if in_l.is_null() || out_l.is_null() || num_samples == 0 { return; }
        let in_r_ptr = if in_r.is_null() { in_l } else { in_r };
        let out_r_ptr = if out_r.is_null() { out_l } else { out_r };
        unsafe {
            let in_l_slice = std::slice::from_raw_parts(in_l, num_samples);
            let in_r_slice = std::slice::from_raw_parts(in_r_ptr, num_samples);
            let out_l_slice = std::slice::from_raw_parts_mut(out_l, num_samples);
            let out_r_slice = std::slice::from_raw_parts_mut(out_r_ptr, num_samples);
            self.process_block(in_l_slice, in_r_slice, out_l_slice, out_r_slice);
        }
    }
}

// --- 33. TransientShaper ---
impl RustProcessorNode for RustTransientShaper {
    #[inline(always)]
    fn type_id(&self) -> u32 { 33 }
    #[inline(always)]
    fn name(&self) -> &'static str { "TransientShaper" }
    #[inline(always)]
    fn prepare(&mut self, sample_rate: f64, _max_block_size: usize) { self.set_sample_rate(sample_rate as f32); }
    #[inline(always)]
    fn reset(&mut self) { self.reset(); }
    #[inline(always)]
    fn set_parameter(&mut self, param_id: u32, value: f32) { self.set_parameter(param_id, value); }
    #[inline(always)]
    fn get_parameter(&self, param_id: u32) -> f32 { self.get_parameter(param_id) }
    #[inline(always)]
    fn process(
        &mut self,
        in_l: *const f32, in_r: *const f32,
        out_l: *mut f32, out_r: *mut f32,
        num_samples: usize,
        _sc_l: *const f32, _sc_r: *const f32,
        __arm_l: *const f32, _arm_r: *const f32,
    ) {
        if in_l.is_null() || out_l.is_null() || num_samples == 0 { return; }
        let in_r_ptr = if in_r.is_null() { in_l } else { in_r };
        let out_r_ptr = if out_r.is_null() { out_l } else { out_r };
        unsafe {
            let in_l_slice = std::slice::from_raw_parts(in_l, num_samples);
            let in_r_slice = std::slice::from_raw_parts(in_r_ptr, num_samples);
            let out_l_slice = std::slice::from_raw_parts_mut(out_l, num_samples);
            let out_r_slice = std::slice::from_raw_parts_mut(out_r_ptr, num_samples);
            self.process_block(in_l_slice, in_r_slice, out_l_slice, out_r_slice);
        }
    }
}

// --- 34. RotarySpeaker ---
impl RustProcessorNode for RustRotarySpeaker {
    #[inline(always)]
    fn type_id(&self) -> u32 { 34 }
    #[inline(always)]
    fn name(&self) -> &'static str { "RotarySpeaker" }
    #[inline(always)]
    fn prepare(&mut self, sample_rate: f64, _max_block_size: usize) { self.prepare(sample_rate); }
    #[inline(always)]
    fn reset(&mut self) { self.reset(); }
    #[inline(always)]
    fn set_parameter(&mut self, param_id: u32, value: f32) { self.set_parameter(param_id, value); }
    #[inline(always)]
    fn get_parameter(&self, param_id: u32) -> f32 { self.get_parameter(param_id) }
    #[inline(always)]
    fn process(
        &mut self,
        in_l: *const f32, in_r: *const f32,
        out_l: *mut f32, out_r: *mut f32,
        num_samples: usize,
        _sc_l: *const f32, _sc_r: *const f32,
        __arm_l: *const f32, _arm_r: *const f32,
    ) {
        if in_l.is_null() || out_l.is_null() || num_samples == 0 { return; }
        let in_r_ptr = if in_r.is_null() { in_l } else { in_r };
        let out_r_ptr = if out_r.is_null() { out_l } else { out_r };
        unsafe {
            let in_l_slice = std::slice::from_raw_parts(in_l, num_samples);
            let in_r_slice = std::slice::from_raw_parts(in_r_ptr, num_samples);
            let out_l_slice = std::slice::from_raw_parts_mut(out_l, num_samples);
            let out_r_slice = std::slice::from_raw_parts_mut(out_r_ptr, num_samples);
            self.process_block(in_l_slice, in_r_slice, out_l_slice, out_r_slice);
        }
    }
}

// --- 35. HarmonicExciter ---
impl RustProcessorNode for RustHarmonicExciter {
    #[inline(always)]
    fn type_id(&self) -> u32 { 35 }
    #[inline(always)]
    fn name(&self) -> &'static str { "HarmonicExciter" }
    #[inline(always)]
    fn prepare(&mut self, sample_rate: f64, _max_block_size: usize) { self.prepare(sample_rate); }
    #[inline(always)]
    fn reset(&mut self) { self.reset(); }
    #[inline(always)]
    fn set_parameter(&mut self, param_id: u32, value: f32) { self.set_parameter(param_id, value); }
    #[inline(always)]
    fn get_parameter(&self, param_id: u32) -> f32 { self.get_parameter(param_id) }
    #[inline(always)]
    fn process(
        &mut self,
        in_l: *const f32, in_r: *const f32,
        out_l: *mut f32, out_r: *mut f32,
        num_samples: usize,
        _sc_l: *const f32, _sc_r: *const f32,
        __arm_l: *const f32, _arm_r: *const f32,
    ) {
        if in_l.is_null() || out_l.is_null() || num_samples == 0 { return; }
        let in_r_ptr = if in_r.is_null() { in_l } else { in_r };
        let out_r_ptr = if out_r.is_null() { out_l } else { out_r };
        unsafe {
            let in_l_slice = std::slice::from_raw_parts(in_l, num_samples);
            let in_r_slice = std::slice::from_raw_parts(in_r_ptr, num_samples);
            let out_l_slice = std::slice::from_raw_parts_mut(out_l, num_samples);
            let out_r_slice = std::slice::from_raw_parts_mut(out_r_ptr, num_samples);
            self.process_block(in_l_slice, in_r_slice, out_l_slice, out_r_slice);
        }
    }
}

// --- 36. Vocoder ---
impl RustProcessorNode for RustVocoder {
    #[inline(always)]
    fn type_id(&self) -> u32 { 36 }
    #[inline(always)]
    fn name(&self) -> &'static str { "Vocoder" }
    #[inline(always)]
    fn prepare(&mut self, sample_rate: f64, _max_block_size: usize) { self.prepare(sample_rate); }
    #[inline(always)]
    fn reset(&mut self) { self.reset(); }
    #[inline(always)]
    fn set_parameter(&mut self, param_id: u32, value: f32) { self.set_parameter(param_id, value); }
    #[inline(always)]
    fn get_parameter(&self, param_id: u32) -> f32 { self.get_parameter(param_id) }
    #[inline(always)]
    fn process(
        &mut self,
        in_l: *const f32, in_r: *const f32,
        out_l: *mut f32, out_r: *mut f32,
        num_samples: usize,
        sc_l: *const f32, sc_r: *const f32,
        __arm_l: *const f32, _arm_r: *const f32,
    ) {
        if in_l.is_null() || out_l.is_null() || num_samples == 0 { return; }
        let in_r_ptr = if in_r.is_null() { in_l } else { in_r };
        let out_r_ptr = if out_r.is_null() { out_l } else { out_r };
        unsafe {
            let in_l_slice = std::slice::from_raw_parts(in_l, num_samples);
            let in_r_slice = std::slice::from_raw_parts(in_r_ptr, num_samples);
            let out_l_slice = std::slice::from_raw_parts_mut(out_l, num_samples);
            let out_r_slice = std::slice::from_raw_parts_mut(out_r_ptr, num_samples);
            let sc_l_slice = if !sc_l.is_null() { Some(std::slice::from_raw_parts(sc_l, num_samples)) } else { None };
            let sc_r_slice = if !sc_r.is_null() { Some(std::slice::from_raw_parts(sc_r, num_samples)) } else { sc_l_slice };
            self.process_block(in_l_slice, in_r_slice, sc_l_slice, sc_r_slice, out_l_slice, out_r_slice);
        }
    }
}

// --- 37. KarplusStrong ---
impl RustProcessorNode for RustKarplusStrong {
    #[inline(always)]
    fn type_id(&self) -> u32 { 37 }
    #[inline(always)]
    fn name(&self) -> &'static str { "KarplusStrong" }
    #[inline(always)]
    fn prepare(&mut self, sample_rate: f64, _max_block_size: usize) { self.set_sample_rate(sample_rate as f32); }
    #[inline(always)]
    fn reset(&mut self) { self.reset(); }
    #[inline(always)]
    fn set_parameter(&mut self, param_id: u32, value: f32) { self.set_parameter(param_id, value); }
    #[inline(always)]
    fn get_parameter(&self, param_id: u32) -> f32 { self.get_parameter(param_id) }
    #[inline(always)]
    fn process(
        &mut self,
        in_l: *const f32, in_r: *const f32,
        out_l: *mut f32, out_r: *mut f32,
        num_samples: usize,
        _sc_l: *const f32, _sc_r: *const f32,
        __arm_l: *const f32, _arm_r: *const f32,
    ) {
        if in_l.is_null() || out_l.is_null() || num_samples == 0 { return; }
        let in_r_ptr = if in_r.is_null() { in_l } else { in_r };
        let out_r_ptr = if out_r.is_null() { out_l } else { out_r };
        unsafe {
            let in_l_slice = std::slice::from_raw_parts(in_l, num_samples);
            let in_r_slice = std::slice::from_raw_parts(in_r_ptr, num_samples);
            let out_l_slice = std::slice::from_raw_parts_mut(out_l, num_samples);
            let out_r_slice = std::slice::from_raw_parts_mut(out_r_ptr, num_samples);
            self.process_block(in_l_slice, in_r_slice, out_l_slice, out_r_slice);
        }
    }
}

// --- 38. ReverseReverb ---
impl RustProcessorNode for RustReverseReverb {
    #[inline(always)]
    fn type_id(&self) -> u32 { 38 }
    #[inline(always)]
    fn name(&self) -> &'static str { "ReverseReverb" }
    #[inline(always)]
    fn prepare(&mut self, sample_rate: f64, _max_block_size: usize) { self.prepare(sample_rate as f32); }
    #[inline(always)]
    fn reset(&mut self) { self.reset(); }
    #[inline(always)]
    fn set_parameter(&mut self, param_id: u32, value: f32) { self.set_parameter(param_id, value); }
    #[inline(always)]
    fn get_parameter(&self, param_id: u32) -> f32 { self.get_parameter(param_id) }
    #[inline(always)]
    fn process(
        &mut self,
        in_l: *const f32, in_r: *const f32,
        out_l: *mut f32, out_r: *mut f32,
        num_samples: usize,
        _sc_l: *const f32, _sc_r: *const f32,
        __arm_l: *const f32, _arm_r: *const f32,
    ) {
        if in_l.is_null() || out_l.is_null() || num_samples == 0 { return; }
        let in_r_ptr = if in_r.is_null() { in_l } else { in_r };
        let out_r_ptr = if out_r.is_null() { out_l } else { out_r };
        unsafe {
            let in_l_slice = std::slice::from_raw_parts(in_l, num_samples);
            let in_r_slice = std::slice::from_raw_parts(in_r_ptr, num_samples);
            let out_l_slice = std::slice::from_raw_parts_mut(out_l, num_samples);
            let out_r_slice = std::slice::from_raw_parts_mut(out_r_ptr, num_samples);
            self.process_block(in_l_slice, in_r_slice, out_l_slice, out_r_slice);
        }
    }
}

// --- 39. BrickwallLimiter ---
impl RustProcessorNode for RustBrickwallLimiter {
    #[inline(always)]
    fn type_id(&self) -> u32 { 39 }
    #[inline(always)]
    fn name(&self) -> &'static str { "BrickwallLimiter" }
    #[inline(always)]
    fn prepare(&mut self, sample_rate: f64, _max_block_size: usize) { self.prepare(sample_rate as f32); }
    #[inline(always)]
    fn reset(&mut self) { self.reset(); }
    #[inline(always)]
    fn set_parameter(&mut self, param_id: u32, value: f32) { self.set_parameter(param_id, value); }
    #[inline(always)]
    fn get_parameter(&self, param_id: u32) -> f32 { self.get_parameter(param_id) }
    #[inline(always)]
    fn process(
        &mut self,
        in_l: *const f32, in_r: *const f32,
        out_l: *mut f32, out_r: *mut f32,
        num_samples: usize,
        _sc_l: *const f32, _sc_r: *const f32,
        __arm_l: *const f32, _arm_r: *const f32,
    ) {
        if in_l.is_null() || out_l.is_null() || num_samples == 0 { return; }
        let in_r_ptr = if in_r.is_null() { in_l } else { in_r };
        let out_r_ptr = if out_r.is_null() { out_l } else { out_r };
        unsafe {
            let in_l_slice = std::slice::from_raw_parts(in_l, num_samples);
            let in_r_slice = std::slice::from_raw_parts(in_r_ptr, num_samples);
            let out_l_slice = std::slice::from_raw_parts_mut(out_l, num_samples);
            let out_r_slice = std::slice::from_raw_parts_mut(out_r_ptr, num_samples);
            self.process_block(in_l_slice, in_r_slice, out_l_slice, out_r_slice);
        }
    }
}

// --- 40. Bitcrusher ---
impl RustProcessorNode for RustBitcrusher {
    #[inline(always)]
    fn type_id(&self) -> u32 { 40 }
    #[inline(always)]
    fn name(&self) -> &'static str { "Bitcrusher" }
    #[inline(always)]
    fn prepare(&mut self, sample_rate: f64, _max_block_size: usize) { self.prepare(sample_rate as f32); }
    #[inline(always)]
    fn reset(&mut self) { self.reset(); }
    #[inline(always)]
    fn set_parameter(&mut self, param_id: u32, value: f32) { self.set_parameter(param_id, value); }
    #[inline(always)]
    fn get_parameter(&self, param_id: u32) -> f32 { self.get_parameter(param_id) }
    #[inline(always)]
    fn process(
        &mut self,
        in_l: *const f32, in_r: *const f32,
        out_l: *mut f32, out_r: *mut f32,
        num_samples: usize,
        _sc_l: *const f32, _sc_r: *const f32,
        __arm_l: *const f32, _arm_r: *const f32,
    ) {
        if in_l.is_null() || out_l.is_null() || num_samples == 0 { return; }
        let in_r_ptr = if in_r.is_null() { in_l } else { in_r };
        let out_r_ptr = if out_r.is_null() { out_l } else { out_r };
        unsafe {
            let in_l_slice = std::slice::from_raw_parts(in_l, num_samples);
            let in_r_slice = std::slice::from_raw_parts(in_r_ptr, num_samples);
            let out_l_slice = std::slice::from_raw_parts_mut(out_l, num_samples);
            let out_r_slice = std::slice::from_raw_parts_mut(out_r_ptr, num_samples);
            self.process_block(in_l_slice, in_r_slice, out_l_slice, out_r_slice);
        }
    }
}

// --- 41. NoiseGate ---
impl RustProcessorNode for RustNoiseGate {
    #[inline(always)]
    fn type_id(&self) -> u32 { 41 }
    #[inline(always)]
    fn name(&self) -> &'static str { "NoiseGate" }
    #[inline(always)]
    fn prepare(&mut self, sample_rate: f64, _max_block_size: usize) { self.prepare(sample_rate as f32); }
    #[inline(always)]
    fn reset(&mut self) { self.reset(); }
    #[inline(always)]
    fn set_parameter(&mut self, param_id: u32, value: f32) { self.set_parameter(param_id, value); }
    #[inline(always)]
    fn get_parameter(&self, param_id: u32) -> f32 { self.get_parameter(param_id) }
    #[inline(always)]
    fn process(
        &mut self,
        in_l: *const f32, in_r: *const f32,
        out_l: *mut f32, out_r: *mut f32,
        num_samples: usize,
        sc_l: *const f32, sc_r: *const f32,
        __arm_l: *const f32, _arm_r: *const f32,
    ) {
        if in_l.is_null() || out_l.is_null() || num_samples == 0 { return; }
        let in_r_ptr = if in_r.is_null() { in_l } else { in_r };
        let out_r_ptr = if out_r.is_null() { out_l } else { out_r };
        unsafe {
            let in_l_slice = std::slice::from_raw_parts(in_l, num_samples);
            let in_r_slice = std::slice::from_raw_parts(in_r_ptr, num_samples);
            let out_l_slice = std::slice::from_raw_parts_mut(out_l, num_samples);
            let out_r_slice = std::slice::from_raw_parts_mut(out_r_ptr, num_samples);
            let sc_l_slice = if !sc_l.is_null() { Some(std::slice::from_raw_parts(sc_l, num_samples)) } else { None };
            let sc_r_slice = if !sc_r.is_null() { Some(std::slice::from_raw_parts(sc_r, num_samples)) } else { sc_l_slice };
            self.process_block(in_l_slice, in_r_slice, sc_l_slice, sc_r_slice, out_l_slice, out_r_slice);
        }
    }
}

// --- 42. DeEsser ---
impl RustProcessorNode for RustDeEsser {
    #[inline(always)]
    fn type_id(&self) -> u32 { 42 }
    #[inline(always)]
    fn name(&self) -> &'static str { "DeEsser" }
    #[inline(always)]
    fn prepare(&mut self, sample_rate: f64, _max_block_size: usize) { self.prepare(sample_rate as f32); }
    #[inline(always)]
    fn reset(&mut self) { self.reset(); }
    #[inline(always)]
    fn set_parameter(&mut self, param_id: u32, value: f32) { self.set_parameter(param_id, value); }
    #[inline(always)]
    fn get_parameter(&self, param_id: u32) -> f32 { self.get_parameter(param_id) }
    #[inline(always)]
    fn process(
        &mut self,
        in_l: *const f32, in_r: *const f32,
        out_l: *mut f32, out_r: *mut f32,
        num_samples: usize,
        _sc_l: *const f32, _sc_r: *const f32,
        __arm_l: *const f32, _arm_r: *const f32,
    ) {
        if in_l.is_null() || out_l.is_null() || num_samples == 0 { return; }
        let in_r_ptr = if in_r.is_null() { in_l } else { in_r };
        let out_r_ptr = if out_r.is_null() { out_l } else { out_r };
        unsafe {
            let in_l_slice = std::slice::from_raw_parts(in_l, num_samples);
            let in_r_slice = std::slice::from_raw_parts(in_r_ptr, num_samples);
            let out_l_slice = std::slice::from_raw_parts_mut(out_l, num_samples);
            let out_r_slice = std::slice::from_raw_parts_mut(out_r_ptr, num_samples);
            self.process_block(in_l_slice, in_r_slice, out_l_slice, out_r_slice);
        }
    }
}

// --- 43. MidiArpeggiator ---
impl RustProcessorNode for MidiArpeggiatorCore {
    #[inline(always)]
    fn type_id(&self) -> u32 { 43 }
    #[inline(always)]
    fn name(&self) -> &'static str { "MidiArpeggiator" }
    #[inline(always)]
    fn prepare(&mut self, sample_rate: f64, _max_block_size: usize) { self.prepare(sample_rate as f32); }
    #[inline(always)]
    fn reset(&mut self) { self.reset(); }
    #[inline(always)]
    fn set_parameter(&mut self, param_id: u32, value: f32) { self.set_parameter(param_id, value); }
    #[inline(always)]
    fn get_parameter(&self, param_id: u32) -> f32 { self.get_parameter(param_id) }
    #[inline(always)]
    fn process(
        &mut self,
        in_l: *const f32, in_r: *const f32,
        out_l: *mut f32, out_r: *mut f32,
        num_samples: usize,
        _sc_l: *const f32, _sc_r: *const f32,
        __arm_l: *const f32, _arm_r: *const f32,
    ) {
        if in_l.is_null() || out_l.is_null() || num_samples == 0 { return; }
        let in_r_ptr = if in_r.is_null() { in_l } else { in_r };
        let out_r_ptr = if out_r.is_null() { out_l } else { out_r };
        unsafe {
            let in_l_slice = std::slice::from_raw_parts(in_l, num_samples);
            let in_r_slice = std::slice::from_raw_parts(in_r_ptr, num_samples);
            let out_l_slice = std::slice::from_raw_parts_mut(out_l, num_samples);
            let out_r_slice = std::slice::from_raw_parts_mut(out_r_ptr, num_samples);
            self.process(Some(in_l_slice), Some(in_r_slice), out_l_slice, Some(out_r_slice), num_samples, 0.0, 120.0);
        }
    }
}

// --- 44. MidiChordEngine ---
impl RustProcessorNode for MidiChordEngineCore {
    #[inline(always)]
    fn type_id(&self) -> u32 { 44 }
    #[inline(always)]
    fn name(&self) -> &'static str { "MidiChordEngine" }
    #[inline(always)]
    fn prepare(&mut self, sample_rate: f64, _max_block_size: usize) { self.prepare(sample_rate as f32); }
    #[inline(always)]
    fn reset(&mut self) { self.reset(); }
    #[inline(always)]
    fn set_parameter(&mut self, param_id: u32, value: f32) { self.set_parameter(param_id, value); }
    #[inline(always)]
    fn get_parameter(&self, param_id: u32) -> f32 { self.get_parameter(param_id) }
    #[inline(always)]
    fn process(
        &mut self,
        in_l: *const f32, in_r: *const f32,
        out_l: *mut f32, out_r: *mut f32,
        num_samples: usize,
        _sc_l: *const f32, _sc_r: *const f32,
        __arm_l: *const f32, _arm_r: *const f32,
    ) {
        if in_l.is_null() || out_l.is_null() || num_samples == 0 { return; }
        let in_r_ptr = if in_r.is_null() { in_l } else { in_r };
        let out_r_ptr = if out_r.is_null() { out_l } else { out_r };
        unsafe {
            let in_l_slice = std::slice::from_raw_parts(in_l, num_samples);
            let in_r_slice = std::slice::from_raw_parts(in_r_ptr, num_samples);
            let out_l_slice = std::slice::from_raw_parts_mut(out_l, num_samples);
            let out_r_slice = std::slice::from_raw_parts_mut(out_r_ptr, num_samples);
            self.process(Some(in_l_slice), Some(in_r_slice), out_l_slice, Some(out_r_slice), num_samples);
        }
    }
}

// --- 45. MidiScaleQuantizer ---
impl RustProcessorNode for MidiScaleQuantizerCore {
    #[inline(always)]
    fn type_id(&self) -> u32 { 45 }
    #[inline(always)]
    fn name(&self) -> &'static str { "MidiScaleQuantizer" }
    #[inline(always)]
    fn prepare(&mut self, sample_rate: f64, _max_block_size: usize) { self.prepare(sample_rate as f32); }
    #[inline(always)]
    fn reset(&mut self) { self.reset(); }
    #[inline(always)]
    fn set_parameter(&mut self, param_id: u32, value: f32) { self.set_parameter(param_id, value); }
    #[inline(always)]
    fn get_parameter(&self, param_id: u32) -> f32 { self.get_parameter(param_id) }
    #[inline(always)]
    fn process(
        &mut self,
        in_l: *const f32, in_r: *const f32,
        out_l: *mut f32, out_r: *mut f32,
        num_samples: usize,
        _sc_l: *const f32, _sc_r: *const f32,
        __arm_l: *const f32, _arm_r: *const f32,
    ) {
        if in_l.is_null() || out_l.is_null() || num_samples == 0 { return; }
        let in_r_ptr = if in_r.is_null() { in_l } else { in_r };
        let out_r_ptr = if out_r.is_null() { out_l } else { out_r };
        unsafe {
            let in_l_slice = std::slice::from_raw_parts(in_l, num_samples);
            let in_r_slice = std::slice::from_raw_parts(in_r_ptr, num_samples);
            let out_l_slice = std::slice::from_raw_parts_mut(out_l, num_samples);
            let out_r_slice = std::slice::from_raw_parts_mut(out_r_ptr, num_samples);
            self.process(in_l_slice, in_r_slice, out_l_slice, Some(out_r_slice), num_samples);
        }
    }
}

// --- 46. ExternalSidechain ---
impl RustProcessorNode for ExternalSidechainCore {
    #[inline(always)]
    fn type_id(&self) -> u32 { 46 }
    #[inline(always)]
    fn name(&self) -> &'static str { "ExternalSidechain" }
    #[inline(always)]
    fn prepare(&mut self, sample_rate: f64, _max_block_size: usize) { self.prepare(sample_rate as f32); }
    #[inline(always)]
    fn reset(&mut self) { self.reset(); }
    #[inline(always)]
    fn set_parameter(&mut self, param_id: u32, value: f32) { self.set_parameter(param_id, value); }
    #[inline(always)]
    fn get_parameter(&self, param_id: u32) -> f32 { self.get_parameter(param_id) }
    #[inline(always)]
    fn process(
        &mut self,
        in_l: *const f32, in_r: *const f32,
        out_l: *mut f32, out_r: *mut f32,
        num_samples: usize,
        sc_l: *const f32, sc_r: *const f32,
        __arm_l: *const f32, _arm_r: *const f32,
    ) {
        if out_l.is_null() || num_samples == 0 { return; }
        let sc_l_ptr = if !sc_l.is_null() { sc_l } else { in_l };
        let sc_r_ptr = if !sc_r.is_null() { sc_r } else if !in_r.is_null() { in_r } else { sc_l_ptr };
        let out_r_ptr = if out_r.is_null() { out_l } else { out_r };
        unsafe {
            let sc_l_slice = if !sc_l_ptr.is_null() { Some(std::slice::from_raw_parts(sc_l_ptr, num_samples)) } else { None };
            let sc_r_slice = if !sc_r_ptr.is_null() { Some(std::slice::from_raw_parts(sc_r_ptr, num_samples)) } else { sc_l_slice };
            let out_l_slice = std::slice::from_raw_parts_mut(out_l, num_samples);
            let out_r_slice = std::slice::from_raw_parts_mut(out_r_ptr, num_samples);
            self.process(sc_l_slice, sc_r_slice, out_l_slice, Some(out_r_slice), num_samples);
        }
    }
}

// --- 47. Convolution ---
impl RustProcessorNode for RustConvolution {
    #[inline(always)]
    fn type_id(&self) -> u32 { 47 }
    #[inline(always)]
    fn name(&self) -> &'static str { "Convolution" }
    #[inline(always)]
    fn prepare(&mut self, sample_rate: f64, _max_block_size: usize) { self.prepare(sample_rate); }
    #[inline(always)]
    fn reset(&mut self) { self.reset(); }
    #[inline(always)]
    fn set_parameter(&mut self, param_id: u32, value: f32) { self.set_parameter(param_id, value); }
    #[inline(always)]
    fn get_parameter(&self, param_id: u32) -> f32 { self.get_parameter(param_id) }
    #[inline(always)]
    fn process(
        &mut self,
        in_l: *const f32, in_r: *const f32,
        out_l: *mut f32, out_r: *mut f32,
        num_samples: usize,
        _sc_l: *const f32, _sc_r: *const f32,
        __arm_l: *const f32, _arm_r: *const f32,
    ) {
        if in_l.is_null() || out_l.is_null() || num_samples == 0 { return; }
        let in_r_ptr = if in_r.is_null() { in_l } else { in_r };
        let out_r_ptr = if out_r.is_null() { out_l } else { out_r };
        unsafe {
            let in_l_slice = std::slice::from_raw_parts(in_l, num_samples);
            let in_r_slice = std::slice::from_raw_parts(in_r_ptr, num_samples);
            let out_l_slice = std::slice::from_raw_parts_mut(out_l, num_samples);
            let out_r_slice = std::slice::from_raw_parts_mut(out_r_ptr, num_samples);
            self.process(in_l_slice, in_r_slice, out_l_slice, out_r_slice);
        }
    }
}

// --- 48. Oversampler ---
impl RustProcessorNode for RustOversampler {
    #[inline(always)]
    fn type_id(&self) -> u32 { 48 }
    #[inline(always)]
    fn name(&self) -> &'static str { "Oversampler" }
    #[inline(always)]
    fn prepare(&mut self, sample_rate: f64, max_block_size: usize) { self.prepare(sample_rate, max_block_size); }
    #[inline(always)]
    fn reset(&mut self) { self.reset(); }
    #[inline(always)]
    fn set_parameter(&mut self, param_id: u32, value: f32) { self.set_parameter(param_id, value); }
    #[inline(always)]
    fn get_parameter(&self, param_id: u32) -> f32 { self.get_parameter(param_id) }
    #[inline(always)]
    fn process(
        &mut self,
        in_l: *const f32, in_r: *const f32,
        out_l: *mut f32, out_r: *mut f32,
        num_samples: usize,
        _sc_l: *const f32, _sc_r: *const f32,
        __arm_l: *const f32, _arm_r: *const f32,
    ) {
        if in_l.is_null() || out_l.is_null() || num_samples == 0 { return; }
        let in_r_ptr = if in_r.is_null() { in_l } else { in_r };
        let out_r_ptr = if out_r.is_null() { out_l } else { out_r };
        unsafe {
            let in_l_slice = std::slice::from_raw_parts(in_l, num_samples);
            let in_r_slice = std::slice::from_raw_parts(in_r_ptr, num_samples);
            let out_l_slice = std::slice::from_raw_parts_mut(out_l, num_samples);
            let out_r_slice = std::slice::from_raw_parts_mut(out_r_ptr, num_samples);
            self.process(in_l_slice, in_r_slice, out_l_slice, out_r_slice);
        }
    }
}

// --- 49. AudioSlicer ---
impl RustProcessorNode for RustAudioSlicer {
    #[inline(always)]
    fn type_id(&self) -> u32 { 49 }
    #[inline(always)]
    fn name(&self) -> &'static str { "AudioSlicer" }
    #[inline(always)]
    fn prepare(&mut self, sample_rate: f64, _max_block_size: usize) { self.prepare(sample_rate); }
    #[inline(always)]
    fn reset(&mut self) { self.reset(); }
    #[inline(always)]
    fn set_parameter(&mut self, param_id: u32, value: f32) { self.set_parameter(param_id, value); }
    #[inline(always)]
    fn get_parameter(&self, param_id: u32) -> f32 { self.get_parameter(param_id) }
    #[inline(always)]
    fn process(
        &mut self,
        in_l: *const f32, in_r: *const f32,
        out_l: *mut f32, out_r: *mut f32,
        num_samples: usize,
        _sc_l: *const f32, _sc_r: *const f32,
        _arm_l: *const f32, _arm_r: *const f32,
    ) {
        if in_l.is_null() || out_l.is_null() || num_samples == 0 { return; }
        let in_r_ptr = if in_r.is_null() { in_l } else { in_r };
        let out_r_ptr = if out_r.is_null() { out_l } else { out_r };
        unsafe {
            let in_l_slice = std::slice::from_raw_parts(in_l, num_samples);
            let in_r_slice = std::slice::from_raw_parts(in_r_ptr, num_samples);
            let out_l_slice = std::slice::from_raw_parts_mut(out_l, num_samples);
            let out_r_slice = std::slice::from_raw_parts_mut(out_r_ptr, num_samples);
            self.process_block(in_l_slice, in_r_slice, out_l_slice, out_r_slice);
        }
    }
}

// --- 50. ShimmerReverb ---
impl RustProcessorNode for RustShimmerReverb {
    #[inline(always)]
    fn type_id(&self) -> u32 { 50 }
    #[inline(always)]
    fn name(&self) -> &'static str { "ShimmerReverb" }
    #[inline(always)]
    fn prepare(&mut self, sample_rate: f64, _max_block_size: usize) { self.set_sample_rate(sample_rate as f32); }
    #[inline(always)]
    fn reset(&mut self) { self.reset(); }
    #[inline(always)]
    fn set_parameter(&mut self, param_id: u32, value: f32) { self.set_parameter(param_id, value); }
    #[inline(always)]
    fn get_parameter(&self, param_id: u32) -> f32 { self.get_parameter(param_id) }
    #[inline(always)]
    fn process(
        &mut self,
        in_l: *const f32, in_r: *const f32,
        out_l: *mut f32, out_r: *mut f32,
        num_samples: usize,
        _sc_l: *const f32, _sc_r: *const f32,
        __arm_l: *const f32, _arm_r: *const f32,
    ) {
        if in_l.is_null() || out_l.is_null() || num_samples == 0 { return; }
        let in_r_ptr = if in_r.is_null() { in_l } else { in_r };
        let out_r_ptr = if out_r.is_null() { out_l } else { out_r };
        unsafe {
            let in_l_slice = std::slice::from_raw_parts(in_l, num_samples);
            let in_r_slice = std::slice::from_raw_parts(in_r_ptr, num_samples);
            let out_l_slice = std::slice::from_raw_parts_mut(out_l, num_samples);
            let out_r_slice = std::slice::from_raw_parts_mut(out_r_ptr, num_samples);
            self.process_block(in_l_slice, in_r_slice, out_l_slice, out_r_slice);
        }
    }
}

// --- 51. Refraction ---
impl RustProcessorNode for RustRefraction {
    #[inline(always)]
    fn type_id(&self) -> u32 { 51 }
    #[inline(always)]
    fn name(&self) -> &'static str { "Refraction" }
    #[inline(always)]
    fn prepare(&mut self, sample_rate: f64, _max_block_size: usize) { self.set_sample_rate(sample_rate as f32); }
    #[inline(always)]
    fn reset(&mut self) { self.reset(); }
    #[inline(always)]
    fn set_parameter(&mut self, param_id: u32, value: f32) { self.set_parameter(param_id, value); }
    #[inline(always)]
    fn get_parameter(&self, param_id: u32) -> f32 { self.get_parameter(param_id) }
    #[inline(always)]
    fn process(
        &mut self,
        in_l: *const f32, in_r: *const f32,
        out_l: *mut f32, out_r: *mut f32,
        num_samples: usize,
        _sc_l: *const f32, _sc_r: *const f32,
        __arm_l: *const f32, _arm_r: *const f32,
    ) {
        if in_l.is_null() || out_l.is_null() || num_samples == 0 { return; }
        let in_r_ptr = if in_r.is_null() { in_l } else { in_r };
        let out_r_ptr = if out_r.is_null() { out_l } else { out_r };
        unsafe {
            let in_l_slice = std::slice::from_raw_parts(in_l, num_samples);
            let in_r_slice = std::slice::from_raw_parts(in_r_ptr, num_samples);
            let out_l_slice = std::slice::from_raw_parts_mut(out_l, num_samples);
            let out_r_slice = std::slice::from_raw_parts_mut(out_r_ptr, num_samples);
            self.process_block(in_l_slice, in_r_slice, out_l_slice, out_r_slice);
        }
    }
}

// --- 52. SpectralSmear ---
impl RustProcessorNode for RustSpectralSmear {
    #[inline(always)]
    fn type_id(&self) -> u32 { 52 }
    #[inline(always)]
    fn name(&self) -> &'static str { "SpectralSmear" }
    #[inline(always)]
    fn prepare(&mut self, sample_rate: f64, _max_block_size: usize) { self.prepare(sample_rate as f32); }
    #[inline(always)]
    fn reset(&mut self) { self.reset(); }
    #[inline(always)]
    fn set_parameter(&mut self, param_id: u32, value: f32) { self.set_parameter(param_id, value); }
    #[inline(always)]
    fn get_parameter(&self, param_id: u32) -> f32 { self.get_parameter(param_id) }
    #[inline(always)]
    fn process(
        &mut self,
        in_l: *const f32, in_r: *const f32,
        out_l: *mut f32, out_r: *mut f32,
        num_samples: usize,
        _sc_l: *const f32, _sc_r: *const f32,
        __arm_l: *const f32, _arm_r: *const f32,
    ) {
        if in_l.is_null() || out_l.is_null() || num_samples == 0 { return; }
        let in_r_ptr = if in_r.is_null() { in_l } else { in_r };
        let out_r_ptr = if out_r.is_null() { out_l } else { out_r };
        unsafe {
            let in_l_slice = std::slice::from_raw_parts(in_l, num_samples);
            let in_r_slice = std::slice::from_raw_parts(in_r_ptr, num_samples);
            let out_l_slice = std::slice::from_raw_parts_mut(out_l, num_samples);
            let out_r_slice = std::slice::from_raw_parts_mut(out_r_ptr, num_samples);
            self.process(in_l_slice, in_r_slice, out_l_slice, out_r_slice);
        }
    }
}

// --- 53. SamplePlayer ---
impl RustProcessorNode for RustSamplePlayer {
    #[inline(always)]
    fn type_id(&self) -> u32 { 53 }
    #[inline(always)]
    fn name(&self) -> &'static str { "SamplePlayer" }
    #[inline(always)]
    fn prepare(&mut self, sample_rate: f64, _max_block_size: usize) { self.prepare(sample_rate); }
    #[inline(always)]
    fn reset(&mut self) { self.reset(); }
    #[inline(always)]
    fn set_parameter(&mut self, param_id: u32, value: f32) { self.set_parameter(param_id, value); }
    #[inline(always)]
    fn get_parameter(&self, param_id: u32) -> f32 { self.get_parameter(param_id) }
    #[inline(always)]
    fn process(
        &mut self,
        in_l: *const f32, in_r: *const f32,
        out_l: *mut f32, out_r: *mut f32,
        num_samples: usize,
        _sc_l: *const f32, _sc_r: *const f32,
        arm_l: *const f32, _arm_r: *const f32,
    ) {
        if in_l.is_null() || out_l.is_null() || num_samples == 0 { return; }
        let in_r_ptr = if in_r.is_null() { in_l } else { in_r };
        let out_r_ptr = if out_r.is_null() { out_l } else { out_r };
        unsafe {
            let in_l_slice = std::slice::from_raw_parts(in_l, num_samples);
            let in_r_slice = std::slice::from_raw_parts(in_r_ptr, num_samples);
            let out_l_slice = std::slice::from_raw_parts_mut(out_l, num_samples);
            let out_r_slice = std::slice::from_raw_parts_mut(out_r_ptr, num_samples);
            let trigger_opt = if !arm_l.is_null() { Some(std::slice::from_raw_parts(arm_l, num_samples)) } else { None };
            self.process(in_l_slice, in_r_slice, trigger_opt, out_l_slice, out_r_slice);
        }
    }
}

// --- 54. DynamicEq ---
impl RustProcessorNode for RustDynamicEq {
    #[inline(always)]
    fn type_id(&self) -> u32 { 54 }
    #[inline(always)]
    fn name(&self) -> &'static str { "DynamicEq" }
    #[inline(always)]
    fn prepare(&mut self, sample_rate: f64, _max_block_size: usize) { self.set_sample_rate(sample_rate as f32); }
    #[inline(always)]
    fn reset(&mut self) { self.reset(); }
    #[inline(always)]
    fn set_parameter(&mut self, param_id: u32, value: f32) { self.set_parameter(param_id, value); }
    #[inline(always)]
    fn get_parameter(&self, param_id: u32) -> f32 { self.get_parameter(param_id) }
    #[inline(always)]
    fn process(
        &mut self,
        in_l: *const f32, in_r: *const f32,
        out_l: *mut f32, out_r: *mut f32,
        num_samples: usize,
        _sc_l: *const f32, _sc_r: *const f32,
        __arm_l: *const f32, _arm_r: *const f32,
    ) {
        if in_l.is_null() || out_l.is_null() || num_samples == 0 { return; }
        let in_r_ptr = if in_r.is_null() { in_l } else { in_r };
        let out_r_ptr = if out_r.is_null() { out_l } else { out_r };
        unsafe {
            let in_l_slice = std::slice::from_raw_parts(in_l, num_samples);
            let in_r_slice = std::slice::from_raw_parts(in_r_ptr, num_samples);
            let out_l_slice = std::slice::from_raw_parts_mut(out_l, num_samples);
            let out_r_slice = std::slice::from_raw_parts_mut(out_r_ptr, num_samples);
            self.process_block(in_l_slice, in_r_slice, out_l_slice, out_r_slice);
        }
    }
}

// --- 55. StereoImager ---
impl RustProcessorNode for RustStereoImager {
    #[inline(always)]
    fn type_id(&self) -> u32 { 55 }
    #[inline(always)]
    fn name(&self) -> &'static str { "StereoImager" }
    #[inline(always)]
    fn prepare(&mut self, sample_rate: f64, _max_block_size: usize) { self.prepare(sample_rate as f32); }
    #[inline(always)]
    fn reset(&mut self) { self.reset(); }
    #[inline(always)]
    fn set_parameter(&mut self, param_id: u32, value: f32) { self.set_parameter(param_id, value); }
    #[inline(always)]
    fn get_parameter(&self, param_id: u32) -> f32 { self.get_parameter(param_id) }
    #[inline(always)]
    fn process(
        &mut self,
        in_l: *const f32, in_r: *const f32,
        out_l: *mut f32, out_r: *mut f32,
        num_samples: usize,
        _sc_l: *const f32, _sc_r: *const f32,
        __arm_l: *const f32, _arm_r: *const f32,
    ) {
        if in_l.is_null() || out_l.is_null() || num_samples == 0 { return; }
        let in_r_ptr = if in_r.is_null() { in_l } else { in_r };
        let out_r_ptr = if out_r.is_null() { out_l } else { out_r };
        unsafe {
            let in_l_slice = std::slice::from_raw_parts(in_l, num_samples);
            let in_r_slice = std::slice::from_raw_parts(in_r_ptr, num_samples);
            let out_l_slice = std::slice::from_raw_parts_mut(out_l, num_samples);
            let out_r_slice = std::slice::from_raw_parts_mut(out_r_ptr, num_samples);
            self.process_block(in_l_slice, in_r_slice, out_l_slice, out_r_slice);
        }
    }
}

// --- 56. LoFiCassette ---
impl RustProcessorNode for RustLoFiCassette {
    #[inline(always)]
    fn type_id(&self) -> u32 { 56 }
    #[inline(always)]
    fn name(&self) -> &'static str { "LoFiCassette" }
    #[inline(always)]
    fn prepare(&mut self, sample_rate: f64, _max_block_size: usize) { self.prepare(sample_rate as f32); }
    #[inline(always)]
    fn reset(&mut self) { self.reset(); }
    #[inline(always)]
    fn set_parameter(&mut self, param_id: u32, value: f32) { self.set_parameter(param_id, value); }
    #[inline(always)]
    fn get_parameter(&self, param_id: u32) -> f32 { self.get_parameter(param_id) }
    #[inline(always)]
    fn process(
        &mut self,
        in_l: *const f32, in_r: *const f32,
        out_l: *mut f32, out_r: *mut f32,
        num_samples: usize,
        _sc_l: *const f32, _sc_r: *const f32,
        __arm_l: *const f32, _arm_r: *const f32,
    ) {
        if in_l.is_null() || out_l.is_null() || num_samples == 0 { return; }
        let in_r_ptr = if in_r.is_null() { in_l } else { in_r };
        let out_r_ptr = if out_r.is_null() { out_l } else { out_r };
        unsafe {
            let in_l_slice = std::slice::from_raw_parts(in_l, num_samples);
            let in_r_slice = std::slice::from_raw_parts(in_r_ptr, num_samples);
            let out_l_slice = std::slice::from_raw_parts_mut(out_l, num_samples);
            let out_r_slice = std::slice::from_raw_parts_mut(out_r_ptr, num_samples);
            self.process_block(in_l_slice, in_r_slice, out_l_slice, out_r_slice);
        }
    }
}

// --- 57. AutoWah ---
impl RustProcessorNode for RustAutoWah {
    #[inline(always)]
    fn type_id(&self) -> u32 { 57 }
    #[inline(always)]
    fn name(&self) -> &'static str { "AutoWah" }
    #[inline(always)]
    fn prepare(&mut self, sample_rate: f64, _max_block_size: usize) { self.prepare(sample_rate as f32); }
    #[inline(always)]
    fn reset(&mut self) { self.reset(); }
    #[inline(always)]
    fn set_parameter(&mut self, param_id: u32, value: f32) { self.set_parameter(param_id, value); }
    #[inline(always)]
    fn get_parameter(&self, param_id: u32) -> f32 { self.get_parameter(param_id) }
    #[inline(always)]
    fn process(
        &mut self,
        in_l: *const f32, in_r: *const f32,
        out_l: *mut f32, out_r: *mut f32,
        num_samples: usize,
        sc_l: *const f32, _sc_r: *const f32,
        __arm_l: *const f32, _arm_r: *const f32,
    ) {
        if in_l.is_null() || out_l.is_null() || num_samples == 0 { return; }
        let in_r_ptr = if in_r.is_null() { in_l } else { in_r };
        let out_r_ptr = if out_r.is_null() { out_l } else { out_r };
        unsafe {
            let in_l_slice = std::slice::from_raw_parts(in_l, num_samples);
            let in_r_slice = std::slice::from_raw_parts(in_r_ptr, num_samples);
            let out_l_slice = std::slice::from_raw_parts_mut(out_l, num_samples);
            let out_r_slice = std::slice::from_raw_parts_mut(out_r_ptr, num_samples);
            let sc_l_slice = if !sc_l.is_null() { Some(std::slice::from_raw_parts(sc_l, num_samples)) } else { None };
            self.process_block(in_l_slice, in_r_slice, sc_l_slice, out_l_slice, out_r_slice);
        }
    }
}

// --- 58. ModalResonator ---
impl RustProcessorNode for RustModalResonator {
    #[inline(always)]
    fn type_id(&self) -> u32 { 58 }
    #[inline(always)]
    fn name(&self) -> &'static str { "ModalResonator" }
    #[inline(always)]
    fn prepare(&mut self, sample_rate: f64, _max_block_size: usize) { self.set_sample_rate(sample_rate as f32); }
    #[inline(always)]
    fn reset(&mut self) { self.reset(); }
    #[inline(always)]
    fn set_parameter(&mut self, param_id: u32, value: f32) { self.set_parameter(param_id, value); }
    #[inline(always)]
    fn get_parameter(&self, param_id: u32) -> f32 { self.get_parameter(param_id) }
    #[inline(always)]
    fn process(
        &mut self,
        in_l: *const f32, in_r: *const f32,
        out_l: *mut f32, out_r: *mut f32,
        num_samples: usize,
        _sc_l: *const f32, _sc_r: *const f32,
        __arm_l: *const f32, _arm_r: *const f32,
    ) {
        if in_l.is_null() || out_l.is_null() || num_samples == 0 { return; }
        let in_r_ptr = if in_r.is_null() { in_l } else { in_r };
        let out_r_ptr = if out_r.is_null() { out_l } else { out_r };
        unsafe {
            let in_l_slice = std::slice::from_raw_parts(in_l, num_samples);
            let in_r_slice = std::slice::from_raw_parts(in_r_ptr, num_samples);
            let out_l_slice = std::slice::from_raw_parts_mut(out_l, num_samples);
            let out_r_slice = std::slice::from_raw_parts_mut(out_r_ptr, num_samples);
            self.process_block(in_l_slice, in_r_slice, out_l_slice, out_r_slice);
        }
    }
}

// --- 59. RustWaveguide ---
impl RustProcessorNode for RustWaveguide {
    #[inline(always)]
    fn type_id(&self) -> u32 { 59 }
    #[inline(always)]
    fn name(&self) -> &'static str { "RustWaveguide" }
    #[inline(always)]
    fn prepare(&mut self, sample_rate: f64, _max_block_size: usize) { self.set_sample_rate(sample_rate as f32); }
    #[inline(always)]
    fn reset(&mut self) { self.reset(); }
    #[inline(always)]
    fn set_parameter(&mut self, param_id: u32, value: f32) {
        match param_id {
            1 => self.set_sample_rate(value), // or frequency
            _ => {}
        }
    }
    #[inline(always)]
    fn get_parameter(&self, _param_id: u32) -> f32 { 0.0 }
    #[inline(always)]
    fn process(
        &mut self,
        in_l: *const f32, in_r: *const f32,
        out_l: *mut f32, out_r: *mut f32,
        num_samples: usize,
        _sc_l: *const f32, _sc_r: *const f32,
        __arm_l: *const f32, _arm_r: *const f32,
    ) {
        if in_l.is_null() || out_l.is_null() || num_samples == 0 { return; }
        let in_r_ptr = if in_r.is_null() { in_l } else { in_r };
        let out_r_ptr = if out_r.is_null() { out_l } else { out_r };
        unsafe {
            let in_l_slice = std::slice::from_raw_parts(in_l, num_samples);
            let in_r_slice = std::slice::from_raw_parts(in_r_ptr, num_samples);
            let out_l_slice = std::slice::from_raw_parts_mut(out_l, num_samples);
            let out_r_slice = std::slice::from_raw_parts_mut(out_r_ptr, num_samples);
            self.process_block(in_l_slice, in_r_slice, out_l_slice, out_r_slice);
        }
    }
}

// --- 60. DimensionChorus ---
impl RustProcessorNode for RustDimensionChorus {
    #[inline(always)]
    fn type_id(&self) -> u32 { 60 }
    #[inline(always)]
    fn name(&self) -> &'static str { "DimensionChorus" }
    #[inline(always)]
    fn prepare(&mut self, sample_rate: f64, _max_block_size: usize) { self.prepare(sample_rate as f32); }
    #[inline(always)]
    fn reset(&mut self) { self.reset(); }
    #[inline(always)]
    fn set_parameter(&mut self, param_id: u32, value: f32) {
        match param_id {
            1 => self.set_mode(value as u32),
            2 => self.set_rate(value),
            3 => self.set_depth(value),
            4 => self.set_stereo_spread(value),
            5 => self.set_drive(value),
            6 => self.set_mix(value),
            _ => {}
        }
    }
    #[inline(always)]
    fn get_parameter(&self, _param_id: u32) -> f32 { 0.0 }
    #[inline(always)]
    fn process(
        &mut self,
        in_l: *const f32, in_r: *const f32,
        out_l: *mut f32, out_r: *mut f32,
        num_samples: usize,
        _sc_l: *const f32, _sc_r: *const f32,
        __arm_l: *const f32, _arm_r: *const f32,
    ) {
        if in_l.is_null() || out_l.is_null() || num_samples == 0 { return; }
        let in_r_ptr = if in_r.is_null() { in_l } else { in_r };
        let out_r_ptr = if out_r.is_null() { out_l } else { out_r };
        unsafe {
            let in_l_slice = std::slice::from_raw_parts(in_l, num_samples);
            let in_r_slice = std::slice::from_raw_parts(in_r_ptr, num_samples);
            let out_l_slice = std::slice::from_raw_parts_mut(out_l, num_samples);
            let out_r_slice = std::slice::from_raw_parts_mut(out_r_ptr, num_samples);
            self.process_block(in_l_slice, in_r_slice, out_l_slice, out_r_slice);
        }
    }
}

// --- 61. DynamicTremolo ---
impl RustProcessorNode for RustDynamicTremolo {
    #[inline(always)]
    fn type_id(&self) -> u32 { 61 }
    #[inline(always)]
    fn name(&self) -> &'static str { "DynamicTremolo" }
    #[inline(always)]
    fn prepare(&mut self, sample_rate: f64, _max_block_size: usize) { self.prepare(sample_rate as f32); }
    #[inline(always)]
    fn reset(&mut self) { self.reset(); }
    #[inline(always)]
    fn set_parameter(&mut self, param_id: u32, value: f32) {
        match param_id {
            1 => self.set_rate(value),
            2 => self.set_depth(value),
            3 => self.set_shape(value as u32),
            4 => self.set_stereo_phase(value),
            5 => self.set_dynamics(value),
            6 => self.set_mix(value),
            _ => {}
        }
    }
    #[inline(always)]
    fn get_parameter(&self, _param_id: u32) -> f32 { 0.0 }
    #[inline(always)]
    fn process(
        &mut self,
        in_l: *const f32, in_r: *const f32,
        out_l: *mut f32, out_r: *mut f32,
        num_samples: usize,
        _sc_l: *const f32, _sc_r: *const f32,
        __arm_l: *const f32, _arm_r: *const f32,
    ) {
        if in_l.is_null() || out_l.is_null() || num_samples == 0 { return; }
        let in_r_ptr = if in_r.is_null() { in_l } else { in_r };
        let out_r_ptr = if out_r.is_null() { out_l } else { out_r };
        unsafe {
            let in_l_slice = std::slice::from_raw_parts(in_l, num_samples);
            let in_r_slice = std::slice::from_raw_parts(in_r_ptr, num_samples);
            let out_l_slice = std::slice::from_raw_parts_mut(out_l, num_samples);
            let out_r_slice = std::slice::from_raw_parts_mut(out_r_ptr, num_samples);
            self.process_block(in_l_slice, in_r_slice, out_l_slice, out_r_slice);
        }
    }
}

// --- 62. MultibandSaturator ---
impl RustProcessorNode for RustMultibandSaturator {
    #[inline(always)]
    fn type_id(&self) -> u32 { 62 }
    #[inline(always)]
    fn name(&self) -> &'static str { "MultibandSaturator" }
    #[inline(always)]
    fn prepare(&mut self, sample_rate: f64, _max_block_size: usize) { self.prepare(sample_rate as f32); }
    #[inline(always)]
    fn reset(&mut self) { self.reset(); }
    #[inline(always)]
    fn set_parameter(&mut self, param_id: u32, value: f32) {
        match param_id {
            1 => self.set_crossover_low(value),
            2 => self.set_crossover_high(value),
            3 => self.set_drive_low(value),
            4 => self.set_drive_mid(value),
            5 => self.set_drive_high(value),
            6 => self.set_model_low(value as u32),
            7 => self.set_model_mid(value as u32),
            8 => self.set_model_high(value as u32),
            9 => self.set_mix(value),
            _ => {}
        }
    }
    #[inline(always)]
    fn get_parameter(&self, _param_id: u32) -> f32 { 0.0 }
    #[inline(always)]
    fn process(
        &mut self,
        in_l: *const f32, in_r: *const f32,
        out_l: *mut f32, out_r: *mut f32,
        num_samples: usize,
        _sc_l: *const f32, _sc_r: *const f32,
        __arm_l: *const f32, _arm_r: *const f32,
    ) {
        if in_l.is_null() || out_l.is_null() || num_samples == 0 { return; }
        let in_r_ptr = if in_r.is_null() { in_l } else { in_r };
        let out_r_ptr = if out_r.is_null() { out_l } else { out_r };
        unsafe {
            let in_l_slice = std::slice::from_raw_parts(in_l, num_samples);
            let in_r_slice = std::slice::from_raw_parts(in_r_ptr, num_samples);
            let out_l_slice = std::slice::from_raw_parts_mut(out_l, num_samples);
            let out_r_slice = std::slice::from_raw_parts_mut(out_r_ptr, num_samples);
            self.process_block(in_l_slice, in_r_slice, out_l_slice, out_r_slice);
        }
    }
}

// --- 63. CombResonator ---
impl RustProcessorNode for RustCombResonator {
    #[inline(always)]
    fn type_id(&self) -> u32 { 63 }
    #[inline(always)]
    fn name(&self) -> &'static str { "CombResonator" }
    #[inline(always)]
    fn prepare(&mut self, sample_rate: f64, _max_block_size: usize) { self.prepare(sample_rate as f32); }
    #[inline(always)]
    fn reset(&mut self) { self.reset(); }
    #[inline(always)]
    fn set_parameter(&mut self, param_id: u32, value: f32) {
        match param_id {
            1 => self.set_freq(value),
            2 => self.set_feedback(value),
            3 => self.set_damping(value),
            4 => self.set_stereo_detune(value),
            5 => self.set_mix(value),
            _ => {}
        }
    }
    #[inline(always)]
    fn get_parameter(&self, _param_id: u32) -> f32 { 0.0 }
    #[inline(always)]
    fn process(
        &mut self,
        in_l: *const f32, in_r: *const f32,
        out_l: *mut f32, out_r: *mut f32,
        num_samples: usize,
        _sc_l: *const f32, _sc_r: *const f32,
        __arm_l: *const f32, _arm_r: *const f32,
    ) {
        if in_l.is_null() || out_l.is_null() || num_samples == 0 { return; }
        let in_r_ptr = if in_r.is_null() { in_l } else { in_r };
        let out_r_ptr = if out_r.is_null() { out_l } else { out_r };
        unsafe {
            let in_l_slice = std::slice::from_raw_parts(in_l, num_samples);
            let in_r_slice = std::slice::from_raw_parts(in_r_ptr, num_samples);
            let out_l_slice = std::slice::from_raw_parts_mut(out_l, num_samples);
            let out_r_slice = std::slice::from_raw_parts_mut(out_r_ptr, num_samples);
            self.process_block(in_l_slice, in_r_slice, out_l_slice, out_r_slice);
        }
    }
}

// --- 64. PitchDelay ---
impl RustProcessorNode for RustPitchDelay {
    #[inline(always)]
    fn type_id(&self) -> u32 { 64 }
    #[inline(always)]
    fn name(&self) -> &'static str { "PitchDelay" }
    #[inline(always)]
    fn prepare(&mut self, sample_rate: f64, _max_block_size: usize) { self.prepare(sample_rate as f32); }
    #[inline(always)]
    fn reset(&mut self) { self.reset(); }
    #[inline(always)]
    fn set_parameter(&mut self, param_id: u32, value: f32) {
        match param_id {
            1 => self.set_delay_l(value),
            2 => self.set_delay_r(value),
            3 => self.set_pitch(value),
            4 => self.set_detune(value),
            5 => self.set_feedback(value),
            6 => self.set_reverse(value >= 0.5),
            7 => self.set_damping(value),
            8 => self.set_mix(value),
            _ => {}
        }
    }
    #[inline(always)]
    fn get_parameter(&self, _param_id: u32) -> f32 { 0.0 }
    #[inline(always)]
    fn process(
        &mut self,
        in_l: *const f32, in_r: *const f32,
        out_l: *mut f32, out_r: *mut f32,
        num_samples: usize,
        _sc_l: *const f32, _sc_r: *const f32,
        __arm_l: *const f32, _arm_r: *const f32,
    ) {
        if in_l.is_null() || out_l.is_null() || num_samples == 0 { return; }
        let in_r_ptr = if in_r.is_null() { in_l } else { in_r };
        let out_r_ptr = if out_r.is_null() { out_l } else { out_r };
        unsafe {
            let in_l_slice = std::slice::from_raw_parts(in_l, num_samples);
            let in_r_slice = std::slice::from_raw_parts(in_r_ptr, num_samples);
            let out_l_slice = std::slice::from_raw_parts_mut(out_l, num_samples);
            let out_r_slice = std::slice::from_raw_parts_mut(out_r_ptr, num_samples);
            self.process_block(in_l_slice, in_r_slice, out_l_slice, out_r_slice);
        }
    }
}

// --- 65. PhaseDispersion ---
impl RustProcessorNode for RustPhaseDispersion {
    #[inline(always)]
    fn type_id(&self) -> u32 { 65 }
    #[inline(always)]
    fn name(&self) -> &'static str { "PhaseDispersion" }
    #[inline(always)]
    fn prepare(&mut self, sample_rate: f64, _max_block_size: usize) { self.prepare(sample_rate as f32); }
    #[inline(always)]
    fn reset(&mut self) { self.reset(); }
    #[inline(always)]
    fn set_parameter(&mut self, param_id: u32, value: f32) {
        match param_id {
            1 => self.set_stage_count(value),
            2 => self.set_dispersion_freq(value),
            3 => self.set_spread(value),
            4 => self.set_damping(value),
            5 => self.set_feedback(value),
            6 => self.set_mix(value),
            _ => {}
        }
    }
    #[inline(always)]
    fn get_parameter(&self, _param_id: u32) -> f32 { 0.0 }
    #[inline(always)]
    fn process(
        &mut self,
        in_l: *const f32, in_r: *const f32,
        out_l: *mut f32, out_r: *mut f32,
        num_samples: usize,
        _sc_l: *const f32, _sc_r: *const f32,
        __arm_l: *const f32, _arm_r: *const f32,
    ) {
        if in_l.is_null() || out_l.is_null() || num_samples == 0 { return; }
        let in_r_ptr = if in_r.is_null() { in_l } else { in_r };
        let out_r_ptr = if out_r.is_null() { out_l } else { out_r };
        unsafe {
            let in_l_slice = std::slice::from_raw_parts(in_l, num_samples);
            let in_r_slice = std::slice::from_raw_parts(in_r_ptr, num_samples);
            let out_l_slice = std::slice::from_raw_parts_mut(out_l, num_samples);
            let out_r_slice = std::slice::from_raw_parts_mut(out_r_ptr, num_samples);
            self.process_block(in_l_slice, in_r_slice, out_l_slice, out_r_slice, num_samples);
        }
    }
}

// --- 66. MorphingSVF ---
impl RustProcessorNode for RustMorphingSvf {
    #[inline(always)]
    fn type_id(&self) -> u32 { 66 }
    #[inline(always)]
    fn name(&self) -> &'static str { "MorphingSVF" }
    #[inline(always)]
    fn prepare(&mut self, sample_rate: f64, _max_block_size: usize) { self.prepare(sample_rate as f32); }
    #[inline(always)]
    fn reset(&mut self) { self.reset(); }
    #[inline(always)]
    fn set_parameter(&mut self, param_id: u32, value: f32) {
        match param_id {
            1 => self.set_cutoff(value),
            2 => self.set_resonance(value),
            3 => self.set_morph(value),
            4 => self.set_drive(value),
            5 => self.set_fm_depth(value),
            _ => {}
        }
    }
    #[inline(always)]
    fn get_parameter(&self, _param_id: u32) -> f32 { 0.0 }
    #[inline(always)]
    fn process(
        &mut self,
        in_l: *const f32, in_r: *const f32,
        out_l: *mut f32, out_r: *mut f32,
        num_samples: usize,
        _sc_l: *const f32, _sc_r: *const f32,
        arm_l: *const f32, _arm_r: *const f32,
    ) {
        if in_l.is_null() || out_l.is_null() || num_samples == 0 { return; }
        let in_r_ptr = if in_r.is_null() { in_l } else { in_r };
        let out_r_ptr = if out_r.is_null() { out_l } else { out_r };
        unsafe {
            let in_l_slice = std::slice::from_raw_parts(in_l, num_samples);
            let in_r_slice = std::slice::from_raw_parts(in_r_ptr, num_samples);
            let out_l_slice = std::slice::from_raw_parts_mut(out_l, num_samples);
            let out_r_slice = std::slice::from_raw_parts_mut(out_r_ptr, num_samples);
            let fm_opt = if !arm_l.is_null() { Some(std::slice::from_raw_parts(arm_l, num_samples)) } else { None };
            self.process_block(in_l_slice, in_r_slice, fm_opt, out_l_slice, out_r_slice, num_samples);
        }
    }
}

// --- 67. FractalResonator ---
impl RustProcessorNode for RustFractalResonator {
    #[inline(always)]
    fn type_id(&self) -> u32 { 67 }
    #[inline(always)]
    fn name(&self) -> &'static str { "FractalResonator" }
    #[inline(always)]
    fn prepare(&mut self, sample_rate: f64, _max_block_size: usize) { self.prepare(sample_rate as f32); }
    #[inline(always)]
    fn reset(&mut self) { self.reset(); }
    #[inline(always)]
    fn set_parameter(&mut self, param_id: u32, value: f32) {
        match param_id {
            1 => self.set_base_freq(value),
            2 => self.set_fractal_ratio(value),
            3 => self.set_decay(value),
            4 => self.set_damping(value),
            5 => self.set_stereo_width(value),
            6 => self.set_mix(value),
            _ => {}
        }
    }
    #[inline(always)]
    fn get_parameter(&self, _param_id: u32) -> f32 { 0.0 }
    #[inline(always)]
    fn process(
        &mut self,
        in_l: *const f32, in_r: *const f32,
        out_l: *mut f32, out_r: *mut f32,
        num_samples: usize,
        _sc_l: *const f32, _sc_r: *const f32,
        __arm_l: *const f32, _arm_r: *const f32,
    ) {
        if in_l.is_null() || out_l.is_null() || num_samples == 0 { return; }
        let in_r_ptr = if in_r.is_null() { in_l } else { in_r };
        let out_r_ptr = if out_r.is_null() { out_l } else { out_r };
        unsafe {
            let in_l_slice = std::slice::from_raw_parts(in_l, num_samples);
            let in_r_slice = std::slice::from_raw_parts(in_r_ptr, num_samples);
            let out_l_slice = std::slice::from_raw_parts_mut(out_l, num_samples);
            let out_r_slice = std::slice::from_raw_parts_mut(out_r_ptr, num_samples);
            self.process_block(in_l_slice, in_r_slice, out_l_slice, out_r_slice, num_samples);
        }
    }
}

// --- 68. VactrolLPG ---
impl RustProcessorNode for RustVactrolLPG {
    #[inline(always)]
    fn type_id(&self) -> u32 { 68 }
    #[inline(always)]
    fn name(&self) -> &'static str { "VactrolLPG" }
    #[inline(always)]
    fn prepare(&mut self, sample_rate: f64, _max_block_size: usize) { self.prepare(sample_rate as f32); }
    #[inline(always)]
    fn reset(&mut self) { self.reset(); }
    #[inline(always)]
    fn set_parameter(&mut self, param_id: u32, value: f32) {
        match param_id {
            1 => self.set_cutoff(value),
            2 => self.set_resonance(value),
            3 => self.set_decay_ms(value),
            4 => self.set_strike(value),
            5 => self.set_mode_balance(value),
            6 => self.set_drive(value),
            7 => self.set_mix(value),
            _ => {}
        }
    }
    #[inline(always)]
    fn get_parameter(&self, _param_id: u32) -> f32 { 0.0 }
    #[inline(always)]
    fn process(
        &mut self,
        in_l: *const f32, in_r: *const f32,
        out_l: *mut f32, out_r: *mut f32,
        num_samples: usize,
        _sc_l: *const f32, _sc_r: *const f32,
        arm_l: *const f32, _arm_r: *const f32,
    ) {
        if in_l.is_null() || out_l.is_null() || num_samples == 0 { return; }
        let in_r_ptr = if in_r.is_null() { in_l } else { in_r };
        let out_r_ptr = if out_r.is_null() { out_l } else { out_r };
        unsafe {
            let in_l_slice = std::slice::from_raw_parts(in_l, num_samples);
            let in_r_slice = std::slice::from_raw_parts(in_r_ptr, num_samples);
            let out_l_slice = std::slice::from_raw_parts_mut(out_l, num_samples);
            let out_r_slice = std::slice::from_raw_parts_mut(out_r_ptr, num_samples);
            let strike_opt = if !arm_l.is_null() { Some(std::slice::from_raw_parts(arm_l, num_samples)) } else { None };
            self.process_block(in_l_slice, in_r_slice, strike_opt, out_l_slice, out_r_slice, num_samples);
        }
    }
}

// --- 69. BarberpoleFilter ---
impl RustProcessorNode for RustBarberpoleFilter {
    #[inline(always)]
    fn type_id(&self) -> u32 { 69 }
    #[inline(always)]
    fn name(&self) -> &'static str { "BarberpoleFilter" }
    #[inline(always)]
    fn prepare(&mut self, sample_rate: f64, _max_block_size: usize) { self.prepare(sample_rate as f32); }
    #[inline(always)]
    fn reset(&mut self) { self.reset(); }
    #[inline(always)]
    fn set_parameter(&mut self, param_id: u32, value: f32) {
        match param_id {
            1 => self.set_rate(value),
            2 => self.set_feedback(value),
            3 => self.set_stereo_spread(value),
            4 => self.set_f_min(value),
            5 => self.set_f_max(value),
            6 => self.set_mix(value),
            _ => {}
        }
    }
    #[inline(always)]
    fn get_parameter(&self, _param_id: u32) -> f32 { 0.0 }
    #[inline(always)]
    fn process(
        &mut self,
        in_l: *const f32, in_r: *const f32,
        out_l: *mut f32, out_r: *mut f32,
        num_samples: usize,
        _sc_l: *const f32, _sc_r: *const f32,
        __arm_l: *const f32, _arm_r: *const f32,
    ) {
        if in_l.is_null() || out_l.is_null() || num_samples == 0 { return; }
        let in_r_ptr = if in_r.is_null() { in_l } else { in_r };
        let out_r_ptr = if out_r.is_null() { out_l } else { out_r };
        unsafe {
            let in_l_slice = std::slice::from_raw_parts(in_l, num_samples);
            let in_r_slice = std::slice::from_raw_parts(in_r_ptr, num_samples);
            let out_l_slice = std::slice::from_raw_parts_mut(out_l, num_samples);
            let out_r_slice = std::slice::from_raw_parts_mut(out_r_ptr, num_samples);
            self.process_block(in_l_slice, in_r_slice, out_l_slice, out_r_slice, num_samples);
        }
    }
}

// --- 70. SidechainPumper ---
impl RustProcessorNode for RustSidechainPumper {
    #[inline(always)]
    fn type_id(&self) -> u32 { 70 }
    #[inline(always)]
    fn name(&self) -> &'static str { "SidechainPumper" }
    #[inline(always)]
    fn prepare(&mut self, sample_rate: f64, _max_block_size: usize) { self.prepare(sample_rate as f32); }
    #[inline(always)]
    fn reset(&mut self) { self.reset(); }
    #[inline(always)]
    fn set_parameter(&mut self, param_id: u32, value: f32) { self.set_param(param_id, value); }
    #[inline(always)]
    fn get_parameter(&self, _param_id: u32) -> f32 { 0.0 }
    #[inline(always)]
    fn process(
        &mut self,
        in_l: *const f32, in_r: *const f32,
        out_l: *mut f32, out_r: *mut f32,
        num_samples: usize,
        sc_l: *const f32, sc_r: *const f32,
        __arm_l: *const f32, _arm_r: *const f32,
    ) {
        self.process(in_l, in_r, out_l, out_r, sc_l, sc_r, num_samples as u32);
    }
}

// --- 71. FrequencySplitPanner ---
impl RustProcessorNode for RustFrequencySplitPanner {
    #[inline(always)]
    fn type_id(&self) -> u32 { 71 }
    #[inline(always)]
    fn name(&self) -> &'static str { "FrequencySplitPanner" }
    #[inline(always)]
    fn prepare(&mut self, sample_rate: f64, _max_block_size: usize) { self.prepare(sample_rate as f32); }
    #[inline(always)]
    fn reset(&mut self) { self.reset(); }
    #[inline(always)]
    fn set_parameter(&mut self, param_id: u32, value: f32) { self.set_param(param_id, value); }
    #[inline(always)]
    fn get_parameter(&self, _param_id: u32) -> f32 { 0.0 }
    #[inline(always)]
    fn process(
        &mut self,
        in_l: *const f32, in_r: *const f32,
        out_l: *mut f32, out_r: *mut f32,
        num_samples: usize,
        _sc_l: *const f32, _sc_r: *const f32,
        __arm_l: *const f32, _arm_r: *const f32,
    ) {
        self.process(in_l, in_r, out_l, out_r, num_samples as u32);
    }
}

// --- 72. ChebyshevHarmonicShaper ---
impl RustProcessorNode for RustChebyshevHarmonicShaper {
    #[inline(always)]
    fn type_id(&self) -> u32 { 72 }
    #[inline(always)]
    fn name(&self) -> &'static str { "ChebyshevHarmonicShaper" }
    #[inline(always)]
    fn prepare(&mut self, sample_rate: f64, _max_block_size: usize) { self.prepare(sample_rate as f32); }
    #[inline(always)]
    fn reset(&mut self) { self.reset(); }
    #[inline(always)]
    fn set_parameter(&mut self, param_id: u32, value: f32) { self.set_param(param_id, value); }
    #[inline(always)]
    fn get_parameter(&self, _param_id: u32) -> f32 { 0.0 }
    #[inline(always)]
    fn process(
        &mut self,
        in_l: *const f32, in_r: *const f32,
        out_l: *mut f32, out_r: *mut f32,
        num_samples: usize,
        _sc_l: *const f32, _sc_r: *const f32,
        __arm_l: *const f32, _arm_r: *const f32,
    ) {
        self.process(in_l, in_r, out_l, out_r, num_samples as u32);
    }
}

// --- 73. SubharmonicSynthesizer ---
impl RustProcessorNode for RustSubharmonicSynthesizer {
    #[inline(always)]
    fn type_id(&self) -> u32 { 73 }
    #[inline(always)]
    fn name(&self) -> &'static str { "SubharmonicSynthesizer" }
    #[inline(always)]
    fn prepare(&mut self, sample_rate: f64, _max_block_size: usize) { self.prepare(sample_rate as f32); }
    #[inline(always)]
    fn reset(&mut self) { self.reset(); }
    #[inline(always)]
    fn set_parameter(&mut self, param_id: u32, value: f32) { self.set_param(param_id, value); }
    #[inline(always)]
    fn get_parameter(&self, _param_id: u32) -> f32 { 0.0 }
    #[inline(always)]
    fn process(
        &mut self,
        in_l: *const f32, in_r: *const f32,
        out_l: *mut f32, out_r: *mut f32,
        num_samples: usize,
        _sc_l: *const f32, _sc_r: *const f32,
        __arm_l: *const f32, _arm_r: *const f32,
    ) {
        self.process(in_l, in_r, out_l, out_r, num_samples as u32);
    }
}

// --- 74. TransauralCrosstalkCanceller ---
impl RustProcessorNode for RustTransauralCrosstalkCanceller {
    #[inline(always)]
    fn type_id(&self) -> u32 { 74 }
    #[inline(always)]
    fn name(&self) -> &'static str { "TransauralCrosstalkCanceller" }
    #[inline(always)]
    fn prepare(&mut self, sample_rate: f64, _max_block_size: usize) { self.prepare(sample_rate as f32); }
    #[inline(always)]
    fn reset(&mut self) { self.reset(); }
    #[inline(always)]
    fn set_parameter(&mut self, param_id: u32, value: f32) { self.set_param(param_id, value); }
    #[inline(always)]
    fn get_parameter(&self, _param_id: u32) -> f32 { 0.0 }
    #[inline(always)]
    fn process(
        &mut self,
        in_l: *const f32, in_r: *const f32,
        out_l: *mut f32, out_r: *mut f32,
        num_samples: usize,
        _sc_l: *const f32, _sc_r: *const f32,
        __arm_l: *const f32, _arm_r: *const f32,
    ) {
        self.process(in_l, in_r, out_l, out_r, num_samples as u32);
    }
}

// --- 75. DopplerOrbitalSpatializer ---
impl RustProcessorNode for RustDopplerOrbitalSpatializer {
    #[inline(always)]
    fn type_id(&self) -> u32 { 75 }
    #[inline(always)]
    fn name(&self) -> &'static str { "DopplerOrbitalSpatializer" }
    #[inline(always)]
    fn prepare(&mut self, sample_rate: f64, _max_block_size: usize) { self.prepare(sample_rate as f32); }
    #[inline(always)]
    fn reset(&mut self) { self.reset(); }
    #[inline(always)]
    fn set_parameter(&mut self, param_id: u32, value: f32) { self.set_param(param_id, value); }
    #[inline(always)]
    fn get_parameter(&self, _param_id: u32) -> f32 { 0.0 }
    #[inline(always)]
    fn process(
        &mut self,
        in_l: *const f32, in_r: *const f32,
        out_l: *mut f32, out_r: *mut f32,
        num_samples: usize,
        _sc_l: *const f32, _sc_r: *const f32,
        __arm_l: *const f32, _arm_r: *const f32,
    ) {
        self.process(in_l, in_r, out_l, out_r, num_samples as u32);
    }
}

// --- 76. GranularBufferShuffler ---
impl RustProcessorNode for RustGranularBufferShuffler {
    #[inline(always)]
    fn type_id(&self) -> u32 { 76 }
    #[inline(always)]
    fn name(&self) -> &'static str { "GranularBufferShuffler" }
    #[inline(always)]
    fn prepare(&mut self, sample_rate: f64, _max_block_size: usize) { self.prepare(sample_rate as f32); }
    #[inline(always)]
    fn reset(&mut self) { self.reset(); }
    #[inline(always)]
    fn set_parameter(&mut self, param_id: u32, value: f32) { self.set_param(param_id, value); }
    #[inline(always)]
    fn get_parameter(&self, _param_id: u32) -> f32 { 0.0 }
    #[inline(always)]
    fn process(
        &mut self,
        in_l: *const f32, in_r: *const f32,
        out_l: *mut f32, out_r: *mut f32,
        num_samples: usize,
        _sc_l: *const f32, _sc_r: *const f32,
        __arm_l: *const f32, _arm_r: *const f32,
    ) {
        self.process(in_l, in_r, out_l, out_r, num_samples as u32);
    }
}

// --- 77. DisintegrationLooper ---
impl RustProcessorNode for RustDisintegrationLooper {
    #[inline(always)]
    fn type_id(&self) -> u32 { 77 }
    #[inline(always)]
    fn name(&self) -> &'static str { "DisintegrationLooper" }
    #[inline(always)]
    fn prepare(&mut self, sample_rate: f64, _max_block_size: usize) { self.prepare(sample_rate as f32); }
    #[inline(always)]
    fn reset(&mut self) { self.reset(); }
    #[inline(always)]
    fn set_parameter(&mut self, param_id: u32, value: f32) { self.set_param(param_id, value); }
    #[inline(always)]
    fn get_parameter(&self, _param_id: u32) -> f32 { 0.0 }
    #[inline(always)]
    fn process(
        &mut self,
        in_l: *const f32, in_r: *const f32,
        out_l: *mut f32, out_r: *mut f32,
        num_samples: usize,
        _sc_l: *const f32, _sc_r: *const f32,
        __arm_l: *const f32, _arm_r: *const f32,
    ) {
        self.process(in_l, in_r, out_l, out_r, num_samples as u32);
    }
}

// --- 78. SpringReverbTank ---
impl RustProcessorNode for RustSpringReverbTank {
    #[inline(always)]
    fn type_id(&self) -> u32 { 78 }
    #[inline(always)]
    fn name(&self) -> &'static str { "SpringReverbTank" }
    #[inline(always)]
    fn prepare(&mut self, sample_rate: f64, _max_block_size: usize) { self.prepare(sample_rate as f32); }
    #[inline(always)]
    fn reset(&mut self) { self.reset(); }
    #[inline(always)]
    fn set_parameter(&mut self, param_id: u32, value: f32) { self.set_param(param_id, value); }
    #[inline(always)]
    fn get_parameter(&self, _param_id: u32) -> f32 { 0.0 }
    #[inline(always)]
    fn process(
        &mut self,
        in_l: *const f32, in_r: *const f32,
        out_l: *mut f32, out_r: *mut f32,
        num_samples: usize,
        _sc_l: *const f32, _sc_r: *const f32,
        __arm_l: *const f32, _arm_r: *const f32,
    ) {
        self.process(in_l, in_r, out_l, out_r, num_samples as u32);
    }
}

// --- 79. DualBodeFrequencyShifter ---
impl RustProcessorNode for RustDualBodeFrequencyShifter {
    #[inline(always)]
    fn type_id(&self) -> u32 { 79 }
    #[inline(always)]
    fn name(&self) -> &'static str { "DualBodeFrequencyShifter" }
    #[inline(always)]
    fn prepare(&mut self, sample_rate: f64, _max_block_size: usize) { self.prepare(sample_rate as f32); }
    #[inline(always)]
    fn reset(&mut self) { self.reset(); }
    #[inline(always)]
    fn set_parameter(&mut self, param_id: u32, value: f32) { self.set_param(param_id, value); }
    #[inline(always)]
    fn get_parameter(&self, _param_id: u32) -> f32 { 0.0 }
    #[inline(always)]
    fn process(
        &mut self,
        in_l: *const f32, in_r: *const f32,
        out_l: *mut f32, out_r: *mut f32,
        num_samples: usize,
        _sc_l: *const f32, _sc_r: *const f32,
        __arm_l: *const f32, _arm_r: *const f32,
    ) {
        self.process(in_l, in_r, out_l, out_r, num_samples as u32);
    }
}

// --- 80. MicrotonalHarmonizer ---
impl RustProcessorNode for RustMicrotonalHarmonizer {
    #[inline(always)]
    fn type_id(&self) -> u32 { 80 }
    #[inline(always)]
    fn name(&self) -> &'static str { "MicrotonalHarmonizer" }
    #[inline(always)]
    fn prepare(&mut self, sample_rate: f64, _max_block_size: usize) { self.prepare(sample_rate as f32); }
    #[inline(always)]
    fn reset(&mut self) { self.reset(); }
    #[inline(always)]
    fn set_parameter(&mut self, param_id: u32, value: f32) { self.set_param(param_id, value); }
    #[inline(always)]
    fn get_parameter(&self, _param_id: u32) -> f32 { 0.0 }
    #[inline(always)]
    fn process(
        &mut self,
        in_l: *const f32, in_r: *const f32,
        out_l: *mut f32, out_r: *mut f32,
        num_samples: usize,
        _sc_l: *const f32, _sc_r: *const f32,
        __arm_l: *const f32, _arm_r: *const f32,
    ) {
        self.process(in_l, in_r, out_l, out_r, num_samples as u32);
    }
}

// --- 81. FormantPitchShifter ---
impl RustProcessorNode for RustFormantPitchShifter {
    #[inline(always)]
    fn type_id(&self) -> u32 { 81 }
    #[inline(always)]
    fn name(&self) -> &'static str { "FormantPitchShifter" }
    #[inline(always)]
    fn prepare(&mut self, sample_rate: f64, _max_block_size: usize) { self.prepare(sample_rate as f32); }
    #[inline(always)]
    fn reset(&mut self) { self.reset(); }
    #[inline(always)]
    fn set_parameter(&mut self, param_id: u32, value: f32) { self.set_param(param_id, value); }
    #[inline(always)]
    fn get_parameter(&self, _param_id: u32) -> f32 { 0.0 }
    #[inline(always)]
    fn process(
        &mut self,
        in_l: *const f32, in_r: *const f32,
        out_l: *mut f32, out_r: *mut f32,
        num_samples: usize,
        _sc_l: *const f32, _sc_r: *const f32,
        __arm_l: *const f32, _arm_r: *const f32,
    ) {
        self.process(in_l, in_r, out_l, out_r, num_samples as u32);
    }
}

// --- 82. SpectralResonanceTamer ---
impl RustProcessorNode for RustSpectralResonanceTamer {
    #[inline(always)]
    fn type_id(&self) -> u32 { 82 }
    #[inline(always)]
    fn name(&self) -> &'static str { "SpectralResonanceTamer" }
    #[inline(always)]
    fn prepare(&mut self, sample_rate: f64, _max_block_size: usize) { self.prepare(sample_rate as f32); }
    #[inline(always)]
    fn reset(&mut self) { self.reset(); }
    #[inline(always)]
    fn set_parameter(&mut self, param_id: u32, value: f32) { self.set_param(param_id, value); }
    #[inline(always)]
    fn get_parameter(&self, _param_id: u32) -> f32 { 0.0 }
    #[inline(always)]
    fn process(
        &mut self,
        in_l: *const f32, in_r: *const f32,
        out_l: *mut f32, out_r: *mut f32,
        num_samples: usize,
        _sc_l: *const f32, _sc_r: *const f32,
        __arm_l: *const f32, _arm_r: *const f32,
    ) {
        self.process(in_l, in_r, out_l, out_r, num_samples as u32);
    }
}

// --- 83. SpectralMorphingConvolver ---
impl RustProcessorNode for RustSpectralMorphingConvolver {
    #[inline(always)]
    fn type_id(&self) -> u32 { 83 }
    #[inline(always)]
    fn name(&self) -> &'static str { "SpectralMorphingConvolver" }
    #[inline(always)]
    fn prepare(&mut self, sample_rate: f64, _max_block_size: usize) { self.prepare(sample_rate as f32); }
    #[inline(always)]
    fn reset(&mut self) { self.reset(); }
    #[inline(always)]
    fn set_parameter(&mut self, param_id: u32, value: f32) { self.set_param(param_id, value); }
    #[inline(always)]
    fn get_parameter(&self, _param_id: u32) -> f32 { 0.0 }
    #[inline(always)]
    fn process(
        &mut self,
        in_l: *const f32, in_r: *const f32,
        out_l: *mut f32, out_r: *mut f32,
        num_samples: usize,
        _sc_l: *const f32, _sc_r: *const f32,
        __arm_l: *const f32, _arm_r: *const f32,
    ) {
        self.process(in_l, in_r, out_l, out_r, num_samples as u32);
    }
}

// ---------------------------------------------------------------------------
// RustNodeInstance & RustProcessorRegistry
// ---------------------------------------------------------------------------

/// Runtime graph node wrapper (Rule 5).
pub struct RustNodeInstance {
    pub node_id: u32,
    pub type_id: u32,
    pub is_bypassed: bool,
    pub processor: Box<dyn RustProcessorNode>,
}

impl RustNodeInstance {
    pub fn new(node_id: u32, type_id: u32, processor: Box<dyn RustProcessorNode>) -> Self {
        Self {
            node_id,
            type_id,
            is_bypassed: false,
            processor,
        }
    }
}

pub type NodeCreator = fn() -> Box<dyn RustProcessorNode>;

pub const MAX_NODE_TYPES: usize = 128;

/// Extensible O(1) Processor Registry & Factory (Rules 19, 20, 46).
pub struct RustProcessorRegistry {
    creators: [Option<NodeCreator>; MAX_NODE_TYPES],
    names: [&'static str; MAX_NODE_TYPES],
}

impl RustProcessorRegistry {
    pub fn new() -> Self {
        let mut reg = Self {
            creators: [None; MAX_NODE_TYPES],
            names: ["Unknown"; MAX_NODE_TYPES],
        };
        reg.register_builtins();
        reg
    }

    pub fn register(&mut self, type_id: u32, name: &'static str, creator: NodeCreator) {
        let idx = type_id as usize;
        if idx < MAX_NODE_TYPES {
            self.creators[idx] = Some(creator);
            self.names[idx] = name;
        }
    }

    #[inline(always)]
    pub fn create_node(&self, type_id: u32) -> Option<Box<dyn RustProcessorNode>> {
        let idx = type_id as usize;
        if idx < MAX_NODE_TYPES {
            if let Some(creator) = self.creators[idx] {
                return Some(creator());
            }
        }
        None
    }

    #[inline(always)]
    pub fn get_name(&self, type_id: u32) -> &'static str {
        let idx = type_id as usize;
        if idx < MAX_NODE_TYPES {
            self.names[idx]
        } else {
            "Unknown"
        }
    }

    fn register_builtins(&mut self) {
        // Built-in foundational nodes
        self.register(1, "Input", || Box::new(RustPassthrough::new()));
        self.register(2, "Output", || Box::new(RustPassthrough::new()));
        self.register(3, "Passthrough", || Box::new(RustPassthrough::new()));
        self.register(4, "Filter", || Box::new(RustSimpleFilter::new()));
        self.register(5, "Delay", || Box::new(RustSimpleDelay::new()));
        self.register(6, "Reverb", || Box::new(RustReverb::new(44100.0)));
        self.register(8, "Distortion", || Box::new(RustDistortion::new(44100.0)));
        self.register(9, "Compressor", || Box::new(RustCompressor::new(44100.0)));
        self.register(10, "Container", || Box::new(RustContainerCore::new(44100.0)));
        self.register(16, "Phaser", || Box::new(RustPhaser::new(44100.0)));
        self.register(17, "Chorus", || Box::new(RustChorus::new(44100.0)));
        self.register(18, "Flanger", || Box::new(RustFlanger::new(44100.0)));
        self.register(19, "RingModulator", || Box::new(RustRingModulator::new(44100.0)));
        self.register(20, "FrequencyShifter", || Box::new(RustFrequencyShifter::new(44100.0)));
        self.register(21, "Tape", || Box::new(RustTapeSaturation::new(44100.0)));
        self.register(24, "MidSideEncoder", || Box::new(RustMidSideEncoder::new()));
        self.register(25, "MidSideDecoder", || Box::new(RustMidSideDecoder::new(44100.0)));
        self.register(26, "SpatialPanner", || Box::new(RustSpatialPanner::new(44100.0)));
        self.register(27, "ParametricEQ", || Box::new(RustParametricEq::new(44100.0)));
        self.register(28, "AdvancedDelay", || Box::new(RustAdvancedDelay::new(44100.0)));
        self.register(30, "TapeStop", || Box::new(RustTapeStop::new(44100.0)));
        self.register(7, "Granular", || Box::new(RustGranular::new(44100.0)));
        self.register(11, "PitchShifter", || Box::new(RustPitchShifter::new()));
        self.register(12, "Spectral", || Box::new(RustSpectralProcessor::new()));
        self.register(13, "Resonator", || Box::new(RustResonatorBank::new(44100.0)));
        self.register(14, "Glitch", || Box::new(RustGlitch::new(44100.0)));
        self.register(15, "Multiband", || Box::new(RustMultibandDynamics::new(44100.0)));
        self.register(22, "Feedback", || Box::new(FeedbackContainerCore::new(44100.0)));
        self.register(23, "EventContainer", || Box::new(EventContainerCore::new(44100.0)));
        self.register(29, "SpectralProcessor", || Box::new(RustSpectralProcessor::new()));
        self.register(31, "FormantFilter", || Box::new(RustFormantFilter::new(44100.0)));
        self.register(32, "NoiseTexture", || Box::new(RustNoiseTexture::new(44100.0)));
        self.register(33, "TransientShaper", || Box::new(RustTransientShaper::new(44100.0)));
        self.register(34, "RotarySpeaker", || Box::new(RustRotarySpeaker::new(44100.0)));
        self.register(35, "HarmonicExciter", || Box::new(RustHarmonicExciter::new(44100.0)));
        self.register(36, "Vocoder", || Box::new(RustVocoder::new(44100.0)));
        self.register(37, "KarplusStrong", || Box::new(RustKarplusStrong::new(44100.0)));
        self.register(38, "ReverseReverb", || Box::new(RustReverseReverb::new(44100.0)));
        self.register(39, "BrickwallLimiter", || Box::new(RustBrickwallLimiter::new(44100.0)));
        self.register(40, "Bitcrusher", || Box::new(RustBitcrusher::new(44100.0)));
        self.register(41, "NoiseGate", || Box::new(RustNoiseGate::new(44100.0)));
        self.register(42, "DeEsser", || Box::new(RustDeEsser::new(44100.0)));
        self.register(43, "MidiArpeggiator", || Box::new(MidiArpeggiatorCore::new()));
        self.register(44, "MidiChordEngine", || Box::new(MidiChordEngineCore::new()));
        self.register(45, "MidiScaleQuantizer", || Box::new(MidiScaleQuantizerCore::new()));
        self.register(46, "ExternalSidechain", || Box::new(ExternalSidechainCore::new()));
        self.register(47, "Convolution", || Box::new(RustConvolution::new()));
        self.register(48, "Oversampler", || Box::new(RustOversampler::new()));
        self.register(49, "AudioSlicer", || Box::new(RustAudioSlicer::new(44100.0)));
        self.register(50, "ShimmerReverb", || Box::new(RustShimmerReverb::new(44100.0)));
        self.register(51, "Refraction", || Box::new(RustRefraction::new(44100.0)));
        self.register(52, "SpectralSmear", || Box::new(RustSpectralSmear::new()));
        self.register(53, "SamplePlayer", || Box::new(RustSamplePlayer::new()));
        self.register(54, "DynamicEq", || Box::new(RustDynamicEq::new(44100.0)));
        self.register(55, "StereoImager", || Box::new(RustStereoImager::new(44100.0)));
        self.register(56, "LoFiCassette", || Box::new(RustLoFiCassette::new(44100.0)));
        self.register(57, "AutoWahFilter", || Box::new(RustAutoWah::new(44100.0)));
        self.register(58, "ModalResonator", || Box::new(RustModalResonator::new(44100.0)));
        self.register(59, "RustWaveguide", || Box::new(RustWaveguide::new(44100.0)));
        self.register(60, "DimensionChorus", || Box::new(RustDimensionChorus::new(44100.0)));
        self.register(61, "DynamicTremolo", || Box::new(RustDynamicTremolo::new(44100.0)));
        self.register(62, "MultibandSaturator", || Box::new(RustMultibandSaturator::new(44100.0)));
        self.register(63, "CombResonator", || Box::new(RustCombResonator::new(44100.0)));
        self.register(64, "PitchDelay", || Box::new(RustPitchDelay::new(44100.0)));
        self.register(65, "PhaseDispersion", || Box::new(RustPhaseDispersion::new(44100.0)));
        self.register(66, "MorphingSVF", || Box::new(RustMorphingSvf::new(44100.0)));
        self.register(67, "FractalResonator", || Box::new(RustFractalResonator::new(44100.0)));
        self.register(68, "VactrolLPG", || Box::new(RustVactrolLPG::new(44100.0)));
        self.register(69, "BarberpoleFilter", || Box::new(RustBarberpoleFilter::new(44100.0)));
        self.register(70, "SidechainPumper", || Box::new(RustSidechainPumper::new(44100.0)));
        self.register(71, "FrequencySplitPanner", || Box::new(RustFrequencySplitPanner::new(44100.0)));
        self.register(72, "ChebyshevHarmonicShaper", || Box::new(RustChebyshevHarmonicShaper::new(44100.0)));
        self.register(73, "SubharmonicSynthesizer", || Box::new(RustSubharmonicSynthesizer::new(44100.0)));
        self.register(74, "TransauralCrosstalkCanceller", || Box::new(RustTransauralCrosstalkCanceller::new(44100.0)));
        self.register(75, "DopplerOrbitalSpatializer", || Box::new(RustDopplerOrbitalSpatializer::new(44100.0)));
        self.register(76, "GranularBufferShuffler", || Box::new(RustGranularBufferShuffler::new(44100.0)));
        self.register(77, "DisintegrationLooper", || Box::new(RustDisintegrationLooper::new(44100.0)));
        self.register(78, "SpringReverbTank", || Box::new(RustSpringReverbTank::new(44100.0)));
        self.register(79, "DualBodeFrequencyShifter", || Box::new(RustDualBodeFrequencyShifter::new(44100.0)));
        self.register(80, "MicrotonalHarmonizer", || Box::new(RustMicrotonalHarmonizer::new(44100.0)));
        self.register(81, "FormantPitchShifter", || Box::new(RustFormantPitchShifter::new(44100.0)));
        self.register(82, "SpectralResonanceTamer", || Box::new(RustSpectralResonanceTamer::new(44100.0)));
        self.register(83, "SpectralMorphingConvolver", || Box::new(RustSpectralMorphingConvolver::new(44100.0)));
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_all_83_nodes_in_registry() {
        let reg = RustProcessorRegistry::new();
        let in_l = vec![0.3f32; 128];
        let in_r = vec![-0.3f32; 128];
        let mut out_l = vec![0.0f32; 128];
        let mut out_r = vec![0.0f32; 128];

        for type_id in 1..=83 {
            let name = reg.get_name(type_id);
            assert_ne!(name, "Unknown", "Node type {} must have a registered name", type_id);

            let mut node = reg.create_node(type_id).expect(&format!("Node {} failed to instantiate", type_id));

            node.prepare(48000.0, 256);
            node.reset();

            // Set and get parameter test
            node.set_parameter(1, 0.5);
            let _ = node.get_parameter(1);

            // Process test
            node.process(
                in_l.as_ptr(),
                in_r.as_ptr(),
                out_l.as_mut_ptr(),
                out_r.as_mut_ptr(),
                128,
                std::ptr::null(),
                std::ptr::null(),
                std::ptr::null(),
                std::ptr::null(),
            );

            // Check no NaN or Inf
            for s in 0..128 {
                assert!(!out_l[s].is_nan() && !out_l[s].is_infinite());
                assert!(!out_r[s].is_nan() && !out_r[s].is_infinite());
            }
        }
    }
}
