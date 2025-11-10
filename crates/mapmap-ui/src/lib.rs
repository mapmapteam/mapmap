//! MapMap UI - ImGui Integration
//!
//! This crate provides the user interface layer using ImGui, including:
//! - ImGui context setup
//! - Window management
//! - Control panels

use imgui::*;
use imgui_wgpu::{Renderer, RendererConfig};
use imgui_winit_support::{HiDpiMode, WinitPlatform};
use std::time::Instant;

pub struct ImGuiContext {
    pub imgui: Context,
    pub platform: WinitPlatform,
    pub renderer: Renderer,
    last_frame: Instant,
}

impl ImGuiContext {
    /// Create a new ImGui context
    pub fn new(
        window: &winit::window::Window,
        device: &wgpu::Device,
        queue: &wgpu::Queue,
        surface_format: wgpu::TextureFormat,
    ) -> Self {
        let mut imgui = Context::create();
        imgui.set_ini_filename(None);

        let mut platform = WinitPlatform::init(&mut imgui);
        platform.attach_window(imgui.io_mut(), window, HiDpiMode::Default);

        // Setup fonts
        imgui.io_mut().font_global_scale = 1.0;

        // Create renderer
        let renderer_config = RendererConfig {
            texture_format: surface_format,
            ..Default::default()
        };

        let renderer = Renderer::new(&mut imgui, device, queue, renderer_config);

        Self {
            imgui,
            platform,
            renderer,
            last_frame: Instant::now(),
        }
    }

    /// Prepare frame for rendering
    pub fn prepare_frame(&mut self, window: &winit::window::Window) {
        let now = Instant::now();
        self.imgui.io_mut().update_delta_time(now - self.last_frame);
        self.last_frame = now;

        self.platform
            .prepare_frame(self.imgui.io_mut(), window)
            .expect("Failed to prepare frame");
    }

    /// Begin new frame
    pub fn begin_frame(&mut self) -> &mut Ui {
        self.imgui.frame()
    }

    /// Render ImGui
    pub fn render(
        &mut self,
        window: &winit::window::Window,
        device: &wgpu::Device,
        queue: &wgpu::Queue,
        encoder: &mut wgpu::CommandEncoder,
        view: &wgpu::TextureView,
    ) {
        // Prepare draw data
        self.platform.prepare_render(self.imgui.io_mut(), window);
        let draw_data = self.imgui.render();

        // Render
        self.renderer
            .render(draw_data, queue, device, encoder, view)
            .expect("Failed to render ImGui");
    }

    /// Handle window events
    pub fn handle_event<T>(
        &mut self,
        window: &winit::window::Window,
        event: &winit::event::Event<T>,
    ) {
        self.platform
            .handle_event(self.imgui.io_mut(), window, event);
    }
}

/// UI state for the application
pub struct AppUI {
    pub show_controls: bool,
    pub show_stats: bool,
    pub playback_speed: f32,
    pub looping: bool,
}

impl Default for AppUI {
    fn default() -> Self {
        Self {
            show_controls: true,
            show_stats: true,
            playback_speed: 1.0,
            looping: true,
        }
    }
}

impl AppUI {
    /// Render the control panel
    pub fn render_controls(&mut self, ui: &Ui) {
        if !self.show_controls {
            return;
        }

        ui.window("Playback Controls")
            .size([300.0, 200.0], Condition::FirstUseEver)
            .build(|| {
                ui.text("Video Playback");
                ui.separator();

                ui.slider("Speed", 0.1, 2.0, &mut self.playback_speed);
                ui.checkbox("Loop", &mut self.looping);

                ui.separator();

                if ui.button("Play") {
                    // Handled by main app
                }
                ui.same_line();
                if ui.button("Pause") {
                    // Handled by main app
                }
                ui.same_line();
                if ui.button("Stop") {
                    // Handled by main app
                }
            });
    }

    /// Render performance stats
    pub fn render_stats(&mut self, ui: &Ui, fps: f32, frame_time_ms: f32) {
        if !self.show_stats {
            return;
        }

        ui.window("Performance")
            .size([250.0, 120.0], Condition::FirstUseEver)
            .position([10.0, 10.0], Condition::FirstUseEver)
            .build(|| {
                ui.text(format!("FPS: {:.1}", fps));
                ui.text(format!("Frame Time: {:.2} ms", frame_time_ms));
                ui.separator();
                ui.text("MapMap Phase 0 Demo");
            });
    }

    /// Render main menu bar
    pub fn render_menu_bar(&mut self, ui: &Ui) {
        ui.main_menu_bar(|| {
            ui.menu("File", || {
                if ui.menu_item("Load Video") {
                    // Handled by main app
                }
                ui.separator();
                if ui.menu_item("Exit") {
                    // Handled by main app
                }
            });

            ui.menu("View", || {
                ui.checkbox("Show Controls", &mut self.show_controls);
                ui.checkbox("Show Stats", &mut self.show_stats);
            });

            ui.menu("Help", || {
                if ui.menu_item("About") {
                    // Show about dialog
                }
            });
        });
    }
}
