//! Parameter Animation System - Keyframe-based Timeline
//!
//! Phase 3: Effects Pipeline
//! Provides keyframe animation for all animatable properties

use serde::{Deserialize, Serialize};
use std::collections::BTreeMap;

/// Time in seconds
pub type TimePoint = f64;

/// Interpolation mode for keyframes
#[derive(Debug, Clone, Copy, PartialEq, Eq, Serialize, Deserialize)]
pub enum InterpolationMode {
    /// No interpolation, step to next value
    Constant,
    /// Linear interpolation
    Linear,
    /// Smooth interpolation (ease in/out)
    Smooth,
    /// Bezier curve (requires control points)
    Bezier,
}

/// Animatable value types
#[derive(Debug, Clone, Serialize, Deserialize)]
pub enum AnimValue {
    Float(f32),
    Vec2([f32; 2]),
    Vec3([f32; 3]),
    Vec4([f32; 4]),
    Color([f32; 4]),
    Bool(bool),
}

impl AnimValue {
    /// Interpolate between two values
    pub fn lerp(&self, other: &AnimValue, t: f32) -> AnimValue {
        match (self, other) {
            (AnimValue::Float(a), AnimValue::Float(b)) => {
                AnimValue::Float(a + (b - a) * t)
            }
            (AnimValue::Vec2(a), AnimValue::Vec2(b)) => {
                AnimValue::Vec2([
                    a[0] + (b[0] - a[0]) * t,
                    a[1] + (b[1] - a[1]) * t,
                ])
            }
            (AnimValue::Vec3(a), AnimValue::Vec3(b)) => {
                AnimValue::Vec3([
                    a[0] + (b[0] - a[0]) * t,
                    a[1] + (b[1] - a[1]) * t,
                    a[2] + (b[2] - a[2]) * t,
                ])
            }
            (AnimValue::Vec4(a), AnimValue::Vec4(b)) => {
                AnimValue::Vec4([
                    a[0] + (b[0] - a[0]) * t,
                    a[1] + (b[1] - a[1]) * t,
                    a[2] + (b[2] - a[2]) * t,
                    a[3] + (b[3] - a[3]) * t,
                ])
            }
            (AnimValue::Color(a), AnimValue::Color(b)) => {
                AnimValue::Color([
                    a[0] + (b[0] - a[0]) * t,
                    a[1] + (b[1] - a[1]) * t,
                    a[2] + (b[2] - a[2]) * t,
                    a[3] + (b[3] - a[3]) * t,
                ])
            }
            (AnimValue::Bool(a), AnimValue::Bool(_)) => {
                // Step interpolation for booleans
                AnimValue::Bool(*a)
            }
            _ => self.clone(), // Type mismatch, return original
        }
    }

    /// Smooth interpolation (ease in/out)
    pub fn smooth_lerp(&self, other: &AnimValue, t: f32) -> AnimValue {
        // Smoothstep: 3t² - 2t³
        let smooth_t = t * t * (3.0 - 2.0 * t);
        self.lerp(other, smooth_t)
    }
}

/// Keyframe - a value at a specific time
#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct Keyframe {
    pub time: TimePoint,
    pub value: AnimValue,
    pub interpolation: InterpolationMode,
    // For Bezier interpolation
    pub in_tangent: Option<[f32; 2]>,  // (time_offset, value_offset)
    pub out_tangent: Option<[f32; 2]>,
}

impl Keyframe {
    /// Create a new keyframe
    pub fn new(time: TimePoint, value: AnimValue) -> Self {
        Self {
            time,
            value,
            interpolation: InterpolationMode::Linear,
            in_tangent: None,
            out_tangent: None,
        }
    }

    /// Create a keyframe with smooth interpolation
    pub fn smooth(time: TimePoint, value: AnimValue) -> Self {
        Self {
            time,
            value,
            interpolation: InterpolationMode::Smooth,
            in_tangent: None,
            out_tangent: None,
        }
    }

    /// Create a keyframe with constant (step) interpolation
    pub fn constant(time: TimePoint, value: AnimValue) -> Self {
        Self {
            time,
            value,
            interpolation: InterpolationMode::Constant,
            in_tangent: None,
            out_tangent: None,
        }
    }
}

/// Animation track - series of keyframes for a single property
#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct AnimationTrack {
    pub name: String,
    pub keyframes: BTreeMap<u64, Keyframe>, // Key is time in microseconds for ordering
    pub default_value: AnimValue,
    pub enabled: bool,
}

impl AnimationTrack {
    /// Create a new animation track
    pub fn new(name: String, default_value: AnimValue) -> Self {
        Self {
            name,
            keyframes: BTreeMap::new(),
            default_value,
            enabled: true,
        }
    }

    /// Add a keyframe
    pub fn add_keyframe(&mut self, keyframe: Keyframe) {
        let key = (keyframe.time * 1_000_000.0) as u64; // Convert to microseconds
        self.keyframes.insert(key, keyframe);
    }

    /// Remove a keyframe at the specified time
    pub fn remove_keyframe(&mut self, time: TimePoint) -> Option<Keyframe> {
        let key = (time * 1_000_000.0) as u64;
        self.keyframes.remove(&key)
    }

    /// Evaluate the track at a given time
    pub fn evaluate(&self, time: TimePoint) -> AnimValue {
        if !self.enabled {
            return self.default_value.clone();
        }

        if self.keyframes.is_empty() {
            return self.default_value.clone();
        }

        let time_key = (time * 1_000_000.0) as u64;

        // Find keyframes before and after current time
        let mut before = None;
        let mut after = None;

        for (&key, keyframe) in &self.keyframes {
            if key <= time_key {
                before = Some(keyframe);
            }
            if key >= time_key && after.is_none() {
                after = Some(keyframe);
            }
        }

        match (before, after) {
            (None, None) => self.default_value.clone(),
            (Some(kf), None) => kf.value.clone(),
            (None, Some(kf)) => kf.value.clone(),
            (Some(kf1), Some(kf2)) if kf1.time == kf2.time => kf1.value.clone(),
            (Some(kf1), Some(kf2)) => {
                // Interpolate between keyframes
                let t = ((time - kf1.time) / (kf2.time - kf1.time)) as f32;
                let t = t.clamp(0.0, 1.0);

                match kf1.interpolation {
                    InterpolationMode::Constant => kf1.value.clone(),
                    InterpolationMode::Linear => kf1.value.lerp(&kf2.value, t),
                    InterpolationMode::Smooth => kf1.value.smooth_lerp(&kf2.value, t),
                    InterpolationMode::Bezier => {
                        // TODO: Implement Bezier interpolation
                        kf1.value.lerp(&kf2.value, t)
                    }
                }
            }
        }
    }

    /// Get all keyframes in time order
    pub fn keyframes_ordered(&self) -> Vec<&Keyframe> {
        self.keyframes.values().collect()
    }

    /// Get the time range of this track
    pub fn time_range(&self) -> Option<(TimePoint, TimePoint)> {
        if self.keyframes.is_empty() {
            return None;
        }

        let first = self.keyframes.values().next()?.time;
        let last = self.keyframes.values().last()?.time;

        Some((first, last))
    }
}

/// Animation clip - collection of tracks
#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct AnimationClip {
    pub name: String,
    pub tracks: Vec<AnimationTrack>,
    pub duration: TimePoint,
    pub looping: bool,
}

impl AnimationClip {
    /// Create a new animation clip
    pub fn new(name: String) -> Self {
        Self {
            name,
            tracks: Vec::new(),
            duration: 10.0, // Default 10 seconds
            looping: false,
        }
    }

    /// Add a track
    pub fn add_track(&mut self, track: AnimationTrack) {
        self.tracks.push(track);
    }

    /// Get a track by name
    pub fn get_track(&self, name: &str) -> Option<&AnimationTrack> {
        self.tracks.iter().find(|t| t.name == name)
    }

    /// Get a mutable track by name
    pub fn get_track_mut(&mut self, name: &str) -> Option<&mut AnimationTrack> {
        self.tracks.iter_mut().find(|t| t.name == name)
    }

    /// Evaluate all tracks at a given time
    pub fn evaluate(&self, time: TimePoint) -> Vec<(String, AnimValue)> {
        let wrapped_time = if self.looping && self.duration > 0.0 {
            time % self.duration
        } else {
            time.min(self.duration)
        };

        self.tracks
            .iter()
            .map(|track| (track.name.clone(), track.evaluate(wrapped_time)))
            .collect()
    }

    /// Calculate total duration from all tracks
    pub fn calculate_duration(&mut self) {
        let max_time = self.tracks
            .iter()
            .filter_map(|track| track.time_range())
            .map(|(_, end)| end)
            .fold(0.0, f64::max);

        if max_time > 0.0 {
            self.duration = max_time;
        }
    }
}

/// Animation player state
#[derive(Debug, Clone)]
pub struct AnimationPlayer {
    pub clip: AnimationClip,
    pub current_time: TimePoint,
    pub playing: bool,
    pub speed: f32,
}

impl AnimationPlayer {
    /// Create a new animation player
    pub fn new(clip: AnimationClip) -> Self {
        Self {
            clip,
            current_time: 0.0,
            playing: false,
            speed: 1.0,
        }
    }

    /// Play the animation
    pub fn play(&mut self) {
        self.playing = true;
    }

    /// Pause the animation
    pub fn pause(&mut self) {
        self.playing = false;
    }

    /// Stop and reset the animation
    pub fn stop(&mut self) {
        self.playing = false;
        self.current_time = 0.0;
    }

    /// Update the player (call every frame)
    pub fn update(&mut self, delta_time: f64) -> Vec<(String, AnimValue)> {
        if self.playing {
            self.current_time += delta_time * self.speed as f64;

            if self.clip.looping {
                if self.current_time >= self.clip.duration {
                    self.current_time = self.current_time % self.clip.duration;
                }
            } else {
                if self.current_time >= self.clip.duration {
                    self.current_time = self.clip.duration;
                    self.playing = false;
                }
            }
        }

        self.clip.evaluate(self.current_time)
    }

    /// Seek to a specific time
    pub fn seek(&mut self, time: TimePoint) {
        self.current_time = time.clamp(0.0, self.clip.duration);
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_keyframe_animation() {
        let mut track = AnimationTrack::new(
            "opacity".to_string(),
            AnimValue::Float(1.0),
        );

        track.add_keyframe(Keyframe::new(0.0, AnimValue::Float(0.0)));
        track.add_keyframe(Keyframe::new(1.0, AnimValue::Float(1.0)));
        track.add_keyframe(Keyframe::new(2.0, AnimValue::Float(0.5)));

        // Test evaluation
        let val = track.evaluate(0.5);
        if let AnimValue::Float(v) = val {
            assert!((v - 0.5).abs() < 0.01);
        } else {
            panic!("Expected Float value");
        }
    }

    #[test]
    fn test_animation_clip() {
        let mut clip = AnimationClip::new("test".to_string());

        let mut track = AnimationTrack::new("x".to_string(), AnimValue::Float(0.0));
        track.add_keyframe(Keyframe::new(0.0, AnimValue::Float(0.0)));
        track.add_keyframe(Keyframe::new(2.0, AnimValue::Float(10.0)));

        clip.add_track(track);
        clip.calculate_duration();

        assert_eq!(clip.duration, 2.0);

        let values = clip.evaluate(1.0);
        assert_eq!(values.len(), 1);
        assert_eq!(values[0].0, "x");
    }

    #[test]
    fn test_smooth_interpolation() {
        let a = AnimValue::Float(0.0);
        let b = AnimValue::Float(1.0);

        let mid = a.smooth_lerp(&b, 0.5);
        if let AnimValue::Float(v) = mid {
            // Smoothstep at 0.5 should be 0.5
            assert!((v - 0.5).abs() < 0.01);
        }
    }
}
