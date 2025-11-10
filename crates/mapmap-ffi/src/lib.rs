//! MapMap FFI - Foreign Function Interface Bridge
//!
//! This crate provides FFI bindings to external SDKs including:
//! - NDI (Network Device Interface)
//! - DeckLink SDI
//! - Spout (Windows)
//! - Syphon (macOS)
//!
//! NOTE: This is a placeholder for Phase 0.
//! Full implementation will be completed in Phase 5.

use thiserror::Error;

/// FFI errors
#[derive(Error, Debug)]
pub enum FfiError {
    #[error("NDI error: {0}")]
    NdiError(String),

    #[error("DeckLink error: {0}")]
    DeckLinkError(String),

    #[error("Spout error: {0}")]
    SpoutError(String),

    #[error("Syphon error: {0}")]
    SyphonError(String),
}

/// Result type for FFI operations
pub type Result<T> = std::result::Result<T, FfiError>;

/// C-ABI plugin interface (placeholder)
#[repr(C)]
pub struct PluginApi {
    pub version: u32,
}

impl PluginApi {
    pub const VERSION: u32 = 1;

    pub fn new() -> Self {
        Self {
            version: Self::VERSION,
        }
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_plugin_api() {
        let api = PluginApi::new();
        assert_eq!(api.version, PluginApi::VERSION);
    }
}
