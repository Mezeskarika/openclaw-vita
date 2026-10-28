#!/usr/bin/env python3
"""Packs a directory into a zip the game's own reader can load.

Usage: python3 -I make_assets_zip.py <source dir> <output zip>

The engine's zip reader (OpenClaw/Engine/Resource/ZipFile.cpp) takes each entry's sizes from its
local header. Streaming zip writers such as CMake's `cmake -E tar --format=zip` (libarchive) leave
those at zero and put the sizes in a trailing data descriptor, which makes every file read back
empty. Python's zipfile writes the sizes into the local header when writing from a file.
Entries are named relative to the source dir and sorted, so the output only changes when the
content does.
"""
import os
import sys
import zipfile


def main():
    source_dir, output = sys.argv[1], sys.argv[2]
    paths = []
    for root, dirs, files in os.walk(source_dir):
        dirs.sort()
        for name in sorted(files):
            paths.append(os.path.join(root, name))

    with zipfile.ZipFile(output, "w", zipfile.ZIP_DEFLATED) as archive:
        for path in paths:
            entry = os.path.relpath(path, source_dir).replace(os.sep, "/")
            info = zipfile.ZipInfo(entry, date_time=(2000, 1, 1, 0, 0, 0))
            info.compress_type = zipfile.ZIP_DEFLATED
            info.external_attr = 0o644 << 16
            with open(path, "rb") as source:
                archive.writestr(info, source.read())
    return 0


if __name__ == "__main__":
    sys.exit(main())
