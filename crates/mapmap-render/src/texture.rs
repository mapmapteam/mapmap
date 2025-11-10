//! Texture management and pooling

use std::collections::HashMap;
use std::sync::Arc;
use parking_lot::RwLock;

/// Handle to a GPU texture
#[derive(Clone)]
pub struct TextureHandle {
    pub id: u64,
    pub texture: Arc<wgpu::Texture>,
    pub width: u32,
    pub height: u32,
    pub format: wgpu::TextureFormat,
}

impl TextureHandle {
    /// Create a texture view
    pub fn create_view(&self) -> wgpu::TextureView {
        self.texture.create_view(&wgpu::TextureViewDescriptor::default())
    }

    /// Get texture size in bytes
    pub fn size_bytes(&self) -> u64 {
        let bytes_per_pixel = match self.format {
            wgpu::TextureFormat::Rgba8Unorm | wgpu::TextureFormat::Rgba8UnormSrgb => 4,
            wgpu::TextureFormat::Bgra8Unorm | wgpu::TextureFormat::Bgra8UnormSrgb => 4,
            _ => 4, // Default to 4 bytes
        };
        (self.width * self.height * bytes_per_pixel) as u64
    }
}

/// Texture descriptor
#[derive(Debug, Clone, Copy)]
pub struct TextureDescriptor {
    pub width: u32,
    pub height: u32,
    pub format: wgpu::TextureFormat,
    pub usage: wgpu::TextureUsages,
    pub mip_levels: u32,
}

impl Default for TextureDescriptor {
    fn default() -> Self {
        Self {
            width: 1,
            height: 1,
            format: wgpu::TextureFormat::Rgba8UnormSrgb,
            usage: wgpu::TextureUsages::TEXTURE_BINDING | wgpu::TextureUsages::COPY_DST,
            mip_levels: 1,
        }
    }
}

/// Texture pool for reusing allocations
pub struct TexturePool {
    textures: RwLock<HashMap<u64, TextureHandle>>,
    free_list: RwLock<Vec<TextureHandle>>,
    max_pool_size: usize,
}

impl TexturePool {
    pub fn new(max_pool_size: usize) -> Self {
        Self {
            textures: RwLock::new(HashMap::new()),
            free_list: RwLock::new(Vec::new()),
            max_pool_size,
        }
    }

    /// Get a texture from the pool or create a new one
    pub fn acquire(&self, desc: TextureDescriptor, device: &wgpu::Device) -> TextureHandle {
        // Try to find a matching texture in the free list
        let mut free_list = self.free_list.write();

        if let Some(idx) = free_list.iter().position(|t| {
            t.width == desc.width && t.height == desc.height && t.format == desc.format
        }) {
            let handle = free_list.swap_remove(idx);
            return handle;
        }

        // No matching texture found, create a new one
        drop(free_list);

        static COUNTER: std::sync::atomic::AtomicU64 = std::sync::atomic::AtomicU64::new(0);
        let id = COUNTER.fetch_add(1, std::sync::atomic::Ordering::Relaxed);

        let texture = device.create_texture(&wgpu::TextureDescriptor {
            label: Some(&format!("Pooled Texture {}", id)),
            size: wgpu::Extent3d {
                width: desc.width,
                height: desc.height,
                depth_or_array_layers: 1,
            },
            mip_level_count: desc.mip_levels,
            sample_count: 1,
            dimension: wgpu::TextureDimension::D2,
            format: desc.format,
            usage: desc.usage,
            view_formats: &[],
        });

        let handle = TextureHandle {
            id,
            texture: Arc::new(texture),
            width: desc.width,
            height: desc.height,
            format: desc.format,
        };

        let mut textures = self.textures.write();
        textures.insert(id, handle.clone());

        handle
    }

    /// Return a texture to the pool
    pub fn release(&self, handle: TextureHandle) {
        let mut free_list = self.free_list.write();

        // Only keep textures if we haven't exceeded max pool size
        if free_list.len() < self.max_pool_size {
            free_list.push(handle);
        }
    }

    /// Clear the entire pool
    pub fn clear(&self) {
        self.textures.write().clear();
        self.free_list.write().clear();
    }

    /// Get current pool statistics
    pub fn stats(&self) -> PoolStats {
        PoolStats {
            total_textures: self.textures.read().len(),
            free_textures: self.free_list.read().len(),
            total_memory: self
                .textures
                .read()
                .values()
                .map(|t| t.size_bytes())
                .sum(),
        }
    }
}

/// Pool statistics
#[derive(Debug, Clone)]
pub struct PoolStats {
    pub total_textures: usize,
    pub free_textures: usize,
    pub total_memory: u64,
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_texture_descriptor_default() {
        let desc = TextureDescriptor::default();
        assert_eq!(desc.width, 1);
        assert_eq!(desc.height, 1);
        assert_eq!(desc.mip_levels, 1);
    }

    #[test]
    fn test_texture_pool() {
        let pool = TexturePool::new(10);
        let stats = pool.stats();
        assert_eq!(stats.total_textures, 0);
        assert_eq!(stats.free_textures, 0);
    }
}
