//! Syphon texture sharing support (macOS only).
//!
//! Syphon is a macOS-only system for sharing textures between applications
//! using IOSurface and OpenGL/Metal.
//!
//! # Note
//!
//! Syphon support requires the Syphon framework, which is not included.
//! This is currently a stub implementation showing the intended API.
//!
//! To enable full Syphon support:
//! 1. Download the Syphon framework from http://syphon.v002.info/
//! 2. Create FFI bindings using Objective-C interop
//! 3. Integrate with wgpu's Metal backend
//! 4. Implement server and client

#[cfg(all(feature = "syphon", target_os = "macos"))]
use crate::error::{IoError, Result};
#[cfg(all(feature = "syphon", target_os = "macos"))]
use crate::format::{VideoFormat, VideoFrame};
#[cfg(all(feature = "syphon", target_os = "macos"))]
use crate::sink::VideoSink;
#[cfg(all(feature = "syphon", target_os = "macos"))]
use crate::source::VideoSource;

/// Information about a Syphon server.
#[cfg(all(feature = "syphon", target_os = "macos"))]
#[derive(Debug, Clone)]
pub struct SyphonServerInfo {
    /// Server name
    pub name: String,
    /// Application name
    pub app_name: String,
    /// Video format
    pub format: VideoFormat,
}

/// Syphon client for receiving textures from other applications.
///
/// # Note
///
/// This is a stub implementation. Full implementation requires the Syphon framework.
#[cfg(all(feature = "syphon", target_os = "macos"))]
pub struct SyphonClient {
    name: String,
    format: VideoFormat,
    frame_count: u64,
}

#[cfg(all(feature = "syphon", target_os = "macos"))]
impl SyphonClient {
    /// Creates a new Syphon client.
    pub fn new() -> Result<Self> {
        Err(IoError::SyphonInitFailed)
    }

    /// Lists available Syphon servers.
    pub fn list_servers() -> Result<Vec<SyphonServerInfo>> {
        Err(IoError::SyphonInitFailed)
    }

    /// Connects to a specific Syphon server.
    pub fn connect(&mut self, _server_name: &str, _app_name: &str) -> Result<()> {
        Err(IoError::SyphonInitFailed)
    }
}

#[cfg(all(feature = "syphon", target_os = "macos"))]
impl VideoSource for SyphonClient {
    fn name(&self) -> &str {
        &self.name
    }

    fn format(&self) -> VideoFormat {
        self.format.clone()
    }

    fn receive_frame(&mut self) -> Result<VideoFrame> {
        Err(IoError::SyphonError("Syphon framework not available".to_string()))
    }

    fn is_available(&self) -> bool {
        false
    }

    fn frame_count(&self) -> u64 {
        self.frame_count
    }
}

/// Syphon server for sharing textures with other applications.
///
/// # Note
///
/// This is a stub implementation. Full implementation requires the Syphon framework.
#[cfg(all(feature = "syphon", target_os = "macos"))]
pub struct SyphonServer {
    name: String,
    format: VideoFormat,
    frame_count: u64,
}

#[cfg(all(feature = "syphon", target_os = "macos"))]
impl SyphonServer {
    /// Creates a new Syphon server.
    ///
    /// # Parameters
    ///
    /// - `name` - Name of this Syphon server (visible to clients)
    /// - `format` - Video format to share
    pub fn new(_name: impl Into<String>, _format: VideoFormat) -> Result<Self> {
        Err(IoError::SyphonInitFailed)
    }
}

#[cfg(all(feature = "syphon", target_os = "macos"))]
impl VideoSink for SyphonServer {
    fn name(&self) -> &str {
        &self.name
    }

    fn format(&self) -> VideoFormat {
        self.format.clone()
    }

    fn send_frame(&mut self, _frame: &VideoFrame) -> Result<()> {
        Err(IoError::SyphonError("Syphon framework not available".to_string()))
    }

    fn is_available(&self) -> bool {
        false
    }

    fn frame_count(&self) -> u64 {
        self.frame_count
    }
}

// Stub types for non-macOS platforms
/// Syphon client (stub implementation when feature is disabled or on non-macOS platforms)
#[cfg(not(all(feature = "syphon", target_os = "macos")))]
pub struct SyphonClient;

#[cfg(not(all(feature = "syphon", target_os = "macos")))]
impl SyphonClient {
    /// Create a new Syphon client (returns error when feature is disabled or on non-macOS platforms)
    pub fn new() -> crate::error::Result<Self> {
        #[cfg(not(target_os = "macos"))]
        return Err(crate::error::IoError::platform_not_supported("Syphon is only available on macOS"));

        #[cfg(target_os = "macos")]
        Err(crate::error::IoError::feature_not_enabled("Syphon", "syphon"))
    }

    /// List available Syphon servers (returns error when feature is disabled or on non-macOS platforms)
    pub fn list_servers() -> crate::error::Result<Vec<SyphonServerInfo>> {
        #[cfg(not(target_os = "macos"))]
        return Err(crate::error::IoError::platform_not_supported("Syphon is only available on macOS"));

        #[cfg(target_os = "macos")]
        Err(crate::error::IoError::feature_not_enabled("Syphon", "syphon"))
    }
}

/// Syphon server (stub implementation when feature is disabled or on non-macOS platforms)
#[cfg(not(all(feature = "syphon", target_os = "macos")))]
pub struct SyphonServer;

#[cfg(not(all(feature = "syphon", target_os = "macos")))]
impl SyphonServer {
    /// Create a new Syphon server (returns error when feature is disabled or on non-macOS platforms)
    pub fn new(_name: impl Into<String>, _format: crate::format::VideoFormat) -> crate::error::Result<Self> {
        #[cfg(not(target_os = "macos"))]
        return Err(crate::error::IoError::platform_not_supported("Syphon is only available on macOS"));

        #[cfg(target_os = "macos")]
        Err(crate::error::IoError::feature_not_enabled("Syphon", "syphon"))
    }
}

/// Syphon server information
#[cfg(not(all(feature = "syphon", target_os = "macos")))]
#[derive(Debug, Clone)]
pub struct SyphonServerInfo {
    /// Server name
    pub name: String,
    /// Application name hosting the server
    pub app_name: String,
    /// Video format
    pub format: crate::format::VideoFormat,
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_syphon_client_unavailable() {
        let result = SyphonClient::new();
        assert!(result.is_err());
    }

    #[test]
    fn test_syphon_server_unavailable() {
        #[cfg(all(feature = "syphon", target_os = "macos"))]
        {
            let format = VideoFormat::hd_1080p60_rgba();
            let result = SyphonServer::new("Test", format);
            assert!(result.is_err());
        }
    }

    #[test]
    fn test_syphon_list_servers_unavailable() {
        let result = SyphonClient::list_servers();
        assert!(result.is_err());
    }
}
