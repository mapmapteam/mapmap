//! LUT (Look-Up Table) System for Color Grading
//!
//! Phase 3: Effects Pipeline
//! Provides 3D LUT support for advanced color grading and correction

use glam::Vec3;
use serde::{Deserialize, Serialize};
use std::path::{Path, PathBuf};

/// LUT size (standard is 32x32x32 or 64x64x64)
pub const LUT_SIZE_32: usize = 32;
pub const LUT_SIZE_64: usize = 64;

/// LUT format
#[derive(Debug, Clone, Copy, PartialEq, Eq, Serialize, Deserialize)]
pub enum LutFormat {
    /// .cube format (most common)
    Cube,
    /// 3D LUT texture (32x32x32)
    Texture32,
    /// 3D LUT texture (64x64x64)
    Texture64,
    /// Hald CLUT (image-based LUT)
    HaldClut,
}

/// 3D LUT for color grading
#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct Lut3D {
    /// LUT name
    pub name: String,

    /// LUT size (dimension of the cube)
    pub size: usize,

    /// LUT data as flat array: [R, G, B, R, G, B, ...]
    /// Total length is size^3 * 3
    pub data: Vec<f32>,

    /// Original file path (if loaded from file)
    pub file_path: Option<PathBuf>,
}

impl Lut3D {
    /// Create a new identity LUT (no color transformation)
    pub fn identity(size: usize) -> Self {
        let total_entries = size * size * size;
        let mut data = Vec::with_capacity(total_entries * 3);

        for b in 0..size {
            for g in 0..size {
                for r in 0..size {
                    data.push(r as f32 / (size - 1) as f32);
                    data.push(g as f32 / (size - 1) as f32);
                    data.push(b as f32 / (size - 1) as f32);
                }
            }
        }

        Self {
            name: "Identity".to_string(),
            size,
            data,
            file_path: None,
        }
    }

    /// Load a LUT from a .cube file
    pub fn from_cube_file(path: impl AsRef<Path>) -> Result<Self, LutError> {
        let path = path.as_ref();
        let content = std::fs::read_to_string(path)
            .map_err(|e| LutError::IoError(e.to_string()))?;

        Self::parse_cube(&content, Some(path.to_path_buf()))
    }

    /// Parse .cube file format
    pub fn parse_cube(content: &str, file_path: Option<PathBuf>) -> Result<Self, LutError> {
        let mut size = 0;
        let mut data = Vec::new();
        let mut name = file_path
            .as_ref()
            .and_then(|p| p.file_stem())
            .and_then(|s| s.to_str())
            .unwrap_or("Unnamed LUT")
            .to_string();

        for line in content.lines() {
            let line = line.trim();

            // Skip empty lines and comments
            if line.is_empty() || line.starts_with('#') {
                continue;
            }

            // Parse header
            if line.starts_with("TITLE") {
                name = line.split_whitespace().skip(1).collect::<Vec<_>>().join(" ");
                name = name.trim_matches('"').to_string();
                continue;
            }

            if line.starts_with("LUT_3D_SIZE") {
                let parts: Vec<&str> = line.split_whitespace().collect();
                if parts.len() < 2 {
                    return Err(LutError::ParseError("Invalid LUT_3D_SIZE".to_string()));
                }
                size = parts[1].parse()
                    .map_err(|_| LutError::ParseError("Invalid size value".to_string()))?;
                continue;
            }

            if line.starts_with("DOMAIN_MIN") || line.starts_with("DOMAIN_MAX") {
                // Skip domain specifications for now
                continue;
            }

            // Parse RGB values
            let parts: Vec<&str> = line.split_whitespace().collect();
            if parts.len() == 3 {
                let r: f32 = parts[0].parse()
                    .map_err(|_| LutError::ParseError(format!("Invalid R value: {}", parts[0])))?;
                let g: f32 = parts[1].parse()
                    .map_err(|_| LutError::ParseError(format!("Invalid G value: {}", parts[1])))?;
                let b: f32 = parts[2].parse()
                    .map_err(|_| LutError::ParseError(format!("Invalid B value: {}", parts[2])))?;

                data.push(r);
                data.push(g);
                data.push(b);
            }
        }

        // Validate
        if size == 0 {
            return Err(LutError::ParseError("No LUT_3D_SIZE found".to_string()));
        }

        let expected_entries = size * size * size * 3;
        if data.len() != expected_entries {
            return Err(LutError::ParseError(
                format!("Expected {} values, got {}", expected_entries, data.len())
            ));
        }

        Ok(Self {
            name,
            size,
            data,
            file_path,
        })
    }

    /// Apply LUT to a color (CPU-side, for testing)
    pub fn apply(&self, color: Vec3) -> Vec3 {
        let r = color.x.clamp(0.0, 1.0);
        let g = color.y.clamp(0.0, 1.0);
        let b = color.z.clamp(0.0, 1.0);

        // Scale to LUT coordinates
        let size_f = (self.size - 1) as f32;
        let r_coord = r * size_f;
        let g_coord = g * size_f;
        let b_coord = b * size_f;

        // Get integer indices
        let r0 = r_coord.floor() as usize;
        let g0 = g_coord.floor() as usize;
        let b0 = b_coord.floor() as usize;

        let r1 = (r0 + 1).min(self.size - 1);
        let g1 = (g0 + 1).min(self.size - 1);
        let b1 = (b0 + 1).min(self.size - 1);

        // Get fractional parts for interpolation
        let r_frac = r_coord - r0 as f32;
        let g_frac = g_coord - g0 as f32;
        let b_frac = b_coord - b0 as f32;

        // Trilinear interpolation (8 samples)
        let c000 = self.sample(r0, g0, b0);
        let c001 = self.sample(r0, g0, b1);
        let c010 = self.sample(r0, g1, b0);
        let c011 = self.sample(r0, g1, b1);
        let c100 = self.sample(r1, g0, b0);
        let c101 = self.sample(r1, g0, b1);
        let c110 = self.sample(r1, g1, b0);
        let c111 = self.sample(r1, g1, b1);

        // Interpolate along R axis
        let c00 = c000.lerp(c100, r_frac);
        let c01 = c001.lerp(c101, r_frac);
        let c10 = c010.lerp(c110, r_frac);
        let c11 = c011.lerp(c111, r_frac);

        // Interpolate along G axis
        let c0 = c00.lerp(c10, g_frac);
        let c1 = c01.lerp(c11, g_frac);

        // Interpolate along B axis
        c0.lerp(c1, b_frac)
    }

    /// Sample a specific LUT entry
    fn sample(&self, r: usize, g: usize, b: usize) -> Vec3 {
        let index = (b * self.size * self.size + g * self.size + r) * 3;
        Vec3::new(
            self.data[index],
            self.data[index + 1],
            self.data[index + 2],
        )
    }

    /// Convert LUT to texture data for GPU (RGBA format)
    pub fn to_texture_data(&self) -> Vec<u8> {
        let total_entries = self.size * self.size * self.size;
        let mut rgba_data = Vec::with_capacity(total_entries * 4);

        for i in 0..total_entries {
            let base = i * 3;
            rgba_data.push((self.data[base] * 255.0) as u8);
            rgba_data.push((self.data[base + 1] * 255.0) as u8);
            rgba_data.push((self.data[base + 2] * 255.0) as u8);
            rgba_data.push(255); // Alpha
        }

        rgba_data
    }

    /// Convert LUT to 2D texture atlas (for easier GPU upload)
    /// Arranges 3D LUT as a 2D grid: size x (size * size)
    pub fn to_2d_texture_data(&self) -> (Vec<u8>, u32, u32) {
        let width = self.size as u32;
        let height = (self.size * self.size) as u32;
        let mut rgba_data = Vec::with_capacity((width * height * 4) as usize);

        for slice in 0..self.size {
            for row in 0..self.size {
                for col in 0..self.size {
                    let index = (slice * self.size * self.size + row * self.size + col) * 3;
                    rgba_data.push((self.data[index] * 255.0) as u8);
                    rgba_data.push((self.data[index + 1] * 255.0) as u8);
                    rgba_data.push((self.data[index + 2] * 255.0) as u8);
                    rgba_data.push(255);
                }
            }
        }

        (rgba_data, width, height)
    }

    /// Generate common preset LUTs
    pub fn preset(preset: LutPreset, size: usize) -> Self {
        let mut lut = Self::identity(size);
        lut.name = preset.name().to_string();

        match preset {
            LutPreset::Identity => lut,
            LutPreset::Grayscale => {
                for i in 0..(size * size * size) {
                    let base = i * 3;
                    let gray = 0.299 * lut.data[base]
                             + 0.587 * lut.data[base + 1]
                             + 0.114 * lut.data[base + 2];
                    lut.data[base] = gray;
                    lut.data[base + 1] = gray;
                    lut.data[base + 2] = gray;
                }
                lut
            }
            LutPreset::Sepia => {
                for i in 0..(size * size * size) {
                    let base = i * 3;
                    let r = lut.data[base];
                    let g = lut.data[base + 1];
                    let b = lut.data[base + 2];

                    lut.data[base] = (0.393 * r + 0.769 * g + 0.189 * b).min(1.0);
                    lut.data[base + 1] = (0.349 * r + 0.686 * g + 0.168 * b).min(1.0);
                    lut.data[base + 2] = (0.272 * r + 0.534 * g + 0.131 * b).min(1.0);
                }
                lut
            }
            LutPreset::CoolTone => {
                for i in 0..(size * size * size) {
                    let base = i * 3;
                    lut.data[base] *= 0.9;      // Reduce red
                    lut.data[base + 2] *= 1.1;  // Increase blue
                }
                lut
            }
            LutPreset::WarmTone => {
                for i in 0..(size * size * size) {
                    let base = i * 3;
                    lut.data[base] *= 1.1;      // Increase red
                    lut.data[base + 2] *= 0.9;  // Reduce blue
                }
                lut
            }
            LutPreset::HighContrast => {
                for i in 0..(size * size * size) {
                    let base = i * 3;
                    for j in 0..3 {
                        let v = lut.data[base + j];
                        lut.data[base + j] = ((v - 0.5) * 1.5 + 0.5).clamp(0.0, 1.0);
                    }
                }
                lut
            }
            LutPreset::Inverted => {
                for i in 0..(size * size * size) {
                    let base = i * 3;
                    lut.data[base] = 1.0 - lut.data[base];
                    lut.data[base + 1] = 1.0 - lut.data[base + 1];
                    lut.data[base + 2] = 1.0 - lut.data[base + 2];
                }
                lut
            }
        }
    }

    /// Save LUT to .cube file
    pub fn save_cube(&self, path: impl AsRef<Path>) -> Result<(), LutError> {
        let mut content = String::new();

        content.push_str(&format!("TITLE \"{}\"\n", self.name));
        content.push_str(&format!("LUT_3D_SIZE {}\n", self.size));
        content.push_str("DOMAIN_MIN 0.0 0.0 0.0\n");
        content.push_str("DOMAIN_MAX 1.0 1.0 1.0\n\n");

        for i in 0..(self.size * self.size * self.size) {
            let base = i * 3;
            content.push_str(&format!(
                "{:.6} {:.6} {:.6}\n",
                self.data[base],
                self.data[base + 1],
                self.data[base + 2]
            ));
        }

        std::fs::write(path, content)
            .map_err(|e| LutError::IoError(e.to_string()))?;

        Ok(())
    }
}

/// LUT preset types
#[derive(Debug, Clone, Copy, PartialEq, Eq, Serialize, Deserialize)]
pub enum LutPreset {
    Identity,
    Grayscale,
    Sepia,
    CoolTone,
    WarmTone,
    HighContrast,
    Inverted,
}

impl LutPreset {
    pub fn name(&self) -> &'static str {
        match self {
            LutPreset::Identity => "Identity",
            LutPreset::Grayscale => "Grayscale",
            LutPreset::Sepia => "Sepia",
            LutPreset::CoolTone => "Cool Tone",
            LutPreset::WarmTone => "Warm Tone",
            LutPreset::HighContrast => "High Contrast",
            LutPreset::Inverted => "Inverted",
        }
    }

    pub fn all() -> Vec<LutPreset> {
        vec![
            LutPreset::Identity,
            LutPreset::Grayscale,
            LutPreset::Sepia,
            LutPreset::CoolTone,
            LutPreset::WarmTone,
            LutPreset::HighContrast,
            LutPreset::Inverted,
        ]
    }
}

/// LUT manager for handling multiple LUTs
#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct LutManager {
    luts: Vec<Lut3D>,
    active_lut_index: Option<usize>,
}

impl LutManager {
    pub fn new() -> Self {
        let mut manager = Self {
            luts: Vec::new(),
            active_lut_index: None,
        };

        // Add identity LUT by default
        manager.add_lut(Lut3D::identity(LUT_SIZE_32));

        manager
    }

    pub fn add_lut(&mut self, lut: Lut3D) -> usize {
        self.luts.push(lut);
        self.luts.len() - 1
    }

    pub fn remove_lut(&mut self, index: usize) -> Option<Lut3D> {
        if index < self.luts.len() {
            let lut = self.luts.remove(index);
            if self.active_lut_index == Some(index) {
                self.active_lut_index = None;
            }
            Some(lut)
        } else {
            None
        }
    }

    pub fn get_lut(&self, index: usize) -> Option<&Lut3D> {
        self.luts.get(index)
    }

    pub fn active_lut(&self) -> Option<&Lut3D> {
        self.active_lut_index.and_then(|i| self.luts.get(i))
    }

    pub fn set_active_lut(&mut self, index: usize) {
        if index < self.luts.len() {
            self.active_lut_index = Some(index);
        }
    }

    pub fn luts(&self) -> &[Lut3D] {
        &self.luts
    }

    pub fn load_from_file(&mut self, path: impl AsRef<Path>) -> Result<usize, LutError> {
        let lut = Lut3D::from_cube_file(path)?;
        Ok(self.add_lut(lut))
    }
}

impl Default for LutManager {
    fn default() -> Self {
        Self::new()
    }
}

/// LUT error types
#[derive(Debug, thiserror::Error)]
pub enum LutError {
    #[error("IO error: {0}")]
    IoError(String),

    #[error("Parse error: {0}")]
    ParseError(String),

    #[error("Invalid LUT size: {0}")]
    InvalidSize(usize),

    #[error("LUT not found")]
    NotFound,
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_identity_lut() {
        let lut = Lut3D::identity(LUT_SIZE_32);
        assert_eq!(lut.size, LUT_SIZE_32);
        assert_eq!(lut.data.len(), LUT_SIZE_32 * LUT_SIZE_32 * LUT_SIZE_32 * 3);

        // Test that identity LUT doesn't change colors
        let color = Vec3::new(0.5, 0.7, 0.3);
        let result = lut.apply(color);
        assert!((result - color).length() < 0.01);
    }

    #[test]
    fn test_grayscale_lut() {
        let lut = Lut3D::preset(LutPreset::Grayscale, LUT_SIZE_32);

        let color = Vec3::new(1.0, 0.0, 0.0); // Pure red
        let result = lut.apply(color);

        // Should be grayscale (all components equal)
        assert!((result.x - result.y).abs() < 0.01);
        assert!((result.y - result.z).abs() < 0.01);
    }

    #[test]
    fn test_lut_manager() {
        let mut manager = LutManager::new();
        assert_eq!(manager.luts().len(), 1); // Identity LUT

        let grayscale = Lut3D::preset(LutPreset::Grayscale, LUT_SIZE_32);
        let index = manager.add_lut(grayscale);

        manager.set_active_lut(index);
        assert!(manager.active_lut().is_some());
        assert_eq!(manager.active_lut().unwrap().name, "Grayscale");
    }

    #[test]
    fn test_parse_cube_format() {
        let cube_content = r#"
TITLE "Test LUT"
LUT_3D_SIZE 2
0.0 0.0 0.0
1.0 0.0 0.0
0.0 1.0 0.0
1.0 1.0 0.0
0.0 0.0 1.0
1.0 0.0 1.0
0.0 1.0 1.0
1.0 1.0 1.0
"#;

        let lut = Lut3D::parse_cube(cube_content, None);
        assert!(lut.is_ok());

        let lut = lut.unwrap();
        assert_eq!(lut.size, 2);
        assert_eq!(lut.name, "Test LUT");
    }

    #[test]
    fn test_lut_texture_conversion() {
        let lut = Lut3D::identity(LUT_SIZE_32);
        let texture_data = lut.to_texture_data();
        assert_eq!(texture_data.len(), LUT_SIZE_32 * LUT_SIZE_32 * LUT_SIZE_32 * 4);

        let (data_2d, width, height) = lut.to_2d_texture_data();
        assert_eq!(width as usize, LUT_SIZE_32);
        assert_eq!(height as usize, LUT_SIZE_32 * LUT_SIZE_32);
        assert_eq!(data_2d.len(), (width * height * 4) as usize);
    }
}
