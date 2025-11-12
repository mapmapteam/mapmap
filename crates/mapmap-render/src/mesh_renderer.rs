//! Mesh Renderer - Renders arbitrary warped meshes with texture mapping
//!
//! Supports perspective-correct texture mapping for projection mapping applications.

use crate::Result;
use bytemuck::{Pod, Zeroable};
use glam::Mat4;
use mapmap_core::{Mesh, MeshVertex};
use std::sync::Arc;
use tracing::info;
use wgpu::util::DeviceExt;

/// Vertex format for mesh rendering (matches mesh_warp.wgsl)
#[repr(C)]
#[derive(Copy, Clone, Debug, Pod, Zeroable)]
pub struct GpuVertex {
    position: [f32; 3],
    tex_coords: [f32; 2],
}

impl GpuVertex {
    fn from_mesh_vertex(vertex: &MeshVertex) -> Self {
        Self {
            position: [vertex.position.x, vertex.position.y, 0.0],
            tex_coords: [vertex.tex_coords.x, vertex.tex_coords.y],
        }
    }
}

/// Uniforms for mesh rendering (matches mesh_warp.wgsl)
/// Note: Must be padded to 96 bytes (multiple of 16) for std140 layout
#[repr(C)]
#[derive(Copy, Clone, Debug, Pod, Zeroable)]
struct MeshUniforms {
    transform: [[f32; 4]; 4],  // 64 bytes
    opacity: f32,              // 4 bytes
    _padding: [f32; 7],        // 28 bytes (total 96 bytes for std140 alignment)
}

/// Mesh renderer for warped texture mapping
pub struct MeshRenderer {
    pipeline: wgpu::RenderPipeline,
    pipeline_simple: wgpu::RenderPipeline,
    uniform_bind_group_layout: wgpu::BindGroupLayout,
    texture_bind_group_layout: wgpu::BindGroupLayout,
    sampler: wgpu::Sampler,
    device: Arc<wgpu::Device>,
}

impl MeshRenderer {
    /// Create a new mesh renderer
    pub fn new(device: Arc<wgpu::Device>, target_format: wgpu::TextureFormat) -> Result<Self> {
        info!("Creating mesh renderer");

        // Create sampler
        let sampler = device.create_sampler(&wgpu::SamplerDescriptor {
            label: Some("Mesh Sampler"),
            address_mode_u: wgpu::AddressMode::ClampToEdge,
            address_mode_v: wgpu::AddressMode::ClampToEdge,
            address_mode_w: wgpu::AddressMode::ClampToEdge,
            mag_filter: wgpu::FilterMode::Linear,
            min_filter: wgpu::FilterMode::Linear,
            mipmap_filter: wgpu::FilterMode::Linear,
            ..Default::default()
        });

        // Create uniform bind group layout
        let uniform_bind_group_layout =
            device.create_bind_group_layout(&wgpu::BindGroupLayoutDescriptor {
                label: Some("Mesh Uniform Bind Group Layout"),
                entries: &[wgpu::BindGroupLayoutEntry {
                    binding: 0,
                    visibility: wgpu::ShaderStages::VERTEX_FRAGMENT,
                    ty: wgpu::BindingType::Buffer {
                        ty: wgpu::BufferBindingType::Uniform,
                        has_dynamic_offset: false,
                        min_binding_size: None,
                    },
                    count: None,
                }],
            });

        // Create texture bind group layout
        let texture_bind_group_layout =
            device.create_bind_group_layout(&wgpu::BindGroupLayoutDescriptor {
                label: Some("Mesh Texture Bind Group Layout"),
                entries: &[
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
                    wgpu::BindGroupLayoutEntry {
                        binding: 1,
                        visibility: wgpu::ShaderStages::FRAGMENT,
                        ty: wgpu::BindingType::Sampler(wgpu::SamplerBindingType::Filtering),
                        count: None,
                    },
                ],
            });

        // Load shader
        let shader_source = include_str!("../../../shaders/mesh_warp.wgsl");
        let shader_module = device.create_shader_module(wgpu::ShaderModuleDescriptor {
            label: Some("Mesh Warp Shader"),
            source: wgpu::ShaderSource::Wgsl(shader_source.into()),
        });

        // Create pipeline layout
        let pipeline_layout = device.create_pipeline_layout(&wgpu::PipelineLayoutDescriptor {
            label: Some("Mesh Pipeline Layout"),
            bind_group_layouts: &[&uniform_bind_group_layout, &texture_bind_group_layout],
            push_constant_ranges: &[],
        });

        // Create render pipeline (perspective-correct)
        let pipeline = device.create_render_pipeline(&wgpu::RenderPipelineDescriptor {
            label: Some("Mesh Render Pipeline"),
            layout: Some(&pipeline_layout),
            vertex: wgpu::VertexState {
                module: &shader_module,
                entry_point: "vs_main",
                buffers: &[wgpu::VertexBufferLayout {
                    array_stride: std::mem::size_of::<GpuVertex>() as wgpu::BufferAddress,
                    step_mode: wgpu::VertexStepMode::Vertex,
                    attributes: &wgpu::vertex_attr_array![
                        0 => Float32x3, // position
                        1 => Float32x2, // tex_coords
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

        // Create simple pipeline (no perspective correction)
        let pipeline_simple = device.create_render_pipeline(&wgpu::RenderPipelineDescriptor {
            label: Some("Mesh Render Pipeline (Simple)"),
            layout: Some(&pipeline_layout),
            vertex: wgpu::VertexState {
                module: &shader_module,
                entry_point: "vs_main",
                buffers: &[wgpu::VertexBufferLayout {
                    array_stride: std::mem::size_of::<GpuVertex>() as wgpu::BufferAddress,
                    step_mode: wgpu::VertexStepMode::Vertex,
                    attributes: &wgpu::vertex_attr_array![
                        0 => Float32x3,
                        1 => Float32x2,
                    ],
                }],
            },
            fragment: Some(wgpu::FragmentState {
                module: &shader_module,
                entry_point: "fs_main_simple",
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
            pipeline_simple,
            uniform_bind_group_layout,
            texture_bind_group_layout,
            sampler,
            device,
        })
    }

    /// Create GPU buffers from a mesh
    pub fn create_mesh_buffers(&self, mesh: &Mesh) -> (wgpu::Buffer, wgpu::Buffer) {
        // Convert mesh vertices to GPU format
        let vertices: Vec<GpuVertex> = mesh
            .vertices
            .iter()
            .map(GpuVertex::from_mesh_vertex)
            .collect();

        let vertex_buffer = self
            .device
            .create_buffer_init(&wgpu::util::BufferInitDescriptor {
                label: Some("Mesh Vertex Buffer"),
                contents: bytemuck::cast_slice(&vertices),
                usage: wgpu::BufferUsages::VERTEX,
            });

        let index_buffer = self
            .device
            .create_buffer_init(&wgpu::util::BufferInitDescriptor {
                label: Some("Mesh Index Buffer"),
                contents: bytemuck::cast_slice(&mesh.indices),
                usage: wgpu::BufferUsages::INDEX,
            });

        (vertex_buffer, index_buffer)
    }

    /// Create a uniform buffer
    pub fn create_uniform_buffer(&self, transform: Mat4, opacity: f32) -> wgpu::Buffer {
        let uniforms = MeshUniforms {
            transform: transform.to_cols_array_2d(),
            opacity,
            _padding: [0.0; 7],
        };

        self.device
            .create_buffer_init(&wgpu::util::BufferInitDescriptor {
                label: Some("Mesh Uniform Buffer"),
                contents: bytemuck::cast_slice(&[uniforms]),
                usage: wgpu::BufferUsages::UNIFORM | wgpu::BufferUsages::COPY_DST,
            })
    }

    /// Create a uniform bind group
    pub fn create_uniform_bind_group(&self, uniform_buffer: &wgpu::Buffer) -> wgpu::BindGroup {
        self.device.create_bind_group(&wgpu::BindGroupDescriptor {
            label: Some("Mesh Uniform Bind Group"),
            layout: &self.uniform_bind_group_layout,
            entries: &[wgpu::BindGroupEntry {
                binding: 0,
                resource: uniform_buffer.as_entire_binding(),
            }],
        })
    }

    /// Create a texture bind group
    pub fn create_texture_bind_group(&self, texture_view: &wgpu::TextureView) -> wgpu::BindGroup {
        self.device.create_bind_group(&wgpu::BindGroupDescriptor {
            label: Some("Mesh Texture Bind Group"),
            layout: &self.texture_bind_group_layout,
            entries: &[
                wgpu::BindGroupEntry {
                    binding: 0,
                    resource: wgpu::BindingResource::TextureView(texture_view),
                },
                wgpu::BindGroupEntry {
                    binding: 1,
                    resource: wgpu::BindingResource::Sampler(&self.sampler),
                },
            ],
        })
    }

    /// Render a mesh
    pub fn draw<'a>(
        &'a self,
        render_pass: &mut wgpu::RenderPass<'a>,
        vertex_buffer: &'a wgpu::Buffer,
        index_buffer: &'a wgpu::Buffer,
        index_count: u32,
        uniform_bind_group: &'a wgpu::BindGroup,
        texture_bind_group: &'a wgpu::BindGroup,
        use_perspective_correction: bool,
    ) {
        // Choose pipeline based on perspective correction setting
        if use_perspective_correction {
            render_pass.set_pipeline(&self.pipeline);
        } else {
            render_pass.set_pipeline(&self.pipeline_simple);
        }

        render_pass.set_bind_group(0, uniform_bind_group, &[]);
        render_pass.set_bind_group(1, texture_bind_group, &[]);
        render_pass.set_vertex_buffer(0, vertex_buffer.slice(..));
        render_pass.set_index_buffer(index_buffer.slice(..), wgpu::IndexFormat::Uint16);
        render_pass.draw_indexed(0..index_count, 0, 0..1);
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_gpu_vertex_conversion() {
        let mesh_vertex = MeshVertex::new(
            glam::Vec2::new(0.5, 0.5),
            glam::Vec2::new(0.25, 0.75),
        );

        let gpu_vertex = GpuVertex::from_mesh_vertex(&mesh_vertex);

        assert_eq!(gpu_vertex.position[0], 0.5);
        assert_eq!(gpu_vertex.position[1], 0.5);
        assert_eq!(gpu_vertex.position[2], 0.0);
        assert_eq!(gpu_vertex.tex_coords[0], 0.25);
        assert_eq!(gpu_vertex.tex_coords[1], 0.75);
    }

    #[test]
    fn test_mesh_uniforms_size() {
        assert_eq!(
            std::mem::size_of::<MeshUniforms>(),
            80 // 64 (mat4x4) + 4 (f32) + 12 (padding)
        );
    }
}
