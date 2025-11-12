//! MIDI input/output system

#[cfg(feature = "midi")]
mod input;
#[cfg(feature = "midi")]
mod output;
#[cfg(feature = "midi")]
mod learn;
#[cfg(feature = "midi")]
mod mapping;
#[cfg(feature = "midi")]
mod profiles;
#[cfg(feature = "midi")]
mod clock;

#[cfg(feature = "midi")]
pub use input::*;
#[cfg(feature = "midi")]
pub use output::*;
#[cfg(feature = "midi")]
pub use learn::*;
#[cfg(feature = "midi")]
pub use mapping::*;
#[cfg(feature = "midi")]
pub use profiles::*;
#[cfg(feature = "midi")]
pub use clock::*;

use serde::{Deserialize, Serialize};

/// MIDI message types
#[derive(Debug, Clone, Copy, PartialEq, Eq, Hash, Serialize, Deserialize)]
pub enum MidiMessage {
    NoteOn {
        channel: u8,
        note: u8,
        velocity: u8,
    },
    NoteOff {
        channel: u8,
        note: u8,
    },
    ControlChange {
        channel: u8,
        controller: u8,
        value: u8,
    },
    ProgramChange {
        channel: u8,
        program: u8,
    },
    PitchBend {
        channel: u8,
        value: u16,
    },
    Clock,
    Start,
    Stop,
    Continue,
}

impl MidiMessage {
    /// Parse a MIDI message from raw bytes
    pub fn from_bytes(bytes: &[u8]) -> Option<Self> {
        if bytes.is_empty() {
            return None;
        }

        let status = bytes[0];

        // Real-time messages (single byte)
        match status {
            0xF8 => return Some(MidiMessage::Clock),
            0xFA => return Some(MidiMessage::Start),
            0xFC => return Some(MidiMessage::Stop),
            0xFB => return Some(MidiMessage::Continue),
            _ => {}
        }

        // Channel messages (need at least 2 bytes)
        if bytes.len() < 2 {
            return None;
        }

        let message_type = status & 0xF0;
        let channel = status & 0x0F;

        match message_type {
            0x90 => {
                // Note On
                let velocity = bytes[2];
                if velocity == 0 {
                    // Note On with velocity 0 is treated as Note Off
                    Some(MidiMessage::NoteOff {
                        channel,
                        note: bytes[1],
                    })
                } else {
                    Some(MidiMessage::NoteOn {
                        channel,
                        note: bytes[1],
                        velocity,
                    })
                }
            }
            0x80 => {
                // Note Off
                Some(MidiMessage::NoteOff {
                    channel,
                    note: bytes[1],
                })
            }
            0xB0 => {
                // Control Change
                Some(MidiMessage::ControlChange {
                    channel,
                    controller: bytes[1],
                    value: bytes[2],
                })
            }
            0xC0 => {
                // Program Change
                Some(MidiMessage::ProgramChange {
                    channel,
                    program: bytes[1],
                })
            }
            0xE0 => {
                // Pitch Bend
                let value = ((bytes[2] as u16) << 7) | (bytes[1] as u16);
                Some(MidiMessage::PitchBend { channel, value })
            }
            _ => None,
        }
    }

    /// Convert to raw MIDI bytes
    pub fn to_bytes(&self) -> Vec<u8> {
        match self {
            MidiMessage::NoteOn {
                channel,
                note,
                velocity,
            } => vec![0x90 | channel, *note, *velocity],
            MidiMessage::NoteOff { channel, note } => vec![0x80 | channel, *note, 0],
            MidiMessage::ControlChange {
                channel,
                controller,
                value,
            } => vec![0xB0 | channel, *controller, *value],
            MidiMessage::ProgramChange { channel, program } => vec![0xC0 | channel, *program],
            MidiMessage::PitchBend { channel, value } => {
                vec![0xE0 | channel, (*value & 0x7F) as u8, (*value >> 7) as u8]
            }
            MidiMessage::Clock => vec![0xF8],
            MidiMessage::Start => vec![0xFA],
            MidiMessage::Stop => vec![0xFC],
            MidiMessage::Continue => vec![0xFB],
        }
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_midi_message_parsing() {
        // Note On
        let msg = MidiMessage::from_bytes(&[0x90, 60, 100]);
        assert_eq!(
            msg,
            Some(MidiMessage::NoteOn {
                channel: 0,
                note: 60,
                velocity: 100
            })
        );

        // Note Off (via Note On with velocity 0)
        let msg = MidiMessage::from_bytes(&[0x90, 60, 0]);
        assert_eq!(
            msg,
            Some(MidiMessage::NoteOff {
                channel: 0,
                note: 60
            })
        );

        // Control Change
        let msg = MidiMessage::from_bytes(&[0xB0, 7, 64]);
        assert_eq!(
            msg,
            Some(MidiMessage::ControlChange {
                channel: 0,
                controller: 7,
                value: 64
            })
        );

        // Clock
        let msg = MidiMessage::from_bytes(&[0xF8]);
        assert_eq!(msg, Some(MidiMessage::Clock));
    }

    #[test]
    fn test_midi_message_to_bytes() {
        let msg = MidiMessage::NoteOn {
            channel: 0,
            note: 60,
            velocity: 100,
        };
        assert_eq!(msg.to_bytes(), vec![0x90, 60, 100]);

        let msg = MidiMessage::ControlChange {
            channel: 0,
            controller: 7,
            value: 64,
        };
        assert_eq!(msg.to_bytes(), vec![0xB0, 7, 64]);

        let msg = MidiMessage::Clock;
        assert_eq!(msg.to_bytes(), vec![0xF8]);
    }
}
