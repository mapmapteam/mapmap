//! Axum HTTP server

#[cfg(feature = "http-api")]
use axum::{
    http::{header, Method},
    Router,
};

#[cfg(feature = "http-api")]
use tower_http::cors::{Any, CorsLayer};

use std::net::SocketAddr;
use std::sync::Arc;

#[cfg(feature = "http-api")]
use tokio::sync::RwLock;

use crate::{error::ControlError, Result};

use super::auth::AuthConfig;
#[cfg(feature = "http-api")]
use super::routes::build_router;
#[cfg(feature = "http-api")]
use super::websocket::ws_handler;

/// Application state shared across all requests
#[derive(Clone)]
#[cfg(feature = "http-api")]
pub struct AppState {
    pub auth: Arc<RwLock<AuthConfig>>,
}

/// Web server configuration
#[derive(Debug, Clone)]
pub struct WebServerConfig {
    pub host: String,
    pub port: u16,
    pub enable_cors: bool,
    pub auth: AuthConfig,
}

impl Default for WebServerConfig {
    fn default() -> Self {
        Self {
            host: "0.0.0.0".to_string(),
            port: 8080,
            enable_cors: true,
            auth: AuthConfig::new(),
        }
    }
}

impl WebServerConfig {
    /// Create a new web server config
    pub fn new(port: u16) -> Self {
        Self {
            port,
            ..Default::default()
        }
    }

    /// Set the host address
    pub fn with_host(mut self, host: String) -> Self {
        self.host = host;
        self
    }

    /// Set CORS enabled/disabled
    pub fn with_cors(mut self, enable: bool) -> Self {
        self.enable_cors = enable;
        self
    }

    /// Set authentication config
    pub fn with_auth(mut self, auth: AuthConfig) -> Self {
        self.auth = auth;
        self
    }
}

/// Web server for REST API and WebSocket
pub struct WebServer {
    #[cfg(feature = "http-api")]
    config: WebServerConfig,
}

impl WebServer {
    /// Create a new web server
    #[cfg(feature = "http-api")]
    pub fn new(config: WebServerConfig) -> Self {
        Self { config }
    }

    #[cfg(not(feature = "http-api"))]
    pub fn new(_config: WebServerConfig) -> Self {
        Self {}
    }

    /// Run the web server (blocking)
    #[cfg(feature = "http-api")]
    pub async fn run(self) -> Result<()> {
        let addr: SocketAddr = format!("{}:{}", self.config.host, self.config.port)
            .parse()
            .map_err(|e| ControlError::HttpError(format!("Invalid address: {}", e)))?;

        let state = AppState {
            auth: Arc::new(RwLock::new(self.config.auth.clone())),
        };

        // Build router with state
        let app = build_router()
            .route("/ws", axum::routing::get(ws_handler))
            .with_state(state);

        // Add CORS if enabled
        let app = if self.config.enable_cors {
            let cors = CorsLayer::new()
                .allow_origin(Any)
                .allow_methods([Method::GET, Method::POST, Method::PATCH, Method::DELETE])
                .allow_headers([header::CONTENT_TYPE, header::AUTHORIZATION]);

            app.layer(cors)
        } else {
            app
        };

        tracing::info!("Web server listening on {}", addr);

        // Bind to the TCP listener
        let listener = tokio::net::TcpListener::bind(addr)
            .await
            .map_err(|e| ControlError::HttpError(format!("Failed to bind: {}", e)))?;

        // Serve the application - use into_make_service()
        let make_service = app.into_make_service();
        axum::serve(listener, make_service)
            .await
            .map_err(|e| ControlError::HttpError(format!("Server error: {}", e)))?;

        Ok(())
    }

    #[cfg(not(feature = "http-api"))]
    pub async fn run(self) -> Result<()> {
        Err(ControlError::HttpError(
            "HTTP API feature not enabled".to_string(),
        ))
    }

    /// Spawn the server in a background task
    #[cfg(feature = "http-api")]
    pub fn spawn(self) -> tokio::task::JoinHandle<Result<()>> {
        tokio::spawn(async move { self.run().await })
    }

    #[cfg(not(feature = "http-api"))]
    pub fn spawn(self) -> Result<()> {
        Err(ControlError::HttpError(
            "HTTP API feature not enabled".to_string(),
        ))
    }
}

#[cfg(all(test, feature = "http-api"))]
mod tests {
    use super::*;

    #[test]
    fn test_web_server_config() {
        let config = WebServerConfig::new(8080)
            .with_host("127.0.0.1".to_string())
            .with_cors(false);

        assert_eq!(config.host, "127.0.0.1");
        assert_eq!(config.port, 8080);
        assert!(!config.enable_cors);
    }

    #[tokio::test]
    async fn test_web_server_creation() {
        let config = WebServerConfig::new(18080);
        let _server = WebServer::new(config);
        // Server created successfully
    }
}
