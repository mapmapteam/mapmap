#!/usr/bin/env python3
"""Convert MapMap 1.0.0-beta.1 JSON projects to the beta.2 file format.

Usage: python3 scripts/convert-beta1-project.py show.mmp [another.mmp ...]
Writes show-beta.2.mmp next to show.mmp, so relative media references keep
the same base directory. Requires only the Python 3 standard library.
"""

import argparse
import json
import math
import sys
from pathlib import Path

OLD_VERSION = "1.0.0-beta.1"
NEW_VERSION = "1.0.0-beta.2"


def convert_value(value, kind):
    """Read one setting using its declared type."""
    if kind is bool:
        if isinstance(value, str):
            return {"true": True, "1": True, "false": False, "0": False}[value.lower()]
        if isinstance(value, (bool, int, float)) and math.isfinite(value):
            return bool(value)
    elif isinstance(value, (int, float, str)) and not isinstance(value, bool):
        if kind is int:
            number = int(value)
            if not isinstance(value, float) or number == value:
                return number
        else:
            number = float(value)
            if math.isfinite(number):
                return int(number) if number.is_integer() else number
    raise ValueError("invalid setting value")


def migrate_fields(obj, path, fields):
    """Convert declared fields and retain all other data verbatim as JSON values."""
    for key, kind in fields.items():
        if key in obj:
            try:
                obj[key] = convert_value(obj[key], kind)
            except (KeyError, ValueError, TypeError, OverflowError):
                raise ValueError("{}.{}: expected {}, got {!r}".format(path, key, kind.__name__, obj[key]))


def rename(obj, former, current):
    """Prefer the current spelling if a project contains both."""
    if former in obj:
        obj.setdefault(current, obj.pop(former))


def convert(project):
    """Migrate only the built-in properties of beta.1 sources, layers and shapes."""
    if not isinstance(project, dict) or project.get("version") != OLD_VERSION:
        raise ValueError("expected a MapMap {} JSON project".format(OLD_VERSION))
    for key in ("sources", "layers"):
        if not isinstance(project.get(key), list):
            raise ValueError("{}: expected an array".format(key))
        for i, obj in enumerate(project[key]):
            if not isinstance(obj, dict):
                raise ValueError("{}[{}]: expected an object".format(key, i))

    for index, source in enumerate(project["sources"]):
        path = "sources[{}]".format(index)
        cls = source.get("className")
        if cls not in ("Color", "Image", "Video", "Camera", "Syphon"):
            continue
        migrate_fields(source, path, {"locked": bool, "opacity": float})
        if cls in ("Image", "Video", "Camera"):
            migrate_fields(source, path, {"rate": float})
        if cls in ("Video", "Camera"):
            migrate_fields(source, path, {"volume": float})
        if cls == "Syphon":
            for former, current in (("serverUUID", "server_uuid"), ("serverName", "server_name"),
                                    ("appName", "app_name"), ("respectAlpha", "respect_alpha")):
                rename(source, former, current)
            migrate_fields(source, path, {"respect_alpha": bool})

    for index, layer in enumerate(project["layers"]):
        if layer.get("className") not in ("ColorLayer", "TextureLayer"):
            continue
        path = "layers[{}]".format(index)
        rename(layer, "sourceId", "source_id")
        migrate_fields(layer, path, {"locked": bool, "opacity": float, "solo": bool,
                                     "visible": bool, "depth": int, "source_id": int})
        for key in ("source", "destination"):
            if key not in layer:
                continue
            shape = layer[key]
            if not isinstance(shape, dict):
                raise ValueError("{}.{}: expected an object".format(path, key))
            if shape.get("className") in ("Mesh", "Quad", "Triangle", "Ellipse"):
                migrate_fields(shape, path + "." + key, {"locked": bool})
            if shape.get("className") == "Mesh":
                rename(shape, "nColumns", "n_columns")
                rename(shape, "nRows", "n_rows")
                migrate_fields(shape, path + "." + key, {"n_columns": int, "n_rows": int})

    project["version"] = NEW_VERSION
    return project


def convert_file(path):
    """Write a sibling copy after successful conversion, preserving existing files."""
    output = path.with_name(path.stem + "-beta.2" + path.suffix)
    with path.open(encoding="utf-8-sig") as source:
        project = convert(json.load(source))
    data = json.dumps(project, ensure_ascii=False, indent=4, sort_keys=True, allow_nan=False) + "\n"
    with output.open("xb") as target:
        try:
            target.write(data.encode("utf-8"))
            target.flush()
        except (OSError, UnicodeError):
            target.close()
            output.unlink()
            raise
    return output


def main():
    """Convert each given file and report a failing exit status if any failed."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("projects", nargs="+", type=Path)
    options = parser.parse_args()
    success = True
    for project in options.projects:
        try:
            print("{} -> {}".format(project, convert_file(project)))
        except (ValueError, OSError) as error:
            print("{}: {}".format(project, error), file=sys.stderr)
            success = False
    return 0 if success else 1


if __name__ == "__main__":
    sys.exit(main())
