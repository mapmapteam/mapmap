//! Video playback control

use crate::{DecodedFrame, VideoDecoder};
use std::time::Duration;

/// Playback state
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum PlaybackState {
    Playing,
    Paused,
    Stopped,
}

/// Playback direction (Phase 1, Month 5)
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum PlaybackDirection {
    /// Play forward (default)
    Forward,
    /// Play backward (reverse)
    Backward,
}

impl Default for PlaybackDirection {
    fn default() -> Self {
        PlaybackDirection::Forward
    }
}

/// Playback mode (Phase 1, Month 5)
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum PlaybackMode {
    /// Loop - repeat indefinitely (existing behavior)
    Loop,
    /// Ping Pong - bounce forward and backward
    PingPong,
    /// Play Once and Eject - stop and unload after completion
    PlayOnceAndEject,
    /// Play Once and Hold - stop on last frame
    PlayOnceAndHold,
}

impl Default for PlaybackMode {
    fn default() -> Self {
        PlaybackMode::Loop
    }
}

/// Video player with playback control
pub struct VideoPlayer {
    decoder: Box<dyn VideoDecoder>,
    state: PlaybackState,
    current_time: Duration,
    playback_speed: f32,
    /// Legacy looping flag (deprecated - use playback_mode instead)
    looping: bool,
    /// Playback direction - forward or backward (Phase 1, Month 5)
    direction: PlaybackDirection,
    /// Playback mode - loop, ping pong, play once variants (Phase 1, Month 5)
    playback_mode: PlaybackMode,
    last_frame: Option<DecodedFrame>,
    /// Track whether we should eject after completing playback
    should_eject: bool,
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
            direction: PlaybackDirection::default(),
            playback_mode: PlaybackMode::default(),
            last_frame: None,
            should_eject: false,
        }
    }

    /// Update the player (call every frame)
    pub fn update(&mut self, dt: Duration) -> Option<DecodedFrame> {
        if self.state != PlaybackState::Playing {
            return self.last_frame.clone();
        }

        let duration = self.decoder.duration();

        // Advance playback time based on direction
        match self.direction {
            PlaybackDirection::Forward => {
                self.current_time += dt.mul_f32(self.playback_speed);

                // Check if we've reached the end
                if self.current_time >= duration {
                    self.handle_end_of_playback();
                    if self.should_eject {
                        return None; // Eject - return no frame
                    }
                }
            }
            PlaybackDirection::Backward => {
                // Go backward in time
                let delta = dt.mul_f32(self.playback_speed);
                if self.current_time > delta {
                    self.current_time -= delta;
                } else {
                    // Reached the beginning
                    self.current_time = Duration::ZERO;
                    self.handle_beginning_of_playback();
                    if self.should_eject {
                        return None; // Eject - return no frame
                    }
                }
            }
        }

        // For backward playback, we need to seek to current_time each frame
        // For forward playback, just get next frame sequentially
        if self.direction == PlaybackDirection::Backward {
            // Seek to current position for backward playback
            if self.decoder.seek(self.current_time).is_ok() {
                match self.decoder.next_frame() {
                    Ok(frame) => {
                        self.last_frame = Some(frame.clone());
                        return Some(frame);
                    }
                    Err(_) => {
                        return self.last_frame.clone();
                    }
                }
            } else {
                return self.last_frame.clone();
            }
        }

        // Forward playback - get next frame sequentially
        match self.decoder.next_frame() {
            Ok(frame) => {
                self.last_frame = Some(frame.clone());
                Some(frame)
            }
            Err(_) => {
                // End of stream reached - handle according to playback mode
                // The handle_end_of_playback should have already been called
                // when current_time >= duration, but call it again to be safe
                self.handle_end_of_playback();

                if self.should_eject {
                    // Eject mode - return no frame
                    return None;
                }

                // After handling end of playback, try to get a frame
                // This will work for Loop and PingPong modes
                match self.decoder.next_frame() {
                    Ok(frame) => {
                        self.last_frame = Some(frame.clone());
                        Some(frame)
                    }
                    Err(_) => {
                        // Still can't get frame, return last frame
                        self.last_frame.clone()
                    }
                }
            }
        }
    }

    /// Handle reaching the end of playback (forward direction)
    fn handle_end_of_playback(&mut self) {
        match self.playback_mode {
            PlaybackMode::Loop => {
                // Loop back to beginning
                self.seek(Duration::ZERO);
            }
            PlaybackMode::PingPong => {
                // Reverse direction
                self.direction = PlaybackDirection::Backward;
                self.current_time = self.decoder.duration();
            }
            PlaybackMode::PlayOnceAndEject => {
                // Stop and mark for ejection
                self.state = PlaybackState::Stopped;
                self.should_eject = true;
            }
            PlaybackMode::PlayOnceAndHold => {
                // Stop and hold on last frame
                self.state = PlaybackState::Stopped;
                self.current_time = self.decoder.duration();
            }
        }
    }

    /// Handle reaching the beginning of playback (backward direction)
    fn handle_beginning_of_playback(&mut self) {
        match self.playback_mode {
            PlaybackMode::Loop => {
                // Loop to end
                self.seek(self.decoder.duration());
            }
            PlaybackMode::PingPong => {
                // Reverse direction to forward
                self.direction = PlaybackDirection::Forward;
                self.current_time = Duration::ZERO;
            }
            PlaybackMode::PlayOnceAndEject => {
                // Stop and mark for ejection
                self.state = PlaybackState::Stopped;
                self.should_eject = true;
            }
            PlaybackMode::PlayOnceAndHold => {
                // Stop and hold on first frame
                self.state = PlaybackState::Stopped;
                self.current_time = Duration::ZERO;
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

    /// Set playback direction (Phase 1, Month 5)
    pub fn set_direction(&mut self, direction: PlaybackDirection) {
        self.direction = direction;
    }

    /// Get current playback direction
    pub fn direction(&self) -> PlaybackDirection {
        self.direction
    }

    /// Set playback mode (Phase 1, Month 5)
    pub fn set_playback_mode(&mut self, mode: PlaybackMode) {
        self.playback_mode = mode;
        self.should_eject = false; // Reset eject flag when changing mode
    }

    /// Get current playback mode
    pub fn playback_mode(&self) -> PlaybackMode {
        self.playback_mode
    }

    /// Check if the player should eject (for PlayOnceAndEject mode)
    pub fn should_eject(&self) -> bool {
        self.should_eject
    }

    /// Toggle direction between forward and backward
    pub fn toggle_direction(&mut self) {
        self.direction = match self.direction {
            PlaybackDirection::Forward => PlaybackDirection::Backward,
            PlaybackDirection::Backward => PlaybackDirection::Forward,
        };
    }

    /// Reset the eject flag (call after ejecting content)
    pub fn reset_eject(&mut self) {
        self.should_eject = false;
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    use crate::decoder::TestPatternDecoder;

    #[test]
    fn test_player_creation() {
        // Use TestPatternDecoder for testing
        let decoder = TestPatternDecoder::new(1920, 1080, Duration::from_secs(60), 30.0);
        let player = VideoPlayer::new(decoder);

        assert_eq!(player.state(), PlaybackState::Stopped);
        assert_eq!(player.speed(), 1.0);
        assert_eq!(player.is_looping(), false);
        assert_eq!(player.direction(), PlaybackDirection::Forward);
        assert_eq!(player.playback_mode(), PlaybackMode::Loop);
    }

    #[test]
    fn test_player_playback_control() {
        let decoder = TestPatternDecoder::new(1920, 1080, Duration::from_secs(60), 30.0);
        let mut player = VideoPlayer::new(decoder);

        player.play();
        assert_eq!(player.state(), PlaybackState::Playing);

        player.pause();
        assert_eq!(player.state(), PlaybackState::Paused);

        player.stop();
        assert_eq!(player.state(), PlaybackState::Stopped);
    }

    #[test]
    fn test_player_speed_control() {
        let decoder = TestPatternDecoder::new(1920, 1080, Duration::from_secs(60), 30.0);
        let mut player = VideoPlayer::new(decoder);

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

    #[test]
    fn test_playback_direction() {
        let decoder = TestPatternDecoder::new(1920, 1080, Duration::from_secs(60), 30.0);
        let mut player = VideoPlayer::new(decoder);

        assert_eq!(player.direction(), PlaybackDirection::Forward);

        player.set_direction(PlaybackDirection::Backward);
        assert_eq!(player.direction(), PlaybackDirection::Backward);

        player.toggle_direction();
        assert_eq!(player.direction(), PlaybackDirection::Forward);
    }

    #[test]
    fn test_playback_modes() {
        let decoder = TestPatternDecoder::new(1920, 1080, Duration::from_secs(60), 30.0);
        let mut player = VideoPlayer::new(decoder);

        assert_eq!(player.playback_mode(), PlaybackMode::Loop);

        player.set_playback_mode(PlaybackMode::PingPong);
        assert_eq!(player.playback_mode(), PlaybackMode::PingPong);

        player.set_playback_mode(PlaybackMode::PlayOnceAndEject);
        assert_eq!(player.playback_mode(), PlaybackMode::PlayOnceAndEject);
        assert!(!player.should_eject());

        player.set_playback_mode(PlaybackMode::PlayOnceAndHold);
        assert_eq!(player.playback_mode(), PlaybackMode::PlayOnceAndHold);
    }
}
