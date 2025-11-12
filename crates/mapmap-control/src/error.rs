///! Error types for the control system

use thiserror::Error;

/// Control system errors
#[derive(Error, Debug)]
pub enum ControlError {
    #[error("MIDI error: {0}")]
    MidiError(String),

    #[error("MIDI connection error: {0}")]
    #[cfg(feature = "midi")]
    MidiConnectionError(#[from] midir::ConnectError<midir::MidiInput>),

    #[error("MIDI init error: {0}")]
    #[cfg(feature = "midi")]
    MidiInitError(#[from] midir::InitError),

    #[error("MIDI send error: {0}")]
    #[cfg(feature = "midi")]
    MidiSendError(#[from] midir::SendError),

    #[error("OSC error: {0}")]
    OscError(String),

    #[error("DMX error: {0}")]
    DmxError(String),

    #[error("HTTP error: {0}")]
    HttpError(String),

    #[error("IO error: {0}")]
    IoError(#[from] std::io::Error),

    #[error("JSON error: {0}")]
    JsonError(#[from] serde_json::Error),

    #[error("Invalid parameter: {0}")]
    InvalidParameter(String),

    #[error("Target not found: {0}")]
    TargetNotFound(String),

    #[error("Invalid message: {0}")]
    InvalidMessage(String),
}

/// Result type for control operations
pub type Result<T> = std::result::Result<T, ControlError>;
