//! Event Engine Data Types & Structures (Roadmap Phase 2, Rules 2, 3, 10, 11, 46)
//! Explicit lifecycle states, disappearance modes, event attributes and telemetry.

#[repr(u16)]
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum EventType {
    Unknown = 0,
    Fragment = 1,
    Grain = 2,
    Transient = 3,
    Note = 4,
    AudioSlice = 5,
    Loop = 6,
    Burst = 7,
    Echo = 8,
    Tail = 9,
    Freeze = 10,
    Reverse = 11,
    PitchEvent = 12,
    SpectralEvent = 13,
    GeneratedEvent = 14,
}

impl From<u16> for EventType {
    fn from(val: u16) -> Self {
        match val {
            1 => EventType::Fragment,
            2 => EventType::Grain,
            3 => EventType::Transient,
            4 => EventType::Note,
            5 => EventType::AudioSlice,
            6 => EventType::Loop,
            7 => EventType::Burst,
            8 => EventType::Echo,
            9 => EventType::Tail,
            10 => EventType::Freeze,
            11 => EventType::Reverse,
            12 => EventType::PitchEvent,
            13 => EventType::SpectralEvent,
            14 => EventType::GeneratedEvent,
            _ => EventType::Unknown,
        }
    }
}

#[repr(u8)]
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum EventLifecycle {
    Created = 0,
    Playing = 1,
    Releasing = 2,
    Frozen = 3,
    Looping = 4,
    Killed = 5,
    Destroyed = 6,
}

impl From<u8> for EventLifecycle {
    fn from(val: u8) -> Self {
        match val {
            0 => EventLifecycle::Created,
            1 => EventLifecycle::Playing,
            2 => EventLifecycle::Releasing,
            3 => EventLifecycle::Frozen,
            4 => EventLifecycle::Looping,
            5 => EventLifecycle::Killed,
            6 => EventLifecycle::Destroyed,
            _ => EventLifecycle::Created,
        }
    }
}

#[repr(u8)]
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum SourceDisappearanceMode {
    Cut = 0,
    Short = 1,
    Fade = 2,
    Natural = 3,
    Hold = 4,
    Freeze = 5,
}

impl From<u8> for SourceDisappearanceMode {
    fn from(val: u8) -> Self {
        match val {
            0 => SourceDisappearanceMode::Cut,
            1 => SourceDisappearanceMode::Short,
            2 => SourceDisappearanceMode::Fade,
            3 => SourceDisappearanceMode::Natural,
            4 => SourceDisappearanceMode::Hold,
            5 => SourceDisappearanceMode::Freeze,
            _ => SourceDisappearanceMode::Fade,
        }
    }
}

#[repr(C)]
#[derive(Debug, Clone, Copy)]
pub struct RustEventAttributes {
    pub id: u32,
    pub event_type: u16,
    pub state: u8,
    pub disappearance_mode: u8,

    pub source_follow: f32,
    pub pitch_ratio: f32,
    pub gain: f32,
    pub pan: f32,
    pub azimuth: f32,
    pub elevation: f32,
    pub distance: f32,
    pub energy: f32,

    pub generation: u32,
    pub parent_id: u32,

    pub age_samples: u64,
    pub lifetime_samples: u64,
    pub attack_samples: u32,
    pub release_samples: u32,
}

impl Default for RustEventAttributes {
    fn default() -> Self {
        Self {
            id: 0,
            event_type: EventType::Fragment as u16,
            state: EventLifecycle::Created as u8,
            disappearance_mode: SourceDisappearanceMode::Fade as u8,
            source_follow: 0.0,
            pitch_ratio: 1.0,
            gain: 1.0,
            pan: 0.0,
            azimuth: 0.0,
            elevation: 0.0,
            distance: 1.0,
            energy: 1.0,
            generation: 0,
            parent_id: 0,
            age_samples: 0,
            lifetime_samples: 0,
            attack_samples: 128,
            release_samples: 512,
        }
    }
}

#[repr(C)]
#[derive(Debug, Clone, Copy)]
pub struct RustEventTelemetryItem {
    pub pan: f32,
    pub pitch_ratio: f32,
    pub energy: f32,
    pub distance: f32,
    pub azimuth: f32,
    pub event_type: u16,
    pub generation: u8,
    pub is_alive: bool,
}

impl Default for RustEventTelemetryItem {
    fn default() -> Self {
        Self {
            pan: 0.0,
            pitch_ratio: 1.0,
            energy: 0.0,
            distance: 1.0,
            azimuth: 0.0,
            event_type: EventType::Fragment as u16,
            generation: 0,
            is_alive: true,
        }
    }
}
