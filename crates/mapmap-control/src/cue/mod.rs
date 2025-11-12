//! Cue system for show automation
//!
//! This module provides a cue system for creating automated shows with smooth
//! transitions between states.
//!
//! ## Features
//!
//! - **Cues**: Snapshots of complete project state
//! - **Crossfades**: Smooth transitions with configurable curves
//! - **Triggers**: MIDI, OSC, and time-based cue activation
//! - **Auto-follow**: Automatic progression through cue list
//!
//! ## Example Usage
//!
//! ```rust
//! use mapmap_control::cue::{Cue, CueList, LayerState};
//! use std::time::Duration;
//!
//! // Create a cue list
//! let mut list = CueList::new();
//!
//! // Create cues
//! let mut cue1 = Cue::new(0, "Opening".to_string());
//! cue1.add_layer_state(0, LayerState::default_visible());
//!
//! let mut cue2 = Cue::new(1, "Main".to_string())
//!     .with_fade_duration(Duration::from_secs(3));
//! cue2.add_layer_state(0, LayerState::new(0.5, true, (0.0, 0.0), 0.0, 1.0));
//!
//! // Add cues to list
//! list.add_cue(cue1);
//! list.add_cue(cue2);
//!
//! // Trigger cues
//! list.goto_cue(0, None).unwrap();
//! list.next().unwrap();
//! ```
//!
//! ## Fade Curves
//!
//! The cue system supports multiple fade curves for crossfades:
//! - `Linear`: Constant rate of change
//! - `EaseIn`: Slow start, fast end
//! - `EaseOut`: Fast start, slow end
//! - `EaseInOut`: Slow start and end, fast middle
//! - `Exponential`: Exponential curve
//!
//! ## Triggers
//!
//! Cues can be triggered by:
//! - **MIDI**: Note, CC, or Program Change messages
//! - **OSC**: OSC address patterns
//! - **Time**: Specific time of day
//! - **Auto-follow**: Automatic progression after a delay
//!
//! ```rust
//! use mapmap_control::cue::{Cue, triggers::MidiTrigger};
//! use std::time::Duration;
//!
//! let cue = Cue::new(0, "Test".to_string())
//!     .with_auto_follow(Duration::from_secs(10));
//! ```

pub mod cue;
pub mod cue_list;
pub mod crossfade;
pub mod triggers;

pub use cue::{Cue, EffectState, GlobalState, LayerState, PaintState};
pub use cue_list::{CueList, CueListState};
pub use crossfade::{Crossfade, FadeCurve, interpolate_f32, interpolate_position};
pub use triggers::{MidiTrigger, MidiTriggerType, OscTrigger, TimeTrigger};
