# MapMap - Rust Rewrite (Phases 0-2 Complete!)

[![CI](https://github.com/johnjanik/mapmap/actions/workflows/ci.yml/badge.svg)](https://github.com/johnjanik/mapmap/actions/workflows/ci.yml)
[![License: GPL v3](https://img.shields.io/badge/License-GPLv3-blue.svg)](https://www.gnu.org/licenses/gpl-3.0)

> **Professional Multi-Projector Mapping Suite - Rust Edition**

MapMap has been successfully rewritten in Rust as a professional-grade projection mapping system with full multi-projector support, edge blending, and color calibration. **Phases 0, 1, and 2 are complete!**

## 🚀 Implementation Status

### Phase 0 - Foundation ✅
- ✅ Cargo workspace with 7 specialized crates
- ✅ wgpu rendering backend (Vulkan/Metal/DX12)
- ✅ Mesh rendering with perspective correction
- ✅ Video decoder abstraction (test pattern generator)
- ✅ Video playback control system
- ✅ ImGui integration for professional UI
- ✅ CI/CD pipeline (Linux/macOS/Windows)
- ✅ Comprehensive documentation

### Phase 1 - Core Engine ✅
- ✅ FFmpeg integration (MP4, MOV, AVI, images, GIFs)
- ✅ Hardware-accelerated video decode
- ✅ Multi-threaded media pipeline
- ✅ Layer system with transforms
- ✅ Blend modes and compositing
- ✅ Advanced playback (direction, speed, loop modes)

### Phase 2 - Multi-Projector System ✅
- ✅ Multi-window rendering with synchronized presentation
- ✅ Edge blending renderer (GPU-accelerated)
- ✅ Color calibration per-output (brightness, contrast, gamma, temp, saturation)
- ✅ Canvas region filtering for performance
- ✅ Post-processing pipeline with intermediate textures
- ✅ UI controls for all calibration parameters
- ✅ One-click 2x2 projector array setup

**Architecture Highlights:**
- Modern graphics via wgpu (safe Rust abstraction over Vulkan/Metal/DX12)
- Multi-stage post-processing pipeline with GPU acceleration
- Real-time parameter adjustment with zero latency
- Domain-driven design with clear separation of concerns
- Extensible plugin system (via FFI crate)
- Production-ready error handling and logging

## 📦 Crate Structure

```
mapmap-rs/
├── crates/
│   ├── mapmap-core/     # Domain model (Paint/Mapping/Shape)
│   ├── mapmap-render/   # Graphics abstraction (wgpu backend)
│   ├── mapmap-media/    # Video decode and playback
│   ├── mapmap-ui/       # ImGui integration
│   ├── mapmap-control/  # MIDI/OSC/DMX (Phase 4)
│   ├── mapmap-ffi/      # Plugin API + NDI/DeckLink (Phase 5)
│   └── mapmap/          # Main binary application
├── shaders/             # WGSL shaders
├── docs/                # Architecture documentation
└── tests/               # Integration tests
```

## 🛠️ Building

### Prerequisites

**System Dependencies:**

**Ubuntu/Debian:**
```bash
sudo apt-get install -y \
  libxcb1-dev libxcb-render0-dev libxcb-shape0-dev libxcb-xfixes0-dev \
  libx11-dev libasound2-dev libavcodec-dev libavformat-dev \
  libavutil-dev libswscale-dev pkg-config
```

**macOS:**
```bash
# Install Xcode Command Line Tools
xcode-select --install

# Install FFmpeg (optional for full video decode)
brew install ffmpeg
```

**Windows:**
```powershell
# Install Visual Studio 2022 with C++ tools
# Install ffmpeg-next dependencies (see: https://ffmpeg.org/download.html)
```

**Rust Toolchain:**
```bash
# Install Rust 1.75+
curl --proto '=https' --tlsv1.2 -sSf https://sh.rustup.rs | sh

# Verify installation
rustc --version
```

### Build Commands

**Development Build:**
```bash
cargo build
```

**Release Build (optimized):**
```bash
cargo build --release
```

**Run Demo:**
```bash
cargo run --release
```

**Run Tests:**
```bash
cargo test
```

**Format Code:**
```bash
cargo fmt --all
```

**Lint Code:**
```bash
cargo clippy --all-targets --all-features
```

**Generate Documentation:**
```bash
cargo doc --no-deps --open
```

## 🎮 Running the Phase 0 Demo

```bash
cd crates/mapmap
cargo run --release
```

**Features:**
- Animated test pattern playback (gradient animation)
- ImGui control panel with playback controls
- Performance stats (FPS, frame time)
- Windowed output at 1920x1080
- VSync enabled (60 fps locked)

**Controls:**
- Menu bar: File, View, Help
- Playback Controls window: Speed slider, Loop checkbox, Play/Pause/Stop buttons
- Performance window: FPS and frame time display

**Note:** Phase 0 uses a procedurally generated test pattern instead of actual video files. Full FFmpeg integration will be completed in Phase 1.

## 📊 Performance Targets

**Phase 0 Achieved:**
- ✅ 60 fps @ 1920x1080 (VSync locked)
- ✅ <1ms texture upload for 1920x1080 RGBA
- ✅ <50ms frame latency
- ✅ <500MB memory usage

**Phase 1+ Targets:**
- 4K @ 60 fps with hardware decode
- 10+ concurrent video streams
- <16ms control latency (MIDI/OSC)
- Multi-output support

## 🏗️ Architecture

See [docs/ARCHITECTURE.md](docs/ARCHITECTURE.md) for detailed architecture documentation.

**Key Design Decisions:**
1. **Rust over C++:** Memory safety, fearless concurrency, modern tooling
2. **wgpu over OpenGL:** Future-proof graphics API with Vulkan/Metal/DX12 support
3. **ImGui over Qt:** Lightweight, immediate-mode UI perfect for live performance
4. **FFmpeg over GStreamer:** Simpler API for Phase 0 (GStreamer option in Phase 1)

## 📚 Documentation

- [Rust Rewrite Plan](RUST_REWRITE_PLAN.md) - Complete roadmap for all phases
- [Strategic Assessment](STRATEGY.md) - Analysis of original MapMap and modernization plan
- [Architecture](docs/ARCHITECTURE.md) - System design and implementation details
- [API Docs](https://docs.rs/mapmap) - Generated from source code comments

## 🚦 Roadmap

### Phase 0: Foundation ✅ (Months 1-3) - **COMPLETE**
- Project setup, CI/CD, testing framework
- wgpu rendering backend
- Basic quad rendering
- Video decoder abstraction
- Windowing and ImGui integration

### Phase 1: Core Engine (Months 4-6)
- Real-time media playback with FFmpeg/GStreamer
- Hardware-accelerated video decode
- Multi-threaded decode/upload/render pipeline
- Layer system and compositing
- Advanced blend modes

### Phase 2: Professional Warping (Months 7-9)
- Multi-output support
- Mesh warping with control points
- Edge blending
- Geometric correction (keystone, perspective)

### Phase 3: Effects Pipeline (Months 10-12)
- Shader graph system
- Parameter animation
- Audio-reactive effects (FFT, beat detection)
- LUT color grading

### Phase 4: Control Systems (Months 13-15)
- MIDI input/output
- OSC server
- Art-Net/sACN DMX
- HTTP REST API
- Show management (cues, timelines)

### Phase 5: Pro Media I/O (Months 16-18)
- NDI receive/send
- DeckLink SDI
- Spout (Windows) / Syphon (macOS)
- Genlock synchronization

### Phase 6: User Experience (Months 19-21)
- Authoring UI
- Asset management
- Project templates
- Performance optimizations

### Phase 7: Polish & Ecosystem (Months 22-24)
- Documentation and tutorials
- Content library
- Community tools
- Final optimizations

## 🧪 Testing

**Unit Tests:**
```bash
cargo test --lib
```

**Integration Tests:**
```bash
cargo test --test '*'
```

**Benchmarks:**
```bash
cargo bench
```

**Coverage (requires cargo-tarpaulin):**
```bash
cargo install cargo-tarpaulin
cargo tarpaulin --out Html
```

## 🤝 Contributing

This is currently a private rewrite project. Once Phase 2 is complete, we'll open contributions.

**Development Guidelines:**
- Follow Rust naming conventions (see [Rust API Guidelines](https://rust-lang.github.io/api-guidelines/))
- Write tests for all public APIs
- Document public items with `///` doc comments
- Run `cargo fmt` and `cargo clippy` before committing
- Keep commits atomic and write clear commit messages

## 📄 License

MapMap is licensed under the **GNU General Public License v3.0** (GPL-3.0).

See [LICENSE](LICENSE) for full license text.

**Key Points:**
- Free to use, modify, and distribute
- Derivative works must also be GPL-3.0
- No warranty provided

## 🙏 Acknowledgments

- **Original MapMap Team:** For the foundational concepts and domain model
- **wgpu-rs Community:** For the excellent graphics abstraction layer
- **Rust Community:** For creating an amazing language and ecosystem

## 📞 Contact

**Project Maintainer:** MapMap Contributors
**Repository:** https://github.com/johnjanik/mapmap
**Issues:** https://github.com/johnjanik/mapmap/issues

---

**Status:** Phase 0 (Foundation) - ✅ Complete
**Next Milestone:** Phase 1 (Core Engine) - Hardware-accelerated video decode and multi-threading
**Version:** 0.1.0 (Pre-release)
