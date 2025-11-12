//! Video sink trait and implementations.
//!
//! This module defines the `VideoSink` trait which all video output sinks
//! (NDI, DeckLink, Spout, Syphon, streaming, etc.) must implement.

use crate::error::Result;
use crate::format::{VideoFormat, VideoFrame};

/// Video sink trait for all output types.
///
/// This trait provides a unified interface for sending video frames to
/// various outputs such as NDI, DeckLink, Spout, Syphon, streaming servers, etc.
///
/// # Thread Safety
///
/// Implementations must be `Send` to allow video sinks to run on dedicated threads.
///
/// # Example
///
/// ```ignore
/// use mapmap_io::{VideoSink, VideoFrame};
///
/// fn output_video<S: VideoSink>(mut sink: S, frame: VideoFrame) {
///     println!("Sink: {}", sink.name());
///     println!("Format: {}", sink.format());
///
///     match sink.send_frame(&frame) {
///         Ok(()) => println!("Frame sent successfully"),
///         Err(e) => eprintln!("Error sending frame: {}", e),
///     }
/// }
/// ```
pub trait VideoSink: Send {
    /// Returns the name of this video sink.
    ///
    /// This should be a human-readable identifier for the sink,
    /// e.g., "NDI Sender", "DeckLink Output", "RTMP Stream".
    fn name(&self) -> &str;

    /// Returns the video format of this sink.
    ///
    /// The format describes the expected resolution, pixel format, and frame rate
    /// that this sink can handle. Frames sent to this sink should match this format,
    /// or format conversion may be applied.
    fn format(&self) -> VideoFormat;

    /// Sends a video frame to this sink.
    ///
    /// This method attempts to send the provided frame to the output.
    /// It may block briefly if the output buffer is full.
    ///
    /// # Parameters
    ///
    /// - `frame` - The video frame to send. Should match the sink's format.
    ///
    /// # Errors
    ///
    /// - `IoError::StreamDisconnected` - Sink disconnected
    /// - `IoError::InvalidFrameData` - Frame data is invalid
    /// - `IoError::FrameSizeMismatch` - Frame size doesn't match expected format
    /// - Other I/O errors specific to the sink type
    fn send_frame(&mut self, frame: &VideoFrame) -> Result<()>;

    /// Returns true if this sink is currently available and operational.
    ///
    /// This can be used to check if the sink is still connected and
    /// capable of accepting frames.
    fn is_available(&self) -> bool {
        true
    }

    /// Returns the number of frames sent since the sink was created.
    ///
    /// Default implementation returns 0. Implementations should override
    /// this to provide accurate frame counts if available.
    fn frame_count(&self) -> u64 {
        0
    }

    /// Flushes any buffered frames to the output.
    ///
    /// Default implementation does nothing and returns Ok(()).
    /// Implementations that buffer frames should override this.
    fn flush(&mut self) -> Result<()> {
        Ok(())
    }

    /// Attempts to reconnect to the sink if disconnected.
    ///
    /// Default implementation does nothing and returns Ok(()).
    /// Implementations for network sinks (RTMP, etc.) should override this.
    fn reconnect(&mut self) -> Result<()> {
        Ok(())
    }

    /// Returns statistics about the sink's performance.
    ///
    /// Default implementation returns None. Implementations can override
    /// to provide detailed statistics (bitrate, dropped frames, etc.).
    fn statistics(&self) -> Option<SinkStatistics> {
        None
    }
}

/// Statistics about a video sink's performance.
#[derive(Debug, Clone, Default)]
pub struct SinkStatistics {
    /// Total frames sent
    pub frames_sent: u64,
    /// Total frames dropped
    pub frames_dropped: u64,
    /// Current bitrate in bits per second (for streaming sinks)
    pub bitrate: Option<u64>,
    /// Average latency in milliseconds
    pub average_latency_ms: Option<f32>,
}

impl SinkStatistics {
    /// Creates new sink statistics.
    pub fn new() -> Self {
        Self::default()
    }

    /// Returns the drop rate as a percentage (0.0 to 100.0).
    pub fn drop_rate(&self) -> f32 {
        if self.frames_sent == 0 {
            0.0
        } else {
            (self.frames_dropped as f32 / self.frames_sent as f32) * 100.0
        }
    }
}

/// A test/mock video sink for testing purposes.
///
/// Accepts and validates frames without actually outputting them.
#[cfg(test)]
pub struct TestVideoSink {
    name: String,
    format: VideoFormat,
    frame_count: u64,
    should_fail: bool,
}

#[cfg(test)]
impl TestVideoSink {
    /// Creates a new test video sink.
    pub fn new(name: impl Into<String>, format: VideoFormat) -> Self {
        Self {
            name: name.into(),
            format,
            frame_count: 0,
            should_fail: false,
        }
    }

    /// Configures the sink to fail on send.
    pub fn with_fail(mut self) -> Self {
        self.should_fail = true;
        self
    }
}

#[cfg(test)]
impl VideoSink for TestVideoSink {
    fn name(&self) -> &str {
        &self.name
    }

    fn format(&self) -> VideoFormat {
        self.format.clone()
    }

    fn send_frame(&mut self, frame: &VideoFrame) -> Result<()> {
        if self.should_fail {
            return Err(crate::error::IoError::StreamDisconnected);
        }

        frame.validate()?;
        self.frame_count += 1;
        Ok(())
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
    fn test_video_sink_trait() {
        let format = VideoFormat::hd_1080p60_rgba();
        let mut sink = TestVideoSink::new("Test", format.clone());

        assert_eq!(sink.name(), "Test");
        assert_eq!(sink.format(), format);
        assert_eq!(sink.frame_count(), 0);

        let frame = VideoFrame::empty(format);
        assert!(sink.send_frame(&frame).is_ok());
        assert_eq!(sink.frame_count(), 1);
    }

    #[test]
    fn test_test_video_sink_fail() {
        let format = VideoFormat::hd_1080p60_rgba();
        let mut sink = TestVideoSink::new("Test", format.clone()).with_fail();

        let frame = VideoFrame::empty(format);
        assert!(sink.send_frame(&frame).is_err());
    }

    #[test]
    fn test_sink_statistics() {
        let mut stats = SinkStatistics::new();
        stats.frames_sent = 100;
        stats.frames_dropped = 5;

        assert_eq!(stats.drop_rate(), 5.0);
    }

    #[test]
    fn test_sink_statistics_no_frames() {
        let stats = SinkStatistics::new();
        assert_eq!(stats.drop_rate(), 0.0);
    }
}
