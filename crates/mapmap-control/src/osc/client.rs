//! OSC client for sending messages

#[cfg(feature = "osc")]
use rosc::{encoder, OscMessage, OscPacket};

use crate::{ControlTarget, ControlValue, Result, error::ControlError};

#[cfg(feature = "osc")]
use super::types::control_value_to_osc;

/// OSC client for sending state updates
pub struct OscClient {
    #[cfg(feature = "osc")]
    socket: UdpSocket,
    #[cfg(feature = "osc")]
    destination: SocketAddr,
}

impl OscClient {
    /// Create a new OSC client
    ///
    /// # Arguments
    /// * `destination` - Destination address (e.g., "192.168.1.100:8001")
    #[cfg(feature = "osc")]
    pub fn new(destination: &str) -> Result<Self> {
        let socket = UdpSocket::bind("0.0.0.0:0")?;
        let destination: SocketAddr = destination
            .parse()
            .map_err(|e| ControlError::OscError(format!("Invalid destination address: {}", e)))?;

        tracing::info!("OSC client created, sending to {}", destination);

        Ok(Self {
            socket,
            destination,
        })
    }

    #[cfg(not(feature = "osc"))]
    pub fn new(_destination: &str) -> Result<Self> {
        Err(ControlError::OscError(
            "OSC feature not enabled".to_string(),
        ))
    }

    /// Send a control value update
    #[cfg(feature = "osc")]
    pub fn send_update(&self, target: &ControlTarget, value: &ControlValue) -> Result<()> {
        let address = control_target_to_address(target);
        let args = control_value_to_osc(value);

        let msg = OscMessage {
            addr: address.clone(),
            args,
        };

        let packet = OscPacket::Message(msg);
        let buf = encoder::encode(&packet)
            .map_err(|e| ControlError::OscError(format!("Failed to encode OSC message: {}", e)))?;

        self.socket.send_to(&buf, self.destination)?;

        tracing::trace!("Sent OSC: {} to {}", address, self.destination);

        Ok(())
    }

    #[cfg(not(feature = "osc"))]
    pub fn send_update(&self, _target: &ControlTarget, _value: &ControlValue) -> Result<()> {
        Err(ControlError::OscError(
            "OSC feature not enabled".to_string(),
        ))
    }

    /// Send a raw OSC message
    #[cfg(feature = "osc")]
    pub fn send_message(&self, address: &str, args: Vec<rosc::OscType>) -> Result<()> {
        let msg = OscMessage {
            addr: address.to_string(),
            args,
        };

        let packet = OscPacket::Message(msg);
        let buf = encoder::encode(&packet)
            .map_err(|e| ControlError::OscError(format!("Failed to encode OSC message: {}", e)))?;

        self.socket.send_to(&buf, self.destination)?;

        tracing::trace!("Sent raw OSC: {} to {}", address, self.destination);

        Ok(())
    }

    #[cfg(not(feature = "osc"))]
    pub fn send_message(&self, _address: &str, _args: Vec<()>) -> Result<()> {
        Err(ControlError::OscError(
            "OSC feature not enabled".to_string(),
        ))
    }

    /// Get the destination address
    #[cfg(feature = "osc")]
    pub fn destination(&self) -> SocketAddr {
        self.destination
    }

    #[cfg(not(feature = "osc"))]
    pub fn destination(&self) -> String {
        String::new()
    }
}

#[cfg(all(test, feature = "osc"))]
mod tests {
    use super::*;

    #[test]
    fn test_osc_client_creation() {
        let client = OscClient::new("127.0.0.1:8001");
        assert!(client.is_ok());
    }

    #[test]
    fn test_invalid_destination() {
        let client = OscClient::new("invalid:address");
        assert!(client.is_err());
    }
}
