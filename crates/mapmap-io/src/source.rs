//! Video source trait and implementations.
//!
//! This module defines the `VideoSource` trait which all video input sources
//! (NDI, DeckLink, Spout, Syphon, etc.) must implement.

use crate::error::Result;
use crate::format::{VideoFormat, VideoFrame};

/// Video source trait for all input types.
///
/// This trait provides a unified interface for receiving video frames from
/// various sources such as NDI, DeckLink, Spout, Syphon, and others.
///
/// # Thread Safety
///
/// Implementations must be `Send` to allow video sources to run on dedicated threads.
///
/// # Example
///
/// ```ignore
/// use mapmap_io::{VideoSource, VideoFormat, PixelFormat};
///
/// fn process_video_source<S: VideoSource>(mut source: S) {
///     println!("Source: {}", source.name());
///     println!("Format: {}", source.format());
///
///     while source.is_available() {
///         match source.receive_frame() {
///             Ok(frame) => {
///                 // Process frame...
///                 println!("Received frame at {:?}", frame.timestamp);
///             }
///             Err(e) => {
///                 eprintln!("Error receiving frame: {}", e);
///                 break;
///             }
///         }
///     }
/// }
/// ```
pub trait VideoSource: Send {
    /// Returns the name of this video source.
    ///
    /// This should be a human-readable identifier for the source,
    /// e.g., "NDI Camera 1", "DeckLink Input", "Spout Receiver".
    fn name(&self) -> &str;

    /// Returns the video format of this source.
    ///
    /// The format describes the resolution, pixel format, and frame rate
    /// of frames produced by this source.
    fn format(&self) -> VideoFormat;

    /// Receives the next available frame from this source.
    ///
    /// This method blocks until a frame is available or an error occurs.
    /// It may return `IoError::NoFrameAvailable` if no frame is ready
    /// or `IoError::FrameTimeout` if the operation times out.
    ///
    /// # Errors
    ///
    /// - `IoError::NoFrameAvailable` - No frame is currently available
    /// - `IoError::FrameTimeout` - Timeout waiting for frame
    /// - `IoError::StreamDisconnected` - Source disconnected
    /// - Other I/O errors specific to the source type
    fn receive_frame(&mut self) -> Result<VideoFrame>;

    /// Returns true if this source is currently available and operational.
    ///
    /// This can be used to check if the source is still connected and
    /// capable of producing frames.
    fn is_available(&self) -> bool;

    /// Returns the number of frames received since the source was created.
    ///
    /// Default implementation returns 0. Implementations should override
    /// this to provide accurate frame counts if available.
    fn frame_count(&self) -> u64 {
        0
    }

    /// Returns true if this source supports seeking/random access.
    ///
    /// Most live sources (NDI, DeckLink, etc.) return false.
    /// File-based sources may return true.
    fn is_seekable(&self) -> bool {
        false
    }

    /// Attempts to reconnect to the source if disconnected.
    ///
    /// Default implementation does nothing and returns Ok(()).
    /// Implementations for network sources (NDI) should override this.
    fn reconnect(&mut self) -> Result<()> {
        Ok(())
    }
}

/// A test/mock video source for testing purposes.
///
/// Generates simple test patterns or solid color frames.
#[cfg(test)]
pub struct TestVideoSource {
    name: String,
    format: VideoFormat,
    frame_count: u64,
    max_frames: Option<u64>,
}

#[cfg(test)]
impl TestVideoSource {
    /// Creates a new test video source.
    pub fn new(name: impl Into<String>, format: VideoFormat) -> Self {
        Self {
            name: name.into(),
            format,
            frame_count: 0,
            max_frames: None,
        }
    }

    /// Sets a maximum number of frames to produce.
    pub fn with_max_frames(mut self, max: u64) -> Self {
        self.max_frames = Some(max);
        self
    }
}

#[cfg(test)]
impl VideoSource for TestVideoSource {
    fn name(&self) -> &str {
        &self.name
    }

    fn format(&self) -> VideoFormat {
        self.format.clone()
    }

    fn receive_frame(&mut self) -> Result<VideoFrame> {
        if let Some(max) = self.max_frames {
            if self.frame_count >= max {
                return Err(crate::error::IoError::NoFrameAvailable);
            }
        }

        let frame = VideoFrame::empty(self.format.clone());
        self.frame_count += 1;
        Ok(frame)
    }

    fn is_available(&self) -> bool {
        if let Some(max) = self.max_frames {
            self.frame_count < max
        } else {
            true
        }
    }

    fn frame_count(&self) -> u64 {
        self.frame_count
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    use crate::format::PixelFormat;

    #[test]
    fn test_video_source_trait() {
        let format = VideoFormat::hd_1080p60_rgba();
        let mut source = TestVideoSource::new("Test", format.clone());

        assert_eq!(source.name(), "Test");
        assert_eq!(source.format(), format);
        assert!(source.is_available());
        assert_eq!(source.frame_count(), 0);

        let frame = source.receive_frame().unwrap();
        assert!(frame.is_valid());
        assert_eq!(source.frame_count(), 1);
    }

    #[test]
    fn test_test_video_source_max_frames() {
        let format = VideoFormat::hd_1080p60_rgba();
        let mut source = TestVideoSource::new("Test", format).with_max_frames(2);

        assert!(source.is_available());
        assert!(source.receive_frame().is_ok());
        assert!(source.is_available());
        assert!(source.receive_frame().is_ok());
        assert!(!source.is_available());
        assert!(source.receive_frame().is_err());
    }
}
