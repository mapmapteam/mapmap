//! Multi-threaded video decode and upload pipeline
//!
//! This module implements a lock-free pipeline for video decoding and texture upload:
//! - Decode thread: Reads video frames from disk and decodes them
//! - Upload thread: Uploads decoded frames to GPU textures
//! - Render thread: Renders the uploaded textures (runs in main thread)
//!
//! ## Thread-Local Scaler Implementation Plan
//!
//! Currently disabled due to FFmpeg's SwsContext not being thread-safe.
//! To re-enable multi-threading:
//!
//! 1. Modify FFmpegDecoder to create scaler in the thread where decoding happens
//! 2. Remove scaler from struct, create as local variable in next_frame()
//! 3. Or: Use thread_local! macro to cache scaler per thread
//! 4. This makes VideoDecoder Send-safe without performance overhead
//!
//! Benefits:
//! - Zero overhead compared to single-threaded
//! - Clean separation of concerns
//! - Pre-buffering prevents frame stutters
//! - Overlapped decode + GPU upload

use crate::{DecodedFrame, MediaError, Result, VideoDecoder, VideoPlayer};
use crossbeam_channel::{bounded, Receiver, Sender};
use std::sync::atomic::{AtomicBool, Ordering};
use std::sync::Arc;
use std::thread::{self, JoinHandle};
use std::time::Duration;
use tracing::{debug, error, info, warn};

/// Frame pipeline statistics
#[derive(Debug, Clone, Copy, Default)]
pub struct PipelineStats {
    pub decoded_frames: u64,
    pub uploaded_frames: u64,
    pub rendered_frames: u64,
    pub dropped_frames: u64,
    pub decode_time_ms: f64,
    pub upload_time_ms: f64,
}

/// Priority level for pipeline stages
#[derive(Debug, Clone, Copy, PartialEq, Eq, PartialOrd, Ord)]
pub enum Priority {
    Low = 0,
    Normal = 1,
    High = 2,
    Critical = 3,
}

/// Pipeline configuration
#[derive(Debug, Clone)]
pub struct PipelineConfig {
    /// Queue depth (number of frames to buffer)
    pub queue_depth: usize,
    /// Enable frame dropping when queues are full
    pub enable_frame_drop: bool,
    /// Priority for decode thread
    pub decode_priority: Priority,
    /// Priority for upload thread
    pub upload_priority: Priority,
}

impl Default for PipelineConfig {
    fn default() -> Self {
        Self {
            queue_depth: 3, // Triple buffering
            enable_frame_drop: true,
            decode_priority: Priority::Normal,
            upload_priority: Priority::High,
        }
    }
}

/// Frame with metadata for pipeline processing
#[derive(Clone)]
pub struct PipelineFrame {
    pub frame: DecodedFrame,
    pub sequence: u64,
    pub priority: Priority,
}

/// Multi-threaded frame pipeline
pub struct FramePipeline {
    // Decode → Upload channel
    decode_tx: Sender<PipelineFrame>,
    decode_rx: Receiver<PipelineFrame>,

    // Upload → Render channel
    upload_tx: Sender<PipelineFrame>,
    pub upload_rx: Receiver<PipelineFrame>,

    // Control
    running: Arc<AtomicBool>,
    stats: Arc<parking_lot::RwLock<PipelineStats>>,

    // Threads
    decode_thread: Option<JoinHandle<()>>,
    config: PipelineConfig,
}

impl FramePipeline {
    /// Create a new frame pipeline with default configuration
    pub fn new() -> Self {
        Self::with_config(PipelineConfig::default())
    }

    /// Create a new frame pipeline with custom configuration
    pub fn with_config(config: PipelineConfig) -> Self {
        let (decode_tx, decode_rx) = bounded(config.queue_depth);
        let (upload_tx, upload_rx) = bounded(config.queue_depth);

        Self {
            decode_tx,
            decode_rx,
            upload_tx,
            upload_rx,
            running: Arc::new(AtomicBool::new(false)),
            stats: Arc::new(parking_lot::RwLock::new(PipelineStats::default())),
            decode_thread: None,
            config,
        }
    }

    /// Start the decode thread
    pub fn start_decode_thread<D: VideoDecoder + 'static>(
        &mut self,
        mut decoder: D,
        mut player: VideoPlayer,
    ) {
        if self.running.load(Ordering::Relaxed) {
            warn!("Decode thread already running");
            return;
        }

        self.running.store(true, Ordering::Relaxed);

        let decode_tx = self.decode_tx.clone();
        let running = self.running.clone();
        let stats = self.stats.clone();
        let enable_drop = self.config.enable_frame_drop;

        let thread = thread::Builder::new()
            .name("decode-thread".to_string())
            .spawn(move || {
                info!("Decode thread started");
                let mut sequence = 0u64;

                while running.load(Ordering::Relaxed) {
                    let start = std::time::Instant::now();

                    // Update player and get next frame
                    if let Some(frame) = player.update(Duration::from_secs_f64(1.0 / decoder.fps()))
                    {
                        let pipeline_frame = PipelineFrame {
                            frame,
                            sequence,
                            priority: Priority::Normal,
                        };

                        // Try to send frame to upload thread
                        let result = if enable_drop {
                            decode_tx.try_send(pipeline_frame)
                        } else {
                            decode_tx.send(pipeline_frame).map_err(|e| e.into())
                        };

                        match result {
                            Ok(_) => {
                                sequence += 1;
                                let mut stats = stats.write();
                                stats.decoded_frames += 1;
                                stats.decode_time_ms = start.elapsed().as_secs_f64() * 1000.0;
                            }
                            Err(_) => {
                                if enable_drop {
                                    stats.write().dropped_frames += 1;
                                    debug!("Dropped frame {} (queue full)", sequence);
                                }
                            }
                        }
                    }

                    // Throttle to approximately match video FPS
                    let elapsed = start.elapsed();
                    let frame_duration = Duration::from_secs_f64(1.0 / decoder.fps());
                    if elapsed < frame_duration {
                        std::thread::sleep(frame_duration - elapsed);
                    }
                }

                info!("Decode thread stopped");
            })
            .expect("Failed to spawn decode thread");

        self.decode_thread = Some(thread);
    }

    /// Start the upload thread (requires render backend)
    /// Note: In Phase 1, this is a placeholder. Full implementation requires wgpu integration.
    pub fn start_upload_thread(&mut self) {
        let decode_rx = self.decode_rx.clone();
        let upload_tx = self.upload_tx.clone();
        let running = self.running.clone();
        let stats = self.stats.clone();

        thread::Builder::new()
            .name("upload-thread".to_string())
            .spawn(move || {
                info!("Upload thread started");

                while running.load(Ordering::Relaxed) {
                    match decode_rx.recv_timeout(Duration::from_millis(100)) {
                        Ok(pipeline_frame) => {
                            let start = std::time::Instant::now();

                            // TODO: Upload to GPU here (Phase 1 placeholder)
                            // In real implementation:
                            // 1. Get staging buffer from pool
                            // 2. Copy frame data to staging buffer
                            // 3. Record copy command
                            // 4. Submit to GPU queue
                            // 5. Send texture handle to render thread

                            // For now, just forward the frame
                            if upload_tx.send(pipeline_frame).is_ok() {
                                let mut stats = stats.write();
                                stats.uploaded_frames += 1;
                                stats.upload_time_ms = start.elapsed().as_secs_f64() * 1000.0;
                            }
                        }
                        Err(crossbeam_channel::RecvTimeoutError::Timeout) => {
                            // No frame available, continue
                        }
                        Err(crossbeam_channel::RecvTimeoutError::Disconnected) => {
                            info!("Decode channel disconnected");
                            break;
                        }
                    }
                }

                info!("Upload thread stopped");
            })
            .expect("Failed to spawn upload thread");
    }

    /// Stop the pipeline
    pub fn stop(&mut self) {
        info!("Stopping pipeline");
        self.running.store(false, Ordering::Relaxed);

        if let Some(thread) = self.decode_thread.take() {
            thread.join().expect("Failed to join decode thread");
        }
    }

    /// Get pipeline statistics
    pub fn stats(&self) -> PipelineStats {
        *self.stats.read()
    }

    /// Reset statistics
    pub fn reset_stats(&self) {
        *self.stats.write() = PipelineStats::default();
    }
}

impl Drop for FramePipeline {
    fn drop(&mut self) {
        self.stop();
    }
}

/// Frame scheduler for prioritizing frames
pub struct FrameScheduler {
    frames: Vec<PipelineFrame>,
    max_frames: usize,
}

impl FrameScheduler {
    pub fn new(max_frames: usize) -> Self {
        Self {
            frames: Vec::with_capacity(max_frames),
            max_frames,
        }
    }

    /// Add a frame to the scheduler
    pub fn push(&mut self, frame: PipelineFrame) {
        if self.frames.len() >= self.max_frames {
            // Remove lowest priority frame
            if let Some(min_idx) = self
                .frames
                .iter()
                .enumerate()
                .min_by_key(|(_, f)| f.priority)
                .map(|(i, _)| i)
            {
                self.frames.remove(min_idx);
            }
        }

        self.frames.push(frame);
        self.frames.sort_by_key(|f| std::cmp::Reverse(f.priority));
    }

    /// Get the highest priority frame
    pub fn pop(&mut self) -> Option<PipelineFrame> {
        if self.frames.is_empty() {
            None
        } else {
            Some(self.frames.remove(0))
        }
    }

    /// Get number of queued frames
    pub fn len(&self) -> usize {
        self.frames.len()
    }

    /// Check if scheduler is empty
    pub fn is_empty(&self) -> bool {
        self.frames.is_empty()
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    use crate::decoder::{TestPatternDecoder, VideoDecoder};
    use crate::player::VideoPlayer;

    #[test]
    fn test_pipeline_config_default() {
        let config = PipelineConfig::default();
        assert_eq!(config.queue_depth, 3);
        assert!(config.enable_frame_drop);
    }

    #[test]
    fn test_frame_scheduler() {
        let mut scheduler = FrameScheduler::new(3);

        let frame1 = PipelineFrame {
            frame: DecodedFrame {
                data: vec![],
                format: crate::decoder::PixelFormat::RGBA8,
                width: 100,
                height: 100,
                pts: Duration::ZERO,
            },
            sequence: 1,
            priority: Priority::Low,
        };

        let frame2 = PipelineFrame {
            frame: DecodedFrame {
                data: vec![],
                format: crate::decoder::PixelFormat::RGBA8,
                width: 100,
                height: 100,
                pts: Duration::ZERO,
            },
            sequence: 2,
            priority: Priority::High,
        };

        scheduler.push(frame1);
        scheduler.push(frame2);

        assert_eq!(scheduler.len(), 2);

        // Should pop high priority frame first
        let popped = scheduler.pop().unwrap();
        assert_eq!(popped.priority, Priority::High);
    }

    #[test]
    fn test_pipeline_creation() {
        let pipeline = FramePipeline::new();
        let stats = pipeline.stats();

        assert_eq!(stats.decoded_frames, 0);
        assert_eq!(stats.uploaded_frames, 0);
        assert_eq!(stats.rendered_frames, 0);
    }

    #[test]
    fn test_pipeline_with_test_pattern() {
        let mut pipeline = FramePipeline::new();
        let decoder = TestPatternDecoder::new(640, 480, Duration::from_secs(1), 30.0);
        let mut player = VideoPlayer::new(decoder);
        player.set_looping(true);
        player.play();

        // Start decode thread
        let test_decoder = TestPatternDecoder::new(640, 480, Duration::from_secs(1), 30.0);
        pipeline.start_decode_thread(test_decoder, player);
        pipeline.start_upload_thread();

        // Let it run for a bit
        std::thread::sleep(Duration::from_millis(200));

        // Check stats
        let stats = pipeline.stats();
        assert!(stats.decoded_frames > 0, "Should have decoded frames");

        // Stop pipeline
        pipeline.stop();
    }
}
