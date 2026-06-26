#!/usr/bin/env python3
"""Tiny, dependency-free OSC client for driving MapMap over UDP.

MapMap listens for OSC on UDP port 12345 by default (see the ``OSC`` file at
the root of the repository for the full address scheme). This script encodes a
single OSC message and sends it, with no third-party dependencies.

Argument types are inferred automatically:

* a token that parses as an integer is sent as an OSC int   (``i``);
* a token that parses as a float   is sent as an OSC float (``f``);
* anything else is sent as an OSC string (``s``).

Prefix a token with ``i:``, ``f:`` or ``s:`` to force its type, e.g. ``s:12``
sends the string "12" rather than the integer 12 (useful to select an element
by a numeric name).

Examples::

    python3 scripts/mapmap-osc.py /mapmap/play
    python3 scripts/mapmap-osc.py /mapmap/layer/opacity 0 0.5
    python3 scripts/mapmap-osc.py /mapmap/layer/move/xy 0 640 360
    python3 scripts/mapmap-osc.py /mapmap/source/color 3 '#ff0000'
    python3 scripts/mapmap-osc.py --host 192.168.1.20 --port 9000 /mapmap/pause
"""

import argparse
import socket
import struct
import sys


def _osc_string(value: str) -> bytes:
    """Encode an OSC string: UTF-8 bytes, null-terminated, padded to 4 bytes."""
    data = value.encode("utf-8") + b"\x00"
    if len(data) % 4:
        data += b"\x00" * (4 - len(data) % 4)
    return data


def _encode_argument(token: str):
    """Return (type_tag, packed_bytes) for one command-line argument token."""
    forced = None
    if len(token) > 1 and token[1] == ":" and token[0] in "ifs":
        forced, token = token[0], token[2:]

    if forced == "s":
        return "s", _osc_string(token)
    if forced == "i":
        return "i", struct.pack(">i", int(token))
    if forced == "f":
        return "f", struct.pack(">f", float(token))

    # Auto-detect: int, then float, then fall back to string.
    try:
        return "i", struct.pack(">i", int(token))
    except ValueError:
        pass
    try:
        return "f", struct.pack(">f", float(token))
    except ValueError:
        pass
    return "s", _osc_string(token)


def build_message(address: str, tokens) -> bytes:
    """Build a complete OSC message packet from an address and argument tokens."""
    tags = ","
    payload = b""
    for token in tokens:
        tag, packed = _encode_argument(token)
        tags += tag
        payload += packed
    return _osc_string(address) + _osc_string(tags) + payload


def main(argv=None) -> int:
    parser = argparse.ArgumentParser(
        description="Send a single OSC message to MapMap.",
        epilog=__doc__,
        formatter_class=argparse.RawDescriptionHelpFormatter,
    )
    parser.add_argument("address", help="OSC address, e.g. /mapmap/layer/opacity")
    parser.add_argument("args", nargs="*", help="OSC arguments (types auto-detected)")
    parser.add_argument("--host", default="127.0.0.1", help="target host (default: 127.0.0.1)")
    parser.add_argument("--port", type=int, default=12345, help="target UDP port (default: 12345)")
    parser.add_argument("-v", "--verbose", action="store_true", help="print what is sent")
    options = parser.parse_args(argv)

    if not options.address.startswith("/"):
        parser.error("OSC address must start with '/' (e.g. /mapmap/play)")

    packet = build_message(options.address, options.args)

    if options.verbose:
        print(f"-> {options.host}:{options.port}  {options.address} {' '.join(options.args)}")

    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    try:
        sock.sendto(packet, (options.host, options.port))
    finally:
        sock.close()
    return 0


if __name__ == "__main__":
    sys.exit(main())
