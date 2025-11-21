# Oscillator Distortion Effect

## Overview

The Oscillator Distortion Effect is a Kuramoto-based coupled oscillator simulation system that creates dynamic, organic distortion effects for projection mapping. It implements a 2D grid of phase oscillators that interact with each other through configurable coupling kernels, generating complex wave patterns that can distort and warp textures in real-time.

## Architecture

### Components

1. **Oscillator Simulation Shader** (`shaders/oscillator_simulation.wgsl`)
   - Implements Kuramoto oscillator model
   - Ring-based coupling kernel with up to 4 configurable rings
   - Ping-pong rendering for temporal evolution
   - Supports Cartesian and log-polar coordinate modes

2. **Distortion Shader** (`shaders/oscillator_distortion.wgsl`)
   - Applies phase-based distortion to input textures
   - Computes distortion vector field from phase gradients
   - Optional color overlay modes (Rainbow, B&W, Complementary)
   - Time-modulated drift for organic motion

3. **OscillatorRenderer** (`crates/mapmap-render/src/oscillator_renderer.rs`)
   - Manages simulation state and GPU resources
   - Handles ping-pong texture updates
   - Provides update/render interface
   - Phase initialization modes

4. **OscillatorConfig** (`crates/mapmap-core/src/oscillator.rs`)
   - Configuration parameters
   - Preset definitions
   - Validation logic

## Features

### Simulation Parameters

- **Simulation Resolution**: Choose grid density (128×128, 256×256, 512×512)
- **Kernel Radius**: Controls interaction distance (1-64 cells)
- **Ring Parameters**: Up to 4 coupling rings with individual:
  - Distance (0-1): Radius fraction
  - Width (0-1): Ring thickness
  - Coupling (-∞ to +∞): Strength (negative=anti-sync, positive=sync)
- **Frequency Range**: Min/max natural frequencies (Hz)
- **Noise Amount**: Random frequency variation (0-1)
- **Coordinate Mode**: Cartesian or Log-Polar

### Distortion Parameters

- **Distortion Amount**: Overall effect strength (0-1)
- **Distortion Scale**: Size of displacement relative to UV (0.001-0.1)
- **Distortion Speed**: Drift modulation speed (0-2)

### Visual Parameters

- **Overlay Opacity**: Color overlay strength (0-1)
- **Color Mode**:
  - Off: Only distortion
  - Rainbow: Phase-mapped hue
  - Black & White: Smooth intensity
  - Complementary: Phase-shifted colors

## Usage Example

```rust
use mapmap_core::OscillatorConfig;
use mapmap_render::OscillatorRenderer;

// Create oscillator renderer
let config = OscillatorConfig::preset_subtle();
let mut oscillator = OscillatorRenderer::new(
    device.clone(),
    queue.clone(),
    wgpu::TextureFormat::Bgra8Unorm,
    &config,
)?;

// Initialize phases
oscillator.initialize_phases(mapmap_core::PhaseInitMode::Random);

// In your render loop:
loop {
    let delta_time = 1.0 / 60.0;

    // Update simulation
    oscillator.update(delta_time, &config);

    // Render distortion effect
    let mut encoder = device.create_command_encoder(&Default::default());
    oscillator.render(
        &mut encoder,
        &input_texture_view,
        &output_texture_view,
        1920,
        1080,
        &config,
    );
    queue.submit(Some(encoder.finish()));
}
```

## Presets

### Subtle
Gentle organic wobble, perfect for subtle animation:
```rust
let config = OscillatorConfig::preset_subtle();
// distortion_amount: 0.3
// distortion_scale: 0.01
// distortion_speed: 0.5
```

### Dramatic
Strong swirling distortion for visual impact:
```rust
let config = OscillatorConfig::preset_dramatic();
// distortion_amount: 0.8
// distortion_scale: 0.05
// distortion_speed: 2.0
```

### Rings
Concentric ring/wave patterns:
```rust
let config = OscillatorConfig::preset_rings();
// distortion_amount: 0.6
// distortion_scale: 0.03
// Optimized ring parameters for wave propagation
```

## Performance Considerations

- **Simulation Resolution**: Lower resolutions (128×128) are faster but less detailed
- **Kernel Radius**: Larger radii increase computation cost quadratically
- **Update Frequency**: Can substep simulation for smoother results
- **Target Performance**:
  - 256×256 @ 60 fps on modern GPUs
  - 512×512 @ 30-60 fps depending on hardware

## Mathematical Background

### Kuramoto Model

The Kuramoto model describes coupled phase oscillators:

```
dθᵢ/dt = ωᵢ + Σⱼ K(dᵢⱼ) sin(θⱼ - θᵢ)
```

Where:
- `θᵢ`: Phase of oscillator i
- `ωᵢ`: Natural frequency
- `K(d)`: Distance-dependent coupling kernel
- `dᵢⱼ`: Distance between oscillators i and j

### Ring-Based Coupling Kernel

Uses difference-of-Gaussians (DoG) profile:

```
K(d) = Σₖ -cₖ [exp(-(d-rₖ)²/2σ₁²) - exp(-(d-rₖ)²/2σ₂²)]
```

This creates ring-shaped interaction regions that enable:
- Synchronization at specific distances
- Anti-synchronization (negative coupling)
- Complex pattern formation

### Distortion Field

The distortion vector field is derived from the phase gradient:

```
D(u,v) = ∇θ/|∇θ| × sin(θ) × drift(t)
```

This creates direction from the gradient and magnitude from the phase value, modulated over time for organic motion.

## Phase Initialization Modes

- **Random**: Uniform random phases in [0, 2π)
- **Uniform**: All oscillators start at phase 0
- **Plane Horizontal**: Linear phase gradient in X direction
- **Plane Vertical**: Linear phase gradient in Y direction
- **Plane Diagonal**: Linear phase gradient in X+Y direction

Different initialization patterns create different emergent behaviors in the simulation.

## Integration with MapMap

The oscillator effect can be applied as:

1. **Post-Processing Effect**: Apply to the final composited output
2. **Layer Effect**: Apply to individual layers before compositing
3. **Chained Effect**: Combine with other effects (blur, color grade, etc.)

## Technical Specifications

### GPU Resources

- **Phase Textures**: 2× R32Float textures (ping-pong)
- **Vertex Buffer**: Fullscreen quad (4 vertices)
- **Index Buffer**: 2 triangles (6 indices)
- **Uniform Buffers**:
  - Simulation params (~128 bytes)
  - Distortion params (~64 bytes)

### Shader Stages

1. **Simulation Pass**: Update phase texture
   - Input: Previous phase texture
   - Output: New phase texture
   - Frequency: Once per frame

2. **Distortion Pass**: Apply effect to input
   - Input: Source texture + phase texture
   - Output: Distorted texture
   - Frequency: Once per render

## Future Enhancements

Potential additions:
- [ ] Log-polar coordinate mode implementation
- [ ] Multiple oscillator layers
- [ ] Audio-reactive frequency modulation
- [ ] Frequency field from texture luminance
- [ ] Save/load parameter presets
- [ ] Visual preset browser
- [ ] Real-time parameter interpolation
- [ ] Displacement map export

## References

- Kuramoto, Y. (1984). "Chemical Oscillations, Waves, and Turbulence"
- Acebrón et al. (2005). "The Kuramoto model: A simple paradigm for synchronization phenomena"
- oscillator_layer.md: Original specification document

## License

GPL-3.0 (same as MapMap)
