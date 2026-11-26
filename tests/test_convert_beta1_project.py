"""Test the beta.1 to beta.2 project converter with Python's standard library."""

import json
from pathlib import Path
import runpy
import subprocess
import sys
import tempfile
import unittest

TESTS = Path(__file__).resolve().parent
SCRIPT = TESTS.parent / "scripts" / "convert-beta1-project.py"
CONVERT = runpy.run_path(str(SCRIPT))["convert"]
FIXTURES = TESTS / "fixtures"


class ConvertBeta1ProjectTest(unittest.TestCase):
    def test_cli_writes_reference_bytes_and_preserves_existing_files(self):
        with tempfile.TemporaryDirectory() as directory:
            project = Path(directory) / "été.mmp"
            original = (FIXTURES / "beta.1" / "triangle-color.mmp").read_bytes()
            project.write_bytes(original)
            command = [sys.executable, str(SCRIPT), str(project)]
            subprocess.run(command, check=True, capture_output=True)
            output = project.with_name("été-beta.2.mmp")
            self.assertEqual(output.read_bytes(), (FIXTURES / "beta.2" / "triangle-color.mmp").read_bytes())
            output.write_bytes(b"existing")
            self.assertNotEqual(subprocess.run(command, capture_output=True).returncode, 0)
            self.assertEqual(output.read_bytes(), b"existing")
            self.assertEqual(project.read_bytes(), original)

    def test_preserves_order_paths_text_and_unknown_fields(self):
        source = {"className": "Image", "uri": "../media/été.png", "name": "001", "rate": "-0.5"}
        syphon = {"className": "Syphon", "serverUUID": "001", "serverName": "false",
                  "appName": "004", "respectAlpha": "true", "respect_alpha": False}
        mesh = {"className": "Mesh", "nColumns": "3", "n_columns": "2", "nRows": "2",
                "vertices": [[0, 3], [2, 1], [9, 0], [1, 2]]}
        unknown = {"className": "Other", "opacity": "0.25", "extra": {"locked": "false"}}
        layer = {"className": "TextureLayer", "sourceId": 1, "source_id": 2,
                 "destination": dict(mesh), "source": dict(mesh)}
        project = {"version": "1.0.0-beta.1", "sources": [source, unknown, syphon],
                   "layers": [layer], "metadata": {"name": "false"}}
        converted = CONVERT(project)
        self.assertEqual(converted["sources"], [
            {"className": "Image", "uri": "../media/été.png", "name": "001", "rate": -0.5},
            {"className": "Other", "opacity": "0.25", "extra": {"locked": "false"}},
            {"className": "Syphon", "server_uuid": "001", "server_name": "false",
             "app_name": "004", "respect_alpha": False}])
        self.assertEqual(converted["metadata"], {"name": "false"})
        for key in ("destination", "source"):
            self.assertEqual(layer[key], {"className": "Mesh", "n_columns": 2, "n_rows": 2,
                                         "vertices": mesh["vertices"]})
        self.assertNotIn("sourceId", layer)
        self.assertEqual(layer["source_id"], 2)

    def test_rejects_wrong_version_damaged_structure_and_invalid_values(self):
        for project in [
                {"version": "1.0.0-beta.2", "sources": [], "layers": []},
                {"version": "1.0.0-beta.1", "sources": [], "layers": {}},
                {"version": "1.0.0-beta.1", "sources": [None], "layers": []},
                {"version": "1.0.0-beta.1", "sources": [{"className": "Color", "opacity": "abc"}],
                 "layers": []}]:
            with self.subTest(project=project), tempfile.TemporaryDirectory() as directory:
                path = Path(directory) / "show.mmp"
                path.write_text(json.dumps(project))
                result = subprocess.run([sys.executable, str(SCRIPT), str(path)], capture_output=True)
                self.assertNotEqual(result.returncode, 0)
                self.assertFalse(path.with_name("show-beta.2.mmp").exists())


if __name__ == "__main__":
    unittest.main()
