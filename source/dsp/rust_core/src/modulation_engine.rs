//! Sovereign Universal Modulation Engine Core (Rules 7, 8, 25, 37, 46, 47)
//! Coordinates 4 LFOs, 2 Envelopes, StepSequencer, AudioFollower, 8 Macros + XY Pad,
//! 2 MSEGs, 2 Euclidean modulators, Lorenz Chaos, and 64-route ModulationMatrix.
//! Zero heap allocations in audio processing thread.

use crate::lfo::RustLFO;
use crate::envelope_generator::RustEnvelopeGenerator;
use crate::audio_follower::RustAudioFollower;
use crate::chaos_modulator::RustChaosModulator;
use crate::step_sequencer::RustStepSequencer;
use crate::euclidean_modulator::RustEuclideanModulator;
use crate::mseg_modulator::RustMSEGModulator;
use crate::modulation_matrix::{RustMacroManager, RustModulationMatrix, MOD_SOURCE_COUNT};

pub struct RustModulationEngine {
    sample_rate: f64,
    pub lfos: [RustLFO; 4],
    pub envelopes: [RustEnvelopeGenerator; 2],
    pub step_seq: RustStepSequencer,
    pub audio_follower: RustAudioFollower,
    pub macro_manager: RustMacroManager,
    pub matrix: RustModulationMatrix,
    pub msegs: [RustMSEGModulator; 2],
    pub euclideans: [RustEuclideanModulator; 2],
    pub chaos: RustChaosModulator,
    source_values: [f32; MOD_SOURCE_COUNT],
}

impl RustModulationEngine {
    pub fn new() -> Self {
        Self {
            sample_rate: 44100.0,
            lfos: [RustLFO::new(), RustLFO::new(), RustLFO::new(), RustLFO::new()],
            envelopes: [RustEnvelopeGenerator::new(), RustEnvelopeGenerator::new()],
            step_seq: RustStepSequencer::new(),
            audio_follower: RustAudioFollower::new(),
            macro_manager: RustMacroManager::new(),
            matrix: RustModulationMatrix::new(),
            msegs: [RustMSEGModulator::new(), RustMSEGModulator::new()],
            euclideans: [RustEuclideanModulator::new(), RustEuclideanModulator::new()],
            chaos: RustChaosModulator::new(),
            source_values: [0.0; MOD_SOURCE_COUNT],
        }
    }

    pub fn prepare(&mut self, sample_rate: f64) {
        self.sample_rate = if sample_rate > 0.0 { sample_rate } else { 44100.0 };
        for lfo in &mut self.lfos {
            lfo.prepare(self.sample_rate);
        }
        for env in &mut self.envelopes {
            env.prepare(self.sample_rate);
        }
        self.step_seq.prepare(self.sample_rate);
        self.audio_follower.prepare(self.sample_rate);
        for mseg in &mut self.msegs {
            mseg.prepare(self.sample_rate);
        }
        for euc in &mut self.euclideans {
            euc.prepare(self.sample_rate);
        }
        self.chaos.prepare(self.sample_rate);
    }

    pub fn reset(&mut self) {
        for lfo in &mut self.lfos {
            lfo.reset();
        }
        for env in &mut self.envelopes {
            env.reset();
        }
        self.step_seq.reset();
        self.audio_follower.reset();
        self.macro_manager.reset();
        for mseg in &mut self.msegs {
            mseg.reset();
        }
        for euc in &mut self.euclideans {
            euc.reset();
        }
        self.chaos.reset();
        self.source_values.fill(0.0);
    }

    #[inline]
    pub fn update_analysis_sources(
        &mut self,
        transient: f32,
        pitch_normalized: f32,
        centroid: f32,
        flux: f32,
    ) {
        self.source_values[20] = transient;        // AnalysisTransient = 20
        self.source_values[21] = pitch_normalized; // AnalysisPitch = 21
        self.source_values[22] = centroid;         // AnalysisCentroid = 22
        self.source_values[23] = flux;             // AnalysisFlux = 23
    }

    #[inline]
    pub fn get_source_value(&self, source_type: u16) -> f32 {
        let idx = source_type as usize;
        if idx < MOD_SOURCE_COUNT {
            self.source_values[idx]
        } else {
            0.0
        }
    }

    #[inline]
    pub fn calculate_modulation_offset(&self, target_node: u32, target_param: u32) -> f32 {
        self.matrix.calculate_modulation_offset(target_node, target_param, &self.source_values)
    }

    #[inline]
    pub fn process_sources(
        &mut self,
        bpm: f64,
        ppq_position: f64,
        is_host_playing: bool,
        input_channels: *const *const f32,
        num_channels: u32,
        num_samples: u32,
    ) {
        // 1. Macros smoothing
        self.macro_manager.process_smoothing();

        // 2. Audio Follower
        let follower_val = self.audio_follower.process_block(input_channels, num_channels, num_samples);

        // 3. LFOs
        for lfo in &mut self.lfos {
            lfo.process_sample(bpm, ppq_position, is_host_playing);
        }

        // 4. Envelopes
        for env in &mut self.envelopes {
            env.process_sample();
        }

        // 5. Step Sequencer
        self.step_seq.process_sample(ppq_position, is_host_playing);

        // 6. MSEGs
        for mseg in &mut self.msegs {
            mseg.process_sample(bpm, ppq_position, is_host_playing);
        }

        // 7. Euclideans
        for euc in &mut self.euclideans {
            euc.process_sample(bpm, ppq_position, is_host_playing);
        }

        // 8. Chaos
        self.chaos.process_sample();

        // 9. Rellenar valores de fuentes en la tabla unificada
        self.source_values[1] = self.lfos[0].get_current_value();
        self.source_values[2] = self.lfos[1].get_current_value();
        self.source_values[3] = self.lfos[2].get_current_value();
        self.source_values[4] = self.lfos[3].get_current_value();

        self.source_values[5] = self.envelopes[0].get_current_value();
        self.source_values[6] = self.envelopes[1].get_current_value();

        self.source_values[7] = self.step_seq.get_current_value();
        self.source_values[8] = follower_val;

        // 10..17 Macros: Texture, Motion, Space, Color, Chaos, Density, Energy, Morph
        for i in 0..8 {
            self.source_values[10 + i] = self.macro_manager.get_smoothed_macro(i);
        }

        // 18..19 Pad XY
        self.source_values[18] = self.macro_manager.get_smoothed_pad_x();
        self.source_values[19] = self.macro_manager.get_smoothed_pad_y();

        // 24..25 MSEGs
        self.source_values[24] = self.msegs[0].get_current_value();
        self.source_values[25] = self.msegs[1].get_current_value();

        // 26..27 Euclideans
        self.source_values[26] = self.euclideans[0].get_current_value();
        self.source_values[27] = self.euclideans[1].get_current_value();

        // 28..30 Chaos
        self.source_values[28] = self.chaos.get_normalized_x();
        self.source_values[29] = self.chaos.get_normalized_y();
        self.source_values[30] = self.chaos.get_normalized_z();
    }
}
