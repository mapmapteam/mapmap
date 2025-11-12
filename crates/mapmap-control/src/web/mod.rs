//! Web control interface
//!
//! This module provides a REST API and WebSocket interface for remote control of MapMap.
//!
//! ## REST API Endpoints
//!
//! - `GET /api/status` - Get system status
//! - `GET /api/layers` - List all layers
//! - `GET /api/layers/:id` - Get layer details
//! - `PATCH /api/layers/:id` - Update layer parameters
//! - `GET /api/paints` - List all paints
//! - `GET /api/effects` - List all effects
//! - `GET /ws` - WebSocket connection for real-time updates
//!
//! ## WebSocket Messages
//!
//! ### Client to Server
//!
//! ```json
//! {
//!   "type": "set_parameter",
//!   "target": {"LayerOpacity": 0},
//!   "value": {"Float": 0.75}
//! }
//! ```
//!
//! ### Server to Client
//!
//! ```json
//! {
//!   "type": "parameter_changed",
//!   "target": {"LayerOpacity": 0},
//!   "value": {"Float": 0.75}
//! }
//! ```
//!
//! ## Example Usage
//!
//! ```rust,no_run
//! use mapmap_control::web::{WebServer, WebServerConfig};
//!
//! # #[cfg(feature = "http-api")]
//! # #[tokio::main]
//! # async fn main() -> mapmap_control::Result<()> {
//! // Create server configuration
//! let config = WebServerConfig::new(8080)
//!     .with_cors(true);
//!
//! // Create and run server
//! let server = WebServer::new(config);
//! server.run().await?;
//! # Ok(())
//! # }
//! # #[cfg(not(feature = "http-api"))]
//! # fn main() {}
//! ```
//!
//! ## Authentication
//!
//! The web API supports optional authentication via API keys. Keys can be provided via:
//! - `Authorization: Bearer <token>` header
//! - `X-API-Key: <key>` header
//! - `?api_key=<key>` query parameter
//!
//! ```rust
//! use mapmap_control::web::{WebServerConfig, auth::AuthConfig};
//!
//! let auth = AuthConfig::with_keys(vec!["my-secret-key".to_string()]);
//! let config = WebServerConfig::new(8080).with_auth(auth);
//! ```

pub mod auth;
pub mod handlers;
pub mod routes;
pub mod server;
pub mod websocket;

pub use auth::AuthConfig;
pub use handlers::{
    ApiResponse, LayerInfo, StatusResponse, UpdateLayerRequest, UpdateParameterRequest,
};
pub use server::{WebServer, WebServerConfig};
pub use websocket::{WsClientMessage, WsServerMessage};

#[cfg(feature = "http-api")]
pub use server::AppState;
