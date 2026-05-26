---
name: mapmap-mcp
description: Control MapMap via its embedded MCP server over local HTTP. Use this skill whenever the user asks to create, modify, inspect, or control sources, layers, playback, or project state in the running MapMap instance.
trigger: When the user wants to interact with the running MapMap application - creating sources/layers, controlling playback, querying state, modifying properties, loading/saving projects.
---

# MapMap MCP Control

MapMap exposes a JSON-RPC 2.0 MCP server at `http://localhost:49452/mcp` (POST only).

## How to call

```bash
curl -s -X POST http://localhost:49452/mcp \
  -H 'Content-Type: application/json' \
  -d '{"jsonrpc":"2.0","id":1,"method":"tools/call","params":{"name":"TOOL_NAME","arguments":{...}}}' \
  | python3 -m json.tool
```

Increment the `id` for each call in a session. Always pipe through `python3 -m json.tool` for readable output.

### First call: initialize

Before any tool calls, send an initialize request:

```bash
curl -s -X POST http://localhost:49452/mcp \
  -H 'Content-Type: application/json' \
  -d '{"jsonrpc":"2.0","id":1,"method":"initialize","params":{"protocolVersion":"2024-11-05","capabilities":{},"clientInfo":{"name":"claude","version":"1.0"}}}'
```

## Available tools

### Playback & Project
| Tool | Args | Description |
|------|------|-------------|
| `play` | — | Start playback |
| `pause` | — | Pause playback |
| `rewind` | — | Rewind to start |
| `quit` | — | Quit MapMap |
| `clear_project` | — | Clear all sources and layers |
| `load_project` | `path` (string, required) | Load a .mmp project file |
| `save_project` | `path` (string, required) | Save project to file |
| `set_fps` | `fps` (number, required, > 0) | Set playback frame rate |

### Sources
| Tool | Args | Description |
|------|------|-------------|
| `create_color_source` | `color` (string, required: "#rrggbb" or name) | Create solid-color source |
| `create_media_source` | `uri` (string, required), `is_image` (bool, default false) | Import image or video |
| `delete_source` | `id` (int, required) | Delete source and its layers |

### Layers
| Tool | Args | Description |
|------|------|-------------|
| `create_layer` | `source_id` (int, required), `shape` (string, required: "triangle", "quad", or "ellipse") | Create a layer with a shape for a source |
| `delete_layer` | `id` (int, required) | Delete a layer |
| `duplicate_layer` | `id` (int, required) | Duplicate a layer |
| `move_layer` | `id` (int, required), `index` (int, required, 0=bottom) | Move layer in stack |
| `set_layer_visible` | `id` (int), `value` (bool) | Show/hide layer |
| `set_layer_solo` | `id` (int), `value` (bool) | Solo a layer |
| `set_layer_locked` | `id` (int), `value` (bool) | Lock/unlock layer |

### Geometry
| Tool | Args | Description |
|------|------|-------------|
| `set_vertices` | `id` (int, layer id), `vertices` (array of `{x, y}`) | Set the output vertices of a layer's shape |

Vertex order is **clockwise from top-left**: top-left, top-right, bottom-right, bottom-left.
- Quad: 4 vertices
- Triangle: 3 vertices (bottom-left, bottom-right, top-center)
- Ellipse: 5 vertices (left, top, right, bottom, rotation-handle)

### Properties
| Tool | Args | Description |
|------|------|-------------|
| `set_property` | `kind` ("source"/"layer"), `id` (int), `property` (string), `value` (any) | Set arbitrary property |

Common properties:
- **Source (color):** `color` ("#rrggbb"), `name`, `opacity` (0.0-1.0), `locked`
- **Layer:** `name`, `opacity`, `visible`, `solo`, `locked`, `depth`

### Introspection
| Tool | Args | Description |
|------|------|-------------|
| `get_state` | — | Get playing, fps, counts, current selection |
| `list_sources` | — | List all sources |
| `list_layers` | — | List all layers |
| `get_source` | `id` (int) | Get all properties of a source |
| `get_layer` | `id` (int) | Get all properties of a layer |

## Typical workflow

1. Create a source (`create_color_source` or `create_media_source`)
2. Create a layer for it (`create_layer` with `source_id` from step 1)
3. Position the layer (`set_vertices` with the desired corner positions)
4. Adjust properties as needed (`set_property`, `set_layer_visible`, etc.)
5. Control playback (`play`, `pause`, `rewind`)

## Response format

Success returns `{"isError": false, "content": [...]}` with optional `structuredContent`.
Errors return `{"isError": true, "content": [{"type":"text","text":"error message"}]}`.
