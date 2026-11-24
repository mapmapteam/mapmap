#!/usr/bin/env python3
"""Measures the image path of MapMap before and after GPU color conversion.

Generates two H.264 test clips (720p and 1080p) and a project that shows each
clip on three shapes, then runs MapMap with --frame-stats in four modes (GPU
or CPU conversion, normal or synchronous measure) and prints a table of the
median of each step, in milliseconds.

Needs ffmpeg (with libx264) only if the clips are not in the work folder yet.
Run it from a session with a screen: MapMap opens and closes its windows.

    python3 scripts/measure-frame-stats.py
"""

import argparse
import json
import os
import re
import shutil
import statistics
import subprocess
import sys
import tempfile
import time

RESOLUTIONS = [(1280, 720), (1920, 1080)]
MODES = [
    ("after", []),
    ("before", ["--force-cpu-conversion"]),
    ("after-sync", ["--frame-stats-sync"]),
    ("before-sync", ["--frame-stats-sync", "--force-cpu-conversion"]),
]
# Seconds of log skipped at start (loading, first frames).
WARMUP_SECONDS = 4

LINE_RE = re.compile(r"Info: (?P<name>[^:(]+?)(?: \((?P<path>[^)]*)\))?: (?P<stages>.*?) - ")
STAGE_RE = re.compile(r"(prepare|upload|draw|interval) ([\d.]+) ms, max ([\d.]+) ms, (\d+)/s")
DROPPED_RE = re.compile(r"dropped (\d+)/s")


def find_mapmap(repo):
    """Returns the MapMap executable built in the repository."""
    for candidate in ["mapmap", "mapmap.app/Contents/MacOS/mapmap"]:
        path = os.path.join(repo, candidate)
        if os.path.isfile(path) and os.access(path, os.X_OK):
            return path
    sys.exit(f"No MapMap executable in {repo}: build it first, or give --mapmap.")


def make_clip(path, width, height, seconds):
    """Generates a moving H.264 test clip, 30 frames per second, YUV420P."""
    if os.path.exists(path):
        return
    if not shutil.which("ffmpeg"):
        sys.exit(f"{path} is missing and ffmpeg is not installed to generate it.")
    print(f"Generating {os.path.basename(path)}...")
    subprocess.run(["ffmpeg", "-y", "-hide_banner", "-loglevel", "error",
                    "-f", "lavfi", "-i", f"testsrc2=s={width}x{height}:r=30:d={seconds}",
                    "-c:v", "libx264", "-pix_fmt", "yuv420p", "-g", "30", path], check=True)


def make_project(path, clip, width, height):
    """Writes a project that shows the clip on three shapes."""
    quads = [[(0, 0), (640, 0), (0, 360), (640, 360)],
             [(640, 0), (1280, 0), (640, 360), (1280, 360)],
             [(320, 360), (960, 360), (320, 720), (960, 720)]]
    layers = []
    for i, quad in enumerate(quads):
        layers.append({
            "className": "TextureLayer", "id": i + 1, "depth": str(i + 1), "name": f"Quad {i + 1}",
            "locked": "false", "opacity": "1", "solo": "false", "visible": "true", "sourceId": 1,
            "destination": {"className": "Mesh", "locked": "false", "nColumns": "2", "nRows": "2",
                            "vertices": [list(v) for v in quad]},
            "source": {"className": "Mesh", "locked": "false", "nColumns": "2", "nRows": "2",
                       "vertices": [[0, 0], [width, 0], [0, height], [width, height]]},
        })
    project = {"version": "1.0.0-beta.2", "layers": layers,
               "sources": [{"className": "Video", "id": 1, "name": "clip", "locked": "false",
                            "opacity": "1", "rate": "1", "volume": "0", "x": 0, "y": 0,
                            "uri": os.path.abspath(clip)}]}
    with open(path, "w") as f:
        json.dump(project, f, indent=2)


def run_mapmap(mapmap, project, options, seconds, log_path):
    """Runs MapMap for a few seconds and returns its log."""
    open(log_path, "w").close()
    command = [mapmap, "-r", "30", "--frame-stats", *options, "-f", project]
    process = subprocess.Popen(command, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
    try:
        process.wait(timeout=seconds)
    except subprocess.TimeoutExpired:
        process.terminate()
        try:
            process.wait(timeout=5)
        except subprocess.TimeoutExpired:
            process.kill()
    time.sleep(0.5)
    with open(log_path, errors="replace") as f:
        return f.read()


def parse_log(log):
    """Returns {name: {"path": str, step: [means in ms], "dropped": [counts]}}."""
    lines = [line for line in log.splitlines() if "Info: " in line and STAGE_RE.search(line)]
    names_per_second = len({LINE_RE.search(l).group("name") for l in lines if LINE_RE.search(l)}) or 1
    lines = lines[WARMUP_SECONDS * names_per_second:]
    result = {}
    for line in lines:
        match = LINE_RE.search(line)
        if not match:
            continue
        entry = result.setdefault(match.group("name"), {"path": match.group("path") or ""})
        for stage, mean, _maximum, _count in STAGE_RE.findall(match.group("stages")):
            entry.setdefault(stage, []).append(float(mean))
        dropped = DROPPED_RE.search(match.group("stages"))
        entry.setdefault("dropped", []).append(int(dropped.group(1)) if dropped else 0)
    return result


def median(values):
    return f"{statistics.median(values):.2f}" if values else "-"


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.ArgumentDefaultsHelpFormatter)
    parser.add_argument("repo", nargs="?",
                        default=os.path.dirname(os.path.dirname(os.path.abspath(__file__))),
                        help="Built MapMap repository")
    parser.add_argument("--mapmap", help="MapMap executable (default: found in the repository)")
    parser.add_argument("--seconds", type=int, default=16, help="Length of each run")
    parser.add_argument("--work-dir", default=os.path.join(tempfile.gettempdir(), "mapmap-frame-stats"),
                        help="Folder of the clips, projects and logs")
    parser.add_argument("--only", choices=["720", "1080"], help="Measure a single resolution")
    args = parser.parse_args()

    mapmap = os.path.abspath(args.mapmap) if args.mapmap else find_mapmap(args.repo)
    # MapMap writes its log to mapmap.log in the system's temporary folder.
    log_path = os.path.join(tempfile.gettempdir(), "mapmap.log")
    os.makedirs(args.work_dir, exist_ok=True)

    rows = []
    for width, height in RESOLUTIONS:
        if args.only and str(height) != args.only:
            continue
        clip = os.path.join(args.work_dir, f"clip-{width}x{height}.mp4")
        project = os.path.join(args.work_dir, f"three-shapes-{width}x{height}-local.mmp")
        make_clip(clip, width, height, args.seconds + 10)
        make_project(project, clip, width, height)
        for mode, options in MODES:
            print(f"{height}p {mode}: running MapMap for {args.seconds} s...")
            log = run_mapmap(mapmap, project, options, args.seconds, log_path)
            with open(os.path.join(args.work_dir, f"log-{height}p-{mode}.txt"), "w") as f:
                f.write(log)
            stats = parse_log(log)
            if not stats:
                print(f"  No frame stats in {log_path}: did MapMap start and open the clip?")
            for name, entry in stats.items():
                rows.append((f"{height}p", mode, name, entry))

    print("\nMedian of the means per second, in ms (dropped: frames per second).\n")
    print("| resolution | mode | source or view | path | prepare | upload | draw | interval | dropped |")
    print("|---|---|---|---|---|---|---|---|---|")
    for resolution, mode, name, entry in rows:
        print(f"| {resolution} | {mode} | {name} | {entry['path']} | {median(entry.get('prepare', []))} | "
              f"{median(entry.get('upload', []))} | {median(entry.get('draw', []))} | "
              f"{median(entry.get('interval', []))} | {median(entry.get('dropped', []))} |")


if __name__ == "__main__":
    main()
