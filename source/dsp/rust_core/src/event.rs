//! Audio Event Entity & Lifecycle Machine (Roadmap Phase 2, Rules 2, 3, 18, 46, 47)
//! Individual event rendering, source-following, 3D/stereo panning, and anti-click envelopes.

use std::f32::consts::PI;
use crate::event_types::{EventLifecycle, RustEventAttributes, SourceDisappearanceMode};
use crate::event_capture_buffer::RustEventCaptureBuffer;
use crate::audio_fragment::RustAudioFragment;
use crate::spatial_panner::RustSpatialPanner;
use crate::common::denormal_guard;

pub struct RustEvent {
    attrs: RustEventAttributes,
    fragment: RustAudioFragment,
    spatial_panner: RustSpatialPanner,
    current_envelope: f32,
    is_active: bool,
}

impl RustEvent {
    pub fn new() -> Self {
        Self {
            attrs: RustEventAttributes::default(),
            fragment: RustAudioFragment::new(),
            spatial_panner: RustSpatialPanner::new(44100.0),
            current_envelope: 0.0,
            is_active: false,
        }
    }

    pub fn initialize(&mut self, mut attrs: RustEventAttributes, capture_offset: f64, duration_samples: u64, reverse: bool) {
        attrs.state = EventLifecycle::Created as u8;
        attrs.age_samples = 0;
        attrs.lifetime_samples = duration_samples;

        self.fragment.configure(capture_offset, duration_samples, attrs.pitch_ratio, reverse);
        self.spatial_panner.prepare(44100.0);
        self.spatial_panner.set_coordinates(attrs.azimuth, attrs.elevation, attrs.distance);
        self.current_envelope = 0.0;
        self.is_active = true;
        self.attrs = attrs;
    }

    pub fn start(&mut self) {
        if self.attrs.state == EventLifecycle::Created as u8 {
            self.attrs.state = EventLifecycle::Playing as u8;
        }
    }

    pub fn kill(&mut self) {
        self.attrs.state = EventLifecycle::Killed as u8;
        self.is_active = false;
        self.current_envelope = 0.0;
    }

    pub fn release(&mut self) {
        let st = self.attrs.state;
        if st == EventLifecycle::Playing as u8 || st == EventLifecycle::Frozen as u8 || st == EventLifecycle::Looping as u8 {
            self.attrs.state = EventLifecycle::Releasing as u8;
        }
    }

    pub fn handle_source_disappearance(&mut self) {
        match SourceDisappearanceMode::from(self.attrs.disappearance_mode) {
            SourceDisappearanceMode::Cut => {
                self.attrs.state = EventLifecycle::Killed as u8;
                self.is_active = false;
            }
            SourceDisappearanceMode::Short => {
                if self.attrs.lifetime_samples > self.attrs.age_samples + 256 {
                    self.attrs.lifetime_samples = self.attrs.age_samples + 256;
                }
            }
            SourceDisappearanceMode::Fade => {
                if self.attrs.state == EventLifecycle::Playing as u8 {
                    self.attrs.state = EventLifecycle::Releasing as u8;
                }
            }
            SourceDisappearanceMode::Hold => {
                // Mantener nivel actual sin avanzar a liberación
            }
            SourceDisappearanceMode::Freeze => {
                self.attrs.state = EventLifecycle::Frozen as u8;
            }
            SourceDisappearanceMode::Natural => {
                // Continúa su ciclo natural (Regla 18: Tails independientes)
            }
        }
    }

    fn update_envelope(&mut self) {
        let attack_step = 1.0 / (self.attrs.attack_samples.max(1) as f32);
        let release_step = 1.0 / (self.attrs.release_samples.max(1) as f32);

        match EventLifecycle::from(self.attrs.state) {
            EventLifecycle::Created | EventLifecycle::Playing | EventLifecycle::Looping | EventLifecycle::Frozen => {
                if self.current_envelope < 1.0 {
                    self.current_envelope = (self.current_envelope + attack_step).min(1.0);
                }
            }
            EventLifecycle::Releasing => {
                if self.current_envelope > 0.0 {
                    self.current_envelope = (self.current_envelope - release_step).max(0.0);
                }
                if self.current_envelope <= 0.0001 {
                    self.attrs.state = EventLifecycle::Killed as u8;
                    self.is_active = false;
                }
            }
            EventLifecycle::Killed | EventLifecycle::Destroyed => {
                self.current_envelope = 0.0;
                self.is_active = false;
            }
        }
    }

    pub fn render(
        &mut self,
        capture: &RustEventCaptureBuffer,
        out_l: &mut [f32],
        out_r: &mut [f32],
        num_samples: usize,
        source_level: f32,
        source_silence_override: i32,
    ) {
        if !self.is_active || self.attrs.state == EventLifecycle::Killed as u8 || self.attrs.state == EventLifecycle::Destroyed as u8 {
            return;
        }

        // 1. Evaluar Source Following con histéresis adaptativa (Regla 3)
        let source_is_silent = if source_silence_override >= 0 {
            source_silence_override != 0
        } else {
            source_level < 1e-4
        };

        if source_is_silent && self.attrs.source_follow > 0.05 {
            self.handle_source_disappearance();
        }

        // 2. Factores de paneo de ley de potencia constante (-3dB pan law)
        let pan_clamped = self.attrs.pan.clamp(-1.0, 1.0);
        let pan_angle = (pan_clamped + 1.0) * 0.25 * PI;
        let pan_gain_l = pan_angle.cos() * self.attrs.gain;
        let pan_gain_r = pan_angle.sin() * self.attrs.gain;

        let has_3d = self.attrs.azimuth.abs() > 0.01 || self.attrs.elevation.abs() > 0.01 || (self.attrs.distance - 1.0).abs() > 0.01;

        let limit = num_samples.min(out_l.len()).min(out_r.len());

        for s in 0..limit {
            if self.attrs.state == EventLifecycle::Killed as u8 || self.attrs.state == EventLifecycle::Destroyed as u8 {
                self.is_active = false;
                break;
            }

            self.update_envelope();

            let (has_more, sample_l, sample_r) = self.fragment.get_next_sample(capture, self.current_envelope);

            if !has_more && self.attrs.state != EventLifecycle::Looping as u8 && self.attrs.state != EventLifecycle::Frozen as u8 {
                if self.attrs.state == EventLifecycle::Playing as u8 {
                    self.attrs.state = EventLifecycle::Releasing as u8;
                } else if self.attrs.state == EventLifecycle::Releasing as u8 && self.current_envelope <= 0.001 {
                    self.attrs.state = EventLifecycle::Killed as u8;
                    self.is_active = false;
                    break;
                }
            }

            let effective_gain = if self.attrs.source_follow > 0.0 {
                (1.0 - self.attrs.source_follow) + self.attrs.source_follow * (source_level * 2.0).clamp(0.0, 1.0)
            } else {
                1.0
            };

            if has_3d {
                let (spat_l, spat_r) = self.spatial_panner.process_sample_pair(sample_l, sample_r);
                out_l[s] += denormal_guard(spat_l * self.attrs.gain * effective_gain);
                out_r[s] += denormal_guard(spat_r * self.attrs.gain * effective_gain);
            } else {
                out_l[s] += denormal_guard(sample_l * pan_gain_l * effective_gain);
                out_r[s] += denormal_guard(sample_r * pan_gain_r * effective_gain);
            }

            self.attrs.age_samples += 1;
            if self.attrs.lifetime_samples > 0
                && self.attrs.age_samples >= self.attrs.lifetime_samples
                && self.attrs.state == EventLifecycle::Playing as u8
            {
                self.attrs.state = EventLifecycle::Releasing as u8;
            }
        }
    }

    #[inline]
    pub fn is_active(&self) -> bool {
        self.is_active
    }

    #[inline]
    pub fn get_attributes(&self) -> &RustEventAttributes {
        &self.attrs
    }

    #[inline]
    pub fn get_attributes_mut(&mut self) -> &mut RustEventAttributes {
        &mut self.attrs
    }

    #[inline]
    pub fn get_state(&self) -> EventLifecycle {
        EventLifecycle::from(self.attrs.state)
    }

    #[inline]
    pub fn set_state(&mut self, state: EventLifecycle) {
        self.attrs.state = state as u8;
    }
}
