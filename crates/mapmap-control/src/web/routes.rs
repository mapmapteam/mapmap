//! REST API route definitions

#[cfg(feature = "http-api")]
use axum::{
    extract::{Path, State},
    http::StatusCode,
    response::Json,
    routing::{get, patch},
    Router,
};

#[cfg(feature = "http-api")]
use super::handlers::{ApiResponse, LayerInfo, StatusResponse, UpdateLayerRequest};
#[cfg(feature = "http-api")]
use super::server::AppState;

/// Build the API router
#[cfg(feature = "http-api")]
pub fn build_router() -> Router<AppState> {
    Router::new()
        .route("/api/status", get(get_status))
        .route("/api/layers", get(get_layers))
        .route("/api/layers/:id", get(get_layer))
        .route("/api/layers/:id", patch(update_layer))
        .route("/api/paints", get(get_paints))
        .route("/api/effects", get(get_effects))
}

#[cfg(not(feature = "http-api"))]
pub fn build_router() -> () {
    ()
}

/// GET /api/status - Get system status
#[cfg(feature = "http-api")]
async fn get_status(State(_state): State<AppState>) -> Json<ApiResponse<StatusResponse>> {
    // In a real implementation, this would query the actual system state
    let status = StatusResponse {
        version: env!("CARGO_PKG_VERSION").to_string(),
        uptime_seconds: 0, // TODO: Track actual uptime
        active_layers: 0,  // TODO: Get from project
        fps: 60.0,         // TODO: Get actual FPS
    };

    Json(ApiResponse::success(status))
}

/// GET /api/layers - List all layers
#[cfg(feature = "http-api")]
async fn get_layers(State(_state): State<AppState>) -> Json<ApiResponse<Vec<LayerInfo>>> {
    // In a real implementation, this would query the actual layers
    let layers = vec![
        LayerInfo {
            id: 0,
            name: "Layer 1".to_string(),
            opacity: 1.0,
            visible: true,
        },
        LayerInfo {
            id: 1,
            name: "Layer 2".to_string(),
            opacity: 0.75,
            visible: true,
        },
    ];

    Json(ApiResponse::success(layers))
}

/// GET /api/layers/:id - Get layer details
#[cfg(feature = "http-api")]
async fn get_layer(
    Path(id): Path<u32>,
    State(_state): State<AppState>,
) -> Result<Json<ApiResponse<LayerInfo>>, StatusCode> {
    // In a real implementation, this would query the actual layer
    let layer = LayerInfo {
        id,
        name: format!("Layer {}", id + 1),
        opacity: 1.0,
        visible: true,
    };

    Ok(Json(ApiResponse::success(layer)))
}

/// PATCH /api/layers/:id - Update layer parameters
#[cfg(feature = "http-api")]
async fn update_layer(
    Path(id): Path<u32>,
    State(_state): State<AppState>,
    Json(request): Json<UpdateLayerRequest>,
) -> Result<Json<ApiResponse<LayerInfo>>, StatusCode> {
    if request.is_empty() {
        return Err(StatusCode::BAD_REQUEST);
    }

    // In a real implementation, this would update the actual layer
    tracing::info!("Updating layer {}: {:?}", id, request);

    // Return updated layer info
    let layer = LayerInfo {
        id,
        name: format!("Layer {}", id + 1),
        opacity: request.opacity.unwrap_or(1.0),
        visible: request.visible.unwrap_or(true),
    };

    Ok(Json(ApiResponse::success(layer)))
}

/// GET /api/paints - List all paints
#[cfg(feature = "http-api")]
async fn get_paints(State(_state): State<AppState>) -> Json<ApiResponse<Vec<String>>> {
    // In a real implementation, this would query the actual paints
    let paints = vec!["Paint 1".to_string(), "Paint 2".to_string()];

    Json(ApiResponse::success(paints))
}

/// GET /api/effects - List all effects
#[cfg(feature = "http-api")]
async fn get_effects(State(_state): State<AppState>) -> Json<ApiResponse<Vec<String>>> {
    // In a real implementation, this would query the actual effects
    let effects = vec!["Effect 1".to_string(), "Effect 2".to_string()];

    Json(ApiResponse::success(effects))
}

#[cfg(all(test, feature = "http-api"))]
mod tests {
    use super::*;
    use std::sync::Arc;
    use tokio::sync::RwLock;

    #[tokio::test]
    async fn test_get_status() {
        let state = AppState {
            auth: Arc::new(RwLock::new(super::super::auth::AuthConfig::new())),
        };

        let response = get_status(State(state)).await;
        assert!(response.0.success);
    }

    #[tokio::test]
    async fn test_get_layers() {
        let state = AppState {
            auth: Arc::new(RwLock::new(super::super::auth::AuthConfig::new())),
        };

        let response = get_layers(State(state)).await;
        assert!(response.0.success);
        assert!(response.0.data.is_some());
    }
}
