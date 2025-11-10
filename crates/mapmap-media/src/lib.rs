//! MapMap Media - Video Decoding and Playback
//!
//! This crate provides video decoding capabilities via FFmpeg, including:
//! - Video decoder abstraction
//! - Multi-threaded decoding pipeline
//! - Playback control (seek, speed, loop)

use std::path::Path;
use std::time::Duration;
use thiserror::Error;

pub mod decoder;
pub mod player;

pub use decoder::{VideoDecoder, FFmpegDecoder, DecodedFrame, PixelFormat};
pub use player::{VideoPlayer, PlaybackState};

/// Media errors
#[derive(Error, Debug)]
pub enum MediaError {
    #[error("Failed to open file: {0}")]
    FileOpen(String),

    #[error("No video stream found")]
    NoVideoStream,

    #[error("Decoder error: {0}")]
    DecoderError(String),

    #[error("End of stream")]
    EndOfStream,

    #[error("Seek error: {0}")]
    SeekError(String),
}

/// Result type for media operations
pub type Result<T> = std::result::Result<T, MediaError>;
