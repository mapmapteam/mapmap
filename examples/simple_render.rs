//! Simple rendering example
//!
//! Demonstrates basic usage of mapmap-render crate

use mapmap_render::{QuadRenderer, TextureDescriptor, WgpuBackend};
use winit::{
    event::{Event, WindowEvent},
    event_loop::{ControlFlow, EventLoop},
    window::WindowBuilder,
};

fn main() {
    println!("MapMap Simple Render Example");

    let event_loop = EventLoop::new();
    let window = WindowBuilder::new()
        .with_title("MapMap - Simple Render")
        .with_inner_size(winit::dpi::PhysicalSize::new(800, 600))
        .build(&event_loop)
        .unwrap();

    // Create wgpu backend
    let mut backend = pollster::block_on(WgpuBackend::new()).unwrap();
    println!("Backend initialized: {:?}", backend.adapter_info());

    // Create surface
    let instance = wgpu::Instance::new(wgpu::InstanceDescriptor {
        backends: wgpu::Backends::all(),
        ..Default::default()
    });

    let surface = unsafe { instance.create_surface(&window) }.unwrap();

    let surface_config = wgpu::SurfaceConfiguration {
        usage: wgpu::TextureUsages::RENDER_ATTACHMENT,
        format: wgpu::TextureFormat::Bgra8Unorm,
        width: 800,
        height: 600,
        present_mode: wgpu::PresentMode::Fifo,
        alpha_mode: wgpu::CompositeAlphaMode::Opaque,
        view_formats: vec![],
    };

    surface.configure(backend.device(), &surface_config);

    // Create quad renderer
    let quad_renderer = QuadRenderer::new(backend.device(), surface_config.format).unwrap();

    // Create a test texture (red square)
    let tex_desc = TextureDescriptor {
        width: 256,
        height: 256,
        format: wgpu::TextureFormat::Rgba8UnormSrgb,
        usage: wgpu::TextureUsages::TEXTURE_BINDING | wgpu::TextureUsages::COPY_DST,
        mip_levels: 1,
    };

    let texture = backend.create_texture(tex_desc).unwrap();

    // Fill with red color
    let red_data: Vec<u8> = (0..256 * 256)
        .flat_map(|_| [255u8, 0, 0, 255]) // RGBA red
        .collect();

    backend.upload_texture(texture.clone(), &red_data).unwrap();

    println!("Setup complete. Rendering...");

    event_loop.run(move |event, _, control_flow| {
        *control_flow = ControlFlow::Poll;

        match event {
            Event::WindowEvent {
                event: WindowEvent::CloseRequested,
                ..
            } => {
                println!("Close requested");
                *control_flow = ControlFlow::Exit;
            }
            Event::RedrawRequested(_) => {
                let frame = match surface.get_current_texture() {
                    Ok(frame) => frame,
                    Err(_) => return,
                };

                let view = frame
                    .texture
                    .create_view(&wgpu::TextureViewDescriptor::default());

                let mut encoder =
                    backend
                        .device()
                        .create_command_encoder(&wgpu::CommandEncoderDescriptor {
                            label: Some("Render Encoder"),
                        });

                {
                    let mut render_pass = encoder.begin_render_pass(&wgpu::RenderPassDescriptor {
                        label: Some("Main Render Pass"),
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
                                store: wgpu::StoreOp::Store,
                            },
                        })],
                        depth_stencil_attachment: None,
                        ..Default::default()
                    });

                    let texture_view = texture.create_view();
                    let bind_group = quad_renderer.create_bind_group(backend.device(), &texture_view);
                    quad_renderer.draw(&mut render_pass, &bind_group);
                }

                backend.queue().submit(Some(encoder.finish()));
                frame.present();
            }
            Event::MainEventsCleared => {
                window.request_redraw();
            }
            _ => {}
        }
    });
}
