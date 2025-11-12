//! Phase 6: Theme System
//!
//! Professional theme support with dark, light, and high-contrast modes.
//! Includes accessibility features and customizable color schemes.

use egui::{Color32, Style, Visuals};
use serde::{Deserialize, Serialize};

#[derive(Debug, Clone, Copy, PartialEq, Eq, Serialize, Deserialize)]
pub enum Theme {
    /// Dark theme (default for professional video applications)
    Dark,
    /// Light theme
    Light,
    /// High contrast for accessibility
    HighContrast,
    /// Custom theme
    Custom,
}

impl Default for Theme {
    fn default() -> Self {
        Self::Dark
    }
}

/// Theme configuration
#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct ThemeConfig {
    pub theme: Theme,
    pub custom_colors: Option<CustomColors>,
    pub font_size: f32,
    pub spacing: f32,
}

impl Default for ThemeConfig {
    fn default() -> Self {
        Self {
            theme: Theme::Dark,
            custom_colors: None,
            font_size: 14.0,
            spacing: 4.0,
        }
    }
}

/// Custom color scheme
#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct CustomColors {
    pub background: [u8; 4],
    pub panel_background: [u8; 4],
    pub text: [u8; 4],
    pub accent: [u8; 4],
    pub warning: [u8; 4],
    pub error: [u8; 4],
}

impl ThemeConfig {
    /// Apply theme to egui context
    pub fn apply(&self, ctx: &egui::Context) {
        let mut style = Style::default();
        let visuals = match self.theme {
            Theme::Dark => Self::dark_visuals(),
            Theme::Light => Self::light_visuals(),
            Theme::HighContrast => Self::high_contrast_visuals(),
            Theme::Custom => self.custom_visuals(),
        };

        style.visuals = visuals;
        style.spacing.item_spacing = egui::vec2(self.spacing * 2.0, self.spacing);
        style.spacing.button_padding = egui::vec2(self.spacing * 2.0, self.spacing);

        ctx.set_style(style);
    }

    /// Dark theme visuals (professional video application style)
    fn dark_visuals() -> Visuals {
        Visuals {
            dark_mode: true,
            override_text_color: Some(Color32::from_rgb(220, 220, 220)),
            widgets: egui::style::Widgets {
                noninteractive: egui::style::WidgetVisuals {
                    bg_fill: Color32::from_rgb(30, 30, 30),
                    weak_bg_fill: Color32::from_rgb(25, 25, 25),
                    bg_stroke: egui::Stroke::new(1.0, Color32::from_rgb(50, 50, 50)),
                    fg_stroke: egui::Stroke::new(1.0, Color32::from_rgb(180, 180, 180)),
                    rounding: egui::Rounding::same(2.0),
                    expansion: 0.0,
                },
                inactive: egui::style::WidgetVisuals {
                    bg_fill: Color32::from_rgb(40, 40, 40),
                    weak_bg_fill: Color32::from_rgb(35, 35, 35),
                    bg_stroke: egui::Stroke::new(1.0, Color32::from_rgb(60, 60, 60)),
                    fg_stroke: egui::Stroke::new(1.0, Color32::from_rgb(200, 200, 200)),
                    rounding: egui::Rounding::same(2.0),
                    expansion: 0.0,
                },
                hovered: egui::style::WidgetVisuals {
                    bg_fill: Color32::from_rgb(50, 50, 50),
                    weak_bg_fill: Color32::from_rgb(45, 45, 45),
                    bg_stroke: egui::Stroke::new(1.0, Color32::from_rgb(80, 80, 80)),
                    fg_stroke: egui::Stroke::new(1.5, Color32::from_rgb(220, 220, 220)),
                    rounding: egui::Rounding::same(2.0),
                    expansion: 1.0,
                },
                active: egui::style::WidgetVisuals {
                    bg_fill: Color32::from_rgb(60, 120, 200),
                    weak_bg_fill: Color32::from_rgb(55, 110, 180),
                    bg_stroke: egui::Stroke::new(1.0, Color32::from_rgb(80, 140, 220)),
                    fg_stroke: egui::Stroke::new(2.0, Color32::WHITE),
                    rounding: egui::Rounding::same(2.0),
                    expansion: 1.0,
                },
                open: egui::style::WidgetVisuals {
                    bg_fill: Color32::from_rgb(45, 45, 45),
                    weak_bg_fill: Color32::from_rgb(40, 40, 40),
                    bg_stroke: egui::Stroke::new(1.0, Color32::from_rgb(70, 70, 70)),
                    fg_stroke: egui::Stroke::new(1.0, Color32::from_rgb(210, 210, 210)),
                    rounding: egui::Rounding::same(2.0),
                    expansion: 0.0,
                },
            },
            selection: egui::style::Selection {
                bg_fill: Color32::from_rgb(60, 120, 200).linear_multiply(0.4),
                stroke: egui::Stroke::new(1.0, Color32::from_rgb(80, 140, 220)),
            },
            hyperlink_color: Color32::from_rgb(100, 150, 255),
            faint_bg_color: Color32::from_rgb(20, 20, 20),
            extreme_bg_color: Color32::from_rgb(10, 10, 10),
            code_bg_color: Color32::from_rgb(35, 35, 35),
            warn_fg_color: Color32::from_rgb(255, 200, 100),
            error_fg_color: Color32::from_rgb(255, 100, 100),
            window_fill: Color32::from_rgb(25, 25, 25),
            panel_fill: Color32::from_rgb(30, 30, 30),
            window_stroke: egui::Stroke::new(1.0, Color32::from_rgb(50, 50, 50)),
            ..Default::default()
        }
    }

    /// Light theme visuals
    fn light_visuals() -> Visuals {
        Visuals {
            dark_mode: false,
            override_text_color: Some(Color32::from_rgb(30, 30, 30)),
            widgets: egui::style::Widgets {
                noninteractive: egui::style::WidgetVisuals {
                    bg_fill: Color32::from_rgb(240, 240, 240),
                    weak_bg_fill: Color32::from_rgb(245, 245, 245),
                    bg_stroke: egui::Stroke::new(1.0, Color32::from_rgb(200, 200, 200)),
                    fg_stroke: egui::Stroke::new(1.0, Color32::from_rgb(60, 60, 60)),
                    rounding: egui::Rounding::same(2.0),
                    expansion: 0.0,
                },
                inactive: egui::style::WidgetVisuals {
                    bg_fill: Color32::from_rgb(230, 230, 230),
                    weak_bg_fill: Color32::from_rgb(235, 235, 235),
                    bg_stroke: egui::Stroke::new(1.0, Color32::from_rgb(190, 190, 190)),
                    fg_stroke: egui::Stroke::new(1.0, Color32::from_rgb(50, 50, 50)),
                    rounding: egui::Rounding::same(2.0),
                    expansion: 0.0,
                },
                hovered: egui::style::WidgetVisuals {
                    bg_fill: Color32::from_rgb(220, 220, 220),
                    weak_bg_fill: Color32::from_rgb(225, 225, 225),
                    bg_stroke: egui::Stroke::new(1.0, Color32::from_rgb(170, 170, 170)),
                    fg_stroke: egui::Stroke::new(1.5, Color32::from_rgb(30, 30, 30)),
                    rounding: egui::Rounding::same(2.0),
                    expansion: 1.0,
                },
                active: egui::style::WidgetVisuals {
                    bg_fill: Color32::from_rgb(60, 120, 200),
                    weak_bg_fill: Color32::from_rgb(70, 130, 210),
                    bg_stroke: egui::Stroke::new(1.0, Color32::from_rgb(50, 110, 190)),
                    fg_stroke: egui::Stroke::new(2.0, Color32::WHITE),
                    rounding: egui::Rounding::same(2.0),
                    expansion: 1.0,
                },
                open: egui::style::WidgetVisuals {
                    bg_fill: Color32::from_rgb(235, 235, 235),
                    weak_bg_fill: Color32::from_rgb(240, 240, 240),
                    bg_stroke: egui::Stroke::new(1.0, Color32::from_rgb(180, 180, 180)),
                    fg_stroke: egui::Stroke::new(1.0, Color32::from_rgb(40, 40, 40)),
                    rounding: egui::Rounding::same(2.0),
                    expansion: 0.0,
                },
            },
            selection: egui::style::Selection {
                bg_fill: Color32::from_rgb(60, 120, 200).linear_multiply(0.3),
                stroke: egui::Stroke::new(1.0, Color32::from_rgb(50, 110, 190)),
            },
            hyperlink_color: Color32::from_rgb(50, 100, 200),
            faint_bg_color: Color32::from_rgb(250, 250, 250),
            extreme_bg_color: Color32::WHITE,
            code_bg_color: Color32::from_rgb(235, 235, 235),
            warn_fg_color: Color32::from_rgb(200, 120, 0),
            error_fg_color: Color32::from_rgb(200, 0, 0),
            window_fill: Color32::from_rgb(245, 245, 245),
            panel_fill: Color32::from_rgb(240, 240, 240),
            window_stroke: egui::Stroke::new(1.0, Color32::from_rgb(200, 200, 200)),
            ..Default::default()
        }
    }

    /// High contrast visuals for accessibility
    fn high_contrast_visuals() -> Visuals {
        Visuals {
            dark_mode: true,
            override_text_color: Some(Color32::WHITE),
            widgets: egui::style::Widgets {
                noninteractive: egui::style::WidgetVisuals {
                    bg_fill: Color32::BLACK,
                    weak_bg_fill: Color32::from_rgb(10, 10, 10),
                    bg_stroke: egui::Stroke::new(2.0, Color32::WHITE),
                    fg_stroke: egui::Stroke::new(2.0, Color32::WHITE),
                    rounding: egui::Rounding::same(0.0),
                    expansion: 0.0,
                },
                inactive: egui::style::WidgetVisuals {
                    bg_fill: Color32::from_rgb(20, 20, 20),
                    weak_bg_fill: Color32::from_rgb(15, 15, 15),
                    bg_stroke: egui::Stroke::new(2.0, Color32::from_rgb(200, 200, 200)),
                    fg_stroke: egui::Stroke::new(2.0, Color32::WHITE),
                    rounding: egui::Rounding::same(0.0),
                    expansion: 0.0,
                },
                hovered: egui::style::WidgetVisuals {
                    bg_fill: Color32::from_rgb(50, 50, 50),
                    weak_bg_fill: Color32::from_rgb(40, 40, 40),
                    bg_stroke: egui::Stroke::new(3.0, Color32::from_rgb(255, 255, 0)),
                    fg_stroke: egui::Stroke::new(2.0, Color32::WHITE),
                    rounding: egui::Rounding::same(0.0),
                    expansion: 2.0,
                },
                active: egui::style::WidgetVisuals {
                    bg_fill: Color32::from_rgb(0, 200, 255),
                    weak_bg_fill: Color32::from_rgb(0, 180, 230),
                    bg_stroke: egui::Stroke::new(3.0, Color32::WHITE),
                    fg_stroke: egui::Stroke::new(3.0, Color32::BLACK),
                    rounding: egui::Rounding::same(0.0),
                    expansion: 2.0,
                },
                open: egui::style::WidgetVisuals {
                    bg_fill: Color32::from_rgb(30, 30, 30),
                    weak_bg_fill: Color32::from_rgb(25, 25, 25),
                    bg_stroke: egui::Stroke::new(2.0, Color32::from_rgb(220, 220, 220)),
                    fg_stroke: egui::Stroke::new(2.0, Color32::WHITE),
                    rounding: egui::Rounding::same(0.0),
                    expansion: 0.0,
                },
            },
            selection: egui::style::Selection {
                bg_fill: Color32::from_rgb(0, 200, 255),
                stroke: egui::Stroke::new(3.0, Color32::WHITE),
            },
            hyperlink_color: Color32::from_rgb(100, 200, 255),
            faint_bg_color: Color32::BLACK,
            extreme_bg_color: Color32::BLACK,
            code_bg_color: Color32::from_rgb(20, 20, 20),
            warn_fg_color: Color32::from_rgb(255, 255, 0),
            error_fg_color: Color32::from_rgb(255, 50, 50),
            window_fill: Color32::BLACK,
            panel_fill: Color32::from_rgb(10, 10, 10),
            window_stroke: egui::Stroke::new(3.0, Color32::WHITE),
            ..Default::default()
        }
    }

    /// Custom theme visuals
    fn custom_visuals(&self) -> Visuals {
        if let Some(colors) = &self.custom_colors {
            let bg = Color32::from_rgba_premultiplied(
                colors.background[0],
                colors.background[1],
                colors.background[2],
                colors.background[3],
            );
            let panel = Color32::from_rgba_premultiplied(
                colors.panel_background[0],
                colors.panel_background[1],
                colors.panel_background[2],
                colors.panel_background[3],
            );
            let text = Color32::from_rgba_premultiplied(
                colors.text[0],
                colors.text[1],
                colors.text[2],
                colors.text[3],
            );
            let accent = Color32::from_rgba_premultiplied(
                colors.accent[0],
                colors.accent[1],
                colors.accent[2],
                colors.accent[3],
            );

            let mut visuals = Self::dark_visuals();
            visuals.override_text_color = Some(text);
            visuals.window_fill = bg;
            visuals.panel_fill = panel;
            visuals.widgets.active.bg_fill = accent;
            visuals
        } else {
            Self::dark_visuals()
        }
    }
}

/// Theme picker widget
pub fn theme_picker(ui: &mut egui::Ui, theme: &mut Theme) -> bool {
    let mut changed = false;

    ui.label("Theme:");
    ui.horizontal(|ui| {
        changed |= ui
            .selectable_value(theme, Theme::Dark, "Dark")
            .clicked();
        changed |= ui
            .selectable_value(theme, Theme::Light, "Light")
            .clicked();
        changed |= ui
            .selectable_value(theme, Theme::HighContrast, "High Contrast")
            .clicked();
    });

    changed
}
