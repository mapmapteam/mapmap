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
        let decoder1 = FFmpegDecoder::TestPattern(
            TestPatternDecoder::new(1920, 1080, std::time::Duration::from_secs(60), 30.0)
        );
        let mut player1 = VideoPlayer::new(decoder1);
        player1.set_looping(true);
        player1.play();
        video_players.insert(paint_id_1, player1);

        let decoder2 = FFmpegDecoder::TestPattern(
            TestPatternDecoder::new(1920, 1080, std::time::Duration::from_secs(60), 30.0)
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
                ui_state.render_paint_panel(ui, paint_manager);
                ui_state.render_mapping_panel(ui, mapping_manager);
                ui_state.render_stats(ui, fps, frame_time);
            },
        );

        // Submit commands
        self.backend.queue().submit(Some(encoder.finish()));
        frame.present();

        Ok(())
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
