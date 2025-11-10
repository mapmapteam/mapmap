//! Shader compilation and management

use std::sync::Arc;

/// Handle to a compiled shader
#[derive(Clone)]
pub struct ShaderHandle {
    pub id: u64,
    pub module: Arc<wgpu::ShaderModule>,
}

/// Shader source types
#[derive(Debug, Clone)]
pub enum ShaderSource {
    Wgsl(String),
}

impl ShaderSource {
    /// Load shader from a file path
    pub fn from_wgsl_file(path: &std::path::Path) -> std::io::Result<Self> {
        let source = std::fs::read_to_string(path)?;
        Ok(Self::Wgsl(source))
    }

    /// Create from WGSL string
    pub fn from_wgsl(source: impl Into<String>) -> Self {
        Self::Wgsl(source.into())
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_shader_source_creation() {
        let source = ShaderSource::from_wgsl("@vertex fn main() {}");
        match source {
            ShaderSource::Wgsl(code) => assert!(code.contains("@vertex")),
            _ => panic!("Expected WGSL source"),
        }
    }
}
