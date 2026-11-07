#!/bin/bash
# 增量编译 QtScrcpy
set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"
cd "$REPO_ROOT"

cmake --build build --config Release -j"$(nproc)"
echo "编译完成: $REPO_ROOT/output/x64/Release/QtScrcpy"
