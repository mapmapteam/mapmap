# MapMap Rust Rewrite Plan - Phase 0: Foundation

## Executive Summary

This document outlines the complete Rust rewrite strategy for MapMap, transforming it from a C++/Qt application into a modern, high-performance Rust-based projection mapping system capable of competing with Resolume Arena. This plan addresses Phase 0 (Foundation, Months 1-3) while establishing the architectural foundation for all subsequent phases.

## Why Rust?

**Performance & Safety:**
- Zero-cost abstractions with guaranteed memory safety
- Fearless concurrency (critical for decode/upload/render threading)
- No garbage collection overhead for real-time performance
- Native cross-compilation support

**Ecosystem Advantages:**
- Modern graphics APIs via wgpu (Vulkan/Metal/DX12)
- Mature FFmpeg and GStreamer bindings
- Excellent MIDI/OSC/DMX support
- Strong async runtime (Tokio) for network I/O

**Long-term Benefits:**
- Smaller attack surface (memory safety eliminates entire bug classes)
- Better resource utilization for battery-powered setups
- Easier to maintain and extend
- Active community and corporate backing

---

## Technology Stack - Rust Edition

### Core Framework
- **Language:** Rust 2021 Edition (MSRV 1.75+)
- **Build System:** Cargo + cargo-make for complex workflows
- **FFI Layer:** bindgen for C libraries, cbindgen for plugin API

### Graphics & Windowing
- **Rendering:** `wgpu` 0.19+ (Vulkan/Metal/DX12 backends)
  - Alternative: `ash` + `ash-window` for direct Vulkan control
- **Windowing:** `winit` 0.29+ (cross-platform window management)
- **Shader Compilation:** `naga` (embedded in wgpu) + `shaderc` for GLSL
- **GPU Buffer Management:** `wgpu` buffer pool with staging buffer reuse

### Media Pipeline
- **Primary:** `gstreamer-rs` 0.21+ (hardware accel via VA-API/VideoToolbox/DXVA)
  - Fallback: `ffmpeg-next` 6.x for simpler decode paths
- **HAP Codec:** Rust implementation or FFI to official HAP library
- **Image Loading:** `image` crate for stills, `dds-rs` for compressed formats
- **Texture Streaming:** Custom async loader with `tokio` + wgpu staging buffers

### Audio & MIDI
- **Audio I/O:** `cpal` 0.15+ (cross-platform audio streams)
- **Audio Analysis:** `rustfft` 6.x + custom peak detection
- **MIDI:** `midir` 0.9+ (low-latency MIDI I/O)
- **OSC:** `rosc` 0.10+ (OSC protocol implementation)

### Professional I/O (FFI Bridges)
- **NDI:** C API bindings via `bindgen` to `libndi`
- **DeckLink SDI:** COM/Obj-C bindings via platform-specific crates
  - Windows: `windows` crate COM interop
  - macOS: `objc` + `cocoa-foundation` crates
- **Spout (Windows):** Minimal C++ wrapper → C API → Rust bindings
- **Syphon (macOS):** Obj-C wrapper → C API → Rust bindings
- **DMX:** Direct Art-Net/sACN UDP implementation, optional OLA FFI

### User Interface
- **Live UI:** `imgui-rs` 0.11+ (retained-mode, immediate-draw UI)
  - Integration: `imgui-wgpu` + `imgui-winit-support`
- **Future Authoring UI:** Consider `egui` or native Qt 6 bindings (deferred to Phase 6)
- **Widgets:** Custom ImGui widgets for warp mesh editing, layer controls

### Control & Networking
- **HTTP API:** `axum` 0.7+ (REST API server)
- **WebSocket:** `tokio-tungstenite` (show control interface)
- **DMX/Art-Net:** `artnet_protocol` or hand-rolled UDP (sACN similar)
- **Async Runtime:** `tokio` 1.36+ (network I/O, file ops, timers)

### Concurrency & Threading
- **Thread Pool:** `rayon` for CPU-parallel tasks (effects, decoding)
- **Lock-Free Queues:** `crossbeam-channel` for decode→upload→render pipeline
- **Atomic State:** `parking_lot` for low-contention locks, `arc-swap` for config updates
- **Frame Scheduler:** Custom priority-based scheduler with tokio timers

### Utilities & Infrastructure
- **Logging:** `tracing` + `tracing-subscriber` (structured logging)
- **Config:** `serde` + `toml`/`ron` for settings, maintain XML compatibility for projects
- **Testing:** `cargo test` + `proptest` for property testing, `criterion` for benchmarks
- **CI/CD:** GitHub Actions with cross-platform matrix (Linux, macOS, Windows)

---

## Phase 0 Deliverables (Months 1-3)

### 1. Project Setup & Infrastructure

**Month 1, Weeks 1-2:**

**Repository Structure:**
```
mapmap-rs/
├── Cargo.toml           # Workspace root
├── crates/
│   ├── mapmap-core/     # Domain model (Paint/Mapping/Shape)
│   ├── mapmap-render/   # Graphics abstraction + wgpu backend
│   ├── mapmap-media/    # Video decode + texture streaming
│   ├── mapmap-ui/       # ImGui integration
│   ├── mapmap-control/  # MIDI/OSC/DMX handlers
│   └── mapmap-ffi/      # C API for plugins + NDI/DeckLink/Spout/Syphon
├── examples/            # Minimal test apps per crate
├── benches/             # Criterion benchmarks
├── shaders/             # GLSL/WGSL shaders
├── tests/               # Integration tests
└── docs/                # Architecture docs
```

**Build System:**
- Cargo workspace with 6+ internal crates
- `cargo-make` Makefile.toml for:
  - Shader compilation (glslc → SPIR-V)
  - C FFI header generation (cbindgen)
  - Cross-compilation targets (Linux x64, macOS arm64/x64, Windows x64)
- Pre-commit hooks: `rustfmt`, `clippy`, `cargo deny` (license/security checks)

**CI/CD Pipeline (GitHub Actions):**
- Matrix builds: Ubuntu 22.04, macOS 13 (Intel + ARM), Windows Server 2022
- Dependency caching (sccache for Rust, build artifact cache)
- Unit tests + integration tests
- Benchmark regression detection (criterion vs. baseline)
- Automated releases with `cargo-dist`

**Testing Framework:**
- Unit tests: `#[cfg(test)]` modules in each crate
- Integration tests: Real wgpu device + headless rendering
- Property tests: `proptest` for geometry math (warp transforms)
- Benchmarks: `criterion` for:
  - Texture upload throughput
  - Shader compilation
  - Frame scheduler latency
  - Media decode rates

**Documentation:**
- `cargo doc` for API documentation
- Architecture Decision Records (ADRs) for major choices
- ARCHITECTURE.md: System design, threading model, data flow

**Milestones:**
- ✅ Week 1: Repo structure, CI matrix green on all platforms
- ✅ Week 2: First benchmark (empty wgpu render loop), docs generated

---

### 2. Modern Rendering Abstraction

**Month 1, Weeks 3-4 + Month 2, Weeks 1-2:**

**Crate: `mapmap-render`**

**Core Abstractions:**
```rust
pub trait RenderBackend {
    fn create_texture(&mut self, desc: TextureDescriptor) -> TextureHandle;
    fn upload_texture(&mut self, handle: TextureHandle, data: &[u8]);
    fn create_shader(&mut self, source: ShaderSource) -> ShaderHandle;
    fn create_pipeline(&mut self, desc: PipelineDescriptor) -> PipelineHandle;
    fn submit_commands(&mut self, cmd: CommandBuffer);
}

pub struct WgpuBackend {
    device: wgpu::Device,
    queue: wgpu::Queue,
    staging_belt: wgpu::util::StagingBelt,
    texture_pool: TexturePool,
}
```

**Features:**
- Multi-backend support via trait (wgpu for now, vulkano later if needed)
- Texture pool with automatic reuse for same-sized allocations
- Shader hot-reloading (watch `shaders/` directory, recompile on change)
- Validation layers in debug builds (wgpu::Features::SHADER_VALIDATION)
- GPU profiler integration (`wgpu::Features::TIMESTAMP_QUERY`)

**Platform-Specific:**
- **Linux:** Vulkan via `wgpu` with X11/Wayland support
- **macOS:** Metal backend (wgpu default on macOS)
- **Windows:** DX12 primary, Vulkan fallback

**Error Handling:**
- Device lost recovery (recreate wgpu device, reload all resources)
- Shader compilation errors (log + fallback to solid color shader)
- OOM handling (reduce texture pool size, drop cached resources)

**Performance Targets:**
- <1ms texture upload for 1920x1080 RGBA (via staging buffers)
- <100μs pipeline bind switch
- Zero allocations in hot render loop

**Milestones:**
- ✅ Week 3: WgpuBackend trait + device initialization
- ✅ Week 4: Texture pool, shader compiler integration
- ✅ Month 2, Week 1: Timestamp queries, GPU profiler
- ✅ Month 2, Week 2: Device lost recovery, error handling tests

---

### 3. Basic Rendering (Triangle/Quad)

**Month 2, Weeks 2-3:**

**Implementation:**
```rust
// Vertex buffer for fullscreen quad
struct Vertex {
    position: [f32; 3],
    uv: [f32; 2],
}

// Simple textured quad shader (WGSL)
// shaders/textured_quad.wgsl
@vertex
fn vs_main(@location(0) pos: vec3<f32>, @location(1) uv: vec2<f32>) -> VertexOutput {
    var out: VertexOutput;
    out.clip_position = vec4<f32>(pos, 1.0);
    out.uv = uv;
    return out;
}

@fragment
fn fs_main(in: VertexOutput) -> @location(0) vec4<f32> {
    return textureSample(t_diffuse, s_diffuse, in.uv);
}
```

**Render Pass Structure:**
```rust
pub struct QuadRenderer {
    vertex_buffer: wgpu::Buffer,
    index_buffer: wgpu::Buffer,
    pipeline: wgpu::RenderPipeline,
    bind_group_layout: wgpu::BindGroupLayout,
}

impl QuadRenderer {
    pub fn draw(&self, encoder: &mut wgpu::CommandEncoder, texture: &Texture, target: &wgpu::TextureView) {
        let bind_group = create_bind_group(texture);
        let mut pass = encoder.begin_render_pass(...);
        pass.set_pipeline(&self.pipeline);
        pass.set_bind_group(0, &bind_group, &[]);
        pass.set_vertex_buffer(0, self.vertex_buffer.slice(..));
        pass.set_index_buffer(self.index_buffer.slice(..), wgpu::IndexFormat::Uint16);
        pass.draw_indexed(0..6, 0, 0..1);
    }
}
```

**Test Cases:**
- Render triangle with solid color (no texture)
- Render textured quad (512x512 test pattern)
- Render chain: quad A → framebuffer → quad B (compositing test)
- Render 100 quads at 60fps (early stress test)

**Milestones:**
- ✅ Week 2: Triangle rendering (solid color)
- ✅ Week 3: Textured quad, bind group management

---

### 4. Multi-Threaded Frame Scheduler

**Month 2, Week 4 + Month 3, Week 1:**

**Architecture:**
```
[Decode Thread] → [Upload Queue] → [Render Thread]
      ↓                                  ↓
[Media Files]                    [wgpu::Queue::submit]
      ↓                                  ↓
[FFmpeg/GStreamer]              [GPU Execution]
```

**Lock-Free Pipeline:**
```rust
use crossbeam_channel::{bounded, Sender, Receiver};

pub struct FramePipeline {
    // Decode → Upload
    decoded_frames: Sender<DecodedFrame>,
    upload_rx: Receiver<DecodedFrame>,

    // Upload → Render
    uploaded_textures: Sender<TextureHandle>,
    render_rx: Receiver<TextureHandle>,
}

// Decode thread (dedicated thread per video source)
fn decode_loop(pipeline: &FramePipeline, video_path: &Path) {
    let mut decoder = FFmpegDecoder::open(video_path).unwrap();
    loop {
        let frame = decoder.next_frame().unwrap();
        pipeline.decoded_frames.send(frame).unwrap();
    }
}

// Upload thread (runs on tokio threadpool)
async fn upload_loop(pipeline: &FramePipeline, backend: &mut WgpuBackend) {
    while let Ok(frame) = pipeline.upload_rx.recv() {
        let texture = backend.upload_texture_async(frame.data).await;
        pipeline.uploaded_textures.send(texture).unwrap();
    }
}

// Render thread (main thread with vsync)
fn render_loop(pipeline: &FramePipeline, renderer: &mut QuadRenderer) {
    while let Ok(texture) = pipeline.render_rx.recv_timeout(Duration::from_millis(16)) {
        renderer.draw(&texture);
        // Present to swapchain
    }
}
```

**Scheduler Features:**
- **Priority Levels:** UI (highest), Media Decode, Texture Upload, Effect Compute
- **Deadline Scheduling:** Each frame has target present time, scheduler reorders to meet deadlines
- **Backpressure:** Bounded channels (depth=3) prevent decode from outrunning upload
- **Adaptive Frame Skip:** If decode can't keep up, skip frames to maintain realtime playback

**Synchronization:**
- `wgpu::Queue::on_submitted_work_done` for GPU fence callbacks
- `parking_lot::RwLock` for shared render state (layer visibility, blend modes)
- `arc-swap::ArcSwap` for hot-swappable configuration (shader params, output mappings)

**Performance Monitoring:**
- Per-thread CPU usage (via `perf` on Linux, Instruments on macOS)
- Inter-thread latency histogram (tracing spans)
- Frame drop counter (render misses target present time)

**Milestones:**
- ✅ Week 4: Decode thread + upload thread separation
- ✅ Month 3, Week 1: Priority scheduler, backpressure tests

---

### 5. Texture Upload Pipeline (with PBOs)

**Month 3, Weeks 1-2:**

**wgpu Equivalent of PBOs:**
wgpu uses staging buffers (mappable `wgpu::Buffer` with `MAP_WRITE` usage) instead of PBOs. The workflow is identical:

1. **Map** staging buffer for CPU write
2. **Copy** decoded frame data into mapped buffer
3. **Unmap** staging buffer
4. **Record** `copy_buffer_to_texture` command
5. **Submit** command buffer to GPU queue

**Staging Buffer Pool:**
```rust
pub struct StagingPool {
    buffers: Vec<StagingBuffer>,
    free_list: VecDeque<usize>,
}

pub struct StagingBuffer {
    buffer: wgpu::Buffer,
    size: u64,
    mapped: bool,
}

impl StagingPool {
    pub fn get_buffer(&mut self, size: u64) -> &mut StagingBuffer {
        if let Some(idx) = self.free_list.pop_front() {
            if self.buffers[idx].size >= size {
                return &mut self.buffers[idx];
            }
        }
        // Allocate new staging buffer
        let buffer = device.create_buffer(&wgpu::BufferDescriptor {
            size,
            usage: wgpu::BufferUsages::MAP_WRITE | wgpu::BufferUsages::COPY_SRC,
            mapped_at_creation: false,
        });
        self.buffers.push(StagingBuffer { buffer, size, mapped: false });
        self.buffers.last_mut().unwrap()
    }

    pub fn recycle(&mut self, buffer_idx: usize) {
        self.free_list.push_back(buffer_idx);
    }
}
```

**Async Upload Path:**
```rust
pub async fn upload_texture_async(
    device: &wgpu::Device,
    queue: &wgpu::Queue,
    pool: &mut StagingPool,
    texture: &wgpu::Texture,
    data: &[u8],
) {
    let staging = pool.get_buffer(data.len() as u64);

    // Map buffer (async operation)
    let buffer_slice = staging.buffer.slice(..);
    buffer_slice.map_async(wgpu::MapMode::Write).await.unwrap();

    // Copy data (CPU-side)
    {
        let mut view = buffer_slice.get_mapped_range_mut();
        view.copy_from_slice(data);
    }
    staging.buffer.unmap();

    // Record copy command (GPU-side)
    let mut encoder = device.create_command_encoder(&Default::default());
    encoder.copy_buffer_to_texture(
        wgpu::ImageCopyBuffer {
            buffer: &staging.buffer,
            layout: wgpu::ImageDataLayout {
                offset: 0,
                bytes_per_row: Some(width * 4),
                rows_per_image: Some(height),
            },
        },
        texture.as_image_copy(),
        texture_size,
    );
    queue.submit(Some(encoder.finish()));

    // Recycle staging buffer after GPU finishes
    pool.recycle(staging_idx);
}
```

**Optimizations:**
- **Persistent Mapping:** Keep staging buffers mapped across frames (not yet in wgpu, use double-buffering)
- **Batch Uploads:** Coalesce multiple small textures into one command buffer
- **Compressed Uploads:** BC1/BC3/BC7 upload (smaller staging buffer, GPU decompression)
- **DMA Buf (Linux):** Import dmabuf from VA-API decode directly into wgpu texture (zero-copy)

**Performance Targets:**
- 4K RGBA upload: <2ms (60MB/s sustained)
- 1080p RGBA upload: <0.5ms
- Pool overhead: <50μs per get_buffer call

**Milestones:**
- ✅ Week 1: Staging buffer pool implementation
- ✅ Week 2: Async upload path, benchmark vs. synchronous path

---

### 6. Simple FFmpeg-Based Video Decode

**Month 3, Weeks 2-3:**

**Crate: `mapmap-media`**

**Decoder Abstraction:**
```rust
pub trait VideoDecoder: Send {
    fn next_frame(&mut self) -> Result<DecodedFrame, DecodeError>;
    fn seek(&mut self, timestamp: Duration) -> Result<(), DecodeError>;
    fn duration(&self) -> Duration;
    fn resolution(&self) -> (u32, u32);
}

pub struct DecodedFrame {
    pub data: Vec<u8>,      // RGBA8 or YUV420p
    pub format: PixelFormat,
    pub width: u32,
    pub height: u32,
    pub pts: Duration,
}
```

**FFmpeg Implementation (via `ffmpeg-next`):**
```rust
use ffmpeg_next as ffmpeg;

pub struct FFmpegDecoder {
    input_ctx: ffmpeg::format::context::Input,
    decoder: ffmpeg::decoder::Video,
    scaler: ffmpeg::software::scaling::Context,
    video_stream_idx: usize,
}

impl FFmpegDecoder {
    pub fn open(path: &Path) -> Result<Self, DecodeError> {
        let input_ctx = ffmpeg::format::input(&path)?;
        let video_stream = input_ctx.streams()
            .best(ffmpeg::media::Type::Video)
            .ok_or(DecodeError::NoVideoStream)?;

        let decoder = video_stream.codec().decoder().video()?;

        // Scaler to convert to RGBA8
        let scaler = ffmpeg::software::scaling::Context::get(
            decoder.format(),
            decoder.width(),
            decoder.height(),
            ffmpeg::format::Pixel::RGBA,
            decoder.width(),
            decoder.height(),
            ffmpeg::software::scaling::Flags::BILINEAR,
        )?;

        Ok(Self { input_ctx, decoder, scaler, video_stream_idx: video_stream.index() })
    }
}

impl VideoDecoder for FFmpegDecoder {
    fn next_frame(&mut self) -> Result<DecodedFrame, DecodeError> {
        for (stream, packet) in self.input_ctx.packets() {
            if stream.index() != self.video_stream_idx {
                continue;
            }

            self.decoder.send_packet(&packet)?;
            let mut decoded = ffmpeg::util::frame::Video::empty();

            if self.decoder.receive_frame(&mut decoded).is_ok() {
                let mut rgb_frame = ffmpeg::util::frame::Video::empty();
                self.scaler.run(&decoded, &mut rgb_frame)?;

                return Ok(DecodedFrame {
                    data: rgb_frame.data(0).to_vec(),
                    format: PixelFormat::RGBA8,
                    width: rgb_frame.width(),
                    height: rgb_frame.height(),
                    pts: Duration::from_secs_f64(
                        decoded.timestamp().unwrap_or(0) as f64 *
                        f64::from(stream.time_base())
                    ),
                });
            }
        }
        Err(DecodeError::EndOfStream)
    }
}
```

**Supported Formats (Phase 0):**
- Container: MP4, MOV, AVI, MKV
- Codecs: H.264, H.265, VP9, ProRes, HAP (via FFmpeg HAP decoder)
- Pixel Formats: YUV420p → RGBA8 conversion

**Hardware Acceleration (Deferred to Phase 1):**
- Linux: VA-API (`ffmpeg::hwaccel::vaapi`)
- macOS: VideoToolbox (`ffmpeg::hwaccel::videotoolbox`)
- Windows: DXVA2 / D3D11VA

**Looping & Playback:**
```rust
pub struct VideoPlayer {
    decoder: Box<dyn VideoDecoder>,
    current_time: Duration,
    playback_speed: f32,
    looping: bool,
}

impl VideoPlayer {
    pub fn update(&mut self, dt: Duration) {
        self.current_time += dt.mul_f32(self.playback_speed);

        if self.current_time >= self.decoder.duration() {
            if self.looping {
                self.decoder.seek(Duration::ZERO).unwrap();
                self.current_time = Duration::ZERO;
            } else {
                // Stop playback
            }
        }
    }
}
```

**Performance Targets:**
- 1080p H.264 decode: >60fps on modern CPU (single stream)
- 4K H.264 decode: >30fps
- Memory usage: <500MB for 5 concurrent 1080p streams

**Milestones:**
- ✅ Week 2: FFmpegDecoder implementation, test with H.264 files
- ✅ Week 3: VideoPlayer with looping, seek, speed control

---

### 7. Basic Windowing (Single Output)

**Month 3, Week 4:**

**Crate: `mapmap-ui`**

**Window Management (`winit`):**
```rust
use winit::event_loop::{EventLoop, ControlFlow};
use winit::window::WindowBuilder;

pub struct AppWindow {
    window: winit::window::Window,
    surface: wgpu::Surface,
    surface_config: wgpu::SurfaceConfiguration,
}

impl AppWindow {
    pub fn new(event_loop: &EventLoop<()>, device: &wgpu::Device) -> Self {
        let window = WindowBuilder::new()
            .with_title("MapMap - Output 1")
            .with_inner_size(winit::dpi::PhysicalSize::new(1920, 1080))
            .with_fullscreen(None) // Windowed for Phase 0
            .build(event_loop)
            .unwrap();

        let surface = unsafe { instance.create_surface(&window) }.unwrap();
        let surface_config = wgpu::SurfaceConfiguration {
            usage: wgpu::TextureUsages::RENDER_ATTACHMENT,
            format: wgpu::TextureFormat::Bgra8Unorm,
            width: 1920,
            height: 1080,
            present_mode: wgpu::PresentMode::Fifo, // VSync
            alpha_mode: wgpu::CompositeAlphaMode::Opaque,
        };
        surface.configure(device, &surface_config);

        Self { window, surface, surface_config }
    }

    pub fn render(&mut self, renderer: &QuadRenderer, texture: &wgpu::Texture) {
        let frame = self.surface.get_current_texture().unwrap();
        let view = frame.texture.create_view(&Default::default());

        // Render to swapchain
        renderer.draw(&mut encoder, texture, &view);

        frame.present();
    }
}
```

**Event Loop Integration:**
```rust
pub fn run_app() {
    let event_loop = EventLoop::new();
    let window = AppWindow::new(&event_loop, &device);
    let mut pipeline = FramePipeline::new();
    let mut renderer = QuadRenderer::new(&device);

    // Start decode + upload threads
    std::thread::spawn(move || decode_loop(&pipeline, "test_video.mp4"));
    tokio::spawn(upload_loop(pipeline.clone(), backend));

    event_loop.run(move |event, _, control_flow| {
        *control_flow = ControlFlow::Poll;

        match event {
            Event::WindowEvent { event: WindowEvent::CloseRequested, .. } => {
                *control_flow = ControlFlow::Exit;
            }
            Event::MainEventsCleared => {
                // Render loop tick
                if let Ok(texture) = pipeline.render_rx.try_recv() {
                    window.render(&renderer, &texture);
                }
            }
            _ => {}
        }
    });
}
```

**ImGui Integration:**
```rust
use imgui_wgpu::Renderer as ImGuiRenderer;
use imgui_winit_support::WinitPlatform;

pub struct ImGuiContext {
    imgui: imgui::Context,
    platform: WinitPlatform,
    renderer: ImGuiRenderer,
}

impl ImGuiContext {
    pub fn render_ui(&mut self, ui: &imgui::Ui, window: &AppWindow) {
        ui.window("Controls")
            .size([300.0, 400.0], imgui::Condition::FirstUseEver)
            .build(|| {
                ui.text("Video Playback");
                ui.slider("Speed", 0.0, 2.0, &mut playback_speed);
                ui.checkbox("Loop", &mut looping);

                if ui.button("Load Video") {
                    // File dialog (via rfd crate)
                }
            });
    }
}
```

**Features (Phase 0):**
- Single window output (windowed mode)
- VSync enabled (Fifo present mode)
- ImGui overlay for basic controls (playback speed, looping toggle)
- Window resize handling (recreate swapchain)

**Deferred to Phase 2:**
- Multi-window support (separate windows per projector)
- Fullscreen exclusive mode
- Borderless fullscreen
- Monitor topology detection

**Milestones:**
- ✅ Week 4: Windowed output with textured quad
- ✅ Week 4: ImGui overlay rendering

---

## Friction Point Resolutions

### 1. Rendering Abstraction (bgfx/Diligent are C++)
**Solution:** Use **wgpu** exclusively in Phase 0. It provides:
- Vulkan/Metal/DX12 backends (matches STRATEGY.md targets)
- Safe Rust API with excellent documentation
- Shader translation via naga (WGSL/GLSL/HLSL input)
- Active development (Mozilla/gfx-rs team)

**Alternative:** If wgpu lacks features (e.g., advanced compute), use **ash** for direct Vulkan control. Defer this decision to Phase 3 (Effects Pipeline) when compute requirements are clearer.

### 2. Qt/QML UI Option
**Solution:** Use **imgui-rs** for Phase 0-5 operator UI. ImGui advantages:
- Immediate-mode paradigm (trivial state management)
- Low overhead (ideal for live performance UI)
- Excellent wgpu integration (imgui-wgpu crate)
- Matches STRATEGY.md recommendation (ImGui for live operation)

**Alternative:** For Phase 6 (authoring UI), evaluate:
- **egui:** Pure Rust, similar to ImGui but with better layout
- **Qt 6 QML:** Via `qmetaobject-rs` or CXX.rs bindings (if Qt familiarity is critical)

Recommend **egui** to stay Rust-native unless customer base demands Qt.

### 3. JUCE/RtMidi References
**Solution:** Replace with Rust equivalents:
- **MIDI:** `midir` (cross-platform, low-latency)
- **Audio I/O:** `cpal` (supports ASIO on Windows, CoreAudio on macOS, ALSA/JACK on Linux)
- **Audio Analysis:** `rustfft` for FFT, hand-roll beat detection / envelope followers

**Phase 0:** MIDI input only (midir::MidiInput). Audio reactivity deferred to Phase 3.

### 4. NDI / DeckLink / Spout / Syphon
**Solution:** FFI bridges to C/C++/Obj-C SDKs. All expose stable ABIs:

**NDI (Phase 5 target):**
- Use `bindgen` to generate Rust bindings to `Processing.NDI.Lib.h`
- Link against `libndi.so` / `libndi.dylib` / `Processing.NDI.Lib.x64.dll`
- Receive: NDI receiver → RGBA frame → wgpu texture upload
- Send: wgpu texture download → RGBA frame → NDI sender

**DeckLink SDI (Phase 5 target):**
- **Windows:** Use `windows` crate to call `IDeckLink` COM interfaces
- **macOS:** Use `objc` crate to call `DeckLinkAPI.h` Obj-C classes
- **Linux:** DeckLink SDK provides C++ classes; write thin C wrapper, bind with `bindgen`

**Spout (Windows, Phase 5 target):**
- Spout 2 shares textures via DX11 shared handles
- Write C++ shim: `extern "C" void* spout_receive_texture(const char* name, int* width, int* height)`
- Import DX11 shared handle into wgpu via `wgpu::Device::create_texture_from_hal` (unsafe)

**Syphon (macOS, Phase 5 target):**
- Syphon shares IOSurface via Metal textures
- Write Obj-C wrapper: `void* syphon_receive_texture(const char* name)`
- Import IOSurface into wgpu Metal backend via `wgpu::Device::create_texture_from_hal`

**Phase 0 Action:** Create `mapmap-ffi` crate skeleton with placeholder headers. Implement in Phase 5.

### 5. OLA (DMX)
**Solution:** Bypass OLA entirely. Implement Art-Net/sACN directly:
- **Art-Net:** UDP protocol (port 6454), simple packet structure
- **sACN (E1.31):** UDP multicast, slightly more complex

```rust
use std::net::UdpSocket;

pub struct ArtNetSender {
    socket: UdpSocket,
    universe: u16,
}

impl ArtNetSender {
    pub fn send_dmx(&self, channels: &[u8; 512]) {
        let mut packet = vec![0u8; 18 + 512];
        packet[0..8].copy_from_slice(b"Art-Net\0");
        packet[8..10].copy_from_slice(&0x5000u16.to_le_bytes()); // OpDmx
        packet[10..12].copy_from_slice(&14u16.to_be_bytes()); // ProtVer
        packet[14..16].copy_from_slice(&self.universe.to_le_bytes());
        packet[16..18].copy_from_slice(&512u16.to_be_bytes());
        packet[18..].copy_from_slice(channels);

        self.socket.send_to(&packet, "255.255.255.255:6454").unwrap();
    }
}
```

**Phase 0 Action:** Defer to Phase 4 (Control Systems). Document Art-Net as primary DMX output.

### 6. FFmpeg/GStreamer
**Solution:** Both have mature Rust bindings. Choose based on needs:

**FFmpeg (`ffmpeg-next`):**
- Pros: Simple API, smaller dependency, easier cross-compilation
- Cons: Harder to access hardware decode on Linux (VA-API setup is manual)

**GStreamer (`gstreamer-rs`):**
- Pros: Automatic hardware decode (vaapidecodebin), plugin ecosystem, easier HAP integration
- Cons: Larger dependency (GStreamer runtime required), more complex API

**Phase 0 Decision:** Start with **ffmpeg-next** for simplicity. Add GStreamer backend in Phase 1 when hardware accel is critical.

**Hardware Decode Setup:**
```rust
// FFmpeg with VA-API (Linux)
use ffmpeg_next::hwaccel;

let mut decoder = video_stream.codec().decoder().video()?;
decoder.set_hwaccel(hwaccel::Vaapi)?;

// Decoded frames are in GPU memory (VASurface)
// Map to system memory or import as wgpu texture (via dmabuf)
```

---

## Architecture Decisions (ADRs)

### ADR-001: Rust as Primary Language
**Status:** Accepted

**Context:** MapMap legacy is C++/Qt. Rewrite offers opportunity to choose new language.

**Decision:** Use Rust 2021 for all new code except FFI shims to C libraries.

**Rationale:**
- Memory safety eliminates crashes in long-running live shows
- Fearless concurrency simplifies multi-threaded media pipeline
- wgpu provides production-ready graphics abstraction
- Growing ecosystem for media (FFmpeg, GStreamer, HAP)

**Consequences:**
- Team must learn Rust (2-4 week ramp-up for C++ developers)
- Some SDKs (NDI, DeckLink) require FFI (acceptable overhead)
- Compile times longer than C++ (mitigated by incremental compilation)

---

### ADR-002: wgpu as Rendering Backend
**Status:** Accepted

**Context:** Need Vulkan/Metal/DX12 abstraction. Options: wgpu, ash, custom wrapper.

**Decision:** Use **wgpu** for Phase 0-3. Re-evaluate in Phase 3 if compute needs aren't met.

**Rationale:**
- Safe API reduces GPU driver-related bugs
- Shader translation (naga) avoids manual SPIR-V/MSL/DXIL handling
- Active development, strong community support

**Consequences:**
- Limited to wgpu's feature set (no ray tracing in Phase 0)
- If advanced features needed (e.g., mesh shaders), may need ash fallback

---

### ADR-003: ImGui for Live UI, Defer Authoring UI
**Status:** Accepted

**Context:** STRATEGY.md recommends ImGui for live operation, Qt for authoring. Rust Qt bindings are immature.

**Decision:** Use **imgui-rs** exclusively in Phase 0-5. Defer authoring UI to Phase 6, re-evaluate Qt vs. egui.

**Rationale:**
- ImGui matches STRATEGY.md recommendation
- Immediate-mode paradigm simplifies state sync with render engine
- imgui-wgpu integration is production-ready

**Consequences:**
- Advanced authoring features (node graph editor) limited by ImGui capabilities
- May need to port to egui or Qt in Phase 6 (acceptable delay)

---

### ADR-004: FFmpeg over GStreamer for Phase 0
**Status:** Accepted

**Context:** Need video decode. Both FFmpeg and GStreamer have Rust bindings.

**Decision:** Use **ffmpeg-next** in Phase 0. Add GStreamer backend in Phase 1 when hardware accel is critical.

**Rationale:**
- Simpler API for basic decode (fewer moving parts)
- Smaller binary size (~20MB vs. ~100MB for GStreamer)
- Easier cross-compilation

**Consequences:**
- Hardware decode on Linux requires manual VA-API setup (more code)
- May need to rewrite decoder abstraction for GStreamer in Phase 1 (planned)

---

### ADR-005: Direct Art-Net/sACN, Skip OLA
**Status:** Accepted

**Context:** STRATEGY.md mentions OLA for DMX. OLA is C++ library with no Rust bindings.

**Decision:** Implement Art-Net and sACN directly in Rust. No OLA dependency.

**Rationale:**
- Art-Net/sACN protocols are simple (100 lines of code each)
- Avoids C++ dependency and OLA daemon requirement
- Industry standard (Resolume uses Art-Net/sACN, not OLA)

**Consequences:**
- No support for USB DMX adapters in Phase 0 (acceptable, Art-Net is industry norm)
- If USB DMX needed later, can add OLA FFI in Phase 4

---

## Risk Mitigation

### High-Risk: Multi-GPU Synchronization (Deferred to Phase 2)
**Risk:** Multiple outputs on different GPUs require frame sync (genlock).

**Mitigation:**
- Phase 0: Single GPU, single output only
- Phase 2: Research wgpu multi-adapter support (create devices per GPU, manual sync via fences)
- Fallback: Use NVIDIA Mosaic / AMD Eyefinity (OS-level multi-display as single GPU)

---

### High-Risk: Professional Video I/O (NDI/DeckLink)
**Risk:** FFI to C/C++/Obj-C SDKs may have bugs or performance issues.

**Mitigation:**
- Phase 0: Document FFI requirements in `mapmap-ffi` crate
- Phase 5: Prototype NDI receive in isolation (separate test program)
- Fallback: If FFI is too fragile, use SDI/NDI → ffmpeg → file → MapMap pipeline (higher latency)

---

### Medium-Risk: Cross-Platform Graphics
**Risk:** wgpu backends may have driver bugs (e.g., Intel Vulkan on Linux).

**Mitigation:**
- CI tests on real hardware (not just VMs)
- Maintain backend fallback order: Vulkan → OpenGL (via wgpu GLES3 backend)
- User-facing backend selection in settings (`wgpu::BackendBit` flags)

---

### Medium-Risk: Real-Time Performance
**Risk:** Rust async overhead or wgpu validation may cause frame drops.

**Mitigation:**
- Benchmark early and often (criterion benchmarks in CI)
- Profile with `cargo flamegraph`, `perf`, Xcode Instruments
- Disable wgpu validation in release builds (`wgpu::Features::empty()`)

---

## Success Metrics - Phase 0

**Functional:**
- ✅ Render 1080p video at 60fps (locked to VSync)
- ✅ Single window output with ImGui overlay
- ✅ Video looping, speed control (0.1x - 2.0x)
- ✅ Texture upload <1ms for 1080p RGBA

**Performance:**
- ✅ Frame time <16.6ms (60fps target)
- ✅ Decode → Upload → Render latency <50ms
- ✅ Memory usage <500MB for single 1080p stream
- ✅ Zero frame drops in 1-hour stress test

**Code Quality:**
- ✅ Clippy clean (no warnings in CI)
- ✅ >80% test coverage (unit + integration)
- ✅ All public APIs documented (cargo doc)
- ✅ CI green on Linux/macOS/Windows

**Documentation:**
- ✅ ARCHITECTURE.md explaining threading model
- ✅ ADRs for major decisions (5+ records)
- ✅ Per-crate README with examples
- ✅ Benchmark report (before/after comparisons)

---

## Phase 0 Timeline Breakdown

**Month 1:**
- **Week 1:** Repository structure, CI/CD pipeline, first green build
- **Week 2:** Cargo workspace setup, testing framework, first benchmark
- **Week 3:** WgpuBackend trait, device initialization, texture pool
- **Week 4:** Shader compiler integration, hot-reloading

**Month 2:**
- **Week 1:** GPU profiler (timestamp queries), error handling
- **Week 2:** Triangle rendering, textured quad, bind groups
- **Week 3:** Quad renderer, framebuffer compositing tests
- **Week 4:** Multi-threaded frame pipeline (decode + upload threads)

**Month 3:**
- **Week 1:** Priority scheduler, backpressure handling
- **Week 2:** Staging buffer pool, async texture upload, FFmpeg decoder skeleton
- **Week 3:** Video playback loop, seek, speed control
- **Week 4:** Windowed output, ImGui integration, Phase 0 demo

**Deliverables (End of Month 3):**
- ✅ Working demo: Play MP4 video in window, ImGui controls for playback
- ✅ All CI tests green
- ✅ Benchmark report (texture upload, decode throughput, frame time)
- ✅ Documentation (ARCHITECTURE.md, 5+ ADRs, API docs)

---

## Next Steps (Immediate Actions)

1. **Initialize Repository (Day 1):**
   ```bash
   cargo new --lib mapmap-core
   cargo new --lib mapmap-render
   cargo new --lib mapmap-media
   cargo new --lib mapmap-ui
   cargo new --lib mapmap-control
   cargo new --lib mapmap-ffi
   cargo new --bin mapmap
   ```

2. **Setup CI (Day 1-2):**
   - Create `.github/workflows/ci.yml` (Linux/macOS/Windows matrix)
   - Add `clippy`, `rustfmt`, `cargo test`, `cargo doc` jobs
   - Configure sccache for faster builds

3. **Write ARCHITECTURE.md (Day 3-5):**
   - Threading model diagram (decode/upload/render threads)
   - Data flow (FFmpeg → staging buffer → wgpu texture → swapchain)
   - Module dependency graph

4. **First Benchmark (Day 5):**
   - Empty wgpu render loop (measure baseline frame time)
   - Texture upload (1080p RGBA, measure throughput)

5. **Milestone Review (End of Week 1):**
   - Green CI on all platforms
   - Repository structure finalized
   - ARCHITECTURE.md first draft complete

---

## Appendix: Crate Dependency Graph

```
mapmap (binary)
  ├── mapmap-ui
  │   ├── mapmap-render
  │   └── imgui-rs
  ├── mapmap-media
  │   ├── ffmpeg-next
  │   └── mapmap-render (for texture uploads)
  ├── mapmap-control
  │   ├── midir
  │   └── rosc
  └── mapmap-core
      └── serde

mapmap-render
  ├── wgpu
  ├── winit
  └── naga

mapmap-ffi (Phase 5+)
  ├── bindgen (build dependency)
  └── libndi-sys (FFI bindings)
```

---

## Appendix: Shader Pipeline (Phase 0)

**WGSL Shader (textured_quad.wgsl):**
```wgsl
struct VertexInput {
    @location(0) position: vec3<f32>,
    @location(1) uv: vec2<f32>,
}

struct VertexOutput {
    @builtin(position) clip_position: vec4<f32>,
    @location(0) uv: vec2<f32>,
}

@vertex
fn vs_main(in: VertexInput) -> VertexOutput {
    var out: VertexOutput;
    out.clip_position = vec4<f32>(in.position, 1.0);
    out.uv = in.uv;
    return out;
}

@group(0) @binding(0)
var t_diffuse: texture_2d<f32>;
@group(0) @binding(1)
var s_diffuse: sampler;

@fragment
fn fs_main(in: VertexOutput) -> @location(0) vec4<f32> {
    return textureSample(t_diffuse, s_diffuse, in.uv);
}
```

**Build Process:**
1. **Dev:** wgpu loads WGSL directly (naga compiles at runtime)
2. **Release:** Pre-compile to SPIR-V with `naga` CLI (faster startup)

---

## Appendix: Performance Baselines (Target Hardware)

**Minimum Spec (Phase 0):**
- CPU: Intel i5-8400 / AMD Ryzen 5 2600
- GPU: NVIDIA GTX 1060 / AMD RX 580 (Vulkan 1.2)
- RAM: 8GB
- OS: Ubuntu 22.04 / macOS 12 / Windows 10

**Expected Performance:**
- 1080p video: 60fps (locked)
- 4K video: 30fps (decode-bound, GPU has headroom)
- Texture upload: 1080p RGBA in <1ms
- Frame latency: <50ms (decode to present)

**Stretch Spec (Phase 1+):**
- CPU: Intel i9-12900K / AMD Ryzen 9 5950X
- GPU: NVIDIA RTX 4070 / AMD RX 7800 XT
- RAM: 32GB
- OS: Ubuntu 24.04 / macOS 14 / Windows 11

**Expected Performance:**
- 4K video: 60fps (with hardware decode)
- 8K video: 30fps
- 10+ concurrent 1080p streams (effects applied)

---

## Conclusion

This plan provides a complete roadmap for Phase 0, establishing the foundational architecture for MapMap's Rust rewrite. By the end of Month 3, we will have:

1. **Production-grade infrastructure:** CI/CD, testing, benchmarks, documentation
2. **Modern graphics stack:** wgpu rendering with Vulkan/Metal/DX12 backends
3. **Multi-threaded media pipeline:** Decode/upload/render separation with lock-free queues
4. **Working demo:** Single-window video playback with ImGui controls

All subsequent phases (1-7) will build upon this foundation, adding professional features (multi-output, effects, control systems, pro I/O) while maintaining the performance and safety guarantees established in Phase 0.

**Recommendation:** Proceed with this plan. The Rust ecosystem is mature enough to meet all technical requirements outlined in STRATEGY.md, and the friction points (FFI to C libraries) are manageable with well-defined shim layers.

**Next Action:** Initialize repository structure and begin Month 1, Week 1 tasks (CI setup, project scaffolding).
