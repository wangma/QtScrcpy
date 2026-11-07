# adb (linux-aarch64, static)

## 来源

- 项目：https://github.com/lzhiyong/android-sdk-tools
- 版本：`33.0.3`
- 构建：静态链接，无动态库依赖

## 用途

Orange Pi 5 Plus 等 aarch64 Linux 设备上运行 QtScrcpy。Ubuntu 22.04 官方源 adb 版本过低（28.0.2），不支持 `adb pair`；Ubuntu 24.04 的 adb 包依赖 glibc 2.38，无法在 22.04 安装。本二进制是社区验证可用的替代方案。

## 校验

```bash
sha256sum -c SHA256SUMS
```

预期输出：`adb: OK`

哈希：85e7dd2966950178e889593bf8a33a508bf635f5a953fd1ed19ca417c1cf0478  adb

## 安装

```bash
sudo install -m 755 adb /usr/local/bin/adb
```

或使用仓库脚本：`scripts/linux/install-adb.sh`。

## 许可

adb 源码来自 AOSP，Apache License 2.0。本二进制由 lzhiyong/android-sdk-tools 项目编译发布，原样收录，未做修改。
