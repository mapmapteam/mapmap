//! Video decoder abstraction

use crate::{MediaError, Result};
use std::path::Path;
use std::time::Duration;

/// Pixel format for decoded frames
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum PixelFormat {
    RGBA8,
    BGRA8,
    YUV420P,
}

/// A decoded video frame
#[derive(Clone)]
pub struct DecodedFrame {
    pub data: Vec<u8>,
    pub format: PixelFormat,
    pub width: u32,
    pub height: u32,
    pub pts: Duration,
}

impl DecodedFrame {
    /// Get the size of the frame data in bytes
    pub fn size_bytes(&self) -> usize {
        self.data.len()
    }

    /// Convert YUV420P to RGBA8 (if needed)
    pub fn to_rgba(&self) -> Vec<u8> {
        match self.format {
            PixelFormat::RGBA8 => self.data.clone(),
            PixelFormat::BGRA8 => {
                // Convert BGRA to RGBA
                self.data
                    .chunks_exact(4)
                    .flat_map(|pixel| [pixel[2], pixel[1], pixel[0], pixel[3]])
                    .collect()
            }
            PixelFormat::YUV420P => {
                // TODO: Implement YUV420P to RGBA conversion
                // For now, return empty data
                vec![0; (self.width * self.height * 4) as usize]
            }
        }
    }
}

/// Video decoder trait
pub trait VideoDecoder: Send {
    fn next_frame(&mut self) -> Result<DecodedFrame>;
    fn seek(&mut self, timestamp: Duration) -> Result<()>;
    fn duration(&self) -> Duration;
    fn resolution(&self) -> (u32, u32);
    fn fps(&self) -> f64;
}

/// FFmpeg-based video decoder (stub implementation for Phase 0)
/// NOTE: Full FFmpeg integration requires ffmpeg-next crate setup
pub struct FFmpegDecoder {
    width: u32,
    height: u32,
    duration: Duration,
    fps: f64,
    current_time: Duration,
    frame_count: u64,
}

impl FFmpegDecoder {
    /// Open a video file
    /// NOTE: This is a stub implementation. Full implementation would use ffmpeg-next
    pub fn open<P: AsRef<Path>>(path: P) -> Result<Self> {
        let path = path.as_ref();

        // Check if file exists
        if !path.exists() {
            return Err(MediaError::FileOpen(format!(
                "File not found: {}",
                path.display()
            )));
        }

        // Stub: Return a mock decoder
        // In full implementation, this would:
        // 1. Open FFmpeg format context
        // 2. Find video stream
        // 3. Create decoder
        // 4. Setup scaler for RGBA output

        Ok(Self {
            width: 1920,
            height: 1080,
            duration: Duration::from_secs(60),
            fps: 30.0,
            current_time: Duration::ZERO,
            frame_count: 0,
        })
    }

    /// Generate a test pattern frame
    fn generate_test_frame(&self) -> DecodedFrame {
        let size = (self.width * self.height * 4) as usize;
        let mut data = vec![0u8; size];

        // Generate a simple gradient pattern with animation
        let time_offset = (self.frame_count % 255) as u8;

        for y in 0..self.height {
            for x in 0..self.width {
                let idx = ((y * self.width + x) * 4) as usize;
                data[idx] = ((x * 255 / self.width) as u8).wrapping_add(time_offset); // R
                data[idx + 1] = ((y * 255 / self.height) as u8).wrapping_add(time_offset); // G
                data[idx + 2] = 128; // B
                data[idx + 3] = 255; // A
            }
        }

        DecodedFrame {
            data,
            format: PixelFormat::RGBA8,
            width: self.width,
            height: self.height,
            pts: self.current_time,
        }
    }
}

impl VideoDecoder for FFmpegDecoder {
    fn next_frame(&mut self) -> Result<DecodedFrame> {
        if self.current_time >= self.duration {
            return Err(MediaError::EndOfStream);
        }

        let frame = self.generate_test_frame();

        // Advance time
        self.current_time += Duration::from_secs_f64(1.0 / self.fps);
        self.frame_count += 1;

        Ok(frame)
    }

    fn seek(&mut self, timestamp: Duration) -> Result<()> {
        if timestamp > self.duration {
            return Err(MediaError::SeekError(
                "Timestamp beyond duration".to_string(),
            ));
        }

        self.current_time = timestamp;
        self.frame_count = (timestamp.as_secs_f64() * self.fps) as u64;

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
    use std::path::PathBuf;

    #[test]
    fn test_decoded_frame_size() {
        let frame = DecodedFrame {
            data: vec![0; 1920 * 1080 * 4],
            format: PixelFormat::RGBA8,
            width: 1920,
            height: 1080,
            pts: Duration::ZERO,
        };

        assert_eq!(frame.size_bytes(), 1920 * 1080 * 4);
    }

    #[test]
    fn test_pixel_format_conversion() {
        let frame = DecodedFrame {
            data: vec![255, 0, 0, 255], // Red pixel in RGBA
            format: PixelFormat::RGBA8,
            width: 1,
            height: 1,
            pts: Duration::ZERO,
        };

        let rgba = frame.to_rgba();
        assert_eq!(rgba, vec![255, 0, 0, 255]);
    }
}
