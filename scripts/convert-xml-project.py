#!/usr/bin/env python3
"""Convert a MapMap project from the old XML format to the JSON format of 1.0.

MapMap 0.4 to 0.6 saved projects as XML; MapMap 1.0 saves them as JSON and
cannot open the XML ones. This script reads an XML project and writes a JSON
project next to it. The original file is never changed. It needs Python 3.6 or
later, and nothing else.

Examples::

    python3 scripts/convert-xml-project.py show.mmp
    python3 scripts/convert-xml-project.py *.mmp
    python3 scripts/convert-xml-project.py show.mmp --output new-show.mmp
    python3 scripts/convert-xml-project.py show.mmp --output-size 1920x1080

Without ``--output``, ``show.mmp`` becomes ``show-converted.mmp``.

Before MapMap 1.0.0-beta.1, the point (0, 0) was near the middle of the output
screen instead of its top left corner, at a place that depends on the size of
the screen. Projects did not save that size: give the one of the screen or
projector the project was made for with ``--output-size``, and the script moves
the output shapes so that they stay at the same place on that screen. They may
be a few pixels off, since the old position also depended on the system. The
size is also saved as the output resolution of the project, so that MapMap
shows the output frame at that size.

With ``--output-size``, a JSON project made with a development version of 1.0,
before that was fixed, is accepted too: its shapes are moved. Do it only once
per project. A project that already has an output resolution is refused: its
shapes are already at the right place.

The exit status is 0 when every project was converted, 1 when at least one
could not be, and 2 when the command line is wrong.
"""

import argparse
import json
import math
import os
import sys
import xml.etree.ElementTree as ElementTree

# File format version, independent of the application release.
CONVERTED_PROJECT_VERSION = "1.0.0-beta.2"

# What is added to the name of a project to name the converted one.
CONVERTED_SUFFIX = "-converted"

# Versions of MapMap before 0.4 used another XML format, and all wrote this
# number in their projects.
UNSUPPORTED_PROJECT_VERSION = "0.1"

# Classes that were renamed in MapMap 1.0.
RENAMED_CLASSES = {
    "ColorMapping": "ColorLayer",
    "TextureMapping": "TextureLayer",
}

# Properties that were renamed in MapMap 1.0.
RENAMED_PROPERTIES = {
    "nColumns": "n_columns",
    "nRows": "n_rows",
}

SOURCE_CLASSES = ("Color", "Image", "Video")
TEXTURE_CLASSES = ("Image", "Video")
LAYER_CLASSES = ("ColorLayer", "TextureLayer")
SHAPE_CLASSES = ("Triangle", "Mesh", "Ellipse")

# A quad is saved as a mesh of 2 by 2 vertices.
QUAD_VERTEX_COUNT = 4
QUAD_AS_MESH_SIZE = 2


# Before MapMap 1.0.0-beta.1, the output was as large as a new Qt widget, in the
# middle of the screen, instead of as large as the screen.
OLD_OUTPUT_WIDTH = 100
OLD_OUTPUT_HEIGHT = 30

# Key of the output resolution in a project (see MappingManager in MapMap).
OUTPUT_RESOLUTION = "outputResolution"


class ConversionError(Exception):
    """The file cannot be converted; the message says why."""


def read_properties(element, excluded, integers=(), numbers=(), flags=()):
    """Read attributes and child properties in their beta.2 JSON types."""
    properties = {RENAMED_PROPERTIES.get(name, name): value
                  for name, value in element.attrib.items()
                  if name not in excluded}
    for child in element:
        if child.tag not in excluded:
            properties[RENAMED_PROPERTIES.get(child.tag, child.tag)] = (child.text or "").strip()
    for name in properties:
        value = properties[name]
        try:
            if name in integers:
                properties[name] = int(value)
            elif name in numbers:
                number = float(value)
                if not math.isfinite(number):
                    raise ValueError()
                properties[name] = int(number) if number.is_integer() else number
            elif name in flags:
                properties[name] = {"true": True, "1": True, "false": False, "0": False}[value.lower()]
        except (ValueError, KeyError):
            raise ConversionError("<{}> has an invalid {} value {!r}".format(element.tag, name, value))
    return properties


def read_integer(element, name):
    """Return an integer attribute of an XML element, or raise ConversionError."""
    value = element.get(name)
    try:
        return int(value)
    except (TypeError, ValueError):
        raise ConversionError("<{}> has no valid {!r} attribute".format(element.tag, name))


def read_number(element, name):
    """Return a numeric attribute of an XML element, or raise ConversionError.

    A whole number is returned as an int, so it is written without a decimal
    point.
    """
    value = element.get(name)
    try:
        number = float(value)
    except (TypeError, ValueError):
        raise ConversionError("<{}> has no valid {!r} attribute".format(element.tag, name))
    return int(number) if number.is_integer() else number


def convert_shape(element):
    """Convert the XML element of a shape (<source> or <destination>)."""
    class_name = element.get("className")
    shape = read_properties(element, excluded=("vertices",), integers=("n_columns", "n_rows"),
                            flags=("locked",))
    shape["vertices"] = [[read_number(vertex, "x"), read_number(vertex, "y")]
                         for vertex in element.findall("vertices/vertex")]

    if class_name == "Quad":
        # MapMap 1.0 has no quad: it is a mesh. The vertices of a quad go
        # around it; those of a mesh go row by row.
        if len(shape["vertices"]) != QUAD_VERTEX_COUNT:
            raise ConversionError("a quad has {} vertices instead of {}".format(
                len(shape["vertices"]), QUAD_VERTEX_COUNT))
        first, second, third, fourth = shape["vertices"]
        shape["className"] = "Mesh"
        shape["n_columns"] = QUAD_AS_MESH_SIZE
        shape["n_rows"] = QUAD_AS_MESH_SIZE
        shape["vertices"] = [first, second, fourth, third]
    elif class_name not in SHAPE_CLASSES:
        raise ConversionError("unknown shape {!r}".format(class_name))
    return shape


def convert_source(element):
    """Convert a <paint> element to a source."""
    class_name = element.get("className")
    if class_name not in SOURCE_CLASSES:
        raise ConversionError("unknown source {!r}".format(class_name))

    source = read_properties(element, excluded=("id", "x", "y"), numbers=("opacity", "rate", "volume"),
                             flags=("locked",))
    source["id"] = read_integer(element, "id")
    if class_name in TEXTURE_CLASSES:
        for name in ("x", "y"):
            try:
                number = float(element.findtext(name, "0"))
            except ValueError:
                raise ConversionError("<{}> is not a number".format(name))
            source[name] = int(number) if number.is_integer() else number
    return source


def convert_layer(element):
    """Convert a <mapping> element to a layer."""
    old_class_name = element.get("className")
    class_name = RENAMED_CLASSES.get(old_class_name, old_class_name)
    if class_name not in LAYER_CLASSES:
        raise ConversionError("unknown layer {!r}".format(old_class_name))

    layer = read_properties(element, excluded=("id", "paintId", "source", "destination"),
                            integers=("depth",), numbers=("opacity",), flags=("locked", "solo", "visible"))
    layer["className"] = class_name
    layer["id"] = read_integer(element, "id")
    layer["source_id"] = read_integer(element, "paintId")

    shape_tags = ["destination"]
    if class_name == "TextureLayer":
        shape_tags.append("source")
    for tag in shape_tags:
        shape = element.find(tag)
        if shape is None:
            raise ConversionError("the layer has no <{}> shape".format(tag))
        layer[tag] = convert_shape(shape)
    return layer


def convert_elements(elements, convert, kind, warn):
    """Convert XML elements one by one, skipping those that cannot be.

    Args:
        elements: the XML elements.
        convert: function that converts one of them.
        kind: what they are ("source" or "layer"), for the warnings.
        warn: function called with the message of each warning.
    """
    converted = []
    for element in elements:
        try:
            converted.append(convert(element))
        except ConversionError as error:
            name = element.get("name") or element.get("id") or "without a name"
            warn("{} {!r} is left out: {}".format(kind, name, error))
    return converted


def old_origin(output_size):
    """Return where MapMap used to show the point (0, 0) on the output screen.

    Args:
        output_size: width and height of the screen.
    """
    width, height = output_size
    return (width - OLD_OUTPUT_WIDTH) // 2, (height - OLD_OUTPUT_HEIGHT) // 2


def move_output_shapes(project, output_size):
    """Move the output shapes of a JSON project made before 1.0.0-beta.1.

    They are displayed at the same place as before on a screen of this size.
    The shapes that cut the image (the input shapes) do not change.

    Raises:
        ConversionError: if a shape of the project cannot be read.
    """
    x_offset, y_offset = old_origin(output_size)
    try:
        for layer in project["layers"]:
            shape = layer["destination"]
            shape["vertices"] = [[x + x_offset, y + y_offset] for x, y in shape["vertices"]]
    except (KeyError, TypeError, ValueError):
        raise ConversionError("an output shape cannot be read")


def is_json(contents):
    """Return whether the contents of a project (bytes) look like JSON."""
    return contents.lstrip(b"\xef\xbb\xbf \t\r\n").startswith(b"{")


def read_project(contents, output_size, warn):
    """Read the contents of an old project; return the new one as a dict.

    Args:
        contents: contents of the file, as bytes.
        output_size: width and height of the screen the project was made for,
            or None to leave the output shapes where they are.
        warn: function called with the message of each warning.

    Raises:
        ConversionError: if this is not a project that can be converted.
    """
    if output_size and is_json(contents):
        try:
            project = json.loads(contents.decode("utf-8-sig"))
        except ValueError as error:
            raise ConversionError("this is not a valid JSON file ({})".format(error))
        if isinstance(project, dict) and OUTPUT_RESOLUTION in project:
            raise ConversionError("this project already has an output resolution: its output "
                                  "shapes are already at the right place")
    else:
        project = convert_project(contents, warn)
        if not output_size and project["layers"]:
            warn("the output shapes are not moved, and will not be at the same place on the "
                 "output screen: give its size with --output-size to move them")
    if output_size:
        move_output_shapes(project, output_size)
        width, height = output_size
        project[OUTPUT_RESOLUTION] = {"width": width, "height": height}
    return project


def convert_project(xml_text, warn):
    """Convert the text of an XML project; return the JSON project as a dict.

    Args:
        xml_text: contents of the XML file, as bytes.
        warn: function called with a message for each source or layer that
            cannot be converted.

    Raises:
        ConversionError: if this is not a project that can be converted.
    """
    if is_json(xml_text):
        raise ConversionError("this project is already in the JSON format (use --output-size "
                              "to move its output shapes)")
    try:
        project = ElementTree.fromstring(xml_text)
    except (ElementTree.ParseError, ValueError, LookupError) as error:
        raise ConversionError("this is not a valid XML file ({})".format(error))
    if project.tag != "project":
        raise ConversionError("this does not look like a MapMap project")
    if project.get("version") == UNSUPPORTED_PROJECT_VERSION:
        raise ConversionError("projects of MapMap 0.3 and earlier are not supported")

    sources = convert_elements(project.findall("paints/paint"), convert_source, "source", warn)
    layers = convert_elements(project.findall("mappings/mapping"), convert_layer, "layer", warn)

    source_ids = {source["id"] for source in sources}
    usable_layers = []
    for layer in layers:
        if layer["source_id"] in source_ids:
            usable_layers.append(layer)
        else:
            warn("layer {!r} is left out: its source {} does not exist".format(
                layer.get("name", ""), layer["source_id"]))

    return {
        "version": CONVERTED_PROJECT_VERSION,
        "sources": sources,
        "layers": usable_layers,
    }


def default_output_path(input_path):
    """Return where the converted project goes: next to the original one."""
    root, extension = os.path.splitext(input_path)
    return root + CONVERTED_SUFFIX + extension


def convert_file(input_path, output_path, output_size, overwrite):
    """Convert one file, and report what happens. Return True if it worked."""
    def warn(message):
        print("{}: warning: {}".format(input_path, message), file=sys.stderr)

    try:
        if os.path.realpath(input_path) == os.path.realpath(output_path):
            raise ConversionError("the converted project cannot replace the original one")
        with open(input_path, "rb") as input_file:
            project = read_project(input_file.read(), output_size, warn)
        text = json.dumps(project, ensure_ascii=False, indent=4, sort_keys=True) + "\n"
        with open(output_path, "w" if overwrite else "x", encoding="utf-8", newline="\n") as output_file:
            output_file.write(text)
    except FileExistsError:
        print("{}: error: {} already exists (use --overwrite to replace it)".format(
            input_path, output_path), file=sys.stderr)
        return False
    except (ConversionError, OSError) as error:
        print("{}: error: {}".format(input_path, error), file=sys.stderr)
        return False

    print("{} -> {} ({} sources, {} layers)".format(
        input_path, output_path, len(project.get("sources", [])), len(project["layers"])))
    return True


def parse_output_size(text):
    """Read a size such as "1920x1080"; return the width and the height."""
    try:
        width, height = (int(number) for number in text.lower().split("x"))
    except ValueError:
        raise argparse.ArgumentTypeError("{!r} is not a size such as 1920x1080".format(text))
    if width < OLD_OUTPUT_WIDTH or height < OLD_OUTPUT_HEIGHT:
        raise argparse.ArgumentTypeError("{!r} is too small for a screen".format(text))
    return width, height


def parse_arguments(arguments):
    """Parse the command line."""
    parser = argparse.ArgumentParser(
        description="Convert MapMap projects from the XML format of MapMap 0.4 to 0.6 "
                    "to the JSON format of MapMap 1.0. The original files are not changed.")
    parser.add_argument("projects", nargs="+", metavar="PROJECT", help="project to convert")
    parser.add_argument("--output", metavar="FILE",
                        help="where to write the converted project (with a single project; "
                             "default: its name followed by '" + CONVERTED_SUFFIX + "')")
    parser.add_argument("--output-size", metavar="WIDTHxHEIGHT", type=parse_output_size,
                        help="size of the output screen the projects were made for, such as "
                             "1920x1080: move the output shapes so that they stay at the same "
                             "place on that screen")
    parser.add_argument("--overwrite", action="store_true",
                        help="replace the converted project if it already exists")
    options = parser.parse_args(arguments)
    if options.output and len(options.projects) > 1:
        parser.error("--output can only be used with a single project")
    return options


def main(arguments=None):
    """Run the program; return its exit status."""
    options = parse_arguments(arguments)
    results = [convert_file(path, options.output or default_output_path(path),
                            options.output_size, options.overwrite)
               for path in options.projects]
    return 0 if all(results) else 1


if __name__ == "__main__":
    sys.exit(main())
