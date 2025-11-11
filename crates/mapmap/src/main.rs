//! MapMap - Professional Projection Mapping Suite
//!
//! Phase 2 Demo Application - Projection Mapping with Warping

use anyhow::Result;
use tracing::{error, info};
use glam::{Mat4, Vec2};
use mapmap_core::{
    LayerManager, Mapping, MappingManager, Paint, PaintManager,
};
use mapmap_media::{FFmpegDecoder, TestPatternDecoder, VideoPlayer};
use mapmap_render::{
    Compositor, MeshRenderer, QuadRenderer, RenderBackend, TextureDescriptor, WgpuBackend,
};
use mapmap_ui::{AppUI, ImGuiContext};
use std::collections::HashMap;
use std::time::Instant;
use tracing_subscriber;
use winit::{
    event::{Event, WindowEvent},
    event_loop::{ControlFlow, EventLoop},
    window::WindowBuilder,
};

struct App {
    window: winit::window::Window,
    surface: wgpu::Surface,
    surface_config: wgpu::SurfaceConfiguration,
    backend: WgpuBackend,
    quad_renderer: QuadRenderer,
    mesh_renderer: MeshRenderer,
    compositor: Compositor,
    imgui_context: ImGuiContext,
    ui_state: AppUI,
    layer_manager: LayerManager,
    paint_manager: PaintManager,
    mapping_manager: MappingManager,
    video_players: HashMap<u64, VideoPlayer>, // Paint ID -> VideoPlayer
    paint_textures: HashMap<u64, mapmap_render::TextureHandle>, // Paint ID -> Texture
    layer_textures: HashMap<u64, mapmap_render::TextureHandle>, // Layer ID -> Texture
    render_target: Option<mapmap_render::TextureHandle>, // Intermediate render target
    last_frame: Instant,
    frame_count: u32,
    fps: f32,
}

impl App {
    async fn new(event_loop: &EventLoop<()>) -> Result<Self> {
        info!("Initializing MapMap Phase 2 Demo - Projection Mapping with Warping");

        // Create window
        let window = WindowBuilder::new()
            .with_title("MapMap - Phase 2 Demo - Projection Mapping with Warping")
            .with_inner_size(winit::dpi::PhysicalSize::new(1920, 1080))
            .build(event_loop)?;

        // Create wgpu backend
        let backend = WgpuBackend::new().await?;

        // Create surface using the backend's instance
        let surface = unsafe { backend.create_surface(&window) }?;

        let surface_config = wgpu::SurfaceConfiguration {
            usage: wgpu::TextureUsages::RENDER_ATTACHMENT,
            format: wgpu::TextureFormat::Bgra8Unorm,
            width: 1920,
            height: 1080,
            present_mode: wgpu::PresentMode::Fifo, // VSync
            alpha_mode: wgpu::CompositeAlphaMode::Opaque,
            view_formats: vec![],
        };

        surface.configure(backend.device(), &surface_config);

        // Create quad renderer
        let quad_renderer = QuadRenderer::new(backend.device(), surface_config.format)?;

        // Create mesh renderer
        let mesh_renderer = MeshRenderer::new(
            backend.device.clone(),
            surface_config.format,
        )?;

        // Create compositor
        let compositor = Compositor::new(
            backend.device.clone(),
            surface_config.format,
        )?;

        // Create ImGui context
        let imgui_context = ImGuiContext::new(
            &window,
            backend.device(),
            backend.queue(),
            surface_config.format,
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
            window,
            surface,
            surface_config,
            backend,
            quad_renderer,
            mesh_renderer,
            compositor,
            imgui_context,
            ui_state: AppUI::default(),
            layer_manager,
            paint_manager,
            mapping_manager,
            video_players,
            paint_textures: HashMap::new(),
            layer_textures: HashMap::new(),
            render_target: None,
            last_frame: Instant::now(),
            frame_count: 0,
            fps: 0.0,
        })
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
        let frame = self.surface.get_current_texture()?;
        let view = frame
            .texture
            .create_view(&wgpu::TextureViewDescriptor::default());

        let mut encoder = self
            .backend
            .device()
            .create_command_encoder(&wgpu::CommandEncoderDescriptor {
                label: Some("Render Encoder"),
            });

        // Render mappings with mesh warping
        {
            // Prepare all rendering data before creating render pass
            let visible_mappings = self.mapping_manager.visible_mappings();

            // Collect all rendering resources (they need to outlive the render pass)
            let render_data: Vec<_> = visible_mappings
                .iter()
                .filter_map(|mapping| {
                    self.paint_textures.get(&mapping.paint_id).map(|texture| {
                        let (vertex_buffer, index_buffer) =
                            self.mesh_renderer.create_mesh_buffers(&mapping.mesh);
                        let transform = Mat4::IDENTITY;
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

            // Create render pass
            let mut render_pass = encoder.begin_render_pass(&wgpu::RenderPassDescriptor {
                label: Some("Mapping Render Pass"),
                color_attachments: &[Some(wgpu::RenderPassColorAttachment {
                    view: &view,
                    resolve_target: None,
                    ops: wgpu::Operations {
                        load: wgpu::LoadOp::Clear(wgpu::Color {
                            r: 0.1,
                            g: 0.1,
                            b: 0.1,
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

        // Render ImGui
        let ui_state = &mut self.ui_state;
        let layer_manager = &mut self.layer_manager;
        let paint_manager = &mut self.paint_manager;
        let mapping_manager = &mut self.mapping_manager;
        let fps = self.fps;
        let frame_time = self.last_frame.elapsed().as_secs_f32() * 1000.0;

        self.imgui_context.render(
            &self.window,
            self.backend.device(),
            self.backend.queue(),
            &mut encoder,
            &view,
            |ui| {
                ui_state.render_menu_bar(ui);
                ui_state.render_controls(ui);
                ui_state.render_layer_panel(ui, layer_manager);
                ui_state.render_paint_panel(ui, paint_manager);
                ui_state.render_mapping_panel(ui, mapping_manager);
                ui_state.render_transform_panel(ui, layer_manager); // Phase 1
                ui_state.render_master_controls(ui, layer_manager);  // Phase 1
                ui_state.render_stats(ui, fps, frame_time);
            },
        );

        // Submit commands
        self.backend.queue().submit(Some(encoder.finish()));
        frame.present();

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

    fn handle_window_event(&mut self, event: &WindowEvent) -> bool {
        match event {
            WindowEvent::CloseRequested => return false,
            WindowEvent::Resized(size) => {
                self.surface_config.width = size.width;
                self.surface_config.height = size.height;
                self.surface
                    .configure(self.backend.device(), &self.surface_config);
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

    event_loop.run(move |event, _, control_flow| {
        *control_flow = ControlFlow::Poll;

        // Let ImGui handle events first
        app.imgui_context.handle_event(&app.window, &event);

        match event {
            Event::WindowEvent { event, .. } => {
                if !app.handle_window_event(&event) {
                    *control_flow = ControlFlow::Exit;
                }
            }
            Event::MainEventsCleared => {
                app.update();

                // Handle UI actions
                if !app.handle_ui_actions() {
                    *control_flow = ControlFlow::Exit;
                    return;
                }

                app.window.request_redraw();
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
