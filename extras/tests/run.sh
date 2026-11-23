#!/usr/bin/env bash
# Builds and runs the desktop tests. Needs only a C++ compiler.
set -euo pipefail
here="$(cd "$(dirname "$0")" && pwd)"
root="$(cd "$here/../.." && pwd)"
out="$(mktemp -d)"
trap 'rm -rf "$out"' EXIT
"${CXX:-c++}" -std=c++17 -Wall -Wextra -Werror -I"$here/stub" -I"$root/src" \
  "$here/test_core.cpp" "$root/src/DropCeilingArtNet.cpp" -o "$out/test_core"
"$out/test_core"
