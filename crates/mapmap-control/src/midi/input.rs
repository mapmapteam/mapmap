//! MIDI input handling

use crate::error::{ControlError, Result};
use super::{MidiMessage, MidiMapping};
use midir::{MidiInput as MidirInput, MidiInputConnection, Ignore};
use std::sync::{Arc, Mutex};
use std::sync::mpsc::{channel, Sender, Receiver};
use tracing::{info, warn, error};

/// MIDI input handler
pub struct MidiInputHandler {
    _midi_input: MidirInput,
    connection: Option<MidiInputConnection<()>>,
    message_sender: Sender<MidiMessage>,
    message_receiver: Arc<Mutex<Receiver<MidiMessage>>>,
    mapping: Arc<Mutex<MidiMapping>>,
}

impl MidiInputHandler {
    /// Create a new MIDI input handler
    pub fn new() -> Result<Self> {
        let midi_input = MidirInput::new("MapMap MIDI Input")?;
        let (message_sender, message_receiver) = channel();

        Ok(Self {
            _midi_input: midi_input,
            connection: None,
            message_sender,
            message_receiver: Arc::new(Mutex::new(message_receiver)),
            mapping: Arc::new(Mutex::new(MidiMapping::new())),
        })
    }

    /// List available MIDI input ports
    pub fn list_ports() -> Result<Vec<String>> {
        let midi_input = MidirInput::new("MapMap MIDI Input")?;
        let mut ports = Vec::new();

        for port in midi_input.ports() {
            if let Ok(name) = midi_input.port_name(&port) {
                ports.push(name);
            }
        }

        Ok(ports)
    }

    /// Connect to a MIDI input port by index
    pub fn connect(&mut self, port_index: usize) -> Result<()> {
        // Disconnect existing connection if any
        self.disconnect();

        let mut midi_input = MidirInput::new("MapMap MIDI Input")?;
        midi_input.ignore(Ignore::None);

        let ports = midi_input.ports();
        if port_index >= ports.len() {
            return Err(ControlError::InvalidParameter(format!(
                "Port index {} out of range (max: {})",
                port_index,
                ports.len()
            )));
        }

        let port = &ports[port_index];
        let port_name = midi_input
            .port_name(port)
            .unwrap_or_else(|_| "Unknown".to_string());

        info!("Connecting to MIDI input port: {}", port_name);

        let message_sender = self.message_sender.clone();

        let connection = midi_input
            .connect(
                port,
                "mapmap-input",
                move |_timestamp, message, _| {
                    if let Some(midi_msg) = MidiMessage::from_bytes(message) {
                        if let Err(e) = message_sender.send(midi_msg) {
                            error!("Failed to send MIDI message: {}", e);
                        }
                    }
                },
                (),
            )
            .map_err(|e| ControlError::MidiError(format!("Connection failed: {:?}", e)))?;

        self.connection = Some(connection);

        info!("Successfully connected to MIDI input: {}", port_name);

        Ok(())
    }

    /// Disconnect from MIDI input
    pub fn disconnect(&mut self) {
        if let Some(connection) = self.connection.take() {
            drop(connection);
            info!("Disconnected from MIDI input");
        }
    }

    /// Get the next MIDI message (non-blocking)
    pub fn poll_message(&self) -> Option<MidiMessage> {
        self.message_receiver
            .lock()
            .ok()?
            .try_recv()
            .ok()
    }

    /// Set the MIDI mapping
    pub fn set_mapping(&self, mapping: MidiMapping) {
        if let Ok(mut map) = self.mapping.lock() {
            *map = mapping;
        }
    }

    /// Get a clone of the current mapping
    pub fn get_mapping(&self) -> Option<MidiMapping> {
        self.mapping.lock().ok().map(|m| m.clone())
    }

    /// Check if connected
    pub fn is_connected(&self) -> bool {
        self.connection.is_some()
    }
}

impl Drop for MidiInputHandler {
    fn drop(&mut self) {
        self.disconnect();
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_list_ports() {
        // This test will work even without MIDI ports
        let result = MidiInputHandler::list_ports();
        assert!(result.is_ok());
    }

    #[test]
    fn test_create_handler() {
        let handler = MidiInputHandler::new();
        assert!(handler.is_ok());
    }
}
