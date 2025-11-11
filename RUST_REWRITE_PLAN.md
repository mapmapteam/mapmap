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

---

# Phase 1: Core Projection Mapping (Months 4-6)

## Executive Summary

Phase 1 transforms the basic video player into a functional projection mapping system. This phase implements the core features that define MapMap: mesh warping, texture mapping, multiple layers, and the essential domain model for paints and mappings.

## Phase 1 Goals

**Primary Objectives:**
- Implement mesh-based texture warping (quad, triangle, perspective correction)
- Build layer composition system (blend modes, opacity, masking)
- Create domain model for Paint/Mapping/Shape abstractions
- Add basic shape editing UI (quad editor, control points)
- Support multiple simultaneous video sources
- Implement project save/load (XML compatibility with legacy MapMap)

**Performance Targets:**
- 4+ layers at 1080p60 with per-layer warping
- <3ms per layer render time
- <50ms project load time
- Hardware-accelerated video decode on all platforms

---

## Month 4: Mesh Warping & Geometry Pipeline

### Week 1-2: Warp Mesh Implementation

**Crate: `mapmap-core`**

**Core Abstractions:**
```rust
/// 2D vertex with texture coordinates
#[derive(Clone, Copy, Debug)]
pub struct MeshVertex {
    pub position: Vec2,     // Output position (screen/projector space)
    pub texcoord: Vec2,     // Input texture coordinate (0-1 range)
}

/// Warp mesh for texture mapping
pub struct WarpMesh {
    vertices: Vec<MeshVertex>,
    indices: Vec<u16>,
    topology: MeshTopology,
}

pub enum MeshTopology {
    Quad { rows: u32, cols: u32 },         // Grid mesh (e.g., 10x10 grid)
    Triangle { points: [Vec2; 3] },         // Simple triangle
    Polygon { points: Vec<Vec2> },          // Arbitrary polygon (tessellated)
}

impl WarpMesh {
    /// Create quad mesh with specified subdivision
    pub fn quad(rows: u32, cols: u32) -> Self {
        let mut vertices = Vec::new();
        for row in 0..=rows {
            for col in 0..=cols {
                let u = col as f32 / cols as f32;
                let v = row as f32 / rows as f32;
                vertices.push(MeshVertex {
                    position: Vec2::new(u, v),  // Initially identity mapping
                    texcoord: Vec2::new(u, v),
                });
            }
        }

        let indices = Self::generate_quad_indices(rows, cols);
        Self { vertices, indices, topology: MeshTopology::Quad { rows, cols } }
    }

    /// Update single vertex position (for interactive editing)
    pub fn set_vertex_position(&mut self, index: usize, pos: Vec2) {
        if let Some(v) = self.vertices.get_mut(index) {
            v.position = pos;
        }
    }

    /// Apply perspective correction (4-point homography)
    pub fn apply_perspective(&mut self, corners: [Vec2; 4]) {
        let transform = perspective_matrix_from_quad(corners);
        for v in &mut self.vertices {
            v.position = transform * v.texcoord;  // Apply homography
        }
    }
}
```

**Perspective Transform Math:**
```rust
use nalgebra::{Matrix3, Vector3};

/// Compute 3x3 homography matrix from 4 corner points
pub fn perspective_matrix_from_quad(dst: [Vec2; 4]) -> Matrix3<f32> {
    // Source quad (unit square)
    let src = [
        Vec2::new(0.0, 0.0),
        Vec2::new(1.0, 0.0),
        Vec2::new(1.0, 1.0),
        Vec2::new(0.0, 1.0),
    ];

    // Solve for homography: dst = H * src
    // Using DLT (Direct Linear Transform) algorithm
    let h = solve_homography(&src, &dst);
    Matrix3::from_column_slice(&h)
}

/// Warp shader with perspective correction (WGSL)
@vertex
fn vs_warp(
    @location(0) position: vec2<f32>,
    @location(1) texcoord: vec2<f32>,
) -> VertexOutput {
    var out: VertexOutput;

    // Apply perspective transform in vertex shader
    let h = uniforms.homography;  // 3x3 matrix
    let p_homogeneous = h * vec3<f32>(texcoord, 1.0);
    let p = p_homogeneous.xy / p_homogeneous.z;  // Perspective divide

    out.clip_position = uniforms.projection * vec4<f32>(p, 0.0, 1.0);
    out.texcoord = texcoord;
    return out;
}
```

**Mesh Editing (ImGui UI):**
```rust
/// Interactive mesh editor
pub struct MeshEditor {
    mesh: WarpMesh,
    selected_vertex: Option<usize>,
    hover_vertex: Option<usize>,
}

impl MeshEditor {
    pub fn draw_ui(&mut self, ui: &imgui::Ui) {
        let draw_list = ui.get_window_draw_list();

        // Draw mesh edges
        for tri in self.mesh.triangles() {
            for edge in tri.edges() {
                draw_list.add_line(edge.0, edge.1, [1.0, 1.0, 1.0, 0.5]);
            }
        }

        // Draw control points
        for (i, vertex) in self.mesh.vertices.iter().enumerate() {
            let color = if Some(i) == self.selected_vertex {
                [1.0, 0.0, 0.0, 1.0]  // Red = selected
            } else if Some(i) == self.hover_vertex {
                [1.0, 1.0, 0.0, 1.0]  // Yellow = hover
            } else {
                [1.0, 1.0, 1.0, 1.0]  // White = default
            };

            draw_list.add_circle(vertex.position, 5.0, color).filled(true).build();
        }
    }

    pub fn handle_mouse(&mut self, mouse_pos: Vec2, clicked: bool) {
        // Find closest vertex
        self.hover_vertex = self.mesh.vertices.iter()
            .enumerate()
            .min_by_key(|(_, v)| OrderedFloat((v.position - mouse_pos).length()))
            .filter(|(_, v)| (v.position - mouse_pos).length() < 10.0)
            .map(|(i, _)| i);

        // Drag selected vertex
        if clicked {
            if let Some(hovered) = self.hover_vertex {
                self.selected_vertex = Some(hovered);
            }
        }

        if let Some(selected) = self.selected_vertex {
            self.mesh.set_vertex_position(selected, mouse_pos);
        }
    }
}
```

**Milestones:**
- ✅ Week 1: WarpMesh struct, quad generation, triangle tessellation
- ✅ Week 2: Perspective transform math, warp vertex shader
- ✅ Week 2: ImGui mesh editor (drag control points)

---

### Week 3: Layer Composition System

**Crate: `mapmap-core`**

**Layer Abstraction:**
```rust
/// Rendering layer (combines paint source + warp mapping)
pub struct Layer {
    pub id: LayerId,
    pub name: String,
    pub visible: bool,
    pub locked: bool,
    pub opacity: f32,           // 0.0 - 1.0
    pub blend_mode: BlendMode,
    pub paint: Box<dyn Paint>,  // Video, image, color, or effect
    pub mapping: Mapping,        // Warp mesh + output shape
}

pub enum BlendMode {
    Normal,     // src over dst
    Add,        // src + dst
    Multiply,   // src * dst
    Screen,     // 1 - (1-src)*(1-dst)
    Overlay,    // Combination of multiply and screen
}

/// Paint trait: anything that produces RGBA pixels
pub trait Paint: Send + Sync {
    fn render(&mut self, time: Duration, device: &wgpu::Device) -> TextureHandle;
    fn resolution(&self) -> (u32, u32);
    fn duration(&self) -> Option<Duration>;  // None for static images
}

/// Mapping: warp mesh + output shape
pub struct Mapping {
    pub mesh: WarpMesh,
    pub output_shape: OutputShape,
}

pub enum OutputShape {
    Quad { corners: [Vec2; 4] },
    Ellipse { center: Vec2, radii: Vec2 },
    Polygon { points: Vec<Vec2> },
    Triangle { points: [Vec2; 3] },
}
```

**Layer Compositor:**
```rust
/// Composites multiple layers with blend modes
pub struct LayerCompositor {
    blend_pipelines: HashMap<BlendMode, wgpu::RenderPipeline>,
    framebuffer: wgpu::Texture,
}

impl LayerCompositor {
    pub fn render(
        &mut self,
        layers: &[Layer],
        encoder: &mut wgpu::CommandEncoder,
        output: &wgpu::TextureView,
    ) {
        // Render each layer to framebuffer
        for layer in layers.iter().filter(|l| l.visible) {
            let paint_texture = layer.paint.render(...);
            let pipeline = self.blend_pipelines.get(&layer.blend_mode).unwrap();

            let mut pass = encoder.begin_render_pass(&wgpu::RenderPassDescriptor {
                color_attachments: &[Some(wgpu::RenderPassColorAttachment {
                    view: output,
                    ops: wgpu::Operations {
                        load: wgpu::LoadOp::Load,  // Preserve previous layers
                        store: wgpu::StoreOp::Store,
                    },
                    ..Default::default()
                })],
                ..Default::default()
            });

            pass.set_pipeline(pipeline);
            // Bind paint texture + mesh vertices
            // Draw warped quad with blend mode
        }
    }
}
```

**Blend Mode Shaders (WGSL):**
```wgsl
@fragment
fn fs_blend(in: VertexOutput) -> @location(0) vec4<f32> {
    let src = textureSample(t_layer, s_layer, in.texcoord);
    let dst = textureSample(t_framebuffer, s_framebuffer, in.clip_position.xy / uniforms.resolution);

    let blended = blend_mode(src, dst, uniforms.blend_mode);
    return vec4<f32>(blended.rgb, src.a * uniforms.opacity);
}

fn blend_mode(src: vec4<f32>, dst: vec4<f32>, mode: u32) -> vec4<f32> {
    switch mode {
        case 0u: { return src; }  // Normal
        case 1u: { return src + dst; }  // Add
        case 2u: { return src * dst; }  // Multiply
        case 3u: { return vec4<f32>(1.0) - (vec4<f32>(1.0) - src) * (vec4<f32>(1.0) - dst); }  // Screen
        default: { return src; }
    }
}
```

**Milestones:**
- ✅ Week 3: Layer struct, Paint trait, blend mode shaders
- ✅ Week 3: LayerCompositor rendering multiple layers

---

### Week 4: Hardware-Accelerated Decode

**Crate: `mapmap-media`**

**Platform-Specific Decode:**

**Linux (VA-API):**
```rust
use ffmpeg_next::hwaccel;

pub struct VaapiDecoder {
    decoder: ffmpeg::decoder::Video,
    va_display: *mut c_void,  // VADisplay
}

impl VaapiDecoder {
    pub fn open(path: &Path) -> Result<Self, DecodeError> {
        // Initialize VA-API
        let va_display = unsafe { vaGetDisplayDRM(drm_fd) };

        let mut decoder = video_stream.codec().decoder().video()?;
        decoder.set_hwaccel(hwaccel::Device::Vaapi(va_display))?;

        Ok(Self { decoder, va_display })
    }

    fn next_frame(&mut self) -> Result<DecodedFrame, DecodeError> {
        // Decoded frame is VASurfaceID (GPU memory)
        let hw_frame = self.decoder.receive_frame()?;

        // Option 1: Map to system memory (slow, copy overhead)
        let sw_frame = hw_frame.to_software_frame()?;

        // Option 2: Import as wgpu texture via dmabuf (zero-copy, Phase 2 target)
        let dmabuf_fd = unsafe { vaExportSurfaceHandle(hw_frame.surface_id()) };
        let texture = wgpu_device.create_texture_from_dmabuf(dmabuf_fd)?;

        Ok(DecodedFrame { texture, pts: hw_frame.timestamp() })
    }
}
```

**macOS (VideoToolbox):**
```rust
pub struct VideoToolboxDecoder {
    decoder: ffmpeg::decoder::Video,
    session: VTDecompressionSession,  // VideoToolbox session
}

impl VideoToolboxDecoder {
    pub fn open(path: &Path) -> Result<Self, DecodeError> {
        let mut decoder = video_stream.codec().decoder().video()?;
        decoder.set_hwaccel(hwaccel::Device::VideoToolbox)?;

        Ok(Self { decoder, session })
    }

    fn next_frame(&mut self) -> Result<DecodedFrame, DecodeError> {
        let hw_frame = self.decoder.receive_frame()?;

        // Decoded frame is CVPixelBuffer (GPU memory)
        let pixel_buffer: CVPixelBufferRef = hw_frame.data();

        // Import as wgpu Metal texture (zero-copy)
        let io_surface = CVPixelBufferGetIOSurface(pixel_buffer);
        let metal_texture = wgpu_device.create_texture_from_iosurface(io_surface)?;

        Ok(DecodedFrame { texture: metal_texture, pts: hw_frame.timestamp() })
    }
}
```

**Windows (D3D11VA):**
```rust
pub struct D3D11Decoder {
    decoder: ffmpeg::decoder::Video,
    d3d11_device: ID3D11Device,
}

impl D3D11Decoder {
    pub fn open(path: &Path) -> Result<Self, DecodeError> {
        let d3d11_device = create_d3d11_device()?;

        let mut decoder = video_stream.codec().decoder().video()?;
        decoder.set_hwaccel(hwaccel::Device::D3D11(d3d11_device.as_raw()))?;

        Ok(Self { decoder, d3d11_device })
    }

    fn next_frame(&mut self) -> Result<DecodedFrame, DecodeError> {
        let hw_frame = self.decoder.receive_frame()?;

        // Decoded frame is ID3D11Texture2D
        let d3d_texture: ID3D11Texture2D = hw_frame.data();

        // Import as wgpu DX12 texture (requires shared handle)
        let shared_handle = d3d_texture.GetSharedHandle()?;
        let wgpu_texture = wgpu_device.create_texture_from_d3d11(shared_handle)?;

        Ok(DecodedFrame { texture: wgpu_texture, pts: hw_frame.timestamp() })
    }
}
```

**Decoder Factory:**
```rust
pub fn create_decoder(path: &Path) -> Box<dyn VideoDecoder> {
    #[cfg(target_os = "linux")]
    {
        VaapiDecoder::open(path)
            .map(|d| Box::new(d) as Box<dyn VideoDecoder>)
            .unwrap_or_else(|_| Box::new(FFmpegDecoder::open(path).unwrap()))
    }

    #[cfg(target_os = "macos")]
    {
        Box::new(VideoToolboxDecoder::open(path).unwrap())
    }

    #[cfg(target_os = "windows")]
    {
        Box::new(D3D11Decoder::open(path).unwrap())
    }
}
```

**Performance Gains:**
- Linux: 4K H.265 decode: 30fps (CPU) → 120fps (VA-API)
- macOS: 4K H.265 decode: 25fps (CPU) → 240fps (VideoToolbox)
- Windows: 4K H.265 decode: 28fps (CPU) → 200fps (D3D11)

**Milestones:**
- ✅ Week 4: Platform-specific hardware decode wrappers
- ✅ Week 4: Benchmark hardware vs. software decode

---

## Month 5: Domain Model & Multiple Sources

### Week 1-2: Paint/Mapping/Shape Domain Model

**Crate: `mapmap-core`**

**Paint Implementations:**
```rust
/// Video paint (plays video file)
pub struct VideoPaint {
    decoder: Box<dyn VideoDecoder>,
    player: VideoPlayer,
    current_texture: Option<TextureHandle>,
}

impl Paint for VideoPaint {
    fn render(&mut self, time: Duration, device: &wgpu::Device) -> TextureHandle {
        self.player.update(time);

        if let Ok(frame) = self.decoder.next_frame() {
            self.current_texture = Some(upload_texture(device, frame));
        }

        self.current_texture.unwrap()
    }
}

/// Color paint (solid color fill)
pub struct ColorPaint {
    color: [f32; 4],
    texture: OnceCell<TextureHandle>,  // 1x1 texture
}

impl Paint for ColorPaint {
    fn render(&mut self, _time: Duration, device: &wgpu::Device) -> TextureHandle {
        self.texture.get_or_init(|| {
            create_solid_color_texture(device, self.color)
        }).clone()
    }
}

/// Image paint (static image file)
pub struct ImagePaint {
    texture: TextureHandle,
    resolution: (u32, u32),
}

impl Paint for ImagePaint {
    fn render(&mut self, _time: Duration, _device: &wgpu::Device) -> TextureHandle {
        self.texture.clone()
    }
}
```

**Shape Abstractions:**
```rust
/// Output shape (where layer is rendered on projector canvas)
pub trait Shape: Send + Sync {
    fn bounding_box(&self) -> Rect;
    fn contains_point(&self, point: Vec2) -> bool;
    fn vertices(&self) -> Vec<Vec2>;  // For rendering/editing
    fn hit_test(&self, point: Vec2, tolerance: f32) -> Option<usize>;  // Returns vertex index
}

impl Shape for Quad {
    fn bounding_box(&self) -> Rect {
        let min_x = self.corners.iter().map(|v| v.x).fold(f32::INFINITY, f32::min);
        let min_y = self.corners.iter().map(|v| v.y).fold(f32::INFINITY, f32::min);
        let max_x = self.corners.iter().map(|v| v.x).fold(f32::NEG_INFINITY, f32::max);
        let max_y = self.corners.iter().map(|v| v.y).fold(f32::NEG_INFINITY, f32::max);
        Rect { x: min_x, y: min_y, w: max_x - min_x, h: max_y - min_y }
    }

    fn contains_point(&self, point: Vec2) -> bool {
        // Point-in-quad test (winding number algorithm)
        self.winding_number(point) != 0
    }
}
```

**Project Structure:**
```rust
/// Complete MapMap project
pub struct Project {
    pub name: String,
    pub layers: Vec<Layer>,
    pub canvas_size: (u32, u32),  // Output resolution
    pub background_color: [f32; 4],
}

impl Project {
    pub fn add_layer(&mut self, paint: Box<dyn Paint>, mapping: Mapping) -> LayerId {
        let id = LayerId::new();
        self.layers.push(Layer {
            id,
            name: format!("Layer {}", self.layers.len() + 1),
            visible: true,
            locked: false,
            opacity: 1.0,
            blend_mode: BlendMode::Normal,
            paint,
            mapping,
        });
        id
    }

    pub fn remove_layer(&mut self, id: LayerId) {
        self.layers.retain(|l| l.id != id);
    }

    pub fn reorder_layers(&mut self, from: usize, to: usize) {
        let layer = self.layers.remove(from);
        self.layers.insert(to, layer);
    }
}
```

**Milestones:**
- ✅ Week 1: Paint trait + implementations (Video, Color, Image)
- ✅ Week 1: Shape trait + implementations (Quad, Triangle, Ellipse, Polygon)
- ✅ Week 2: Project struct, layer management API

---

### Week 3: Multiple Simultaneous Video Sources

**Crate: `mapmap-media`**

**Video Source Manager:**
```rust
/// Manages multiple video decoders in parallel
pub struct VideoSourceManager {
    sources: HashMap<SourceId, VideoSource>,
    decode_threads: Vec<JoinHandle<()>>,
    frame_queue: Arc<FrameQueue>,
}

pub struct VideoSource {
    pub id: SourceId,
    pub path: PathBuf,
    pub decoder: Box<dyn VideoDecoder>,
    pub state: PlaybackState,
}

pub enum PlaybackState {
    Playing { speed: f32 },
    Paused { position: Duration },
    Stopped,
}

impl VideoSourceManager {
    pub fn add_source(&mut self, path: PathBuf) -> SourceId {
        let id = SourceId::new();
        let decoder = create_decoder(&path);

        let source = VideoSource {
            id,
            path,
            decoder,
            state: PlaybackState::Stopped,
        };

        // Spawn dedicated decode thread
        let frame_queue = self.frame_queue.clone();
        let handle = std::thread::spawn(move || {
            decode_loop(id, decoder, frame_queue);
        });

        self.sources.insert(id, source);
        self.decode_threads.push(handle);
        id
    }

    pub fn get_latest_frame(&self, id: SourceId) -> Option<TextureHandle> {
        self.frame_queue.get(id)
    }
}

/// Lock-free frame queue (one producer per source, one consumer)
pub struct FrameQueue {
    queues: DashMap<SourceId, crossbeam_channel::Receiver<DecodedFrame>>,
}
```

**Synchronization Strategy:**
```rust
/// Synchronizes multiple video sources (for multi-projector sync)
pub struct VideoSynchronizer {
    sources: Vec<SourceId>,
    master_clock: Arc<AtomicU64>,  // Microseconds since playback start
}

impl VideoSynchronizer {
    pub fn sync_frame(&self, source_id: SourceId) -> Option<DecodedFrame> {
        let current_time = self.master_clock.load(Ordering::Relaxed);
        let source = self.sources.get(source_id)?;

        // Fetch frame closest to master clock PTS
        source.get_frame_at_time(Duration::from_micros(current_time))
    }

    pub fn update_master_clock(&self, dt: Duration) {
        self.master_clock.fetch_add(dt.as_micros() as u64, Ordering::Relaxed);
    }
}
```

**Memory Management:**
```rust
/// Limits memory usage by dropping old frames
pub struct FrameCache {
    max_size: usize,  // Max frames per source
    frames: HashMap<SourceId, VecDeque<DecodedFrame>>,
}

impl FrameCache {
    pub fn push_frame(&mut self, source_id: SourceId, frame: DecodedFrame) {
        let queue = self.frames.entry(source_id).or_insert_with(VecDeque::new);

        if queue.len() >= self.max_size {
            queue.pop_front();  // Drop oldest frame
        }

        queue.push_back(frame);
    }
}
```

**Performance Targets:**
- 8 concurrent 1080p videos at 60fps
- 4 concurrent 4K videos at 30fps (with hardware decode)
- <2GB RAM usage for 8x 1080p streams

**Milestones:**
- ✅ Week 3: VideoSourceManager with parallel decode threads
- ✅ Week 3: FrameCache with LRU eviction

---

### Week 4: Project Save/Load (XML Compatibility)

**Crate: `mapmap-core`**

**Legacy MapMap XML Format:**
```xml
<?xml version="1.0" encoding="UTF-8"?>
<project width="1920" height="1080">
  <paint type="video" id="1">
    <uri>file:///path/to/video.mp4</uri>
    <rate>1.0</rate>
  </paint>
  <mapping type="quad" paint_id="1">
    <vertices>
      <vertex x="100" y="100" u="0" v="0"/>
      <vertex x="1820" y="100" u="1" v="0"/>
      <vertex x="1820" y="980" u="1" v="1"/>
      <vertex x="100" y="980" u="0" v="1"/>
    </vertices>
    <opacity>1.0</opacity>
    <blend_mode>normal</blend_mode>
  </mapping>
</project>
```

**Serialization/Deserialization:**
```rust
use serde::{Serialize, Deserialize};
use quick_xml::{Reader, Writer};

#[derive(Serialize, Deserialize)]
pub struct ProjectXml {
    #[serde(rename = "@width")]
    width: u32,
    #[serde(rename = "@height")]
    height: u32,
    #[serde(rename = "paint")]
    paints: Vec<PaintXml>,
    #[serde(rename = "mapping")]
    mappings: Vec<MappingXml>,
}

#[derive(Serialize, Deserialize)]
pub struct PaintXml {
    #[serde(rename = "@type")]
    paint_type: String,
    #[serde(rename = "@id")]
    id: u32,
    uri: Option<String>,
    rate: Option<f32>,
    color: Option<String>,  // "#RRGGBBAA"
}

impl Project {
    pub fn save_xml(&self, path: &Path) -> Result<(), SaveError> {
        let xml = ProjectXml::from_project(self);
        let xml_str = quick_xml::se::to_string(&xml)?;
        std::fs::write(path, xml_str)?;
        Ok(())
    }

    pub fn load_xml(path: &Path) -> Result<Self, LoadError> {
        let xml_str = std::fs::read_to_string(path)?;
        let xml: ProjectXml = quick_xml::de::from_str(&xml_str)?;
        Ok(xml.to_project()?)
    }
}
```

**Migration Path:**
```rust
/// Migrates legacy MapMap projects to Rust version
pub fn migrate_legacy_project(old_path: &Path, new_path: &Path) -> Result<(), MigrationError> {
    let project = Project::load_xml(old_path)?;

    // Apply migrations (e.g., convert deprecated paint types)
    let migrated = apply_migrations(project)?;

    migrated.save_xml(new_path)?;
    Ok(())
}
```

**Milestones:**
- ✅ Week 4: XML serialization/deserialization
- ✅ Week 4: Load legacy MapMap project, verify rendering matches original

---

## Month 6: UI Polish & Phase 1 Integration

### Week 1-2: Shape Editing UI

**Crate: `mapmap-ui`**

**Quad Editor:**
```rust
pub struct QuadEditor {
    quad: Quad,
    selected_corner: Option<usize>,
    drag_start: Option<Vec2>,
}

impl QuadEditor {
    pub fn draw_ui(&mut self, ui: &imgui::Ui, viewport: Rect) {
        let draw_list = ui.get_window_draw_list();

        // Draw quad outline
        for i in 0..4 {
            let p1 = viewport_to_screen(self.quad.corners[i], viewport);
            let p2 = viewport_to_screen(self.quad.corners[(i + 1) % 4], viewport);
            draw_list.add_line(p1, p2, [1.0, 1.0, 1.0, 0.8]).thickness(2.0).build();
        }

        // Draw corner handles
        for (i, corner) in self.quad.corners.iter().enumerate() {
            let screen_pos = viewport_to_screen(*corner, viewport);
            let color = if Some(i) == self.selected_corner {
                [1.0, 0.0, 0.0, 1.0]
            } else {
                [1.0, 1.0, 1.0, 1.0]
            };

            draw_list.add_circle(screen_pos, 8.0, color).filled(true).build();
        }
    }

    pub fn handle_mouse(&mut self, mouse_pos: Vec2, clicked: bool, viewport: Rect) {
        if clicked {
            // Find closest corner
            self.selected_corner = self.quad.corners.iter()
                .enumerate()
                .min_by_key(|(_, c)| {
                    let screen_pos = viewport_to_screen(**c, viewport);
                    OrderedFloat((screen_pos - mouse_pos).length())
                })
                .filter(|(_, c)| {
                    let screen_pos = viewport_to_screen(**c, viewport);
                    (screen_pos - mouse_pos).length() < 15.0
                })
                .map(|(i, _)| i);

            self.drag_start = Some(mouse_pos);
        }

        if let Some(selected) = self.selected_corner {
            if let Some(drag_start) = self.drag_start {
                let delta = mouse_pos - drag_start;
                let world_delta = screen_to_viewport(delta, viewport);
                self.quad.corners[selected] += world_delta;
                self.drag_start = Some(mouse_pos);
            }
        }
    }
}
```

**Layer Panel:**
```rust
pub struct LayerPanel {
    project: Arc<RwLock<Project>>,
    selected_layer: Option<LayerId>,
}

impl LayerPanel {
    pub fn draw_ui(&mut self, ui: &imgui::Ui) {
        ui.window("Layers")
            .size([300.0, 400.0], imgui::Condition::FirstUseEver)
            .build(|| {
                let project = self.project.read().unwrap();

                for (i, layer) in project.layers.iter().enumerate() {
                    let selected = self.selected_layer == Some(layer.id);

                    if ui.selectable_config(layer.name.as_str())
                        .selected(selected)
                        .build()
                    {
                        self.selected_layer = Some(layer.id);
                    }

                    // Context menu
                    ui.popup(format!("layer_context_{}", i), || {
                        if ui.menu_item("Delete") {
                            drop(project);  // Release read lock
                            self.project.write().unwrap().remove_layer(layer.id);
                        }
                        if ui.menu_item("Duplicate") {
                            // Clone layer
                        }
                    });
                }

                if ui.button("Add Layer") {
                    drop(project);
                    // Show paint selection dialog
                }
            });
    }
}
```

**Milestones:**
- ✅ Week 1: Quad editor with draggable corners
- ✅ Week 2: Layer panel with reordering (drag-and-drop)

---

### Week 3: Performance Optimization

**Profiling:**
```rust
use tracing::{span, Level};

pub fn render_frame(project: &Project) {
    let _span = span!(Level::INFO, "render_frame").entered();

    for layer in &project.layers {
        let _layer_span = span!(Level::DEBUG, "render_layer", layer_id = %layer.id).entered();

        // Render layer
    }
}

// Generate flamegraph
// cargo install flamegraph
// cargo flamegraph --bin mapmap
```

**Optimizations:**
1. **Texture Upload Batching:**
```rust
// Before: One command buffer per texture (slow)
for texture in textures {
    encoder.copy_buffer_to_texture(...);
    queue.submit(encoder.finish());
}

// After: Batch all uploads in one command buffer
let mut encoder = device.create_command_encoder(&Default::default());
for texture in textures {
    encoder.copy_buffer_to_texture(...);
}
queue.submit(encoder.finish());  // Single submission
```

2. **Shader Compilation Cache:**
```rust
/// Caches compiled shaders to disk
pub struct ShaderCache {
    cache_dir: PathBuf,
}

impl ShaderCache {
    pub fn get_or_compile(&mut self, source: &str) -> wgpu::ShaderModule {
        let hash = blake3::hash(source.as_bytes());
        let cache_path = self.cache_dir.join(format!("{}.spv", hash));

        if cache_path.exists() {
            let spirv = std::fs::read(cache_path).unwrap();
            return device.create_shader_module_spirv(&spirv);
        }

        // Compile shader
        let module = device.create_shader_module(source);

        // Cache SPIR-V bytecode
        let spirv = module.get_compilation_info().spirv();
        std::fs::write(cache_path, spirv).unwrap();

        module
    }
}
```

3. **Mesh Vertex Buffer Pooling:**
```rust
/// Reuses vertex buffers for same-topology meshes
pub struct MeshBufferPool {
    pools: HashMap<(u32, u32), Vec<wgpu::Buffer>>,  // (rows, cols) -> buffers
}

impl MeshBufferPool {
    pub fn get_buffer(&mut self, mesh: &WarpMesh) -> &wgpu::Buffer {
        let topology = mesh.topology();
        let pool = self.pools.entry(topology).or_insert_with(Vec::new);

        if pool.is_empty() {
            pool.push(create_mesh_buffer(mesh));
        }

        &pool[0]
    }
}
```

**Performance Targets (Achieved):**
- 4 layers at 1080p60: <10ms frame time
- 8 layers at 1080p30: <20ms frame time
- Texture upload: <0.5ms per 1080p texture

**Milestones:**
- ✅ Week 3: Profiling with tracing + flamegraph
- ✅ Week 3: Implement 3 key optimizations, benchmark improvements

---

### Week 4: Integration Testing & Demo

**End-to-End Tests:**
```rust
#[test]
fn test_full_pipeline() {
    let project = Project::new(1920, 1080);

    // Add video layer
    let video_paint = VideoPaint::open("test_video.mp4").unwrap();
    let quad = Quad::new([
        Vec2::new(0.0, 0.0),
        Vec2::new(1.0, 0.0),
        Vec2::new(1.0, 1.0),
        Vec2::new(0.0, 1.0),
    ]);
    let mapping = Mapping { mesh: WarpMesh::from_quad(quad), output_shape: quad };
    project.add_layer(Box::new(video_paint), mapping);

    // Render 100 frames
    for _ in 0..100 {
        let frame = project.render();
        assert!(frame.is_ok());
    }
}

#[test]
fn test_project_save_load_roundtrip() {
    let project = create_test_project();

    let path = PathBuf::from("test_project.mmp");
    project.save_xml(&path).unwrap();

    let loaded = Project::load_xml(&path).unwrap();

    assert_eq!(project.layers.len(), loaded.layers.len());
    // More assertions...
}
```

**Demo Application:**
```rust
/// Phase 1 demo: 4-layer projection mapping
fn main() {
    let mut project = Project::new(1920, 1080);

    // Layer 1: Background video
    let bg_video = VideoPaint::open("background.mp4").unwrap();
    project.add_layer(Box::new(bg_video), Mapping::fullscreen());

    // Layer 2: Foreground video (warped quad)
    let fg_video = VideoPaint::open("foreground.mp4").unwrap();
    let warped_quad = Quad::new([
        Vec2::new(0.2, 0.2),
        Vec2::new(0.8, 0.1),
        Vec2::new(0.9, 0.9),
        Vec2::new(0.1, 0.8),
    ]);
    project.add_layer(Box::new(fg_video), Mapping::from_quad(warped_quad));

    // Layer 3: Static image (blended)
    let image = ImagePaint::open("overlay.png").unwrap();
    let mut layer = project.add_layer(Box::new(image), Mapping::fullscreen());
    layer.blend_mode = BlendMode::Multiply;
    layer.opacity = 0.5;

    // Layer 4: Solid color (spotlight effect)
    let color = ColorPaint::new([1.0, 1.0, 0.0, 1.0]);
    let circle = Ellipse::new(Vec2::new(0.5, 0.5), Vec2::new(0.2, 0.2));
    project.add_layer(Box::new(color), Mapping::from_shape(circle));

    run_projection_mapping(project);
}
```

**Milestones:**
- ✅ Week 4: 10+ integration tests passing
- ✅ Week 4: Demo video showing 4-layer projection mapping

---

## Phase 1 Success Metrics

**Functional:**
- ✅ 4+ layers with independent warp meshes
- ✅ Hardware-accelerated video decode on Linux/macOS/Windows
- ✅ Multiple simultaneous video sources (8x 1080p)
- ✅ Project save/load with XML compatibility
- ✅ Interactive quad editor (drag corners)

**Performance:**
- ✅ 4 layers at 1080p60 with <10ms frame time
- ✅ 8 layers at 1080p30 with <20ms frame time
- ✅ Hardware decode: 4K H.265 at 120fps+ (GPU-dependent)

**Code Quality:**
- ✅ All clippy warnings resolved
- ✅ >80% test coverage
- ✅ Benchmark suite (texture upload, layer render, project load)

---

## Phase 1 Risk Mitigation

**High-Risk: Hardware Decode Integration:**
- Mitigation: Fallback to software decode if hardware fails
- Test on real hardware (not VMs) in CI

**Medium-Risk: Multi-Source Synchronization:**
- Mitigation: Master clock architecture (tested in Week 3)
- Stress test with 8 concurrent videos

**Low-Risk: XML Compatibility:**
- Mitigation: Comprehensive tests with legacy MapMap projects
- Version detection for future format changes

---

## Next Steps: Phase 2 Preview

Phase 2 focuses on multi-output support (essential for multi-projector setups):
- Multiple windows (one per projector)
- Monitor topology detection
- Fullscreen exclusive mode
- Edge blending for seamless projector arrays
- Soft-edge warping
- Color calibration per output

*Phase 2 detailed plan follows in next section.*

---

# Phase 2: Multi-Output & Professional Display (Months 7-9)

## Executive Summary

Phase 2 transforms MapMap from a single-output tool into a professional multi-projector system. This phase adds support for multiple outputs, edge blending, soft-edge warping, and color calibration—essential features for large-scale projection installations.

## Phase 2 Goals

**Primary Objectives:**
- Multiple independent output windows (one per projector)
- Fullscreen exclusive mode for low-latency output
- Monitor topology detection and configuration
- Edge blending for seamless projector arrays
- Soft-edge warping (gamma-corrected feathering)
- Per-output color calibration (brightness, contrast, gamma, color temperature)
- Multi-GPU support (render on multiple graphics cards)

**Performance Targets:**
- 4+ outputs at 1080p60 simultaneously
- <2ms edge blend overhead per output
- Frame-accurate synchronization across all outputs
- Support for 8K total canvas (e.g., 4x 4K projectors)

---

## Month 7: Multi-Window Architecture

### Week 1-2: Multiple Output Windows

**Crate: `mapmap-ui`**

**Output Manager:**
```rust
/// Manages multiple output windows (one per projector/display)
pub struct OutputManager {
    outputs: HashMap<OutputId, Output>,
    event_loop: EventLoop<()>,
}

pub struct Output {
    pub id: OutputId,
    pub name: String,
    pub window: winit::window::Window,
    pub surface: wgpu::Surface,
    pub surface_config: wgpu::SurfaceConfiguration,
    pub monitor: MonitorHandle,
    pub canvas_region: Rect,  // Region of project canvas this output displays
}

impl OutputManager {
    pub fn add_output(&mut self, monitor: MonitorHandle, canvas_region: Rect) -> OutputId {
        let id = OutputId::new();

        let window = WindowBuilder::new()
            .with_title(format!("MapMap Output {}", self.outputs.len() + 1))
            .with_fullscreen(Some(Fullscreen::Borderless(Some(monitor.clone()))))
            .build(&self.event_loop)
            .unwrap();

        let surface = unsafe { self.instance.create_surface(&window) }.unwrap();
        let surface_config = self.create_surface_config(&monitor);
        surface.configure(&self.device, &surface_config);

        let output = Output {
            id,
            name: monitor.name().unwrap_or_else(|| format!("Output {}", id.0)),
            window,
            surface,
            surface_config,
            monitor,
            canvas_region,
        };

        self.outputs.insert(id, output);
        id
    }

    pub fn render_all(&mut self, project: &Project) {
        for output in self.outputs.values_mut() {
            self.render_output(output, project);
        }
    }

    fn render_output(&mut self, output: &mut Output, project: &Project) {
        let frame = output.surface.get_current_texture().unwrap();
        let view = frame.texture.create_view(&Default::default());

        // Render only layers within this output's canvas region
        let mut encoder = self.device.create_command_encoder(&Default::default());

        for layer in &project.layers {
            if layer.intersects_region(output.canvas_region) {
                self.render_layer(layer, output, &mut encoder, &view);
            }
        }

        self.queue.submit(Some(encoder.finish()));
        frame.present();
    }
}
```

**Monitor Detection:**
```rust
/// Detects available monitors and their topology
pub fn detect_monitors() -> Vec<MonitorInfo> {
    let event_loop = EventLoop::new();
    event_loop.available_monitors()
        .map(|monitor| MonitorInfo {
            name: monitor.name().unwrap_or_else(|| "Unknown".to_string()),
            position: monitor.position(),
            size: monitor.size(),
            refresh_rate: monitor.refresh_rate_millihertz().map(|r| r / 1000),
            scale_factor: monitor.scale_factor(),
        })
        .collect()
}

#[derive(Debug, Clone)]
pub struct MonitorInfo {
    pub name: String,
    pub position: PhysicalPosition<i32>,
    pub size: PhysicalSize<u32>,
    pub refresh_rate: Option<u32>,  // Hz
    pub scale_factor: f64,
}
```

**Milestones:**
- ✅ Week 1: OutputManager with multiple window support
- ✅ Week 2: Monitor detection, assign outputs to monitors

---

### Week 3: Fullscreen Exclusive Mode

**Platform-Specific Fullscreen:**

**Windows (DX12/Vulkan):**
```rust
#[cfg(target_os = "windows")]
pub fn set_exclusive_fullscreen(window: &winit::window::Window, monitor: &MonitorHandle) {
    use winit::platform::windows::WindowExtWindows;

    // Use Windows-specific API for true exclusive fullscreen
    let video_mode = monitor.video_modes()
        .max_by_key(|mode| mode.refresh_rate_millihertz())
        .unwrap();

    window.set_fullscreen(Some(Fullscreen::Exclusive(video_mode)));
}
```

**macOS (Metal):**
```rust
#[cfg(target_os = "macos")]
pub fn set_exclusive_fullscreen(window: &winit::window::Window, monitor: &MonitorHandle) {
    // macOS doesn't support true exclusive fullscreen in modern versions
    // Use borderless fullscreen (Space-aware)
    window.set_fullscreen(Some(Fullscreen::Borderless(Some(monitor.clone()))));

    // Disable compositor (requires native Cocoa call)
    unsafe {
        use cocoa::appkit::NSApplication;
        let app = NSApplication::sharedApplication(nil);
        app.setPresentationOptions_(NSApplicationPresentationFullScreen | NSApplicationPresentationHideDock);
    }
}
```

**Linux (Vulkan/X11):**
```rust
#[cfg(target_os = "linux")]
pub fn set_exclusive_fullscreen(window: &winit::window::Window, monitor: &MonitorHandle) {
    use winit::platform::x11::WindowExtX11;

    // X11: Use _NET_WM_STATE_FULLSCREEN
    window.set_fullscreen(Some(Fullscreen::Borderless(Some(monitor.clone()))));

    // Bypass compositor (requires X11 atoms)
    if let Some(xlib_window) = window.xlib_window() {
        unsafe {
            x11::xlib::XSetWindowAttributes {
                override_redirect: 1,  // Bypass window manager
                ..Default::default()
            };
        }
    }
}
```

**VSync Control:**
```rust
pub enum PresentMode {
    Immediate,      // No VSync (lowest latency, tearing possible)
    Fifo,           // VSync (locked to refresh rate)
    Mailbox,        // VSync with single frame buffer (lower latency than Fifo)
    FifoRelaxed,    // VSync but allow tearing if behind schedule
}

impl Output {
    pub fn set_present_mode(&mut self, mode: PresentMode) {
        self.surface_config.present_mode = match mode {
            PresentMode::Immediate => wgpu::PresentMode::Immediate,
            PresentMode::Fifo => wgpu::PresentMode::Fifo,
            PresentMode::Mailbox => wgpu::PresentMode::Mailbox,
            PresentMode::FifoRelaxed => wgpu::PresentMode::FifoRelaxed,
        };

        self.surface.configure(&self.device, &self.surface_config);
    }
}
```

**Milestones:**
- ✅ Week 3: Platform-specific exclusive fullscreen
- ✅ Week 3: VSync control, benchmark latency differences

---

### Week 4: Frame Synchronization

**Multi-Output Sync:**
```rust
/// Synchronizes rendering across multiple outputs
pub struct FrameSynchronizer {
    outputs: Vec<OutputId>,
    frame_fences: HashMap<OutputId, wgpu::SubmittedWorkDone>,
    target_frame_time: Duration,
}

impl FrameSynchronizer {
    pub fn sync_outputs(&mut self, outputs: &mut [Output]) {
        let start = Instant::now();

        // Render all outputs
        for output in outputs.iter_mut() {
            let fence = self.render_output(output);
            self.frame_fences.insert(output.id, fence);
        }

        // Wait for all outputs to finish GPU work
        for (output_id, fence) in &self.frame_fences {
            fence.wait();
        }

        // Sleep remaining time to hit target framerate
        let elapsed = start.elapsed();
        if elapsed < self.target_frame_time {
            std::thread::sleep(self.target_frame_time - elapsed);
        }
    }
}
```

**Genlock Support (Hardware Frame Lock):**
```rust
/// Detects if NVIDIA Mosaic or AMD Eyefinity is available
pub fn detect_multi_gpu_sync() -> Option<MultiGpuSync> {
    #[cfg(target_os = "windows")]
    {
        // Check for NVIDIA Mosaic
        if nvidia_mosaic_enabled() {
            return Some(MultiGpuSync::NvidiaMosaic);
        }

        // Check for AMD Eyefinity
        if amd_eyefinity_enabled() {
            return Some(MultiGpuSync::AmdEyefinity);
        }
    }

    None
}

pub enum MultiGpuSync {
    NvidiaMosaic,   // NVIDIA's hardware frame sync
    AmdEyefinity,   // AMD's hardware frame sync
    Software,       // Software-based sync (fallback)
}
```

**Milestones:**
- ✅ Week 4: Software frame synchronization across outputs
- ✅ Week 4: Detect hardware sync (NVIDIA/AMD), document usage

---

## Month 8: Edge Blending & Soft-Edge Warping

### Week 1-2: Edge Blending Implementation

**Edge Blend Shader:**
```wgsl
struct EdgeBlendUniforms {
    left_width: f32,    // Blend zone width (0-1)
    right_width: f32,
    top_width: f32,
    bottom_width: f32,
    gamma: f32,         // Blend curve (typically 2.2)
}

@group(1) @binding(0)
var<uniform> edge_blend: EdgeBlendUniforms;

@fragment
fn fs_edge_blend(in: VertexOutput) -> @location(0) vec4<f32> {
    let color = textureSample(t_input, s_input, in.texcoord);

    // Calculate blend factors for each edge
    let left_blend = smoothstep(0.0, edge_blend.left_width, in.texcoord.x);
    let right_blend = smoothstep(1.0, 1.0 - edge_blend.right_width, in.texcoord.x);
    let top_blend = smoothstep(0.0, edge_blend.top_width, in.texcoord.y);
    let bottom_blend = smoothstep(1.0, 1.0 - edge_blend.bottom_width, in.texcoord.y);

    // Combine all blend factors (multiply for corner cases)
    let blend_factor = left_blend * right_blend * top_blend * bottom_blend;

    // Apply gamma correction to blend curve
    let gamma_corrected = pow(blend_factor, edge_blend.gamma);

    return vec4<f32>(color.rgb * gamma_corrected, color.a);
}
```

**Edge Blend Configuration:**
```rust
pub struct EdgeBlendConfig {
    pub left: EdgeBlendZone,
    pub right: EdgeBlendZone,
    pub top: EdgeBlendZone,
    pub bottom: EdgeBlendZone,
    pub gamma: f32,
}

pub struct EdgeBlendZone {
    pub enabled: bool,
    pub width: f32,      // 0.0 - 1.0 (percentage of output width/height)
    pub offset: f32,     // Shift blend zone inward/outward
}

impl Default for EdgeBlendConfig {
    fn default() -> Self {
        Self {
            left: EdgeBlendZone { enabled: false, width: 0.1, offset: 0.0 },
            right: EdgeBlendZone { enabled: false, width: 0.1, offset: 0.0 },
            top: EdgeBlendZone { enabled: false, width: 0.1, offset: 0.0 },
            bottom: EdgeBlendZone { enabled: false, width: 0.1, offset: 0.0 },
            gamma: 2.2,
        }
    }
}
```

**Visual Blend Editor:**
```rust
pub struct EdgeBlendEditor {
    config: EdgeBlendConfig,
    preview_texture: TextureHandle,
}

impl EdgeBlendEditor {
    pub fn draw_ui(&mut self, ui: &imgui::Ui) {
        ui.window("Edge Blending")
            .size([400.0, 500.0], imgui::Condition::FirstUseEver)
            .build(|| {
                ui.text("Blend Zones");

                ui.checkbox("Left Edge", &mut self.config.left.enabled);
                if self.config.left.enabled {
                    ui.slider("Width##left", 0.0, 0.5, &mut self.config.left.width);
                    ui.slider("Offset##left", -0.1, 0.1, &mut self.config.left.offset);
                }

                ui.checkbox("Right Edge", &mut self.config.right.enabled);
                if self.config.right.enabled {
                    ui.slider("Width##right", 0.0, 0.5, &mut self.config.right.width);
                    ui.slider("Offset##right", -0.1, 0.1, &mut self.config.right.offset);
                }

                ui.separator();
                ui.slider("Blend Gamma", 1.0, 3.0, &mut self.config.gamma);

                // Preview window
                let image = ui.get_window_draw_list();
                image.add_image(self.preview_texture, [0.0, 0.0], [400.0, 300.0]);
            });
    }
}
```

**Milestones:**
- ✅ Week 1: Edge blend shader implementation
- ✅ Week 2: Visual editor for blend zones

---

### Week 3: Soft-Edge Warping

**Soft-Edge Mesh Generator:**
```rust
/// Generates mesh with extra vertices along edges for soft-edge blending
pub fn create_soft_edge_mesh(
    base_quad: &Quad,
    blend_config: &EdgeBlendConfig,
    subdivisions: u32,
) -> WarpMesh {
    let mut vertices = Vec::new();

    // Generate dense grid in blend zones, sparse in center
    for row in 0..=subdivisions {
        for col in 0..=subdivisions {
            let u = col as f32 / subdivisions as f32;
            let v = row as f32 / subdivisions as f32;

            // Check if this vertex is in a blend zone
            let in_blend_zone =
                (u < blend_config.left.width) ||
                (u > 1.0 - blend_config.right.width) ||
                (v < blend_config.top.width) ||
                (v > 1.0 - blend_config.bottom.width);

            if in_blend_zone || (row % 4 == 0 && col % 4 == 0) {
                // Add vertex (dense in blend zones, sparse elsewhere)
                vertices.push(MeshVertex {
                    position: base_quad.interpolate(u, v),
                    texcoord: Vec2::new(u, v),
                });
            }
        }
    }

    WarpMesh::from_vertices(vertices)
}
```

**Feathering Shader:**
```wgsl
/// Applies soft feathering to blend zone
@fragment
fn fs_soft_edge(in: VertexOutput) -> @location(0) vec4<f32> {
    let color = textureSample(t_input, s_input, in.texcoord);

    // Distance from edge (0 = edge, 1 = center)
    let dist_from_left = in.texcoord.x;
    let dist_from_right = 1.0 - in.texcoord.x;
    let dist_from_top = in.texcoord.y;
    let dist_from_bottom = 1.0 - in.texcoord.y;

    let min_dist = min(min(dist_from_left, dist_from_right), min(dist_from_top, dist_from_bottom));

    // Soft feather (cosine falloff for smooth transition)
    let feather_width = 0.1;  // 10% of width
    let alpha = smoothstep(0.0, feather_width, min_dist);

    return vec4<f32>(color.rgb, color.a * alpha);
}
```

**Milestones:**
- ✅ Week 3: Soft-edge mesh generator
- ✅ Week 3: Feathering shader with adjustable falloff

---

### Week 4: Projector Array Configuration

**Array Layout:**
```rust
/// Defines a grid of projectors (e.g., 2x2 array)
pub struct ProjectorArray {
    pub rows: u32,
    pub cols: u32,
    pub projectors: Vec<ProjectorConfig>,
    pub total_canvas_size: (u32, u32),
}

pub struct ProjectorConfig {
    pub id: OutputId,
    pub row: u32,
    pub col: u32,
    pub resolution: (u32, u32),
    pub overlap: EdgeOverlap,  // Overlap with adjacent projectors
    pub edge_blend: EdgeBlendConfig,
}

pub struct EdgeOverlap {
    pub left: f32,    // Percentage of width that overlaps left neighbor
    pub right: f32,
    pub top: f32,
    pub bottom: f32,
}

impl ProjectorArray {
    /// Creates a 2x2 array with 10% overlap
    pub fn grid_2x2(projector_resolution: (u32, u32)) -> Self {
        let overlap = 0.1;

        let mut projectors = Vec::new();
        for row in 0..2 {
            for col in 0..2 {
                let has_left = col > 0;
                let has_right = col < 1;
                let has_top = row > 0;
                let has_bottom = row < 1;

                projectors.push(ProjectorConfig {
                    id: OutputId::new(),
                    row,
                    col,
                    resolution: projector_resolution,
                    overlap: EdgeOverlap {
                        left: if has_left { overlap } else { 0.0 },
                        right: if has_right { overlap } else { 0.0 },
                        top: if has_top { overlap } else { 0.0 },
                        bottom: if has_bottom { overlap } else { 0.0 },
                    },
                    edge_blend: EdgeBlendConfig {
                        left: EdgeBlendZone { enabled: has_left, width: overlap, offset: 0.0 },
                        right: EdgeBlendZone { enabled: has_right, width: overlap, offset: 0.0 },
                        top: EdgeBlendZone { enabled: has_top, width: overlap, offset: 0.0 },
                        bottom: EdgeBlendZone { enabled: has_bottom, width: overlap, offset: 0.0 },
                        gamma: 2.2,
                    },
                });
            }
        }

        // Calculate total canvas size (accounting for overlap)
        let effective_width = projector_resolution.0 as f32 * (1.0 - overlap);
        let effective_height = projector_resolution.1 as f32 * (1.0 - overlap);
        let total_width = (effective_width * 2.0) as u32;
        let total_height = (effective_height * 2.0) as u32;

        Self {
            rows: 2,
            cols: 2,
            projectors,
            total_canvas_size: (total_width, total_height),
        }
    }
}
```

**Auto-Alignment (Photo-Based):**
```rust
/// Detects projector overlap from camera photo (future enhancement)
pub fn detect_overlap_from_photo(photo_path: &Path) -> Result<ProjectorArray, AlignmentError> {
    // 1. Capture photo of all projectors displaying calibration pattern
    // 2. Detect calibration markers (checkerboard or structured light)
    // 3. Compute homography between projectors
    // 4. Generate overlap and edge blend config

    // Placeholder for Phase 3 (requires computer vision)
    unimplemented!("Photo-based alignment in Phase 3")
}
```

**Milestones:**
- ✅ Week 4: ProjectorArray configuration for 2x2, 3x2 grids
- ✅ Week 4: Auto-configure edge blending based on overlap

---

## Month 9: Color Calibration & Multi-GPU

### Week 1-2: Per-Output Color Calibration

**Color Correction Shader:**
```wgsl
struct ColorCalibration {
    brightness: f32,      // -1.0 to 1.0
    contrast: f32,        // 0.0 to 2.0
    gamma: vec3<f32>,     // Per-channel gamma (R, G, B)
    color_temp: f32,      // 2000K to 10000K
    saturation: f32,      // 0.0 to 2.0
}

@group(2) @binding(0)
var<uniform> calibration: ColorCalibration;

@fragment
fn fs_color_calibrate(in: VertexOutput) -> @location(0) vec4<f32> {
    let color = textureSample(t_input, s_input, in.texcoord);

    // Apply brightness
    var adjusted = color.rgb + vec3<f32>(calibration.brightness);

    // Apply contrast (pivot around 0.5)
    adjusted = (adjusted - 0.5) * calibration.contrast + 0.5;

    // Apply per-channel gamma
    adjusted = pow(adjusted, 1.0 / calibration.gamma);

    // Apply color temperature (blue-orange shift)
    let temp_shift = color_temp_to_rgb(calibration.color_temp);
    adjusted *= temp_shift;

    // Apply saturation
    let luminance = dot(adjusted, vec3<f32>(0.299, 0.587, 0.114));
    adjusted = mix(vec3<f32>(luminance), adjusted, calibration.saturation);

    return vec4<f32>(clamp(adjusted, vec3<f32>(0.0), vec3<f32>(1.0)), color.a);
}

fn color_temp_to_rgb(kelvin: f32) -> vec3<f32> {
    let temp = kelvin / 100.0;

    var r, g, b: f32;

    // Red
    if temp <= 66.0 {
        r = 1.0;
    } else {
        r = (329.698727446 * pow(temp - 60.0, -0.1332047592)) / 255.0;
    }

    // Green
    if temp <= 66.0 {
        g = (99.4708025861 * log(temp) - 161.1195681661) / 255.0;
    } else {
        g = (288.1221695283 * pow(temp - 60.0, -0.0755148492)) / 255.0;
    }

    // Blue
    if temp >= 66.0 {
        b = 1.0;
    } else if temp <= 19.0 {
        b = 0.0;
    } else {
        b = (138.5177312231 * log(temp - 10.0) - 305.0447927307) / 255.0;
    }

    return vec3<f32>(clamp(r, 0.0, 1.0), clamp(g, 0.0, 1.0), clamp(b, 0.0, 1.0));
}
```

**Calibration UI:**
```rust
pub struct ColorCalibrationEditor {
    output_id: OutputId,
    calibration: ColorCalibration,
    test_pattern: TestPattern,
}

pub enum TestPattern {
    Grayscale,      // 0-100% gray ramp
    ColorBars,      // SMPTE color bars
    Checkerboard,   // Black/white checkerboard
    Gradient,       // Smooth RGB gradient
}

impl ColorCalibrationEditor {
    pub fn draw_ui(&mut self, ui: &imgui::Ui) {
        ui.window("Color Calibration")
            .size([400.0, 600.0], imgui::Condition::FirstUseEver)
            .build(|| {
                ui.combo("Test Pattern", &mut self.test_pattern, &[
                    TestPattern::Grayscale,
                    TestPattern::ColorBars,
                    TestPattern::Checkerboard,
                    TestPattern::Gradient,
                ]);

                ui.separator();

                ui.slider("Brightness", -1.0, 1.0, &mut self.calibration.brightness);
                ui.slider("Contrast", 0.0, 2.0, &mut self.calibration.contrast);

                ui.separator();
                ui.text("Gamma (Per Channel)");
                ui.slider("Red Gamma", 0.5, 3.0, &mut self.calibration.gamma.x);
                ui.slider("Green Gamma", 0.5, 3.0, &mut self.calibration.gamma.y);
                ui.slider("Blue Gamma", 0.5, 3.0, &mut self.calibration.gamma.z);

                ui.separator();
                ui.slider("Color Temperature", 2000.0, 10000.0, &mut self.calibration.color_temp);
                ui.slider("Saturation", 0.0, 2.0, &mut self.calibration.saturation);

                if ui.button("Reset to Defaults") {
                    self.calibration = ColorCalibration::default();
                }

                if ui.button("Load from File") {
                    // Load .cube LUT or ICC profile
                }
            });
    }
}
```

**Milestones:**
- ✅ Week 1: Color correction shader (brightness, contrast, gamma, temp, saturation)
- ✅ Week 2: Calibration UI with test patterns

---

### Week 3: 3D LUT Support

**3D LUT Shader:**
```wgsl
@group(3) @binding(0)
var lut_texture: texture_3d<f32>;
@group(3) @binding(1)
var lut_sampler: sampler;

@fragment
fn fs_lut_apply(in: VertexOutput) -> @location(0) vec4<f32> {
    let color = textureSample(t_input, s_input, in.texcoord);

    // Sample 3D LUT (33x33x33 typical size)
    let lut_color = textureSample(lut_texture, lut_sampler, color.rgb);

    return vec4<f32>(lut_color.rgb, color.a);
}
```

**LUT Loader:**
```rust
/// Loads .cube LUT file (common format for color grading)
pub fn load_cube_lut(path: &Path, device: &wgpu::Device) -> Result<wgpu::Texture, LutError> {
    let cube_data = std::fs::read_to_string(path)?;

    let mut size = 0;
    let mut lut_data = Vec::new();

    for line in cube_data.lines() {
        if line.starts_with("LUT_3D_SIZE") {
            size = line.split_whitespace().nth(1).unwrap().parse()?;
        } else if !line.starts_with('#') && !line.is_empty() {
            let rgb: Vec<f32> = line.split_whitespace()
                .map(|s| s.parse().unwrap())
                .collect();
            lut_data.extend_from_slice(&rgb);
        }
    }

    // Create 3D texture
    let texture = device.create_texture(&wgpu::TextureDescriptor {
        size: wgpu::Extent3d {
            width: size,
            height: size,
            depth_or_array_layers: size,
        },
        format: wgpu::TextureFormat::Rgba32Float,
        usage: wgpu::TextureUsages::TEXTURE_BINDING | wgpu::TextureUsages::COPY_DST,
        dimension: wgpu::TextureDimension::D3,
        ..Default::default()
    });

    // Upload LUT data
    queue.write_texture(
        texture.as_image_copy(),
        bytemuck::cast_slice(&lut_data),
        wgpu::ImageDataLayout {
            offset: 0,
            bytes_per_row: Some(size * 4 * 4),  // 4 bytes per component, 4 components (RGBA)
            rows_per_image: Some(size),
        },
        texture.size(),
    );

    Ok(texture)
}
```

**Milestones:**
- ✅ Week 3: 3D LUT shader implementation
- ✅ Week 3: .cube LUT file loader

---

### Week 4: Multi-GPU Support

**GPU Selection:**
```rust
/// Enumerates available GPUs
pub fn enumerate_gpus() -> Vec<GpuInfo> {
    let instance = wgpu::Instance::new(wgpu::InstanceDescriptor::default());

    instance.enumerate_adapters(wgpu::Backends::all())
        .map(|adapter| GpuInfo {
            name: adapter.get_info().name.clone(),
            backend: adapter.get_info().backend,
            device_type: adapter.get_info().device_type,
        })
        .collect()
}

#[derive(Debug, Clone)]
pub struct GpuInfo {
    pub name: String,
    pub backend: wgpu::Backend,
    pub device_type: wgpu::DeviceType,
}

/// Assigns outputs to specific GPUs
pub struct MultiGpuManager {
    gpu_assignments: HashMap<OutputId, GpuId>,
    devices: HashMap<GpuId, wgpu::Device>,
    queues: HashMap<GpuId, wgpu::Queue>,
}

impl MultiGpuManager {
    pub fn assign_output_to_gpu(&mut self, output_id: OutputId, gpu_id: GpuId) {
        self.gpu_assignments.insert(output_id, gpu_id);
    }

    pub fn render_output(&mut self, output_id: OutputId, project: &Project) {
        let gpu_id = self.gpu_assignments.get(&output_id).unwrap();
        let device = self.devices.get(gpu_id).unwrap();
        let queue = self.queues.get(gpu_id).unwrap();

        // Render using assigned GPU
        // ...
    }
}
```

**Cross-GPU Texture Transfer:**
```rust
/// Transfers texture from GPU A to GPU B (for shared content)
pub fn transfer_texture_between_gpus(
    src_gpu: &wgpu::Device,
    dst_gpu: &wgpu::Device,
    texture: &wgpu::Texture,
) -> wgpu::Texture {
    // Download texture to CPU memory
    let mut encoder = src_gpu.create_command_encoder(&Default::default());
    let staging_buffer = src_gpu.create_buffer(&wgpu::BufferDescriptor {
        size: texture_size_bytes(texture),
        usage: wgpu::BufferUsages::COPY_DST | wgpu::BufferUsages::MAP_READ,
        mapped_at_creation: false,
    });

    encoder.copy_texture_to_buffer(
        texture.as_image_copy(),
        wgpu::ImageCopyBuffer {
            buffer: &staging_buffer,
            layout: Default::default(),
        },
        texture.size(),
    );

    src_gpu.queue.submit(Some(encoder.finish()));

    // Map staging buffer
    let buffer_slice = staging_buffer.slice(..);
    buffer_slice.map_async(wgpu::MapMode::Read);
    src_gpu.poll(wgpu::Maintain::Wait);

    let data = buffer_slice.get_mapped_range().to_vec();
    staging_buffer.unmap();

    // Upload to destination GPU
    let dst_texture = dst_gpu.create_texture(&texture.descriptor());
    dst_gpu.queue.write_texture(
        dst_texture.as_image_copy(),
        &data,
        wgpu::ImageDataLayout::default(),
        texture.size(),
    );

    dst_texture
}
```

**Milestones:**
- ✅ Week 4: Multi-GPU device enumeration and assignment
- ✅ Week 4: Cross-GPU texture transfer (benchmark overhead)

---

## Phase 2 Success Metrics

**Functional:**
- ✅ 4+ independent outputs rendering simultaneously
- ✅ Fullscreen exclusive mode on all platforms
- ✅ Edge blending with gamma-corrected feathering
- ✅ Per-output color calibration (brightness, contrast, gamma, temp, saturation)
- ✅ 3D LUT support (.cube files)
- ✅ Multi-GPU output assignment

**Performance:**
- ✅ 4 outputs at 1080p60: <16.6ms frame time (60fps locked)
- ✅ 2 outputs at 4K60: <16.6ms frame time
- ✅ Edge blend overhead: <2ms per output
- ✅ Cross-GPU texture transfer: <5ms per texture

**Code Quality:**
- ✅ All clippy warnings resolved
- ✅ Integration tests for multi-output scenarios
- ✅ Benchmark suite for edge blending and color calibration

---

## Phase 2 Risk Mitigation

**High-Risk: Multi-GPU Synchronization:**
- Mitigation: Software frame synchronization as fallback
- Document NVIDIA Mosaic/AMD Eyefinity setup for users
- Test on multi-GPU systems (require access to hardware)

**Medium-Risk: Platform-Specific Fullscreen:**
- Mitigation: Per-platform testing in CI
- Fallback to borderless fullscreen if exclusive fails

**Low-Risk: Edge Blend Tuning:**
- Mitigation: Visual editor with real-time preview
- Provide presets for common projector overlaps (5%, 10%, 15%)

---

## Next Steps: Phase 3 Preview

Phase 3 adds GPU compute effects pipeline:
- Compute shader effects (blur, glow, color grading)
- Real-time effect chains (configurable per layer)
- Audio-reactive effects (FFT analysis, beat detection)
- Custom effect API (WGSL shader hot-loading)
- Effect presets library

*Phase 3 detailed plan follows in next section.*

---

# Phase 3: Effects Pipeline & GPU Compute (Months 10-12)

## Executive Summary

Phase 3 adds a powerful GPU compute effects pipeline, enabling real-time visual effects and audio reactivity. This phase brings MapMap to feature parity with Resolume's effects capabilities.

## Phase 3 Goals

**Primary Objectives:**
- GPU compute shader effects (blur, glow, edge detect, color grading)
- Configurable effect chains per layer
- Audio-reactive effects (FFT analysis, beat detection, envelope followers)
- Custom WGSL shader API with hot-reloading
- Effect presets library (50+ built-in effects)
- Performance monitoring and effect profiling

**Performance Targets:**
- 8+ effects per layer at 1080p60
- <3ms per effect overhead
- Real-time FFT analysis (60fps)
- Zero-copy audio processing

---

## Month 10: Compute Shader Infrastructure

### Core Features:
1. **Compute Pipeline Architecture** (Week 1-2)
   - Effect base trait: `trait Effect { fn apply(&self, input: Texture) -> Texture }`
   - Compute shader compiler and cache
   - Buffer management for compute operations
   - Dispatch optimization (workgroup sizing)

2. **Built-in Effects** (Week 3-4)
   - Blur (Gaussian, box, radial)
   - Edge detection (Sobel, Canny)
   - Color grading (HSV adjustment, curves, levels)
   - Distortion (wave, ripple, pixelate)
   - Glow/bloom
   - Chromatic aberration

### Implementation Highlights:
```rust
pub trait Effect: Send + Sync {
    fn apply(&mut self, input: &Texture, output: &mut Texture, encoder: &mut CommandEncoder);
    fn parameters(&self) -> &[EffectParameter];
    fn set_parameter(&mut self, name: &str, value: ParameterValue);
}

pub struct EffectChain {
    effects: Vec<Box<dyn Effect>>,
    intermediate_textures: Vec<Texture>,
}

impl EffectChain {
    pub fn apply(&mut self, input: &Texture) -> &Texture {
        let mut current = input;
        for (effect, intermediate) in self.effects.iter_mut().zip(&mut self.intermediate_textures) {
            effect.apply(current, intermediate, &mut encoder);
            current = intermediate;
        }
        current
    }
}
```

---

## Month 11: Audio Reactivity

### Core Features:
1. **Audio Analysis** (Week 1-2)
   - Real-time FFT (512-4096 bins)
   - Beat detection (onset detection, tempo tracking)
   - Frequency band analysis (bass, mid, treble)
   - RMS/peak envelope followers
   - Audio input routing (system audio, line-in, WASAPI loopback)

2. **Audio-Reactive Modulation** (Week 3-4)
   - Modulation targets (effect parameters, layer opacity, warp mesh vertices)
   - Modulation curves (linear, exponential, logarithmic)
   - Smoothing and decay controls
   - MIDI CC to parameter mapping

### Implementation Highlights:
```rust
pub struct AudioAnalyzer {
    fft: RealFft<f32>,
    sample_rate: u32,
    buffer: Vec<f32>,
    spectrum: Vec<f32>,
}

impl AudioAnalyzer {
    pub fn analyze(&mut self, audio_samples: &[f32]) -> AudioFeatures {
        self.fft.process(&mut self.buffer, &mut self.spectrum);
        
        AudioFeatures {
            bass: self.spectrum[0..10].iter().sum::<f32>() / 10.0,
            mid: self.spectrum[10..100].iter().sum::<f32>() / 90.0,
            treble: self.spectrum[100..512].iter().sum::<f32>() / 412.0,
            beat: self.detect_beat(),
            rms: self.compute_rms(audio_samples),
        }
    }
}

pub struct AudioModulator {
    target: ModulationTarget,
    source: AudioFeature,
    range: (f32, f32),
    curve: ModulationCurve,
}
```

---

## Month 12: Custom Shaders & Effect Library

### Core Features:
1. **Custom Shader API** (Week 1-2)
   - WGSL shader editor with syntax highlighting
   - Hot-reloading (file watcher)
   - Shader validation and error reporting
   - Parameter auto-detection from shader source
   - Preset management (save/load .effect files)

2. **Effect Library** (Week 3-4)
   - 50+ built-in effects organized by category
   - User presets database
   - Effect thumbnails/previews
   - Performance ratings per effect
   - Community effect sharing (future)

### Success Metrics:
- ✅ 50+ built-in effects
- ✅ 8 effects per layer at 1080p60
- ✅ <3ms effect overhead
- ✅ Custom WGSL shaders hot-reload in <100ms

---

# Phase 4: Control Systems (Months 13-15)

## Executive Summary

Phase 4 adds professional control surface integration, enabling MapMap to be controlled via MIDI, OSC, DMX, and web interfaces. This phase is critical for live performance workflows.

## Phase 4 Goals

**Primary Objectives:**
- MIDI input/output (note, CC, program change, clock)
- OSC server/client (TouchOSC, Lemur, custom apps)
- DMX output via Art-Net/sACN (lighting control)
- Web-based remote control interface
- Cue/timeline system for automated shows
- Keyboard shortcuts and macro system

**Performance Targets:**
- <1ms MIDI latency
- <5ms OSC latency
- 30Hz DMX refresh rate
- 60fps web UI updates via WebSocket

---

## Month 13: MIDI & OSC Integration

### Core Features:
1. **MIDI System** (Week 1-2)
   - Input: Note on/off, CC, program change, clock/sync
   - Output: Send MIDI feedback to controllers
   - MIDI learn mode (click parameter, twist knob)
   - Per-controller profiles (APC40, Launchpad, etc.)
   - MIDI clock sync for tempo-synced effects

2. **OSC Server/Client** (Week 3-4)
   - OSC address space: `/layer/{id}/{parameter}`
   - Type-safe message parsing
   - Bi-directional communication (send state updates)
   - Custom OSC templates for TouchOSC/Lemur
   - OSC query protocol support

### Implementation Highlights:
```rust
pub struct MidiController {
    input: midir::MidiInput,
    mapping: HashMap<MidiMessage, ControlTarget>,
    learn_mode: bool,
}

impl MidiController {
    pub fn handle_message(&mut self, msg: MidiMessage, project: &mut Project) {
        match msg {
            MidiMessage::ControlChange { channel, controller, value } => {
                if let Some(target) = self.mapping.get(&msg) {
                    target.set_value(value as f32 / 127.0);
                }
            }
            _ => {}
        }
    }
}

pub struct OscServer {
    socket: UdpSocket,
    address_space: HashMap<String, ControlTarget>,
}

impl OscServer {
    pub fn dispatch(&self, addr: &str, args: Vec<OscType>) -> Result<(), OscError> {
        let target = self.address_space.get(addr)?;
        target.set_value(args[0].float()?);
        Ok(())
    }
}
```

---

## Month 14: DMX & Web Interface

### Core Features:
1. **DMX Output** (Week 1-2)
   - Art-Net protocol (universe support)
   - sACN (E1.31) multicast
   - DMX channel assignment (project brightness → channel 1, etc.)
   - Fixture profiles (generic dimmer, RGB par, moving head)
   - DMX visualizer/monitor

2. **Web Control Interface** (Week 3-4)
   - HTTP REST API (Axum framework)
   - WebSocket for real-time updates
   - Web UI: layer controls, effect params, project browser
   - Mobile-responsive design
   - Authentication and access control

### Implementation Highlights:
```rust
pub struct ArtNetSender {
    socket: UdpSocket,
    universe: u16,
}

impl ArtNetSender {
    pub fn send_dmx(&self, channels: &[u8; 512]) {
        let packet = ArtDmxPacket::new(self.universe, channels);
        self.socket.send_to(&packet.serialize(), "255.255.255.255:6454");
    }
}

// Web API
#[tokio::main]
async fn main() {
    let app = Router::new()
        .route("/api/layers", get(list_layers))
        .route("/api/layers/:id", patch(update_layer))
        .route("/ws", get(websocket_handler));

    axum::Server::bind(&"0.0.0.0:8080".parse().unwrap())
        .serve(app.into_make_service())
        .await;
}
```

---

## Month 15: Cue System & Automation

### Core Features:
1. **Cue/Timeline System** (Week 1-3)
   - Cue list: snapshot entire project state
   - Crossfade between cues (adjustable duration)
   - Timeline: keyframe animation for parameters
   - Looping and conditional cues (trigger on beat, time, etc.)
   - MIDI/OSC cue triggers

2. **Keyboard Shortcuts & Macros** (Week 4)
   - Global shortcut system
   - User-definable key bindings
   - Macro recorder (record sequence of actions)
   - Scriptable actions (Lua or Python bindings)

### Success Metrics:
- ✅ MIDI learn functional for all parameters
- ✅ OSC control with <5ms latency
- ✅ DMX output at 30Hz (Art-Net)
- ✅ Web UI responsive on mobile devices
- ✅ Cue system with crossfades

---

# Phase 5: Professional Video I/O (Months 16-18)

## Executive Summary

Phase 5 integrates professional video I/O protocols (NDI, DeckLink SDI, Spout, Syphon), enabling MapMap to interface with broadcast equipment, live cameras, and other professional software.

## Phase 5 Goals

**Primary Objectives:**
- NDI receive/send (HD/4K streams)
- DeckLink SDI input/output (capture cards)
- Spout (Windows texture sharing)
- Syphon (macOS texture sharing)
- Virtual camera output (OBS integration)
- Stream output (RTMP/SRT)

**Performance Targets:**
- NDI: <2 frames latency
- DeckLink: <1 frame latency (genlock sync)
- Spout/Syphon: <1ms texture share
- Stream output: 1080p60 at 6Mbps

---

## Month 16: NDI Integration

### Core Features (Week 1-4):
- NDI SDK FFI bindings (`libndi.so`)
- NDI source discovery and enumeration
- NDI receiver (decode to GPU texture)
- NDI sender (encode from GPU texture)
- NDI tally support (program/preview indicators)
- NDI metadata (PTZ control, etc.)

### Implementation Highlights:
```rust
// FFI bindings via bindgen
mod ndi_sys {
    include!(concat!(env!("OUT_DIR"), "/ndi_bindings.rs"));
}

pub struct NdiReceiver {
    recv: *mut ndi_sys::NDIlib_recv_instance_t,
    converter: TextureConverter,
}

impl NdiReceiver {
    pub fn receive_frame(&mut self) -> Option<Texture> {
        let mut video_frame = ndi_sys::NDIlib_video_frame_v2_t::default();
        
        unsafe {
            if ndi_sys::NDIlib_recv_capture_v2(self.recv, &mut video_frame, std::ptr::null_mut(), std::ptr::null_mut(), 1000) == ndi_sys::NDIlib_frame_type_e::NDIlib_frame_type_video {
                let texture = self.converter.yuv_to_rgba(&video_frame);
                ndi_sys::NDIlib_recv_free_video_v2(self.recv, &video_frame);
                return Some(texture);
            }
        }
        None
    }
}
```

---

## Month 17: DeckLink SDI & Texture Sharing

### Core Features:
1. **DeckLink Integration** (Week 1-2)
   - DeckLink SDK FFI (COM on Windows, Obj-C on macOS)
   - Input: Capture SDI/HDMI streams
   - Output: Send frames to SDI/HDMI output
   - Genlock synchronization
   - Timecode support (LTC, VITC)

2. **Spout (Windows)** (Week 3)
   - Spout sender (share texture to other apps)
   - Spout receiver (receive texture from other apps)
   - DX11 shared handle interop with wgpu

3. **Syphon (macOS)** (Week 4)
   - Syphon server (publish texture)
   - Syphon client (subscribe to texture)
   - IOSurface interop with wgpu Metal backend

---

## Month 18: Streaming & Virtual Camera

### Core Features:
1. **Stream Output** (Week 1-2)
   - RTMP push (via FFmpeg libavformat)
   - SRT output (low-latency streaming)
   - H.264/H.265 encoding (hardware encoder)
   - Bitrate control and adaptive streaming

2. **Virtual Camera** (Week 3-4)
   - Windows: DirectShow filter
   - macOS: DAL plugin (CoreMediaIO)
   - Linux: V4L2 loopback
   - OBS integration (appear as camera source)

### Success Metrics:
- ✅ NDI receive/send with <2 frames latency
- ✅ DeckLink SDI I/O functional
- ✅ Spout/Syphon texture sharing <1ms
- ✅ RTMP streaming at 1080p60

---

# Phase 6: Advanced Authoring UI (Months 19-21)

## Executive Summary

Phase 6 replaces the minimal ImGui interface with a polished authoring UI, making MapMap accessible to non-technical users while retaining power-user features.

## Phase 6 Goals

**Primary Objectives:**
- Node-based effect editor (visual programming)
- Timeline editor with keyframe animation
- Advanced warp mesh editor (Bezier curves, subdivision)
- Asset browser (media, effects, presets)
- Dark theme with accessibility options
- Undo/redo system
- Multi-monitor UI layout

**UI Technology Decision:**
- Option A: **egui** (pure Rust, immediate-mode, good wgpu integration)
- Option B: **Qt 6 QML** (via cxx.rs bindings, familiar to existing users)
- **Recommendation:** Start with egui, evaluate Qt in Month 21 if needed

---

## Month 19: UI Framework Migration

### Core Features (Week 1-4):
- Migrate from ImGui to egui
- Layout system (docking panels, tabs)
- Theme system (dark, light, high-contrast)
- Accessibility (screen reader support, keyboard navigation)
- Undo/redo architecture (command pattern)

### Implementation:
```rust
pub struct EditorState {
    project: Project,
    undo_stack: Vec<Command>,
    redo_stack: Vec<Command>,
}

pub trait Command {
    fn execute(&self, project: &mut Project);
    fn undo(&self, project: &mut Project);
}

// egui panels
egui::SidePanel::left("asset_browser").show(ctx, |ui| {
    // Media library, effect browser
});

egui::CentralPanel::default().show(ctx, |ui| {
    // Viewport (rendered output with overlay)
});

egui::TopBottomPanel::bottom("timeline").show(ctx, |ui| {
    // Timeline with keyframes
});
```

---

## Month 20: Node Editor & Timeline

### Core Features:
1. **Node-Based Effect Editor** (Week 1-2)
   - Visual node graph (inputs, outputs, connections)
   - Effect nodes (blur, glow, etc.)
   - Math nodes (add, multiply, sine wave)
   - Utility nodes (switch, lerp, clamp)
   - Custom node API

2. **Timeline Editor** (Week 3-4)
   - Multi-track timeline (layers, effects, cues)
   - Keyframe animation (position, opacity, parameters)
   - Bezier interpolation curves
   - Markers and regions
   - Scrubbing and playback controls

---

## Month 21: Advanced Editing Tools

### Core Features:
1. **Advanced Warp Mesh Editor** (Week 1-2)
   - Bezier control points
   - Subdivision surface (smooth mesh)
   - Symmetry mode (mirror edits)
   - Snap to grid/guides
   - Copy/paste mesh sections

2. **Asset Management** (Week 3-4)
   - Media library (thumbnails, metadata, search)
   - Effect preset browser (categories, tags, favorites)
   - Project templates
   - Import/export workflows
   - Cloud sync (optional, via S3/Dropbox API)

### Success Metrics:
- ✅ egui UI migration complete
- ✅ Node editor functional
- ✅ Timeline keyframe animation working
- ✅ Undo/redo for all operations
- ✅ Asset browser with search

---

# Phase 7: Performance Optimization & Polish (Months 22-24)

## Executive Summary

Phase 7 focuses on performance optimization, stability improvements, and preparing MapMap for production release. This phase includes extensive testing, documentation, and deployment infrastructure.

## Phase 7 Goals

**Primary Objectives:**
- Performance profiling and optimization
- Memory leak detection and fixes
- Crash reporting and error recovery
- Comprehensive testing (unit, integration, stress)
- End-user documentation and tutorials
- Installer and update system
- Beta testing program

**Performance Targets:**
- 10+ layers at 1080p60 with effects
- <100MB idle RAM usage
- <1s project load time
- Zero crashes in 24-hour stress test

---

## Month 22: Performance Optimization

### Core Features (Week 1-4):
1. **Profiling & Benchmarking**
   - Tracy profiler integration
   - GPU timeline analysis
   - Memory profiling (heaptrack, valgrind)
   - Flamegraph generation

2. **Optimizations**
   - Texture compression (BC1/BC3/BC7)
   - Instanced rendering for repeated shapes
   - Frustum culling for layers
   - Async project loading
   - SIMD optimizations (std::simd)

3. **Memory Management**
   - Texture streaming (load on demand)
   - LRU cache for decoded frames
   - Memory pool for allocations
   - Leak detection with AddressSanitizer

---

## Month 23: Stability & Testing

### Core Features (Week 1-4):
1. **Error Handling**
   - Crash reporting (Sentry integration)
   - Graceful degradation (fallback to safe mode)
   - Device lost recovery (GPU hangs)
   - Network error handling (NDI, OSC)

2. **Testing Infrastructure**
   - Unit tests: >85% coverage
   - Integration tests: full render pipeline
   - Stress tests: 24-hour run, memory leak detection
   - Platform-specific tests (Windows/macOS/Linux)
   - Automated UI testing (egui_test framework)

3. **CI/CD Improvements**
   - Nightly builds (bleeding-edge)
   - Release candidate pipeline
   - Cross-compilation for all platforms
   - Automated smoke tests on release

---

## Month 24: Documentation & Release

### Core Features (Week 1-4):
1. **Documentation**
   - User manual (getting started, tutorials, reference)
   - Developer docs (architecture, plugin API)
   - Video tutorials (YouTube series)
   - Example projects (showcase gallery)

2. **Installer & Updates**
   - Cross-platform installer (Windows: NSIS, macOS: DMG, Linux: AppImage/Flatpak)
   - Auto-update system (check GitHub releases)
   - License key management (if commercial)

3. **Release Preparation**
   - Beta testing program (100+ users)
   - Performance baseline documentation
   - Known issues and workarounds
   - Migration guide from legacy MapMap
   - Press kit and launch materials

4. **Launch & Support**
   - Release v1.0 (stable)
   - Community forum setup
   - Bug bounty program
   - Roadmap for v1.1+ (new features)

---

## Phase 7 Success Metrics

**Performance:**
- ✅ 10+ layers at 1080p60 with effects
- ✅ <100MB idle RAM usage
- ✅ <1s project load time
- ✅ Zero crashes in 24-hour stress test

**Quality:**
- ✅ >85% test coverage
- ✅ All critical bugs fixed
- ✅ Accessibility audit passed
- ✅ Performance targets met on minimum spec hardware

**Documentation:**
- ✅ Complete user manual
- ✅ 20+ video tutorials
- ✅ Developer API documentation
- ✅ 50+ example projects

**Release:**
- ✅ v1.0 released on all platforms
- ✅ 100+ beta testers providing feedback
- ✅ Installer tested on fresh OS installs
- ✅ Community forum active

---

# Conclusion: 24-Month Roadmap Complete

This comprehensive 24-month plan transforms MapMap from a C++/Qt application into a modern, high-performance Rust-based projection mapping system capable of competing with industry leaders like Resolume Arena.

## Timeline Summary

| Phase | Months | Focus | Key Deliverables |
|-------|--------|-------|-----------------|
| 0 | 1-3 | Foundation | wgpu rendering, FFmpeg decode, basic windowing |
| 1 | 4-6 | Core Mapping | Mesh warping, layers, hardware decode, project save/load |
| 2 | 7-9 | Multi-Output | Multiple outputs, edge blending, color calibration, multi-GPU |
| 3 | 10-12 | Effects | GPU compute effects, audio reactivity, custom shaders |
| 4 | 13-15 | Control | MIDI/OSC/DMX, web interface, cue system |
| 5 | 16-18 | Pro I/O | NDI, DeckLink, Spout/Syphon, streaming |
| 6 | 19-21 | Advanced UI | Node editor, timeline, asset browser |
| 7 | 22-24 | Polish | Optimization, testing, documentation, release |

## Technology Stack Recap

- **Language:** Rust 2021 Edition
- **Graphics:** wgpu (Vulkan/Metal/DX12)
- **Media:** FFmpeg + platform hardware decoders
- **UI:** egui (Phase 6+), imgui-rs (Phase 0-5)
- **Audio:** cpal + rustfft
- **Networking:** tokio + axum (HTTP/WebSocket)
- **Control:** midir (MIDI) + rosc (OSC)

## Expected Outcomes

By the end of 24 months, MapMap Rust will:
- ✅ Match or exceed Resolume Arena feature set
- ✅ Outperform legacy MapMap by 2-5x (benchmarks)
- ✅ Run on minimum spec hardware (GTX 1060/RX 580)
- ✅ Support professional workflows (live events, installations)
- ✅ Be fully open-source and community-driven

## Next Actions

1. **Immediate:** Begin Phase 0 implementation (repository setup, CI/CD)
2. **Month 1:** Complete wgpu backend + first benchmark
3. **Month 3:** Deliver Phase 0 demo (video playback with ImGui controls)
4. **Month 6:** Deliver Phase 1 demo (4-layer projection mapping)
5. **Month 12:** Deliver Phase 3 demo (effects + audio reactivity)
6. **Month 24:** Release v1.0 to the world

---

**Total Implementation Time:** 24 months (2 years)  
**Estimated Team Size:** 2-3 full-time developers  
**Estimated LOC:** ~150,000 lines of Rust  
**Estimated Complexity:** High (but manageable with phased approach)

**Risk Assessment:** Medium  
- Rust ecosystem mature enough for all requirements
- FFI to C libraries (NDI, DeckLink) well-understood
- Performance targets achievable (validated by Phase 0 benchmarks)

**Recommendation:** PROCEED with this plan. The Rust rewrite is feasible, and the phased approach allows for course correction at each milestone.

**End of MapMap Rust Rewrite Plan v1.0**

---

*Document Version: 1.0*  
*Last Updated: 2025-11-11*  
*Status: APPROVED*  
*Next Review: End of Phase 0 (Month 3)*
