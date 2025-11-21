//! Oscillator Distortion Renderer
//!
//! Implements Kuramoto-based coupled oscillator simulation for dynamic distortion effects

use crate::Result;
use bytemuck::{Pod, Zeroable};
use mapmap_core::{OscillatorConfig, ColorMode, SimulationResolution, PhaseInitMode};
use std::sync::Arc;
use tracing::{info, debug};
use wgpu::util::DeviceExt;

/// Simulation uniform parameters
#[repr(C)]
#[derive(Copy, Clone, Debug, Pod, Zeroable)]
struct SimulationParams {
    sim_resolution: [f32; 2],
    delta_time: f32,
    kernel_radius: f32,

    frequency_min: f32,
    frequency_max: f32,
    time: f32,
    kernel_shrink: f32,

    // Ring parameters (4 rings)
    ring_distances: [f32; 4],
    ring_widths: [f32; 4],
    ring_couplings: [f32; 4],

    noise_amount: f32,
    use_log_polar: u32,
    _padding: [f32; 2],
}

/// Distortion uniform parameters
#[repr(C)]
#[derive(Copy, Clone, Debug, Pod, Zeroable)]
struct DistortionParams {
    resolution: [f32; 2],
    sim_resolution: [f32; 2],

    distortion_amount: f32,
    distortion_scale: f32,
    distortion_speed: f32,
    overlay_opacity: f32,

    time: f32,
    color_mode: u32,
    use_log_polar: u32,
    _padding: f32,
}

/// Fullscreen quad vertex
#[repr(C)]
#[derive(Copy, Clone, Debug, Pod, Zeroable)]
struct Vertex {
    position: [f32; 2],
    texcoord: [f32; 2],
}

impl Vertex {
    fn desc() -> wgpu::VertexBufferLayout<'static> {
        wgpu::VertexBufferLayout {
            array_stride: std::mem::size_of::<Vertex>() as wgpu::BufferAddress,
            step_mode: wgpu::VertexStepMode::Vertex,
            attributes: &[
                wgpu::VertexAttribute {
                    offset: 0,
                    shader_location: 0,
                    format: wgpu::VertexFormat::Float32x2,
                },
                wgpu::VertexAttribute {
                    offset: std::mem::size_of::<[f32; 2]>() as wgpu::BufferAddress,
                    shader_location: 1,
                    format: wgpu::VertexFormat::Float32x2,
                },
            ],
        }
    }
}

// Fullscreen quad vertices
const QUAD_VERTICES: &[Vertex] = &[
    Vertex { position: [-1.0, -1.0], texcoord: [0.0, 1.0] },
    Vertex { position: [1.0, -1.0], texcoord: [1.0, 1.0] },
    Vertex { position: [1.0, 1.0], texcoord: [1.0, 0.0] },
    Vertex { position: [-1.0, 1.0], texcoord: [0.0, 0.0] },
];

const QUAD_INDICES: &[u16] = &[0, 1, 2, 0, 2, 3];

/// Oscillator distortion renderer
pub struct OscillatorRenderer {
    // Pipelines
    simulation_pipeline: wgpu::RenderPipeline,
    distortion_pipeline: wgpu::RenderPipeline,

    // Bind group layouts
    sim_texture_layout: wgpu::BindGroupLayout,
    sim_uniform_layout: wgpu::BindGroupLayout,
    dist_texture_layout: wgpu::BindGroupLayout,
    dist_uniform_layout: wgpu::BindGroupLayout,

    // Phase textures (ping-pong)
    phase_texture_a: wgpu::Texture,
    phase_view_a: wgpu::TextureView,
    phase_texture_b: wgpu::Texture,
    phase_view_b: wgpu::TextureView,

    // Framebuffers for simulation
    sim_fbo_a: wgpu::TextureView,
    sim_fbo_b: wgpu::TextureView,

    // Buffers
    vertex_buffer: wgpu::Buffer,
    index_buffer: wgpu::Buffer,
    sim_uniform_buffer: wgpu::Buffer,
    dist_uniform_buffer: wgpu::Buffer,

    // Bind groups
    sim_bind_group_a: wgpu::BindGroup,
    sim_bind_group_b: wgpu::BindGroup,
    sim_uniform_bind_group: wgpu::BindGroup,
    dist_uniform_bind_group: wgpu::BindGroup,

    // Sampler
    sampler: wgpu::Sampler,

    // State
    sim_width: u32,
    sim_height: u32,
    current_phase: bool,  // false = A, true = B
    time_elapsed: f32,

    // Device reference
    device: Arc<wgpu::Device>,
    queue: Arc<wgpu::Queue>,
}

impl OscillatorRenderer {
    /// Create a new oscillator renderer
    pub fn new(
        device: Arc<wgpu::Device>,
        queue: Arc<wgpu::Queue>,
        target_format: wgpu::TextureFormat,
        config: &OscillatorConfig,
    ) -> Result<Self> {
        info!("Creating oscillator renderer");

        let (sim_width, sim_height) = config.simulation_resolution.dimensions();

        // Create sampler
        let sampler = device.create_sampler(&wgpu::SamplerDescriptor {
            label: Some("Oscillator Sampler"),
            address_mode_u: wgpu::AddressMode::ClampToEdge,
            address_mode_v: wgpu::AddressMode::ClampToEdge,
            address_mode_w: wgpu::AddressMode::ClampToEdge,
            mag_filter: wgpu::FilterMode::Linear,
            min_filter: wgpu::FilterMode::Linear,
            mipmap_filter: wgpu::FilterMode::Linear,
            ..Default::default()
        });

        // Create phase textures (ping-pong)
        let phase_texture_a = Self::create_phase_texture(&device, sim_width, sim_height, "Phase Texture A");
        let phase_view_a = phase_texture_a.create_view(&wgpu::TextureViewDescriptor::default());
        let sim_fbo_a = phase_texture_a.create_view(&wgpu::TextureViewDescriptor::default());

        let phase_texture_b = Self::create_phase_texture(&device, sim_width, sim_height, "Phase Texture B");
        let phase_view_b = phase_texture_b.create_view(&wgpu::TextureViewDescriptor::default());
        let sim_fbo_b = phase_texture_b.create_view(&wgpu::TextureViewDescriptor::default());

        // Create bind group layouts for simulation
        let sim_texture_layout = device.create_bind_group_layout(&wgpu::BindGroupLayoutDescriptor {
            label: Some("Sim Texture Bind Group Layout"),
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

        let sim_uniform_layout = device.create_bind_group_layout(&wgpu::BindGroupLayoutDescriptor {
            label: Some("Sim Uniform Bind Group Layout"),
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

        // Create bind group layouts for distortion
        let dist_texture_layout = device.create_bind_group_layout(&wgpu::BindGroupLayoutDescriptor {
            label: Some("Dist Texture Bind Group Layout"),
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
                wgpu::BindGroupLayoutEntry {
                    binding: 3,
                    visibility: wgpu::ShaderStages::FRAGMENT,
                    ty: wgpu::BindingType::Sampler(wgpu::SamplerBindingType::Filtering),
                    count: None,
                },
            ],
        });

        let dist_uniform_layout = device.create_bind_group_layout(&wgpu::BindGroupLayoutDescriptor {
            label: Some("Dist Uniform Bind Group Layout"),
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

        // Load shaders
        let sim_shader_source = include_str!("../../../shaders/oscillator_simulation.wgsl");
        let sim_shader = device.create_shader_module(wgpu::ShaderModuleDescriptor {
            label: Some("Oscillator Simulation Shader"),
            source: wgpu::ShaderSource::Wgsl(sim_shader_source.into()),
        });

        let dist_shader_source = include_str!("../../../shaders/oscillator_distortion.wgsl");
        let dist_shader = device.create_shader_module(wgpu::ShaderModuleDescriptor {
            label: Some("Oscillator Distortion Shader"),
            source: wgpu::ShaderSource::Wgsl(dist_shader_source.into()),
        });

        // Create simulation pipeline
        let sim_pipeline_layout = device.create_pipeline_layout(&wgpu::PipelineLayoutDescriptor {
            label: Some("Sim Pipeline Layout"),
            bind_group_layouts: &[&sim_texture_layout, &sim_uniform_layout],
            push_constant_ranges: &[],
        });

        let simulation_pipeline = device.create_render_pipeline(&wgpu::RenderPipelineDescriptor {
            label: Some("Simulation Pipeline"),
            layout: Some(&sim_pipeline_layout),
            vertex: wgpu::VertexState {
                module: &sim_shader,
                entry_point: "vs_main",
                buffers: &[Vertex::desc()],
            },
            fragment: Some(wgpu::FragmentState {
                module: &sim_shader,
                entry_point: "fs_main",
                targets: &[Some(wgpu::ColorTargetState {
                    format: wgpu::TextureFormat::R32Float,
                    blend: None,
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

        // Create distortion pipeline
        let dist_pipeline_layout = device.create_pipeline_layout(&wgpu::PipelineLayoutDescriptor {
            label: Some("Dist Pipeline Layout"),
            bind_group_layouts: &[&dist_texture_layout, &dist_uniform_layout],
            push_constant_ranges: &[],
        });

        let distortion_pipeline = device.create_render_pipeline(&wgpu::RenderPipelineDescriptor {
            label: Some("Distortion Pipeline"),
            layout: Some(&dist_pipeline_layout),
            vertex: wgpu::VertexState {
                module: &dist_shader,
                entry_point: "vs_main",
                buffers: &[Vertex::desc()],
            },
            fragment: Some(wgpu::FragmentState {
                module: &dist_shader,
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

        // Create vertex and index buffers
        let vertex_buffer = device.create_buffer_init(&wgpu::util::BufferInitDescriptor {
            label: Some("Oscillator Vertex Buffer"),
            contents: bytemuck::cast_slice(QUAD_VERTICES),
            usage: wgpu::BufferUsages::VERTEX,
        });

        let index_buffer = device.create_buffer_init(&wgpu::util::BufferInitDescriptor {
            label: Some("Oscillator Index Buffer"),
            contents: bytemuck::cast_slice(QUAD_INDICES),
            usage: wgpu::BufferUsages::INDEX,
        });

        // Create uniform buffers
        let sim_params = Self::create_sim_params(config, sim_width, sim_height, 0.0, 0.016);
        let sim_uniform_buffer = device.create_buffer_init(&wgpu::util::BufferInitDescriptor {
            label: Some("Sim Uniform Buffer"),
            contents: bytemuck::cast_slice(&[sim_params]),
            usage: wgpu::BufferUsages::UNIFORM | wgpu::BufferUsages::COPY_DST,
        });

        let dist_params = Self::create_dist_params(config, 1920, 1080, sim_width, sim_height, 0.0);
        let dist_uniform_buffer = device.create_buffer_init(&wgpu::util::BufferInitDescriptor {
            label: Some("Dist Uniform Buffer"),
            contents: bytemuck::cast_slice(&[dist_params]),
            usage: wgpu::BufferUsages::UNIFORM | wgpu::BufferUsages::COPY_DST,
        });

        // Create bind groups for simulation (ping-pong)
        let sim_bind_group_a = device.create_bind_group(&wgpu::BindGroupDescriptor {
            label: Some("Sim Bind Group A"),
            layout: &sim_texture_layout,
            entries: &[
                wgpu::BindGroupEntry {
                    binding: 0,
                    resource: wgpu::BindingResource::TextureView(&phase_view_a),
                },
                wgpu::BindGroupEntry {
                    binding: 1,
                    resource: wgpu::BindingResource::Sampler(&sampler),
                },
            ],
        });

        let sim_bind_group_b = device.create_bind_group(&wgpu::BindGroupDescriptor {
            label: Some("Sim Bind Group B"),
            layout: &sim_texture_layout,
            entries: &[
                wgpu::BindGroupEntry {
                    binding: 0,
                    resource: wgpu::BindingResource::TextureView(&phase_view_b),
                },
                wgpu::BindGroupEntry {
                    binding: 1,
                    resource: wgpu::BindingResource::Sampler(&sampler),
                },
            ],
        });

        let sim_uniform_bind_group = device.create_bind_group(&wgpu::BindGroupDescriptor {
            label: Some("Sim Uniform Bind Group"),
            layout: &sim_uniform_layout,
            entries: &[wgpu::BindGroupEntry {
                binding: 0,
                resource: sim_uniform_buffer.as_entire_binding(),
            }],
        });

        let dist_uniform_bind_group = device.create_bind_group(&wgpu::BindGroupDescriptor {
            label: Some("Dist Uniform Bind Group"),
            layout: &dist_uniform_layout,
            entries: &[wgpu::BindGroupEntry {
                binding: 0,
                resource: dist_uniform_buffer.as_entire_binding(),
            }],
        });

        Ok(Self {
            simulation_pipeline,
            distortion_pipeline,
            sim_texture_layout,
            sim_uniform_layout,
            dist_texture_layout,
            dist_uniform_layout,
            phase_texture_a,
            phase_view_a,
            phase_texture_b,
            phase_view_b,
            sim_fbo_a,
            sim_fbo_b,
            vertex_buffer,
            index_buffer,
            sim_uniform_buffer,
            dist_uniform_buffer,
            sim_bind_group_a,
            sim_bind_group_b,
            sim_uniform_bind_group,
            dist_uniform_bind_group,
            sampler,
            sim_width,
            sim_height,
            current_phase: false,
            time_elapsed: 0.0,
            device,
            queue,
        })
    }

    /// Create a phase texture for simulation
    fn create_phase_texture(
        device: &wgpu::Device,
        width: u32,
        height: u32,
        label: &str,
    ) -> wgpu::Texture {
        device.create_texture(&wgpu::TextureDescriptor {
            label: Some(label),
            size: wgpu::Extent3d {
                width,
                height,
                depth_or_array_layers: 1,
            },
            mip_level_count: 1,
            sample_count: 1,
            dimension: wgpu::TextureDimension::D2,
            format: wgpu::TextureFormat::R32Float,
            usage: wgpu::TextureUsages::TEXTURE_BINDING
                | wgpu::TextureUsages::RENDER_ATTACHMENT
                | wgpu::TextureUsages::COPY_DST,
            view_formats: &[],
        })
    }

    /// Initialize phase texture with a specific pattern
    pub fn initialize_phases(&mut self, mode: PhaseInitMode) {
        debug!("Initializing phases with mode: {:?}", mode);

        let size = (self.sim_width * self.sim_height) as usize;
        let mut phase_data = vec![0.0f32; size];

        match mode {
            PhaseInitMode::Random => {
                use rand::Rng;
                let mut rng = rand::thread_rng();
                for phase in &mut phase_data {
                    *phase = rng.gen::<f32>() * 2.0 * std::f32::consts::PI;
                }
            }
            PhaseInitMode::Uniform => {
                // All zeros (already initialized)
            }
            PhaseInitMode::PlaneHorizontal => {
                for y in 0..self.sim_height {
                    for x in 0..self.sim_width {
                        let u = x as f32 / self.sim_width as f32;
                        let idx = (y * self.sim_width + x) as usize;
                        phase_data[idx] = u * 2.0 * std::f32::consts::PI;
                    }
                }
            }
            PhaseInitMode::PlaneVertical => {
                for y in 0..self.sim_height {
                    for x in 0..self.sim_width {
                        let v = y as f32 / self.sim_height as f32;
                        let idx = (y * self.sim_width + x) as usize;
                        phase_data[idx] = v * 2.0 * std::f32::consts::PI;
                    }
                }
            }
            PhaseInitMode::PlaneDiagonal => {
                for y in 0..self.sim_height {
                    for x in 0..self.sim_width {
                        let u = x as f32 / self.sim_width as f32;
                        let v = y as f32 / self.sim_height as f32;
                        let idx = (y * self.sim_width + x) as usize;
                        phase_data[idx] = (u + v) * std::f32::consts::PI;
                    }
                }
            }
        }

        // Upload to both phase textures
        self.queue.write_texture(
            wgpu::ImageCopyTexture {
                texture: &self.phase_texture_a,
                mip_level: 0,
                origin: wgpu::Origin3d::ZERO,
                aspect: wgpu::TextureAspect::All,
            },
            bytemuck::cast_slice(&phase_data),
            wgpu::ImageDataLayout {
                offset: 0,
                bytes_per_row: Some(self.sim_width * 4),
                rows_per_image: Some(self.sim_height),
            },
            wgpu::Extent3d {
                width: self.sim_width,
                height: self.sim_height,
                depth_or_array_layers: 1,
            },
        );

        self.queue.write_texture(
            wgpu::ImageCopyTexture {
                texture: &self.phase_texture_b,
                mip_level: 0,
                origin: wgpu::Origin3d::ZERO,
                aspect: wgpu::TextureAspect::All,
            },
            bytemuck::cast_slice(&phase_data),
            wgpu::ImageDataLayout {
                offset: 0,
                bytes_per_row: Some(self.sim_width * 4),
                rows_per_image: Some(self.sim_height),
            },
            wgpu::Extent3d {
                width: self.sim_width,
                height: self.sim_height,
                depth_or_array_layers: 1,
            },
        );
    }

    /// Create simulation parameters
    fn create_sim_params(
        config: &OscillatorConfig,
        sim_width: u32,
        sim_height: u32,
        time: f32,
        delta_time: f32,
    ) -> SimulationParams {
        SimulationParams {
            sim_resolution: [sim_width as f32, sim_height as f32],
            delta_time,
            kernel_radius: config.kernel_radius,
            frequency_min: config.frequency_min,
            frequency_max: config.frequency_max,
            time,
            kernel_shrink: 1.0,
            ring_distances: [
                config.rings[0].distance,
                config.rings[1].distance,
                config.rings[2].distance,
                config.rings[3].distance,
            ],
            ring_widths: [
                config.rings[0].width,
                config.rings[1].width,
                config.rings[2].width,
                config.rings[3].width,
            ],
            ring_couplings: [
                config.rings[0].coupling,
                config.rings[1].coupling,
                config.rings[2].coupling,
                config.rings[3].coupling,
            ],
            noise_amount: config.noise_amount,
            use_log_polar: match config.coordinate_mode {
                mapmap_core::CoordinateMode::LogPolar => 1,
                _ => 0,
            },
            _padding: [0.0; 2],
        }
    }

    /// Create distortion parameters
    fn create_dist_params(
        config: &OscillatorConfig,
        width: u32,
        height: u32,
        sim_width: u32,
        sim_height: u32,
        time: f32,
    ) -> DistortionParams {
        DistortionParams {
            resolution: [width as f32, height as f32],
            sim_resolution: [sim_width as f32, sim_height as f32],
            distortion_amount: config.distortion_amount,
            distortion_scale: config.distortion_scale,
            distortion_speed: config.distortion_speed,
            overlay_opacity: config.overlay_opacity,
            time,
            color_mode: config.color_mode.to_u32(),
            use_log_polar: match config.coordinate_mode {
                mapmap_core::CoordinateMode::LogPolar => 1,
                _ => 0,
            },
            _padding: 0.0,
        }
    }

    /// Update simulation for one timestep
    pub fn update(&mut self, delta_time: f32, config: &OscillatorConfig) {
        self.time_elapsed += delta_time;

        // Update simulation uniforms
        let sim_params = Self::create_sim_params(
            config,
            self.sim_width,
            self.sim_height,
            self.time_elapsed,
            delta_time,
        );
        self.queue.write_buffer(
            &self.sim_uniform_buffer,
            0,
            bytemuck::cast_slice(&[sim_params]),
        );

        // Create command encoder for simulation step
        let mut encoder = self.device.create_command_encoder(&wgpu::CommandEncoderDescriptor {
            label: Some("Oscillator Simulation Encoder"),
        });

        // Determine which phase texture to read from and write to
        let (input_bind_group, output_view) = if self.current_phase {
            (&self.sim_bind_group_b, &self.sim_fbo_a)
        } else {
            (&self.sim_bind_group_a, &self.sim_fbo_b)
        };

        // Run simulation pass
        {
            let mut render_pass = encoder.begin_render_pass(&wgpu::RenderPassDescriptor {
                label: Some("Oscillator Simulation Pass"),
                color_attachments: &[Some(wgpu::RenderPassColorAttachment {
                    view: output_view,
                    resolve_target: None,
                    ops: wgpu::Operations {
                        load: wgpu::LoadOp::Clear(wgpu::Color::BLACK),
                        store: wgpu::StoreOp::Store,
                    },
                })],
                depth_stencil_attachment: None,
                timestamp_writes: None,
                occlusion_query_set: None,
            });

            render_pass.set_pipeline(&self.simulation_pipeline);
            render_pass.set_bind_group(0, input_bind_group, &[]);
            render_pass.set_bind_group(1, &self.sim_uniform_bind_group, &[]);
            render_pass.set_vertex_buffer(0, self.vertex_buffer.slice(..));
            render_pass.set_index_buffer(self.index_buffer.slice(..), wgpu::IndexFormat::Uint16);
            render_pass.draw_indexed(0..6, 0, 0..1);
        }

        self.queue.submit(std::iter::once(encoder.finish()));

        // Swap phase textures
        self.current_phase = !self.current_phase;
    }

    /// Render distortion effect to output
    pub fn render(
        &mut self,
        encoder: &mut wgpu::CommandEncoder,
        input_view: &wgpu::TextureView,
        output_view: &wgpu::TextureView,
        width: u32,
        height: u32,
        config: &OscillatorConfig,
    ) {
        // Update distortion uniforms
        let dist_params = Self::create_dist_params(
            config,
            width,
            height,
            self.sim_width,
            self.sim_height,
            self.time_elapsed,
        );
        self.queue.write_buffer(
            &self.dist_uniform_buffer,
            0,
            bytemuck::cast_slice(&[dist_params]),
        );

        // Get current phase texture
        let phase_view = if self.current_phase {
            &self.phase_view_b
        } else {
            &self.phase_view_a
        };

        // Create bind group for distortion (input texture + phase texture)
        let dist_bind_group = self.device.create_bind_group(&wgpu::BindGroupDescriptor {
            label: Some("Dist Bind Group"),
            layout: &self.dist_texture_layout,
            entries: &[
                wgpu::BindGroupEntry {
                    binding: 0,
                    resource: wgpu::BindingResource::TextureView(input_view),
                },
                wgpu::BindGroupEntry {
                    binding: 1,
                    resource: wgpu::BindingResource::Sampler(&self.sampler),
                },
                wgpu::BindGroupEntry {
                    binding: 2,
                    resource: wgpu::BindingResource::TextureView(phase_view),
                },
                wgpu::BindGroupEntry {
                    binding: 3,
                    resource: wgpu::BindingResource::Sampler(&self.sampler),
                },
            ],
        });

        // Render distortion pass
        {
            let mut render_pass = encoder.begin_render_pass(&wgpu::RenderPassDescriptor {
                label: Some("Oscillator Distortion Pass"),
                color_attachments: &[Some(wgpu::RenderPassColorAttachment {
                    view: output_view,
                    resolve_target: None,
                    ops: wgpu::Operations {
                        load: wgpu::LoadOp::Clear(wgpu::Color::BLACK),
                        store: wgpu::StoreOp::Store,
                    },
                })],
                depth_stencil_attachment: None,
                timestamp_writes: None,
                occlusion_query_set: None,
            });

            render_pass.set_pipeline(&self.distortion_pipeline);
            render_pass.set_bind_group(0, &dist_bind_group, &[]);
            render_pass.set_bind_group(1, &self.dist_uniform_bind_group, &[]);
            render_pass.set_vertex_buffer(0, self.vertex_buffer.slice(..));
            render_pass.set_index_buffer(self.index_buffer.slice(..), wgpu::IndexFormat::Uint16);
            render_pass.draw_indexed(0..6, 0, 0..1);
        }
    }
}
