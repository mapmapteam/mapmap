//! Error types for video I/O operations.
//!
//! This module defines comprehensive error types for all video I/O operations
//! including NDI, DeckLink, Spout, Syphon, streaming, and format conversion.

/// Result type alias for video I/O operations.
pub type Result<T> = std::result::Result<T, IoError>;

/// Comprehensive error type for video I/O operations.
#[derive(Debug, thiserror::Error)]
pub enum IoError {
    /// Generic I/O error
    #[error("I/O error: {0}")]
    Io(#[from] std::io::Error),

    /// NDI-related errors
    #[error("NDI error: {0}")]
    NdiError(String),

    /// NDI initialization failed
    #[error("Failed to initialize NDI runtime")]
    NdiInitFailed,

    /// NDI source not found
    #[error("NDI source not found: {0}")]
    NdiSourceNotFound(String),

    /// NDI receiver creation failed
    #[error("Failed to create NDI receiver")]
    NdiReceiverFailed,

    /// NDI sender creation failed
    #[error("Failed to create NDI sender: {0}")]
    NdiSenderFailed(String),

    /// DeckLink-related errors
    #[error("DeckLink error: {0}")]
    DeckLinkError(String),

    /// DeckLink device not found
    #[error("DeckLink device not found")]
    DeckLinkDeviceNotFound,

    /// DeckLink SDK not available
    #[error("DeckLink SDK not available or not installed")]
    DeckLinkSdkNotAvailable,

    /// Spout-related errors (Windows only)
    #[cfg(target_os = "windows")]
    #[error("Spout error: {0}")]
    SpoutError(String),

    /// Spout initialization failed
    #[cfg(target_os = "windows")]
    #[error("Failed to initialize Spout")]
    SpoutInitFailed,

    /// Spout sender/receiver not found
    #[cfg(target_os = "windows")]
    #[error("Spout sender not found: {0}")]
    SpoutNotFound(String),

    /// Syphon-related errors (macOS only)
    #[cfg(target_os = "macos")]
    #[error("Syphon error: {0}")]
    SyphonError(String),

    /// Syphon initialization failed
    #[cfg(target_os = "macos")]
    #[error("Failed to initialize Syphon")]
    SyphonInitFailed,

    /// Syphon server/client not found
    #[cfg(target_os = "macos")]
    #[error("Syphon server not found: {0}")]
    SyphonNotFound(String),

    /// Streaming errors
    #[error("Stream error: {0}")]
    StreamError(String),

    /// Failed to initialize encoder
    #[error("Failed to initialize encoder: {0}")]
    EncoderInitFailed(String),

    /// Failed to encode frame
    #[error("Failed to encode frame: {0}")]
    EncodeFailed(String),

    /// Failed to connect to streaming server
    #[error("Failed to connect to streaming server: {0}")]
    StreamConnectionFailed(String),

    /// Stream disconnected
    #[error("Stream disconnected")]
    StreamDisconnected,

    /// RTMP-specific errors
    #[error("RTMP error: {0}")]
    RtmpError(String),

    /// SRT-specific errors
    #[error("SRT error: {0}")]
    SrtError(String),

    /// Format conversion errors
    #[error("Format conversion error: {0}")]
    ConversionError(String),

    /// Unsupported pixel format
    #[error("Unsupported pixel format: {0}")]
    UnsupportedPixelFormat(String),

    /// Unsupported video format
    #[error("Unsupported video format: {width}x{height} @ {fps}fps")]
    UnsupportedVideoFormat {
        /// Video width in pixels
        width: u32,
        /// Video height in pixels
        height: u32,
        /// Frames per second
        fps: f32,
    },

    /// Invalid frame data
    #[error("Invalid frame data: {0}")]
    InvalidFrameData(String),

    /// Frame size mismatch
    #[error("Frame size mismatch: expected {expected} bytes, got {actual} bytes")]
    FrameSizeMismatch {
        /// Expected frame size in bytes
        expected: usize,
        /// Actual frame size in bytes
        actual: usize,
    },

    /// Virtual camera errors
    #[error("Virtual camera error: {0}")]
    VirtualCameraError(String),

    /// Virtual camera not available
    #[error("Virtual camera not available on this platform")]
    VirtualCameraNotAvailable,

    /// No frame available
    #[error("No frame available")]
    NoFrameAvailable,

    /// Timeout waiting for frame
    #[error("Timeout waiting for frame")]
    FrameTimeout,

    /// Device not available
    #[error("Device not available: {0}")]
    DeviceNotAvailable(String),

    /// Feature not enabled
    #[error("Feature not enabled: {0}. Enable the '{1}' feature flag to use this functionality")]
    FeatureNotEnabled(String, String),

    /// Platform not supported
    #[error("Operation not supported on this platform: {0}")]
    PlatformNotSupported(String),

    /// Resource allocation failed
    #[error("Failed to allocate resource: {0}")]
    AllocationFailed(String),

    /// Invalid parameter
    #[error("Invalid parameter: {0}")]
    InvalidParameter(String),

    /// Other errors
    #[error("{0}")]
    Other(String),
}

impl IoError {
    /// Creates a new generic error with a custom message.
    pub fn other(msg: impl Into<String>) -> Self {
        Self::Other(msg.into())
    }

    /// Creates a feature not enabled error.
    pub fn feature_not_enabled(feature: &str, feature_flag: &str) -> Self {
        Self::FeatureNotEnabled(feature.to_string(), feature_flag.to_string())
    }

    /// Creates a platform not supported error.
    pub fn platform_not_supported(operation: &str) -> Self {
        Self::PlatformNotSupported(operation.to_string())
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_error_display() {
        let err = IoError::NdiError("test error".to_string());
        assert_eq!(err.to_string(), "NDI error: test error");
    }

    #[test]
    fn test_feature_not_enabled() {
        let err = IoError::feature_not_enabled("NDI", "ndi");
        assert!(err.to_string().contains("ndi"));
    }

    #[test]
    fn test_frame_size_mismatch() {
        let err = IoError::FrameSizeMismatch {
            expected: 1920 * 1080 * 4,
            actual: 1000,
        };
        let err_str = err.to_string();
        assert!(err_str.contains("expected"));
        assert!(err_str.contains("got"));
        assert!(err_str.contains("8294400"));
        assert!(err_str.contains("1000"));
    }
}
