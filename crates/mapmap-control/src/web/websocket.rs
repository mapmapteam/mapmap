//! WebSocket handler for real-time updates

#[cfg(feature = "http-api")]
use axum::{
    extract::{
        ws::{Message, WebSocket},
        State, WebSocketUpgrade,
    },
    response::Response,
};

#[cfg(feature = "http-api")]
use futures::{SinkExt, StreamExt};
use serde::{Deserialize, Serialize};

use crate::{ControlTarget, ControlValue};

#[cfg(feature = "http-api")]
use super::server::AppState;

/// WebSocket message from client to server
#[derive(Debug, Serialize, Deserialize)]
#[serde(tag = "type")]
pub enum WsClientMessage {
    #[serde(rename = "set_parameter")]
    SetParameter {
        target: ControlTarget,
        value: ControlValue,
    },
    #[serde(rename = "subscribe")]
    Subscribe { targets: Vec<ControlTarget> },
    #[serde(rename = "unsubscribe")]
    Unsubscribe { targets: Vec<ControlTarget> },
    #[serde(rename = "ping")]
    Ping,
}

/// WebSocket message from server to client
#[derive(Debug, Serialize, Deserialize)]
#[serde(tag = "type")]
pub enum WsServerMessage {
    #[serde(rename = "parameter_changed")]
    ParameterChanged {
        target: ControlTarget,
        value: ControlValue,
    },
    #[serde(rename = "stats")]
    Stats { fps: f32, frame_time_ms: f32 },
    #[serde(rename = "error")]
    Error { message: String },
    #[serde(rename = "pong")]
    Pong,
}

/// WebSocket upgrade handler
#[cfg(feature = "http-api")]
pub async fn ws_handler(
    ws: WebSocketUpgrade,
    State(state): State<AppState>,
) -> Response {
    ws.on_upgrade(|socket| handle_socket(socket, state))
}

#[cfg(not(feature = "http-api"))]
pub async fn ws_handler() -> () {
    ()
}

/// Handle a WebSocket connection
#[cfg(feature = "http-api")]
async fn handle_socket(socket: WebSocket, _state: AppState) {
    let (mut sender, mut receiver) = socket.split();

    tracing::info!("WebSocket client connected");

    // Spawn a task to send periodic stats updates
    let stats_task = tokio::spawn(async move {
        let mut interval = tokio::time::interval(tokio::time::Duration::from_millis(1000 / 60));

        loop {
            interval.tick().await;

            let stats = WsServerMessage::Stats {
                fps: 60.0,
                frame_time_ms: 16.6,
            };

            if let Ok(json) = serde_json::to_string(&stats) {
                if sender.send(Message::Text(json)).await.is_err() {
                    break;
                }
            } else {
                break;
            }
        }
    });

    // Handle incoming messages
    while let Some(msg) = receiver.next().await {
        match msg {
            Ok(Message::Text(text)) => {
                if let Err(e) = handle_text_message(&text).await {
                    tracing::warn!("Error handling WebSocket message: {}", e);
                }
            }
            Ok(Message::Close(_)) => {
                tracing::info!("WebSocket client disconnected");
                break;
            }
            Err(e) => {
                tracing::error!("WebSocket error: {}", e);
                break;
            }
            _ => {}
        }
    }

    stats_task.abort();
}

/// Handle a text message from the client
#[cfg(feature = "http-api")]
async fn handle_text_message(text: &str) -> Result<(), String> {
    let message: WsClientMessage =
        serde_json::from_str(text).map_err(|e| format!("Invalid JSON: {}", e))?;

    match message {
        WsClientMessage::SetParameter { target, value } => {
            tracing::debug!("WebSocket set parameter: {:?} = {:?}", target, value);
            // In a real implementation, this would update the project state
        }
        WsClientMessage::Subscribe { targets } => {
            tracing::debug!("WebSocket subscribe: {:?}", targets);
            // In a real implementation, this would track subscriptions
        }
        WsClientMessage::Unsubscribe { targets } => {
            tracing::debug!("WebSocket unsubscribe: {:?}", targets);
        }
        WsClientMessage::Ping => {
            tracing::trace!("WebSocket ping");
        }
    }

    Ok(())
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_ws_client_message_serialization() {
        let msg = WsClientMessage::SetParameter {
            target: ControlTarget::LayerOpacity(0),
            value: ControlValue::Float(0.5),
        };

        let json = serde_json::to_string(&msg).unwrap();
        assert!(json.contains("set_parameter"));
        assert!(json.contains("LayerOpacity"));
    }

    #[test]
    fn test_ws_server_message_serialization() {
        let msg = WsServerMessage::Stats {
            fps: 60.0,
            frame_time_ms: 16.6,
        };

        let json = serde_json::to_string(&msg).unwrap();
        assert!(json.contains("stats"));
        assert!(json.contains("60.0"));
    }

    #[test]
    fn test_ws_client_message_deserialization() {
        let json = r#"{"type":"ping"}"#;
        let msg: WsClientMessage = serde_json::from_str(json).unwrap();
        matches!(msg, WsClientMessage::Ping);
    }
}
