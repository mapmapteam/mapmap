//! Layer system for compositing multiple video sources
//!
//! Layers provide a hierarchical structure for organizing and compositing
//! multiple media sources with different blend modes and transforms.

use crate::{Paint, Mapping};
use glam::{Mat4, Vec2};
use serde::{Deserialize, Serialize};
use std::collections::HashMap;

/// Blend mode for compositing layers
#[derive(Debug, Clone, Copy, PartialEq, Eq, Serialize, Deserialize)]
pub enum BlendMode {
    /// Normal alpha blending (default)
    Normal,
    /// Add colors (lighten)
    Add,
    /// Subtract colors (darken)
    Subtract,
    /// Multiply colors (darken)
    Multiply,
    /// Screen colors (lighten)
    Screen,
    /// Overlay (combination of multiply and screen)
    Overlay,
    /// Soft light
    SoftLight,
    /// Hard light
    HardLight,
    /// Lighten only (max)
    Lighten,
    /// Darken only (min)
    Darken,
    /// Color dodge
    ColorDodge,
    /// Color burn
    ColorBurn,
    /// Difference
    Difference,
    /// Exclusion
    Exclusion,
}

impl Default for BlendMode {
    fn default() -> Self {
        BlendMode::Normal
    }
}

impl BlendMode {
    /// Get shader function name for this blend mode
    pub fn shader_function(&self) -> &'static str {
        match self {
            BlendMode::Normal => "blend_normal",
            BlendMode::Add => "blend_add",
            BlendMode::Subtract => "blend_subtract",
            BlendMode::Multiply => "blend_multiply",
            BlendMode::Screen => "blend_screen",
            BlendMode::Overlay => "blend_overlay",
            BlendMode::SoftLight => "blend_soft_light",
            BlendMode::HardLight => "blend_hard_light",
            BlendMode::Lighten => "blend_lighten",
            BlendMode::Darken => "blend_darken",
            BlendMode::ColorDodge => "blend_color_dodge",
            BlendMode::ColorBurn => "blend_color_burn",
            BlendMode::Difference => "blend_difference",
            BlendMode::Exclusion => "blend_exclusion",
        }
    }

    /// List all available blend modes
    pub fn all() -> &'static [BlendMode] {
        &[
            BlendMode::Normal,
            BlendMode::Add,
            BlendMode::Subtract,
            BlendMode::Multiply,
            BlendMode::Screen,
            BlendMode::Overlay,
            BlendMode::SoftLight,
            BlendMode::HardLight,
            BlendMode::Lighten,
            BlendMode::Darken,
            BlendMode::ColorDodge,
            BlendMode::ColorBurn,
            BlendMode::Difference,
            BlendMode::Exclusion,
        ]
    }
}

/// A single layer in the composition
#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct Layer {
    pub id: u64,
    pub name: String,
    pub paint_id: Option<u64>,
    pub mapping_ids: Vec<u64>,
    pub blend_mode: BlendMode,
    pub opacity: f32,
    pub visible: bool,
    pub solo: bool,
    pub locked: bool,
    pub transform: Mat4,
}

impl Layer {
    /// Create a new layer
    pub fn new(id: u64, name: impl Into<String>) -> Self {
        Self {
            id,
            name: name.into(),
            paint_id: None,
            mapping_ids: Vec::new(),
            blend_mode: BlendMode::default(),
            opacity: 1.0,
            visible: true,
            solo: false,
            locked: false,
            transform: Mat4::IDENTITY,
        }
    }

    /// Set the paint for this layer
    pub fn with_paint(mut self, paint_id: u64) -> Self {
        self.paint_id = Some(paint_id);
        self
    }

    /// Set blend mode
    pub fn with_blend_mode(mut self, blend_mode: BlendMode) -> Self {
        self.blend_mode = blend_mode;
        self
    }

    /// Set opacity
    pub fn with_opacity(mut self, opacity: f32) -> Self {
        self.opacity = opacity.clamp(0.0, 1.0);
        self
    }

    /// Add a mapping to this layer
    pub fn add_mapping(&mut self, mapping_id: u64) {
        if !self.mapping_ids.contains(&mapping_id) {
            self.mapping_ids.push(mapping_id);
        }
    }

    /// Remove a mapping from this layer
    pub fn remove_mapping(&mut self, mapping_id: u64) {
        self.mapping_ids.retain(|&id| id != mapping_id);
    }

    /// Check if layer should be rendered
    pub fn should_render(&self) -> bool {
        self.visible && self.opacity > 0.0 && self.paint_id.is_some()
    }
}

/// Layer manager for organizing and rendering layers
#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct LayerManager {
    layers: Vec<Layer>,
    next_id: u64,
}

impl LayerManager {
    /// Create a new layer manager
    pub fn new() -> Self {
        Self {
            layers: Vec::new(),
            next_id: 1,
        }
    }

    /// Add a new layer
    pub fn add_layer(&mut self, mut layer: Layer) -> u64 {
        if layer.id == 0 {
            layer.id = self.next_id;
            self.next_id += 1;
        }
        let id = layer.id;
        self.layers.push(layer);
        id
    }

    /// Create and add a new layer
    pub fn create_layer(&mut self, name: impl Into<String>) -> u64 {
        let id = self.next_id;
        self.next_id += 1;
        let layer = Layer::new(id, name);
        self.layers.push(layer);
        id
    }

    /// Remove a layer by ID
    pub fn remove_layer(&mut self, id: u64) -> Option<Layer> {
        if let Some(index) = self.layers.iter().position(|l| l.id == id) {
            Some(self.layers.remove(index))
        } else {
            None
        }
    }

    /// Get a layer by ID
    pub fn get_layer(&self, id: u64) -> Option<&Layer> {
        self.layers.iter().find(|l| l.id == id)
    }

    /// Get a mutable layer by ID
    pub fn get_layer_mut(&mut self, id: u64) -> Option<&mut Layer> {
        self.layers.iter_mut().find(|l| l.id == id)
    }

    /// Get all layers
    pub fn layers(&self) -> &[Layer] {
        &self.layers
    }

    /// Get all visible layers in render order
    pub fn visible_layers(&self) -> Vec<&Layer> {
        // Check if any layer is solo'd
        let has_solo = self.layers.iter().any(|l| l.solo);

        self.layers
            .iter()
            .filter(|layer| {
                if has_solo {
                    // Only render solo layers when any layer is solo'd
                    layer.solo && layer.should_render()
                } else {
                    layer.should_render()
                }
            })
            .collect()
    }

    /// Move layer up in stack (higher z-order)
    pub fn move_layer_up(&mut self, id: u64) -> bool {
        if let Some(index) = self.layers.iter().position(|l| l.id == id) {
            if index < self.layers.len() - 1 {
                self.layers.swap(index, index + 1);
                return true;
            }
        }
        false
    }

    /// Move layer down in stack (lower z-order)
    pub fn move_layer_down(&mut self, id: u64) -> bool {
        if let Some(index) = self.layers.iter().position(|l| l.id == id) {
            if index > 0 {
                self.layers.swap(index, index - 1);
                return true;
            }
        }
        false
    }

    /// Move layer to specific index
    pub fn move_layer_to(&mut self, id: u64, new_index: usize) -> bool {
        if let Some(old_index) = self.layers.iter().position(|l| l.id == id) {
            if new_index < self.layers.len() {
                let layer = self.layers.remove(old_index);
                self.layers.insert(new_index, layer);
                return true;
            }
        }
        false
    }

    /// Get number of layers
    pub fn len(&self) -> usize {
        self.layers.len()
    }

    /// Check if manager is empty
    pub fn is_empty(&self) -> bool {
        self.layers.is_empty()
    }

    /// Clear all layers
    pub fn clear(&mut self) {
        self.layers.clear();
    }

    /// Duplicate a layer
    pub fn duplicate_layer(&mut self, id: u64) -> Option<u64> {
        if let Some(layer) = self.get_layer(id).cloned() {
            let new_id = self.next_id;
            self.next_id += 1;
            let mut new_layer = layer;
            new_layer.id = new_id;
            new_layer.name = format!("{} (copy)", new_layer.name);
            self.layers.push(new_layer);
            Some(new_id)
        } else {
            None
        }
    }
}

impl Default for LayerManager {
    fn default() -> Self {
        Self::new()
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_blend_mode_shader_function() {
        assert_eq!(BlendMode::Normal.shader_function(), "blend_normal");
        assert_eq!(BlendMode::Multiply.shader_function(), "blend_multiply");
        assert_eq!(BlendMode::Screen.shader_function(), "blend_screen");
    }

    #[test]
    fn test_layer_creation() {
        let layer = Layer::new(1, "Test Layer")
            .with_paint(100)
            .with_blend_mode(BlendMode::Multiply)
            .with_opacity(0.5);

        assert_eq!(layer.id, 1);
        assert_eq!(layer.name, "Test Layer");
        assert_eq!(layer.paint_id, Some(100));
        assert_eq!(layer.blend_mode, BlendMode::Multiply);
        assert_eq!(layer.opacity, 0.5);
        assert!(layer.visible);
    }

    #[test]
    fn test_layer_should_render() {
        let mut layer = Layer::new(1, "Test");

        // Not visible without paint
        assert!(!layer.should_render());

        layer.paint_id = Some(100);
        assert!(layer.should_render());

        layer.visible = false;
        assert!(!layer.should_render());

        layer.visible = true;
        layer.opacity = 0.0;
        assert!(!layer.should_render());
    }

    #[test]
    fn test_layer_manager_basic() {
        let mut manager = LayerManager::new();

        let id1 = manager.create_layer("Layer 1");
        let id2 = manager.create_layer("Layer 2");

        assert_eq!(manager.len(), 2);
        assert!(manager.get_layer(id1).is_some());
        assert!(manager.get_layer(id2).is_some());
    }

    #[test]
    fn test_layer_manager_remove() {
        let mut manager = LayerManager::new();

        let id = manager.create_layer("Test Layer");
        assert_eq!(manager.len(), 1);

        let removed = manager.remove_layer(id);
        assert!(removed.is_some());
        assert_eq!(manager.len(), 0);
    }

    #[test]
    fn test_layer_manager_reorder() {
        let mut manager = LayerManager::new();

        let id1 = manager.create_layer("Layer 1");
        let id2 = manager.create_layer("Layer 2");
        let id3 = manager.create_layer("Layer 3");

        // Initially: [1, 2, 3]
        assert_eq!(manager.layers()[0].id, id1);
        assert_eq!(manager.layers()[2].id, id3);

        // Move layer 1 up: [2, 1, 3]
        manager.move_layer_up(id1);
        assert_eq!(manager.layers()[0].id, id2);
        assert_eq!(manager.layers()[1].id, id1);

        // Move layer 3 down: [3, 2, 1]
        manager.move_layer_down(id3);
        manager.move_layer_down(id3);
        assert_eq!(manager.layers()[0].id, id3);
    }

    #[test]
    fn test_layer_manager_visible_layers() {
        let mut manager = LayerManager::new();

        let id1 = manager.create_layer("Layer 1");
        let id2 = manager.create_layer("Layer 2");
        let id3 = manager.create_layer("Layer 3");

        // Set paint IDs so they can render
        manager.get_layer_mut(id1).unwrap().paint_id = Some(100);
        manager.get_layer_mut(id2).unwrap().paint_id = Some(101);
        manager.get_layer_mut(id3).unwrap().paint_id = Some(102);

        // All visible
        assert_eq!(manager.visible_layers().len(), 3);

        // Hide one layer
        manager.get_layer_mut(id2).unwrap().visible = false;
        assert_eq!(manager.visible_layers().len(), 2);

        // Solo one layer
        manager.get_layer_mut(id1).unwrap().solo = true;
        assert_eq!(manager.visible_layers().len(), 1);
        assert_eq!(manager.visible_layers()[0].id, id1);
    }

    #[test]
    fn test_layer_manager_duplicate() {
        let mut manager = LayerManager::new();

        let id1 = manager.create_layer("Original");
        manager.get_layer_mut(id1).unwrap().paint_id = Some(100);
        manager.get_layer_mut(id1).unwrap().opacity = 0.7;

        let id2 = manager.duplicate_layer(id1).unwrap();

        assert_eq!(manager.len(), 2);
        let dup = manager.get_layer(id2).unwrap();
        assert!(dup.name.contains("copy"));
        assert_eq!(dup.paint_id, Some(100));
        assert_eq!(dup.opacity, 0.7);
    }

    #[test]
    fn test_layer_mappings() {
        let mut layer = Layer::new(1, "Test");

        layer.add_mapping(10);
        layer.add_mapping(20);
        assert_eq!(layer.mapping_ids.len(), 2);

        // Adding duplicate should not increase count
        layer.add_mapping(10);
        assert_eq!(layer.mapping_ids.len(), 2);

        layer.remove_mapping(10);
        assert_eq!(layer.mapping_ids.len(), 1);
        assert_eq!(layer.mapping_ids[0], 20);
    }
}
