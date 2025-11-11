// Color Calibration Shader for Per-Output Color Correction
// Phase 2 Feature: Brightness, contrast, gamma, color temperature, and saturation control

struct VertexInput {
    @location(0) position: vec2<f32>,
    @location(1) texcoord: vec2<f32>,
}

struct VertexOutput {
    @builtin(position) position: vec4<f32>,
    @location(0) texcoord: vec2<f32>,
}

struct ColorCalibration {
    brightness: f32,       // -1.0 to 1.0
    contrast: f32,         // 0.0 to 2.0
    gamma_r: f32,          // Red channel gamma
    gamma_g: f32,          // Green channel gamma
    gamma_b: f32,          // Blue channel gamma
    color_temp: f32,       // 2000K to 10000K
    saturation: f32,       // 0.0 to 2.0
    padding: f32,          // Alignment padding
}

@group(0) @binding(0)
var t_input: texture_2d<f32>;

@group(0) @binding(1)
var s_input: sampler;

@group(1) @binding(0)
var<uniform> calibration: ColorCalibration;

@vertex
fn vs_main(in: VertexInput) -> VertexOutput {
    var out: VertexOutput;
    out.position = vec4<f32>(in.position, 0.0, 1.0);
    out.texcoord = in.texcoord;
    return out;
}

// Convert color temperature (in Kelvin) to RGB multiplier
fn color_temp_to_rgb(kelvin: f32) -> vec3<f32> {
    let temp = kelvin / 100.0;
    var r: f32;
    var g: f32;
    var b: f32;

    // Red
    if (temp <= 66.0) {
        r = 1.0;
    } else {
        r = clamp((329.698727446 * pow(temp - 60.0, -0.1332047592)) / 255.0, 0.0, 1.0);
    }

    // Green
    if (temp <= 66.0) {
        g = clamp((99.4708025861 * log(temp) - 161.1195681661) / 255.0, 0.0, 1.0);
    } else {
        g = clamp((288.1221695283 * pow(temp - 60.0, -0.0755148492)) / 255.0, 0.0, 1.0);
    }

    // Blue
    if (temp >= 66.0) {
        b = 1.0;
    } else if (temp <= 19.0) {
        b = 0.0;
    } else {
        b = clamp((138.5177312231 * log(temp - 10.0) - 305.0447927307) / 255.0, 0.0, 1.0);
    }

    return vec3<f32>(r, g, b);
}

@fragment
fn fs_main(in: VertexOutput) -> @location(0) vec4<f32> {
    var color = textureSample(t_input, s_input, in.texcoord);

    // Apply brightness (additive)
    var adjusted = color.rgb + vec3<f32>(calibration.brightness);

    // Apply contrast (pivot around 0.5 for mid-gray preservation)
    adjusted = (adjusted - 0.5) * calibration.contrast + 0.5;

    // Apply per-channel gamma correction
    let gamma_vec = vec3<f32>(calibration.gamma_r, calibration.gamma_g, calibration.gamma_b);
    adjusted = pow(max(adjusted, vec3<f32>(0.0)), 1.0 / gamma_vec);

    // Apply color temperature shift
    let temp_shift = color_temp_to_rgb(calibration.color_temp);
    adjusted = adjusted * temp_shift;

    // Apply saturation (preserve luminance)
    let luminance = dot(adjusted, vec3<f32>(0.299, 0.587, 0.114));
    adjusted = mix(vec3<f32>(luminance), adjusted, calibration.saturation);

    // Clamp final output to valid range
    adjusted = clamp(adjusted, vec3<f32>(0.0), vec3<f32>(1.0));

    return vec4<f32>(adjusted, color.a);
}
