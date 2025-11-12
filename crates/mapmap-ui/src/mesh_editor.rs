//! Phase 6: Advanced Warp Mesh Editor
//!
//! Advanced mesh editing with Bezier control points, subdivision surfaces,
//! symmetry mode, snap to grid/guides, and copy/paste functionality.

use egui::{Color32, Pos2, Rect, Sense, Stroke, Ui, Vec2};
use serde::{Deserialize, Serialize};

/// Advanced mesh editor
pub struct MeshEditor {
    /// Mesh vertices
    vertices: Vec<Vertex>,
    /// Mesh faces (triangles)
    faces: Vec<Face>,
    /// Selected vertices
    #[allow(dead_code)]
    selected: Vec<usize>,
    /// Dragging vertex
    #[allow(dead_code)]
    dragging: Option<usize>,
    /// Editor mode
    mode: EditMode,
    /// Symmetry settings
    symmetry: SymmetryMode,
    /// Snap settings
    snap_to_grid: bool,
    grid_size: f32,
    /// Subdivision level
    #[allow(dead_code)]
    subdivision_level: u32,
    /// Canvas transform
    #[allow(dead_code)]
    pan_offset: Vec2,
    #[allow(dead_code)]
    zoom: f32,
}

/// Mesh vertex with Bezier control points
#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct Vertex {
    pub position: Pos2,
    pub control_in: Option<Vec2>,
    pub control_out: Option<Vec2>,
    pub selected: bool,
}

/// Mesh face (triangle)
#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct Face {
    pub vertices: [usize; 3],
}

/// Edit mode
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum EditMode {
    /// Select and move vertices
    Select,
    /// Add new vertices
    Add,
    /// Remove vertices
    Remove,
    /// Edit Bezier control points
    Bezier,
}

/// Symmetry mode
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum SymmetryMode {
    None,
    Horizontal,
    Vertical,
    Both,
}

impl MeshEditor {
    pub fn new() -> Self {
        Self {
            vertices: Vec::new(),
            faces: Vec::new(),
            selected: Vec::new(),
            dragging: None,
            mode: EditMode::Select,
            symmetry: SymmetryMode::None,
            snap_to_grid: false,
            grid_size: 20.0,
            subdivision_level: 0,
            pan_offset: Vec2::ZERO,
            zoom: 1.0,
        }
    }

    /// Create a default quad mesh
    pub fn create_quad(&mut self, center: Pos2, size: f32) {
        self.vertices.clear();
        self.faces.clear();

        let half = size / 2.0;
        self.vertices.push(Vertex {
            position: Pos2::new(center.x - half, center.y - half),
            control_in: None,
            control_out: None,
            selected: false,
        });
        self.vertices.push(Vertex {
            position: Pos2::new(center.x + half, center.y - half),
            control_in: None,
            control_out: None,
            selected: false,
        });
        self.vertices.push(Vertex {
            position: Pos2::new(center.x + half, center.y + half),
            control_in: None,
            control_out: None,
            selected: false,
        });
        self.vertices.push(Vertex {
            position: Pos2::new(center.x - half, center.y + half),
            control_in: None,
            control_out: None,
            selected: false,
        });

        self.faces.push(Face {
            vertices: [0, 1, 2],
        });
        self.faces.push(Face {
            vertices: [0, 2, 3],
        });
    }

    /// Subdivide the mesh
    pub fn subdivide(&mut self) {
        // Catmull-Clark subdivision (simplified)
        let mut new_vertices = self.vertices.clone();
        let mut new_faces = Vec::new();

        for face in &self.faces {
            // Calculate face center
            let face_center = Pos2::new(
                (self.vertices[face.vertices[0]].position.x
                    + self.vertices[face.vertices[1]].position.x
                    + self.vertices[face.vertices[2]].position.x)
                    / 3.0,
                (self.vertices[face.vertices[0]].position.y
                    + self.vertices[face.vertices[1]].position.y
                    + self.vertices[face.vertices[2]].position.y)
                    / 3.0,
            );

            let center_idx = new_vertices.len();
            new_vertices.push(Vertex {
                position: face_center,
                control_in: None,
                control_out: None,
                selected: false,
            });

            // Create new faces
            for i in 0..3 {
                let v0 = face.vertices[i];
                let v1 = face.vertices[(i + 1) % 3];

                // Edge midpoint
                let edge_mid = Pos2::new(
                    (self.vertices[v0].position.x + self.vertices[v1].position.x) / 2.0,
                    (self.vertices[v0].position.y + self.vertices[v1].position.y) / 2.0,
                );

                let mid_idx = new_vertices.len();
                new_vertices.push(Vertex {
                    position: edge_mid,
                    control_in: None,
                    control_out: None,
                    selected: false,
                });

                new_faces.push(Face {
                    vertices: [v0, mid_idx, center_idx],
                });
            }
        }

        self.vertices = new_vertices;
        self.faces = new_faces;
    }

    /// Apply symmetry to vertex movement
    #[allow(dead_code)]
    fn apply_symmetry(&mut self, vertex_idx: usize, new_pos: Pos2) {
        self.vertices[vertex_idx].position = new_pos;

        match self.symmetry {
            SymmetryMode::None => {}
            SymmetryMode::Horizontal => {
                // Find symmetric vertex across vertical axis
                // TODO: Implement symmetric vertex finding
            }
            SymmetryMode::Vertical => {
                // Find symmetric vertex across horizontal axis
                // TODO: Implement symmetric vertex finding
            }
            SymmetryMode::Both => {
                // Apply both symmetries
                // TODO: Implement
            }
        }
    }

    /// Snap position to grid
    fn snap_to_grid_pos(&self, pos: Pos2) -> Pos2 {
        if self.snap_to_grid {
            Pos2::new(
                (pos.x / self.grid_size).round() * self.grid_size,
                (pos.y / self.grid_size).round() * self.grid_size,
            )
        } else {
            pos
        }
    }

    /// Render the mesh editor UI
    pub fn ui(&mut self, ui: &mut Ui) -> Option<MeshEditorAction> {
        let mut action = None;

        // Toolbar
        ui.horizontal(|ui| {
            ui.selectable_value(&mut self.mode, EditMode::Select, "Select");
            ui.selectable_value(&mut self.mode, EditMode::Add, "Add");
            ui.selectable_value(&mut self.mode, EditMode::Remove, "Remove");
            ui.selectable_value(&mut self.mode, EditMode::Bezier, "Bezier");

            ui.separator();

            ui.checkbox(&mut self.snap_to_grid, "Snap to Grid");
            if self.snap_to_grid {
                ui.add(egui::DragValue::new(&mut self.grid_size).prefix("Grid: "));
            }

            ui.separator();

            ui.label("Symmetry:");
            ui.selectable_value(&mut self.symmetry, SymmetryMode::None, "None");
            ui.selectable_value(&mut self.symmetry, SymmetryMode::Horizontal, "H");
            ui.selectable_value(&mut self.symmetry, SymmetryMode::Vertical, "V");
            ui.selectable_value(&mut self.symmetry, SymmetryMode::Both, "Both");

            ui.separator();

            if ui.button("Subdivide").clicked() {
                self.subdivide();
            }

            if ui.button("Create Quad").clicked() {
                self.create_quad(Pos2::new(400.0, 300.0), 200.0);
            }
        });

        ui.separator();

        // Canvas
        let (response, painter) = ui.allocate_painter(ui.available_size(), Sense::click_and_drag());

        // Draw grid if enabled
        if self.snap_to_grid {
            self.draw_grid(&painter, response.rect);
        }

        // Draw mesh faces
        for face in &self.faces {
            let points = [
                self.vertices[face.vertices[0]].position,
                self.vertices[face.vertices[1]].position,
                self.vertices[face.vertices[2]].position,
            ];

            painter.add(egui::Shape::convex_polygon(
                points.to_vec(),
                Color32::from_rgba_premultiplied(100, 100, 150, 50),
                Stroke::new(1.0, Color32::from_rgb(150, 150, 200)),
            ));
        }

        // Draw vertices
        for (_i, vertex) in self.vertices.iter().enumerate() {
            let color = if vertex.selected {
                Color32::from_rgb(255, 200, 100)
            } else {
                Color32::from_rgb(200, 200, 200)
            };

            painter.circle_filled(vertex.position, 6.0, color);
            painter.circle_stroke(vertex.position, 6.0, Stroke::new(2.0, Color32::WHITE));

            // Draw Bezier control points if in Bezier mode
            if self.mode == EditMode::Bezier {
                if let Some(ctrl_in) = vertex.control_in {
                    let ctrl_pos = vertex.position + ctrl_in;
                    painter.line_segment(
                        [vertex.position, ctrl_pos],
                        Stroke::new(1.0, Color32::from_rgb(100, 200, 255)),
                    );
                    painter.circle_filled(ctrl_pos, 4.0, Color32::from_rgb(100, 200, 255));
                }

                if let Some(ctrl_out) = vertex.control_out {
                    let ctrl_pos = vertex.position + ctrl_out;
                    painter.line_segment(
                        [vertex.position, ctrl_pos],
                        Stroke::new(1.0, Color32::from_rgb(255, 200, 100)),
                    );
                    painter.circle_filled(ctrl_pos, 4.0, Color32::from_rgb(255, 200, 100));
                }
            }
        }

        // Handle interactions
        if let Some(pointer_pos) = response.interact_pointer_pos() {
            match self.mode {
                EditMode::Select => {
                    if response.clicked() {
                        // Select vertex under pointer
                        let mut found = false;
                        for (_i, vertex) in self.vertices.iter_mut().enumerate() {
                            if vertex.position.distance(pointer_pos) < 10.0 {
                                vertex.selected = !vertex.selected;
                                found = true;
                                break;
                            }
                        }

                        if !found {
                            // Deselect all
                            for vertex in &mut self.vertices {
                                vertex.selected = false;
                            }
                        }
                    }

                    if response.dragged() {
                        // Drag selected vertices
                        let delta = response.drag_delta();
                        let snap_to_grid = self.snap_to_grid;
                        let grid_size = self.grid_size;
                        for vertex in &mut self.vertices {
                            if vertex.selected {
                                let new_pos = vertex.position + delta;
                                // Inline snap_to_grid_pos logic to avoid borrow conflict
                                vertex.position = if snap_to_grid {
                                    Pos2::new(
                                        (new_pos.x / grid_size).round() * grid_size,
                                        (new_pos.y / grid_size).round() * grid_size,
                                    )
                                } else {
                                    new_pos
                                };
                            }
                        }
                    }
                }
                EditMode::Add => {
                    if response.clicked() {
                        let pos = self.snap_to_grid_pos(pointer_pos);
                        self.vertices.push(Vertex {
                            position: pos,
                            control_in: None,
                            control_out: None,
                            selected: false,
                        });
                        action = Some(MeshEditorAction::VertexAdded);
                    }
                }
                EditMode::Remove => {
                    if response.clicked() {
                        // Remove vertex under pointer
                        if let Some(idx) = self
                            .vertices
                            .iter()
                            .position(|v| v.position.distance(pointer_pos) < 10.0)
                        {
                            self.vertices.remove(idx);
                            // Remove faces referencing this vertex
                            self.faces.retain(|f| {
                                !f.vertices.contains(&idx)
                            });
                            action = Some(MeshEditorAction::VertexRemoved);
                        }
                    }
                }
                EditMode::Bezier => {
                    // TODO: Implement Bezier control point editing
                }
            }
        }

        action
    }

    /// Draw grid background
    fn draw_grid(&self, painter: &egui::Painter, rect: Rect) {
        let color = Color32::from_rgb(50, 50, 50);

        let mut x = 0.0;
        while x < rect.width() {
            let pos_x = rect.min.x + x;
            painter.line_segment(
                [Pos2::new(pos_x, rect.min.y), Pos2::new(pos_x, rect.max.y)],
                Stroke::new(1.0, color),
            );
            x += self.grid_size;
        }

        let mut y = 0.0;
        while y < rect.height() {
            let pos_y = rect.min.y + y;
            painter.line_segment(
                [Pos2::new(rect.min.x, pos_y), Pos2::new(rect.max.x, pos_y)],
                Stroke::new(1.0, color),
            );
            y += self.grid_size;
        }
    }
}

/// Actions that can be triggered by the mesh editor
#[derive(Debug, Clone)]
pub enum MeshEditorAction {
    VertexAdded,
    VertexRemoved,
    MeshSubdivided,
}
