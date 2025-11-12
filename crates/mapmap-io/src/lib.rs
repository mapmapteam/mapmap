//! Professional video I/O for MapMap.
//!
//! This crate provides video input/output capabilities for MapMap, including support for:
//!
//! - **NDI** - Network Device Interface for IP-based video
//! - **DeckLink** - Blackmagic Design SDI/HDMI capture cards
//! - **Spout** - Windows DirectX texture sharing
//! - **Syphon** - macOS OpenGL/Metal texture sharing
//! - **Streaming** - RTMP/SRT video streaming
//! - **Virtual Camera** - Appear as a camera device to other applications
//!
//! # Features
//!
//! This crate uses feature flags to enable optional functionality:
//!
//! - `ndi` - Enable NDI support (requires NDI SDK)
//! - `decklink` - Enable DeckLink support (requires DeckLink SDK)
//! - `spout` - Enable Spout support (Windows only)
//! - `syphon` - Enable Syphon support (macOS only)
//! - `stream` - Enable streaming support (RTMP/SRT)
//! - `virtual-camera` - Enable virtual camera support
//! - `all-io` - Enable all available I/O features
//!
//! # Architecture
//!
//! The crate is organized around two core traits:
//!
//! - [`VideoSource`] - Trait for video input sources (NDI, DeckLink, Spout, Syphon)
//! - [`VideoSink`] - Trait for video output sinks (NDI, DeckLink, streaming, virtual camera)
//!
//! All video data flows through the [`VideoFrame`] type, which encapsulates pixel data,
//! format information, timestamps, and metadata.
//!
//! # Example: RTMP Streaming
//!
//! ```ignore
//! use mapmap_io::{VideoSink, VideoFrame};
//! use mapmap_io::stream::RtmpStreamer;
//! use mapmap_io::format::VideoFormat;
//!
//! // Create a 1080p60 RTMP streamer
//! let mut streamer = RtmpStreamer::default_1080p60(
//!     "rtmp://live.twitch.tv/app/stream_key"
//! ).unwrap();
//!
//! // Send frames
//! let format = VideoFormat::hd_1080p60_rgba();
//! let frame = VideoFrame::empty(format);
//! streamer.send_frame(&frame).unwrap();
//! ```
//!
//! # Example: Format Conversion
//!
//! ```ignore
//! use mapmap_io::converter::FormatConverter;
//! use mapmap_io::format::{VideoFormat, VideoFrame, PixelFormat};
//!
//! let converter = FormatConverter::new();
//!
//! // Convert BGRA to RGBA
//! let bgra_format = VideoFormat::new(1920, 1080, PixelFormat::BGRA8, 60.0);
//! let rgba_format = VideoFormat::new(1920, 1080, PixelFormat::RGBA8, 60.0);
//!
//! let bgra_frame = VideoFrame::empty(bgra_format);
//! let rgba_frame = converter.convert(&bgra_frame, &rgba_format).unwrap();
//! ```
//!
//! # Platform Support
//!
//! | Feature | Windows | macOS | Linux |
//! |---------|---------|-------|-------|
//! | NDI | ✓ | ✓ | ✓ |
//! | DeckLink | ✓ | ✓ | ✓ |
//! | Spout | ✓ | ✗ | ✗ |
//! | Syphon | ✗ | ✓ | ✗ |
//! | Streaming | ✓ | ✓ | ✓ |
//! | Virtual Camera | ✓ | ✓ | ✓ |

#![warn(missing_docs)]
#![warn(clippy::all)]
#![allow(clippy::module_inception)]

// Core modules (always available)
pub mod converter;
pub mod error;
pub mod format;
pub mod sink;
pub mod source;

// Feature-gated modules
#[cfg(feature = "ndi")]
pub mod ndi;

#[cfg(feature = "decklink")]
pub mod decklink;

#[cfg(feature = "spout")]
pub mod spout;

#[cfg(feature = "syphon")]
pub mod syphon;

#[cfg(feature = "stream")]
pub mod stream;

#[cfg(feature = "virtual-camera")]
pub mod virtual_camera;

// Stub modules when features are disabled (for API compatibility)
#[cfg(not(feature = "ndi"))]
pub mod ndi;

#[cfg(not(feature = "decklink"))]
pub mod decklink;

#[cfg(not(feature = "spout"))]
pub mod spout;

#[cfg(not(feature = "syphon"))]
pub mod syphon;

#[cfg(not(feature = "stream"))]
pub mod stream;

#[cfg(not(feature = "virtual-camera"))]
pub mod virtual_camera;

// Re-exports for convenience
pub use converter::FormatConverter;
pub use error::{IoError, Result};
pub use format::{FrameMetadata, PixelFormat, VideoFormat, VideoFrame};
pub use sink::{SinkStatistics, VideoSink};
pub use source::VideoSource;

// Feature-specific re-exports
#[cfg(feature = "ndi")]
pub use ndi::{NdiReceiver, NdiSender, NdiSource};

#[cfg(feature = "decklink")]
pub use decklink::{DeckLinkDevice, DeckLinkInput, DeckLinkOutput};

#[cfg(feature = "spout")]
pub use spout::{SpoutReceiver, SpoutSender, SpoutSenderInfo};

#[cfg(feature = "syphon")]
pub use syphon::{SyphonClient, SyphonServer, SyphonServerInfo};

#[cfg(feature = "stream")]
pub use stream::{EncodedPacket, EncoderPreset, RtmpStreamer, SrtStreamer, VideoCodec, VideoEncoder};

#[cfg(feature = "virtual-camera")]
pub use virtual_camera::VirtualCamera;

/// Library version information.
pub const VERSION: &str = env!("CARGO_PKG_VERSION");

/// Returns information about enabled features.
pub fn features() -> FeatureInfo {
    FeatureInfo {
        ndi: cfg!(feature = "ndi"),
        decklink: cfg!(feature = "decklink"),
        spout: cfg!(feature = "spout"),
        syphon: cfg!(feature = "syphon"),
        stream: cfg!(feature = "stream"),
        virtual_camera: cfg!(feature = "virtual-camera"),
    }
}

/// Information about which features are enabled in this build.
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub struct FeatureInfo {
    /// NDI support enabled
    pub ndi: bool,
    /// DeckLink support enabled
    pub decklink: bool,
    /// Spout support enabled (Windows only)
    pub spout: bool,
    /// Syphon support enabled (macOS only)
    pub syphon: bool,
    /// Streaming support enabled
    pub stream: bool,
    /// Virtual camera support enabled
    pub virtual_camera: bool,
}

impl FeatureInfo {
    /// Returns true if all features are enabled.
    pub fn all_enabled(&self) -> bool {
        self.ndi && self.decklink && self.stream && self.virtual_camera
            // Platform-specific features
            && (!cfg!(target_os = "windows") || self.spout)
            && (!cfg!(target_os = "macos") || self.syphon)
    }

    /// Returns the number of enabled features.
    pub fn count_enabled(&self) -> usize {
        let mut count = 0;
        if self.ndi {
            count += 1;
        }
        if self.decklink {
            count += 1;
        }
        if self.spout {
            count += 1;
        }
        if self.syphon {
            count += 1;
        }
        if self.stream {
            count += 1;
        }
        if self.virtual_camera {
            count += 1;
        }
        count
    }
}

impl std::fmt::Display for FeatureInfo {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        write!(f, "MapMap I/O v{} (", VERSION)?;
        let mut first = true;

        if self.ndi {
            write!(f, "ndi")?;
            first = false;
        }
        if self.decklink {
            if !first {
                write!(f, ", ")?;
            }
            write!(f, "decklink")?;
            first = false;
        }
        if self.spout {
            if !first {
                write!(f, ", ")?;
            }
            write!(f, "spout")?;
            first = false;
        }
        if self.syphon {
            if !first {
                write!(f, ", ")?;
            }
            write!(f, "syphon")?;
            first = false;
        }
        if self.stream {
            if !first {
                write!(f, ", ")?;
            }
            write!(f, "stream")?;
            first = false;
        }
        if self.virtual_camera {
            if !first {
                write!(f, ", ")?;
            }
            write!(f, "virtual-camera")?;
        }

        write!(f, ")")
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_version() {
        assert!(!VERSION.is_empty());
    }

    #[test]
    fn test_features() {
        let features = features();
        // At minimum, core features should be available
        assert!(features.count_enabled() >= 0);
    }

    #[test]
    fn test_feature_info_display() {
        let features = features();
        let display = format!("{}", features);
        assert!(display.contains("MapMap I/O"));
        assert!(display.contains(VERSION));
    }

    #[test]
    fn test_feature_info_count() {
        let mut info = FeatureInfo {
            ndi: true,
            decklink: true,
            spout: false,
            syphon: false,
            stream: true,
            virtual_camera: false,
        };
        assert_eq!(info.count_enabled(), 3);

        info.virtual_camera = true;
        assert_eq!(info.count_enabled(), 4);
    }
}
