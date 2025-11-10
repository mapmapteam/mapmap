use criterion::{black_box, criterion_group, criterion_main, Criterion};
use mapmap_media::{FFmpegDecoder, VideoDecoder};

fn bench_video_decode(c: &mut Criterion) {
    c.benchmark_group("video_decode")
        .bench_function("decode_frame_1080p", |b| {
            let mut decoder = FFmpegDecoder {
                width: 1920,
                height: 1080,
                duration: std::time::Duration::from_secs(60),
                fps: 30.0,
                current_time: std::time::Duration::ZERO,
                frame_count: 0,
            };

            b.iter(|| {
                let frame = decoder.next_frame().unwrap();
                black_box(frame);
            });
        });
}

criterion_group!(benches, bench_video_decode);
criterion_main!(benches);
