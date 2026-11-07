#!/bin/bash
# 从本仓库安装 aarch64 静态版 adb 到 /usr/local/bin
set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"
ADB_BIN="$REPO_ROOT/tools/adb/linux-aarch64/adb"

if [ ! -f "$ADB_BIN" ]; then
    echo "错误: 未找到 $ADB_BIN" >&2
    exit 1
fi

ARCH="$(uname -m)"
if [ "$ARCH" != "aarch64" ]; then
    echo "警告: 当前架构为 $ARCH，本二进制为 aarch64，可能无法运行" >&2
fi

if [ -f "$REPO_ROOT/tools/adb/linux-aarch64/SHA256SUMS" ]; then
    (cd "$REPO_ROOT/tools/adb/linux-aarch64" && sha256sum -c SHA256SUMS) || {
        echo "错误: SHA256 校验失败" >&2
        exit 1
    }
fi

sudo install -m 755 "$ADB_BIN" /usr/local/bin/adb
hash -r
echo "已安装到 /usr/local/bin/adb"
adb version
