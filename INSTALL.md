Build instructions
==================

Build on GNU/Linux
------------------

Install the dependencies.

Build it:

```
qmake mapmap.pro
make
```

Alternatively:

```
./scripts/build.sh
```

### Ubuntu

NOTE: Tested on 13.10, 14.04, 15.04 and 16.04

Install basic development tools for Qt projects, plus liblo for OSC support:

```
sudo apt-get install -y \
      liblo-dev liblo-tools \
      qttools5-dev-tools \
      qtmultimedia5-dev \
      libqt5opengl5-dev \
      qtwebengine5-dev \
      libqt5multimedia5-plugins \
      qt5-default
```

Install GStreamer 1.0 libraries and plugins:

```
sudo apt-get install -y \
      libgstreamer1.0-dev \
      libgstreamer-plugins-base1.0-dev \
      gstreamer1.0-plugins-bad \
      gstreamer1.0-libav \
      gstreamer1.0-vaapi \
      gstreamer1.0-plugins-base \
      gstreamer1.0-plugins-base-apps \
      gstreamer1.0-plugins-good \
      gstreamer1.0-plugins-ugly \
      gstreamer1.0-x \
      gstreamer1.0-tools
```

Install extra packages if you want to build the documentation:

```
sudo apt-get install -y \
      doxygen \
      graphviz \
      rst2pdf \
      markdown
```

### Arch Linux

Install basic development tools for Qt projects, GStreamer 1.0 and liblo for OSC support:

```
sudo pacman -S qt5-tools qt5-multimedia qt5-webengine liblo gstreamer
```

Install GStreamer 1.0 libraries and plugins::

```
sudo pacman -S gst-libav \
               gstreamer-vaapi \
               gst-plugins-bad \
               gst-plugins-base \
               gst-plugins-base-libs \
               gst-plugins-good \
               gst-plugins-ugly
```

Build on Mac OS X
-----------------

NOTE: This has been tested on OS X 10.11 (El Capitan).

Install tools and dependencies:

1) Install the Apple Developer Tools
  - You need to register with a credit card in the Apple Store in order to download it (no need to pay, but Apple requires your credit card number).
2) Install Qt5
  - You can get the open source version from http://www.qt.io/download-open-source/
  - Run the installer and choose the default location (which should be ~/Qt).
  - Latest tested version: 5.5.1=
3) Install liblo
  - Use the following guide: http://macappstore.org/liblo/
  - OR compile from the tar.gz - it should install it to /usr/local
4) Install the GStreamer framework. You need the runtime and devel packages to be installed:
  - https://gstreamer.freedesktop.org/data/pkg/osx/1.6.0/gstreamer-1.0-1.6.0-x86_64.pkg
  - https://gstreamer.freedesktop.org/data/pkg/osx/1.6.0/gstreamer-1.0-devel-1.6.0-x86_64.pkg
  - http://gstreamer.freedesktop.org/data/pkg/osx/1.6.0/gstreamer-1.0-1.6.0-x86_64-packages.dmg

Do this:

```
./scripts/build.sh
```

It will create a .app and a .dmg.

DMGVERSION.txt should be created automatically with "1" as its contents. Update to "2", and so on, if needed.

### Troubleshooting

#### GStreamer header not found

If you have a compilation error saying that file ```<gst/gst.h>``` cannot be found: make sure your GStreamer.framework folder is installed and is _not_ read-protected.

#### Corrupted OSC port

If the appearance of the window of the OSC port number in the preferences seem corrupted, you might want to reset MapMap's preferences:

```
rm -f ~/Library/Preferences/info.mapmap.MapMap.plist
```

To print debugging informations, launch it from the Terminal app like this::

```
GSTPLUGIN_PATH=/Library/Frameworks/GStreamer.framework/Libraries GST_DEBUG=2 /Applications/MapMap.app/Contents/MacOS/MapMap
```

Build on Windows
----------------

## Build dynamic version to debug project:
- Download and install gstreamer-x86 [runtime](https://gstreamer.freedesktop.org/data/pkg/windows/1.16.2/gstreamer-1.0-mingw-x86-1.16.2.msi) and [devel](https://gstreamer.freedesktop.org/data/pkg/windows/1.16.2/gstreamer-1.0-devel-mingw-x86-1.16.2.msi)
- Download and install [Qt5 MinGW incl. QtCreator](https://www.qt.io/download-thank-you?os=windows)
- Add the GStreamer bin path (e.g. C:\gstreamer\1.0\x86\bin) to PATH variable into the QtCreator project build enviroment settings
- Build and run MapMap project within QtCreator (Ctrl-R)

## Build static version for release:
- Download and install gstreamer-x86 [runtime](https://gstreamer.freedesktop.org/data/pkg/windows/1.16.2/gstreamer-1.0-mingw-x86-1.16.2.msi) and [devel](https://gstreamer.freedesktop.org/data/pkg/windows/1.16.2/gstreamer-1.0-devel-mingw-x86-1.16.2.msi)
- Build a [Qt static environment](https://wiki.qt.io/Building_a_static_Qt_for_Windows_using_MinGW) (This [video](https://www.youtube.com/watch?v=nEQGrBiz2T0) may explain it better)
- Build MapMap using QtCreator (qmake, build release)
- Copy all dll files of the gstreamer's bin folder (e.g. C:\gstreamer\1.0\x86\bin) into the target folder together with MapMap.exe
- Copy all dll files of the gstreamer's plugin folder (e.g. C:\gstreamer\1.0\x86\lib\gstreamer-1.0) into an new folder named 'plugin' in parallel to MapMap.exe.
- Run MapMap.exe

#### For packaging
- Open Qt terminal via a Start Menu and run the following command:
`windeployqt --release --no-system-d3d-compiler <path-to-app-binary>`
- Replace all the `*.qm` files that exists in `released-binary`/translations folder by the ones from `source-code`/translations folder
- Download and install [Inno Setup](https://jrsoftware.org/isdl.php) to create an installation wizard setup

Editing translations
--------------------
You might need to update the files:
  
```
cd src/mapmap
lupdate mapmap.pro 
```

Then, do this:

```  
lrelease mapmap.pro
```
