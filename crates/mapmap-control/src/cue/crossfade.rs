//! Crossfade engine for smooth transitions between cues

use serde::{Deserialize, Serialize};
use std::time::{Duration, Instant};

/// Fade curve types for crossfades
#[derive(Debug, Clone, Copy, PartialEq, Serialize, Deserialize)]
pub enum FadeCurve {
    Linear,
    EaseIn,
    EaseOut,
    EaseInOut,
    Exponential,
}

/// Crossfade state tracker
pub struct Crossfade {
    start_time: Instant,
    duration: Duration,
    curve: FadeCurve,
    from_cue_id: u32,
    to_cue_id: u32,
}

impl Crossfade {
    /// Create a new crossfade
    pub fn new(from_cue_id: u32, to_cue_id: u32, duration: Duration, curve: FadeCurve) -> Self {
        Self {
            start_time: Instant::now(),
            duration,
            curve,
            from_cue_id,
            to_cue_id,
        }
    }

    /// Get the current progress (0.0 to 1.0)
    pub fn progress(&self) -> f32 {
        let elapsed = self.start_time.elapsed();
        if elapsed >= self.duration {
            return 1.0;
        }

        let linear_progress = elapsed.as_secs_f32() / self.duration.as_secs_f32();
        self.curve.apply(linear_progress)
    }

    /// Check if the crossfade is complete
    pub fn is_complete(&self) -> bool {
        self.start_time.elapsed() >= self.duration
    }

    /// Get the source cue ID
    pub fn from_cue_id(&self) -> u32 {
        self.from_cue_id
    }

    /// Get the destination cue ID
    pub fn to_cue_id(&self) -> u32 {
        self.to_cue_id
    }

    /// Get the fade curve
    pub fn curve(&self) -> FadeCurve {
        self.curve
    }

    /// Get the duration
    pub fn duration(&self) -> Duration {
        self.duration
    }
}

impl FadeCurve {
    /// Apply the curve to a linear progress value
    pub fn apply(self, t: f32) -> f32 {
        let t = t.clamp(0.0, 1.0);

        match self {
            FadeCurve::Linear => t,
            FadeCurve::EaseIn => t * t,
            FadeCurve::EaseOut => t * (2.0 - t),
            FadeCurve::EaseInOut => {
                if t < 0.5 {
                    2.0 * t * t
                } else {
                    -1.0 + (4.0 - 2.0 * t) * t
                }
            }
            FadeCurve::Exponential => t * t * t,
        }
    }
}

/// Interpolate between two values using a progress value (0.0 to 1.0)
pub fn interpolate_f32(from: f32, to: f32, progress: f32) -> f32 {
    from + (to - from) * progress
}

/// Interpolate between two positions
pub fn interpolate_position(
    from: (f32, f32),
    to: (f32, f32),
    progress: f32,
) -> (f32, f32) {
    (
        interpolate_f32(from.0, to.0, progress),
        interpolate_f32(from.1, to.1, progress),
    )
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_linear_curve() {
        assert_eq!(FadeCurve::Linear.apply(0.0), 0.0);
        assert_eq!(FadeCurve::Linear.apply(0.5), 0.5);
        assert_eq!(FadeCurve::Linear.apply(1.0), 1.0);
    }

    #[test]
    fn test_ease_in_curve() {
        assert_eq!(FadeCurve::EaseIn.apply(0.0), 0.0);
        assert_eq!(FadeCurve::EaseIn.apply(0.5), 0.25);
        assert_eq!(FadeCurve::EaseIn.apply(1.0), 1.0);
    }

    #[test]
    fn test_ease_out_curve() {
        assert_eq!(FadeCurve::EaseOut.apply(0.0), 0.0);
        assert_eq!(FadeCurve::EaseOut.apply(0.5), 0.75);
        assert_eq!(FadeCurve::EaseOut.apply(1.0), 1.0);
    }

    #[test]
    fn test_ease_in_out_curve() {
        assert_eq!(FadeCurve::EaseInOut.apply(0.0), 0.0);
        assert!(FadeCurve::EaseInOut.apply(0.5) > 0.45);
        assert!(FadeCurve::EaseInOut.apply(0.5) < 0.55);
        assert_eq!(FadeCurve::EaseInOut.apply(1.0), 1.0);
    }

    #[test]
    fn test_interpolate_f32() {
        assert_eq!(interpolate_f32(0.0, 10.0, 0.0), 0.0);
        assert_eq!(interpolate_f32(0.0, 10.0, 0.5), 5.0);
        assert_eq!(interpolate_f32(0.0, 10.0, 1.0), 10.0);
    }

    #[test]
    fn test_interpolate_position() {
        let from = (0.0, 0.0);
        let to = (100.0, 200.0);

        assert_eq!(interpolate_position(from, to, 0.0), (0.0, 0.0));
        assert_eq!(interpolate_position(from, to, 0.5), (50.0, 100.0));
        assert_eq!(interpolate_position(from, to, 1.0), (100.0, 200.0));
    }

    #[test]
    fn test_crossfade_creation() {
        let crossfade = Crossfade::new(0, 1, Duration::from_secs(2), FadeCurve::Linear);
        assert_eq!(crossfade.from_cue_id(), 0);
        assert_eq!(crossfade.to_cue_id(), 1);
        assert_eq!(crossfade.duration(), Duration::from_secs(2));
    }

    #[test]
    fn test_crossfade_progress() {
        let crossfade = Crossfade::new(0, 1, Duration::from_millis(100), FadeCurve::Linear);

        // Should start at 0
        let initial_progress = crossfade.progress();
        assert!(initial_progress >= 0.0 && initial_progress < 0.1);

        // Wait and check progress increases
        std::thread::sleep(Duration::from_millis(150));
        assert!(crossfade.is_complete());
        assert_eq!(crossfade.progress(), 1.0);
    }
}
