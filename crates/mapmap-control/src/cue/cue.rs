//! Cue definition and state
//!
//! A cue is a snapshot of the entire project state at a point in time.

use serde::{Deserialize, Serialize};
use std::collections::HashMap;
use std::time::Duration;

use super::crossfade::FadeCurve;
use super::triggers::{MidiTrigger, TimeTrigger};

/// A cue stores a complete snapshot of project state
#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct Cue {
    pub id: u32,
    pub name: String,
    pub description: String,

    // State snapshots
    pub layer_states: HashMap<u32, LayerState>,
    pub paint_states: HashMap<u32, PaintState>,
    pub effect_states: HashMap<u32, EffectState>,
    pub global_state: GlobalState,

    // Transition settings
    pub fade_duration: Duration,
    pub fade_curve: FadeCurve,

    // Triggers
    pub auto_follow: Option<Duration>, // Auto-advance after duration
    pub midi_trigger: Option<MidiTrigger>,
    pub time_trigger: Option<TimeTrigger>,
}

/// Snapshot of a layer's state
#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct LayerState {
    pub opacity: f32,
    pub visible: bool,
    pub position: (f32, f32),
    pub rotation: f32,
    pub scale: f32,
}

/// Snapshot of a paint's state
#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct PaintState {
    pub parameters: HashMap<String, f32>,
}

/// Snapshot of an effect's state
#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct EffectState {
    pub enabled: bool,
    pub parameters: HashMap<String, f32>,
}

/// Global playback state
#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct GlobalState {
    pub playback_speed: f32,
    pub playback_position: f32,
    pub output_brightness: HashMap<u32, f32>,
}

impl Default for GlobalState {
    fn default() -> Self {
        Self {
            playback_speed: 1.0,
            playback_position: 0.0,
            output_brightness: HashMap::new(),
        }
    }
}

impl Cue {
    /// Create a new empty cue
    pub fn new(id: u32, name: String) -> Self {
        Self {
            id,
            name,
            description: String::new(),
            layer_states: HashMap::new(),
            paint_states: HashMap::new(),
            effect_states: HashMap::new(),
            global_state: GlobalState::default(),
            fade_duration: Duration::from_secs(2),
            fade_curve: FadeCurve::Linear,
            auto_follow: None,
            midi_trigger: None,
            time_trigger: None,
        }
    }

    /// Create a cue with a name and description
    pub fn with_description(id: u32, name: String, description: String) -> Self {
        Self {
            description,
            ..Self::new(id, name)
        }
    }

    /// Set the fade duration
    pub fn with_fade_duration(mut self, duration: Duration) -> Self {
        self.fade_duration = duration;
        self
    }

    /// Set the fade curve
    pub fn with_fade_curve(mut self, curve: FadeCurve) -> Self {
        self.fade_curve = curve;
        self
    }

    /// Set auto-follow duration
    pub fn with_auto_follow(mut self, duration: Duration) -> Self {
        self.auto_follow = Some(duration);
        self
    }

    /// Add a layer state snapshot
    pub fn add_layer_state(&mut self, layer_id: u32, state: LayerState) {
        self.layer_states.insert(layer_id, state);
    }

    /// Add a paint state snapshot
    pub fn add_paint_state(&mut self, paint_id: u32, state: PaintState) {
        self.paint_states.insert(paint_id, state);
    }

    /// Add an effect state snapshot
    pub fn add_effect_state(&mut self, effect_id: u32, state: EffectState) {
        self.effect_states.insert(effect_id, state);
    }

    /// Check if this cue has any state
    pub fn is_empty(&self) -> bool {
        self.layer_states.is_empty()
            && self.paint_states.is_empty()
            && self.effect_states.is_empty()
    }
}

impl LayerState {
    /// Create a new layer state
    pub fn new(opacity: f32, visible: bool, position: (f32, f32), rotation: f32, scale: f32) -> Self {
        Self {
            opacity,
            visible,
            position,
            rotation,
            scale,
        }
    }

    /// Create a default visible layer
    pub fn default_visible() -> Self {
        Self {
            opacity: 1.0,
            visible: true,
            position: (0.0, 0.0),
            rotation: 0.0,
            scale: 1.0,
        }
    }
}

impl PaintState {
    /// Create a new paint state
    pub fn new() -> Self {
        Self {
            parameters: HashMap::new(),
        }
    }

    /// Set a parameter value
    pub fn set_parameter(&mut self, name: String, value: f32) {
        self.parameters.insert(name, value);
    }
}

impl Default for PaintState {
    fn default() -> Self {
        Self::new()
    }
}

impl EffectState {
    /// Create a new effect state
    pub fn new(enabled: bool) -> Self {
        Self {
            enabled,
            parameters: HashMap::new(),
        }
    }

    /// Set a parameter value
    pub fn set_parameter(&mut self, name: String, value: f32) {
        self.parameters.insert(name, value);
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_cue_creation() {
        let cue = Cue::new(0, "Test Cue".to_string());
        assert_eq!(cue.id, 0);
        assert_eq!(cue.name, "Test Cue");
        assert!(cue.is_empty());
    }

    #[test]
    fn test_cue_with_states() {
        let mut cue = Cue::new(0, "Test Cue".to_string());
        cue.add_layer_state(0, LayerState::default_visible());

        assert!(!cue.is_empty());
        assert_eq!(cue.layer_states.len(), 1);
    }

    #[test]
    fn test_layer_state() {
        let state = LayerState::new(0.75, true, (100.0, 200.0), 45.0, 1.5);
        assert_eq!(state.opacity, 0.75);
        assert_eq!(state.position, (100.0, 200.0));
        assert_eq!(state.rotation, 45.0);
    }

    #[test]
    fn test_paint_state() {
        let mut state = PaintState::new();
        state.set_parameter("speed".to_string(), 0.5);
        assert_eq!(state.parameters.get("speed"), Some(&0.5));
    }

    #[test]
    fn test_serialization() {
        let cue = Cue::new(0, "Test".to_string())
            .with_fade_duration(Duration::from_secs(3))
            .with_fade_curve(FadeCurve::EaseInOut);

        let json = serde_json::to_string(&cue).unwrap();
        let deserialized: Cue = serde_json::from_str(&json).unwrap();

        assert_eq!(deserialized.id, cue.id);
        assert_eq!(deserialized.fade_duration, cue.fade_duration);
    }
}
