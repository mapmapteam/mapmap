//! Monitor Detection and Display Utilities
//!
//! Phase 2: Detect available monitors and configure outputs

use serde::{Deserialize, Serialize};

/// Information about a detected monitor
#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct MonitorInfo {
    /// Monitor index (0-based)
    pub index: usize,
    /// Display name (if available)
    pub name: String,
    /// Physical position on the virtual desktop
    pub position: (i32, i32),
    /// Resolution (width, height)
    pub size: (u32, u32),
    /// Refresh rate in Hz (if available)
    pub refresh_rate: Option<u32>,
    /// Scale factor (for HiDPI displays)
    pub scale_factor: f64,
    /// Is this the primary monitor?
    pub is_primary: bool,
}

impl MonitorInfo {
    /// Create a new monitor info
    pub fn new(
        index: usize,
        name: String,
        position: (i32, i32),
        size: (u32, u32),
        refresh_rate: Option<u32>,
        scale_factor: f64,
        is_primary: bool,
    ) -> Self {
        Self {
            index,
            name,
            position,
            size,
            refresh_rate,
            scale_factor,
            is_primary,
        }
    }

    /// Get a display string for UI
    pub fn display_string(&self) -> String {
        let refresh = self
            .refresh_rate
            .map(|r| format!(" @ {}Hz", r))
            .unwrap_or_default();
        let primary = if self.is_primary { " (Primary)" } else { "" };

        format!(
            "{}: {}x{}{}{}",
            self.name, self.size.0, self.size.1, refresh, primary
        )
    }
}

/// Monitor topology (arrangement of displays)
#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct MonitorTopology {
    pub monitors: Vec<MonitorInfo>,
    pub total_bounds: (i32, i32, u32, u32), // (x, y, width, height)
}

impl MonitorTopology {
    /// Create a new topology from a list of monitors
    pub fn new(monitors: Vec<MonitorInfo>) -> Self {
        let total_bounds = if monitors.is_empty() {
            (0, 0, 0, 0)
        } else {
            let min_x = monitors.iter().map(|m| m.position.0).min().unwrap_or(0);
            let min_y = monitors.iter().map(|m| m.position.1).min().unwrap_or(0);
            let max_x = monitors
                .iter()
                .map(|m| m.position.0 + m.size.0 as i32)
                .max()
                .unwrap_or(0);
            let max_y = monitors
                .iter()
                .map(|m| m.position.1 + m.size.1 as i32)
                .max()
                .unwrap_or(0);

            let width = (max_x - min_x) as u32;
            let height = (max_y - min_y) as u32;

            (min_x, min_y, width, height)
        };

        Self {
            monitors,
            total_bounds,
        }
    }

    /// Get the primary monitor
    pub fn primary_monitor(&self) -> Option<&MonitorInfo> {
        self.monitors.iter().find(|m| m.is_primary)
    }

    /// Get monitor by index
    pub fn get_monitor(&self, index: usize) -> Option<&MonitorInfo> {
        self.monitors.get(index)
    }

    /// Get number of monitors
    pub fn monitor_count(&self) -> usize {
        self.monitors.len()
    }
}

/// Detect available monitors using winit
///
/// Note: This function requires an active EventLoop, so it's typically
/// called during application initialization
#[allow(unexpected_cfgs)]
#[cfg(feature = "winit")]
pub fn detect_monitors_winit<T>(event_loop: &winit::event_loop::EventLoop<T>) -> MonitorTopology {
    use winit::monitor::MonitorHandle;

    let mut monitors = Vec::new();
    let primary_handle = event_loop.primary_monitor();

    for (index, monitor) in event_loop.available_monitors().enumerate() {
        let name = monitor
            .name()
            .unwrap_or_else(|| format!("Monitor {}", index + 1));

        let position = monitor.position().into();
        let size = monitor.size().into();

        let refresh_rate = monitor.refresh_rate_millihertz().map(|r| r / 1000);
        let scale_factor = monitor.scale_factor();

        let is_primary = if let Some(ref primary) = primary_handle {
            // Compare by handle (winit doesn't have direct comparison, so we use name + position)
            monitor.name() == primary.name() && monitor.position() == primary.position()
        } else {
            index == 0 // Fallback: first monitor is primary
        };

        monitors.push(MonitorInfo::new(
            index,
            name,
            position,
            size,
            refresh_rate,
            scale_factor,
            is_primary,
        ));
    }

    MonitorTopology::new(monitors)
}

/// Create a simple test topology (for testing without real monitors)
pub fn create_test_topology() -> MonitorTopology {
    let monitors = vec![
        MonitorInfo::new(
            0,
            "Monitor 1".to_string(),
            (0, 0),
            (1920, 1080),
            Some(60),
            1.0,
            true,
        ),
        MonitorInfo::new(
            1,
            "Monitor 2".to_string(),
            (1920, 0),
            (1920, 1080),
            Some(60),
            1.0,
            false,
        ),
    ];

    MonitorTopology::new(monitors)
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_monitor_info_display() {
        let monitor = MonitorInfo::new(
            0,
            "Test Monitor".to_string(),
            (0, 0),
            (1920, 1080),
            Some(60),
            1.0,
            true,
        );

        let display = monitor.display_string();
        assert!(display.contains("Test Monitor"));
        assert!(display.contains("1920x1080"));
        assert!(display.contains("60Hz"));
        assert!(display.contains("(Primary)"));
    }

    #[test]
    fn test_topology_bounds() {
        let monitors = vec![
            MonitorInfo::new(0, "M1".to_string(), (0, 0), (1920, 1080), None, 1.0, true),
            MonitorInfo::new(1, "M2".to_string(), (1920, 0), (1920, 1080), None, 1.0, false),
        ];

        let topology = MonitorTopology::new(monitors);

        assert_eq!(topology.monitor_count(), 2);
        assert_eq!(topology.total_bounds, (0, 0, 3840, 1080));
        assert!(topology.primary_monitor().is_some());
    }

    #[test]
    fn test_test_topology() {
        let topology = create_test_topology();
        assert_eq!(topology.monitor_count(), 2);
        assert!(topology.primary_monitor().is_some());
    }
}
