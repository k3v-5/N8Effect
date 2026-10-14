//! MPE Manager & Polyphonic Event Voicing (Roadmap Phase 3, Rules 2, 3, 5, 8, 9, 14, 34, 46, 47)
//! 5D Expression tracking (Strike, Press, Glide, Slide, Lift) across 16 channels,
//! microtonal pitch calculation, and zero dynamic allocation in audio thread.

pub const MAX_MPE_CHANNELS: usize = 16;
pub const DEFAULT_PITCH_BEND_RANGE: f32 = 48.0;
pub const DEFAULT_MASTER_BEND_RANGE: f32 = 2.0;

#[repr(u8)]
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum RustZoneMode {
    LowerZone = 0,
    UpperZone = 1,
    OmniLegacy = 2,
}

#[repr(C)]
#[derive(Debug, Clone, Copy)]
pub struct RustMpeVoice {
    pub channel: u8,
    pub note_number: u8,
    pub is_active: bool,
    pub note_on_timestamp: u64,

    // 5D Expression Dimensions
    pub strike_velocity: f32,
    pub press: f32,
    pub per_note_glide_semitones: f32,
    pub master_glide_semitones: f32,
    pub slide: f32,
    pub lift_velocity: f32,

    // Frequency & Modulation
    pub base_frequency: f32,
    pub current_frequency: f32,
    pub pitch_ratio: f32,

    pub associated_event_id: u32,
}

impl Default for RustMpeVoice {
    fn default() -> Self {
        Self {
            channel: 0,
            note_number: 60,
            is_active: false,
            note_on_timestamp: 0,
            strike_velocity: 0.0,
            press: 0.0,
            per_note_glide_semitones: 0.0,
            master_glide_semitones: 0.0,
            slide: 0.0,
            lift_velocity: 0.0,
            base_frequency: 261.62555,
            current_frequency: 261.62555,
            pitch_ratio: 1.0,
            associated_event_id: u32::MAX, // InvalidEventId
        }
    }
}

#[repr(u8)]
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum RustMpeActionType {
    None = 0,
    SpawnEvent = 1,
    ReleaseEvent = 2,
}

#[repr(C)]
#[derive(Debug, Clone, Copy)]
pub struct RustMpeAction {
    pub action_type: RustMpeActionType,
    pub channel: u8,
    pub note: u8,
    pub pitch_ratio: f32,
    pub strike_velocity: f32,
    pub pan: f32,
    pub max_duration_samples: u64,
}

impl Default for RustMpeAction {
    fn default() -> Self {
        Self {
            action_type: RustMpeActionType::None,
            channel: 0,
            note: 60,
            pitch_ratio: 1.0,
            strike_velocity: 0.0,
            pan: 0.0,
            max_duration_samples: 0,
        }
    }
}

pub struct RustMpeManager {
    pub sample_rate: f64,
    pub zone_mode: RustZoneMode,
    pub per_note_bend_range: f32,
    pub master_bend_range: f32,
    pub master_bend_normalized: f32,
    pub active_voice_count: usize,
    pub voices: [RustMpeVoice; MAX_MPE_CHANNELS],
}

impl RustMpeManager {
    pub fn new() -> Self {
        let mut voices = [RustMpeVoice::default(); MAX_MPE_CHANNELS];
        for ch in 0..MAX_MPE_CHANNELS {
            voices[ch].channel = ch as u8;
        }

        Self {
            sample_rate: 48000.0,
            zone_mode: RustZoneMode::LowerZone,
            per_note_bend_range: DEFAULT_PITCH_BEND_RANGE,
            master_bend_range: DEFAULT_MASTER_BEND_RANGE,
            master_bend_normalized: 0.0,
            active_voice_count: 0,
            voices,
        }
    }

    pub fn prepare(&mut self, sample_rate: f64) {
        self.sample_rate = if sample_rate > 0.0 { sample_rate } else { 48000.0 };
        self.reset();
    }

    pub fn reset(&mut self) {
        for ch in 0..MAX_MPE_CHANNELS {
            let v = &mut self.voices[ch];
            v.channel = ch as u8;
            v.is_active = false;
            v.note_number = 60;
            v.strike_velocity = 0.0;
            v.press = 0.0;
            v.per_note_glide_semitones = 0.0;
            v.master_glide_semitones = 0.0;
            v.slide = 0.0;
            v.lift_velocity = 0.0;
            v.base_frequency = 261.62555;
            v.current_frequency = 261.62555;
            v.pitch_ratio = 1.0;
            v.associated_event_id = u32::MAX;
        }
        self.master_bend_normalized = 0.0;
        self.active_voice_count = 0;
    }

    pub fn set_zone_mode(&mut self, mode: u8) {
        self.zone_mode = match mode {
            1 => RustZoneMode::UpperZone,
            2 => RustZoneMode::OmniLegacy,
            _ => RustZoneMode::LowerZone,
        };
    }

    pub fn get_zone_mode(&self) -> u8 {
        self.zone_mode as u8
    }

    pub fn set_per_note_pitch_bend_range(&mut self, semitones: f32) {
        self.per_note_bend_range = semitones.clamp(1.0, 96.0);
    }

    pub fn get_per_note_pitch_bend_range(&self) -> f32 {
        self.per_note_bend_range
    }

    pub fn set_master_pitch_bend_range(&mut self, semitones: f32) {
        self.master_bend_range = semitones.clamp(0.0, 24.0);
    }

    pub fn get_master_pitch_bend_range(&self) -> f32 {
        self.master_bend_range
    }

    pub fn get_active_voice_count(&self) -> usize {
        self.active_voice_count
    }

    pub fn is_master_channel(&self, channel: u8) -> bool {
        match self.zone_mode {
            RustZoneMode::LowerZone => channel == 0,
            RustZoneMode::UpperZone => channel == 15,
            RustZoneMode::OmniLegacy => false,
        }
    }

    #[inline]
    pub fn midi_note_to_frequency(note: u8) -> f32 {
        440.0 * 2.0f32.powf((note as f32 - 69.0) / 12.0)
    }

    pub fn set_associated_event_id(&mut self, channel: usize, event_id: u32) {
        if channel < MAX_MPE_CHANNELS {
            self.voices[channel].associated_event_id = event_id;
        }
    }

    pub fn process_midi_message(
        &mut self,
        status: u8,
        data1: u8,
        data2: u8,
    ) -> RustMpeAction {
        let message_type = status & 0xF0;
        let channel = (status & 0x0F) as usize;
        if channel >= MAX_MPE_CHANNELS {
            return RustMpeAction::default();
        }

        match message_type {
            0x90 => { // Note On
                let note = data1 & 0x7F;
                let velocity = data2 & 0x7F;
                if velocity > 0 {
                    self.handle_note_on(channel, note, velocity)
                } else {
                    self.handle_note_off(channel, note, 64)
                }
            }
            0x80 => { // Note Off
                let note = data1 & 0x7F;
                let velocity = data2 & 0x7F;
                self.handle_note_off(channel, note, velocity)
            }
            0xA0 => { // Poly Pressure
                let note = data1 & 0x7F;
                let pressure = (data2 & 0x7F) as f32 / 127.0;
                self.handle_poly_pressure(channel, note, pressure);
                RustMpeAction::default()
            }
            0xD0 => { // Channel Pressure
                let pressure = (data1 & 0x7F) as f32 / 127.0;
                self.handle_channel_pressure(channel, pressure);
                RustMpeAction::default()
            }
            0xE0 => { // Pitch Bend (14-bit)
                let raw_bend = (((data2 & 0x7F) as i32) << 7) | ((data1 & 0x7F) as i32);
                let normalized = (raw_bend as f32 - 8192.0) / 8192.0;
                self.handle_pitch_bend(channel, normalized);
                RustMpeAction::default()
            }
            0xB0 => { // Control Change
                let cc_number = data1 & 0x7F;
                let cc_value = data2 & 0x7F;
                if cc_number == 74 {
                    let slide_val = cc_value as f32 / 127.0;
                    self.handle_slide(channel, slide_val);
                } else if cc_number == 123 || cc_number == 120 {
                    self.handle_all_notes_off();
                }
                RustMpeAction::default()
            }
            _ => RustMpeAction::default(),
        }
    }

    fn handle_note_on(&mut self, channel: usize, note: u8, velocity: u8) -> RustMpeAction {
        let v = &mut self.voices[channel];
        v.channel = channel as u8;
        v.note_number = note;
        v.is_active = true;
        v.strike_velocity = velocity as f32 / 127.0;
        v.press = 0.0;
        v.lift_velocity = 0.0;
        v.slide = 0.5;
        v.per_note_glide_semitones = 0.0;
        v.master_glide_semitones = self.master_bend_normalized * self.master_bend_range;

        v.base_frequency = Self::midi_note_to_frequency(note);
        Self::update_voice_frequency_struct(v);

        let max_duration = (self.sample_rate * 4.0) as u64;
        let pan = if channel >= 1 && channel <= 14 {
            (channel as f32 - 8.0) / 7.0 * 0.5
        } else {
            0.0
        };

        let action = RustMpeAction {
            action_type: RustMpeActionType::SpawnEvent,
            channel: channel as u8,
            note,
            pitch_ratio: v.pitch_ratio,
            strike_velocity: v.strike_velocity,
            pan,
            max_duration_samples: max_duration,
        };

        self.recalculate_active_count();
        action
    }

    fn handle_note_off(&mut self, channel: usize, note: u8, velocity: u8) -> RustMpeAction {
        let is_legacy = self.zone_mode == RustZoneMode::OmniLegacy;
        let v = &mut self.voices[channel];
        if v.is_active && (v.note_number == note || is_legacy) {
            v.is_active = false;
            v.lift_velocity = velocity as f32 / 127.0;
            v.associated_event_id = u32::MAX;
            let pitch_ratio = v.pitch_ratio;

            self.recalculate_active_count();
            return RustMpeAction {
                action_type: RustMpeActionType::ReleaseEvent,
                channel: channel as u8,
                note,
                pitch_ratio,
                strike_velocity: 0.0,
                pan: 0.0,
                max_duration_samples: 0,
            };
        }
        self.recalculate_active_count();
        RustMpeAction::default()
    }

    fn handle_poly_pressure(&mut self, channel: usize, note: u8, pressure: f32) {
        let v = &mut self.voices[channel];
        if v.is_active && v.note_number == note {
            v.press = pressure;
        }
    }

    fn handle_channel_pressure(&mut self, channel: usize, pressure: f32) {
        if self.is_master_channel(channel as u8) {
            for v in &mut self.voices {
                if v.is_active {
                    v.press = pressure;
                }
            }
        } else {
            let v = &mut self.voices[channel];
            if v.is_active {
                v.press = pressure;
            }
        }
    }

    fn handle_pitch_bend(&mut self, channel: usize, normalized: f32) {
        if self.is_master_channel(channel as u8) {
            self.master_bend_normalized = normalized;
            let master_semitones = normalized * self.master_bend_range;

            for v in &mut self.voices {
                if v.is_active {
                    v.master_glide_semitones = master_semitones;
                    Self::update_voice_frequency_struct(v);
                }
            }
        } else {
            let v = &mut self.voices[channel];
            if v.is_active {
                v.per_note_glide_semitones = normalized * self.per_note_bend_range;
                Self::update_voice_frequency_struct(v);
            }
        }
    }

    fn handle_slide(&mut self, channel: usize, slide_val: f32) {
        let v = &mut self.voices[channel];
        if v.is_active {
            v.slide = slide_val;
        }
    }

    fn handle_all_notes_off(&mut self) {
        for v in &mut self.voices {
            v.is_active = false;
            v.associated_event_id = u32::MAX;
        }
        self.active_voice_count = 0;
    }

    #[inline]
    fn update_voice_frequency_struct(v: &mut RustMpeVoice) {
        let total_semitones = v.per_note_glide_semitones + v.master_glide_semitones;
        v.pitch_ratio = 2.0f32.powf(total_semitones / 12.0);
        v.current_frequency = v.base_frequency * v.pitch_ratio;
    }

    fn recalculate_active_count(&mut self) {
        self.active_voice_count = self.voices.iter().filter(|v| v.is_active).count();
    }
}
