//! Video streaming output.
//!
//! This module provides video streaming capabilities including RTMP and SRT streaming.
//! All streaming functionality requires the `stream` feature flag.
//!
//! # Features
//!
//! - RTMP streaming to platforms like Twitch, YouTube, Facebook Live
//! - SRT streaming for low-latency applications (stub)
//! - H.264/H.265 encoding
//! - Automatic reconnection on network failure
//!
//! # Example
//!
//! ```ignore
//! use mapmap_io::stream::RtmpStreamer;
//! use mapmap_io::format::VideoFormat;
//! use mapmap_io::VideoSink;
//!
//! let format = VideoFormat::hd_1080p60_rgba();
//! let mut streamer = RtmpStreamer::new(
//!     "rtmp://live.twitch.tv/app/stream_key",
//!     format,
//!     6_000_000, // 6 Mbps
//! ).unwrap();
//!
//! // Send frames...
//! let frame = VideoFrame::empty(format);
//! streamer.send_frame(&frame).unwrap();
//! ```

pub mod encoder;
pub mod rtmp;
pub mod srt;

// Re-exports
#[cfg(feature = "stream")]
pub use encoder::{EncodedPacket, EncoderPreset, VideoCodec, VideoEncoder};
#[cfg(feature = "stream")]
pub use rtmp::RtmpStreamer;
#[cfg(feature = "stream")]
pub use srt::SrtStreamer;

// Stub exports when stream feature is disabled
#[cfg(not(feature = "stream"))]
pub use rtmp::RtmpStreamer;
#[cfg(not(feature = "stream"))]
pub use srt::SrtStreamer;
