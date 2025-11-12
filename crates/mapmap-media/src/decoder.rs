//! Video decoder abstraction with FFmpeg implementation

use crate::{MediaError, Result};
use std::path::Path;
use std::time::Duration;
use tracing::{info, warn};

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

    /// Convert to RGBA8 format (with YUV420P conversion)
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
                // Simple YUV420P to RGBA conversion (BT.601)
                yuv420p_to_rgba(&self.data, self.width, self.height)
            }
        }
    }
}

/// Convert YUV420P to RGBA using BT.601 color space
fn yuv420p_to_rgba(yuv_data: &[u8], width: u32, height: u32) -> Vec<u8> {
    let size = (width * height) as usize;
    let y_plane = &yuv_data[0..size];
    let u_plane = &yuv_data[size..size + size / 4];
    let v_plane = &yuv_data[size + size / 4..size + size / 2];

    let mut rgba = vec![0u8; size * 4];

    for y in 0..height {
        for x in 0..width {
            let y_idx = (y * width + x) as usize;
            let uv_idx = ((y / 2) * (width / 2) + (x / 2)) as usize;

            let y_val = y_plane[y_idx] as i32;
            let u_val = u_plane[uv_idx] as i32 - 128;
            let v_val = v_plane[uv_idx] as i32 - 128;

            // BT.601 conversion
            let r = (y_val + (1.402 * v_val as f32) as i32).clamp(0, 255) as u8;
            let g = (y_val - (0.344 * u_val as f32) as i32 - (0.714 * v_val as f32) as i32)
                .clamp(0, 255) as u8;
            let b = (y_val + (1.772 * u_val as f32) as i32).clamp(0, 255) as u8;

            let rgba_idx = y_idx * 4;
            rgba[rgba_idx] = r;
            rgba[rgba_idx + 1] = g;
            rgba[rgba_idx + 2] = b;
            rgba[rgba_idx + 3] = 255;
        }
    }

    rgba
}

/// Video decoder trait
///
/// Note: VideoDecoder does not require Send because FFmpeg's scaler context
/// is not thread-safe. Decoders should be used on a single thread or wrapped
/// in appropriate synchronization primitives.
pub trait VideoDecoder {
    fn next_frame(&mut self) -> Result<DecodedFrame>;
    fn seek(&mut self, timestamp: Duration) -> Result<()>;
    fn duration(&self) -> Duration;
    fn resolution(&self) -> (u32, u32);
    fn fps(&self) -> f64;
}

/// Hardware acceleration type
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum HwAccelType {
    None,
    #[cfg(target_os = "linux")]
    VAAPI,
    #[cfg(target_os = "macos")]
    VideoToolbox,
    #[cfg(target_os = "windows")]
    DXVA2,
    #[cfg(target_os = "windows")]
    D3D11VA,
}

// ============================================================================
// FFmpeg Implementation (when feature is enabled)
// ============================================================================

#[cfg(feature = "ffmpeg")]
mod ffmpeg_impl {
    use super::*;
    use ffmpeg_next as ffmpeg;

    pub struct RealFFmpegDecoder {
        input_ctx: ffmpeg::format::context::Input,
        decoder: ffmpeg::codec::decoder::Video,
        // NOTE: Scaler is not thread-safe (SwsContext is not Send)
        // For multi-threading: Move scaler creation to next_frame() or use thread_local!
        scaler: ffmpeg::software::scaling::Context,
        video_stream_idx: usize,
        time_base: ffmpeg::Rational,
        duration: Duration,
        fps: f64,
        width: u32,
        height: u32,
        _hw_accel: HwAccelType,
    }

    impl RealFFmpegDecoder {
        /// Open a video file with optional hardware acceleration
        pub fn open<P: AsRef<Path>>(path: P, hw_accel: HwAccelType) -> Result<Self> {
            let path = path.as_ref();

            if !path.exists() {
                return Err(MediaError::FileOpen(format!(
                    "File not found: {}",
                    path.display()
                )));
            }

            // Initialize FFmpeg
            ffmpeg::init().map_err(|e| MediaError::DecoderError(e.to_string()))?;

            // Open input file
            let input_ctx = ffmpeg::format::input(&path)
                .map_err(|e| MediaError::FileOpen(e.to_string()))?;

            // Find best video stream
            let video_stream = input_ctx
                .streams()
                .best(ffmpeg::media::Type::Video)
                .ok_or(MediaError::NoVideoStream)?;

            let video_stream_idx = video_stream.index();
            let time_base = video_stream.time_base();

            // Get stream parameters
            let codec_params = video_stream.parameters();

            // Calculate FPS
            let fps = video_stream.avg_frame_rate();
            let fps_value = fps.numerator() as f64 / fps.denominator() as f64;

            // Calculate duration
            let duration_secs = video_stream.duration() as f64 * f64::from(time_base);
            let duration = Duration::from_secs_f64(duration_secs);

            // Create decoder context
            let mut decoder = ffmpeg::codec::Context::from_parameters(codec_params)
                .map_err(|e| MediaError::DecoderError(e.to_string()))?
                .decoder()
                .video()
                .map_err(|e| MediaError::DecoderError(e.to_string()))?;

            // Setup hardware acceleration if requested
            let actual_hw_accel = Self::setup_hw_accel(&mut decoder, hw_accel)?;

            // Get dimensions from decoder
            let width = decoder.width();
            let height = decoder.height();

            // Create scaler to convert to RGBA
            let scaler = ffmpeg::software::scaling::Context::get(
                decoder.format(),
                width,
                height,
                ffmpeg::format::Pixel::RGBA,
                width,
                height,
                ffmpeg::software::scaling::Flags::BILINEAR,
            )
            .map_err(|e| MediaError::DecoderError(e.to_string()))?;

            info!(
                "Decoder initialized successfully: {}x{} @ {:.2} fps, duration: {:.2}s, hw_accel: {:?}",
                width,
                height,
                fps_value,
                duration_secs,
                actual_hw_accel
            );

            Ok(Self {
                input_ctx,
                decoder,
                scaler,
                video_stream_idx,
                time_base,
                duration,
                fps: fps_value,
                width,
                height,
                _hw_accel: actual_hw_accel,
            })
        }

        /// Setup hardware acceleration
        fn setup_hw_accel(
            _decoder: &mut ffmpeg::codec::decoder::Video,
            requested: HwAccelType,
        ) -> Result<HwAccelType> {
            // Note: Hardware acceleration setup via ffmpeg-next is platform-specific
            // and requires additional configuration. For Phase 1, we document the
            // approach but return None as full implementation requires platform testing
            match requested {
                HwAccelType::None => Ok(HwAccelType::None),
                _ => {
                    warn!(
                        "Hardware acceleration {:?} requested but not yet fully implemented",
                        requested
                    );
                    // TODO: Implement platform-specific hw accel setup
                    Ok(HwAccelType::None)
                }
            }
        }
    }

    impl super::VideoDecoder for RealFFmpegDecoder {
        fn next_frame(&mut self) -> Result<DecodedFrame> {
            for (stream, packet) in self.input_ctx.packets() {
                if stream.index() != self.video_stream_idx {
                    continue;
                }

                self.decoder
                    .send_packet(&packet)
                    .map_err(|e| MediaError::DecoderError(e.to_string()))?;

                let mut decoded = ffmpeg::util::frame::Video::empty();

                if self.decoder.receive_frame(&mut decoded).is_ok() {
                    // Scale to RGBA
                    let mut rgb_frame = ffmpeg::util::frame::Video::empty();
                    self.scaler
                        .run(&decoded, &mut rgb_frame)
                        .map_err(|e| MediaError::DecoderError(e.to_string()))?;

                    let pts = Duration::from_secs_f64(
                        decoded.timestamp().unwrap_or(0) as f64 * f64::from(self.time_base),
                    );

                    return Ok(DecodedFrame {
                        data: rgb_frame.data(0).to_vec(),
                        format: PixelFormat::RGBA8,
                        width: self.width,
                        height: self.height,
                        pts,
                    });
                }
            }

            Err(MediaError::EndOfStream)
        }

        fn seek(&mut self, timestamp: Duration) -> Result<()> {
            let timestamp_ts = (timestamp.as_secs_f64() / f64::from(self.time_base)) as i64;

            self.input_ctx
                .seek(timestamp_ts, ..)
                .map_err(|e| MediaError::SeekError(e.to_string()))?;

            // Flush decoder buffers
            self.decoder.flush();

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
}

// ============================================================================
// Test Pattern Fallback (always available)
// ============================================================================

/// Test pattern decoder (fallback when FFmpeg is not available)
pub struct TestPatternDecoder {
    width: u32,
    height: u32,
    duration: Duration,
    fps: f64,
    current_time: Duration,
    frame_count: u64,
}

impl TestPatternDecoder {
    /// Create a new test pattern decoder
    pub fn new(width: u32, height: u32, duration: Duration, fps: f64) -> Self {
        Self {
            width,
            height,
            duration,
            fps,
            current_time: Duration::ZERO,
            frame_count: 0,
        }
    }

    /// Generate a test pattern frame
    fn generate_test_frame(&self) -> DecodedFrame {
        let size = (self.width * self.height * 4) as usize;
        let mut data = vec![0u8; size];

        // Generate animated gradient pattern
        let time_offset = (self.frame_count % 255) as u8;

        for y in 0..self.height {
            for x in 0..self.width {
                let idx = ((y * self.width + x) * 4) as usize;
                data[idx] = ((x * 255 / self.width) as u8).wrapping_add(time_offset);
                data[idx + 1] = ((y * 255 / self.height) as u8).wrapping_add(time_offset);
                data[idx + 2] = 128;
                data[idx + 3] = 255;
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

impl VideoDecoder for TestPatternDecoder {
    fn next_frame(&mut self) -> Result<DecodedFrame> {
        if self.current_time >= self.duration {
            return Err(MediaError::EndOfStream);
        }

        let frame = self.generate_test_frame();

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

// ============================================================================
// Public API
// ============================================================================

/// Unified decoder that automatically uses FFmpeg if available, test pattern otherwise
pub enum FFmpegDecoder {
    #[cfg(feature = "ffmpeg")]
    Real(ffmpeg_impl::RealFFmpegDecoder),
    TestPattern(TestPatternDecoder),
    StillImage(crate::image_decoder::StillImageDecoder),
    Gif(crate::image_decoder::GifDecoder),
    ImageSequence(crate::image_decoder::ImageSequenceDecoder),
}

impl FFmpegDecoder {
    /// Open a video file (uses FFmpeg if feature is enabled, test pattern otherwise)
    pub fn open<P: AsRef<Path>>(path: P) -> Result<Self> {
        Self::open_with_hw_accel(path, HwAccelType::None)
    }

    /// Open a video file with hardware acceleration
    pub fn open_with_hw_accel<P: AsRef<Path>>(
        path: P,
        #[allow(unused_variables)] hw_accel: HwAccelType,
    ) -> Result<Self> {
        let path_ref = path.as_ref();

        // Try image decoders first based on file extension
        if crate::image_decoder::StillImageDecoder::supports_format(path_ref) {
            info!("Detected still image format, using StillImageDecoder");
            match crate::image_decoder::StillImageDecoder::open(path_ref) {
                Ok(decoder) => return Ok(FFmpegDecoder::StillImage(decoder)),
                Err(e) => {
                    warn!("Still image decoder failed: {}, trying video decoder", e);
                }
            }
        }

        if crate::image_decoder::GifDecoder::supports_format(path_ref) {
            info!("Detected GIF format, using GifDecoder");
            match crate::image_decoder::GifDecoder::open(path_ref) {
                Ok(decoder) => return Ok(FFmpegDecoder::Gif(decoder)),
                Err(e) => {
                    warn!("GIF decoder failed: {}, trying video decoder", e);
                }
            }
        }

        // If it's a directory, try image sequence decoder
        if path_ref.is_dir() {
            info!("Detected directory, trying ImageSequenceDecoder");
            // Default to 30 fps for image sequences
            match crate::image_decoder::ImageSequenceDecoder::open(path_ref, 30.0) {
                Ok(decoder) => return Ok(FFmpegDecoder::ImageSequence(decoder)),
                Err(e) => {
                    warn!("Image sequence decoder failed: {}", e);
                }
            }
        }

        // Fall back to video decoder (FFmpeg or test pattern)
        #[cfg(feature = "ffmpeg")]
        {
            match ffmpeg_impl::RealFFmpegDecoder::open(path_ref, hw_accel) {
                Ok(decoder) => Ok(FFmpegDecoder::Real(decoder)),
                Err(e) => {
                    warn!("FFmpeg decoder failed: {}, using test pattern", e);
                    Ok(FFmpegDecoder::TestPattern(TestPatternDecoder::new(
                        1920,
                        1080,
                        Duration::from_secs(60),
                        30.0,
                    )))
                }
            }
        }

        #[cfg(not(feature = "ffmpeg"))]
        {
            info!("FFmpeg feature not enabled, using test pattern");
            Ok(FFmpegDecoder::TestPattern(TestPatternDecoder::new(
                1920,
                1080,
                Duration::from_secs(60),
                30.0,
            )))
        }
    }

    /// Open an image sequence with a custom frame rate
    pub fn open_image_sequence<P: AsRef<Path>>(directory: P, fps: f64) -> Result<Self> {
        crate::image_decoder::ImageSequenceDecoder::open(directory, fps)
            .map(FFmpegDecoder::ImageSequence)
    }

    /// Detect and use best available hardware acceleration
    pub fn open_with_auto_hw_accel<P: AsRef<Path>>(path: P) -> Result<Self> {
        #[cfg(target_os = "linux")]
        let hw_accel = HwAccelType::VAAPI;

        #[cfg(target_os = "macos")]
        let hw_accel = HwAccelType::VideoToolbox;

        #[cfg(target_os = "windows")]
        let hw_accel = HwAccelType::D3D11VA;

        #[cfg(not(any(target_os = "linux", target_os = "macos", target_os = "windows")))]
        let hw_accel = HwAccelType::None;

        Self::open_with_hw_accel(path, hw_accel)
    }
}

impl VideoDecoder for FFmpegDecoder {
    fn next_frame(&mut self) -> Result<DecodedFrame> {
        match self {
            #[cfg(feature = "ffmpeg")]
            FFmpegDecoder::Real(decoder) => decoder.next_frame(),
            FFmpegDecoder::TestPattern(decoder) => decoder.next_frame(),
            FFmpegDecoder::StillImage(decoder) => decoder.next_frame(),
            FFmpegDecoder::Gif(decoder) => decoder.next_frame(),
            FFmpegDecoder::ImageSequence(decoder) => decoder.next_frame(),
        }
    }

    fn seek(&mut self, timestamp: Duration) -> Result<()> {
        match self {
            #[cfg(feature = "ffmpeg")]
            FFmpegDecoder::Real(decoder) => decoder.seek(timestamp),
            FFmpegDecoder::TestPattern(decoder) => decoder.seek(timestamp),
            FFmpegDecoder::StillImage(decoder) => decoder.seek(timestamp),
            FFmpegDecoder::Gif(decoder) => decoder.seek(timestamp),
            FFmpegDecoder::ImageSequence(decoder) => decoder.seek(timestamp),
        }
    }

    fn duration(&self) -> Duration {
        match self {
            #[cfg(feature = "ffmpeg")]
            FFmpegDecoder::Real(decoder) => decoder.duration(),
            FFmpegDecoder::TestPattern(decoder) => decoder.duration(),
            FFmpegDecoder::StillImage(decoder) => decoder.duration(),
            FFmpegDecoder::Gif(decoder) => decoder.duration(),
            FFmpegDecoder::ImageSequence(decoder) => decoder.duration(),
        }
    }

    fn resolution(&self) -> (u32, u32) {
        match self {
            #[cfg(feature = "ffmpeg")]
            FFmpegDecoder::Real(decoder) => decoder.resolution(),
            FFmpegDecoder::TestPattern(decoder) => decoder.resolution(),
            FFmpegDecoder::StillImage(decoder) => decoder.resolution(),
            FFmpegDecoder::Gif(decoder) => decoder.resolution(),
            FFmpegDecoder::ImageSequence(decoder) => decoder.resolution(),
        }
    }

    fn fps(&self) -> f64 {
        match self {
            #[cfg(feature = "ffmpeg")]
            FFmpegDecoder::Real(decoder) => decoder.fps(),
            FFmpegDecoder::TestPattern(decoder) => decoder.fps(),
            FFmpegDecoder::StillImage(decoder) => decoder.fps(),
            FFmpegDecoder::Gif(decoder) => decoder.fps(),
            FFmpegDecoder::ImageSequence(decoder) => decoder.fps(),
        }
    }
}

#[cfg(test)]
mod tests {
    use super::*;

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
    fn test_pixel_format_conversion_rgba() {
        let frame = DecodedFrame {
            data: vec![255, 0, 0, 255],
            format: PixelFormat::RGBA8,
            width: 1,
            height: 1,
            pts: Duration::ZERO,
        };

        let rgba = frame.to_rgba();
        assert_eq!(rgba, vec![255, 0, 0, 255]);
    }

    #[test]
    fn test_pixel_format_conversion_bgra() {
        let frame = DecodedFrame {
            data: vec![0, 0, 255, 255], // Blue in BGRA
            format: PixelFormat::BGRA8,
            width: 1,
            height: 1,
            pts: Duration::ZERO,
        };

        let rgba = frame.to_rgba();
        assert_eq!(rgba, vec![255, 0, 0, 255]); // Red in RGBA
    }

    #[test]
    fn test_test_pattern_decoder() {
        let mut decoder = TestPatternDecoder::new(640, 480, Duration::from_secs(10), 30.0);

        assert_eq!(decoder.resolution(), (640, 480));
        assert_eq!(decoder.fps(), 30.0);
        assert_eq!(decoder.duration(), Duration::from_secs(10));

        let frame = decoder.next_frame().unwrap();
        assert_eq!(frame.width, 640);
        assert_eq!(frame.height, 480);
        assert_eq!(frame.format, PixelFormat::RGBA8);
    }

    #[test]
    fn test_test_pattern_seek() {
        let mut decoder = TestPatternDecoder::new(640, 480, Duration::from_secs(10), 30.0);

        decoder.seek(Duration::from_secs(5)).unwrap();
        assert_eq!(decoder.current_time, Duration::from_secs(5));

        // Seeking beyond duration should error
        assert!(decoder.seek(Duration::from_secs(15)).is_err());
    }

    #[test]
    fn test_yuv420p_conversion() {
        // Create a simple 2x2 YUV420P frame (white pixel)
        let mut yuv_data = vec![0u8; 6]; // 4 Y + 1 U + 1 V
        yuv_data[0..4].fill(255); // Y plane (white)
        yuv_data[4] = 128; // U
        yuv_data[5] = 128; // V

        let rgba = yuv420p_to_rgba(&yuv_data, 2, 2);

        // All pixels should be white (or close to white due to color space conversion)
        for chunk in rgba.chunks(4) {
            assert!(chunk[0] > 200); // R
            assert!(chunk[1] > 200); // G
            assert!(chunk[2] > 200); // B
            assert_eq!(chunk[3], 255); // A
        }
    }
}
