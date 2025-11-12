//! sACN (E1.31) protocol implementation
//!
//! sACN (Streaming ACN) is a protocol for transmitting DMX512 over IP multicast.

use std::net::UdpSocket;
use std::time::{Duration, Instant};
use uuid::Uuid;

use crate::{error::ControlError, Result};

/// sACN sender for outputting DMX data
pub struct SacnSender {
    socket: UdpSocket,
    universe: u16,
    sequence: u8,
    priority: u8,
    source_name: String,
    cid: [u8; 16], // Component ID (UUID)
    last_send: Instant,
    min_interval: Duration,
}

impl SacnSender {
    /// Create a new sACN sender
    ///
    /// # Arguments
    /// * `universe` - sACN universe (1-63999)
    /// * `source_name` - Source name (up to 64 characters)
    pub fn new(universe: u16, source_name: &str) -> Result<Self> {
        if universe == 0 || universe > 63999 {
            return Err(ControlError::DmxError(format!(
                "Invalid sACN universe: {} (must be 1-63999)",
                universe
            )));
        }

        let socket = UdpSocket::bind("0.0.0.0:0")?;
        socket.set_multicast_loop_v4(false)?;

        // Generate a UUID for this component
        let uuid = Uuid::new_v4();
        let cid = *uuid.as_bytes();

        tracing::info!("sACN sender created for universe {}", universe);

        Ok(Self {
            socket,
            universe,
            sequence: 0,
            priority: 100, // Default priority
            source_name: source_name.to_string(),
            cid,
            last_send: Instant::now(),
            min_interval: Duration::from_millis(1000 / 30), // 30Hz refresh rate
        })
    }

    /// Send DMX data via sACN
    ///
    /// # Arguments
    /// * `channels` - 512 DMX channel values
    pub fn send_dmx(&mut self, channels: &[u8; 512]) -> Result<()> {
        // Rate limiting (30Hz max)
        let now = Instant::now();
        let elapsed = now.duration_since(self.last_send);
        if elapsed < self.min_interval {
            return Ok(());
        }

        let packet = self.build_sacn_packet(channels);

        // Calculate multicast address: 239.255.0.0 + universe
        let multicast_addr = format!("239.255.{}.{}:5568",
            (self.universe >> 8) & 0xFF,
            self.universe & 0xFF
        );

        self.socket.send_to(&packet, multicast_addr)?;
        self.sequence = self.sequence.wrapping_add(1);
        self.last_send = now;

        tracing::trace!("Sent sACN DMX packet for universe {}", self.universe);

        Ok(())
    }

    /// Build an sACN packet
    fn build_sacn_packet(&self, channels: &[u8; 512]) -> Vec<u8> {
        let mut packet = vec![0u8; 638]; // Full E1.31 packet size

        // Root Layer
        let mut offset = 0;

        // Preamble Size (16-bit)
        packet[offset..offset + 2].copy_from_slice(&0x0010u16.to_be_bytes());
        offset += 2;

        // Post-amble Size (16-bit)
        packet[offset..offset + 2].copy_from_slice(&0x0000u16.to_be_bytes());
        offset += 2;

        // ACN Packet Identifier (12 bytes)
        packet[offset..offset + 12].copy_from_slice(&[
            0x41, 0x53, 0x43, 0x2d, 0x45, 0x31, 0x2e, 0x31, 0x37, 0x00, 0x00, 0x00,
        ]);
        offset += 12;

        // Flags and Length (16-bit): 0x7000 | (638 - 16)
        let root_length = 638 - 16;
        packet[offset..offset + 2].copy_from_slice(&((0x7000u16 | root_length as u16).to_be_bytes()));
        offset += 2;

        // Vector (32-bit): VECTOR_ROOT_E131_DATA (0x00000004)
        packet[offset..offset + 4].copy_from_slice(&0x00000004u32.to_be_bytes());
        offset += 4;

        // CID (16 bytes)
        packet[offset..offset + 16].copy_from_slice(&self.cid);
        offset += 16;

        // Framing Layer
        // Flags and Length (16-bit): 0x7000 | (638 - 38)
        let framing_length = 638 - 38;
        packet[offset..offset + 2].copy_from_slice(&((0x7000u16 | framing_length as u16).to_be_bytes()));
        offset += 2;

        // Vector (32-bit): VECTOR_E131_DATA_PACKET (0x00000002)
        packet[offset..offset + 4].copy_from_slice(&0x00000002u32.to_be_bytes());
        offset += 4;

        // Source Name (64 bytes, null-terminated)
        let source_bytes = self.source_name.as_bytes();
        let copy_len = source_bytes.len().min(63);
        packet[offset..offset + copy_len].copy_from_slice(&source_bytes[..copy_len]);
        offset += 64;

        // Priority (1 byte)
        packet[offset] = self.priority;
        offset += 1;

        // Synchronization Address (16-bit) - 0 for no sync
        packet[offset..offset + 2].copy_from_slice(&0x0000u16.to_be_bytes());
        offset += 2;

        // Sequence Number (1 byte)
        packet[offset] = self.sequence;
        offset += 1;

        // Options (1 byte) - 0 for none
        packet[offset] = 0;
        offset += 1;

        // Universe (16-bit)
        packet[offset..offset + 2].copy_from_slice(&self.universe.to_be_bytes());
        offset += 2;

        // DMP Layer
        // Flags and Length (16-bit): 0x7000 | (638 - 115)
        let dmp_length = 638 - 115;
        packet[offset..offset + 2].copy_from_slice(&((0x7000u16 | dmp_length as u16).to_be_bytes()));
        offset += 2;

        // Vector (1 byte): VECTOR_DMP_SET_PROPERTY (0x02)
        packet[offset] = 0x02;
        offset += 1;

        // Address Type & Data Type (1 byte): 0xa1
        packet[offset] = 0xa1;
        offset += 1;

        // First Property Address (16-bit): 0x0000
        packet[offset..offset + 2].copy_from_slice(&0x0000u16.to_be_bytes());
        offset += 2;

        // Address Increment (16-bit): 0x0001
        packet[offset..offset + 2].copy_from_slice(&0x0001u16.to_be_bytes());
        offset += 2;

        // Property value count (16-bit): 513 (start code + 512 channels)
        packet[offset..offset + 2].copy_from_slice(&513u16.to_be_bytes());
        offset += 2;

        // DMX Start Code (1 byte): 0x00
        packet[offset] = 0x00;
        offset += 1;

        // DMX Data (512 bytes)
        packet[offset..offset + 512].copy_from_slice(channels);

        packet
    }

    /// Get the current universe
    pub fn universe(&self) -> u16 {
        self.universe
    }

    /// Set the universe
    pub fn set_universe(&mut self, universe: u16) -> Result<()> {
        if universe == 0 || universe > 63999 {
            return Err(ControlError::DmxError(format!(
                "Invalid sACN universe: {} (must be 1-63999)",
                universe
            )));
        }
        self.universe = universe;
        Ok(())
    }

    /// Set the priority (0-200, default 100)
    pub fn set_priority(&mut self, priority: u8) {
        self.priority = priority.min(200);
    }

    /// Set the refresh rate in Hz
    pub fn set_refresh_rate(&mut self, hz: u32) {
        self.min_interval = Duration::from_millis(1000 / hz as u64);
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_sacn_sender_creation() {
        let sender = SacnSender::new(1, "MapMap");
        assert!(sender.is_ok());
    }

    #[test]
    fn test_invalid_universe() {
        // Universe 0 is invalid
        assert!(SacnSender::new(0, "MapMap").is_err());

        // Universe > 63999 is invalid
        assert!(SacnSender::new(64000, "MapMap").is_err());
    }

    #[test]
    fn test_sacn_packet_structure() {
        let sender = SacnSender::new(1, "MapMap").unwrap();
        let channels = [0u8; 512];
        let packet = sender.build_sacn_packet(&channels);

        // Check packet size
        assert_eq!(packet.len(), 638);

        // Check ACN Packet Identifier
        assert_eq!(
            &packet[4..16],
            &[0x41, 0x53, 0x43, 0x2d, 0x45, 0x31, 0x2e, 0x31, 0x37, 0x00, 0x00, 0x00]
        );

        // Check DMX start code
        assert_eq!(packet[125], 0x00);
    }

    #[test]
    fn test_priority_setting() {
        let mut sender = SacnSender::new(1, "MapMap").unwrap();
        sender.set_priority(150);

        let channels = [0u8; 512];
        let packet = sender.build_sacn_packet(&channels);

        // Priority is at offset 108
        assert_eq!(packet[108], 150);
    }

    #[test]
    fn test_sequence_increment() {
        let mut sender = SacnSender::new(1, "MapMap").unwrap();

        let channels = [0u8; 512];

        // First packet
        let packet1 = sender.build_sacn_packet(&channels);
        let seq1 = packet1[111]; // Sequence is at offset 111

        sender.sequence = sender.sequence.wrapping_add(1);

        // Second packet
        let packet2 = sender.build_sacn_packet(&channels);
        let seq2 = packet2[111];

        assert_eq!(seq2, seq1.wrapping_add(1));
    }
}
