# MapMap Implementation Guide
## Weeks 2-4 Roadmap

This document provides detailed implementation guidance for the remaining features.

---

## Week 2: Real Video Support

### Goal
Replace test patterns with actual video file playback using FFmpeg.

### Prerequisites

**System Dependencies**:
```bash
# Ubuntu/Debian
sudo apt-get install -y \
  libavcodec-dev \
  libavformat-dev \
  libavutil-dev \
  libswscale-dev \
  libavfilter-dev

# macOS
brew install ffmpeg

# Windows
# Download FFmpeg development libraries from ffmpeg.org
```

**Rust Dependencies** - Add to `Cargo.toml`:
```toml
[dependencies]
rfd = "0.14"  # Native file dialogs
```

### Task 1: Implement Real FFmpeg Decoder

**File**: `crates/mapmap-media/src/decoder.rs`

**Current state**: Has `FFmpegDecoder::TestPattern` variant

**Add**:
```rust
pub enum FFmpegDecoder {
    TestPattern(TestPatternDecoder),
    Real {
        format_context: ffmpeg_next::format::context::Input,
        video_stream_index: usize,
        decoder: ffmpeg_next::decoder::Video,
        scaler: ffmpeg_next::software::scaling::Context,
    },
}

impl FFmpegDecoder {
    pub fn from_path(path: &str) -> Result<Self> {
        // Open input file
        let mut ictx = ffmpeg_next::format::input(&path)?;

        // Find video stream
        let input = ictx
            .streams()
            .best(ffmpeg_next::media::Type::Video)
            .ok_or(MediaError::DecodeError("No video stream".into()))?;

        let video_stream_index = input.index();

        // Get decoder
        let context_decoder = ffmpeg_next::codec::context::Context::from_parameters(
            input.parameters()
        )?;
        let decoder = context_decoder.decoder().video()?;

        // Create scaler for RGBA conversion
        let scaler = ffmpeg_next::software::scaling::Context::get(
            decoder.format(),
            decoder.width(),
            decoder.height(),
            ffmpeg_next::format::Pixel::RGBA,
            decoder.width(),
            decoder.height(),
            ffmpeg_next::software::scaling::Flags::BILINEAR,
        )?;

        Ok(FFmpegDecoder::Real {
            format_context: ictx,
            video_stream_index,
            decoder,
            scaler,
        })
    }
}
```

**Update `decode_frame()`**:
```rust
impl VideoDecoder for FFmpegDecoder {
    fn decode_frame(&mut self) -> Result<Option<DecodedFrame>> {
        match self {
            FFmpegDecoder::TestPattern(decoder) => decoder.decode_frame(),
            FFmpegDecoder::Real {
                format_context,
                video_stream_index,
                decoder,
                scaler,
            } => {
                // Read packets until we get a video frame
                for (stream, packet) in format_context.packets() {
                    if stream.index() == *video_stream_index {
                        decoder.send_packet(&packet)?;

                        let mut decoded = ffmpeg_next::util::frame::Video::empty();
                        if decoder.receive_frame(&mut decoded).is_ok() {
                            // Scale to RGBA
                            let mut rgb_frame = ffmpeg_next::util::frame::Video::empty();
                            scaler.run(&decoded, &mut rgb_frame)?;

                            return Ok(Some(DecodedFrame {
                                width: rgb_frame.width(),
                                height: rgb_frame.height(),
                                format: PixelFormat::RGBA,
                                data: rgb_frame.data(0).to_vec(),
                                timestamp: packet.pts(),
                            }));
                        }
                    }
                }

                Ok(None)  // End of stream
            }
        }
    }
}
```

### Task 2: Add File Picker

**File**: `crates/mapmap-ui/src/lib.rs`

**Add method to AppUI**:
```rust
impl AppUI {
    /// Show file picker and return selected path
    pub fn pick_video_file() -> Option<String> {
        rfd::FileDialog::new()
            .add_filter("Video Files", &["mp4", "mov", "avi", "mkv", "webm", "flv"])
            .add_filter("All Files", &["*"])
            .set_title("Select Video File")
            .pick_file()
            .map(|p| p.to_string_lossy().to_string())
    }
}
```

**Update menu action**:
```rust
// In render_menu_bar()
if ui.menu_item("Load Video") {
    if let Some(path) = Self::pick_video_file() {
        self.actions.push(UIAction::LoadVideo(path));
    }
}
```

### Task 3: Wire Up Video Loading

**File**: `crates/mapmap/src/main.rs`

**Update action handler**:
```rust
UIAction::LoadVideo(path) if !path.is_empty() => {
    info!("Loading video from: {}", path);

    match mapmap_media::FFmpegDecoder::from_path(&path) {
        Ok(decoder) => {
            // Create new paint for this video
            let next_id = self.paint_manager.paints().len() as u64 + 1;
            let paint = Paint::video(next_id, &path);
            let paint_id = self.paint_manager.add_paint(paint);

            // Create player
            let mut player = mapmap_media::VideoPlayer::new(decoder);
            player.set_looping(self.ui_state.looping);
            player.set_speed(self.ui_state.playback_speed);
            player.play();

            self.video_players.insert(paint_id, player);

            info!("Video loaded successfully: {} (Paint ID: {})", path, paint_id);
        }
        Err(e) => {
            error!("Failed to load video: {}", e);
        }
    }
}
```

**Add to `Paint` in `mapmap-core/src/paint.rs`**:
```rust
impl Paint {
    pub fn video(id: PaintId, path: &str) -> Self {
        Self {
            id,
            name: path.split('/').last().unwrap_or("Video").to_string(),
            paint_type: PaintType::Video,
            uri: path.to_string(),
            // ... other fields
        }
    }
}
```

### Testing Week 2

```bash
cargo build --release
cargo run --release

# Test:
# 1. File > Load Video
# 2. Select an MP4/MOV file
# 3. Verify it appears in Paints panel
# 4. Verify it plays on mapped surfaces
# 5. Test play/pause/speed controls
```

---

## Week 3: Interactive Editing

### Goal
Enable mouse interaction for editing mapping geometry and project save/load.

### Task 1: Mouse Vertex Dragging

**File**: `crates/mapmap/src/main.rs`

**Add fields to App**:
```rust
struct App {
    // ... existing fields ...

    selected_mapping: Option<u64>,
    dragging_vertex: Option<(u64, usize)>, // (mapping_id, vertex_index)
    mouse_position: (f32, f32),
}
```

**Add mouse event handling**:
```rust
impl App {
    fn handle_window_event(&mut self, event: &WindowEvent) -> bool {
        match event {
            WindowEvent::MouseInput { state, button, .. } => {
                if *button == winit::event::MouseButton::Left {
                    match state {
                        winit::event::ElementState::Pressed => {
                            self.on_mouse_down();
                        }
                        winit::event::ElementState::Released => {
                            self.dragging_vertex = None;
                        }
                    }
                }
            }
            WindowEvent::CursorMoved { position, .. } => {
                // Convert window coordinates to NDC (-1 to 1)
                let ndc_x = (position.x / self.surface_config.width as f64) * 2.0 - 1.0;
                let ndc_y = 1.0 - (position.y / self.surface_config.height as f64) * 2.0;
                self.mouse_position = (ndc_x as f32, ndc_y as f32);

                if let Some((mapping_id, vertex_idx)) = self.dragging_vertex {
                    self.update_vertex_position(mapping_id, vertex_idx);
                }
            }
            // ... existing cases ...
        }
        true
    }

    fn on_mouse_down(&mut self) {
        // Find closest vertex to mouse position
        if let Some(mapping_id) = self.selected_mapping {
            if let Some(mapping) = self.mapping_manager.get_mapping(mapping_id) {
                let threshold = 0.05; // 5% of screen

                for (idx, vertex) in mapping.mesh.vertices.iter().enumerate() {
                    let dx = vertex.position.x - self.mouse_position.0;
                    let dy = vertex.position.y - self.mouse_position.1;
                    let dist = (dx * dx + dy * dy).sqrt();

                    if dist < threshold {
                        self.dragging_vertex = Some((mapping_id, idx));
                        return;
                    }
                }
            }
        }
    }

    fn update_vertex_position(&mut self, mapping_id: u64, vertex_idx: usize) {
        if let Some(mapping) = self.mapping_manager.get_mapping_mut(mapping_id) {
            if let Some(vertex) = mapping.mesh.vertices.get_mut(vertex_idx) {
                vertex.position.x = self.mouse_position.0;
                vertex.position.y = self.mouse_position.1;
            }
        }
    }
}
```

**Update SelectMapping action**:
```rust
UIAction::SelectMapping(id) => {
    info!("Selected mapping {}", id);
    self.selected_mapping = Some(id);
}
```

### Task 2: Project Save/Load

**Add dependency** - `Cargo.toml`:
```toml
serde_json = "1.0"
```

**File**: `crates/mapmap-core/src/lib.rs`

**Add Project struct**:
```rust
#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct Project {
    pub version: String,
    pub paints: Vec<Paint>,
    pub mappings: Vec<Mapping>,
    pub layers: Vec<Layer>,
}

impl Project {
    pub fn new() -> Self {
        Self {
            version: "0.1.0".to_string(),
            paints: Vec::new(),
            mappings: Vec::new(),
            layers: Vec::new(),
        }
    }

    pub fn save_to_file(&self, path: &str) -> std::io::Result<()> {
        let json = serde_json::to_string_pretty(self)?;
        std::fs::write(path, json)?;
        Ok(())
    }

    pub fn load_from_file(path: &str) -> std::io::Result<Self> {
        let json = std::fs::read_to_string(path)?;
        let project = serde_json::from_str(&json)?;
        Ok(project)
    }
}
```

**File**: `crates/mapmap/src/main.rs`

**Implement save/load**:
```rust
impl App {
    fn save_project(&self, path: &str) -> Result<()> {
        let project = mapmap_core::Project {
            version: "0.1.0".to_string(),
            paints: self.paint_manager.paints().to_vec(),
            mappings: self.mapping_manager.mappings().to_vec(),
            layers: self.layer_manager.layers().to_vec(),
        };

        project.save_to_file(path)?;
        info!("Project saved to: {}", path);
        Ok(())
    }

    fn load_project(&mut self, path: &str) -> Result<()> {
        let project = mapmap_core::Project::load_from_file(path)?;

        // Clear current state
        self.paint_manager = PaintManager::new();
        self.mapping_manager = MappingManager::new();
        self.video_players.clear();
        self.paint_textures.clear();

        // Load paints
        for paint in project.paints {
            let paint_id = self.paint_manager.add_paint(paint.clone());

            // Recreate video player
            let decoder = if paint.paint_type == mapmap_core::PaintType::Video {
                mapmap_media::FFmpegDecoder::from_path(&paint.uri)?
            } else {
                mapmap_media::FFmpegDecoder::TestPattern(
                    mapmap_media::TestPatternDecoder::new(1920, 1080, std::time::Duration::from_secs(60), 30.0)
                )
            };

            let mut player = mapmap_media::VideoPlayer::new(decoder);
            player.set_looping(self.ui_state.looping);
            player.play();
            self.video_players.insert(paint_id, player);
        }

        // Load mappings
        for mapping in project.mappings {
            self.mapping_manager.add_mapping(mapping);
        }

        info!("Project loaded from: {}", path);
        Ok(())
    }
}
```

**Update action handlers**:
```rust
UIAction::SaveProject(_) => {
    if let Some(path) = rfd::FileDialog::new()
        .add_filter("MapMap Project", &["mmp", "json"])
        .set_title("Save Project")
        .save_file()
    {
        if let Err(e) = self.save_project(&path.to_string_lossy()) {
            error!("Failed to save project: {}", e);
        }
    }
}

UIAction::LoadProject(_) => {
    if let Some(path) = rfd::FileDialog::new()
        .add_filter("MapMap Project", &["mmp", "json"])
        .set_title("Load Project")
        .pick_file()
    {
        if let Err(e) = self.load_project(&path.to_string_lossy()) {
            error!("Failed to load project: {}", e);
        }
    }
}
```

### Testing Week 3

```bash
cargo run --release

# Test vertex dragging:
# 1. Click on a mapping name to select it
# 2. Click and drag vertices to reposition them
# 3. Verify mapping warps correctly

# Test save/load:
# 1. Create some mappings and load videos
# 2. File > Save Project
# 3. Close and reopen app
# 4. File > Load Project
# 5. Verify all mappings and videos restored
```

---

## Week 4: Multi-Output & Production Features

### Goal
Support multiple output windows, fullscreen mode, and edge blending.

### Task 1: Multiple Output Windows

**Approach**: Each output window needs its own surface and event loop integration.

**File**: `crates/mapmap/src/main.rs`

**Add OutputWindow struct**:
```rust
struct OutputWindow {
    window: winit::window::Window,
    surface: wgpu::Surface,
    surface_config: wgpu::SurfaceConfiguration,
    fullscreen: bool,
}

impl OutputWindow {
    fn new(
        event_loop: &EventLoop<()>,
        backend: &WgpuBackend,
        width: u32,
        height: u32,
        title: &str,
    ) -> Result<Self> {
        let window = WindowBuilder::new()
            .with_title(title)
            .with_inner_size(winit::dpi::PhysicalSize::new(width, height))
            .build(event_loop)?;

        let surface = unsafe { backend.create_surface(&window) }?;

        let surface_config = wgpu::SurfaceConfiguration {
            usage: wgpu::TextureUsages::RENDER_ATTACHMENT,
            format: wgpu::TextureFormat::Bgra8Unorm,
            width,
            height,
            present_mode: wgpu::PresentMode::Fifo,
            alpha_mode: wgpu::CompositeAlphaMode::Opaque,
            view_formats: vec![],
        };

        surface.configure(backend.device(), &surface_config);

        Ok(Self {
            window,
            surface,
            surface_config,
            fullscreen: false,
        })
    }

    fn toggle_fullscreen(&mut self) {
        self.fullscreen = !self.fullscreen;
        if self.fullscreen {
            self.window.set_fullscreen(Some(
                winit::window::Fullscreen::Borderless(None)
            ));
        } else {
            self.window.set_fullscreen(None);
        }
    }

    fn render_mappings(&self, /* ... */) -> Result<()> {
        // Similar to main App::render() but without UI
        // Render only the projection mappings
        Ok(())
    }
}
```

**Update App**:
```rust
struct App {
    // ... existing fields ...
    output_windows: Vec<OutputWindow>,
}

impl App {
    fn add_output_window(&mut self) -> Result<()> {
        // Note: Creating windows outside event loop is tricky
        // Consider using window creation events instead
        let window = OutputWindow::new(
            &self.event_loop,  // Need to store event_loop reference
            &self.backend,
            1920,
            1080,
            "MapMap Output 2",
        )?;

        self.output_windows.push(window);
        Ok(())
    }
}
```

### Task 2: Fullscreen Toggle

**Simple approach** (toggle main window):
```rust
UIAction::ToggleFullscreen => {
    info!("Toggle fullscreen");
    let fullscreen = self.window.fullscreen();

    if fullscreen.is_none() {
        // Enter fullscreen
        self.window.set_fullscreen(Some(
            winit::window::Fullscreen::Borderless(None)
        ));
    } else {
        // Exit fullscreen
        self.window.set_fullscreen(None);
    }
}
```

**Keyboard shortcut**:
```rust
WindowEvent::KeyboardInput { input, .. } => {
    if let Some(winit::event::VirtualKeyCode::F11) = input.virtual_keycode {
        if input.state == winit::event::ElementState::Pressed {
            self.handle_ui_actions();  // Process fullscreen toggle
        }
    }
}
```

### Task 3: Edge Blending Preparation

**Add blend zone struct** - `crates/mapmap-core/src/lib.rs`:
```rust
#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct EdgeBlend {
    pub enabled: bool,
    pub left_width: f32,    // 0.0 to 1.0
    pub right_width: f32,
    pub top_width: f32,
    pub bottom_width: f32,
    pub gamma: f32,         // Blend curve gamma
}

impl Default for EdgeBlend {
    fn default() -> Self {
        Self {
            enabled: false,
            left_width: 0.1,
            right_width: 0.1,
            top_width: 0.1,
            bottom_width: 0.1,
            gamma: 2.2,
        }
    }
}
```

**Add to OutputWindow**:
```rust
struct OutputWindow {
    // ... existing fields ...
    edge_blend: EdgeBlend,
}
```

**Shader implementation** - Create `shaders/edge_blend.wgsl`:
```wgsl
struct EdgeBlendUniforms {
    left_width: f32,
    right_width: f32,
    top_width: f32,
    bottom_width: f32,
    gamma: f32,
}

@group(0) @binding(0)
var<uniform> blend: EdgeBlendUniforms;

@fragment
fn fs_blend(in: VertexOutput) -> @location(0) vec4<f32> {
    let uv = in.tex_coords;
    var alpha = 1.0;

    // Left edge
    if (uv.x < blend.left_width) {
        alpha *= pow(uv.x / blend.left_width, blend.gamma);
    }

    // Right edge
    if (uv.x > (1.0 - blend.right_width)) {
        let dist = (1.0 - uv.x) / blend.right_width;
        alpha *= pow(dist, blend.gamma);
    }

    // Top edge
    if (uv.y < blend.top_width) {
        alpha *= pow(uv.y / blend.top_width, blend.gamma);
    }

    // Bottom edge
    if (uv.y > (1.0 - blend.bottom_width)) {
        let dist = (1.0 - uv.y) / blend.bottom_width;
        alpha *= pow(dist, blend.gamma);
    }

    let color = textureSample(t_texture, s_sampler, uv);
    return vec4<f32>(color.rgb, color.a * alpha);
}
```

**UI Controls** - Add to `AppUI`:
```rust
pub fn render_edge_blend_panel(&mut self, ui: &Ui, output: &mut OutputWindow) {
    ui.window("Edge Blending")
        .build(|| {
            ui.checkbox("Enable Edge Blend", &mut output.edge_blend.enabled);

            if output.edge_blend.enabled {
                ui.slider("Left", 0.0, 0.5, &mut output.edge_blend.left_width);
                ui.slider("Right", 0.0, 0.5, &mut output.edge_blend.right_width);
                ui.slider("Top", 0.0, 0.5, &mut output.edge_blend.top_width);
                ui.slider("Bottom", 0.0, 0.5, &mut output.edge_blend.bottom_width);
                ui.slider("Gamma", 1.0, 4.0, &mut output.edge_blend.gamma);
            }
        });
}
```

### Testing Week 4

```bash
cargo run --release

# Test fullscreen:
# 1. Press F11 or View > Toggle Fullscreen
# 2. Verify window goes fullscreen
# 3. Press F11 again to exit

# Test multiple outputs:
# 1. Create second output window
# 2. Verify mappings render on both
# 3. Test independent control

# Test edge blending:
# 1. Enable edge blend
# 2. Adjust sliders
# 3. Verify smooth fade at edges
```

---

## Summary

### Completed (Week 1) ✅
- Functional play/pause/stop controls
- Speed slider
- Loop toggle
- Mapping visibility
- Add/remove mappings and paints
- Exit menu

### Week 2 Scope 🎬
- Real FFmpeg video decoding
- File picker integration
- Display actual videos

### Week 3 Scope ✏️
- Mouse vertex dragging
- Project save/load (JSON)
- Selection highlighting

### Week 4 Scope 🖥️
- Multiple output windows
- Fullscreen mode
- Edge blending shaders

### Total Estimated Effort
- Week 2: ~300 lines (FFmpeg integration)
- Week 3: ~400 lines (interaction + serialization)
- Week 4: ~500 lines (multi-window + shaders)

---

## Useful Resources

### FFmpeg
- [ffmpeg-next docs](https://docs.rs/ffmpeg-next)
- [FFmpeg decoding guide](https://ffmpeg.org/doxygen/trunk/group__lavc__decoding.html)

### File Dialogs
- [rfd crate](https://docs.rs/rfd)

### Serialization
- [serde_json](https://docs.rs/serde_json)

### Multi-window
- [winit examples](https://github.com/rust-windowing/winit/tree/master/examples)

---

**Last Updated**: 2025-11-11
**Status**: Week 1 Complete ✅
