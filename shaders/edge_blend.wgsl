// Edge Blending Shader for Multi-Projector Setups
// Phase 2 Feature: Seamless projector overlap with gamma-corrected blending

struct VertexInput {
    @location(0) position: vec2<f32>,
    @location(1) texcoord: vec2<f32>,
}

struct VertexOutput {
    @builtin(position) position: vec4<f32>,
    @location(0) texcoord: vec2<f32>,
}

struct EdgeBlendUniforms {
    left_width: f32,      // Blend zone width (0.0-0.5)
    right_width: f32,
    top_width: f32,
    bottom_width: f32,
    gamma: f32,           // Blend curve gamma (typically 2.2)
    padding: vec3<f32>,   // Alignment padding
}

@group(0) @binding(0)
var t_input: texture_2d<f32>;

@group(0) @binding(1)
var s_input: sampler;

@group(1) @binding(0)
var<uniform> edge_blend: EdgeBlendUniforms;

@vertex
fn vs_main(in: VertexInput) -> VertexOutput {
    var out: VertexOutput;
    out.position = vec4<f32>(in.position, 0.0, 1.0);
    out.texcoord = in.texcoord;
    return out;
}

@fragment
fn fs_main(in: VertexOutput) -> @location(0) vec4<f32> {
    let color = textureSample(t_input, s_input, in.texcoord);

    // Calculate blend factors for each edge using smoothstep
    var blend_factor: f32 = 1.0;

    // Left edge blending
    if (edge_blend.left_width > 0.0) {
        let left_blend = smoothstep(0.0, edge_blend.left_width, in.texcoord.x);
        blend_factor = blend_factor * left_blend;
    }

    // Right edge blending
    if (edge_blend.right_width > 0.0) {
        let right_blend = smoothstep(1.0, 1.0 - edge_blend.right_width, in.texcoord.x);
        blend_factor = blend_factor * right_blend;
    }

    // Top edge blending
    if (edge_blend.top_width > 0.0) {
        let top_blend = smoothstep(0.0, edge_blend.top_width, in.texcoord.y);
        blend_factor = blend_factor * top_blend;
    }

    // Bottom edge blending
    if (edge_blend.bottom_width > 0.0) {
        let bottom_blend = smoothstep(1.0, 1.0 - edge_blend.bottom_width, in.texcoord.y);
        blend_factor = blend_factor * bottom_blend;
    }

    // Apply gamma correction to blend curve for perceptually linear blending
    let gamma_corrected = pow(blend_factor, edge_blend.gamma);

    // Apply blending to RGB, preserve alpha
    return vec4<f32>(color.rgb * gamma_corrected, color.a);
}
