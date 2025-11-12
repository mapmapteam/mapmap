//! OSC server for receiving messages

#[cfg(feature = "osc")]
use rosc::{decoder, OscMessage, OscPacket};
#[cfg(feature = "osc")]
use std::net::UdpSocket;
#[cfg(feature = "osc")]
use std::sync::mpsc::{channel, Receiver, Sender};
#[cfg(feature = "osc")]
use std::thread;

use crate::{ControlTarget, ControlValue, Result, error::ControlError};

#[cfg(feature = "osc")]
use super::address::parse_osc_address;
#[cfg(feature = "osc")]
use super::types::{osc_to_control_value, osc_to_vec2};

/// OSC message event
#[derive(Debug, Clone)]
pub struct OscEvent {
    pub target: ControlTarget,
    pub value: ControlValue,
}

/// OSC server for receiving control messages
pub struct OscServer {
    #[cfg(feature = "osc")]
    receiver: Receiver<OscEvent>,
    #[cfg(feature = "osc")]
    _handle: Option<thread::JoinHandle<()>>,
}

impl OscServer {
    /// Create a new OSC server listening on the specified port
    ///
    /// # Arguments
    /// * `port` - UDP port to listen on (default: 8000)
    #[cfg(feature = "osc")]
    pub fn new(port: u16) -> Result<Self> {
        let addr = format!("0.0.0.0:{}", port);
        let socket = UdpSocket::bind(&addr)
            .map_err(|e| ControlError::OscError(format!("Failed to bind to {}: {}", addr, e)))?;

        tracing::info!("OSC server listening on {}", addr);

        let (sender, receiver) = channel();

        // Spawn receiver thread
        let handle = thread::spawn(move || {
            Self::run_receiver(socket, sender);
        });

        Ok(Self {
            receiver,
            _handle: Some(handle),
        })
    }

    #[cfg(not(feature = "osc"))]
    pub fn new(_port: u16) -> Result<Self> {
        Err(ControlError::OscError(
            "OSC feature not enabled".to_string(),
        ))
    }

    /// Run the receiver loop (blocking)
    #[cfg(feature = "osc")]
    fn run_receiver(socket: UdpSocket, sender: Sender<OscEvent>) {
        let mut buf = [0u8; 65536]; // Max UDP packet size

        loop {
            match socket.recv_from(&mut buf) {
                Ok((size, addr)) => {
                    match decoder::decode_udp(&buf[..size]) {
                        Ok((_, packet)) => {
                            if let Err(e) = Self::handle_packet(&packet, &sender) {
                                tracing::warn!("Failed to handle OSC packet from {}: {}", addr, e);
                            }
                        }
                        Err(e) => {
                            tracing::warn!("Failed to decode OSC packet from {}: {}", addr, e);
                        }
                    }
                }
                Err(e) => {
                    tracing::error!("OSC socket error: {}", e);
                    break;
                }
            }
        }
    }

    /// Handle an OSC packet
    #[cfg(feature = "osc")]
    fn handle_packet(packet: &OscPacket, sender: &Sender<OscEvent>) -> Result<()> {
        match packet {
            OscPacket::Message(msg) => Self::handle_message(msg, sender),
            OscPacket::Bundle(bundle) => {
                for packet in &bundle.content {
                    Self::handle_packet(packet, sender)?;
                }
                Ok(())
            }
        }
    }

    /// Handle an OSC message
    #[cfg(feature = "osc")]
    fn handle_message(msg: &OscMessage, sender: &Sender<OscEvent>) -> Result<()> {
        let target = parse_osc_address(&msg.addr)?;

        // Determine if we need Vec2 based on the target
        let value = match &target {
            ControlTarget::LayerPosition(_) => osc_to_vec2(&msg.args)?,
            _ => osc_to_control_value(&msg.args)?,
        };

        let event = OscEvent { target, value };

        sender
            .send(event)
            .map_err(|e| ControlError::OscError(format!("Failed to send event: {}", e)))?;

        tracing::trace!("Received OSC: {} -> {:?}", msg.addr, msg.args);

        Ok(())
    }

    /// Poll for new OSC events (non-blocking)
    ///
    /// Returns `None` if no events are available
    #[cfg(feature = "osc")]
    pub fn poll_event(&self) -> Option<OscEvent> {
        self.receiver.try_recv().ok()
    }

    #[cfg(not(feature = "osc"))]
    pub fn poll_event(&self) -> Option<OscEvent> {
        None
    }

    /// Wait for the next OSC event (blocking)
    ///
    /// Returns `None` if the server has shut down
    #[cfg(feature = "osc")]
    pub fn wait_event(&self) -> Option<OscEvent> {
        self.receiver.recv().ok()
    }

    #[cfg(not(feature = "osc"))]
    pub fn wait_event(&self) -> Option<OscEvent> {
        None
    }
}

#[cfg(all(test, feature = "osc"))]
mod tests {
    use super::*;
    use std::time::Duration;

    #[test]
    fn test_osc_server_creation() {
        // Use a high port to avoid permission issues
        let server = OscServer::new(18000);
        assert!(server.is_ok());
    }

    #[test]
    fn test_osc_server_client_communication() {
        use crate::osc::client::OscClient;
        use rosc::OscType;

        // Create server on a high port
        let server = OscServer::new(18001).unwrap();

        // Give server time to start
        thread::sleep(Duration::from_millis(100));

        // Create client
        let client = OscClient::new("127.0.0.1:18001").unwrap();

        // Send a message
        client
            .send_message("/mapmap/layer/0/opacity", vec![OscType::Float(0.5)])
            .unwrap();

        // Wait a bit for the message to arrive
        thread::sleep(Duration::from_millis(100));

        // Poll for event
        if let Some(event) = server.poll_event() {
            assert_eq!(event.target, ControlTarget::LayerOpacity(0));
            assert_eq!(event.value, ControlValue::Float(0.5));
        } else {
            panic!("Expected OSC event");
        }
    }
}
