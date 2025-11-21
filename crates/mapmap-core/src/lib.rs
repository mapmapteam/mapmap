//! MapMap Core - Domain Model and Data Structures
//!
//! This crate contains the core domain model for MapMap, including:
//! - Paint/Mapping/Mesh hierarchy
//! - Layer system for compositing
//! - Project file format
//! - Geometry primitives
//! - Transform calculations

use glam::{Mat4, Vec2, Vec3};
use serde::{Deserialize, Serialize};
use thiserror::Error;

// Phase 1: Layer system for compositing
pub mod layer;
pub use layer::{BlendMode, Layer, LayerManager, Transform, ResizeMode, Composition};

// Phase 2: Multi-output and projection mapping
pub mod output;
pub mod monitor;
pub mod paint;
pub mod mesh;
pub mod mapping;

// Phase 3: Effects Pipeline
pub mod shader_graph;
pub mod animation;
pub mod audio;
pub mod codegen;
pub mod lut;
pub mod audio_reactive;
pub mod oscillator;

pub use output::{OutputManager, OutputConfig, OutputId, CanvasRegion, EdgeBlendConfig, EdgeBlendZone, ColorCalibration};
pub use monitor::{MonitorInfo, MonitorTopology};
pub use paint::{Paint, PaintId, PaintManager, PaintType};
pub use mesh::{Mesh, MeshType, MeshVertex, VertexId, BezierPatch, keystone};
pub use mapping::{Mapping, MappingId, MappingManager};
pub use shader_graph::{
    ShaderGraph, ShaderNode, NodeType, NodeId, GraphId,
    DataType, InputSocket, OutputSocket, ParameterValue,
};
pub use animation::{
    AnimationClip, AnimationTrack, AnimationPlayer, Keyframe,
    AnimValue, InterpolationMode, TimePoint,
};
pub use audio::{
    AudioAnalyzer, AudioAnalysis, AudioConfig, AudioSource,
    AudioReactiveMapping, AudioMappingType, FrequencyBand,
};
pub use codegen::{WGSLCodegen, CodegenError};
pub use lut::{Lut3D, LutManager, LutPreset, LutFormat, LutError};
pub use audio_reactive::{
    AudioReactiveController, AudioReactiveAnimationSystem,
    AudioReactivePreset, AudioAnimationBlendMode,
};
pub use oscillator::{
    OscillatorConfig, SimulationResolution, PhaseInitMode,
    ColorMode, CoordinateMode, RingParams,
};

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
/// (Legacy - will be replaced by Mesh in Phase 2)
pub trait Shape: Send + Sync {
    fn vertices(&self) -> &[Vertex];
    fn indices(&self) -> &[u16];
    fn update(&mut self, delta_time: f32);
}

/// Legacy shape types (Phase 0/1)
#[derive(Debug, Clone, Serialize, Deserialize)]
pub enum ShapeType {
    Quad(Quad),
    Mesh { vertices: Vec<Vertex>, indices: Vec<u16> },
}

/// Project - top-level container (Phase 2+)
#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct Project {
    pub name: String,
    pub paint_manager: PaintManager,
    pub mapping_manager: MappingManager,
    pub layer_manager: LayerManager,
}

impl Project {
    pub fn new(name: impl Into<String>) -> Self {
        Self {
            name: name.into(),
            paint_manager: PaintManager::new(),
            mapping_manager: MappingManager::new(),
            layer_manager: LayerManager::new(),
        }
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
        let project = Project::new("Test Project");
        assert_eq!(project.name, "Test Project");
        assert_eq!(project.paint_manager.paints().len(), 0);
        assert_eq!(project.mapping_manager.mappings().len(), 0);
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
