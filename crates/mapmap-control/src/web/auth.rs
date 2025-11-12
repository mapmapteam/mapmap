//! Authentication for web API
//!
//! Provides optional API key authentication for the web control interface.

use serde::{Deserialize, Serialize};
use std::collections::HashSet;

/// Authentication configuration
#[derive(Debug, Clone, Default, Serialize, Deserialize)]
pub struct AuthConfig {
    /// Enable authentication
    pub enabled: bool,
    /// API keys (plain text for simplicity; use hashed keys in production)
    pub api_keys: HashSet<String>,
}

impl AuthConfig {
    /// Create a new auth config with authentication disabled
    pub fn new() -> Self {
        Self::default()
    }

    /// Create an auth config with authentication enabled
    pub fn with_keys(keys: Vec<String>) -> Self {
        Self {
            enabled: true,
            api_keys: keys.into_iter().collect(),
        }
    }

    /// Add an API key
    pub fn add_key(&mut self, key: String) {
        self.api_keys.insert(key);
        self.enabled = true;
    }

    /// Remove an API key
    pub fn remove_key(&mut self, key: &str) -> bool {
        self.api_keys.remove(key)
    }

    /// Validate an API key
    pub fn validate(&self, key: &str) -> bool {
        if !self.enabled {
            return true; // No auth required
        }
        self.api_keys.contains(key)
    }

    /// Check if authentication is enabled
    pub fn is_enabled(&self) -> bool {
        self.enabled
    }
}

/// Extract API key from various sources
pub fn extract_api_key(headers: &http::HeaderMap, query: Option<&str>) -> Option<String> {
    // Try Authorization header first (Bearer token)
    if let Some(auth_header) = headers.get(http::header::AUTHORIZATION) {
        if let Ok(auth_str) = auth_header.to_str() {
            if let Some(token) = auth_str.strip_prefix("Bearer ") {
                return Some(token.to_string());
            }
        }
    }

    // Try X-API-Key header
    if let Some(api_key_header) = headers.get("X-API-Key") {
        if let Ok(key) = api_key_header.to_str() {
            return Some(key.to_string());
        }
    }

    // Try query parameter
    if let Some(q) = query {
        if let Some(key) = parse_api_key_from_query(q) {
            return Some(key);
        }
    }

    None
}

fn parse_api_key_from_query(query: &str) -> Option<String> {
    for param in query.split('&') {
        if let Some((key, value)) = param.split_once('=') {
            if key == "api_key" {
                return Some(value.to_string());
            }
        }
    }
    None
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_auth_config() {
        let mut config = AuthConfig::new();
        assert!(!config.is_enabled());
        assert!(config.validate("any_key"));

        config.add_key("test_key".to_string());
        assert!(config.is_enabled());
        assert!(config.validate("test_key"));
        assert!(!config.validate("wrong_key"));
    }

    #[test]
    fn test_extract_bearer_token() {
        let mut headers = http::HeaderMap::new();
        headers.insert(
            http::header::AUTHORIZATION,
            "Bearer test_token".parse().unwrap(),
        );

        let key = extract_api_key(&headers, None);
        assert_eq!(key, Some("test_token".to_string()));
    }

    #[test]
    fn test_extract_api_key_header() {
        let mut headers = http::HeaderMap::new();
        headers.insert("X-API-Key", "test_key".parse().unwrap());

        let key = extract_api_key(&headers, None);
        assert_eq!(key, Some("test_key".to_string()));
    }

    #[test]
    fn test_extract_query_param() {
        let headers = http::HeaderMap::new();
        let key = extract_api_key(&headers, Some("foo=bar&api_key=test_key"));
        assert_eq!(key, Some("test_key".to_string()));
    }
}
