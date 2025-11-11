// Blend Mode Shaders for MapMap
// Implements all standard blend modes for layer compositing

// ============================================================================
// Helper Functions
// ============================================================================

fn luminance(color: vec3<f32>) -> f32 {
    return dot(color, vec3<f32>(0.299, 0.587, 0.114));
}

fn set_luminance(color: vec3<f32>, lum: f32) -> vec3<f32> {
    let d = lum - luminance(color);
    return color + vec3<f32>(d, d, d);
}

fn clip_color(color: vec3<f32>) -> vec3<f32> {
    let lum = luminance(color);
    let n = min(min(color.r, color.g), color.b);
    let x = max(max(color.r, color.g), color.b);

    var result = color;
    if (n < 0.0) {
        result = lum + (result - lum) * lum / (lum - n);
    }
    if (x > 1.0) {
        result = lum + (result - lum) * (1.0 - lum) / (x - lum);
    }
    return result;
}

// ============================================================================
// Blend Mode Functions
// ============================================================================

/// Normal alpha blending
fn blend_normal(base: vec4<f32>, blend: vec4<f32>) -> vec4<f32> {
    let alpha = blend.a;
    return vec4<f32>(
        mix(base.rgb, blend.rgb, alpha),
        base.a + blend.a * (1.0 - base.a)
    );
}

/// Add (lighten)
fn blend_add(base: vec4<f32>, blend: vec4<f32>) -> vec4<f32> {
    return vec4<f32>(
        min(base.rgb + blend.rgb * blend.a, vec3<f32>(1.0)),
        base.a + blend.a * (1.0 - base.a)
    );
}

/// Subtract (darken)
fn blend_subtract(base: vec4<f32>, blend: vec4<f32>) -> vec4<f32> {
    return vec4<f32>(
        max(base.rgb - blend.rgb * blend.a, vec3<f32>(0.0)),
        base.a + blend.a * (1.0 - base.a)
    );
}

/// Multiply (darken)
fn blend_multiply(base: vec4<f32>, blend: vec4<f32>) -> vec4<f32> {
    return vec4<f32>(
        mix(base.rgb, base.rgb * blend.rgb, blend.a),
        base.a + blend.a * (1.0 - base.a)
    );
}

/// Screen (lighten)
fn blend_screen(base: vec4<f32>, blend: vec4<f32>) -> vec4<f32> {
    let screen = 1.0 - (1.0 - base.rgb) * (1.0 - blend.rgb);
    return vec4<f32>(
        mix(base.rgb, screen, blend.a),
        base.a + blend.a * (1.0 - base.a)
    );
}

/// Overlay (combination of multiply and screen)
fn blend_overlay(base: vec4<f32>, blend: vec4<f32>) -> vec4<f32> {
    var result: vec3<f32>;

    if (base.r < 0.5) {
        result.r = 2.0 * base.r * blend.r;
    } else {
        result.r = 1.0 - 2.0 * (1.0 - base.r) * (1.0 - blend.r);
    }

    if (base.g < 0.5) {
        result.g = 2.0 * base.g * blend.g;
    } else {
        result.g = 1.0 - 2.0 * (1.0 - base.g) * (1.0 - blend.g);
    }

    if (base.b < 0.5) {
        result.b = 2.0 * base.b * blend.b;
    } else {
        result.b = 1.0 - 2.0 * (1.0 - base.b) * (1.0 - blend.b);
    }

    return vec4<f32>(
        mix(base.rgb, result, blend.a),
        base.a + blend.a * (1.0 - base.a)
    );
}

/// Soft light
fn blend_soft_light(base: vec4<f32>, blend: vec4<f32>) -> vec4<f32> {
    var result: vec3<f32>;

    // Red channel
    if (blend.r < 0.5) {
        result.r = base.r - (1.0 - 2.0 * blend.r) * base.r * (1.0 - base.r);
    } else {
        var d: f32;
        if (base.r < 0.25) {
            d = ((16.0 * base.r - 12.0) * base.r + 4.0) * base.r;
        } else {
            d = sqrt(base.r);
        }
        result.r = base.r + (2.0 * blend.r - 1.0) * (d - base.r);
    }

    // Green channel
    if (blend.g < 0.5) {
        result.g = base.g - (1.0 - 2.0 * blend.g) * base.g * (1.0 - base.g);
    } else {
        var d: f32;
        if (base.g < 0.25) {
            d = ((16.0 * base.g - 12.0) * base.g + 4.0) * base.g;
        } else {
            d = sqrt(base.g);
        }
        result.g = base.g + (2.0 * blend.g - 1.0) * (d - base.g);
    }

    // Blue channel
    if (blend.b < 0.5) {
        result.b = base.b - (1.0 - 2.0 * blend.b) * base.b * (1.0 - base.b);
    } else {
        var d: f32;
        if (base.b < 0.25) {
            d = ((16.0 * base.b - 12.0) * base.b + 4.0) * base.b;
        } else {
            d = sqrt(base.b);
        }
        result.b = base.b + (2.0 * blend.b - 1.0) * (d - base.b);
    }

    return vec4<f32>(
        mix(base.rgb, result, blend.a),
        base.a + blend.a * (1.0 - base.a)
    );
}

/// Hard light
fn blend_hard_light(base: vec4<f32>, blend: vec4<f32>) -> vec4<f32> {
    var result: vec3<f32>;

    if (blend.r < 0.5) {
        result.r = 2.0 * base.r * blend.r;
    } else {
        result.r = 1.0 - 2.0 * (1.0 - base.r) * (1.0 - blend.r);
    }

    if (blend.g < 0.5) {
        result.g = 2.0 * base.g * blend.g;
    } else {
        result.g = 1.0 - 2.0 * (1.0 - base.g) * (1.0 - blend.g);
    }

    if (blend.b < 0.5) {
        result.b = 2.0 * base.b * blend.b;
    } else {
        result.b = 1.0 - 2.0 * (1.0 - base.b) * (1.0 - blend.b);
    }

    return vec4<f32>(
        mix(base.rgb, result, blend.a),
        base.a + blend.a * (1.0 - base.a)
    );
}

/// Lighten (max)
fn blend_lighten(base: vec4<f32>, blend: vec4<f32>) -> vec4<f32> {
    return vec4<f32>(
        mix(base.rgb, max(base.rgb, blend.rgb), blend.a),
        base.a + blend.a * (1.0 - base.a)
    );
}

/// Darken (min)
fn blend_darken(base: vec4<f32>, blend: vec4<f32>) -> vec4<f32> {
    return vec4<f32>(
        mix(base.rgb, min(base.rgb, blend.rgb), blend.a),
        base.a + blend.a * (1.0 - base.a)
    );
}

/// Color dodge
fn blend_color_dodge(base: vec4<f32>, blend: vec4<f32>) -> vec4<f32> {
    var result: vec3<f32>;

    if (blend.r == 1.0) {
        result.r = 1.0;
    } else {
        result.r = min(1.0, base.r / (1.0 - blend.r));
    }

    if (blend.g == 1.0) {
        result.g = 1.0;
    } else {
        result.g = min(1.0, base.g / (1.0 - blend.g));
    }

    if (blend.b == 1.0) {
        result.b = 1.0;
    } else {
        result.b = min(1.0, base.b / (1.0 - blend.b));
    }

    return vec4<f32>(
        mix(base.rgb, result, blend.a),
        base.a + blend.a * (1.0 - base.a)
    );
}

/// Color burn
fn blend_color_burn(base: vec4<f32>, blend: vec4<f32>) -> vec4<f32> {
    var result: vec3<f32>;

    if (blend.r == 0.0) {
        result.r = 0.0;
    } else {
        result.r = max(0.0, 1.0 - (1.0 - base.r) / blend.r);
    }

    if (blend.g == 0.0) {
        result.g = 0.0;
    } else {
        result.g = max(0.0, 1.0 - (1.0 - base.g) / blend.g);
    }

    if (blend.b == 0.0) {
        result.b = 0.0;
    } else {
        result.b = max(0.0, 1.0 - (1.0 - base.b) / blend.b);
    }

    return vec4<f32>(
        mix(base.rgb, result, blend.a),
        base.a + blend.a * (1.0 - base.a)
    );
}

/// Difference
fn blend_difference(base: vec4<f32>, blend: vec4<f32>) -> vec4<f32> {
    return vec4<f32>(
        mix(base.rgb, abs(base.rgb - blend.rgb), blend.a),
        base.a + blend.a * (1.0 - base.a)
    );
}

/// Exclusion
fn blend_exclusion(base: vec4<f32>, blend: vec4<f32>) -> vec4<f32> {
    let result = base.rgb + blend.rgb - 2.0 * base.rgb * blend.rgb;
    return vec4<f32>(
        mix(base.rgb, result, blend.a),
        base.a + blend.a * (1.0 - base.a)
    );
}

// ============================================================================
// Compositing Shader
// ============================================================================

struct VertexInput {
    @location(0) position: vec3<f32>,
    @location(1) uv: vec2<f32>,
}

struct VertexOutput {
    @builtin(position) clip_position: vec4<f32>,
    @location(0) uv: vec2<f32>,
}

@group(0) @binding(0)
var base_texture: texture_2d<f32>;
@group(0) @binding(1)
var base_sampler: sampler;
@group(0) @binding(2)
var blend_texture: texture_2d<f32>;
@group(0) @binding(3)
var blend_sampler: sampler;

struct CompositeParams {
    blend_mode: u32,     // 0=Normal, 1=Add, 2=Subtract, 3=Multiply, etc.
    opacity: f32,
    _padding: vec2<f32>,
}

@group(1) @binding(0)
var<uniform> params: CompositeParams;

@vertex
fn vs_main(in: VertexInput) -> VertexOutput {
    var out: VertexOutput;
    out.clip_position = vec4<f32>(in.position, 1.0);
    out.uv = in.uv;
    return out;
}

@fragment
fn fs_main(in: VertexOutput) -> @location(0) vec4<f32> {
    let base = textureSample(base_texture, base_sampler, in.uv);
    var blend = textureSample(blend_texture, blend_sampler, in.uv);

    // Apply opacity
    blend = vec4<f32>(blend.rgb, blend.a * params.opacity);

    // Apply blend mode
    var result: vec4<f32>;

    switch (params.blend_mode) {
        case 0u: { result = blend_normal(base, blend); }
        case 1u: { result = blend_add(base, blend); }
        case 2u: { result = blend_subtract(base, blend); }
        case 3u: { result = blend_multiply(base, blend); }
        case 4u: { result = blend_screen(base, blend); }
        case 5u: { result = blend_overlay(base, blend); }
        case 6u: { result = blend_soft_light(base, blend); }
        case 7u: { result = blend_hard_light(base, blend); }
        case 8u: { result = blend_lighten(base, blend); }
        case 9u: { result = blend_darken(base, blend); }
        case 10u: { result = blend_color_dodge(base, blend); }
        case 11u: { result = blend_color_burn(base, blend); }
        case 12u: { result = blend_difference(base, blend); }
        case 13u: { result = blend_exclusion(base, blend); }
        default: { result = blend_normal(base, blend); }
    }

    return result;
}
