#!/bin/bash
# 无线 ADB 一键连接
# 用法: ./wifi-adb.sh <手机IP> <连接端口>
set -e

if [ $# -ne 2 ]; then
    echo "用法: $0 <手机IP> <连接端口>" >&2
    echo "连接端口在 手机 设置→开发者选项→无线调试 主页面查看" >&2
    exit 1
fi

adb disconnect >/dev/null 2>&1 || true
adb connect "$1:$2"
adb devices
