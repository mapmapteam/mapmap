# Phase 4: Control Systems - Implementation Plan

## Overview

Phase 4 implements professional control surface integration for MapMap, enabling MIDI, OSC, DMX, and web-based control. This phase is critical for live performance workflows and remote control capabilities.

## Goals

**Primary Objectives:**
- ✅ MIDI input/output (note, CC, program change, clock)
- ✅ OSC server/client (TouchOSC, Lemur, custom apps)
- ✅ DMX output via Art-Net/sACN (lighting control)
- ✅ Web-based remote control interface (REST API + WebSocket)
- ✅ Cue/timeline system for automated shows
- ✅ Keyboard shortcuts and macro system

**Performance Targets:**
- <1ms MIDI latency
- <5ms OSC latency
- 30Hz DMX refresh rate
- 60fps web UI updates via WebSocket

## Architecture

```
┌─────────────────────────────────────────────────────────┐
│                    MapMap Application                    │
│                                                           │
│  ┌───────────────────────────────────────────────────┐  │
│  │           Control System Manager                   │  │
│  │  - Unified control target mapping                  │  │
│  │  - Event routing                                   │  │
│  │  - State synchronization                           │  │
│  └───────────────────────────────────────────────────┘  │
│           │         │         │         │                │
│           ▼         ▼         ▼         ▼                │
│  ┌──────┐ ┌──────┐ ┌──────┐ ┌──────┐ ┌──────┐          │
│  │ MIDI │ │ OSC  │ │ DMX  │ │ Web  │ │ Cue  │          │
│  │      │ │      │ │      │ │  API │ │System│          │
│  └──────┘ └──────┘ └──────┘ └──────┘ └──────┘          │
│     │         │         │         │         │            │
└─────┼─────────┼─────────┼─────────┼─────────┼──────────┘
      │         │         │         │         │
      ▼         ▼         ▼         ▼         ▼
  Hardware   Network   Network   HTTP     Internal
   Ports      UDP       UDP      TCP      Events
```

## Module Structure

```
crates/mapmap-control/src/
├── lib.rs                    # Public API and re-exports
├── error.rs                  # Error types
├── target.rs                 # Control target abstraction
├── manager.rs                # Unified control manager
│
├── midi/
│   ├── mod.rs               # MIDI module
│   ├── input.rs             # MIDI input handling
│   ├── output.rs            # MIDI output
│   ├── learn.rs             # MIDI learn mode
│   ├── mapping.rs           # MIDI message to control mapping
│   ├── profiles.rs          # Controller profiles (APC40, etc.)
│   └── clock.rs             # MIDI clock/sync
│
├── osc/
│   ├── mod.rs               # OSC module
│   ├── server.rs            # OSC server (receive messages)
│   ├── client.rs            # OSC client (send messages)
│   ├── address.rs           # OSC address space
│   └── types.rs             # Type conversion
│
├── dmx/
│   ├── mod.rs               # DMX module
│   ├── artnet.rs            # Art-Net protocol implementation
│   ├── sacn.rs              # sACN (E1.31) implementation
│   ├── channels.rs          # Channel assignment
│   └── fixtures.rs          # Fixture profiles
│
├── web/
│   ├── mod.rs               # Web API module
│   ├── server.rs            # HTTP server (Axum)
│   ├── routes.rs            # REST API routes
│   ├── websocket.rs         # WebSocket for real-time updates
│   ├── auth.rs              # Authentication
│   └── handlers.rs          # Request handlers
│
├── cue/
│   ├── mod.rs               # Cue system module
│   ├── cue.rs               # Cue definition
│   ├── cue_list.rs          # Cue list management
│   ├── crossfade.rs         # Crossfade engine
│   └── triggers.rs          # Cue triggers (MIDI/OSC/time)
│
└── shortcuts/
    ├── mod.rs               # Keyboard shortcuts module
    ├── shortcuts.rs         # Shortcut definitions
    ├── macros.rs            # Macro recorder
    └── bindings.rs          # Key binding management
```

## Implementation Details

### 1. Control Target System

The control target system provides a unified abstraction for all controllable parameters:

```rust
/// A controllable parameter in the application
#[derive(Debug, Clone)]
pub enum ControlTarget {
    /// Layer opacity (layer_id, opacity: 0.0-1.0)
    LayerOpacity(u32),
    /// Layer position (layer_id, x, y)
    LayerPosition(u32),
    /// Layer scale (layer_id, scale)
    LayerScale(u32),
    /// Layer rotation (layer_id, degrees)
    LayerRotation(u32),
    /// Paint parameter (paint_id, param_name, value)
    PaintParameter(u32, String),
    /// Effect parameter (effect_id, param_name, value)
    EffectParameter(u32, String),
    /// Playback speed (global or per-layer)
    PlaybackSpeed(Option<u32>),
    /// Output brightness (output_id, brightness: 0.0-1.0)
    OutputBrightness(u32),
    /// Custom parameter (name, value)
    Custom(String),
}

/// Control value types
#[derive(Debug, Clone)]
pub enum ControlValue {
    Float(f32),
    Int(i32),
    Bool(bool),
    String(String),
    Color(u32), // RGBA
}
```

### 2. MIDI System

**Dependencies:**
- `midir` - Cross-platform MIDI I/O

**Key Features:**
- Multiple MIDI input/output devices
- MIDI learn mode (click parameter, move controller)
- Controller profiles (predefined mappings for popular controllers)
- MIDI clock synchronization for tempo-based effects

**MIDI Message Types:**
```rust
pub enum MidiMessage {
    NoteOn { channel: u8, note: u8, velocity: u8 },
    NoteOff { channel: u8, note: u8 },
    ControlChange { channel: u8, controller: u8, value: u8 },
    ProgramChange { channel: u8, program: u8 },
    PitchBend { channel: u8, value: u16 },
    Clock,
    Start,
    Stop,
    Continue,
}
```

**Controller Profiles:**
- Generic MIDI controller
- Akai APC40/APC40 MKII
- Novation Launchpad
- Behringer BCF2000/BCR2000
- User-definable profiles (JSON format)

### 3. OSC System

**Dependencies:**
- `rosc` - OSC (Open Sound Control) protocol

**Key Features:**
- OSC server (receive messages)
- OSC client (send state updates)
- Bi-directional communication
- OSC query protocol support (for TouchOSC/Lemur)

**OSC Address Space:**
```
/mapmap/layer/{id}/opacity       [f32: 0.0-1.0]
/mapmap/layer/{id}/position      [f32, f32: x, y]
/mapmap/layer/{id}/rotation      [f32: degrees]
/mapmap/layer/{id}/scale         [f32: scale]
/mapmap/paint/{id}/parameter/{name}  [varies]
/mapmap/effect/{id}/parameter/{name} [varies]
/mapmap/playback/speed           [f32: speed multiplier]
/mapmap/playback/play            []
/mapmap/playback/pause           []
/mapmap/playback/stop            []
/mapmap/cue/goto                 [i32: cue number]
/mapmap/cue/next                 []
/mapmap/cue/prev                 []
```

**Example OSC Messages:**
```
# Set layer 0 opacity to 50%
/mapmap/layer/0/opacity 0.5

# Move layer 1 to position (100, 200)
/mapmap/layer/1/position 100.0 200.0

# Trigger cue 5
/mapmap/cue/goto 5
```

### 4. DMX Output

**Protocols:**
- Art-Net (UDP broadcast)
- sACN (E1.31 multicast)

**Key Features:**
- Multiple universe support (512 channels per universe)
- DMX channel assignment (map parameters to channels)
- Fixture profiles (generic dimmer, RGB par, RGBA, moving heads)
- DMX monitor/visualizer (debug view)

**Art-Net Implementation:**
```rust
pub struct ArtNetSender {
    socket: UdpSocket,
    universe: u16,
    sequence: u8,
}

impl ArtNetSender {
    pub fn send_dmx(&mut self, channels: &[u8; 512]) -> Result<()> {
        // Build Art-Net DMX packet
        let mut packet = vec![0u8; 18 + 512];
        packet[0..8].copy_from_slice(b"Art-Net\0");
        packet[8..10].copy_from_slice(&0x5000u16.to_le_bytes()); // OpDmx
        packet[10..12].copy_from_slice(&14u16.to_be_bytes());    // ProtVer
        packet[12] = self.sequence;
        packet[14..16].copy_from_slice(&self.universe.to_le_bytes());
        packet[16..18].copy_from_slice(&512u16.to_be_bytes());
        packet[18..].copy_from_slice(channels);

        self.socket.send_to(&packet, "255.255.255.255:6454")?;
        self.sequence = self.sequence.wrapping_add(1);
        Ok(())
    }
}
```

**Fixture Profiles:**
- Generic Dimmer (1 channel: dimmer)
- RGB Par (3 channels: R, G, B)
- RGBA Par (4 channels: R, G, B, A)
- RGBW Par (4 channels: R, G, B, W)
- Moving Head (8+ channels: pan, tilt, dimmer, color, gobo, etc.)

### 5. Web Control Interface

**Dependencies:**
- `axum` - Web framework
- `tower` - Middleware
- `tokio-tungstenite` - WebSocket support

**Key Features:**
- REST API for parameter control
- WebSocket for real-time state updates
- Mobile-responsive web UI
- Authentication (optional, API key or JWT)
- CORS support for external apps

**REST API Endpoints:**
```
GET    /api/status              # System status
GET    /api/layers              # List all layers
GET    /api/layers/:id          # Get layer details
PATCH  /api/layers/:id          # Update layer parameters
GET    /api/paints              # List all paints
GET    /api/effects             # List all effects
POST   /api/cues/:id/trigger    # Trigger a cue
GET    /ws                      # WebSocket connection
```

**WebSocket Messages:**
```json
// Client → Server: Set layer opacity
{
  "type": "set_parameter",
  "target": "layer_opacity",
  "layer_id": 0,
  "value": 0.75
}

// Server → Client: Parameter changed
{
  "type": "parameter_changed",
  "target": "layer_opacity",
  "layer_id": 0,
  "value": 0.75
}

// Server → Client: FPS update
{
  "type": "stats",
  "fps": 60.0,
  "frame_time_ms": 16.6
}
```

**Web UI Components:**
- Layer control panel
- Effect parameter editor
- Playback controls
- Cue list trigger
- Performance stats display

### 6. Cue System

**Key Features:**
- Cue list: snapshot entire project state
- Crossfade between cues (adjustable duration)
- Timeline: keyframe animation for parameters
- Looping and conditional cues
- MIDI/OSC cue triggers
- Auto-follow mode

**Cue Definition:**
```rust
#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct Cue {
    pub id: u32,
    pub name: String,
    pub description: String,

    // Parameter snapshots
    pub layer_states: HashMap<u32, LayerState>,
    pub paint_states: HashMap<u32, PaintState>,
    pub effect_states: HashMap<u32, EffectState>,

    // Transition settings
    pub fade_duration: Duration,
    pub fade_curve: FadeCurve,

    // Triggers
    pub auto_follow: Option<Duration>, // Auto-advance after duration
    pub midi_trigger: Option<MidiTrigger>,
    pub osc_trigger: Option<String>,
    pub time_trigger: Option<TimeOfDay>,
}

#[derive(Debug, Clone)]
pub enum FadeCurve {
    Linear,
    EaseIn,
    EaseOut,
    EaseInOut,
    Custom(Vec<(f32, f32)>), // Bezier control points
}
```

**Cue List Operations:**
```rust
pub struct CueList {
    cues: Vec<Cue>,
    current_cue: Option<u32>,
}

impl CueList {
    pub fn goto_cue(&mut self, id: u32, fade_duration: Option<Duration>);
    pub fn next_cue(&mut self);
    pub fn prev_cue(&mut self);
    pub fn trigger_cue(&mut self, id: u32);
    pub fn record_cue(&mut self, project: &Project) -> Cue;
}
```

### 7. Keyboard Shortcuts & Macros

**Key Features:**
- Global keyboard shortcut system
- User-definable key bindings
- Macro recorder (record sequence of actions)
- Context-sensitive shortcuts
- Import/export key bindings

**Shortcut System:**
```rust
#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct Shortcut {
    pub keys: Vec<Key>,
    pub modifiers: Modifiers,
    pub action: Action,
    pub context: ShortcutContext,
}

#[derive(Debug, Clone)]
pub enum Action {
    // Playback
    Play,
    Pause,
    Stop,
    TogglePlayPause,

    // Cues
    NextCue,
    PrevCue,
    GotoCue(u32),

    // Layers
    ToggleLayerVisibility(u32),
    SelectLayer(u32),

    // Macros
    ExecuteMacro(String),

    // Custom
    Custom(String),
}

#[derive(Debug, Clone)]
pub enum ShortcutContext {
    Global,        // Always active
    MainWindow,    // Only in main window
    OutputWindow,  // Only in output windows
    Editor,        // Only in specific editor
}
```

**Macro System:**
```rust
#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct Macro {
    pub name: String,
    pub actions: Vec<MacroAction>,
}

#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct MacroAction {
    pub action: Action,
    pub delay: Option<Duration>, // Wait before next action
}

impl Macro {
    pub fn record_start();
    pub fn record_stop() -> Macro;
    pub fn playback(&self);
}
```

## UI Integration

### ImGui Control Panels

**MIDI Panel:**
- Input device selector
- Output device selector
- MIDI learn button (toggle)
- Mapping table (MIDI → Parameter)
- Controller profile selector
- MIDI activity indicator

**OSC Panel:**
- Server enable/disable
- Port configuration
- OSC address space explorer
- Client output enable/disable
- Activity log

**DMX Panel:**
- Protocol selector (Art-Net / sACN)
- Universe configuration
- Channel assignment table
- Fixture library
- DMX monitor (channel values visualization)

**Web API Panel:**
- Server enable/disable
- Port configuration
- API key management
- Connection status (# of WebSocket clients)
- Recent API calls log

**Cue List Panel:**
- Cue list table
- Current cue indicator
- Next/Prev/Goto buttons
- Record cue button
- Crossfade duration slider
- Auto-follow toggle

**Shortcuts Panel:**
- Shortcut list
- Add/Edit/Delete shortcuts
- Macro recorder toggle
- Macro library
- Import/Export key bindings

## Testing Strategy

### Unit Tests
- MIDI message parsing
- OSC message encoding/decoding
- Art-Net packet construction
- sACN packet construction
- Control target mapping
- Cue state capture/restore

### Integration Tests
- MIDI learn workflow
- OSC server/client communication
- DMX output at target refresh rate
- Web API request/response
- WebSocket real-time updates
- Cue crossfade timing

### Performance Tests
- MIDI latency (target: <1ms)
- OSC latency (target: <5ms)
- DMX refresh rate (target: 30Hz)
- WebSocket update rate (target: 60fps)
- Control mapping overhead

## Dependencies

```toml
[dependencies]
# MIDI
midir = "0.9"

# OSC
rosc = "0.10"

# Web API
axum = "0.7"
tower = "0.4"
tokio = { version = "1.36", features = ["full"] }
tokio-tungstenite = "0.21"

# Serialization
serde = { version = "1.0", features = ["derive"] }
serde_json = "1.0"

# Error handling
thiserror = "1.0"
anyhow = "1.0"

# Logging
tracing = "0.1"
```

## Implementation Phases

### Week 1-2: MIDI System
1. MIDI input/output with `midir`
2. MIDI message parsing
3. MIDI learn mode
4. Controller profiles
5. MIDI clock sync
6. ImGui MIDI panel

### Week 3-4: OSC System
1. OSC server with `rosc`
2. OSC client
3. OSC address space
4. Bi-directional communication
5. OSC query protocol
6. ImGui OSC panel

### Week 5-6: DMX Output
1. Art-Net protocol implementation
2. sACN protocol implementation
3. Channel assignment system
4. Fixture profiles
5. DMX monitor/visualizer
6. ImGui DMX panel

### Week 7-8: Web Control Interface
1. Axum HTTP server setup
2. REST API routes
3. WebSocket implementation
4. Authentication
5. Web UI (HTML/CSS/JS)
6. ImGui Web API panel

### Week 9-11: Cue System
1. Cue definition and storage
2. Cue list management
3. Crossfade engine
4. Cue triggers (MIDI/OSC/time)
5. Auto-follow mode
6. ImGui Cue panel

### Week 12: Keyboard Shortcuts & Macros
1. Global shortcut system
2. User-definable key bindings
3. Macro recorder
4. Context-sensitive shortcuts
5. Import/export
6. ImGui Shortcuts panel

## Success Criteria

- ✅ MIDI learn functional for all parameters
- ✅ MIDI latency <1ms (measured)
- ✅ OSC control with <5ms latency
- ✅ DMX output at 30Hz (Art-Net)
- ✅ Web UI responsive on mobile devices
- ✅ WebSocket updates at 60fps
- ✅ Cue system with smooth crossfades
- ✅ All control systems have ImGui panels
- ✅ >85% test coverage
- ✅ Comprehensive documentation

## Future Enhancements (Post-Phase 4)

- **MIDI Output:** Send feedback to controllers (LED states)
- **DMX Input:** Receive DMX for control (via Art-Net)
- **OSC Templates:** Auto-generate TouchOSC/Lemur templates
- **Web UI Editor:** Full-featured web-based UI
- **Scripting:** Lua/Python bindings for custom control logic
- **Show Control:** Timecode (LTC/MTC) synchronization
- **Cloud Integration:** Remote control via cloud service

## References

- [MIDI 1.0 Specification](https://www.midi.org/specifications)
- [OSC 1.0 Specification](http://opensoundcontrol.org/spec-1_0)
- [Art-Net 4 Specification](https://art-net.org.uk/how-it-works/)
- [sACN (E1.31) Specification](https://tsp.esta.org/tsp/documents/published_docs.php)
- [Axum Web Framework](https://github.com/tokio-rs/axum)
- [midir Documentation](https://docs.rs/midir/)
- [rosc Documentation](https://docs.rs/rosc/)
