#pragma once

#include <cstdint>
#include <cstddef>

#ifdef __cplusplus
extern "C" {
#endif

// Tipo opaco para el estado del resonador de guía de ondas en Rust
struct RustWaveguide;

RustWaveguide* n8_rust_waveguide_create(double sample_rate);
void n8_rust_waveguide_destroy(RustWaveguide* ptr);
void n8_rust_waveguide_reset(RustWaveguide* ptr);
void n8_rust_waveguide_set_sample_rate(RustWaveguide* ptr, double sample_rate);
void n8_rust_waveguide_set_parameter(RustWaveguide* ptr, uint32_t param_id, float value);
float n8_rust_waveguide_get_parameter(const RustWaveguide* ptr, uint32_t param_id);
void n8_rust_waveguide_process(
    RustWaveguide* ptr,
    const float* in_l,
    const float* in_r,
    float* out_l,
    float* out_r,
    uint32_t num_samples
);

// Tipo opaco para el modelo físico Karplus-Strong en Rust
struct RustKarplusStrong;

RustKarplusStrong* n8_rust_karplus_create(double sample_rate);
void n8_rust_karplus_destroy(RustKarplusStrong* ptr);
void n8_rust_karplus_reset(RustKarplusStrong* ptr);
void n8_rust_karplus_set_sample_rate(RustKarplusStrong* ptr, double sample_rate);
void n8_rust_karplus_set_parameter(RustKarplusStrong* ptr, uint32_t param_id, float value);
float n8_rust_karplus_get_parameter(const RustKarplusStrong* ptr, uint32_t param_id);
void n8_rust_karplus_process(
    RustKarplusStrong* ptr,
    const float* in_l,
    const float* in_r,
    float* out_l,
    float* out_r,
    uint32_t num_samples
);

// Tipo opaco para el resonador físico modal en Rust
struct RustModalResonator;

RustModalResonator* n8_rust_modal_resonator_create(double sample_rate);
void n8_rust_modal_resonator_destroy(RustModalResonator* ptr);
void n8_rust_modal_resonator_reset(RustModalResonator* ptr);
void n8_rust_modal_resonator_set_sample_rate(RustModalResonator* ptr, double sample_rate);
void n8_rust_modal_resonator_set_parameter(RustModalResonator* ptr, uint32_t param_id, float value);
float n8_rust_modal_resonator_get_parameter(const RustModalResonator* ptr, uint32_t param_id);
void n8_rust_modal_resonator_process(
    RustModalResonator* ptr,
    const float* in_l,
    const float* in_r,
    float* out_l,
    float* out_r,
    uint32_t num_samples
);

// Tipo opaco para la saturación de cinta magnética e histéresis Jiles-Atherton en Rust
struct RustTapeSaturation;

RustTapeSaturation* n8_rust_tape_saturation_create(double sample_rate);
void n8_rust_tape_saturation_destroy(RustTapeSaturation* ptr);
void n8_rust_tape_saturation_reset(RustTapeSaturation* ptr);
void n8_rust_tape_saturation_set_sample_rate(RustTapeSaturation* ptr, double sample_rate);
void n8_rust_tape_saturation_set_parameter(RustTapeSaturation* ptr, uint32_t param_id, float value);
float n8_rust_tape_saturation_get_parameter(const RustTapeSaturation* ptr, uint32_t param_id);
void n8_rust_tape_saturation_process(
    RustTapeSaturation* ptr,
    const float* in_l,
    const float* in_r,
    float* out_l,
    float* out_r,
    uint32_t num_samples
);

// Tipo opaco para el motor de reverberación Shimmer FDN en Rust
struct RustShimmerReverb;

RustShimmerReverb* n8_rust_shimmer_reverb_create(double sample_rate);
void n8_rust_shimmer_reverb_destroy(RustShimmerReverb* ptr);
void n8_rust_shimmer_reverb_reset(RustShimmerReverb* ptr);
void n8_rust_shimmer_reverb_set_sample_rate(RustShimmerReverb* ptr, double sample_rate);
void n8_rust_shimmer_reverb_set_parameter(RustShimmerReverb* ptr, uint32_t param_id, float value);
float n8_rust_shimmer_reverb_get_parameter(const RustShimmerReverb* ptr, uint32_t param_id);
void n8_rust_shimmer_reverb_process(
    RustShimmerReverb* ptr,
    const float* in_l,
    const float* in_r,
    float* out_l,
    float* out_r,
    uint32_t num_samples
);

// Tipo opaco para el motor de dispersión polifónica Refraction en Rust
struct RustRefraction;

RustRefraction* n8_rust_refraction_create(double sample_rate);
void n8_rust_refraction_destroy(RustRefraction* ptr);
void n8_rust_refraction_reset(RustRefraction* ptr);
void n8_rust_refraction_set_sample_rate(RustRefraction* ptr, double sample_rate);
void n8_rust_refraction_set_parameter(RustRefraction* ptr, uint32_t param_id, float value);
float n8_rust_refraction_get_parameter(const RustRefraction* ptr, uint32_t param_id);
void n8_rust_refraction_process(
    RustRefraction* ptr,
    const float* in_l,
    const float* in_r,
    float* out_l,
    float* out_r,
    uint32_t num_samples
);

// Tipo opaco para el motor de ecualización paramétrica dinámica en Rust
struct RustDynamicEq;

RustDynamicEq* n8_rust_dynamic_eq_create(double sample_rate);
void n8_rust_dynamic_eq_destroy(RustDynamicEq* ptr);
void n8_rust_dynamic_eq_reset(RustDynamicEq* ptr);
void n8_rust_dynamic_eq_set_sample_rate(RustDynamicEq* ptr, double sample_rate);
void n8_rust_dynamic_eq_set_parameter(RustDynamicEq* ptr, uint32_t param_id, float value);
float n8_rust_dynamic_eq_get_parameter(const RustDynamicEq* ptr, uint32_t param_id);
void n8_rust_dynamic_eq_process(
    RustDynamicEq* ptr,
    const float* in_l,
    const float* in_r,
    float* out_l,
    float* out_r,
    uint32_t num_samples
);

// Tipo opaco para el motor de conformado de transientes en Rust
struct RustTransientShaper;

RustTransientShaper* n8_rust_transient_shaper_create(double sample_rate);
void n8_rust_transient_shaper_destroy(RustTransientShaper* ptr);
void n8_rust_transient_shaper_reset(RustTransientShaper* ptr);
void n8_rust_transient_shaper_set_sample_rate(RustTransientShaper* ptr, double sample_rate);
void n8_rust_transient_shaper_set_parameter(RustTransientShaper* ptr, uint32_t param_id, float value);
float n8_rust_transient_shaper_get_parameter(const RustTransientShaper* ptr, uint32_t param_id);
void n8_rust_transient_shaper_process(
    RustTransientShaper* ptr,
    const float* in_l,
    const float* in_r,
    float* out_l,
    float* out_r,
    uint32_t num_samples
);

// Estructura de telemetría de punto de nube granular
struct RustGrainCloudPoint {
    float norm_position;
    float pitch_ratio;
    float pan;
    float envelope;
    bool active;
};

// Tipo opaco para el motor granular en Rust
struct RustGranular;

RustGranular* n8_rust_granular_create(double sample_rate);
void n8_rust_granular_destroy(RustGranular* ptr);
void n8_rust_granular_reset(RustGranular* ptr);
void n8_rust_granular_prepare(RustGranular* ptr, double sample_rate);
void n8_rust_granular_set_parameter(RustGranular* ptr, uint32_t param_id, float value);
float n8_rust_granular_get_parameter(const RustGranular* ptr, uint32_t param_id);
size_t n8_rust_granular_copy_waveform_overview(const RustGranular* ptr, float* out_min_max, size_t num_pairs);
size_t n8_rust_granular_copy_active_grains_snapshot(const RustGranular* ptr, RustGrainCloudPoint* out_points, size_t max_points);
float n8_rust_granular_get_write_position_normalized(const RustGranular* ptr);
void n8_rust_granular_process(
    RustGranular* ptr,
    const float* in_l,
    const float* in_r,
    float* out_l,
    float* out_r,
    uint32_t num_samples
);

// Tipo opaco para el motor Audio Slicer en Rust
struct RustAudioSlicer;

RustAudioSlicer* n8_rust_audio_slicer_create(double sample_rate);
void n8_rust_audio_slicer_destroy(RustAudioSlicer* ptr);
void n8_rust_audio_slicer_reset(RustAudioSlicer* ptr);
void n8_rust_audio_slicer_prepare(RustAudioSlicer* ptr, double sample_rate);
void n8_rust_audio_slicer_set_parameter(RustAudioSlicer* ptr, uint32_t param_id, float value);
float n8_rust_audio_slicer_get_parameter(const RustAudioSlicer* ptr, uint32_t param_id);
size_t n8_rust_audio_slicer_copy_waveform_thumbnail(const RustAudioSlicer* ptr, float* out_points, size_t num_points);
void n8_rust_audio_slicer_process(
    RustAudioSlicer* ptr,
    const float* in_l,
    const float* in_r,
    float* out_l,
    float* out_r,
    uint32_t num_samples
);

// Tipo opaco para el motor Channel Vocoder en Rust
struct RustVocoder;

RustVocoder* n8_rust_vocoder_create(double sample_rate);
void n8_rust_vocoder_destroy(RustVocoder* ptr);
void n8_rust_vocoder_reset(RustVocoder* ptr);
void n8_rust_vocoder_prepare(RustVocoder* ptr, double sample_rate);
void n8_rust_vocoder_set_parameter(RustVocoder* ptr, uint32_t param_id, float value);
float n8_rust_vocoder_get_parameter(const RustVocoder* ptr, uint32_t param_id);
void n8_rust_vocoder_process(
    RustVocoder* ptr,
    const float* in_l,
    const float* in_r,
    const float* sc_l,
    const float* sc_r,
    float* out_l,
    float* out_r,
    uint32_t num_samples
);

// Tipo opaco para el motor Rotary Speaker en Rust
struct RustRotarySpeaker;

RustRotarySpeaker* n8_rust_rotary_speaker_create(double sample_rate);
void n8_rust_rotary_speaker_destroy(RustRotarySpeaker* ptr);
void n8_rust_rotary_speaker_reset(RustRotarySpeaker* ptr);
void n8_rust_rotary_speaker_prepare(RustRotarySpeaker* ptr, double sample_rate);
void n8_rust_rotary_speaker_set_parameter(RustRotarySpeaker* ptr, uint32_t param_id, float value);
float n8_rust_rotary_speaker_get_parameter(const RustRotarySpeaker* ptr, uint32_t param_id);
void n8_rust_rotary_speaker_process(
    RustRotarySpeaker* ptr,
    const float* in_l,
    const float* in_r,
    float* out_l,
    float* out_r,
    uint32_t num_samples
);

// Tipo opaco para el motor Harmonic Exciter en Rust
struct RustHarmonicExciter;

RustHarmonicExciter* n8_rust_harmonic_exciter_create(double sample_rate);
void n8_rust_harmonic_exciter_destroy(RustHarmonicExciter* ptr);
void n8_rust_harmonic_exciter_reset(RustHarmonicExciter* ptr);
void n8_rust_harmonic_exciter_prepare(RustHarmonicExciter* ptr, double sample_rate);
void n8_rust_harmonic_exciter_set_parameter(RustHarmonicExciter* ptr, uint32_t param_id, float value);
float n8_rust_harmonic_exciter_get_parameter(const RustHarmonicExciter* ptr, uint32_t param_id);
void n8_rust_harmonic_exciter_process(
    RustHarmonicExciter* ptr,
    const float* in_l,
    const float* in_r,
    float* out_l,
    float* out_r,
    uint32_t num_samples
);

// Tipo opaco para el motor Brickwall Limiter en Rust
struct RustBrickwallLimiter;

RustBrickwallLimiter* n8_rust_brickwall_limiter_create(float sample_rate);
void n8_rust_brickwall_limiter_destroy(RustBrickwallLimiter* ptr);
void n8_rust_brickwall_limiter_reset(RustBrickwallLimiter* ptr);
void n8_rust_brickwall_limiter_prepare(RustBrickwallLimiter* ptr, float sample_rate);
void n8_rust_brickwall_limiter_set_parameter(RustBrickwallLimiter* ptr, uint32_t param_id, float value);
float n8_rust_brickwall_limiter_get_parameter(const RustBrickwallLimiter* ptr, uint32_t param_id);
void n8_rust_brickwall_limiter_process(
    RustBrickwallLimiter* ptr,
    const float* in_l,
    const float* in_r,
    float* out_l,
    float* out_r,
    uint32_t num_samples
);

// Tipo opaco para el motor Multiband Dynamics en Rust
struct RustMultibandDynamics;

RustMultibandDynamics* n8_rust_multiband_dynamics_create(float sample_rate);
void n8_rust_multiband_dynamics_destroy(RustMultibandDynamics* ptr);
void n8_rust_multiband_dynamics_reset(RustMultibandDynamics* ptr);
void n8_rust_multiband_dynamics_prepare(RustMultibandDynamics* ptr, float sample_rate);
void n8_rust_multiband_dynamics_set_parameter(RustMultibandDynamics* ptr, uint32_t param_id, float value);
float n8_rust_multiband_dynamics_get_parameter(const RustMultibandDynamics* ptr, uint32_t param_id);
void n8_rust_multiband_dynamics_process(
    RustMultibandDynamics* ptr,
    const float* in_l,
    const float* in_r,
    float* out_l,
    float* out_r,
    uint32_t num_samples
);

// Tipo opaco para el motor Resonator Bank en Rust
struct RustResonatorBank;

RustResonatorBank* n8_rust_resonator_bank_create(float sample_rate);
void n8_rust_resonator_bank_destroy(RustResonatorBank* ptr);
void n8_rust_resonator_bank_reset(RustResonatorBank* ptr);
void n8_rust_resonator_bank_prepare(RustResonatorBank* ptr, float sample_rate);
void n8_rust_resonator_bank_set_parameter(RustResonatorBank* ptr, uint32_t param_id, float value);
float n8_rust_resonator_bank_get_parameter(const RustResonatorBank* ptr, uint32_t param_id);
uint32_t n8_rust_resonator_bank_get_tail_samples(const RustResonatorBank* ptr);
void n8_rust_resonator_bank_process(
    RustResonatorBank* ptr,
    const float* in_l,
    const float* in_r,
    float* out_l,
    float* out_r,
    uint32_t num_samples
);

// Tipo opaco para el motor Bitcrusher en Rust
struct RustBitcrusher;

RustBitcrusher* n8_rust_bitcrusher_create(float sample_rate);
void n8_rust_bitcrusher_destroy(RustBitcrusher* ptr);
void n8_rust_bitcrusher_reset(RustBitcrusher* ptr);
void n8_rust_bitcrusher_prepare(RustBitcrusher* ptr, float sample_rate);
void n8_rust_bitcrusher_set_parameter(RustBitcrusher* ptr, uint32_t param_id, float value);
float n8_rust_bitcrusher_get_parameter(const RustBitcrusher* ptr, uint32_t param_id);
void n8_rust_bitcrusher_process(
    RustBitcrusher* ptr,
    const float* in_l,
    const float* in_r,
    float* out_l,
    float* out_r,
    uint32_t num_samples
);

// Tipo opaco para el motor Noise Gate en Rust
struct RustNoiseGate;

RustNoiseGate* n8_rust_noise_gate_create(float sample_rate);
void n8_rust_noise_gate_destroy(RustNoiseGate* ptr);
void n8_rust_noise_gate_reset(RustNoiseGate* ptr);
void n8_rust_noise_gate_prepare(RustNoiseGate* ptr, float sample_rate);
void n8_rust_noise_gate_set_parameter(RustNoiseGate* ptr, uint32_t param_id, float value);
float n8_rust_noise_gate_get_parameter(const RustNoiseGate* ptr, uint32_t param_id);
void n8_rust_noise_gate_process(
    RustNoiseGate* ptr,
    const float* in_l,
    const float* in_r,
    const float* sc_l,
    const float* sc_r,
    float* out_l,
    float* out_r,
    uint32_t num_samples
);

// Tipo opaco para el motor De-Esser en Rust
struct RustDeEsser;

RustDeEsser* n8_rust_de_esser_create(float sample_rate);
void n8_rust_de_esser_destroy(RustDeEsser* ptr);
void n8_rust_de_esser_reset(RustDeEsser* ptr);
void n8_rust_de_esser_prepare(RustDeEsser* ptr, float sample_rate);
void n8_rust_de_esser_set_parameter(RustDeEsser* ptr, uint32_t param_id, float value);
float n8_rust_de_esser_get_parameter(const RustDeEsser* ptr, uint32_t param_id);
void n8_rust_de_esser_process(
    RustDeEsser* ptr,
    const float* in_l,
    const float* in_r,
    float* out_l,
    float* out_r,
    uint32_t num_samples
);

// Tipo opaco para el motor Auto-Wah en Rust
struct RustAutoWah;

RustAutoWah* n8_rust_auto_wah_create(float sample_rate);
void n8_rust_auto_wah_destroy(RustAutoWah* ptr);
void n8_rust_auto_wah_reset(RustAutoWah* ptr);
void n8_rust_auto_wah_prepare(RustAutoWah* ptr, float sample_rate);
void n8_rust_auto_wah_set_parameter(RustAutoWah* ptr, uint32_t param_id, float value);
float n8_rust_auto_wah_get_parameter(const RustAutoWah* ptr, uint32_t param_id);
void n8_rust_auto_wah_process(
    RustAutoWah* ptr,
    const float* in_l,
    const float* in_r,
    const float* sc_l,
    const float* sc_r,
    float* out_l,
    float* out_r,
    uint32_t num_samples
);

// Tipo opaco para el motor VCA Compressor en Rust
struct RustCompressor;

RustCompressor* n8_rust_compressor_create(float sample_rate);
void n8_rust_compressor_destroy(RustCompressor* ptr);
void n8_rust_compressor_reset(RustCompressor* ptr);
void n8_rust_compressor_prepare(RustCompressor* ptr, float sample_rate);
void n8_rust_compressor_set_parameter(RustCompressor* ptr, uint32_t param_id, float value);
float n8_rust_compressor_get_parameter(const RustCompressor* ptr, uint32_t param_id);
void n8_rust_compressor_process(
    RustCompressor* ptr,
    const float* in_l,
    const float* in_r,
    const float* sc_l,
    const float* sc_r,
    float* out_l,
    float* out_r,
    uint32_t num_samples
);

// Tipo opaco para el motor Parametric EQ en Rust
struct RustParametricEq;

RustParametricEq* n8_rust_parametric_eq_create(float sample_rate);
void n8_rust_parametric_eq_destroy(RustParametricEq* ptr);
void n8_rust_parametric_eq_reset(RustParametricEq* ptr);
void n8_rust_parametric_eq_prepare(RustParametricEq* ptr, float sample_rate);
void n8_rust_parametric_eq_set_parameter(RustParametricEq* ptr, uint32_t param_id, float value);
float n8_rust_parametric_eq_get_parameter(const RustParametricEq* ptr, uint32_t param_id);
void n8_rust_parametric_eq_process(
    RustParametricEq* ptr,
    const float* in_l,
    const float* in_r,
    float* out_l,
    float* out_r,
    uint32_t num_samples
);

// Tipo opaco para el motor Stereo Imager en Rust
struct RustStereoImager;

RustStereoImager* n8_rust_stereo_imager_create(float sample_rate);
void n8_rust_stereo_imager_destroy(RustStereoImager* ptr);
void n8_rust_stereo_imager_reset(RustStereoImager* ptr);
void n8_rust_stereo_imager_prepare(RustStereoImager* ptr, float sample_rate);
void n8_rust_stereo_imager_set_parameter(RustStereoImager* ptr, uint32_t param_id, float value);
float n8_rust_stereo_imager_get_parameter(const RustStereoImager* ptr, uint32_t param_id);
float n8_rust_stereo_imager_get_phase_correlation(const RustStereoImager* ptr);
void n8_rust_stereo_imager_process(
    RustStereoImager* ptr,
    const float* in_l,
    const float* in_r,
    float* out_l,
    float* out_r,
    uint32_t num_samples
);

// Tipo opaco para el motor Phaser en Rust
struct RustPhaser;

RustPhaser* n8_rust_phaser_create(float sample_rate);
void n8_rust_phaser_destroy(RustPhaser* ptr);
void n8_rust_phaser_reset(RustPhaser* ptr);
void n8_rust_phaser_prepare(RustPhaser* ptr, float sample_rate);
void n8_rust_phaser_set_parameter(RustPhaser* ptr, uint32_t param_id, float value);
float n8_rust_phaser_get_parameter(const RustPhaser* ptr, uint32_t param_id);
void n8_rust_phaser_process(
    RustPhaser* ptr,
    const float* in_l,
    const float* in_r,
    float* out_l,
    float* out_r,
    uint32_t num_samples
);

// Tipo opaco para el motor Chorus en Rust
struct RustChorus;

RustChorus* n8_rust_chorus_create(float sample_rate);
void n8_rust_chorus_destroy(RustChorus* ptr);
void n8_rust_chorus_reset(RustChorus* ptr);
void n8_rust_chorus_prepare(RustChorus* ptr, float sample_rate);
void n8_rust_chorus_set_parameter(RustChorus* ptr, uint32_t param_id, float value);
float n8_rust_chorus_get_parameter(const RustChorus* ptr, uint32_t param_id);
void n8_rust_chorus_process(
    RustChorus* ptr,
    const float* in_l,
    const float* in_r,
    float* out_l,
    float* out_r,
    uint32_t num_samples
);

// Tipo opaco para el motor Flanger en Rust
struct RustFlanger;

RustFlanger* n8_rust_flanger_create(float sample_rate);
void n8_rust_flanger_destroy(RustFlanger* ptr);
void n8_rust_flanger_reset(RustFlanger* ptr);
void n8_rust_flanger_prepare(RustFlanger* ptr, float sample_rate);
void n8_rust_flanger_set_parameter(RustFlanger* ptr, uint32_t param_id, float value);
float n8_rust_flanger_get_parameter(const RustFlanger* ptr, uint32_t param_id);
void n8_rust_flanger_process(
    RustFlanger* ptr,
    const float* in_l,
    const float* in_r,
    float* out_l,
    float* out_r,
    uint32_t num_samples
);

// Tipo opaco para el motor Ring Modulator en Rust
struct RustRingModulator;

RustRingModulator* n8_rust_ring_modulator_create(float sample_rate);
void n8_rust_ring_modulator_destroy(RustRingModulator* ptr);
void n8_rust_ring_modulator_reset(RustRingModulator* ptr);
void n8_rust_ring_modulator_prepare(RustRingModulator* ptr, float sample_rate);
void n8_rust_ring_modulator_set_parameter(RustRingModulator* ptr, uint32_t param_id, float value);
float n8_rust_ring_modulator_get_parameter(const RustRingModulator* ptr, uint32_t param_id);
void n8_rust_ring_modulator_process(
    RustRingModulator* ptr,
    const float* in_l,
    const float* in_r,
    float* out_l,
    float* out_r,
    uint32_t num_samples
);

// Tipo opaco para el motor Frequency Shifter en Rust
struct RustFrequencyShifter;

RustFrequencyShifter* n8_rust_frequency_shifter_create(float sample_rate);
void n8_rust_frequency_shifter_destroy(RustFrequencyShifter* ptr);
void n8_rust_frequency_shifter_reset(RustFrequencyShifter* ptr);
void n8_rust_frequency_shifter_prepare(RustFrequencyShifter* ptr, float sample_rate);
void n8_rust_frequency_shifter_set_parameter(RustFrequencyShifter* ptr, uint32_t param_id, float value);
float n8_rust_frequency_shifter_get_parameter(const RustFrequencyShifter* ptr, uint32_t param_id);
void n8_rust_frequency_shifter_process(
    RustFrequencyShifter* ptr,
    const float* in_l,
    const float* in_r,
    float* out_l,
    float* out_r,
    uint32_t num_samples
);

// Tipo opaco para el motor Distortion en Rust
struct RustDistortion;

RustDistortion* n8_rust_distortion_create(float sample_rate);
void n8_rust_distortion_destroy(RustDistortion* ptr);
void n8_rust_distortion_reset(RustDistortion* ptr);
void n8_rust_distortion_prepare(RustDistortion* ptr, float sample_rate);
void n8_rust_distortion_set_parameter(RustDistortion* ptr, uint32_t param_id, float value);
float n8_rust_distortion_get_parameter(const RustDistortion* ptr, uint32_t param_id);
void n8_rust_distortion_process(
    RustDistortion* ptr,
    const float* in_l,
    const float* in_r,
    float* out_l,
    float* out_r,
    uint32_t num_samples
);

// Tipo opaco para el motor Formant Filter en Rust
struct RustFormantFilter;

RustFormantFilter* n8_rust_formant_filter_create(float sample_rate);
void n8_rust_formant_filter_destroy(RustFormantFilter* ptr);
void n8_rust_formant_filter_reset(RustFormantFilter* ptr);
void n8_rust_formant_filter_prepare(RustFormantFilter* ptr, float sample_rate);
void n8_rust_formant_filter_set_parameter(RustFormantFilter* ptr, uint32_t param_id, float value);
float n8_rust_formant_filter_get_parameter(const RustFormantFilter* ptr, uint32_t param_id);
void n8_rust_formant_filter_process(
    RustFormantFilter* ptr,
    const float* in_l,
    const float* in_r,
    float* out_l,
    float* out_r,
    uint32_t num_samples
);

// Tipo opaco para el motor Noise & Texture en Rust
struct RustNoiseTexture;

RustNoiseTexture* n8_rust_noise_texture_create(float sample_rate);
void n8_rust_noise_texture_destroy(RustNoiseTexture* ptr);
void n8_rust_noise_texture_reset(RustNoiseTexture* ptr);
void n8_rust_noise_texture_prepare(RustNoiseTexture* ptr, float sample_rate);
void n8_rust_noise_texture_set_parameter(RustNoiseTexture* ptr, uint32_t param_id, float value);
float n8_rust_noise_texture_get_parameter(const RustNoiseTexture* ptr, uint32_t param_id);
void n8_rust_noise_texture_process(
    RustNoiseTexture* ptr,
    const float* in_l,
    const float* in_r,
    float* out_l,
    float* out_r,
    uint32_t num_samples
);

// Tipo opaco para el motor Spatial Panner en Rust
struct RustSpatialPanner;

RustSpatialPanner* n8_rust_spatial_panner_create(float sample_rate);
void n8_rust_spatial_panner_destroy(RustSpatialPanner* ptr);
void n8_rust_spatial_panner_reset(RustSpatialPanner* ptr);
void n8_rust_spatial_panner_prepare(RustSpatialPanner* ptr, float sample_rate);
void n8_rust_spatial_panner_set_parameter(RustSpatialPanner* ptr, uint32_t param_id, float value);
float n8_rust_spatial_panner_get_parameter(const RustSpatialPanner* ptr, uint32_t param_id);
void n8_rust_spatial_panner_process(
    RustSpatialPanner* ptr,
    const float* in_l,
    const float* in_r,
    float* out_l,
    float* out_r,
    uint32_t num_samples
);

// Tipo opaco para el motor Tape Stop en Rust
struct RustTapeStop;

RustTapeStop* n8_rust_tape_stop_create(float sample_rate);
void n8_rust_tape_stop_destroy(RustTapeStop* ptr);
void n8_rust_tape_stop_reset(RustTapeStop* ptr);
void n8_rust_tape_stop_prepare(RustTapeStop* ptr, float sample_rate);
void n8_rust_tape_stop_set_parameter(RustTapeStop* ptr, uint32_t param_id, float value);
float n8_rust_tape_stop_get_parameter(const RustTapeStop* ptr, uint32_t param_id);
void n8_rust_tape_stop_process(
    RustTapeStop* ptr,
    const float* in_l,
    const float* in_r,
    float* out_l,
    float* out_r,
    uint32_t num_samples
);

// Tipo opaco para el motor Lo-Fi Cassette en Rust
struct RustLoFiCassette;

RustLoFiCassette* n8_rust_lofi_cassette_create(float sample_rate);
void n8_rust_lofi_cassette_destroy(RustLoFiCassette* ptr);
void n8_rust_lofi_cassette_reset(RustLoFiCassette* ptr);
void n8_rust_lofi_cassette_prepare(RustLoFiCassette* ptr, float sample_rate);
void n8_rust_lofi_cassette_set_parameter(RustLoFiCassette* ptr, uint32_t param_id, float value);
float n8_rust_lofi_cassette_get_parameter(const RustLoFiCassette* ptr, uint32_t param_id);
void n8_rust_lofi_cassette_process(
    RustLoFiCassette* ptr,
    const float* in_l,
    const float* in_r,
    float* out_l,
    float* out_r,
    uint32_t num_samples
);

// Tipo opaco para el motor Advanced Delay en Rust
struct RustAdvancedDelay;

RustAdvancedDelay* n8_rust_advanced_delay_create(float sample_rate);
void n8_rust_advanced_delay_destroy(RustAdvancedDelay* ptr);
void n8_rust_advanced_delay_reset(RustAdvancedDelay* ptr);
void n8_rust_advanced_delay_prepare(RustAdvancedDelay* ptr, float sample_rate);
void n8_rust_advanced_delay_set_parameter(RustAdvancedDelay* ptr, uint32_t param_id, float value);
float n8_rust_advanced_delay_get_parameter(const RustAdvancedDelay* ptr, uint32_t param_id);
void n8_rust_advanced_delay_process(
    RustAdvancedDelay* ptr,
    const float* in_l,
    const float* in_r,
    float* out_l,
    float* out_r,
    uint32_t num_samples
);

// Tipo opaco para el motor Reverb Algorítmica FDN en Rust
struct RustReverb;

RustReverb* n8_rust_reverb_create(float sample_rate);
void n8_rust_reverb_destroy(RustReverb* ptr);
void n8_rust_reverb_reset(RustReverb* ptr);
void n8_rust_reverb_prepare(RustReverb* ptr, float sample_rate);
void n8_rust_reverb_set_parameter(RustReverb* ptr, uint32_t param_id, float value);
float n8_rust_reverb_get_parameter(const RustReverb* ptr, uint32_t param_id);
void n8_rust_reverb_process(
    RustReverb* ptr,
    const float* in_l,
    const float* in_r,
    float* out_l,
    float* out_r,
    uint32_t num_samples
);

// Tipo opaco para el motor Reverse Reverb / Swell Bloom en Rust
struct RustReverseReverb;

RustReverseReverb* n8_rust_reverse_reverb_create(float sample_rate);
void n8_rust_reverse_reverb_destroy(RustReverseReverb* ptr);
void n8_rust_reverse_reverb_reset(RustReverseReverb* ptr);
void n8_rust_reverse_reverb_prepare(RustReverseReverb* ptr, float sample_rate);
void n8_rust_reverse_reverb_set_parameter(RustReverseReverb* ptr, uint32_t param_id, float value);
float n8_rust_reverse_reverb_get_parameter(const RustReverseReverb* ptr, uint32_t param_id);
void n8_rust_reverse_reverb_process(
    RustReverseReverb* ptr,
    const float* in_l,
    const float* in_r,
    float* out_l,
    float* out_r,
    uint32_t num_samples
);

// Tipo opaco para el motor Glitch, Stutter & Slicing en Rust
struct RustGlitch;

RustGlitch* n8_rust_glitch_create(float sample_rate);
void n8_rust_glitch_destroy(RustGlitch* ptr);
void n8_rust_glitch_reset(RustGlitch* ptr);
void n8_rust_glitch_prepare(RustGlitch* ptr, float sample_rate);
void n8_rust_glitch_set_parameter(RustGlitch* ptr, uint32_t param_id, float value);
float n8_rust_glitch_get_parameter(const RustGlitch* ptr, uint32_t param_id);
void n8_rust_glitch_process(
    RustGlitch* ptr,
    const float* in_l,
    const float* in_r,
    float* out_l,
    float* out_r,
    uint32_t num_samples
);

// Tipo opaco para el motor Mid/Side Encoder en Rust
struct RustMidSideEncoder;

RustMidSideEncoder* n8_rust_mid_side_encoder_create();
void n8_rust_mid_side_encoder_destroy(RustMidSideEncoder* ptr);
void n8_rust_mid_side_encoder_reset(RustMidSideEncoder* ptr);
void n8_rust_mid_side_encoder_prepare(RustMidSideEncoder* ptr, float sample_rate);
void n8_rust_mid_side_encoder_set_parameter(RustMidSideEncoder* ptr, uint32_t param_id, float value);
float n8_rust_mid_side_encoder_get_parameter(const RustMidSideEncoder* ptr, uint32_t param_id);
void n8_rust_mid_side_encoder_process(
    RustMidSideEncoder* ptr,
    const float* in_l,
    const float* in_r,
    float* out_mid,
    float* out_side,
    uint32_t num_samples
);

// Tipo opaco para el motor Mid/Side Decoder en Rust
struct RustMidSideDecoder;

RustMidSideDecoder* n8_rust_mid_side_decoder_create(float sample_rate);
void n8_rust_mid_side_decoder_destroy(RustMidSideDecoder* ptr);
void n8_rust_mid_side_decoder_reset(RustMidSideDecoder* ptr);
void n8_rust_mid_side_decoder_prepare(RustMidSideDecoder* ptr, float sample_rate);
void n8_rust_mid_side_decoder_set_parameter(RustMidSideDecoder* ptr, uint32_t param_id, float value);
float n8_rust_mid_side_decoder_get_parameter(const RustMidSideDecoder* ptr, uint32_t param_id);
void n8_rust_mid_side_decoder_process(
    RustMidSideDecoder* ptr,
    const float* in_mid,
    const float* in_side,
    float* out_l,
    float* out_r,
    uint32_t num_samples
);

// Tipo opaco para el motor Spectral Processor en Rust
struct RustSpectralProcessor;

RustSpectralProcessor* n8_rust_spectral_processor_create();
void n8_rust_spectral_processor_destroy(RustSpectralProcessor* ptr);
void n8_rust_spectral_processor_reset(RustSpectralProcessor* ptr);
void n8_rust_spectral_processor_prepare(RustSpectralProcessor* ptr, float sample_rate);
void n8_rust_spectral_processor_set_parameter(RustSpectralProcessor* ptr, uint32_t param_id, float value);
float n8_rust_spectral_processor_get_parameter(const RustSpectralProcessor* ptr, uint32_t param_id);
void n8_rust_spectral_processor_process(
    RustSpectralProcessor* ptr,
    const float* in_l,
    const float* in_r,
    float* out_l,
    float* out_r,
    uint32_t num_samples
);

// Tipo opaco para el motor Spectral Freeze en Rust
struct RustSpectralFreeze;

RustSpectralFreeze* n8_rust_spectral_freeze_create();
void n8_rust_spectral_freeze_destroy(RustSpectralFreeze* ptr);
void n8_rust_spectral_freeze_reset(RustSpectralFreeze* ptr);
void n8_rust_spectral_freeze_prepare(RustSpectralFreeze* ptr, float sample_rate);
void n8_rust_spectral_freeze_set_parameter(RustSpectralFreeze* ptr, uint32_t param_id, float value);
float n8_rust_spectral_freeze_get_parameter(const RustSpectralFreeze* ptr, uint32_t param_id);
void n8_rust_spectral_freeze_process(
    RustSpectralFreeze* ptr,
    const float* in_l,
    const float* in_r,
    float* out_l,
    float* out_r,
    uint32_t num_samples
);

// Tipo opaco para el motor Spectral Smear en Rust
struct RustSpectralSmear;

RustSpectralSmear* n8_rust_spectral_smear_create();
void n8_rust_spectral_smear_destroy(RustSpectralSmear* ptr);
void n8_rust_spectral_smear_reset(RustSpectralSmear* ptr);
void n8_rust_spectral_smear_prepare(RustSpectralSmear* ptr, float sample_rate);
void n8_rust_spectral_smear_set_parameter(RustSpectralSmear* ptr, uint32_t param_id, float value);
float n8_rust_spectral_smear_get_parameter(const RustSpectralSmear* ptr, uint32_t param_id);
void n8_rust_spectral_smear_process(
    RustSpectralSmear* ptr,
    const float* in_l,
    const float* in_r,
    float* out_l,
    float* out_r,
    uint32_t num_samples
);

// Tipo opaco para el motor Pitch Shifter en Rust
struct RustPitchShifter;

RustPitchShifter* n8_rust_pitch_shifter_create();
void n8_rust_pitch_shifter_destroy(RustPitchShifter* ptr);
void n8_rust_pitch_shifter_reset(RustPitchShifter* ptr);
void n8_rust_pitch_shifter_prepare(RustPitchShifter* ptr, float sample_rate);
void n8_rust_pitch_shifter_set_parameter(RustPitchShifter* ptr, uint32_t param_id, float value);
float n8_rust_pitch_shifter_get_parameter(const RustPitchShifter* ptr, uint32_t param_id);
void n8_rust_pitch_shifter_process(
    RustPitchShifter* ptr,
    const float* in_l,
    const float* in_r,
    float* out_l,
    float* out_r,
    uint32_t num_samples
);

// Tipo opaco para el motor Sample Player en Rust (DSP #53)
struct RustSamplePlayer;

RustSamplePlayer* n8_rust_sample_player_create();
void n8_rust_sample_player_destroy(RustSamplePlayer* ptr);
void n8_rust_sample_player_reset(RustSamplePlayer* ptr);
void n8_rust_sample_player_prepare(RustSamplePlayer* ptr, double sample_rate);
void n8_rust_sample_player_set_parameter(RustSamplePlayer* ptr, uint32_t param_id, float value);
float n8_rust_sample_player_get_parameter(const RustSamplePlayer* ptr, uint32_t param_id);
void n8_rust_sample_player_trigger(RustSamplePlayer* ptr);
void n8_rust_sample_player_get_thumbnail(const RustSamplePlayer* ptr, float* out_64);
void n8_rust_sample_player_load_sample(
    RustSamplePlayer* ptr,
    const float* l_data,
    const float* r_data,
    size_t num_samples,
    double sample_rate
);
void n8_rust_sample_player_process(
    RustSamplePlayer* ptr,
    const float* in_l,
    const float* in_r,
    const float* trig,
    float* out_l,
    float* out_r,
    uint32_t num_samples
);

// Tipo opaco para el motor Convolution Particionada en Rust (DSP #48)
struct RustConvolution;

RustConvolution* n8_rust_convolution_create();
void n8_rust_convolution_destroy(RustConvolution* ptr);
void n8_rust_convolution_reset(RustConvolution* ptr);
void n8_rust_convolution_prepare(RustConvolution* ptr, double sample_rate);
void n8_rust_convolution_set_parameter(RustConvolution* ptr, uint32_t param_id, float value);
float n8_rust_convolution_get_parameter(const RustConvolution* ptr, uint32_t param_id);
void n8_rust_convolution_get_thumbnail(const RustConvolution* ptr, float* out_64);
void n8_rust_convolution_set_custom_ir(
    RustConvolution* ptr,
    const float* l_data,
    const float* r_data,
    size_t num_samples
);
void n8_rust_convolution_process(
    RustConvolution* ptr,
    const float* in_l,
    const float* in_r,
    float* out_l,
    float* out_r,
    uint32_t num_samples
);

// Tipo opaco para el motor Polyphase Oversampling & Saturation en Rust (DSP #48 / NodeType::Oversampler)
struct RustOversampler;

RustOversampler* n8_rust_oversampler_create();
void n8_rust_oversampler_destroy(RustOversampler* ptr);
void n8_rust_oversampler_reset(RustOversampler* ptr);
void n8_rust_oversampler_prepare(RustOversampler* ptr, double sample_rate, size_t max_block_size);
void n8_rust_oversampler_set_parameter(RustOversampler* ptr, uint32_t param_id, float value);
float n8_rust_oversampler_get_parameter(const RustOversampler* ptr, uint32_t param_id);
size_t n8_rust_oversampler_get_latency(const RustOversampler* ptr);
void n8_rust_oversampler_process(
    RustOversampler* ptr,
    const float* in_l,
    const float* in_r,
    float* out_l,
    float* out_r,
    uint32_t num_samples
);

// Tipo opaco para el motor TPT State Variable Filter en Rust (DSP #4 / NodeType::Filter)
struct RustSimpleFilter;

RustSimpleFilter* n8_rust_simple_filter_create();
void n8_rust_simple_filter_destroy(RustSimpleFilter* ptr);
void n8_rust_simple_filter_reset(RustSimpleFilter* ptr);
void n8_rust_simple_filter_prepare(RustSimpleFilter* ptr, double sample_rate);
void n8_rust_simple_filter_set_parameter(RustSimpleFilter* ptr, uint32_t param_id, float value);
float n8_rust_simple_filter_get_parameter(const RustSimpleFilter* ptr, uint32_t param_id);
void n8_rust_simple_filter_process(
    RustSimpleFilter* ptr,
    const float* in_l,
    const float* in_r,
    const float* fm_mod,
    float* out_l,
    float* out_r,
    uint32_t num_samples
);

// Tipo opaco para el motor Stereo Delay en Rust (DSP #5 / NodeType::Delay)
struct RustSimpleDelay;

RustSimpleDelay* n8_rust_simple_delay_create();
void n8_rust_simple_delay_destroy(RustSimpleDelay* ptr);
void n8_rust_simple_delay_reset(RustSimpleDelay* ptr);
void n8_rust_simple_delay_prepare(RustSimpleDelay* ptr, double sample_rate);
void n8_rust_simple_delay_set_parameter(RustSimpleDelay* ptr, uint32_t param_id, float value);
float n8_rust_simple_delay_get_parameter(const RustSimpleDelay* ptr, uint32_t param_id);
void n8_rust_simple_delay_process(
    RustSimpleDelay* ptr,
    const float* in_l,
    const float* in_r,
    float* out_l,
    float* out_r,
    uint32_t num_samples
);

// Tipo opaco para el motor Passthrough & Gain Utility en Rust (DSP #3 / NodeType::Passthrough)
struct RustPassthrough;

RustPassthrough* n8_rust_passthrough_create();
void n8_rust_passthrough_destroy(RustPassthrough* ptr);
void n8_rust_passthrough_reset(RustPassthrough* ptr);
void n8_rust_passthrough_prepare(RustPassthrough* ptr);
void n8_rust_passthrough_set_parameter(RustPassthrough* ptr, uint32_t param_id, float value);
float n8_rust_passthrough_get_parameter(const RustPassthrough* ptr, uint32_t param_id);
void n8_rust_passthrough_process(
    RustPassthrough* ptr,
    const float* in_l,
    const float* in_r,
    float* out_l,
    float* out_r,
    uint32_t num_samples
);

// Tipo opaco para ExternalSidechainCore en Rust (DSP #49 / NodeType::ExternalSidechain)
struct RustExternalSidechain;

RustExternalSidechain* n8_rust_external_sidechain_create();
void n8_rust_external_sidechain_free(RustExternalSidechain* ptr);
void n8_rust_external_sidechain_prepare(RustExternalSidechain* ptr, float sample_rate);
void n8_rust_external_sidechain_reset(RustExternalSidechain* ptr);
void n8_rust_external_sidechain_set_parameters(RustExternalSidechain* ptr, float gain_db, float mute);
void n8_rust_external_sidechain_process(
    RustExternalSidechain* ptr,
    const float* sc_l,
    const float* sc_r,
    float* out_l,
    float* out_r,
    uint32_t num_samples
);

// Tipo opaco para MidiScaleQuantizerCore en Rust (DSP #50 / NodeType::MidiScaleQuantizer)
struct RustMidiScaleQuantizer;

RustMidiScaleQuantizer* n8_rust_midi_scale_quantizer_create();
void n8_rust_midi_scale_quantizer_free(RustMidiScaleQuantizer* ptr);
void n8_rust_midi_scale_quantizer_prepare(RustMidiScaleQuantizer* ptr, float sample_rate);
void n8_rust_midi_scale_quantizer_reset(RustMidiScaleQuantizer* ptr);
void n8_rust_midi_scale_quantizer_set_parameters(
    RustMidiScaleQuantizer* ptr,
    float root_key,
    float scale,
    float transpose,
    float snap_speed,
    float mix
);
int32_t n8_rust_midi_scale_quantizer_quantize_note(const RustMidiScaleQuantizer* ptr, int32_t midi_note);
void n8_rust_midi_scale_quantizer_process(
    RustMidiScaleQuantizer* ptr,
    const float* in_l,
    const float* in_r,
    float* out_l,
    float* out_r,
    uint32_t num_samples
);

// Tipo opaco para MidiArpeggiatorCore en Rust (DSP #51 / NodeType::MidiArpeggiator)
struct RustMidiArpeggiator;

RustMidiArpeggiator* n8_rust_midi_arpeggiator_create();
void n8_rust_midi_arpeggiator_free(RustMidiArpeggiator* ptr);
void n8_rust_midi_arpeggiator_prepare(RustMidiArpeggiator* ptr, float sample_rate);
void n8_rust_midi_arpeggiator_reset(RustMidiArpeggiator* ptr);
void n8_rust_midi_arpeggiator_set_notes(RustMidiArpeggiator* ptr, const int32_t* notes, size_t count);
void n8_rust_midi_arpeggiator_set_parameters(
    RustMidiArpeggiator* ptr,
    float rate,
    float pattern,
    float octaves,
    float gate,
    float swing,
    float synth_mix
);
void n8_rust_midi_arpeggiator_process(
    RustMidiArpeggiator* ptr,
    const float* in_l,
    const float* in_r,
    float* out_l,
    float* out_r,
    uint32_t num_samples,
    double ppq_position,
    double bpm
);

// Tipo opaco para MidiChordEngineCore en Rust (DSP #52 / NodeType::MidiChordEngine)
struct RustMidiChordEngine;

RustMidiChordEngine* n8_rust_midi_chord_engine_create();
void n8_rust_midi_chord_engine_free(RustMidiChordEngine* ptr);
void n8_rust_midi_chord_engine_prepare(RustMidiChordEngine* ptr, float sample_rate);
void n8_rust_midi_chord_engine_reset(RustMidiChordEngine* ptr);
void n8_rust_midi_chord_engine_trigger_chord(RustMidiChordEngine* ptr);
void n8_rust_midi_chord_engine_set_parameters(
    RustMidiChordEngine* ptr,
    float chord_type,
    float inversion,
    float strum_delay,
    float spread,
    float root_note,
    float synth_mix
);
void n8_rust_midi_chord_engine_process(
    RustMidiChordEngine* ptr,
    const float* in_l,
    const float* in_r,
    float* out_l,
    float* out_r,
    uint32_t num_samples
);

// Tipo opaco para FeedbackContainerCore en Rust (DSP #53 / NodeType::Feedback)
struct RustFeedbackContainer;

RustFeedbackContainer* n8_rust_feedback_container_create(float sample_rate);
void n8_rust_feedback_container_free(RustFeedbackContainer* ptr);
void n8_rust_feedback_container_prepare(RustFeedbackContainer* ptr, float sample_rate);
void n8_rust_feedback_container_reset(RustFeedbackContainer* ptr);
void n8_rust_feedback_container_set_parameter(RustFeedbackContainer* ptr, uint32_t param_id, float value);
float n8_rust_feedback_container_get_parameter(const RustFeedbackContainer* ptr, uint32_t param_id);
void n8_rust_feedback_container_inject_forward(
    RustFeedbackContainer* ptr,
    const float* in_l,
    const float* in_r,
    float* forward_l,
    float* forward_r,
    uint32_t num_samples
);
void n8_rust_feedback_container_process_return(
    RustFeedbackContainer* ptr,
    const float* in_l,
    const float* in_r,
    const float* wet_l,
    const float* wet_r,
    float* out_l,
    float* out_r,
    uint32_t num_samples
);
void n8_rust_feedback_container_process(
    RustFeedbackContainer* ptr,
    const float* in_l,
    const float* in_r,
    float* out_l,
    float* out_r,
    uint32_t num_samples
);

// Tipo opaco para EventContainerCore en Rust (DSP #47 / NodeType::EventContainer)
struct RustEventManager;
struct RustEventContainer;

RustEventContainer* n8_rust_event_container_create(float sample_rate);
void n8_rust_event_container_free(RustEventContainer* ptr);
void n8_rust_event_container_prepare(RustEventContainer* ptr, float sample_rate, uint32_t max_block_size, uint32_t pool_capacity);
void n8_rust_event_container_reset(RustEventContainer* ptr);
void n8_rust_event_container_set_parameter(RustEventContainer* ptr, uint32_t param_id, float value);
float n8_rust_event_container_get_parameter(const RustEventContainer* ptr, uint32_t param_id);
void n8_rust_event_container_inject_forward(
    RustEventContainer* ptr,
    const float* in_l,
    const float* in_r,
    float* forward_l,
    float* forward_r,
    uint32_t num_samples
);
void n8_rust_event_container_process_return(
    RustEventContainer* ptr,
    const float* in_l,
    const float* in_r,
    const float* wet_l,
    const float* wet_r,
    float* out_l,
    float* out_r,
    uint32_t num_samples
);
void n8_rust_event_container_process(
    RustEventContainer* ptr,
    const float* in_l,
    const float* in_r,
    float* out_l,
    float* out_r,
    uint32_t num_samples
);
RustEventManager* n8_rust_event_container_get_event_manager(RustEventContainer* ptr);
uint32_t n8_rust_event_container_get_active_count(const RustEventContainer* ptr);

// Tipo opaco para TransientDetectorCore en Rust (Analysis Core)
struct RustTransientDetector;

RustTransientDetector* n8_rust_transient_detector_create(float sample_rate);
void n8_rust_transient_detector_free(RustTransientDetector* ptr);
void n8_rust_transient_detector_prepare(RustTransientDetector* ptr, float sample_rate);
void n8_rust_transient_detector_reset(RustTransientDetector* ptr);
void n8_rust_transient_detector_set_sensitivity(RustTransientDetector* ptr, float sensitivity);
float n8_rust_transient_detector_get_sensitivity(const RustTransientDetector* ptr);
bool n8_rust_transient_detector_is_transient_detected(const RustTransientDetector* ptr);
float n8_rust_transient_detector_get_onset_strength(const RustTransientDetector* ptr);
void n8_rust_transient_detector_process(
    RustTransientDetector* ptr,
    const float* input,
    uint32_t num_samples
);

// Tipo opaco para PitchTrackerCore en Rust (Analysis Core)
struct RustPitchTracker;

RustPitchTracker* n8_rust_pitch_tracker_create(float sample_rate);
void n8_rust_pitch_tracker_free(RustPitchTracker* ptr);
void n8_rust_pitch_tracker_prepare(RustPitchTracker* ptr, float sample_rate);
void n8_rust_pitch_tracker_reset(RustPitchTracker* ptr);
float n8_rust_pitch_tracker_get_fundamental_hz(const RustPitchTracker* ptr);
float n8_rust_pitch_tracker_get_clarity(const RustPitchTracker* ptr);
float n8_rust_pitch_tracker_get_midi_note(const RustPitchTracker* ptr);
float n8_rust_pitch_tracker_get_pitch_normalized(const RustPitchTracker* ptr);
void n8_rust_pitch_tracker_process(
    RustPitchTracker* ptr,
    const float* input,
    uint32_t num_samples
);

// Tipo opaco para RustSpectralFeatureExtractor en Rust (Analysis Core)
struct RustSpectralFeatureExtractor;

RustSpectralFeatureExtractor* n8_rust_spectral_feature_extractor_create();
void n8_rust_spectral_feature_extractor_destroy(RustSpectralFeatureExtractor* ptr);
void n8_rust_spectral_feature_extractor_prepare(RustSpectralFeatureExtractor* ptr, double sample_rate, size_t fft_size, size_t hop_size);
void n8_rust_spectral_feature_extractor_reset(RustSpectralFeatureExtractor* ptr);
void n8_rust_spectral_feature_extractor_process(RustSpectralFeatureExtractor* ptr, const float* input, uint32_t num_samples, bool is_silent);
float n8_rust_spectral_feature_extractor_get_centroid(const RustSpectralFeatureExtractor* ptr);
float n8_rust_spectral_feature_extractor_get_flux(const RustSpectralFeatureExtractor* ptr);
float n8_rust_spectral_feature_extractor_get_flatness(const RustSpectralFeatureExtractor* ptr);
float n8_rust_spectral_feature_extractor_get_energy(const RustSpectralFeatureExtractor* ptr);

// Snapshot de métricas instantáneas extraídas por AnalysisEngine en Rust
struct RustAnalysisSnapshot {
    float onset_strength;
    bool is_transient;
    uint8_t _pad[3];
    float pitch_hz;
    float pitch_clarity;
    float clarity;
    float pitch_midi;
    float pitch_normalized;
    float spectral_centroid;
    float spectral_flux;
    float spectral_flatness;
    float energy;
};

// Tipo opaco para RustAnalysisEngine en Rust (Analysis Core)
struct RustAnalysisEngine;

RustAnalysisEngine* n8_rust_analysis_engine_create(double sample_rate, size_t max_block_size);
void n8_rust_analysis_engine_destroy(RustAnalysisEngine* ptr);
void n8_rust_analysis_engine_prepare(RustAnalysisEngine* ptr, double sample_rate, size_t max_block_size);
void n8_rust_analysis_engine_reset(RustAnalysisEngine* ptr);
void n8_rust_analysis_engine_process(
    RustAnalysisEngine* ptr,
    const float* const* input_channels,
    uint32_t num_channels,
    uint32_t num_samples
);
void n8_rust_analysis_engine_get_snapshot(const RustAnalysisEngine* ptr, RustAnalysisSnapshot* out);
RustTransientDetector* n8_rust_analysis_engine_get_transient_detector(RustAnalysisEngine* ptr);
RustPitchTracker* n8_rust_analysis_engine_get_pitch_tracker(RustAnalysisEngine* ptr);
RustSpectralFeatureExtractor* n8_rust_analysis_engine_get_spectral_extractor(RustAnalysisEngine* ptr);

// =========================================================================
// Modulation Core C-ABI Declarations (Rules 7, 8, 25, 37, 46, 47)
// =========================================================================

// --- LFO ---
struct RustLFO;
RustLFO* n8_rust_lfo_create();
void n8_rust_lfo_destroy(RustLFO* ptr);
void n8_rust_lfo_prepare(RustLFO* ptr, double sample_rate);
void n8_rust_lfo_reset(RustLFO* ptr);
void n8_rust_lfo_set_shape(RustLFO* ptr, uint8_t shape);
void n8_rust_lfo_set_sync_division(RustLFO* ptr, uint8_t div);
void n8_rust_lfo_set_frequency_hz(RustLFO* ptr, float freq);
void n8_rust_lfo_set_pulse_width(RustLFO* ptr, float pw);
float n8_rust_lfo_process_sample(RustLFO* ptr, double bpm, double ppq, bool is_playing);
float n8_rust_lfo_get_current_value(const RustLFO* ptr);

// --- EnvelopeGenerator ---
struct RustEnvelopeGenerator;
RustEnvelopeGenerator* n8_rust_envelope_create();
void n8_rust_envelope_destroy(RustEnvelopeGenerator* ptr);
void n8_rust_envelope_prepare(RustEnvelopeGenerator* ptr, double sample_rate);
void n8_rust_envelope_reset(RustEnvelopeGenerator* ptr);
void n8_rust_envelope_set_attack_ms(RustEnvelopeGenerator* ptr, float ms);
void n8_rust_envelope_set_decay_ms(RustEnvelopeGenerator* ptr, float ms);
void n8_rust_envelope_set_sustain_level(RustEnvelopeGenerator* ptr, float lvl);
void n8_rust_envelope_set_release_ms(RustEnvelopeGenerator* ptr, float ms);
void n8_rust_envelope_trigger_gate(RustEnvelopeGenerator* ptr, bool gate_on);
float n8_rust_envelope_process_sample(RustEnvelopeGenerator* ptr);
float n8_rust_envelope_get_current_value(const RustEnvelopeGenerator* ptr);
uint8_t n8_rust_envelope_get_state(const RustEnvelopeGenerator* ptr);
bool n8_rust_envelope_is_active(const RustEnvelopeGenerator* ptr);

// --- AudioFollower ---
struct RustAudioFollower;
RustAudioFollower* n8_rust_audio_follower_create();
void n8_rust_audio_follower_destroy(RustAudioFollower* ptr);
void n8_rust_audio_follower_prepare(RustAudioFollower* ptr, double sample_rate);
void n8_rust_audio_follower_reset(RustAudioFollower* ptr);
void n8_rust_audio_follower_set_attack_ms(RustAudioFollower* ptr, float ms);
void n8_rust_audio_follower_set_release_ms(RustAudioFollower* ptr, float ms);
float n8_rust_audio_follower_process_block(RustAudioFollower* ptr, const float* const* input_channels, uint32_t num_channels, uint32_t num_samples);
float n8_rust_audio_follower_get_current_value(const RustAudioFollower* ptr);

// --- ChaosModulator ---
struct RustChaosModulator;
RustChaosModulator* n8_rust_chaos_modulator_create();
void n8_rust_chaos_modulator_destroy(RustChaosModulator* ptr);
void n8_rust_chaos_modulator_prepare(RustChaosModulator* ptr, double sample_rate);
void n8_rust_chaos_modulator_reset(RustChaosModulator* ptr);
void n8_rust_chaos_modulator_set_speed(RustChaosModulator* ptr, float speed);
void n8_rust_chaos_modulator_process_sample(RustChaosModulator* ptr);
float n8_rust_chaos_modulator_get_normalized_x(const RustChaosModulator* ptr);
float n8_rust_chaos_modulator_get_normalized_y(const RustChaosModulator* ptr);
float n8_rust_chaos_modulator_get_normalized_z(const RustChaosModulator* ptr);

// --- StepSequencer ---
struct RustSequencerStep {
    float value;
    float probability;
    float glide;
    bool active;
};

struct RustStepSequencer;
RustStepSequencer* n8_rust_step_sequencer_create();
void n8_rust_step_sequencer_destroy(RustStepSequencer* ptr);
void n8_rust_step_sequencer_prepare(RustStepSequencer* ptr, double sample_rate);
void n8_rust_step_sequencer_reset(RustStepSequencer* ptr);
void n8_rust_step_sequencer_set_num_steps(RustStepSequencer* ptr, uint32_t count);
void n8_rust_step_sequencer_set_step(RustStepSequencer* ptr, uint32_t index, float value, float probability, float glide, bool active);
void n8_rust_step_sequencer_get_step(const RustStepSequencer* ptr, uint32_t index, RustSequencerStep* out);
float n8_rust_step_sequencer_process_sample(RustStepSequencer* ptr, double ppq, bool is_playing);
float n8_rust_step_sequencer_get_current_value(const RustStepSequencer* ptr);
uint32_t n8_rust_step_sequencer_get_current_step(const RustStepSequencer* ptr);

// --- EuclideanModulator ---
struct RustEuclideanModulator;
RustEuclideanModulator* n8_rust_euclidean_modulator_create();
void n8_rust_euclidean_modulator_destroy(RustEuclideanModulator* ptr);
void n8_rust_euclidean_modulator_prepare(RustEuclideanModulator* ptr, double sample_rate);
void n8_rust_euclidean_modulator_reset(RustEuclideanModulator* ptr);
void n8_rust_euclidean_modulator_set_steps(RustEuclideanModulator* ptr, int32_t steps);
void n8_rust_euclidean_modulator_set_pulses(RustEuclideanModulator* ptr, int32_t pulses);
void n8_rust_euclidean_modulator_set_rotation(RustEuclideanModulator* ptr, int32_t rotation);
void n8_rust_euclidean_modulator_set_decay(RustEuclideanModulator* ptr, float seconds);
void n8_rust_euclidean_modulator_set_sync(RustEuclideanModulator* ptr, uint8_t div, float free_bpm);
bool n8_rust_euclidean_modulator_has_pulse_at_step(const RustEuclideanModulator* ptr, int32_t step);
void n8_rust_euclidean_modulator_process_sample(RustEuclideanModulator* ptr, double bpm, double ppq, bool is_playing);
float n8_rust_euclidean_modulator_get_current_value(const RustEuclideanModulator* ptr);

// --- MSEGModulator ---
struct RustMSEGPoint {
    float time;
    float level;
    float curve;
};

struct RustMSEGModulator;
RustMSEGModulator* n8_rust_mseg_modulator_create();
void n8_rust_mseg_modulator_destroy(RustMSEGModulator* ptr);
void n8_rust_mseg_modulator_prepare(RustMSEGModulator* ptr, double sample_rate);
void n8_rust_mseg_modulator_reset(RustMSEGModulator* ptr);
void n8_rust_mseg_modulator_set_default_envelope(RustMSEGModulator* ptr);
void n8_rust_mseg_modulator_set_point(RustMSEGModulator* ptr, size_t index, float time, float level, float curve);
void n8_rust_mseg_modulator_set_num_points(RustMSEGModulator* ptr, size_t count);
void n8_rust_mseg_modulator_set_mode(RustMSEGModulator* ptr, uint8_t mode);
void n8_rust_mseg_modulator_set_sync(RustMSEGModulator* ptr, uint8_t div, float free_hz);
void n8_rust_mseg_modulator_trigger(RustMSEGModulator* ptr);
void n8_rust_mseg_modulator_process_sample(RustMSEGModulator* ptr, double bpm, double ppq, bool is_playing);
float n8_rust_mseg_modulator_get_current_value(const RustMSEGModulator* ptr);

// --- MacroManager ---
struct RustMacroManager;
RustMacroManager* n8_rust_macro_manager_create();
void n8_rust_macro_manager_destroy(RustMacroManager* ptr);
void n8_rust_macro_manager_reset(RustMacroManager* ptr);
void n8_rust_macro_manager_set_macro(RustMacroManager* ptr, size_t idx, float val);
float n8_rust_macro_manager_get_macro(const RustMacroManager* ptr, size_t idx);
float n8_rust_macro_manager_get_smoothed_macro(const RustMacroManager* ptr, size_t idx);
void n8_rust_macro_manager_set_pad(RustMacroManager* ptr, float x, float y);
float n8_rust_macro_manager_get_pad_x(const RustMacroManager* ptr);
float n8_rust_macro_manager_get_pad_y(const RustMacroManager* ptr);
float n8_rust_macro_manager_get_smoothed_pad_x(const RustMacroManager* ptr);
float n8_rust_macro_manager_get_smoothed_pad_y(const RustMacroManager* ptr);
void n8_rust_macro_manager_process_smoothing(RustMacroManager* ptr);

// --- ModulationMatrix ---
struct RustModulationRoute {
    uint16_t source;
    uint8_t _pad[2];
    uint32_t target_node_id;
    uint32_t target_param_id;
    float amount;
    bool bipolar;
    bool is_active;
    uint8_t _pad2[2];
};

struct RustModulationMatrix;
RustModulationMatrix* n8_rust_modulation_matrix_create();
void n8_rust_modulation_matrix_destroy(RustModulationMatrix* ptr);
void n8_rust_modulation_matrix_reset(RustModulationMatrix* ptr);
int32_t n8_rust_modulation_matrix_add_route(RustModulationMatrix* ptr, uint16_t source, uint32_t target_node, uint32_t target_param, float amount, bool bipolar);
void n8_rust_modulation_matrix_remove_route(RustModulationMatrix* ptr, size_t index);
void n8_rust_modulation_matrix_set_route_amount(RustModulationMatrix* ptr, size_t index, float amount);
size_t n8_rust_modulation_matrix_get_active_route_count(const RustModulationMatrix* ptr);
void n8_rust_modulation_matrix_get_route(const RustModulationMatrix* ptr, size_t index, RustModulationRoute* out);
float n8_rust_modulation_matrix_calculate_offset(const RustModulationMatrix* ptr, uint32_t target_node, uint32_t target_param, const float* source_values, size_t num_sources);

// --- ModulationEngine ---
struct RustModulationEngine;
RustModulationEngine* n8_rust_modulation_engine_create();
void n8_rust_modulation_engine_destroy(RustModulationEngine* ptr);
void n8_rust_modulation_engine_prepare(RustModulationEngine* ptr, double sample_rate);
void n8_rust_modulation_engine_reset(RustModulationEngine* ptr);
void n8_rust_modulation_engine_process_sources(RustModulationEngine* ptr, double bpm, double ppq, bool is_playing, const float* const* input_channels, uint32_t num_channels, uint32_t num_samples);
float n8_rust_modulation_engine_calculate_offset(const RustModulationEngine* ptr, uint32_t target_node, uint32_t target_param);
void n8_rust_modulation_engine_update_analysis_sources(RustModulationEngine* ptr, float transient, float pitch_norm, float centroid, float flux);
float n8_rust_modulation_engine_get_source_value(const RustModulationEngine* ptr, uint16_t source_type);
RustLFO* n8_rust_modulation_engine_get_lfo(RustModulationEngine* ptr, size_t index);
RustEnvelopeGenerator* n8_rust_modulation_engine_get_envelope(RustModulationEngine* ptr, size_t index);
RustStepSequencer* n8_rust_modulation_engine_get_step_sequencer(RustModulationEngine* ptr);
RustAudioFollower* n8_rust_modulation_engine_get_audio_follower(RustModulationEngine* ptr);
RustMacroManager* n8_rust_modulation_engine_get_macro_manager(RustModulationEngine* ptr);
RustModulationMatrix* n8_rust_modulation_engine_get_matrix(RustModulationEngine* ptr);
RustMSEGModulator* n8_rust_modulation_engine_get_mseg(RustModulationEngine* ptr, size_t index);
RustEuclideanModulator* n8_rust_modulation_engine_get_euclidean(RustModulationEngine* ptr, size_t index);
RustChaosModulator* n8_rust_modulation_engine_get_chaos(RustModulationEngine* ptr);

// ============================================================================
// EVENT ENGINE (Roadmap Phase 2, Rules 2, 3, 10, 11, 46)
// ============================================================================

struct RustEventAttributes {
    uint32_t id;
    uint16_t event_type;
    uint8_t state;
    uint8_t disappearance_mode;

    float source_follow;
    float pitch_ratio;
    float gain;
    float pan;
    float azimuth;
    float elevation;
    float distance;
    float energy;

    uint32_t generation;
    uint32_t parent_id;

    uint64_t age_samples;
    uint64_t lifetime_samples;
    uint32_t attack_samples;
    uint32_t release_samples;
};

struct RustEventTelemetryItem {
    float pan;
    float pitch_ratio;
    float energy;
    float distance;
    float azimuth;
    uint16_t event_type;
    uint8_t generation;
    bool is_alive;
};

struct RustEventCaptureBuffer;
RustEventCaptureBuffer* n8_rust_event_capture_buffer_create();
void n8_rust_event_capture_buffer_destroy(RustEventCaptureBuffer* ptr);
void n8_rust_event_capture_buffer_prepare(RustEventCaptureBuffer* ptr, double sample_rate, double max_history_seconds, size_t num_channels);
void n8_rust_event_capture_buffer_reset(RustEventCaptureBuffer* ptr);
void n8_rust_event_capture_buffer_write(RustEventCaptureBuffer* ptr, const float* in_l, const float* in_r, size_t num_samples);
float n8_rust_event_capture_buffer_read_sample(const RustEventCaptureBuffer* ptr, size_t channel, double sample_offset);
size_t n8_rust_event_capture_buffer_get_capacity(const RustEventCaptureBuffer* ptr);
size_t n8_rust_event_capture_buffer_get_write_pos(const RustEventCaptureBuffer* ptr);
size_t n8_rust_event_capture_buffer_get_num_channels(const RustEventCaptureBuffer* ptr);

struct RustAudioFragment;
RustAudioFragment* n8_rust_audio_fragment_create();
void n8_rust_audio_fragment_destroy(RustAudioFragment* ptr);
void n8_rust_audio_fragment_configure(RustAudioFragment* ptr, double capture_offset_samples, uint64_t duration_samples, float pitch_ratio, bool reverse);
void n8_rust_audio_fragment_reset(RustAudioFragment* ptr);
bool n8_rust_audio_fragment_get_next_sample(RustAudioFragment* ptr, const RustEventCaptureBuffer* capture_ptr, float envelope_gain, float* out_l, float* out_r);
double n8_rust_audio_fragment_get_current_phase(const RustAudioFragment* ptr);
uint64_t n8_rust_audio_fragment_get_duration_samples(const RustAudioFragment* ptr);
bool n8_rust_audio_fragment_is_finished(const RustAudioFragment* ptr);

struct RustEvent;
RustEvent* n8_rust_event_create();
void n8_rust_event_destroy(RustEvent* ptr);
void n8_rust_event_initialize(RustEvent* ptr, const RustEventAttributes* attrs, double capture_offset, uint64_t duration_samples, bool reverse);
void n8_rust_event_start(RustEvent* ptr);
void n8_rust_event_kill(RustEvent* ptr);
void n8_rust_event_release(RustEvent* ptr);
void n8_rust_event_render(RustEvent* ptr, const RustEventCaptureBuffer* capture_ptr, float* out_l, float* out_r, size_t num_samples, float source_level, int32_t source_silence_override);
bool n8_rust_event_is_active(const RustEvent* ptr);
uint8_t n8_rust_event_get_state(const RustEvent* ptr);
void n8_rust_event_set_state(RustEvent* ptr, uint8_t state);
void n8_rust_event_get_attributes(const RustEvent* ptr, RustEventAttributes* out_attrs);
void n8_rust_event_set_attributes(RustEvent* ptr, const RustEventAttributes* in_attrs);

struct RustEventPool;
RustEventPool* n8_rust_event_pool_create();
void n8_rust_event_pool_destroy(RustEventPool* ptr);
void n8_rust_event_pool_prepare(RustEventPool* ptr, size_t max_events);
void n8_rust_event_pool_reset(RustEventPool* ptr);
RustEvent* n8_rust_event_pool_acquire(RustEventPool* ptr);
void n8_rust_event_pool_release(RustEventPool* ptr, RustEvent* ev);
size_t n8_rust_event_pool_get_capacity(const RustEventPool* ptr);
size_t n8_rust_event_pool_get_active_count(const RustEventPool* ptr);
size_t n8_rust_event_pool_get_available_count(const RustEventPool* ptr);
RustEvent* n8_rust_event_pool_get_event_at(RustEventPool* ptr, size_t index);
size_t n8_rust_event_pool_get_event_index(const RustEventPool* ptr, const RustEvent* ev);

struct RustEventTelemetryBuffer;
RustEventTelemetryBuffer* n8_rust_event_telemetry_create();
void n8_rust_event_telemetry_destroy(RustEventTelemetryBuffer* ptr);
void n8_rust_event_telemetry_reset(const RustEventTelemetryBuffer* ptr);
bool n8_rust_event_telemetry_push(const RustEventTelemetryBuffer* ptr, const RustEventTelemetryItem* item);
bool n8_rust_event_telemetry_pop(const RustEventTelemetryBuffer* ptr, RustEventTelemetryItem* out_item);
size_t n8_rust_event_telemetry_size(const RustEventTelemetryBuffer* ptr);

struct RustEventManager;
RustEventManager* n8_rust_event_manager_create();
void n8_rust_event_manager_destroy(RustEventManager* ptr);
void n8_rust_event_manager_prepare(RustEventManager* ptr, double sample_rate, size_t max_block_size, size_t pool_capacity);
void n8_rust_event_manager_reset(RustEventManager* ptr);
void n8_rust_event_manager_capture_input(RustEventManager* ptr, const float* in_l, const float* in_r, size_t num_samples);
RustEvent* n8_rust_event_manager_spawn_event(RustEventManager* ptr, uint16_t event_type, double capture_offset_samples, uint64_t duration_samples, float pitch_ratio, float gain, float pan, float source_follow, uint8_t disappearance_mode, bool reverse);
RustEvent* n8_rust_event_manager_spawn_child_event(RustEventManager* ptr, const RustEvent* parent_ptr, double offset_shift_samples, float pitch_multiplier, float energy_decay);
void n8_rust_event_manager_render(RustEventManager* ptr, float* out_l, float* out_r, size_t num_samples, float source_level, int32_t source_silence_override);
void n8_rust_event_manager_set_editor_active(const RustEventManager* ptr, bool active);
bool n8_rust_event_manager_is_editor_active(const RustEventManager* ptr);
size_t n8_rust_event_manager_get_active_count(const RustEventManager* ptr);
size_t n8_rust_event_manager_get_pool_capacity(const RustEventManager* ptr);
RustEventCaptureBuffer* n8_rust_event_manager_get_capture_buffer(RustEventManager* ptr);
RustEventTelemetryBuffer* n8_rust_event_manager_get_telemetry_buffer(RustEventManager* ptr);
RustEventPool* n8_rust_event_manager_get_event_pool(RustEventManager* ptr);

// ==============================================================================
// Graph Engine & Topological Compiler C-ABI (Phase 6, Rules 4, 28, 29, 30)
// ==============================================================================

#pragma pack(push, 8)
struct RustGraphConnection {
    uint32_t id;
    uint32_t source_node_id;
    uint32_t source_pin_id;
    uint32_t dest_node_id;
    uint32_t dest_pin_id;
};

struct RustNodeLayout {
    uint32_t node_id;
    uint32_t depth;
    float pos_x;
    float pos_y;
};

struct RustExecutionStepDesc {
    uint32_t node_id;
    bool is_bypassed;
    bool is_root;
    bool is_leaf;
    int32_t sidechain_step_index;
    int32_t audio_rate_mod_step_index;
    uint32_t num_predecessors;
    uint32_t num_successors;
    int16_t predecessor_step_indices[8];
};
#pragma pack(pop)

bool n8_rust_graph_would_create_cycle(
    uint32_t src_node,
    uint32_t dest_node,
    const uint32_t* node_ids,
    uint32_t num_nodes,
    const RustGraphConnection* connections,
    uint32_t num_connections
);

bool n8_rust_graph_validate_and_topological_sort(
    const uint32_t* node_ids,
    uint32_t num_nodes,
    const RustGraphConnection* connections,
    uint32_t num_connections,
    uint32_t* out_sorted
);

void n8_rust_graph_compute_topological_depths(
    const uint32_t* node_ids,
    uint32_t num_nodes,
    const RustGraphConnection* connections,
    uint32_t num_connections,
    uint32_t* out_depths
);

void n8_rust_graph_compute_auto_layout(
    const uint32_t* node_ids,
    uint32_t num_nodes,
    const RustGraphConnection* connections,
    uint32_t num_connections,
    float start_x,
    float start_y,
    float step_x,
    float step_y,
    RustNodeLayout* out_layouts
);

struct RustGraphPlanExchanger;

RustGraphPlanExchanger* n8_rust_graph_exchanger_create();
void n8_rust_graph_exchanger_destroy(RustGraphPlanExchanger* ptr);
void n8_rust_graph_exchanger_compile_standby(
    RustGraphPlanExchanger* ptr,
    const uint32_t* topological_order,
    uint32_t num_nodes,
    const RustGraphConnection* connections,
    uint32_t num_connections,
    const bool* bypassed_flags,
    uint32_t num_flags
);
void n8_rust_graph_exchanger_commit(const RustGraphPlanExchanger* ptr);
size_t n8_rust_graph_exchanger_get_active_step_count(const RustGraphPlanExchanger* ptr);
bool n8_rust_graph_exchanger_get_active_step(const RustGraphPlanExchanger* ptr, size_t index, RustExecutionStepDesc* out_step);
bool n8_rust_graph_exchanger_active_has_explicit_connections(const RustGraphPlanExchanger* ptr);

// ==============================================================================
// SubGraph Container Engine Core (NodeType::Container / Rules 5, 6, 9, 28, 46, 47)
// ==============================================================================

struct RustContainerCore;

RustContainerCore* n8_rust_container_create(float sample_rate);
void n8_rust_container_free(RustContainerCore* ptr);
void n8_rust_container_prepare(RustContainerCore* ptr, float sample_rate, size_t max_block_size);
void n8_rust_container_reset(RustContainerCore* ptr);
void n8_rust_container_set_parameter(RustContainerCore* ptr, uint32_t param_id, float value);
float n8_rust_container_get_parameter(const RustContainerCore* ptr, uint32_t param_id);

void n8_rust_container_process_passthrough(
    RustContainerCore* ptr,
    const float* in_l,
    const float* in_r,
    float* out_l,
    float* out_r,
    size_t num_samples
);

void n8_rust_container_process_mix(
    RustContainerCore* ptr,
    const float* dry_l,
    const float* dry_r,
    const float* wet_l,
    const float* wet_r,
    float* out_l,
    float* out_r,
    size_t num_samples
);

void n8_rust_container_process_channels_passthrough(
    RustContainerCore* ptr,
    const float* const* in_ptrs,
    size_t num_in,
    float* const* out_ptrs,
    size_t num_out,
    size_t num_samples
);

void n8_rust_container_process_channels_mix(
    RustContainerCore* ptr,
    const float* const* dry_ptrs,
    size_t num_dry,
    const float* const* wet_ptrs,
    size_t num_wet,
    float* const* out_ptrs,
    size_t num_out,
    size_t num_samples
);

bool n8_rust_container_validate_subgraph(
    const uint32_t* node_ids,
    size_t num_nodes,
    const RustGraphConnection* connections,
    size_t num_connections
);

// ==============================================================================
// Scene Snapshot, Anti-Click Morphing & Smart Randomizer (Phase 7, Rules 7, 21, 35)
// ==============================================================================

#pragma pack(push, 8)
struct RustParamEntry {
    uint32_t param_id;
    float value;
};
#pragma pack(pop)

struct RustSceneMorphEngine;
struct RustSmartRandomizer;

RustSceneMorphEngine* n8_rust_scene_morph_create();
void n8_rust_scene_morph_destroy(RustSceneMorphEngine* ptr);
void n8_rust_scene_morph_clear_scene(RustSceneMorphEngine* ptr, uint32_t scene_idx);
void n8_rust_scene_morph_set_node_params(RustSceneMorphEngine* ptr, uint32_t scene_idx, uint32_t node_id, const RustParamEntry* params, uint32_t num_params);
void n8_rust_scene_morph_set_macros(RustSceneMorphEngine* ptr, uint32_t scene_idx, const float* macros);
bool n8_rust_scene_morph_has_scene(const RustSceneMorphEngine* ptr, uint32_t scene_idx);

void n8_rust_scene_morph_set_factor_1d(RustSceneMorphEngine* ptr, float t);
float n8_rust_scene_morph_get_target_factor_1d(const RustSceneMorphEngine* ptr);
float n8_rust_scene_morph_get_current_factor_1d(const RustSceneMorphEngine* ptr);
float n8_rust_scene_morph_step_smoothing_1d(RustSceneMorphEngine* ptr, float smoothing_speed);
float n8_rust_scene_morph_interpolate_param_1d(const RustSceneMorphEngine* ptr, uint32_t node_id, uint32_t param_id, float fallback_val);
void n8_rust_scene_morph_interpolate_macros_1d(const RustSceneMorphEngine* ptr, float* out_macros);

void n8_rust_scene_morph_set_xy(RustSceneMorphEngine* ptr, float x, float y);
float n8_rust_scene_morph_get_target_x(const RustSceneMorphEngine* ptr);
float n8_rust_scene_morph_get_target_y(const RustSceneMorphEngine* ptr);
float n8_rust_scene_morph_get_current_x(const RustSceneMorphEngine* ptr);
float n8_rust_scene_morph_get_current_y(const RustSceneMorphEngine* ptr);
void n8_rust_scene_morph_step_smoothing_xy(RustSceneMorphEngine* ptr, float smoothing_speed, float* out_x, float* out_y);
float n8_rust_scene_morph_interpolate_param_xy(const RustSceneMorphEngine* ptr, uint32_t node_id, uint32_t param_id, float fallback_val);
void n8_rust_scene_morph_interpolate_macros_xy(const RustSceneMorphEngine* ptr, float* out_macros);

RustSmartRandomizer* n8_rust_smart_randomizer_create(uint32_t seed);
void n8_rust_smart_randomizer_destroy(RustSmartRandomizer* ptr);
float n8_rust_smart_randomizer_perturb_param(
    RustSmartRandomizer* ptr,
    float current_val,
    float min_val,
    float max_val,
    float intensity,
    bool is_feedback,
    bool is_resonance,
    bool is_drive
);
void n8_rust_smart_randomizer_mutate_macros(RustSmartRandomizer* ptr, float* macros, float intensity);

// ==============================================================================
// Optimization Engine & SIMD Kernels (Phase 8, Rules 9, 34, 47)
// ==============================================================================

#pragma pack(push, 8)
struct RustStepSlotAssignment {
    uint32_t step_index;
    uint32_t allocated_slot;
};
#pragma pack(pop)

uint32_t n8_rust_opt_enable_ftz_daz();
void n8_rust_opt_restore_mxcsr(uint32_t old_csr);
bool n8_rust_opt_is_ftz_daz_active();

float n8_rust_opt_fast_tanh(float x);
float n8_rust_opt_fast_soft_clip(float x);
float n8_rust_opt_fast_exp_negative(float x);
float n8_rust_opt_fast_db_to_gain(float db);

void n8_rust_opt_vec_pade_tanh(const float* input, float* output, uint32_t num_samples);
void n8_rust_opt_vec_multiply_add(const float* a, const float* b, const float* c, float* out, uint32_t num_samples);
void n8_rust_opt_vec_apply_gain(const float* input, float* output, float gain, uint32_t num_samples);
void n8_rust_opt_vec_mix(const float* dry, const float* wet, float* out, float mix, uint32_t num_samples);
void n8_rust_opt_vec_flush_denormals(float* buffer, uint32_t num_samples);
void n8_rust_opt_vec_soft_clip(const float* input, float* output, uint32_t num_samples);

uint32_t n8_rust_opt_compute_optimal_buffer_slots(
    const uint32_t* step_predecessors,
    uint32_t total_predecessors,
    const uint32_t* step_pred_offsets,
    const uint32_t* step_pred_counts,
    uint32_t num_steps,
    RustStepSlotAssignment* out_assignments
);

// ==============================================================================
// Phase 9: Large Effect Library (Rules 5, 8, 9, 34, 46, 47)
// ==============================================================================

// --- 1. Dimension Chorus ---
struct RustDimensionChorus;
RustDimensionChorus* n8_rust_dimension_chorus_create(float sample_rate);
void n8_rust_dimension_chorus_destroy(RustDimensionChorus* ptr);
void n8_rust_dimension_chorus_prepare(RustDimensionChorus* ptr, float sample_rate);
void n8_rust_dimension_chorus_reset(RustDimensionChorus* ptr);
void n8_rust_dimension_chorus_set_param(RustDimensionChorus* ptr, uint32_t param_id, float value);
void n8_rust_dimension_chorus_process(RustDimensionChorus* ptr, const float* in_l, const float* in_r, float* out_l, float* out_r, uint32_t num_samples);

// --- 2. Dynamic Tremolo ---
struct RustDynamicTremolo;
RustDynamicTremolo* n8_rust_dynamic_tremolo_create(float sample_rate);
void n8_rust_dynamic_tremolo_destroy(RustDynamicTremolo* ptr);
void n8_rust_dynamic_tremolo_prepare(RustDynamicTremolo* ptr, float sample_rate);
void n8_rust_dynamic_tremolo_reset(RustDynamicTremolo* ptr);
void n8_rust_dynamic_tremolo_set_param(RustDynamicTremolo* ptr, uint32_t param_id, float value);
void n8_rust_dynamic_tremolo_process(RustDynamicTremolo* ptr, const float* in_l, const float* in_r, float* out_l, float* out_r, uint32_t num_samples);

// --- 3. Multiband Saturator ---
struct RustMultibandSaturator;
RustMultibandSaturator* n8_rust_multiband_saturator_create(float sample_rate);
void n8_rust_multiband_saturator_destroy(RustMultibandSaturator* ptr);
void n8_rust_multiband_saturator_prepare(RustMultibandSaturator* ptr, float sample_rate);
void n8_rust_multiband_saturator_reset(RustMultibandSaturator* ptr);
void n8_rust_multiband_saturator_set_param(RustMultibandSaturator* ptr, uint32_t param_id, float value);
void n8_rust_multiband_saturator_process(RustMultibandSaturator* ptr, const float* in_l, const float* in_r, float* out_l, float* out_r, uint32_t num_samples);

// --- 4. Comb Resonator ---
struct RustCombResonator;
RustCombResonator* n8_rust_comb_resonator_create(float sample_rate);
void n8_rust_comb_resonator_destroy(RustCombResonator* ptr);
void n8_rust_comb_resonator_prepare(RustCombResonator* ptr, float sample_rate);
void n8_rust_comb_resonator_reset(RustCombResonator* ptr);
void n8_rust_comb_resonator_set_param(RustCombResonator* ptr, uint32_t param_id, float value);
void n8_rust_comb_resonator_process(RustCombResonator* ptr, const float* in_l, const float* in_r, float* out_l, float* out_r, uint32_t num_samples);

// --- 5. Pitch Delay ---
struct RustPitchDelay;
RustPitchDelay* n8_rust_pitch_delay_create(float sample_rate);
void n8_rust_pitch_delay_destroy(RustPitchDelay* ptr);
void n8_rust_pitch_delay_prepare(RustPitchDelay* ptr, float sample_rate);
void n8_rust_pitch_delay_reset(RustPitchDelay* ptr);
void n8_rust_pitch_delay_set_param(RustPitchDelay* ptr, uint32_t param_id, float value);
void n8_rust_pitch_delay_process(RustPitchDelay* ptr, const float* in_l, const float* in_r, float* out_l, float* out_r, uint32_t num_samples);

// --- 6. Phase Dispersion ---
struct RustPhaseDispersion;
RustPhaseDispersion* n8_rust_phase_dispersion_create(float sample_rate);
void n8_rust_phase_dispersion_destroy(RustPhaseDispersion* ptr);
void n8_rust_phase_dispersion_prepare(RustPhaseDispersion* ptr, float sample_rate);
void n8_rust_phase_dispersion_reset(RustPhaseDispersion* ptr);
void n8_rust_phase_dispersion_set_param(RustPhaseDispersion* ptr, uint32_t param_id, float value);
void n8_rust_phase_dispersion_process(RustPhaseDispersion* ptr, const float* in_l, const float* in_r, float* out_l, float* out_r, uint32_t num_samples);

// --- 7. Morphing SVF ---
struct RustMorphingSvf;
RustMorphingSvf* n8_rust_morphing_svf_create(float sample_rate);
void n8_rust_morphing_svf_destroy(RustMorphingSvf* ptr);
void n8_rust_morphing_svf_prepare(RustMorphingSvf* ptr, float sample_rate);
void n8_rust_morphing_svf_reset(RustMorphingSvf* ptr);
void n8_rust_morphing_svf_set_param(RustMorphingSvf* ptr, uint32_t param_id, float value);
void n8_rust_morphing_svf_process(RustMorphingSvf* ptr, const float* in_l, const float* in_r, const float* fm_in, float* out_l, float* out_r, uint32_t num_samples);

// --- 8. Fractal Resonator ---
struct RustFractalResonator;
RustFractalResonator* n8_rust_fractal_resonator_create(float sample_rate);
void n8_rust_fractal_resonator_destroy(RustFractalResonator* ptr);
void n8_rust_fractal_resonator_prepare(RustFractalResonator* ptr, float sample_rate);
void n8_rust_fractal_resonator_reset(RustFractalResonator* ptr);
void n8_rust_fractal_resonator_set_param(RustFractalResonator* ptr, uint32_t param_id, float value);
void n8_rust_fractal_resonator_process(RustFractalResonator* ptr, const float* in_l, const float* in_r, float* out_l, float* out_r, uint32_t num_samples);

// --- 9. Buchla 292 Vactrol LPG ---
struct RustVactrolLPG;
RustVactrolLPG* n8_rust_vactrol_lpg_create(float sample_rate);
void n8_rust_vactrol_lpg_destroy(RustVactrolLPG* ptr);
void n8_rust_vactrol_lpg_prepare(RustVactrolLPG* ptr, float sample_rate);
void n8_rust_vactrol_lpg_reset(RustVactrolLPG* ptr);
void n8_rust_vactrol_lpg_set_param(RustVactrolLPG* ptr, uint32_t param_id, float value);
void n8_rust_vactrol_lpg_trigger_strike(RustVactrolLPG* ptr);
void n8_rust_vactrol_lpg_process(
    RustVactrolLPG* ptr,
    const float* in_l,
    const float* in_r,
    const float* strike_in,
    float* out_l,
    float* out_r,
    uint32_t num_samples
);

// --- 10. Shepard-Risset Barberpole Filter ---
struct RustBarberpoleFilter;
RustBarberpoleFilter* n8_rust_barberpole_filter_create(float sample_rate);
void n8_rust_barberpole_filter_destroy(RustBarberpoleFilter* ptr);
void n8_rust_barberpole_filter_prepare(RustBarberpoleFilter* ptr, float sample_rate);
void n8_rust_barberpole_filter_reset(RustBarberpoleFilter* ptr);
void n8_rust_barberpole_filter_set_param(RustBarberpoleFilter* ptr, uint32_t param_id, float value);
void n8_rust_barberpole_filter_process(
    RustBarberpoleFilter* ptr,
    const float* in_l,
    const float* in_r,
    float* out_l,
    float* out_r,
    uint32_t num_samples
);

// --- 11. Sidechain Pumper (Kickstart-Style Ducking & Multiband Pump) ---
struct RustSidechainPumper;
RustSidechainPumper* n8_rust_sidechain_pumper_create(float sample_rate);
void n8_rust_sidechain_pumper_destroy(RustSidechainPumper* ptr);
void n8_rust_sidechain_pumper_prepare(RustSidechainPumper* ptr, float sample_rate);
void n8_rust_sidechain_pumper_reset(RustSidechainPumper* ptr);
void n8_rust_sidechain_pumper_set_param(RustSidechainPumper* ptr, uint32_t param_id, float value);
void n8_rust_sidechain_pumper_set_host_sync(RustSidechainPumper* ptr, float bpm, double ppq, bool is_playing);
void n8_rust_sidechain_pumper_retrigger(RustSidechainPumper* ptr);
void n8_rust_sidechain_pumper_process(
    RustSidechainPumper* ptr,
    const float* in_l,
    const float* in_r,
    float* out_l,
    float* out_r,
    const float* sc_l,
    const float* sc_r,
    uint32_t num_samples
);

// --- 12. Frequency-Split Spectral Dynamic Spatial Panner ---
struct RustFrequencySplitPanner;
RustFrequencySplitPanner* n8_rust_frequency_split_panner_create(float sample_rate);
void n8_rust_frequency_split_panner_destroy(RustFrequencySplitPanner* ptr);
void n8_rust_frequency_split_panner_prepare(RustFrequencySplitPanner* ptr, float sample_rate);
void n8_rust_frequency_split_panner_reset(RustFrequencySplitPanner* ptr);
void n8_rust_frequency_split_panner_set_param(RustFrequencySplitPanner* ptr, uint32_t param_id, float value);
void n8_rust_frequency_split_panner_process(
    RustFrequencySplitPanner* ptr,
    const float* in_l,
    const float* in_r,
    float* out_l,
    float* out_r,
    uint32_t num_samples
);

// --- 13. Chebyshev Polynomial Harmonic Shaper ---
struct RustChebyshevHarmonicShaper;
RustChebyshevHarmonicShaper* n8_rust_chebyshev_harmonic_shaper_create(float sample_rate);
void n8_rust_chebyshev_harmonic_shaper_destroy(RustChebyshevHarmonicShaper* ptr);
void n8_rust_chebyshev_harmonic_shaper_prepare(RustChebyshevHarmonicShaper* ptr, float sample_rate);
void n8_rust_chebyshev_harmonic_shaper_reset(RustChebyshevHarmonicShaper* ptr);
void n8_rust_chebyshev_harmonic_shaper_set_param(RustChebyshevHarmonicShaper* ptr, uint32_t param_id, float value);
void n8_rust_chebyshev_harmonic_shaper_process(
    RustChebyshevHarmonicShaper* ptr,
    const float* in_l,
    const float* in_r,
    float* out_l,
    float* out_r,
    uint32_t num_samples
);

// --- 14. Subharmonic Synthesizer & Bass Extender ---
struct RustSubharmonicSynthesizer;
RustSubharmonicSynthesizer* n8_rust_subharmonic_synthesizer_create(float sample_rate);
void n8_rust_subharmonic_synthesizer_destroy(RustSubharmonicSynthesizer* ptr);
void n8_rust_subharmonic_synthesizer_prepare(RustSubharmonicSynthesizer* ptr, float sample_rate);
void n8_rust_subharmonic_synthesizer_reset(RustSubharmonicSynthesizer* ptr);
void n8_rust_subharmonic_synthesizer_set_param(RustSubharmonicSynthesizer* ptr, uint32_t param_id, float value);
void n8_rust_subharmonic_synthesizer_process(
    RustSubharmonicSynthesizer* ptr,
    const float* in_l,
    const float* in_r,
    float* out_l,
    float* out_r,
    uint32_t num_samples
);

// --- 15. Transaural Crosstalk Canceller ---
struct RustTransauralCrosstalkCanceller;
RustTransauralCrosstalkCanceller* n8_rust_transaural_crosstalk_canceller_create(float sample_rate);
void n8_rust_transaural_crosstalk_canceller_destroy(RustTransauralCrosstalkCanceller* ptr);
void n8_rust_transaural_crosstalk_canceller_prepare(RustTransauralCrosstalkCanceller* ptr, float sample_rate);
void n8_rust_transaural_crosstalk_canceller_reset(RustTransauralCrosstalkCanceller* ptr);
void n8_rust_transaural_crosstalk_canceller_set_param(RustTransauralCrosstalkCanceller* ptr, uint32_t param_id, float value);
void n8_rust_transaural_crosstalk_canceller_process(
    RustTransauralCrosstalkCanceller* ptr,
    const float* in_l,
    const float* in_r,
    float* out_l,
    float* out_r,
    uint32_t num_samples
);

// --- 16. Doppler 3D Orbital Spatializer ---
struct RustDopplerOrbitalSpatializer;
RustDopplerOrbitalSpatializer* n8_rust_doppler_orbital_spatializer_create(float sample_rate);
void n8_rust_doppler_orbital_spatializer_destroy(RustDopplerOrbitalSpatializer* ptr);
void n8_rust_doppler_orbital_spatializer_prepare(RustDopplerOrbitalSpatializer* ptr, float sample_rate);
void n8_rust_doppler_orbital_spatializer_reset(RustDopplerOrbitalSpatializer* ptr);
void n8_rust_doppler_orbital_spatializer_set_param(RustDopplerOrbitalSpatializer* ptr, uint32_t param_id, float value);
void n8_rust_doppler_orbital_spatializer_process(
    RustDopplerOrbitalSpatializer* ptr,
    const float* in_l,
    const float* in_r,
    float* out_l,
    float* out_r,
    uint32_t num_samples
);

// --- 17. Granular Buffer Shuffler & Reverse Memory ---
struct RustGranularBufferShuffler;
RustGranularBufferShuffler* n8_rust_granular_buffer_shuffler_create(float sample_rate);
void n8_rust_granular_buffer_shuffler_destroy(RustGranularBufferShuffler* ptr);
void n8_rust_granular_buffer_shuffler_prepare(RustGranularBufferShuffler* ptr, float sample_rate);
void n8_rust_granular_buffer_shuffler_reset(RustGranularBufferShuffler* ptr);
void n8_rust_granular_buffer_shuffler_set_param(RustGranularBufferShuffler* ptr, uint32_t param_id, float value);
void n8_rust_granular_buffer_shuffler_process(
    RustGranularBufferShuffler* ptr,
    const float* in_l,
    const float* in_r,
    float* out_l,
    float* out_r,
    uint32_t num_samples
);

// --- 18. Disintegration Tape Looper ---
struct RustDisintegrationLooper;
RustDisintegrationLooper* n8_rust_disintegration_looper_create(float sample_rate);
void n8_rust_disintegration_looper_destroy(RustDisintegrationLooper* ptr);
void n8_rust_disintegration_looper_prepare(RustDisintegrationLooper* ptr, float sample_rate);
void n8_rust_disintegration_looper_reset(RustDisintegrationLooper* ptr);
void n8_rust_disintegration_looper_set_param(RustDisintegrationLooper* ptr, uint32_t param_id, float value);
void n8_rust_disintegration_looper_process(
    RustDisintegrationLooper* ptr,
    const float* in_l,
    const float* in_r,
    float* out_l,
    float* out_r,
    uint32_t num_samples
);

// --- 19. Helical Spring Reverb Tank ---
struct RustSpringReverbTank;
RustSpringReverbTank* n8_rust_spring_reverb_tank_create(float sample_rate);
void n8_rust_spring_reverb_tank_destroy(RustSpringReverbTank* ptr);
void n8_rust_spring_reverb_tank_prepare(RustSpringReverbTank* ptr, float sample_rate);
void n8_rust_spring_reverb_tank_reset(RustSpringReverbTank* ptr);
void n8_rust_spring_reverb_tank_set_param(RustSpringReverbTank* ptr, uint32_t param_id, float value);
void n8_rust_spring_reverb_tank_process(
    RustSpringReverbTank* ptr,
    const float* in_l,
    const float* in_r,
    float* out_l,
    float* out_r,
    uint32_t num_samples
);

// --- 20. Dual Bode Frequency Shifter ---
struct RustDualBodeFrequencyShifter;
RustDualBodeFrequencyShifter* n8_rust_dual_bode_frequency_shifter_create(float sample_rate);
void n8_rust_dual_bode_frequency_shifter_destroy(RustDualBodeFrequencyShifter* ptr);
void n8_rust_dual_bode_frequency_shifter_prepare(RustDualBodeFrequencyShifter* ptr, float sample_rate);
void n8_rust_dual_bode_frequency_shifter_reset(RustDualBodeFrequencyShifter* ptr);
void n8_rust_dual_bode_frequency_shifter_set_param(RustDualBodeFrequencyShifter* ptr, uint32_t param_id, float value);
void n8_rust_dual_bode_frequency_shifter_process(
    RustDualBodeFrequencyShifter* ptr,
    const float* in_l,
    const float* in_r,
    float* out_l,
    float* out_r,
    uint32_t num_samples
);

// --- 21. Microtonal Harmonizer ---
struct RustMicrotonalHarmonizer;
RustMicrotonalHarmonizer* n8_rust_microtonal_harmonizer_create(float sample_rate);
void n8_rust_microtonal_harmonizer_destroy(RustMicrotonalHarmonizer* ptr);
void n8_rust_microtonal_harmonizer_prepare(RustMicrotonalHarmonizer* ptr, float sample_rate);
void n8_rust_microtonal_harmonizer_reset(RustMicrotonalHarmonizer* ptr);
void n8_rust_microtonal_harmonizer_set_param(RustMicrotonalHarmonizer* ptr, uint32_t param_id, float value);
void n8_rust_microtonal_harmonizer_process(
    RustMicrotonalHarmonizer* ptr,
    const float* in_l,
    const float* in_r,
    float* out_l,
    float* out_r,
    uint32_t num_samples
);

// --- 22. Formant Pitch Shifter ---
struct RustFormantPitchShifter;
RustFormantPitchShifter* n8_rust_formant_pitch_shifter_create(float sample_rate);
void n8_rust_formant_pitch_shifter_destroy(RustFormantPitchShifter* ptr);
void n8_rust_formant_pitch_shifter_prepare(RustFormantPitchShifter* ptr, float sample_rate);
void n8_rust_formant_pitch_shifter_reset(RustFormantPitchShifter* ptr);
void n8_rust_formant_pitch_shifter_set_param(RustFormantPitchShifter* ptr, uint32_t param_id, float value);
void n8_rust_formant_pitch_shifter_process(
    RustFormantPitchShifter* ptr,
    const float* in_l,
    const float* in_r,
    float* out_l,
    float* out_r,
    uint32_t num_samples
);

// --- 23. Spectral Resonance Tamer ---
struct RustSpectralResonanceTamer;
RustSpectralResonanceTamer* n8_rust_spectral_resonance_tamer_create(float sample_rate);
void n8_rust_spectral_resonance_tamer_destroy(RustSpectralResonanceTamer* ptr);
void n8_rust_spectral_resonance_tamer_prepare(RustSpectralResonanceTamer* ptr, float sample_rate);
void n8_rust_spectral_resonance_tamer_reset(RustSpectralResonanceTamer* ptr);
void n8_rust_spectral_resonance_tamer_set_param(RustSpectralResonanceTamer* ptr, uint32_t param_id, float value);
void n8_rust_spectral_resonance_tamer_process(
    RustSpectralResonanceTamer* ptr,
    const float* in_l,
    const float* in_r,
    float* out_l,
    float* out_r,
    uint32_t num_samples
);

// --- 24. Spectral Morphing Convolver ---
struct RustSpectralMorphingConvolver;
RustSpectralMorphingConvolver* n8_rust_spectral_morphing_convolver_create(float sample_rate);
void n8_rust_spectral_morphing_convolver_destroy(RustSpectralMorphingConvolver* ptr);
void n8_rust_spectral_morphing_convolver_prepare(RustSpectralMorphingConvolver* ptr, float sample_rate);
void n8_rust_spectral_morphing_convolver_reset(RustSpectralMorphingConvolver* ptr);
void n8_rust_spectral_morphing_convolver_set_param(RustSpectralMorphingConvolver* ptr, uint32_t param_id, float value);
void n8_rust_spectral_morphing_convolver_process(
    RustSpectralMorphingConvolver* ptr,
    const float* in_l,
    const float* in_r,
    float* out_l,
    float* out_r,
    uint32_t num_samples
);

// =========================================================================
// Graph Undo / Redo Manager C-ABI (Rules 21, 22, 47)
// =========================================================================
struct RustGraphUndoManager;

RustGraphUndoManager* n8_rust_graph_undo_manager_create(size_t max_history_steps);
void n8_rust_graph_undo_manager_destroy(RustGraphUndoManager* ptr);
void n8_rust_graph_undo_manager_push_action(
    RustGraphUndoManager* ptr,
    const char* desc,
    uint8_t category,
    const char* serialized_state,
    int64_t timestamp_ms
);
bool n8_rust_graph_undo_manager_can_undo(const RustGraphUndoManager* ptr);
bool n8_rust_graph_undo_manager_can_redo(const RustGraphUndoManager* ptr);
bool n8_rust_graph_undo_manager_undo(RustGraphUndoManager* ptr, char* out_buf, size_t max_buf_len);
bool n8_rust_graph_undo_manager_redo(RustGraphUndoManager* ptr, char* out_buf, size_t max_buf_len);
bool n8_rust_graph_undo_manager_jump_to_step(RustGraphUndoManager* ptr, size_t target_index, char* out_buf, size_t max_buf_len);
size_t n8_rust_graph_undo_manager_get_current_index(const RustGraphUndoManager* ptr);
size_t n8_rust_graph_undo_manager_get_history_size(const RustGraphUndoManager* ptr);
size_t n8_rust_graph_undo_manager_get_state_len(const RustGraphUndoManager* ptr, size_t index);
size_t n8_rust_graph_undo_manager_get_current_state_len(const RustGraphUndoManager* ptr);
bool n8_rust_graph_undo_manager_get_checkpoint_info(
    const RustGraphUndoManager* ptr,
    size_t index,
    char* out_desc,
    size_t desc_len,
    uint8_t* out_category,
    int64_t* out_timestamp,
    size_t* out_state_len
);
bool n8_rust_graph_undo_manager_get_checkpoint_state(
    const RustGraphUndoManager* ptr,
    size_t index,
    char* out_buf,
    size_t max_buf_len
);
void n8_rust_graph_undo_manager_get_next_undo_description(const RustGraphUndoManager* ptr, char* out_buf, size_t max_buf_len);
void n8_rust_graph_undo_manager_get_next_redo_description(const RustGraphUndoManager* ptr, char* out_buf, size_t max_buf_len);
void n8_rust_graph_undo_manager_clear(RustGraphUndoManager* ptr);

// =========================================================================
// User Preset Bank C-ABI (Rules 21, 27, 47)
// =========================================================================
struct RustUserPresetBank;

RustUserPresetBank* n8_rust_user_preset_bank_create();
void n8_rust_user_preset_bank_destroy(RustUserPresetBank* ptr);
void n8_rust_user_preset_bank_clear(RustUserPresetBank* ptr);
void n8_rust_user_preset_bank_set_directory(RustUserPresetBank* ptr, const char* dir_path);
void n8_rust_user_preset_bank_add_preset(
    RustUserPresetBank* ptr,
    const char* file_path,
    const char* name,
    const char* category,
    const char* author,
    const char* desc,
    const char* version,
    const char* tags_csv,
    bool favorite,
    const char* json_content
);
void n8_rust_user_preset_bank_sort(RustUserPresetBank* ptr);
size_t n8_rust_user_preset_bank_get_count(const RustUserPresetBank* ptr);
bool n8_rust_user_preset_bank_delete(RustUserPresetBank* ptr, size_t index);
bool n8_rust_user_preset_bank_toggle_favorite(RustUserPresetBank* ptr, size_t index);
bool n8_rust_user_preset_bank_get_preset_info(
    const RustUserPresetBank* ptr,
    size_t index,
    char* out_path,
    size_t path_len,
    char* out_name,
    size_t name_len,
    char* out_cat,
    size_t cat_len,
    char* out_author,
    size_t author_len,
    char* out_desc,
    size_t desc_len,
    char* out_tags,
    size_t tags_len,
    bool* out_fav,
    size_t* out_json_len
);
bool n8_rust_user_preset_bank_get_preset_json(
    const RustUserPresetBank* ptr,
    size_t index,
    char* out_json,
    size_t max_len
);
bool n8_rust_user_preset_bank_update_preset_json(
    RustUserPresetBank* ptr,
    size_t index,
    const char* json_content
);
size_t n8_rust_user_preset_bank_filter(
    const RustUserPresetBank* ptr,
    const char* query,
    const char* category,
    const char* tags_csv,
    bool favorites_only,
    size_t* out_indices,
    size_t max_indices
);
size_t n8_rust_user_preset_bank_get_tags_csv(
    const RustUserPresetBank* ptr,
    char* out_buf,
    size_t max_len
);
size_t n8_rust_user_preset_bank_get_categories_csv(
    const RustUserPresetBank* ptr,
    char* out_buf,
    size_t max_len
);

// =========================================================================
// Preset Packager C-ABI (Rules 21, 27, 47)
// =========================================================================
struct RustPackBuilder;
struct RustPackExtractor;

RustPackBuilder* n8_rust_pack_builder_create(
    const char* format,
    const char* pack_name,
    const char* author,
    const char* description,
    const char* version,
    const char* created_at,
    const char* tags_csv
);
void n8_rust_pack_builder_destroy(RustPackBuilder* ptr);
void n8_rust_pack_builder_add_preset(
    RustPackBuilder* ptr,
    const char* name,
    const char* category,
    const char* author,
    const char* content
);
size_t n8_rust_pack_builder_serialize(
    const RustPackBuilder* ptr,
    char* out_buf,
    size_t max_buf_len
);
bool n8_rust_preset_packager_inspect_manifest(
    const char* content,
    char* out_format,
    size_t format_len,
    uint32_t* out_pack_version,
    char* out_name,
    size_t name_len,
    char* out_author,
    size_t author_len,
    char* out_desc,
    size_t desc_len,
    char* out_ver,
    size_t ver_len,
    char* out_created,
    size_t created_len,
    size_t* out_count,
    char* out_tags,
    size_t tags_len,
    char* out_err,
    size_t err_len
);
RustPackExtractor* n8_rust_pack_extractor_create(const char* content);
void n8_rust_pack_extractor_destroy(RustPackExtractor* ptr);
size_t n8_rust_pack_extractor_get_count(const RustPackExtractor* ptr);
bool n8_rust_pack_extractor_get_preset_info(
    const RustPackExtractor* ptr,
    size_t index,
    char* out_name,
    size_t name_len,
    char* out_cat,
    size_t cat_len,
    char* out_author,
    size_t author_len,
    size_t* out_content_len
);
bool n8_rust_pack_extractor_get_preset_content(
    const RustPackExtractor* ptr,
    size_t index,
    char* out_buf,
    size_t max_len
);

// =========================================================================
// Node Automation Sequencer C-ABI (Rules 7, 8, 9, 25, 37, 46, 47)
// =========================================================================
struct RustNodeAutomationBank;

RustNodeAutomationBank* n8_rust_node_automation_bank_create();
void n8_rust_node_automation_bank_destroy(RustNodeAutomationBank* ptr);
void n8_rust_node_automation_bank_prepare(RustNodeAutomationBank* ptr, double sample_rate);
void n8_rust_node_automation_bank_reset(RustNodeAutomationBank* ptr);
void n8_rust_node_automation_bank_apply_shape(RustNodeAutomationBank* ptr, size_t lane_idx, uint8_t shape);
void n8_rust_node_automation_bank_set_lane_config(
    RustNodeAutomationBank* ptr,
    size_t lane_idx,
    bool active,
    uint32_t target_param_id,
    uint32_t num_steps,
    uint8_t rate,
    float glide,
    float amount
);
void n8_rust_node_automation_bank_get_lane_config(
    const RustNodeAutomationBank* ptr,
    size_t lane_idx,
    bool* out_active,
    uint32_t* out_target_param_id,
    uint32_t* out_num_steps,
    uint8_t* out_rate,
    float* out_glide,
    float* out_amount,
    uint32_t* out_current_step,
    float* out_live_val
);
void n8_rust_node_automation_bank_set_step(RustNodeAutomationBank* ptr, size_t lane_idx, size_t step_idx, float val);
float n8_rust_node_automation_bank_get_step(const RustNodeAutomationBank* ptr, size_t lane_idx, size_t step_idx);
bool n8_rust_node_automation_bank_process_step(
    RustNodeAutomationBank* ptr,
    size_t lane_idx,
    bool is_playing,
    double ppq_position,
    float base_val,
    float min_val,
    float max_val,
    float* out_mod_val
);

// =========================================================================
// Midi Mapping Manager C-ABI (Rules 8, 9, 21, 46, 47)
// =========================================================================
struct RustMidiMappingManager;

RustMidiMappingManager* n8_rust_midi_mapping_manager_create();
void n8_rust_midi_mapping_manager_destroy(RustMidiMappingManager* ptr);
void n8_rust_midi_mapping_manager_reset(RustMidiMappingManager* ptr);
void n8_rust_midi_mapping_manager_start_learning(
    RustMidiMappingManager* ptr,
    uint32_t param_id,
    uint32_t target_node_id,
    float min_val,
    float max_val,
    uint8_t curve,
    bool inverted
);
void n8_rust_midi_mapping_manager_cancel_learning(RustMidiMappingManager* ptr);
bool n8_rust_midi_mapping_manager_is_learning(const RustMidiMappingManager* ptr);
void n8_rust_midi_mapping_manager_get_learn_target(
    const RustMidiMappingManager* ptr,
    uint32_t* out_param_id,
    uint32_t* out_target_node_id,
    float* out_min_val,
    float* out_max_val,
    uint8_t* out_curve,
    bool* out_inverted
);
bool n8_rust_midi_mapping_manager_add_mapping(
    RustMidiMappingManager* ptr,
    uint8_t cc_number,
    uint8_t channel,
    uint32_t param_id,
    uint32_t target_node_id,
    float min_val,
    float max_val,
    bool inverted,
    uint8_t curve,
    const char* custom_label
);
bool n8_rust_midi_mapping_manager_remove_mapping(
    RustMidiMappingManager* ptr,
    uint32_t param_id,
    uint32_t target_node_id
);
void n8_rust_midi_mapping_manager_remove_mapping_by_cc(
    RustMidiMappingManager* ptr,
    uint8_t cc_number,
    uint8_t channel
);
size_t n8_rust_midi_mapping_manager_get_count(const RustMidiMappingManager* ptr);
bool n8_rust_midi_mapping_manager_get_mapping(
    const RustMidiMappingManager* ptr,
    size_t index,
    uint8_t* out_cc,
    uint8_t* out_channel,
    uint32_t* out_param_id,
    uint32_t* out_target_node_id,
    float* out_min_val,
    float* out_max_val,
    bool* out_inverted,
    uint8_t* out_curve,
    char* out_label,
    size_t label_max_len
);
float n8_rust_midi_mapping_manager_calculate_mapped_value(
    float min_val,
    float max_val,
    bool inverted,
    uint8_t curve,
    uint8_t cc_value
);
size_t n8_rust_midi_mapping_manager_process_control_change(
    RustMidiMappingManager* ptr,
    uint8_t channel,
    uint8_t cc_number,
    uint8_t cc_value,
    uint32_t* out_targets,
    uint32_t* out_param_ids,
    float* out_values,
    size_t max_dispatches
);
void n8_rust_midi_mapping_manager_load_factory_profile(
    RustMidiMappingManager* ptr,
    const char* profile_name
);
size_t n8_rust_midi_mapping_manager_export_json(
    const RustMidiMappingManager* ptr,
    const char* profile_name,
    char* out_buf,
    size_t max_len
);
bool n8_rust_midi_mapping_manager_import_json(
    RustMidiMappingManager* ptr,
    const char* json_str
);

// =========================================================================
// MPE Manager C-ABI (Rules 2, 3, 5, 8, 9, 14, 34, 46, 47)
// =========================================================================
struct RustMpeManager;

RustMpeManager* n8_rust_mpe_manager_create();
void n8_rust_mpe_manager_destroy(RustMpeManager* ptr);
void n8_rust_mpe_manager_prepare(RustMpeManager* ptr, double sample_rate);
void n8_rust_mpe_manager_reset(RustMpeManager* ptr);
void n8_rust_mpe_manager_set_zone_mode(RustMpeManager* ptr, uint8_t mode);
uint8_t n8_rust_mpe_manager_get_zone_mode(const RustMpeManager* ptr);
void n8_rust_mpe_manager_set_per_note_pitch_bend_range(RustMpeManager* ptr, float semitones);
float n8_rust_mpe_manager_get_per_note_pitch_bend_range(const RustMpeManager* ptr);
void n8_rust_mpe_manager_set_master_pitch_bend_range(RustMpeManager* ptr, float semitones);
float n8_rust_mpe_manager_get_master_pitch_bend_range(const RustMpeManager* ptr);
size_t n8_rust_mpe_manager_get_active_voice_count(const RustMpeManager* ptr);
bool n8_rust_mpe_manager_get_voice_info(
    const RustMpeManager* ptr,
    size_t channel,
    bool* out_is_active,
    uint8_t* out_note_number,
    float* out_strike_velocity,
    float* out_press,
    float* out_per_note_glide,
    float* out_master_glide,
    float* out_slide,
    float* out_lift_velocity,
    float* out_base_freq,
    float* out_current_freq,
    float* out_pitch_ratio,
    uint32_t* out_associated_event_id
);
void n8_rust_mpe_manager_set_associated_event_id(
    RustMpeManager* ptr,
    size_t channel,
    uint32_t event_id
);
bool n8_rust_mpe_manager_process_midi_message(
    RustMpeManager* ptr,
    uint8_t status,
    uint8_t data1,
    uint8_t data2,
    uint8_t* out_action_type,
    uint8_t* out_channel,
    uint8_t* out_note,
    float* out_pitch_ratio,
    float* out_strike_velocity,
    float* out_pan,
    uint64_t* out_max_duration
);
float n8_rust_mpe_manager_midi_note_to_frequency(uint8_t note);

// =========================================================================
// Preset Manager C-ABI (Rules 21, 22, 27, 46, 47)
// =========================================================================
struct RustPresetManager;

RustPresetManager* n8_rust_preset_manager_create();
void n8_rust_preset_manager_destroy(RustPresetManager* ptr);
void n8_rust_preset_manager_reset(RustPresetManager* ptr);
void n8_rust_preset_manager_store_slot_a(
    RustPresetManager* ptr,
    const char* name,
    const char* json,
    const float* macros,
    uint64_t timestamp
);
void n8_rust_preset_manager_store_slot_b(
    RustPresetManager* ptr,
    const char* name,
    const char* json,
    const float* macros,
    uint64_t timestamp
);
bool n8_rust_preset_manager_get_slot_a(
    const RustPresetManager* ptr,
    char* out_name,
    size_t name_max_len,
    char* out_json,
    size_t json_max_len,
    float* out_macros
);
bool n8_rust_preset_manager_get_slot_b(
    const RustPresetManager* ptr,
    char* out_name,
    size_t name_max_len,
    char* out_json,
    size_t json_max_len,
    float* out_macros
);
void n8_rust_preset_manager_set_active_slot(RustPresetManager* ptr, uint8_t slot);
uint8_t n8_rust_preset_manager_get_active_slot(const RustPresetManager* ptr);
void n8_rust_preset_manager_copy_a_to_b(RustPresetManager* ptr);
void n8_rust_preset_manager_copy_b_to_a(RustPresetManager* ptr);
void n8_rust_preset_manager_swap_slots(RustPresetManager* ptr);
void n8_rust_preset_manager_set_morph_factor(RustPresetManager* ptr, float factor);
float n8_rust_preset_manager_get_morph_factor(const RustPresetManager* ptr);
void n8_rust_preset_manager_get_interpolated_macros(const RustPresetManager* ptr, float* out_macros);
void n8_rust_preset_manager_take_snapshot(
    RustPresetManager* ptr,
    const char* name,
    const char* json,
    const float* macros,
    uint64_t timestamp
);
size_t n8_rust_preset_manager_get_snapshot_count(const RustPresetManager* ptr);
bool n8_rust_preset_manager_get_snapshot(
    const RustPresetManager* ptr,
    size_t index,
    char* out_name,
    size_t name_max_len,
    char* out_json,
    size_t json_max_len,
    float* out_macros
);
void n8_rust_preset_manager_save_user_preset(
    RustPresetManager* ptr,
    const char* name,
    const char* json,
    const float* macros
);
bool n8_rust_preset_manager_load_user_preset(
    const RustPresetManager* ptr,
    const char* name,
    char* out_json,
    size_t json_max_len,
    float* out_macros
);
size_t n8_rust_preset_manager_get_user_preset_count(const RustPresetManager* ptr);
bool n8_rust_preset_manager_get_user_preset_name(
    const RustPresetManager* ptr,
    size_t index,
    char* out_name,
    size_t name_max_len
);

// =========================================================================
// Graph Serializer C-ABI (Rules 21, 22, 28, 46, 47)
// =========================================================================
size_t n8_rust_graph_serializer_escape(
    const char* src,
    char* out_buf,
    size_t max_len
);
size_t n8_rust_graph_serializer_extract_string(
    const char* json,
    const char* key,
    const char* def_val,
    char* out_buf,
    size_t max_len
);
uint32_t n8_rust_graph_serializer_extract_uint(
    const char* json,
    const char* key,
    uint32_t def_val
);
float n8_rust_graph_serializer_extract_float(
    const char* json,
    const char* key,
    float def_val
);
size_t n8_rust_graph_serializer_extract_array_section(
    const char* json,
    const char* key,
    char* out_buf,
    size_t max_len
);
size_t n8_rust_graph_serializer_extract_object_section(
    const char* json,
    const char* key,
    char* out_buf,
    size_t max_len
);
size_t n8_rust_graph_serializer_parse_number_list(
    const char* text,
    float* out_buf,
    size_t max_len
);

// =========================================================================
// Factory Preset Catalog C-ABI (Rules 4, 19, 20, 21, 22, 46)
// =========================================================================
struct RustFactoryPresetCatalog;

RustFactoryPresetCatalog* n8_rust_factory_preset_catalog_create();
void n8_rust_factory_preset_catalog_destroy(RustFactoryPresetCatalog* ptr);
void n8_rust_factory_preset_catalog_reset(RustFactoryPresetCatalog* ptr);
void n8_rust_factory_preset_catalog_register_preset(
    RustFactoryPresetCatalog* ptr,
    const char* name,
    const char* category,
    const char* json
);
size_t n8_rust_factory_preset_catalog_get_count(const RustFactoryPresetCatalog* ptr);
bool n8_rust_factory_preset_catalog_get_preset_name(
    const RustFactoryPresetCatalog* ptr,
    size_t index,
    char* out_buf,
    size_t max_len
);
bool n8_rust_factory_preset_catalog_get_preset_category(
    const RustFactoryPresetCatalog* ptr,
    size_t index,
    char* out_buf,
    size_t max_len
);
size_t n8_rust_factory_preset_catalog_get_preset_json(
    const RustFactoryPresetCatalog* ptr,
    size_t index,
    char* out_buf,
    size_t max_len
);
size_t n8_rust_factory_preset_catalog_get_category_count(const RustFactoryPresetCatalog* ptr);
bool n8_rust_factory_preset_catalog_get_category_name(
    const RustFactoryPresetCatalog* ptr,
    size_t index,
    char* out_buf,
    size_t max_len
);
size_t n8_rust_factory_preset_catalog_search(
    const RustFactoryPresetCatalog* ptr,
    const char* query,
    size_t* out_indices,
    size_t max_results
);
size_t n8_rust_factory_preset_catalog_filter_by_category(
    const RustFactoryPresetCatalog* ptr,
    const char* category,
    size_t* out_indices,
    size_t max_results
);

// =========================================================================
// Work-Stealing Queue C-ABI (Rules 4, 9, 10, 26, 47)
// =========================================================================
struct RustWorkStealingQueue;

RustWorkStealingQueue* n8_rust_work_stealing_queue_create(uint32_t capacity);
void n8_rust_work_stealing_queue_destroy(RustWorkStealingQueue* ptr);
bool n8_rust_work_stealing_queue_push(RustWorkStealingQueue* ptr, uint32_t item);
bool n8_rust_work_stealing_queue_pop(RustWorkStealingQueue* ptr, uint32_t* out_item);
bool n8_rust_work_stealing_queue_steal(RustWorkStealingQueue* ptr, uint32_t* out_item);
bool n8_rust_work_stealing_queue_empty(const RustWorkStealingQueue* ptr);
void n8_rust_work_stealing_queue_clear(RustWorkStealingQueue* ptr);

// =========================================================================
// Work-Stealing Graph Scheduler C-ABI (Rules 4, 9, 10, 26, 47)
// =========================================================================
struct RustWorkStealingScheduler;

RustWorkStealingScheduler* n8_rust_work_stealing_scheduler_create();
void n8_rust_work_stealing_scheduler_destroy(RustWorkStealingScheduler* ptr);
void n8_rust_work_stealing_scheduler_prepare(RustWorkStealingScheduler* ptr, uint32_t num_workers);
void n8_rust_work_stealing_scheduler_clear_queues(RustWorkStealingScheduler* ptr);
void n8_rust_work_stealing_scheduler_init_step(
    RustWorkStealingScheduler* ptr,
    size_t step_idx,
    int32_t dep_count,
    const uint32_t* successors,
    size_t num_successors
);
void n8_rust_work_stealing_scheduler_start_execution(
    RustWorkStealingScheduler* ptr,
    size_t total_steps
);
bool n8_rust_work_stealing_scheduler_pop_or_steal(
    RustWorkStealingScheduler* ptr,
    uint32_t worker_id,
    uint32_t* out_step_idx
);
void n8_rust_work_stealing_scheduler_finish_step(
    RustWorkStealingScheduler* ptr,
    uint32_t worker_id,
    uint32_t step_idx
);
int32_t n8_rust_work_stealing_scheduler_get_steps_remaining(const RustWorkStealingScheduler* ptr);
uint32_t n8_rust_work_stealing_scheduler_get_num_workers(const RustWorkStealingScheduler* ptr);
uint64_t n8_rust_work_stealing_scheduler_increment_epoch(RustWorkStealingScheduler* ptr);
uint64_t n8_rust_work_stealing_scheduler_get_epoch(const RustWorkStealingScheduler* ptr);

// =========================================================================
// Adaptive Noise Floor Estimator C-ABI (Rules 1, 3, 9, 27, 47)
// =========================================================================
struct RustAdaptiveNoiseFloorEstimator;

RustAdaptiveNoiseFloorEstimator* n8_rust_adaptive_noise_floor_estimator_create();
void n8_rust_adaptive_noise_floor_estimator_destroy(RustAdaptiveNoiseFloorEstimator* ptr);
void n8_rust_adaptive_noise_floor_estimator_prepare(RustAdaptiveNoiseFloorEstimator* ptr, double sample_rate);
void n8_rust_adaptive_noise_floor_estimator_reset(RustAdaptiveNoiseFloorEstimator* ptr);
void n8_rust_adaptive_noise_floor_estimator_update_block(
    RustAdaptiveNoiseFloorEstimator* ptr,
    float block_rms,
    uint32_t num_samples
);
bool n8_rust_adaptive_noise_floor_estimator_is_source_active(const RustAdaptiveNoiseFloorEstimator* ptr);
bool n8_rust_adaptive_noise_floor_estimator_is_source_silent(const RustAdaptiveNoiseFloorEstimator* ptr);
float n8_rust_adaptive_noise_floor_estimator_get_noise_floor(const RustAdaptiveNoiseFloorEstimator* ptr);
float n8_rust_adaptive_noise_floor_estimator_get_smoothed_level(const RustAdaptiveNoiseFloorEstimator* ptr);
void n8_rust_adaptive_noise_floor_estimator_set_debounce_blocks(
    RustAdaptiveNoiseFloorEstimator* ptr,
    uint32_t blocks
);

// =========================================================================
// Delay Line C-ABI (Rules 11, 12, 32, 34, 46, 47)
// =========================================================================
struct RustDelayLine;

RustDelayLine* n8_rust_delay_line_create();
void n8_rust_delay_line_destroy(RustDelayLine* ptr);
void n8_rust_delay_line_prepare(RustDelayLine* ptr, size_t max_delay);
void n8_rust_delay_line_reset(RustDelayLine* ptr);
void n8_rust_delay_line_write(RustDelayLine* ptr, float sample);
float n8_rust_delay_line_read_linear(const RustDelayLine* ptr, float delay);
float n8_rust_delay_line_read_cubic(const RustDelayLine* ptr, float delay);
size_t n8_rust_delay_line_get_capacity(const RustDelayLine* ptr);
size_t n8_rust_delay_line_get_mask(const RustDelayLine* ptr);
size_t n8_rust_delay_line_get_max_delay_samples(const RustDelayLine* ptr);

// =========================================================================
// Linkwitz-Riley LR4 3-Way Crossover C-ABI (Rules 32, 33, 34, 46, 47)
// =========================================================================
struct RustLinkwitzRileyCrossover3Way;

RustLinkwitzRileyCrossover3Way* n8_rust_lr_crossover_create();
void n8_rust_lr_crossover_destroy(RustLinkwitzRileyCrossover3Way* ptr);
void n8_rust_lr_crossover_prepare(RustLinkwitzRileyCrossover3Way* ptr, double sample_rate);
void n8_rust_lr_crossover_reset(RustLinkwitzRileyCrossover3Way* ptr);
void n8_rust_lr_crossover_set_crossover_frequencies(
    RustLinkwitzRileyCrossover3Way* ptr,
    float f_low_mid,
    float f_mid_high
);
void n8_rust_lr_crossover_process_sample(
    RustLinkwitzRileyCrossover3Way* ptr,
    size_t ch,
    float in_sample,
    float* out_low,
    float* out_mid,
    float* out_high
);
void n8_rust_lr_crossover_process_block(
    RustLinkwitzRileyCrossover3Way* ptr,
    const float* in_l,
    const float* in_r,
    float* low_l,
    float* low_r,
    float* mid_l,
    float* mid_r,
    float* high_l,
    float* high_r,
    size_t num_samples
);

// Tipo opaco para el pool de buffers de audio alineado a 64 bytes en Rust
struct RustAudioBufferPool;

RustAudioBufferPool* n8_rust_buffer_pool_create(size_t capacity, size_t max_block_size);
void n8_rust_buffer_pool_destroy(RustAudioBufferPool* ptr);
void n8_rust_buffer_pool_prepare(RustAudioBufferPool* ptr, size_t max_block_size);
void n8_rust_buffer_pool_reset(RustAudioBufferPool* ptr);
bool n8_rust_buffer_pool_acquire(
    RustAudioBufferPool* ptr,
    size_t* out_slot,
    float** out_left,
    float** out_right
);
void n8_rust_buffer_pool_release(RustAudioBufferPool* ptr, size_t slot);
void n8_rust_buffer_pool_release_all(RustAudioBufferPool* ptr);
size_t n8_rust_buffer_pool_available_count(const RustAudioBufferPool* ptr);

struct RustProcessorRegistry;
RustProcessorRegistry* n8_rust_processor_registry_create(void);
void n8_rust_processor_registry_destroy(RustProcessorRegistry* ptr);
const char* n8_rust_processor_registry_get_name(const RustProcessorRegistry* ptr, uint32_t type_id);

struct RustDualWorldEngine;
RustDualWorldEngine* n8_rust_dual_world_create(double sample_rate, size_t max_block_size);
void n8_rust_dual_world_destroy(RustDualWorldEngine* ptr);
void n8_rust_dual_world_prepare(RustDualWorldEngine* ptr, double sample_rate, size_t max_block_size);
void n8_rust_dual_world_reset(RustDualWorldEngine* ptr);
void n8_rust_dual_world_set_dry_level(RustDualWorldEngine* ptr, float level);
float n8_rust_dual_world_get_dry_level(const RustDualWorldEngine* ptr);
void n8_rust_dual_world_set_wet_level(RustDualWorldEngine* ptr, float level);
float n8_rust_dual_world_get_wet_level(const RustDualWorldEngine* ptr);
void n8_rust_dual_world_set_editor_active(RustDualWorldEngine* ptr, bool active);
bool n8_rust_dual_world_add_node(RustDualWorldEngine* ptr, uint32_t node_id, uint32_t type_id);
bool n8_rust_dual_world_remove_node(RustDualWorldEngine* ptr, uint32_t node_id);
void n8_rust_dual_world_set_node_bypassed(RustDualWorldEngine* ptr, uint32_t node_id, bool bypassed);
void n8_rust_dual_world_set_node_param(RustDualWorldEngine* ptr, uint32_t node_id, uint32_t param_id, float value);
float n8_rust_dual_world_get_node_param(const RustDualWorldEngine* ptr, uint32_t node_id, uint32_t param_id);
bool n8_rust_dual_world_update_graph(
    RustDualWorldEngine* ptr,
    const uint32_t* node_ids,
    uint32_t num_nodes,
    const RustGraphConnection* connections,
    uint32_t num_connections,
    const bool* bypassed_flags,
    uint32_t num_bypassed
);
void n8_rust_dual_world_process(
    RustDualWorldEngine* ptr,
    const float* in_l,
    const float* in_r,
    float* out_l,
    float* out_r,
    uint32_t num_samples
);

#ifdef __cplusplus
}
#endif










