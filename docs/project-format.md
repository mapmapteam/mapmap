# JSON project files

## Existing files

A MapMap project is a JSON object. The writer emits `sources` and `layers` arrays, including when they are empty. A layer refers to its source by a numeric ID. Array order matters: the first layer is at the top of the stack. Mesh vertices are in row-major order. An older project may omit `outputResolution`.

The current development reader accepts earlier setting names such as `sourceId` and `nColumns`. When both spellings occur, the current spelling takes precedence. IDs and coordinates are already JSON numbers. Beta.1 stores many settings as strings. Beta.2 reads those projects directly. Saving a project writes the beta.2 format, with native numbers and booleans. Use Save As to keep a copy that beta.1 can read.

## Beta.2 file format

### Media paths

Interpret a relative image or video path from the directory containing the project. Resolve it before opening the media. Prefer that file even if the working directory contains another file with the same name. If a legacy path exists only in the working directory, use it as a fallback and write an unambiguous path on the next save.

Use a relative path for a newly added file inside the project directory. Keep an external absolute path usable. Preserve the target of an existing relative reference, including one containing `..`, when using Save As in another directory. If the user cancels locating a missing file, keep the reference so that putting the media back restores the project. Camera device IDs are opaque strings.

Test opening a project from another working directory, moving the project and its media together, and saving a copy to another directory. Check the resulting media target as well as the path written in JSON.

### Structural validation

Require an object containing `sources` and `layers` arrays. Treat an empty array as valid. Check this envelope before replacing the open project. A failure should name the field and expected type and should be reported to callers, including remote callers.

Within valid arrays, keep readable entries and report skipped entries with their array index or ID. Keep accepting projects without an output resolution and projects using earlier setting names.

Keep the keys a later version added, at the top of the file as inside a source or a layer: write them back as they are, so that opening and saving a newer project with this version does not lose them. A project saved by MapMap 1.0.0-beta.2 or earlier still loses the keys it does not know at the top of the file, such as `links`.

### Native scalar types

Write a numeric setting such as `opacity` as a number and a boolean setting such as `visible` as a boolean. Use the setting declaration to choose the type. A text setting whose value is `"001"` or `"false"` is still text. Read both representations of legacy numeric and boolean settings. The XML converter writes the same scalar types when importing a MapMap 0.4 to 0.6 project.

Changing these types causes a one-time diff. For example, 36 lines in the existing `alpha-mixed.mmp` fixture contain numeric or boolean settings stored as strings. Keep the legacy fixture as input and check the types and values of the saved project separately. Beta.1 expects strings for these settings and can misread a beta.2 project.

### Links

A project may have a `links` array: what its triggers, such as the keys of the computer keyboard, do to its settings. It is written only when the project has at least one link, so a project without links keeps the same bytes. Each link has a `trigger`, a `target` and an `action`:

```json
"links": [
    {
        "action": "toggle",
        "target": { "element": "layer", "id": 7, "setting": "visible" },
        "trigger": { "key": "M", "kind": "key" }
    }
]
```

- `trigger.kind` is `key` for a key of the computer keyboard. `trigger.key` is the key and its modifiers in the portable text of `QKeySequence`, such as `M` or `Shift+M`: the same in every language of the system.
- `target.element` is `source` or `layer`, `target.id` the id of the element, `target.setting` the path of the setting, as in OSC.
- `action` is `toggle`, `on` or `off`. A link without an action toggles.

The reader skips, with a warning naming its target, a link to an element or a setting the project does not have, and a link to a setting that cannot be linked (one without a position between 0 and 1, such as a point or a text). The writer leaves out a link whose element was deleted.

### Convert a beta.1 project with Python

Run `python3 scripts/convert-beta1-project.py show.mmp` to create `show-beta.2.mmp` in the same directory. The script accepts several project files in one invocation. It preserves the original, the order of sources, layers and vertices, and unknown properties. It leaves media references in place and needs no access to the media, cameras or Qt. Commit the converted file separately from creative edits so the type changes are easy to review.

## Stability in Git

- Keep serialized names stable across internal renames. Retain read aliases for keys already in released projects.
- Give the file format a version independent of the application build. Change it when the file contract changes. An application-only release should produce identical bytes for the same project.
- Keep the current four-space indentation, object key order and layout of vertices. Write UTF-8 with LF line endings and a final newline on every platform. Preserve IDs and the order of sources, layers and vertices.
- Give new optional fields a default that reproduces the older behavior. Avoid writing a new field into every existing object merely because a release introduces it.
- Test the actual file bytes after opening and saving a current-format project. Changing one layer name should replace one line. Moving one vertex should change only its coordinates. Run the same reference-file test on every supported platform.

Saving a beta.1 project migrates it to beta.2. Use separate tests for that migration and for edits to a project already in the current format.
