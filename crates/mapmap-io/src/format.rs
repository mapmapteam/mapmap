//! Video format definitions and structures.
//!
//! This module defines pixel formats, video formats, frame metadata,
//! and video frame structures used throughout the I/O system.

use std::time::Duration;
use serde::{Deserialize, Serialize};

/// Pixel format enumeration.
///
/// Represents various pixel formats commonly used in video I/O.
#[derive(Debug, Clone, Copy, PartialEq, Eq, Hash, Serialize, Deserialize)]
pub enum PixelFormat {
    /// RGBA 8-bit per channel (32-bit total)
    RGBA8,
    /// BGRA 8-bit per channel (32-bit total, common on Windows)
    BGRA8,
    /// RGB 8-bit per channel (24-bit total)
    RGB8,
    /// YUV 4:2:0 planar format
    YUV420P,
    /// YUV 4:2:2 planar format
    YUV422P,
    /// UYVY 4:2:2 packed format
    UYVY,
    /// NV12 format (Y plane + interleaved UV)
    NV12,
}

impl PixelFormat {
    /// Returns the number of bytes per pixel for this format.
    ///
    /// Note: For planar formats, this returns an average.
    pub fn bytes_per_pixel(&self) -> usize {
        match self {
            PixelFormat::RGBA8 => 4,
            PixelFormat::BGRA8 => 4,
            PixelFormat::RGB8 => 3,
            PixelFormat::YUV420P => 1,  // Average across Y, U, V planes
            PixelFormat::YUV422P => 2,  // Average across Y, U, V planes
            PixelFormat::UYVY => 2,
            PixelFormat::NV12 => 1,     // Average across Y and UV planes
        }
    }

    /// Calculates the total buffer size needed for a frame of this format.
    pub fn buffer_size(&self, width: u32, height: u32) -> usize {
        let pixels = (width * height) as usize;
        match self {
            PixelFormat::RGBA8 => pixels * 4,
            PixelFormat::BGRA8 => pixels * 4,
            PixelFormat::RGB8 => pixels * 3,
            PixelFormat::YUV420P => (pixels * 3) / 2,  // Y + U/4 + V/4
            PixelFormat::YUV422P => pixels * 2,        // Y + U/2 + V/2
            PixelFormat::UYVY => pixels * 2,
            PixelFormat::NV12 => (pixels * 3) / 2,     // Y + UV/2
        }
    }

    /// Returns true if this is a planar format.
    pub fn is_planar(&self) -> bool {
        matches!(
            self,
            PixelFormat::YUV420P | PixelFormat::YUV422P | PixelFormat::NV12
        )
    }

    /// Returns true if this is a YUV format.
    pub fn is_yuv(&self) -> bool {
        matches!(
            self,
            PixelFormat::YUV420P
                | PixelFormat::YUV422P
                | PixelFormat::UYVY
                | PixelFormat::NV12
        )
    }

    /// Returns true if this is an RGB format.
    pub fn is_rgb(&self) -> bool {
        matches!(
            self,
            PixelFormat::RGBA8 | PixelFormat::BGRA8 | PixelFormat::RGB8
        )
    }

    /// Returns the format name as a string.
    pub fn name(&self) -> &'static str {
        match self {
            PixelFormat::RGBA8 => "RGBA8",
            PixelFormat::BGRA8 => "BGRA8",
            PixelFormat::RGB8 => "RGB8",
            PixelFormat::YUV420P => "YUV420P",
            PixelFormat::YUV422P => "YUV422P",
            PixelFormat::UYVY => "UYVY",
            PixelFormat::NV12 => "NV12",
        }
    }
}

impl std::fmt::Display for PixelFormat {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        write!(f, "{}", self.name())
    }
}

/// Video format description.
///
/// Describes the complete video format including resolution, pixel format, and frame rate.
#[derive(Debug, Clone, PartialEq, Serialize, Deserialize)]
pub struct VideoFormat {
    /// Width in pixels
    pub width: u32,
    /// Height in pixels
    pub height: u32,
    /// Pixel format
    pub pixel_format: PixelFormat,
    /// Frame rate in frames per second
    pub frame_rate: f32,
}

impl VideoFormat {
    /// Creates a new video format.
    pub fn new(width: u32, height: u32, pixel_format: PixelFormat, frame_rate: f32) -> Self {
        Self {
            width,
            height,
            pixel_format,
            frame_rate,
        }
    }

    /// Creates a standard 1080p60 RGBA format.
    pub fn hd_1080p60_rgba() -> Self {
        Self::new(1920, 1080, PixelFormat::RGBA8, 60.0)
    }

    /// Creates a standard 1080p30 RGBA format.
    pub fn hd_1080p30_rgba() -> Self {
        Self::new(1920, 1080, PixelFormat::RGBA8, 30.0)
    }

    /// Creates a standard 720p60 RGBA format.
    pub fn hd_720p60_rgba() -> Self {
        Self::new(1280, 720, PixelFormat::RGBA8, 60.0)
    }

    /// Creates a standard 4K60 RGBA format.
    pub fn uhd_4k60_rgba() -> Self {
        Self::new(3840, 2160, PixelFormat::RGBA8, 60.0)
    }

    /// Returns the total number of pixels.
    pub fn pixel_count(&self) -> usize {
        (self.width * self.height) as usize
    }

    /// Returns the buffer size needed for a single frame.
    pub fn buffer_size(&self) -> usize {
        self.pixel_format.buffer_size(self.width, self.height)
    }

    /// Returns the frame duration based on the frame rate.
    pub fn frame_duration(&self) -> Duration {
        Duration::from_secs_f32(1.0 / self.frame_rate)
    }

    /// Returns the aspect ratio (width / height).
    pub fn aspect_ratio(&self) -> f32 {
        self.width as f32 / self.height as f32
    }

    /// Returns true if this is an HD format (1280x720 or higher).
    pub fn is_hd(&self) -> bool {
        self.width >= 1280 && self.height >= 720
    }

    /// Returns true if this is a 4K/UHD format (3840x2160 or higher).
    pub fn is_4k(&self) -> bool {
        self.width >= 3840 && self.height >= 2160
    }
}

impl std::fmt::Display for VideoFormat {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        write!(
            f,
            "{}x{} @ {}fps ({})",
            self.width, self.height, self.frame_rate, self.pixel_format
        )
    }
}

/// Frame metadata.
///
/// Additional information about a video frame beyond the raw pixel data.
#[derive(Debug, Clone, Default, Serialize, Deserialize)]
pub struct FrameMetadata {
    /// Frame number (sequential counter)
    pub frame_number: u64,
    /// Source name (e.g., "NDI Camera 1", "DeckLink Input")
    pub source_name: String,
    /// Timecode (if available)
    pub timecode: Option<String>,
    /// Whether this frame is dropped/late
    pub is_dropped: bool,
    /// Custom metadata key-value pairs
    pub custom: std::collections::HashMap<String, String>,
}

impl FrameMetadata {
    /// Creates new metadata with default values.
    pub fn new() -> Self {
        Self::default()
    }

    /// Creates metadata with a source name.
    pub fn with_source(source_name: impl Into<String>) -> Self {
        Self {
            source_name: source_name.into(),
            ..Default::default()
        }
    }

    /// Sets the frame number.
    pub fn with_frame_number(mut self, frame_number: u64) -> Self {
        self.frame_number = frame_number;
        self
    }

    /// Sets the timecode.
    pub fn with_timecode(mut self, timecode: impl Into<String>) -> Self {
        self.timecode = Some(timecode.into());
        self
    }

    /// Adds custom metadata.
    pub fn add_custom(&mut self, key: impl Into<String>, value: impl Into<String>) {
        self.custom.insert(key.into(), value.into());
    }
}

/// Video frame data.
///
/// Represents a complete video frame with pixel data, format information,
/// timestamp, and metadata.
#[derive(Debug, Clone)]
pub struct VideoFrame {
    /// Raw pixel data
    pub data: Vec<u8>,
    /// Video format
    pub format: VideoFormat,
    /// Timestamp (time since source started)
    pub timestamp: Duration,
    /// Additional metadata
    pub metadata: FrameMetadata,
}

impl VideoFrame {
    /// Creates a new video frame.
    pub fn new(data: Vec<u8>, format: VideoFormat, timestamp: Duration) -> Self {
        Self {
            data,
            format,
            timestamp,
            metadata: FrameMetadata::default(),
        }
    }

    /// Creates a new video frame with metadata.
    pub fn with_metadata(
        data: Vec<u8>,
        format: VideoFormat,
        timestamp: Duration,
        metadata: FrameMetadata,
    ) -> Self {
        Self {
            data,
            format,
            timestamp,
            metadata,
        }
    }

    /// Creates an empty frame (black) with the given format.
    pub fn empty(format: VideoFormat) -> Self {
        let size = format.buffer_size();
        Self {
            data: vec![0; size],
            format,
            timestamp: Duration::ZERO,
            metadata: FrameMetadata::default(),
        }
    }

    /// Validates that the frame data size matches the format.
    pub fn validate(&self) -> Result<(), crate::error::IoError> {
        let expected = self.format.buffer_size();
        let actual = self.data.len();
        if expected != actual {
            return Err(crate::error::IoError::FrameSizeMismatch { expected, actual });
        }
        Ok(())
    }

    /// Returns the frame size in bytes.
    pub fn size(&self) -> usize {
        self.data.len()
    }

    /// Returns true if the frame data is valid.
    pub fn is_valid(&self) -> bool {
        self.validate().is_ok()
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_pixel_format_bytes_per_pixel() {
        assert_eq!(PixelFormat::RGBA8.bytes_per_pixel(), 4);
        assert_eq!(PixelFormat::BGRA8.bytes_per_pixel(), 4);
        assert_eq!(PixelFormat::RGB8.bytes_per_pixel(), 3);
    }

    #[test]
    fn test_pixel_format_buffer_size() {
        let size = PixelFormat::RGBA8.buffer_size(1920, 1080);
        assert_eq!(size, 1920 * 1080 * 4);

        let yuv420_size = PixelFormat::YUV420P.buffer_size(1920, 1080);
        assert_eq!(yuv420_size, (1920 * 1080 * 3) / 2);
    }

    #[test]
    fn test_pixel_format_is_yuv() {
        assert!(PixelFormat::YUV420P.is_yuv());
        assert!(PixelFormat::YUV422P.is_yuv());
        assert!(!PixelFormat::RGBA8.is_yuv());
    }

    #[test]
    fn test_video_format_creation() {
        let format = VideoFormat::hd_1080p60_rgba();
        assert_eq!(format.width, 1920);
        assert_eq!(format.height, 1080);
        assert_eq!(format.frame_rate, 60.0);
        assert_eq!(format.pixel_format, PixelFormat::RGBA8);
    }

    #[test]
    fn test_video_format_buffer_size() {
        let format = VideoFormat::hd_1080p60_rgba();
        assert_eq!(format.buffer_size(), 1920 * 1080 * 4);
    }

    #[test]
    fn test_video_format_frame_duration() {
        let format = VideoFormat::hd_1080p60_rgba();
        let duration = format.frame_duration();
        assert!((duration.as_secs_f32() - 1.0 / 60.0).abs() < 0.0001);
    }

    #[test]
    fn test_video_format_aspect_ratio() {
        let format = VideoFormat::hd_1080p60_rgba();
        assert!((format.aspect_ratio() - 16.0 / 9.0).abs() < 0.0001);
    }

    #[test]
    fn test_video_format_is_hd() {
        assert!(VideoFormat::hd_1080p60_rgba().is_hd());
        assert!(VideoFormat::hd_720p60_rgba().is_hd());
        assert!(!VideoFormat::new(640, 480, PixelFormat::RGBA8, 30.0).is_hd());
    }

    #[test]
    fn test_video_format_is_4k() {
        assert!(VideoFormat::uhd_4k60_rgba().is_4k());
        assert!(!VideoFormat::hd_1080p60_rgba().is_4k());
    }

    #[test]
    fn test_frame_metadata() {
        let mut metadata = FrameMetadata::with_source("Test Source")
            .with_frame_number(42)
            .with_timecode("00:01:23:15");

        assert_eq!(metadata.source_name, "Test Source");
        assert_eq!(metadata.frame_number, 42);
        assert_eq!(metadata.timecode, Some("00:01:23:15".to_string()));

        metadata.add_custom("key", "value");
        assert_eq!(metadata.custom.get("key"), Some(&"value".to_string()));
    }

    #[test]
    fn test_video_frame_validation() {
        let format = VideoFormat::hd_1080p60_rgba();
        let size = format.buffer_size();

        // Valid frame
        let valid_frame = VideoFrame::new(vec![0; size], format.clone(), Duration::ZERO);
        assert!(valid_frame.is_valid());

        // Invalid frame (wrong size)
        let invalid_frame = VideoFrame::new(vec![0; 100], format, Duration::ZERO);
        assert!(!invalid_frame.is_valid());
    }

    #[test]
    fn test_video_frame_empty() {
        let format = VideoFormat::hd_1080p60_rgba();
        let frame = VideoFrame::empty(format.clone());
        assert_eq!(frame.size(), format.buffer_size());
        assert!(frame.is_valid());
    }
}
