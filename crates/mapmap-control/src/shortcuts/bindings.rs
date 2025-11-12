//! Keyboard shortcut bindings manager

use super::{Action, Key, Modifiers, Shortcut, ShortcutContext, DefaultShortcuts, Macro};
use crate::error::Result;
use std::collections::HashMap;
use std::path::Path;
use serde::{Deserialize, Serialize};
use tracing::info;

/// Key binding manager
pub struct KeyBindings {
    shortcuts: Vec<Shortcut>,
    macros: HashMap<String, Macro>,
    context: ShortcutContext,
}

/// Serializable key bindings for save/load
#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct KeyBindingsData {
    pub shortcuts: Vec<Shortcut>,
    pub macros: HashMap<String, Macro>,
}

impl KeyBindings {
    /// Create a new key bindings manager with default shortcuts
    pub fn new() -> Self {
        Self {
            shortcuts: DefaultShortcuts::all(),
            macros: HashMap::new(),
            context: ShortcutContext::Global,
        }
    }

    /// Create an empty key bindings manager
    pub fn empty() -> Self {
        Self {
            shortcuts: Vec::new(),
            macros: HashMap::new(),
            context: ShortcutContext::Global,
        }
    }

    /// Set the current context
    pub fn set_context(&mut self, context: ShortcutContext) {
        self.context = context;
    }

    /// Get the current context
    pub fn get_context(&self) -> ShortcutContext {
        self.context
    }

    /// Find action for a key press
    pub fn find_action(&self, key: Key, modifiers: &Modifiers) -> Option<Action> {
        for shortcut in &self.shortcuts {
            if !shortcut.enabled {
                continue;
            }

            // Check context match
            let context_match = match shortcut.context {
                ShortcutContext::Global => true,
                _ => shortcut.context == self.context,
            };

            if !context_match {
                continue;
            }

            if shortcut.matches(key, modifiers) {
                return Some(shortcut.action.clone());
            }
        }

        None
    }

    /// Add a new shortcut
    pub fn add_shortcut(&mut self, shortcut: Shortcut) {
        self.shortcuts.push(shortcut);
    }

    /// Remove a shortcut by index
    pub fn remove_shortcut(&mut self, index: usize) -> Option<Shortcut> {
        if index < self.shortcuts.len() {
            Some(self.shortcuts.remove(index))
        } else {
            None
        }
    }

    /// Update a shortcut by index
    pub fn update_shortcut(&mut self, index: usize, shortcut: Shortcut) -> bool {
        if index < self.shortcuts.len() {
            self.shortcuts[index] = shortcut;
            true
        } else {
            false
        }
    }

    /// Get all shortcuts
    pub fn get_shortcuts(&self) -> &[Shortcut] {
        &self.shortcuts
    }

    /// Find shortcuts for a specific action
    pub fn find_shortcuts_for_action(&self, action: &Action) -> Vec<&Shortcut> {
        self.shortcuts
            .iter()
            .filter(|s| &s.action == action)
            .collect()
    }

    /// Check if a key combination is already bound
    pub fn is_key_bound(&self, key: Key, modifiers: &Modifiers, context: ShortcutContext) -> bool {
        self.shortcuts.iter().any(|s| {
            s.enabled
                && s.key == key
                && s.modifiers == *modifiers
                && (s.context == context || s.context == ShortcutContext::Global)
        })
    }

    /// Add a macro
    pub fn add_macro(&mut self, macro_def: Macro) {
        let name = macro_def.name.clone();
        self.macros.insert(name, macro_def);
    }

    /// Remove a macro
    pub fn remove_macro(&mut self, name: &str) -> Option<Macro> {
        self.macros.remove(name)
    }

    /// Get a macro by name
    pub fn get_macro(&self, name: &str) -> Option<&Macro> {
        self.macros.get(name)
    }

    /// Get all macros
    pub fn get_macros(&self) -> &HashMap<String, Macro> {
        &self.macros
    }

    /// Reset to default shortcuts
    pub fn reset_to_defaults(&mut self) {
        self.shortcuts = DefaultShortcuts::all();
        info!("Key bindings reset to defaults");
    }

    /// Load from JSON file
    pub fn load_from_file<P: AsRef<Path>>(path: P) -> Result<Self> {
        let json = std::fs::read_to_string(path)?;
        let data: KeyBindingsData = serde_json::from_str(&json)?;

        info!("Loaded {} shortcuts and {} macros", data.shortcuts.len(), data.macros.len());

        Ok(Self {
            shortcuts: data.shortcuts,
            macros: data.macros,
            context: ShortcutContext::Global,
        })
    }

    /// Save to JSON file
    pub fn save_to_file<P: AsRef<Path>>(&self, path: P) -> Result<()> {
        let data = KeyBindingsData {
            shortcuts: self.shortcuts.clone(),
            macros: self.macros.clone(),
        };

        let json = serde_json::to_string_pretty(&data)?;
        std::fs::write(path, json)?;

        info!("Saved {} shortcuts and {} macros", self.shortcuts.len(), self.macros.len());

        Ok(())
    }

    /// Export to JSON string
    pub fn to_json(&self) -> Result<String> {
        let data = KeyBindingsData {
            shortcuts: self.shortcuts.clone(),
            macros: self.macros.clone(),
        };

        Ok(serde_json::to_string_pretty(&data)?)
    }

    /// Import from JSON string
    pub fn from_json(json: &str) -> Result<Self> {
        let data: KeyBindingsData = serde_json::from_str(json)?;

        Ok(Self {
            shortcuts: data.shortcuts,
            macros: data.macros,
            context: ShortcutContext::Global,
        })
    }
}

impl Default for KeyBindings {
    fn default() -> Self {
        Self::new()
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_find_action() {
        let bindings = KeyBindings::new();

        // Should find the default Space -> TogglePlayPause binding
        let action = bindings.find_action(Key::Space, &Modifiers::new());
        assert!(matches!(action, Some(Action::TogglePlayPause)));

        // Should find Ctrl+S -> SaveProject
        let action = bindings.find_action(Key::S, &Modifiers::ctrl());
        assert!(matches!(action, Some(Action::SaveProject)));
    }

    #[test]
    fn test_add_remove_shortcut() {
        let mut bindings = KeyBindings::empty();

        let shortcut = Shortcut::new(
            Key::T,
            Modifiers::new(),
            Action::Play,
            ShortcutContext::Global,
            "Test".to_string(),
        );

        bindings.add_shortcut(shortcut.clone());
        assert_eq!(bindings.get_shortcuts().len(), 1);

        let removed = bindings.remove_shortcut(0);
        assert!(removed.is_some());
        assert_eq!(bindings.get_shortcuts().len(), 0);
    }

    #[test]
    fn test_context_filtering() {
        let mut bindings = KeyBindings::empty();

        bindings.add_shortcut(Shortcut::new(
            Key::T,
            Modifiers::new(),
            Action::Play,
            ShortcutContext::Editor,
            "Test".to_string(),
        ));

        // Should not find when context is MainWindow
        bindings.set_context(ShortcutContext::MainWindow);
        let action = bindings.find_action(Key::T, &Modifiers::new());
        assert!(action.is_none());

        // Should find when context is Editor
        bindings.set_context(ShortcutContext::Editor);
        let action = bindings.find_action(Key::T, &Modifiers::new());
        assert!(matches!(action, Some(Action::Play)));
    }

    #[test]
    fn test_macro_management() {
        let mut bindings = KeyBindings::new();

        let macro_def = Macro {
            name: "Test Macro".to_string(),
            description: "Test".to_string(),
            actions: vec![],
            created_at: "2024-01-01T00:00:00Z".to_string(),
        };

        bindings.add_macro(macro_def.clone());
        assert!(bindings.get_macro("Test Macro").is_some());

        let removed = bindings.remove_macro("Test Macro");
        assert!(removed.is_some());
        assert!(bindings.get_macro("Test Macro").is_none());
    }

    #[test]
    fn test_serialization() {
        let bindings = KeyBindings::new();
        let json = bindings.to_json().unwrap();
        let loaded = KeyBindings::from_json(&json).unwrap();

        assert_eq!(bindings.get_shortcuts().len(), loaded.get_shortcuts().len());
    }
}
