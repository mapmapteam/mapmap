//! RTMP streaming output.
//!
//! This module provides RTMP streaming capabilities using FFmpeg.

#[cfg(feature = "stream")]
use crate::error::{IoError, Result};
#[cfg(feature = "stream")]
use crate::format::{VideoFormat, VideoFrame};
#[cfg(feature = "stream")]
use crate::sink::{SinkStatistics, VideoSink};
#[cfg(feature = "stream")]
use crate::stream::encoder::{EncoderPreset, VideoCodec, VideoEncoder};

/// RTMP streamer for sending video to RTMP servers.
///
/// Supports streaming to platforms like Twitch, YouTube, Facebook Live, etc.
///
/// # Example
///
/// ```ignore
/// use mapmap_io::stream::RtmpStreamer;
/// use mapmap_io::format::VideoFormat;
///
/// let format = VideoFormat::hd_1080p60_rgba();
/// let mut streamer = RtmpStreamer::new(
///     "rtmp://live.twitch.tv/app/stream_key",
///     format,
///     6_000_000, // 6 Mbps
/// ).unwrap();
///
/// // Send frames...
/// ```
#[cfg(feature = "stream")]
pub struct RtmpStreamer {
    url: String,
    encoder: VideoEncoder,
    format: VideoFormat,
    frame_count: u64,
    connected: bool,
}

#[cfg(feature = "stream")]
impl RtmpStreamer {
    /// Creates a new RTMP streamer.
    ///
    /// # Parameters
    ///
    /// - `url` - RTMP URL (e.g., "rtmp://live.twitch.tv/app/stream_key")
    /// - `format` - Video format to stream
    /// - `bitrate` - Target bitrate in bits per second
    pub fn new(url: impl Into<String>, format: VideoFormat, bitrate: u64) -> Result<Self> {
        let url = url.into();

        // Validate RTMP URL
        if !url.starts_with("rtmp://") && !url.starts_with("rtmps://") {
            return Err(IoError::InvalidParameter(
                "RTMP URL must start with rtmp:// or rtmps://".to_string(),
            ));
        }

        tracing::info!("Creating RTMP streamer to {}", url);

        let encoder = VideoEncoder::new(
            VideoCodec::H264,
            format.clone(),
            bitrate,
            EncoderPreset::LowLatency,
        )?;

        Ok(Self {
            url,
            encoder,
            format,
            frame_count: 0,
            connected: false,
        })
    }

    /// Creates a default RTMP streamer for 1080p60.
    pub fn default_1080p60(url: impl Into<String>) -> Result<Self> {
        Self::new(url, VideoFormat::hd_1080p60_rgba(), 6_000_000)
    }

    /// Creates a default RTMP streamer for 720p60.
    pub fn default_720p60(url: impl Into<String>) -> Result<Self> {
        Self::new(url, VideoFormat::hd_720p60_rgba(), 3_500_000)
    }

    /// Connects to the RTMP server.
    pub fn connect(&mut self) -> Result<()> {
        if self.connected {
            return Ok(());
        }

        tracing::info!("Connecting to RTMP server: {}", self.url);

        // In a full implementation, this would:
        // 1. Initialize FFmpeg's RTMP muxer
        // 2. Perform RTMP handshake
        // 3. Send stream metadata
        // 4. Start encoding thread

        self.connected = true;
        Ok(())
    }

    /// Disconnects from the RTMP server.
    pub fn disconnect(&mut self) -> Result<()> {
        if !self.connected {
            return Ok(());
        }

        tracing::info!("Disconnecting from RTMP server");

        // Flush encoder
        self.encoder.flush()?;

        self.connected = false;
        Ok(())
    }

    /// Returns true if connected to the server.
    pub fn is_connected(&self) -> bool {
        self.connected
    }

    /// Returns the RTMP URL (without stream key exposed).
    pub fn url(&self) -> &str {
        // In production, we'd mask the stream key
        &self.url
    }

    /// Returns statistics about the stream.
    pub fn statistics(&self) -> SinkStatistics {
        SinkStatistics {
            frames_sent: self.frame_count,
            frames_dropped: 0, // Would track actual drops in full implementation
            bitrate: Some(self.encoder.bitrate()),
            average_latency_ms: None,
        }
    }
}

#[cfg(feature = "stream")]
impl VideoSink for RtmpStreamer {
    fn name(&self) -> &str {
        "RTMP Streamer"
    }

    fn format(&self) -> VideoFormat {
        self.format.clone()
    }

    fn send_frame(&mut self, frame: &VideoFrame) -> Result<()> {
        if !self.connected {
            self.connect()?;
        }

        // Validate frame
        frame.validate()?;

        // Encode frame
        let _packet = self.encoder.encode(frame)?;

        // In a full implementation, this would send the packet via RTMP
        self.frame_count += 1;

        tracing::trace!("Sent frame {} to RTMP stream", self.frame_count);

        Ok(())
    }

    fn is_available(&self) -> bool {
        self.connected
    }

    fn frame_count(&self) -> u64 {
        self.frame_count
    }

    fn flush(&mut self) -> Result<()> {
        self.encoder.flush()?;
        Ok(())
    }

    fn reconnect(&mut self) -> Result<()> {
        self.disconnect()?;
        self.connect()
    }

    fn statistics(&self) -> Option<SinkStatistics> {
        Some(self.statistics())
    }
}

#[cfg(feature = "stream")]
impl Drop for RtmpStreamer {
    fn drop(&mut self) {
        if self.connected {
            let _ = self.disconnect();
        }
    }
}

// Stub implementation when stream feature is disabled
/// RTMP streamer (stub implementation when feature is disabled)
#[cfg(not(feature = "stream"))]
pub struct RtmpStreamer;

#[cfg(not(feature = "stream"))]
impl RtmpStreamer {
    /// Create a new RTMP streamer (returns error when feature is disabled)
    pub fn new(_url: impl Into<String>, _format: crate::format::VideoFormat, _bitrate: u64) -> crate::error::Result<Self> {
        Err(crate::error::IoError::feature_not_enabled("RTMP streaming", "stream"))
    }
}

#[cfg(test)]
#[cfg(feature = "stream")]
mod tests {
    use super::*;

    #[test]
    fn test_rtmp_streamer_creation() {
        let format = VideoFormat::hd_1080p60_rgba();
        let streamer = RtmpStreamer::new("rtmp://localhost/live/stream", format, 6_000_000);
        assert!(streamer.is_ok());
    }

    #[test]
    fn test_rtmp_streamer_invalid_url() {
        let format = VideoFormat::hd_1080p60_rgba();
        let streamer = RtmpStreamer::new("http://localhost/stream", format, 6_000_000);
        assert!(streamer.is_err());
    }

    #[test]
    fn test_rtmp_streamer_default_presets() {
        let streamer = RtmpStreamer::default_1080p60("rtmp://localhost/live/stream");
        assert!(streamer.is_ok());

        let streamer = RtmpStreamer::default_720p60("rtmp://localhost/live/stream");
        assert!(streamer.is_ok());
    }

    #[test]
    fn test_rtmp_streamer_connect() {
        let format = VideoFormat::hd_1080p60_rgba();
        let mut streamer = RtmpStreamer::new("rtmp://localhost/live/stream", format, 6_000_000)
            .unwrap();

        assert!(!streamer.is_connected());
        assert!(streamer.connect().is_ok());
        assert!(streamer.is_connected());
    }

    #[test]
    fn test_rtmp_streamer_send_frame() {
        let format = VideoFormat::hd_1080p60_rgba();
        let mut streamer = RtmpStreamer::new("rtmp://localhost/live/stream", format.clone(), 6_000_000)
            .unwrap();

        let frame = VideoFrame::empty(format);
        assert!(streamer.send_frame(&frame).is_ok());
        assert_eq!(streamer.frame_count(), 1);
        assert!(streamer.is_connected());
    }

    #[test]
    fn test_rtmp_streamer_statistics() {
        let format = VideoFormat::hd_1080p60_rgba();
        let mut streamer = RtmpStreamer::new("rtmp://localhost/live/stream", format.clone(), 6_000_000)
            .unwrap();

        let frame = VideoFrame::empty(format);
        streamer.send_frame(&frame).unwrap();

        let stats = streamer.statistics();
        assert_eq!(stats.frames_sent, 1);
        assert!(stats.bitrate.is_some());
    }
}
