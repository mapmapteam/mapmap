// Oscillator Simulation Shader (WGSL)
// Kuramoto-based coupled oscillator simulation for distortion effects

// Simulation parameters
struct SimulationParams {
    sim_resolution: vec2<f32>,
    delta_time: f32,
    kernel_radius: f32,

    frequency_min: f32,
    frequency_max: f32,
    time: f32,
    kernel_shrink: f32,

    // Ring parameters (4 rings)
    ring_distances: vec4<f32>,
    ring_widths: vec4<f32>,
    ring_couplings: vec4<f32>,

    noise_amount: f32,
    use_log_polar: u32,
    _padding: vec2<f32>,
}

@group(0) @binding(0)
var phase_texture: texture_2d<f32>;

@group(0) @binding(1)
var phase_sampler: sampler;

@group(1) @binding(0)
var<uniform> params: SimulationParams;

struct VertexInput {
    @location(0) position: vec2<f32>,
    @location(1) texcoord: vec2<f32>,
}

struct VertexOutput {
    @builtin(position) clip_position: vec4<f32>,
    @location(0) texcoord: vec2<f32>,
}

@vertex
fn vs_main(in: VertexInput) -> VertexOutput {
    var out: VertexOutput;
    out.clip_position = vec4<f32>(in.position, 0.0, 1.0);
    out.texcoord = in.texcoord;
    return out;
}

// Simple hash function for pseudo-random noise
fn hash(p: vec2<f32>) -> f32 {
    let p3 = fract(vec3<f32>(p.x, p.y, p.x) * 0.1031);
    let p4 = dot(vec3<f32>(p3.x, p3.y, p3.z + 33.33), vec3<f32>(p3.y, p3.z, p3.x) + 33.33);
    return fract((p4.x + p4.y) * p3.z);
}

// Compute natural frequency for a cell
fn compute_frequency(uv: vec2<f32>) -> f32 {
    // Procedural pattern: 2D cosine
    let scale_x = 2.0;
    let scale_y = 2.0;
    let pattern = 0.5 * (1.0 + cos(6.28318530718 * scale_x * uv.x) * cos(6.28318530718 * scale_y * uv.y));

    // Base frequency
    let omega = params.frequency_min + (params.frequency_max - params.frequency_min) * pattern;

    // Add noise component
    let noise = (hash(uv * 1000.0) * 2.0 - 1.0) * params.noise_amount;

    // Convert from Hz to radians/s
    return (omega + noise) * 6.28318530718;
}

// Ring-based kernel function
fn kernel_function(dist: f32) -> f32 {
    let R = params.kernel_radius;
    var weight = 0.0;

    // Accumulate contributions from all 4 rings
    for (var i = 0u; i < 4u; i = i + 1u) {
        let distance_frac = params.ring_distances[i];
        let width_frac = params.ring_widths[i];
        let coupling = params.ring_couplings[i];

        // Convert to absolute ring parameters
        let r_k = distance_frac * R;
        let w_k = width_frac * R;

        // Difference of Gaussians profile
        let sigma1 = 0.5 * w_k;
        let sigma2 = w_k;

        let d_diff = dist - r_k;
        let dog = exp(-d_diff * d_diff / (2.0 * sigma1 * sigma1))
                - exp(-d_diff * d_diff / (2.0 * sigma2 * sigma2));

        // Negative coupling for sync/anti-sync
        weight += -coupling * dog;
    }

    return weight;
}

// Normalize phase difference to [-π, π]
fn normalize_phase_diff(diff: f32) -> f32 {
    let PI = 3.14159265359;
    var d = diff;
    while (d > PI) {
        d = d - 2.0 * PI;
    }
    while (d < -PI) {
        d = d + 2.0 * PI;
    }
    return d;
}

@fragment
fn fs_main(in: VertexOutput) -> @location(0) vec4<f32> {
    let uv = in.texcoord;
    let pixel_size = 1.0 / params.sim_resolution;

    // Read current phase
    let theta_i = textureSample(phase_texture, phase_sampler, uv).r;

    // Compute natural frequency
    let omega_i = compute_frequency(uv);

    // Compute coupling term by sampling neighbors
    var coupling_sum = 0.0;
    var weight_sum = 0.0;

    let max_offset = i32(ceil(params.kernel_radius));

    for (var dy = -max_offset; dy <= max_offset; dy = dy + 1) {
        for (var dx = -max_offset; dx <= max_offset; dx = dx + 1) {
            if (dx == 0 && dy == 0) {
                continue;
            }

            let offset = vec2<f32>(f32(dx), f32(dy));
            let dist = length(offset);

            if (dist > params.kernel_radius) {
                continue;
            }

            // Sample neighbor
            let neighbor_uv = uv + offset * pixel_size;

            // Clamp to valid range
            if (neighbor_uv.x < 0.0 || neighbor_uv.x > 1.0 ||
                neighbor_uv.y < 0.0 || neighbor_uv.y > 1.0) {
                continue;
            }

            let theta_j = textureSample(phase_texture, phase_sampler, neighbor_uv).r;

            // Kernel weight
            let K = kernel_function(dist);

            // Phase difference (normalized)
            let phase_diff = normalize_phase_diff(theta_j - theta_i);

            // Kuramoto coupling: K * sin(theta_j - theta_i)
            coupling_sum += K * sin(phase_diff);
            weight_sum += abs(K);
        }
    }

    // Normalize coupling term
    let coupling = coupling_sum / max(weight_sum, 1.0);

    // Euler integration: theta(t + dt) = theta(t) + [omega + coupling] * dt
    var new_theta = theta_i + (omega_i + coupling) * params.delta_time;

    // Wrap phase to [0, 2π)
    let TWO_PI = 6.28318530718;
    new_theta = new_theta - floor(new_theta / TWO_PI) * TWO_PI;

    // Store new phase (only in red channel)
    return vec4<f32>(new_theta, 0.0, 0.0, 1.0);
}
