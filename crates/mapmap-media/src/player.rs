//! Video playback control

use crate::{DecodedFrame, Result, VideoDecoder};
use std::time::Duration;

/// Playback state
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum PlaybackState {
    Playing,
    Paused,
    Stopped,
}

/// Video player with playback control
pub struct VideoPlayer {
    decoder: Box<dyn VideoDecoder>,
    state: PlaybackState,
    current_time: Duration,
    playback_speed: f32,
    looping: bool,
    last_frame: Option<DecodedFrame>,
}

impl VideoPlayer {
    /// Create a new video player with a decoder
    pub fn new(decoder: impl VideoDecoder + 'static) -> Self {
        Self {
            decoder: Box::new(decoder),
            state: PlaybackState::Stopped,
            current_time: Duration::ZERO,
            playback_speed: 1.0,
            looping: false,
            last_frame: None,
        }
    }

    /// Update the player (call every frame)
    pub fn update(&mut self, dt: Duration) -> Option<DecodedFrame> {
        if self.state != PlaybackState::Playing {
            return self.last_frame.clone();
        }

        // Advance playback time
        self.current_time += dt.mul_f32(self.playback_speed);

        // Check if we've reached the end
        if self.current_time >= self.decoder.duration() {
            if self.looping {
                self.seek(Duration::ZERO);
            } else {
                self.state = PlaybackState::Stopped;
                return self.last_frame.clone();
            }
        }

        // Try to get the next frame
        match self.decoder.next_frame() {
            Ok(frame) => {
                self.last_frame = Some(frame.clone());
                Some(frame)
            }
            Err(_) => {
                if self.looping {
                    self.seek(Duration::ZERO);
                    self.decoder.next_frame().ok()
                } else {
                    self.state = PlaybackState::Stopped;
                    self.last_frame.clone()
                }
            }
        }
    }

    /// Start or resume playback
    pub fn play(&mut self) {
        self.state = PlaybackState::Playing;
    }

    /// Pause playback
    pub fn pause(&mut self) {
        self.state = PlaybackState::Paused;
    }

    /// Stop playback and reset to beginning
    pub fn stop(&mut self) {
        self.state = PlaybackState::Stopped;
        self.seek(Duration::ZERO);
    }

    /// Seek to a specific timestamp
    pub fn seek(&mut self, timestamp: Duration) {
        if self.decoder.seek(timestamp).is_ok() {
            self.current_time = timestamp;
        }
    }

    /// Set playback speed (1.0 = normal, 0.5 = half speed, 2.0 = double speed)
    pub fn set_speed(&mut self, speed: f32) {
        self.playback_speed = speed.max(0.0).min(10.0);
    }

    /// Enable or disable looping
    pub fn set_looping(&mut self, looping: bool) {
        self.looping = looping;
    }

    /// Get current playback state
    pub fn state(&self) -> PlaybackState {
        self.state
    }

    /// Get current playback time
    pub fn current_time(&self) -> Duration {
        self.current_time
    }

    /// Get total duration
    pub fn duration(&self) -> Duration {
        self.decoder.duration()
    }

    /// Get playback speed
    pub fn speed(&self) -> f32 {
        self.playback_speed
    }

    /// Check if looping is enabled
    pub fn is_looping(&self) -> bool {
        self.looping
    }

    /// Get video resolution
    pub fn resolution(&self) -> (u32, u32) {
        self.decoder.resolution()
    }

    /// Get FPS
    pub fn fps(&self) -> f64 {
        self.decoder.fps()
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    use crate::decoder::{DecodedFrame, FFmpegDecoder, PixelFormat};
    use std::path::PathBuf;

    #[test]
    fn test_player_creation() {
        // Create a mock decoder (requires a dummy file path)
        // In a real test, we'd use a mock decoder
        let player = VideoPlayer {
            decoder: Box::new(FFmpegDecoder {
                width: 1920,
                height: 1080,
                duration: Duration::from_secs(60),
                fps: 30.0,
                current_time: Duration::ZERO,
                frame_count: 0,
            }),
            state: PlaybackState::Stopped,
            current_time: Duration::ZERO,
            playback_speed: 1.0,
            looping: false,
            last_frame: None,
        };

        assert_eq!(player.state(), PlaybackState::Stopped);
        assert_eq!(player.speed(), 1.0);
        assert_eq!(player.is_looping(), false);
    }

    #[test]
    fn test_player_playback_control() {
        let mut player = VideoPlayer {
            decoder: Box::new(FFmpegDecoder {
                width: 1920,
                height: 1080,
                duration: Duration::from_secs(60),
                fps: 30.0,
                current_time: Duration::ZERO,
                frame_count: 0,
            }),
            state: PlaybackState::Stopped,
            current_time: Duration::ZERO,
            playback_speed: 1.0,
            looping: false,
            last_frame: None,
        };

        player.play();
        assert_eq!(player.state(), PlaybackState::Playing);

        player.pause();
        assert_eq!(player.state(), PlaybackState::Paused);

        player.stop();
        assert_eq!(player.state(), PlaybackState::Stopped);
    }

    #[test]
    fn test_player_speed_control() {
        let mut player = VideoPlayer {
            decoder: Box::new(FFmpegDecoder {
                width: 1920,
                height: 1080,
                duration: Duration::from_secs(60),
                fps: 30.0,
                current_time: Duration::ZERO,
                frame_count: 0,
            }),
            state: PlaybackState::Stopped,
            current_time: Duration::ZERO,
            playback_speed: 1.0,
            looping: false,
            last_frame: None,
        };

        player.set_speed(2.0);
        assert_eq!(player.speed(), 2.0);

        player.set_speed(0.5);
        assert_eq!(player.speed(), 0.5);

        // Test clamping
        player.set_speed(20.0);
        assert_eq!(player.speed(), 10.0);

        player.set_speed(-1.0);
        assert_eq!(player.speed(), 0.0);
    }
}
