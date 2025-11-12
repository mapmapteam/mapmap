//! MIDI message to control target mapping

use crate::error::Result;
use crate::target::{ControlTarget, ControlValue};
use super::MidiMessage;
use serde::{Deserialize, Serialize};
use std::collections::HashMap;

/// Maps MIDI messages to control targets
#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct MidiMapping {
    /// Message -> Target mappings
    pub mappings: HashMap<MidiMessage, MidiControlMapping>,
}

/// A single MIDI to control mapping
#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct MidiControlMapping {
    pub target: ControlTarget,
    pub min_value: f32,
    pub max_value: f32,
    pub curve: MappingCurve,
}

/// Value mapping curve
#[derive(Debug, Clone, Copy, PartialEq, Serialize, Deserialize)]
pub enum MappingCurve {
    Linear,
    Exponential,
    Logarithmic,
    SCurve,
}

impl MidiMapping {
    pub fn new() -> Self {
        Self {
            mappings: HashMap::new(),
        }
    }

    /// Add a mapping from MIDI message to control target
    pub fn add_mapping(
        &mut self,
        message: MidiMessage,
        target: ControlTarget,
        min_value: f32,
        max_value: f32,
        curve: MappingCurve,
    ) {
        self.mappings.insert(
            message,
            MidiControlMapping {
                target,
                min_value,
                max_value,
                curve,
            },
        );
    }

    /// Remove a mapping
    pub fn remove_mapping(&mut self, message: &MidiMessage) -> Option<MidiControlMapping> {
        self.mappings.remove(message)
    }

    /// Get the control value for a MIDI message
    pub fn get_control_value(&self, message: &MidiMessage) -> Option<(ControlTarget, ControlValue)> {
        let mapping = self.mappings.get(message)?;

        // Get the normalized value (0.0-1.0) from the MIDI message
        let normalized = match message {
            MidiMessage::ControlChange { value, .. } => *value as f32 / 127.0,
            MidiMessage::NoteOn { velocity, .. } => *velocity as f32 / 127.0,
            MidiMessage::PitchBend { value, .. } => *value as f32 / 16383.0,
            MidiMessage::NoteOff { .. } => 0.0,
            _ => return None,
        };

        // Apply curve
        let curved = mapping.curve.apply(normalized);

        // Map to target range
        let value = mapping.min_value + curved * (mapping.max_value - mapping.min_value);

        Some((mapping.target.clone(), ControlValue::Float(value)))
    }

    /// Load from JSON
    pub fn from_json(json: &str) -> Result<Self> {
        Ok(serde_json::from_str(json)?)
    }

    /// Save to JSON
    pub fn to_json(&self) -> Result<String> {
        Ok(serde_json::to_string_pretty(self)?)
    }
}

impl Default for MidiMapping {
    fn default() -> Self {
        Self::new()
    }
}

impl MappingCurve {
    /// Apply the curve to a normalized value (0.0-1.0)
    pub fn apply(&self, value: f32) -> f32 {
        let value = value.clamp(0.0, 1.0);
        match self {
            MappingCurve::Linear => value,
            MappingCurve::Exponential => value * value,
            MappingCurve::Logarithmic => value.sqrt(),
            MappingCurve::SCurve => {
                // Smoothstep function
                value * value * (3.0 - 2.0 * value)
            }
        }
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_mapping_curves() {
        assert_eq!(MappingCurve::Linear.apply(0.5), 0.5);
        assert_eq!(MappingCurve::Exponential.apply(0.5), 0.25);
        assert!((MappingCurve::Logarithmic.apply(0.25) - 0.5).abs() < 0.01);
    }

    #[test]
    fn test_midi_mapping() {
        let mut mapping = MidiMapping::new();

        let msg = MidiMessage::ControlChange {
            channel: 0,
            controller: 7,
            value: 64,
        };

        mapping.add_mapping(
            msg,
            ControlTarget::LayerOpacity(0),
            0.0,
            1.0,
            MappingCurve::Linear,
        );

        let (target, value) = mapping.get_control_value(&msg).unwrap();
        assert_eq!(target, ControlTarget::LayerOpacity(0));

        if let ControlValue::Float(v) = value {
            assert!((v - 0.503).abs() < 0.01); // 64/127 ≈ 0.503
        } else {
            panic!("Expected Float value");
        }
    }

    #[test]
    fn test_mapping_serialization() {
        let mut mapping = MidiMapping::new();
        mapping.add_mapping(
            MidiMessage::ControlChange {
                channel: 0,
                controller: 7,
                value: 0,
            },
            ControlTarget::LayerOpacity(0),
            0.0,
            1.0,
            MappingCurve::Linear,
        );

        let json = mapping.to_json().unwrap();
        let loaded = MidiMapping::from_json(&json).unwrap();

        assert_eq!(mapping.mappings.len(), loaded.mappings.len());
    }
}
