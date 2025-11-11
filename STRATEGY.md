# Premium Projection Mapping Suite - Strategic Assessment & Implementation Plan

**Project**: MapMap Fork → Professional Projection Mapping Suite
**Goal**: Create a premium open-source alternative to Resolume Arena
**Date**: 2025-11-10
**Status**: Initial Assessment

---

## Executive Summary

### Current State Assessment

MapMap provides a **solid proof-of-concept** for projection mapping with clean domain modeling but suffers from fundamental architectural limitations that prevent scaling to professional use:

**Critical Blockers:**
- ❌ Legacy OpenGL 1.x/2.x (immediate mode, no shaders)
- ❌ Single-threaded architecture (decode/render/UI all on main thread)
- ❌ No professional rendering pipeline (no FBOs, no compute, no modern effects)
- ❌ Missing 90% of required features (MIDI, DMX, NDI, edge blending, multi-output, etc.)

**Recommendation:** **Major rewrite required** with selective component reuse

**Estimated Effort:**
- Complete rewrite: 18-24 months (small team)
- Hybrid approach: 12-18 months (salvage domain model + file format)
- Extend current: 24+ months (not recommended - architectural debt)

---

## Gap Analysis: Requirements vs Current Capabilities

### 1. Core Real-Time Media Engine

| Requirement | MapMap Status | Gap Severity |
|------------|---------------|--------------|
| **Compositing Model** | | |
| Layer → Clip → Transform → Effects | ❌ No clips, basic layers | **CRITICAL** |
| Per-layer blend modes | ❌ Alpha only | **HIGH** |
| Masks | ❌ None | **HIGH** |
| Groups/subcomps | ❌ None | **MEDIUM** |
| **Effects & Generators** | | |
| GLSL/HLSL shaders | ❌ No shader support | **CRITICAL** |
| Node/chain system | ❌ None | **CRITICAL** |
| Parameter animation | ❌ No animation system | **HIGH** |
| Audio-reactive | ❌ No audio analysis | **HIGH** |
| 60/120 fps target | ⚠️ 60 fps theoretical, drops under load | **HIGH** |
| **Timing** | | |
| Internal clock | ✅ QTimer-based | **OK** |
| External sync (MIDI/LTC/SMPTE) | ❌ None | **CRITICAL** |
| Deterministic playback | ❌ No frame accuracy | **CRITICAL** |

**Verdict:** **0% Ready** - Complete rebuild required

### 2. Projection-Mapping Toolset

| Requirement | MapMap Status | Gap Severity |
|------------|---------------|--------------|
| **Surfaces/Outputs** | | |
| Logical surfaces | ⚠️ Mappings exist but no multi-output | **HIGH** |
| Surface → Output routing | ❌ Single window only | **CRITICAL** |
| **Geometry Warping** | | |
| 2D corner-pin | ✅ Quad with manual corners | **OK** |
| Bézier grids | ❌ Only linear mesh | **MEDIUM** |
| Free-form mesh | ✅ Mesh subdivision | **OK** |
| Perspective correction | ⚠️ Manual adjustment only | **MEDIUM** |
| Interactive handles | ✅ Vertex dragging in UI | **OK** |
| **Edge Blending** | | |
| Per-edge curves | ❌ None | **CRITICAL** |
| Gamma compensation | ❌ None | **CRITICAL** |
| Black-level compensation | ❌ None | **CRITICAL** |
| **Multi-Display Management** | | |
| GPU/display enumeration | ❌ Single window | **CRITICAL** |
| Refresh rate control | ❌ None | **HIGH** |
| Span outputs | ❌ None | **CRITICAL** |
| Bezel/overlap compensation | ❌ None | **CRITICAL** |
| **Calibration** | | |
| Projection masks | ❌ None | **HIGH** |
| Camera-based calibration | ❌ None | **MEDIUM** |
| Keystone presets | ❌ None | **MEDIUM** |
| **Resolution Independence** | ❌ Limited by desktop res | **HIGH** |

**Verdict:** **30% Ready** - Basic warping works; needs multi-output, blending, calibration

### 3. Control, Show, and Integration

| Requirement | MapMap Status | Gap Severity |
|------------|---------------|--------------|
| **Clip/Deck System** | | |
| Hierarchical organization | ❌ Flat layer list | **HIGH** |
| Instant triggering | ❌ No cue system | **CRITICAL** |
| Quantized launching | ❌ None | **CRITICAL** |
| Fallback media | ❌ None | **HIGH** |
| **Param Control** | | |
| MIDI | ❌ Not implemented | **CRITICAL** |
| OSC | ✅ Basic implementation | **OK** |
| DMX/Art-Net/sACN | ❌ Not implemented | **CRITICAL** |
| MIDI learn | ❌ None | **HIGH** |
| Per-param scaling | ⚠️ Hard-coded ranges | **MEDIUM** |
| **Lighting/Theatre Integration** | | |
| DMX in/out | ❌ None | **CRITICAL** |
| Art-Net | ❌ None | **CRITICAL** |
| CITP/MA-Net | ❌ None | **LOW** |
| **Audio I/O** | | |
| Multi-channel audio | ⚠️ GStreamer audio only | **HIGH** |
| Side-chain to FX | ❌ None | **HIGH** |
| FFT/spectrum analysis | ❌ None | **HIGH** |
| **Recording/Broadcast** | | |
| NDI in/out | ❌ None | **CRITICAL** |
| SDI/DeckLink | ❌ None | **CRITICAL** |
| Screen capture | ❌ None | **MEDIUM** |
| Timecode burn-in | ❌ None | **MEDIUM** |
| **Scripting/Automation** | | |
| Lua/Python/JS | ❌ None | **HIGH** |
| OSC+HTTP automation | ⚠️ OSC exists, no HTTP | **MEDIUM** |
| **Failover** | | |
| Safe mode | ❌ None | **HIGH** |
| Auto-retrigger | ❌ None | **HIGH** |
| GPU context loss recovery | ❌ None | **HIGH** |

**Verdict:** **5% Ready** - Only basic OSC exists; missing all critical control systems

### 4. Product Layer (Premium UX)

| Requirement | MapMap Status | Gap Severity |
|------------|---------------|--------------|
| **Cross-Platform UX** | ✅ Qt-based, works on Linux/macOS/Windows | **OK** |
| Visual scene graph | ⚠️ Layer list, no graph view | **MEDIUM** |
| Fast inspector | ✅ Qt Property Browser | **OK** |
| Resizable previews | ✅ Dockable UI | **OK** |
| Multi-monitor aware | ❌ Single window | **HIGH** |
| **Asset Management** | | |
| Thumbnails | ❌ None | **MEDIUM** |
| Proxies | ❌ None | **MEDIUM** |
| Media relinking | ❌ None | **HIGH** |
| Collect-and-save | ❌ None | **HIGH** |
| **Performance Tools** | | |
| GPU/CPU meters | ❌ None | **HIGH** |
| Frame-time display | ⚠️ Console log only | **HIGH** |
| Dropped-frame logs | ❌ None | **HIGH** |
| **Content Ecosystem** | | |
| Template patches | ❌ None | **MEDIUM** |
| Effects library | ❌ None | **HIGH** |
| Projector presets | ❌ None | **MEDIUM** |

**Verdict:** **40% Ready** - Qt UI is solid; needs performance tools, asset mgmt, content

---

## What Can Be Salvaged from MapMap?

### ✅ Keep & Extend

1. **Domain Model Concepts** (HIGH VALUE)
   - Paint/Mapping/Shape hierarchy is clean and extensible
   - Separation of content (Paint) from geometry (Shape) is correct
   - Can be ported to new architecture

2. **Project File Format** (MEDIUM VALUE)
   - XML structure is readable and versionable
   - Can extend with new features while maintaining compatibility
   - Consider JSON for new version (better tooling)

3. **OSC Control Structure** (MEDIUM VALUE)
   - Path-based addressing (`/mapmap/paint/... i`) is not standard, a better approach would be ('/mapmap/paint/0/...')
   - Can extend to full OSC API spec
   - Good foundation for control system

4. **UI Layout Pattern** (MEDIUM VALUE)
   - Dockable panels work well for operators
   - Source/Destination canvas split is intuitive
   - Property inspector pattern is standard

5. **GStreamer Integration Pattern** (LOW VALUE)
   - Appsink approach is correct
   - BUT: Needs complete rewrite for multi-threading
   - Useful as reference only

### ❌ Must Replace

1. **Rendering Engine** (CRITICAL)
   - OpenGL 1.x immediate mode → Modern Vulkan/Metal/DX12
   - No shaders → Full GLSL/HLSL pipeline
   - Single-threaded → Multi-threaded render engine

2. **Media Playback** (CRITICAL)
   - Add HAP codec support (fast GPU upload)
   - Add NDI input/output
   - Hardware-accelerated decoding (VA-API, NVDEC)

3. **Threading Architecture** (CRITICAL)
   - Separate decode/upload/render threads
   - Frame queue with timestamping
   - Lock-free or minimally-locked pipelines

4. **Output System** (CRITICAL)
   - Multi-output support (enumerate displays)
   - Independent render contexts per output
   - Exclusive fullscreen/DirectX Flip Model

---

## Strategic Options Analysis

### Option A: Complete Rewrite (RECOMMENDED)

**Approach:**
- New C++/Vulkan rendering engine (or Unreal/Unity as base)
- Keep MapMap domain concepts (Paint/Mapping/Surface)
- Import MapMap project files for migration path
- Build professional features from ground up

**Pros:**
- ✅ No technical debt
- ✅ Modern architecture from day one
- ✅ Can use best-in-class libraries (JUCE, ImGui, etc.)
- ✅ Full control over performance

**Cons:**
- ❌ 18-24 month timeline
- ❌ Requires strong C++/graphics expertise
- ❌ High initial investment

**Timeline:** 18-24 months (3-person team)

**Cost:** ~$500K-$800K (salary + infra)

---

### Option B: Hybrid Approach (VIABLE)

**Approach:**
- Fork MapMap repository
- Keep: Domain model, project format, UI shell
- Replace: Rendering backend, media engine, control systems
- Incremental migration (dual rendering paths during transition)

**Pros:**
- ✅ 30% head start (UI, project format)
- ✅ Can ship incremental updates
- ✅ Lower upfront risk

**Cons:**
- ⚠️ Still requires rewriting 70% of code
- ⚠️ Constrained by Qt/legacy decisions
- ⚠️ Technical debt from bridging old/new

**Timeline:** 12-18 months (3-person team)

**Cost:** ~$400K-$600K

---

### Option C: Extend Current (NOT RECOMMENDED)

**Approach:**
- Port MapMap to modern OpenGL (3.3+/4.5)
- Add threading to existing architecture
- Incrementally add features

**Pros:**
- ✅ Preserves existing work
- ✅ Community familiarity

**Cons:**
- ❌ Fighting architectural constraints constantly
- ❌ Qt OpenGL limitations (no compute shaders, etc.)
- ❌ Would still need to replace most code eventually
- ❌ 24+ months to reach parity with Option A

**Timeline:** 24+ months (likely never reaches premium quality)

**Cost:** $600K+ (inefficient use of resources)

---

## Recommended Architecture: Premium System

### Technology Stack

#### Core Engine
```
Language: C++17/20
Graphics API: Vulkan (primary) + Metal (macOS) + DX12 (Windows)
  - Abstraction layer: Custom or bgfx/Diligent Engine
  - Fallback: OpenGL 4.5 for old hardware

Media Decoding:
  - FFmpeg/GStreamer for general codecs
  - Custom HAP decoder (GPU texture upload)
  - Hardware accel: VA-API (Linux), VideoToolbox (macOS), DXVA (Windows)

Threading Model:
  - Decode thread pool (per video stream)
  - Upload thread (texture transfer to GPU)
  - Render thread (per output display)
  - Main thread (UI + control)
  - Lock-free queues (readerwriterqueue or similar)
```

#### Effects Pipeline
```
Shader Language: GLSL 4.5 / HLSL 5.0 / MSL
Node System: Custom graph or integrate Natron/Blender shader nodes
Parameters: Bezier/keyframe animation system
Audio: JUCE for FFT/spectrum analysis
```

#### Control Layer
```
MIDI: RtMidi or JUCE MIDI
OSC: oscpack or liblo
DMX: OLA (Open Lighting Architecture)
Art-Net: Custom or ArtnetLib
HTTP API: Crow/Drogon for REST + WebSocket
Scripting: Embedded Lua (sol2) or Python (pybind11)
```

#### UI Framework
```
Primary: ImGui (fast, GPU-accelerated, portable)
  - For performance-critical operator UI

Alternative: Qt 6 QML
  - For asset management / setup UI

Dual approach: ImGui for live operation, Qt for authoring
```

#### Video I/O
```
NDI: NDI SDK (free for open source)
SDI: Blackmagic DeckLink SDK
Spout/Syphon: Official SDKs
Screen Capture: Desktop Duplication API (Windows), SCKit (macOS)
```

### System Architecture

```
┌─────────────────────────────────────────────────────────────┐
│                        CONTROL LAYER                         │
│  ┌──────┐  ┌──────┐  ┌──────┐  ┌──────┐  ┌──────┐         │
│  │ MIDI │  │ OSC  │  │ DMX  │  │ HTTP │  │  UI  │         │
│  └──┬───┘  └──┬───┘  └──┬───┘  └──┬───┘  └──┬───┘         │
│     └─────────┴─────────┴─────────┴─────────┘              │
│                          │                                   │
│                    ┌─────▼─────┐                            │
│                    │  COMMAND  │                            │
│                    │   QUEUE   │                            │
│                    └─────┬─────┘                            │
└──────────────────────────┼──────────────────────────────────┘
                           │
┌──────────────────────────▼──────────────────────────────────┐
│                      ENGINE CORE                             │
│  ┌──────────────┐  ┌──────────────┐  ┌──────────────┐     │
│  │  CLIP DECK   │  │    LAYERS    │  │  SURFACES    │     │
│  │  MANAGER     │  │   MANAGER    │  │   MANAGER    │     │
│  └──────┬───────┘  └──────┬───────┘  └──────┬───────┘     │
│         │                  │                  │              │
│         └──────────────────┴──────────────────┘              │
│                            │                                 │
│                    ┌───────▼────────┐                       │
│                    │  RENDER GRAPH  │                       │
│                    │   SCHEDULER    │                       │
│                    └───────┬────────┘                       │
└────────────────────────────┼─────────────────────────────────┘
                             │
        ┌────────────────────┼────────────────────┐
        │                    │                    │
┌───────▼────────┐  ┌────────▼────────┐  ┌───────▼────────┐
│  MEDIA ENGINE  │  │  EFFECTS ENGINE │  │  OUTPUT ENGINE │
├────────────────┤  ├─────────────────┤  ├────────────────┤
│ • Decode Pool  │  │ • Shader Graph  │  │ • Multi-Output │
│ • Format Conv  │  │ • FX Chain      │  │ • Warping      │
│ • Upload Queue │  │ • LUTs/Color    │  │ • Blending     │
│ • Cache Mgr    │  │ • Audio React   │  │ • Sync/Genlock │
└────────────────┘  └─────────────────┘  └────────────────┘
        │                    │                    │
        └────────────────────┴────────────────────┘
                             │
                    ┌────────▼─────────┐
                    │   GPU RESOURCE   │
                    │     MANAGER      │
                    │ • Textures       │
                    │ • Buffers        │
                    │ • Pipelines      │
                    │ • Contexts       │
                    └──────────────────┘
```

### Threading Model

```
┌──────────────┐     ┌──────────────┐     ┌──────────────┐
│ DECODE #1    │     │ DECODE #2    │     │ DECODE #N    │
│ (Video 1)    │     │ (Video 2)    │     │ (Video N)    │
└──────┬───────┘     └──────┬───────┘     └──────┬───────┘
       │                    │                    │
       └────────────────────┴────────────────────┘
                            │
                   ┌────────▼─────────┐
                   │  UPLOAD THREAD   │
                   │ • Texture upload │
                   │ • PBO transfer   │
                   │ • Format convert │
                   └────────┬─────────┘
                            │
       ┌────────────────────┴────────────────────┐
       │                                         │
┌──────▼───────┐                         ┌──────▼───────┐
│ RENDER #1    │                         │ RENDER #2    │
│ (Output 1)   │                         │ (Output 2)   │
│ • Warp/Blend │                         │ • Warp/Blend │
│ • Effects    │                         │ • Effects    │
│ • Vsync lock │                         │ • Vsync lock │
└──────────────┘                         └──────────────┘

       ┌────────────────────┐
       │   MAIN THREAD      │
       │ • UI updates       │
       │ • Control input    │
       │ • Command dispatch │
       └────────────────────┘
```

### Data Structures

```cpp
// Core domain model (ported from MapMap concepts)

struct Clip {
    UID id;
    std::string name;
    MediaSource* source;  // Video, Image, Generator, NDI, etc.
    float inPoint, outPoint;
    bool loop;
};

struct Layer {
    UID id;
    Clip* clip;
    Transform transform;  // Position, rotation, scale
    EffectChain effects;
    BlendMode blendMode;
    float opacity;
    Mask* mask;
};

struct Surface {
    UID id;
    std::string name;
    Geometry* shape;      // Quad, Mesh, Bezier
    std::vector<Layer*> layers;
    Transform transform;
};

struct Output {
    UID id;
    Display* display;     // GPU + monitor info
    Resolution resolution;
    float refreshRate;
    EdgeBlendingProfile* blending;
    ColorProfile* colorSpace;
    std::vector<Surface*> surfaces;
};

struct Project {
    std::vector<Clip*> clips;
    std::vector<Surface*> surfaces;
    std::vector<Output*> outputs;
    ControlMappings mappings;
    Metadata metadata;
};
```

---

## Phased Implementation Roadmap

### Phase 0: Foundation (Months 1-3)

**Goal:** Establish core architecture and rendering engine

**Deliverables:**
- [ ] Project setup (CMake, CI/CD, testing framework)
- [ ] Modern rendering abstraction (Vulkan/Metal/DX12)
- [ ] Basic triangle/quad rendering with shaders
- [ ] Multi-threaded frame scheduler
- [ ] Texture upload pipeline (with PBOs)
- [ ] Simple FFmpeg-based video decode
- [ ] Basic windowing (single output)

**Team:** 2 engineers (1 graphics, 1 systems)

**Risk:** Rendering abstraction complexity

---

### Phase 1: Core Engine (Months 4-6)

**Goal:** Media playback and basic projection mapping

**Deliverables:**
- [ ] Video playback (H.264, HAP codec)
- [ ] Image support (PNG, JPEG, animated GIF)
- [ ] Quad geometry with corner-pin warping
- [ ] Mesh subdivision and free-form warping
- [ ] Layer compositing (opacity, alpha blend)
- [ ] Project file format (JSON-based)
- [ ] Import MapMap .mmp files (migration)

**Team:** 3 engineers

**Risk:** HAP codec performance tuning

---

### Phase 2: Professional Warping (Months 7-9)

**Goal:** Multi-output and edge blending

**Deliverables:**
- [ ] Multi-display enumeration and management
- [ ] Independent render contexts per output
- [ ] Edge blending (soft-edge curves)
- [ ] Gamma and black-level compensation
- [ ] Bezier curve warping
- [ ] Projection masks and calibration helpers
- [ ] Output resolution independence (render > display)

**Team:** 3 engineers (1 focus on calibration)

**Risk:** Multi-GPU synchronization

---

### Phase 3: Effects Pipeline (Months 10-12)

**Goal:** Shader-based effects and color grading

**Deliverables:**
- [ ] GLSL/HLSL shader loader
- [ ] Built-in effects library (blur, color correction, keying)
- [ ] Effect chain system (per-layer)
- [ ] LUT support (3D color grading)
- [ ] Parameter keyframe animation system
- [ ] Audio FFT/spectrum analysis
- [ ] Audio-reactive parameter modulation

**Team:** 3 engineers (1 shader specialist)

**Risk:** Shader performance on diverse GPUs

---

### Phase 4: Control Systems (Months 13-15)

**Goal:** MIDI, OSC, DMX integration

**Deliverables:**
- [ ] MIDI input/output (RtMidi)
- [ ] MIDI learn system
- [ ] OSC server (full API spec)
- [ ] DMX output (OLA integration)
- [ ] Art-Net output
- [ ] HTTP REST API + WebSocket
- [ ] Control mapping UI (MIDI/OSC → parameters)
- [ ] Cue system (snapshots, transitions)

**Team:** 2 engineers

**Risk:** Protocol interoperability testing

---

### Phase 5: Pro Media I/O (Months 16-18)

**Goal:** NDI, SDI, and broadcast features

**Deliverables:**
- [ ] NDI input (receive streams)
- [ ] NDI output (send program out)
- [ ] Spout/Syphon texture sharing
- [ ] Blackmagic DeckLink SDI I/O
- [ ] Screen capture (system desktop)
- [ ] Timecode support (LTC, SMPTE)
- [ ] Genlock/sync to external reference

**Team:** 2 engineers (1 A/V specialist)

**Risk:** SDI hardware availability for testing

---

### Phase 6: User Experience (Months 19-21)

**Goal:** Professional UI and workflow

**Deliverables:**
- [ ] ImGui-based operator UI
- [ ] Asset browser with thumbnails
- [ ] Timeline/cue editor
- [ ] Performance monitoring (GPU/CPU/frame time)
- [ ] Keyboard shortcut system
- [ ] Multi-monitor UI layout
- [ ] Media relinking and proxy system
- [ ] Collect-and-save for show transport

**Team:** 2 engineers (1 UI/UX)

**Risk:** UI performance with many assets

---

### Phase 7: Ecosystem & Polish (Months 22-24)

**Goal:** Ship-ready product with content

**Deliverables:**
- [ ] Effects template library
- [ ] Projector preset database
- [ ] Example projects and tutorials
- [ ] User manual and API documentation
- [ ] Plugin SDK (if architecture supports)
- [ ] Licensing system (if commercial)
- [ ] Auto-update mechanism
- [ ] Crash reporting and diagnostics
- [ ] Performance profiling tools
- [ ] Beta testing program (100+ users)

**Team:** 3 engineers + 1 tech writer

**Risk:** Content creation bandwidth

---

### Post-Launch: Maintenance & Features

**Ongoing:**
- [ ] Community support and bug fixes
- [ ] Additional effects and generators
- [ ] Support for new codecs (AV1, etc.)
- [ ] VR/AR output modes
- [ ] Cloud collaboration features
- [ ] Scripting API (Lua/Python)

---

## Technical Risk Analysis

### High-Risk Areas

1. **Multi-GPU Synchronization** (Phase 2)
   - Problem: Tearing between outputs, frame drops
   - Mitigation: Swap group extensions, compositor bypass, hardware genlock
   - Contingency: Single-GPU multi-output fallback

2. **Real-Time Determinism** (Phase 1-4)
   - Problem: Dropped frames under load, jitter
   - Mitigation: Lock-free queues, render budget enforcement, priority threads
   - Contingency: Automatic quality degradation

3. **Cross-Platform Rendering** (Phase 0)
   - Problem: Vulkan/Metal/DX12 abstraction complexity
   - Mitigation: Use battle-tested libs (bgfx, Diligent), comprehensive testing
   - Contingency: OpenGL 4.5 fallback for all platforms

4. **HAP Codec Performance** (Phase 1)
   - Problem: Slow texture upload, format conversion overhead
   - Mitigation: GPU-side decompression, PBO streaming, async upload
   - Contingency: Recommend HAP Q over HAP (less data)

5. **MIDI/DMX Protocol Bugs** (Phase 4)
   - Problem: Device incompatibilities, timing issues
   - Mitigation: Extensive hardware testing, community beta
   - Contingency: Generic profiles + user-editable mappings

### Medium-Risk Areas

- **Shader Compatibility**: Test on Intel/AMD/NVIDIA GPUs from past 5 years
- **FFmpeg API Changes**: Pin to stable release, maintain upgrade path
- **Qt/ImGui Integration**: Keep UI decoupled from engine for swapping
- **Project File Versioning**: Strict backward-compat testing

---

## Competitive Positioning

### Premium Features vs. Resolume Arena

| Feature | Resolume Arena | Our Target | Advantage |
|---------|----------------|------------|-----------|
| **Core** |
| Multi-layer compositing | ✅ Yes | ✅ Yes | Parity |
| GLSL shader FX | ✅ Yes | ✅ Yes | Parity |
| HAP codec | ✅ Yes | ✅ Yes | Parity |
| 4K/8K output | ✅ Yes | ✅ Yes | Parity |
| **Projection** |
| Advanced warping | ✅ Yes | ✅ Yes | Parity |
| Edge blending | ✅ Yes | ✅ Yes | Parity |
| Multi-projector | ✅ Yes | ✅ Yes | Parity |
| **Control** |
| MIDI/OSC | ✅ Yes | ✅ Yes | Parity |
| DMX output | ✅ Yes | ✅ Yes | Parity |
| Art-Net | ✅ Yes | ✅ Yes | Parity |
| **I/O** |
| NDI | ✅ Yes | ✅ Yes | Parity |
| Syphon/Spout | ✅ Yes | ✅ Yes | Parity |
| SDI (DeckLink) | ✅ Yes | ✅ Yes | Parity |
| **Differentiators** |
| Price | $699 | **Free/Open** | **ADVANTAGE** |
| Plugin SDK | Limited | **Full API** | **ADVANTAGE** |
| Scripting | Basic | **Python/Lua** | **ADVANTAGE** |
| Community mods | No | **Yes** | **ADVANTAGE** |
| Source available | No | **Yes** | **ADVANTAGE** |

**Strategy:**
- **Match** core functionality (must-haves)
- **Exceed** on openness, extensibility, price
- **Differentiate** on pro features (scripting, modding, integrations)

---

## Resource Requirements

### Team Composition

**Core Team (Full-time):**
- 1× Senior Graphics Engineer (Vulkan/shaders)
- 1× Senior Systems Engineer (threading/performance)
- 1× Media Engineer (codecs/A/V protocols)
- 1× UI/UX Engineer (ImGui/Qt)
- 1× DevOps/Build Engineer (CI/CD, testing)

**Part-time/Contract:**
- 1× Technical Writer (docs, tutorials)
- 1× QA Engineer (testing, bug triage)
- 1× Content Creator (effects, presets, demos)

**Advisory:**
- 1× Projection Mapping Professional (user feedback)
- 1× Lighting Designer (DMX/control workflows)

### Budget Estimate (24 months)

| Category | Cost |
|----------|------|
| **Salaries** (5 FTE × 24 mo × $10K/mo) | $1.2M |
| **Contract Work** (docs, QA, content) | $150K |
| **Infrastructure** (CI, servers, testing) | $50K |
| **Hardware** (GPUs, projectors, SDI cards) | $80K |
| **Software Licenses** (IDEs, NDI, etc.) | $20K |
| **Marketing/Launch** | $100K |
| **Buffer (20%)** | $320K |
| **TOTAL** | **~$1.92M** |

### Minimum Viable Product (MVP) Budget

**12-month MVP** (Phases 0-4):
- Team: 3 engineers
- Cost: ~$400K (salary + infra)
- Output: Core engine + projection mapping + basic control
- Goal: Usable for simple shows, attract community

---

## Recommendations

### Immediate Next Steps (Week 1-2)

1. **Decision Meeting**
   - Choose: Option A (rewrite) or Option B (hybrid)?
   - Commit to budget and timeline
   - Define MVP scope (12-month vs 24-month plan)

2. **Technical Prototype** (2 weeks)
   - Proof-of-concept: Vulkan + HAP video + quad warping
   - Validate rendering abstraction approach
   - Measure frame timing and GPU load

3. **Repository Setup**
   - Fork MapMap (keep as reference + migration path)
   - Create new repo: "MapMapPro" or "ProjectionKit"
   - Set up CI/CD (GitHub Actions, Docker builds)
   - Establish coding standards and review process

4. **Community Outreach**
   - Announce fork and goals on MapMap forums
   - Create Discord/Slack for contributors
   - Write RFC for architecture (get feedback)
   - Recruit 2-3 open-source contributors

### Success Metrics

**6-Month Check-in:**
- [ ] Render engine hits 60 fps with 4× 4K video layers
- [ ] Multi-output working on 2+ displays
- [ ] Import and render MapMap .mmp projects
- [ ] 5+ community contributors

**12-Month Check-in (MVP):**
- [ ] Feature parity with MapMap + multi-output + blending
- [ ] MIDI/OSC control working
- [ ] 50+ users testing in real shows
- [ ] Zero-crash runs for 8+ hour shows

**24-Month Check-in (v1.0):**
- [ ] 80% feature parity with Resolume Arena
- [ ] 500+ active users
- [ ] 3+ commercial installations
- [ ] Self-sustaining community (plugins, effects)

---

## Conclusion

**MapMap provides a valuable proof-of-concept** but requires a **major rewrite** to become a premium projection mapping suite. The existing domain model and project structure offer a 20-30% head start, but the rendering engine, media pipeline, and control systems must be built from scratch.

**Recommended Path:**
- **Option B (Hybrid Approach)** with aggressive replacement of core systems
- Keep: Domain concepts, project format (extended), UI shell
- Replace: Rendering (Vulkan), media (multi-threaded), control (full protocols)

**Investment Required:**
- **MVP (12 months):** $400K, 3 engineers → Usable for simple shows
- **Full Product (24 months):** $1.9M, 5 engineers → Resolume Arena competitor

**Risk Level:** MEDIUM
- High confidence in technical feasibility (proven tech stack)
- Medium confidence in timeline (depends on team experience)
- Low confidence in market risk (clear demand, no open-source competitor)

**Next Steps:**
1. Build 2-week prototype (Vulkan + HAP + warping)
2. Decide on budget commitment
3. Hire team or recruit open-source contributors
4. Launch Phase 0 (Foundation)

---

**End of Strategic Assessment**

*Document Version: 1.0*
*Last Updated: 2025-11-10*
*Author: Technical Assessment Team*
