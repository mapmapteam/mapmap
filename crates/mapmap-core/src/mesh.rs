//! Mesh - Geometry for Warping
//!
//! Defines the shape and warping of mapped content

use glam::{Vec2, Vec3};
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
