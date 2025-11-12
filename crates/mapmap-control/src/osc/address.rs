//! OSC address space parser
//!
//! Parses OSC addresses like `/mapmap/layer/0/opacity` to control targets

use crate::{ControlTarget, error::ControlError, Result};

/// Parse an OSC address to a control target
///
/// Supported address patterns:
/// - `/mapmap/layer/{id}/opacity` - Layer opacity (0.0-1.0)
/// - `/mapmap/layer/{id}/position` - Layer position (x, y)
/// - `/mapmap/layer/{id}/rotation` - Layer rotation (degrees)
/// - `/mapmap/layer/{id}/scale` - Layer scale
/// - `/mapmap/layer/{id}/visibility` - Layer visibility (bool)
/// - `/mapmap/paint/{id}/parameter/{name}` - Paint parameter
/// - `/mapmap/effect/{id}/parameter/{name}` - Effect parameter
/// - `/mapmap/playback/speed` - Playback speed
/// - `/mapmap/playback/position` - Playback position
/// - `/mapmap/output/{id}/brightness` - Output brightness
pub fn parse_osc_address(address: &str) -> Result<ControlTarget> {
    let parts: Vec<&str> = address.trim_start_matches('/').split('/').collect();

    if parts.is_empty() || parts[0] != "mapmap" {
        return Err(ControlError::InvalidMessage(format!(
            "OSC address must start with /mapmap: {}",
            address
        )));
    }

    if parts.len() < 2 {
        return Err(ControlError::InvalidMessage(format!(
            "Invalid OSC address: {}",
            address
        )));
    }

    match parts[1] {
        "layer" => parse_layer_address(&parts[2..]),
        "paint" => parse_paint_address(&parts[2..]),
        "effect" => parse_effect_address(&parts[2..]),
        "playback" => parse_playback_address(&parts[2..]),
        "output" => parse_output_address(&parts[2..]),
        _ => Err(ControlError::InvalidMessage(format!(
            "Unknown OSC category: {}",
            parts[1]
        ))),
    }
}

fn parse_layer_address(parts: &[&str]) -> Result<ControlTarget> {
    if parts.is_empty() {
        return Err(ControlError::InvalidMessage(
            "Missing layer ID".to_string(),
        ));
    }

    let layer_id: u32 = parts[0].parse().map_err(|_| {
        ControlError::InvalidMessage(format!("Invalid layer ID: {}", parts[0]))
    })?;

    if parts.len() < 2 {
        return Err(ControlError::InvalidMessage(
            "Missing layer parameter".to_string(),
        ));
    }

    match parts[1] {
        "opacity" => Ok(ControlTarget::LayerOpacity(layer_id)),
        "position" => Ok(ControlTarget::LayerPosition(layer_id)),
        "rotation" => Ok(ControlTarget::LayerRotation(layer_id)),
        "scale" => Ok(ControlTarget::LayerScale(layer_id)),
        "visibility" => Ok(ControlTarget::LayerVisibility(layer_id)),
        _ => Err(ControlError::InvalidMessage(format!(
            "Unknown layer parameter: {}",
            parts[1]
        ))),
    }
}

fn parse_paint_address(parts: &[&str]) -> Result<ControlTarget> {
    if parts.is_empty() {
        return Err(ControlError::InvalidMessage(
            "Missing paint ID".to_string(),
        ));
    }

    let paint_id: u32 = parts[0].parse().map_err(|_| {
        ControlError::InvalidMessage(format!("Invalid paint ID: {}", parts[0]))
    })?;

    if parts.len() < 3 || parts[1] != "parameter" {
        return Err(ControlError::InvalidMessage(
            "Paint address must be /paint/{id}/parameter/{name}".to_string(),
        ));
    }

    Ok(ControlTarget::PaintParameter(
        paint_id,
        parts[2].to_string(),
    ))
}

fn parse_effect_address(parts: &[&str]) -> Result<ControlTarget> {
    if parts.is_empty() {
        return Err(ControlError::InvalidMessage(
            "Missing effect ID".to_string(),
        ));
    }

    let effect_id: u32 = parts[0].parse().map_err(|_| {
        ControlError::InvalidMessage(format!("Invalid effect ID: {}", parts[0]))
    })?;

    if parts.len() < 3 || parts[1] != "parameter" {
        return Err(ControlError::InvalidMessage(
            "Effect address must be /effect/{id}/parameter/{name}".to_string(),
        ));
    }

    Ok(ControlTarget::EffectParameter(
        effect_id,
        parts[2].to_string(),
    ))
}

fn parse_playback_address(parts: &[&str]) -> Result<ControlTarget> {
    if parts.is_empty() {
        return Err(ControlError::InvalidMessage(
            "Missing playback parameter".to_string(),
        ));
    }

    match parts[0] {
        "speed" => Ok(ControlTarget::PlaybackSpeed(None)),
        "position" => Ok(ControlTarget::PlaybackPosition),
        _ => Err(ControlError::InvalidMessage(format!(
            "Unknown playback parameter: {}",
            parts[0]
        ))),
    }
}

fn parse_output_address(parts: &[&str]) -> Result<ControlTarget> {
    if parts.is_empty() {
        return Err(ControlError::InvalidMessage(
            "Missing output ID".to_string(),
        ));
    }

    let output_id: u32 = parts[0].parse().map_err(|_| {
        ControlError::InvalidMessage(format!("Invalid output ID: {}", parts[0]))
    })?;

    if parts.len() < 2 {
        return Err(ControlError::InvalidMessage(
            "Missing output parameter".to_string(),
        ));
    }

    match parts[1] {
        "brightness" => Ok(ControlTarget::OutputBrightness(output_id)),
        _ => Err(ControlError::InvalidMessage(format!(
            "Unknown output parameter: {}",
            parts[1]
        ))),
    }
}

/// Generate OSC address from control target
pub fn control_target_to_address(target: &ControlTarget) -> String {
    match target {
        ControlTarget::LayerOpacity(id) => format!("/mapmap/layer/{}/opacity", id),
        ControlTarget::LayerPosition(id) => format!("/mapmap/layer/{}/position", id),
        ControlTarget::LayerScale(id) => format!("/mapmap/layer/{}/scale", id),
        ControlTarget::LayerRotation(id) => format!("/mapmap/layer/{}/rotation", id),
        ControlTarget::LayerVisibility(id) => format!("/mapmap/layer/{}/visibility", id),
        ControlTarget::PaintParameter(id, name) => {
            format!("/mapmap/paint/{}/parameter/{}", id, name)
        }
        ControlTarget::EffectParameter(id, name) => {
            format!("/mapmap/effect/{}/parameter/{}", id, name)
        }
        ControlTarget::PlaybackSpeed(_) => "/mapmap/playback/speed".to_string(),
        ControlTarget::PlaybackPosition => "/mapmap/playback/position".to_string(),
        ControlTarget::OutputBrightness(id) => format!("/mapmap/output/{}/brightness", id),
        ControlTarget::OutputEdgeBlend(id, edge) => {
            format!("/mapmap/output/{}/edge_blend/{:?}", id, edge)
        }
        ControlTarget::Custom(name) => format!("/mapmap/custom/{}", name),
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_parse_layer_opacity() {
        let target = parse_osc_address("/mapmap/layer/0/opacity").unwrap();
        assert_eq!(target, ControlTarget::LayerOpacity(0));
    }

    #[test]
    fn test_parse_layer_position() {
        let target = parse_osc_address("/mapmap/layer/5/position").unwrap();
        assert_eq!(target, ControlTarget::LayerPosition(5));
    }

    #[test]
    fn test_parse_paint_parameter() {
        let target = parse_osc_address("/mapmap/paint/3/parameter/speed").unwrap();
        assert_eq!(
            target,
            ControlTarget::PaintParameter(3, "speed".to_string())
        );
    }

    #[test]
    fn test_parse_effect_parameter() {
        let target = parse_osc_address("/mapmap/effect/1/parameter/intensity").unwrap();
        assert_eq!(
            target,
            ControlTarget::EffectParameter(1, "intensity".to_string())
        );
    }

    #[test]
    fn test_parse_playback_speed() {
        let target = parse_osc_address("/mapmap/playback/speed").unwrap();
        assert_eq!(target, ControlTarget::PlaybackSpeed(None));
    }

    #[test]
    fn test_invalid_address() {
        assert!(parse_osc_address("/invalid/address").is_err());
        assert!(parse_osc_address("/mapmap").is_err());
        assert!(parse_osc_address("/mapmap/layer").is_err());
        assert!(parse_osc_address("/mapmap/layer/notanumber/opacity").is_err());
    }

    #[test]
    fn test_control_target_to_address() {
        let target = ControlTarget::LayerOpacity(0);
        assert_eq!(control_target_to_address(&target), "/mapmap/layer/0/opacity");

        let target = ControlTarget::PaintParameter(3, "speed".to_string());
        assert_eq!(
            control_target_to_address(&target),
            "/mapmap/paint/3/parameter/speed"
        );
    }
}
