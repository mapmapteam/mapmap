//! Hello World Projection Mapping Example
//!
//! This example demonstrates the basics of projection mapping:
//! 1. Creating a Paint (media source)
//! 2. Creating a Mesh (warping geometry)
//! 3. Creating a Mapping (connecting Paint to Mesh)
//! 4. Rendering the result

use mapmap_core::{Paint, PaintType, Mesh, MeshType, Mapping};
use mapmap_render::{QuadRenderer, TextureDescriptor, WgpuBackend};
use glam::Vec2;
use winit::{
    event::{Event, WindowEvent, KeyboardInput, VirtualKeyCode, ElementState},
    event_loop::{ControlFlow, EventLoop},
    window::WindowBuilder,
};

fn main() {
    println!("MapMap - Hello World Projection Mapping Example");
    println!("===============================================\n");

    // Step 1: Create the window
    let event_loop = EventLoop::new();
    let window = WindowBuilder::new()
        .with_title("MapMap - Hello World Projection")
        .with_inner_size(winit::dpi::PhysicalSize::new(1280, 720))
        .build(&event_loop)
        .unwrap();

    println!("✓ Window created (1280x720)");

    // Step 2: Initialize GPU backend
    let mut backend = pollster::block_on(WgpuBackend::new()).unwrap();
    println!("✓ GPU Backend initialized");
    println!("  Adapter: {:?}", backend.adapter_info());

    // Step 3: Create surface for rendering
    let instance = wgpu::Instance::new(wgpu::InstanceDescriptor {
        backends: wgpu::Backends::all(),
        ..Default::default()
    });

    let surface = unsafe { instance.create_surface(&window) }.unwrap();

    let surface_config = wgpu::SurfaceConfiguration {
        usage: wgpu::TextureUsages::RENDER_ATTACHMENT,
        format: wgpu::TextureFormat::Bgra8Unorm,
        width: 1280,
        height: 720,
        present_mode: wgpu::PresentMode::Fifo,
        alpha_mode: wgpu::CompositeAlphaMode::Opaque,
        view_formats: vec![],
    };

    surface.configure(backend.device(), &surface_config);
    println!("✓ Surface configured");

    // Step 4: Create quad renderer
    let quad_renderer = QuadRenderer::new(backend.device(), surface_config.format).unwrap();
    println!("✓ Quad renderer created");

    // Step 5: Create a Paint (media source)
    // For this example, we'll create a simple colored texture
    let paint = Paint::color(1, "Hello World Paint", [0.2, 0.6, 1.0, 1.0]); // Blue color
    println!("✓ Paint created: '{}'", paint.name);

    // Step 6: Create a Mesh (warping geometry)
    // We'll use a simple quad mesh
    let mesh = Mesh::new_quad(
        1, // mesh_id
        "Hello World Mesh",
        Vec2::new(0.0, 0.0),    // top-left
        Vec2::new(800.0, 0.0),  // top-right
        Vec2::new(800.0, 600.0), // bottom-right
        Vec2::new(0.0, 600.0),  // bottom-left
    );
    println!("✓ Mesh created: '{}'", mesh.name);

    // Step 7: Create a Mapping (connects Paint to Mesh)
    let mapping = Mapping::new(
        1,                // mapping_id
        "Hello World Mapping",
        paint.id,         // paint_id
        mesh.id,          // mesh_id
    );
    println!("✓ Mapping created: '{}'", mapping.name);
    println!("  Paint ID: {} → Mesh ID: {}", mapping.paint_id, mapping.mesh_id);

    // Step 8: Create GPU texture for the Paint
    let tex_desc = TextureDescriptor {
        width: 512,
        height: 512,
        format: wgpu::TextureFormat::Rgba8UnormSrgb,
        usage: wgpu::TextureUsages::TEXTURE_BINDING | wgpu::TextureUsages::COPY_DST,
        mip_levels: 1,
    };

    let texture = backend.create_texture(tex_desc).unwrap();

    // Create a "Hello World" pattern
    // We'll create a simple gradient with the Paint's color
    let texture_data = create_hello_world_texture(512, 512, paint.color);
    backend.upload_texture(texture.clone(), &texture_data).unwrap();
    println!("✓ Texture uploaded (512x512)");

    println!("\n🎉 Setup complete! Rendering...\n");
    println!("Controls:");
    println!("  ESC - Exit");
    println!("  Any key - See the magic!\n");

    // Step 9: Render loop
    event_loop.run(move |event, _, control_flow| {
        *control_flow = ControlFlow::Poll;

        match event {
            Event::WindowEvent {
                event: WindowEvent::CloseRequested,
                ..
            } => {
                println!("Goodbye! 👋");
                *control_flow = ControlFlow::Exit;
            }
            Event::WindowEvent {
                event: WindowEvent::KeyboardInput {
                    input: KeyboardInput {
                        virtual_keycode: Some(VirtualKeyCode::Escape),
                        state: ElementState::Pressed,
                        ..
                    },
                    ..
                },
                ..
            } => {
                println!("Goodbye! 👋");
                *control_flow = ControlFlow::Exit;
            }
            Event::RedrawRequested(_) => {
                // Get the current frame
                let frame = match surface.get_current_texture() {
                    Ok(frame) => frame,
                    Err(_) => return,
                };

                let view = frame
                    .texture
                    .create_view(&wgpu::TextureViewDescriptor::default());

                // Create command encoder
                let mut encoder = backend.device().create_command_encoder(
                    &wgpu::CommandEncoderDescriptor {
                        label: Some("Render Encoder"),
                    }
                );

                {
                    // Begin render pass
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

                    // Render the textured quad (our projection mapping!)
                    let texture_view = texture.create_view();
                    let bind_group = quad_renderer.create_bind_group(
                        backend.device(),
                        &texture_view
                    );
                    quad_renderer.draw(&mut render_pass, &bind_group);
                }

                // Submit commands and present
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

/// Creates a "Hello World" texture with a gradient pattern
fn create_hello_world_texture(width: u32, height: u32, base_color: [f32; 4]) -> Vec<u8> {
    let mut data = Vec::with_capacity((width * height * 4) as usize);

    for y in 0..height {
        for x in 0..width {
            // Create a radial gradient effect
            let center_x = width as f32 / 2.0;
            let center_y = height as f32 / 2.0;
            let dx = x as f32 - center_x;
            let dy = y as f32 - center_y;
            let distance = (dx * dx + dy * dy).sqrt();
            let max_distance = (center_x * center_x + center_y * center_y).sqrt();
            let gradient = 1.0 - (distance / max_distance).min(1.0);

            // Apply gradient to base color
            let r = (base_color[0] * gradient * 255.0) as u8;
            let g = (base_color[1] * gradient * 255.0) as u8;
            let b = (base_color[2] * gradient * 255.0) as u8;
            let a = (base_color[3] * 255.0) as u8;

            data.push(r);
            data.push(g);
            data.push(b);
            data.push(a);
        }
    }

    data
}
