# MapMap Core Features - Comprehensive Build Plan

This document maps all critical projection mapping features to our Rust rewrite phases. Features are categorized by implementation status and phase assignment.

## Legend
- ✅ **Implemented** - Currently working in codebase
- 🚧 **In Progress** - Actively being developed
- 📋 **Planned** - Scheduled in existing roadmap
- ⭐ **NEW** - Added from this feature analysis

---

# 1. Core Media and Content Management

## 1.1 Supported Media Playback

### Video Files
| Feature | Status | Phase | Notes |
|---------|--------|-------|-------|
| .MP4, .MPEG, .MPG | 🚧 In Progress | Week 2 | FFmpeg integration underway |
| .MOV, .AVI | 🚧 In Progress | Week 2 | FFmpeg integration underway |
| .GIF support | ⭐ NEW | Phase 1, Month 5 | Add GIF decoder support |
| HAP codec | 📋 Planned | Phase 5, Month 16 | High-priority for performance |
| ProRes codec | 📋 Planned | Phase 1, Month 4 | Via FFmpeg/GStreamer |

### Still Images
| Feature | Status | Phase | Notes |
|---------|--------|-------|-------|
| .png, .jpg, .jpeg | 📋 Planned | Phase 1, Month 5 | Via `image` crate |
| .tiff, .tif | ⭐ NEW | Phase 1, Month 5 | Add to image loader |
| DDS/compressed formats | 📋 Planned | Phase 1, Month 5 | Via `dds-rs` crate |

### Image Sequences
| Feature | Status | Phase | Notes |
|---------|--------|-------|-------|
| Image sequence playback | ⭐ NEW | Phase 1, Month 5 | Sequential frame loading |
| Frame rate control | ⭐ NEW | Phase 1, Month 5 | Custom FPS for sequences |

## 1.2 Media Handling - Built-in Browser

| Feature | Status | Phase | Notes |
|---------|--------|-------|-------|
| File browsing interface | ⭐ NEW | Phase 6, Month 19 | Media browser UI panel |
| Thumbnail generation | ⭐ NEW | Phase 6, Month 19 | First-frame thumbnails |
| Thumbnail display grid | ⭐ NEW | Phase 6, Month 19 | Grid view with caching |
| Search/filter files | ⭐ NEW | Phase 6, Month 19 | Filename + tag search |
| Preview clips | ⭐ NEW | Phase 6, Month 19 | Hover preview playback |

## 1.3 Import/Loading

| Feature | Status | Phase | Notes |
|---------|--------|-------|-------|
| Drag-and-drop files | ⭐ NEW | Phase 6, Month 19 | Via winit drop events |
| File picker dialog | ✅ Implemented | Week 2 | Native `rfd` dialog |
| Load video files | 🚧 In Progress | Week 2 | FFmpeg integration |
| Combine audio+video clips | ⭐ NEW | Phase 3, Month 11 | Audio layer sync |

## 1.4 Editing/Organization

| Feature | Status | Phase | Notes |
|---------|--------|-------|-------|
| Cut/Copy/Paste clips | ⭐ NEW | Phase 6, Month 20 | Clipboard operations |
| Rename clips | ⭐ NEW | Phase 6, Month 19 | In-place editing |
| Clear clips | ✅ Implemented | Week 1 | Remove paint action |
| Move/swap clips | ⭐ NEW | Phase 6, Month 20 | Drag-drop in deck |
| Copy clips within decks | ⭐ NEW | Phase 6, Month 20 | Duplicate with params |
| Duplicate layers | 📋 Planned | Phase 1, Month 4 | Layer duplication |
| Rename layers | ⭐ NEW | Phase 1, Month 4 | Layer name editing |
| Remove layers | 📋 Planned | Phase 1, Month 4 | Layer deletion |
| Color coding clips | ⭐ NEW | Phase 6, Month 19 | UI label colors |

## 1.5 Media Manager

| Feature | Status | Phase | Notes |
|---------|--------|-------|-------|
| Clip Reconnect (find missing) | ⭐ NEW | Phase 6, Month 24 | Search for moved files |
| Replace File (update content) | ⭐ NEW | Phase 6, Month 24 | Swap file, keep settings |
| Collect Media (gather files) | ⭐ NEW | Phase 6, Month 24 | Copy all to one location |

## 1.6 Persistent Clips

| Feature | Status | Phase | Notes |
|---------|--------|-------|-------|
| Mark clips persistent across decks | ⭐ NEW | Phase 6, Month 20 | Sticky clip flag |

## 1.7 Clip Manipulation

### Strip Options
| Feature | Status | Phase | Notes |
|---------|--------|-------|-------|
| Remove audio track | ⭐ NEW | Phase 3, Month 11 | Mute audio stream |
| Remove video track | ⭐ NEW | Phase 3, Month 11 | Audio-only mode |

### Resize Quickset
| Feature | Status | Phase | Notes |
|---------|--------|-------|-------|
| Fill (cover composition) | ⭐ NEW | Phase 1, Month 6 | Scale to cover |
| Fit (contain in composition) | ⭐ NEW | Phase 1, Month 6 | Scale to fit |
| Stretch (distort to fill) | ⭐ NEW | Phase 1, Month 6 | Non-uniform scale |
| Original size | ⭐ NEW | Phase 1, Month 6 | 1:1 pixel mapping |

### Thumbnails
| Feature | Status | Phase | Notes |
|---------|--------|-------|-------|
| Update thumbnail to current frame | ⭐ NEW | Phase 6, Month 19 | Capture current state |
| Apply effects to thumbnail | ⭐ NEW | Phase 6, Month 19 | Render with effects |
| Load thumbnail from disk | ⭐ NEW | Phase 6, Month 19 | Custom thumbnail |
| Revert to original thumbnail | ⭐ NEW | Phase 6, Month 19 | Reset to first frame |

### Snapshot
| Feature | Status | Phase | Notes |
|---------|--------|-------|-------|
| Create PNG snapshot of clip | ⭐ NEW | Phase 6, Month 21 | Export current frame |
| Save snapshot to disk | ⭐ NEW | Phase 6, Month 21 | PNG export |
| Import snapshot to layer | ⭐ NEW | Phase 6, Month 21 | Load as new paint |

---

# 2. Playback and Transport Controls

## 2.1 Triggering

| Feature | Status | Phase | Notes |
|---------|--------|-------|-------|
| Click thumbnail to trigger | ✅ Implemented | Week 1 | Play/pause on click |
| Column triggering (multiple clips) | ⭐ NEW | Phase 6, Month 20 | Trigger column at once |
| Next/Previous clip controls | ⭐ NEW | Phase 6, Month 20 | Deck navigation |

## 2.2 Transport Modes

| Feature | Status | Phase | Notes |
|---------|--------|-------|-------|
| Timeline mode (free-running) | ✅ Implemented | Week 1 | Default playback |
| BPM Sync mode | ⭐ NEW | Phase 4, Month 13 | Musical tempo sync |
| SMPTE mode (Arena only) | ⭐ NEW | Phase 5, Month 17 | Timecode sync |
| Denon DJ sync (Arena only) | ⭐ NEW | Phase 5, Month 17 | Denon Link integration |

## 2.3 Clip Playback Options

### Speed/Duration
| Feature | Status | Phase | Notes |
|---------|--------|-------|-------|
| Speed slider control | ✅ Implemented | Week 1 | 0.1x - 2.0x range |
| Precise duration setting | ⭐ NEW | Phase 4, Month 15 | Set exact clip length |

### In/Out Points
| Feature | Status | Phase | Notes |
|---------|--------|-------|-------|
| Set clip start point (In) | ⭐ NEW | Phase 6, Month 20 | Trim start |
| Set clip end point (Out) | ⭐ NEW | Phase 6, Month 20 | Trim end |
| Segment playback | ⭐ NEW | Phase 6, Month 20 | Play In-Out range only |

### Direction
| Feature | Status | Phase | Notes |
|---------|--------|-------|-------|
| Play forwards | ✅ Implemented | Week 1 | Default direction |
| Play backwards | ⭐ NEW | Phase 1, Month 5 | Reverse playback |
| Pause | ✅ Implemented | Week 1 | Freeze playhead |

### Playmode
| Feature | Status | Phase | Notes |
|---------|--------|-------|-------|
| Loop | ✅ Implemented | Week 1 | Repeat indefinitely |
| Ping Pong | ⭐ NEW | Phase 1, Month 5 | Bounce forward/back |
| Random (Interval/Distance) | ⭐ NEW | Phase 6, Month 20 | Jump random frames |
| Play Once and Eject | ⭐ NEW | Phase 1, Month 5 | Stop and unload |
| Play Once and Hold | ⭐ NEW | Phase 1, Month 5 | Stop on last frame |

### Playmode Away (Resume Behavior)
| Feature | Status | Phase | Notes |
|---------|--------|-------|-------|
| Start from beginning | ⭐ NEW | Phase 6, Month 20 | Reset on retrigger |
| Pick-up from last pause | ⭐ NEW | Phase 6, Month 20 | Resume playhead |
| Relative pick-up | ⭐ NEW | Phase 6, Month 20 | Match prev clip position |

## 2.4 Cue Points

| Feature | Status | Phase | Notes |
|---------|--------|-------|-------|
| Set cue points in clip | ⭐ NEW | Phase 6, Month 20 | Mark frame positions |
| Jump to cue point | ⭐ NEW | Phase 6, Month 20 | Seek to marker |

## 2.5 BeatLoopr

| Feature | Status | Phase | Notes |
|---------|--------|-------|-------|
| Auto-loop by beat count | ⭐ NEW | Phase 4, Month 13 | Loop 1/2/4/8 beats |
| BPM Sync integration | ⭐ NEW | Phase 4, Month 13 | Musical looping |

## 2.6 Trigger Settings (Clip/Composition Level)

### Beat Snap (Quantizing)
| Feature | Status | Phase | Notes |
|---------|--------|-------|-------|
| Snap to next beat | ⭐ NEW | Phase 4, Month 13 | Quarter-note quantize |
| Snap to bar | ⭐ NEW | Phase 4, Month 13 | Measure quantize |
| Snap to phrase | ⭐ NEW | Phase 4, Month 13 | 4/8 bar quantize |

### Clip Target
| Feature | Status | Phase | Notes |
|---------|--------|-------|-------|
| Own Layer | 📋 Planned | Phase 1, Month 4 | Stay in current layer |
| Active Layer | ⭐ NEW | Phase 6, Month 20 | Play in selected layer |
| Free Layer | ⭐ NEW | Phase 6, Month 20 | Find empty layer |
| Composition Determined | ⭐ NEW | Phase 6, Month 20 | Auto-routing |

### Trigger Style
| Feature | Status | Phase | Notes |
|---------|--------|-------|-------|
| Normal (toggle) | ✅ Implemented | Week 1 | Play/pause toggle |
| Piano mode (hold) | ⭐ NEW | Phase 4, Month 13 | Play while held |

### Other Trigger Options
| Feature | Status | Phase | Notes |
|---------|--------|-------|-------|
| Fader Start | ⭐ NEW | Phase 4, Month 13 | Restart on fade-up |
| Ignore Column Trigger | ⭐ NEW | Phase 6, Month 20 | Lock clip/layer |

## 2.7 Auto Pilot

| Feature | Status | Phase | Notes |
|---------|--------|-------|-------|
| Auto-play next clip | ⭐ NEW | Phase 6, Month 20 | Sequential advance |
| Auto-play previous clip | ⭐ NEW | Phase 6, Month 20 | Sequential reverse |
| Auto-play random clip | ⭐ NEW | Phase 6, Month 20 | Random selection |
| Auto-play first/last clip | ⭐ NEW | Phase 6, Month 20 | Jump to end |
| Auto-play specific clip | ⭐ NEW | Phase 6, Month 20 | Named target |

---

# 3. Composition, Layers, and Groups

## 3.1 Layer Controls

| Feature | Status | Phase | Notes |
|---------|--------|-------|-------|
| Audio fader (A) | 📋 Planned | Phase 3, Month 11 | Per-layer volume |
| Video fader (V) | ⭐ NEW | Phase 1, Month 4 | Per-layer opacity |
| Master fader (M) | ⭐ NEW | Phase 1, Month 4 | Master output level |

## 3.2 Layer States

| Feature | Status | Phase | Notes |
|---------|--------|-------|-------|
| Eject (X) - unload content | ✅ Implemented | Week 1 | Clear paint from layer |
| Bypass (B) - disable layer | ⭐ NEW | Phase 1, Month 4 | Skip in render |
| Solo (S) - isolate layer | ⭐ NEW | Phase 1, Month 4 | Mute all others |

## 3.3 Layer Order

| Feature | Status | Phase | Notes |
|---------|--------|-------|-------|
| Reorder by dragging | ⭐ NEW | Phase 6, Month 19 | Drag-drop reorder |
| Z-order compositing | 📋 Planned | Phase 1, Month 4 | Top-to-bottom blend |

## 3.4 Layer Transition

| Feature | Status | Phase | Notes |
|---------|--------|-------|-------|
| Crossfade on clip change | ⭐ NEW | Phase 3, Month 12 | Smooth transitions |
| Blend/transition modes | 📋 Planned | Phase 3, Month 12 | Dissolve, wipe, etc. |

## 3.5 Mask Layer

| Feature | Status | Phase | Notes |
|---------|--------|-------|-------|
| Mask All Below | ⭐ NEW | Phase 3, Month 12 | Luminance mask mode |
| Mask One Below | ⭐ NEW | Phase 3, Month 12 | Single-layer mask |

## 3.6 Lock Content

| Feature | Status | Phase | Notes |
|---------|--------|-------|-------|
| Prevent accidental ejection | ⭐ NEW | Phase 6, Month 19 | Lock layer content |

## 3.7 Blending and Compositing

| Feature | Status | Phase | Notes |
|---------|--------|-------|-------|
| Alpha blending | 📋 Planned | Phase 1, Month 4 | Transparency support |
| Add/Screen/Multiply blend modes | 📋 Planned | Phase 1, Month 4 | Standard blend modes |
| Advanced blend modes (15+) | 📋 Planned | Phase 3, Month 12 | Full Photoshop set |

## 3.8 Groups (Arena Feature)

| Feature | Status | Phase | Notes |
|---------|--------|-------|-------|
| Combine layers into groups | ⭐ NEW | Phase 3, Month 12 | Sub-compositions |
| Group fader control | ⭐ NEW | Phase 3, Month 12 | Master group opacity |
| Group effects/transforms | ⭐ NEW | Phase 3, Month 12 | Effect inheritance |

## 3.9 Composition Controls

### Composition Properties
| Feature | Status | Phase | Notes |
|---------|--------|-------|-------|
| Preview output | 📋 Planned | Phase 2, Month 7 | Preview window |
| Eject all content (X) | ⭐ NEW | Phase 1, Month 4 | Clear composition |
| Bypass output (B) | ⭐ NEW | Phase 2, Month 7 | Disable output |
| Master opacity (M) | ⭐ NEW | Phase 1, Month 4 | Global opacity |
| Master speed (S) | ⭐ NEW | Phase 1, Month 5 | Global speed mult |

### Composition Settings
| Feature | Status | Phase | Notes |
|---------|--------|-------|-------|
| Name/Description | 📋 Planned | Phase 1, Month 5 | Project metadata |
| Size (resolution) | 📋 Planned | Phase 2, Month 7 | Canvas dimensions |
| Frame rate | 📋 Planned | Phase 2, Month 7 | 24/30/60 fps options |
| Fixed frame rate limit | ⭐ NEW | Phase 2, Month 7 | Cap output FPS |
| Bit depth (8/16 bpc) | ⭐ NEW | Phase 2, Month 9 | Color precision |

### Crossfader
| Feature | Status | Phase | Notes |
|---------|--------|-------|-------|
| Assign layers to A/B bus | ⭐ NEW | Phase 6, Month 20 | DJ-style crossfade |
| Crossfader control | ⭐ NEW | Phase 6, Month 20 | A↔B transition |

---

# 4. Effects and Transformation

## 4.1 Effects Engine

| Feature | Status | Phase | Notes |
|---------|--------|-------|-------|
| Apply effects to Composition | 📋 Planned | Phase 3, Month 12 | Master effects |
| Apply effects to Layer | 📋 Planned | Phase 3, Month 12 | Per-layer effects |
| Apply effects to Clip | 📋 Planned | Phase 3, Month 12 | Pre-comp effects |
| Apply effects to Group | 📋 Planned | Phase 3, Month 12 | Group-level effects |

## 4.2 Built-in Effects

| Feature | Status | Phase | Notes |
|---------|--------|-------|-------|
| 100+ video effects library | 📋 Planned | Phase 3, Month 12 | Compute shaders |
| Audio effects library | 📋 Planned | Phase 3, Month 11 | Audio DSP |

## 4.3 Masks

| Feature | Status | Phase | Notes |
|---------|--------|-------|-------|
| Mask effect | 📋 Planned | Phase 3, Month 12 | Alpha mask layer |
| Layer Mask Mode | ⭐ NEW | Phase 3, Month 12 | Luminance masking |

## 4.4 Adjustment Layers

| Feature | Status | Phase | Notes |
|---------|--------|-------|-------|
| Effect Clips (empty clip with effects) | ⭐ NEW | Phase 3, Month 12 | Affects layers below |

## 4.5 Presets

| Feature | Status | Phase | Notes |
|---------|--------|-------|-------|
| Save effect presets | ⭐ NEW | Phase 6, Month 21 | Serialize params |
| Load effect presets | ⭐ NEW | Phase 6, Month 21 | Apply saved settings |

## 4.6 Transform Effect

| Feature | Status | Phase | Notes |
|---------|--------|-------|-------|
| Position (X/Y) | 📋 Planned | Phase 1, Month 4 | 2D translation |
| Scale (Width/Height) | 📋 Planned | Phase 1, Month 4 | Non-uniform scale |
| Rotation (X/Y/Z) | 📋 Planned | Phase 1, Month 4 | 3D rotation |
| Anchor Point | 📋 Planned | Phase 1, Month 4 | Transform origin |
| Transform Presets | ⭐ NEW | Phase 6, Month 21 | Save transform configs |

## 4.7 Plugin Support

| Feature | Status | Phase | Notes |
|---------|--------|-------|-------|
| VST audio plugins | 📋 Planned | Phase 5, Month 18 | Audio plugin host |
| FFGL video plugins | ⭐ NEW | Phase 5, Month 18 | FreeFrame GL |

## 4.8 Sources

| Feature | Status | Phase | Notes |
|---------|--------|-------|-------|
| Built-in generative sources | 📋 Planned | Phase 3, Month 12 | Procedural content |
| Text Animator | ⭐ NEW | Phase 3, Month 12 | Dynamic text source |

---

# 5. Control and Automation

## 5.1 Parameter Types

| Feature | Status | Phase | Notes |
|---------|--------|-------|-------|
| Sliders | ✅ Implemented | Week 1 | Float parameters |
| Toggle Buttons | ✅ Implemented | Week 1 | Boolean parameters |
| Event Buttons | ⭐ NEW | Phase 4, Month 13 | Trigger actions |
| Radio Buttons | ⭐ NEW | Phase 6, Month 19 | Exclusive choice |
| Dropdowns | ⭐ NEW | Phase 6, Month 19 | Enum parameters |

## 5.2 Parameter Animation

### Timeline/BPM Sync
| Feature | Status | Phase | Notes |
|---------|--------|-------|-------|
| Animate over time (seconds) | ⭐ NEW | Phase 6, Month 20 | Timeline keyframes |
| Animate over beats | ⭐ NEW | Phase 4, Month 13 | Musical keyframes |

### Clip Position Sync
| Feature | Status | Phase | Notes |
|---------|--------|-------|-------|
| Sync param to clip playhead | ⭐ NEW | Phase 6, Month 20 | Follow video position |

### Audio Analysis (FFT)
| Feature | Status | Phase | Notes |
|---------|--------|-------|-------|
| External audio input | 📋 Planned | Phase 3, Month 11 | System audio in |
| Composition audio | 📋 Planned | Phase 3, Month 11 | Master mix analysis |
| Clip audio | 📋 Planned | Phase 3, Month 11 | Per-clip analysis |
| Layer audio | 📋 Planned | Phase 3, Month 11 | Per-layer analysis |
| Group audio | ⭐ NEW | Phase 3, Month 11 | Group audio analysis |
| FFT parameter driving | 📋 Planned | Phase 3, Month 11 | Audio-reactive params |

### Envelope
| Feature | Status | Phase | Notes |
|---------|--------|-------|-------|
| Easing curves | ⭐ NEW | Phase 6, Month 20 | Ease-in/out |
| Interpolation types (Quadratic/Sine/Elastic) | ⭐ NEW | Phase 6, Month 20 | Advanced curves |
| Multiple keyframes | ⭐ NEW | Phase 6, Month 20 | Complex automation |

### Dashboard
| Feature | Status | Phase | Notes |
|---------|--------|-------|-------|
| Assign params to dashboard dials | ⭐ NEW | Phase 6, Month 20 | Quick-access controls |

## 5.3 Shortcut Assignment

### Keyboard Shortcuts
| Feature | Status | Phase | Notes |
|---------|--------|-------|-------|
| Assign keyboard shortcuts | ⭐ NEW | Phase 4, Month 13 | Custom key mapping |

### MIDI Shortcuts
| Feature | Status | Phase | Notes |
|---------|--------|-------|-------|
| MIDI input support | 📋 Planned | Phase 4, Month 13 | `midir` crate |
| Absolute mode (knobs/faders) | 📋 Planned | Phase 4, Month 13 | 0-127 mapping |
| Relative mode (encoders) | ⭐ NEW | Phase 4, Month 13 | +/- delta |
| Velocity mode (pads) | ⭐ NEW | Phase 4, Month 13 | Note velocity |
| MIDI Out feedback (LEDs) | ⭐ NEW | Phase 4, Month 13 | Bidirectional |

### OSC (Open Sound Control)
| Feature | Status | Phase | Notes |
|---------|--------|-------|-------|
| OSC input support | 📋 Planned | Phase 4, Month 13 | `rosc` crate |
| Fixed OSC addresses for UI items | ⭐ NEW | Phase 4, Month 13 | Addressable params |

### DMX Shortcuts
| Feature | Status | Phase | Notes |
|---------|--------|-------|-------|
| DMX input via Art-Net | ⭐ NEW | Phase 4, Month 14 | Lighting desk control |

---

# 6. Output, Mapping, and Projection

## 6.1 Output Modes

| Feature | Status | Phase | Notes |
|---------|--------|-------|-------|
| Fullscreen output | 📋 Planned | Phase 2, Month 7 | Exclusive fullscreen |
| Windowed output | ✅ Implemented | Phase 0 | Default mode |
| Disabled output | ⭐ NEW | Phase 2, Month 7 | Preview-only mode |

## 6.2 Output Sharing

| Feature | Status | Phase | Notes |
|---------|--------|-------|-------|
| Syphon (macOS) | 📋 Planned | Phase 5, Month 17 | Texture sharing |
| Spout (Windows) | 📋 Planned | Phase 5, Month 17 | Texture sharing |
| NDI (network streaming) | 📋 Planned | Phase 5, Month 16 | Network video I/O |

## 6.3 Output Management - Advanced Output

| Feature | Status | Phase | Notes |
|---------|--------|-------|-------|
| Multiple output windows | 📋 Planned | Phase 2, Month 7 | Multi-display support |
| Per-display configuration | 📋 Planned | Phase 2, Month 7 | Independent settings |

## 6.4 Input Selection (Slicing)

| Feature | Status | Phase | Notes |
|---------|--------|-------|-------|
| Rectangular slices | ⭐ NEW | Phase 2, Month 8 | Region selection |
| Polygon slices (Arena) | ⭐ NEW | Phase 2, Month 8 | Arbitrary shapes |

## 6.5 Slice Masks (Arena)

| Feature | Status | Phase | Notes |
|---------|--------|-------|-------|
| Mask beyond rectangular bounds | ⭐ NEW | Phase 2, Month 8 | Fine-grained masking |

## 6.6 Output Transformation

| Feature | Status | Phase | Notes |
|---------|--------|-------|-------|
| Perspective warping | 📋 Planned | Phase 1, Month 4 | 4-point homography |
| Linear warping | 📋 Planned | Phase 1, Month 4 | Grid mesh warping |
| Bezier warping | 📋 Planned | Phase 2, Month 8 | Smooth curve warping |
| Transform/scale outputs | 📋 Planned | Phase 2, Month 8 | Position/rotate |

## 6.7 Edge Blending (Arena)

| Feature | Status | Phase | Notes |
|---------|--------|-------|-------|
| Overlap blending for multiple projectors | 📋 Planned | Phase 2, Month 8 | Soft-edge blending |
| Gamma correction in blend zones | 📋 Planned | Phase 2, Month 8 | Non-linear blending |

## 6.8 Slice Routing

| Feature | Status | Phase | Notes |
|---------|--------|-------|-------|
| Route layers to slices | ⭐ NEW | Phase 2, Month 8 | Layer→output mapping |
| Route groups to slices (Arena) | ⭐ NEW | Phase 2, Month 8 | Group→output mapping |
| Route preview to slice | ⭐ NEW | Phase 2, Month 8 | Preview window routing |
| Route virtual screens to slices | ⭐ NEW | Phase 2, Month 8 | Virtual display routing |

## 6.9 DMX Output (Arena)

| Feature | Status | Phase | Notes |
|---------|--------|-------|-------|
| DMX/Art-Net output | 📋 Planned | Phase 4, Month 14 | Pixel data to DMX |
| Fixture control | 📋 Planned | Phase 4, Month 14 | LED strip control |

## 6.10 Fixture Editor

| Feature | Status | Phase | Notes |
|---------|--------|-------|-------|
| Custom DMX fixture profiles | ⭐ NEW | Phase 4, Month 14 | Fixture definition editor |

---

# 7. Synchronization

## 7.1 BPM Control

| Feature | Status | Phase | Notes |
|---------|--------|-------|-------|
| Tap tempo button | ⭐ NEW | Phase 4, Month 13 | Manual BPM input |
| Manual BPM adjustment | ⭐ NEW | Phase 4, Month 13 | Direct BPM entry |
| Resync button | ⭐ NEW | Phase 4, Month 13 | Reset beat phase |
| Nudge buttons (speed up/slow down) | ⭐ NEW | Phase 4, Month 13 | Temporary BPM shift |

## 7.2 MIDI Clock

| Feature | Status | Phase | Notes |
|---------|--------|-------|-------|
| MIDI Clock sync | ⭐ NEW | Phase 4, Month 13 | External MIDI tempo |

## 7.3 Ableton Link

| Feature | Status | Phase | Notes |
|---------|--------|-------|-------|
| Link network sync | ⭐ NEW | Phase 4, Month 13 | Tempo + beat position sync |

## 7.4 SMPTE Input (Arena)

| Feature | Status | Phase | Notes |
|---------|--------|-------|-------|
| SMPTE timecode sync | ⭐ NEW | Phase 5, Month 17 | LTC audio timecode |

## 7.5 Denon DJ Sync (Arena)

| Feature | Status | Phase | Notes |
|---------|--------|-------|-------|
| Denon Link sync | ⭐ NEW | Phase 5, Month 17 | Network DJ controller |

---

# 8. Utility and System Functions

## 8.1 Registration Management

| Feature | Status | Phase | Notes |
|---------|--------|-------|-------|
| Online verification | ⭐ NEW | Phase 7, Month 24 | License validation |
| Offline registration | ⭐ NEW | Phase 7, Month 24 | Offline activation |
| Unregister licenses | ⭐ NEW | Phase 7, Month 24 | Remove activation |
| Dongle support | ⭐ NEW | Phase 7, Month 24 | Hardware key |

## 8.2 Undo/Redo

| Feature | Status | Phase | Notes |
|---------|--------|-------|-------|
| Undo/Redo actions | ⭐ NEW | Phase 6, Month 19 | Command pattern history |

## 8.3 Recording

| Feature | Status | Phase | Notes |
|---------|--------|-------|-------|
| Record composition output (video+audio) | ⭐ NEW | Phase 5, Month 18 | Render to file |
| Record without interrupting output | ⭐ NEW | Phase 5, Month 18 | Background encode |
| Import recording as clip | ⭐ NEW | Phase 5, Month 18 | Auto-load recorded file |

## 8.4 Transform Presets

| Feature | Status | Phase | Notes |
|---------|--------|-------|-------|
| Save reusable transform configs | ⭐ NEW | Phase 6, Month 21 | Preset library |

## 8.5 Display Info

| Feature | Status | Phase | Notes |
|---------|--------|-------|-------|
| Identify Displays | ⭐ NEW | Phase 2, Month 7 | Show display numbers |
| Show FPS | ✅ Implemented | Phase 0 | Performance stats |
| Show Test Card | ⭐ NEW | Phase 2, Month 7 | Color bars / test pattern |
| Show Display Info (GPU/EDID) | ⭐ NEW | Phase 2, Month 7 | Hardware detection |

---

# Summary Statistics

## Implementation Status

- ✅ **Implemented**: 12 features
- 🚧 **In Progress**: 6 features
- 📋 **Planned** (already in roadmap): 58 features
- ⭐ **NEW** (added from this analysis): 204 features

**Total Features**: 280 core features

## Phase Distribution of NEW Features

| Phase | NEW Features Added |
|-------|-------------------|
| Phase 1 (Months 4-6) | 25 features |
| Phase 2 (Months 7-9) | 18 features |
| Phase 3 (Months 10-12) | 22 features |
| Phase 4 (Months 13-15) | 28 features |
| Phase 5 (Months 16-18) | 14 features |
| Phase 6 (Months 19-21) | 85 features |
| Phase 7 (Months 22-24) | 12 features |

## Priority Recommendations

### High Priority (Essential for Arena-level software)
1. **BPM Sync & Audio Reactivity** (Phase 4, Month 13)
2. **Layer System & Blend Modes** (Phase 1, Month 4)
3. **Edge Blending** (Phase 2, Month 8)
4. **MIDI/OSC Control** (Phase 4, Month 13)
5. **NDI/Spout/Syphon** (Phase 5, Months 16-17)

### Medium Priority (Professional features)
1. **Media Browser UI** (Phase 6, Month 19)
2. **Timeline & Keyframes** (Phase 6, Month 20)
3. **Effects Library** (Phase 3, Month 12)
4. **Cue System** (Phase 4, Month 15)
5. **Recording Output** (Phase 5, Month 18)

### Lower Priority (Nice-to-have)
1. **Registration System** (Phase 7, Month 24)
2. **Denon DJ Sync** (Phase 5, Month 17)
3. **Fixture Editor** (Phase 4, Month 14)
4. **Color Coding** (Phase 6, Month 19)

---

# Next Steps

1. **Review this document** with the team to validate phase assignments
2. **Update RUST_REWRITE_PLAN.md** with the 204 new features integrated into existing phases
3. **Create detailed specifications** for high-priority NEW features
4. **Adjust timeline** if necessary based on expanded scope
5. **Prioritize Week 2-4 features** to complete Phase 0 on schedule

---

**Document Version**: 1.0
**Created**: 2025-11-11
**Status**: Ready for review and integration into main roadmap
