# MapMap Architecture Documentation

## Overview

MapMap is a modern projection mapping system written in Rust, designed to compete with professional tools like Resolume Arena. This document describes the architecture implemented in Phase 0 (Foundation).

## System Architecture

### High-Level Architecture

```
┌─────────────────────────────────────────────────────────────┐
│                        Application                           │
│                       (mapmap binary)                        │
└───────────────┬─────────────────────────────────────────────┘
                │
    ┌───────────┼───────────┬───────────────┬──────────────┐
    │           │           │               │              │
    ▼           ▼           ▼               ▼              ▼
┌────────┐ ┌────────┐ ┌─────────┐ ┌──────────────┐ ┌──────────┐
│ Core   │ │ Render │ │  Media  │ │      UI      │ │ Control  │
│        │ │        │ │         │ │              │ │          │
└────────┘ └────┬───┘ └────┬────┘ └──────┬───────┘ └──────────┘
               │          │              │
               ▼          ▼              ▼
          ┌────────────────────────────────┐
          │        wgpu / FFmpeg           │
          │     (External Dependencies)    │
          └────────────────────────────────┘
```

### Crate Structure

**mapmap-core:**
- Domain model (Paint, Mapping, Shape)
- Project file format
- Geometry primitives and transforms
- Pure Rust, no external dependencies

**mapmap-render:**
- Graphics abstraction layer
- wgpu backend implementation
- Texture pool management
- Shader compilation and loading
- Quad renderer for textured quads

**mapmap-media:**
- Video decoder abstraction
- FFmpeg integration (stub in Phase 0)
- Video player with playback control
- Frame management

**mapmap-ui:**
- ImGui integration
- Control panels and UI widgets
- Window management helpers
- UI state management

**mapmap-control:**
- MIDI input/output (Phase 4)
- OSC server (Phase 4)
- Art-Net/sACN DMX (Phase 4)
- HTTP REST API (Phase 4)

**mapmap-ffi:**
- C API for plugins (Phase 5)
- NDI/DeckLink/Spout/Syphon bindings (Phase 5)

## Threading Model

### Phase 0 Architecture (Single-Threaded)

```
┌──────────────────────────────────────────────────────────┐
│                      Main Thread                          │
│                                                           │
│  ┌──────────┐   ┌───────────┐   ┌──────────────┐        │
│  │  Decode  │──▶│  Upload   │──▶│    Render    │        │
│  │  Frame   │   │  Texture  │   │  + Present   │        │
│  └──────────┘   └───────────┘   └──────────────┘        │
│       │              │                   │               │
│       └──────────────┴───────────────────┘               │
│                VSync @ 60Hz                              │
└──────────────────────────────────────────────────────────┘
```

### Planned Phase 2 Architecture (Multi-Threaded)

```
┌─────────────────┐    ┌──────────────────┐    ┌────────────────┐
│ Decode Thread   │    │ Upload Thread    │    │ Render Thread  │
│                 │    │                  │    │ (Main Thread)  │
│ ┌────────────┐  │    │ ┌──────────────┐ │    │ ┌────────────┐ │
│ │  FFmpeg    │  │    │ │   Staging    │ │    │ │   wgpu     │ │
│ │  Decode    │──┼───▶│ │   Buffer     │─┼───▶│ │  Render    │ │
│ │            │  │    │ │   Upload     │ │    │ │            │ │
│ └────────────┘  │    │ └──────────────┘ │    │ └────────────┘ │
│                 │    │                  │    │       │        │
└─────────────────┘    └──────────────────┘    └───────┼────────┘
        │                      │                       │
        └──────────────────────┴───────────────────────┘
            Lock-Free Channels (crossbeam)
```

## Data Flow

### Video Playback Pipeline

```
1. Video File
   │
   ▼
2. FFmpeg Decoder
   │ (Decode to RGBA)
   ▼
3. DecodedFrame
   │ (CPU memory)
   ▼
4. Texture Upload
   │ (CPU → GPU transfer via wgpu::Queue)
   ▼
5. GPU Texture
   │
   ▼
6. Quad Renderer
   │ (Shader-based rendering)
   ▼
7. Swapchain
   │
   ▼
8. Display
```

### Frame Synchronization

In Phase 0, all operations happen synchronously on the main thread:

```rust
loop {
    // 1. Update video player
    if let Some(frame) = player.update(delta_time) {
        // 2. Upload to GPU
        let texture = backend.create_texture(desc);
        backend.upload_texture(texture, frame.data);

        // 3. Render
        let bind_group = renderer.create_bind_group(&texture);
        render_pass.draw(&bind_group);
    }

    // 4. Present (VSync)
    frame.present();
}
```

In Phase 2+, operations will be pipelined across threads with lock-free queues.

## Graphics Pipeline

### wgpu Rendering Architecture

```
┌────────────────────────────────────────────────────────────┐
│                     wgpu Device                             │
└────────────────┬───────────────────────────────────────────┘
                 │
       ┌─────────┼─────────┬──────────────────┐
       │         │         │                  │
       ▼         ▼         ▼                  ▼
  ┌────────┐ ┌──────┐ ┌────────┐      ┌─────────────┐
  │Texture │ │Buffer│ │Shader  │      │  Pipeline   │
  │ Pool   │ │ Pool │ │Compiler│      │   Cache     │
  └────────┘ └──────┘ └────────┘      └─────────────┘
       │         │         │                  │
       └─────────┴─────────┴──────────────────┘
                          │
                          ▼
                  ┌───────────────┐
                  │ Command Queue │
                  └───────┬───────┘
                          │
                          ▼
                      ┌───────┐
                      │  GPU  │
                      └───────┘
```

### Shader Pipeline

**Vertex Shader (WGSL):**
- Input: Position (vec3), UV (vec2)
- Output: Clip position, UV coordinates
- Transform: Identity (fullscreen quad)

**Fragment Shader (WGSL):**
- Input: UV coordinates
- Texture sampling from bound texture
- Output: RGBA color

**Shader Compilation:**
- Development: WGSL → naga → SPIR-V (runtime)
- Production: Pre-compiled SPIR-V for faster startup

## Memory Management

### Texture Pool

The texture pool reuses GPU allocations to reduce allocation overhead:

```rust
struct TexturePool {
    // Active textures (in use)
    textures: HashMap<TextureId, Texture>,

    // Free textures (available for reuse)
    free_list: Vec<Texture>,

    // Maximum pool size (prevents unbounded growth)
    max_pool_size: usize,
}
```

**Allocation Strategy:**
1. Request texture with specific dimensions/format
2. Check free_list for matching texture
3. If found, return existing texture
4. If not found, allocate new texture
5. When released, return to free_list (if under max_pool_size)

### Staging Buffer Pool (Phase 2+)

For efficient CPU→GPU transfers:

```rust
struct StagingPool {
    // Reusable staging buffers
    buffers: Vec<StagingBuffer>,

    // Chunk size for allocations
    chunk_size: u64, // e.g., 1MB
}
```

**Upload Flow:**
1. Get staging buffer from pool
2. Map buffer for CPU write
3. Copy decoded frame data
4. Unmap buffer
5. Record copy_buffer_to_texture command
6. Submit command buffer
7. Recycle staging buffer to pool

## Error Handling

### Error Hierarchy

```
anyhow::Error (Application-level)
    │
    ├─ RenderError (mapmap-render)
    │   ├─ DeviceError
    │   ├─ ShaderCompilation
    │   ├─ TextureCreation
    │   └─ DeviceLost
    │
    ├─ MediaError (mapmap-media)
    │   ├─ FileOpen
    │   ├─ NoVideoStream
    │   ├─ DecoderError
    │   └─ EndOfStream
    │
    └─ ControlError (mapmap-control)
        ├─ MidiError
        ├─ OscError
        └─ DmxError
```

### Recovery Strategies

**Device Lost (GPU crash/reset):**
1. Detect via `wgpu::Device::poll()`
2. Recreate device and queue
3. Reload all resources (textures, shaders, pipelines)
4. Resume rendering

**Decode Error:**
1. Log error
2. Continue with last valid frame
3. Attempt to seek past corrupt region

**Out of Memory:**
1. Clear texture pool
2. Reduce quality settings
3. Notify user

## Performance Targets

### Phase 0 Goals

- **Frame Rate:** 60 fps @ 1920x1080
- **Frame Latency:** <50ms (decode → present)
- **Texture Upload:** <1ms for 1920x1080 RGBA
- **Memory Usage:** <500MB for single 1080p stream

### Measurement Points

```rust
// CPU timing
let start = Instant::now();
decoder.next_frame()?;
let decode_time = start.elapsed();

// GPU timing (timestamp queries)
let query_set = device.create_query_set(...);
encoder.write_timestamp(&query_set, 0); // Start
render_pass.draw(...);
encoder.write_timestamp(&query_set, 1); // End
// Resolve timestamps and calculate GPU time
```

## Configuration

### Settings Files

**Runtime Configuration:**
- Location: `~/.config/mapmap/settings.toml`
- Format: TOML
- Contents: Window size, backend preferences, logging level

**Project Files:**
- Format: JSON or XML (maintaining compatibility with old MapMap)
- Contents: Paints, Mappings, Shapes, Media sources

### Example Configuration

```toml
[window]
width = 1920
height = 1080
fullscreen = false
vsync = true

[graphics]
backend = "auto"  # auto, vulkan, metal, dx12
validation = false  # Enable in debug builds only

[logging]
level = "info"  # trace, debug, info, warn, error
```

## Testing Strategy

### Unit Tests
- Each crate has `#[cfg(test)]` modules
- Test public API surface
- Use `proptest` for geometry calculations

### Integration Tests
- Located in `tests/` directory
- Test cross-crate interactions
- Require real wgpu device (headless mode)

### Benchmark Tests
- Located in `benches/` directory
- Use `criterion` for statistical analysis
- Key metrics: texture upload, decode throughput, frame time

### CI Pipeline
- GitHub Actions matrix: Linux, macOS, Windows
- Checks: format, clippy, tests, docs, security audit
- Caching: cargo registry, build artifacts

## Future Phases

### Phase 1 (Core Engine)
- Multi-threaded decode/upload/render pipeline
- Hardware-accelerated video decode (VA-API, VideoToolbox, DXVA)
- Advanced compositing (blend modes, opacity)
- Layer system implementation

### Phase 2 (Professional Warping)
- Multi-window/multi-output support
- Mesh warping with control points
- Edge blending
- Geometric correction (keystone, perspective)

### Phase 3 (Effects Pipeline)
- Shader graph system
- Parameter animation
- Audio-reactive effects (FFT, beat detection)
- LUT color grading

### Phase 4 (Control Systems)
- Full MIDI implementation
- OSC server and routing
- Art-Net/sACN DMX output
- HTTP REST API
- Show management (cues, timelines)

### Phase 5 (Pro Media I/O)
- NDI receive/send
- DeckLink SDI input/output
- Spout (Windows) and Syphon (macOS)
- Genlock and frame synchronization

## References

- [wgpu Documentation](https://wgpu.rs/)
- [Rust Graphics Community](https://arewegameyet.rs/)
- [Resolume Arena](https://resolume.com/) - Reference implementation
- [MapMap Legacy](https://github.com/mapmapteam/mapmap) - Original C++/Qt codebase

## Glossary

- **Paint:** A media source (video, image, or color)
- **Mapping:** Connection between a Paint and a Shape
- **Shape:** Geometry to project onto (quad, mesh, etc.)
- **wgpu:** WebGPU implementation for Rust
- **Staging Buffer:** GPU-mappable buffer for CPU→GPU transfers
- **PBO:** Pixel Buffer Object (OpenGL term, equivalent to staging buffer)
- **VSync:** Vertical synchronization (locks frame rate to display refresh)
- **HAP:** High-performance video codec for real-time playback
- **NDI:** Network Device Interface (IP-based video I/O)
- **DMX:** Digital Multiplex (lighting control protocol)
