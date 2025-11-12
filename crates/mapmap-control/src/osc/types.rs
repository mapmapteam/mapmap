//! OSC type conversion helpers

#[cfg(feature = "osc")]
use rosc::OscType;
#[cfg(feature = "osc")]
use crate::{ControlValue, error::ControlError, Result};

/// Convert OSC arguments to ControlValue
#[cfg(feature = "osc")]
pub fn osc_to_control_value(osc_args: &[OscType]) -> Result<ControlValue> {
    if osc_args.is_empty() {
        return Err(ControlError::InvalidMessage("No OSC arguments".to_string()));
    }

    match &osc_args[0] {
        OscType::Float(f) => Ok(ControlValue::Float(*f)),
        OscType::Int(i) => Ok(ControlValue::Int(*i)),
        OscType::String(s) => Ok(ControlValue::String(s.clone())),
        OscType::Bool(b) => Ok(ControlValue::Bool(*b)),
        OscType::Color(color) => {
            // OscColor has r, g, b, a fields (each u8)
            let rgba = ((color.red as u32) << 24)
                | ((color.green as u32) << 16)
                | ((color.blue as u32) << 8)
                | (color.alpha as u32);
            Ok(ControlValue::Color(rgba))
        }
        OscType::Double(d) => Ok(ControlValue::Float(*d as f32)),
        OscType::Long(l) => Ok(ControlValue::Int(*l as i32)),
        _ => Err(ControlError::InvalidMessage(format!(
            "Unsupported OSC type: {:?}",
            osc_args[0]
        ))),
    }
}

/// Convert multiple OSC arguments to Vec2
#[cfg(feature = "osc")]
pub fn osc_to_vec2(osc_args: &[OscType]) -> Result<ControlValue> {
    if osc_args.len() < 2 {
        return Err(ControlError::InvalidMessage(
            "Need at least 2 OSC arguments for Vec2".to_string(),
        ));
    }

    let x = match &osc_args[0] {
        OscType::Float(f) => *f,
        OscType::Int(i) => *i as f32,
        OscType::Double(d) => *d as f32,
        _ => {
            return Err(ControlError::InvalidMessage(
                "Invalid OSC type for Vec2 x".to_string(),
            ))
        }
    };

    let y = match &osc_args[1] {
        OscType::Float(f) => *f,
        OscType::Int(i) => *i as f32,
        OscType::Double(d) => *d as f32,
        _ => {
            return Err(ControlError::InvalidMessage(
                "Invalid OSC type for Vec2 y".to_string(),
            ))
        }
    };

    Ok(ControlValue::Vec2(x, y))
}

/// Convert multiple OSC arguments to Vec3
#[cfg(feature = "osc")]
pub fn osc_to_vec3(osc_args: &[OscType]) -> Result<ControlValue> {
    if osc_args.len() < 3 {
        return Err(ControlError::InvalidMessage(
            "Need at least 3 OSC arguments for Vec3".to_string(),
        ));
    }

    let x = match &osc_args[0] {
        OscType::Float(f) => *f,
        OscType::Int(i) => *i as f32,
        OscType::Double(d) => *d as f32,
        _ => {
            return Err(ControlError::InvalidMessage(
                "Invalid OSC type for Vec3 x".to_string(),
            ))
        }
    };

    let y = match &osc_args[1] {
        OscType::Float(f) => *f,
        OscType::Int(i) => *i as f32,
        OscType::Double(d) => *d as f32,
        _ => {
            return Err(ControlError::InvalidMessage(
                "Invalid OSC type for Vec3 y".to_string(),
            ))
        }
    };

    let z = match &osc_args[2] {
        OscType::Float(f) => *f,
        OscType::Int(i) => *i as f32,
        OscType::Double(d) => *d as f32,
        _ => {
            return Err(ControlError::InvalidMessage(
                "Invalid OSC type for Vec3 z".to_string(),
            ))
        }
    };

    Ok(ControlValue::Vec3(x, y, z))
}

/// Convert ControlValue to OSC type
#[cfg(feature = "osc")]
pub fn control_value_to_osc(value: &ControlValue) -> Vec<OscType> {
    match value {
        ControlValue::Float(f) => vec![OscType::Float(*f)],
        ControlValue::Int(i) => vec![OscType::Int(*i)],
        ControlValue::Bool(b) => vec![OscType::Bool(*b)],
        ControlValue::String(s) => vec![OscType::String(s.clone())],
        ControlValue::Color(c) => {
            // Convert RGBA u32 to OscColor
            let red = ((c >> 24) & 0xFF) as u8;
            let green = ((c >> 16) & 0xFF) as u8;
            let blue = ((c >> 8) & 0xFF) as u8;
            let alpha = (c & 0xFF) as u8;
            vec![OscType::Color(rosc::OscColor {
                red,
                green,
                blue,
                alpha,
            })]
        }
        ControlValue::Vec2(x, y) => vec![OscType::Float(*x), OscType::Float(*y)],
        ControlValue::Vec3(x, y, z) => {
            vec![OscType::Float(*x), OscType::Float(*y), OscType::Float(*z)]
        }
    }
}

#[cfg(all(test, feature = "osc"))]
mod tests {
    use super::*;

    #[test]
    fn test_osc_to_control_value() {
        let args = vec![OscType::Float(0.5)];
        let value = osc_to_control_value(&args).unwrap();
        assert_eq!(value, ControlValue::Float(0.5));

        let args = vec![OscType::Int(42)];
        let value = osc_to_control_value(&args).unwrap();
        assert_eq!(value, ControlValue::Int(42));

        let args = vec![OscType::Bool(true)];
        let value = osc_to_control_value(&args).unwrap();
        assert_eq!(value, ControlValue::Bool(true));
    }

    #[test]
    fn test_osc_to_vec2() {
        let args = vec![OscType::Float(1.0), OscType::Float(2.0)];
        let value = osc_to_vec2(&args).unwrap();
        assert_eq!(value, ControlValue::Vec2(1.0, 2.0));

        // Mixed types
        let args = vec![OscType::Int(1), OscType::Float(2.0)];
        let value = osc_to_vec2(&args).unwrap();
        assert_eq!(value, ControlValue::Vec2(1.0, 2.0));
    }

    #[test]
    fn test_control_value_to_osc() {
        let value = ControlValue::Float(0.5);
        let osc = control_value_to_osc(&value);
        assert_eq!(osc, vec![OscType::Float(0.5)]);

        let value = ControlValue::Vec2(1.0, 2.0);
        let osc = control_value_to_osc(&value);
        assert_eq!(osc, vec![OscType::Float(1.0), OscType::Float(2.0)]);
    }
}
