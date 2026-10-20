#!/bin/sh
set -eu
cd "$(dirname "$0")/.."
python3 tools/sync_demo_atlas.py
