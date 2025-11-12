//! Format conversion utilities.
//!
//! This module provides utilities for converting between different pixel formats,
//! particularly YUV to RGB conversions which are common in video I/O.

use crate::error::{IoError, Result};
use crate::format::{PixelFormat, VideoFormat, VideoFrame};

/// Format converter for pixel format conversion.
///
/// Provides software-based conversion between various pixel formats.
/// For performance-critical applications, consider using GPU-accelerated
/// conversion via compute shaders.
pub struct FormatConverter {
    // Future: Add GPU context for hardware-accelerated conversion
}

impl FormatConverter {
    /// Creates a new format converter.
    pub fn new() -> Self {
        Self {}
    }

    /// Converts a video frame from one format to another.
    ///
    /// # Parameters
    ///
    /// - `frame` - The source frame to convert
    /// - `target_format` - The desired output format
    ///
    /// # Returns
    ///
    /// A new frame with the converted pixel data.
    ///
    /// # Errors
    ///
    /// Returns an error if the conversion is not supported or fails.
    pub fn convert(&self, frame: &VideoFrame, target_format: &VideoFormat) -> Result<VideoFrame> {
        // If formats match, just clone the frame
        if frame.format.pixel_format == target_format.pixel_format {
            if frame.format.width == target_format.width
                && frame.format.height == target_format.height
            {
                return Ok(frame.clone());
            }
        }

        // Dispatch to specific conversion functions
        match (
            &frame.format.pixel_format,
            &target_format.pixel_format,
        ) {
            // RGB conversions
            (PixelFormat::RGBA8, PixelFormat::BGRA8) => {
                self.rgba_to_bgra(frame, target_format)
            }
            (PixelFormat::BGRA8, PixelFormat::RGBA8) => {
                self.bgra_to_rgba(frame, target_format)
            }
            (PixelFormat::RGB8, PixelFormat::RGBA8) => {
                self.rgb_to_rgba(frame, target_format)
            }
            (PixelFormat::RGBA8, PixelFormat::RGB8) => {
                self.rgba_to_rgb(frame, target_format)
            }

            // YUV to RGB conversions
            (PixelFormat::YUV420P, PixelFormat::RGBA8) => {
                self.yuv420p_to_rgba(frame, target_format)
            }
            (PixelFormat::YUV422P, PixelFormat::RGBA8) => {
                self.yuv422p_to_rgba(frame, target_format)
            }
            (PixelFormat::UYVY, PixelFormat::RGBA8) => {
                self.uyvy_to_rgba(frame, target_format)
            }
            (PixelFormat::NV12, PixelFormat::RGBA8) => {
                self.nv12_to_rgba(frame, target_format)
            }

            // Unsupported conversions
            _ => Err(IoError::UnsupportedPixelFormat(format!(
                "Conversion from {} to {} not supported",
                frame.format.pixel_format, target_format.pixel_format
            ))),
        }
    }

    /// Converts RGBA to BGRA by swapping R and B channels.
    fn rgba_to_bgra(&self, frame: &VideoFrame, target_format: &VideoFormat) -> Result<VideoFrame> {
        let mut output = Vec::with_capacity(frame.data.len());

        for pixel in frame.data.chunks_exact(4) {
            output.push(pixel[2]); // B
            output.push(pixel[1]); // G
            output.push(pixel[0]); // R
            output.push(pixel[3]); // A
        }

        Ok(VideoFrame::with_metadata(
            output,
            target_format.clone(),
            frame.timestamp,
            frame.metadata.clone(),
        ))
    }

    /// Converts BGRA to RGBA by swapping R and B channels.
    fn bgra_to_rgba(&self, frame: &VideoFrame, target_format: &VideoFormat) -> Result<VideoFrame> {
        // Same as RGBA to BGRA
        self.rgba_to_bgra(frame, target_format)
    }

    /// Converts RGB to RGBA by adding an alpha channel.
    fn rgb_to_rgba(&self, frame: &VideoFrame, target_format: &VideoFormat) -> Result<VideoFrame> {
        let pixel_count = (frame.format.width * frame.format.height) as usize;
        let mut output = Vec::with_capacity(pixel_count * 4);

        for pixel in frame.data.chunks_exact(3) {
            output.push(pixel[0]); // R
            output.push(pixel[1]); // G
            output.push(pixel[2]); // B
            output.push(255);      // A (fully opaque)
        }

        Ok(VideoFrame::with_metadata(
            output,
            target_format.clone(),
            frame.timestamp,
            frame.metadata.clone(),
        ))
    }

    /// Converts RGBA to RGB by dropping the alpha channel.
    fn rgba_to_rgb(&self, frame: &VideoFrame, target_format: &VideoFormat) -> Result<VideoFrame> {
        let pixel_count = (frame.format.width * frame.format.height) as usize;
        let mut output = Vec::with_capacity(pixel_count * 3);

        for pixel in frame.data.chunks_exact(4) {
            output.push(pixel[0]); // R
            output.push(pixel[1]); // G
            output.push(pixel[2]); // B
        }

        Ok(VideoFrame::with_metadata(
            output,
            target_format.clone(),
            frame.timestamp,
            frame.metadata.clone(),
        ))
    }

    /// Converts YUV420P to RGBA.
    ///
    /// YUV420P is a planar format with full resolution Y plane and
    /// quarter resolution U and V planes.
    fn yuv420p_to_rgba(&self, frame: &VideoFrame, target_format: &VideoFormat) -> Result<VideoFrame> {
        let width = frame.format.width as usize;
        let height = frame.format.height as usize;
        let y_size = width * height;
        let uv_size = y_size / 4;

        if frame.data.len() < y_size + 2 * uv_size {
            return Err(IoError::InvalidFrameData(
                "Insufficient data for YUV420P frame".to_string(),
            ));
        }

        let y_plane = &frame.data[0..y_size];
        let u_plane = &frame.data[y_size..y_size + uv_size];
        let v_plane = &frame.data[y_size + uv_size..y_size + 2 * uv_size];

        let mut output = vec![0u8; width * height * 4];

        for y in 0..height {
            for x in 0..width {
                let y_idx = y * width + x;
                let uv_idx = (y / 2) * (width / 2) + (x / 2);

                let y_val = y_plane[y_idx] as i32;
                let u_val = u_plane[uv_idx] as i32 - 128;
                let v_val = v_plane[uv_idx] as i32 - 128;

                let (r, g, b) = yuv_to_rgb(y_val, u_val, v_val);

                let out_idx = y_idx * 4;
                output[out_idx] = r;
                output[out_idx + 1] = g;
                output[out_idx + 2] = b;
                output[out_idx + 3] = 255; // Alpha
            }
        }

        Ok(VideoFrame::with_metadata(
            output,
            target_format.clone(),
            frame.timestamp,
            frame.metadata.clone(),
        ))
    }

    /// Converts YUV422P to RGBA.
    ///
    /// YUV422P is a planar format with full resolution Y plane and
    /// half resolution U and V planes (horizontally subsampled).
    fn yuv422p_to_rgba(&self, frame: &VideoFrame, target_format: &VideoFormat) -> Result<VideoFrame> {
        let width = frame.format.width as usize;
        let height = frame.format.height as usize;
        let y_size = width * height;
        let uv_size = y_size / 2;

        if frame.data.len() < y_size + 2 * uv_size {
            return Err(IoError::InvalidFrameData(
                "Insufficient data for YUV422P frame".to_string(),
            ));
        }

        let y_plane = &frame.data[0..y_size];
        let u_plane = &frame.data[y_size..y_size + uv_size];
        let v_plane = &frame.data[y_size + uv_size..y_size + 2 * uv_size];

        let mut output = vec![0u8; width * height * 4];

        for y in 0..height {
            for x in 0..width {
                let y_idx = y * width + x;
                let uv_idx = y * (width / 2) + (x / 2);

                let y_val = y_plane[y_idx] as i32;
                let u_val = u_plane[uv_idx] as i32 - 128;
                let v_val = v_plane[uv_idx] as i32 - 128;

                let (r, g, b) = yuv_to_rgb(y_val, u_val, v_val);

                let out_idx = y_idx * 4;
                output[out_idx] = r;
                output[out_idx + 1] = g;
                output[out_idx + 2] = b;
                output[out_idx + 3] = 255; // Alpha
            }
        }

        Ok(VideoFrame::with_metadata(
            output,
            target_format.clone(),
            frame.timestamp,
            frame.metadata.clone(),
        ))
    }

    /// Converts UYVY to RGBA.
    ///
    /// UYVY is a packed YUV 4:2:2 format.
    fn uyvy_to_rgba(&self, frame: &VideoFrame, target_format: &VideoFormat) -> Result<VideoFrame> {
        let width = frame.format.width as usize;
        let height = frame.format.height as usize;
        let expected_size = width * height * 2;

        if frame.data.len() < expected_size {
            return Err(IoError::InvalidFrameData(
                "Insufficient data for UYVY frame".to_string(),
            ));
        }

        let mut output = vec![0u8; width * height * 4];

        for y in 0..height {
            for x in 0..(width / 2) {
                let in_idx = (y * width + x * 2) * 2;
                let out_idx1 = (y * width + x * 2) * 4;
                let out_idx2 = out_idx1 + 4;

                let u = frame.data[in_idx] as i32 - 128;
                let y1 = frame.data[in_idx + 1] as i32;
                let v = frame.data[in_idx + 2] as i32 - 128;
                let y2 = frame.data[in_idx + 3] as i32;

                let (r1, g1, b1) = yuv_to_rgb(y1, u, v);
                let (r2, g2, b2) = yuv_to_rgb(y2, u, v);

                output[out_idx1] = r1;
                output[out_idx1 + 1] = g1;
                output[out_idx1 + 2] = b1;
                output[out_idx1 + 3] = 255;

                output[out_idx2] = r2;
                output[out_idx2 + 1] = g2;
                output[out_idx2 + 2] = b2;
                output[out_idx2 + 3] = 255;
            }
        }

        Ok(VideoFrame::with_metadata(
            output,
            target_format.clone(),
            frame.timestamp,
            frame.metadata.clone(),
        ))
    }

    /// Converts NV12 to RGBA.
    ///
    /// NV12 is a semi-planar format with full resolution Y plane and
    /// quarter resolution interleaved UV plane.
    fn nv12_to_rgba(&self, frame: &VideoFrame, target_format: &VideoFormat) -> Result<VideoFrame> {
        let width = frame.format.width as usize;
        let height = frame.format.height as usize;
        let y_size = width * height;
        let uv_size = y_size / 2;

        if frame.data.len() < y_size + uv_size {
            return Err(IoError::InvalidFrameData(
                "Insufficient data for NV12 frame".to_string(),
            ));
        }

        let y_plane = &frame.data[0..y_size];
        let uv_plane = &frame.data[y_size..y_size + uv_size];

        let mut output = vec![0u8; width * height * 4];

        for y in 0..height {
            for x in 0..width {
                let y_idx = y * width + x;
                let uv_idx = ((y / 2) * (width / 2) + (x / 2)) * 2;

                let y_val = y_plane[y_idx] as i32;
                let u_val = uv_plane[uv_idx] as i32 - 128;
                let v_val = uv_plane[uv_idx + 1] as i32 - 128;

                let (r, g, b) = yuv_to_rgb(y_val, u_val, v_val);

                let out_idx = y_idx * 4;
                output[out_idx] = r;
                output[out_idx + 1] = g;
                output[out_idx + 2] = b;
                output[out_idx + 3] = 255; // Alpha
            }
        }

        Ok(VideoFrame::with_metadata(
            output,
            target_format.clone(),
            frame.timestamp,
            frame.metadata.clone(),
        ))
    }
}

impl Default for FormatConverter {
    fn default() -> Self {
        Self::new()
    }
}

/// Converts YUV color values to RGB.
///
/// Uses the BT.709 color space conversion matrix.
#[inline]
fn yuv_to_rgb(y: i32, u: i32, v: i32) -> (u8, u8, u8) {
    let r = y + (1.402 * v as f32) as i32;
    let g = y - (0.344 * u as f32) as i32 - (0.714 * v as f32) as i32;
    let b = y + (1.772 * u as f32) as i32;

    (
        r.clamp(0, 255) as u8,
        g.clamp(0, 255) as u8,
        b.clamp(0, 255) as u8,
    )
}

#[cfg(test)]
mod tests {
    use super::*;
    use std::time::Duration;

    #[test]
    fn test_rgba_to_bgra() {
        let converter = FormatConverter::new();
        let format = VideoFormat::new(2, 2, PixelFormat::RGBA8, 30.0);
        let target_format = VideoFormat::new(2, 2, PixelFormat::BGRA8, 30.0);

        // Create a simple RGBA frame: one red pixel
        let data = vec![
            255, 0, 0, 255,  // Red pixel
            0, 255, 0, 255,  // Green pixel
            0, 0, 255, 255,  // Blue pixel
            255, 255, 255, 255,  // White pixel
        ];

        let frame = VideoFrame::new(data, format, Duration::ZERO);
        let converted = converter.convert(&frame, &target_format).unwrap();

        // After conversion, R and B should be swapped
        assert_eq!(converted.data[0], 0);   // B (was R)
        assert_eq!(converted.data[1], 0);   // G
        assert_eq!(converted.data[2], 255); // R (was B)
        assert_eq!(converted.data[3], 255); // A
    }

    #[test]
    fn test_rgb_to_rgba() {
        let converter = FormatConverter::new();
        let format = VideoFormat::new(1, 1, PixelFormat::RGB8, 30.0);
        let target_format = VideoFormat::new(1, 1, PixelFormat::RGBA8, 30.0);

        let data = vec![128, 64, 32]; // RGB
        let frame = VideoFrame::new(data, format, Duration::ZERO);
        let converted = converter.convert(&frame, &target_format).unwrap();

        assert_eq!(converted.data, vec![128, 64, 32, 255]); // RGBA with alpha added
    }

    #[test]
    fn test_rgba_to_rgb() {
        let converter = FormatConverter::new();
        let format = VideoFormat::new(1, 1, PixelFormat::RGBA8, 30.0);
        let target_format = VideoFormat::new(1, 1, PixelFormat::RGB8, 30.0);

        let data = vec![128, 64, 32, 200]; // RGBA
        let frame = VideoFrame::new(data, format, Duration::ZERO);
        let converted = converter.convert(&frame, &target_format).unwrap();

        assert_eq!(converted.data, vec![128, 64, 32]); // RGB with alpha dropped
    }

    #[test]
    fn test_same_format_clone() {
        let converter = FormatConverter::new();
        let format = VideoFormat::new(2, 2, PixelFormat::RGBA8, 30.0);
        let data = vec![0u8; format.buffer_size()];

        let frame = VideoFrame::new(data.clone(), format.clone(), Duration::ZERO);
        let converted = converter.convert(&frame, &format).unwrap();

        assert_eq!(converted.data, data);
    }

    #[test]
    fn test_unsupported_conversion() {
        let converter = FormatConverter::new();
        let format = VideoFormat::new(2, 2, PixelFormat::RGBA8, 30.0);
        let target_format = VideoFormat::new(2, 2, PixelFormat::YUV420P, 30.0);
        let data = vec![0u8; format.buffer_size()];

        let frame = VideoFrame::new(data, format, Duration::ZERO);
        let result = converter.convert(&frame, &target_format);

        assert!(result.is_err());
    }

    #[test]
    fn test_yuv_to_rgb() {
        // Test with known YUV values (white)
        let (r, g, b) = yuv_to_rgb(235, 0, 0);
        assert!(r > 200 && g > 200 && b > 200); // Should be close to white

        // Test with black
        let (r, g, b) = yuv_to_rgb(16, 0, 0);
        assert!(r < 50 && g < 50 && b < 50); // Should be close to black
    }
}
