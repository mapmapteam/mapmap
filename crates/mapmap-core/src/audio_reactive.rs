//! Audio-Reactive Shader Parameter Integration
//!
//! Phase 3: Effects Pipeline
//! Connects audio analysis to shader graph parameters for audio-reactive effects

use crate::audio::{AudioAnalysis, AudioReactiveMapping, AudioMappingType, FrequencyBand};
use crate::shader_graph::{ShaderGraph, NodeId, ParameterValue};
use crate::animation::{AnimationPlayer, AnimationClip, AnimValue};
use serde::{Deserialize, Serialize};
use std::collections::HashMap;

/// Audio-reactive parameter controller
#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct AudioReactiveController {
    /// Parameter mappings (parameter_path -> mapping)
    pub mappings: HashMap<String, AudioReactiveMapping>,

    /// Previous values for smooth transitions
    previous_values: HashMap<String, f32>,

    /// Last update time
    last_update_time: f64,

    /// Enable/disable audio reactivity
    pub enabled: bool,
}

impl Default for AudioReactiveController {
    fn default() -> Self {
        Self {
            mappings: HashMap::new(),
            previous_values: HashMap::new(),
            last_update_time: 0.0,
            enabled: true,
        }
    }
}

impl AudioReactiveController {
    /// Create a new audio-reactive controller
    pub fn new() -> Self {
        Self::default()
    }

    /// Add a parameter mapping
    pub fn add_mapping(&mut self, parameter_path: String, mapping: AudioReactiveMapping) {
        self.mappings.insert(parameter_path, mapping);
    }

    /// Remove a parameter mapping
    pub fn remove_mapping(&mut self, parameter_path: &str) -> Option<AudioReactiveMapping> {
        self.previous_values.remove(parameter_path);
        self.mappings.remove(parameter_path)
    }

    /// Update all parameters based on audio analysis
    pub fn update(&mut self, audio: &AudioAnalysis, current_time: f64) -> HashMap<String, f32> {
        if !self.enabled {
            return HashMap::new();
        }

        let delta_time = (current_time - self.last_update_time) as f32;
        self.last_update_time = current_time;

        let mut updated_values = HashMap::new();

        for (param_path, mapping) in &self.mappings {
            let previous = self.previous_values.get(param_path).copied().unwrap_or(0.0);
            let new_value = mapping.apply(audio, previous, delta_time);

            self.previous_values.insert(param_path.clone(), new_value);
            updated_values.insert(param_path.clone(), new_value);
        }

        updated_values
    }

    /// Apply audio-reactive values to shader graph parameters
    pub fn apply_to_shader_graph(&self, graph: &mut ShaderGraph, values: &HashMap<String, f32>) {
        for (param_path, &value) in values {
            // Parse parameter path (format: "node_id.parameter_name")
            if let Some((node_id_str, param_name)) = param_path.split_once('.') {
                if let Ok(node_id) = node_id_str.parse::<NodeId>() {
                    if let Some(node) = graph.nodes.get_mut(&node_id) {
                        // Update parameter value
                        if let Some(param_value) = node.parameters.get_mut(param_name) {
                            match param_value {
                                ParameterValue::Float(v) => *v = value,
                                ParameterValue::Vec2(v) => v[0] = value,
                                ParameterValue::Vec3(v) => v[0] = value,
                                ParameterValue::Vec4(v) => v[0] = value,
                                ParameterValue::Color(c) => c[0] = value,
                            }
                        }
                    }
                }
            }
        }
    }

    /// Create preset mappings for common audio-reactive effects
    pub fn create_preset_mappings(&mut self, preset: AudioReactivePreset, node_id: NodeId) {
        match preset {
            AudioReactivePreset::BassScale => {
                // Scale based on bass frequency
                let mapping = AudioReactiveMapping {
                    parameter_name: "scale".to_string(),
                    source: crate::audio::AudioSource::SystemInput,
                    mapping_type: AudioMappingType::BandEnergy,
                    frequency_band: Some(FrequencyBand::Bass),
                    output_min: 0.8,
                    output_max: 1.2,
                    smoothing: 0.9,
                    attack: 0.05,
                    release: 0.2,
                };
                self.add_mapping(format!("{}.scale", node_id), mapping);
            }

            AudioReactivePreset::BeatPulse => {
                // Pulse opacity on beats
                let mapping = AudioReactiveMapping {
                    parameter_name: "opacity".to_string(),
                    source: crate::audio::AudioSource::SystemInput,
                    mapping_type: AudioMappingType::BeatStrength,
                    frequency_band: None,
                    output_min: 0.5,
                    output_max: 1.0,
                    smoothing: 0.7,
                    attack: 0.01,
                    release: 0.3,
                };
                self.add_mapping(format!("{}.opacity", node_id), mapping);
            }

            AudioReactivePreset::FrequencyColor => {
                // Map frequency bands to color channels
                let bass_mapping = AudioReactiveMapping {
                    parameter_name: "color_r".to_string(),
                    source: crate::audio::AudioSource::SystemInput,
                    mapping_type: AudioMappingType::BandEnergy,
                    frequency_band: Some(FrequencyBand::Bass),
                    output_min: 0.0,
                    output_max: 1.0,
                    smoothing: 0.8,
                    attack: 0.1,
                    release: 0.2,
                };
                self.add_mapping(format!("{}.color_r", node_id), bass_mapping);

                let mid_mapping = AudioReactiveMapping {
                    parameter_name: "color_g".to_string(),
                    source: crate::audio::AudioSource::SystemInput,
                    mapping_type: AudioMappingType::BandEnergy,
                    frequency_band: Some(FrequencyBand::Mid),
                    output_min: 0.0,
                    output_max: 1.0,
                    smoothing: 0.8,
                    attack: 0.1,
                    release: 0.2,
                };
                self.add_mapping(format!("{}.color_g", node_id), mid_mapping);

                let treble_mapping = AudioReactiveMapping {
                    parameter_name: "color_b".to_string(),
                    source: crate::audio::AudioSource::SystemInput,
                    mapping_type: AudioMappingType::BandEnergy,
                    frequency_band: Some(FrequencyBand::Brilliance),
                    output_min: 0.0,
                    output_max: 1.0,
                    smoothing: 0.8,
                    attack: 0.1,
                    release: 0.2,
                };
                self.add_mapping(format!("{}.color_b", node_id), treble_mapping);
            }

            AudioReactivePreset::VolumeBlur => {
                // Blur amount based on volume
                let mapping = AudioReactiveMapping {
                    parameter_name: "blur_radius".to_string(),
                    source: crate::audio::AudioSource::SystemInput,
                    mapping_type: AudioMappingType::Volume,
                    frequency_band: None,
                    output_min: 0.0,
                    output_max: 5.0,
                    smoothing: 0.85,
                    attack: 0.05,
                    release: 0.15,
                };
                self.add_mapping(format!("{}.blur_radius", node_id), mapping);
            }

            AudioReactivePreset::TempoRotation => {
                // Rotation speed based on tempo
                let mapping = AudioReactiveMapping {
                    parameter_name: "rotation_speed".to_string(),
                    source: crate::audio::AudioSource::SystemInput,
                    mapping_type: AudioMappingType::Tempo,
                    frequency_band: None,
                    output_min: 0.0,
                    output_max: 2.0,
                    smoothing: 0.95,
                    attack: 0.5,
                    release: 1.0,
                };
                self.add_mapping(format!("{}.rotation_speed", node_id), mapping);
            }
        }
    }

    /// Get all active mappings
    pub fn get_mappings(&self) -> &HashMap<String, AudioReactiveMapping> {
        &self.mappings
    }

    /// Clear all mappings
    pub fn clear_mappings(&mut self) {
        self.mappings.clear();
        self.previous_values.clear();
    }
}

/// Audio-reactive preset types
#[derive(Debug, Clone, Copy, PartialEq, Eq, Serialize, Deserialize)]
pub enum AudioReactivePreset {
    /// Scale based on bass frequency
    BassScale,
    /// Pulse opacity on beats
    BeatPulse,
    /// Map frequency bands to RGB color
    FrequencyColor,
    /// Blur amount based on volume
    VolumeBlur,
    /// Rotation speed based on tempo
    TempoRotation,
}

impl AudioReactivePreset {
    pub fn name(&self) -> &'static str {
        match self {
            AudioReactivePreset::BassScale => "Bass Scale",
            AudioReactivePreset::BeatPulse => "Beat Pulse",
            AudioReactivePreset::FrequencyColor => "Frequency Color",
            AudioReactivePreset::VolumeBlur => "Volume Blur",
            AudioReactivePreset::TempoRotation => "Tempo Rotation",
        }
    }

    pub fn description(&self) -> &'static str {
        match self {
            AudioReactivePreset::BassScale => "Scale layer based on bass frequencies",
            AudioReactivePreset::BeatPulse => "Pulse opacity in sync with beats",
            AudioReactivePreset::FrequencyColor => "Map frequency bands to RGB color channels",
            AudioReactivePreset::VolumeBlur => "Control blur amount with volume",
            AudioReactivePreset::TempoRotation => "Sync rotation speed with tempo (BPM)",
        }
    }

    pub fn all() -> Vec<AudioReactivePreset> {
        vec![
            AudioReactivePreset::BassScale,
            AudioReactivePreset::BeatPulse,
            AudioReactivePreset::FrequencyColor,
            AudioReactivePreset::VolumeBlur,
            AudioReactivePreset::TempoRotation,
        ]
    }
}

/// Audio-reactive animation system
/// Combines keyframe animation with audio reactivity
pub struct AudioReactiveAnimationSystem {
    /// Base animation player
    pub animation_player: AnimationPlayer,

    /// Audio-reactive controller
    pub audio_controller: AudioReactiveController,

    /// Blend mode between animation and audio reactivity
    pub blend_mode: AudioAnimationBlendMode,

    /// Blend factor (0.0 = animation only, 1.0 = audio only)
    pub blend_factor: f32,
}

impl Default for AudioReactiveAnimationSystem {
    fn default() -> Self {
        let empty_clip = AnimationClip::new("empty".to_string());
        Self {
            animation_player: AnimationPlayer::new(empty_clip),
            audio_controller: AudioReactiveController::new(),
            blend_mode: AudioAnimationBlendMode::Add,
            blend_factor: 1.0,
        }
    }
}

impl AudioReactiveAnimationSystem {
    pub fn new() -> Self {
        Self::default()
    }

    /// Update both animation and audio reactivity
    pub fn update(
        &mut self,
        audio: &AudioAnalysis,
        current_time: f64,
        graph: &mut ShaderGraph,
    ) {
        // Get animated values
        self.animation_player.seek(current_time);
        let animated_values_raw = self.animation_player.clip.evaluate(current_time);

        // Convert AnimValue to f32 for blending
        let animated_values: HashMap<String, f32> = animated_values_raw
            .into_iter()
            .map(|(name, value)| (name, Self::anim_value_to_f32(&value)))
            .collect();

        // Get audio-reactive values
        let audio_values = self.audio_controller.update(audio, current_time);

        // Blend values based on blend mode
        let final_values = self.blend_values(&animated_values, &audio_values);

        // Apply to shader graph
        for (param_path, value) in final_values {
            if let Some((node_id_str, param_name)) = param_path.split_once('.') {
                if let Ok(node_id) = node_id_str.parse::<NodeId>() {
                    if let Some(node) = graph.nodes.get_mut(&node_id) {
                        if let Some(param_value) = node.parameters.get_mut(param_name) {
                            match param_value {
                                ParameterValue::Float(v) => *v = value,
                                ParameterValue::Vec2(v) => v[0] = value,
                                ParameterValue::Vec3(v) => v[0] = value,
                                ParameterValue::Vec4(v) => v[0] = value,
                                ParameterValue::Color(c) => c[0] = value,
                            }
                        }
                    }
                }
            }
        }
    }

    /// Convert AnimValue to f32 (takes first component for vectors)
    fn anim_value_to_f32(value: &AnimValue) -> f32 {
        match value {
            AnimValue::Float(f) => *f,
            AnimValue::Vec2(v) => v[0],
            AnimValue::Vec3(v) => v[0],
            AnimValue::Vec4(v) => v[0],
            AnimValue::Color(c) => c[0],
            AnimValue::Bool(b) => if *b { 1.0 } else { 0.0 },
        }
    }

    /// Blend animated and audio-reactive values
    fn blend_values(
        &self,
        animated: &HashMap<String, f32>,
        audio: &HashMap<String, f32>,
    ) -> HashMap<String, f32> {
        let mut result = HashMap::new();

        // Collect all parameter paths
        let mut all_params: std::collections::HashSet<String> = animated.keys().cloned().collect();
        all_params.extend(audio.keys().cloned());

        for param_path in all_params {
            let anim_value = animated.get(&param_path).copied().unwrap_or(0.0);
            let audio_value = audio.get(&param_path).copied().unwrap_or(0.0);

            let blended = match self.blend_mode {
                AudioAnimationBlendMode::Replace => {
                    anim_value * (1.0 - self.blend_factor) + audio_value * self.blend_factor
                }
                AudioAnimationBlendMode::Add => {
                    anim_value + audio_value * self.blend_factor
                }
                AudioAnimationBlendMode::Multiply => {
                    anim_value * (1.0 + (audio_value - 1.0) * self.blend_factor)
                }
            };

            result.insert(param_path, blended);
        }

        result
    }
}

/// Blend mode for combining animation and audio reactivity
#[derive(Debug, Clone, Copy, PartialEq, Eq, Serialize, Deserialize)]
pub enum AudioAnimationBlendMode {
    /// Replace animated value with audio value (crossfade)
    Replace,
    /// Add audio value to animated value
    Add,
    /// Multiply animated value by audio value
    Multiply,
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_audio_reactive_controller() {
        let mut controller = AudioReactiveController::new();

        let mapping = AudioReactiveMapping {
            parameter_name: "opacity".to_string(),
            source: crate::audio::AudioSource::SystemInput,
            mapping_type: AudioMappingType::Volume,
            frequency_band: None,
            output_min: 0.0,
            output_max: 1.0,
            smoothing: 0.5,
            attack: 0.1,
            release: 0.3,
        };

        controller.add_mapping("1.opacity".to_string(), mapping);

        let mut audio = AudioAnalysis::default();
        audio.rms_volume = 0.8;

        let values = controller.update(&audio, 0.0);
        assert!(values.contains_key("1.opacity"));
    }

    #[test]
    fn test_preset_mappings() {
        let mut controller = AudioReactiveController::new();
        controller.create_preset_mappings(AudioReactivePreset::BassScale, 1);

        assert!(!controller.mappings.is_empty());
    }

    #[test]
    fn test_blend_modes() {
        let system = AudioReactiveAnimationSystem::new();

        let mut animated = HashMap::new();
        animated.insert("1.opacity".to_string(), 0.5);

        let mut audio = HashMap::new();
        audio.insert("1.opacity".to_string(), 0.3);

        let blended = system.blend_values(&animated, &audio);
        assert!(blended.contains_key("1.opacity"));
    }
}
