//! Phase 6: Media Browser UI
//!
//! File browsing interface with thumbnails, search/filter, drag-and-drop support,
//! color coding, and hover preview playback.

use egui::{Color32, Response, Sense, Ui, Vec2};
use parking_lot::RwLock;
use std::collections::HashMap;
use std::path::{Path, PathBuf};
use std::sync::Arc;
use std::time::Instant;

/// Media file entry in the browser
#[derive(Debug, Clone)]
pub struct MediaEntry {
    pub path: PathBuf,
    pub name: String,
    pub file_type: MediaType,
    pub size_bytes: u64,
    pub duration_secs: Option<f32>,
    pub thumbnail: Option<ThumbnailHandle>,
    pub color_tag: Option<Color32>,
    pub tags: Vec<String>,
}

/// Media type classification
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum MediaType {
    Video,
    Image,
    ImageSequence,
    Audio,
    Unknown,
}

impl MediaType {
    pub fn from_extension(ext: &str) -> Self {
        match ext.to_lowercase().as_str() {
            "mp4" | "mov" | "avi" | "mpeg" | "mpg" | "mkv" | "webm" => Self::Video,
            "png" | "jpg" | "jpeg" | "tiff" | "tif" | "bmp" | "dds" => Self::Image,
            "gif" => Self::ImageSequence,
            "wav" | "mp3" | "aac" | "flac" | "ogg" => Self::Audio,
            _ => Self::Unknown,
        }
    }

    pub fn icon(&self) -> &'static str {
        match self {
            Self::Video => "🎬",
            Self::Image => "🖼",
            Self::ImageSequence => "🎞",
            Self::Audio => "🎵",
            Self::Unknown => "📄",
        }
    }
}

/// Thumbnail handle (reference to generated thumbnail)
#[derive(Debug, Clone)]
pub struct ThumbnailHandle {
    pub texture_id: egui::TextureId,
    pub size: (u32, u32),
}

/// Media browser state
pub struct MediaBrowser {
    /// Current directory
    current_dir: PathBuf,
    /// Media entries in current directory
    entries: Vec<MediaEntry>,
    /// Search query
    search_query: String,
    /// Filter by type
    filter_type: Option<MediaType>,
    /// View mode
    view_mode: ViewMode,
    /// Grid size (thumbnails per row)
    #[allow(dead_code)]
    grid_columns: usize,
    /// Thumbnail size in pixels
    thumbnail_size: f32,
    /// Selected entry index
    selected: Option<usize>,
    /// Hovered entry (for preview)
    hovered: Option<usize>,
    /// Hover start time (for delayed preview)
    hover_start: Option<Instant>,
    /// Preview delay in seconds
    preview_delay: f32,
    /// Thumbnail cache
    thumbnail_cache: Arc<RwLock<HashMap<PathBuf, ThumbnailHandle>>>,
    /// Show hidden files
    show_hidden: bool,
    /// Sort mode
    sort_mode: SortMode,
    /// Directory history (for back/forward navigation)
    history: Vec<PathBuf>,
    history_index: usize,
}

#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum ViewMode {
    Grid,
    List,
}

#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum SortMode {
    Name,
    Type,
    Size,
    DateModified,
}

impl MediaBrowser {
    pub fn new(initial_dir: PathBuf) -> Self {
        let mut browser = Self {
            current_dir: initial_dir.clone(),
            entries: Vec::new(),
            search_query: String::new(),
            filter_type: None,
            view_mode: ViewMode::Grid,
            grid_columns: 4,
            thumbnail_size: 120.0,
            selected: None,
            hovered: None,
            hover_start: None,
            preview_delay: 0.5,
            thumbnail_cache: Arc::new(RwLock::new(HashMap::new())),
            show_hidden: false,
            sort_mode: SortMode::Name,
            history: vec![initial_dir],
            history_index: 0,
        };
        browser.refresh();
        browser
    }

    /// Refresh the file list
    pub fn refresh(&mut self) {
        self.entries.clear();
        if let Ok(entries) = std::fs::read_dir(&self.current_dir) {
            for entry in entries.flatten() {
                if let Ok(metadata) = entry.metadata() {
                    if metadata.is_file() {
                        let path = entry.path();
                        let name = entry
                            .file_name()
                            .to_string_lossy()
                            .to_string();

                        // Skip hidden files if not showing them
                        if !self.show_hidden && name.starts_with('.') {
                            continue;
                        }

                        let file_type = path
                            .extension()
                            .and_then(|e| e.to_str())
                            .map(MediaType::from_extension)
                            .unwrap_or(MediaType::Unknown);

                        // Only include media files
                        if matches!(
                            file_type,
                            MediaType::Video | MediaType::Image | MediaType::ImageSequence | MediaType::Audio
                        ) {
                            let thumbnail = self.get_or_generate_thumbnail(&path);

                            self.entries.push(MediaEntry {
                                path,
                                name,
                                file_type,
                                size_bytes: metadata.len(),
                                duration_secs: None, // TODO: Extract from media file
                                thumbnail,
                                color_tag: None,
                                tags: Vec::new(),
                            });
                        }
                    }
                }
            }
        }

        self.sort_entries();
    }

    /// Sort entries based on sort mode
    fn sort_entries(&mut self) {
        match self.sort_mode {
            SortMode::Name => self.entries.sort_by(|a, b| a.name.cmp(&b.name)),
            SortMode::Type => self.entries.sort_by_key(|e| e.file_type as u8),
            SortMode::Size => self.entries.sort_by_key(|e| e.size_bytes),
            SortMode::DateModified => {
                // Would need to store modification time
            }
        }
    }

    /// Get or generate thumbnail for a file
    fn get_or_generate_thumbnail(&self, path: &Path) -> Option<ThumbnailHandle> {
        // Check cache first
        if let Some(thumb) = self.thumbnail_cache.read().get(path) {
            return Some(thumb.clone());
        }

        // TODO: Generate thumbnail in background
        // For now, return None - thumbnails will be generated asynchronously
        None
    }

    /// Navigate to a directory
    pub fn navigate_to(&mut self, path: PathBuf) {
        if path.is_dir() {
            self.current_dir = path.clone();
            self.refresh();

            // Update history
            self.history.truncate(self.history_index + 1);
            self.history.push(path);
            self.history_index = self.history.len() - 1;
        }
    }

    /// Navigate back in history
    pub fn navigate_back(&mut self) {
        if self.history_index > 0 {
            self.history_index -= 1;
            self.current_dir = self.history[self.history_index].clone();
            self.refresh();
        }
    }

    /// Navigate forward in history
    pub fn navigate_forward(&mut self) {
        if self.history_index < self.history.len() - 1 {
            self.history_index += 1;
            self.current_dir = self.history[self.history_index].clone();
            self.refresh();
        }
    }

    /// Navigate to parent directory
    pub fn navigate_up(&mut self) {
        if let Some(parent) = self.current_dir.parent() {
            self.navigate_to(parent.to_path_buf());
        }
    }

    /// Get filtered and searched entries
    fn filtered_entries(&self) -> Vec<(usize, &MediaEntry)> {
        self.entries
            .iter()
            .enumerate()
            .filter(|(_, entry)| {
                // Filter by type
                if let Some(filter) = self.filter_type {
                    if entry.file_type != filter {
                        return false;
                    }
                }

                // Filter by search query
                if !self.search_query.is_empty() {
                    let query = self.search_query.to_lowercase();
                    let name_matches = entry.name.to_lowercase().contains(&query);
                    let tag_matches = entry.tags.iter().any(|t| t.to_lowercase().contains(&query));
                    if !name_matches && !tag_matches {
                        return false;
                    }
                }

                true
            })
            .collect()
    }

    /// Render the media browser UI
    pub fn ui(&mut self, ui: &mut Ui) -> Option<MediaBrowserAction> {
        let mut action = None;

        // Toolbar
        ui.horizontal(|ui| {
            // Navigation buttons
            ui.add_enabled_ui(self.history_index > 0, |ui| {
                if ui.button("◀ Back").clicked() {
                    self.navigate_back();
                }
            });

            ui.add_enabled_ui(self.history_index < self.history.len() - 1, |ui| {
                if ui.button("Forward ▶").clicked() {
                    self.navigate_forward();
                }
            });

            if ui.button("⬆ Up").clicked() {
                self.navigate_up();
            }

            if ui.button("🔄 Refresh").clicked() {
                self.refresh();
            }

            ui.separator();

            // Path display
            ui.label("Path:");
            ui.label(self.current_dir.display().to_string());
        });

        ui.separator();

        // Search and filter bar
        ui.horizontal(|ui| {
            ui.label("🔍");
            let search_response = ui.text_edit_singleline(&mut self.search_query);
            if search_response.changed() {
                // Search query changed
            }

            ui.separator();

            ui.label("Filter:");
            ui.selectable_value(&mut self.filter_type, None, "All");
            ui.selectable_value(&mut self.filter_type, Some(MediaType::Video), "Video");
            ui.selectable_value(&mut self.filter_type, Some(MediaType::Image), "Image");
            ui.selectable_value(&mut self.filter_type, Some(MediaType::Audio), "Audio");

            ui.separator();

            // View mode
            ui.selectable_value(&mut self.view_mode, ViewMode::Grid, "Grid");
            ui.selectable_value(&mut self.view_mode, ViewMode::List, "List");

            ui.separator();

            // Sort mode
            egui::ComboBox::from_label("Sort")
                .selected_text(format!("{:?}", self.sort_mode))
                .show_ui(ui, |ui| {
                    ui.selectable_value(&mut self.sort_mode, SortMode::Name, "Name");
                    ui.selectable_value(&mut self.sort_mode, SortMode::Type, "Type");
                    ui.selectable_value(&mut self.sort_mode, SortMode::Size, "Size");
                });
        });

        ui.separator();

        // Content area
        egui::ScrollArea::vertical().show(ui, |ui| {
            // Collect indices to avoid borrowing issues
            let entry_indices: Vec<usize> = self.filtered_entries().into_iter().map(|(i, _)| i).collect();

            match self.view_mode {
                ViewMode::Grid => {
                    action = self.render_grid_view(ui, &entry_indices);
                }
                ViewMode::List => {
                    action = self.render_list_view(ui, &entry_indices);
                }
            }
        });

        action
    }

    /// Render grid view
    fn render_grid_view(
        &mut self,
        ui: &mut Ui,
        entry_indices: &[usize],
    ) -> Option<MediaBrowserAction> {
        let mut action = None;
        let item_size = Vec2::new(self.thumbnail_size, self.thumbnail_size + 40.0);
        let available_width = ui.available_width();
        let columns = (available_width / (item_size.x + 8.0)).floor().max(1.0) as usize;

        egui::Grid::new("media_grid")
            .spacing([8.0, 8.0])
            .min_col_width(item_size.x)
            .show(ui, |ui| {
                for (i, &idx) in entry_indices.iter().enumerate() {
                    if i > 0 && i % columns == 0 {
                        ui.end_row();
                    }

                    let entry = &self.entries[idx];
                    let response = self.render_thumbnail_item(ui, entry, idx);

                    if response.clicked() {
                        self.selected = Some(idx);
                        action = Some(MediaBrowserAction::FileSelected(entry.path.clone()));
                    }

                    if response.double_clicked() {
                        action = Some(MediaBrowserAction::FileDoubleClicked(entry.path.clone()));
                    }

                    if response.hovered() {
                        if self.hovered != Some(idx) {
                            self.hovered = Some(idx);
                            self.hover_start = Some(Instant::now());
                        }
                    }
                }
            });

        // Check for preview trigger
        if let Some(hover_time) = self.hover_start {
            if hover_time.elapsed().as_secs_f32() > self.preview_delay {
                if let Some(hovered_idx) = self.hovered {
                    if hovered_idx < self.entries.len() {
                        let entry = &self.entries[hovered_idx];
                        action = Some(MediaBrowserAction::StartPreview(entry.path.clone()));
                    }
                }
            }
        }

        action
    }

    /// Render list view
    fn render_list_view(
        &mut self,
        ui: &mut Ui,
        entry_indices: &[usize],
    ) -> Option<MediaBrowserAction> {
        let mut action = None;

        for &idx in entry_indices {
            let entry = &self.entries[idx];
            ui.horizontal(|ui| {
                // Icon
                ui.label(entry.file_type.icon());

                // Color tag
                if let Some(color) = entry.color_tag {
                    ui.colored_label(color, "●");
                }

                // Name (clickable)
                let name_label = ui.selectable_label(self.selected == Some(idx), &entry.name);
                if name_label.clicked() {
                    self.selected = Some(idx);
                    action = Some(MediaBrowserAction::FileSelected(entry.path.clone()));
                }
                if name_label.double_clicked() {
                    action = Some(MediaBrowserAction::FileDoubleClicked(entry.path.clone()));
                }

                // Size
                ui.label(format_size(entry.size_bytes));

                // Duration
                if let Some(duration) = entry.duration_secs {
                    ui.label(format_duration(duration));
                }
            });
        }

        action
    }

    /// Render a thumbnail item
    fn render_thumbnail_item(&self, ui: &mut Ui, entry: &MediaEntry, idx: usize) -> Response {
        let size = Vec2::new(self.thumbnail_size, self.thumbnail_size + 40.0);
        let (rect, response) = ui.allocate_exact_size(size, Sense::click());

        if ui.is_rect_visible(rect) {
            let visuals = ui.style().interact(&response);

            // Background
            let bg_color = if self.selected == Some(idx) {
                Color32::from_rgb(60, 120, 200)
            } else if response.hovered() {
                Color32::from_rgb(50, 50, 50)
            } else {
                Color32::from_rgb(35, 35, 35)
            };

            ui.painter().rect_filled(rect, 2.0, bg_color);

            // Thumbnail area
            let thumb_rect = egui::Rect::from_min_size(
                rect.min,
                Vec2::new(self.thumbnail_size, self.thumbnail_size),
            );

            if let Some(thumbnail) = &entry.thumbnail {
                // Render thumbnail texture
                ui.painter().image(
                    thumbnail.texture_id,
                    thumb_rect,
                    egui::Rect::from_min_max(egui::pos2(0.0, 0.0), egui::pos2(1.0, 1.0)),
                    Color32::WHITE,
                );
            } else {
                // Placeholder
                ui.painter().rect_filled(thumb_rect, 2.0, Color32::from_rgb(45, 45, 45));
                let icon_pos = thumb_rect.center() - Vec2::new(20.0, 20.0);
                ui.painter().text(
                    icon_pos,
                    egui::Align2::LEFT_TOP,
                    entry.file_type.icon(),
                    egui::FontId::proportional(40.0),
                    Color32::from_rgb(100, 100, 100),
                );
            }

            // Color tag indicator
            if let Some(color) = entry.color_tag {
                let tag_rect = egui::Rect::from_min_size(
                    thumb_rect.min + Vec2::new(4.0, 4.0),
                    Vec2::new(12.0, 12.0),
                );
                ui.painter().circle_filled(tag_rect.center(), 6.0, color);
            }

            // Name label
            let name_rect = egui::Rect::from_min_size(
                rect.min + Vec2::new(0.0, self.thumbnail_size),
                Vec2::new(self.thumbnail_size, 40.0),
            );
            ui.painter().text(
                name_rect.center_top() + Vec2::new(0.0, 4.0),
                egui::Align2::CENTER_TOP,
                &entry.name,
                egui::FontId::proportional(12.0),
                visuals.text_color(),
            );
        }

        response
    }
}

/// Actions that can be triggered by the media browser
#[derive(Debug, Clone)]
pub enum MediaBrowserAction {
    FileSelected(PathBuf),
    FileDoubleClicked(PathBuf),
    StartPreview(PathBuf),
    StopPreview,
}

/// Format file size for display
fn format_size(bytes: u64) -> String {
    const UNITS: &[&str] = &["B", "KB", "MB", "GB", "TB"];
    let mut size = bytes as f64;
    let mut unit_idx = 0;

    while size >= 1024.0 && unit_idx < UNITS.len() - 1 {
        size /= 1024.0;
        unit_idx += 1;
    }

    format!("{:.1} {}", size, UNITS[unit_idx])
}

/// Format duration for display
fn format_duration(seconds: f32) -> String {
    let minutes = (seconds / 60.0).floor() as u32;
    let secs = (seconds % 60.0).floor() as u32;
    format!("{:02}:{:02}", minutes, secs)
}
