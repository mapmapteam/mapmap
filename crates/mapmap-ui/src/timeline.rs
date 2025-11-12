//! Timeline UI for Keyframe Animation
//!
//! Phase 3: Effects Pipeline
//! Timeline editor for keyframe-based parameter animation

use imgui::*;
use mapmap_core::{
    AnimationClip, AnimationTrack, InterpolationMode,
};

/// Timeline editor state
pub struct TimelineEditor {
    /// Current animation clip
    pub clip: Option<AnimationClip>,

    /// Current playback time (seconds)
    pub current_time: f64,

    /// Timeline zoom level (pixels per second)
    pub zoom: f32,

    /// Timeline scroll offset (seconds)
    pub scroll_offset: f64,

    /// Selected keyframes (track_name, keyframe_index)
    pub selected_keyframes: Vec<(String, usize)>,

    /// Currently dragging keyframe
    pub dragging_keyframe: Option<(String, usize)>,

    /// Show only selected tracks
    pub solo_mode: bool,

    /// Soloed tracks
    pub soloed_tracks: Vec<String>,

    /// Timeline snap settings
    pub snap_enabled: bool,
    pub snap_interval: f64,

    /// Playback controls
    pub playing: bool,
    pub loop_enabled: bool,
    pub loop_start: f64,
    pub loop_end: f64,

    /// Track heights
    pub track_height: f32,
    pub track_header_width: f32,

    /// Show curve editor
    pub show_curve_editor: bool,

    /// Curve editor selected track
    pub curve_editor_track: Option<String>,
}

impl Default for TimelineEditor {
    fn default() -> Self {
        Self {
            clip: None,
            current_time: 0.0,
            zoom: 100.0, // 100 pixels per second
            scroll_offset: 0.0,
            selected_keyframes: Vec::new(),
            dragging_keyframe: None,
            solo_mode: false,
            soloed_tracks: Vec::new(),
            snap_enabled: true,
            snap_interval: 0.1, // 100ms
            playing: false,
            loop_enabled: false,
            loop_start: 0.0,
            loop_end: 10.0,
            track_height: 30.0,
            track_header_width: 200.0,
            show_curve_editor: false,
            curve_editor_track: None,
        }
    }
}

impl TimelineEditor {
    /// Create a new timeline editor
    pub fn new() -> Self {
        Self::default()
    }

    /// Load an animation clip
    pub fn load_clip(&mut self, clip: AnimationClip) {
        self.clip = Some(clip);
        self.current_time = 0.0;
        self.selected_keyframes.clear();
    }

    /// Draw the timeline UI
    pub fn draw(&mut self, ui: &Ui) -> Vec<TimelineAction> {
        let mut actions = Vec::new();

        // Transport controls
        self.draw_transport_controls(ui, &mut actions);

        ui.separator();

        // Timeline ruler and tracks
        self.draw_timeline(ui, &mut actions);

        // Curve editor (if enabled)
        if self.show_curve_editor {
            self.draw_curve_editor(ui, &mut actions);
        }

        actions
    }

    /// Draw transport controls (play/pause/stop)
    fn draw_transport_controls(&mut self, ui: &Ui, actions: &mut Vec<TimelineAction>) {
        // Play/Pause button
        if self.playing {
            if ui.button("Pause") {
                self.playing = false;
                actions.push(TimelineAction::Pause);
            }
        } else {
            if ui.button("Play") {
                self.playing = true;
                actions.push(TimelineAction::Play);
            }
        }

        ui.same_line();

        // Stop button
        if ui.button("Stop") {
            self.playing = false;
            self.current_time = 0.0;
            actions.push(TimelineAction::Stop);
        }

        ui.same_line();

        // Current time display and input
        ui.text(format!("Time: {:.2}s", self.current_time));

        ui.same_line();

        // Loop toggle
        ui.checkbox("Loop", &mut self.loop_enabled);

        ui.same_line();

        // Snap toggle
        ui.checkbox("Snap", &mut self.snap_enabled);

        ui.same_line();

        // Zoom controls
        if ui.button("-") {
            self.zoom = (self.zoom * 0.8).max(20.0);
        }
        ui.same_line();
        ui.text(format!("Zoom: {:.0}px/s", self.zoom));
        ui.same_line();
        if ui.button("+") {
            self.zoom = (self.zoom * 1.25).min(500.0);
        }

        ui.same_line();

        // Curve editor toggle
        ui.checkbox("Curves", &mut self.show_curve_editor);

        ui.same_line();

        // Add keyframe button
        if ui.button("Add Keyframe") {
            actions.push(TimelineAction::AddKeyframe(self.current_time));
        }
    }

    /// Draw timeline ruler and tracks
    fn draw_timeline(&mut self, ui: &Ui, actions: &mut Vec<TimelineAction>) {
        if self.clip.is_none() {
            ui.text("No animation clip loaded");
            return;
        }

        ui.window("Timeline")
            .size([ui.window_size()[0] - 20.0, 300.0], Condition::FirstUseEver)
            .build(|| {
                let draw_list = ui.get_window_draw_list();
                let canvas_pos = ui.cursor_screen_pos();
                let canvas_size = ui.content_region_avail();

                // Background
                draw_list
                    .add_rect(
                        canvas_pos,
                        [canvas_pos[0] + canvas_size[0], canvas_pos[1] + canvas_size[1]],
                        [0.15, 0.15, 0.15, 1.0],
                    )
                    .filled(true)
                    .build();

                // Draw ruler
                self.draw_ruler(&draw_list, canvas_pos, canvas_size);

                // Draw playhead
                self.draw_playhead(&draw_list, canvas_pos, canvas_size);

                // Draw tracks
                if let Some(clip) = &self.clip {
                    let mut y_offset = 40.0; // Start below ruler
                    for track in &clip.tracks {
                        self.draw_track(&draw_list, canvas_pos, canvas_size, &track.name, track, y_offset);
                        y_offset += self.track_height;
                    }
                }

                // Handle mouse input
                if ui.is_window_hovered() {
                    // Seek with click
                    if ui.is_mouse_clicked(MouseButton::Left) {
                        let mouse_pos = ui.io().mouse_pos;
                        let rel_x = mouse_pos[0] - canvas_pos[0] - self.track_header_width;
                        if rel_x >= 0.0 {
                            self.current_time = (rel_x / self.zoom) as f64 + self.scroll_offset;
                            actions.push(TimelineAction::Seek(self.current_time));
                        }
                    }

                    // Scroll with mouse wheel
                    let scroll = ui.io().mouse_wheel;
                    if ui.io().key_shift && scroll != 0.0 {
                        // Horizontal scroll
                        self.scroll_offset -= (scroll * 0.5) as f64;
                        self.scroll_offset = self.scroll_offset.max(0.0);
                    } else if scroll != 0.0 {
                        // Zoom
                        let old_zoom = self.zoom;
                        self.zoom = (self.zoom * (1.0 + scroll * 0.1)).clamp(20.0, 500.0);

                        // Adjust scroll to zoom around mouse position
                        let mouse_pos = ui.io().mouse_pos;
                        let rel_x = mouse_pos[0] - canvas_pos[0] - self.track_header_width;
                        if rel_x >= 0.0 {
                            let time_at_mouse = (rel_x / old_zoom) as f64 + self.scroll_offset;
                            self.scroll_offset = time_at_mouse - (rel_x / self.zoom) as f64;
                            self.scroll_offset = self.scroll_offset.max(0.0);
                        }
                    }
                }
            });
    }

    /// Draw timeline ruler
    fn draw_ruler(&self, draw_list: &DrawListMut, canvas_pos: [f32; 2], canvas_size: [f32; 2]) {
        let ruler_height = 30.0;
        let ruler_color = [0.25, 0.25, 0.25, 1.0];
        let text_color = [0.8, 0.8, 0.8, 1.0];

        // Ruler background
        draw_list
            .add_rect(
                [canvas_pos[0] + self.track_header_width, canvas_pos[1]],
                [canvas_pos[0] + canvas_size[0], canvas_pos[1] + ruler_height],
                ruler_color,
            )
            .filled(true)
            .build();

        // Time markers
        let visible_start = self.scroll_offset;
        let visible_end = visible_start + (canvas_size[0] - self.track_header_width) as f64 / self.zoom as f64;

        let marker_interval = if self.zoom > 200.0 {
            0.1 // Every 100ms
        } else if self.zoom > 100.0 {
            0.5 // Every 500ms
        } else {
            1.0 // Every second
        };

        let start_marker = (visible_start / marker_interval).floor() * marker_interval;
        let mut t = start_marker;
        while t <= visible_end {
            let x = canvas_pos[0] + self.track_header_width + ((t - self.scroll_offset) * self.zoom as f64) as f32;

            // Draw tick
            let is_second = (t % 1.0).abs() < 0.01;
            let tick_height = if is_second { 20.0 } else { 10.0 };
            draw_list.add_line(
                [x, canvas_pos[1] + ruler_height],
                [x, canvas_pos[1] + ruler_height - tick_height],
                text_color,
            ).build();

            // Draw time label (only for seconds)
            if is_second {
                draw_list.add_text(
                    [x + 2.0, canvas_pos[1] + 5.0],
                    text_color,
                    &format!("{:.0}s", t),
                );
            }

            t += marker_interval;
        }
    }

    /// Draw playhead indicator
    fn draw_playhead(&self, draw_list: &DrawListMut, canvas_pos: [f32; 2], canvas_size: [f32; 2]) {
        let x = canvas_pos[0] + self.track_header_width +
                ((self.current_time - self.scroll_offset) * self.zoom as f64) as f32;

        if x >= canvas_pos[0] + self.track_header_width && x <= canvas_pos[0] + canvas_size[0] {
            // Vertical line
            draw_list.add_line(
                [x, canvas_pos[1]],
                [x, canvas_pos[1] + canvas_size[1]],
                [1.0, 0.3, 0.3, 1.0],
            ).thickness(2.0).build();

            // Triangle at top
            let triangle_size = 8.0;
            draw_list.add_triangle(
                [x, canvas_pos[1] + 30.0],
                [x - triangle_size, canvas_pos[1] + 30.0 - triangle_size],
                [x + triangle_size, canvas_pos[1] + 30.0 - triangle_size],
                [1.0, 0.3, 0.3, 1.0],
            ).filled(true).build();
        }
    }

    /// Draw a single track
    fn draw_track(
        &self,
        draw_list: &DrawListMut,
        canvas_pos: [f32; 2],
        canvas_size: [f32; 2],
        track_name: &str,
        track: &AnimationTrack,
        y_offset: f32,
    ) {
        let track_y = canvas_pos[1] + y_offset;
        let track_color = [0.2, 0.2, 0.2, 1.0];
        let header_color = [0.25, 0.25, 0.25, 1.0];

        // Track header
        draw_list
            .add_rect(
                [canvas_pos[0], track_y],
                [canvas_pos[0] + self.track_header_width, track_y + self.track_height],
                header_color,
            )
            .filled(true)
            .build();

        draw_list.add_text(
            [canvas_pos[0] + 5.0, track_y + 8.0],
            [1.0, 1.0, 1.0, 1.0],
            track_name,
        );

        // Track content area
        draw_list
            .add_rect(
                [canvas_pos[0] + self.track_header_width, track_y],
                [canvas_pos[0] + canvas_size[0], track_y + self.track_height],
                track_color,
            )
            .filled(true)
            .build();

        // Draw keyframes
        for (i, (_time_us, keyframe)) in track.keyframes.iter().enumerate() {
            let x = canvas_pos[0] + self.track_header_width +
                    ((keyframe.time - self.scroll_offset) * self.zoom as f64) as f32;

            if x >= canvas_pos[0] + self.track_header_width && x <= canvas_pos[0] + canvas_size[0] {
                let is_selected = self.selected_keyframes.contains(&(track_name.to_string(), i));
                let keyframe_color = if is_selected {
                    [1.0, 0.8, 0.2, 1.0]
                } else {
                    [0.6, 0.8, 1.0, 1.0]
                };

                // Draw keyframe diamond
                let diamond_size = 6.0;
                let center_y = track_y + self.track_height * 0.5;

                // Draw diamond as four triangles since add_quad doesn't exist
                draw_list.add_triangle(
                    [x, center_y - diamond_size],
                    [x + diamond_size, center_y],
                    [x, center_y],
                    keyframe_color,
                ).filled(true).build();
                draw_list.add_triangle(
                    [x + diamond_size, center_y],
                    [x, center_y + diamond_size],
                    [x, center_y],
                    keyframe_color,
                ).filled(true).build();
                draw_list.add_triangle(
                    [x, center_y + diamond_size],
                    [x - diamond_size, center_y],
                    [x, center_y],
                    keyframe_color,
                ).filled(true).build();
                draw_list.add_triangle(
                    [x - diamond_size, center_y],
                    [x, center_y - diamond_size],
                    [x, center_y],
                    keyframe_color,
                ).filled(true).build();
            }
        }
    }

    /// Draw curve editor
    fn draw_curve_editor(&self, ui: &Ui, _actions: &mut Vec<TimelineAction>) {
        ui.window("Curve Editor")
            .size([600.0, 300.0], Condition::FirstUseEver)
            .build(|| {
                if let Some(track_name) = &self.curve_editor_track {
                    ui.text(format!("Editing: {}", track_name));
                    ui.separator();

                    if let Some(clip) = &self.clip {
                        if let Some(track) = clip.tracks.iter().find(|t| &t.name == track_name) {
                            // Draw curve graph
                            let draw_list = ui.get_window_draw_list();
                            let canvas_pos = ui.cursor_screen_pos();
                            let canvas_size = ui.content_region_avail();

                            // Background
                            draw_list
                                .add_rect(
                                    canvas_pos,
                                    [canvas_pos[0] + canvas_size[0], canvas_pos[1] + canvas_size[1]],
                                    [0.1, 0.1, 0.1, 1.0],
                                )
                                .filled(true)
                                .build();

                            // Draw curve
                            self.draw_animation_curve(&draw_list, canvas_pos, canvas_size, track);
                        }
                    }
                } else {
                    ui.text("Select a track to edit curves");
                }
            });
    }

    /// Draw animation curve for a track
    fn draw_animation_curve(
        &self,
        draw_list: &DrawListMut,
        canvas_pos: [f32; 2],
        canvas_size: [f32; 2],
        track: &AnimationTrack,
    ) {
        if track.keyframes.len() < 2 {
            return;
        }

        let curve_color = [0.2, 0.8, 0.4, 1.0];

        // Sample the curve and draw line segments
        let num_samples = 200;
        let first_keyframe = track.keyframes.values().next().unwrap();
        let last_keyframe = track.keyframes.values().last().unwrap();
        let time_range = last_keyframe.time - first_keyframe.time;

        for i in 0..num_samples {
            let t1 = first_keyframe.time + (time_range * i as f64 / num_samples as f64);
            let t2 = first_keyframe.time + (time_range * (i + 1) as f64 / num_samples as f64);

            // Map time to x position
            let x1 = canvas_pos[0] + (t1 - self.scroll_offset) as f32 / time_range as f32 * canvas_size[0];
            let x2 = canvas_pos[0] + (t2 - self.scroll_offset) as f32 / time_range as f32 * canvas_size[0];

            // Sample values (simplified - would need actual interpolation)
            // For now, just draw a placeholder line
            let y = canvas_pos[1] + canvas_size[1] * 0.5;

            draw_list.add_line([x1, y], [x2, y], curve_color).build();
        }
    }
}

/// Actions that can be performed in the timeline editor
#[derive(Debug, Clone)]
pub enum TimelineAction {
    Play,
    Pause,
    Stop,
    Seek(f64),
    AddKeyframe(f64),
    DeleteKeyframe(String, usize),
    MoveKeyframe(String, usize, f64),
    SetInterpolation(String, usize, InterpolationMode),
    SelectKeyframe(String, usize),
    DeselectAll,
}
