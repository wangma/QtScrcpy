# Orange Pi 5 Plus (Ubuntu 22.04, aarch64) ADB 排查记录

本文记录在 Orange Pi 5 Plus 上为 QtScrcpy 配置 ADB 的完整过程，包括所有失败的尝试和最终可行方案，供后续遇到类似问题时参考。

## 环境

- 硬件：Orange Pi 5 Plus (RK3588, aarch64)
- 系统：Ubuntu 22.04 (glibc 2.35)
- 手机：vivo S10 (OriginOS)
- QtScrcpy：v4.2.1，分支 `camera-dev`

## 现象

1. QtScrcpy 日志反复输出 `adb not found`，设备列表为空。
2. 终端直接执行 `adb devices` 显示 `device offline`。
3. 手机通过 USB 连接时，`lsusb` 能看到 `vivo S10`，但 `dmesg` 无 USB 枚举日志，`lsusb -v` 无法读取接口描述符。

## 排查过程

### 1. QtScrcpy 的 adb 查找路径

日志显示 QtScrcpy 查找的是**可执行文件同级目录**下的 adb：

```
adb path: /home/orangepi/GithubProject/QtScrcpy/output/x64/Release/adb
```

`config/config.ini` 中的 `AdbPath` 未生效。正确做法是在 `output/x64/Release/` 下放 adb（符号链接或二进制）。`output/x64` 是 64 位 Linux 的固定命名，不代表架构错误。

### 2. 系统 adb 版本过低

Ubuntu 22.04 官方源的 adb 是 `1.0.41 (Version 28.0.2-debian)`，**不支持 `adb pair`**（该命令需要 platform-tools 30.0.0+）。执行 `adb pair` 报 `unknown command pair`。

### 3. 尝试升级 adb 的失败路径

| 尝试 | 结果 |
|---|---|
| Google 官方 `platform-tools-latest-linux.zip` | x86_64 二进制，aarch64 上 `Exec format error` |
| Ubuntu 24.04 的 `adb_34.0.5-12_arm64.deb` | 依赖 `libc6>=2.38`、`libstdc++6>=13.1` 等，22.04 无法满足 |
| 直接 `dpkg-deb -x` 提取 24.04 adb 二进制 | 缺 `libprotobuf.so.32` |
| 下载 `libprotobuf32_3.21.12-8ubuntu5_arm64.deb` | 镜像 404 |
| Termux `android-tools` (aarch64) | 解释器为 `/system/bin/linker64`（Android），Ubuntu 上 `invalid ELF header` |
| `patchelf` 修改 Termux adb 解释器 | 运行时仍因 Bionic libc vs glibc 差异失败 |

### 4. 最终可行方案

使用 **lzhiyong/android-sdk-tools** 提供的**静态编译版 aarch64 adb**：

- 项目：https://github.com/lzhiyong/android-sdk-tools
- 版本：`33.0.3`（静态链接，无动态库依赖）
- 文件类型：`ELF 64-bit LSB executable, ARM aarch64, statically linked`
- 已收录到本仓库：`tools/adb/linux-aarch64/adb`

验证：

```bash
$ adb version
Android Debug Bridge version 1.0.41
Version 33.0.3-
```

`adb pair` 命令可用。

## USB 连接失败说明

Orange Pi 5 Plus 的 USB 3.0 端口在连接 vivo S10 时存在兼容性问题：`lsusb` 能列出设备，但 `dmesg` 无枚举日志、`lsusb -v` 无法读取接口，导致 ADB 握手始终 `offline`。更换 USB 2.0 端口、数据线、禁用 `brltty` 均未解决。**最终放弃 USB，改用无线 ADB**。

## 无线 ADB 配对流程

### 前提

- 手机与 Orange Pi 连接同一 Wi-Fi（同一网段）
- 手机 Android 11+（支持“无线调试”）
- adb 版本 ≥ 30.0.0（本仓库内置 33.0.3 满足）

### 步骤

1. 手机开启 `设置 → 系统管理 → 开发者选项 → 无线调试`。
2. 点击“**无线调试**”文字进入二级页面（只拨开关不够）。
3. 点击“**使用配对码配对设备**”，记下 **IP、配对端口、6 位配对码**（30 秒内有效）。
4. 在 Orange Pi 上执行：

```bash
adb kill-server
adb pair <手机IP>:<配对端口>
# 输入配对码
```

成功显示 `Successfully paired to <IP>:<端口>`。

5. 关闭配对弹窗，回到“无线调试”主页面，记下顶部显示的 **IP 和连接端口**（与配对端口不同）。
6. 连接：

```bash
adb connect <手机IP>:<连接端口>
adb devices
```

状态为 `device` 即成功。

## vivo 手机稳定性设置

无线 ADB 连接后，vivo 的后台限制可能导致掉线。建议：

1. `设置 → 电池 → 后台耗电管理`，将“ADB”或“Shell”相关进程设为**允许后台高耗电**。
2. `设置 → WLAN → 网络助理`，关闭“自动切换数据网络”“智能选网”。
3. `设置 → WLAN → 高级设置`，开启“锁屏保持 Wi-Fi 连接”。
4. 开发者选项中关闭“**监控 ADB 安装应用**”。

## 重连

无线调试端口每次重启手机或开关无线调试都会变化。重连：

```bash
adb disconnect
adb connect <手机IP>:<新连接端口>
```

若提示 `unauthorized` 或 `failed to authenticate`，说明配对记录失效，需重新执行 `adb pair`。

## 相关脚本

- `scripts/linux/install-adb.sh`：从本仓库安装 adb 到 `/usr/local/bin`
- `scripts/linux/wifi-adb.sh`：一键无线连接
- `scripts/linux/build.sh`：增量编译
