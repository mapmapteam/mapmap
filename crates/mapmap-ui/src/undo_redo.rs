//! Phase 6: Undo/Redo Command Pattern Architecture
//!
//! This module implements a command pattern for undo/redo functionality across all MapMap operations.
//! All editor operations should be wrapped in commands to enable comprehensive undo/redo support.

use serde::{Deserialize, Serialize};
use std::collections::VecDeque;

/// Maximum number of undo/redo operations to keep in history
const MAX_HISTORY: usize = 100;

/// Core trait for all undoable commands
pub trait Command: Send + Sync + std::fmt::Debug + std::any::Any {
    /// Execute the command forward
    fn execute(&self, state: &mut EditorState) -> Result<(), CommandError>;

    /// Undo the command (reverse the operation)
    fn undo(&self, state: &mut EditorState) -> Result<(), CommandError>;

    /// Get a human-readable description of the command
    fn description(&self) -> String;

    /// Check if this command can be merged with another (for optimization)
    fn can_merge_with(&self, _other: &dyn Command) -> bool {
        false
    }
}

/// Errors that can occur during command execution
#[derive(Debug, thiserror::Error)]
pub enum CommandError {
    #[error("Command execution failed: {0}")]
    ExecutionFailed(String),

    #[error("Layer not found: {0}")]
    LayerNotFound(u64),

    #[error("Paint not found: {0}")]
    PaintNotFound(u64),

    #[error("Invalid state: {0}")]
    InvalidState(String),
}

/// Editor state snapshot for undo/redo
#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct EditorState {
    pub layers: Vec<LayerState>,
    pub master_opacity: f32,
    pub master_speed: f32,
    pub composition_name: String,
}

#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct LayerState {
    pub id: u64,
    pub name: String,
    pub opacity: f32,
    pub bypass: bool,
    pub solo: bool,
    pub paint_id: Option<u64>,
}

/// Undo/Redo manager
pub struct UndoManager {
    undo_stack: VecDeque<Box<dyn Command>>,
    redo_stack: VecDeque<Box<dyn Command>>,
    current_state: EditorState,
}

impl UndoManager {
    pub fn new(initial_state: EditorState) -> Self {
        Self {
            undo_stack: VecDeque::new(),
            redo_stack: VecDeque::new(),
            current_state: initial_state,
        }
    }

    /// Execute a command and add it to the undo stack
    pub fn execute(&mut self, command: Box<dyn Command>) -> Result<(), CommandError> {
        command.execute(&mut self.current_state)?;

        // Clear redo stack when a new command is executed
        self.redo_stack.clear();

        // Add to undo stack
        self.undo_stack.push_back(command);

        // Limit stack size
        if self.undo_stack.len() > MAX_HISTORY {
            self.undo_stack.pop_front();
        }

        Ok(())
    }

    /// Undo the last command
    pub fn undo(&mut self) -> Result<(), CommandError> {
        if let Some(command) = self.undo_stack.pop_back() {
            command.undo(&mut self.current_state)?;
            self.redo_stack.push_back(command);
            Ok(())
        } else {
            Err(CommandError::InvalidState("Nothing to undo".to_string()))
        }
    }

    /// Redo the last undone command
    pub fn redo(&mut self) -> Result<(), CommandError> {
        if let Some(command) = self.redo_stack.pop_back() {
            command.execute(&mut self.current_state)?;
            self.undo_stack.push_back(command);
            Ok(())
        } else {
            Err(CommandError::InvalidState("Nothing to redo".to_string()))
        }
    }

    /// Check if undo is available
    pub fn can_undo(&self) -> bool {
        !self.undo_stack.is_empty()
    }

    /// Check if redo is available
    pub fn can_redo(&self) -> bool {
        !self.redo_stack.is_empty()
    }

    /// Get description of the next undo operation
    pub fn undo_description(&self) -> Option<String> {
        self.undo_stack.back().map(|cmd| cmd.description())
    }

    /// Get description of the next redo operation
    pub fn redo_description(&self) -> Option<String> {
        self.redo_stack.back().map(|cmd| cmd.description())
    }

    /// Clear all history
    pub fn clear(&mut self) {
        self.undo_stack.clear();
        self.redo_stack.clear();
    }

    /// Get current editor state
    pub fn state(&self) -> &EditorState {
        &self.current_state
    }
}

// ============================================================================
// Concrete Command Implementations
// ============================================================================

/// Command: Set layer opacity
#[derive(Debug)]
pub struct SetLayerOpacityCommand {
    layer_id: u64,
    old_opacity: f32,
    new_opacity: f32,
}

impl SetLayerOpacityCommand {
    pub fn new(layer_id: u64, old_opacity: f32, new_opacity: f32) -> Self {
        Self {
            layer_id,
            old_opacity,
            new_opacity,
        }
    }
}

impl Command for SetLayerOpacityCommand {
    fn execute(&self, state: &mut EditorState) -> Result<(), CommandError> {
        let layer = state
            .layers
            .iter_mut()
            .find(|l| l.id == self.layer_id)
            .ok_or(CommandError::LayerNotFound(self.layer_id))?;
        layer.opacity = self.new_opacity;
        Ok(())
    }

    fn undo(&self, state: &mut EditorState) -> Result<(), CommandError> {
        let layer = state
            .layers
            .iter_mut()
            .find(|l| l.id == self.layer_id)
            .ok_or(CommandError::LayerNotFound(self.layer_id))?;
        layer.opacity = self.old_opacity;
        Ok(())
    }

    fn description(&self) -> String {
        format!("Set layer opacity to {:.2}", self.new_opacity)
    }

    fn can_merge_with(&self, other: &dyn Command) -> bool {
        // Merge multiple opacity changes on the same layer
        if let Some(other) = (other as &dyn std::any::Any).downcast_ref::<Self>() {
            self.layer_id == other.layer_id
        } else {
            false
        }
    }
}

/// Command: Add layer
#[derive(Debug)]
pub struct AddLayerCommand {
    layer: LayerState,
}

impl AddLayerCommand {
    pub fn new(layer: LayerState) -> Self {
        Self { layer }
    }
}

impl Command for AddLayerCommand {
    fn execute(&self, state: &mut EditorState) -> Result<(), CommandError> {
        state.layers.push(self.layer.clone());
        Ok(())
    }

    fn undo(&self, state: &mut EditorState) -> Result<(), CommandError> {
        state.layers.retain(|l| l.id != self.layer.id);
        Ok(())
    }

    fn description(&self) -> String {
        format!("Add layer '{}'", self.layer.name)
    }
}

/// Command: Remove layer
#[derive(Debug)]
pub struct RemoveLayerCommand {
    layer: LayerState,
    index: usize,
}

impl RemoveLayerCommand {
    pub fn new(layer: LayerState, index: usize) -> Self {
        Self { layer, index }
    }
}

impl Command for RemoveLayerCommand {
    fn execute(&self, state: &mut EditorState) -> Result<(), CommandError> {
        state.layers.retain(|l| l.id != self.layer.id);
        Ok(())
    }

    fn undo(&self, state: &mut EditorState) -> Result<(), CommandError> {
        state.layers.insert(self.index, self.layer.clone());
        Ok(())
    }

    fn description(&self) -> String {
        format!("Remove layer '{}'", self.layer.name)
    }
}

/// Command: Rename layer
#[derive(Debug)]
pub struct RenameLayerCommand {
    layer_id: u64,
    old_name: String,
    new_name: String,
}

impl RenameLayerCommand {
    pub fn new(layer_id: u64, old_name: String, new_name: String) -> Self {
        Self {
            layer_id,
            old_name,
            new_name,
        }
    }
}

impl Command for RenameLayerCommand {
    fn execute(&self, state: &mut EditorState) -> Result<(), CommandError> {
        let layer = state
            .layers
            .iter_mut()
            .find(|l| l.id == self.layer_id)
            .ok_or(CommandError::LayerNotFound(self.layer_id))?;
        layer.name = self.new_name.clone();
        Ok(())
    }

    fn undo(&self, state: &mut EditorState) -> Result<(), CommandError> {
        let layer = state
            .layers
            .iter_mut()
            .find(|l| l.id == self.layer_id)
            .ok_or(CommandError::LayerNotFound(self.layer_id))?;
        layer.name = self.old_name.clone();
        Ok(())
    }

    fn description(&self) -> String {
        format!("Rename layer to '{}'", self.new_name)
    }
}

/// Command: Set master opacity
#[derive(Debug)]
pub struct SetMasterOpacityCommand {
    old_opacity: f32,
    new_opacity: f32,
}

impl SetMasterOpacityCommand {
    pub fn new(old_opacity: f32, new_opacity: f32) -> Self {
        Self {
            old_opacity,
            new_opacity,
        }
    }
}

impl Command for SetMasterOpacityCommand {
    fn execute(&self, state: &mut EditorState) -> Result<(), CommandError> {
        state.master_opacity = self.new_opacity;
        Ok(())
    }

    fn undo(&self, state: &mut EditorState) -> Result<(), CommandError> {
        state.master_opacity = self.old_opacity;
        Ok(())
    }

    fn description(&self) -> String {
        format!("Set master opacity to {:.2}", self.new_opacity)
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_undo_redo_opacity() {
        let initial_state = EditorState {
            layers: vec![LayerState {
                id: 1,
                name: "Layer 1".to_string(),
                opacity: 1.0,
                bypass: false,
                solo: false,
                paint_id: None,
            }],
            master_opacity: 1.0,
            master_speed: 1.0,
            composition_name: "Test".to_string(),
        };

        let mut manager = UndoManager::new(initial_state);

        // Execute command
        let cmd = Box::new(SetLayerOpacityCommand::new(1, 1.0, 0.5));
        manager.execute(cmd).unwrap();
        assert_eq!(manager.state().layers[0].opacity, 0.5);

        // Undo
        manager.undo().unwrap();
        assert_eq!(manager.state().layers[0].opacity, 1.0);

        // Redo
        manager.redo().unwrap();
        assert_eq!(manager.state().layers[0].opacity, 0.5);
    }

    #[test]
    fn test_add_remove_layer() {
        let initial_state = EditorState {
            layers: vec![],
            master_opacity: 1.0,
            master_speed: 1.0,
            composition_name: "Test".to_string(),
        };

        let mut manager = UndoManager::new(initial_state);

        // Add layer
        let layer = LayerState {
            id: 1,
            name: "New Layer".to_string(),
            opacity: 1.0,
            bypass: false,
            solo: false,
            paint_id: None,
        };
        let cmd = Box::new(AddLayerCommand::new(layer));
        manager.execute(cmd).unwrap();
        assert_eq!(manager.state().layers.len(), 1);

        // Undo
        manager.undo().unwrap();
        assert_eq!(manager.state().layers.len(), 0);

        // Redo
        manager.redo().unwrap();
        assert_eq!(manager.state().layers.len(), 1);
    }
}
