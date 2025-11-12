//! NDI (Network Device Interface) support.
//!
//! NDI is a protocol for sending and receiving high-quality video over IP networks.
//! This module provides NDI source and sink implementations.
//!
//! # Note
//!
//! NDI support requires the NDI SDK from NewTek/Vizrt, which is not included.
//! This is currently a stub implementation showing the intended API.
//!
//! To enable full NDI support:
//! 1. Download the NDI SDK from https://www.ndi.tv/sdk/
//! 2. Install the SDK on your system
//! 3. Create FFI bindings using bindgen
//! 4. Implement the receiver and sender

#[cfg(feature = "ndi")]
use crate::error::{IoError, Result};
#[cfg(feature = "ndi")]
use crate::format::{VideoFormat, VideoFrame};
#[cfg(feature = "ndi")]
use crate::sink::VideoSink;
#[cfg(feature = "ndi")]
use crate::source::VideoSource;

/// Information about an NDI source on the network.
#[cfg(feature = "ndi")]
#[derive(Debug, Clone)]
pub struct NdiSource {
    /// Source name
    pub name: String,
    /// Source URL/address
    pub url: String,
}

/// NDI receiver for capturing video from NDI sources.
///
/// # Note
///
/// This is a stub implementation. Full implementation requires the NDI SDK.
#[cfg(feature = "ndi")]
pub struct NdiReceiver {
    source_name: String,
    format: VideoFormat,
    frame_count: u64,
}

#[cfg(feature = "ndi")]
impl NdiReceiver {
    /// Creates a new NDI receiver.
    pub fn new() -> Result<Self> {
        Err(IoError::NdiError(
            "NDI SDK not available. Please install the NDI SDK and rebuild with proper bindings."
                .to_string(),
        ))
    }

    /// Discovers available NDI sources on the network.
    ///
    /// # Parameters
    ///
    /// - `timeout_ms` - Time to wait for sources to appear (in milliseconds)
    pub fn discover_sources(_timeout_ms: u32) -> Result<Vec<NdiSource>> {
        Err(IoError::NdiError(
            "NDI SDK not available. Please install the NDI SDK.".to_string(),
        ))
    }

    /// Connects to a specific NDI source.
    pub fn connect(&mut self, _source: &NdiSource) -> Result<()> {
        Err(IoError::NdiError("NDI SDK not available".to_string()))
    }
}

#[cfg(feature = "ndi")]
impl VideoSource for NdiReceiver {
    fn name(&self) -> &str {
        &self.source_name
    }

    fn format(&self) -> VideoFormat {
        self.format.clone()
    }

    fn receive_frame(&mut self) -> Result<VideoFrame> {
        Err(IoError::NdiError("NDI SDK not available".to_string()))
    }

    fn is_available(&self) -> bool {
        false
    }

    fn frame_count(&self) -> u64 {
        self.frame_count
    }
}

/// NDI sender for broadcasting video to the network.
///
/// # Note
///
/// This is a stub implementation. Full implementation requires the NDI SDK.
#[cfg(feature = "ndi")]
pub struct NdiSender {
    name: String,
    format: VideoFormat,
    frame_count: u64,
}

#[cfg(feature = "ndi")]
impl NdiSender {
    /// Creates a new NDI sender.
    ///
    /// # Parameters
    ///
    /// - `name` - The name of this NDI source (visible to other devices)
    /// - `format` - The video format to send
    pub fn new(_name: impl Into<String>, _format: VideoFormat) -> Result<Self> {
        Err(IoError::NdiSenderFailed(
            "NDI SDK not available. Please install the NDI SDK and rebuild with proper bindings."
                .to_string(),
        ))
    }
}

#[cfg(feature = "ndi")]
impl VideoSink for NdiSender {
    fn name(&self) -> &str {
        &self.name
    }

    fn format(&self) -> VideoFormat {
        self.format.clone()
    }

    fn send_frame(&mut self, _frame: &VideoFrame) -> Result<()> {
        Err(IoError::NdiError("NDI SDK not available".to_string()))
    }

    fn is_available(&self) -> bool {
        false
    }

    fn frame_count(&self) -> u64 {
        self.frame_count
    }
}

// Stub types when NDI feature is disabled
/// NDI receiver (stub implementation when feature is disabled)
#[cfg(not(feature = "ndi"))]
pub struct NdiReceiver;

#[cfg(not(feature = "ndi"))]
impl NdiReceiver {
    /// Create a new NDI receiver (returns error when feature is disabled)
    pub fn new() -> crate::error::Result<Self> {
        Err(crate::error::IoError::feature_not_enabled("NDI", "ndi"))
    }

    /// Discover available NDI sources (returns error when feature is disabled)
    pub fn discover_sources(_timeout_ms: u32) -> crate::error::Result<Vec<NdiSource>> {
        Err(crate::error::IoError::feature_not_enabled("NDI", "ndi"))
    }
}

/// NDI sender (stub implementation when feature is disabled)
#[cfg(not(feature = "ndi"))]
pub struct NdiSender;

#[cfg(not(feature = "ndi"))]
impl NdiSender {
    /// Create a new NDI sender (returns error when feature is disabled)
    pub fn new(_name: impl Into<String>, _format: crate::format::VideoFormat) -> crate::error::Result<Self> {
        Err(crate::error::IoError::feature_not_enabled("NDI", "ndi"))
    }
}

/// NDI source information
#[cfg(not(feature = "ndi"))]
#[derive(Debug, Clone)]
pub struct NdiSource {
    /// NDI source name
    pub name: String,
    /// NDI source URL
    pub url: String,
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_ndi_receiver_unavailable() {
        let result = NdiReceiver::new();
        assert!(result.is_err());
    }

    #[test]
    fn test_ndi_sender_unavailable() {
        #[cfg(feature = "ndi")]
        {
            let format = VideoFormat::hd_1080p60_rgba();
            let result = NdiSender::new("Test", format);
            assert!(result.is_err());
        }
    }

    #[test]
    fn test_ndi_discover_unavailable() {
        let result = NdiReceiver::discover_sources(1000);
        assert!(result.is_err());
    }
}
