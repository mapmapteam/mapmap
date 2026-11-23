#!/usr/bin/env python3
"""Tests of scripts/convert-xml-project.py.

Run them with: python3 tests/test_convert_xml_project.py

tests/fixtures/xml/color-0.6.3.mmp was saved by MapMap 0.6.3 (see issue #490).
textures-0.6.2.mmp was written by hand, after the code of MapMap 0.6.2.
"""

import importlib.util
import json
import os
import shutil
import subprocess
import sys
import tempfile
import unittest

TESTS_DIRECTORY = os.path.dirname(os.path.abspath(__file__))
SCRIPT = os.path.join(TESTS_DIRECTORY, os.pardir, "scripts", "convert-xml-project.py")
FIXTURES = os.path.join(TESTS_DIRECTORY, "fixtures")


def load_script():
    """Import the script, whose name is not a valid module name."""
    sys.dont_write_bytecode = True  # Leave no __pycache__ folder in scripts/.
    spec = importlib.util.spec_from_file_location("convert_xml_project", SCRIPT)
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


CONVERTER = load_script()


def convert(xml_text):
    """Convert XML text; return the project and the list of warnings."""
    warnings = []
    return CONVERTER.convert_project(xml_text, warnings.append), warnings


def convert_fixture(name):
    """Convert a fixture; return the project and the list of warnings."""
    with open(os.path.join(FIXTURES, "xml", name), "rb") as fixture:
        return convert(fixture.read())


class ConvertProjectTest(unittest.TestCase):
    """Conversion of the contents of a project."""

    def test_project_saved_by_mapmap_0_6_3(self):
        project, warnings = convert_fixture("color-0.6.3.mmp")
        self.assertEqual(warnings, [])
        self.assertEqual(project, {
            "version": "1.0.0",
            "sources": [{
                "className": "Color", "id": 1, "name": "#cc49ff", "locked": "false",
                "opacity": "1", "color": "#cc49ff",
            }],
            "layers": [{
                "className": "ColorLayer", "id": 1, "sourceId": 1, "name": "Triangle 1",
                "visible": "true", "locked": "false", "solo": "false", "depth": "1",
                "opacity": "1",
                "destination": {
                    "className": "Triangle", "locked": "false",
                    "vertices": [[265, 223], [795, 223], [530, 74]],
                },
            }],
        })

    def test_image_and_video_sources(self):
        project, warnings = convert_fixture("textures-0.6.2.mmp")
        self.assertEqual(warnings, [])
        image, video = project["sources"]
        self.assertEqual(image, {
            "className": "Image", "id": 1, "name": "tiny.png", "locked": "false",
            "opacity": "0.5", "uri": "/media/été/tiny.png", "rate": "1", "x": 0, "y": 12.5,
        })
        self.assertEqual(video["name"], "clip & loop")
        self.assertEqual((video["volume"], video["rate"]), ("0.25", "-2"))

    def test_texture_layer_keeps_both_shapes(self):
        project, _ = convert_fixture("textures-0.6.2.mmp")
        mesh = project["layers"][0]
        self.assertEqual(mesh["className"], "TextureLayer")
        self.assertEqual((mesh["destination"]["nColumns"], mesh["destination"]["nRows"]), ("3", "2"))
        self.assertEqual(mesh["destination"]["vertices"][4], [50, 55.5])
        self.assertEqual(mesh["source"]["vertices"][5], [2, 2])

    def test_quad_becomes_a_mesh_with_its_vertices_row_by_row(self):
        project, _ = convert_fixture("textures-0.6.2.mmp")
        quad = project["layers"][1]["destination"]
        self.assertEqual((quad["className"], quad["nColumns"], quad["nRows"]), ("Mesh", "2", "2"))
        self.assertEqual(quad["vertices"], [[0, 0], [100, 0], [0, 80], [100, 80]])

    def test_layer_order_is_kept(self):
        project, _ = convert_fixture("textures-0.6.2.mmp")
        self.assertEqual([layer["id"] for layer in project["layers"]], [1, 2])

    def test_unknown_source_and_its_layer_are_left_out_with_warnings(self):
        project, warnings = convert(
            b'<project version="0.6.2"><paints><paint className="Hologram" id="1" name="h"/></paints>'
            b'<mappings><mapping className="ColorMapping" id="1" paintId="1" name="m">'
            b'<destination className="Triangle"><vertices/></destination></mapping></mappings></project>')
        self.assertEqual((project["sources"], project["layers"]), ([], []))
        self.assertEqual(len(warnings), 2)

    def test_malformed_vertex_leaves_the_layer_out(self):
        project, warnings = convert(
            b'<project version="0.6.2"><paints><paint className="Color" id="1"/></paints>'
            b'<mappings><mapping className="ColorMapping" id="1" paintId="1">'
            b'<destination className="Triangle"><vertices><vertex x="a" y="1"/></vertices>'
            b'</destination></mapping></mappings></project>')
        self.assertEqual(project["layers"], [])
        self.assertEqual(len(warnings), 1)

    def test_empty_project(self):
        project, warnings = convert(b'<project version="0.6.2"><paints/><mappings/></project>')
        self.assertEqual((project["sources"], project["layers"], warnings), ([], [], []))

    def test_files_that_cannot_be_converted(self):
        cases = {
            "json": b'{"version": "1.0.0"}',
            "not xml": b"hello",
            "empty": b"",
            "truncated": b"<project><paints>",
            "other xml": b"<html/>",
            "too old": b'<project version="0.1"/>',
        }
        for name, text in cases.items():
            with self.subTest(name), self.assertRaises(CONVERTER.ConversionError):
                convert(text)


class CommandLineTest(unittest.TestCase):
    """The program, as a user runs it."""

    def setUp(self):
        self.directory = tempfile.mkdtemp()
        self.addCleanup(shutil.rmtree, self.directory)
        self.project = os.path.join(self.directory, "show.mmp")
        self.converted = os.path.join(self.directory, "show-converted.mmp")
        shutil.copy(os.path.join(FIXTURES, "xml", "textures-0.6.2.mmp"), self.project)

    def run_script(self, *arguments):
        """Run the script; return its exit status."""
        return subprocess.run([sys.executable, SCRIPT] + list(arguments),
                              stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL).returncode

    def test_converted_project_is_written_next_to_the_original(self):
        with open(self.project, "rb") as original:
            before = original.read()
        self.assertEqual(self.run_script(self.project), 0)
        with open(self.converted, encoding="utf-8") as converted:
            self.assertEqual(len(json.load(converted)["layers"]), 2)
        with open(self.project, "rb") as original:
            self.assertEqual(original.read(), before)

    def test_existing_file_is_only_replaced_when_asked(self):
        with open(self.converted, "w") as existing:
            existing.write("keep me")
        self.assertEqual(self.run_script(self.project), 1)
        with open(self.converted) as existing:
            self.assertEqual(existing.read(), "keep me")
        self.assertEqual(self.run_script(self.project, "--overwrite"), 0)

    def test_original_is_never_replaced(self):
        self.assertEqual(self.run_script(self.project, "--output", self.project, "--overwrite"), 1)

    def test_output_size_moves_the_output_shapes_only(self):
        with open(self.project, "rb") as original:
            before, _ = convert(original.read())
        self.assertEqual(self.run_script(self.project, "--output-size", "1920x1080"), 0)
        with open(self.converted, encoding="utf-8") as converted:
            after = json.load(converted)
        for old_layer, new_layer in zip(before["layers"], after["layers"]):
            self.assertEqual(new_layer["destination"]["vertices"],
                             [[x + 910, y + 525]
                              for x, y in old_layer["destination"]["vertices"]])
            self.assertEqual(new_layer.get("source"), old_layer.get("source"))
        self.assertEqual(after["sources"], before["sources"])

    def test_json_project_is_only_accepted_with_an_output_size(self):
        shutil.copy(os.path.join(FIXTURES, "quad-image.mmp"), self.project)
        with open(self.project, encoding="utf-8") as original:
            before = json.load(original)
        self.assertEqual(self.run_script(self.project), 1)
        self.assertEqual(self.run_script(self.project, "--output-size", "1280x720"), 0)
        with open(self.converted, encoding="utf-8") as converted:
            after = json.load(converted)
        x, y = before["layers"][0]["destination"]["vertices"][0]
        self.assertEqual(after["layers"][0]["destination"]["vertices"][0], [x + 590, y + 345])
        self.assertEqual(after["layers"][0]["source"], before["layers"][0]["source"])
        self.assertEqual(after["sources"], before["sources"])

    def test_damaged_json_project_is_refused(self):
        for text in ("{", "{}", '{"layers": 3}', '{"layers": [{"destination": {"vertices": [1]}}]}'):
            with open(self.project, "w") as project:
                project.write(text)
            self.assertEqual(self.run_script(self.project, "--output-size", "1280x720"), 1, text)
            self.assertFalse(os.path.exists(self.converted))

    def test_wrong_output_size_is_refused(self):
        for size in ("1920", "1920x", "wide x high", "1920x1080x3", "0x0", "-1920x-1080"):
            self.assertEqual(self.run_script(self.project, "--output-size=" + size), 2, size)

    def test_warning_when_the_output_shapes_are_not_moved(self):
        with open(self.project, "rb") as original:
            contents = original.read()
        warnings = []
        CONVERTER.read_project(contents, None, warnings.append)
        self.assertIn("--output-size", " ".join(warnings))
        warnings = []
        CONVERTER.read_project(contents, (1920, 1080), warnings.append)
        self.assertEqual(warnings, [])

    def test_one_bad_file_does_not_stop_the_others(self):
        missing = os.path.join(self.directory, "missing.mmp")
        self.assertEqual(self.run_script(missing, self.project), 1)
        self.assertTrue(os.path.exists(self.converted))

    def test_output_with_several_projects_is_refused(self):
        self.assertEqual(self.run_script(self.project, self.project, "--output", "x.mmp"), 2)


if __name__ == "__main__":
    unittest.main()
