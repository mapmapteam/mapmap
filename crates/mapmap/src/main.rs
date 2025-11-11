//! MapMap - Professional Projection Mapping Suite
//!
//! Phase 1 Demo Application - Layer Compositing

use anyhow::Result;
use mapmap_core::{BlendMode, Layer, LayerManager};
use mapmap_media::{FFmpegDecoder, TestPatternDecoder, VideoPlayer};
use mapmap_render::{Compositor, QuadRenderer, RenderBackend, TextureDescriptor, WgpuBackend};
use mapmap_ui::{AppUI, ImGuiContext};
use std::collections::HashMap;
use std::sync::Arc;
use std::time::Instant;
use tracing::{error, info};
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
    compositor: Compositor,
    imgui_context: ImGuiContext,
    ui_state: AppUI,
    layer_manager: LayerManager,
    video_players: HashMap<u64, VideoPlayer>, // Paint ID -> VideoPlayer
    layer_textures: HashMap<u64, mapmap_render::TextureHandle>, // Layer ID -> Texture
    render_target: Option<mapmap_render::TextureHandle>, // Intermediate render target
    last_frame: Instant,
    frame_count: u32,
    fps: f32,
}

impl App {
    async fn new(event_loop: &EventLoop<()>) -> Result<Self> {
        info!("Initializing MapMap Phase 1 Demo - Layer Compositing");

        // Create window
        let window = WindowBuilder::new()
            .with_title("MapMap - Phase 1 Demo - Layer Compositing")
            .with_inner_size(winit::dpi::PhysicalSize::new(1920, 1080))
            .build(event_loop)?;

        // Create wgpu backend
        let mut backend = WgpuBackend::new().await?;

        // Create surface
        let instance = wgpu::Instance::new(wgpu::InstanceDescriptor {
            backends: wgpu::Backends::all(),
            ..Default::default()
        });

        let surface = unsafe { instance.create_surface(&window) }?;

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

        // Initialize layer manager with demo layers
        let mut layer_manager = LayerManager::new();

        // Create first demo layer with test pattern
        let paint_id_1 = 1u64;
        let mut layer1 = Layer::new(0, "Layer 1 - Test Pattern");
        layer1.paint_id = Some(paint_id_1);
        layer1.blend_mode = BlendMode::Normal;
        layer1.opacity = 1.0;
        layer_manager.add_layer(layer1);

        // Create second demo layer
        let paint_id_2 = 2u64;
        let mut layer2 = Layer::new(0, "Layer 2 - Multiply");
        layer2.paint_id = Some(paint_id_2);
        layer2.blend_mode = BlendMode::Multiply;
        layer2.opacity = 0.8;
        layer_manager.add_layer(layer2);

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

        info!("Initialization complete - {} layers created", layer_manager.layers().len());

        Ok(Self {
            window,
            surface,
            surface_config,
            backend,
            quad_renderer,
            compositor,
            imgui_context,
            ui_state: AppUI::default(),
            layer_manager,
            video_players,
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

        // Update all video players and upload textures for visible layers
        let visible_layers = self.layer_manager.visible_layers();

        for layer in visible_layers {
            if let Some(paint_id) = layer.paint_id {
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
                                    self.layer_textures.insert(layer.id, handle);
                                }
                            }
                            Err(e) => error!("Failed to create texture for layer {}: {}", layer.id, e),
                        }
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

        // Render layers with compositing
        {
            // Prepare bind groups for all visible layers
            let visible_layers = self.layer_manager.visible_layers();
            let layer_data: Vec<_> = visible_layers
                .iter()
                .filter_map(|layer| {
                    self.layer_textures.get(&layer.id).map(|texture| {
                        let texture_view = texture.create_view();
                        let bind_group = self
                            .quad_renderer
                            .create_bind_group(self.backend.device(), &texture_view);
                        (texture_view, bind_group)
                    })
                })
                .collect();

            let mut render_pass = encoder.begin_render_pass(&wgpu::RenderPassDescriptor {
                label: Some("Layer Render Pass"),
                color_attachments: &[Some(wgpu::RenderPassColorAttachment {
                    view: &view,
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

            // For Phase 1 demo, render all visible layers sequentially
            // In Phase 2, we'll implement proper multi-pass compositing
            for (_texture_view, bind_group) in &layer_data {
                self.quad_renderer.draw(&mut render_pass, bind_group);
            }

            // Note: For now, we're just drawing layers on top of each other
            // The compositor with blend modes will be integrated in a follow-up
            // when we implement multi-pass rendering with intermediate targets
        }

        // Render ImGui
        self.imgui_context.prepare_frame(&self.window);
        let ui = self.imgui_context.begin_frame();

        // Render UI
        self.ui_state.render_menu_bar(ui);
        self.ui_state.render_controls(ui);
        self.ui_state.render_layer_panel(
            ui,
            &mut self.layer_manager,
        );
        self.ui_state.render_stats(
            ui,
            self.fps,
            (self.last_frame.elapsed().as_secs_f32() * 1000.0),
        );

        self.imgui_context.render(
            &self.window,
            self.backend.device(),
            self.backend.queue(),
            &mut encoder,
            &view,
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

    info!("Starting MapMap Phase 0 Demo");

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
