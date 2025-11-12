//! OSC (Open Sound Control) system
//!
//! This module provides OSC server and client functionality for remote control of MapMap.
//!
//! ## OSC Address Space
//!
//! ```text
//! /mapmap/layer/{id}/opacity       [f32: 0.0-1.0]
//! /mapmap/layer/{id}/position      [f32, f32: x, y]
//! /mapmap/layer/{id}/rotation      [f32: degrees]
//! /mapmap/layer/{id}/scale         [f32: scale]
//! /mapmap/layer/{id}/visibility    [bool]
//! /mapmap/paint/{id}/parameter/{name}  [varies]
//! /mapmap/effect/{id}/parameter/{name} [varies]
//! /mapmap/playback/speed           [f32: speed multiplier]
//! /mapmap/playback/position        [f32: 0.0-1.0]
//! /mapmap/output/{id}/brightness   [f32: 0.0-1.0]
//! ```
//!
//! ## Example Usage
//!
//! ```rust,no_run
//! use mapmap_control::osc::{OscServer, OscClient};
//! use mapmap_control::{ControlTarget, ControlValue};
//!
//! # #[cfg(feature = "osc")]
//! # fn main() -> mapmap_control::Result<()> {
//! // Create server
//! let server = OscServer::new(8000)?;
//!
//! // Create client for sending state updates
//! let client = OscClient::new("192.168.1.100:8001")?;
//!
//! // Poll for events
//! while let Some(event) = server.poll_event() {
//!     println!("Received: {:?} = {:?}", event.target, event.value);
//!
//!     // Send update to other clients
//!     client.send_update(&event.target, &event.value)?;
//! }
//! # Ok(())
//! # }
//! # #[cfg(not(feature = "osc"))]
//! # fn main() {}
//! ```

pub mod address;
pub mod types;
pub mod client;
pub mod server;

pub use address::{control_target_to_address, parse_osc_address};
pub use client::OscClient;
pub use server::{OscEvent, OscServer};

#[cfg(feature = "osc")]
pub use types::{control_value_to_osc, osc_to_control_value, osc_to_vec2, osc_to_vec3};
