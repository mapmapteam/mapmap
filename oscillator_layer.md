# Oscillatory Distortion Layer – Specification (`oscillator_layer.md`)

## 1. Purpose and Scope

This document specifies a **single effect layer** to be added to an existing C++ projection-mapping application.

The layer:

- Takes an input texture (the rendered content of an existing layer or composition).
- Runs a **2D coupled oscillator (Kuramoto) simulation** on a grid over the layer’s UV domain.
- Uses the oscillator phases to:
  - Optionally generate a color overlay.
  - Generate a **dynamic distortion field** that warps the input texture (drift/wobble, ring structures, etc.).
- Outputs a texture of the **same resolution** as the input, suitable for insertion anywhere in the existing layer stack.

You already have the projection mapping application; this specification only covers the **effect layer**.

---

## 2. Integration Assumptions

Assume the host projection-mapping engine provides:

- A **C++ plugin / effect interface**, with callbacks like:
  - `init(const InitContext&)`
  - `update(const UpdateContext&)`
  - `render(const RenderContext&)`
  - `onParameterChange(const ParameterSet&)`
- Access to a GPU API (strongly assumed: **OpenGL / OpenGL ES**; adapt similarly for DirectX/Metal).
- Per-frame data:
  - `double timeSeconds` – monotonic time.
  - `double deltaTimeSeconds` – time since last frame.
  - Input texture: `GLuint inputTexture` (or equivalent texture handle).
  - Render target: FBO or texture for the effect output, matching the input resolution.
- A parameter system for UI sliders, checkboxes, and enums.

If your host API differs, adjust the naming and wiring; the internal logic remains the same.

---

## 3. High-Level Effect Architecture

The oscillator layer consists of two conceptual stages:

1. **Simulation stage (off-screen):**
   - Evolve a 2D grid of phases `θ(x, y)` according to a Kuramoto-type update.
   - Run on a **reduced-resolution grid** (e.g. 256×256, configurable) in its own textures.
   - Optionally treat coordinates as log-polar instead of Cartesian.

2. **Render/distortion stage (on-screen):**
   - Sample the oscillator grid and derive:
     - A **distortion vector field** `D(u, v, t)` over normalized layer coordinates.
     - Optionally a **color overlay** from phase information.
   - Warp the input texture using `D` and blend the overlay, controlled by user parameters.

Data flow per frame:

```text
phaseTexture (ping-pong A/B)
       ↓ oscillator-update shader
updatedPhaseTexture
       ↓ sampled by distortion shader
distortion field & optional color overlay
       ↓
warp inputTexture → outputTexture
````

---

## 4. Simulation Model

### 4.1 Grid and Coordinates

* Simulation grid resolution: `simWidth × simHeight`, e.g. `256 × 256`.
* Domain: normalized coordinates `(sx, sy) ∈ [0, 1]²`.
* Two coordinate modes:

  * **Cartesian** (default): `sx, sy` map linearly to UV.
  * **Log-polar** (optional):

    * Map `(u, v)` ∈ [0, 1]² into centered coordinates `(x, y) ∈ [-1, 1]²`.
    * Let `d = sqrt(x² + y²)` (radius), `φ = atan2(y, x)`.
    * Define a radial log mapping:

      [
      ρ = \frac{\ln(a + b\cdot d)}{\pi}
      ]

      with constants `a > 0`, `b > 0` chosen so that `ρ` is roughly in [0, 1] for the visible region.
    * Simulation runs on `(ρ, φ)` grid; rendering converts back to `(u, v)`.

For a first implementation you can ignore log-polar and add it later.

### 4.2 Kuramoto Update

For each simulation cell `i` (index `(x, y)`):

* State: `θ_i(t)` = phase, stored as float in [0, 1) representing `[0, 2π)` or as raw radians.
* Natural frequency: `ω_i` (radians / second).
* Local coupling term from neighbors:

[
C_i^{\text{local}} = \frac{1}{N_i}\sum_{j} K(d_{ij}) \sin(\theta_j - \theta_i)
]

* `d_ij`: Euclidean distance between cell centers in simulation coords.
* `K(d)`: scalar kernel (see below).
* `N_i`: normalization factor (sum of kernel weights of sampled neighbors).

Discrete integration (Euler) with step `Δt`:

```text
θ_i(t + Δt) = θ_i(t) + [ω_i + C_i^{local}] * Δt
```

Optionally wrap phase into `[0, 2π)` or `[0, 1)` using `fmod` or `fract`.

#### 4.2.1 Ring-Based Kernel

Define a maximum radius `R` (in grid cells) and up to `N_rings = 4` rings.

For each ring `k`:

* `distance_k ∈ [0, 1]` – radius fraction.
* `width_k ∈ (0, 1]`    – relative thickness.
* `coupling_k`          – strength (positive sync, negative anti-sync).

Per neighbor sample at distance `d`:

1. Convert to absolute ring parameters:

   ```text
   r_k = distance_k * R
   w_k = width_k    * R
   ```

2. Define a ring profile, e.g. difference of Gaussians:

   ```text
   Rk(d) = exp(-(d - r_k)^2 / (2 * (0.5 * w_k)^2))
          - exp(-(d - r_k)^2 / (2 * w_k * w_k))
   ```

3. Total kernel:

   ```text
   K(d) = sum_k (-coupling_k) * Rk(d)
   ```

During simulation:

* For each cell, iterate over integer offsets `(dx, dy)` with `sqrt(dx²+dy²) <= R`.
* Accumulate `K(d_ij) * sin(θ_j - θ_i)`.

You can cache sampling offsets and `d` values in a CPU side table once and upload to the shader as a uniform array or use a fixed maximum radius and compute `d` in the shader.

### 4.3 Frequency Field

Natural frequencies define the “driving pattern” of the distortion. Use a combination of:

* Global min/max frequencies: `ω_min`, `ω_max` (in Hz, convert to radians/s by `2π * Hz`).
* Optional spatial variation:

  1. **Brightness-based (if you want):**

     * Sample luminance from a downscaled version of the input texture or a static mask.

  2. **Purely procedural pattern (simpler and independent of content):**

     * For example, a 2D cosine:

       ```text
       f(x, y) = 0.5 * (1 + cos(2π * scaleX * x) * cos(2π * scaleY * y))
       ω_i = ω_min + (ω_max - ω_min) * f(x, y)
       ```

  3. **Random noise component:**

     * `ω_i += noiseAmount * random(-1, 1)` per cell, fixed at initialization.

The simplest first implementation: a fixed `ω` everywhere; add spatial variation later.

### 4.4 Initialization

On reset / parameter change:

* Initialize `θ_i` according to selected mode:

  * `Random` – uniform random in `[0, 2π)`.
  * `Uniform` – all equal.
  * `Plane wave horizontal` – `θ_i ∝ x`.
  * `Plane wave vertical` – `θ_i ∝ y`.
  * `Plane wave diagonal` – `θ_i ∝ x + y`.

You can implement these via a one-shot initialization pass over the phase texture.

---

## 5. GPU Resources and Shaders

### 5.1 Textures

* **Simulation:**

  * `phaseTexA` (floating point, size `simWidth × simHeight`).
  * `phaseTexB` (same; ping-pong target).
  * Optional: `freqTex` (precomputed frequencies), if not computed procedurally in shader.

* **Input/Output:**

  * `inputTex` – supplied by host (projected content).
  * `outputTex` – FBO’s color attachment used for the effect output.

### 5.2 Oscillator Update Shader

Fragment shader (GLSL-style outline):

* Inputs:

  * `sampler2D phaseTexPrev`
  * Optional: `sampler2D freqTex`
  * Uniforms: `simResolution`, `Δt`, `R`, ring parameters, kernelShrink, etc.
* For each fragment (simulation cell):

  * Read `θ_i`.
  * Compute `ω_i` (from `freqTex` or procedurally).
  * Loop over neighbor offsets (predefined).
  * Accumulate coupling term `C_i^{local}`.
  * Integrate `θ_i` forward by `Δt`.
  * Write new `θ_i` to `phaseTexNext`.

Ping-pong each frame: swap `phaseTexA` and `phaseTexB`.

### 5.3 Distortion / Render Shader

Fragment shader for the projection effect:

* Inputs:

  * `sampler2D inputTex`
  * `sampler2D phaseTex` (current simulation state).
  * Uniforms:

    * `vec2 resolution` (output size).
    * `vec2 simResolution` (simulation grid size).
    * `float distortionAmount`
    * `float distortionScale`
    * `float distortionSpeed`
    * `float overlayOpacity`
    * `int colorMode` (for optional overlay).
    * `bool useLogPolar`
* Steps, per output pixel:

  1. Compute normalized coordinate `uv = (x + 0.5, y + 0.5) / resolution`.

  2. Map `uv` to simulation coordinate `suv`:

     * Cartesian: `suv = uv`.
     * Log-polar (if enabled): transform `uv` → `(ρ, φ)`.

  3. Sample phase (and, if desired, neighbors) from `phaseTex`:

     * E.g. `θ(x, y)` and `θ(x+1, y)`, `θ(x, y+1)` to approximate local gradient.

  4. Construct a distortion vector `D` from phase/gradient:

     * Example 1 (phase gradient):

       ```text
       θ_c = θ(suv)
       θ_dx = θ(suv + vec2(1/simWidth, 0))
       θ_dy = θ(suv + vec2(0, 1/simHeight))

       // unwrap small differences
       g = vec2(θ_dx - θ_c, θ_dy - θ_c)
       normalize phase differences to [-π, π]

       D = normalize(g) * sin(θ_c)   // direction ~ gradient, magnitude ~ sin phase
       ```

     * Example 2 (direct phase-based swirl):

       ```text
       float a = θ_c;   // in [0, 2π)
       D = vec2(cos(a), sin(a));
       ```

     * Final displacement:

       ```text
       vec2 disp = distortionAmount * D * distortionScale;
       ```

  5. Apply time modulation for drifting:

     ```text
     float phaseT = distortionSpeed * timeSeconds;
     disp *= (0.5 + 0.5 * sin(phaseT + θ_c));  // slow drift in magnitude
     ```

  6. Distorted sample coordinate:

     ```text
     vec2 uvDistorted = uv + disp;
     ```

     Optionally clamp or wrap coordinates.

  7. Sample base color:

     ```text
     vec4 baseColor = texture(inputTex, uvDistorted);
     ```

  8. Optional overlay from phase:

     * Phase → color mapping:

       ```text
       // Rainbow
       float h = θ_c / (2.0 * PI); // [0,1)
       vec3 overlayRGB = hsv2rgb(vec3(h, 1.0, 1.0));

       vec4 overlayColor = vec4(overlayRGB, 1.0);
       ```

     * Mix with base:

       ```text
       vec4 color = mix(baseColor, overlayColor, overlayOpacity);
       ```

  9. Output `color`.

The effect then feeds into the host’s composition pipeline as a normal layer.

---

## 6. Parameters for the Host UI

Expose parameters as the projection-mapping application’s UI controls:

### 6.1 Simulation Parameters

* `Simulation Resolution` (enum): `128×128`, `256×256`, `512×512`.
* `Kernel Radius` (float): 1–64 (cells).
* `Ring[k].Distance` (float): 0–1, for k=0..3.
* `Ring[k].Width` (float): 0–1.
* `Ring[k].Coupling` (float): negative to positive (e.g. −2.0 to +2.0).
* `Frequency Min` (float, Hz): e.g. 0.0–4.0.
* `Frequency Max` (float, Hz): e.g. 0.0–4.0.
* `Noise Amount` (float): 0–1, random component in `ω`.
* `Coordinate Mode` (enum): Cartesian / Log-Polar.
* `Phase Initialization` (action buttons or enum + button): Random / Uniform / Plane H / Plane V / Diagonal.
* `Reset Simulation` (button): reinitialize phases and frequencies.

### 6.2 Distortion / Visual Parameters

* `Distortion Amount` (float): 0–1 (overall strength of warp).
* `Distortion Scale` (float): e.g. 0.001–0.05 (size of displacement relative to UV).
* `Distortion Speed` (float): 0–2 (speed of drift modulation).
* `Overlay Opacity` (float): 0–1 (0 = no color overlay, only warp).
* `Color Mode` (enum):

  * Off (only warp).
  * Rainbow.
  * Black & White smooth.
  * Complementary colors.
* `Log-Polar Strength` (optional float): 0–1, fade between Cartesian and log-polar behavior smoothly.

---

## 7. C++ Plugin Skeleton (Conceptual)

Below is an outline showing how the effect would look structurally. Adjust to your host API.

```cpp
class OscillatorLayer : public IEffectPlugin {
public:
    bool init(const InitContext& ctx) override {
        // compile shaders
        simProgram_     = compileProgram(simVertexSrc, simFragSrc);
        distortProgram_ = compileProgram(quadVertexSrc, distortFragSrc);

        // allocate phase textures & FBOs
        allocateSimulationTextures(ctx.defaultSimResolution);
        return true;
    }

    void resize(int width, int height) override {
        outputWidth_  = width;
        outputHeight_ = height;
        // no need to change sim size unless you want to tie it to output size
    }

    void update(const UpdateContext& ctx) override {
        float dt = static_cast<float>(ctx.deltaTimeSeconds);

        // might substep simulation if dt is large
        runSimulationStep(dt);
    }

    void render(const RenderContext& ctx) override {
        glBindFramebuffer(GL_FRAMEBUFFER, ctx.outputFbo);

        glViewport(0, 0, outputWidth_, outputHeight_);

        glUseProgram(distortProgram_);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, ctx.inputTexture);

        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, currentPhaseTex_);

        // set uniforms: resolution, simResolution, distortion params, time, etc.

        drawFullscreenQuad();
    }

    void onParameterChange(const ParameterSet& p) override {
        // update member variables that control sim & distortion
        // if resolution changed, reallocate simulation textures
    }

private:
    void allocateSimulationTextures(SimResolution r);
    void runSimulationStep(float dt);

    GLuint simProgram_ = 0;
    GLuint distortProgram_ = 0;

    GLuint phaseTexA_ = 0;
    GLuint phaseTexB_ = 0;
    GLuint currentPhaseTex_ = 0;

    int simWidth_ = 256;
    int simHeight_ = 256;

    int outputWidth_  = 0;
    int outputHeight_ = 0;

    // parameters (cached copies of UI state)...
};
```

`runSimulationStep`:

* Bind an offscreen FBO with `phaseTexB_`.
* Use `simProgram_`, bind `phaseTexA_` as input.
* Draw fullscreen quad over `simWidth × simHeight`.
* Swap `phaseTexA_` and `phaseTexB_`.

---

## 8. Implementation Order

1. **Basic pass-through effect:**

   * Render input texture directly to output via one shader.

2. **Add simple drift (no oscillators):**

   * Procedural sine-based `D(u, v, t)` using one fragment shader.
   * Confirm warp integration works correctly.

3. **Add oscillator grid:**

   * Implement simulation textures, update shader, ping-pong logic.
   * Visualize phases as pure overlay (no distortion) to verify simulation.

4. **Use oscillator field for distortion:**

   * Replace procedural drift with sampled `θ` and gradients from `phaseTex`.
   * Fine-tune scaling and speed.

5. **Add ring kernel and parameters:**

   * Introduce ring parameters and more complex coupling.
   * Tune performance vs quality (simulation resolution, kernel radius).

6. **Optional enhancements:**

   * Log-polar mode.
   * Multiple presets (predefined parameter sets).
   * Save/load parameter sets from JSON or host preset system.

---

## 9. Performance Considerations

* Keep simulation resolution modest (e.g. 256×256) to maintain real-time performance.
* Use a fixed maximum kernel radius and a precomputed offset list to minimize per-pixel branching.
* If FPS drops:

  * Reduce `simWidth × simHeight`.
  * Reduce `Kernel Radius`.
  * Limit the number of rings or pre-bake `K(d)` into a small 1D LUT texture.

---

This specification describes how to implement the oscillatory distortion as a **self-contained C++ effect layer** that can sit inside an existing projection-mapping pipeline, taking a single input texture and outputting a warped, animated version driven by a Kuramoto oscillator grid.

