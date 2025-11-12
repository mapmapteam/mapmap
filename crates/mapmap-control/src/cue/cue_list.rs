//! Cue list management

use std::time::Duration;

use super::cue::Cue;
use super::crossfade::Crossfade;

use crate::{error::ControlError, Result};

/// Cue list state
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum CueListState {
    Idle,
    Playing,
    Crossfading,
}

/// Cue list manager
pub struct CueList {
    cues: Vec<Cue>,
    current_cue: Option<u32>,
    next_cue: Option<u32>,
    state: CueListState,
    current_crossfade: Option<Crossfade>,
}

impl CueList {
    /// Create a new empty cue list
    pub fn new() -> Self {
        Self {
            cues: Vec::new(),
            current_cue: None,
            next_cue: None,
            state: CueListState::Idle,
            current_crossfade: None,
        }
    }

    /// Add a cue to the list
    pub fn add_cue(&mut self, cue: Cue) {
        self.cues.push(cue);
        self.sort_cues();
    }

    /// Remove a cue by ID
    pub fn remove_cue(&mut self, id: u32) -> Option<Cue> {
        if let Some(index) = self.cues.iter().position(|c| c.id == id) {
            Some(self.cues.remove(index))
        } else {
            None
        }
    }

    /// Get a cue by ID
    pub fn get_cue(&self, id: u32) -> Option<&Cue> {
        self.cues.iter().find(|c| c.id == id)
    }

    /// Get a mutable reference to a cue by ID
    pub fn get_cue_mut(&mut self, id: u32) -> Option<&mut Cue> {
        self.cues.iter_mut().find(|c| c.id == id)
    }

    /// Get all cues
    pub fn cues(&self) -> &[Cue] {
        &self.cues
    }

    /// Get the current cue ID
    pub fn current_cue(&self) -> Option<u32> {
        self.current_cue
    }

    /// Get the next cue ID
    pub fn next_cue(&self) -> Option<u32> {
        self.next_cue
    }

    /// Go to a specific cue
    pub fn goto_cue(&mut self, id: u32, fade_duration: Option<Duration>) -> Result<()> {
        let cue = self
            .get_cue(id)
            .ok_or_else(|| ControlError::TargetNotFound(format!("Cue {} not found", id)))?;

        let duration = fade_duration.unwrap_or(cue.fade_duration);
        let curve = cue.fade_curve;

        if let Some(current) = self.current_cue {
            // Start crossfade from current to target
            self.current_crossfade = Some(Crossfade::new(current, id, duration, curve));
            self.state = CueListState::Crossfading;
        } else {
            // No current cue, just set it
            self.current_cue = Some(id);
            self.state = CueListState::Playing;
        }

        self.update_next_cue();
        Ok(())
    }

    /// Go to the next cue in the list
    pub fn next(&mut self) -> Result<()> {
        if let Some(next_id) = self.next_cue {
            self.goto_cue(next_id, None)
        } else {
            Err(ControlError::InvalidParameter(
                "No next cue available".to_string(),
            ))
        }
    }

    /// Go to the previous cue in the list
    pub fn prev(&mut self) -> Result<()> {
        if let Some(current_id) = self.current_cue {
            let current_index = self
                .cues
                .iter()
                .position(|c| c.id == current_id)
                .ok_or_else(|| {
                    ControlError::TargetNotFound(format!("Current cue {} not found", current_id))
                })?;

            if current_index > 0 {
                let prev_id = self.cues[current_index - 1].id;
                self.goto_cue(prev_id, None)
            } else {
                Err(ControlError::InvalidParameter(
                    "Already at first cue".to_string(),
                ))
            }
        } else {
            Err(ControlError::InvalidParameter("No current cue".to_string()))
        }
    }

    /// Update the cue list (call this regularly to handle crossfades)
    pub fn update(&mut self) {
        if let Some(crossfade) = &self.current_crossfade {
            if crossfade.is_complete() {
                // Crossfade complete, update current cue
                self.current_cue = Some(crossfade.to_cue_id());
                self.current_crossfade = None;
                self.state = CueListState::Playing;
                self.update_next_cue();
            }
        }
    }

    /// Get the current crossfade state
    pub fn current_crossfade(&self) -> Option<&Crossfade> {
        self.current_crossfade.as_ref()
    }

    /// Get the current state
    pub fn state(&self) -> CueListState {
        self.state
    }

    /// Clear all cues
    pub fn clear(&mut self) {
        self.cues.clear();
        self.current_cue = None;
        self.next_cue = None;
        self.state = CueListState::Idle;
        self.current_crossfade = None;
    }

    /// Get the number of cues
    pub fn len(&self) -> usize {
        self.cues.len()
    }

    /// Check if the list is empty
    pub fn is_empty(&self) -> bool {
        self.cues.is_empty()
    }

    /// Sort cues by ID
    fn sort_cues(&mut self) {
        self.cues.sort_by_key(|c| c.id);
    }

    /// Update the next cue based on current cue
    fn update_next_cue(&mut self) {
        if let Some(current_id) = self.current_cue {
            if let Some(current_index) = self.cues.iter().position(|c| c.id == current_id) {
                if current_index + 1 < self.cues.len() {
                    self.next_cue = Some(self.cues[current_index + 1].id);
                } else {
                    self.next_cue = None;
                }
            }
        } else {
            self.next_cue = self.cues.first().map(|c| c.id);
        }
    }
}

impl Default for CueList {
    fn default() -> Self {
        Self::new()
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    use super::super::crossfade::FadeCurve;

    #[test]
    fn test_cue_list_creation() {
        let list = CueList::new();
        assert!(list.is_empty());
        assert_eq!(list.state(), CueListState::Idle);
    }

    #[test]
    fn test_add_cue() {
        let mut list = CueList::new();
        let cue = Cue::new(0, "Cue 1".to_string());
        list.add_cue(cue);

        assert_eq!(list.len(), 1);
        assert!(list.get_cue(0).is_some());
    }

    #[test]
    fn test_remove_cue() {
        let mut list = CueList::new();
        list.add_cue(Cue::new(0, "Cue 1".to_string()));
        list.add_cue(Cue::new(1, "Cue 2".to_string()));

        let removed = list.remove_cue(0);
        assert!(removed.is_some());
        assert_eq!(list.len(), 1);
    }

    #[test]
    fn test_goto_cue() {
        let mut list = CueList::new();
        list.add_cue(Cue::new(0, "Cue 1".to_string()));
        list.add_cue(Cue::new(1, "Cue 2".to_string()));

        list.goto_cue(0, None).unwrap();
        assert_eq!(list.current_cue(), Some(0));
    }

    #[test]
    fn test_next_prev() {
        let mut list = CueList::new();
        list.add_cue(Cue::new(0, "Cue 1".to_string()).with_fade_duration(Duration::from_millis(1)));
        list.add_cue(Cue::new(1, "Cue 2".to_string()).with_fade_duration(Duration::from_millis(1)));
        list.add_cue(Cue::new(2, "Cue 3".to_string()).with_fade_duration(Duration::from_millis(1)));

        list.goto_cue(0, Some(Duration::from_millis(1))).unwrap();

        // Wait for crossfade
        std::thread::sleep(Duration::from_millis(10));
        list.update();

        assert_eq!(list.current_cue(), Some(0));
        assert_eq!(list.next_cue(), Some(1));

        list.next().unwrap();
        std::thread::sleep(Duration::from_millis(10));
        list.update();

        assert_eq!(list.current_cue(), Some(1));
        assert_eq!(list.next_cue(), Some(2));

        list.prev().unwrap();
        std::thread::sleep(Duration::from_millis(10));
        list.update();

        assert_eq!(list.current_cue(), Some(0));
    }

    #[test]
    fn test_crossfade() {
        let mut list = CueList::new();
        list.add_cue(Cue::new(0, "Cue 1".to_string()));
        list.add_cue(
            Cue::new(1, "Cue 2".to_string())
                .with_fade_duration(Duration::from_millis(100))
                .with_fade_curve(FadeCurve::Linear),
        );

        list.goto_cue(0, Some(Duration::from_millis(1))).unwrap();
        std::thread::sleep(Duration::from_millis(10));
        list.update();

        assert_eq!(list.state(), CueListState::Playing);

        list.goto_cue(1, None).unwrap();
        assert_eq!(list.state(), CueListState::Crossfading);
        assert!(list.current_crossfade().is_some());

        std::thread::sleep(Duration::from_millis(150));
        list.update();

        assert_eq!(list.state(), CueListState::Playing);
        assert_eq!(list.current_cue(), Some(1));
        assert!(list.current_crossfade().is_none());
    }
}
