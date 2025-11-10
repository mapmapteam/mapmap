//! MapMap - Professional Projection Mapping Suite
//!
//! Phase 0 Demo Application

use anyhow::Result;
use mapmap_media::{FFmpegDecoder, VideoPlayer};
use mapmap_render::{QuadRenderer, TextureDescriptor, WgpuBackend};
use mapmap_ui::{AppUI, ImGuiContext};
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
    imgui_context: ImGuiContext,
    ui_state: AppUI,
    video_player: Option<VideoPlayer>,
    current_texture: Option<mapmap_render::TextureHandle>,
    last_frame: Instant,
    frame_count: u32,
    fps: f32,
}

impl App {
    async fn new(event_loop: &EventLoop<()>) -> Result<Self> {
        info!("Initializing MapMap Phase 0 Demo");

        // Create window
        let window = WindowBuilder::new()
            .with_title("MapMap - Phase 0 Demo")
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

        // Create ImGui context
        let imgui_context = ImGuiContext::new(
            &window,
            backend.device(),
            backend.queue(),
            surface_config.format,
        );

        // Initialize video player with test pattern
        let decoder = FFmpegDecoder {
            width: 1920,
            height: 1080,
            duration: std::time::Duration::from_secs(60),
            fps: 30.0,
            current_time: std::time::Duration::ZERO,
            frame_count: 0,
        };
        let mut video_player = VideoPlayer::new(decoder);
        video_player.set_looping(true);
        video_player.play();

        info!("Initialization complete");

        Ok(Self {
            window,
            surface,
            surface_config,
            backend,
            quad_renderer,
            imgui_context,
            ui_state: AppUI::default(),
            video_player: Some(video_player),
            current_texture: None,
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

        // Update video player
        if let Some(ref mut player) = self.video_player {
            if let Some(frame) = player.update(dt) {
                // Upload frame to GPU
                let tex_desc = TextureDescriptor {
                    width: frame.width,
                    height: frame.height,
                    format: wgpu::TextureFormat::Rgba8UnormSrgb,
                    usage: wgpu::TextureUsages::TEXTURE_BINDING | wgpu::TextureUsages::COPY_DST,
                    mip_levels: 1,
                };

                match self.backend.create_texture(tex_desc) {
                    Ok(handle) => {
                        let rgba_data = frame.to_rgba();
                        if self.backend.upload_texture(handle.clone(), &rgba_data).is_ok() {
                            self.current_texture = Some(handle);
                        }
                    }
                    Err(e) => error!("Failed to create texture: {}", e),
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

        // Render quad with video texture
        {
            let mut render_pass = encoder.begin_render_pass(&wgpu::RenderPassDescriptor {
                label: Some("Main Render Pass"),
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
                        store: wgpu::StoreOp::Store,
                    },
                })],
                depth_stencil_attachment: None,
                ..Default::default()
            });

            if let Some(ref texture) = self.current_texture {
                let texture_view = texture.create_view();
                let bind_group = self
                    .quad_renderer
                    .create_bind_group(self.backend.device(), &texture_view);
                self.quad_renderer.draw(&mut render_pass, &bind_group);
            }
        }

        // Render ImGui
        self.imgui_context.prepare_frame(&self.window);
        let ui = self.imgui_context.begin_frame();

        // Render UI
        self.ui_state.render_menu_bar(ui);
        self.ui_state.render_controls(ui);
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
