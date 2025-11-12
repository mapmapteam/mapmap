//! DMX fixture profiles

use serde::{Deserialize, Serialize};

/// DMX fixture profile defining channel layout
#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct FixtureProfile {
    pub name: String,
    pub manufacturer: String,
    pub channels: Vec<FixtureChannel>,
}

/// A channel in a fixture profile
#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct FixtureChannel {
    pub name: String,
    pub channel_type: ChannelType,
    pub default_value: u8,
}

/// Type of DMX channel
#[derive(Debug, Clone, Copy, PartialEq, Eq, Serialize, Deserialize)]
pub enum ChannelType {
    Dimmer,
    Red,
    Green,
    Blue,
    Amber,
    White,
    Pan,
    Tilt,
    ColorWheel,
    Gobo,
    Shutter,
    Speed,
    Generic,
}

impl FixtureProfile {
    /// Create a generic dimmer fixture (1 channel)
    pub fn generic_dimmer() -> Self {
        Self {
            name: "Generic Dimmer".to_string(),
            manufacturer: "Generic".to_string(),
            channels: vec![FixtureChannel {
                name: "Dimmer".to_string(),
                channel_type: ChannelType::Dimmer,
                default_value: 0,
            }],
        }
    }

    /// Create an RGB fixture (3 channels)
    pub fn rgb_par() -> Self {
        Self {
            name: "RGB Par".to_string(),
            manufacturer: "Generic".to_string(),
            channels: vec![
                FixtureChannel {
                    name: "Red".to_string(),
                    channel_type: ChannelType::Red,
                    default_value: 0,
                },
                FixtureChannel {
                    name: "Green".to_string(),
                    channel_type: ChannelType::Green,
                    default_value: 0,
                },
                FixtureChannel {
                    name: "Blue".to_string(),
                    channel_type: ChannelType::Blue,
                    default_value: 0,
                },
            ],
        }
    }

    /// Create an RGBA fixture (4 channels)
    pub fn rgba_par() -> Self {
        Self {
            name: "RGBA Par".to_string(),
            manufacturer: "Generic".to_string(),
            channels: vec![
                FixtureChannel {
                    name: "Red".to_string(),
                    channel_type: ChannelType::Red,
                    default_value: 0,
                },
                FixtureChannel {
                    name: "Green".to_string(),
                    channel_type: ChannelType::Green,
                    default_value: 0,
                },
                FixtureChannel {
                    name: "Blue".to_string(),
                    channel_type: ChannelType::Blue,
                    default_value: 0,
                },
                FixtureChannel {
                    name: "Amber".to_string(),
                    channel_type: ChannelType::Amber,
                    default_value: 0,
                },
            ],
        }
    }

    /// Create an RGBW fixture (4 channels)
    pub fn rgbw_par() -> Self {
        Self {
            name: "RGBW Par".to_string(),
            manufacturer: "Generic".to_string(),
            channels: vec![
                FixtureChannel {
                    name: "Red".to_string(),
                    channel_type: ChannelType::Red,
                    default_value: 0,
                },
                FixtureChannel {
                    name: "Green".to_string(),
                    channel_type: ChannelType::Green,
                    default_value: 0,
                },
                FixtureChannel {
                    name: "Blue".to_string(),
                    channel_type: ChannelType::Blue,
                    default_value: 0,
                },
                FixtureChannel {
                    name: "White".to_string(),
                    channel_type: ChannelType::White,
                    default_value: 0,
                },
            ],
        }
    }

    /// Get the number of channels this fixture uses
    pub fn channel_count(&self) -> usize {
        self.channels.len()
    }
}

/// A fixture instance with a starting DMX address
#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct Fixture {
    pub id: u32,
    pub name: String,
    pub profile: FixtureProfile,
    pub universe: u16,
    pub start_address: u16, // 1-512
}

impl Fixture {
    /// Create a new fixture instance
    pub fn new(
        id: u32,
        name: String,
        profile: FixtureProfile,
        universe: u16,
        start_address: u16,
    ) -> Self {
        Self {
            id,
            name,
            profile,
            universe,
            start_address,
        }
    }

    /// Get the end address of this fixture
    pub fn end_address(&self) -> u16 {
        self.start_address + self.profile.channel_count() as u16 - 1
    }

    /// Set a channel value by type
    pub fn set_channel_value(
        &self,
        dmx_data: &mut [u8; 512],
        channel_type: ChannelType,
        value: u8,
    ) {
        for (i, channel) in self.profile.channels.iter().enumerate() {
            if channel.channel_type == channel_type {
                let addr = (self.start_address as usize + i).saturating_sub(1);
                if addr < 512 {
                    dmx_data[addr] = value;
                }
            }
        }
    }

    /// Set RGB values
    pub fn set_rgb(&self, dmx_data: &mut [u8; 512], r: u8, g: u8, b: u8) {
        self.set_channel_value(dmx_data, ChannelType::Red, r);
        self.set_channel_value(dmx_data, ChannelType::Green, g);
        self.set_channel_value(dmx_data, ChannelType::Blue, b);
    }

    /// Set RGBA values
    pub fn set_rgba(&self, dmx_data: &mut [u8; 512], r: u8, g: u8, b: u8, a: u8) {
        self.set_rgb(dmx_data, r, g, b);
        self.set_channel_value(dmx_data, ChannelType::Amber, a);
    }

    /// Set RGBW values
    pub fn set_rgbw(&self, dmx_data: &mut [u8; 512], r: u8, g: u8, b: u8, w: u8) {
        self.set_rgb(dmx_data, r, g, b);
        self.set_channel_value(dmx_data, ChannelType::White, w);
    }

    /// Set dimmer value
    pub fn set_dimmer(&self, dmx_data: &mut [u8; 512], value: u8) {
        self.set_channel_value(dmx_data, ChannelType::Dimmer, value);
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_generic_dimmer() {
        let profile = FixtureProfile::generic_dimmer();
        assert_eq!(profile.channel_count(), 1);
        assert_eq!(profile.channels[0].channel_type, ChannelType::Dimmer);
    }

    #[test]
    fn test_rgb_par() {
        let profile = FixtureProfile::rgb_par();
        assert_eq!(profile.channel_count(), 3);
        assert_eq!(profile.channels[0].channel_type, ChannelType::Red);
        assert_eq!(profile.channels[1].channel_type, ChannelType::Green);
        assert_eq!(profile.channels[2].channel_type, ChannelType::Blue);
    }

    #[test]
    fn test_fixture_addressing() {
        let profile = FixtureProfile::rgb_par();
        let fixture = Fixture::new(0, "Test".to_string(), profile, 0, 1);

        assert_eq!(fixture.start_address, 1);
        assert_eq!(fixture.end_address(), 3);
    }

    #[test]
    fn test_fixture_set_rgb() {
        let profile = FixtureProfile::rgb_par();
        let fixture = Fixture::new(0, "Test".to_string(), profile, 0, 1);

        let mut dmx_data = [0u8; 512];
        fixture.set_rgb(&mut dmx_data, 255, 128, 64);

        assert_eq!(dmx_data[0], 255); // Red
        assert_eq!(dmx_data[1], 128); // Green
        assert_eq!(dmx_data[2], 64); // Blue
    }
}
