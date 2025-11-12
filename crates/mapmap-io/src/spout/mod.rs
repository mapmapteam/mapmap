//! Spout texture sharing support (Windows only).
//!
//! Spout is a Windows-only system for sharing textures between applications
//! using DirectX shared resources.
//!
//! # Note
//!
//! Spout support requires the Spout SDK and DirectX 11, which are not included.
//! This is currently a stub implementation showing the intended API.
//!
//! To enable full Spout support:
//! 1. Download the Spout SDK from https://spout.zeal.co/
//! 2. Create FFI bindings for the Spout library
//! 3. Integrate with wgpu's DirectX 11 backend
//! 4. Implement sender and receiver

#[cfg(all(feature = "spout", target_os = "windows"))]
use crate::error::{IoError, Result};
#[cfg(all(feature = "spout", target_os = "windows"))]
use crate::format::{VideoFormat, VideoFrame};
#[cfg(all(feature = "spout", target_os = "windows"))]
use crate::sink::VideoSink;
#[cfg(all(feature = "spout", target_os = "windows"))]
use crate::source::VideoSource;

/// Information about a Spout sender.
#[cfg(all(feature = "spout", target_os = "windows"))]
#[derive(Debug, Clone)]
pub struct SpoutSenderInfo {
    /// Sender name
    pub name: String,
    /// Video format
    pub format: VideoFormat,
}

/// Spout receiver for receiving textures from other applications.
///
/// # Note
///
/// This is a stub implementation. Full implementation requires the Spout SDK.
#[cfg(all(feature = "spout", target_os = "windows"))]
pub struct SpoutReceiver {
    name: String,
    format: VideoFormat,
    frame_count: u64,
}

#[cfg(all(feature = "spout", target_os = "windows"))]
impl SpoutReceiver {
    /// Creates a new Spout receiver.
    pub fn new() -> Result<Self> {
        Err(IoError::SpoutInitFailed)
    }

    /// Lists available Spout senders.
    pub fn list_senders() -> Result<Vec<SpoutSenderInfo>> {
        Err(IoError::SpoutInitFailed)
    }

    /// Connects to a specific Spout sender.
    pub fn connect(&mut self, _sender_name: &str) -> Result<()> {
        Err(IoError::SpoutInitFailed)
    }
}

#[cfg(all(feature = "spout", target_os = "windows"))]
impl VideoSource for SpoutReceiver {
    fn name(&self) -> &str {
        &self.name
    }

    fn format(&self) -> VideoFormat {
        self.format.clone()
    }

    fn receive_frame(&mut self) -> Result<VideoFrame> {
        Err(IoError::SpoutError("Spout SDK not available".to_string()))
    }

    fn is_available(&self) -> bool {
        false
    }

    fn frame_count(&self) -> u64 {
        self.frame_count
    }
}

/// Spout sender for sharing textures with other applications.
///
/// # Note
///
/// This is a stub implementation. Full implementation requires the Spout SDK.
#[cfg(all(feature = "spout", target_os = "windows"))]
pub struct SpoutSender {
    name: String,
    format: VideoFormat,
    frame_count: u64,
}

#[cfg(all(feature = "spout", target_os = "windows"))]
impl SpoutSender {
    /// Creates a new Spout sender.
    ///
    /// # Parameters
    ///
    /// - `name` - Name of this Spout sender (visible to receivers)
    /// - `format` - Video format to share
    pub fn new(_name: impl Into<String>, _format: VideoFormat) -> Result<Self> {
        Err(IoError::SpoutInitFailed)
    }
}

#[cfg(all(feature = "spout", target_os = "windows"))]
impl VideoSink for SpoutSender {
    fn name(&self) -> &str {
        &self.name
    }

    fn format(&self) -> VideoFormat {
        self.format.clone()
    }

    fn send_frame(&mut self, _frame: &VideoFrame) -> Result<()> {
        Err(IoError::SpoutError("Spout SDK not available".to_string()))
    }

    fn is_available(&self) -> bool {
        false
    }

    fn frame_count(&self) -> u64 {
        self.frame_count
    }
}

// Stub types for non-Windows platforms
/// Spout receiver (stub implementation when feature is disabled or on non-Windows platforms)
#[cfg(not(all(feature = "spout", target_os = "windows")))]
pub struct SpoutReceiver;

#[cfg(not(all(feature = "spout", target_os = "windows")))]
impl SpoutReceiver {
    /// Create a new Spout receiver (returns error when feature is disabled or on non-Windows platforms)
    pub fn new() -> crate::error::Result<Self> {
        #[cfg(not(target_os = "windows"))]
        return Err(crate::error::IoError::platform_not_supported("Spout is only available on Windows"));

        #[cfg(target_os = "windows")]
        Err(crate::error::IoError::feature_not_enabled("Spout", "spout"))
    }

    /// List available Spout senders (returns error when feature is disabled or on non-Windows platforms)
    pub fn list_senders() -> crate::error::Result<Vec<SpoutSenderInfo>> {
        #[cfg(not(target_os = "windows"))]
        return Err(crate::error::IoError::platform_not_supported("Spout is only available on Windows"));

        #[cfg(target_os = "windows")]
        Err(crate::error::IoError::feature_not_enabled("Spout", "spout"))
    }
}

/// Spout sender (stub implementation when feature is disabled or on non-Windows platforms)
#[cfg(not(all(feature = "spout", target_os = "windows")))]
pub struct SpoutSender;

#[cfg(not(all(feature = "spout", target_os = "windows")))]
impl SpoutSender {
    /// Create a new Spout sender (returns error when feature is disabled or on non-Windows platforms)
    pub fn new(_name: impl Into<String>, _format: crate::format::VideoFormat) -> crate::error::Result<Self> {
        #[cfg(not(target_os = "windows"))]
        return Err(crate::error::IoError::platform_not_supported("Spout is only available on Windows"));

        #[cfg(target_os = "windows")]
        Err(crate::error::IoError::feature_not_enabled("Spout", "spout"))
    }
}

/// Spout sender information
#[cfg(not(all(feature = "spout", target_os = "windows")))]
#[derive(Debug, Clone)]
pub struct SpoutSenderInfo {
    /// Sender name
    pub name: String,
    /// Video format
    pub format: crate::format::VideoFormat,
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_spout_receiver_unavailable() {
        let result = SpoutReceiver::new();
        assert!(result.is_err());
    }

    #[test]
    fn test_spout_sender_unavailable() {
        #[cfg(all(feature = "spout", target_os = "windows"))]
        {
            let format = VideoFormat::hd_1080p60_rgba();
            let result = SpoutSender::new("Test", format);
            assert!(result.is_err());
        }
    }

    #[test]
    fn test_spout_list_senders_unavailable() {
        let result = SpoutReceiver::list_senders();
        assert!(result.is_err());
    }
}
