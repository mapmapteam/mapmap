//! Output Management - Multi-Window and Multi-Projector Support
//!
//! Phase 2 feature: Multiple independent output windows for multi-projector setups

use glam::Vec2;
use serde::{Deserialize, Serialize};

/// Unique identifier for an output window
pub type OutputId = u64;

/// Rectangular region in normalized canvas coordinates (0.0-1.0)
#[derive(Debug, Clone, Copy, PartialEq, Serialize, Deserialize)]
pub struct CanvasRegion {
    pub x: f32,
    pub y: f32,
    pub width: f32,
    pub height: f32,
}

impl CanvasRegion {
    /// Create a new canvas region
    pub fn new(x: f32, y: f32, width: f32, height: f32) -> Self {
        Self { x, y, width, height }
    }

    /// Check if this region intersects with another region
    pub fn intersects(&self, other: &CanvasRegion) -> bool {
        !(self.x + self.width < other.x
            || other.x + other.width < self.x
            || self.y + self.height < other.y
            || other.y + other.height < self.y)
    }

    /// Get the overlap region with another region
    pub fn intersection(&self, other: &CanvasRegion) -> Option<CanvasRegion> {
        if !self.intersects(other) {
            return None;
        }

        let x = self.x.max(other.x);
        let y = self.y.max(other.y);
        let right = (self.x + self.width).min(other.x + other.width);
        let bottom = (self.y + self.height).min(other.y + other.height);

        Some(CanvasRegion::new(x, y, right - x, bottom - y))
    }

    /// Convert to absolute pixel coordinates given canvas size
    pub fn to_pixels(&self, canvas_width: u32, canvas_height: u32) -> (i32, i32, u32, u32) {
        let x = (self.x * canvas_width as f32) as i32;
        let y = (self.y * canvas_height as f32) as i32;
        let width = (self.width * canvas_width as f32) as u32;
        let height = (self.height * canvas_height as f32) as u32;
        (x, y, width, height)
    }
}

/// Edge blending configuration for seamless projector overlap
#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct EdgeBlendConfig {
    pub left: EdgeBlendZone,
    pub right: EdgeBlendZone,
    pub top: EdgeBlendZone,
    pub bottom: EdgeBlendZone,
    pub gamma: f32, // Blend curve gamma (typically 2.2)
}

impl Default for EdgeBlendConfig {
    fn default() -> Self {
        Self {
            left: EdgeBlendZone::default(),
            right: EdgeBlendZone::default(),
            top: EdgeBlendZone::default(),
            bottom: EdgeBlendZone::default(),
            gamma: 2.2,
        }
    }
}

/// Configuration for one edge of the blend zone
#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct EdgeBlendZone {
    pub enabled: bool,
    pub width: f32,   // 0.0-0.5 (percentage of output width/height)
    pub offset: f32,  // Shift blend zone inward/outward (-0.1 to 0.1)
}

impl Default for EdgeBlendZone {
    fn default() -> Self {
        Self {
            enabled: false,
            width: 0.1,
            offset: 0.0,
        }
    }
}

/// Color calibration for per-output color correction
#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct ColorCalibration {
    pub brightness: f32,        // -1.0 to 1.0
    pub contrast: f32,           // 0.0 to 2.0
    pub gamma: Vec2,           // Per-channel gamma (R, G, B) - using Vec3 requires glam feature
    pub gamma_b: f32,            // Blue gamma (separate due to Vec2)
    pub color_temp: f32,         // 2000K to 10000K
    pub saturation: f32,         // 0.0 to 2.0
}

impl Default for ColorCalibration {
    fn default() -> Self {
        Self {
            brightness: 0.0,
            contrast: 1.0,
            gamma: Vec2::new(1.0, 1.0), // R, G
            gamma_b: 1.0,                // B
            color_temp: 6500.0,          // D65 standard
            saturation: 1.0,
        }
    }
}

/// Configuration for a single output window (projector)
#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct OutputConfig {
    pub id: OutputId,
    pub name: String,
    pub canvas_region: CanvasRegion,
    pub resolution: (u32, u32),
    pub edge_blend: EdgeBlendConfig,
    pub color_calibration: ColorCalibration,
    pub fullscreen: bool,
}

impl OutputConfig {
    /// Create a new output configuration
    pub fn new(id: OutputId, name: String, canvas_region: CanvasRegion, resolution: (u32, u32)) -> Self {
        Self {
            id,
            name,
            canvas_region,
            resolution,
            edge_blend: EdgeBlendConfig::default(),
            color_calibration: ColorCalibration::default(),
            fullscreen: false,
        }
    }
}

/// Manages multiple output configurations
#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct OutputManager {
    outputs: Vec<OutputConfig>,
    canvas_size: (u32, u32),
    next_id: u64,
}

impl OutputManager {
    /// Create a new output manager
    pub fn new(canvas_size: (u32, u32)) -> Self {
        Self {
            outputs: Vec::new(),
            canvas_size,
            next_id: 1,
        }
    }

    /// Add a new output
    pub fn add_output(&mut self, name: String, canvas_region: CanvasRegion, resolution: (u32, u32)) -> OutputId {
        let id = self.next_id;
        self.next_id += 1;

        let output = OutputConfig::new(id, name, canvas_region, resolution);
        self.outputs.push(output);
        id
    }

    /// Remove an output by ID
    pub fn remove_output(&mut self, id: OutputId) -> Option<OutputConfig> {
        if let Some(index) = self.outputs.iter().position(|o| o.id == id) {
            Some(self.outputs.remove(index))
        } else {
            None
        }
    }

    /// Get an output by ID
    pub fn get_output(&self, id: OutputId) -> Option<&OutputConfig> {
        self.outputs.iter().find(|o| o.id == id)
    }

    /// Get a mutable reference to an output by ID
    pub fn get_output_mut(&mut self, id: OutputId) -> Option<&mut OutputConfig> {
        self.outputs.iter_mut().find(|o| o.id == id)
    }

    /// Get all outputs
    pub fn outputs(&self) -> &[OutputConfig] {
        &self.outputs
    }

    /// Get canvas size
    pub fn canvas_size(&self) -> (u32, u32) {
        self.canvas_size
    }

    /// Set canvas size
    pub fn set_canvas_size(&mut self, width: u32, height: u32) {
        self.canvas_size = (width, height);
    }

    /// Create a 2x2 projector array with automatic configuration
    pub fn create_projector_array_2x2(
        &mut self,
        projector_resolution: (u32, u32),
        overlap: f32, // 0.0-0.5
    ) {
        let effective_width = projector_resolution.0 as f32 * (1.0 - overlap);
        let effective_height = projector_resolution.1 as f32 * (1.0 - overlap);

        // Update canvas size to accommodate 2x2 grid
        let total_width = (effective_width * 2.0) as u32;
        let total_height = (effective_height * 2.0) as u32;
        self.set_canvas_size(total_width, total_height);

        // Clear existing outputs
        self.outputs.clear();
        self.next_id = 1;

        let cell_width = 1.0 / 2.0;
        let cell_height = 1.0 / 2.0;

        // Create 2x2 grid of outputs
        for row in 0..2 {
            for col in 0..2 {
                let x = col as f32 * cell_width;
                let y = row as f32 * cell_height;

                // Determine which edges need blending
                let has_left = col > 0;
                let has_right = col < 1;
                let has_top = row > 0;
                let has_bottom = row < 1;

                let canvas_region = CanvasRegion::new(x, y, cell_width, cell_height);
                let id = self.add_output(
                    format!("Projector {}-{}", row + 1, col + 1),
                    canvas_region,
                    projector_resolution,
                );

                // Configure edge blending for overlapping edges
                if let Some(output) = self.get_output_mut(id) {
                    output.edge_blend = EdgeBlendConfig {
                        left: EdgeBlendZone {
                            enabled: has_left,
                            width: overlap,
                            offset: 0.0,
                        },
                        right: EdgeBlendZone {
                            enabled: has_right,
                            width: overlap,
                            offset: 0.0,
                        },
                        top: EdgeBlendZone {
                            enabled: has_top,
                            width: overlap,
                            offset: 0.0,
                        },
                        bottom: EdgeBlendZone {
                            enabled: has_bottom,
                            width: overlap,
                            offset: 0.0,
                        },
                        gamma: 2.2,
                    };
                }
            }
        }
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_canvas_region_intersection() {
        let region1 = CanvasRegion::new(0.0, 0.0, 0.5, 0.5);
        let region2 = CanvasRegion::new(0.25, 0.25, 0.5, 0.5);

        assert!(region1.intersects(&region2));

        let intersection = region1.intersection(&region2).unwrap();
        assert_eq!(intersection.x, 0.25);
        assert_eq!(intersection.y, 0.25);
        assert_eq!(intersection.width, 0.25);
        assert_eq!(intersection.height, 0.25);
    }

    #[test]
    fn test_output_manager() {
        let mut manager = OutputManager::new((1920, 1080));

        let id = manager.add_output(
            "Output 1".to_string(),
            CanvasRegion::new(0.0, 0.0, 0.5, 1.0),
            (1920, 1080),
        );

        assert_eq!(manager.outputs().len(), 1);
        assert_eq!(manager.get_output(id).unwrap().name, "Output 1");

        manager.remove_output(id);
        assert_eq!(manager.outputs().len(), 0);
    }

    #[test]
    fn test_projector_array_2x2() {
        let mut manager = OutputManager::new((3840, 2160));
        manager.create_projector_array_2x2((1920, 1080), 0.1);

        assert_eq!(manager.outputs().len(), 4);

        // Check that edge blending is configured correctly
        for output in manager.outputs() {
            // At least one edge should be enabled for blending
            let blend_count = [
                output.edge_blend.left.enabled,
                output.edge_blend.right.enabled,
                output.edge_blend.top.enabled,
                output.edge_blend.bottom.enabled,
            ]
            .iter()
            .filter(|&&x| x)
            .count();

            assert!(blend_count >= 2); // Corner projectors have 2 edges, others have more
        }
    }
}
