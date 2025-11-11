# Build Instructions

This document provides comprehensive build instructions for MapMap (Rust Edition) on all supported platforms.

## Table of Contents

- [Prerequisites](#prerequisites)
- [Linux Build](#linux-build)
- [macOS Build](#macos-build)
- [Windows Build](#windows-build)
- [Building for Development](#building-for-development)
- [Building for Release](#building-for-release)
- [Running Tests](#running-tests)
- [Troubleshooting](#troubleshooting)

---

## Prerequisites

### Rust Toolchain

MapMap requires **Rust 1.75 or later**. Install it using rustup:

```bash
# Install Rust
curl --proto '=https' --tlsv1.2 -sSf https://sh.rustup.rs | sh

# Follow the on-screen instructions, then restart your terminal

# Verify installation
rustc --version
cargo --version
```

### Git

Ensure you have Git installed to clone the repository:

```bash
# Verify Git installation
git --version
```

---

## Linux Build

### System Requirements

- **OS:** Ubuntu 20.04+, Debian 11+, Fedora 35+, or equivalent
- **CPU:** x86_64 with SSE2 support
- **GPU:** Vulkan 1.2 compatible (NVIDIA GTX 900+, AMD GCN 2.0+, Intel HD 4000+)
- **RAM:** 4GB minimum, 8GB recommended
- **Disk Space:** 2GB for build artifacts

### Install Dependencies

#### Ubuntu/Debian

```bash
# Update package lists
sudo apt-get update

# Install build essentials
sudo apt-get install -y build-essential pkg-config

# Install X11/Wayland development libraries
sudo apt-get install -y \
  libxcb1-dev \
  libxcb-render0-dev \
  libxcb-shape0-dev \
  libxcb-xfixes0-dev \
  libx11-dev \
  libwayland-dev \
  libxkbcommon-dev

# Install font rendering libraries (required for ImGui)
sudo apt-get install -y \
  libfontconfig1-dev \
  libfreetype6-dev

# Install audio libraries (for future audio support)
sudo apt-get install -y libasound2-dev

# REQUIRED for real video playback: Install FFmpeg development libraries
# (Week 2+ feature - file picker and video loading)
sudo apt-get install -y \
  libavcodec-dev \
  libavformat-dev \
  libavutil-dev \
  libswscale-dev \
  libavdevice-dev \
  libavfilter-dev
```

#### Fedora

```bash
# Install build essentials
sudo dnf install -y gcc gcc-c++ pkg-config

# Install X11/Wayland development libraries
sudo dnf install -y \
  libxcb-devel \
  libX11-devel \
  wayland-devel \
  libxkbcommon-devel

# Install font rendering libraries
sudo dnf install -y \
  fontconfig-devel \
  freetype-devel

# Install audio libraries
sudo dnf install -y alsa-lib-devel

# Optional: Install FFmpeg development libraries
sudo dnf install -y \
  ffmpeg-devel \
  ffmpeg-free-devel
```

#### Arch Linux

```bash
# Install base development tools
sudo pacman -Syu base-devel

# Install required libraries
sudo pacman -S \
  libxcb \
  libx11 \
  wayland \
  libxkbcommon \
  fontconfig \
  freetype2 \
  alsa-lib

# Optional: Install FFmpeg
sudo pacman -S ffmpeg
```

### Build Steps

```bash
# Clone the repository
git clone https://github.com/johnjanik/mapmap.git
cd mapmap

# Build in debug mode (faster compile, slower runtime)
cargo build

# Build in release mode (slower compile, optimized runtime)
cargo build --release

# Run the application
cargo run --release

# Run with Wayland backend (if preferred over X11)
WINIT_UNIX_BACKEND=wayland cargo run --release
```

### Building with FFmpeg Video Support

To enable real video file playback (Week 2+ feature), you must:

1. **Install FFmpeg development libraries** (see above)
2. **Build with the `ffmpeg` feature flag**

```bash
# Build with FFmpeg support (requires FFmpeg dev libraries)
cargo build --release --features ffmpeg

# Run with FFmpeg support
cargo run --release --features ffmpeg
```

**Without FFmpeg feature:**
- File > Load Video will open dialog but fall back to test patterns
- Only procedural test patterns will play

**With FFmpeg feature:**
- Real video files (.mp4, .mov, .avi, etc.) will load and play
- Hardware acceleration available (when configured)

**Quick install script:**
```bash
# Run the helper script to install FFmpeg dev libraries
./install-ffmpeg-dev.sh
```

### GPU Backend Selection

By default, MapMap will use Vulkan on Linux. You can override this:

```bash
# Force Vulkan (default)
WGPU_BACKEND=vulkan cargo run --release

# Fallback to OpenGL (if Vulkan drivers are problematic)
WGPU_BACKEND=gl cargo run --release
```

---

## macOS Build

### System Requirements

- **OS:** macOS 12 (Monterey) or later
- **CPU:** Intel x86_64 or Apple Silicon (M1/M2/M3)
- **GPU:** Metal-compatible (all modern Macs)
- **RAM:** 8GB minimum
- **Disk Space:** 2GB for build artifacts

### Install Dependencies

#### Install Xcode Command Line Tools

```bash
# Install Xcode CLI tools
xcode-select --install

# Verify installation
xcode-select -p
```

#### Optional: Install FFmpeg (for Phase 1+)

```bash
# Install Homebrew (if not already installed)
/bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"

# Install FFmpeg
brew install ffmpeg
```

### Build Steps

```bash
# Clone the repository
git clone https://github.com/johnjanik/mapmap.git
cd mapmap

# Build in debug mode
cargo build

# Build in release mode
cargo build --release

# Run the application
cargo run --release

# Create a macOS app bundle (future feature)
# cargo bundle --release
```

### Apple Silicon Notes

MapMap builds natively for Apple Silicon. For Intel Macs:

```bash
# Force Intel build on Apple Silicon (not recommended)
rustup target add x86_64-apple-darwin
cargo build --release --target x86_64-apple-darwin
```

---

## Windows Build

### System Requirements

- **OS:** Windows 10 (64-bit, version 1903+) or Windows 11
- **CPU:** x86_64 with SSE2 support
- **GPU:** DirectX 12 compatible (most modern GPUs)
- **RAM:** 8GB minimum
- **Disk Space:** 3GB for build artifacts

### Install Dependencies

#### Install Visual Studio 2022

MapMap requires the Microsoft C++ Build Tools:

1. Download [Visual Studio 2022 Community](https://visualstudio.microsoft.com/downloads/) (free)
2. Run the installer and select **"Desktop development with C++"**
3. Ensure these components are selected:
   - MSVC v143 - VS 2022 C++ x64/x86 build tools
   - Windows 10 SDK (latest version)
   - C++ CMake tools for Windows

#### Alternative: Build Tools Only

If you don't want the full Visual Studio IDE:

1. Download [Build Tools for Visual Studio 2022](https://visualstudio.microsoft.com/downloads/#build-tools-for-visual-studio-2022)
2. Install with the same components as above

### Build Steps

```bash
# Clone the repository (in PowerShell or Command Prompt)
git clone https://github.com/johnjanik/mapmap.git
cd mapmap

# Build in debug mode
cargo build

# Build in release mode
cargo build --release

# Run the application
cargo run --release
```

### GPU Backend Selection

By default, MapMap will use DirectX 12 on Windows. You can override:

```powershell
# Force DirectX 12 (default)
$env:WGPU_BACKEND="dx12"
cargo run --release

# Fallback to Vulkan (if DX12 drivers are problematic)
$env:WGPU_BACKEND="vulkan"
cargo run --release
```

### Optional: Install FFmpeg (for Phase 1+)

Using [vcpkg](https://vcpkg.io/):

```powershell
# Install vcpkg
git clone https://github.com/Microsoft/vcpkg.git
cd vcpkg
.\bootstrap-vcpkg.bat

# Install FFmpeg
.\vcpkg install ffmpeg:x64-windows

# Set environment variable for Cargo
$env:FFMPEG_DIR="C:\path\to\vcpkg\installed\x64-windows"
```

---

## Building for Development

### Fast Iterative Builds

```bash
# Use debug mode for faster compilation
cargo build

# Run with debug logging
RUST_LOG=debug cargo run

# Run with specific module logging
RUST_LOG=mapmap_render=trace,mapmap_media=debug cargo run

# Enable Rust backtrace on panic
RUST_BACKTRACE=1 cargo run
```

### Code Quality Checks

```bash
# Format code (required before committing)
cargo fmt --all

# Run linter (fix auto-fixable issues)
cargo clippy --all-targets --all-features --fix

# Run linter (check only)
cargo clippy --all-targets --all-features -- -D warnings

# Check code without building
cargo check --all-targets
```

### Building Specific Crates

```bash
# Build only the render crate
cargo build -p mapmap-render

# Build only the media crate
cargo build -p mapmap-media

# Build only the main binary
cargo build -p mapmap
```

---

## Building for Release

### Optimized Build

```bash
# Build with full optimizations
cargo build --release

# Binary will be in: target/release/mapmap (or mapmap.exe on Windows)

# Run the optimized binary directly
./target/release/mapmap  # Linux/macOS
.\target\release\mapmap.exe  # Windows
```

### Profile-Guided Optimization (Advanced)

For maximum performance:

```bash
# Step 1: Build with instrumentation
RUSTFLAGS="-Cprofile-generate=/tmp/pgo-data" cargo build --release

# Step 2: Run the application to collect profile data
./target/release/mapmap
# (Use the application for a few minutes, then close)

# Step 3: Merge profile data
llvm-profdata merge -o /tmp/pgo-data/merged.profdata /tmp/pgo-data

# Step 4: Build with PGO
RUSTFLAGS="-Cprofile-use=/tmp/pgo-data/merged.profdata" cargo build --release
```

### Link-Time Optimization (LTO)

LTO is already enabled in the release profile (`Cargo.toml`):

```toml
[profile.release]
opt-level = 3
lto = true          # Already enabled
codegen-units = 1
strip = true
```

---

## Running Tests

### Unit Tests

```bash
# Run all tests
cargo test

# Run tests for a specific crate
cargo test -p mapmap-render

# Run tests with output
cargo test -- --nocapture

# Run a specific test
cargo test test_texture_upload
```

### Integration Tests

```bash
# Run integration tests only
cargo test --test '*'

# Run specific integration test
cargo test --test rendering_tests
```

### Benchmarks

```bash
# Run all benchmarks (requires nightly Rust)
cargo +nightly bench

# Run specific benchmark
cargo bench -- texture_upload
```

### Test Coverage (Linux/macOS)

```bash
# Install tarpaulin
cargo install cargo-tarpaulin

# Generate coverage report
cargo tarpaulin --out Html

# Open report
open tarpaulin-report.html  # macOS
xdg-open tarpaulin-report.html  # Linux
```

---

## Building Documentation

```bash
# Generate and open API documentation
cargo doc --no-deps --open

# Generate documentation for all dependencies
cargo doc --open

# Check documentation for errors
cargo doc --no-deps --document-private-items
```

---

## Troubleshooting

### Common Build Issues

#### Linux: Missing X11/Wayland Libraries

**Error:**
```
error: failed to run custom build command for `winit v0.29.X`
```

**Solution:**
```bash
sudo apt-get install libxcb1-dev libx11-dev libwayland-dev
```

#### Linux: Missing fontconfig Library

**Error:**
```
error: failed to run custom build command for `servo-fontconfig-sys`
The system library `fontconfig` required by crate `servo-fontconfig-sys` was not found.
```

**Solution:**
```bash
# Ubuntu/Debian
sudo apt-get install libfontconfig1-dev libfreetype6-dev

# Fedora
sudo dnf install fontconfig-devel freetype-devel

# Arch Linux
sudo pacman -S fontconfig freetype2
```

#### macOS: Missing Xcode Tools

**Error:**
```
xcrun: error: invalid active developer path
```

**Solution:**
```bash
xcode-select --install
```

#### Windows: Missing Visual Studio

**Error:**
```
error: linker `link.exe` not found
```

**Solution:**
Install Visual Studio 2022 with C++ build tools (see [Windows Build](#windows-build))

#### All Platforms: Slow Build Times

**Solution:**
```bash
# Install sccache for caching compiled dependencies
cargo install sccache

# Configure Cargo to use sccache
export RUSTC_WRAPPER=sccache  # Linux/macOS
$env:RUSTC_WRAPPER="sccache"  # Windows PowerShell

# Verify caching
sccache --show-stats
```

#### GPU/Driver Issues

**Error:**
```
wgpu error: No suitable GPU adapters found
```

**Solution (Linux):**
```bash
# Update GPU drivers
sudo apt-get install mesa-vulkan-drivers vulkan-tools

# Verify Vulkan support
vulkaninfo | grep deviceName

# Force OpenGL fallback if needed
WGPU_BACKEND=gl cargo run --release
```

**Solution (Windows):**
- Update GPU drivers from manufacturer (NVIDIA/AMD/Intel)
- Ensure DirectX 12 is installed (Windows 10/11 includes it by default)

### Performance Issues

#### Slow Debug Builds

Debug builds are intentionally slower. Always use release builds for performance testing:

```bash
cargo build --release
cargo run --release
```

#### High Memory Usage During Compilation

```bash
# Reduce parallel compilation jobs
cargo build --release -j 4  # Use 4 cores instead of all
```

### Runtime Issues

#### Application Crashes on Startup

**Enable debug logging:**
```bash
RUST_LOG=debug RUST_BACKTRACE=full cargo run --release
```

**Check GPU compatibility:**
```bash
# Linux
vulkaninfo | head -n 20

# macOS
system_profiler SPDisplaysDataType

# Windows (PowerShell)
dxdiag
```

#### Black Screen / No Rendering

**Try different GPU backends:**

```bash
# Linux
WGPU_BACKEND=gl cargo run --release

# Windows
$env:WGPU_BACKEND="vulkan"
cargo run --release
```

---

## Advanced Build Options

### Cross-Compilation

#### Build for Windows from Linux

```bash
# Install cross-compilation toolchain
rustup target add x86_64-pc-windows-gnu
sudo apt-get install mingw-w64

# Build
cargo build --release --target x86_64-pc-windows-gnu
```

#### Build for Linux from Windows (via WSL2)

Use Windows Subsystem for Linux and follow the [Linux Build](#linux-build) instructions.

### Custom Build Features

```bash
# Build with optional features (when available)
cargo build --release --features "advanced-effects,pro-io"

# Build without default features
cargo build --release --no-default-features

# List available features
cargo metadata --format-version 1 | jq '.packages[0].features'
```

---

## CI/CD Build

MapMap uses GitHub Actions for continuous integration. See [`.github/workflows/ci.yml`](.github/workflows/ci.yml) for the exact build commands used in CI.

**Local CI simulation:**

```bash
# Run the same checks as CI
cargo fmt --all -- --check
cargo clippy --all-targets --all-features -- -D warnings
cargo test --all-features
cargo build --release
```

---

## Next Steps

After successfully building MapMap:

1. **Run the demo:** `cargo run --release`
2. **Read the architecture docs:** [docs/ARCHITECTURE.md](docs/ARCHITECTURE.md)
3. **Explore the code:** Start with `crates/mapmap/src/main.rs`
4. **Check Phase 1 roadmap:** [RUST_REWRITE_PLAN.md](RUST_REWRITE_PLAN.md)

---

## Getting Help

- **Build issues:** Open an issue at https://github.com/johnjanik/mapmap/issues
- **Platform-specific problems:** Check [Troubleshooting](#troubleshooting) section
- **General questions:** See [README.md](README.md) and [RUST_REWRITE_PLAN.md](RUST_REWRITE_PLAN.md)

---

**Last Updated:** 2025-11-11
**MapMap Version:** 0.1.0 (Phase 0 - Foundation)
