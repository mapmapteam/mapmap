//! Control target abstraction
//!
//! This module provides a unified abstraction for all controllable parameters in MapMap.

use serde::{Deserialize, Serialize};

/// A controllable parameter in the application
#[derive(Debug, Clone, PartialEq, Eq, Hash, Serialize, Deserialize)]
pub enum ControlTarget {
    /// Layer opacity (layer_id, opacity: 0.0-1.0)
    LayerOpacity(u32),
    /// Layer position (layer_id)
    LayerPosition(u32),
    /// Layer scale (layer_id)
    LayerScale(u32),
    /// Layer rotation (layer_id, degrees)
    LayerRotation(u32),
    /// Layer visibility (layer_id)
    LayerVisibility(u32),
    /// Paint parameter (paint_id, param_name)
    PaintParameter(u32, String),
    /// Effect parameter (effect_id, param_name)
    EffectParameter(u32, String),
    /// Playback speed (global or per-layer)
    PlaybackSpeed(Option<u32>),
    /// Playback position (0.0-1.0)
    PlaybackPosition,
    /// Output brightness (output_id, brightness: 0.0-1.0)
    OutputBrightness(u32),
    /// Output edge blend (output_id, edge, width: 0.0-1.0)
    OutputEdgeBlend(u32, EdgeSide),
    /// Custom parameter (name)
    Custom(String),
}

/// Edge sides for edge blending
#[derive(Debug, Clone, Copy, PartialEq, Eq, Hash, Serialize, Deserialize)]
pub enum EdgeSide {
    Left,
    Right,
    Top,
    Bottom,
}

/// Control value types
#[derive(Debug, Clone, PartialEq, Serialize, Deserialize)]
pub enum ControlValue {
    Float(f32),
    Int(i32),
    Bool(bool),
    String(String),
    Color(u32), // RGBA
    Vec2(f32, f32),
    Vec3(f32, f32, f32),
}

impl ControlValue {
    /// Get as float, converting if necessary
    pub fn as_float(&self) -> Option<f32> {
        match self {
            ControlValue::Float(v) => Some(*v),
            ControlValue::Int(v) => Some(*v as f32),
            ControlValue::Bool(v) => Some(if *v { 1.0 } else { 0.0 }),
            _ => None,
        }
    }

    /// Get as int, converting if necessary
    pub fn as_int(&self) -> Option<i32> {
        match self {
            ControlValue::Int(v) => Some(*v),
            ControlValue::Float(v) => Some(*v as i32),
            ControlValue::Bool(v) => Some(if *v { 1 } else { 0 }),
            _ => None,
        }
    }

    /// Get as bool, converting if necessary
    pub fn as_bool(&self) -> Option<bool> {
        match self {
            ControlValue::Bool(v) => Some(*v),
            ControlValue::Int(v) => Some(*v != 0),
            ControlValue::Float(v) => Some(*v != 0.0),
            _ => None,
        }
    }

    /// Get as string
    pub fn as_string(&self) -> Option<&str> {
        match self {
            ControlValue::String(v) => Some(v),
            _ => None,
        }
    }
}

impl From<f32> for ControlValue {
    fn from(v: f32) -> Self {
        ControlValue::Float(v)
    }
}

impl From<i32> for ControlValue {
    fn from(v: i32) -> Self {
        ControlValue::Int(v)
    }
}

impl From<bool> for ControlValue {
    fn from(v: bool) -> Self {
        ControlValue::Bool(v)
    }
}

impl From<String> for ControlValue {
    fn from(v: String) -> Self {
        ControlValue::String(v)
    }
}

impl From<(f32, f32)> for ControlValue {
    fn from((x, y): (f32, f32)) -> Self {
        ControlValue::Vec2(x, y)
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_control_value_conversions() {
        let float_val = ControlValue::Float(0.75);
        assert_eq!(float_val.as_float(), Some(0.75));
        assert_eq!(float_val.as_int(), Some(0));

        let int_val = ControlValue::Int(42);
        assert_eq!(int_val.as_int(), Some(42));
        assert_eq!(int_val.as_float(), Some(42.0));

        let bool_val = ControlValue::Bool(true);
        assert_eq!(bool_val.as_bool(), Some(true));
        assert_eq!(bool_val.as_float(), Some(1.0));
        assert_eq!(bool_val.as_int(), Some(1));
    }

    #[test]
    fn test_control_target_serialization() {
        let target = ControlTarget::LayerOpacity(5);
        let json = serde_json::to_string(&target).unwrap();
        let deserialized: ControlTarget = serde_json::from_str(&json).unwrap();
        assert_eq!(target, deserialized);
    }
}
