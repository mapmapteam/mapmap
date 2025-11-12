# Phase 5: Professional Video I/O - Implementation Plan

## Overview

Phase 5 integrates professional video I/O protocols, enabling MapMap to interface with broadcast equipment, live cameras, network video streams, and other professional software. This phase transforms MapMap from a standalone tool into a fully-integrated component of professional video workflows.

## Goals

**Primary Objectives:**
- ✅ NDI receive/send (HD/4K network video streams)
- ✅ DeckLink SDI input/output (capture cards)
- ✅ Spout texture sharing (Windows)
- ✅ Syphon texture sharing (macOS)
- ✅ Stream output (RTMP/SRT)
- ✅ Virtual camera output (OBS integration)

**Performance Targets:**
- NDI: <2 frames latency
- DeckLink: <1 frame latency (genlock sync)
- Spout/Syphon: <1ms texture share
- Stream output: 1080p60 @ 6Mbps

## Architecture

```
┌─────────────────────────────────────────────────────────┐
│                    MapMap Render Pipeline                │
│                                                           │
│  ┌───────────────────────────────────────────────────┐  │
│  │              Video I/O Manager                     │  │
│  │  - Source registry                                 │  │
│  │  - Output routing                                  │  │
│  │  - Format conversion                               │  │
│  └───────────────────────────────────────────────────┘  │
│           │         │         │         │                │
│           ▼         ▼         ▼         ▼                │
│  ┌──────┐ ┌──────┐ ┌──────┐ ┌──────┐ ┌──────┐          │
│  │ NDI  │ │Deck  │ │Spout │ │Syphon│ │Stream│          │
│  │      │ │Link  │ │      │ │      │ │      │          │
│  └──────┘ └──────┘ └──────┘ └──────┘ └──────┘          │
│     │         │         │         │         │            │
└─────┼─────────┼─────────┼─────────┼─────────┼──────────┘
      │         │         │         │         │
      ▼         ▼         ▼         ▼         ▼
  Network    SDI/HDMI  Shared    IOSurface  RTMP/SRT
   Video     Hardware   Memory                Network
```

## Module Structure

```
crates/mapmap-io/src/
├── lib.rs                    # Public API and re-exports
├── error.rs                  # Error types
├── source.rs                 # Video source trait
├── sink.rs                   # Video sink trait
├── format.rs                 # Video format definitions
├── converter.rs              # Format conversion utilities
│
├── ndi/
│   ├── mod.rs               # NDI module
│   ├── discovery.rs         # NDI source discovery
│   ├── receiver.rs          # NDI receiver
│   ├── sender.rs            # NDI sender
│   ├── tally.rs             # Tally support
│   └── sys.rs               # FFI bindings (bindgen)
│
├── decklink/
│   ├── mod.rs               # DeckLink module
│   ├── input.rs             # SDI/HDMI input
│   ├── output.rs            # SDI/HDMI output
│   ├── genlock.rs           # Genlock synchronization
│   ├── timecode.rs          # Timecode support
│   └── sys.rs               # FFI bindings
│
├── spout/
│   ├── mod.rs               # Spout module (Windows only)
│   ├── sender.rs            # Spout sender
│   ├── receiver.rs          # Spout receiver
│   └── sys.rs               # FFI bindings
│
├── syphon/
│   ├── mod.rs               # Syphon module (macOS only)
│   ├── server.rs            # Syphon server
│   ├── client.rs            # Syphon client
│   └── sys.rs               # FFI bindings (Objective-C)
│
├── stream/
│   ├── mod.rs               # Streaming module
│   ├── rtmp.rs              # RTMP output
│   ├── srt.rs               # SRT output
│   ├── encoder.rs           # Video encoding
│   └── muxer.rs             # Stream muxing
│
└── virtual_camera/
    ├── mod.rs               # Virtual camera module
    ├── directshow.rs        # Windows DirectShow
    ├── coremedia.rs         # macOS CoreMediaIO
    └── v4l2.rs              # Linux V4L2 loopback
```

## Implementation Strategy

### Phase 5A: Foundation & Architecture (Week 1-2)

**Create Core Abstractions:**

```rust
/// Video source trait for all input types
pub trait VideoSource: Send {
    fn name(&self) -> &str;
    fn format(&self) -> VideoFormat;
    fn receive_frame(&mut self) -> Result<VideoFrame>;
    fn is_available(&self) -> bool;
}

/// Video sink trait for all output types
pub trait VideoSink: Send {
    fn name(&self) -> &str;
    fn format(&self) -> VideoFormat;
    fn send_frame(&mut self, frame: &VideoFrame) -> Result<()>;
}

/// Video format description
#[derive(Debug, Clone)]
pub struct VideoFormat {
    pub width: u32,
    pub height: u32,
    pub pixel_format: PixelFormat,
    pub frame_rate: f32,
}

/// Pixel format enumeration
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum PixelFormat {
    RGBA8,
    BGRA8,
    RGB8,
    YUV420P,
    YUV422P,
    UYVY,
    NV12,
}

/// Video frame data
pub struct VideoFrame {
    pub data: Vec<u8>,
    pub format: VideoFormat,
    pub timestamp: Duration,
    pub metadata: FrameMetadata,
}
```

**Feature Flags:**
```toml
[features]
default = []
ndi = ["ndi-sdk"]
decklink = ["decklink-sdk"]
spout = ["spout-sys"]  # Windows only
syphon = ["syphon-sys"]  # macOS only
stream = ["ffmpeg-next"]
virtual-camera = []
all-io = ["ndi", "decklink", "spout", "syphon", "stream", "virtual-camera"]
```

### Phase 5B: NDI Integration (Week 3-5)

**NDI SDK Setup:**
- Download NDI SDK from NewTek
- Use `bindgen` to generate Rust FFI bindings
- Create safe Rust wrapper

**Implementation:**

```rust
// crates/mapmap-io/src/ndi/receiver.rs
#[cfg(feature = "ndi")]
pub struct NdiReceiver {
    recv: NonNull<ndi_sys::NDIlib_recv_instance_t>,
    sources: Vec<NdiSource>,
    converter: FormatConverter,
}

#[cfg(feature = "ndi")]
impl NdiReceiver {
    pub fn new() -> Result<Self> {
        unsafe {
            // Initialize NDI
            if !ndi_sys::NDIlib_initialize() {
                return Err(IoError::NdiError("Failed to initialize NDI".into()));
            }

            // Create receiver
            let recv_create = ndi_sys::NDIlib_recv_create_v3_t {
                source_to_connect_to: std::ptr::null(),
                color_format: ndi_sys::NDIlib_recv_color_format_e::NDIlib_recv_color_format_BGRX_BGRA,
                bandwidth: ndi_sys::NDIlib_recv_bandwidth_e::NDIlib_recv_bandwidth_highest,
                allow_video_fields: false,
                name: CString::new("MapMap")?.as_ptr(),
            };

            let recv = ndi_sys::NDIlib_recv_create_v3(&recv_create);
            if recv.is_null() {
                return Err(IoError::NdiError("Failed to create receiver".into()));
            }

            Ok(Self {
                recv: NonNull::new_unchecked(recv),
                sources: Vec::new(),
                converter: FormatConverter::new(),
            })
        }
    }

    pub fn find_sources(&mut self, timeout_ms: u32) -> Result<Vec<NdiSource>> {
        unsafe {
            let find = ndi_sys::NDIlib_find_create_v2(std::ptr::null());
            if find.is_null() {
                return Err(IoError::NdiError("Failed to create finder".into()));
            }

            std::thread::sleep(Duration::from_millis(timeout_ms as u64));

            let mut num_sources = 0u32;
            let sources_ptr = ndi_sys::NDIlib_find_get_current_sources(find, &mut num_sources);

            let mut sources = Vec::new();
            for i in 0..num_sources {
                let source = sources_ptr.add(i as usize).read();
                sources.push(NdiSource {
                    name: CStr::from_ptr(source.p_ndi_name).to_string_lossy().into_owned(),
                    url: CStr::from_ptr(source.p_url_address).to_string_lossy().into_owned(),
                });
            }

            ndi_sys::NDIlib_find_destroy(find);
            Ok(sources)
        }
    }

    pub fn connect(&mut self, source: &NdiSource) -> Result<()> {
        unsafe {
            let ndi_source = ndi_sys::NDIlib_source_t {
                p_ndi_name: CString::new(source.name.clone())?.as_ptr(),
                p_url_address: CString::new(source.url.clone())?.as_ptr(),
            };

            ndi_sys::NDIlib_recv_connect(self.recv.as_ptr(), &ndi_source);
            Ok(())
        }
    }
}

#[cfg(feature = "ndi")]
impl VideoSource for NdiReceiver {
    fn receive_frame(&mut self) -> Result<VideoFrame> {
        unsafe {
            let mut video_frame: ndi_sys::NDIlib_video_frame_v2_t = std::mem::zeroed();

            let frame_type = ndi_sys::NDIlib_recv_capture_v2(
                self.recv.as_ptr(),
                &mut video_frame,
                std::ptr::null_mut(),
                std::ptr::null_mut(),
                1000,
            );

            if frame_type != ndi_sys::NDIlib_frame_type_e::NDIlib_frame_type_video {
                return Err(IoError::NoFrameAvailable);
            }

            // Convert from NDI format to RGBA
            let frame = self.converter.ndi_to_rgba(&video_frame)?;

            ndi_sys::NDIlib_recv_free_video_v2(self.recv.as_ptr(), &video_frame);

            Ok(frame)
        }
    }

    fn name(&self) -> &str {
        "NDI Receiver"
    }

    fn format(&self) -> VideoFormat {
        // Return current format
        VideoFormat {
            width: 1920,
            height: 1080,
            pixel_format: PixelFormat::RGBA8,
            frame_rate: 60.0,
        }
    }

    fn is_available(&self) -> bool {
        !self.recv.as_ptr().is_null()
    }
}
```

### Phase 5C: DeckLink SDI Integration (Week 6-8)

**DeckLink SDK Setup:**
- Use Blackmagic DeckLink SDK
- Platform-specific FFI (COM on Windows, Objective-C on macOS)
- Safe Rust wrapper with COM/Objective-C interop

**Challenges:**
- COM interface on Windows (use `windows-rs`)
- Objective-C on macOS (use `objc` crate)
- Genlock synchronization
- Multiple SDI standards (SD/HD/3G/6G/12G)

**Stub Implementation:**
```rust
#[cfg(all(feature = "decklink", target_os = "windows"))]
pub struct DeckLinkInput {
    device: ComPtr<IDeckLink>,
    input: ComPtr<IDeckLinkInput>,
}

#[cfg(all(feature = "decklink", target_os = "macos"))]
pub struct DeckLinkInput {
    device: *mut Object,  // Objective-C object
}
```

### Phase 5D: Texture Sharing (Week 9-10)

**Spout (Windows):**
- Uses DirectX 11 shared textures
- Integrate with wgpu's DX11 backend
- Sender and receiver support

```rust
#[cfg(all(feature = "spout", target_os = "windows"))]
pub struct SpoutSender {
    name: String,
    width: u32,
    height: u32,
    handle: HANDLE,  // D3D11 shared texture handle
}
```

**Syphon (macOS):**
- Uses IOSurface for texture sharing
- Integrate with wgpu's Metal backend
- Server and client support

```rust
#[cfg(all(feature = "syphon", target_os = "macos"))]
pub struct SyphonServer {
    server: *mut Object,  // NSObject
    context: id,          // OpenGL/Metal context
}
```

### Phase 5E: Stream Output (Week 11-12)

**RTMP/SRT via FFmpeg:**
- Use existing `ffmpeg-next` bindings
- H.264/H.265 hardware encoding
- Adaptive bitrate
- Connection retry logic

```rust
#[cfg(feature = "stream")]
pub struct RtmpStreamer {
    encoder: VideoEncoder,
    muxer: FormatMuxer,
    url: String,
}

impl RtmpStreamer {
    pub fn new(url: &str, format: VideoFormat, bitrate: u64) -> Result<Self> {
        let encoder = VideoEncoder::new(
            EncoderCodec::H264,
            format,
            bitrate,
            EncoderPreset::LowLatency,
        )?;

        let muxer = FormatMuxer::new("flv", url)?;

        Ok(Self {
            encoder,
            muxer,
            url: url.to_string(),
        })
    }
}

impl VideoSink for RtmpStreamer {
    fn send_frame(&mut self, frame: &VideoFrame) -> Result<()> {
        let packet = self.encoder.encode(frame)?;
        self.muxer.write_packet(&packet)?;
        Ok(())
    }
}
```

### Phase 5F: Virtual Camera (Week 13-14)

**Platform-Specific Implementations:**

**Windows (DirectShow):**
- Create DirectShow filter DLL
- Register as virtual camera device
- Push frames from MapMap

**macOS (CoreMediaIO):**
- Create DAL plugin
- Register with CoreMediaIO
- Appear as camera in applications

**Linux (V4L2 Loopback):**
- Use existing v4l2loopback kernel module
- Write frames to /dev/video device

## Dependencies

```toml
[dependencies]
# Core
thiserror = "1.0"
tracing = "0.1"

# NDI
ndi-sdk = { version = "5.0", optional = true }

# DeckLink
decklink-sdk = { path = "../decklink-sys", optional = true }
windows = { version = "0.52", features = ["Win32_System_Com"], optional = true }
objc = { version = "0.2", optional = true }

# Spout (Windows)
spout-sys = { path = "../spout-sys", optional = true }

# Syphon (macOS)
syphon-sys = { path = "../syphon-sys", optional = true }
cocoa = { version = "0.25", optional = true }
core-foundation = { version = "0.9", optional = true }

# Streaming
ffmpeg-next = { workspace = true, optional = true }

# Format conversion
image = "0.24"
bytemuck = { version = "1.14", features = ["derive"] }

[build-dependencies]
bindgen = "0.69"
cc = "1.0"
```

## Testing Strategy

### Unit Tests
- Format conversion accuracy
- Timestamp handling
- Error recovery
- Resource cleanup

### Integration Tests
- NDI discovery and connection
- DeckLink device enumeration
- Spout/Syphon texture sharing
- RTMP streaming to test server

### Performance Tests
- NDI latency measurement
- DeckLink frame timing
- Texture share overhead
- Encoding performance

## Platform Matrix

| Feature | Windows | macOS | Linux |
|---------|---------|-------|-------|
| NDI | ✅ | ✅ | ✅ |
| DeckLink | ✅ | ✅ | ✅ |
| Spout | ✅ | ❌ | ❌ |
| Syphon | ❌ | ✅ | ❌ |
| RTMP | ✅ | ✅ | ✅ |
| Virtual Camera | ✅ | ✅ | ✅ |

## Challenges & Solutions

### Challenge 1: Proprietary SDKs
**Problem:** NDI and DeckLink require proprietary SDKs that may not be redistributable.

**Solution:**
- Feature-gate all proprietary code
- Provide clear SDK installation instructions
- Create stub implementations for testing without SDKs
- Document SDK licensing requirements

### Challenge 2: Platform-Specific APIs
**Problem:** Spout/Syphon are platform-specific and require different GPU APIs.

**Solution:**
- Use conditional compilation (`#[cfg(target_os = "...")]`)
- Abstract texture sharing behind common trait
- Leverage wgpu's multi-backend support
- Provide fallback implementations

### Challenge 3: Real-Time Performance
**Problem:** Video I/O requires consistent low-latency performance.

**Solution:**
- Dedicated thread per video source
- Lock-free queues for frame passing
- Zero-copy where possible
- GPU-side format conversion
- Frame dropping when behind

### Challenge 4: Format Conversion
**Problem:** Different sources use different pixel formats and color spaces.

**Solution:**
- Centralized `FormatConverter` utility
- GPU-accelerated conversion shaders
- Support for common formats (YUV, RGB, etc.)
- Color space transformation (Rec.709, Rec.2020)

## Success Metrics

### Performance
- ✅ NDI receive/send <2 frames latency
- ✅ DeckLink SDI I/O <1 frame latency
- ✅ Spout/Syphon <1ms texture share
- ✅ RTMP stream 1080p60 @ 6Mbps
- ✅ Virtual camera 30fps minimum

### Reliability
- ✅ Automatic reconnection on network loss
- ✅ Graceful degradation when devices unavailable
- ✅ No frame drops under normal load
- ✅ Clean shutdown without resource leaks

### Compatibility
- ✅ NDI 5.x support
- ✅ DeckLink SDK 12.x support
- ✅ Spout 2.x support
- ✅ Syphon framework support
- ✅ OBS Studio virtual camera compatibility

## Future Enhancements (Post-Phase 5)

- **AJA Video Systems** support
- **Datapath capture cards** support
- **NewBlue NDI Bridge** integration
- **Dante audio** over IP
- **PTP/genlock** synchronization
- **HDR** video support (HDR10, HLG)
- **10-bit** video pipeline
- **Alpha channel** support throughout
- **Timecode** synchronization (LTC, MIDI clock, NTP)

## References

- [NDI SDK Documentation](https://www.ndi.tv/sdk/)
- [Blackmagic DeckLink SDK](https://www.blackmagicdesign.com/developer/)
- [Spout for Windows](https://spout.zeal.co/)
- [Syphon for macOS](http://syphon.v002.info/)
- [FFmpeg Documentation](https://ffmpeg.org/documentation.html)
- [OBS Virtual Camera](https://obsproject.com/forum/resources/obs-virtualcam.949/)

---

## Implementation Timeline

**Week 1-2:** Core architecture, traits, format conversion
**Week 3-5:** NDI integration (discovery, receive, send)
**Week 6-8:** DeckLink SDI (input, output, genlock)
**Week 9-10:** Texture sharing (Spout, Syphon)
**Week 11-12:** Stream output (RTMP, SRT, encoding)
**Week 13-14:** Virtual camera (DirectShow, DAL, V4L2)
**Week 15:** Integration testing and documentation
**Week 16:** Performance optimization and polish

**Total Duration:** 16 weeks (4 months)
