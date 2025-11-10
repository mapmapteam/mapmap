# MapMap Installation Guide for Ubuntu 24.04

This guide provides step-by-step instructions for building and installing MapMap on Ubuntu 24.04 LTS (Noble Numbat).

## Prerequisites

- Ubuntu 24.04 LTS
- Sudo access for package installation
- Approximately 500 MB of free disk space
- Graphical display (X11 or Wayland)

## Installation Steps

### Step 1: Install Qt5 and Development Tools

Install the Qt5 framework and development tools required for building MapMap:

```bash
sudo apt-get update
sudo apt-get install -y \
      liblo-dev \
      qttools5-dev-tools \
      qtmultimedia5-dev \
      libqt5opengl5-dev \
      qtwebengine5-dev \
      libqt5multimedia5-plugins \
      qtbase5-dev
```

**What this installs:**
- Qt 5.15.x - Cross-platform application framework
- liblo - Open Sound Control (OSC) library
- Qt development tools including qmake

### Step 2: Install GStreamer 1.0 Libraries and Plugins

Install GStreamer for video playback and processing:

```bash
sudo apt-get install -y \
      libgstreamer1.0-dev \
      libgstreamer-plugins-base1.0-dev \
      gstreamer1.0-plugins-bad \
      gstreamer1.0-libav \
      gstreamer1.0-plugins-base \
      gstreamer1.0-plugins-base-apps \
      gstreamer1.0-plugins-good \
      gstreamer1.0-plugins-ugly \
      gstreamer1.0-x \
      gstreamer1.0-tools
```

**What this installs:**
- GStreamer 1.24.x - Multimedia framework
- Video codec plugins (H.264, VP8, VP9, etc.)
- Audio codec plugins
- GStreamer development headers

### Step 3: Verify Dependencies

Confirm that the required tools are installed correctly:

```bash
qmake --version
```

Expected output:
```
QMake version 3.1
Using Qt version 5.15.13 in /usr/lib/x86_64-linux-gnu
```

Check GStreamer:

```bash
gst-inspect-1.0 --version
```

Expected output:
```
gst-inspect-1.0 version 1.24.2
GStreamer 1.24.2
```

### Step 4: Build MapMap

Navigate to the MapMap source directory and build:

```bash
cd /path/to/mapmap
qmake mapmap.pro
make -j$(nproc)
```

**Build process:**
- `qmake` generates the Makefile from the Qt project file
- `make -j$(nproc)` compiles using all available CPU cores
- Build time: 2-3 minutes on a modern system
- The resulting binary will be approximately 41 MB

### Step 5: Verify the Build

Check that the executable was created successfully:

```bash
ls -lh mapmap
```

Expected output:
```
-rwxr-xr-x 1 user user 41M Nov 10 12:28 mapmap
```

Test the application:

```bash
./mapmap --version
```

Expected output:
```
MapMap 0.6.3
```

View available command-line options:

```bash
./mapmap --help
```

### Step 6: Run MapMap

Launch the application:

```bash
./mapmap
```

Or open a specific project file:

```bash
./mapmap myproject.mmp
```

## Command-Line Options

```
Usage: ./mapmap [options] file

Options:
  -h, --help                     Display help on commandline options
  --help-all                     Display help including Qt specific options
  -v, --version                  Display version information
  -F, --fullscreen               Display the output window and make it fullscreen
  -f, --file <file>              Load project from <file>
  -R, --reset-settings           Reset MapMap settings, such as GUI properties
  -p, --osc-port <osc-port>      Use OSC port number <osc-port> (default: 12345)
  -l, --lang <lang>              Use language <lang> (en, fr, es, zh_CN, zh_TW)
  -r, --frame-rate <frame-rate>  Use a framerate of <frame-rate> per second

Arguments:
  file                           Load project from that file
```

## Quick Installation Script

For convenience, you can use this automated installation script:

```bash
#!/bin/bash
set -e

echo "==> Installing Qt5 and dependencies..."
sudo apt-get update
sudo apt-get install -y \
      liblo-dev \
      qttools5-dev-tools \
      qtmultimedia5-dev \
      libqt5opengl5-dev \
      qtwebengine5-dev \
      libqt5multimedia5-plugins \
      qtbase5-dev

echo "==> Installing GStreamer..."
sudo apt-get install -y \
      libgstreamer1.0-dev \
      libgstreamer-plugins-base1.0-dev \
      gstreamer1.0-plugins-bad \
      gstreamer1.0-libav \
      gstreamer1.0-plugins-base \
      gstreamer1.0-plugins-base-apps \
      gstreamer1.0-plugins-good \
      gstreamer1.0-plugins-ugly \
      gstreamer1.0-x \
      gstreamer1.0-tools

echo "==> Verifying installations..."
qmake --version
gst-inspect-1.0 --version

echo "==> Building MapMap..."
qmake mapmap.pro
make -j$(nproc)

echo "==> Build complete!"
ls -lh mapmap
./mapmap --version

echo ""
echo "Installation successful! Run './mapmap' to start the application."
```

Save this as `install.sh`, make it executable, and run:

```bash
chmod +x install.sh
./install.sh
```

## Optional: Install Globally

To install MapMap system-wide (optional):

```bash
sudo cp mapmap /usr/local/bin/
```

Then you can run it from anywhere:

```bash
mapmap
```

## System Requirements

- **OS**: Ubuntu 24.04 LTS (tested) or compatible Debian-based distribution
- **CPU**: x86_64 architecture
- **RAM**: 2 GB minimum, 4 GB recommended
- **GPU**: OpenGL 2.0+ capable graphics card
- **Display**: X11 or Wayland display server

## Troubleshooting

### Qt Platform Plugin Error

If you see an error about Qt platform plugins when running on a headless system:

```bash
QT_QPA_PLATFORM=offscreen ./mapmap --help
```

### Missing Display

MapMap requires a graphical display. For remote systems, use X11 forwarding:

```bash
ssh -X user@remote
./mapmap
```

### Build Errors

If you encounter build errors, ensure all dependencies are installed:

```bash
sudo apt-get install -y build-essential
```

### GStreamer Plugin Issues

If videos don't play, install additional codecs:

```bash
sudo apt-get install -y ubuntu-restricted-extras
```

## Additional Resources

- **Project Website**: http://mapmap.info
- **Documentation**: See the `docs/` directory
- **User Manual**: `docs/mapmap-user-manual.pdf`
- **OSC API Reference**: `docs/osc-api.md`

## License

MapMap is free software licensed under the GNU GPL v3.
See the LICENSE file for details.

## Build Information

- **Tested on**: Ubuntu 24.04 LTS (Noble Numbat)
- **Qt Version**: 5.15.13
- **GStreamer Version**: 1.24.2
- **MapMap Version**: 0.6.3
- **Last Updated**: 2025-11-10
