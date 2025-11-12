//! MapMap UI - ImGui Integration
//!
//! This crate provides the user interface layer using ImGui, including:
//! - ImGui context setup
//! - Window management
//! - Control panels

// Phase 3: Effects Pipeline UI
pub mod shader_graph_editor;
pub mod timeline;

pub use shader_graph_editor::{ShaderGraphEditor, ShaderGraphAction};
pub use timeline::{TimelineEditor, TimelineAction};

use imgui::*;
use imgui_wgpu::{Renderer, RendererConfig};
use imgui_winit_support::{HiDpiMode, WinitPlatform};
use std::time::Instant;

/// UI actions that can be triggered by the user interface
#[derive(Debug, Clone)]
pub enum UIAction {
    // Playback actions
    Play,
    Pause,
    Stop,
    SetSpeed(f32),
    ToggleLoop(bool),
    // Phase 1: Advanced playback
    SetPlaybackDirection(mapmap_media::PlaybackDirection),
    TogglePlaybackDirection,
    SetPlaybackMode(mapmap_media::PlaybackMode),

    // File actions
    LoadVideo(String),
    SaveProject(String),
    LoadProject(String),
    Exit,

    // Mapping actions
    AddMapping,
    RemoveMapping(u64),
    ToggleMappingVisibility(u64, bool),
    SelectMapping(u64),

    // Paint actions
    AddPaint,
    RemovePaint(u64),

    // Layer actions (Phase 1)
    AddLayer,
    RemoveLayer(u64),
    DuplicateLayer(u64),
    RenameLayer(u64, String),
    ToggleLayerBypass(u64),
    ToggleLayerSolo(u64),
    SetLayerOpacity(u64, f32),
    EjectAllLayers,

    // Transform actions (Phase 1)
    SetLayerTransform(u64, mapmap_core::Transform),
    ApplyResizeMode(u64, mapmap_core::ResizeMode),

    // Master controls (Phase 1)
    SetMasterOpacity(f32),
    SetMasterSpeed(f32),
    SetCompositionName(String),

    // Phase 2: Output management
    AddOutput(String, mapmap_core::CanvasRegion, (u32, u32)),
    RemoveOutput(u64),
    ConfigureOutput(u64, mapmap_core::OutputConfig),
    SetOutputEdgeBlend(u64, mapmap_core::EdgeBlendConfig),
    SetOutputColorCalibration(u64, mapmap_core::ColorCalibration),
    CreateProjectorArray2x2((u32, u32), f32),

    // View actions
    ToggleFullscreen,
}

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

    /// Render ImGui with a closure for building UI
    pub fn render<F>(
        &mut self,
        window: &winit::window::Window,
        device: &wgpu::Device,
        queue: &wgpu::Queue,
        encoder: &mut wgpu::CommandEncoder,
        view: &wgpu::TextureView,
        build_ui: F,
    ) where
        F: FnOnce(&mut Ui),
    {
        // Update delta time
        let now = Instant::now();
        self.imgui.io_mut().update_delta_time(now - self.last_frame);
        self.last_frame = now;

        // Prepare frame
        self.platform
            .prepare_frame(self.imgui.io_mut(), window)
            .expect("Failed to prepare frame");

        // Begin frame and build UI
        let ui = self.imgui.frame();
        build_ui(ui);

        // End frame and prepare for rendering
        self.platform.prepare_render(ui, window);
        let draw_data = self.imgui.render();

        // Create render pass for ImGui
        let mut render_pass = encoder.begin_render_pass(&wgpu::RenderPassDescriptor {
            label: Some("ImGui Render Pass"),
            color_attachments: &[Some(wgpu::RenderPassColorAttachment {
                view,
                resolve_target: None,
                ops: wgpu::Operations {
                    load: wgpu::LoadOp::Load,
                    store: true,
                },
            })],
            depth_stencil_attachment: None,
        });

        // Render
        self.renderer
            .render(draw_data, queue, device, &mut render_pass)
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
    pub show_layers: bool,
    pub show_paints: bool,
    pub show_mappings: bool,
    pub show_transforms: bool,      // Phase 1
    pub show_master_controls: bool, // Phase 1
    pub show_outputs: bool,          // Phase 2
    pub show_edge_blend: bool,       // Phase 2
    pub show_color_calibration: bool, // Phase 2
    pub playback_speed: f32,
    pub looping: bool,
    // Phase 1: Advanced playback state
    pub playback_direction: mapmap_media::PlaybackDirection,
    pub playback_mode: mapmap_media::PlaybackMode,
    // Phase 1: Transform editing state
    pub selected_layer_id: Option<u64>,
    // Phase 2: Output configuration state
    pub selected_output_id: Option<u64>,
    pub actions: Vec<UIAction>,
}

impl Default for AppUI {
    fn default() -> Self {
        Self {
            show_controls: true,
            show_stats: true,
            show_layers: true,
            show_paints: true,
            show_mappings: true,
            show_transforms: true,
            show_master_controls: true,
            show_outputs: true,
            show_edge_blend: false,  // Show only when output selected
            show_color_calibration: false,  // Show only when output selected
            playback_speed: 1.0,
            looping: true,
            playback_direction: mapmap_media::PlaybackDirection::Forward,
            playback_mode: mapmap_media::PlaybackMode::Loop,
            selected_layer_id: None,
            selected_output_id: None,
            actions: Vec::new(),
        }
    }
}

impl AppUI {
    /// Take all pending actions and clear the list
    pub fn take_actions(&mut self) -> Vec<UIAction> {
        std::mem::take(&mut self.actions)
    }

    /// Render the control panel
    pub fn render_controls(&mut self, ui: &Ui) {
        if !self.show_controls {
            return;
        }

        ui.window("Playback Controls")
            .size([320.0, 360.0], Condition::FirstUseEver)
            .build(|| {
                ui.text("Video Playback");
                ui.separator();

                // Transport controls
                if ui.button("Play") {
                    self.actions.push(UIAction::Play);
                }
                ui.same_line();
                if ui.button("Pause") {
                    self.actions.push(UIAction::Pause);
                }
                ui.same_line();
                if ui.button("Stop") {
                    self.actions.push(UIAction::Stop);
                }

                ui.separator();

                // Speed control
                let old_speed = self.playback_speed;
                ui.slider("Speed", 0.1, 2.0, &mut self.playback_speed);
                if (self.playback_speed - old_speed).abs() > 0.001 {
                    self.actions.push(UIAction::SetSpeed(self.playback_speed));
                }

                // Legacy loop control
                let old_looping = self.looping;
                ui.checkbox("Loop (legacy)", &mut self.looping);
                if self.looping != old_looping {
                    self.actions.push(UIAction::ToggleLoop(self.looping));
                }

                ui.separator();
                ui.text("Phase 1: Advanced Playback");
                ui.separator();

                // Playback Direction (Phase 1)
                ui.text("Direction:");
                let direction_names = ["Forward", "Backward"];
                let mut direction_idx = match self.playback_direction {
                    mapmap_media::PlaybackDirection::Forward => 0,
                    mapmap_media::PlaybackDirection::Backward => 1,
                };

                if ui.combo("##direction", &mut direction_idx, &direction_names, |item| {
                    std::borrow::Cow::Borrowed(item)
                }) {
                    let new_direction = match direction_idx {
                        0 => mapmap_media::PlaybackDirection::Forward,
                        1 => mapmap_media::PlaybackDirection::Backward,
                        _ => mapmap_media::PlaybackDirection::Forward,
                    };
                    self.playback_direction = new_direction;
                    self.actions.push(UIAction::SetPlaybackDirection(new_direction));
                }

                ui.same_line();
                if ui.button("Toggle ⇄") {
                    self.actions.push(UIAction::TogglePlaybackDirection);
                    self.playback_direction = match self.playback_direction {
                        mapmap_media::PlaybackDirection::Forward => mapmap_media::PlaybackDirection::Backward,
                        mapmap_media::PlaybackDirection::Backward => mapmap_media::PlaybackDirection::Forward,
                    };
                }

                // Playback Mode (Phase 1)
                ui.text("Mode:");
                let mode_names = ["Loop", "Ping Pong", "Play Once & Eject", "Play Once & Hold"];
                let mut mode_idx = match self.playback_mode {
                    mapmap_media::PlaybackMode::Loop => 0,
                    mapmap_media::PlaybackMode::PingPong => 1,
                    mapmap_media::PlaybackMode::PlayOnceAndEject => 2,
                    mapmap_media::PlaybackMode::PlayOnceAndHold => 3,
                };

                if ui.combo("##mode", &mut mode_idx, &mode_names, |item| {
                    std::borrow::Cow::Borrowed(item)
                }) {
                    let new_mode = match mode_idx {
                        0 => mapmap_media::PlaybackMode::Loop,
                        1 => mapmap_media::PlaybackMode::PingPong,
                        2 => mapmap_media::PlaybackMode::PlayOnceAndEject,
                        3 => mapmap_media::PlaybackMode::PlayOnceAndHold,
                        _ => mapmap_media::PlaybackMode::Loop,
                    };
                    self.playback_mode = new_mode;
                    self.actions.push(UIAction::SetPlaybackMode(new_mode));
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
                    // TODO: Open file dialog
                    self.actions.push(UIAction::LoadVideo(String::new()));
                }
                if ui.menu_item("Save Project") {
                    self.actions.push(UIAction::SaveProject(String::new()));
                }
                if ui.menu_item("Load Project") {
                    self.actions.push(UIAction::LoadProject(String::new()));
                }
                ui.separator();
                if ui.menu_item("Exit") {
                    self.actions.push(UIAction::Exit);
                }
            });

            ui.menu("View", || {
                ui.checkbox("Show Controls", &mut self.show_controls);
                ui.checkbox("Show Layers", &mut self.show_layers);
                ui.checkbox("Show Paints", &mut self.show_paints);
                ui.checkbox("Show Mappings", &mut self.show_mappings);
                ui.checkbox("Show Transforms", &mut self.show_transforms);
                ui.checkbox("Show Master Controls", &mut self.show_master_controls);
                ui.checkbox("Show Stats", &mut self.show_stats);
                ui.separator();
                if ui.menu_item("Toggle Fullscreen") {
                    self.actions.push(UIAction::ToggleFullscreen);
                }
            });

            ui.menu("Help", || {
                if ui.menu_item("About") {
                    // Show about dialog
                }
            });
        });
    }

    /// Render layer management panel
    pub fn render_layer_panel(&mut self, ui: &Ui, layer_manager: &mut mapmap_core::LayerManager) {
        use mapmap_core::BlendMode;

        if !self.show_layers {
            return;
        }

        ui.window("Layers")
            .size([380.0, 600.0], Condition::FirstUseEver)
            .position([1520.0, 100.0], Condition::FirstUseEver)
            .build(|| {
                ui.text(format!("Total Layers: {}", layer_manager.layers().len()));
                ui.separator();

                // Collect layer IDs to avoid borrow issues
                let layer_ids: Vec<u64> = layer_manager.layers().iter().map(|l| l.id).collect();

                // Layer list
                for layer_id in layer_ids {
                    if let Some(layer) = layer_manager.get_layer_mut(layer_id) {
                        let _id = ui.push_id_usize(layer.id as usize);

                        // Layer header with visibility toggle
                        let mut visible = layer.visible;
                        if ui.checkbox(&format!("##visible_{}", layer.id), &mut visible) {
                            layer.visible = visible;
                        }
                        ui.same_line();

                        // Layer name (clickable to select)
                        if ui.small_button(&layer.name) {
                            self.selected_layer_id = Some(layer.id);
                        }

                        // Indent for layer properties
                        ui.indent();

                        // Phase 1: Bypass, Solo, Lock toggles
                        let mut bypass = layer.bypass;
                        if ui.checkbox("Bypass (B)", &mut bypass) {
                            layer.bypass = bypass;
                            self.actions.push(UIAction::ToggleLayerBypass(layer.id));
                        }
                        ui.same_line();

                        let mut solo = layer.solo;
                        if ui.checkbox("Solo (S)", &mut solo) {
                            layer.solo = solo;
                            self.actions.push(UIAction::ToggleLayerSolo(layer.id));
                        }

                        // Blend mode selector
                        let blend_modes = [
                            "Normal", "Add", "Subtract", "Multiply", "Screen",
                            "Overlay", "Soft Light", "Hard Light", "Lighten", "Darken",
                            "Color Dodge", "Color Burn", "Difference", "Exclusion",
                        ];

                        let current_mode_idx = layer.blend_mode as usize;
                        let mut selected = current_mode_idx;

                        if ui.combo("Blend Mode", &mut selected, &blend_modes, |item| {
                            std::borrow::Cow::Borrowed(item)
                        }) {
                            layer.blend_mode = match selected {
                                0 => BlendMode::Normal,
                                1 => BlendMode::Add,
                                2 => BlendMode::Subtract,
                                3 => BlendMode::Multiply,
                                4 => BlendMode::Screen,
                                5 => BlendMode::Overlay,
                                6 => BlendMode::SoftLight,
                                7 => BlendMode::HardLight,
                                8 => BlendMode::Lighten,
                                9 => BlendMode::Darken,
                                10 => BlendMode::ColorDodge,
                                11 => BlendMode::ColorBurn,
                                12 => BlendMode::Difference,
                                13 => BlendMode::Exclusion,
                                _ => BlendMode::Normal,
                            };
                        }

                        // Phase 1: Opacity slider (Video Fader)
                        let old_opacity = layer.opacity;
                        ui.slider("Opacity (V)", 0.0, 1.0, &mut layer.opacity);
                        if (layer.opacity - old_opacity).abs() > 0.001 {
                            self.actions.push(UIAction::SetLayerOpacity(layer.id, layer.opacity));
                        }

                        // Phase 1: Layer management buttons
                        if ui.button("Duplicate") {
                            self.actions.push(UIAction::DuplicateLayer(layer.id));
                        }
                        ui.same_line();
                        if ui.button("Remove") {
                            self.actions.push(UIAction::RemoveLayer(layer.id));
                        }

                        ui.unindent();
                        ui.separator();
                    }
                }

                ui.separator();

                // Layer management buttons
                if ui.button("Add Layer") {
                    self.actions.push(UIAction::AddLayer);
                }
                ui.same_line();
                if ui.button("Eject All (X)") {
                    self.actions.push(UIAction::EjectAllLayers);
                }
            });
    }

    /// Render paint management panel
    pub fn render_paint_panel(&mut self, ui: &Ui, paint_manager: &mut mapmap_core::PaintManager) {
        if !self.show_paints {
            return;
        }

        ui.window("Paints")
            .size([350.0, 400.0], Condition::FirstUseEver)
            .position([10.0, 400.0], Condition::FirstUseEver)
            .build(|| {
                ui.text(format!("Total Paints: {}", paint_manager.paints().len()));
                ui.separator();

                // Paint list
                let paint_ids: Vec<mapmap_core::PaintId> = paint_manager
                    .paints()
                    .iter()
                    .map(|p| p.id)
                    .collect();

                for paint_id in paint_ids {
                    if let Some(paint) = paint_manager.get_paint_mut(paint_id) {
                        let _id = ui.push_id_usize(paint.id as usize);

                        // Paint header
                        ui.text(&format!("{} ({:?})", paint.name, paint.paint_type));

                        // Indent for paint properties
                        ui.indent();

                        // Opacity slider
                        ui.slider("Opacity", 0.0, 1.0, &mut paint.opacity);

                        // Playback controls for video
                        if paint.paint_type == mapmap_core::PaintType::Video {
                            ui.checkbox("Playing", &mut paint.is_playing);
                            ui.same_line();
                            ui.checkbox("Loop", &mut paint.loop_playback);
                            ui.slider("Speed", 0.1, 2.0, &mut paint.rate);
                        }

                        // Color picker for color type
                        if paint.paint_type == mapmap_core::PaintType::Color {
                            ui.color_edit4("Color", &mut paint.color);
                        }

                        ui.unindent();
                        ui.separator();
                    }
                }

                ui.separator();

                // Paint management buttons
                if ui.button("Add Paint") {
                    self.actions.push(UIAction::AddPaint);
                }
            });
    }

    /// Render mapping management panel
    pub fn render_mapping_panel(
        &mut self,
        ui: &Ui,
        mapping_manager: &mut mapmap_core::MappingManager,
    ) {
        if !self.show_mappings {
            return;
        }

        ui.window("Mappings")
            .size([350.0, 450.0], Condition::FirstUseEver)
            .position([380.0, 400.0], Condition::FirstUseEver)
            .build(|| {
                ui.text(format!(
                    "Total Mappings: {}",
                    mapping_manager.mappings().len()
                ));
                ui.separator();

                // Mapping list
                let mapping_ids: Vec<mapmap_core::MappingId> = mapping_manager
                    .mappings()
                    .iter()
                    .map(|m| m.id)
                    .collect();

                let mut selected_mapping_id: Option<u64> = None;

                for mapping_id in mapping_ids {
                    if let Some(mapping) = mapping_manager.get_mapping_mut(mapping_id) {
                        let _id = ui.push_id_usize(mapping.id as usize);

                        // Mapping header with visibility
                        let old_visible = mapping.visible;
                        if ui.checkbox(&format!("##visible_{}", mapping.id), &mut mapping.visible) {
                            if mapping.visible != old_visible {
                                self.actions.push(UIAction::ToggleMappingVisibility(
                                    mapping.id,
                                    mapping.visible,
                                ));
                            }
                        }
                        ui.same_line();

                        // Make the mapping name clickable to select it
                        if ui.small_button(&format!("{} (Paint #{})", mapping.name, mapping.paint_id)) {
                            selected_mapping_id = Some(mapping.id);
                            self.actions.push(UIAction::SelectMapping(mapping.id));
                        }

                        // Indent for mapping properties
                        ui.indent();

                        // Solo and Lock toggles
                        ui.checkbox("Solo", &mut mapping.solo);
                        ui.same_line();
                        ui.checkbox("Lock", &mut mapping.locked);

                        // Opacity slider
                        ui.slider("Opacity", 0.0, 1.0, &mut mapping.opacity);

                        // Depth (Z-order)
                        ui.slider("Depth", -10.0, 10.0, &mut mapping.depth);

                        // Mesh info
                        ui.text(format!(
                            "Mesh: {:?} ({} vertices)",
                            mapping.mesh.mesh_type,
                            mapping.mesh.vertex_count()
                        ));

                        // Remove button for this mapping
                        if ui.button("Remove This") {
                            self.actions.push(UIAction::RemoveMapping(mapping.id));
                        }

                        ui.unindent();
                        ui.separator();
                    }
                }

                ui.separator();

                // Mapping management buttons
                if ui.button("Add Quad Mapping") {
                    self.actions.push(UIAction::AddMapping);
                }
            });
    }

    /// Render transform controls panel (Phase 1)
    pub fn render_transform_panel(&mut self, ui: &Ui, layer_manager: &mut mapmap_core::LayerManager) {
        use mapmap_core::ResizeMode;

        if !self.show_transforms {
            return;
        }

        ui.window("Transform Controls")
            .size([360.0, 520.0], Condition::FirstUseEver)
            .position([10.0, 150.0], Condition::FirstUseEver)
            .build(|| {
                ui.text("Phase 1: Transform System");
                ui.separator();

                if let Some(selected_id) = self.selected_layer_id {
                    if let Some(layer) = layer_manager.get_layer_mut(selected_id) {
                        ui.text(format!("Editing: {}", layer.name));
                        ui.separator();

                        let transform = &mut layer.transform;

                        // Position controls
                        ui.text("Position:");
                        ui.slider("X", -1000.0, 1000.0, &mut transform.position.x);
                        ui.slider("Y", -1000.0, 1000.0, &mut transform.position.y);

                        ui.separator();

                        // Scale controls
                        ui.text("Scale:");
                        ui.slider("Width", 0.1, 5.0, &mut transform.scale.x);
                        ui.slider("Height", 0.1, 5.0, &mut transform.scale.y);

                        // Uniform scale toggle
                        if ui.button("Reset Scale (1:1)") {
                            transform.scale.x = 1.0;
                            transform.scale.y = 1.0;
                        }

                        ui.separator();

                        // Rotation controls (in degrees for UI)
                        ui.text("Rotation (degrees):");
                        let mut rot_x_deg = transform.rotation.x.to_degrees();
                        let mut rot_y_deg = transform.rotation.y.to_degrees();
                        let mut rot_z_deg = transform.rotation.z.to_degrees();

                        ui.slider("X", -180.0, 180.0, &mut rot_x_deg);
                        ui.slider("Y", -180.0, 180.0, &mut rot_y_deg);
                        ui.slider("Z", -180.0, 180.0, &mut rot_z_deg);

                        transform.rotation.x = rot_x_deg.to_radians();
                        transform.rotation.y = rot_y_deg.to_radians();
                        transform.rotation.z = rot_z_deg.to_radians();

                        if ui.button("Reset Rotation") {
                            transform.rotation = glam::Vec3::ZERO;
                        }

                        ui.separator();

                        // Anchor point controls
                        ui.text("Anchor Point (0-1):");
                        ui.slider("Anchor X", 0.0, 1.0, &mut transform.anchor.x);
                        ui.slider("Anchor Y", 0.0, 1.0, &mut transform.anchor.y);

                        if ui.button("Center Anchor (0.5, 0.5)") {
                            transform.anchor = glam::Vec2::splat(0.5);
                        }

                        ui.separator();

                        // Resize mode presets (Phase 1, Month 6)
                        ui.text("Resize Presets:");
                        if ui.button("Fill (Cover)") {
                            self.actions.push(UIAction::ApplyResizeMode(selected_id, ResizeMode::Fill));
                        }
                        ui.same_line();
                        if ui.button("Fit (Contain)") {
                            self.actions.push(UIAction::ApplyResizeMode(selected_id, ResizeMode::Fit));
                        }

                        if ui.button("Stretch (Distort)") {
                            self.actions.push(UIAction::ApplyResizeMode(selected_id, ResizeMode::Stretch));
                        }
                        ui.same_line();
                        if ui.button("Original (1:1)") {
                            self.actions.push(UIAction::ApplyResizeMode(selected_id, ResizeMode::Original));
                        }

                    } else {
                        ui.text("Selected layer not found.");
                    }
                } else {
                    ui.text("No layer selected.");
                    ui.text("Click a layer name in the");
                    ui.text("Layers panel to select it.");
                }
            });
    }

    /// Render master controls panel (Phase 1)
    pub fn render_master_controls(&mut self, ui: &Ui, layer_manager: &mut mapmap_core::LayerManager) {
        if !self.show_master_controls {
            return;
        }

        ui.window("Master Controls")
            .size([340.0, 280.0], Condition::FirstUseEver)
            .position([10.0, 680.0], Condition::FirstUseEver)
            .build(|| {
                ui.text("Phase 1: Master Controls");
                ui.separator();

                let composition = &mut layer_manager.composition;

                // Composition name (Phase 1, Month 5)
                ui.text("Composition:");
                ui.text_wrapped(&composition.name);

                // Note: ImGui text input requires mutable String buffer
                // For now, just display the name
                ui.separator();

                // Master Opacity (Phase 1, Month 4)
                let old_master_opacity = composition.master_opacity;
                ui.slider("Master Opacity (M)", 0.0, 1.0, &mut composition.master_opacity);
                if (composition.master_opacity - old_master_opacity).abs() > 0.001 {
                    self.actions.push(UIAction::SetMasterOpacity(composition.master_opacity));
                }

                // Master Speed (Phase 1, Month 5)
                let old_master_speed = composition.master_speed;
                ui.slider("Master Speed (S)", 0.1, 10.0, &mut composition.master_speed);
                if (composition.master_speed - old_master_speed).abs() > 0.001 {
                    self.actions.push(UIAction::SetMasterSpeed(composition.master_speed));
                }

                ui.separator();
                ui.text(format!("Size: {}x{}", composition.size.0, composition.size.1));
                ui.text(format!("Frame Rate: {:.1} fps", composition.frame_rate));

                ui.separator();
                ui.text("Effective Multipliers:");
                ui.text("All layer opacity × Master Opacity");
                ui.text("All playback speed × Master Speed");
            });
    }

    /// Phase 2: Render output configuration panel
    pub fn render_output_panel(&mut self, ui: &Ui, output_manager: &mut mapmap_core::OutputManager) {
        if !self.show_outputs {
            return;
        }

        ui.window("Outputs")
            .size([420.0, 500.0], Condition::FirstUseEver)
            .position([10.0, 450.0], Condition::FirstUseEver)
            .build(|| {
                ui.text("Multi-Output Configuration");
                ui.separator();

                // Canvas size display
                let canvas_size = output_manager.canvas_size();
                ui.text(format!("Canvas: {}x{}", canvas_size.0, canvas_size.1));
                ui.separator();

                // Output list
                ui.text(format!("Outputs: {}", output_manager.outputs().len()));

                for output in output_manager.outputs() {
                    let _id = ui.push_id_usize(output.id as usize);

                    let is_selected = self.selected_output_id == Some(output.id);
                    if ui.selectable_config(&output.name)
                        .selected(is_selected)
                        .build()
                    {
                        self.selected_output_id = Some(output.id);
                    }

                    // Show output info
                    ui.same_line();
                    ui.text_disabled(format!(
                        "{}x{} | {}",
                        output.resolution.0,
                        output.resolution.1,
                        if output.fullscreen { "FS" } else { "Win" }
                    ));
                }

                ui.separator();

                // Quick setup buttons
                if ui.button("2x2 Projector Array") {
                    self.actions.push(UIAction::CreateProjectorArray2x2(
                        (1920, 1080),
                        0.1, // 10% overlap
                    ));
                }

                ui.same_line();
                if ui.button("Add Output") {
                    // Add a single output covering full canvas
                    self.actions.push(UIAction::AddOutput(
                        "New Output".to_string(),
                        mapmap_core::CanvasRegion::new(0.0, 0.0, 1.0, 1.0),
                        (1920, 1080),
                    ));
                }

                ui.separator();

                // Edit selected output
                if let Some(output_id) = self.selected_output_id {
                    if let Some(output) = output_manager.outputs().iter().find(|o| o.id == output_id) {
                        ui.text("Selected Output Settings");
                        ui.separator();

                        ui.text(format!("Name: {}", output.name));
                        ui.text(format!("Resolution: {}x{}", output.resolution.0, output.resolution.1));

                        ui.separator();
                        ui.text("Canvas Region:");
                        ui.text(format!("  X: {:.2}, Y: {:.2}", output.canvas_region.x, output.canvas_region.y));
                        ui.text(format!("  W: {:.2}, H: {:.2}", output.canvas_region.width, output.canvas_region.height));

                        ui.separator();

                        // Edge blending status
                        let blend = &output.edge_blend;
                        ui.text("Edge Blending:");
                        if blend.left.enabled { ui.text("  Left"); }
                        if blend.right.enabled { ui.text("  Right"); }
                        if blend.top.enabled { ui.text("  Top"); }
                        if blend.bottom.enabled { ui.text("  Bottom"); }
                        if !blend.left.enabled && !blend.right.enabled && !blend.top.enabled && !blend.bottom.enabled {
                            ui.text_disabled("  (None)");
                        }

                        ui.separator();

                        // Color calibration status
                        let cal = &output.color_calibration;
                        ui.text("Color Calibration:");
                        if cal.brightness != 0.0 { ui.text(format!("  Brightness: {:.2}", cal.brightness)); }
                        if cal.contrast != 1.0 { ui.text(format!("  Contrast: {:.2}", cal.contrast)); }
                        if cal.saturation != 1.0 { ui.text(format!("  Saturation: {:.2}", cal.saturation)); }
                        if cal.brightness == 0.0 && cal.contrast == 1.0 && cal.saturation == 1.0 {
                            ui.text_disabled("  (Defaults)");
                        }

                        ui.separator();

                        ui.text_colored([0.5, 0.8, 1.0, 1.0], "Tip:");
                        ui.text_wrapped("Edge Blending and Color Calibration panels open automatically!");

                        ui.separator();

                        if ui.button("Remove Output") {
                            self.actions.push(UIAction::RemoveOutput(output_id));
                            self.selected_output_id = None;
                        }
                    }
                }

                ui.separator();
                ui.text_colored(
                    [0.0, 1.0, 0.0, 1.0],
                    "Multi-window rendering: ACTIVE"
                );
                ui.text_disabled("Output windows are automatically created and synchronized");
            });
    }

    /// Phase 2: Render edge blend configuration
    pub fn render_edge_blend_panel(&mut self, ui: &Ui, output_manager: &mut mapmap_core::OutputManager) {
        // Auto-show when output is selected
        if self.selected_output_id.is_some() {
            self.show_edge_blend = true;
        }

        if !self.show_edge_blend || self.selected_output_id.is_none() {
            return;
        }

        let output_id = self.selected_output_id.unwrap();
        let output = output_manager.get_output_mut(output_id);

        if output.is_none() {
            return;
        }

        let output = output.unwrap();

        ui.window("Edge Blending")
            .size([380.0, 450.0], Condition::FirstUseEver)
            .position([440.0, 450.0], Condition::FirstUseEver)
            .build(|| {
                ui.text(format!("Output: {}", output.name));
                ui.separator();

                let blend = &mut output.edge_blend;

                // Left edge
                ui.checkbox("Left Edge", &mut blend.left.enabled);
                if blend.left.enabled {
                    ui.indent();
                    ui.slider("Width##left", 0.0, 0.5, &mut blend.left.width);
                    ui.slider("Offset##left", -0.1, 0.1, &mut blend.left.offset);
                    ui.unindent();
                }

                ui.separator();

                // Right edge
                ui.checkbox("Right Edge", &mut blend.right.enabled);
                if blend.right.enabled {
                    ui.indent();
                    ui.slider("Width##right", 0.0, 0.5, &mut blend.right.width);
                    ui.slider("Offset##right", -0.1, 0.1, &mut blend.right.offset);
                    ui.unindent();
                }

                ui.separator();

                // Top edge
                ui.checkbox("Top Edge", &mut blend.top.enabled);
                if blend.top.enabled {
                    ui.indent();
                    ui.slider("Width##top", 0.0, 0.5, &mut blend.top.width);
                    ui.slider("Offset##top", -0.1, 0.1, &mut blend.top.offset);
                    ui.unindent();
                }

                ui.separator();

                // Bottom edge
                ui.checkbox("Bottom Edge", &mut blend.bottom.enabled);
                if blend.bottom.enabled {
                    ui.indent();
                    ui.slider("Width##bottom", 0.0, 0.5, &mut blend.bottom.width);
                    ui.slider("Offset##bottom", -0.1, 0.1, &mut blend.bottom.offset);
                    ui.unindent();
                }

                ui.separator();

                // Gamma control
                ui.slider("Blend Gamma", 1.0, 3.0, &mut blend.gamma);

                ui.separator();

                if ui.button("Reset to Defaults") {
                    *blend = mapmap_core::EdgeBlendConfig::default();
                }
            });
    }

    /// Phase 2: Render color calibration panel
    pub fn render_color_calibration_panel(&mut self, ui: &Ui, output_manager: &mut mapmap_core::OutputManager) {
        // Auto-show when output is selected
        if self.selected_output_id.is_some() {
            self.show_color_calibration = true;
        }

        if !self.show_color_calibration || self.selected_output_id.is_none() {
            return;
        }

        let output_id = self.selected_output_id.unwrap();
        let output = output_manager.get_output_mut(output_id);

        if output.is_none() {
            return;
        }

        let output = output.unwrap();

        ui.window("Color Calibration")
            .size([380.0, 500.0], Condition::FirstUseEver)
            .position([830.0, 450.0], Condition::FirstUseEver)
            .build(|| {
                ui.text(format!("Output: {}", output.name));
                ui.separator();

                let cal = &mut output.color_calibration;

                ui.slider("Brightness", -1.0, 1.0, &mut cal.brightness);
                ui.slider("Contrast", 0.0, 2.0, &mut cal.contrast);

                ui.separator();
                ui.text("Gamma (Per Channel)");
                ui.slider("Red Gamma", 0.5, 3.0, &mut cal.gamma.x);
                ui.slider("Green Gamma", 0.5, 3.0, &mut cal.gamma.y);
                ui.slider("Blue Gamma", 0.5, 3.0, &mut cal.gamma_b);

                ui.separator();
                ui.slider("Color Temperature", 2000.0, 10000.0, &mut cal.color_temp);
                ui.text_disabled("(D65 = 6500K)");

                ui.separator();
                ui.slider("Saturation", 0.0, 2.0, &mut cal.saturation);

                ui.separator();

                if ui.button("Reset to Defaults") {
                    *cal = mapmap_core::ColorCalibration::default();
                }
            });
    }
}
