//! Image decoder implementations for still images, GIFs, and image sequences
//!
//! Phase 1 feature implementation:
//! - Still images: PNG, JPEG, TIFF via `image` crate
//! - Animated GIF: Frame-by-frame playback with timing
//! - Image sequences: Directory of numbered frames with custom FPS

use crate::{DecodedFrame, MediaError, PixelFormat, Result, VideoDecoder};
use image::{AnimationDecoder, DynamicImage};
use std::path::{Path, PathBuf};
use std::time::Duration;
use tracing::info;
use walkdir::WalkDir;

// ============================================================================
// Still Image Decoder
// ============================================================================

/// Decoder for still images (PNG, JPEG, TIFF, etc.)
///
/// Still images are treated as single-frame "videos" with infinite duration.
/// Seeking has no effect, and next_frame() always returns the same image.
pub struct StillImageDecoder {
    width: u32,
    height: u32,
    frame_data: Vec<u8>,
    has_been_read: bool,
}

impl StillImageDecoder {
    /// Load a still image from a file
    pub fn open<P: AsRef<Path>>(path: P) -> Result<Self> {
        let path = path.as_ref();

        if !path.exists() {
            return Err(MediaError::FileOpen(format!(
                "File not found: {}",
                path.display()
            )));
        }

        // Load image using the `image` crate
        let image = image::open(path)
            .map_err(|e| MediaError::DecoderError(format!("Failed to load image: {}", e)))?;

        let width = image.width();
        let height = image.height();

        // Convert to RGBA8
        let rgba_image = image.to_rgba8();
        let frame_data = rgba_image.into_raw();

        info!(
            "Still image loaded: {}x{} from {}",
            width,
            height,
            path.display()
        );

        Ok(Self {
            width,
            height,
            frame_data,
            has_been_read: false,
        })
    }

    /// Check if the file format is supported
    pub fn supports_format<P: AsRef<Path>>(path: P) -> bool {
        if let Some(ext) = path.as_ref().extension() {
            let ext_str = ext.to_string_lossy().to_lowercase();
            matches!(
                ext_str.as_str(),
                "png" | "jpg" | "jpeg" | "tif" | "tiff" | "bmp" | "webp"
            )
        } else {
            false
        }
    }
}

impl VideoDecoder for StillImageDecoder {
    fn next_frame(&mut self) -> Result<DecodedFrame> {
        // Still images can be read repeatedly
        // For proper "video" behavior, we only return the frame once
        // and then return EndOfStream to match video semantics
        if self.has_been_read {
            return Err(MediaError::EndOfStream);
        }

        self.has_been_read = true;

        Ok(DecodedFrame {
            data: self.frame_data.clone(),
            format: PixelFormat::RGBA8,
            width: self.width,
            height: self.height,
            pts: Duration::ZERO,
        })
    }

    fn seek(&mut self, _timestamp: Duration) -> Result<()> {
        // Seeking in a still image resets to beginning
        self.has_been_read = false;
        Ok(())
    }

    fn duration(&self) -> Duration {
        // Still images have "infinite" duration represented as a very long time
        Duration::from_secs(3600 * 24 * 365) // 1 year
    }

    fn resolution(&self) -> (u32, u32) {
        (self.width, self.height)
    }

    fn fps(&self) -> f64 {
        // Still images don't have FPS, but we return 1 for consistency
        1.0
    }
}

// ============================================================================
// GIF Decoder
// ============================================================================

/// Decoder for animated GIF files
///
/// Supports frame-by-frame playback with proper timing based on GIF delays.
pub struct GifDecoder {
    frames: Vec<(DynamicImage, Duration)>, // (frame, delay)
    width: u32,
    height: u32,
    current_frame: usize,
    current_time: Duration,
    total_duration: Duration,
    fps: f64,
}

impl GifDecoder {
    /// Load an animated GIF from a file
    pub fn open<P: AsRef<Path>>(path: P) -> Result<Self> {
        let path = path.as_ref();

        if !path.exists() {
            return Err(MediaError::FileOpen(format!(
                "File not found: {}",
                path.display()
            )));
        }

        // Open the file
        let file = std::fs::File::open(path)
            .map_err(|e| MediaError::FileOpen(format!("Failed to open file: {}", e)))?;

        let decoder = image::codecs::gif::GifDecoder::new(file)
            .map_err(|e| MediaError::DecoderError(format!("Failed to decode GIF: {}", e)))?;

        // Extract frames
        let frames_iter = decoder.into_frames();

        let mut frames = Vec::new();
        let mut total_duration = Duration::ZERO;

        for frame_result in frames_iter {
            let frame = frame_result
                .map_err(|e| MediaError::DecoderError(format!("Failed to decode frame: {}", e)))?;

            let delay = frame.delay();
            let delay_duration = Duration::from_millis(
                (delay.numer_denom_ms().0 as f64 / delay.numer_denom_ms().1 as f64 * 1000.0) as u64,
            );

            let image = DynamicImage::ImageRgba8(frame.into_buffer());
            frames.push((image, delay_duration));
            total_duration += delay_duration;
        }

        if frames.is_empty() {
            return Err(MediaError::DecoderError(
                "GIF has no frames".to_string(),
            ));
        }

        let width = frames[0].0.width();
        let height = frames[0].0.height();
        let fps = frames.len() as f64 / total_duration.as_secs_f64();

        info!(
            "GIF loaded: {}x{}, {} frames, {:.2}s duration, {:.2} fps from {}",
            width,
            height,
            frames.len(),
            total_duration.as_secs_f64(),
            fps,
            path.display()
        );

        Ok(Self {
            frames,
            width,
            height,
            current_frame: 0,
            current_time: Duration::ZERO,
            total_duration,
            fps,
        })
    }

    /// Check if the file is a GIF
    pub fn supports_format<P: AsRef<Path>>(path: P) -> bool {
        if let Some(ext) = path.as_ref().extension() {
            let ext_str = ext.to_string_lossy().to_lowercase();
            ext_str == "gif"
        } else {
            false
        }
    }

    /// Get the frame at the current time
    fn get_current_frame(&self) -> &(DynamicImage, Duration) {
        &self.frames[self.current_frame]
    }
}

impl VideoDecoder for GifDecoder {
    fn next_frame(&mut self) -> Result<DecodedFrame> {
        if self.current_time >= self.total_duration {
            return Err(MediaError::EndOfStream);
        }

        let (image, delay) = self.get_current_frame();

        // Convert to RGBA8
        let rgba_image = image.to_rgba8();
        let frame_data = rgba_image.into_raw();

        let pts = self.current_time;

        // Advance to next frame
        self.current_time += *delay;
        self.current_frame = (self.current_frame + 1) % self.frames.len();

        Ok(DecodedFrame {
            data: frame_data,
            format: PixelFormat::RGBA8,
            width: self.width,
            height: self.height,
            pts,
        })
    }

    fn seek(&mut self, timestamp: Duration) -> Result<()> {
        if timestamp > self.total_duration {
            return Err(MediaError::SeekError(
                "Timestamp beyond duration".to_string(),
            ));
        }

        // Find the frame at the given timestamp
        let mut accumulated_time = Duration::ZERO;
        for (idx, (_image, delay)) in self.frames.iter().enumerate() {
            if accumulated_time + *delay > timestamp {
                self.current_frame = idx;
                self.current_time = accumulated_time;
                return Ok(());
            }
            accumulated_time += *delay;
        }

        // If we get here, seek to last frame
        self.current_frame = self.frames.len() - 1;
        self.current_time = self.total_duration;
        Ok(())
    }

    fn duration(&self) -> Duration {
        self.total_duration
    }

    fn resolution(&self) -> (u32, u32) {
        (self.width, self.height)
    }

    fn fps(&self) -> f64 {
        self.fps
    }
}

// ============================================================================
// Image Sequence Decoder
// ============================================================================

/// Decoder for image sequences (directory of numbered frames)
///
/// Loads a directory of images and plays them back as a video at a specified FPS.
/// Supports common naming patterns: frame_001.png, img001.jpg, etc.
pub struct ImageSequenceDecoder {
    frames: Vec<PathBuf>,
    width: u32,
    height: u32,
    current_frame: usize,
    fps: f64,
    duration: Duration,
    current_time: Duration,
    // Cache for the current frame to avoid re-loading
    cached_frame: Option<(usize, Vec<u8>)>,
}

impl ImageSequenceDecoder {
    /// Load an image sequence from a directory
    ///
    /// # Arguments
    /// * `directory` - Path to directory containing numbered images
    /// * `fps` - Frame rate for playback (e.g., 30.0)
    pub fn open<P: AsRef<Path>>(directory: P, fps: f64) -> Result<Self> {
        let directory = directory.as_ref();

        if !directory.exists() {
            return Err(MediaError::FileOpen(format!(
                "Directory not found: {}",
                directory.display()
            )));
        }

        if !directory.is_dir() {
            return Err(MediaError::FileOpen(format!(
                "Path is not a directory: {}",
                directory.display()
            )));
        }

        // Scan directory for image files
        let mut frames = Vec::new();

        for entry in WalkDir::new(directory)
            .max_depth(1)
            .sort_by_file_name()
            .into_iter()
            .filter_map(|e| e.ok())
        {
            let path = entry.path();
            if path.is_file() && Self::is_supported_image(path) {
                frames.push(path.to_path_buf());
            }
        }

        if frames.is_empty() {
            return Err(MediaError::DecoderError(format!(
                "No image files found in directory: {}",
                directory.display()
            )));
        }

        // Sort frames by filename (natural sorting for numbered sequences)
        frames.sort();

        // Load first frame to get dimensions
        let first_image = image::open(&frames[0])
            .map_err(|e| MediaError::DecoderError(format!("Failed to load first frame: {}", e)))?;

        let width = first_image.width();
        let height = first_image.height();

        let duration = Duration::from_secs_f64(frames.len() as f64 / fps);

        info!(
            "Image sequence loaded: {}x{}, {} frames, {:.2}s duration @ {:.2} fps from {}",
            width,
            height,
            frames.len(),
            duration.as_secs_f64(),
            fps,
            directory.display()
        );

        Ok(Self {
            frames,
            width,
            height,
            current_frame: 0,
            fps,
            duration,
            current_time: Duration::ZERO,
            cached_frame: None,
        })
    }

    /// Check if a file is a supported image format
    fn is_supported_image(path: &Path) -> bool {
        if let Some(ext) = path.extension() {
            let ext_str = ext.to_string_lossy().to_lowercase();
            matches!(
                ext_str.as_str(),
                "png" | "jpg" | "jpeg" | "tif" | "tiff" | "bmp" | "webp"
            )
        } else {
            false
        }
    }

    /// Load and cache a frame
    fn load_frame(&mut self, index: usize) -> Result<Vec<u8>> {
        // Check cache
        if let Some((cached_idx, ref data)) = self.cached_frame {
            if cached_idx == index {
                return Ok(data.clone());
            }
        }

        // Load frame
        let path = &self.frames[index];
        let image = image::open(path)
            .map_err(|e| MediaError::DecoderError(format!("Failed to load frame: {}", e)))?;

        let rgba_image = image.to_rgba8();
        let frame_data = rgba_image.into_raw();

        // Update cache
        self.cached_frame = Some((index, frame_data.clone()));

        Ok(frame_data)
    }
}

impl VideoDecoder for ImageSequenceDecoder {
    fn next_frame(&mut self) -> Result<DecodedFrame> {
        if self.current_frame >= self.frames.len() {
            return Err(MediaError::EndOfStream);
        }

        let frame_data = self.load_frame(self.current_frame)?;
        let pts = self.current_time;

        // Advance to next frame
        self.current_frame += 1;
        self.current_time += Duration::from_secs_f64(1.0 / self.fps);

        Ok(DecodedFrame {
            data: frame_data,
            format: PixelFormat::RGBA8,
            width: self.width,
            height: self.height,
            pts,
        })
    }

    fn seek(&mut self, timestamp: Duration) -> Result<()> {
        if timestamp > self.duration {
            return Err(MediaError::SeekError(
                "Timestamp beyond duration".to_string(),
            ));
        }

        let frame_index = (timestamp.as_secs_f64() * self.fps) as usize;
        self.current_frame = frame_index.min(self.frames.len() - 1);
        self.current_time = Duration::from_secs_f64(self.current_frame as f64 / self.fps);

        Ok(())
    }

    fn duration(&self) -> Duration {
        self.duration
    }

    fn resolution(&self) -> (u32, u32) {
        (self.width, self.height)
    }

    fn fps(&self) -> f64 {
        self.fps
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_still_image_supports_format() {
        assert!(StillImageDecoder::supports_format("test.png"));
        assert!(StillImageDecoder::supports_format("test.jpg"));
        assert!(StillImageDecoder::supports_format("test.jpeg"));
        assert!(StillImageDecoder::supports_format("test.tif"));
        assert!(StillImageDecoder::supports_format("test.tiff"));
        assert!(!StillImageDecoder::supports_format("test.mp4"));
        assert!(!StillImageDecoder::supports_format("test.txt"));
    }

    #[test]
    fn test_gif_supports_format() {
        assert!(GifDecoder::supports_format("test.gif"));
        assert!(!GifDecoder::supports_format("test.png"));
        assert!(!GifDecoder::supports_format("test.jpg"));
    }

    #[test]
    fn test_image_sequence_is_supported_image() {
        assert!(ImageSequenceDecoder::is_supported_image(Path::new(
            "frame001.png"
        )));
        assert!(ImageSequenceDecoder::is_supported_image(Path::new(
            "frame001.jpg"
        )));
        assert!(!ImageSequenceDecoder::is_supported_image(Path::new(
            "frame001.mp4"
        )));
    }
}
