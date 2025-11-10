//! MapMap Render - Graphics Abstraction Layer
//!
//! This crate provides the rendering abstraction for MapMap, including:
//! - wgpu backend implementation
//! - Texture pool management
//! - Shader compilation and hot-reloading
//! - GPU profiling

use std::collections::HashMap;
use parking_lot::RwLock;
use std::sync::Arc;
use thiserror::Error;
use tracing::{debug, error, info, warn};
use wgpu::util::DeviceExt;

pub mod backend;
pub mod texture;
pub mod shader;
pub mod quad;

pub use backend::{RenderBackend, WgpuBackend};
pub use texture::{TextureHandle, TexturePool, TextureDescriptor};
pub use shader::{ShaderHandle, ShaderSource};
pub use quad::QuadRenderer;

/// Rendering errors
#[derive(Error, Debug)]
pub enum RenderError {
    #[error("Device error: {0}")]
    DeviceError(String),

    #[error("Shader compilation failed: {0}")]
    ShaderCompilation(String),

    #[error("Texture creation failed: {0}")]
    TextureCreation(String),

    #[error("Device lost")]
    DeviceLost,

    #[error("Surface error: {0}")]
    SurfaceError(String),
}

/// Result type for rendering operations
pub type Result<T> = std::result::Result<T, RenderError>;

/// Re-export commonly used wgpu types
pub use wgpu::{
    Device, Queue, Surface, SurfaceConfiguration, Texture, TextureView, TextureFormat,
    PresentMode, CompositeAlphaMode, TextureUsages, BufferUsages, CommandEncoder,
};
