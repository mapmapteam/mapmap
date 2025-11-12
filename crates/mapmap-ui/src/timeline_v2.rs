//! Phase 6: Enhanced Timeline Editor with Keyframe Animation
//!
//! Multi-track timeline with keyframe animation, Bezier interpolation curves,
//! markers, regions, scrubbing, and curve editor.

use egui::{Color32, Pos2, Rect, Sense, Stroke, Ui, Vec2};
use serde::{Deserialize, Serialize};

/// Timeline editor with keyframe animation
pub struct TimelineV2 {
    /// Timeline tracks
    tracks: Vec<Track>,
    /// Current playhead position (in seconds)
    playhead: f32,
    /// Timeline duration (in seconds)
    duration: f32,
    /// Zoom level (pixels per second)
    zoom: f32,
    /// Pan offset
    #[allow(dead_code)]
    pan_offset: f32,
    /// Snap settings
    snap_enabled: bool,
    snap_interval: f32,
    /// Selected keyframes
    selected_keyframes: Vec<(usize, usize)>, // (track_idx, keyframe_idx)
    /// Markers
    #[allow(dead_code)]
    markers: Vec<Marker>,
    /// Regions
    #[allow(dead_code)]
    regions: Vec<Region>,
    /// Show curve editor
    show_curve_editor: bool,
}

/// Timeline track (represents an animatable property)
#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct Track {
    pub name: String,
    pub keyframes: Vec<Keyframe>,
    pub color: Color32,
    pub enabled: bool,
    pub solo: bool,
    pub locked: bool,
}

/// Keyframe with value and interpolation
#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct Keyframe {
    pub time: f32,
    pub value: f32,
    pub interpolation: InterpolationType,
    /// Bezier control points (for bezier interpolation)
    pub control_in: Option<Vec2>,
    pub control_out: Option<Vec2>,
}

/// Interpolation type for keyframes
#[derive(Debug, Clone, Copy, PartialEq, Eq, Serialize, Deserialize)]
pub enum InterpolationType {
    Linear,
    Constant,
    Bezier,
    EaseIn,
    EaseOut,
    EaseInOut,
    Elastic,
    Bounce,
}

impl InterpolationType {
    /// Get human-readable name
    pub fn name(&self) -> &str {
        match self {
            Self::Linear => "Linear",
            Self::Constant => "Constant",
            Self::Bezier => "Bezier",
            Self::EaseIn => "Ease In",
            Self::EaseOut => "Ease Out",
            Self::EaseInOut => "Ease In-Out",
            Self::Elastic => "Elastic",
            Self::Bounce => "Bounce",
        }
    }

    /// Interpolate between two values based on interpolation type
    pub fn interpolate(&self, a: f32, b: f32, t: f32, ctrl_in: Option<Vec2>, ctrl_out: Option<Vec2>) -> f32 {
        match self {
            Self::Linear => a + (b - a) * t,
            Self::Constant => if t < 1.0 { a } else { b },
            Self::Bezier => {
                // Cubic Bezier interpolation using control points
                if let (Some(c_in), Some(c_out)) = (ctrl_in, ctrl_out) {
                    let p0 = a;
                    let p1 = a + c_out.y;
                    let p2 = b + c_in.y;
                    let p3 = b;

                    let t2 = t * t;
                    let t3 = t2 * t;
                    let mt = 1.0 - t;
                    let mt2 = mt * mt;
                    let mt3 = mt2 * mt;

                    mt3 * p0 + 3.0 * mt2 * t * p1 + 3.0 * mt * t2 * p2 + t3 * p3
                } else {
                    a + (b - a) * t
                }
            }
            Self::EaseIn => a + (b - a) * (t * t),
            Self::EaseOut => a + (b - a) * (t * (2.0 - t)),
            Self::EaseInOut => {
                let t2 = if t < 0.5 { 2.0 * t * t } else { -1.0 + (4.0 - 2.0 * t) * t };
                a + (b - a) * t2
            }
            Self::Elastic => {
                if t == 0.0 {
                    a
                } else if t == 1.0 {
                    b
                } else {
                    let p = 0.3;
                    let s = p / 4.0;
                    let post = (b - a) * (2.0_f32).powf(-10.0 * t) * ((t - s) * (2.0 * std::f32::consts::PI) / p).sin();
                    a + (b - a) - post
                }
            }
            Self::Bounce => {
                // Bounce easing
                let mut t = t;
                let b_minus_a = b - a;
                if t < 1.0 / 2.75 {
                    a + b_minus_a * (7.5625 * t * t)
                } else if t < 2.0 / 2.75 {
                    t -= 1.5 / 2.75;
                    a + b_minus_a * (7.5625 * t * t + 0.75)
                } else if t < 2.5 / 2.75 {
                    t -= 2.25 / 2.75;
                    a + b_minus_a * (7.5625 * t * t + 0.9375)
                } else {
                    t -= 2.625 / 2.75;
                    a + b_minus_a * (7.5625 * t * t + 0.984375)
                }
            }
        }
    }
}

/// Timeline marker
#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct Marker {
    pub time: f32,
    pub name: String,
    pub color: Color32,
}

/// Timeline region
#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct Region {
    pub start: f32,
    pub end: f32,
    pub name: String,
    pub color: Color32,
}

impl TimelineV2 {
    pub fn new() -> Self {
        Self {
            tracks: Vec::new(),
            playhead: 0.0,
            duration: 60.0,
            zoom: 50.0,
            pan_offset: 0.0,
            snap_enabled: true,
            snap_interval: 1.0,
            selected_keyframes: Vec::new(),
            markers: Vec::new(),
            regions: Vec::new(),
            show_curve_editor: false,
        }
    }

    /// Add a new track
    pub fn add_track(&mut self, name: String) {
        self.tracks.push(Track {
            name,
            keyframes: Vec::new(),
            color: Color32::from_rgb(100, 150, 200),
            enabled: true,
            solo: false,
            locked: false,
        });
    }

    /// Add a keyframe to a track
    pub fn add_keyframe(&mut self, track_idx: usize, time: f32, value: f32) {
        if let Some(track) = self.tracks.get_mut(track_idx) {
            // Check if keyframe already exists at this time
            if let Some(existing) = track.keyframes.iter_mut().find(|k| (k.time - time).abs() < 0.01) {
                existing.value = value;
            } else {
                track.keyframes.push(Keyframe {
                    time,
                    value,
                    interpolation: InterpolationType::Linear,
                    control_in: None,
                    control_out: None,
                });
                track.keyframes.sort_by(|a, b| a.time.partial_cmp(&b.time).unwrap());
            }
        }
    }

    /// Get interpolated value for a track at a given time
    pub fn get_value(&self, track_idx: usize, time: f32) -> Option<f32> {
        if let Some(track) = self.tracks.get(track_idx) {
            if track.keyframes.is_empty() {
                return None;
            }

            // Find surrounding keyframes
            let mut prev_kf: Option<&Keyframe> = None;
            let mut next_kf: Option<&Keyframe> = None;

            for kf in &track.keyframes {
                if kf.time <= time {
                    prev_kf = Some(kf);
                } else if next_kf.is_none() {
                    next_kf = Some(kf);
                    break;
                }
            }

            match (prev_kf, next_kf) {
                (Some(prev), Some(next)) => {
                    let t = (time - prev.time) / (next.time - prev.time);
                    Some(prev.interpolation.interpolate(
                        prev.value,
                        next.value,
                        t,
                        prev.control_in,
                        prev.control_out,
                    ))
                }
                (Some(kf), None) => Some(kf.value),
                (None, Some(kf)) => Some(kf.value),
                (None, None) => None,
            }
        } else {
            None
        }
    }

    /// Snap time to grid
    fn snap_time(&self, time: f32) -> f32 {
        if self.snap_enabled {
            (time / self.snap_interval).round() * self.snap_interval
        } else {
            time
        }
    }

    /// Render the timeline UI
    pub fn ui(&mut self, ui: &mut Ui) -> Option<TimelineAction> {
        let mut action = None;

        // Toolbar
        ui.horizontal(|ui| {
            if ui.button("⏵ Play").clicked() {
                action = Some(TimelineAction::Play);
            }
            if ui.button("⏸ Pause").clicked() {
                action = Some(TimelineAction::Pause);
            }
            if ui.button("⏹ Stop").clicked() {
                action = Some(TimelineAction::Stop);
                self.playhead = 0.0;
            }

            ui.separator();

            ui.label(format!("Time: {:.2}s", self.playhead));

            ui.separator();

            ui.checkbox(&mut self.snap_enabled, "Snap");
            if self.snap_enabled {
                ui.add(egui::DragValue::new(&mut self.snap_interval).prefix("Snap: ").suffix("s"));
            }

            ui.separator();

            ui.label(format!("Zoom: {:.0}px/s", self.zoom));
            if ui.button("➕").clicked() {
                self.zoom *= 1.2;
            }
            if ui.button("➖").clicked() {
                self.zoom /= 1.2;
            }

            ui.separator();

            ui.checkbox(&mut self.show_curve_editor, "Curve Editor");

            ui.separator();

            if ui.button("Add Track").clicked() {
                action = Some(TimelineAction::AddTrack);
            }
        });

        ui.separator();

        // Timeline area
        egui::ScrollArea::both().show(ui, |ui| {
            let available_height = 200.0 + (self.tracks.len() as f32 * 50.0);
            let available_width = self.duration * self.zoom;

            let (response, painter) = ui.allocate_painter(
                Vec2::new(available_width, available_height),
                Sense::click_and_drag(),
            );

            let rect = response.rect;

            // Draw time ruler
            let ruler_rect = Rect::from_min_size(rect.min, Vec2::new(rect.width(), 30.0));
            painter.rect_filled(ruler_rect, 0.0, Color32::from_rgb(40, 40, 40));

            // Draw time markers
            let mut time = 0.0;
            while time <= self.duration {
                let x = rect.min.x + time * self.zoom;
                painter.line_segment(
                    [Pos2::new(x, ruler_rect.min.y), Pos2::new(x, ruler_rect.max.y)],
                    Stroke::new(1.0, Color32::from_rgb(100, 100, 100)),
                );
                painter.text(
                    Pos2::new(x + 2.0, ruler_rect.min.y + 2.0),
                    egui::Align2::LEFT_TOP,
                    format!("{:.1}s", time),
                    egui::FontId::proportional(10.0),
                    Color32::WHITE,
                );
                time += 1.0;
            }

            // Draw playhead in ruler
            let playhead_x = rect.min.x + self.playhead * self.zoom;
            painter.line_segment(
                [
                    Pos2::new(playhead_x, ruler_rect.min.y),
                    Pos2::new(playhead_x, rect.max.y),
                ],
                Stroke::new(2.0, Color32::from_rgb(255, 100, 100)),
            );

            // Draw tracks
            let track_start_y = ruler_rect.max.y;
            for (i, track) in self.tracks.iter().enumerate() {
                let track_y = track_start_y + (i as f32 * 50.0);
                let track_rect = Rect::from_min_size(
                    Pos2::new(rect.min.x, track_y),
                    Vec2::new(rect.width(), 50.0),
                );

                // Track background
                let bg_color = if i % 2 == 0 {
                    Color32::from_rgb(35, 35, 35)
                } else {
                    Color32::from_rgb(40, 40, 40)
                };
                painter.rect_filled(track_rect, 0.0, bg_color);

                // Track name
                painter.text(
                    Pos2::new(track_rect.min.x + 5.0, track_rect.center().y),
                    egui::Align2::LEFT_CENTER,
                    &track.name,
                    egui::FontId::proportional(12.0),
                    track.color,
                );

                // Draw keyframes
                for (kf_idx, keyframe) in track.keyframes.iter().enumerate() {
                    let kf_x = rect.min.x + keyframe.time * self.zoom;
                    let kf_y = track_rect.center().y;

                    let is_selected = self.selected_keyframes.contains(&(i, kf_idx));
                    let kf_color = if is_selected {
                        Color32::from_rgb(255, 200, 100)
                    } else {
                        track.color
                    };

                    // Draw keyframe diamond
                    let size = 6.0;
                    let points = [
                        Pos2::new(kf_x, kf_y - size),
                        Pos2::new(kf_x + size, kf_y),
                        Pos2::new(kf_x, kf_y + size),
                        Pos2::new(kf_x - size, kf_y),
                    ];
                    painter.add(egui::Shape::convex_polygon(
                        points.to_vec(),
                        kf_color,
                        Stroke::new(1.0, Color32::WHITE),
                    ));
                }

                // Draw interpolation curve between keyframes
                if track.keyframes.len() >= 2 {
                    for i in 0..track.keyframes.len() - 1 {
                        let kf1 = &track.keyframes[i];
                        let kf2 = &track.keyframes[i + 1];

                        let _x1 = rect.min.x + kf1.time * self.zoom;
                        let _x2 = rect.min.x + kf2.time * self.zoom;

                        // Normalize values to track height
                        let _y1 = track_rect.max.y - (kf1.value * track_rect.height());
                        let _y2 = track_rect.max.y - (kf2.value * track_rect.height());

                        // Draw curve with multiple segments
                        let segments = 10;
                        for seg in 0..segments {
                            let t1 = seg as f32 / segments as f32;
                            let t2 = (seg + 1) as f32 / segments as f32;

                            let time1 = kf1.time + (kf2.time - kf1.time) * t1;
                            let time2 = kf1.time + (kf2.time - kf1.time) * t2;

                            let val1 = kf1.interpolation.interpolate(
                                kf1.value,
                                kf2.value,
                                t1,
                                kf1.control_in,
                                kf1.control_out,
                            );
                            let val2 = kf1.interpolation.interpolate(
                                kf1.value,
                                kf2.value,
                                t2,
                                kf1.control_in,
                                kf1.control_out,
                            );

                            let px1 = rect.min.x + time1 * self.zoom;
                            let py1 = track_rect.max.y - (val1 * track_rect.height());
                            let px2 = rect.min.x + time2 * self.zoom;
                            let py2 = track_rect.max.y - (val2 * track_rect.height());

                            painter.line_segment(
                                [Pos2::new(px1, py1), Pos2::new(px2, py2)],
                                Stroke::new(2.0, track.color.linear_multiply(0.5)),
                            );
                        }
                    }
                }
            }

            // Handle timeline click
            if response.clicked() {
                if let Some(pos) = response.interact_pointer_pos() {
                    // Set playhead
                    let time = (pos.x - rect.min.x) / self.zoom;
                    self.playhead = self.snap_time(time.max(0.0).min(self.duration));
                    action = Some(TimelineAction::Seek(self.playhead));
                }
            }
        });

        action
    }
}

/// Actions that can be triggered by the timeline
#[derive(Debug, Clone)]
pub enum TimelineAction {
    Play,
    Pause,
    Stop,
    Seek(f32),
    AddTrack,
    AddKeyframe(usize, f32, f32),
    RemoveKeyframe(usize, usize),
    AddMarker(f32, String),
}
