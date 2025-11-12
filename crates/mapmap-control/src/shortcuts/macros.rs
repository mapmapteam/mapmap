//! Macro recording and playback system

use super::Action;
use serde::{Deserialize, Serialize};
use std::time::Duration;

/// A recorded macro
#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct Macro {
    pub name: String,
    pub description: String,
    pub actions: Vec<MacroAction>,
    pub created_at: String,
}

/// A single action within a macro
#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct MacroAction {
    pub action: Action,
    /// Delay before executing this action (relative to previous action)
    pub delay: Duration,
}

/// Macro recorder state
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum RecordingState {
    Idle,
    Recording,
    Paused,
}

/// Macro playback state
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum PlaybackState {
    Idle,
    Playing,
    Paused,
}

/// Macro recorder
pub struct MacroRecorder {
    state: RecordingState,
    recorded_actions: Vec<MacroAction>,
    last_action_time: Option<std::time::Instant>,
}

impl MacroRecorder {
    pub fn new() -> Self {
        Self {
            state: RecordingState::Idle,
            recorded_actions: Vec::new(),
            last_action_time: None,
        }
    }

    /// Start recording a macro
    pub fn start_recording(&mut self) {
        self.state = RecordingState::Recording;
        self.recorded_actions.clear();
        self.last_action_time = None;
    }

    /// Stop recording and return the macro
    pub fn stop_recording(&mut self, name: String, description: String) -> Option<Macro> {
        if self.state != RecordingState::Recording {
            return None;
        }

        self.state = RecordingState::Idle;

        if self.recorded_actions.is_empty() {
            return None;
        }

        let macro_def = Macro {
            name,
            description,
            actions: self.recorded_actions.clone(),
            created_at: chrono::Utc::now().to_rfc3339(),
        };

        self.recorded_actions.clear();
        self.last_action_time = None;

        Some(macro_def)
    }

    /// Pause recording
    pub fn pause_recording(&mut self) {
        if self.state == RecordingState::Recording {
            self.state = RecordingState::Paused;
        }
    }

    /// Resume recording
    pub fn resume_recording(&mut self) {
        if self.state == RecordingState::Paused {
            self.state = RecordingState::Recording;
            self.last_action_time = None; // Reset timing
        }
    }

    /// Cancel recording
    pub fn cancel_recording(&mut self) {
        self.state = RecordingState::Idle;
        self.recorded_actions.clear();
        self.last_action_time = None;
    }

    /// Record an action
    pub fn record_action(&mut self, action: Action) {
        if self.state != RecordingState::Recording {
            return;
        }

        let now = std::time::Instant::now();
        let delay = if let Some(last_time) = self.last_action_time {
            now.duration_since(last_time)
        } else {
            Duration::ZERO
        };

        self.recorded_actions.push(MacroAction { action, delay });
        self.last_action_time = Some(now);
    }

    /// Get current recording state
    pub fn get_state(&self) -> RecordingState {
        self.state
    }

    /// Get number of recorded actions
    pub fn action_count(&self) -> usize {
        self.recorded_actions.len()
    }
}

impl Default for MacroRecorder {
    fn default() -> Self {
        Self::new()
    }
}

/// Macro player
pub struct MacroPlayer {
    state: PlaybackState,
    current_macro: Option<Macro>,
    current_action_index: usize,
    action_start_time: Option<std::time::Instant>,
}

impl MacroPlayer {
    pub fn new() -> Self {
        Self {
            state: PlaybackState::Idle,
            current_macro: None,
            current_action_index: 0,
            action_start_time: None,
        }
    }

    /// Start playing a macro
    pub fn play_macro(&mut self, macro_def: Macro) {
        self.current_macro = Some(macro_def);
        self.current_action_index = 0;
        self.state = PlaybackState::Playing;
        self.action_start_time = Some(std::time::Instant::now());
    }

    /// Stop playback
    pub fn stop(&mut self) {
        self.state = PlaybackState::Idle;
        self.current_macro = None;
        self.current_action_index = 0;
        self.action_start_time = None;
    }

    /// Pause playback
    pub fn pause(&mut self) {
        if self.state == PlaybackState::Playing {
            self.state = PlaybackState::Paused;
        }
    }

    /// Resume playback
    pub fn resume(&mut self) {
        if self.state == PlaybackState::Paused {
            self.state = PlaybackState::Playing;
            self.action_start_time = Some(std::time::Instant::now());
        }
    }

    /// Update playback and return next action to execute
    pub fn update(&mut self) -> Option<Action> {
        if self.state != PlaybackState::Playing {
            return None;
        }

        let macro_def = self.current_macro.as_ref()?;

        if self.current_action_index >= macro_def.actions.len() {
            // Playback complete
            self.stop();
            return None;
        }

        let action = &macro_def.actions[self.current_action_index];
        let start_time = self.action_start_time?;
        let elapsed = std::time::Instant::now().duration_since(start_time);

        if elapsed >= action.delay {
            // Time to execute this action
            let result = action.action.clone();
            self.current_action_index += 1;
            self.action_start_time = Some(std::time::Instant::now());
            Some(result)
        } else {
            None
        }
    }

    /// Get current playback state
    pub fn get_state(&self) -> PlaybackState {
        self.state
    }

    /// Get current playback progress (0.0-1.0)
    pub fn get_progress(&self) -> f32 {
        if let Some(macro_def) = &self.current_macro {
            if !macro_def.actions.is_empty() {
                self.current_action_index as f32 / macro_def.actions.len() as f32
            } else {
                0.0
            }
        } else {
            0.0
        }
    }
}

impl Default for MacroPlayer {
    fn default() -> Self {
        Self::new()
    }
}

impl Macro {
    /// Load from JSON
    pub fn from_json(json: &str) -> Result<Self, serde_json::Error> {
        serde_json::from_str(json)
    }

    /// Save to JSON
    pub fn to_json(&self) -> Result<String, serde_json::Error> {
        serde_json::to_string_pretty(self)
    }

    /// Get total duration of the macro
    pub fn total_duration(&self) -> Duration {
        self.actions.iter().map(|a| a.delay).sum()
    }
}

// For compatibility with systems that don't have chrono
mod chrono {
    use std::time::SystemTime;

    pub struct Utc;

    impl Utc {
        pub fn now() -> DateTime {
            DateTime(SystemTime::now())
        }
    }

    pub struct DateTime(SystemTime);

    impl DateTime {
        pub fn to_rfc3339(&self) -> String {
            format!("{:?}", self.0)
        }
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_macro_recording() {
        let mut recorder = MacroRecorder::new();

        recorder.start_recording();
        assert_eq!(recorder.get_state(), RecordingState::Recording);

        recorder.record_action(Action::Play);
        recorder.record_action(Action::Stop);

        assert_eq!(recorder.action_count(), 2);

        let macro_def = recorder
            .stop_recording("Test Macro".to_string(), "Test description".to_string())
            .unwrap();

        assert_eq!(macro_def.name, "Test Macro");
        assert_eq!(macro_def.actions.len(), 2);
    }

    #[test]
    fn test_macro_playback() {
        let macro_def = Macro {
            name: "Test".to_string(),
            description: "Test".to_string(),
            actions: vec![
                MacroAction {
                    action: Action::Play,
                    delay: Duration::ZERO,
                },
                MacroAction {
                    action: Action::Stop,
                    delay: Duration::from_millis(10),
                },
            ],
            created_at: "2024-01-01T00:00:00Z".to_string(),
        };

        let mut player = MacroPlayer::new();
        player.play_macro(macro_def);

        assert_eq!(player.get_state(), PlaybackState::Playing);

        // First action should execute immediately
        let action = player.update();
        assert!(matches!(action, Some(Action::Play)));
    }

    #[test]
    fn test_macro_serialization() {
        let macro_def = Macro {
            name: "Test".to_string(),
            description: "Test macro".to_string(),
            actions: vec![MacroAction {
                action: Action::Play,
                delay: Duration::ZERO,
            }],
            created_at: "2024-01-01T00:00:00Z".to_string(),
        };

        let json = macro_def.to_json().unwrap();
        let loaded = Macro::from_json(&json).unwrap();

        assert_eq!(macro_def.name, loaded.name);
        assert_eq!(macro_def.actions.len(), loaded.actions.len());
    }
}
