Below is `oscillator.md`, a specification for a visual distortion layer for MapMap.

---

# Product Specification =

## 1. Purpose and Scope

This document specifies a visual tool that produces a visual distortion layer for MapMap:

* Start from a still image or valid video file (we will use the word image for both still and video) (preset or user-supplied)
* Apply three stacked effects to animate it:

  1. **Pattern Recognition** (precomputed DeepDream-style overlays).
  2. **Coupled Oscillators Simulation** (grid of Kuramoto oscillators on the image). ([Qri][1])
  3. **Drifting** (screen-space wobble of the final composite). ([Qri][1])

The implementation targets **WebGL2** with support for `EXT_color_buffer_float` and `OES_texture_float_linear`, so that the simulation runs fully client-side in a browser on desktop GPUs. ([Qri][1])

The goal is functional and perceptual equivalence, not identical low-level code.

---

## 2. High-level System Overview

### 2.1 Core Effects and Data Flow

The original tool has three effect families and a fixed dataflow: ([Qri][1])

1. **Pattern Recognition**

   * For built-in photos, precompute several **InceptionV3 DeepDream layers** and store as static images.
   * At runtime, select a DeepDream layer and blend it over the base photo with controls for **opacity**, **saturation**, optional **video cycling**, and a **mask** (speed/size).

2. **Coupled Oscillators Simulation**

   * Treat each pixel of a reduced-resolution grid (up to 1280×720) as a **Kuramoto oscillator** with phase θ and natural frequency ω. ([Qri][1])
   * Neighbor interactions are controlled by a **ring-based coupling kernel** with 4 concentric rings (distance/width/coupling). ([Qri][1])
   * Two independent layers (Layer 1 & Layer 2), with optional cross-layer coupling.
   * The **photo controls the simulation** via brightness/depth/edges affecting kernel radius and oscillator frequencies.

3. **Drifting**

   * A final **wobble/warping** of the composite image, with controls for **Amplitude**, **Speed**, and **Pattern Size**.

**Rendering pipeline per frame:**

1. Start with base image + auxiliary maps (brightness, edges, depth, precomputed DeepDream layers).
2. Update Kuramoto phase textures for each oscillator layer using the coupling kernel.
3. Colorize phases and blend with base image + pattern recognition overlays.
4. Apply the Drifting warp in screen space.
5. Present to screen and optionally stream to video recorder.

---

## 3. Functional Requirements

The UI layout and controls follow the original tool’s structure. ([Qri][2])

### 3.1 Global Controls

* **Toggle Controls**
  Show/hide the control panel (for full-screen viewing).

* **Intro / Tutorial Video**
  External link; not essential to core simulation.

* **Detailed Instruction Manual**
  Link to a markdown or HTML page summarizing the tool’s function (like QRI’s reference manual).

* **Playback & Simulation**

  * `Play` / `Pause` buttons.
  * `Single Step`: advance one simulation frame.
  * `Speed:` read-only display of measured FPS (e.g., “60 fps”).

* **Recording**

  * `Record video`: begins recording a canvas capture stream via `MediaRecorder`.
  * `Finish & download`: stops recording and saves as WebM/MP4.

* **Compatibility / Capability Checks**

  * If JavaScript disabled, show “page only works if Javascript is enabled”.
  * If `canvas` or WebGL2/float extensions are missing, show fallback message and disable simulation.

### 3.2 Image Source: “Select Photo”

* **Preset / Load**

  * Small gallery of ~8 static example photos.
  * `Preset` button: open gallery modal.
  * `Load` button: open file picker.

* **Custom Image Input**

  * Drag & drop area.
  * Clipboard paste support.
  * Important: **custom images stay local**; no upload to server.

* **Limitations**

  * For custom images, no stored DeepDream layers or depth maps; some options disabled:

    * “Kernel Shrinks By Depth”
    * “Frequencies Vary By Depth”
    * DeepDream layer selection.

### 3.3 Drifting Panel

Controls: `Amplitude`, `Speed`, `Pattern Size`.

Behavior:

* **Amplitude**: scales displacement magnitude in pixels.
* **Speed**: controls temporal frequency of the drift pattern.
* **Pattern Size**: physical size (wavelength) of the drift pattern in image space.

Implementation model:

* Maintain a procedural vector field `D(u, v, t)` over normalized coordinates.
* Warp final color sample as `color(u, v) := baseColor(u + A·D_x, v + A·D_y)`.
* Use a low-frequency combination of sine/cosine or noise fields for D.

### 3.4 Pattern Recognition Panel

Controls:

* `DeepDream Layer` (selection)
* `Use Video Pattern` (toggle)
* `Video Speed` (float)
* `Opacity`
* `Saturation`
* `Mask Speed`
* `Mask Size`

Behavior:

* **DeepDream Layer**

  * Select one of several precomputed DeepDream images for the current preset photo.
  * Each layer corresponds to an InceptionV3 layer amplified once, stored offline. ([Qri][1])

* **Use Video Pattern** / **Video Speed**

  * Cycle through a short sequence of DeepDream frames or parameterized distortions to create motion.
  * `Video Speed` controls frame advancement per second.

* **Opacity**

  * Blend factor `α_PR` between base photo and pattern recognition layer.

* **Saturation**

  * Post-processing on the PR layer: HSL/HSV saturation scaling.

* **Mask Speed / Mask Size**

  * Generate a scalar “mask” field `M(u, v, t)` ∈ [0, 1]:

    * `Mask Size` controls spatial scale (large vs fine features).
    * `Mask Speed` controls temporal evolution.
  * Effective PR blending becomes: `α_PR * M(u, v, t)`.

### 3.5 Coupled Oscillators Simulation – Global Panel

Controls: ([Qri][1])

* `Opacity` (oscillator layers vs underlying photo)
* `Colors` (enumerated color modes)
* `Num Layers` (1 or 2)
* `Blending Balance`
* (Note: “No effect in bottom color mode” for some color schemes).

Behavior:

* **Opacity**

  * Blend factor `α_CO` between composite (photo+PR) and oscillator visualization.

* **Colors**
  A set of phase→color mapping modes:

  * `Rainbow, normal blending`
    `hue = θ / 2π`, standard HSV to RGB; layers blended by weighted average.
  * `Rainbow double`
    Two full rainbows per 2π (i.e., periodicity π).
  * `Black & white smooth, normal blending`
    `θ` mapped to grayscale [0,1]; average layers.
  * `Black & white smooth, interference blending`
    Use a nonlinear blending formula of grayscale layers to create constructive/destructive patterns.
  * `Black & white sharp, interference blending`
    Thresholded B/W with XOR-like combination between layers.
  * `Complementary colors sharp, normal blending`
    Layer 1: blue–yellow gradient; Layer 2: green–magenta gradient.
  * `Complementary colors sine, asymmetric`
    Layer 2 phases define a gradient; Layer 1 phases modulate its rotation over a limited range.
  * `Complementary colors rotate, asymmetric`
    Layer 2 gradient; Layer 1 rotates it around entire color wheel; blending balance ignored.

* **Num Layers**

  * 1 or 2 Kuramoto layers in parallel.
  * If `Num Layers = 1`, all controls for Layer 2 disabled.

* **Blending Balance**

  * Scalar `b ∈ [1, 2]` roughly; controlling relative visibility of layer 1 vs layer 2 in combined oscillator image (before color blend with photo).

### 3.6 Per-Layer Controls (Layer 1 and Layer 2)

Each layer has duplicated controls. ([Qri][1])

#### Spatial and Kernel Controls

* `Log-Polar Transform` (checkbox)
* `Resolution Halvings` (integer ≥ 0)
* `Effective Layer Resolution:` (read-only)
* `Effective Kernel Radius:` (read-only)
* `Kernel Radius`
* `Kernel Shrinks By`: `Depth` | `Edges` | `Nothing`
* `Kernel Shrink Factor`
* `Coupling Distance/Width` table:

  * For each ring `k ∈ {0,1,2,3}`:

    * `Distance` (center radius as fraction of outer radius)
    * `Width` (ring thickness)
    * `Coupling` (positive = synchronizing, negative = anti-synchronizing)
* `Coupling to Layer 2` / `Coupling to Layer 1` (cross-layer coupling constant)

Behavior:

* **Log-Polar Transform**

  * Use mapping for simulation coordinates:

    * Let `(x, y)` be normalized coordinates in [−1,1]², distance `d = √(x² + y²)`, angle `φ = atan2(y, x)`.
    * Map radial coordinate to `ρ = ln(0.2 + 23·d) / π`, clipping to [0,1]. ([Qri][1])
  * Simulation runs on the log-polar grid; visualization reprojects back to the original image.

* **Resolution Halvings**

  * If `h` halvings, simulation grid becomes `(W/2^h, H/2^h)`; visual output upsampled.
  * Used to reduce GPU load; quality is reduced for large `h` as kernel becomes smaller than pixel size.

* **Kernel Radius / Shrink**

  * `Kernel Radius`: outer radius R in pixels at full resolution.
  * `Kernel Shrinks By`: choose per-pixel scaling factor `s(x,y)` from:

    * Depth map (for presets).
    * Edge distance map.
    * Constant 1 (Nothing).
  * `Kernel Shrink Factor` `∈ (0,1]` sets minimum radius on maximal shrink locations.

* **Ring-based Coupling Kernel**

  * Rings parameterized by `Distance_k ∈ [0, 1]` (fraction of R) and `Width_k ∈ [0, 1]`.
  * Coupling per ring `C_k` > 0 (synchronizing), < 0 (anti-synchronizing), or 0 (neutral).
  * Continuous kernel function suggested (following QRI’s ring function in other simulations): ([Qri][3])

    * For distance `d` from oscillator:

      * `r_k = Distance_k * R_local`

      * `w_k = Width_k * R_local`

      * Ring function:

        [
        R(d, r_k, w_k) = \exp!\left(-\frac{(d - r_k)^2}{2(0.5w_k)^2}\right)
        - \exp!\left(-\frac{(d - r_k)^2}{2w_k^2}\right)
        ]

      * Influence amplitude:

        [
        I(d) = \sum_{k=0}^{3} -C_k \cdot R(d, r_k, w_k)
        ]

* **Cross-layer Coupling**

  * Constant `K_cross` controls influence of layer B on layer A:

    * Extra term added to phase derivative:
      [
      C^{\text{cross}}*i = K*{\text{cross}} \sin(\theta^{(B)}_i - \theta^{(A)}_i)
      ]

#### Oscillator Frequency & Initialization

Per layer: `Min Hz`, `Max Hz`, `Frequencies Vary By`, `Size`, `Random noise`, and phase initialization shortcuts. ([Qri][2])

* **Frequency Range**

  * Map from `Min Hz` / `Max Hz` to per-pixel natural frequencies `ω_i`.

* **Frequencies Vary By** (choose one or combination):

  * Depth (lower frequency far away).
  * Edges (higher frequency at edges).
  * Brightness (frequency ∝ local brightness).
  * Spatial Sine & Cosine Pattern:

    * `Size` sets spatial wavelength; e.g.:
      [
      \omega_i = \omega_{\text{min}} + (\omega_{\text{max}} - \omega_{\text{min}})\cdot
      \frac{1+\cos(2\pi k_x x_i) \cos(2\pi k_y y_i)}{2}
      ]
  * Random noise:

    * Add small random component to `ω_i` for diversity.

* **Phase Initialization Shortcuts**

  * `Randomize phases`: θ_i ∼ Uniform[0, 2π).
  * `Unify`: θ_i = constant for all pixels.
  * `Plane waves H`: θ_i ∝ x (horizontal wave).
  * `Plane waves V`: θ_i ∝ y (vertical wave).
  * `Plane waves 45°`: θ_i ∝ x + y.

* **Global Phase Actions**

  * `Randomize All Phases`
  * `Unify All Phases`

---

## 4. Simulation Model

### 4.1 Kuramoto Lattice

Let the simulation grid define indices `i = (x, y)` over W×H lattice (per layer, possibly with resolution halving).

Discrete Kuramoto update per time step Δt: ([Wikipedia][4])

1. **Base update equation**

[
\theta_i(t + \Delta t) = \theta_i(t) + [\omega_i + C_i^{\text{local}} + C^{\text{cross}}_i]\Delta t
]

Where:

* `ω_i`: natural frequency (Hz converted to radians per unit time).
* `C_i^{\text{local}}`: effect of neighbors via spatial coupling kernel.
* `C_i^{\text{cross}}`: cross-layer influence (if 2 layers).

2. **Local coupling term**

[
C_i^{\text{local}} = \frac{1}{N_i} \sum_{j} K(d_{ij}) \sin(\theta_j - \theta_i)
]

* `d_ij`: Euclidean distance between lattice sites i and j in the chosen metric (cartesian or log-polar).
* `K(d)`: scalar kernel defined via ring function `I(d)` above, scaled appropriately.
* `N_i`: normalization factor for discrete sampling.

Implementation detail:

* In GLSL, for each pixel, iterate over integer offsets `(dx, dy)` such that `d = √(dx²+dy²)` ≤ `R_local(i)`, and accumulate contributions with per-sample weight approximating `K(d)`.

### 4.2 Coordinate Systems

* **Cartesian mode**: `d` computed in pixel units or normalized.

* **Log-polar mode**:

  * Precompute forward and inverse maps:

    * `cartesian → logpolar`:

      * `d = √(x² + y²) ∈ [0, d_max]`
      * `ρ = ln(0.2 + 23·d_norm)/π` (clipped). ([Qri][1])
      * `φ = atan2(y, x)/(2π)` normalized to [0,1).
    * `logpolar → cartesian` by inverse mapping for visualization.

* Use log-polar grid for simulation (distance for kernel, neighbors, etc.), but sample base photo in cartesian coordinates.

### 4.3 Image-Derived Maps

For each image:

* **Brightness map**: grayscale luminance L(x,y).
* **Edge map**: distance to nearest edge (e.g. using Sobel + distance transform). ([Qri][1])
* **Depth map**:

  * Precomputed for preset photos, stored as static textures (0 = near, 1 = far).
  * For custom photos, either:

    * Generate approximate monocular depth (optional, requires ML in browser) or
    * Disable depth-dependent options as original tool does.

These maps feed:

* Kernel shrink logic.
* Frequency variation logic.

---

## 5. Rendering and WebGL Architecture

### 5.1 Technology Stack

* **Language:** TypeScript.

* **Frontend framework:** React or Svelte (optional; not strictly required).

* **Graphics API:** WebGL2 with:

  * `EXT_color_buffer_float`
  * `OES_texture_float_linear` ([Qri][1])

* **Build tooling:** Vite / Webpack.

### 5.2 GPU Resources

Per layer (1 or 2):

* Float RG32F or RGBA32F textures:

  * `phaseTextureA`, `phaseTextureB` (ping-pong).
* Optionally separate textures for:

  * `ωTexture` (frequencies).
  * `kernelParamTexture` (if packing kernel parameters).

Global:

* `baseImageTexture` (RGB8).
* `brightnessTexture`, `edgeTexture`, `depthTexture` (R16F/R32F).
* `deepDreamTextures[]` (RGB8 sequence per preset).
* `driftFieldBase` (noise seed textures or param seed only).

### 5.3 Shaders

1. **Oscillator Update Shader (`oscillator_update.frag`)**

   * Inputs:

     * Previous phase texture (sampler2D).
     * Frequency map.
     * Edge/depth/brightness maps.
     * Kernel parameters (R, ring distances/widths/couplings, shrink factor, cross-layer coupling).
     * Log-polar flag & transform parameters.
   * Output:

     * New phase texture (single float encoded in [0,1] or [-π,π], e.g. use `phase = fract(phase + step)`).

2. **Colorization & Composite Shader (`composite.frag`)**

   * Inputs:

     * Final phase textures for each layer.
     * Base image & PR overlay (DeepDream).
     * Drifting parameters & procedural vector field.
     * Opacities & blending balance.
     * Color mode selection.
   * Steps:

     * Map phase(s) to color(s) based on chosen mode.
     * Combine layers using blending balance.
     * Blend oscillator composite with base+PR composite.
     * Apply Drifting warp by sampling color at displaced UV.

3. **Pattern Recognition Shader (optional extra)**

   * To handle video pattern & mask generation:

     * Evaluate `M(u,v,t)` via procedural noise or wave patterns.
     * Combine DeepDream layer(s) with mask and saturation.

---

## 6. Parameter Sharing and Telemetry (Optional)

The original tool supports:

* `Submit Parameters to QRI` with fields:

  * Drug, dose, method, timing, subjective match rating, notes. ([Qri][2])
* `Share / export parameters` and `Import parameters` via JSON.

Replica behavior:

* **Parameter export/import**

  * Serialize current UI state (all sliders, toggles, image ID) into a JSON blob.
  * Allow import from JSON text or link query parameter (e.g., `?params=...` base64-encoded).

* **Parameter submission**

  * Optional endpoint; if enabled, POST anonymized parameter JSON + questionnaire answers to a backend.
  * Preserve privacy: do not log IP or identifying info on client side; backend policy is out of scope but should avoid storing PII.

---

## 7. Implementation Roadmap – How to Build the Replica

A concrete sequence to implement the application:

1. **Project Setup**

   * Initialize TypeScript project with bundler (Vite).
   * Create full-screen WebGL2 canvas and UI pane (HTML/CSS/React).

2. **Base Image Pipeline**

   * Implement file input, drag-and-drop, and paste for image load.
   * Generate brightness and edge maps on CPU (Canvas2D or WebGL) and send to GPU.
   * For presets, also load precomputed depth and DeepDream layers.

3. **Minimal Kuramoto Layer (Single Layer, No Rings Yet)**

   * Create phase textures for a lower-resolution grid (e.g. 320×180).
   * Implement simple all-neighbors coupling within a small fixed radius with uniform K.
   * Implement Euler integration of:
     [
     \theta_i' = \theta_i + (\omega_i + \frac{K}{N_i}\sum_j \sin(\theta_j - \theta_i))\Delta t
     ]
   * Map phase to rainbow color and overlay on base image with a single opacity control.

4. **Full Coupling Kernel and Rings**

   * Replace uniform K with continuous ring-based kernel `I(d)` as specified in §3.6/4.1.
   * Implement 4 rings with Distance/Width/Coupling UI control.
   * Add `Kernel Radius`, `Kernel Shrinks By`, and `Kernel Shrink Factor`.

5. **Second Layer and Cross-Layer Coupling**

   * Duplicate textures and parameters for Layer 2.
   * Add cross-layer coupling term `K_cross sin(θ_other - θ_self)`.
   * Implement `Num Layers`, `Blending Balance`, and asymmetric color modes.

6. **Log-Polar Mode and Resolution Halving**

   * Precompute log-polar mapping coordinate textures or inline mapping functions.
   * Let user switch between Cartesian and log-polar in the shader via uniform.
   * Add `Resolution Halvings` control:

     * Compute simulation grid size on CPU (for display).
     * Adjust phase texture size and resample/initialize as needed.

7. **Drifting Effect**

   * Implement global procedural drift field:

     * Example: sum of a few sinusoids at different orientations and frequencies:

       * `D_x = sin(k1·x + ω1 t) + sin(k2·y + ω2 t)`, etc.
     * Or use a tiled noise texture animated over time.
   * Expose `Amplitude`, `Speed`, `Pattern Size` as uniforms controlling field parameters.

8. **Pattern Recognition Overlay**

   * For presets:

     * Load per-photo DeepDream layers into GPU textures.
     * Implement layering, saturation, opacity.
     * Add `Use Video Pattern` and `Video Speed` to animate between frames or along a parameter path.
     * Implement mask generator `M(u,v,t)` with controls `Mask Speed` and `Mask Size`.
   * For custom images:

     * Disable or stub DeepDream-specific controls (matching original tool behavior).

9. **Playback, Single Step, and FPS**

   * Use `requestAnimationFrame` loop with time accumulation for fixed simulation Δt steps.
   * Implement `Play/Pause/Single Step` UI.
   * Track moving average FPS and display in `Speed` label.

10. **Video Recording**

    * Use `canvas.captureStream()` to obtain a `MediaStream`.
    * Create `MediaRecorder(stream)`; start/stop on `Record video` / `Finish & download`.
    * Offer resulting Blob as downloadable video file.

11. **Parameter Export/Import and Sharing**

    * Serialize UI state to JSON.
    * Provide text area to copy/paste JSON.
    * Optionally URL-encode parameters into query string for shareable links.

12. **Performance and Graceful Degradation**

    * On slower machines, automatically suggest increasing `Resolution Halvings` or reducing `Kernel Radius` when FPS drops below a threshold.
    * Provide presets tuned to different performance levels.

---

## 8. Non-functional Requirements

* **Performance**

  * Aim for 60 fps at 1280×720 output on mid-range GPUs in common parameter ranges.
  * Maintain responsiveness of UI even when simulation FPS drops.

* **Portability**

  * Work in major desktop browsers with WebGL2 + required extensions.
  * Explicitly detect and clearly report unsupported cases.

* **Privacy**

  * Do not upload user images by default; processing is local.
  * Parameter submission is optional, clearly marked, and anonymized.

* **Maintainability**

  * Centralize shader code and parameter schemas.
  * Use typed configuration objects (TypeScript interfaces) to map between UI, uniforms, and saved JSON.

---

This specification provides the functional behavior and a concrete implementation path to build a working replica of QRI’s Oscilleditor, including its Kuramoto-based animation of still images, pattern recognition overlays, and drifting warp.

[1]: https://qri.org/oscilleditor/doc/reference-manual "Reference manual for QRI's Oscilleditor"
[2]: https://qri.org/oscilleditor/ "QRI's Oscilleditor"
[3]: https://qri.org/blog/cessation-simulations "Towards Computational Simulations of Cessation"
[4]: https://en.wikipedia.org/wiki/Kuramoto_model?utm_source=chatgpt.com "Kuramoto model"
