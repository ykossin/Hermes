#!/bin/sh
set -eu
root=$(cd "$(dirname "$0")/.." && pwd)
tray=$root/third-party/tray
if [ -f "$tray/CMakeLists.txt" ]; then
  exit 0
fi
if [ -d "$root/../_deprecated-Hermes/third-party/tray" ]; then
  cp -a "$root/../_deprecated-Hermes/third-party/tray" "$tray"
  exit 0
fi
git -C "$root" submodule update --init third-party/tray
