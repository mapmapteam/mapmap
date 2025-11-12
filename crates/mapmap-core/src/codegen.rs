//! WGSL Code Generation from Shader Graphs
//!
//! Phase 3: Effects Pipeline
//! Generates WGSL shader code from node-based shader graphs

use crate::shader_graph::{
    ShaderGraph, ShaderNode, NodeType, NodeId, DataType,
    InputSocket, ParameterValue,
};
use std::collections::HashSet;
use std::fmt::Write;

/// WGSL code generator error
#[derive(Debug, thiserror::Error)]
pub enum CodegenError {
    #[error("Graph validation failed: {0}")]
    ValidationError(String),

    #[error("No output node found in graph")]
    NoOutputNode,

    #[error("Node {0} not found")]
    NodeNotFound(NodeId),

    #[error("Cyclic dependency detected")]
    CyclicDependency,

    #[error("Type mismatch: cannot connect {0} to {1}")]
    TypeMismatch(String, String),

    #[error("Code generation failed: {0}")]
    GenerationError(String),
}

/// Result type for codegen operations
pub type Result<T> = std::result::Result<T, CodegenError>;

/// WGSL code generator
pub struct WGSLCodegen {
    graph: ShaderGraph,
    generated_functions: HashSet<String>,
    node_execution_order: Vec<NodeId>,
}

impl WGSLCodegen {
    /// Create a new WGSL code generator
    pub fn new(graph: ShaderGraph) -> Self {
        Self {
            graph,
            generated_functions: HashSet::new(),
            node_execution_order: Vec::new(),
        }
    }

    /// Generate complete WGSL shader code
    pub fn generate(&mut self) -> Result<String> {
        // Validate graph
        self.graph.validate()
            .map_err(|errors| CodegenError::ValidationError(errors.join(", ")))?;

        // Determine execution order (topological sort)
        self.compute_execution_order()?;

        let mut code = String::new();

        // Generate shader structure
        writeln!(code, "// Auto-generated WGSL shader from shader graph").unwrap();
        writeln!(code, "// Graph: {}\n", self.graph.name).unwrap();

        // Generate uniforms
        self.generate_uniforms(&mut code)?;

        // Generate texture bindings
        self.generate_texture_bindings(&mut code)?;

        // Generate helper functions
        self.generate_helper_functions(&mut code)?;

        // Generate main fragment shader
        self.generate_fragment_shader(&mut code)?;

        Ok(code)
    }

    /// Compute node execution order using topological sort
    fn compute_execution_order(&mut self) -> Result<()> {
        let output_node = self.graph.output_node()
            .ok_or(CodegenError::NoOutputNode)?;

        let mut visited = HashSet::new();
        let mut stack = HashSet::new();
        let mut order = Vec::new();

        self.visit_node(output_node.id, &mut visited, &mut stack, &mut order)?;

        // Reverse to get correct execution order
        order.reverse();
        self.node_execution_order = order;

        Ok(())
    }

    /// Visit node for topological sort (DFS)
    fn visit_node(
        &self,
        node_id: NodeId,
        visited: &mut HashSet<NodeId>,
        stack: &mut HashSet<NodeId>,
        order: &mut Vec<NodeId>,
    ) -> Result<()> {
        if stack.contains(&node_id) {
            return Err(CodegenError::CyclicDependency);
        }

        if visited.contains(&node_id) {
            return Ok(());
        }

        stack.insert(node_id);

        let node = self.graph.nodes.get(&node_id)
            .ok_or(CodegenError::NodeNotFound(node_id))?;

        // Visit all input dependencies
        for input in &node.inputs {
            if let Some((source_node, _)) = &input.connected_output {
                self.visit_node(*source_node, visited, stack, order)?;
            }
        }

        stack.remove(&node_id);
        visited.insert(node_id);
        order.push(node_id);

        Ok(())
    }

    /// Generate uniform declarations
    fn generate_uniforms(&self, code: &mut String) -> Result<()> {
        writeln!(code, "// Uniforms").unwrap();
        writeln!(code, "struct Uniforms {{").unwrap();
        writeln!(code, "    time: f32,").unwrap();
        writeln!(code, "    resolution: vec2<f32>,").unwrap();
        writeln!(code, "    mouse: vec2<f32>,").unwrap();

        // Add parameter uniforms
        for node_id in &self.node_execution_order {
            if let Some(node) = self.graph.nodes.get(node_id) {
                if node.node_type == NodeType::ParameterInput {
                    for (name, _) in &node.parameters {
                        writeln!(code, "    param_{}: f32,", name).unwrap();
                    }
                }
            }
        }

        writeln!(code, "}}").unwrap();
        writeln!(code, "@group(0) @binding(0) var<uniform> uniforms: Uniforms;\n").unwrap();

        Ok(())
    }

    /// Generate texture binding declarations
    fn generate_texture_bindings(&self, code: &mut String) -> Result<()> {
        writeln!(code, "// Textures").unwrap();

        let mut texture_count = 0;
        for node_id in &self.node_execution_order {
            if let Some(node) = self.graph.nodes.get(node_id) {
                if node.node_type == NodeType::TextureInput {
                    let binding = 1 + texture_count;
                    writeln!(code,
                        "@group(0) @binding({}) var texture_{}: texture_2d<f32>;",
                        binding, node.id
                    ).unwrap();
                    writeln!(code,
                        "@group(0) @binding({}) var sampler_{}: sampler;",
                        binding + 1, node.id
                    ).unwrap();
                    texture_count += 2;
                }
            }
        }

        writeln!(code).unwrap();
        Ok(())
    }

    /// Generate helper functions for node operations
    fn generate_helper_functions(&mut self, code: &mut String) -> Result<()> {
        writeln!(code, "// Helper Functions\n").unwrap();

        // Generate functions for complex node types
        for node_id in &self.node_execution_order.clone() {
            if let Some(node) = self.graph.nodes.get(node_id) {
                match node.node_type {
                    NodeType::Blur => self.generate_blur_function(code)?,
                    NodeType::ChromaticAberration => self.generate_chromatic_aberration_function(code)?,
                    NodeType::EdgeDetect => self.generate_edge_detect_function(code)?,
                    NodeType::Kaleidoscope => self.generate_kaleidoscope_function(code)?,
                    NodeType::HSVToRGB => self.generate_hsv_to_rgb_function(code)?,
                    NodeType::RGBToHSV => self.generate_rgb_to_hsv_function(code)?,
                    _ => {}
                }
            }
        }

        Ok(())
    }

    /// Generate main fragment shader
    fn generate_fragment_shader(&self, code: &mut String) -> Result<()> {
        writeln!(code, "// Fragment Shader").unwrap();
        writeln!(code, "@fragment").unwrap();
        writeln!(code, "fn fs_main(").unwrap();
        writeln!(code, "    @location(0) uv: vec2<f32>,").unwrap();
        writeln!(code, ") -> @location(0) vec4<f32> {{").unwrap();

        // Generate variable declarations and computations
        for node_id in &self.node_execution_order {
            if let Some(node) = self.graph.nodes.get(node_id) {
                self.generate_node_code(code, node)?;
            }
        }

        // Return output
        let output_node = self.graph.output_node().unwrap();
        let output_input = &output_node.inputs[0];

        if let Some((source_node, output_name)) = &output_input.connected_output {
            writeln!(code, "    return node_{}_{};", source_node, output_name.to_lowercase()).unwrap();
        } else if let Some(default) = &output_input.default_value {
            writeln!(code, "    return vec4<f32>({}, {}, {}, {});",
                default.x, default.y, default.z, default.w).unwrap();
        }

        writeln!(code, "}}").unwrap();

        Ok(())
    }

    /// Generate code for a specific node
    fn generate_node_code(&self, code: &mut String, node: &ShaderNode) -> Result<()> {
        match node.node_type {
            NodeType::UVInput => {
                writeln!(code, "    let node_{}_uv = uv;", node.id).unwrap();
            }

            NodeType::TimeInput => {
                writeln!(code, "    let node_{}_time = uniforms.time;", node.id).unwrap();
            }

            NodeType::TextureInput => {
                // Texture binding already handled in generate_texture_bindings
            }

            NodeType::TextureSample => {
                let tex_input = &node.inputs[0];
                let uv_input = &node.inputs[1];

                let tex_var = self.get_input_variable(tex_input)?;
                let uv_var = self.get_input_variable(uv_input)?;

                writeln!(code,
                    "    let node_{}_color = textureSample({}, {}, {});",
                    node.id, tex_var, tex_var.replace("texture", "sampler"), uv_var
                ).unwrap();
                writeln!(code,
                    "    let node_{}_alpha = node_{}_color.a;",
                    node.id, node.id
                ).unwrap();
            }

            NodeType::Add | NodeType::Subtract | NodeType::Multiply | NodeType::Divide => {
                self.generate_math_op(code, node)?;
            }

            NodeType::Sin | NodeType::Cos => {
                self.generate_trig_op(code, node)?;
            }

            NodeType::Mix => {
                self.generate_mix_op(code, node)?;
            }

            NodeType::Brightness => {
                self.generate_brightness_op(code, node)?;
            }

            NodeType::Contrast => {
                self.generate_contrast_op(code, node)?;
            }

            NodeType::Desaturate => {
                self.generate_desaturate_op(code, node)?;
            }

            NodeType::UVTransform => {
                self.generate_uv_transform(code, node)?;
            }

            NodeType::AudioInput => {
                // Audio values will be passed as uniforms
                writeln!(code, "    let node_{}_value = uniforms.audio_value;", node.id).unwrap();
            }

            NodeType::Output => {
                // Output node doesn't generate code, just connects
            }

            _ => {
                // Placeholder for unimplemented nodes
                writeln!(code, "    // TODO: Implement {}", node.node_type.display_name()).unwrap();
            }
        }

        Ok(())
    }

    /// Generate math operation code
    fn generate_math_op(&self, code: &mut String, node: &ShaderNode) -> Result<()> {
        let a = self.get_input_variable(&node.inputs[0])?;
        let b = self.get_input_variable(&node.inputs[1])?;

        let op = match node.node_type {
            NodeType::Add => "+",
            NodeType::Subtract => "-",
            NodeType::Multiply => "*",
            NodeType::Divide => "/",
            _ => return Err(CodegenError::GenerationError("Invalid math op".to_string())),
        };

        writeln!(code, "    let node_{}_result = {} {} {};", node.id, a, op, b).unwrap();

        Ok(())
    }

    /// Generate trigonometric operation code
    fn generate_trig_op(&self, code: &mut String, node: &ShaderNode) -> Result<()> {
        let input = self.get_input_variable(&node.inputs[0])?;

        let func = match node.node_type {
            NodeType::Sin => "sin",
            NodeType::Cos => "cos",
            _ => return Err(CodegenError::GenerationError("Invalid trig op".to_string())),
        };

        writeln!(code, "    let node_{}_result = {}({});", node.id, func, input).unwrap();

        Ok(())
    }

    /// Generate mix operation code
    fn generate_mix_op(&self, code: &mut String, node: &ShaderNode) -> Result<()> {
        let a = self.get_input_variable(&node.inputs[0])?;
        let b = self.get_input_variable(&node.inputs[1])?;
        let t = self.get_input_variable(&node.inputs[2])?;

        writeln!(code, "    let node_{}_result = mix({}, {}, {});", node.id, a, b, t).unwrap();

        Ok(())
    }

    /// Generate brightness operation code
    fn generate_brightness_op(&self, code: &mut String, node: &ShaderNode) -> Result<()> {
        let color = self.get_input_variable(&node.inputs[0])?;
        let amount = node.parameters.get("amount")
            .map(|v| format!("{}", v))
            .unwrap_or_else(|| "0.0".to_string());

        writeln!(code,
            "    let node_{}_result = {} + vec4<f32>({}, {}, {}, 0.0);",
            node.id, color, amount, amount, amount
        ).unwrap();

        Ok(())
    }

    /// Generate contrast operation code
    fn generate_contrast_op(&self, code: &mut String, node: &ShaderNode) -> Result<()> {
        let color = self.get_input_variable(&node.inputs[0])?;
        let amount = node.parameters.get("amount")
            .map(|v| format!("{}", v))
            .unwrap_or_else(|| "1.0".to_string());

        writeln!(code,
            "    let node_{}_result = ({} - 0.5) * {} + 0.5;",
            node.id, color, amount
        ).unwrap();

        Ok(())
    }

    /// Generate desaturate operation code
    fn generate_desaturate_op(&self, code: &mut String, node: &ShaderNode) -> Result<()> {
        let color = self.get_input_variable(&node.inputs[0])?;

        writeln!(code, "    let gray = dot({}.rgb, vec3<f32>(0.299, 0.587, 0.114));", color).unwrap();
        writeln!(code, "    let node_{}_result = vec4<f32>(vec3<f32>(gray), {}.a);", node.id, color).unwrap();

        Ok(())
    }

    /// Generate UV transform code
    fn generate_uv_transform(&self, code: &mut String, node: &ShaderNode) -> Result<()> {
        let uv = self.get_input_variable(&node.inputs[0])?;
        // TODO: Add scale, rotation, translation parameters

        writeln!(code, "    let node_{}_uv = {};", node.id, uv).unwrap();

        Ok(())
    }

    /// Get the variable name for an input socket
    fn get_input_variable(&self, input: &InputSocket) -> Result<String> {
        if let Some((source_node, output_name)) = &input.connected_output {
            Ok(format!("node_{}_{}", source_node, output_name.to_lowercase()))
        } else if let Some(default) = &input.default_value {
            match input.data_type {
                DataType::Float => Ok(format!("{}", default.x)),
                DataType::Vec2 => Ok(format!("vec2<f32>({}, {})", default.x, default.y)),
                DataType::Vec3 => Ok(format!("vec3<f32>({}, {}, {})", default.x, default.y, default.z)),
                DataType::Vec4 | DataType::Color => {
                    Ok(format!("vec4<f32>({}, {}, {}, {})", default.x, default.y, default.z, default.w))
                }
                _ => Err(CodegenError::GenerationError("Cannot generate default for texture/sampler".to_string()))
            }
        } else {
            Err(CodegenError::GenerationError(format!("Input '{}' has no connection or default", input.name)))
        }
    }

    // Helper function generators

    fn generate_blur_function(&mut self, code: &mut String) -> Result<()> {
        if self.generated_functions.contains("blur") {
            return Ok(());
        }

        writeln!(code, "fn blur_sample(tex: texture_2d<f32>, samp: sampler, uv: vec2<f32>, radius: f32) -> vec4<f32> {{").unwrap();
        writeln!(code, "    var color = vec4<f32>(0.0);").unwrap();
        writeln!(code, "    let samples = 9;").unwrap();
        writeln!(code, "    let offset = radius / 100.0;").unwrap();
        writeln!(code, "    for (var x = -1; x <= 1; x++) {{").unwrap();
        writeln!(code, "        for (var y = -1; y <= 1; y++) {{").unwrap();
        writeln!(code, "            let sample_uv = uv + vec2<f32>(f32(x), f32(y)) * offset;").unwrap();
        writeln!(code, "            color += textureSample(tex, samp, sample_uv);").unwrap();
        writeln!(code, "        }}").unwrap();
        writeln!(code, "    }}").unwrap();
        writeln!(code, "    return color / f32(samples);").unwrap();
        writeln!(code, "}}\n").unwrap();

        self.generated_functions.insert("blur".to_string());
        Ok(())
    }

    fn generate_chromatic_aberration_function(&mut self, code: &mut String) -> Result<()> {
        if self.generated_functions.contains("chromatic_aberration") {
            return Ok(());
        }

        writeln!(code, "fn chromatic_aberration(tex: texture_2d<f32>, samp: sampler, uv: vec2<f32>, amount: f32) -> vec4<f32> {{").unwrap();
        writeln!(code, "    let offset = (uv - 0.5) * amount;").unwrap();
        writeln!(code, "    let r = textureSample(tex, samp, uv + offset).r;").unwrap();
        writeln!(code, "    let g = textureSample(tex, samp, uv).g;").unwrap();
        writeln!(code, "    let b = textureSample(tex, samp, uv - offset).b;").unwrap();
        writeln!(code, "    return vec4<f32>(r, g, b, 1.0);").unwrap();
        writeln!(code, "}}\n").unwrap();

        self.generated_functions.insert("chromatic_aberration".to_string());
        Ok(())
    }

    fn generate_edge_detect_function(&mut self, code: &mut String) -> Result<()> {
        if self.generated_functions.contains("edge_detect") {
            return Ok(());
        }

        writeln!(code, "fn edge_detect(tex: texture_2d<f32>, samp: sampler, uv: vec2<f32>) -> vec4<f32> {{").unwrap();
        writeln!(code, "    let offset = 1.0 / 512.0;").unwrap();
        writeln!(code, "    let c = textureSample(tex, samp, uv).rgb;").unwrap();
        writeln!(code, "    let t = textureSample(tex, samp, uv + vec2<f32>(0.0, offset)).rgb;").unwrap();
        writeln!(code, "    let b = textureSample(tex, samp, uv - vec2<f32>(0.0, offset)).rgb;").unwrap();
        writeln!(code, "    let l = textureSample(tex, samp, uv - vec2<f32>(offset, 0.0)).rgb;").unwrap();
        writeln!(code, "    let r = textureSample(tex, samp, uv + vec2<f32>(offset, 0.0)).rgb;").unwrap();
        writeln!(code, "    let edge = abs(c - t) + abs(c - b) + abs(c - l) + abs(c - r);").unwrap();
        writeln!(code, "    return vec4<f32>(edge, 1.0);").unwrap();
        writeln!(code, "}}\n").unwrap();

        self.generated_functions.insert("edge_detect".to_string());
        Ok(())
    }

    fn generate_kaleidoscope_function(&mut self, code: &mut String) -> Result<()> {
        if self.generated_functions.contains("kaleidoscope") {
            return Ok(());
        }

        writeln!(code, "fn kaleidoscope(uv: vec2<f32>, segments: f32) -> vec2<f32> {{").unwrap();
        writeln!(code, "    let center = uv - 0.5;").unwrap();
        writeln!(code, "    let angle = atan2(center.y, center.x);").unwrap();
        writeln!(code, "    let radius = length(center);").unwrap();
        writeln!(code, "    let slice = 6.28318530718 / segments;").unwrap();
        writeln!(code, "    let new_angle = abs((angle % slice) - slice * 0.5) + slice * 0.5;").unwrap();
        writeln!(code, "    return vec2<f32>(cos(new_angle), sin(new_angle)) * radius + 0.5;").unwrap();
        writeln!(code, "}}\n").unwrap();

        self.generated_functions.insert("kaleidoscope".to_string());
        Ok(())
    }

    fn generate_hsv_to_rgb_function(&mut self, code: &mut String) -> Result<()> {
        if self.generated_functions.contains("hsv_to_rgb") {
            return Ok(());
        }

        writeln!(code, "fn hsv_to_rgb(hsv: vec3<f32>) -> vec3<f32> {{").unwrap();
        writeln!(code, "    let h = hsv.x * 6.0;").unwrap();
        writeln!(code, "    let s = hsv.y;").unwrap();
        writeln!(code, "    let v = hsv.z;").unwrap();
        writeln!(code, "    let c = v * s;").unwrap();
        writeln!(code, "    let x = c * (1.0 - abs((h % 2.0) - 1.0));").unwrap();
        writeln!(code, "    let m = v - c;").unwrap();
        writeln!(code, "    var rgb = vec3<f32>(0.0);").unwrap();
        writeln!(code, "    if (h < 1.0) {{ rgb = vec3<f32>(c, x, 0.0); }}").unwrap();
        writeln!(code, "    else if (h < 2.0) {{ rgb = vec3<f32>(x, c, 0.0); }}").unwrap();
        writeln!(code, "    else if (h < 3.0) {{ rgb = vec3<f32>(0.0, c, x); }}").unwrap();
        writeln!(code, "    else if (h < 4.0) {{ rgb = vec3<f32>(0.0, x, c); }}").unwrap();
        writeln!(code, "    else if (h < 5.0) {{ rgb = vec3<f32>(x, 0.0, c); }}").unwrap();
        writeln!(code, "    else {{ rgb = vec3<f32>(c, 0.0, x); }}").unwrap();
        writeln!(code, "    return rgb + m;").unwrap();
        writeln!(code, "}}\n").unwrap();

        self.generated_functions.insert("hsv_to_rgb".to_string());
        Ok(())
    }

    fn generate_rgb_to_hsv_function(&mut self, code: &mut String) -> Result<()> {
        if self.generated_functions.contains("rgb_to_hsv") {
            return Ok(());
        }

        writeln!(code, "fn rgb_to_hsv(rgb: vec3<f32>) -> vec3<f32> {{").unwrap();
        writeln!(code, "    let max_c = max(max(rgb.r, rgb.g), rgb.b);").unwrap();
        writeln!(code, "    let min_c = min(min(rgb.r, rgb.g), rgb.b);").unwrap();
        writeln!(code, "    let delta = max_c - min_c;").unwrap();
        writeln!(code, "    var h = 0.0;").unwrap();
        writeln!(code, "    if (delta > 0.0) {{").unwrap();
        writeln!(code, "        if (max_c == rgb.r) {{ h = ((rgb.g - rgb.b) / delta) % 6.0; }}").unwrap();
        writeln!(code, "        else if (max_c == rgb.g) {{ h = (rgb.b - rgb.r) / delta + 2.0; }}").unwrap();
        writeln!(code, "        else {{ h = (rgb.r - rgb.g) / delta + 4.0; }}").unwrap();
        writeln!(code, "        h = h / 6.0;").unwrap();
        writeln!(code, "    }}").unwrap();
        writeln!(code, "    let s = select(0.0, delta / max_c, max_c > 0.0);").unwrap();
        writeln!(code, "    return vec3<f32>(h, s, max_c);").unwrap();
        writeln!(code, "}}\n").unwrap();

        self.generated_functions.insert("rgb_to_hsv".to_string());
        Ok(())
    }
}

impl std::fmt::Display for ParameterValue {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        match self {
            ParameterValue::Float(v) => write!(f, "{}", v),
            ParameterValue::Vec2(v) => write!(f, "vec2<f32>({}, {})", v[0], v[1]),
            ParameterValue::Vec3(v) => write!(f, "vec3<f32>({}, {}, {})", v[0], v[1], v[2]),
            ParameterValue::Vec4(v) => write!(f, "vec4<f32>({}, {}, {}, {})", v[0], v[1], v[2], v[3]),
            ParameterValue::Color(c) => write!(f, "vec4<f32>({}, {}, {}, {})", c[0], c[1], c[2], c[3]),
        }
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    use crate::shader_graph::{ShaderGraph, NodeType};

    #[test]
    fn test_simple_shader_generation() {
        let mut graph = ShaderGraph::new(1, "Test Shader".to_string());

        let uv_node = graph.add_node(NodeType::UVInput);
        let texture_node = graph.add_node(NodeType::TextureInput);
        let sample_node = graph.add_node(NodeType::TextureSample);
        let output_node = graph.add_node(NodeType::Output);

        graph.connect(uv_node, "UV", sample_node, "UV").unwrap();
        graph.connect(texture_node, "Texture", sample_node, "Texture").unwrap();
        graph.connect(sample_node, "Color", output_node, "Color").unwrap();

        let mut codegen = WGSLCodegen::new(graph);
        let result = codegen.generate();

        assert!(result.is_ok());
        let code = result.unwrap();
        assert!(code.contains("@fragment"));
        assert!(code.contains("textureSample"));
    }

    #[test]
    fn test_math_nodes() {
        let mut graph = ShaderGraph::new(1, "Math Test".to_string());

        let time_node = graph.add_node(NodeType::TimeInput);
        let sin_node = graph.add_node(NodeType::Sin);
        let output_node = graph.add_node(NodeType::Output);

        graph.connect(time_node, "Time", sin_node, "A").unwrap();
        graph.connect(sin_node, "Result", output_node, "Color").unwrap();

        let mut codegen = WGSLCodegen::new(graph);
        let result = codegen.generate();

        assert!(result.is_ok());
    }
}
