//! Compositing engine for blending multiple layers
//!
//! The compositor handles rendering multiple layers with different blend modes
//! and compositing them into a single output.

use crate::Result;
use bytemuck::{Pod, Zeroable};
use mapmap_core::BlendMode;
use std::sync::Arc;
use tracing::info;
use wgpu::util::DeviceExt;

/// Compositor parameters for blend modes
#[repr(C)]
#[derive(Copy, Clone, Debug, Pod, Zeroable)]
struct CompositeParams {
    blend_mode: u32,
    opacity: f32,
    _padding: [f32; 2],
}

/// Compositor for blending layers
pub struct Compositor {
    pipeline: wgpu::RenderPipeline,
    bind_group_layout: wgpu::BindGroupLayout,
    uniform_bind_group_layout: wgpu::BindGroupLayout,
    sampler: wgpu::Sampler,
    device: Arc<wgpu::Device>,
}

impl Compositor {
    /// Create a new compositor
    pub fn new(device: Arc<wgpu::Device>, target_format: wgpu::TextureFormat) -> Result<Self> {
        info!("Creating compositor");

        // Create sampler
        let sampler = device.create_sampler(&wgpu::SamplerDescriptor {
            label: Some("Compositor Sampler"),
            address_mode_u: wgpu::AddressMode::ClampToEdge,
            address_mode_v: wgpu::AddressMode::ClampToEdge,
            address_mode_w: wgpu::AddressMode::ClampToEdge,
            mag_filter: wgpu::FilterMode::Linear,
            min_filter: wgpu::FilterMode::Linear,
            mipmap_filter: wgpu::FilterMode::Linear,
            ..Default::default()
        });

        // Create bind group layouts
        let bind_group_layout = device.create_bind_group_layout(&wgpu::BindGroupLayoutDescriptor {
            label: Some("Compositor Bind Group Layout"),
            entries: &[
                // Base texture
                wgpu::BindGroupLayoutEntry {
                    binding: 0,
                    visibility: wgpu::ShaderStages::FRAGMENT,
                    ty: wgpu::BindingType::Texture {
                        sample_type: wgpu::TextureSampleType::Float { filterable: true },
                        view_dimension: wgpu::TextureViewDimension::D2,
                        multisampled: false,
                    },
                    count: None,
                },
                // Base sampler
                wgpu::BindGroupLayoutEntry {
                    binding: 1,
                    visibility: wgpu::ShaderStages::FRAGMENT,
                    ty: wgpu::BindingType::Sampler(wgpu::SamplerBindingType::Filtering),
                    count: None,
                },
                // Blend texture
                wgpu::BindGroupLayoutEntry {
                    binding: 2,
                    visibility: wgpu::ShaderStages::FRAGMENT,
                    ty: wgpu::BindingType::Texture {
                        sample_type: wgpu::TextureSampleType::Float { filterable: true },
                        view_dimension: wgpu::TextureViewDimension::D2,
                        multisampled: false,
                    },
                    count: None,
                },
                // Blend sampler
                wgpu::BindGroupLayoutEntry {
                    binding: 3,
                    visibility: wgpu::ShaderStages::FRAGMENT,
                    ty: wgpu::BindingType::Sampler(wgpu::SamplerBindingType::Filtering),
                    count: None,
                },
            ],
        });

        let uniform_bind_group_layout =
            device.create_bind_group_layout(&wgpu::BindGroupLayoutDescriptor {
                label: Some("Compositor Uniform Bind Group Layout"),
                entries: &[wgpu::BindGroupLayoutEntry {
                    binding: 0,
                    visibility: wgpu::ShaderStages::FRAGMENT,
                    ty: wgpu::BindingType::Buffer {
                        ty: wgpu::BufferBindingType::Uniform,
                        has_dynamic_offset: false,
                        min_binding_size: None,
                    },
                    count: None,
                }],
            });

        // Load shader
        let shader_source = include_str!("../../../shaders/blend_modes.wgsl");
        let shader_module = device.create_shader_module(wgpu::ShaderModuleDescriptor {
            label: Some("Compositor Shader"),
            source: wgpu::ShaderSource::Wgsl(shader_source.into()),
        });

        // Create pipeline layout
        let pipeline_layout = device.create_pipeline_layout(&wgpu::PipelineLayoutDescriptor {
            label: Some("Compositor Pipeline Layout"),
            bind_group_layouts: &[&bind_group_layout, &uniform_bind_group_layout],
            push_constant_ranges: &[],
        });

        // Create render pipeline
        let pipeline = device.create_render_pipeline(&wgpu::RenderPipelineDescriptor {
            label: Some("Compositor Pipeline"),
            layout: Some(&pipeline_layout),
            vertex: wgpu::VertexState {
                module: &shader_module,
                entry_point: "vs_main",
                buffers: &[wgpu::VertexBufferLayout {
                    array_stride: 20, // 3 floats (pos) + 2 floats (uv) = 5 * 4 bytes
                    step_mode: wgpu::VertexStepMode::Vertex,
                    attributes: &wgpu::vertex_attr_array![
                        0 => Float32x3,
                        1 => Float32x2,
                    ],
                }],
            },
            fragment: Some(wgpu::FragmentState {
                module: &shader_module,
                entry_point: "fs_main",
                targets: &[Some(wgpu::ColorTargetState {
                    format: target_format,
                    blend: Some(wgpu::BlendState::ALPHA_BLENDING),
                    write_mask: wgpu::ColorWrites::ALL,
                })],
            }),
            primitive: wgpu::PrimitiveState {
                topology: wgpu::PrimitiveTopology::TriangleList,
                strip_index_format: None,
                front_face: wgpu::FrontFace::Ccw,
                cull_mode: None,
                unclipped_depth: false,
                polygon_mode: wgpu::PolygonMode::Fill,
                conservative: false,
            },
            depth_stencil: None,
            multisample: wgpu::MultisampleState {
                count: 1,
                mask: !0,
                alpha_to_coverage_enabled: false,
            },
            multiview: None,
        });

        Ok(Self {
            pipeline,
            bind_group_layout,
            uniform_bind_group_layout,
            sampler,
            device,
        })
    }

    /// Create a bind group for compositing two textures
    pub fn create_bind_group(
        &self,
        base_view: &wgpu::TextureView,
        blend_view: &wgpu::TextureView,
    ) -> wgpu::BindGroup {
        self.device.create_bind_group(&wgpu::BindGroupDescriptor {
            label: Some("Compositor Bind Group"),
            layout: &self.bind_group_layout,
            entries: &[
                wgpu::BindGroupEntry {
                    binding: 0,
                    resource: wgpu::BindingResource::TextureView(base_view),
                },
                wgpu::BindGroupEntry {
                    binding: 1,
                    resource: wgpu::BindingResource::Sampler(&self.sampler),
                },
                wgpu::BindGroupEntry {
                    binding: 2,
                    resource: wgpu::BindingResource::TextureView(blend_view),
                },
                wgpu::BindGroupEntry {
                    binding: 3,
                    resource: wgpu::BindingResource::Sampler(&self.sampler),
                },
            ],
        })
    }

    /// Create a uniform buffer for composite parameters
    pub fn create_uniform_buffer(
        &self,
        blend_mode: BlendMode,
        opacity: f32,
    ) -> wgpu::Buffer {
        let params = CompositeParams {
            blend_mode: blend_mode_to_u32(blend_mode),
            opacity,
            _padding: [0.0; 2],
        };

        self.device
            .create_buffer_init(&wgpu::util::BufferInitDescriptor {
                label: Some("Compositor Uniform Buffer"),
                contents: bytemuck::cast_slice(&[params]),
                usage: wgpu::BufferUsages::UNIFORM | wgpu::BufferUsages::COPY_DST,
            })
    }

    /// Create a uniform bind group
    pub fn create_uniform_bind_group(&self, buffer: &wgpu::Buffer) -> wgpu::BindGroup {
        self.device.create_bind_group(&wgpu::BindGroupDescriptor {
            label: Some("Compositor Uniform Bind Group"),
            layout: &self.uniform_bind_group_layout,
            entries: &[wgpu::BindGroupEntry {
                binding: 0,
                resource: buffer.as_entire_binding(),
            }],
        })
    }

    /// Composite two textures with a specific blend mode
    pub fn composite<'a>(
        &'a self,
        render_pass: &mut wgpu::RenderPass<'a>,
        vertex_buffer: &'a wgpu::Buffer,
        index_buffer: &'a wgpu::Buffer,
        bind_group: &'a wgpu::BindGroup,
        uniform_bind_group: &'a wgpu::BindGroup,
    ) {
        render_pass.set_pipeline(&self.pipeline);
        render_pass.set_bind_group(0, bind_group, &[]);
        render_pass.set_bind_group(1, uniform_bind_group, &[]);
        render_pass.set_vertex_buffer(0, vertex_buffer.slice(..));
        render_pass.set_index_buffer(index_buffer.slice(..), wgpu::IndexFormat::Uint16);
        render_pass.draw_indexed(0..6, 0, 0..1);
    }
}

/// Convert BlendMode to u32 for shader
fn blend_mode_to_u32(mode: BlendMode) -> u32 {
    match mode {
        BlendMode::Normal => 0,
        BlendMode::Add => 1,
        BlendMode::Subtract => 2,
        BlendMode::Multiply => 3,
        BlendMode::Screen => 4,
        BlendMode::Overlay => 5,
        BlendMode::SoftLight => 6,
        BlendMode::HardLight => 7,
        BlendMode::Lighten => 8,
        BlendMode::Darken => 9,
        BlendMode::ColorDodge => 10,
        BlendMode::ColorBurn => 11,
        BlendMode::Difference => 12,
        BlendMode::Exclusion => 13,
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_blend_mode_conversion() {
        assert_eq!(blend_mode_to_u32(BlendMode::Normal), 0);
        assert_eq!(blend_mode_to_u32(BlendMode::Multiply), 3);
        assert_eq!(blend_mode_to_u32(BlendMode::Screen), 4);
        assert_eq!(blend_mode_to_u32(BlendMode::Difference), 12);
    }

    #[test]
    fn test_composite_params_size() {
        assert_eq!(
            std::mem::size_of::<CompositeParams>(),
            16 // 4 bytes * 4 (u32 + f32 + 2*f32 padding)
        );
    }

    #[test]
    fn test_compositor_creation() {
        pollster::block_on(async {
            let backend = crate::WgpuBackend::new().await;
            if let Ok(backend) = backend {
                let compositor =
                    Compositor::new(backend.device.clone(), wgpu::TextureFormat::Bgra8Unorm);
                assert!(compositor.is_ok());
            }
        });
    }
}
