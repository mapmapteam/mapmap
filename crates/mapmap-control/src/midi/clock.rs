//! MIDI clock synchronization

use super::MidiMessage;
use std::time::{Duration, Instant};
use tracing::{info, debug};

/// MIDI clock synchronization state
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum ClockState {
    Stopped,
    Playing,
    Paused,
}

/// MIDI clock handler for tempo synchronization
pub struct MidiClock {
    state: ClockState,
    tempo_bpm: f32,
    last_clock_time: Option<Instant>,
    clock_count: u32,
    start_time: Option<Instant>,
}

impl MidiClock {
    /// MIDI clock ticks per quarter note
    pub const TICKS_PER_BEAT: u32 = 24;

    pub fn new() -> Self {
        Self {
            state: ClockState::Stopped,
            tempo_bpm: 120.0,
            last_clock_time: None,
            clock_count: 0,
            start_time: None,
        }
    }

    /// Process a MIDI clock message
    pub fn process_message(&mut self, message: MidiMessage) {
        match message {
            MidiMessage::Start => {
                info!("MIDI Clock: Start");
                self.state = ClockState::Playing;
                self.clock_count = 0;
                self.start_time = Some(Instant::now());
                self.last_clock_time = None;
            }
            MidiMessage::Stop => {
                info!("MIDI Clock: Stop");
                self.state = ClockState::Stopped;
                self.clock_count = 0;
                self.start_time = None;
                self.last_clock_time = None;
            }
            MidiMessage::Continue => {
                info!("MIDI Clock: Continue");
                self.state = ClockState::Playing;
            }
            MidiMessage::Clock => {
                if self.state == ClockState::Playing {
                    self.process_clock_tick();
                }
            }
            _ => {}
        }
    }

    /// Process a clock tick
    fn process_clock_tick(&mut self) {
        let now = Instant::now();

        if let Some(last_time) = self.last_clock_time {
            let delta = now.duration_since(last_time);

            // Calculate tempo from clock interval
            // 24 ticks per beat, so interval between ticks is 1/24 of a beat
            let beat_duration = delta.as_secs_f32() * Self::TICKS_PER_BEAT as f32;
            let bpm = 60.0 / beat_duration;

            // Smooth tempo changes with exponential moving average
            const SMOOTHING: f32 = 0.9;
            self.tempo_bpm = self.tempo_bpm * SMOOTHING + bpm * (1.0 - SMOOTHING);

            debug!("MIDI Clock: BPM = {:.2}", self.tempo_bpm);
        }

        self.last_clock_time = Some(now);
        self.clock_count += 1;
    }

    /// Get current tempo in BPM
    pub fn get_tempo_bpm(&self) -> f32 {
        self.tempo_bpm
    }

    /// Get current state
    pub fn get_state(&self) -> ClockState {
        self.state
    }

    /// Get current beat position (0-based)
    pub fn get_beat_position(&self) -> f32 {
        self.clock_count as f32 / Self::TICKS_PER_BEAT as f32
    }

    /// Get time since start
    pub fn get_elapsed_time(&self) -> Option<Duration> {
        self.start_time.map(|t| Instant::now().duration_since(t))
    }

    /// Get current phase within a beat (0.0-1.0)
    pub fn get_beat_phase(&self) -> f32 {
        (self.clock_count % Self::TICKS_PER_BEAT) as f32 / Self::TICKS_PER_BEAT as f32
    }

    /// Reset clock
    pub fn reset(&mut self) {
        self.clock_count = 0;
        self.start_time = None;
        self.last_clock_time = None;
    }
}

impl Default for MidiClock {
    fn default() -> Self {
        Self::new()
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    use std::thread;
    use std::time::Duration;

    #[test]
    fn test_clock_state() {
        let mut clock = MidiClock::new();
        assert_eq!(clock.get_state(), ClockState::Stopped);

        clock.process_message(MidiMessage::Start);
        assert_eq!(clock.get_state(), ClockState::Playing);

        clock.process_message(MidiMessage::Stop);
        assert_eq!(clock.get_state(), ClockState::Stopped);

        clock.process_message(MidiMessage::Continue);
        assert_eq!(clock.get_state(), ClockState::Playing);
    }

    #[test]
    fn test_beat_position() {
        let mut clock = MidiClock::new();
        clock.process_message(MidiMessage::Start);

        assert_eq!(clock.get_beat_position(), 0.0);

        for _ in 0..24 {
            clock.process_message(MidiMessage::Clock);
        }

        assert_eq!(clock.get_beat_position(), 1.0);
    }

    #[test]
    fn test_beat_phase() {
        let mut clock = MidiClock::new();
        clock.process_message(MidiMessage::Start);

        assert_eq!(clock.get_beat_phase(), 0.0);

        for _ in 0..12 {
            clock.process_message(MidiMessage::Clock);
        }

        assert_eq!(clock.get_beat_phase(), 0.5);
    }
}
