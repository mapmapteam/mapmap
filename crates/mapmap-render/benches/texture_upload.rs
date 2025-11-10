use criterion::{black_box, criterion_group, criterion_main, Criterion, BenchmarkId};
use mapmap_render::{TextureDescriptor, WgpuBackend};

fn bench_texture_upload(c: &mut Criterion) {
    let mut group = c.benchmark_group("texture_upload");

    let backend = pollster::block_on(WgpuBackend::new()).unwrap();

    for size in [512, 1024, 1920].iter() {
        let data = vec![0u8; (size * size * 4) as usize];

        group.bench_with_input(
            BenchmarkId::from_parameter(format!("{}x{}", size, size)),
            size,
            |b, &size| {
                let desc = TextureDescriptor {
                    width: size,
                    height: size,
                    format: wgpu::TextureFormat::Rgba8UnormSrgb,
                    usage: wgpu::TextureUsages::TEXTURE_BINDING | wgpu::TextureUsages::COPY_DST,
                    mip_levels: 1,
                };

                b.iter(|| {
                    let mut backend_clone = backend.clone();
                    // Note: This is a stub benchmark. Full implementation would measure actual upload time
                    black_box(&data);
                });
            },
        );
    }

    group.finish();
}

criterion_group!(benches, bench_texture_upload);
criterion_main!(benches);
