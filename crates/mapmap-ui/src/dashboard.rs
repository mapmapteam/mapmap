//! Phase 6: Dashboard Controls
//!
//! Quick-access parameter controls with customizable layouts.
//! Allows users to assign frequently-used parameters to dashboard dials and sliders.

use egui::{Color32, Pos2, Sense, Stroke, Ui, Vec2};
use serde::{Deserialize, Serialize};

/// Dashboard control panel
pub struct Dashboard {
    /// Dashboard widgets
    widgets: Vec<DashboardWidget>,
    /// Layout mode
    layout: LayoutMode,
    /// Grid columns (for grid layout)
    grid_columns: usize,
}

#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum LayoutMode {
    Grid,
    Freeform,
}

/// Dashboard widget (assignable control)
#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct DashboardWidget {
    pub id: u64,
    pub widget_type: WidgetType,
    pub parameter_name: String,
    pub position: Option<Pos2>,
    pub size: Option<Vec2>,
}

/// Widget type
#[derive(Debug, Clone, Serialize, Deserialize)]
pub enum WidgetType {
    /// Slider control
    Slider {
        value: f32,
        min: f32,
        max: f32,
    },
    /// Knob/dial control
    Knob {
        value: f32,
        min: f32,
        max: f32,
    },
    /// Toggle button
    Toggle {
        value: bool,
    },
    /// XY Pad (2D control)
    XYPad {
        x: f32,
        y: f32,
        x_min: f32,
        x_max: f32,
        y_min: f32,
        y_max: f32,
    },
    /// Button (trigger)
    Button,
    /// Label (display value)
    Label {
        value: String,
    },
}

impl Dashboard {
    pub fn new() -> Self {
        Self {
            widgets: Vec::new(),
            layout: LayoutMode::Grid,
            grid_columns: 4,
        }
    }

    /// Add a widget to the dashboard
    pub fn add_widget(&mut self, widget: DashboardWidget) {
        self.widgets.push(widget);
    }

    /// Remove a widget from the dashboard
    pub fn remove_widget(&mut self, widget_id: u64) {
        self.widgets.retain(|w| w.id != widget_id);
    }

    /// Get widget by ID
    pub fn get_widget_mut(&mut self, widget_id: u64) -> Option<&mut DashboardWidget> {
        self.widgets.iter_mut().find(|w| w.id == widget_id)
    }

    /// Render the dashboard UI
    pub fn ui(&mut self, ui: &mut Ui) -> Option<DashboardAction> {
        let mut action = None;

        // Toolbar
        ui.horizontal(|ui| {
            ui.selectable_value(&mut self.layout, LayoutMode::Grid, "Grid");
            ui.selectable_value(&mut self.layout, LayoutMode::Freeform, "Freeform");

            if self.layout == LayoutMode::Grid {
                ui.separator();
                ui.label("Columns:");
                ui.add(egui::DragValue::new(&mut self.grid_columns).range(1..=8));
            }

            ui.separator();

            if ui.button("➕ Add Widget").clicked() {
                action = Some(DashboardAction::AddWidget);
            }
        });

        ui.separator();

        // Render widgets based on layout mode
        match self.layout {
            LayoutMode::Grid => {
                action = self.render_grid_layout(ui);
            }
            LayoutMode::Freeform => {
                action = self.render_freeform_layout(ui);
            }
        }

        action
    }

    /// Render grid layout
    fn render_grid_layout(&mut self, ui: &mut Ui) -> Option<DashboardAction> {
        let mut action = None;

        egui::Grid::new("dashboard_grid")
            .spacing([12.0, 12.0])
            .min_col_width(150.0)
            .show(ui, |ui| {
                for (i, widget) in self.widgets.iter_mut().enumerate() {
                    if i > 0 && i % self.grid_columns == 0 {
                        ui.end_row();
                    }

                    if let Some(a) = Self::render_widget(ui, widget) {
                        action = Some(a);
                    }
                }
            });

        action
    }

    /// Render freeform layout
    fn render_freeform_layout(&mut self, ui: &mut Ui) -> Option<DashboardAction> {
        let mut action = None;

        let (_response, _painter) = ui.allocate_painter(ui.available_size(), Sense::click_and_drag());

        for widget in &mut self.widgets {
            // Use egui::Area for freeform positioning
            let widget_pos = widget.position.unwrap_or(Pos2::new(100.0, 100.0));

            egui::Area::new(egui::Id::new(format!("widget_{}", widget.id)))
                .fixed_pos(widget_pos)
                .movable(true)
                .show(ui.ctx(), |ui| {
                    egui::Frame::group(ui.style()).show(ui, |ui| {
                        if let Some(a) = Self::render_widget(ui, widget) {
                            action = Some(a);
                        }

                        // Update position after potential move
                        if let Some(new_pos) = ui.input(|i| i.pointer.hover_pos()) {
                            if ui.input(|i| i.pointer.is_decidedly_dragging()) {
                                widget.position = Some(new_pos);
                            }
                        }
                    });
                });
        }

        action
    }

    /// Render a single widget
    fn render_widget(ui: &mut Ui, widget: &mut DashboardWidget) -> Option<DashboardAction> {
        let mut action = None;

        ui.vertical(|ui| {
            // Widget label
            ui.label(&widget.parameter_name);

            // Widget control
            match &mut widget.widget_type {
                WidgetType::Slider { value, min, max } => {
                    let slider = egui::Slider::new(value, *min..=*max)
                        .show_value(true);
                    if ui.add(slider).changed() {
                        action = Some(DashboardAction::ValueChanged(widget.id, *value));
                    }
                }
                WidgetType::Knob { value, min, max } => {
                    // Draw custom knob
                    let desired_size = Vec2::new(80.0, 80.0);
                    let (rect, response) = ui.allocate_exact_size(desired_size, Sense::click_and_drag());

                    if ui.is_rect_visible(rect) {
                        let painter = ui.painter();
                        let center = rect.center();
                        let radius = rect.width().min(rect.height()) * 0.4;

                        // Background circle
                        painter.circle_filled(center, radius, Color32::from_rgb(40, 40, 40));
                        painter.circle_stroke(center, radius, Stroke::new(2.0, Color32::from_rgb(100, 100, 100)));

                        // Value arc
                        let normalized = (*value - *min) / (*max - *min);
                        let angle = -135.0 + (normalized * 270.0);
                        let angle_rad = angle.to_radians();

                        let indicator_pos = center + Vec2::new(
                            angle_rad.cos() * radius * 0.7,
                            angle_rad.sin() * radius * 0.7,
                        );

                        painter.line_segment(
                            [center, indicator_pos],
                            Stroke::new(3.0, Color32::from_rgb(100, 150, 255)),
                        );

                        // Handle interaction
                        if response.dragged() {
                            let delta_y = response.drag_delta().y;
                            let new_value = *value - delta_y * (*max - *min) * 0.01;
                            *value = new_value.clamp(*min, *max);
                            action = Some(DashboardAction::ValueChanged(widget.id, *value));
                        }

                        // Value text
                        painter.text(
                            center + Vec2::new(0.0, radius + 15.0),
                            egui::Align2::CENTER_CENTER,
                            format!("{:.2}", *value),
                            egui::FontId::proportional(12.0),
                            Color32::WHITE,
                        );
                    }
                }
                WidgetType::Toggle { value } => {
                    if ui.checkbox(value, "").changed() {
                        action = Some(DashboardAction::BoolChanged(widget.id, *value));
                    }
                }
                WidgetType::XYPad { x, y, x_min, x_max, y_min, y_max } => {
                    let desired_size = Vec2::new(150.0, 150.0);
                    let (rect, response) = ui.allocate_exact_size(desired_size, Sense::click_and_drag());

                    if ui.is_rect_visible(rect) {
                        let painter = ui.painter();

                        // Background
                        painter.rect_filled(rect, 2.0, Color32::from_rgb(30, 30, 30));
                        painter.rect_stroke(rect, 2.0, Stroke::new(2.0, Color32::from_rgb(80, 80, 80)));

                        // Crosshairs
                        let x_norm = (*x - *x_min) / (*x_max - *x_min);
                        let y_norm = (*y - *y_min) / (*y_max - *y_min);

                        let pad_x = rect.min.x + x_norm * rect.width();
                        let pad_y = rect.min.y + (1.0 - y_norm) * rect.height();

                        painter.line_segment(
                            [Pos2::new(pad_x, rect.min.y), Pos2::new(pad_x, rect.max.y)],
                            Stroke::new(1.0, Color32::from_rgb(100, 100, 100)),
                        );
                        painter.line_segment(
                            [Pos2::new(rect.min.x, pad_y), Pos2::new(rect.max.x, pad_y)],
                            Stroke::new(1.0, Color32::from_rgb(100, 100, 100)),
                        );

                        // Control point
                        painter.circle_filled(Pos2::new(pad_x, pad_y), 8.0, Color32::from_rgb(100, 150, 255));
                        painter.circle_stroke(Pos2::new(pad_x, pad_y), 8.0, Stroke::new(2.0, Color32::WHITE));

                        // Handle interaction
                        if response.clicked() || response.dragged() {
                            if let Some(pos) = response.interact_pointer_pos() {
                                let x_norm = ((pos.x - rect.min.x) / rect.width()).clamp(0.0, 1.0);
                                let y_norm = 1.0 - ((pos.y - rect.min.y) / rect.height()).clamp(0.0, 1.0);

                                *x = *x_min + x_norm * (*x_max - *x_min);
                                *y = *y_min + y_norm * (*y_max - *y_min);

                                action = Some(DashboardAction::XYChanged(widget.id, *x, *y));
                            }
                        }

                        // Value text
                        painter.text(
                            rect.center_bottom() + Vec2::new(0.0, 15.0),
                            egui::Align2::CENTER_CENTER,
                            format!("X: {:.2}, Y: {:.2}", *x, *y),
                            egui::FontId::proportional(10.0),
                            Color32::WHITE,
                        );
                    }
                }
                WidgetType::Button => {
                    if ui.button("Trigger").clicked() {
                        action = Some(DashboardAction::ButtonPressed(widget.id));
                    }
                }
                WidgetType::Label { value } => {
                    ui.label(value.as_str());
                }
            }

            // Remove button
            if ui.small_button("✖").clicked() {
                action = Some(DashboardAction::RemoveWidget(widget.id));
            }
        });

        action
    }
}

/// Actions that can be triggered by the dashboard
#[derive(Debug, Clone)]
pub enum DashboardAction {
    AddWidget,
    RemoveWidget(u64),
    ValueChanged(u64, f32),
    BoolChanged(u64, bool),
    XYChanged(u64, f32, f32),
    ButtonPressed(u64),
}
