//! DMX output system
//!
//! This module provides DMX512 output via Art-Net and sACN protocols.
//!
//! ## Art-Net
//!
//! Art-Net is a UDP broadcast protocol for DMX transmission over Ethernet.
//! - Uses UDP broadcast (255.255.255.255:6454)
//! - Supports 32768 universes
//! - Includes sequence numbering
//!
//! ## sACN (E1.31)
//!
//! sACN (Streaming ACN) is a multicast protocol for DMX transmission.
//! - Uses IP multicast (239.255.x.x:5568)
//! - Supports 63999 universes
//! - Includes priority and synchronization
//!
//! ## Example Usage
//!
//! ```rust,no_run
//! use mapmap_control::dmx::{ArtNetSender, ChannelAssignment, DmxChannel};
//! use mapmap_control::{ControlTarget, ControlValue};
//! use std::collections::HashMap;
//!
//! # fn main() -> mapmap_control::Result<()> {
//! // Create Art-Net sender
//! let mut sender = ArtNetSender::new(0, "255.255.255.255:6454")?;
//!
//! // Create channel assignments
//! let mut assignment = ChannelAssignment::new();
//! assignment.assign(
//!     ControlTarget::LayerOpacity(0),
//!     DmxChannel::new(0, 1)
//! );
//!
//! // Apply control values to DMX
//! let mut dmx_data = HashMap::new();
//! assignment.apply_value(
//!     &ControlTarget::LayerOpacity(0),
//!     &ControlValue::Float(0.75),
//!     &mut dmx_data
//! )?;
//!
//! // Send DMX data
//! if let Some(channels) = dmx_data.get(&0) {
//!     sender.send_dmx(channels, "255.255.255.255:6454")?;
//! }
//! # Ok(())
//! # }
//! ```
//!
//! ## Fixtures
//!
//! ```rust
//! use mapmap_control::dmx::{FixtureProfile, Fixture};
//!
//! // Create an RGB fixture
//! let profile = FixtureProfile::rgb_par();
//! let fixture = Fixture::new(
//!     0,
//!     "Front RGB".to_string(),
//!     profile,
//!     0, // universe
//!     1  // start address
//! );
//!
//! // Set RGB values
//! let mut dmx_data = [0u8; 512];
//! fixture.set_rgb(&mut dmx_data, 255, 128, 64);
//! ```

pub mod artnet;
pub mod sacn;
pub mod channels;
pub mod fixtures;

pub use artnet::ArtNetSender;
pub use sacn::SacnSender;
pub use channels::{ChannelAssignment, DmxChannel};
pub use fixtures::{ChannelType, Fixture, FixtureChannel, FixtureProfile};
