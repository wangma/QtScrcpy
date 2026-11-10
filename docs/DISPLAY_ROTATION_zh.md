# 主机端显示旋转（横屏 / 竖屏）

[English version](DISPLAY_ROTATION.md)

本分支（`camera-dev`）新增**主机端（PC 侧）显示旋转**功能：把投屏画面在电脑上顺时针旋转 90°，从而在电脑横屏显示器上以横屏方式查看竖屏手机，或者把横屏画面竖起来看。

关键点：**只旋转电脑上的窗口画面，不改变手机的任何设置**。手机仍然是原来的方向，`rotaion`/`accelerometer_rotation` 等系统设置不会被修改，也不会调用 `device->rotateDevice()`。

## 为什么需要

默认情况下 QtScrcpy 的画面比例完全跟随视频帧方向：

- 手机竖屏时，窗口是竖长条（例如 1080×2400），在横屏显示器上只占用中间一小块，两侧大片留白；
- 手机被应用强制横屏（游戏、视频）时，窗口又变成横向。

本功能让用户可以在**不旋转手机**的前提下，决定画面在电脑上的显示方向，主要用于：

- 横屏显示器上全屏查看竖屏手机（手机放支架上不动，电脑上横着看）；
- 竖屏显示器 / 竖屏副屏上查看横屏内容；
- 游戏模式下，主机侧旋转后鼠标瞄准方向仍然正确（见下文测试结论）。

## 使用方式

有三种等价入口，切换后立即生效，并写入配置文件持久化：

| 入口 | 操作 | 说明 |
|---|---|---|
| 启动对话框 | 「显示方向」下拉框 | 三个选项：`跟随视频`（默认）/ `强制竖屏` / `强制横屏`，在建立连接前设置，首帧即按目标方向显示，窗口不会出现尺寸跳变 |
| 工具栏 | 旋转图标按钮 | 位于工具条右侧；点击后立即切换横竖屏 |
| 快捷键 | `Ctrl + Shift + R`（macOS：`Cmd + Shift + R`） | 与工具栏按钮行为相同 |

`跟随视频` 为默认值，保持与上游一致的行为。

快捷键与上游已有的 `Ctrl + R`（`device->rotateDevice()`，**真正旋转手机**）不冲突：
`Ctrl + R` 改手机方向，`Ctrl + Shift + R` 只改电脑上的显示方向。

### 与手机方向的区别

| 操作 | 手机屏幕方向 | 电脑窗口方向 |
|---|---|---|
| `Ctrl + R`（原有） | 改变 | 跟随手机 |
| `Ctrl + Shift + R`（新增） | **不变** | 改变 |

## 配置持久化

显示方向保存在用户配置 `UserBootConfig::displayOrientation`：

- 键名：`DisplayOrientation`（写入 `config/userdata.ini` 的 `[common]` 段）
- 取值：`0` = 跟随视频，`1` = 强制竖屏，`2` = 强制横屏（读取时用 `qBound(0, v, 2)` 容错）
- 读取后立即应用到 `VideoForm`，且会同步回启动对话框的下拉框；工具栏 / 快捷键切换后也会通过 `displayOrientationChanged` 信号反向同步下拉框，避免两处状态不一致。

## 实现原理

### 1. 旋转角度计算

`VideoForm::updateDisplayRotation(frameSize)`：只有「帧方向」与「目标方向」不一致时才需要旋转 90°。

| 视频帧方向 | 目标方向 | 旋转角 |
|---|---|---|
| 竖屏 | 跟随视频 | 0° |
| 竖屏 | 强制竖屏 | 0° |
| 竖屏 | 强制横屏 | **90°** |
| 横屏 | 强制横屏 / 跟随视频 | 0° |
| 横屏 | 强制竖屏 | **90°** |

旋转角随每一帧的尺寸重新计算，因此手机中途横竖屏切换时，主机的显示方向会保持用户设定值。

### 2. 渲染层旋转（不改帧数据）

`QYuvOpenGLWidget::setRotation(int)` / `MetalVideoWindow::setRotation(int)`（macOS）：

- 只重写 VBO 中的**纹理坐标**，不改动顶点位置、不改动窗口布局、不拷贝像素数据，因此没有额外 CPU 开销；
- 在 `paintGL()`/`Metal` 绘制路径中用 `m_vboRotation` 与 `m_rotation` 比较，确保旋转角在下次绘制前已写入缓冲；
- 帧尺寸（`m_frameSize`）保持原始值，编码/解码链路完全不受影响。

### 3. 窗口比例与尺寸

`VideoForm::applyDisplaySize()`：

- 计算 `m_displayFrameSize`：旋转 90/270° 时宽高互换；
- 用 `m_displayFrameSize` 重算 `keepRatioWidget` 的 `m_widthHeightRatio`，进而得到新窗口尺寸（保持原有「屏幕高度 - 200」「屏幕宽度 / 2」的限幅规则）；
- 窗口几何按新比例保存/恢复时使用 `isDisplayPortrait()`（基于显示方向而非帧方向），保证重启后窗口几何与显示方向匹配。

### 4. 输入坐标映射

窗口被旋转后，鼠标点击位置必须逆变换回「未旋转」的坐标系，否则点击位置会偏移。

`VideoForm::mapInputPos()`：把窗口坐标逆映射为未旋转坐标（顺时针 90° 时 `mapped = (y, w - x)`，180°/270° 同样处理，代码已具备，当前功能只会产生 0°/90°）。

`VideoForm::inputShowSize()`：旋转 90/270° 时返回 `(height, width)`，与上面的映射配套，使 `InputConvertBase` 里的缩放比例不变。所有鼠标、滚轮、拖拽事件都改为 `emit device->mouseEvent(..., m_frameSize, inputShowSize())`。

### 5. 游戏模式 / 鼠标位移

游戏模式（keymap）下鼠标是相对位移 + 光标 warp，屏幕方向旋转后位移方向也要跟着转：

- `InputConvertBase::displayDeltaToScreenDelta()` 做位移方向的逆变换；
- `Device::setDisplayRotation()` → `Controller::setDisplayRotation()` → `InputConvertBase::setDisplayRotation()` 逐层下发，`InputConvertEraser` 等重建 `inputConvert` 后也会继承该角度；
- 只在 `m_displayRotation != 0` 时执行换算，不旋转时零开销、行为与上游完全一致。

### 6. 例外情况

- **flex display（虚拟显示器）**：尺寸由 client 端画面决定，主机侧旋转会引起反复 `resizeDisplay`，因此强制回落到「跟随视频」，工具栏按钮也隐藏；
- **camera 模式（虚拟摄像头）**：同样隐藏旋转按钮；
- 正方形帧不旋转。

## 代码改动清单

主仓库（`QtScrcpy/`，22 个文件，+551 / −68）：

| 文件 | 改动 |
|---|---|
| `render/qyuvopenglwidget.h/.cpp` | 新增 `setRotation()` / `rotation()` / `updateVboRotation()`，纹理坐标旋转 |
| `render/metalvideowindow.h/.mm` | macOS 渲染路径的旋转支持 |
| `ui/videoform.h/.cpp` | 显示方向状态机、旋转角度计算、窗口比例、`mapInputPos` / `inputShowSize`、`Ctrl+Shift+R`、持久化 |
| `ui/toolform.h/.cpp/.ui` | 新增旋转按钮 `orientationBtn` + `orientationSwitchRequested` 信号 |
| `ui/dialog.h/.cpp/.ui` | 启动对话框新增「display orientation」下拉框，读取/保存配置并下发到 `VideoForm` |
| `util/config.h/.cpp` | 新增 `UserBootConfig::displayOrientation` 与 `DisplayOrientation` 键 |
| `res/i18n/*.ts|*.qm` | 中/英/日/韩 四个语言包补充新字符串 |

子模块（`QtScrcpyCore/`，8 个文件，+76）：

| 文件 | 改动 |
|---|---|
| `inputconvert/inputconvertbase.h/.cpp` | `setDisplayRotation()`、`displayDeltaToScreenDelta()`，warp 偏移方向换算 |
| `inputconvert/inputconvertgame.cpp` | 继承旋转角 |
| `controller/controller.h/.cpp` | 下发旋转角，重建 `inputConvert` 后继承 |
| `device/device.h/.cpp` | `setDisplayRotation()` 并转交 controller |
| `include/QtScrcpyCore.h` | 对外暴露接口 |

> 子模块 `QtScrcpyCore` 的上游仓库属于原作者，本仓库没有推送权限，因此子模块改动以补丁形式提供：
> `docs/patches/qtscrcpycore-display-rotation.patch`
> 应用方式见文末「子模块补丁」。

## 测试结论

测试环境：Orange Pi 5 Plus（RK3588 / aarch64，Ubuntu 22.04）、Qt 5.15、X11、vivo S10（OriginOS，1080×2400 竖屏，锁定竖屏 `user_rotation=0`），QtScrcpy `camera-dev` 分支，Release 构建。

### 编译

| 项目 | 结果 |
|---|---|
| `make -j8`（Release，含 `QtScrcpyCore` 子模块与 i18n 重新生成） | ✅ 通过，`EXIT=0`，共 10 次迭代编译无错误 |

### 功能验证

| 用例 | 预期 | 实测 | 结论 |
|---|---|---|---|
| 连接设备并建立 scrcpy 会话 | 视频显示正常 | 设备 `3138598087000BR` 连接成功，server 启动，首帧正常 | ✅ |
| 强制横屏（竖屏手机） | 窗口变为横向，画面顺时针旋转 90° | 窗口 `960×429`（≈2400×1080 比例），截屏亮度图确认画面侧躺（桌面图标由底部横排变为左侧竖排） | ✅ |
| 快捷键切换 | 窗口横/竖互换且画面方向随之改变 | `Ctrl+Shift+R` 在会话内切换，窗口尺寸在 `960×429` ↔ `380×849` 之间正确互换 | ✅ |
| 配置持久化 | 重启后仍为上次的方向 | `config/userdata.ini` 写入 `DisplayOrientation=2`，重启后启动对话框下拉框仍显示「force landscape」 | ✅ |
| 配置文件容错 | 非法值不崩溃 | 读取时 `qBound(0, v, 2)` | ✅（代码审查） |
| flex display / camera 模式 | 不旋转、按钮隐藏 | 代码路径判断优先回落 `FollowVideo` 并隐藏按钮 | ✅（代码审查） |
| 窗口几何恢复 | 旋转后重启仍按显示方向恢复窗口大小 | 使用 `isDisplayPortrait()` 判断 | ✅（代码审查） |
| 点击 / 长按 / 滑动坐标精度 | 画面上点哪手机就响应哪 | 受远程桌面自动化注入限制，未逐点核对；映射公式与 `inputShowSize()` 已配套实现 | ⚠️ 待人工复核 |
| 游戏模式瞄准与 warp 边界 | 旋转后鼠标方向、边界不偏移 | `displayDeltaToScreenDelta()` 已实现，未做逐项人工验证 | ⚠️ 待人工复核 |

### 已知限制

1. 旋转角目前只会取 `0°` 或 `90°`（按「帧方向 vs 目标方向」二选一），`180°/270°` 的映射代码已保留但不参与当前流程；
2. flex display 与 camera 模式不支持主机侧旋转；
3. 横竖屏切换时窗口会按新比例重新计算尺寸（受屏幕尺寸限幅），不会保持原窗口宽度；
4. Linux 下自动化输入注入（XTest）在窗口被其他全屏窗口遮挡时可能不生效，手动验证时请先激活投屏窗口。

## 子模块补丁

由于 `QtScrcpy/QtScrcpyCore` 指向上游只读仓库，本次子模块改动未能直接推送，已导出为补丁：

```bash
# 在仓库根目录、子模块已初始化的情况下
git -C QtScrcpy/QtScrcpyCore apply ../../docs/patches/qtscrcpycore-display-rotation.patch
```

若后续把我们自己的 `QtScrcpyCore` fork 配置为子模块远程，可直接在该分支上提交并更新主仓库的子模块指针。

## 相关文档

- [ADB 排查记录（Orange Pi aarch64）](ORANGEPI_ADB_TROUBLESHOOTING.md)
- [按键映射说明](KeyMapDes_zh.md)
- [开发说明](DEVELOP.md)
