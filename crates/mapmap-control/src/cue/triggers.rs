//! Cue trigger system
//!
//! Triggers allow cues to be activated by various events.

use serde::{Deserialize, Serialize};

#[cfg(feature = "midi")]
use crate::midi::MidiMessage;

/// MIDI trigger configuration
#[derive(Debug, Clone, PartialEq, Eq, Hash, Serialize, Deserialize)]
pub struct MidiTrigger {
    /// MIDI channel (0-15)
    pub channel: u8,
    /// Trigger type
    pub trigger_type: MidiTriggerType,
}

/// Types of MIDI triggers
#[derive(Debug, Clone, Copy, PartialEq, Eq, Hash, Serialize, Deserialize)]
pub enum MidiTriggerType {
    /// Trigger on note
    Note { note: u8 },
    /// Trigger on control change
    ControlChange { controller: u8, value: u8 },
    /// Trigger on program change
    ProgramChange { program: u8 },
}

impl MidiTrigger {
    /// Create a note trigger
    pub fn note(channel: u8, note: u8) -> Self {
        Self {
            channel,
            trigger_type: MidiTriggerType::Note { note },
        }
    }

    /// Create a control change trigger
    pub fn control_change(channel: u8, controller: u8, value: u8) -> Self {
        Self {
            channel,
            trigger_type: MidiTriggerType::ControlChange { controller, value },
        }
    }

    /// Create a program change trigger
    pub fn program_change(channel: u8, program: u8) -> Self {
        Self {
            channel,
            trigger_type: MidiTriggerType::ProgramChange { program },
        }
    }

    /// Check if a MIDI message matches this trigger
    #[cfg(feature = "midi")]
    pub fn matches(&self, message: &MidiMessage) -> bool {
        match (&self.trigger_type, message) {
            (
                MidiTriggerType::Note { note },
                MidiMessage::NoteOn {
                    channel,
                    note: msg_note,
                    ..
                },
            ) => *channel == self.channel && *msg_note == *note,

            (
                MidiTriggerType::ControlChange { controller, value },
                MidiMessage::ControlChange {
                    channel,
                    controller: msg_controller,
                    value: msg_value,
                },
            ) => {
                *channel == self.channel
                    && *msg_controller == *controller
                    && *msg_value == *value
            }

            (
                MidiTriggerType::ProgramChange { program },
                MidiMessage::ProgramChange {
                    channel,
                    program: msg_program,
                },
            ) => *channel == self.channel && *msg_program == *program,

            _ => false,
        }
    }

    #[cfg(not(feature = "midi"))]
    pub fn matches(&self, _message: &()) -> bool {
        false
    }
}

/// Time-based trigger
#[derive(Debug, Clone, Copy, PartialEq, Eq, Hash, Serialize, Deserialize)]
pub struct TimeTrigger {
    /// Hour (0-23)
    pub hour: u8,
    /// Minute (0-59)
    pub minute: u8,
    /// Second (0-59)
    pub second: u8,
}

impl TimeTrigger {
    /// Create a new time trigger
    pub fn new(hour: u8, minute: u8, second: u8) -> Option<Self> {
        if hour < 24 && minute < 60 && second < 60 {
            Some(Self {
                hour,
                minute,
                second,
            })
        } else {
            None
        }
    }

    /// Check if the current time matches this trigger
    pub fn matches_now(&self) -> bool {
        use std::time::SystemTime;

        if let Ok(duration) = SystemTime::now().duration_since(SystemTime::UNIX_EPOCH) {
            let total_seconds = duration.as_secs();
            let seconds_today = (total_seconds % 86400) as u32;

            let trigger_seconds =
                self.hour as u32 * 3600 + self.minute as u32 * 60 + self.second as u32;

            // Match within a 1-second window
            seconds_today == trigger_seconds
        } else {
            false
        }
    }
}

/// OSC trigger pattern
#[derive(Debug, Clone, PartialEq, Eq, Hash, Serialize, Deserialize)]
pub struct OscTrigger {
    /// OSC address pattern
    pub address: String,
    /// Optional value to match
    pub value: Option<String>,
}

impl OscTrigger {
    /// Create a new OSC trigger
    pub fn new(address: String) -> Self {
        Self {
            address,
            value: None,
        }
    }

    /// Create an OSC trigger with a specific value
    pub fn with_value(address: String, value: String) -> Self {
        Self {
            address,
            value: Some(value),
        }
    }

    /// Check if an OSC address matches this trigger
    pub fn matches_address(&self, address: &str) -> bool {
        self.address == address
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_midi_note_trigger() {
        let trigger = MidiTrigger::note(0, 60);
        assert_eq!(trigger.channel, 0);
        matches!(trigger.trigger_type, MidiTriggerType::Note { note: 60 });
    }

    #[test]
    fn test_midi_control_change_trigger() {
        let trigger = MidiTrigger::control_change(0, 7, 127);
        assert_eq!(trigger.channel, 0);
        matches!(
            trigger.trigger_type,
            MidiTriggerType::ControlChange {
                controller: 7,
                value: 127
            }
        );
    }

    #[cfg(feature = "midi")]
    #[test]
    fn test_midi_trigger_matching() {
        let trigger = MidiTrigger::note(0, 60);

        let matching_msg = MidiMessage::NoteOn {
            channel: 0,
            note: 60,
            velocity: 100,
        };
        assert!(trigger.matches(&matching_msg));

        let non_matching_msg = MidiMessage::NoteOn {
            channel: 0,
            note: 61,
            velocity: 100,
        };
        assert!(!trigger.matches(&non_matching_msg));

        let wrong_channel = MidiMessage::NoteOn {
            channel: 1,
            note: 60,
            velocity: 100,
        };
        assert!(!trigger.matches(&wrong_channel));
    }

    #[test]
    fn test_time_trigger() {
        let trigger = TimeTrigger::new(12, 30, 0);
        assert!(trigger.is_some());

        let trigger = trigger.unwrap();
        assert_eq!(trigger.hour, 12);
        assert_eq!(trigger.minute, 30);
        assert_eq!(trigger.second, 0);
    }

    #[test]
    fn test_invalid_time_trigger() {
        assert!(TimeTrigger::new(24, 0, 0).is_none());
        assert!(TimeTrigger::new(0, 60, 0).is_none());
        assert!(TimeTrigger::new(0, 0, 60).is_none());
    }

    #[test]
    fn test_osc_trigger() {
        let trigger = OscTrigger::new("/mapmap/cue/1".to_string());
        assert!(trigger.matches_address("/mapmap/cue/1"));
        assert!(!trigger.matches_address("/mapmap/cue/2"));
    }

    #[test]
    fn test_osc_trigger_with_value() {
        let trigger = OscTrigger::with_value("/mapmap/cue/1".to_string(), "1.0".to_string());
        assert_eq!(trigger.value, Some("1.0".to_string()));
    }
}
