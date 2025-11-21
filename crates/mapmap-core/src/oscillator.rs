// ! Oscillator Distortion Effect System
//!
//! Provides Kuramoto-based coupled oscillator simulation for creating
//! dynamic distortion effects in projection mapping.

use serde::{Deserialize, Serialize};

/// Simulation resolution presets
#[derive(Debug, Clone, Copy, PartialEq, Eq, Serialize, Deserialize)]
pub enum SimulationResolution {
    Low,      // 128x128
    Medium,   // 256x256
    High,     // 512x512
}

impl SimulationResolution {
    pub fn dimensions(&self) -> (u32, u32) {
        match self {
            SimulationResolution::Low => (128, 128),
            SimulationResolution::Medium => (256, 256),
            SimulationResolution::High => (512, 512),
        }
    }
}

/// Phase initialization modes
#[derive(Debug, Clone, Copy, PartialEq, Eq, Serialize, Deserialize)]
pub enum PhaseInitMode {
    Random,
    Uniform,
    PlaneHorizontal,
    PlaneVertical,
    PlaneDiagonal,
}

/// Color overlay modes
#[derive(Debug, Clone, Copy, PartialEq, Eq, Serialize, Deserialize)]
pub enum ColorMode {
    Off,
    Rainbow,
    BlackWhite,
    Complementary,
}

impl ColorMode {
    pub fn to_u32(&self) -> u32 {
        match self {
            ColorMode::Off => 0,
            ColorMode::Rainbow => 1,
            ColorMode::BlackWhite => 2,
            ColorMode::Complementary => 3,
        }
    }
}

/// Coordinate system modes
#[derive(Debug, Clone, Copy, PartialEq, Eq, Serialize, Deserialize)]
pub enum CoordinateMode {
    Cartesian,
    LogPolar,
}

/// Ring parameters for coupling kernel
#[derive(Debug, Clone, Copy, Serialize, Deserialize)]
pub struct RingParams {
    /// Distance from center (0-1)
    pub distance: f32,
    /// Ring width (0-1)
    pub width: f32,
    /// Coupling strength (negative=anti-sync, positive=sync)
    pub coupling: f32,
}

impl Default for RingParams {
    fn default() -> Self {
        Self {
            distance: 0.5,
            width: 0.2,
            coupling: 1.0,
        }
    }
}

/// Oscillator effect configuration
#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct OscillatorConfig {
    // Simulation parameters
    pub simulation_resolution: SimulationResolution,
    pub kernel_radius: f32,
    pub rings: [RingParams; 4],
    pub frequency_min: f32,  // Hz
    pub frequency_max: f32,  // Hz
    pub noise_amount: f32,
    pub coordinate_mode: CoordinateMode,
    pub phase_init_mode: PhaseInitMode,

    // Distortion parameters
    pub distortion_amount: f32,
    pub distortion_scale: f32,
    pub distortion_speed: f32,

    // Visual parameters
    pub overlay_opacity: f32,
    pub color_mode: ColorMode,

    // Runtime state
    pub enabled: bool,
}

impl Default for OscillatorConfig {
    fn default() -> Self {
        Self {
            // Simulation defaults
            simulation_resolution: SimulationResolution::Medium,
            kernel_radius: 16.0,
            rings: [
                RingParams { distance: 0.2, width: 0.1, coupling: 1.0 },
                RingParams { distance: 0.5, width: 0.15, coupling: -0.5 },
                RingParams { distance: 0.8, width: 0.2, coupling: 0.3 },
                RingParams { distance: 0.0, width: 0.0, coupling: 0.0 },
            ],
            frequency_min: 0.5,
            frequency_max: 2.0,
            noise_amount: 0.1,
            coordinate_mode: CoordinateMode::Cartesian,
            phase_init_mode: PhaseInitMode::Random,

            // Distortion defaults
            distortion_amount: 0.5,
            distortion_scale: 0.02,
            distortion_speed: 1.0,

            // Visual defaults
            overlay_opacity: 0.0,
            color_mode: ColorMode::Off,

            // Runtime
            enabled: true,
        }
    }
}

impl OscillatorConfig {
    /// Create a new oscillator configuration with default values
    pub fn new() -> Self {
        Self::default()
    }

    /// Create a preset for subtle organic wobble
    pub fn preset_subtle() -> Self {
        Self {
            distortion_amount: 0.3,
            distortion_scale: 0.01,
            distortion_speed: 0.5,
            frequency_min: 0.3,
            frequency_max: 0.8,
            ..Default::default()
        }
    }

    /// Create a preset for dramatic swirling distortion
    pub fn preset_dramatic() -> Self {
        Self {
            distortion_amount: 0.8,
            distortion_scale: 0.05,
            distortion_speed: 2.0,
            frequency_min: 1.0,
            frequency_max: 4.0,
            rings: [
                RingParams { distance: 0.25, width: 0.15, coupling: 2.0 },
                RingParams { distance: 0.5, width: 0.2, coupling: -1.5 },
                RingParams { distance: 0.75, width: 0.25, coupling: 1.0 },
                RingParams { distance: 0.0, width: 0.0, coupling: 0.0 },
            ],
            ..Default::default()
        }
    }

    /// Create a preset for ring/wave patterns
    pub fn preset_rings() -> Self {
        Self {
            distortion_amount: 0.6,
            distortion_scale: 0.03,
            distortion_speed: 1.5,
            frequency_min: 0.5,
            frequency_max: 2.0,
            rings: [
                RingParams { distance: 0.3, width: 0.1, coupling: 3.0 },
                RingParams { distance: 0.5, width: 0.1, coupling: -2.0 },
                RingParams { distance: 0.7, width: 0.1, coupling: 1.5 },
                RingParams { distance: 0.0, width: 0.0, coupling: 0.0 },
            ],
            ..Default::default()
        }
    }

    /// Validate configuration parameters
    pub fn validate(&self) -> Result<(), String> {
        if self.kernel_radius < 1.0 || self.kernel_radius > 64.0 {
            return Err("Kernel radius must be between 1 and 64".to_string());
        }

        if self.frequency_min < 0.0 || self.frequency_max < 0.0 {
            return Err("Frequencies must be non-negative".to_string());
        }

        if self.frequency_max < self.frequency_min {
            return Err("Frequency max must be >= frequency min".to_string());
        }

        if self.distortion_amount < 0.0 || self.distortion_amount > 1.0 {
            return Err("Distortion amount must be between 0 and 1".to_string());
        }

        if self.overlay_opacity < 0.0 || self.overlay_opacity > 1.0 {
            return Err("Overlay opacity must be between 0 and 1".to_string());
        }

        Ok(())
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_default_config() {
        let config = OscillatorConfig::default();
        assert!(config.enabled);
        assert_eq!(config.simulation_resolution, SimulationResolution::Medium);
        assert!(config.validate().is_ok());
    }

    #[test]
    fn test_presets() {
        let subtle = OscillatorConfig::preset_subtle();
        let dramatic = OscillatorConfig::preset_dramatic();
        let rings = OscillatorConfig::preset_rings();

        assert!(subtle.validate().is_ok());
        assert!(dramatic.validate().is_ok());
        assert!(rings.validate().is_ok());

        assert!(subtle.distortion_amount < dramatic.distortion_amount);
    }

    #[test]
    fn test_validation() {
        let mut config = OscillatorConfig::default();

        // Valid config
        assert!(config.validate().is_ok());

        // Invalid kernel radius
        config.kernel_radius = 100.0;
        assert!(config.validate().is_err());
        config.kernel_radius = 16.0;

        // Invalid frequency range
        config.frequency_min = 5.0;
        config.frequency_max = 2.0;
        assert!(config.validate().is_err());
        config.frequency_min = 0.5;

        // Invalid distortion amount
        config.distortion_amount = 1.5;
        assert!(config.validate().is_err());
    }

    #[test]
    fn test_simulation_resolution() {
        assert_eq!(SimulationResolution::Low.dimensions(), (128, 128));
        assert_eq!(SimulationResolution::Medium.dimensions(), (256, 256));
        assert_eq!(SimulationResolution::High.dimensions(), (512, 512));
    }

    #[test]
    fn test_color_mode_conversion() {
        assert_eq!(ColorMode::Off.to_u32(), 0);
        assert_eq!(ColorMode::Rainbow.to_u32(), 1);
        assert_eq!(ColorMode::BlackWhite.to_u32(), 2);
        assert_eq!(ColorMode::Complementary.to_u32(), 3);
    }
}
