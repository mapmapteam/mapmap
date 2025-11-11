#!/bin/bash
# Install FFmpeg development libraries for MapMap video support

echo "Installing FFmpeg development libraries..."
echo ""

# Ubuntu/Debian
sudo apt-get update
sudo apt-get install -y \
    libavcodec-dev \
    libavformat-dev \
    libavutil-dev \
    libswscale-dev \
    libavdevice-dev \
    libavfilter-dev \
    pkg-config

echo ""
echo "✅ FFmpeg development libraries installed!"
echo ""
echo "Now rebuild MapMap with FFmpeg support:"
echo "  cd /home/user/mapmap"
echo "  cargo build --release --features ffmpeg"
echo ""
echo "Then run with:"
echo "  cargo run --release --features ffmpeg"
