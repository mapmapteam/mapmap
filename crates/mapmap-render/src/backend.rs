//! Rendering backend abstraction

use crate::{Result, RenderError, TextureHandle, TextureDescriptor, ShaderHandle, ShaderSource};
use std::sync::Arc;
use tracing::{debug, info};
use wgpu::util::StagingBelt;

/// Trait for rendering backends
pub trait RenderBackend: Send {
    fn device(&self) -> &wgpu::Device;
    fn queue(&self) -> &wgpu::Queue;
    fn create_texture(&mut self, desc: TextureDescriptor) -> Result<TextureHandle>;
    fn upload_texture(&mut self, handle: TextureHandle, data: &[u8]) -> Result<()>;
    fn create_shader(&mut self, source: ShaderSource) -> Result<ShaderHandle>;
}

/// wgpu-based rendering backend
pub struct WgpuBackend {
    pub instance: Arc<wgpu::Instance>,
    pub device: Arc<wgpu::Device>,
    pub queue: Arc<wgpu::Queue>,
    pub adapter_info: wgpu::AdapterInfo,
    staging_belt: StagingBelt,
    texture_counter: u64,
    shader_counter: u64,
}

impl WgpuBackend {
    /// Create a new wgpu backend
    pub async fn new() -> Result<Self> {
        Self::new_with_options(wgpu::Backends::all(), wgpu::PowerPreference::HighPerformance).await
    }

    /// Create a new wgpu backend with specific options
    pub async fn new_with_options(
        backends: wgpu::Backends,
        power_preference: wgpu::PowerPreference,
    ) -> Result<Self> {
        info!("Initializing wgpu backend");

        let instance = wgpu::Instance::new(wgpu::InstanceDescriptor {
            backends,
            ..Default::default()
        });

        let adapter = instance
            .request_adapter(&wgpu::RequestAdapterOptions {
                power_preference,
                compatible_surface: None,
                force_fallback_adapter: false,
            })
            .await
            .ok_or_else(|| RenderError::DeviceError("No suitable adapter found".to_string()))?;

        let adapter_info = adapter.get_info();
        info!(
            "Selected adapter: {} ({:?})",
            adapter_info.name, adapter_info.backend
        );

        let (device, queue) = adapter
            .request_device(
                &wgpu::DeviceDescriptor {
                    label: Some("MapMap Device"),
                    features: wgpu::Features::TIMESTAMP_QUERY
                        | wgpu::Features::PUSH_CONSTANTS,
                    limits: wgpu::Limits {
                        max_push_constant_size: 128,
                        ..Default::default()
                    },
                },
                None,
            )
            .await
            .map_err(|e| RenderError::DeviceError(e.to_string()))?;

        info!("Device created successfully");

        let staging_belt = StagingBelt::new(1024 * 1024); // 1MB chunks

        Ok(Self {
            instance: Arc::new(instance),
            device: Arc::new(device),
            queue: Arc::new(queue),
            adapter_info,
            staging_belt,
            texture_counter: 0,
            shader_counter: 0,
        })
    }

    /// Create a surface using the backend's instance
    ///
    /// # Safety
    /// The window must outlive the surface
    pub unsafe fn create_surface(
        &self,
        window: &winit::window::Window,
    ) -> Result<wgpu::Surface> {
        self.instance
            .create_surface(window)
            .map_err(|e| RenderError::DeviceError(format!("Failed to create surface: {}", e)))
    }

    /// Get device limits
    pub fn limits(&self) -> wgpu::Limits {
        self.device.limits()
    }

    /// Get adapter info
    pub fn adapter_info(&self) -> &wgpu::AdapterInfo {
        &self.adapter_info
    }

    /// Recall staging belt buffers
    pub fn recall_staging_belt(&mut self) {
        self.staging_belt.recall();
    }

    /// Finish staging belt
    pub fn finish_staging_belt(&mut self) {
        self.staging_belt.finish();
    }
}

impl RenderBackend for WgpuBackend {
    fn device(&self) -> &wgpu::Device {
        &self.device
    }

    fn queue(&self) -> &wgpu::Queue {
        &self.queue
    }

    fn create_texture(&mut self, desc: TextureDescriptor) -> Result<TextureHandle> {
        let texture = self.device.create_texture(&wgpu::TextureDescriptor {
            label: Some(&format!("Texture {}", self.texture_counter)),
            size: wgpu::Extent3d {
                width: desc.width,
                height: desc.height,
                depth_or_array_layers: 1,
            },
            mip_level_count: desc.mip_levels,
            sample_count: 1,
            dimension: wgpu::TextureDimension::D2,
            format: desc.format,
            usage: desc.usage,
            view_formats: &[],
        });

        let handle = TextureHandle {
            id: self.texture_counter,
            texture: Arc::new(texture),
            width: desc.width,
            height: desc.height,
            format: desc.format,
        };

        self.texture_counter += 1;
        debug!("Created texture {} ({}x{})", handle.id, desc.width, desc.height);

        Ok(handle)
    }

    fn upload_texture(&mut self, handle: TextureHandle, data: &[u8]) -> Result<()> {
        let bytes_per_pixel = match handle.format {
            wgpu::TextureFormat::Rgba8Unorm | wgpu::TextureFormat::Rgba8UnormSrgb => 4,
            wgpu::TextureFormat::Bgra8Unorm | wgpu::TextureFormat::Bgra8UnormSrgb => 4,
            _ => {
                return Err(RenderError::TextureCreation(
                    "Unsupported texture format for upload".to_string(),
                ))
            }
        };

        let expected_size = (handle.width * handle.height * bytes_per_pixel) as usize;
        if data.len() != expected_size {
            return Err(RenderError::TextureCreation(format!(
                "Data size mismatch: expected {}, got {}",
                expected_size,
                data.len()
            )));
        }

        self.queue.write_texture(
            wgpu::ImageCopyTexture {
                texture: &handle.texture,
                mip_level: 0,
                origin: wgpu::Origin3d::ZERO,
                aspect: wgpu::TextureAspect::All,
            },
            data,
            wgpu::ImageDataLayout {
                offset: 0,
                bytes_per_row: Some(handle.width * bytes_per_pixel),
                rows_per_image: Some(handle.height),
            },
            wgpu::Extent3d {
                width: handle.width,
                height: handle.height,
                depth_or_array_layers: 1,
            },
        );

        debug!("Uploaded texture {} ({}x{})", handle.id, handle.width, handle.height);
        Ok(())
    }

    fn create_shader(&mut self, source: ShaderSource) -> Result<ShaderHandle> {
        let module = match source {
            ShaderSource::Wgsl(ref code) => {
                self.device.create_shader_module(wgpu::ShaderModuleDescriptor {
                    label: Some(&format!("Shader {}", self.shader_counter)),
                    source: wgpu::ShaderSource::Wgsl(code.clone().into()),
                })
            }
        };

        let handle = ShaderHandle {
            id: self.shader_counter,
            module: Arc::new(module),
        };

        self.shader_counter += 1;
        debug!("Created shader {}", handle.id);

        Ok(handle)
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_backend_creation() {
        pollster::block_on(async {
            let backend = WgpuBackend::new().await;
            assert!(backend.is_ok());

            if let Ok(backend) = backend {
                println!("Backend: {:?}", backend.adapter_info);
            }
        });
    }
}
