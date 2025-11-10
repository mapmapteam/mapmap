//! MapMap Core - Domain Model and Data Structures
//!
//! This crate contains the core domain model for MapMap, including:
//! - Paint/Mapping/Shape hierarchy
//! - Layer system for compositing
//! - Project file format
//! - Geometry primitives
//! - Transform calculations

use glam::{Mat4, Vec2, Vec3};
use serde::{Deserialize, Serialize};
use thiserror::Error;

pub mod layer;
pub use layer::{Layer, LayerManager, BlendMode};

/// Core error types
#[derive(Error, Debug)]
pub enum CoreError {
    #[error("Invalid geometry: {0}")]
    InvalidGeometry(String),

    #[error("Transform error: {0}")]
    TransformError(String),
}

/// Result type for core operations
pub type Result<T> = std::result::Result<T, CoreError>;

/// Represents a 2D point with texture coordinates
#[derive(Debug, Clone, Copy, Serialize, Deserialize)]
pub struct Vertex {
    pub position: Vec2,
    pub uv: Vec2,
}

impl Vertex {
    pub fn new(x: f32, y: f32, u: f32, v: f32) -> Self {
        Self {
            position: Vec2::new(x, y),
            uv: Vec2::new(u, v),
        }
    }
}

/// Represents a quadrilateral mesh
#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct Quad {
    pub vertices: [Vertex; 4],
}

impl Quad {
    /// Create a unit quad (0,0) to (1,1)
    pub fn unit() -> Self {
        Self {
            vertices: [
                Vertex::new(-1.0, -1.0, 0.0, 0.0),
                Vertex::new(1.0, -1.0, 1.0, 0.0),
                Vertex::new(1.0, 1.0, 1.0, 1.0),
                Vertex::new(-1.0, 1.0, 0.0, 1.0),
            ],
        }
    }

    /// Apply a transform matrix to all vertices
    pub fn transform(&mut self, mat: Mat4) {
        for vertex in &mut self.vertices {
            let pos = mat.transform_point3(Vec3::new(vertex.position.x, vertex.position.y, 0.0));
            vertex.position = Vec2::new(pos.x, pos.y);
        }
    }
}

/// Shape trait - represents any mappable geometry
pub trait Shape: Send + Sync {
    fn vertices(&self) -> &[Vertex];
    fn indices(&self) -> &[u16];
    fn update(&mut self, delta_time: f32);
}

/// Paint - represents a media source
#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct Paint {
    pub id: u64,
    pub name: String,
    pub source: MediaSource,
    pub opacity: f32,
}

/// Media source types
#[derive(Debug, Clone, Serialize, Deserialize)]
pub enum MediaSource {
    Video { path: String },
    Image { path: String },
    Color { r: f32, g: f32, b: f32, a: f32 },
}

/// Mapping - connects a Paint to a Shape
#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct Mapping {
    pub id: u64,
    pub paint_id: u64,
    pub shape: ShapeType,
    pub visible: bool,
}

/// Shape types
#[derive(Debug, Clone, Serialize, Deserialize)]
pub enum ShapeType {
    Quad(Quad),
    Mesh { vertices: Vec<Vertex>, indices: Vec<u16> },
}

/// Project - top-level container
#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct Project {
    pub name: String,
    pub paints: Vec<Paint>,
    pub mappings: Vec<Mapping>,
}

impl Project {
    pub fn new(name: impl Into<String>) -> Self {
        Self {
            name: name.into(),
            paints: Vec::new(),
            mappings: Vec::new(),
        }
    }

    pub fn add_paint(&mut self, paint: Paint) {
        self.paints.push(paint);
    }

    pub fn add_mapping(&mut self, mapping: Mapping) {
        self.mappings.push(mapping);
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_quad_creation() {
        let quad = Quad::unit();
        assert_eq!(quad.vertices.len(), 4);
    }

    #[test]
    fn test_project_creation() {
        let mut project = Project::new("Test Project");
        assert_eq!(project.name, "Test Project");
        assert_eq!(project.paints.len(), 0);
        assert_eq!(project.mappings.len(), 0);

        let paint = Paint {
            id: 1,
            name: "Test Paint".to_string(),
            source: MediaSource::Color { r: 1.0, g: 0.0, b: 0.0, a: 1.0 },
            opacity: 1.0,
        };
        project.add_paint(paint);
        assert_eq!(project.paints.len(), 1);
    }

    #[test]
    fn test_quad_transform() {
        let mut quad = Quad::unit();
        let scale = Mat4::from_scale(Vec3::new(2.0, 2.0, 1.0));
        quad.transform(scale);

        // Check that vertices were scaled
        assert!((quad.vertices[0].position.x - (-2.0)).abs() < 0.001);
        assert!((quad.vertices[0].position.y - (-2.0)).abs() < 0.001);
    }
}
