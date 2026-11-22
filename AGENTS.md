# AGENTS.md — working in the MapMap repository

Instructions for coding agents, and a quick start for new contributors. Human-oriented details are in `CONTRIBUTING.md` and `INSTALL.md`; where they disagree with this file, this file is more recent.

## The project

MapMap is a free video mapping software: it projects videos, images and colors onto shapes that the user distorts to fit real surfaces.

- Language and toolkit: C++17 and Qt 6 (widgets, OpenGL, Qt Multimedia), built with qmake.

- License: GNU GPL version 3.

- Current version: see `MAPMAP_VERSION` in `mapmap.pro`. The file `VERSION.txt` is outdated.

## Build

macOS, with Qt 6 from Homebrew (run `bash scripts/sh_install_deps_macos.sh` once):

```
bash scripts/sh_build_macos.sh
```

GNU/Linux, with the packages listed in `INSTALL.md`:

```
qmake6 mapmap.pro
make -j"$(nproc)"
```

Windows: see `INSTALL.md` and the `build-windows` job in `.github/workflows/build.yml`.

## Package for a release

```
bash scripts/sh_package_macos.sh
bash scripts/sh_package_linux.sh
```

The first one writes `dist/MapMap-<version>-macos-<processor>.dmg`, a disk image with MapMap and the Qt libraries it needs. The second one writes `dist/mapmap-<version>-linux-<processor>.tar.gz`, the program without the Qt libraries. Both build the last commit in a clean copy of the sources; run them with `--help` for their options. The automated builds run them with `--skip-build` for every pull request.

Without `--sign-identity`, the macOS disk image is only signed ad hoc: macOS warns that the developer cannot be verified. A public release needs a "Developer ID Application" identity and Apple's notarization.

## Test

```
cd tests
qmake6 tests.pro
make
QT_QPA_PLATFORM=offscreen make check
```

The same commands run on macOS, Ubuntu and Windows for every pull request. A pull request whose checks fail is not merged.

## Git rules

- The base branch is `dev`. Never commit directly to `dev` or `master`.

- One branch per topic, cut from an up-to-date `origin/dev`, named `type-number-few-words` with the GitHub issue number when there is one: `fix-555-idle-cpu`, `feat-flip-horizontal`. Types: `feat`, `fix`, `doc`, `refactor`, `release`.

- Rebase only. Never merge `dev` into a branch, never a plain `git pull`. Pull requests are integrated with "Rebase and merge".

- Every commit builds and passes the tests, not only the last one.

- Commit messages and pull request titles are in English and start with a verb in the present tense, like an instruction: `Fix projects with a camera source failing to reopen (#542)`. Write `Fix`, `Add`, `Remove`, not `Fixed`, `Added`, `Removed`. Name the issue when there is one. Older commits use the past tense: leave them as they are.

- One idea per commit. An unrelated change goes in its own commit.

- An agent proposes its commits and waits for the maintainer's approval before committing, and never pushes or opens a pull request without being asked to.

- No attribution line for an AI tool in commits or pull requests.

## Where things are

| Folder | Contents |
|---|---|
| `src/core` | The model: sources (`Source.h`: color, image, video, camera, Syphon), layers, the project reader and writer |
| `src/shape` | Shapes: triangle, mesh, ellipse |
| `src/gui` | The main window, the canvases, the property editors |
| `src/control` | Remote control: OSC (`OscAction`, `OscInterface`) and the MCP server |
| `src/app` | `main.cpp`: command-line options, start-up |
| `tests` | Unit tests (Qt Test), one `Test*.cpp` per area |
| `docs/informations` | Documentation shown inside the software (OSC commands, keyboard shortcuts) |
| `scripts` | Build and release scripts |

## When you change something

- A new OSC command adds cases to `tests/TestOsc.cpp` and a line to `docs/informations/osc.md`.

- A new property saved in project files adds a case to the project round-trip test.

- A project file can be written by hand or damaged: reading it must never crash. Skip what cannot be read, with a warning.

- Follow the existing style: Qt coding style, two-space indentation, code and comments in English.

- Document every public class and function. Give every number used in a calculation a named constant.

- Text shown to the user goes through `tr()`.

## Things to know before running MapMap

- Project files (`.mmp`) are JSON. Older XML projects are not supported.

- Settings are the user's own: never run MapMap with `--reset-settings` on someone's machine without asking.

- Log messages go to the console window of the software and to `mapmap.log` in the system's temporary folder, not to the terminal.

- OSC listens on port 12345, on the local machine only unless the user allows the network. The MCP server is off unless a port is set in the preferences or with `--mcp-port`.

- Opening a camera source turns on the user's camera.

## Outdated files

- `CONTRIBUTING.md` still mentions the `develop` branch and a `NEWS` file. The base branch is `dev`, and release notes are in `CHANGELOG.md`.

- `scripts/build.sh` and `scripts/run.sh` date from Qt 5 and no longer work as written.

- `VERSION.txt` still says 0.6.3.
