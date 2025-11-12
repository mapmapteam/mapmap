//! MapMap - Professional Projection Mapping Suite
//!
//! Phase 2 Demo Application - Projection Mapping with Warping

use anyhow::Result;
use tracing::{error, info};
use glam::{Mat4, Vec2};
use mapmap_core::{
    LayerManager, Mapping, MappingManager, Paint, PaintManager, OutputId,
};
use mapmap_media::{FFmpegDecoder, TestPatternDecoder, VideoPlayer};
use mapmap_render::{
    Compositor, EdgeBlendRenderer, ColorCalibrationRenderer, MeshRenderer, QuadRenderer,
    RenderBackend, TextureDescriptor, WgpuBackend,
};
use mapmap_ui::{AppUI, ImGuiContext};
use std::collections::HashMap;
use std::time::Instant;
use tracing_subscriber;
use winit::{
    event::{Event, WindowEvent},
    event_loop::{ControlFlow, EventLoop},
    window::{WindowBuilder, WindowId},
};

/// Context for a single output window with its surface and configuration
struct WindowContext {
    window: winit::window::Window,
    surface: wgpu::Surface,
    surface_config: wgpu::SurfaceConfiguration,
    #[allow(dead_code)]
    output_id: OutputId,
}

struct App {
    // Multi-window rendering: Each output has its own window and surface
    windows: HashMap<OutputId, WindowContext>,
    window_id_map: HashMap<WindowId, OutputId>, // Map winit WindowId to OutputId
    main_window_id: Option<OutputId>, // Primary window for UI

    backend: WgpuBackend,
    #[allow(dead_code)]
    quad_renderer: QuadRenderer,
    mesh_renderer: MeshRenderer,
    #[allow(dead_code)]
    compositor: Compositor,
    edge_blend_renderer: EdgeBlendRenderer,
    color_calibration_renderer: ColorCalibrationRenderer,
    imgui_context: ImGuiContext,
    ui_state: AppUI,
    layer_manager: LayerManager,
    paint_manager: PaintManager,
    mapping_manager: MappingManager,
    output_manager: mapmap_core::OutputManager,
    video_players: HashMap<u64, VideoPlayer>, // Paint ID -> VideoPlayer
    paint_textures: HashMap<u64, mapmap_render::TextureHandle>, // Paint ID -> Texture
    #[allow(dead_code)]
    layer_textures: HashMap<u64, mapmap_render::TextureHandle>, // Layer ID -> Texture
    intermediate_textures: HashMap<OutputId, mapmap_render::TextureHandle>, // Per-output intermediate textures
    last_frame: Instant,
    frame_count: u32,
    fps: f32,
}

impl App {
    async fn new(event_loop: &EventLoop<()>) -> Result<Self> {
        info!("Initializing MapMap Phase 2 Demo - Multi-Window Projection Mapping");

        // Create wgpu backend first (shared across all windows)
        let backend = WgpuBackend::new().await?;

        // Create main preview window for UI and control
        let main_window = WindowBuilder::new()
            .with_title("MapMap - Main Control")
            .with_inner_size(winit::dpi::PhysicalSize::new(1920, 1080))
            .build(event_loop)?;

        let main_window_id_winit = main_window.id();

        // Create surface for main window
        let main_surface = unsafe { backend.create_surface(&main_window) }?;

        let main_surface_config = wgpu::SurfaceConfiguration {
            usage: wgpu::TextureUsages::RENDER_ATTACHMENT,
            format: wgpu::TextureFormat::Bgra8Unorm,
            width: 1920,
            height: 1080,
            present_mode: wgpu::PresentMode::Fifo, // VSync
            alpha_mode: wgpu::CompositeAlphaMode::Opaque,
            view_formats: vec![],
        };

        main_surface.configure(backend.device(), &main_surface_config);

        // Use OutputId 0 for main window (non-output window)
        let main_output_id: OutputId = 0;

        // Store format before moving main_surface_config
        let surface_format = main_surface_config.format;

        let main_window_context = WindowContext {
            window: main_window,
            surface: main_surface,
            surface_config: main_surface_config,
            output_id: main_output_id,
        };

        let mut windows = HashMap::new();
        windows.insert(main_output_id, main_window_context);

        let mut window_id_map = HashMap::new();
        window_id_map.insert(main_window_id_winit, main_output_id);

        // Create quad renderer
        let quad_renderer = QuadRenderer::new(backend.device(), surface_format)?;

        // Create mesh renderer
        let mesh_renderer = MeshRenderer::new(
            backend.device.clone(),
            surface_format,
        )?;

        // Create compositor
        let compositor = Compositor::new(
            backend.device.clone(),
            surface_format,
        )?;

        // Create edge blend renderer
        let edge_blend_renderer = EdgeBlendRenderer::new(
            backend.device.clone(),
            surface_format,
        )?;

        // Create color calibration renderer
        let color_calibration_renderer = ColorCalibrationRenderer::new(
            backend.device.clone(),
            surface_format,
        )?;

        // Create ImGui context for main window
        let main_window_ref = &windows.get(&main_output_id).unwrap().window;
        let imgui_context = ImGuiContext::new(
            main_window_ref,
            backend.device(),
            backend.queue(),
            surface_format,
        );

        // Initialize layer manager (keeping for future layer system integration)
        let layer_manager = LayerManager::new();

        // Initialize paint manager with demo paints
        let mut paint_manager = PaintManager::new();

        // Create demo paints
        let paint1 = Paint::test_pattern(1, "Test Pattern 1");
        let paint_id_1 = paint_manager.add_paint(paint1);

        let paint2 = Paint::test_pattern(2, "Test Pattern 2");
        let paint_id_2 = paint_manager.add_paint(paint2);

        // Initialize mapping manager with demo mappings
        let mut mapping_manager = MappingManager::new();

        // Create first mapping - centered quad
        let mut mapping1 = Mapping::quad(1, "Quad Mapping 1", paint_id_1);
        mapping1.mesh.vertices[0].position = Vec2::new(-0.5, 0.0);
        mapping1.mesh.vertices[1].position = Vec2::new(0.5, 0.0);
        mapping1.mesh.vertices[2].position = Vec2::new(0.5, 0.6);
        mapping1.mesh.vertices[3].position = Vec2::new(-0.5, 0.6);
        mapping_manager.add_mapping(mapping1);

        // Create second mapping - lower triangle
        let mut mapping2 = Mapping::triangle(2, "Triangle Mapping", paint_id_2);
        mapping2.mesh.vertices[0].position = Vec2::new(0.0, -0.2);
        mapping2.mesh.vertices[1].position = Vec2::new(-0.4, -0.8);
        mapping2.mesh.vertices[2].position = Vec2::new(0.4, -0.8);
        mapping2.depth = 1.0;
        mapping_manager.add_mapping(mapping2);

        // Initialize video players for each paint
        let mut video_players = HashMap::new();

        // Use TestPattern decoder for demo (Real FFmpeg decoder requires feature flag)
        // 5-second duration for easier loop testing
        let decoder1 = FFmpegDecoder::TestPattern(
            TestPatternDecoder::new(1920, 1080, std::time::Duration::from_secs(5), 30.0)
        );
        let mut player1 = VideoPlayer::new(decoder1);
        player1.set_looping(true);
        player1.play();
        video_players.insert(paint_id_1, player1);

        let decoder2 = FFmpegDecoder::TestPattern(
            TestPatternDecoder::new(1920, 1080, std::time::Duration::from_secs(5), 30.0)
        );
        let mut player2 = VideoPlayer::new(decoder2);
        player2.set_looping(true);
        player2.play();
        video_players.insert(paint_id_2, player2);

        info!(
            "Initialization complete - {} paints, {} mappings created",
            paint_manager.paints().len(),
            mapping_manager.mappings().len()
        );

        Ok(Self {
            windows,
            window_id_map,
            main_window_id: Some(main_output_id),
            backend,
            quad_renderer,
            mesh_renderer,
            compositor,
            edge_blend_renderer,
            color_calibration_renderer,
            imgui_context,
            ui_state: AppUI::default(),
            layer_manager,
            paint_manager,
            mapping_manager,
            output_manager: mapmap_core::OutputManager::new((1920, 1080)),
            video_players,
            paint_textures: HashMap::new(),
            layer_textures: HashMap::new(),
            intermediate_textures: HashMap::new(),
            last_frame: Instant::now(),
            frame_count: 0,
            fps: 0.0,
        })
    }

    /// Create output windows for all configured outputs
    fn create_output_windows<T>(&mut self, event_loop_target: &winit::event_loop::EventLoopWindowTarget<T>) -> Result<()> {
        info!("Creating output windows for {} configured outputs", self.output_manager.outputs().len());

        for output_config in self.output_manager.outputs() {
            let output_id = output_config.id;

            // Skip if window already exists
            if self.windows.contains_key(&output_id) {
                continue;
            }

            info!("Creating window for output '{}' (ID: {})", output_config.name, output_id);

            let window = WindowBuilder::new()
                .with_title(&format!("MapMap Output - {}", output_config.name))
                .with_inner_size(winit::dpi::PhysicalSize::new(
                    output_config.resolution.0,
                    output_config.resolution.1,
                ))
                .with_fullscreen(if output_config.fullscreen {
                    Some(winit::window::Fullscreen::Borderless(None))
                } else {
                    None
                })
                .build(event_loop_target)?;

            let window_id_winit = window.id();

            // Create surface for this output window
            let surface = unsafe { self.backend.create_surface(&window) }?;

            let surface_config = wgpu::SurfaceConfiguration {
                usage: wgpu::TextureUsages::RENDER_ATTACHMENT,
                format: wgpu::TextureFormat::Bgra8Unorm,
                width: output_config.resolution.0,
                height: output_config.resolution.1,
                present_mode: wgpu::PresentMode::Fifo, // VSync for synchronized output
                alpha_mode: wgpu::CompositeAlphaMode::Opaque,
                view_formats: vec![],
            };

            surface.configure(self.backend.device(), &surface_config);

            let window_context = WindowContext {
                window,
                surface,
                surface_config,
                output_id,
            };

            self.windows.insert(output_id, window_context);
            self.window_id_map.insert(window_id_winit, output_id);

            info!("Created output window for '{}' at {}x{}",
                  output_config.name,
                  output_config.resolution.0,
                  output_config.resolution.1);
        }

        Ok(())
    }

    /// Synchronize windows with output manager configuration
    fn sync_windows(&mut self) {
        // Remove windows for outputs that no longer exist
        let output_ids: Vec<OutputId> = self.output_manager.outputs()
            .iter()
            .map(|o| o.id)
            .collect();

        let mut windows_to_remove = Vec::new();
        for (&window_output_id, _) in &self.windows {
            // Don't remove main window (id 0)
            if window_output_id != 0 && !output_ids.contains(&window_output_id) {
                windows_to_remove.push(window_output_id);
            }
        }

        for output_id in windows_to_remove {
            if let Some(window_context) = self.windows.remove(&output_id) {
                // Remove from window_id_map
                let winit_id = window_context.window.id();
                self.window_id_map.remove(&winit_id);
                info!("Removed output window for output ID {}", output_id);
            }
        }
    }

    fn update(&mut self) {
        let now = Instant::now();
        let dt = now - self.last_frame;

        // Update FPS counter
        self.frame_count += 1;
        if self.frame_count % 60 == 0 {
            self.fps = 1.0 / dt.as_secs_f32();
        }

        // Update all video players and upload textures for visible mappings
        let visible_mappings = self.mapping_manager.visible_mappings();

        for mapping in visible_mappings {
            let paint_id = mapping.paint_id;
            if let Some(player) = self.video_players.get_mut(&paint_id) {
                if let Some(frame) = player.update(dt) {
                    // Upload frame to GPU
                    let tex_desc = TextureDescriptor {
                        width: frame.width,
                        height: frame.height,
                        format: wgpu::TextureFormat::Rgba8UnormSrgb,
                        usage: wgpu::TextureUsages::TEXTURE_BINDING
                            | wgpu::TextureUsages::COPY_DST
                            | wgpu::TextureUsages::RENDER_ATTACHMENT,
                        mip_levels: 1,
                    };

                    match self.backend.create_texture(tex_desc) {
                        Ok(handle) => {
                            let rgba_data = frame.to_rgba();
                            if self.backend.upload_texture(handle.clone(), &rgba_data).is_ok() {
                                self.paint_textures.insert(paint_id, handle);
                            }
                        }
                        Err(e) => error!("Failed to create texture for paint {}: {}", paint_id, e),
                    }
                }
            }
        }

        self.last_frame = now;
    }

    fn render(&mut self) -> Result<()> {
        // Multi-window synchronized rendering
        let mut frames = Vec::new();
        let mut encoders = Vec::new();

        // Get all window IDs to render
        let window_ids: Vec<OutputId> = self.windows.keys().copied().collect();

        // Acquire frames from all surfaces
        for &output_id in &window_ids {
            if let Some(window_context) = self.windows.get(&output_id) {
                match window_context.surface.get_current_texture() {
                    Ok(frame) => frames.push((output_id, frame)),
                    Err(wgpu::SurfaceError::Timeout) => {
                        info!("Surface timeout for output {}", output_id);
                        continue;
                    }
                    Err(wgpu::SurfaceError::Outdated) => {
                        info!("Surface outdated for output {}, reconfiguring", output_id);
                        continue;
                    }
                    Err(e) => {
                        error!("Surface error for output {}: {:?}", output_id, e);
                        continue;
                    }
                }
            }
        }

        // Render to each acquired frame
        for (output_id, frame) in &frames {
            let view = frame
                .texture
                .create_view(&wgpu::TextureViewDescriptor::default());

            let mut encoder = self
                .backend
                .device()
                .create_command_encoder(&wgpu::CommandEncoderDescriptor {
                    label: Some(&format!("Render Encoder Output {}", output_id)),
                });

            // Determine if this is the main window or an output window
            let is_main_window = Some(*output_id) == self.main_window_id;

            // Render content
            self.render_to_view(&mut encoder, &view, *output_id, is_main_window)?;

            encoders.push(encoder);
        }

        // Submit all command buffers together for synchronized presentation
        let command_buffers: Vec<_> = encoders.into_iter().map(|e| e.finish()).collect();
        self.backend.queue().submit(command_buffers);

        // Present all frames synchronously
        for (_, frame) in frames {
            frame.present();
        }

        Ok(())
    }

    fn render_to_view(
        &mut self,
        encoder: &mut wgpu::CommandEncoder,
        view: &wgpu::TextureView,
        output_id: OutputId,
        is_main_window: bool,
    ) -> Result<()> {
        // Get output configuration if this is an output window
        let output_config = if !is_main_window {
            self.output_manager.get_output(output_id).cloned()
        } else {
            None
        };

        // Determine if we need post-processing (edge blend or color calibration)
        let needs_post_processing = if let Some(ref config) = output_config {
            config.edge_blend.left.enabled
                || config.edge_blend.right.enabled
                || config.edge_blend.top.enabled
                || config.edge_blend.bottom.enabled
                || config.color_calibration.brightness != 0.0
                || config.color_calibration.contrast != 1.0
                || config.color_calibration.saturation != 1.0
        } else {
            false
        };

        // Create intermediate texture if needed and doesn't exist
        if needs_post_processing && !self.intermediate_textures.contains_key(&output_id) {
            let window_context = self.windows.get(&output_id).unwrap();
            let tex_desc = TextureDescriptor {
                width: window_context.surface_config.width,
                height: window_context.surface_config.height,
                format: wgpu::TextureFormat::Bgra8Unorm,
                usage: wgpu::TextureUsages::RENDER_ATTACHMENT | wgpu::TextureUsages::TEXTURE_BINDING,
                mip_levels: 1,
            };

            match self.backend.create_texture(tex_desc) {
                Ok(handle) => {
                    self.intermediate_textures.insert(output_id, handle);
                }
                Err(e) => {
                    error!("Failed to create intermediate texture for output {}: {}", output_id, e);
                }
            }
        }

        // Choose render target (intermediate texture or final view)
        let render_target_view = if needs_post_processing {
            if let Some(intermediate_tex) = self.intermediate_textures.get(&output_id) {
                Some(intermediate_tex.create_view())
            } else {
                None
            }
        } else {
            None
        };

        let target_view = render_target_view.as_ref().unwrap_or(view);

        // Render mappings with mesh warping
        {
            let visible_mappings = self.mapping_manager.visible_mappings();

            // Collect all rendering resources
            let render_data: Vec<_> = visible_mappings
                .iter()
                .filter_map(|mapping| {
                    // For output windows, filter mappings by canvas region
                    if let Some(ref config) = output_config {
                        // Check if mapping intersects with this output's canvas region
                        if let Some((bounds_min, bounds_max)) = mapping.mesh.bounds() {
                            let region = &config.canvas_region;

                            // Simple bounding box intersection test
                            let intersects = !(
                                bounds_max.x < region.x
                                    || bounds_min.x > region.x + region.width
                                    || bounds_max.y < region.y
                                    || bounds_min.y > region.y + region.height
                            );

                            if !intersects {
                                return None; // Skip this mapping for this output
                            }
                        }
                    }

                    self.paint_textures.get(&mapping.paint_id).map(|texture| {
                        let (vertex_buffer, index_buffer) =
                            self.mesh_renderer.create_mesh_buffers(&mapping.mesh);

                        // Apply canvas region transformation for output windows
                        let transform = if let Some(ref config) = output_config {
                            // Transform from canvas space to output window space
                            let region = &config.canvas_region;
                            let scale = Mat4::from_scale(glam::Vec3::new(
                                1.0 / region.width,
                                1.0 / region.height,
                                1.0,
                            ));
                            let translate = Mat4::from_translation(glam::Vec3::new(
                                -region.x / region.width,
                                -region.y / region.height,
                                0.0,
                            ));
                            translate * scale
                        } else {
                            Mat4::IDENTITY
                        };

                        let uniform_buffer =
                            self.mesh_renderer.create_uniform_buffer(transform, mapping.opacity);
                        let uniform_bind_group =
                            self.mesh_renderer.create_uniform_bind_group(&uniform_buffer);
                        let texture_view = texture.create_view();
                        let texture_bind_group =
                            self.mesh_renderer.create_texture_bind_group(&texture_view);
                        let index_count = mapping.mesh.indices.len() as u32;

                        (
                            vertex_buffer,
                            index_buffer,
                            uniform_bind_group,
                            texture_bind_group,
                            index_count,
                        )
                    })
                })
                .collect();

            // Create render pass to intermediate or final target
            let mut render_pass = encoder.begin_render_pass(&wgpu::RenderPassDescriptor {
                label: Some(&format!("Mapping Render Pass Output {}", output_id)),
                color_attachments: &[Some(wgpu::RenderPassColorAttachment {
                    view: target_view,
                    resolve_target: None,
                    ops: wgpu::Operations {
                        load: wgpu::LoadOp::Clear(wgpu::Color {
                            r: 0.0,
                            g: 0.0,
                            b: 0.0,
                            a: 1.0,
                        }),
                        store: true,
                    },
                })],
                depth_stencil_attachment: None,
                ..Default::default()
            });

            // Draw all mappings
            for (vertex_buffer, index_buffer, uniform_bind_group, texture_bind_group, index_count) in
                &render_data
            {
                self.mesh_renderer.draw(
                    &mut render_pass,
                    vertex_buffer,
                    index_buffer,
                    *index_count,
                    uniform_bind_group,
                    texture_bind_group,
                    true, // Use perspective correction
                );
            }
        }

        // Apply post-processing if needed
        if needs_post_processing && render_target_view.is_some() {
            let intermediate_view = render_target_view.as_ref().unwrap();
            let config = output_config.as_ref().unwrap();

            // Step 1: Apply color calibration
            // Create another intermediate texture for color calibration result
            let color_corrected_view = {
                let window_context = self.windows.get(&output_id).unwrap();
                let tex_desc = TextureDescriptor {
                    width: window_context.surface_config.width,
                    height: window_context.surface_config.height,
                    format: wgpu::TextureFormat::Bgra8Unorm,
                    usage: wgpu::TextureUsages::RENDER_ATTACHMENT | wgpu::TextureUsages::TEXTURE_BINDING,
                    mip_levels: 1,
                };

                let temp_texture = self.backend.create_texture(tex_desc)?;
                let temp_view = temp_texture.create_view();

                // Apply color calibration
                let texture_bind_group = self.color_calibration_renderer.create_texture_bind_group(intermediate_view);
                let uniform_buffer = self.color_calibration_renderer.create_uniform_buffer(&config.color_calibration);
                let uniform_bind_group = self.color_calibration_renderer.create_uniform_bind_group(&uniform_buffer);

                let mut render_pass = encoder.begin_render_pass(&wgpu::RenderPassDescriptor {
                    label: Some(&format!("Color Calibration Pass Output {}", output_id)),
                    color_attachments: &[Some(wgpu::RenderPassColorAttachment {
                        view: &temp_view,
                        resolve_target: None,
                        ops: wgpu::Operations {
                            load: wgpu::LoadOp::Clear(wgpu::Color::BLACK),
                            store: true,
                        },
                    })],
                    depth_stencil_attachment: None,
                    ..Default::default()
                });

                self.color_calibration_renderer.render(
                    &mut render_pass,
                    &texture_bind_group,
                    &uniform_bind_group,
                );

                drop(render_pass);
                temp_view
            };

            // Step 2: Apply edge blending to final output
            let texture_bind_group = self.edge_blend_renderer.create_texture_bind_group(&color_corrected_view);
            let uniform_buffer = self.edge_blend_renderer.create_uniform_buffer(&config.edge_blend);
            let uniform_bind_group = self.edge_blend_renderer.create_uniform_bind_group(&uniform_buffer);

            let mut render_pass = encoder.begin_render_pass(&wgpu::RenderPassDescriptor {
                label: Some(&format!("Edge Blend Pass Output {}", output_id)),
                color_attachments: &[Some(wgpu::RenderPassColorAttachment {
                    view,
                    resolve_target: None,
                    ops: wgpu::Operations {
                        load: wgpu::LoadOp::Clear(wgpu::Color::BLACK),
                        store: true,
                    },
                })],
                depth_stencil_attachment: None,
                ..Default::default()
            });

            self.edge_blend_renderer.render(
                &mut render_pass,
                &texture_bind_group,
                &uniform_bind_group,
            );
        }

        // Render ImGui only on main window
        if is_main_window {
            let main_window = &self.windows.get(&output_id).unwrap().window;
            let ui_state = &mut self.ui_state;
            let layer_manager = &mut self.layer_manager;
            let paint_manager = &mut self.paint_manager;
            let mapping_manager = &mut self.mapping_manager;
            let output_manager = &mut self.output_manager;
            let fps = self.fps;
            let frame_time = self.last_frame.elapsed().as_secs_f32() * 1000.0;

            self.imgui_context.render(
                main_window,
                self.backend.device(),
                self.backend.queue(),
                encoder,
                view,
                |ui| {
                    ui_state.render_menu_bar(ui);
                    ui_state.render_controls(ui);
                    ui_state.render_layer_panel(ui, layer_manager);
                    ui_state.render_paint_panel(ui, paint_manager);
                    ui_state.render_mapping_panel(ui, mapping_manager);
                    ui_state.render_transform_panel(ui, layer_manager);
                    ui_state.render_master_controls(ui, layer_manager);
                    ui_state.render_output_panel(ui, output_manager);
                    ui_state.render_edge_blend_panel(ui, output_manager);
                    ui_state.render_color_calibration_panel(ui, output_manager);
                    ui_state.render_stats(ui, fps, frame_time);
                },
            );
        }

        Ok(())
    }

    fn handle_ui_actions(&mut self) -> bool {
        use mapmap_ui::UIAction;

        let actions = self.ui_state.take_actions();

        for action in actions {
            match action {
                UIAction::Play => {
                    info!("Play action triggered");
                    for player in self.video_players.values_mut() {
                        player.play();
                    }
                }
                UIAction::Pause => {
                    info!("Pause action triggered");
                    for player in self.video_players.values_mut() {
                        player.pause();
                    }
                }
                UIAction::Stop => {
                    info!("Stop action triggered");
                    for player in self.video_players.values_mut() {
                        player.stop();
                    }
                }
                UIAction::SetSpeed(speed) => {
                    info!("Setting playback speed to {}", speed);
                    for player in self.video_players.values_mut() {
                        player.set_speed(speed);
                    }
                }
                UIAction::ToggleLoop(looping) => {
                    info!("Setting loop mode to {} for {} video players", looping, self.video_players.len());
                    for (paint_id, player) in self.video_players.iter_mut() {
                        player.set_looping(looping);
                        info!("  - Paint {} now has looping={}", paint_id, player.is_looping());
                    }
                }
                UIAction::ToggleMappingVisibility(id, visible) => {
                    info!("Toggling mapping {} visibility to {}", id, visible);
                    if let Some(mapping) = self.mapping_manager.get_mapping_mut(id) {
                        mapping.visible = visible;
                    }
                }
                UIAction::AddMapping => {
                    info!("Adding new quad mapping");
                    let next_id = self.mapping_manager.mappings().len() as u64 + 1;
                    let paint_id = self.paint_manager.paints().first().map(|p| p.id).unwrap_or(1);
                    let mut new_mapping = Mapping::quad(
                        next_id,
                        &format!("Mapping {}", next_id),
                        paint_id,
                    );
                    // Position it slightly offset from center
                    let offset = (next_id as f32 * 0.1) % 1.0;
                    for vertex in &mut new_mapping.mesh.vertices {
                        vertex.position.x += offset - 0.5;
                    }
                    self.mapping_manager.add_mapping(new_mapping);
                }
                UIAction::RemoveMapping(id) => {
                    info!("Removing mapping {}", id);
                    self.mapping_manager.remove_mapping(id);
                }
                UIAction::SelectMapping(id) => {
                    info!("Selected mapping {}", id);
                    // TODO: Highlight selected mapping
                }
                UIAction::AddPaint => {
                    info!("Adding new paint");
                    let next_id = self.paint_manager.paints().len() as u64 + 1;
                    let paint = Paint::test_pattern(next_id, &format!("Test Pattern {}", next_id));
                    let paint_id = self.paint_manager.add_paint(paint);

                    // Create a video player for this paint (shorter 5-second duration for easier loop testing)
                    let decoder = mapmap_media::FFmpegDecoder::TestPattern(
                        mapmap_media::TestPatternDecoder::new(1920, 1080, std::time::Duration::from_secs(5), 30.0)
                    );
                    let mut player = mapmap_media::VideoPlayer::new(decoder);
                    player.set_looping(self.ui_state.looping);
                    player.set_speed(self.ui_state.playback_speed);
                    player.play();
                    self.video_players.insert(paint_id, player);
                    info!("Created video player for paint {} with looping={}, speed={}",
                          paint_id, self.ui_state.looping, self.ui_state.playback_speed);

                    // Create a default quad mapping for the new paint so it's visible
                    let mapping_id = self.mapping_manager.mappings().len() as u64 + 1;
                    let mut new_mapping = Mapping::quad(
                        mapping_id,
                        &format!("Mapping for Paint {}", next_id),
                        paint_id,
                    );
                    // Position it with a slight offset based on count
                    let offset = (mapping_id as f32 * 0.15) % 1.0 - 0.3;
                    for vertex in &mut new_mapping.mesh.vertices {
                        vertex.position.x += offset;
                        vertex.position.y += offset * 0.5;
                    }
                    self.mapping_manager.add_mapping(new_mapping);
                    info!("Created default quad mapping {} for paint {}", mapping_id, paint_id);
                }
                UIAction::RemovePaint(id) => {
                    info!("Removing paint {}", id);
                    self.paint_manager.remove_paint(id);
                    self.video_players.remove(&id);
                    self.paint_textures.remove(&id);
                }
                UIAction::LoadVideo(path) => {
                    if path.is_empty() {
                        // Open file picker dialog
                        info!("Opening file picker for media selection");
                        if let Some(file_path) = rfd::FileDialog::new()
                            .add_filter("Video Files", &["mp4", "mov", "avi", "mkv", "webm", "m4v"])
                            .add_filter("Image Files", &["png", "jpg", "jpeg", "gif", "tif", "tiff", "bmp", "webp"])
                            .add_filter("All Files", &["*"])
                            .set_title("Select Media File")
                            .pick_file()
                        {
                            let path_str = file_path.to_string_lossy().to_string();
                            info!("Selected media file: {}", path_str);
                            self.load_video_file(&path_str);
                        } else {
                            info!("File picker cancelled");
                        }
                    } else {
                        info!("Loading media from path: {}", path);
                        self.load_video_file(&path);
                    }
                }
                UIAction::SaveProject(path) => {
                    info!("Save project: {}", if path.is_empty() { "open dialog" } else { &path });
                    // TODO: Implement project save
                }
                UIAction::LoadProject(path) => {
                    info!("Load project: {}", if path.is_empty() { "open dialog" } else { &path });
                    // TODO: Implement project load
                }
                UIAction::Exit => {
                    info!("Exit action triggered");
                    return false;
                }
                UIAction::ToggleFullscreen => {
                    info!("Toggle fullscreen triggered");
                    // TODO: Implement fullscreen toggle
                }

                // Phase 1: Advanced Playback Actions
                UIAction::SetPlaybackDirection(direction) => {
                    info!("Setting playback direction to {:?}", direction);
                    for player in self.video_players.values_mut() {
                        player.set_direction(direction);
                    }
                }
                UIAction::TogglePlaybackDirection => {
                    info!("Toggling playback direction");
                    for player in self.video_players.values_mut() {
                        player.toggle_direction();
                    }
                }
                UIAction::SetPlaybackMode(mode) => {
                    info!("Setting playback mode to {:?}", mode);
                    for player in self.video_players.values_mut() {
                        player.set_playback_mode(mode);
                    }
                }

                // Phase 1: Layer Actions
                UIAction::AddLayer => {
                    info!("Adding new layer");
                    let layer_id = self.layer_manager.create_layer("New Layer");
                    info!("Created layer {} with ID {}", self.layer_manager.get_layer(layer_id).unwrap().name, layer_id);
                }
                UIAction::RemoveLayer(id) => {
                    info!("Removing layer {}", id);
                    if let Some(layer) = self.layer_manager.remove_layer(id) {
                        info!("Removed layer: {}", layer.name);
                    }
                }
                UIAction::DuplicateLayer(id) => {
                    info!("Duplicating layer {}", id);
                    if let Some(new_id) = self.layer_manager.duplicate_layer(id) {
                        info!("Created duplicate layer with ID {}", new_id);
                    }
                }
                UIAction::RenameLayer(id, new_name) => {
                    info!("Renaming layer {} to {}", id, new_name);
                    self.layer_manager.rename_layer(id, new_name);
                }
                UIAction::ToggleLayerBypass(id) => {
                    info!("Toggling bypass for layer {}", id);
                    if let Some(layer) = self.layer_manager.get_layer_mut(id) {
                        layer.toggle_bypass();
                        info!("Layer {} bypass is now {}", id, layer.bypass);
                    }
                }
                UIAction::ToggleLayerSolo(id) => {
                    info!("Toggling solo for layer {}", id);
                    if let Some(layer) = self.layer_manager.get_layer_mut(id) {
                        layer.toggle_solo();
                        info!("Layer {} solo is now {}", id, layer.solo);
                    }
                }
                UIAction::SetLayerOpacity(id, opacity) => {
                    info!("Setting layer {} opacity to {}", id, opacity);
                    if let Some(layer) = self.layer_manager.get_layer_mut(id) {
                        layer.opacity = opacity;
                    }
                }
                UIAction::EjectAllLayers => {
                    info!("Ejecting all layer content");
                    self.layer_manager.eject_all();
                }

                // Phase 1: Transform Actions
                UIAction::SetLayerTransform(id, transform) => {
                    info!("Setting transform for layer {}", id);
                    if let Some(layer) = self.layer_manager.get_layer_mut(id) {
                        layer.transform = transform;
                    }
                }
                UIAction::ApplyResizeMode(id, mode) => {
                    info!("Applying resize mode {:?} to layer {}", mode, id);

                    // Get composition size first (before borrowing layer)
                    let target_size = glam::Vec2::new(
                        self.layer_manager.composition.size.0 as f32,
                        self.layer_manager.composition.size.1 as f32
                    );

                    if let Some(layer) = self.layer_manager.get_layer_mut(id) {
                        // Get paint dimensions if available
                        let source_size = if let Some(paint_id) = layer.paint_id {
                            if let Some(paint) = self.paint_manager.get_paint(paint_id) {
                                paint.dimensions
                            } else {
                                glam::Vec2::new(1920.0, 1080.0)
                            }
                        } else {
                            glam::Vec2::new(1920.0, 1080.0)
                        };

                        layer.set_transform_with_resize(mode, source_size, target_size);
                        info!("Applied resize mode to layer {}", id);
                    }
                }

                // Phase 1: Master Controls
                UIAction::SetMasterOpacity(opacity) => {
                    info!("Setting master opacity to {}", opacity);
                    self.layer_manager.composition.set_master_opacity(opacity);
                }
                UIAction::SetMasterSpeed(speed) => {
                    info!("Setting master speed to {}", speed);
                    self.layer_manager.composition.set_master_speed(speed);
                    // Note: Master speed application to video players would happen during update loop
                }
                UIAction::SetCompositionName(name) => {
                    info!("Setting composition name to {}", name);
                    self.layer_manager.composition.name = name;
                }

                // Phase 2: Multi-Output Actions
                UIAction::AddOutput(name, region, resolution) => {
                    info!("Adding output: {} at {:?} with resolution {:?}", name, region, resolution);
                    self.output_manager.add_output(name, region, resolution);
                }
                UIAction::RemoveOutput(id) => {
                    info!("Removing output {}", id);
                    self.output_manager.remove_output(id);
                }
                UIAction::ConfigureOutput(id, config) => {
                    info!("Configuring output {} with new settings", id);
                    if let Some(output) = self.output_manager.get_output_mut(id) {
                        *output = config;
                    }
                }
                UIAction::SetOutputEdgeBlend(id, edge_blend) => {
                    info!("Setting edge blend for output {}", id);
                    if let Some(output) = self.output_manager.get_output_mut(id) {
                        output.edge_blend = edge_blend;
                    }
                }
                UIAction::SetOutputColorCalibration(id, calibration) => {
                    info!("Setting color calibration for output {}", id);
                    if let Some(output) = self.output_manager.get_output_mut(id) {
                        output.color_calibration = calibration;
                    }
                }
                UIAction::CreateProjectorArray2x2(resolution, overlap) => {
                    info!("Creating 2x2 projector array with resolution {:?} and {}% overlap", resolution, overlap * 100.0);
                    self.output_manager.create_projector_array_2x2(resolution, overlap);
                }
            }
        }

        true
    }

    fn load_video_file(&mut self, path: &str) {
        use mapmap_core::{Paint, PaintType, Mapping};
        use mapmap_media::{FFmpegDecoder, VideoPlayer, VideoDecoder};
        use glam::Vec2;

        info!("Loading media file: {}", path);

        // Try to open the media file with FFmpeg (which now supports images too)
        match FFmpegDecoder::open(path) {
            Ok(decoder) => {
                // Get media info
                let (width, height) = decoder.resolution();
                let fps = decoder.fps();
                let duration = decoder.duration();

                // Detect media type based on decoder variant
                let is_still_image = matches!(decoder, FFmpegDecoder::StillImage(_));
                let is_gif = matches!(decoder, FFmpegDecoder::Gif(_));
                let is_image_sequence = matches!(decoder, FFmpegDecoder::ImageSequence(_));
                let paint_type = if is_still_image || is_gif || is_image_sequence {
                    PaintType::Image
                } else {
                    PaintType::Video
                };

                info!("Media loaded: {}x{} @ {:.2} fps, duration: {:.2}s, type: {:?}",
                      width, height, fps, duration.as_secs_f64(), paint_type);

                // Create a paint for this media
                let next_id = self.paint_manager.paints().len() as u64 + 1;
                let filename = std::path::Path::new(path)
                    .file_name()
                    .and_then(|n| n.to_str())
                    .unwrap_or("Media");

                let paint = Paint {
                    id: next_id,
                    name: filename.to_string(),
                    paint_type,
                    opacity: 1.0,
                    color: [1.0, 1.0, 1.0, 1.0],
                    is_playing: !is_still_image, // Still images don't "play"
                    loop_playback: if is_still_image { false } else { self.ui_state.looping },
                    rate: self.ui_state.playback_speed,
                    source_path: Some(path.to_string()),
                    dimensions: Vec2::new(width as f32, height as f32),
                    lock_aspect: true,
                };

                let paint_id = self.paint_manager.add_paint(paint);
                info!("Created paint {} for media (type: {:?})", paint_id, paint_type);

                // Create video player (works for all decoder types)
                let mut player = VideoPlayer::new(decoder);

                // Still images don't need looping or speed control
                if !is_still_image {
                    player.set_looping(self.ui_state.looping);
                    player.set_speed(self.ui_state.playback_speed);
                    player.play();
                } else {
                    // For still images, just load the single frame
                    player.play();
                }

                self.video_players.insert(paint_id, player);
                info!("Created player for paint {}", paint_id);

                // Create a default quad mapping for the media
                let mapping_id = self.mapping_manager.mappings().len() as u64 + 1;
                let mut new_mapping = Mapping::quad(
                    mapping_id,
                    &format!("Mapping for {}", filename),
                    paint_id,
                );

                // Position it with a slight offset
                let offset = (mapping_id as f32 * 0.15) % 1.0 - 0.3;
                for vertex in &mut new_mapping.mesh.vertices {
                    vertex.position.x += offset;
                    vertex.position.y += offset * 0.5;
                }

                self.mapping_manager.add_mapping(new_mapping);
                info!("Created mapping {} for media", mapping_id);
            }
            Err(e) => {
                error!("Failed to load media file '{}': {}", path, e);
            }
        }
    }

    fn handle_window_event(&mut self, window_id: WindowId, event: &WindowEvent) -> bool {
        // Map WindowId to OutputId
        let output_id = match self.window_id_map.get(&window_id) {
            Some(&id) => id,
            None => return true, // Unknown window, ignore
        };

        match event {
            WindowEvent::CloseRequested => {
                // If main window is closed, exit application
                if Some(output_id) == self.main_window_id {
                    info!("Main window closed, exiting application");
                    return false;
                } else {
                    // Close specific output window
                    info!("Closing output window {}", output_id);
                    self.windows.remove(&output_id);
                    self.window_id_map.remove(&window_id);
                    // Remove output from manager
                    self.output_manager.remove_output(output_id);
                }
            }
            WindowEvent::Resized(size) => {
                if let Some(window_context) = self.windows.get_mut(&output_id) {
                    info!("Window {} resized to {}x{}", output_id, size.width, size.height);
                    window_context.surface_config.width = size.width;
                    window_context.surface_config.height = size.height;
                    window_context
                        .surface
                        .configure(self.backend.device(), &window_context.surface_config);
                }
            }
            _ => {}
        }
        true
    }
}

fn main() -> Result<()> {
    // Setup logging
    tracing_subscriber::fmt()
        .with_env_filter(
            tracing_subscriber::EnvFilter::from_default_env()
                .add_directive(tracing::Level::INFO.into()),
        )
        .init();

    info!("Starting MapMap Phase 2 Demo - Projection Mapping with Warping");

    let event_loop = EventLoop::new();
    let mut app = pollster::block_on(App::new(&event_loop))?;

    event_loop.run(move |event, event_loop_target, control_flow| {
        *control_flow = ControlFlow::Poll;

        // Let ImGui handle events only for main window
        if let Some(main_window_id) = app.main_window_id {
            if let Some(main_window_context) = app.windows.get(&main_window_id) {
                app.imgui_context.handle_event(&main_window_context.window, &event);
            }
        }

        match event {
            Event::WindowEvent { event, window_id } => {
                if !app.handle_window_event(window_id, &event) {
                    *control_flow = ControlFlow::Exit;
                }
            }
            Event::MainEventsCleared => {
                app.update();

                // Handle UI actions (may create/remove windows)
                if !app.handle_ui_actions() {
                    *control_flow = ControlFlow::Exit;
                    return;
                }

                // Synchronize windows with output manager and create any new windows
                app.sync_windows();
                if let Err(e) = app.create_output_windows(event_loop_target) {
                    error!("Failed to create output windows: {}", e);
                }

                // Request redraw for all windows
                for window_context in app.windows.values() {
                    window_context.window.request_redraw();
                }
            }
            Event::RedrawRequested(_) => {
                if let Err(e) = app.render() {
                    error!("Render error: {}", e);
                    *control_flow = ControlFlow::Exit;
                }
            }
            _ => {}
        }
    });
}
