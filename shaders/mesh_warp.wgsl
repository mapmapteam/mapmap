// Mesh Warp Shader - Perspective Correction and Mesh Warping
//
// Supports:
// - Arbitrary mesh warping (quad, triangle, custom meshes)
// - Perspective-correct texture mapping
// - Per-vertex UV interpolation

struct VertexInput {
    @location(0) position: vec3<f32>,  // Output position (screen space)
    @location(1) tex_coords: vec2<f32>, // Texture coordinates
}

struct VertexOutput {
    @builtin(position) clip_position: vec4<f32>,
    @location(0) tex_coords: vec2<f32>,
    @location(1) @interpolate(perspective) perspective_tex_coords: vec2<f32>,
}

struct Uniforms {
    transform: mat4x4<f32>,  // Model-View-Projection transform
    opacity: f32,
    _padding: vec3<f32>,
}

@group(0) @binding(0)
var<uniform> uniforms: Uniforms;

@group(1) @binding(0)
var t_texture: texture_2d<f32>;

@group(1) @binding(1)
var s_sampler: sampler;

// Vertex shader
@vertex
fn vs_main(in: VertexInput) -> VertexOutput {
    var out: VertexOutput;

    // Transform vertex position
    out.clip_position = uniforms.transform * vec4<f32>(in.position, 1.0);

    // Pass through texture coordinates
    out.tex_coords = in.tex_coords;

    // For perspective-correct interpolation, multiply by w
    let w = out.clip_position.w;
    out.perspective_tex_coords = in.tex_coords * w;

    return out;
}

// Fragment shader
@fragment
fn fs_main(in: VertexOutput) -> @location(0) vec4<f32> {
    // Perspective-correct texture sampling
    // Divide interpolated tex_coords by interpolated w
    let tex_coords = in.perspective_tex_coords / in.clip_position.w;

    // Sample texture
    var color = textureSample(t_texture, s_sampler, tex_coords);

    // Apply opacity
    color.a *= uniforms.opacity;

    return color;
}

// Alternative: Simple (non-perspective-correct) version for flat mappings
@fragment
fn fs_main_simple(in: VertexOutput) -> @location(0) vec4<f32> {
    // Direct texture sampling (no perspective correction)
    var color = textureSample(t_texture, s_sampler, in.tex_coords);

    // Apply opacity
    color.a *= uniforms.opacity;

    return color;
}
