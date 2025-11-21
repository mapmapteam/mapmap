// Oscillator Distortion Effect Demo
//
// This example demonstrates how to integrate the oscillator distortion effect
// into a MapMap rendering pipeline.

use mapmap_core::{OscillatorConfig, PhaseInitMode, ColorMode, SimulationResolution};
use mapmap_render::{OscillatorRenderer, WgpuBackend};
use std::sync::Arc;

/// Example: Basic oscillator effect setup
pub fn example_basic_setup(
    device: Arc<wgpu::Device>,
    queue: Arc<wgpu::Queue>,
) -> Result<OscillatorRenderer, Box<dyn std::error::Error>> {

    // Create a default configuration
    let config = OscillatorConfig::default();

    // Create the oscillator renderer
    let mut oscillator = OscillatorRenderer::new(
        device,
        queue,
        wgpu::TextureFormat::Bgra8Unorm,
        &config,
    )?;

    // Initialize with random phases
    oscillator.initialize_phases(PhaseInitMode::Random);

    Ok(oscillator)
}

/// Example: Using presets
pub fn example_presets() {
    // Subtle organic motion
    let subtle = OscillatorConfig::preset_subtle();

    // Dramatic swirling
    let dramatic = OscillatorConfig::preset_dramatic();

    // Ring patterns
    let rings = OscillatorConfig::preset_rings();

    println!("Subtle preset: {:#?}", subtle);
    println!("Dramatic preset: {:#?}", dramatic);
    println!("Rings preset: {:#?}", rings);
}

/// Example: Custom configuration
pub fn example_custom_config() -> OscillatorConfig {
    let mut config = OscillatorConfig::new();

    // Set simulation parameters
    config.simulation_resolution = SimulationResolution::Medium;
    config.kernel_radius = 20.0;

    // Configure coupling rings
    config.rings[0].distance = 0.25;
    config.rings[0].width = 0.15;
    config.rings[0].coupling = 2.0;  // Strong sync

    config.rings[1].distance = 0.5;
    config.rings[1].width = 0.2;
    config.rings[1].coupling = -1.0; // Anti-sync

    // Set frequency range
    config.frequency_min = 0.5;
    config.frequency_max = 2.0;
    config.noise_amount = 0.2;

    // Configure distortion
    config.distortion_amount = 0.6;
    config.distortion_scale = 0.03;
    config.distortion_speed = 1.5;

    // Add color overlay
    config.overlay_opacity = 0.3;
    config.color_mode = ColorMode::Rainbow;

    config
}

/// Example: Render loop integration
pub fn example_render_loop(
    oscillator: &mut OscillatorRenderer,
    device: &wgpu::Device,
    queue: &wgpu::Queue,
    input_view: &wgpu::TextureView,
    output_view: &wgpu::TextureView,
    config: &OscillatorConfig,
) {
    // Calculate delta time (assuming 60 fps)
    let delta_time = 1.0 / 60.0;

    // Update oscillator simulation
    oscillator.update(delta_time, config);

    // Create command encoder
    let mut encoder = device.create_command_encoder(&wgpu::CommandEncoderDescriptor {
        label: Some("Oscillator Render"),
    });

    // Render distortion effect
    oscillator.render(
        &mut encoder,
        input_view,
        output_view,
        1920,  // width
        1080,  // height
        config,
    );

    // Submit commands
    queue.submit(std::iter::once(encoder.finish()));
}

/// Example: Dynamic parameter animation
pub struct AnimatedOscillator {
    config: OscillatorConfig,
    time: f32,
}

impl AnimatedOscillator {
    pub fn new() -> Self {
        Self {
            config: OscillatorConfig::preset_subtle(),
            time: 0.0,
        }
    }

    /// Update parameters over time
    pub fn update(&mut self, delta_time: f32) {
        self.time += delta_time;

        // Animate distortion amount with a sine wave
        let base_amount = 0.5;
        let variation = 0.3;
        self.config.distortion_amount = base_amount +
            variation * (self.time * 0.5).sin();

        // Animate coupling strength
        let base_coupling = 1.0;
        let coupling_var = 0.5;
        self.config.rings[0].coupling = base_coupling +
            coupling_var * (self.time * 0.3).cos();

        // Animate overlay opacity
        self.config.overlay_opacity = 0.5 +
            0.5 * (self.time * 0.2).sin().abs();
    }

    pub fn config(&self) -> &OscillatorConfig {
        &self.config
    }
}

/// Example: Multiple effect layers
pub struct MultiLayerOscillator {
    layers: Vec<(OscillatorRenderer, OscillatorConfig)>,
}

impl MultiLayerOscillator {
    pub fn new(
        device: Arc<wgpu::Device>,
        queue: Arc<wgpu::Queue>,
        num_layers: usize,
    ) -> Result<Self, Box<dyn std::error::Error>> {
        let mut layers = Vec::new();

        for i in 0..num_layers {
            let mut config = OscillatorConfig::default();

            // Vary parameters per layer
            config.frequency_min = 0.5 + i as f32 * 0.5;
            config.frequency_max = 1.0 + i as f32 * 1.0;
            config.distortion_scale = 0.01 * (i + 1) as f32;

            let renderer = OscillatorRenderer::new(
                device.clone(),
                queue.clone(),
                wgpu::TextureFormat::Bgra8Unorm,
                &config,
            )?;

            layers.push((renderer, config));
        }

        Ok(Self { layers })
    }

    pub fn render_all(
        &mut self,
        device: &wgpu::Device,
        queue: &wgpu::Queue,
        input_view: &wgpu::TextureView,
        output_view: &wgpu::TextureView,
        width: u32,
        height: u32,
        delta_time: f32,
    ) {
        // Update all layers
        for (renderer, config) in &mut self.layers {
            renderer.update(delta_time, config);
        }

        // Render in sequence (would need intermediate textures in real impl)
        let mut encoder = device.create_command_encoder(&Default::default());

        for (renderer, config) in &mut self.layers {
            renderer.render(
                &mut encoder,
                input_view,
                output_view,
                width,
                height,
                config,
            );
        }

        queue.submit(Some(encoder.finish()));
    }
}

/// Example: Phase initialization patterns
pub fn example_phase_patterns(oscillator: &mut OscillatorRenderer) {
    // Different patterns create different effects

    // Random: Chaotic, organic motion
    oscillator.initialize_phases(PhaseInitMode::Random);

    // Uniform: All oscillators start synchronized
    oscillator.initialize_phases(PhaseInitMode::Uniform);

    // Horizontal plane wave
    oscillator.initialize_phases(PhaseInitMode::PlaneHorizontal);

    // Vertical plane wave
    oscillator.initialize_phases(PhaseInitMode::PlaneVertical);

    // Diagonal plane wave
    oscillator.initialize_phases(PhaseInitMode::PlaneDiagonal);
}

/// Example: Interactive controls
pub struct OscillatorControls {
    config: OscillatorConfig,
}

impl OscillatorControls {
    pub fn new() -> Self {
        Self {
            config: OscillatorConfig::default(),
        }
    }

    /// Handle UI input (pseudo-code for ImGui integration)
    pub fn draw_ui(&mut self, ui: &imgui::Ui) {
        ui.text("Oscillator Distortion Effect");

        // Simulation parameters
        if let Some(_token) = ui.collapsing_header("Simulation", imgui::TreeNodeFlags::empty()) {
            ui.slider("Kernel Radius", 1.0, 64.0, &mut self.config.kernel_radius);
            ui.slider("Frequency Min", 0.0, 4.0, &mut self.config.frequency_min);
            ui.slider("Frequency Max", 0.0, 4.0, &mut self.config.frequency_max);
            ui.slider("Noise Amount", 0.0, 1.0, &mut self.config.noise_amount);
        }

        // Distortion parameters
        if let Some(_token) = ui.collapsing_header("Distortion", imgui::TreeNodeFlags::empty()) {
            ui.slider("Amount", 0.0, 1.0, &mut self.config.distortion_amount);
            ui.slider("Scale", 0.001, 0.1, &mut self.config.distortion_scale);
            ui.slider("Speed", 0.0, 2.0, &mut self.config.distortion_speed);
        }

        // Visual parameters
        if let Some(_token) = ui.collapsing_header("Visual", imgui::TreeNodeFlags::empty()) {
            ui.slider("Overlay Opacity", 0.0, 1.0, &mut self.config.overlay_opacity);

            // Color mode combo box
            let color_modes = ["Off", "Rainbow", "Black & White", "Complementary"];
            let mut current_mode = self.config.color_mode.to_u32() as usize;
            if ui.combo("Color Mode", &mut current_mode, &color_modes, 4) {
                self.config.color_mode = match current_mode {
                    0 => ColorMode::Off,
                    1 => ColorMode::Rainbow,
                    2 => ColorMode::BlackWhite,
                    3 => ColorMode::Complementary,
                    _ => ColorMode::Off,
                };
            }
        }

        // Ring parameters
        if let Some(_token) = ui.collapsing_header("Rings", imgui::TreeNodeFlags::empty()) {
            for i in 0..4 {
                ui.text(format!("Ring {}", i));
                ui.slider(
                    format!("Distance##ring{}", i),
                    0.0,
                    1.0,
                    &mut self.config.rings[i].distance,
                );
                ui.slider(
                    format!("Width##ring{}", i),
                    0.0,
                    1.0,
                    &mut self.config.rings[i].width,
                );
                ui.slider(
                    format!("Coupling##ring{}", i),
                    -2.0,
                    2.0,
                    &mut self.config.rings[i].coupling,
                );
                ui.separator();
            }
        }

        // Presets
        if let Some(_token) = ui.collapsing_header("Presets", imgui::TreeNodeFlags::empty()) {
            if ui.button("Subtle") {
                self.config = OscillatorConfig::preset_subtle();
            }
            ui.same_line();
            if ui.button("Dramatic") {
                self.config = OscillatorConfig::preset_dramatic();
            }
            ui.same_line();
            if ui.button("Rings") {
                self.config = OscillatorConfig::preset_rings();
            }
        }
    }

    pub fn config(&self) -> &OscillatorConfig {
        &self.config
    }
}

fn main() {
    println!("Oscillator Distortion Effect Examples");
    println!("======================================\n");

    // Show presets
    println!("Available Presets:");
    example_presets();

    // Show custom config
    println!("\nCustom Configuration Example:");
    let custom = example_custom_config();
    println!("{:#?}", custom);
}
