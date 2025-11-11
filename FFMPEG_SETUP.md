# FFmpeg Setup for Video Playback

## Current Issue: Build Environment Isolation ⚠️

**You've installed FFmpeg packages, but `cargo build` still can't find them.**

This happens when your build environment is isolated from your system (common in containers, VMs, or some IDEs). The packages are installed on your system, but the Cargo build process runs in a different environment.

**Quick check:**
```bash
pkg-config --modversion libavutil
```

- ✅ **Shows version**: Packages are accessible, proceed to rebuild
- ❌ **"Package libavutil was not found"**: Environment isolation issue (see [Troubleshooting](#troubleshooting))

---

## Why You're Not Seeing Video Playback

You have the **FFmpeg runtime** installed (the `ffmpeg` command-line tool), but MapMap needs the **FFmpeg development libraries** to enable real video playback.

Currently, when you import an MP4 file:
- ✅ The file picker opens successfully
- ✅ You can select a video file
- ❌ The app falls back to test patterns (because it wasn't built with FFmpeg feature)

## Solution: Install FFmpeg Development Libraries

### Option 1: Quick Install (Recommended)

Run the provided helper script:

```bash
cd /home/user/mapmap
./install-ffmpeg-dev.sh
```

### Option 2: Manual Install

```bash
sudo apt-get update
sudo apt-get install -y \
    libavcodec-dev \
    libavformat-dev \
    libavutil-dev \
    libswscale-dev \
    libavdevice-dev \
    libavfilter-dev \
    pkg-config
```

## After Installing FFmpeg Dev Libraries

### Rebuild with FFmpeg Support

```bash
cd /home/user/mapmap

# Clean previous build
cargo clean

# Build with FFmpeg feature enabled
cargo build --release --features ffmpeg
```

This will take a few minutes as it compiles the FFmpeg bindings.

### Run with Video Playback Enabled

```bash
# Run the app with FFmpeg support
cargo run --release --features ffmpeg
```

Now when you:
1. Click **File > Load Video**
2. Select an MP4, MOV, or other video file
3. The video should **actually play** on screen!

## Verify It's Working

After building with `--features ffmpeg`, check the console output when loading a video:

**Success (with FFmpeg):**
```
INFO Loading video file: /path/to/video.mp4
INFO Video loaded: 1920x1080 @ 30.00 fps, duration: 120.50s
INFO Created paint 3 for video
INFO Created video player for paint 3
INFO Created mapping 4 for video
```

**Fallback (without FFmpeg):**
```
INFO Loading video file: /path/to/video.mp4
INFO FFmpeg feature not enabled, using test pattern
```

## What's the Difference?

| Component | Runtime FFmpeg | Development Libraries |
|-----------|---------------|----------------------|
| **Package** | `ffmpeg` | `libavcodec-dev`, `libavformat-dev`, etc. |
| **Contains** | Compiled binary executable | Header files (.h), pkg-config (.pc), link libraries |
| **Used by** | Command-line video processing | Compiling software that uses FFmpeg |
| **MapMap needs** | ❌ No (optional, for debug) | ✅ **YES** (required for video playback) |

## Check What You Have

```bash
# Check if runtime FFmpeg is installed
ffmpeg -version
# ✅ You have this

# Check if development libraries are installed
dpkg -l | grep libavcodec-dev
# ❌ You probably don't have this (yet)
```

## Troubleshooting

### "The system library 'libavutil' required by crate 'ffmpeg-sys-next' was not found"

This error means FFmpeg development libraries aren't installed **in the build environment**.

**Common causes:**

1. **Packages not installed**: Run the install script
2. **Build environment isolation**: Your `cargo build` may run in a container/sandbox separate from your system
3. **PKG_CONFIG_PATH not set**: The .pc files exist but pkg-config can't find them

**Solutions:**

**Check if packages are actually visible:**
```bash
# Check if pkg-config can find FFmpeg
pkg-config --modversion libavutil

# If this returns a version number, packages are accessible
# If it fails, packages need to be installed in the build environment
```

**For isolated build environments (common in some IDEs/containers):**

You may need to set PKG_CONFIG_PATH manually:
```bash
# Find where .pc files are installed
find /usr -name "libavutil.pc" 2>/dev/null

# If found at /usr/lib/x86_64-linux-gnu/pkgconfig/libavutil.pc:
export PKG_CONFIG_PATH=/usr/lib/x86_64-linux-gnu/pkgconfig
cargo build --release --features ffmpeg
```

**For truly isolated environments:**

If your build runs in a container/VM, install FFmpeg there:
```bash
# Enter your build environment first, then:
sudo apt-get install -y libavcodec-dev libavformat-dev libavutil-dev \
    libswscale-dev libavdevice-dev libavfilter-dev
```

### "Package libavcodec-dev has no installation candidate"

You may need to enable additional repositories:
```bash
sudo apt-get update
sudo apt-get install software-properties-common
sudo add-apt-repository universe
sudo apt-get update
```

### Build still falls back to test patterns

Make sure you're using the `--features ffmpeg` flag:
```bash
# Wrong (no FFmpeg)
cargo run --release

# Correct (with FFmpeg)
cargo run --release --features ffmpeg
```

## Performance Notes

**With FFmpeg:**
- Real video file support (.mp4, .mov, .avi, .mkv, .webm, etc.)
- Hardware acceleration available (VA-API on Linux)
- Automatic codec detection
- Proper video metadata (resolution, fps, duration)

**Without FFmpeg:**
- Only procedural test patterns
- File picker works but videos fall back to patterns
- Useful for development/testing without video files

## Summary

**To enable video playback:**

1. Install FFmpeg dev libraries: `./install-ffmpeg-dev.sh`
2. Rebuild: `cargo build --release --features ffmpeg`
3. Run: `cargo run --release --features ffmpeg`
4. Load video: **File > Load Video** → select MP4 → watch it play!
