//! Unified control system manager
//!
//! This module provides a unified interface for managing all control systems
//! (MIDI, OSC, DMX, Web API, Cue system, and keyboard shortcuts).

use crate::error::{ControlError, Result};
use crate::target::{ControlTarget, ControlValue};
use crate::shortcuts::{Action, Key, KeyBindings, Modifiers};
use std::sync::{Arc, Mutex};
use tracing::{info, warn};

#[cfg(feature = "midi")]
use crate::midi::{MidiInputHandler, MidiMapping, MidiLearn};

use crate::osc::{OscServer, OscClient};
use crate::dmx::{ArtNetSender, SacnSender};
use crate::cue::CueList;

/// Unified control system manager
pub struct ControlManager {
    #[cfg(feature = "midi")]
    pub midi_input: Option<MidiInputHandler>,

    #[cfg(feature = "midi")]
    pub midi_learn: Option<MidiLearn>,

    pub osc_server: Option<OscServer>,
    pub osc_client: Option<OscClient>,

    pub artnet_sender: Option<ArtNetSender>,
    pub sacn_sender: Option<SacnSender>,

    pub cue_list: CueList,
    pub key_bindings: KeyBindings,

    /// Event callback for control changes
    control_callback: Option<Arc<Mutex<dyn FnMut(ControlTarget, ControlValue) + Send>>>,
}

impl ControlManager {
    pub fn new() -> Self {
        Self {
            #[cfg(feature = "midi")]
            midi_input: None,

            #[cfg(feature = "midi")]
            midi_learn: None,

            osc_server: None,
            osc_client: None,

            artnet_sender: None,
            sacn_sender: None,

            cue_list: CueList::new(),
            key_bindings: KeyBindings::new(),

            control_callback: None,
        }
    }

    /// Set control change callback
    pub fn set_control_callback<F>(&mut self, callback: F)
    where
        F: FnMut(ControlTarget, ControlValue) + Send + 'static,
    {
        self.control_callback = Some(Arc::new(Mutex::new(callback)));
    }

    /// Initialize MIDI input
    #[cfg(feature = "midi")]
    pub fn init_midi_input(&mut self) -> Result<()> {
        info!("Initializing MIDI input");
        let handler = MidiInputHandler::new()?;
        self.midi_input = Some(handler);
        self.midi_learn = Some(MidiLearn::new());
        Ok(())
    }

    /// Initialize OSC server
    pub fn init_osc_server(&mut self, port: u16) -> Result<()> {
        info!("Initializing OSC server on port {}", port);
        let server = OscServer::new(port)?;
        self.osc_server = Some(server);
        Ok(())
    }

    /// Initialize OSC client
    pub fn init_osc_client(&mut self, addr: &str) -> Result<()> {
        info!("Initializing OSC client to {}", addr);
        let client = OscClient::new(addr)?;
        self.osc_client = Some(client);
        Ok(())
    }

    /// Initialize Art-Net sender
    pub fn init_artnet(&mut self, universe: u16, target: &str) -> Result<()> {
        info!("Initializing Art-Net sender for universe {} to {}", universe, target);
        let sender = ArtNetSender::new(universe, target)?;
        self.artnet_sender = Some(sender);
        Ok(())
    }

    /// Initialize sACN sender
    pub fn init_sacn(&mut self, universe: u16, source_name: &str) -> Result<()> {
        info!("Initializing sACN sender for universe {} with source {}", universe, source_name);
        let sender = SacnSender::new(universe, source_name)?;
        self.sacn_sender = Some(sender);
        Ok(())
    }

    /// Update all control systems (call every frame)
    pub fn update(&mut self) {
        // Process MIDI messages
        #[cfg(feature = "midi")]
        self.process_midi_messages();

        // Process OSC messages
        self.process_osc_messages();

        // Update cue system (for auto-follow, crossfades, etc.)
        // This would integrate with the project state
    }

    /// Process MIDI messages
    #[cfg(feature = "midi")]
    fn process_midi_messages(&mut self) {
        if let Some(midi_input) = &self.midi_input {
            while let Some(message) = midi_input.poll_message() {
                // Check if in learn mode
                if let Some(learn) = &self.midi_learn {
                    if learn.process_message(message) {
                        continue; // Message consumed by learn mode
                    }
                }

                // Get mapping and apply control
                if let Some(mapping) = midi_input.get_mapping() {
                    if let Some((target, value)) = mapping.get_control_value(&message) {
                        self.apply_control(target, value);
                    }
                }
            }
        }
    }

    /// Process OSC messages
    fn process_osc_messages(&mut self) {
        // Collect events first to avoid borrow checker issues
        let mut events = Vec::new();
        if let Some(osc_server) = &mut self.osc_server {
            while let Some(event) = osc_server.poll_event() {
                events.push(event);
            }
        }

        // Process collected events
        for event in events {
            self.apply_control(event.target, event.value);
        }
    }

    /// Apply a control change
    pub fn apply_control(&mut self, target: ControlTarget, value: ControlValue) {
        info!("Control change: {:?} = {:?}", target, value);

        // Call the control callback if set
        if let Some(callback) = &self.control_callback {
            if let Ok(mut cb) = callback.lock() {
                cb(target.clone(), value.clone());
            }
        }

        // Send OSC feedback if client is configured
        if let Some(osc_client) = &mut self.osc_client {
            if let Err(e) = osc_client.send_update(&target, &value) {
                warn!("Failed to send OSC feedback: {}", e);
            }
        }
    }

    /// Execute an action
    pub fn execute_action(&mut self, action: Action) {
        info!("Executing action: {:?}", action);

        match action {
            Action::NextCue => {
                let _ = self.cue_list.next();
            }
            Action::PrevCue => {
                let _ = self.cue_list.prev();
            }
            Action::GotoCue(id) => {
                let _ = self.cue_list.goto_cue(id, None);
            }
            _ => {
                // Other actions would be handled by the application
                info!("Action requires application handling: {:?}", action);
            }
        }
    }

    /// Handle keyboard input
    pub fn handle_key_press(&mut self, key: Key, modifiers: &Modifiers) {
        if let Some(action) = self.key_bindings.find_action(key, modifiers) {
            self.execute_action(action);
        }
    }

    /// Send DMX data via Art-Net
    pub fn send_artnet(&mut self, channels: &[u8; 512], target: &str) -> Result<()> {
        if let Some(sender) = &mut self.artnet_sender {
            sender.send_dmx(channels, target)?;
        } else {
            return Err(ControlError::DmxError("Art-Net not initialized".to_string()));
        }
        Ok(())
    }

    /// Send DMX data via sACN
    pub fn send_sacn(&mut self, channels: &[u8; 512]) -> Result<()> {
        if let Some(sender) = &mut self.sacn_sender {
            sender.send_dmx(channels)?;
        } else {
            return Err(ControlError::DmxError("sACN not initialized".to_string()));
        }
        Ok(())
    }
}

impl Default for ControlManager {
    fn default() -> Self {
        Self::new()
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_create_manager() {
        let manager = ControlManager::new();
        assert!(manager.osc_server.is_none());
        assert!(manager.osc_client.is_none());
    }

    #[test]
    fn test_key_bindings() {
        let mut manager = ControlManager::new();

        // Default bindings should include Space -> TogglePlayPause
        manager.handle_key_press(Key::Space, &Modifiers::new());

        // This should work without panicking
    }

    #[test]
    fn test_control_callback() {
        use std::sync::atomic::{AtomicBool, Ordering};

        let mut manager = ControlManager::new();
        let called = Arc::new(AtomicBool::new(false));
        let called_clone = called.clone();

        manager.set_control_callback(move |_target, _value| {
            called_clone.store(true, Ordering::SeqCst);
        });

        manager.apply_control(ControlTarget::LayerOpacity(0), ControlValue::Float(0.5));

        assert!(called.load(Ordering::SeqCst));
    }
}
