//! DeckLink SDI/HDMI capture and output support.
//!
//! DeckLink devices from Blackmagic Design provide professional SDI and HDMI I/O.
//! This module provides DeckLink input and output implementations.
//!
//! # Note
//!
//! DeckLink support requires the Blackmagic DeckLink SDK, which is not included.
//! This is currently a stub implementation showing the intended API.
//!
//! To enable full DeckLink support:
//! 1. Download the DeckLink SDK from https://www.blackmagicdesign.com/developer/
//! 2. Install the SDK on your system
//! 3. Create FFI bindings (COM on Windows, Objective-C on macOS)
//! 4. Implement the input and output classes

#[cfg(feature = "decklink")]
use crate::error::{IoError, Result};
#[cfg(feature = "decklink")]
use crate::format::{VideoFormat, VideoFrame};
#[cfg(feature = "decklink")]
use crate::sink::VideoSink;
#[cfg(feature = "decklink")]
use crate::source::VideoSource;

/// Information about a DeckLink device.
#[cfg(feature = "decklink")]
#[derive(Debug, Clone)]
pub struct DeckLinkDevice {
    /// Device name
    pub name: String,
    /// Device index
    pub index: usize,
    /// Model name
    pub model: String,
}

/// DeckLink input for capturing video from SDI/HDMI.
///
/// # Note
///
/// This is a stub implementation. Full implementation requires the DeckLink SDK.
#[cfg(feature = "decklink")]
pub struct DeckLinkInput {
    device_name: String,
    format: VideoFormat,
    frame_count: u64,
}

#[cfg(feature = "decklink")]
impl DeckLinkInput {
    /// Creates a new DeckLink input.
    ///
    /// # Parameters
    ///
    /// - `device_index` - Index of the DeckLink device to use
    pub fn new(_device_index: usize) -> Result<Self> {
        Err(IoError::DeckLinkSdkNotAvailable)
    }

    /// Enumerates available DeckLink devices.
    pub fn enumerate_devices() -> Result<Vec<DeckLinkDevice>> {
        Err(IoError::DeckLinkSdkNotAvailable)
    }

    /// Starts capture with the specified format.
    pub fn start_capture(&mut self, _format: VideoFormat) -> Result<()> {
        Err(IoError::DeckLinkSdkNotAvailable)
    }

    /// Stops capture.
    pub fn stop_capture(&mut self) -> Result<()> {
        Ok(())
    }
}

#[cfg(feature = "decklink")]
impl VideoSource for DeckLinkInput {
    fn name(&self) -> &str {
        &self.device_name
    }

    fn format(&self) -> VideoFormat {
        self.format.clone()
    }

    fn receive_frame(&mut self) -> Result<VideoFrame> {
        Err(IoError::DeckLinkSdkNotAvailable)
    }

    fn is_available(&self) -> bool {
        false
    }

    fn frame_count(&self) -> u64 {
        self.frame_count
    }
}

/// DeckLink output for sending video to SDI/HDMI.
///
/// # Note
///
/// This is a stub implementation. Full implementation requires the DeckLink SDK.
#[cfg(feature = "decklink")]
pub struct DeckLinkOutput {
    device_name: String,
    format: VideoFormat,
    frame_count: u64,
}

#[cfg(feature = "decklink")]
impl DeckLinkOutput {
    /// Creates a new DeckLink output.
    ///
    /// # Parameters
    ///
    /// - `device_index` - Index of the DeckLink device to use
    /// - `format` - Video format to output
    pub fn new(_device_index: usize, _format: VideoFormat) -> Result<Self> {
        Err(IoError::DeckLinkSdkNotAvailable)
    }

    /// Starts output with the specified format.
    pub fn start_output(&mut self) -> Result<()> {
        Err(IoError::DeckLinkSdkNotAvailable)
    }

    /// Stops output.
    pub fn stop_output(&mut self) -> Result<()> {
        Ok(())
    }
}

#[cfg(feature = "decklink")]
impl VideoSink for DeckLinkOutput {
    fn name(&self) -> &str {
        &self.device_name
    }

    fn format(&self) -> VideoFormat {
        self.format.clone()
    }

    fn send_frame(&mut self, _frame: &VideoFrame) -> Result<()> {
        Err(IoError::DeckLinkSdkNotAvailable)
    }

    fn is_available(&self) -> bool {
        false
    }

    fn frame_count(&self) -> u64 {
        self.frame_count
    }
}

// Stub types when decklink feature is disabled
/// DeckLink input (stub implementation when feature is disabled)
#[cfg(not(feature = "decklink"))]
pub struct DeckLinkInput;

#[cfg(not(feature = "decklink"))]
impl DeckLinkInput {
    /// Create a new DeckLink input (returns error when feature is disabled)
    pub fn new(_device_index: usize) -> crate::error::Result<Self> {
        Err(crate::error::IoError::feature_not_enabled("DeckLink", "decklink"))
    }

    /// Enumerate DeckLink devices (returns error when feature is disabled)
    pub fn enumerate_devices() -> crate::error::Result<Vec<DeckLinkDevice>> {
        Err(crate::error::IoError::feature_not_enabled("DeckLink", "decklink"))
    }
}

/// DeckLink output (stub implementation when feature is disabled)
#[cfg(not(feature = "decklink"))]
pub struct DeckLinkOutput;

#[cfg(not(feature = "decklink"))]
impl DeckLinkOutput {
    /// Create a new DeckLink output (returns error when feature is disabled)
    pub fn new(_device_index: usize, _format: crate::format::VideoFormat) -> crate::error::Result<Self> {
        Err(crate::error::IoError::feature_not_enabled("DeckLink", "decklink"))
    }
}

/// DeckLink device information
#[cfg(not(feature = "decklink"))]
#[derive(Debug, Clone)]
pub struct DeckLinkDevice {
    /// Device name
    pub name: String,
    /// Device index
    pub index: usize,
    /// Device model
    pub model: String,
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_decklink_input_unavailable() {
        let result = DeckLinkInput::new(0);
        assert!(result.is_err());
    }

    #[test]
    fn test_decklink_output_unavailable() {
        #[cfg(feature = "decklink")]
        {
            let format = VideoFormat::hd_1080p60_rgba();
            let result = DeckLinkOutput::new(0, format);
            assert!(result.is_err());
        }
    }

    #[test]
    fn test_decklink_enumerate_unavailable() {
        let result = DeckLinkInput::enumerate_devices();
        assert!(result.is_err());
    }
}
