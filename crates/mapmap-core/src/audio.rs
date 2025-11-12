//! Audio Analysis Module
//!
//! Phase 3: Audio-reactive Effects
//! Provides FFT analysis, beat detection, and audio-reactive parameter mapping

use crossbeam_channel::{Receiver, Sender, unbounded};
use rustfft::{Fft, FftPlanner, num_complex::Complex};
use serde::{Deserialize, Serialize};
use std::sync::Arc;
use std::collections::VecDeque;

/// Audio analysis configuration
#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct AudioConfig {
    /// Sample rate (e.g., 44100, 48000)
    pub sample_rate: u32,

    /// FFT window size (power of 2, e.g., 512, 1024, 2048)
    pub fft_size: usize,

    /// Overlap factor (0.0-1.0, typically 0.5)
    pub overlap: f32,

    /// Smoothing factor for FFT results (0.0-1.0)
    pub smoothing: f32,
}

impl Default for AudioConfig {
    fn default() -> Self {
        Self {
            sample_rate: 44100,
            fft_size: 1024,
            overlap: 0.5,
            smoothing: 0.8,
        }
    }
}

/// Audio frequency bands for analysis
#[derive(Debug, Clone, Copy, Serialize, Deserialize)]
pub enum FrequencyBand {
    /// Sub-bass (20-60 Hz)
    SubBass,
    /// Bass (60-250 Hz)
    Bass,
    /// Low midrange (250-500 Hz)
    LowMid,
    /// Midrange (500-2000 Hz)
    Mid,
    /// High midrange (2000-4000 Hz)
    HighMid,
    /// Presence (4000-6000 Hz)
    Presence,
    /// Brilliance (6000-20000 Hz)
    Brilliance,
}

impl FrequencyBand {
    /// Get the frequency range for this band
    pub fn frequency_range(&self) -> (f32, f32) {
        match self {
            FrequencyBand::SubBass => (20.0, 60.0),
            FrequencyBand::Bass => (60.0, 250.0),
            FrequencyBand::LowMid => (250.0, 500.0),
            FrequencyBand::Mid => (500.0, 2000.0),
            FrequencyBand::HighMid => (2000.0, 4000.0),
            FrequencyBand::Presence => (4000.0, 6000.0),
            FrequencyBand::Brilliance => (6000.0, 20000.0),
        }
    }

    /// Get all frequency bands
    pub fn all() -> Vec<FrequencyBand> {
        vec![
            FrequencyBand::SubBass,
            FrequencyBand::Bass,
            FrequencyBand::LowMid,
            FrequencyBand::Mid,
            FrequencyBand::HighMid,
            FrequencyBand::Presence,
            FrequencyBand::Brilliance,
        ]
    }
}

/// Audio analysis results
#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct AudioAnalysis {
    /// Current timestamp
    pub timestamp: f64,

    /// FFT magnitudes (frequency domain)
    pub fft_magnitudes: Vec<f32>,

    /// Frequency band energies
    pub band_energies: [f32; 7],

    /// Overall RMS volume (0.0-1.0)
    pub rms_volume: f32,

    /// Peak volume (0.0-1.0)
    pub peak_volume: f32,

    /// Beat detected (kick drum)
    pub beat_detected: bool,

    /// Beat strength (0.0-1.0)
    pub beat_strength: f32,

    /// Onset detected (sudden volume increase)
    pub onset_detected: bool,

    /// Tempo (BPM) estimate
    pub tempo_bpm: Option<f32>,
}

impl Default for AudioAnalysis {
    fn default() -> Self {
        Self {
            timestamp: 0.0,
            fft_magnitudes: vec![0.0; 512],
            band_energies: [0.0; 7],
            rms_volume: 0.0,
            peak_volume: 0.0,
            beat_detected: false,
            beat_strength: 0.0,
            onset_detected: false,
            tempo_bpm: None,
        }
    }
}

/// Audio analyzer - performs FFT and beat detection
pub struct AudioAnalyzer {
    config: AudioConfig,
    fft: Arc<dyn Fft<f32>>,

    // FFT buffers
    input_buffer: VecDeque<f32>,
    fft_buffer: Vec<Complex<f32>>,
    magnitude_buffer: Vec<f32>,
    previous_magnitudes: Vec<f32>,

    // Beat detection
    beat_history: VecDeque<f32>,
    energy_history: VecDeque<f32>,
    last_beat_time: f64,

    // Tempo tracking
    beat_intervals: VecDeque<f64>,

    // Communication
    analysis_sender: Sender<AudioAnalysis>,
    analysis_receiver: Receiver<AudioAnalysis>,

    // Time tracking
    current_time: f64,
}

impl AudioAnalyzer {
    /// Create a new audio analyzer
    pub fn new(config: AudioConfig) -> Self {
        let mut planner = FftPlanner::new();
        let fft = planner.plan_fft_forward(config.fft_size);

        let (tx, rx) = unbounded();

        Self {
            config: config.clone(),
            fft,
            input_buffer: VecDeque::with_capacity(config.fft_size * 2),
            fft_buffer: vec![Complex::new(0.0, 0.0); config.fft_size],
            magnitude_buffer: vec![0.0; config.fft_size / 2],
            previous_magnitudes: vec![0.0; config.fft_size / 2],
            beat_history: VecDeque::with_capacity(100),
            energy_history: VecDeque::with_capacity(100),
            last_beat_time: 0.0,
            beat_intervals: VecDeque::with_capacity(10),
            analysis_sender: tx,
            analysis_receiver: rx,
            current_time: 0.0,
        }
    }

    /// Process audio samples
    pub fn process_samples(&mut self, samples: &[f32], timestamp: f64) -> AudioAnalysis {
        self.current_time = timestamp;

        // Add samples to input buffer
        for &sample in samples {
            self.input_buffer.push_back(sample);
        }

        // Check if we have enough samples for FFT
        let hop_size = (self.config.fft_size as f32 * (1.0 - self.config.overlap)) as usize;
        if self.input_buffer.len() < self.config.fft_size {
            return self.get_latest_analysis();
        }

        // Perform FFT
        self.perform_fft();

        // Calculate analysis metrics
        let analysis = self.calculate_analysis();

        // Send analysis to receiver channel
        let _ = self.analysis_sender.send(analysis.clone());

        // Remove processed samples
        for _ in 0..hop_size {
            self.input_buffer.pop_front();
        }

        analysis
    }

    /// Perform FFT on current buffer
    fn perform_fft(&mut self) {
        // Apply Hann window and copy to FFT buffer
        for i in 0..self.config.fft_size {
            let sample = self.input_buffer[i];
            let window = 0.5 * (1.0 - (2.0 * std::f32::consts::PI * i as f32 / (self.config.fft_size - 1) as f32).cos());
            self.fft_buffer[i] = Complex::new(sample * window, 0.0);
        }

        // Perform FFT
        self.fft.process(&mut self.fft_buffer);

        // Calculate magnitudes (only first half due to symmetry)
        let half_size = self.config.fft_size / 2;
        for i in 0..half_size {
            let magnitude = self.fft_buffer[i].norm() / self.config.fft_size as f32;

            // Apply smoothing
            let smoothed = self.config.smoothing * self.previous_magnitudes[i]
                         + (1.0 - self.config.smoothing) * magnitude;

            self.magnitude_buffer[i] = smoothed;
            self.previous_magnitudes[i] = smoothed;
        }
    }

    /// Calculate audio analysis from FFT results
    fn calculate_analysis(&mut self) -> AudioAnalysis {
        let _half_size = self.config.fft_size / 2;

        // Calculate RMS volume
        let rms_volume = self.calculate_rms();

        // Calculate peak volume
        let peak_volume = self.magnitude_buffer.iter()
            .copied()
            .fold(0.0f32, |a, b| a.max(b));

        // Calculate frequency band energies
        let band_energies = self.calculate_band_energies();

        // Detect beats
        let (beat_detected, beat_strength) = self.detect_beat(&band_energies);

        // Detect onsets
        let onset_detected = self.detect_onset();

        // Estimate tempo
        let tempo_bpm = self.estimate_tempo();

        AudioAnalysis {
            timestamp: self.current_time,
            fft_magnitudes: self.magnitude_buffer.clone(),
            band_energies,
            rms_volume,
            peak_volume,
            beat_detected,
            beat_strength,
            onset_detected,
            tempo_bpm,
        }
    }

    /// Calculate RMS (Root Mean Square) volume
    fn calculate_rms(&self) -> f32 {
        let sum: f32 = self.input_buffer.iter()
            .take(self.config.fft_size)
            .map(|&s| s * s)
            .sum();

        (sum / self.config.fft_size as f32).sqrt()
    }

    /// Calculate energy in each frequency band
    fn calculate_band_energies(&self) -> [f32; 7] {
        let mut energies = [0.0f32; 7];

        let bands = FrequencyBand::all();
        let bin_width = self.config.sample_rate as f32 / self.config.fft_size as f32;

        for (i, band) in bands.iter().enumerate() {
            let (min_freq, max_freq) = band.frequency_range();
            let min_bin = (min_freq / bin_width) as usize;
            let max_bin = ((max_freq / bin_width) as usize).min(self.magnitude_buffer.len() - 1);

            let energy: f32 = self.magnitude_buffer[min_bin..=max_bin].iter().sum();
            energies[i] = energy / (max_bin - min_bin + 1) as f32;
        }

        energies
    }

    /// Detect beat (kick drum) using energy threshold
    fn detect_beat(&mut self, band_energies: &[f32; 7]) -> (bool, f32) {
        // Focus on bass frequencies for beat detection
        let bass_energy = band_energies[0] + band_energies[1]; // SubBass + Bass

        self.energy_history.push_back(bass_energy);
        if self.energy_history.len() > 43 {  // ~1 second at 43Hz update rate
            self.energy_history.pop_front();
        }

        // Calculate average energy
        let avg_energy: f32 = self.energy_history.iter().sum::<f32>()
                            / self.energy_history.len() as f32;

        // Beat detection threshold
        let threshold = avg_energy * 1.5;
        let beat_detected = bass_energy > threshold
                         && (self.current_time - self.last_beat_time) > 0.1; // Min 100ms between beats

        let beat_strength = if beat_detected {
            ((bass_energy - threshold) / threshold).min(1.0)
        } else {
            0.0
        };

        if beat_detected {
            // Track beat interval for tempo estimation
            if self.last_beat_time > 0.0 {
                let interval = self.current_time - self.last_beat_time;
                self.beat_intervals.push_back(interval);
                if self.beat_intervals.len() > 10 {
                    self.beat_intervals.pop_front();
                }
            }
            self.last_beat_time = self.current_time;
        }

        (beat_detected, beat_strength)
    }

    /// Detect onset (sudden increase in energy across all frequencies)
    fn detect_onset(&mut self) -> bool {
        let total_energy: f32 = self.magnitude_buffer.iter().sum();

        self.beat_history.push_back(total_energy);
        if self.beat_history.len() > 5 {
            self.beat_history.pop_front();
        }

        if self.beat_history.len() < 5 {
            return false;
        }

        let avg: f32 = self.beat_history.iter().take(4).sum::<f32>() / 4.0;
        let current = self.beat_history.back().unwrap();

        current > &(avg * 1.8)
    }

    /// Estimate tempo in BPM from beat intervals
    fn estimate_tempo(&self) -> Option<f32> {
        if self.beat_intervals.len() < 4 {
            return None;
        }

        let avg_interval: f64 = self.beat_intervals.iter().sum::<f64>()
                               / self.beat_intervals.len() as f64;

        if avg_interval > 0.0 {
            Some((60.0 / avg_interval) as f32)
        } else {
            None
        }
    }

    /// Get the latest analysis result
    pub fn get_latest_analysis(&self) -> AudioAnalysis {
        self.analysis_receiver.try_recv()
            .unwrap_or_default()
    }

    /// Get analysis receiver for async updates
    pub fn analysis_receiver(&self) -> Receiver<AudioAnalysis> {
        self.analysis_receiver.clone()
    }
}

/// Audio input source type
#[derive(Debug, Clone, Copy, PartialEq, Eq, Serialize, Deserialize)]
pub enum AudioSource {
    /// System audio input (microphone/line-in)
    SystemInput,
    /// Audio from video file
    VideoAudio,
    /// External audio file
    AudioFile,
}

/// Audio reactive parameter mapping
#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct AudioReactiveMapping {
    /// Parameter name to control
    pub parameter_name: String,

    /// Audio source to use
    pub source: AudioSource,

    /// Type of audio data to map
    pub mapping_type: AudioMappingType,

    /// Frequency band (if applicable)
    pub frequency_band: Option<FrequencyBand>,

    /// Minimum output value
    pub output_min: f32,

    /// Maximum output value
    pub output_max: f32,

    /// Smoothing factor (0.0-1.0)
    pub smoothing: f32,

    /// Attack time (seconds) - how fast to respond to increases
    pub attack: f32,

    /// Release time (seconds) - how fast to respond to decreases
    pub release: f32,
}

/// Type of audio data to map to parameters
#[derive(Debug, Clone, Copy, PartialEq, Eq, Serialize, Deserialize)]
pub enum AudioMappingType {
    /// Overall RMS volume
    Volume,
    /// Peak volume
    Peak,
    /// Specific frequency band energy
    BandEnergy,
    /// Beat detection trigger
    Beat,
    /// Beat strength
    BeatStrength,
    /// Onset detection
    Onset,
    /// Tempo (BPM)
    Tempo,
    /// Specific FFT bin
    FFTBin(usize),
}

impl AudioReactiveMapping {
    /// Apply the mapping to audio analysis
    pub fn apply(&self, analysis: &AudioAnalysis, previous_value: f32, delta_time: f32) -> f32 {
        // Get raw value from audio analysis
        let raw_value = match self.mapping_type {
            AudioMappingType::Volume => analysis.rms_volume,
            AudioMappingType::Peak => analysis.peak_volume,
            AudioMappingType::BandEnergy => {
                if let Some(band) = self.frequency_band {
                    let index = match band {
                        FrequencyBand::SubBass => 0,
                        FrequencyBand::Bass => 1,
                        FrequencyBand::LowMid => 2,
                        FrequencyBand::Mid => 3,
                        FrequencyBand::HighMid => 4,
                        FrequencyBand::Presence => 5,
                        FrequencyBand::Brilliance => 6,
                    };
                    analysis.band_energies[index]
                } else {
                    0.0
                }
            }
            AudioMappingType::Beat => if analysis.beat_detected { 1.0 } else { 0.0 },
            AudioMappingType::BeatStrength => analysis.beat_strength,
            AudioMappingType::Onset => if analysis.onset_detected { 1.0 } else { 0.0 },
            AudioMappingType::Tempo => analysis.tempo_bpm.unwrap_or(0.0) / 200.0, // Normalize to 0-1 (assuming 200 BPM max)
            AudioMappingType::FFTBin(bin) => {
                analysis.fft_magnitudes.get(bin).copied().unwrap_or(0.0)
            }
        };

        // Apply attack/release envelope
        let target_value = if raw_value > previous_value {
            // Attack
            let t = (delta_time / self.attack).min(1.0);
            previous_value + (raw_value - previous_value) * t
        } else {
            // Release
            let t = (delta_time / self.release).min(1.0);
            previous_value + (raw_value - previous_value) * t
        };

        // Map to output range
        let normalized = target_value.clamp(0.0, 1.0);
        self.output_min + normalized * (self.output_max - self.output_min)
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_audio_analyzer_creation() {
        let config = AudioConfig::default();
        let analyzer = AudioAnalyzer::new(config);
        assert_eq!(analyzer.magnitude_buffer.len(), 512);
    }

    #[test]
    fn test_frequency_bands() {
        let bass = FrequencyBand::Bass;
        let (min, max) = bass.frequency_range();
        assert_eq!(min, 60.0);
        assert_eq!(max, 250.0);
    }

    #[test]
    fn test_audio_reactive_mapping() {
        let mapping = AudioReactiveMapping {
            parameter_name: "opacity".to_string(),
            source: AudioSource::SystemInput,
            mapping_type: AudioMappingType::Volume,
            frequency_band: None,
            output_min: 0.0,
            output_max: 1.0,
            smoothing: 0.5,
            attack: 0.1,
            release: 0.3,
        };

        let mut analysis = AudioAnalysis::default();
        analysis.rms_volume = 0.5;

        let value = mapping.apply(&analysis, 0.0, 0.016);
        assert!(value > 0.0 && value <= 1.0);
    }
}
