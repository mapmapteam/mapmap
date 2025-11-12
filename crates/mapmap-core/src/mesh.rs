//! Mesh - Geometry for Warping
//!
//! Defines the shape and warping of mapped content
//! Phase 2: Bezier-based mesh warping with control points

use glam::Vec2;
use serde::{Deserialize, Serialize};

/// Unique identifier for a vertex
pub type VertexId = usize;

/// Mesh vertex with position and texture coordinates
#[derive(Debug, Clone, Copy, Serialize, Deserialize)]
pub struct MeshVertex {
    /// Position in output space (normalized 0-1)
    pub position: Vec2,
    /// Texture coordinates (UV mapping)
    pub tex_coords: Vec2,
    /// Is this vertex selected? (for editing)
    #[serde(skip)]
    pub selected: bool,
}

impl MeshVertex {
    pub fn new(position: Vec2, tex_coords: Vec2) -> Self {
        Self {
            position,
            tex_coords,
            selected: false,
        }
    }
}

/// Type of mesh
#[derive(Debug, Clone, Copy, PartialEq, Eq, Serialize, Deserialize)]
pub enum MeshType {
    /// Simple quad (4 vertices)
    Quad,
    /// Triangle (3 vertices)
    Triangle,
    /// Ellipse (approximated by N vertices)
    Ellipse,
    /// Custom mesh (arbitrary vertices)
    Custom,
}

/// Mesh - defines geometry for mapping
#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct Mesh {
    pub mesh_type: MeshType,
    pub vertices: Vec<MeshVertex>,
    /// Triangle indices (3 per triangle)
    pub indices: Vec<u16>,
}

impl Mesh {
    /// Create a new quad mesh
    pub fn quad() -> Self {
        let vertices = vec![
            MeshVertex::new(Vec2::new(0.0, 0.0), Vec2::new(0.0, 0.0)), // Top-left
            MeshVertex::new(Vec2::new(1.0, 0.0), Vec2::new(1.0, 0.0)), // Top-right
            MeshVertex::new(Vec2::new(1.0, 1.0), Vec2::new(1.0, 1.0)), // Bottom-right
            MeshVertex::new(Vec2::new(0.0, 1.0), Vec2::new(0.0, 1.0)), // Bottom-left
        ];

        let indices = vec![
            0, 1, 2, // First triangle
            0, 2, 3, // Second triangle
        ];

        Self {
            mesh_type: MeshType::Quad,
            vertices,
            indices,
        }
    }

    /// Create a quad mesh with specific dimensions
    pub fn quad_with_bounds(x: f32, y: f32, width: f32, height: f32) -> Self {
        let mut mesh = Self::quad();

        mesh.vertices[0].position = Vec2::new(x, y);
        mesh.vertices[1].position = Vec2::new(x + width, y);
        mesh.vertices[2].position = Vec2::new(x + width, y + height);
        mesh.vertices[3].position = Vec2::new(x, y + height);

        mesh
    }

    /// Create a triangle mesh
    pub fn triangle() -> Self {
        let vertices = vec![
            MeshVertex::new(Vec2::new(0.5, 0.0), Vec2::new(0.5, 0.0)), // Top
            MeshVertex::new(Vec2::new(1.0, 1.0), Vec2::new(1.0, 1.0)), // Bottom-right
            MeshVertex::new(Vec2::new(0.0, 1.0), Vec2::new(0.0, 1.0)), // Bottom-left
        ];

        let indices = vec![0, 1, 2];

        Self {
            mesh_type: MeshType::Triangle,
            vertices,
            indices,
        }
    }

    /// Create an ellipse mesh (approximated by N segments)
    pub fn ellipse(center: Vec2, radius_x: f32, radius_y: f32, segments: u32) -> Self {
        let segments = segments.max(3);
        let mut vertices = vec![MeshVertex::new(center, Vec2::new(0.5, 0.5))]; // Center vertex

        // Create vertices around the ellipse
        for i in 0..segments {
            let angle = (i as f32) * std::f32::consts::TAU / (segments as f32);
            let x = center.x + radius_x * angle.cos();
            let y = center.y + radius_y * angle.sin();
            let u = 0.5 + 0.5 * angle.cos();
            let v = 0.5 + 0.5 * angle.sin();

            vertices.push(MeshVertex::new(Vec2::new(x, y), Vec2::new(u, v)));
        }

        // Create triangle fan indices
        let mut indices = Vec::new();
        for i in 1..segments {
            indices.push(0); // Center
            indices.push(i as u16);
            indices.push(((i % segments) + 1) as u16);
        }
        // Close the fan
        indices.push(0);
        indices.push(segments as u16);
        indices.push(1);

        Self {
            mesh_type: MeshType::Ellipse,
            vertices,
            indices,
        }
    }

    /// Get mutable vertex by index
    pub fn get_vertex_mut(&mut self, index: usize) -> Option<&mut MeshVertex> {
        self.vertices.get_mut(index)
    }

    /// Get vertex by index
    pub fn get_vertex(&self, index: usize) -> Option<&MeshVertex> {
        self.vertices.get(index)
    }

    /// Get number of vertices
    pub fn vertex_count(&self) -> usize {
        self.vertices.len()
    }

    /// Get number of triangles
    pub fn triangle_count(&self) -> usize {
        self.indices.len() / 3
    }

    /// Select/deselect all vertices
    pub fn select_all(&mut self, selected: bool) {
        for vertex in &mut self.vertices {
            vertex.selected = selected;
        }
    }

    /// Get selected vertices
    pub fn selected_vertices(&self) -> Vec<VertexId> {
        self.vertices
            .iter()
            .enumerate()
            .filter(|(_, v)| v.selected)
            .map(|(i, _)| i)
            .collect()
    }

    /// Translate selected vertices
    pub fn translate_selected(&mut self, delta: Vec2) {
        for vertex in &mut self.vertices {
            if vertex.selected {
                vertex.position += delta;
            }
        }
    }

    /// Get bounding box
    pub fn bounds(&self) -> Option<(Vec2, Vec2)> {
        if self.vertices.is_empty() {
            return None;
        }

        let mut min = self.vertices[0].position;
        let mut max = self.vertices[0].position;

        for vertex in &self.vertices {
            min = min.min(vertex.position);
            max = max.max(vertex.position);
        }

        Some((min, max))
    }

    /// Apply keystone correction (4-point perspective warp)
    /// Maps a quad to arbitrary four corner positions
    pub fn apply_keystone(&mut self, corners: [Vec2; 4]) {
        if self.mesh_type != MeshType::Quad || self.vertices.len() != 4 {
            return;
        }

        // Direct mapping for 4-corner quad
        for i in 0..4 {
            self.vertices[i].position = corners[i];
        }
    }

    /// Create a subdivided grid mesh for smooth warping
    /// rows x cols grid with Bezier interpolation
    pub fn create_grid(rows: u32, cols: u32) -> Self {
        let rows = rows.max(1);
        let cols = cols.max(1);

        let mut vertices = Vec::new();
        let mut indices = Vec::new();

        // Create grid vertices
        for row in 0..=rows {
            for col in 0..=cols {
                let u = col as f32 / cols as f32;
                let v = row as f32 / rows as f32;
                vertices.push(MeshVertex::new(Vec2::new(u, v), Vec2::new(u, v)));
            }
        }

        // Create triangle indices (two triangles per grid cell)
        for row in 0..rows {
            for col in 0..cols {
                let top_left = (row * (cols + 1) + col) as u16;
                let top_right = top_left + 1;
                let bottom_left = ((row + 1) * (cols + 1) + col) as u16;
                let bottom_right = bottom_left + 1;

                // First triangle (top-left, bottom-left, top-right)
                indices.push(top_left);
                indices.push(bottom_left);
                indices.push(top_right);

                // Second triangle (top-right, bottom-left, bottom-right)
                indices.push(top_right);
                indices.push(bottom_left);
                indices.push(bottom_right);
            }
        }

        Self {
            mesh_type: MeshType::Custom,
            vertices,
            indices,
        }
    }
}

/// Phase 2: Bezier patch for smooth warping
#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct BezierPatch {
    /// 4x4 control points for bicubic Bezier surface
    pub control_points: [[Vec2; 4]; 4],
}

impl BezierPatch {
    /// Create a new Bezier patch with default planar surface
    pub fn new() -> Self {
        let mut control_points = [[Vec2::ZERO; 4]; 4];

        for i in 0..4 {
            for j in 0..4 {
                let u = j as f32 / 3.0;
                let v = i as f32 / 3.0;
                control_points[i][j] = Vec2::new(u, v);
            }
        }

        Self { control_points }
    }

    /// Evaluate Bezier surface at parametric coordinates (u, v)
    /// u, v in range [0, 1]
    pub fn evaluate(&self, u: f32, v: f32) -> Vec2 {
        // Cubic Bezier basis functions
        let basis = |t: f32| -> [f32; 4] {
            let t2 = t * t;
            let t3 = t2 * t;
            let mt = 1.0 - t;
            let mt2 = mt * mt;
            let mt3 = mt2 * mt;

            [
                mt3,           // (1-t)^3
                3.0 * mt2 * t, // 3(1-t)^2 * t
                3.0 * mt * t2, // 3(1-t) * t^2
                t3,            // t^3
            ]
        };

        let u_basis = basis(u);
        let v_basis = basis(v);

        let mut result = Vec2::ZERO;

        for i in 0..4 {
            for j in 0..4 {
                result += self.control_points[i][j] * u_basis[j] * v_basis[i];
            }
        }

        result
    }

    /// Apply this Bezier patch to a mesh
    /// Warps mesh vertices according to the patch surface
    pub fn apply_to_mesh(&self, mesh: &mut Mesh) {
        for vertex in &mut mesh.vertices {
            // Use texture coordinates as parametric coordinates
            let u = vertex.tex_coords.x;
            let v = vertex.tex_coords.y;

            // Evaluate Bezier surface at this point
            vertex.position = self.evaluate(u, v);
        }
    }

    /// Set corner control points (for keystone correction)
    pub fn set_corners(&mut self, corners: [Vec2; 4]) {
        // Map corners to control points: [top-left, top-right, bottom-right, bottom-left]
        self.control_points[0][0] = corners[0]; // Top-left
        self.control_points[0][3] = corners[1]; // Top-right
        self.control_points[3][3] = corners[2]; // Bottom-right
        self.control_points[3][0] = corners[3]; // Bottom-left

        // Interpolate edge control points
        for i in 1..3 {
            let t = i as f32 / 3.0;
            // Top edge
            self.control_points[0][i] = corners[0].lerp(corners[1], t);
            // Bottom edge
            self.control_points[3][i] = corners[3].lerp(corners[2], t);
            // Left edge
            self.control_points[i][0] = corners[0].lerp(corners[3], t);
            // Right edge
            self.control_points[i][3] = corners[1].lerp(corners[2], t);
        }

        // Interpolate interior control points (bilinear)
        for i in 1..3 {
            for j in 1..3 {
                let u = j as f32 / 3.0;
                let v = i as f32 / 3.0;

                let top = corners[0].lerp(corners[1], u);
                let bottom = corners[3].lerp(corners[2], u);
                self.control_points[i][j] = top.lerp(bottom, v);
            }
        }
    }
}

impl Default for BezierPatch {
    fn default() -> Self {
        Self::new()
    }
}

/// Keystone correction utilities
pub mod keystone {
    use super::*;

    /// Apply quick keystone correction to a quad mesh
    /// corners: [top-left, top-right, bottom-right, bottom-left]
    pub fn apply_to_quad(mesh: &mut Mesh, corners: [Vec2; 4]) {
        mesh.apply_keystone(corners);
    }

    /// Create a perspective-corrected grid mesh
    pub fn create_warped_grid(rows: u32, cols: u32, corners: [Vec2; 4]) -> Mesh {
        let mut mesh = Mesh::create_grid(rows, cols);
        let mut patch = BezierPatch::new();
        patch.set_corners(corners);
        patch.apply_to_mesh(&mut mesh);
        mesh
    }

    /// Calculate corners for common keystone presets
    pub fn preset_keystone(preset: KeystonePreset, amount: f32) -> [Vec2; 4] {
        match preset {
            KeystonePreset::Horizontal => {
                let offset = amount.clamp(0.0, 0.5);
                [
                    Vec2::new(offset, 0.0),        // Top-left
                    Vec2::new(1.0 - offset, 0.0),  // Top-right
                    Vec2::new(1.0, 1.0),           // Bottom-right
                    Vec2::new(0.0, 1.0),           // Bottom-left
                ]
            }
            KeystonePreset::Vertical => {
                let offset = amount.clamp(0.0, 0.5);
                [
                    Vec2::new(0.0, offset),        // Top-left
                    Vec2::new(1.0, offset),        // Top-right
                    Vec2::new(1.0, 1.0),           // Bottom-right
                    Vec2::new(0.0, 1.0),           // Bottom-left
                ]
            }
            KeystonePreset::Rotate => {
                let angle = amount * std::f32::consts::PI / 4.0; // Max 45 degrees
                let cos = angle.cos();
                let sin = angle.sin();
                [
                    Vec2::new(0.5 - cos * 0.5, 0.5 - sin * 0.5),
                    Vec2::new(0.5 + cos * 0.5, 0.5 - sin * 0.5),
                    Vec2::new(0.5 + cos * 0.5, 0.5 + sin * 0.5),
                    Vec2::new(0.5 - cos * 0.5, 0.5 + sin * 0.5),
                ]
            }
        }
    }

    /// Keystone correction presets
    #[derive(Debug, Clone, Copy)]
    pub enum KeystonePreset {
        Horizontal,
        Vertical,
        Rotate,
    }
}

#[cfg(test)]
mod phase2_tests {
    use super::*;

    #[test]
    fn test_grid_mesh() {
        let mesh = Mesh::create_grid(3, 3);
        assert_eq!(mesh.vertex_count(), 16); // 4x4 vertices
        assert_eq!(mesh.triangle_count(), 18); // 3x3 cells * 2 triangles
    }

    #[test]
    fn test_bezier_patch_planar() {
        let patch = BezierPatch::new();

        // Evaluate at corners
        let tl = patch.evaluate(0.0, 0.0);
        let tr = patch.evaluate(1.0, 0.0);
        let br = patch.evaluate(1.0, 1.0);
        let bl = patch.evaluate(0.0, 1.0);

        assert!((tl - Vec2::new(0.0, 0.0)).length() < 0.001);
        assert!((tr - Vec2::new(1.0, 0.0)).length() < 0.001);
        assert!((br - Vec2::new(1.0, 1.0)).length() < 0.001);
        assert!((bl - Vec2::new(0.0, 1.0)).length() < 0.001);
    }

    #[test]
    fn test_bezier_patch_corners() {
        let mut patch = BezierPatch::new();
        let corners = [
            Vec2::new(0.1, 0.1),
            Vec2::new(0.9, 0.2),
            Vec2::new(0.8, 0.9),
            Vec2::new(0.2, 0.8),
        ];
        patch.set_corners(corners);

        // Verify corners
        assert!((patch.evaluate(0.0, 0.0) - corners[0]).length() < 0.001);
        assert!((patch.evaluate(1.0, 0.0) - corners[1]).length() < 0.001);
        assert!((patch.evaluate(1.0, 1.0) - corners[2]).length() < 0.001);
        assert!((patch.evaluate(0.0, 1.0) - corners[3]).length() < 0.001);
    }

    #[test]
    fn test_keystone_application() {
        let mut mesh = Mesh::quad();
        let corners = [
            Vec2::new(0.1, 0.1),
            Vec2::new(0.9, 0.1),
            Vec2::new(1.0, 1.0),
            Vec2::new(0.0, 1.0),
        ];

        mesh.apply_keystone(corners);

        for i in 0..4 {
            assert!((mesh.vertices[i].position - corners[i]).length() < 0.001);
        }
    }

    #[test]
    fn test_keystone_preset() {
        let corners = keystone::preset_keystone(keystone::KeystonePreset::Horizontal, 0.1);

        assert!((corners[0].x - 0.1).abs() < 0.001);
        assert!((corners[1].x - 0.9).abs() < 0.001);
        assert!((corners[2] - Vec2::new(1.0, 1.0)).length() < 0.001);
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_quad_mesh() {
        let mesh = Mesh::quad();
        assert_eq!(mesh.vertex_count(), 4);
        assert_eq!(mesh.triangle_count(), 2);
        assert_eq!(mesh.mesh_type, MeshType::Quad);
    }

    #[test]
    fn test_triangle_mesh() {
        let mesh = Mesh::triangle();
        assert_eq!(mesh.vertex_count(), 3);
        assert_eq!(mesh.triangle_count(), 1);
        assert_eq!(mesh.mesh_type, MeshType::Triangle);
    }

    #[test]
    fn test_ellipse_mesh() {
        let mesh = Mesh::ellipse(Vec2::new(0.5, 0.5), 0.5, 0.5, 16);
        assert_eq!(mesh.vertex_count(), 17); // Center + 16 segments
        assert_eq!(mesh.triangle_count(), 16);
        assert_eq!(mesh.mesh_type, MeshType::Ellipse);
    }

    #[test]
    fn test_vertex_selection() {
        let mut mesh = Mesh::quad();
        mesh.vertices[0].selected = true;
        mesh.vertices[2].selected = true;

        let selected = mesh.selected_vertices();
        assert_eq!(selected.len(), 2);
        assert!(selected.contains(&0));
        assert!(selected.contains(&2));
    }

    #[test]
    fn test_translate_selected() {
        let mut mesh = Mesh::quad();
        mesh.vertices[0].selected = true;

        let original = mesh.vertices[0].position;
        mesh.translate_selected(Vec2::new(0.1, 0.2));

        assert!((mesh.vertices[0].position.x - (original.x + 0.1)).abs() < 0.001);
        assert!((mesh.vertices[0].position.y - (original.y + 0.2)).abs() < 0.001);
    }

    #[test]
    fn test_bounds() {
        let mesh = Mesh::quad_with_bounds(0.2, 0.3, 0.5, 0.6);
        let (min, max) = mesh.bounds().unwrap();

        assert!((min.x - 0.2).abs() < 0.001);
        assert!((min.y - 0.3).abs() < 0.001);
        assert!((max.x - 0.7).abs() < 0.001);
        assert!((max.y - 0.9).abs() < 0.001);
    }
}
