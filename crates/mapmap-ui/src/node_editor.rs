//! Phase 6: Node-Based Effect Editor
//!
//! Visual node graph for creating complex effects by connecting nodes.
//! Supports effect nodes, math nodes, utility nodes, and custom node API.

use egui::{Color32, Pos2, Rect, Response, Sense, Stroke, Ui, Vec2};
use serde::{Deserialize, Serialize};
use std::collections::HashMap;

/// Node graph editor
pub struct NodeEditor {
    /// All nodes in the graph
    nodes: HashMap<NodeId, Node>,
    /// All connections between nodes
    connections: Vec<Connection>,
    /// Next node ID
    next_id: u64,
    /// Selected nodes
    selected_nodes: Vec<NodeId>,
    /// Node being dragged
    dragging_node: Option<(NodeId, Vec2)>,
    /// Connection being created (from socket)
    creating_connection: Option<(NodeId, SocketId, Pos2)>,
    /// Canvas pan offset
    pan_offset: Vec2,
    /// Canvas zoom level
    zoom: f32,
    /// Node palette (available node types)
    node_palette: Vec<NodeType>,
    /// Show node palette
    show_palette: bool,
    /// Palette position
    palette_pos: Option<Pos2>,
}

/// Unique node identifier
pub type NodeId = u64;

/// Socket identifier (input/output index)
pub type SocketId = usize;

/// Node in the graph
#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct Node {
    pub id: NodeId,
    pub node_type: NodeType,
    pub position: Pos2,
    pub inputs: Vec<Socket>,
    pub outputs: Vec<Socket>,
    pub parameters: HashMap<String, Parameter>,
    pub size: Vec2,
}

/// Node type
#[derive(Debug, Clone, PartialEq, Serialize, Deserialize)]
pub enum NodeType {
    // Effect nodes
    Blur { radius: f32 },
    Glow { intensity: f32, threshold: f32 },
    ColorCorrection { hue: f32, saturation: f32, brightness: f32 },
    Sharpen { amount: f32 },
    EdgeDetect,
    ChromaKey { key_color: [f32; 3], threshold: f32 },

    // Math nodes
    Add,
    Subtract,
    Multiply,
    Divide,
    Sin,
    Cos,
    Abs,
    Clamp { min: f32, max: f32 },
    Lerp,
    SmoothStep,

    // Utility nodes
    Switch,
    Merge,
    Split,
    Value(f32),
    Vector3 { x: f32, y: f32, z: f32 },
    Color { r: f32, g: f32, b: f32, a: f32 },

    // Input/Output
    Input { name: String },
    Output { name: String },

    // Custom
    Custom { name: String, code: String },
}

impl NodeType {
    /// Get human-readable name
    pub fn name(&self) -> &str {
        match self {
            Self::Blur { .. } => "Blur",
            Self::Glow { .. } => "Glow",
            Self::ColorCorrection { .. } => "Color Correction",
            Self::Sharpen { .. } => "Sharpen",
            Self::EdgeDetect => "Edge Detect",
            Self::ChromaKey { .. } => "Chroma Key",
            Self::Add => "Add",
            Self::Subtract => "Subtract",
            Self::Multiply => "Multiply",
            Self::Divide => "Divide",
            Self::Sin => "Sin",
            Self::Cos => "Cos",
            Self::Abs => "Abs",
            Self::Clamp { .. } => "Clamp",
            Self::Lerp => "Lerp",
            Self::SmoothStep => "Smooth Step",
            Self::Switch => "Switch",
            Self::Merge => "Merge",
            Self::Split => "Split",
            Self::Value(_) => "Value",
            Self::Vector3 { .. } => "Vector3",
            Self::Color { .. } => "Color",
            Self::Input { .. } => "Input",
            Self::Output { .. } => "Output",
            Self::Custom { name, .. } => name,
        }
    }

    /// Get category for palette grouping
    pub fn category(&self) -> &str {
        match self {
            Self::Blur { .. }
            | Self::Glow { .. }
            | Self::ColorCorrection { .. }
            | Self::Sharpen { .. }
            | Self::EdgeDetect
            | Self::ChromaKey { .. } => "Effects",
            Self::Add
            | Self::Subtract
            | Self::Multiply
            | Self::Divide
            | Self::Sin
            | Self::Cos
            | Self::Abs
            | Self::Clamp { .. }
            | Self::Lerp
            | Self::SmoothStep => "Math",
            Self::Switch | Self::Merge | Self::Split => "Utility",
            Self::Value(_) | Self::Vector3 { .. } | Self::Color { .. } => "Constants",
            Self::Input { .. } | Self::Output { .. } => "I/O",
            Self::Custom { .. } => "Custom",
        }
    }

    /// Get default inputs for this node type
    pub fn default_inputs(&self) -> Vec<Socket> {
        match self {
            Self::Blur { .. } => vec![
                Socket::new("Input", SocketType::Image),
                Socket::new("Radius", SocketType::Float),
            ],
            Self::Glow { .. } => vec![
                Socket::new("Input", SocketType::Image),
                Socket::new("Intensity", SocketType::Float),
                Socket::new("Threshold", SocketType::Float),
            ],
            Self::ColorCorrection { .. } => vec![
                Socket::new("Input", SocketType::Image),
                Socket::new("Hue", SocketType::Float),
                Socket::new("Saturation", SocketType::Float),
                Socket::new("Brightness", SocketType::Float),
            ],
            Self::Add | Self::Subtract | Self::Multiply | Self::Divide => vec![
                Socket::new("A", SocketType::Float),
                Socket::new("B", SocketType::Float),
            ],
            Self::Lerp => vec![
                Socket::new("A", SocketType::Float),
                Socket::new("B", SocketType::Float),
                Socket::new("T", SocketType::Float),
            ],
            Self::Clamp { .. } => vec![
                Socket::new("Value", SocketType::Float),
                Socket::new("Min", SocketType::Float),
                Socket::new("Max", SocketType::Float),
            ],
            Self::Switch => vec![
                Socket::new("Condition", SocketType::Bool),
                Socket::new("True", SocketType::Any),
                Socket::new("False", SocketType::Any),
            ],
            Self::Merge => vec![
                Socket::new("A", SocketType::Image),
                Socket::new("B", SocketType::Image),
                Socket::new("Mix", SocketType::Float),
            ],
            Self::Output { .. } => vec![Socket::new("Input", SocketType::Any)],
            _ => vec![],
        }
    }

    /// Get default outputs for this node type
    pub fn default_outputs(&self) -> Vec<Socket> {
        match self {
            Self::Blur { .. }
            | Self::Glow { .. }
            | Self::ColorCorrection { .. }
            | Self::Sharpen { .. }
            | Self::EdgeDetect
            | Self::ChromaKey { .. } => vec![Socket::new("Output", SocketType::Image)],
            Self::Add
            | Self::Subtract
            | Self::Multiply
            | Self::Divide
            | Self::Sin
            | Self::Cos
            | Self::Abs
            | Self::Clamp { .. }
            | Self::Lerp
            | Self::SmoothStep => vec![Socket::new("Result", SocketType::Float)],
            Self::Switch | Self::Merge => vec![Socket::new("Output", SocketType::Any)],
            Self::Split => vec![
                Socket::new("R", SocketType::Float),
                Socket::new("G", SocketType::Float),
                Socket::new("B", SocketType::Float),
                Socket::new("A", SocketType::Float),
            ],
            Self::Value(_) => vec![Socket::new("Value", SocketType::Float)],
            Self::Vector3 { .. } => vec![Socket::new("Vector", SocketType::Vector)],
            Self::Color { .. } => vec![Socket::new("Color", SocketType::Color)],
            Self::Input { .. } => vec![Socket::new("Output", SocketType::Any)],
            Self::Output { .. } => vec![],
            Self::Custom { .. } => vec![Socket::new("Output", SocketType::Any)],
        }
    }
}

/// Socket (input or output connection point)
#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct Socket {
    pub name: String,
    pub socket_type: SocketType,
    pub connected: bool,
}

impl Socket {
    pub fn new(name: &str, socket_type: SocketType) -> Self {
        Self {
            name: name.to_string(),
            socket_type,
            connected: false,
        }
    }
}

/// Socket data type
#[derive(Debug, Clone, Copy, PartialEq, Serialize, Deserialize)]
pub enum SocketType {
    Float,
    Vector,
    Color,
    Image,
    Bool,
    Any,
}

impl SocketType {
    pub fn color(&self) -> Color32 {
        match self {
            Self::Float => Color32::from_rgb(100, 150, 255),
            Self::Vector => Color32::from_rgb(150, 100, 255),
            Self::Color => Color32::from_rgb(255, 150, 100),
            Self::Image => Color32::from_rgb(255, 200, 100),
            Self::Bool => Color32::from_rgb(100, 255, 150),
            Self::Any => Color32::from_rgb(150, 150, 150),
        }
    }

    /// Check if types are compatible for connection
    pub fn compatible_with(&self, other: &SocketType) -> bool {
        *self == *other || *self == SocketType::Any || *other == SocketType::Any
    }
}

/// Connection between two node sockets
#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct Connection {
    pub from_node: NodeId,
    pub from_socket: SocketId,
    pub to_node: NodeId,
    pub to_socket: SocketId,
}

/// Parameter value
#[derive(Debug, Clone, Serialize, Deserialize)]
pub enum Parameter {
    Float(f32),
    Int(i32),
    Bool(bool),
    String(String),
    Color([f32; 4]),
}

impl NodeEditor {
    pub fn new() -> Self {
        Self {
            nodes: HashMap::new(),
            connections: Vec::new(),
            next_id: 1,
            selected_nodes: Vec::new(),
            dragging_node: None,
            creating_connection: None,
            pan_offset: Vec2::ZERO,
            zoom: 1.0,
            node_palette: Self::create_palette(),
            show_palette: false,
            palette_pos: None,
        }
    }

    /// Create the node palette with all available node types
    fn create_palette() -> Vec<NodeType> {
        vec![
            // Effects
            NodeType::Blur { radius: 5.0 },
            NodeType::Glow {
                intensity: 1.0,
                threshold: 0.5,
            },
            NodeType::ColorCorrection {
                hue: 0.0,
                saturation: 1.0,
                brightness: 1.0,
            },
            NodeType::Sharpen { amount: 1.0 },
            NodeType::EdgeDetect,
            NodeType::ChromaKey {
                key_color: [0.0, 1.0, 0.0],
                threshold: 0.1,
            },
            // Math
            NodeType::Add,
            NodeType::Multiply,
            NodeType::Lerp,
            NodeType::Clamp { min: 0.0, max: 1.0 },
            // Utility
            NodeType::Switch,
            NodeType::Merge,
            NodeType::Split,
            // Constants
            NodeType::Value(0.0),
            NodeType::Color {
                r: 1.0,
                g: 1.0,
                b: 1.0,
                a: 1.0,
            },
        ]
    }

    /// Add a new node to the graph
    pub fn add_node(&mut self, node_type: NodeType, position: Pos2) -> NodeId {
        let id = self.next_id;
        self.next_id += 1;

        let inputs = node_type.default_inputs();
        let outputs = node_type.default_outputs();

        let size = Vec2::new(
            180.0,
            80.0 + (inputs.len().max(outputs.len()) as f32 * 24.0),
        );

        let node = Node {
            id,
            node_type,
            position,
            inputs,
            outputs,
            parameters: HashMap::new(),
            size,
        };

        self.nodes.insert(id, node);
        id
    }

    /// Remove a node from the graph
    pub fn remove_node(&mut self, node_id: NodeId) {
        self.nodes.remove(&node_id);
        self.connections
            .retain(|c| c.from_node != node_id && c.to_node != node_id);
        self.selected_nodes.retain(|id| *id != node_id);
    }

    /// Add a connection between two sockets
    pub fn add_connection(
        &mut self,
        from_node: NodeId,
        from_socket: SocketId,
        to_node: NodeId,
        to_socket: SocketId,
    ) -> bool {
        // Validate connection
        if let (Some(from), Some(to)) = (self.nodes.get(&from_node), self.nodes.get(&to_node)) {
            if from_socket < from.outputs.len() && to_socket < to.inputs.len() {
                let from_type = &from.outputs[from_socket].socket_type;
                let to_type = &to.inputs[to_socket].socket_type;

                if from_type.compatible_with(to_type) {
                    // Remove existing connection to this input
                    self.connections
                        .retain(|c| c.to_node != to_node || c.to_socket != to_socket);

                    self.connections.push(Connection {
                        from_node,
                        from_socket,
                        to_node,
                        to_socket,
                    });
                    return true;
                }
            }
        }
        false
    }

    /// Render the node editor UI
    pub fn ui(&mut self, ui: &mut Ui) -> Option<NodeEditorAction> {
        let mut action = None;

        // Canvas background
        let (response, painter) = ui.allocate_painter(ui.available_size(), Sense::click_and_drag());

        // Handle canvas interactions
        if response.dragged() && !self.dragging_node.is_some() && !self.creating_connection.is_some()
        {
            self.pan_offset += response.drag_delta();
        }

        // Zoom
        if response.hovered() {
            let scroll = ui.input(|i| i.raw_scroll_delta.y);
            if scroll != 0.0 {
                self.zoom *= 1.0 + scroll * 0.001;
                self.zoom = self.zoom.clamp(0.2, 3.0);
            }
        }

        // Right-click to show palette
        if response.secondary_clicked() {
            self.show_palette = true;
            self.palette_pos = response.interact_pointer_pos();
        }

        let canvas_rect = response.rect;
        let to_screen = |pos: Pos2| -> Pos2 {
            canvas_rect.min + (pos.to_vec2() + self.pan_offset) * self.zoom
        };

        // Draw grid
        self.draw_grid(&painter, canvas_rect);

        // Draw connections
        for conn in &self.connections {
            if let (Some(from_node), Some(to_node)) =
                (self.nodes.get(&conn.from_node), self.nodes.get(&conn.to_node))
            {
                let from_pos = self.get_socket_pos(from_node, conn.from_socket, false);
                let to_pos = self.get_socket_pos(to_node, conn.to_socket, true);

                let from_screen = to_screen(from_pos);
                let to_screen = to_screen(to_pos);

                let color = from_node.outputs[conn.from_socket].socket_type.color();
                self.draw_connection(&painter, from_screen, to_screen, color);
            }
        }

        // Draw nodes
        let mut nodes_vec: Vec<_> = self.nodes.values().collect();
        nodes_vec.sort_by_key(|n| n.id); // Stable ordering

        for node in nodes_vec {
            let node_screen_pos = to_screen(node.position);
            let node_screen_rect =
                Rect::from_min_size(node_screen_pos, node.size * self.zoom);

            let node_response = self.draw_node(ui, &painter, node, node_screen_rect);

            if node_response.clicked() {
                self.selected_nodes.clear();
                self.selected_nodes.push(node.id);
            }

            if node_response.dragged() {
                self.dragging_node = Some((node.id, response.drag_delta() / self.zoom));
            }
        }

        // Apply node dragging
        if let Some((node_id, delta)) = self.dragging_node {
            if let Some(node) = self.nodes.get_mut(&node_id) {
                node.position += delta;
            }
            if !response.dragged() {
                self.dragging_node = None;
            }
        }

        // Draw connection being created
        if let Some((_node_id, _socket_id, start_pos)) = self.creating_connection {
            if let Some(pointer_pos) = response.interact_pointer_pos() {
                self.draw_connection(
                    &painter,
                    start_pos,
                    pointer_pos,
                    Color32::from_rgb(150, 150, 150),
                );

                if response.clicked() {
                    // Try to complete connection
                    // TODO: Detect socket under pointer
                    self.creating_connection = None;
                }
            }
        }

        // Node palette popup
        if self.show_palette {
            if let Some(pos) = self.palette_pos {
                egui::Area::new(egui::Id::new("node_palette"))
                    .fixed_pos(pos)
                    .show(ui.ctx(), |ui| {
                        egui::Frame::popup(ui.style()).show(ui, |ui| {
                            ui.set_min_width(200.0);
                            ui.label("Add Node:");
                            ui.separator();

                            let mut selected_type: Option<NodeType> = None;
                            let mut current_category = "";

                            for node_type in &self.node_palette {
                                if node_type.category() != current_category {
                                    current_category = node_type.category();
                                    ui.separator();
                                    ui.label(current_category);
                                }

                                if ui.button(node_type.name()).clicked() {
                                    selected_type = Some(node_type.clone());
                                    self.show_palette = false;
                                }
                            }

                            if let Some(node_type) = selected_type {
                                let world_pos = (pos - canvas_rect.min - self.pan_offset) / self.zoom;
                                action = Some(NodeEditorAction::AddNode(node_type, world_pos.to_pos2()));
                            }
                        });
                    });
            }

            // Close palette if clicked outside
            if response.clicked() {
                self.show_palette = false;
            }
        }

        action
    }

    /// Draw grid background
    fn draw_grid(&self, painter: &egui::Painter, rect: Rect) {
        let grid_size = 20.0 * self.zoom;
        let color = Color32::from_rgb(40, 40, 40);

        let start_x = (rect.min.x / grid_size).floor() * grid_size;
        let start_y = (rect.min.y / grid_size).floor() * grid_size;

        let mut x = start_x;
        while x < rect.max.x {
            painter.line_segment(
                [Pos2::new(x, rect.min.y), Pos2::new(x, rect.max.y)],
                Stroke::new(1.0, color),
            );
            x += grid_size;
        }

        let mut y = start_y;
        while y < rect.max.y {
            painter.line_segment(
                [Pos2::new(rect.min.x, y), Pos2::new(rect.max.x, y)],
                Stroke::new(1.0, color),
            );
            y += grid_size;
        }
    }

    /// Draw a connection curve
    fn draw_connection(&self, painter: &egui::Painter, from: Pos2, to: Pos2, color: Color32) {
        let control_offset = ((to.x - from.x) * 0.5).abs().max(50.0);
        let ctrl1 = Pos2::new(from.x + control_offset, from.y);
        let ctrl2 = Pos2::new(to.x - control_offset, to.y);

        // Draw bezier curve with multiple segments
        let segments = 20;
        let mut points = Vec::new();
        for i in 0..=segments {
            let t = i as f32 / segments as f32;
            let point = cubic_bezier(from, ctrl1, ctrl2, to, t);
            points.push(point);
        }

        for i in 0..points.len() - 1 {
            painter.line_segment([points[i], points[i + 1]], Stroke::new(2.0, color));
        }
    }

    /// Draw a node
    fn draw_node(
        &self,
        ui: &Ui,
        painter: &egui::Painter,
        node: &Node,
        rect: Rect,
    ) -> Response {
        let response = ui.interact(rect, egui::Id::new(node.id), Sense::click_and_drag());

        let is_selected = self.selected_nodes.contains(&node.id);
        let bg_color = if is_selected {
            Color32::from_rgb(50, 100, 150)
        } else {
            Color32::from_rgb(40, 40, 40)
        };

        // Node background
        painter.rect_filled(rect, 4.0, bg_color);
        painter.rect_stroke(
            rect,
            4.0,
            Stroke::new(2.0, Color32::from_rgb(80, 80, 80)),
        );

        // Title bar
        let title_rect = Rect::from_min_size(rect.min, Vec2::new(rect.width(), 24.0 * self.zoom));
        painter.rect_filled(title_rect, 4.0, Color32::from_rgb(30, 30, 30));
        painter.text(
            title_rect.center(),
            egui::Align2::CENTER_CENTER,
            node.node_type.name(),
            egui::FontId::proportional(14.0 * self.zoom),
            Color32::WHITE,
        );

        // Input sockets
        for (i, input) in node.inputs.iter().enumerate() {
            let socket_pos = self.get_socket_pos(node, i, true);
            self.draw_socket(painter, socket_pos, input.socket_type, true);
        }

        // Output sockets
        for (i, output) in node.outputs.iter().enumerate() {
            let socket_pos = self.get_socket_pos(node, i, false);
            self.draw_socket(painter, socket_pos, output.socket_type, false);
        }

        response
    }

    /// Draw a socket
    fn draw_socket(&self, painter: &egui::Painter, pos: Pos2, socket_type: SocketType, _is_input: bool) {
        let radius = 6.0 * self.zoom;
        painter.circle_filled(pos, radius, socket_type.color());
        painter.circle_stroke(pos, radius, Stroke::new(2.0, Color32::WHITE));
    }

    /// Get socket position in world space
    fn get_socket_pos(&self, node: &Node, socket_idx: usize, is_input: bool) -> Pos2 {
        let socket_y = node.position.y + 40.0 + (socket_idx as f32 * 24.0);
        let socket_x = if is_input {
            node.position.x
        } else {
            node.position.x + node.size.x
        };
        Pos2::new(socket_x, socket_y)
    }
}

/// Cubic bezier interpolation
fn cubic_bezier(p0: Pos2, p1: Pos2, p2: Pos2, p3: Pos2, t: f32) -> Pos2 {
    let t2 = t * t;
    let t3 = t2 * t;
    let mt = 1.0 - t;
    let mt2 = mt * mt;
    let mt3 = mt2 * mt;

    Pos2::new(
        mt3 * p0.x + 3.0 * mt2 * t * p1.x + 3.0 * mt * t2 * p2.x + t3 * p3.x,
        mt3 * p0.y + 3.0 * mt2 * t * p1.y + 3.0 * mt * t2 * p2.y + t3 * p3.y,
    )
}

/// Actions that can be triggered by the node editor
#[derive(Debug, Clone)]
pub enum NodeEditorAction {
    AddNode(NodeType, Pos2),
    RemoveNode(NodeId),
    SelectNode(NodeId),
    AddConnection(NodeId, SocketId, NodeId, SocketId),
    RemoveConnection(NodeId, SocketId, NodeId, SocketId),
}
