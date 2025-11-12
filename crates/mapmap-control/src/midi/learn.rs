//! MIDI learn mode implementation

use crate::target::ControlTarget;
use super::{MidiMessage, MidiMapping, MappingCurve};
use std::sync::{Arc, Mutex};
use tracing::info;

/// MIDI learn state
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum LearnState {
    Idle,
    WaitingForMessage,
    MessageReceived,
}

/// MIDI learn mode handler
pub struct MidiLearn {
    state: Arc<Mutex<LearnState>>,
    pending_target: Arc<Mutex<Option<ControlTarget>>>,
    last_message: Arc<Mutex<Option<MidiMessage>>>,
}

impl MidiLearn {
    pub fn new() -> Self {
        Self {
            state: Arc::new(Mutex::new(LearnState::Idle)),
            pending_target: Arc::new(Mutex::new(None)),
            last_message: Arc::new(Mutex::new(None)),
        }
    }

    /// Start learning mode for a specific control target
    pub fn start_learn(&self, target: ControlTarget) {
        info!("Starting MIDI learn for target: {:?}", target);

        if let Ok(mut state) = self.state.lock() {
            *state = LearnState::WaitingForMessage;
        }

        if let Ok(mut pending) = self.pending_target.lock() {
            *pending = Some(target);
        }

        if let Ok(mut last) = self.last_message.lock() {
            *last = None;
        }
    }

    /// Cancel learn mode
    pub fn cancel_learn(&self) {
        info!("Cancelling MIDI learn");

        if let Ok(mut state) = self.state.lock() {
            *state = LearnState::Idle;
        }

        if let Ok(mut pending) = self.pending_target.lock() {
            *pending = None;
        }

        if let Ok(mut last) = self.last_message.lock() {
            *last = None;
        }
    }

    /// Process incoming MIDI message during learn mode
    pub fn process_message(&self, message: MidiMessage) -> bool {
        if let Ok(state) = self.state.lock() {
            if *state != LearnState::WaitingForMessage {
                return false;
            }
        } else {
            return false;
        }

        // Only learn from certain message types
        match message {
            MidiMessage::ControlChange { .. }
            | MidiMessage::NoteOn { .. }
            | MidiMessage::PitchBend { .. } => {}
            _ => return false,
        }

        info!("MIDI learn captured message: {:?}", message);

        if let Ok(mut last) = self.last_message.lock() {
            *last = Some(message);
        }

        if let Ok(mut state) = self.state.lock() {
            *state = LearnState::MessageReceived;
        }

        true
    }

    /// Complete the learn process and add mapping
    pub fn complete_learn(
        &self,
        mapping: &mut MidiMapping,
        min_value: f32,
        max_value: f32,
        curve: MappingCurve,
    ) -> Option<(MidiMessage, ControlTarget)> {
        let state = self.state.lock().ok()?;
        if *state != LearnState::MessageReceived {
            return None;
        }
        drop(state);

        let message = self.last_message.lock().ok()?.clone()?;
        let target = self.pending_target.lock().ok()?.clone()?;

        // For Note On messages, we use a template that matches any velocity
        let template_message = match message {
            MidiMessage::NoteOn { channel, note, .. } => MidiMessage::ControlChange {
                channel,
                controller: note,
                value: 0,
            },
            MidiMessage::ControlChange {
                channel,
                controller,
                ..
            } => MidiMessage::ControlChange {
                channel,
                controller,
                value: 0,
            },
            MidiMessage::PitchBend { channel, .. } => MidiMessage::PitchBend {
                channel,
                value: 0,
            },
            _ => message,
        };

        info!(
            "MIDI learn completed: {:?} -> {:?}",
            template_message, target
        );

        mapping.add_mapping(template_message, target.clone(), min_value, max_value, curve);

        // Reset state
        if let Ok(mut state) = self.state.lock() {
            *state = LearnState::Idle;
        }
        if let Ok(mut pending) = self.pending_target.lock() {
            *pending = None;
        }
        if let Ok(mut last) = self.last_message.lock() {
            *last = None;
        }

        Some((template_message, target))
    }

    /// Get current learn state
    pub fn get_state(&self) -> LearnState {
        self.state.lock().ok().copied().unwrap_or(LearnState::Idle)
    }

    /// Get pending target
    pub fn get_pending_target(&self) -> Option<ControlTarget> {
        self.pending_target.lock().ok()?.clone()
    }

    /// Get last received message
    pub fn get_last_message(&self) -> Option<MidiMessage> {
        self.last_message.lock().ok()?.clone()
    }
}

impl Default for MidiLearn {
    fn default() -> Self {
        Self::new()
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    use crate::target::ControlTarget;

    #[test]
    fn test_learn_workflow() {
        let learn = MidiLearn::new();
        let mut mapping = MidiMapping::new();

        // Start learning
        learn.start_learn(ControlTarget::LayerOpacity(0));
        assert_eq!(learn.get_state(), LearnState::WaitingForMessage);

        // Process message
        let msg = MidiMessage::ControlChange {
            channel: 0,
            controller: 7,
            value: 64,
        };
        assert!(learn.process_message(msg));
        assert_eq!(learn.get_state(), LearnState::MessageReceived);

        // Complete learning
        let result = learn.complete_learn(&mut mapping, 0.0, 1.0, MappingCurve::Linear);
        assert!(result.is_some());
        assert_eq!(learn.get_state(), LearnState::Idle);

        // Verify mapping was added
        assert_eq!(mapping.mappings.len(), 1);
    }

    #[test]
    fn test_cancel_learn() {
        let learn = MidiLearn::new();

        learn.start_learn(ControlTarget::LayerOpacity(0));
        assert_eq!(learn.get_state(), LearnState::WaitingForMessage);

        learn.cancel_learn();
        assert_eq!(learn.get_state(), LearnState::Idle);
        assert!(learn.get_pending_target().is_none());
    }
}
