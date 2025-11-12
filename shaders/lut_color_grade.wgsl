// LUT (Look-Up Table) Color Grading Shader
// Phase 3: Effects Pipeline
//
// Applies 3D LUT for advanced color correction and grading
// Supports 32x32x32 and 64x64x64 LUT sizes

struct VertexInput {
    @location(0) position: vec2<f32>,
    @location(1) uv: vec2<f32>,
}

struct VertexOutput {
    @builtin(position) position: vec4<f32>,
    @location(0) uv: vec2<f32>,
}

struct Uniforms {
    lut_size: f32,
    intensity: f32,  // Blend between original (0.0) and LUT (1.0)
    _padding: vec2<f32>,
}

@group(0) @binding(0) var<uniform> uniforms: Uniforms;
@group(0) @binding(1) var input_texture: texture_2d<f32>;
@group(0) @binding(2) var input_sampler: sampler;
@group(0) @binding(3) var lut_texture: texture_2d<f32>;
@group(0) @binding(4) var lut_sampler: sampler;

@vertex
fn vs_main(in: VertexInput) -> VertexOutput {
    var out: VertexOutput;
    out.position = vec4<f32>(in.position, 0.0, 1.0);
    out.uv = in.uv;
    return out;
}

// Apply 3D LUT using 2D texture atlas lookup with trilinear interpolation
@fragment
fn fs_main(in: VertexOutput) -> @location(0) vec4<f32> {
    // Sample input color
    let input_color = textureSample(input_texture, input_sampler, in.uv);

    // Clamp input to [0, 1] range
    let clamped = clamp(input_color.rgb, vec3<f32>(0.0), vec3<f32>(1.0));

    // Apply LUT
    let lut_color = sample_lut_trilinear(clamped);

    // Blend between original and LUT
    let output_rgb = mix(input_color.rgb, lut_color, uniforms.intensity);

    return vec4<f32>(output_rgb, input_color.a);
}

// Sample 3D LUT with trilinear interpolation
// LUT is stored as 2D texture atlas: width=size, height=size*size
fn sample_lut_trilinear(color: vec3<f32>) -> vec3<f32> {
    let size = uniforms.lut_size;
    let size_minus_one = size - 1.0;

    // Scale color to LUT coordinates
    let scaled = color * size_minus_one;

    // Get integer coordinates (cell indices)
    let cell = floor(scaled);
    let r0 = cell.r;
    let g0 = cell.g;
    let b0 = cell.b;

    let r1 = min(r0 + 1.0, size_minus_one);
    let g1 = min(g0 + 1.0, size_minus_one);
    let b1 = min(b0 + 1.0, size_minus_one);

    // Get fractional parts for interpolation
    let frac = fract(scaled);

    // Sample 8 corners of the cube
    let c000 = sample_lut_cell(r0, g0, b0);
    let c001 = sample_lut_cell(r0, g0, b1);
    let c010 = sample_lut_cell(r0, g1, b0);
    let c011 = sample_lut_cell(r0, g1, b1);
    let c100 = sample_lut_cell(r1, g0, b0);
    let c101 = sample_lut_cell(r1, g0, b1);
    let c110 = sample_lut_cell(r1, g1, b0);
    let c111 = sample_lut_cell(r1, g1, b1);

    // Trilinear interpolation
    // Interpolate along R axis
    let c00 = mix(c000, c100, frac.r);
    let c01 = mix(c001, c101, frac.r);
    let c10 = mix(c010, c110, frac.r);
    let c11 = mix(c011, c111, frac.r);

    // Interpolate along G axis
    let c0 = mix(c00, c10, frac.g);
    let c1 = mix(c01, c11, frac.g);

    // Interpolate along B axis
    return mix(c0, c1, frac.b);
}

// Sample a specific cell from the 2D LUT atlas
// LUT layout: horizontal=R, vertical stacks of (G,B) planes
fn sample_lut_cell(r: f32, g: f32, b: f32) -> vec3<f32> {
    let size = uniforms.lut_size;

    // Convert 3D coordinates to 2D atlas coordinates
    // Each B-slice is a size x size grid
    let u = (r + 0.5) / size;
    let v = (b * size + g + 0.5) / (size * size);

    let sample = textureSample(lut_texture, lut_sampler, vec2<f32>(u, v));
    return sample.rgb;
}

// Alternative: Nearest neighbor sampling (faster, lower quality)
@fragment
fn fs_main_nearest(in: VertexOutput) -> @location(0) vec4<f32> {
    let input_color = textureSample(input_texture, input_sampler, in.uv);
    let clamped = clamp(input_color.rgb, vec3<f32>(0.0), vec3<f32>(1.0));

    let size = uniforms.lut_size;
    let size_minus_one = size - 1.0;

    // Scale and round to nearest cell
    let scaled = round(clamped * size_minus_one);

    // Convert to atlas UV
    let u = (scaled.r + 0.5) / size;
    let v = (scaled.b * size + scaled.g + 0.5) / (size * size);

    let lut_color = textureSample(lut_texture, lut_sampler, vec2<f32>(u, v)).rgb;
    let output_rgb = mix(input_color.rgb, lut_color, uniforms.intensity);

    return vec4<f32>(output_rgb, input_color.a);
}

// Alternative: Tetrahedral interpolation (higher quality, more expensive)
// This provides better color accuracy than trilinear for the same number of samples
@fragment
fn fs_main_tetrahedral(in: VertexOutput) -> @location(0) vec4<f32> {
    let input_color = textureSample(input_texture, input_sampler, in.uv);
    let clamped = clamp(input_color.rgb, vec3<f32>(0.0), vec3<f32>(1.0));

    let size = uniforms.lut_size;
    let size_minus_one = size - 1.0;

    let scaled = clamped * size_minus_one;
    let cell = floor(scaled);
    let frac = fract(scaled);

    let r0 = cell.r;
    let g0 = cell.g;
    let b0 = cell.b;
    let r1 = min(r0 + 1.0, size_minus_one);
    let g1 = min(g0 + 1.0, size_minus_one);
    let b1 = min(b0 + 1.0, size_minus_one);

    // Sample corners
    let c000 = sample_lut_cell(r0, g0, b0);
    let c111 = sample_lut_cell(r1, g1, b1);

    var result: vec3<f32>;

    // Determine which tetrahedron we're in based on fractional coordinates
    if (frac.r > frac.g) {
        if (frac.g > frac.b) {
            // Tetrahedron 1: r > g > b
            let c100 = sample_lut_cell(r1, g0, b0);
            let c110 = sample_lut_cell(r1, g1, b0);
            result = c000 + frac.r * (c100 - c000) + frac.g * (c110 - c100) + frac.b * (c111 - c110);
        } else if (frac.r > frac.b) {
            // Tetrahedron 2: r > b > g
            let c100 = sample_lut_cell(r1, g0, b0);
            let c101 = sample_lut_cell(r1, g0, b1);
            result = c000 + frac.r * (c100 - c000) + frac.b * (c101 - c100) + frac.g * (c111 - c101);
        } else {
            // Tetrahedron 3: b > r > g
            let c001 = sample_lut_cell(r0, g0, b1);
            let c101 = sample_lut_cell(r1, g0, b1);
            result = c000 + frac.b * (c001 - c000) + frac.r * (c101 - c001) + frac.g * (c111 - c101);
        }
    } else {
        if (frac.b > frac.g) {
            // Tetrahedron 4: b > g > r
            let c001 = sample_lut_cell(r0, g0, b1);
            let c011 = sample_lut_cell(r0, g1, b1);
            result = c000 + frac.b * (c001 - c000) + frac.g * (c011 - c001) + frac.r * (c111 - c011);
        } else if (frac.b > frac.r) {
            // Tetrahedron 5: g > b > r
            let c010 = sample_lut_cell(r0, g1, b0);
            let c011 = sample_lut_cell(r0, g1, b1);
            result = c000 + frac.g * (c010 - c000) + frac.b * (c011 - c010) + frac.r * (c111 - c011);
        } else {
            // Tetrahedron 6: g > r > b
            let c010 = sample_lut_cell(r0, g1, b0);
            let c110 = sample_lut_cell(r1, g1, b0);
            result = c000 + frac.g * (c010 - c000) + frac.r * (c110 - c010) + frac.b * (c111 - c110);
        }
    }

    let output_rgb = mix(input_color.rgb, result, uniforms.intensity);
    return vec4<f32>(output_rgb, input_color.a);
}
