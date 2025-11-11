#!/bin/bash
# Diagnostic script to check FFmpeg build environment

echo "=== FFmpeg Build Environment Diagnostic ==="
echo ""

echo "1. Checking FFmpeg runtime:"
if command -v ffmpeg &> /dev/null; then
    ffmpeg -version | head -1
    echo "   ✅ FFmpeg runtime installed"
else
    echo "   ❌ FFmpeg runtime NOT found"
fi
echo ""

echo "2. Checking pkg-config:"
if command -v pkg-config &> /dev/null; then
    echo "   ✅ pkg-config found: $(pkg-config --version)"
else
    echo "   ❌ pkg-config NOT found (install with: sudo apt-get install pkg-config)"
    exit 1
fi
echo ""

echo "3. Checking for libavutil (core FFmpeg library):"
if pkg-config --exists libavutil; then
    echo "   ✅ libavutil found: $(pkg-config --modversion libavutil)"
else
    echo "   ❌ libavutil NOT found by pkg-config"
    echo "   This means the build environment cannot see FFmpeg dev libraries"
fi
echo ""

echo "4. Checking for all required FFmpeg libraries:"
REQUIRED_LIBS="libavcodec libavformat libavutil libswscale libavdevice libavfilter"
ALL_FOUND=true
for lib in $REQUIRED_LIBS; do
    if pkg-config --exists $lib; then
        VERSION=$(pkg-config --modversion $lib)
        echo "   ✅ $lib: $VERSION"
    else
        echo "   ❌ $lib: NOT FOUND"
        ALL_FOUND=false
    fi
done
echo ""

echo "5. Searching for .pc files on filesystem:"
PC_FILES=$(find /usr /opt 2>/dev/null | grep -E "libav(codec|format|util|device|filter|swscale)\.pc$" | head -5)
if [ -n "$PC_FILES" ]; then
    echo "   Found .pc files:"
    echo "$PC_FILES" | while read file; do
        echo "      $file"
    done
    echo ""
    echo "   If files found but pkg-config can't see them, try:"
    FIRST_DIR=$(echo "$PC_FILES" | head -1 | xargs dirname)
    echo "   export PKG_CONFIG_PATH=$FIRST_DIR"
else
    echo "   ❌ No .pc files found - packages may not be installed"
fi
echo ""

echo "6. Current PKG_CONFIG_PATH:"
if [ -z "$PKG_CONFIG_PATH" ]; then
    echo "   (not set - using defaults)"
else
    echo "   $PKG_CONFIG_PATH"
fi
echo ""

echo "=== Summary ==="
if [ "$ALL_FOUND" = true ]; then
    echo "✅ All FFmpeg libraries found! You should be able to build with:"
    echo "   cargo build --release --features ffmpeg"
else
    echo "❌ Some FFmpeg libraries missing from build environment"
    echo ""
    echo "Possible solutions:"
    echo "1. Install packages: ./install-ffmpeg-dev.sh"
    echo "2. Check if running in isolated environment (container/VM)"
    echo "3. Set PKG_CONFIG_PATH if .pc files exist but aren't found"
    echo ""
    echo "See FFMPEG_SETUP.md for detailed troubleshooting"
fi
