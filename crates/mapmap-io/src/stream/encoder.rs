//! Video encoding using FFmpeg.
//!
//! This module provides video encoding capabilities for streaming using FFmpeg.

#[cfg(feature = "stream")]
use crate::error::{IoError, Result};
#[cfg(feature = "stream")]
use crate::format::{PixelFormat, VideoFormat, VideoFrame};

/// Video codec enumeration.
#[cfg(feature = "stream")]
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum VideoCodec {
    /// H.264 (AVC)
    H264,
    /// H.265 (HEVC)
    H265,
    /// VP8
    VP8,
    /// VP9
    VP9,
}

#[cfg(feature = "stream")]
impl VideoCodec {
    /// Returns the codec name for FFmpeg.
    pub fn ffmpeg_name(&self) -> &'static str {
        match self {
            VideoCodec::H264 => "libx264",
            VideoCodec::H265 => "libx265",
            VideoCodec::VP8 => "libvpx",
            VideoCodec::VP9 => "libvpx-vp9",
        }
    }
}

/// Encoder preset for quality/speed tradeoff.
#[cfg(feature = "stream")]
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum EncoderPreset {
    /// Ultra fast encoding, lower quality
    UltraFast,
    /// Super fast encoding
    SuperFast,
    /// Very fast encoding
    VeryFast,
    /// Fast encoding
    Fast,
    /// Medium speed/quality (default)
    Medium,
    /// Slow encoding, better quality
    Slow,
    /// Very slow encoding, best quality
    VerySlow,
    /// Low latency preset for streaming
    LowLatency,
}

#[cfg(feature = "stream")]
impl EncoderPreset {
    /// Returns the preset name for FFmpeg.
    pub fn ffmpeg_name(&self) -> &'static str {
        match self {
            EncoderPreset::UltraFast => "ultrafast",
            EncoderPreset::SuperFast => "superfast",
            EncoderPreset::VeryFast => "veryfast",
            EncoderPreset::Fast => "fast",
            EncoderPreset::Medium => "medium",
            EncoderPreset::Slow => "slow",
            EncoderPreset::VerySlow => "veryslow",
            EncoderPreset::LowLatency => "ultrafast", // Use ultrafast for low latency
        }
    }
}

/// Video encoder using FFmpeg.
///
/// Encodes raw video frames into compressed video packets suitable for streaming.
#[cfg(feature = "stream")]
pub struct VideoEncoder {
    codec: VideoCodec,
    format: VideoFormat,
    bitrate: u64,
    preset: EncoderPreset,
    frame_count: u64,
}

#[cfg(feature = "stream")]
impl VideoEncoder {
    /// Creates a new video encoder.
    ///
    /// # Parameters
    ///
    /// - `codec` - The video codec to use
    /// - `format` - The input video format
    /// - `bitrate` - Target bitrate in bits per second
    /// - `preset` - Encoder preset for quality/speed tradeoff
    pub fn new(
        codec: VideoCodec,
        format: VideoFormat,
        bitrate: u64,
        preset: EncoderPreset,
    ) -> Result<Self> {
        // In a full implementation, this would initialize FFmpeg encoder
        tracing::info!(
            "Creating video encoder: codec={:?}, format={}, bitrate={}, preset={:?}",
            codec,
            format,
            bitrate,
            preset
        );

        Ok(Self {
            codec,
            format,
            bitrate,
            preset,
            frame_count: 0,
        })
    }

    /// Creates a default H.264 encoder for 1080p60 streaming.
    pub fn default_h264_1080p60() -> Result<Self> {
        Self::new(
            VideoCodec::H264,
            VideoFormat::hd_1080p60_rgba(),
            6_000_000, // 6 Mbps
            EncoderPreset::LowLatency,
        )
    }

    /// Creates a default H.264 encoder for 720p60 streaming.
    pub fn default_h264_720p60() -> Result<Self> {
        Self::new(
            VideoCodec::H264,
            VideoFormat::hd_720p60_rgba(),
            3_500_000, // 3.5 Mbps
            EncoderPreset::LowLatency,
        )
    }

    /// Encodes a video frame into a compressed packet.
    ///
    /// # Parameters
    ///
    /// - `frame` - The raw video frame to encode
    ///
    /// # Returns
    ///
    /// An encoded video packet ready for streaming.
    pub fn encode(&mut self, frame: &VideoFrame) -> Result<EncodedPacket> {
        // Validate frame format matches encoder format
        if frame.format.pixel_format != self.format.pixel_format {
            return Err(IoError::ConversionError(format!(
                "Frame format {} doesn't match encoder format {}",
                frame.format.pixel_format, self.format.pixel_format
            )));
        }

        // In a full implementation, this would use FFmpeg to encode the frame
        self.frame_count += 1;

        tracing::trace!(
            "Encoding frame {}: timestamp={:?}",
            self.frame_count,
            frame.timestamp
        );

        // Return a stub packet
        Ok(EncodedPacket {
            data: Vec::new(), // Would contain actual encoded data
            pts: self.frame_count as i64,
            dts: self.frame_count as i64,
            is_keyframe: self.frame_count % 60 == 0, // Keyframe every 2 seconds at 30fps
        })
    }

    /// Flushes any buffered frames from the encoder.
    ///
    /// Should be called before closing the encoder to ensure all frames are encoded.
    pub fn flush(&mut self) -> Result<Vec<EncodedPacket>> {
        tracing::debug!("Flushing encoder, {} frames encoded", self.frame_count);
        Ok(Vec::new())
    }

    /// Returns the number of frames encoded.
    pub fn frame_count(&self) -> u64 {
        self.frame_count
    }

    /// Returns the codec being used.
    pub fn codec(&self) -> VideoCodec {
        self.codec
    }

    /// Returns the encoder format.
    pub fn format(&self) -> &VideoFormat {
        &self.format
    }

    /// Returns the target bitrate.
    pub fn bitrate(&self) -> u64 {
        self.bitrate
    }
}

/// An encoded video packet.
#[cfg(feature = "stream")]
#[derive(Debug, Clone)]
pub struct EncodedPacket {
    /// Compressed packet data
    pub data: Vec<u8>,
    /// Presentation timestamp
    pub pts: i64,
    /// Decode timestamp
    pub dts: i64,
    /// Whether this is a keyframe
    pub is_keyframe: bool,
}

#[cfg(feature = "stream")]
impl EncodedPacket {
    /// Returns the packet size in bytes.
    pub fn size(&self) -> usize {
        self.data.len()
    }
}

// Stub implementations when stream feature is disabled
/// Video encoder (stub implementation when feature is disabled)
#[cfg(not(feature = "stream"))]
pub struct VideoEncoder;

#[cfg(not(feature = "stream"))]
impl VideoEncoder {
    /// Create a new video encoder (returns error when feature is disabled)
    pub fn new() -> crate::error::Result<Self> {
        Err(crate::error::IoError::feature_not_enabled("Stream encoding", "stream"))
    }
}

#[cfg(test)]
#[cfg(feature = "stream")]
mod tests {
    use super::*;
    use std::time::Duration;

    #[test]
    fn test_video_codec_names() {
        assert_eq!(VideoCodec::H264.ffmpeg_name(), "libx264");
        assert_eq!(VideoCodec::H265.ffmpeg_name(), "libx265");
    }

    #[test]
    fn test_encoder_preset_names() {
        assert_eq!(EncoderPreset::UltraFast.ffmpeg_name(), "ultrafast");
        assert_eq!(EncoderPreset::Medium.ffmpeg_name(), "medium");
        assert_eq!(EncoderPreset::LowLatency.ffmpeg_name(), "ultrafast");
    }

    #[test]
    fn test_video_encoder_creation() {
        let encoder = VideoEncoder::default_h264_1080p60();
        assert!(encoder.is_ok());

        let encoder = encoder.unwrap();
        assert_eq!(encoder.codec(), VideoCodec::H264);
        assert_eq!(encoder.frame_count(), 0);
    }

    #[test]
    fn test_video_encoder_encode() {
        let mut encoder = VideoEncoder::default_h264_1080p60().unwrap();
        let format = VideoFormat::hd_1080p60_rgba();
        let frame = VideoFrame::empty(format);

        let packet = encoder.encode(&frame);
        assert!(packet.is_ok());
        assert_eq!(encoder.frame_count(), 1);
    }

    #[test]
    fn test_video_encoder_keyframe() {
        let mut encoder = VideoEncoder::default_h264_1080p60().unwrap();
        let format = VideoFormat::hd_1080p60_rgba();

        // First frame should be keyframe
        let frame = VideoFrame::empty(format.clone());
        let packet = encoder.encode(&frame).unwrap();
        assert!(!packet.is_keyframe); // Actually frame 1, so not keyframe

        // Encode 59 more frames
        for _ in 0..59 {
            let frame = VideoFrame::empty(format.clone());
            encoder.encode(&frame).unwrap();
        }

        // Frame 60 should be keyframe
        let frame = VideoFrame::empty(format.clone());
        let packet = encoder.encode(&frame).unwrap();
        assert!(packet.is_keyframe);
    }

    #[test]
    fn test_video_encoder_wrong_format() {
        let mut encoder = VideoEncoder::default_h264_1080p60().unwrap();
        let wrong_format = VideoFormat::new(1920, 1080, PixelFormat::YUV420P, 60.0);
        let frame = VideoFrame::empty(wrong_format);

        let result = encoder.encode(&frame);
        assert!(result.is_err());
    }
}
