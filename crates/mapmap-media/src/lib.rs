//! MapMap Media - Video Decoding and Playback
//!
//! This crate provides video decoding capabilities via FFmpeg, including:
//! - Video decoder abstraction
//! - Playback control (seek, speed, loop)
//!
//! Multi-threaded decoding pipeline is planned for a future phase.

use thiserror::Error;

pub mod decoder;
pub mod player;
pub mod image_decoder;
// TODO: Enable pipeline with thread-local scaler approach
// The pipeline module requires VideoDecoder to be Send, but FFmpeg's scaler (SwsContext) is not thread-safe.
// Solution: Use thread-local scaler - create scaler once in decode thread, avoiding Send requirement.
// This provides zero overhead and clean separation. See pipeline.rs for implementation details.
// pub mod pipeline;

pub use decoder::{VideoDecoder, FFmpegDecoder, TestPatternDecoder, DecodedFrame, PixelFormat, HwAccelType};
pub use player::{VideoPlayer, PlaybackState, PlaybackDirection, PlaybackMode};
pub use image_decoder::{StillImageDecoder, GifDecoder, ImageSequenceDecoder};
// pub use pipeline::{FramePipeline, PipelineConfig, PipelineStats, Priority, FrameScheduler};

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
