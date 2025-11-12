//! Keyboard shortcut definitions and actions

use serde::{Deserialize, Serialize};

/// Keyboard key codes
#[derive(Debug, Clone, Copy, PartialEq, Eq, Hash, Serialize, Deserialize)]
pub enum Key {
    // Letters
    A, B, C, D, E, F, G, H, I, J, K, L, M,
    N, O, P, Q, R, S, T, U, V, W, X, Y, Z,

    // Numbers
    Key0, Key1, Key2, Key3, Key4, Key5, Key6, Key7, Key8, Key9,

    // Function keys
    F1, F2, F3, F4, F5, F6, F7, F8, F9, F10, F11, F12,

    // Special keys
    Space,
    Enter,
    Escape,
    Tab,
    Backspace,
    Delete,
    Insert,
    Home,
    End,
    PageUp,
    PageDown,

    // Arrow keys
    ArrowUp,
    ArrowDown,
    ArrowLeft,
    ArrowRight,

    // Symbols
    Minus,
    Plus,
    LeftBracket,
    RightBracket,
    Semicolon,
    Quote,
    Comma,
    Period,
    Slash,
    Backslash,
}

/// Keyboard modifiers
#[derive(Debug, Clone, Default, PartialEq, Eq, Hash, Serialize, Deserialize)]
pub struct Modifiers {
    pub ctrl: bool,
    pub alt: bool,
    pub shift: bool,
    pub meta: bool, // Command on macOS, Windows key on Windows
}

impl Modifiers {
    pub fn new() -> Self {
        Self::default()
    }

    pub fn ctrl() -> Self {
        Self {
            ctrl: true,
            ..Default::default()
        }
    }

    pub fn alt() -> Self {
        Self {
            alt: true,
            ..Default::default()
        }
    }

    pub fn shift() -> Self {
        Self {
            shift: true,
            ..Default::default()
        }
    }

    pub fn ctrl_shift() -> Self {
        Self {
            ctrl: true,
            shift: true,
            ..Default::default()
        }
    }

    pub fn is_empty(&self) -> bool {
        !self.ctrl && !self.alt && !self.shift && !self.meta
    }
}

/// Shortcut action types
#[derive(Debug, Clone, PartialEq, Serialize, Deserialize)]
pub enum Action {
    // Playback controls
    Play,
    Pause,
    Stop,
    TogglePlayPause,
    Rewind,
    FastForward,
    FrameForward,
    FrameBackward,

    // Speed control
    IncreaseSpeed,
    DecreaseSpeed,
    ResetSpeed,
    HalfSpeed,
    DoubleSpeed,

    // Cue system
    NextCue,
    PrevCue,
    GotoCue(u32),
    RecordCue,

    // Layer control
    ToggleLayerVisibility(u32),
    SelectLayer(u32),
    NextLayer,
    PrevLayer,
    DeleteLayer,

    // Output control
    ToggleOutput(u32),
    ToggleAllOutputs,
    ToggleFullscreen,

    // View control
    ZoomIn,
    ZoomOut,
    ZoomReset,
    FitToView,

    // File operations
    NewProject,
    OpenProject,
    SaveProject,
    SaveProjectAs,

    // Edit operations
    Undo,
    Redo,
    Cut,
    Copy,
    Paste,
    Delete,
    SelectAll,

    // Application
    Quit,
    ToggleUI,
    ShowPreferences,
    ShowHelp,

    // Macros
    ExecuteMacro(String),

    // Custom action
    Custom(String),
}

/// Shortcut context - where the shortcut is active
#[derive(Debug, Clone, Copy, PartialEq, Eq, Hash, Serialize, Deserialize)]
pub enum ShortcutContext {
    /// Always active
    Global,
    /// Only in main window
    MainWindow,
    /// Only in output windows
    OutputWindow,
    /// Only when editing
    Editor,
    /// Only when timeline is focused
    Timeline,
    /// Only when layer panel is focused
    LayerPanel,
}

/// A keyboard shortcut definition
#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct Shortcut {
    pub key: Key,
    pub modifiers: Modifiers,
    pub action: Action,
    pub context: ShortcutContext,
    pub description: String,
    pub enabled: bool,
}

impl Shortcut {
    pub fn new(
        key: Key,
        modifiers: Modifiers,
        action: Action,
        context: ShortcutContext,
        description: String,
    ) -> Self {
        Self {
            key,
            modifiers,
            action,
            context,
            description,
            enabled: true,
        }
    }

    /// Check if this shortcut matches the given key and modifiers
    pub fn matches(&self, key: Key, modifiers: &Modifiers) -> bool {
        self.enabled && self.key == key && self.modifiers == *modifiers
    }

    /// Get a human-readable string representation
    pub fn to_shortcut_string(&self) -> String {
        let mut parts = Vec::new();

        if self.modifiers.ctrl {
            parts.push("Ctrl".to_string());
        }
        if self.modifiers.alt {
            parts.push("Alt".to_string());
        }
        if self.modifiers.shift {
            parts.push("Shift".to_string());
        }
        if self.modifiers.meta {
            #[cfg(target_os = "macos")]
            parts.push("Cmd".to_string());
            #[cfg(not(target_os = "macos"))]
            parts.push("Win".to_string());
        }

        parts.push(format!("{:?}", self.key));

        parts.join("+")
    }
}

/// Default shortcuts for MapMap
pub struct DefaultShortcuts;

impl DefaultShortcuts {
    pub fn all() -> Vec<Shortcut> {
        vec![
            // Playback
            Shortcut::new(
                Key::Space,
                Modifiers::new(),
                Action::TogglePlayPause,
                ShortcutContext::Global,
                "Toggle Play/Pause".to_string(),
            ),
            Shortcut::new(
                Key::S,
                Modifiers::new(),
                Action::Stop,
                ShortcutContext::Global,
                "Stop playback".to_string(),
            ),
            Shortcut::new(
                Key::ArrowRight,
                Modifiers::new(),
                Action::FrameForward,
                ShortcutContext::Global,
                "Next frame".to_string(),
            ),
            Shortcut::new(
                Key::ArrowLeft,
                Modifiers::new(),
                Action::FrameBackward,
                ShortcutContext::Global,
                "Previous frame".to_string(),
            ),
            // Speed
            Shortcut::new(
                Key::Plus,
                Modifiers::new(),
                Action::IncreaseSpeed,
                ShortcutContext::Global,
                "Increase playback speed".to_string(),
            ),
            Shortcut::new(
                Key::Minus,
                Modifiers::new(),
                Action::DecreaseSpeed,
                ShortcutContext::Global,
                "Decrease playback speed".to_string(),
            ),
            Shortcut::new(
                Key::Key0,
                Modifiers::new(),
                Action::ResetSpeed,
                ShortcutContext::Global,
                "Reset playback speed".to_string(),
            ),
            // Cues
            Shortcut::new(
                Key::ArrowUp,
                Modifiers::new(),
                Action::NextCue,
                ShortcutContext::Global,
                "Next cue".to_string(),
            ),
            Shortcut::new(
                Key::ArrowDown,
                Modifiers::new(),
                Action::PrevCue,
                ShortcutContext::Global,
                "Previous cue".to_string(),
            ),
            Shortcut::new(
                Key::R,
                Modifiers::ctrl(),
                Action::RecordCue,
                ShortcutContext::Global,
                "Record current state as cue".to_string(),
            ),
            // File operations
            Shortcut::new(
                Key::N,
                Modifiers::ctrl(),
                Action::NewProject,
                ShortcutContext::Global,
                "New project".to_string(),
            ),
            Shortcut::new(
                Key::O,
                Modifiers::ctrl(),
                Action::OpenProject,
                ShortcutContext::Global,
                "Open project".to_string(),
            ),
            Shortcut::new(
                Key::S,
                Modifiers::ctrl(),
                Action::SaveProject,
                ShortcutContext::Global,
                "Save project".to_string(),
            ),
            Shortcut::new(
                Key::S,
                Modifiers::ctrl_shift(),
                Action::SaveProjectAs,
                ShortcutContext::Global,
                "Save project as...".to_string(),
            ),
            // Edit operations
            Shortcut::new(
                Key::Z,
                Modifiers::ctrl(),
                Action::Undo,
                ShortcutContext::Global,
                "Undo".to_string(),
            ),
            Shortcut::new(
                Key::Y,
                Modifiers::ctrl(),
                Action::Redo,
                ShortcutContext::Global,
                "Redo".to_string(),
            ),
            Shortcut::new(
                Key::X,
                Modifiers::ctrl(),
                Action::Cut,
                ShortcutContext::Editor,
                "Cut".to_string(),
            ),
            Shortcut::new(
                Key::C,
                Modifiers::ctrl(),
                Action::Copy,
                ShortcutContext::Editor,
                "Copy".to_string(),
            ),
            Shortcut::new(
                Key::V,
                Modifiers::ctrl(),
                Action::Paste,
                ShortcutContext::Editor,
                "Paste".to_string(),
            ),
            Shortcut::new(
                Key::Delete,
                Modifiers::new(),
                Action::Delete,
                ShortcutContext::Editor,
                "Delete".to_string(),
            ),
            // View
            Shortcut::new(
                Key::F,
                Modifiers::new(),
                Action::FitToView,
                ShortcutContext::MainWindow,
                "Fit to view".to_string(),
            ),
            Shortcut::new(
                Key::F11,
                Modifiers::new(),
                Action::ToggleFullscreen,
                ShortcutContext::Global,
                "Toggle fullscreen".to_string(),
            ),
            Shortcut::new(
                Key::Tab,
                Modifiers::new(),
                Action::ToggleUI,
                ShortcutContext::Global,
                "Toggle UI".to_string(),
            ),
            // Application
            Shortcut::new(
                Key::Q,
                Modifiers::ctrl(),
                Action::Quit,
                ShortcutContext::Global,
                "Quit application".to_string(),
            ),
        ]
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_shortcut_matching() {
        let shortcut = Shortcut::new(
            Key::S,
            Modifiers::ctrl(),
            Action::SaveProject,
            ShortcutContext::Global,
            "Save".to_string(),
        );

        assert!(shortcut.matches(Key::S, &Modifiers::ctrl()));
        assert!(!shortcut.matches(Key::S, &Modifiers::new()));
        assert!(!shortcut.matches(Key::O, &Modifiers::ctrl()));
    }

    #[test]
    fn test_default_shortcuts() {
        let shortcuts = DefaultShortcuts::all();
        assert!(!shortcuts.is_empty());

        // Check for essential shortcuts
        let has_play = shortcuts
            .iter()
            .any(|s| matches!(s.action, Action::TogglePlayPause));
        assert!(has_play);

        let has_save = shortcuts
            .iter()
            .any(|s| matches!(s.action, Action::SaveProject));
        assert!(has_save);
    }

    #[test]
    fn test_shortcut_to_string() {
        let shortcut = Shortcut::new(
            Key::S,
            Modifiers::ctrl_shift(),
            Action::SaveProjectAs,
            ShortcutContext::Global,
            "Save As".to_string(),
        );

        let str_repr = shortcut.to_shortcut_string();
        assert!(str_repr.contains("Ctrl"));
        assert!(str_repr.contains("Shift"));
        assert!(str_repr.contains("S"));
    }
}
