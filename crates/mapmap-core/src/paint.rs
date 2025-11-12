//! Paint - Media Source Abstraction
//!
//! A Paint represents a media source (video, image, test pattern) that can be
//! mapped onto surfaces through Mappings.

use glam::Vec2;
use serde::{Deserialize, Serialize};

/// Unique identifier for a Paint
pub type PaintId = u64;

/// Type of media source
#[derive(Debug, Clone, Copy, PartialEq, Eq, Serialize, Deserialize)]
pub enum PaintType {
    /// Video file or stream
    Video,
    /// Still image
    Image,
    /// Procedural test pattern
    TestPattern,
    /// Solid color
    Color,
    /// Camera/capture device
    Camera,
}

/// Paint - represents a media source
#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct Paint {
    pub id: PaintId,
    pub name: String,
    pub paint_type: PaintType,

    /// Source path (for Video/Image types)
    pub source_path: Option<String>,

    /// Playback rate (for Video type)
    pub rate: f32,

    /// Is the paint playing?
    pub is_playing: bool,

    /// Loop playback (for Video type)
    pub loop_playback: bool,

    /// Opacity (0.0 = transparent, 1.0 = opaque)
    pub opacity: f32,

    /// Color (for Color type) - RGBA
    pub color: [f32; 4],

    /// Lock aspect ratio when mapping
    pub lock_aspect: bool,

    /// Source dimensions (width x height)
    pub dimensions: Vec2,
}

impl Paint {
    /// Create a new Paint
    pub fn new(id: PaintId, name: impl Into<String>, paint_type: PaintType) -> Self {
        Self {
            id,
            name: name.into(),
            paint_type,
            source_path: None,
            rate: 1.0,
            is_playing: false,
            loop_playback: true,
            opacity: 1.0,
            color: [1.0, 1.0, 1.0, 1.0],
            lock_aspect: true,
            dimensions: Vec2::new(1920.0, 1080.0),
        }
    }

    /// Create a video paint
    pub fn video(id: PaintId, name: impl Into<String>, path: impl Into<String>) -> Self {
        let mut paint = Self::new(id, name, PaintType::Video);
        paint.source_path = Some(path.into());
        paint
    }

    /// Create an image paint
    pub fn image(id: PaintId, name: impl Into<String>, path: impl Into<String>) -> Self {
        let mut paint = Self::new(id, name, PaintType::Image);
        paint.source_path = Some(path.into());
        paint
    }

    /// Create a test pattern paint
    pub fn test_pattern(id: PaintId, name: impl Into<String>) -> Self {
        Self::new(id, name, PaintType::TestPattern)
    }

    /// Create a solid color paint
    pub fn color(id: PaintId, name: impl Into<String>, color: [f32; 4]) -> Self {
        let mut paint = Self::new(id, name, PaintType::Color);
        paint.color = color;
        paint
    }

    /// Get aspect ratio
    pub fn aspect_ratio(&self) -> f32 {
        if self.dimensions.y > 0.0 {
            self.dimensions.x / self.dimensions.y
        } else {
            16.0 / 9.0
        }
    }
}

/// Manages all paints in the project
#[derive(Debug, Clone, Default, Serialize, Deserialize)]
pub struct PaintManager {
    paints: Vec<Paint>,
    next_id: PaintId,
}

impl PaintManager {
    /// Create a new paint manager
    pub fn new() -> Self {
        Self {
            paints: Vec::new(),
            next_id: 1,
        }
    }

    /// Add a paint
    pub fn add_paint(&mut self, mut paint: Paint) -> PaintId {
        if paint.id == 0 {
            paint.id = self.next_id;
            self.next_id += 1;
        }
        let id = paint.id;
        self.paints.push(paint);
        id
    }

    /// Remove a paint
    pub fn remove_paint(&mut self, id: PaintId) -> Option<Paint> {
        self.paints
            .iter()
            .position(|p| p.id == id)
            .map(|index| self.paints.remove(index))
    }

    /// Get a paint by ID
    pub fn get_paint(&self, id: PaintId) -> Option<&Paint> {
        self.paints.iter().find(|p| p.id == id)
    }

    /// Get a mutable paint by ID
    pub fn get_paint_mut(&mut self, id: PaintId) -> Option<&mut Paint> {
        self.paints.iter_mut().find(|p| p.id == id)
    }

    /// Get all paints
    pub fn paints(&self) -> &[Paint] {
        &self.paints
    }

    /// Get all paints (mutable)
    pub fn paints_mut(&mut self) -> &mut [Paint] {
        &mut self.paints
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_paint_creation() {
        let paint = Paint::test_pattern(1, "Test");
        assert_eq!(paint.id, 1);
        assert_eq!(paint.name, "Test");
        assert_eq!(paint.paint_type, PaintType::TestPattern);
    }

    #[test]
    fn test_paint_manager() {
        let mut manager = PaintManager::new();

        let paint1 = Paint::test_pattern(0, "Test 1");
        let id1 = manager.add_paint(paint1);

        let paint2 = Paint::test_pattern(0, "Test 2");
        let id2 = manager.add_paint(paint2);

        assert_ne!(id1, id2);
        assert_eq!(manager.paints().len(), 2);

        manager.remove_paint(id1);
        assert_eq!(manager.paints().len(), 1);
    }

    #[test]
    fn test_aspect_ratio() {
        let paint = Paint::test_pattern(1, "Test");
        assert!((paint.aspect_ratio() - 16.0/9.0).abs() < 0.01);
    }
}
