# Architecture notes

How the things the user handles are built in the code: sources, layers, shapes, settings. `docs/ux-charter.md` says what the user sees, names and touches; this document says what is behind it. `AGENTS.md` gives the folders and the rules for changing the code.

Read in the code at commit `fe5c173` (branch `dev`). It describes what exists; what is planned is listed at the end, with the questions that are still open.

## Words of the interface, names of the code

The interface was renamed; some names of the code and of the project files still carry the old words.

| The user reads | In the code | Older names still found |
|---|---|---|
| Source | `Source` (`src/core/Source.h`) | paint, media |
| Layer | `Layer` (`src/core/Layer.h`) | mapping |
| Shape | `MShape` (`src/shape/Shape.h`) | |
| Project | `MappingManager` (`src/core/MappingManager.h`), read and written by `ProjectReader` and `ProjectWriter` | |

## The model

```
  MappingManager
    sources:  Source, Source, ...
    layers:   Layer, Layer, ...        (a stack)

  Layer ----> Source                   (one source; a source can have many layers)
    |-------> output shape  (MShape)   always
    '-------> input shape   (MShape)   only when the source has a picture to choose from
```

- **`MappingManager` holds the project**: the sources, the layers in the order of their stack, and the size of the output frame. Sources and layers are found by a unique id.

- **A `Source`** is where a picture comes from: a color, an image, a video, a camera, a Syphon input.

- **A `Layer`** joins one source, one output shape and, for a source that has a picture, one input shape. Two classes exist: `ColorLayer` (no input shape) and `TextureLayer` (with one). A layer keeps its states (visible, solo) and its depth in the stack; its opacity is multiplied by the opacity of its source (`Layer::getComputedOpacity()`).

- **An `MShape`** is a list of vertices and a lock. The kinds are in `src/shape`: `Triangle`, `Quad`, `Mesh` (a grid of quads), `Ellipse`. The input shape and the output shape of a layer are of the same kind; for a mesh, the two grids are kept equal (`Layer`, `_keepingMeshGridsEqual`), so that a vertex of one goes with a vertex of the other.

- **Deleting a layer leaves its source. Deleting a source deletes its layers** (`MappingManager::getSourceLayers()` lists them).

## Settings

- **Every value the user can change is a setting**, declared once in the constructor of its class (`src/core/Setting.h`). Sources, layers and shapes are `Element` or `Serializable` objects that carry a tree of settings.

- **The id of a setting is its name everywhere**: in the project file, in OSC addresses and in MCP. This is what lets the interface and remote control change the same things under the same names.

- **The project file is JSON**, written from the tree of settings. Reading must never crash on a file written by hand or damaged.

- **Preferences are not in the project**: they are kept on the computer with `QSettings`.

## Changes and undo

- **A change made by hand is an undo command** (`src/core/Commands.h`): `MoveVertexCommand`, `TranslateShapeCommand` and the others. Commands of the same gesture merge, so that one drag, or arrow keys pressed in a row, make one step.

## The interface

| What | Where |
|---|---|
| Main window: menus, toolbar, lists, panels | `src/gui/MainWindow.cpp` |
| Input and output editors | `MapperGLCanvas` |
| Output window (the projection) | `OutputGLWindow`, `OutputGLCanvas` |
| Drawing of the outlines, vertices and handle | `ShapeControlPainter`, `ShapeGraphicsItem` |
| List of layers | `LayerListModel`, `LayerItemDelegate` |
| Panels of settings | `SourceGui`, `LayerGui`, `SettingsBrowser` |
| What a gesture means, without a window | `HandleModeKeys`, `src/shape/ShapeHandles.h` |
| Remote control | `src/control` (OSC, MCP) |

What can be decided without a window (which key does what, how far a drag turns a shape) is put in a class of its own, with a test: the mouse cannot be simulated on every machine.

## Planned, and what stays open

These are in the roadmap, each with its own slice. The charter fixes their words and what the user should expect; the points below are choices of architecture that the charter leaves to those slices.

**Groups of layers.** Layers become a hierarchy.

- How the stack is stored when it is a tree, and how the project file changes.

- How the state of a group (hidden, solo, locked) combines with the state of each layer, and how the opacity of a group is applied.

- Whether a group is drawn as one picture before being put on the output, or whether what is done to a group is applied to each of its layers. An effect on a group does not give the same picture in the two cases.

**Effects.** Something that changes a picture, stacked on a layer, a group, possibly a source.

- Where in the rendering each stack is applied, and in what order (source, layer, group).

- How an effect declares its settings, so that the panel, the project file, OSC and MCP see them like any other.

**Shaders.**

- A fragment shader that changes a picture is the natural way to make an effect; one that makes a picture from nothing behaves like a source. Whether both are offered, and under which type of source, belongs to the slice on types of sources and shaders.

- A vertex shader moves geometry, which is today the job of the shape. If it is offered, it is neither an effect nor a source: it needs a place in the model and a word in the vocabulary.

**Settings of the software and of extensions.**

- Which settings travel with the project and which stay on the computer: the ports of OSC, MIDI and MCP are the case to decide. An installation moved to another computer may want to keep them.

- How an extension declares its settings so that they appear with the others.
