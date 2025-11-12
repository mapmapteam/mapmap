//! Art-Net protocol implementation (Art-Net 4)
//!
//! Art-Net is a UDP-based protocol for transmitting DMX512 over Ethernet.

use std::net::{UdpSocket, SocketAddr};
use std::time::{Duration, Instant};

use crate::{error::ControlError, Result};

/// Art-Net sender for outputting DMX data
pub struct ArtNetSender {
    socket: UdpSocket,
    universe: u16,
    sequence: u8,
    last_send: Instant,
    min_interval: Duration,
}

impl ArtNetSender {
    /// Create a new Art-Net sender
    ///
    /// # Arguments
    /// * `universe` - Art-Net universe (0-32767)
    /// * `target` - Broadcast address (typically "255.255.255.255:6454")
    pub fn new(universe: u16, target: &str) -> Result<Self> {
        let socket = UdpSocket::bind("0.0.0.0:0")?;
        socket.set_broadcast(true)?;

        // Verify target is a valid address
        let _: SocketAddr = target.parse().map_err(|e| {
            ControlError::DmxError(format!("Invalid Art-Net target address: {}", e))
        })?;

        tracing::info!("Art-Net sender created for universe {} -> {}", universe, target);

        Ok(Self {
            socket,
            universe,
            sequence: 0,
            last_send: Instant::now(),
            min_interval: Duration::from_millis(1000 / 30), // 30Hz refresh rate
        })
    }

    /// Send DMX data via Art-Net
    ///
    /// # Arguments
    /// * `channels` - 512 DMX channel values
    /// * `target` - Destination address
    pub fn send_dmx(&mut self, channels: &[u8; 512], target: &str) -> Result<()> {
        // Rate limiting (30Hz max)
        let now = Instant::now();
        let elapsed = now.duration_since(self.last_send);
        if elapsed < self.min_interval {
            return Ok(());
        }

        let packet = self.build_artnet_packet(channels);

        self.socket.send_to(&packet, target)?;
        self.sequence = self.sequence.wrapping_add(1);
        self.last_send = now;

        tracing::trace!("Sent Art-Net DMX packet for universe {}", self.universe);

        Ok(())
    }

    /// Build an Art-Net DMX packet (OpDmx)
    fn build_artnet_packet(&self, channels: &[u8; 512]) -> Vec<u8> {
        let mut packet = vec![0u8; 18 + 512];

        // Header: "Art-Net\0"
        packet[0..8].copy_from_slice(b"Art-Net\0");

        // OpCode: OpDmx (0x5000)
        packet[8..10].copy_from_slice(&0x5000u16.to_le_bytes());

        // Protocol version (14)
        packet[10..12].copy_from_slice(&14u16.to_be_bytes());

        // Sequence
        packet[12] = self.sequence;

        // Physical (0)
        packet[13] = 0;

        // Universe (Port-Address)
        packet[14..16].copy_from_slice(&self.universe.to_le_bytes());

        // Length (512 channels, big-endian)
        packet[16..18].copy_from_slice(&512u16.to_be_bytes());

        // DMX data
        packet[18..].copy_from_slice(channels);

        packet
    }

    /// Get the current universe
    pub fn universe(&self) -> u16 {
        self.universe
    }

    /// Set the universe
    pub fn set_universe(&mut self, universe: u16) {
        self.universe = universe;
    }

    /// Set the minimum send interval (for rate limiting)
    pub fn set_refresh_rate(&mut self, hz: u32) {
        self.min_interval = Duration::from_millis(1000 / hz as u64);
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_artnet_packet_structure() {
        let sender = ArtNetSender {
            socket: UdpSocket::bind("0.0.0.0:0").unwrap(),
            universe: 0,
            sequence: 0,
            last_send: Instant::now(),
            min_interval: Duration::from_millis(33),
        };

        let channels = [0u8; 512];
        let packet = sender.build_artnet_packet(&channels);

        // Check header
        assert_eq!(&packet[0..8], b"Art-Net\0");

        // Check OpCode (little-endian)
        assert_eq!(packet[8], 0x00);
        assert_eq!(packet[9], 0x50);

        // Check protocol version (big-endian)
        assert_eq!(packet[10], 0);
        assert_eq!(packet[11], 14);

        // Check length (big-endian)
        assert_eq!(packet[16], 0x02);
        assert_eq!(packet[17], 0x00);

        // Total packet size
        assert_eq!(packet.len(), 18 + 512);
    }

    #[test]
    fn test_artnet_sender_creation() {
        let sender = ArtNetSender::new(0, "255.255.255.255:6454");
        assert!(sender.is_ok());
    }

    #[test]
    fn test_invalid_target() {
        let sender = ArtNetSender::new(0, "invalid:address");
        assert!(sender.is_err());
    }

    #[test]
    fn test_universe_setting() {
        let mut sender = ArtNetSender::new(0, "255.255.255.255:6454").unwrap();
        assert_eq!(sender.universe(), 0);

        sender.set_universe(5);
        assert_eq!(sender.universe(), 5);
    }

    #[test]
    fn test_sequence_increment() {
        let mut sender = ArtNetSender::new(0, "255.255.255.255:6454").unwrap();

        let channels = [0u8; 512];

        // First packet
        let packet1 = sender.build_artnet_packet(&channels);
        let seq1 = packet1[12];

        sender.sequence = sender.sequence.wrapping_add(1);

        // Second packet
        let packet2 = sender.build_artnet_packet(&channels);
        let seq2 = packet2[12];

        assert_eq!(seq2, seq1.wrapping_add(1));
    }
}
