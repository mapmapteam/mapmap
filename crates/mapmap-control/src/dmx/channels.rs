//! DMX channel assignment system

use std::collections::HashMap;
use serde::{Deserialize, Serialize};

use crate::{ControlTarget, ControlValue, error::ControlError, Result};

/// Maps control targets to DMX channels
#[derive(Debug, Clone, Default, Serialize, Deserialize)]
pub struct ChannelAssignment {
    assignments: HashMap<ControlTarget, DmxChannel>,
}

/// A DMX channel assignment
#[derive(Debug, Clone, Copy, Serialize, Deserialize)]
pub struct DmxChannel {
    pub universe: u16,
    pub channel: u16, // 1-512
    pub range: Option<(u8, u8)>, // Optional value range remapping (min, max)
}

impl ChannelAssignment {
    /// Create a new channel assignment map
    pub fn new() -> Self {
        Self::default()
    }

    /// Assign a control target to a DMX channel
    pub fn assign(&mut self, target: ControlTarget, channel: DmxChannel) {
        self.assignments.insert(target, channel);
    }

    /// Remove an assignment
    pub fn remove(&mut self, target: &ControlTarget) -> Option<DmxChannel> {
        self.assignments.remove(target)
    }

    /// Get the DMX channel for a control target
    pub fn get(&self, target: &ControlTarget) -> Option<&DmxChannel> {
        self.assignments.get(target)
    }

    /// Apply a control value to DMX data
    pub fn apply_value(
        &self,
        target: &ControlTarget,
        value: &ControlValue,
        dmx_data: &mut HashMap<u16, [u8; 512]>,
    ) -> Result<()> {
        if let Some(channel) = self.get(target) {
            let dmx_value = self.control_value_to_dmx(value, channel)?;

            // Get or create universe
            let universe_data = dmx_data.entry(channel.universe).or_insert([0u8; 512]);

            // Set channel value (DMX channels are 1-indexed, array is 0-indexed)
            let index = (channel.channel as usize).saturating_sub(1);
            if index < 512 {
                universe_data[index] = dmx_value;
            }
        }

        Ok(())
    }

    /// Convert a control value to a DMX value (0-255)
    fn control_value_to_dmx(&self, value: &ControlValue, channel: &DmxChannel) -> Result<u8> {
        let float_value = value.as_float().ok_or_else(|| {
            ControlError::InvalidParameter(format!(
                "Cannot convert {:?} to DMX value",
                value
            ))
        })?;

        // Clamp to 0.0-1.0
        let clamped = float_value.clamp(0.0, 1.0);

        // Apply range remapping if specified
        let dmx_value = if let Some((min, max)) = channel.range {
            let range = (max - min) as f32;
            min + (clamped * range) as u8
        } else {
            (clamped * 255.0) as u8
        };

        Ok(dmx_value)
    }

    /// Get all assignments for a specific universe
    pub fn assignments_for_universe(&self, universe: u16) -> Vec<(&ControlTarget, &DmxChannel)> {
        self.assignments
            .iter()
            .filter(|(_, channel)| channel.universe == universe)
            .collect()
    }

    /// Get all universes that have assignments
    pub fn used_universes(&self) -> Vec<u16> {
        let mut universes: Vec<u16> = self
            .assignments
            .values()
            .map(|channel| channel.universe)
            .collect();
        universes.sort_unstable();
        universes.dedup();
        universes
    }

    /// Clear all assignments
    pub fn clear(&mut self) {
        self.assignments.clear();
    }

    /// Get the number of assignments
    pub fn len(&self) -> usize {
        self.assignments.len()
    }

    /// Check if there are no assignments
    pub fn is_empty(&self) -> bool {
        self.assignments.is_empty()
    }
}

impl DmxChannel {
    /// Create a new DMX channel assignment
    pub fn new(universe: u16, channel: u16) -> Self {
        Self {
            universe,
            channel,
            range: None,
        }
    }

    /// Create a DMX channel with a value range
    pub fn with_range(universe: u16, channel: u16, min: u8, max: u8) -> Self {
        Self {
            universe,
            channel,
            range: Some((min, max)),
        }
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_channel_assignment() {
        let mut assignment = ChannelAssignment::new();

        let target = ControlTarget::LayerOpacity(0);
        let channel = DmxChannel::new(0, 1);

        assignment.assign(target.clone(), channel);

        assert!(assignment.get(&target).is_some());
        assert_eq!(assignment.len(), 1);
    }

    #[test]
    fn test_control_value_to_dmx() {
        let assignment = ChannelAssignment::new();
        let channel = DmxChannel::new(0, 1);

        // Test 0.0 -> 0
        let value = ControlValue::Float(0.0);
        let dmx = assignment.control_value_to_dmx(&value, &channel).unwrap();
        assert_eq!(dmx, 0);

        // Test 1.0 -> 255
        let value = ControlValue::Float(1.0);
        let dmx = assignment.control_value_to_dmx(&value, &channel).unwrap();
        assert_eq!(dmx, 255);

        // Test 0.5 -> 127
        let value = ControlValue::Float(0.5);
        let dmx = assignment.control_value_to_dmx(&value, &channel).unwrap();
        assert_eq!(dmx, 127);
    }

    #[test]
    fn test_range_remapping() {
        let assignment = ChannelAssignment::new();
        let channel = DmxChannel::with_range(0, 1, 100, 200);

        // Test 0.0 -> 100
        let value = ControlValue::Float(0.0);
        let dmx = assignment.control_value_to_dmx(&value, &channel).unwrap();
        assert_eq!(dmx, 100);

        // Test 1.0 -> 200
        let value = ControlValue::Float(1.0);
        let dmx = assignment.control_value_to_dmx(&value, &channel).unwrap();
        assert_eq!(dmx, 200);

        // Test 0.5 -> 150
        let value = ControlValue::Float(0.5);
        let dmx = assignment.control_value_to_dmx(&value, &channel).unwrap();
        assert_eq!(dmx, 150);
    }

    #[test]
    fn test_apply_value() {
        let mut assignment = ChannelAssignment::new();
        let target = ControlTarget::LayerOpacity(0);
        let channel = DmxChannel::new(0, 1);

        assignment.assign(target.clone(), channel);

        let mut dmx_data = HashMap::new();
        let value = ControlValue::Float(0.5);

        assignment
            .apply_value(&target, &value, &mut dmx_data)
            .unwrap();

        assert_eq!(dmx_data[&0][0], 127);
    }

    #[test]
    fn test_used_universes() {
        let mut assignment = ChannelAssignment::new();

        assignment.assign(ControlTarget::LayerOpacity(0), DmxChannel::new(0, 1));
        assignment.assign(ControlTarget::LayerOpacity(1), DmxChannel::new(0, 2));
        assignment.assign(ControlTarget::LayerOpacity(2), DmxChannel::new(1, 1));

        let universes = assignment.used_universes();
        assert_eq!(universes, vec![0, 1]);
    }
}
