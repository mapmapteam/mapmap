//! MapMap Control - MIDI, OSC, and DMX Control
//!
//! This crate provides control system integration including:
//! - MIDI input/output
//! - OSC (Open Sound Control)
//! - Art-Net/sACN DMX output
//! - HTTP REST API
//!
//! NOTE: This is a stub implementation for Phase 0.
//! Full implementation will be completed in Phase 4.

use thiserror::Error;

/// Control system errors
#[derive(Error, Debug)]
pub enum ControlError {
    #[error("MIDI error: {0}")]
    MidiError(String),

    #[error("OSC error: {0}")]
    OscError(String),

    #[error("DMX error: {0}")]
    DmxError(String),

    #[error("HTTP error: {0}")]
    HttpError(String),
}

/// Result type for control operations
pub type Result<T> = std::result::Result<T, ControlError>;

/// MIDI input handler (stub)
pub struct MidiInput {
    // Will be implemented in Phase 4
}

impl MidiInput {
    pub fn new() -> Result<Self> {
        Ok(Self {})
    }
}

/// OSC server (stub)
pub struct OscServer {
    // Will be implemented in Phase 4
}

impl OscServer {
    pub fn new(port: u16) -> Result<Self> {
        Ok(Self {})
    }
}

/// Art-Net sender (stub)
pub struct ArtNetSender {
    // Will be implemented in Phase 4
}

impl ArtNetSender {
    pub fn new() -> Result<Self> {
        Ok(Self {})
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_stub_creation() {
        assert!(MidiInput::new().is_ok());
        assert!(OscServer::new(8000).is_ok());
        assert!(ArtNetSender::new().is_ok());
    }
}
