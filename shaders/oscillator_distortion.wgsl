// Oscillator Distortion Shader (WGSL)
// Applies distortion from oscillator phase field to input texture

// Distortion parameters
struct DistortionParams {
    resolution: vec2<f32>,
    sim_resolution: vec2<f32>,

    distortion_amount: f32,
    distortion_scale: f32,
    distortion_speed: f32,
    overlay_opacity: f32,

    time: f32,
    color_mode: u32,  // 0=Off, 1=Rainbow, 2=BW, 3=Complementary
    use_log_polar: u32,
    _padding: f32,
}

@group(0) @binding(0)
var input_texture: texture_2d<f32>;

@group(0) @binding(1)
var input_sampler: sampler;

@group(0) @binding(2)
var phase_texture: texture_2d<f32>;

@group(0) @binding(3)
var phase_sampler: sampler;

@group(1) @binding(0)
var<uniform> params: DistortionParams;

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

// HSV to RGB conversion
fn hsv_to_rgb(hsv: vec3<f32>) -> vec3<f32> {
    let h = hsv.x * 6.0;
    let s = hsv.y;
    let v = hsv.z;

    let c = v * s;
    let x = c * (1.0 - abs(h % 2.0 - 1.0));
    let m = v - c;

    var rgb = vec3<f32>(0.0);

    if (h < 1.0) {
        rgb = vec3<f32>(c, x, 0.0);
    } else if (h < 2.0) {
        rgb = vec3<f32>(x, c, 0.0);
    } else if (h < 3.0) {
        rgb = vec3<f32>(0.0, c, x);
    } else if (h < 4.0) {
        rgb = vec3<f32>(0.0, x, c);
    } else if (h < 5.0) {
        rgb = vec3<f32>(x, 0.0, c);
    } else {
        rgb = vec3<f32>(c, 0.0, x);
    }

    return rgb + vec3<f32>(m);
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

    // Sample phase texture at current position
    let theta_c = textureSample(phase_texture, phase_sampler, uv).r;

    // Sample neighboring phases to compute gradient
    let theta_dx = textureSample(phase_texture, phase_sampler, uv + vec2<f32>(pixel_size.x, 0.0)).r;
    let theta_dy = textureSample(phase_texture, phase_sampler, uv + vec2<f32>(0.0, pixel_size.y)).r;

    // Compute phase gradient (normalized to [-π, π])
    let grad_x = normalize_phase_diff(theta_dx - theta_c);
    let grad_y = normalize_phase_diff(theta_dy - theta_c);
    let gradient = vec2<f32>(grad_x, grad_y);

    // Construct distortion vector from phase gradient
    // Direction from gradient, magnitude from sin(phase)
    var distortion_dir = vec2<f32>(0.0);
    let grad_len = length(gradient);
    if (grad_len > 0.001) {
        distortion_dir = gradient / grad_len;
    } else {
        // Fallback: use phase angle directly for swirl effect
        distortion_dir = vec2<f32>(cos(theta_c), sin(theta_c));
    }

    let distortion_mag = sin(theta_c);

    // Combine into final distortion vector
    var distortion = distortion_dir * distortion_mag;

    // Apply time modulation for drifting effect
    let phase_t = params.distortion_speed * params.time;
    let drift_mod = 0.5 + 0.5 * sin(phase_t + theta_c);
    distortion = distortion * drift_mod;

    // Scale displacement
    let displacement = distortion * params.distortion_amount * params.distortion_scale;

    // Apply distortion to UV coordinates
    let distorted_uv = uv + displacement;

    // Clamp UV coordinates to valid range
    let clamped_uv = clamp(distorted_uv, vec2<f32>(0.0), vec2<f32>(1.0));

    // Sample base color from input texture
    var base_color = textureSample(input_texture, input_sampler, clamped_uv);

    // Apply color overlay based on mode
    if (params.color_mode > 0u && params.overlay_opacity > 0.0) {
        var overlay_rgb = vec3<f32>(0.0);

        if (params.color_mode == 1u) {
            // Rainbow mode
            let PI = 3.14159265359;
            let TWO_PI = 6.28318530718;
            let hue = theta_c / TWO_PI;
            overlay_rgb = hsv_to_rgb(vec3<f32>(hue, 1.0, 1.0));
        } else if (params.color_mode == 2u) {
            // Black & White smooth mode
            let PI = 3.14159265359;
            let intensity = 0.5 + 0.5 * sin(theta_c);
            overlay_rgb = vec3<f32>(intensity);
        } else if (params.color_mode == 3u) {
            // Complementary colors mode
            let PI = 3.14159265359;
            let TWO_PI = 6.28318530718;
            let hue = (theta_c / TWO_PI + 0.5) % 1.0;
            overlay_rgb = hsv_to_rgb(vec3<f32>(hue, 0.8, 1.0));
        }

        let overlay_color = vec4<f32>(overlay_rgb, 1.0);
        base_color = mix(base_color, overlay_color, params.overlay_opacity);
    }

    return base_color;
}
