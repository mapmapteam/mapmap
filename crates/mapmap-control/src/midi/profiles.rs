//! MIDI controller profiles

use super::{MidiMessage, MidiMapping, MappingCurve};
use crate::target::ControlTarget;
use serde::{Deserialize, Serialize};
use std::collections::HashMap;

/// Predefined MIDI controller profile
#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct ControllerProfile {
    pub name: String,
    pub manufacturer: String,
    pub description: String,
    pub mappings: Vec<ProfileMapping>,
}

#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct ProfileMapping {
    pub message_template: MidiMessageTemplate,
    pub target: ControlTarget,
    pub min_value: f32,
    pub max_value: f32,
    pub curve: MappingCurve,
    pub label: String,
}

#[derive(Debug, Clone, Serialize, Deserialize)]
pub enum MidiMessageTemplate {
    ControlChange { channel: u8, controller: u8 },
    Note { channel: u8, note: u8 },
    PitchBend { channel: u8 },
}

impl ControllerProfile {
    /// Convert profile to MIDI mapping
    pub fn to_midi_mapping(&self) -> MidiMapping {
        let mut mapping = MidiMapping::new();

        for profile_mapping in &self.mappings {
            let message = match profile_mapping.message_template {
                MidiMessageTemplate::ControlChange { channel, controller } => {
                    MidiMessage::ControlChange {
                        channel,
                        controller,
                        value: 0,
                    }
                }
                MidiMessageTemplate::Note { channel, note } => MidiMessage::NoteOn {
                    channel,
                    note,
                    velocity: 0,
                },
                MidiMessageTemplate::PitchBend { channel } => {
                    MidiMessage::PitchBend { channel, value: 0 }
                }
            };

            mapping.add_mapping(
                message,
                profile_mapping.target.clone(),
                profile_mapping.min_value,
                profile_mapping.max_value,
                profile_mapping.curve,
            );
        }

        mapping
    }

    /// Load from JSON
    pub fn from_json(json: &str) -> Result<Self, serde_json::Error> {
        serde_json::from_str(json)
    }

    /// Save to JSON
    pub fn to_json(&self) -> Result<String, serde_json::Error> {
        serde_json::to_string_pretty(self)
    }
}

/// Built-in controller profiles
pub struct BuiltInProfiles;

impl BuiltInProfiles {
    /// Get all built-in profiles
    pub fn all() -> Vec<ControllerProfile> {
        vec![
            Self::generic_controller(),
            Self::akai_apc40(),
            Self::novation_launchpad(),
        ]
    }

    /// Generic MIDI controller with common CC mappings
    pub fn generic_controller() -> ControllerProfile {
        ControllerProfile {
            name: "Generic MIDI Controller".to_string(),
            manufacturer: "Generic".to_string(),
            description: "Generic MIDI controller with common CC assignments".to_string(),
            mappings: vec![
                ProfileMapping {
                    message_template: MidiMessageTemplate::ControlChange {
                        channel: 0,
                        controller: 7,
                    },
                    target: ControlTarget::LayerOpacity(0),
                    min_value: 0.0,
                    max_value: 1.0,
                    curve: MappingCurve::Linear,
                    label: "Master Volume → Layer 0 Opacity".to_string(),
                },
                ProfileMapping {
                    message_template: MidiMessageTemplate::ControlChange {
                        channel: 0,
                        controller: 1,
                    },
                    target: ControlTarget::PlaybackSpeed(None),
                    min_value: 0.0,
                    max_value: 2.0,
                    curve: MappingCurve::Linear,
                    label: "Modulation → Playback Speed".to_string(),
                },
            ],
        }
    }

    /// Akai APC40 profile
    pub fn akai_apc40() -> ControllerProfile {
        let mut mappings = Vec::new();

        // Track volume faders (CC 7, 48-55) -> Layer opacity
        for i in 0..8 {
            mappings.push(ProfileMapping {
                message_template: MidiMessageTemplate::ControlChange {
                    channel: 0,
                    controller: 48 + i,
                },
                target: ControlTarget::LayerOpacity(i as u32),
                min_value: 0.0,
                max_value: 1.0,
                curve: MappingCurve::Linear,
                label: format!("Fader {} → Layer {} Opacity", i + 1, i),
            });
        }

        // Track knobs (CC 16-23) -> Layer rotation
        for i in 0..8 {
            mappings.push(ProfileMapping {
                message_template: MidiMessageTemplate::ControlChange {
                    channel: 0,
                    controller: 16 + i,
                },
                target: ControlTarget::LayerRotation(i as u32),
                min_value: 0.0,
                max_value: 360.0,
                curve: MappingCurve::Linear,
                label: format!("Knob {} → Layer {} Rotation", i + 1, i),
            });
        }

        // Crossfader (CC 15) -> Playback speed
        mappings.push(ProfileMapping {
            message_template: MidiMessageTemplate::ControlChange {
                channel: 0,
                controller: 15,
            },
            target: ControlTarget::PlaybackSpeed(None),
            min_value: 0.0,
            max_value: 2.0,
            curve: MappingCurve::Linear,
            label: "Crossfader → Playback Speed".to_string(),
        });

        ControllerProfile {
            name: "Akai APC40".to_string(),
            manufacturer: "Akai".to_string(),
            description: "Akai APC40/APC40 MKII controller mapping".to_string(),
            mappings,
        }
    }

    /// Novation Launchpad profile
    pub fn novation_launchpad() -> ControllerProfile {
        let mut mappings = Vec::new();

        // Top row buttons (CC 104-111) -> Layer visibility
        for i in 0..8 {
            mappings.push(ProfileMapping {
                message_template: MidiMessageTemplate::ControlChange {
                    channel: 0,
                    controller: 104 + i,
                },
                target: ControlTarget::LayerVisibility(i as u32),
                min_value: 0.0,
                max_value: 1.0,
                curve: MappingCurve::Linear,
                label: format!("Button {} → Layer {} Visibility", i + 1, i),
            });
        }

        ControllerProfile {
            name: "Novation Launchpad".to_string(),
            manufacturer: "Novation".to_string(),
            description: "Novation Launchpad controller mapping".to_string(),
            mappings,
        }
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_generic_profile() {
        let profile = BuiltInProfiles::generic_controller();
        assert_eq!(profile.name, "Generic MIDI Controller");
        assert!(!profile.mappings.is_empty());

        let mapping = profile.to_midi_mapping();
        assert!(!mapping.mappings.is_empty());
    }

    #[test]
    fn test_apc40_profile() {
        let profile = BuiltInProfiles::akai_apc40();
        assert_eq!(profile.name, "Akai APC40");
        assert!(profile.mappings.len() > 10); // Should have many mappings

        let mapping = profile.to_midi_mapping();
        assert!(mapping.mappings.len() > 10);
    }

    #[test]
    fn test_profile_serialization() {
        let profile = BuiltInProfiles::generic_controller();
        let json = profile.to_json().unwrap();
        let loaded = ControllerProfile::from_json(&json).unwrap();

        assert_eq!(profile.name, loaded.name);
        assert_eq!(profile.mappings.len(), loaded.mappings.len());
    }
}
