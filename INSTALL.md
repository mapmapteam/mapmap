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

Install basic development tools for Qt 6 projects, plus liblo for OSC support:

```
sudo apt-get install -y \
      liblo-dev liblo-tools \
      qt6-tools-dev \
      qt6-multimedia-dev \
      qt6-base-dev \
      libqt6opengl6-dev \
      libqt6openglwidgets6
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

Install basic development tools for Qt 6 projects and liblo for OSC support:

```
sudo pacman -S qt6-tools qt6-multimedia qt6-base liblo
```

Build on macOS
--------------

Install tools and dependencies:

```
./scripts/sh_install_deps_macos.sh
```

Build:

```
./scripts/sh_build_macos.sh
```

### Troubleshooting

#### Corrupted OSC port

If the appearance of the window of the OSC port number in the preferences seem corrupted, you might want to reset MapMap's preferences:

```
rm -f ~/Library/Preferences/info.mapmap.MapMap.plist
```

Build on Windows
----------------

## Build dynamic version to debug project:
- Download and install [Qt6 MinGW incl. QtCreator](https://www.qt.io/download-open-source/)
- Build and run MapMap project within QtCreator (Ctrl-R)

## Build static version for release:
- Build a [Qt static environment](https://wiki.qt.io/Building_a_static_Qt_for_Windows_using_MinGW)
- Build MapMap using QtCreator (qmake, build release)
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
