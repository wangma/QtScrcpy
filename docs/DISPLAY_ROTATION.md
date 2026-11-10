# Host-side display rotation (landscape / portrait)

[中文版本](DISPLAY_ROTATION_zh.md)

This fork (`camera-dev` branch) adds **host-side display rotation**: the mirrored picture can be rotated by 90° clockwise on the PC, so a portrait phone can be viewed in landscape on a landscape monitor (and vice versa).

The phone itself is **never** rotated by this feature - no `rotateDevice()` call, no change to `user_rotation` / `accelerometer_rotation`. Only the mirroring window is affected.

## Why

By default the window aspect ratio follows the video frame: a portrait phone produces a tall, narrow window that wastes most of a landscape monitor, while a portrait-locked phone in a landscape game produces a wide window.

Host-side rotation lets the user decide how the picture is laid out on the PC without touching the device:

- watch a portrait phone full-width on a landscape monitor (phone stays on its stand);
- watch landscape content on a portrait / secondary portrait monitor;
- keep mouse aim and cursor warp correct in key-mapping (game) mode after rotating.

## Usage

Three equivalent entry points; the change is applied immediately and persisted:

| Entry | Action | Notes |
|---|---|---|
| Start dialog | `display orientation` combo box | `follow video` (default) / `force portrait` / `force landscape`; applied before the first frame so the window does not jump |
| Tool bar | rotation icon button | Right side of the tool form; toggles portrait/landscape |
| Shortcut | `Ctrl + Shift + R` (`Cmd + Shift + R` on macOS) | Same as the tool bar button |

`follow video` is the default and keeps the upstream behaviour unchanged.

The new shortcut does not conflict with the existing `Ctrl + R`, which really rotates the device:

| Action | Phone orientation | PC window orientation |
|---|---|---|
| `Ctrl + R` (existing) | changed | follows the phone |
| `Ctrl + Shift + R` (new) | **unchanged** | changed |

## Persistence

Stored in `UserBootConfig::displayOrientation`:

- key `DisplayOrientation` in `config/userdata.ini` (`[common]` section);
- values `0` = follow video, `1` = force portrait, `2` = force landscape; read with `qBound(0, v, 2)`;
- the value is applied to `VideoForm` right after connecting and mirrored back into the start dialog combo box (`displayOrientationChanged` signal), so the tool bar, the shortcut and the dialog never disagree.


## Implementation

### Rotation angle

`VideoForm::updateDisplayRotation(frameSize)` rotates only when the frame orientation differs from the requested one:

| Frame | Requested | Rotation |
|---|---|---|
| portrait | follow video / force portrait | 0° |
| portrait | force landscape | **90°** |
| landscape | follow video / force landscape | 0° |
| landscape | force portrait | **90°** |

The angle is recalculated whenever the frame size changes, so a mid-session phone rotation keeps the user's choice.

### Renderer

`QYuvOpenGLWidget::setRotation()` and `MetalVideoWindow::setRotation()` (macOS) only rewrite the **texture coordinates** in the VBO:

- vertex positions, window layout and pixel data stay untouched - no extra CPU cost, no re-encoding;
- `m_vboRotation` / `m_rotation` are compared on each paint so the rotation is always applied before the next frame is drawn;
- the frame size (`m_frameSize`) stays the original one, so capture and decode are unaffected.

### Window ratio

`VideoForm::applyDisplaySize()` swaps width/height into `m_displayFrameSize` for 90°/270°, recalculates `keepRatioWidget`'s ratio and resizes the window with the existing limits. Window geometry save/restore uses `isDisplayPortrait()`, based on the display orientation instead of the frame orientation.

### Input mapping

`VideoForm::mapInputPos()` maps the window position back into the unrotated coordinate system (`mapped = (y, w - x)` for 90° clockwise), and `VideoForm::inputShowSize()` returns `(height, width)` for 90°/270°, so the scale factor inside `InputConvertBase` stays correct. All mouse / wheel / drag events emit `device->mouseEvent(..., m_frameSize, inputShowSize())`.

### Key-mapping (game) mode

Game mode uses relative movement plus cursor warping, so the delta direction has to be rotated too:

- `InputConvertBase::displayDeltaToScreenDelta()` performs the inverse rotation of the delta;
- the angle is propagated through `Device::setDisplayRotation()` -> `Controller::setDisplayRotation()` -> `InputConvertBase::setDisplayRotation()`, and is restored when `inputConvert` is recreated;
- the conversion is skipped when the angle is 0, keeping upstream behaviour with zero overhead.

### Exceptions

- **flex display** (virtual display): forced back to `follow video` and the tool bar button is hidden, because the client owns the display size;
- **camera mode**: rotation button hidden as well;
- square frames are never rotated.

## Changed files

Main repository (22 files, +551 / -68): `render/qyuvopenglwidget.*`, `render/metalvideowindow.*`, `ui/videoform.*`, `ui/toolform.*`, `ui/dialog.*`, `util/config.*`, and the four i18n language packs (`en_US`, `zh_CN`, `ja_JP`, `ko_KR`).

Submodule `QtScrcpyCore` (8 files, +76): `inputconvert/inputconvertbase.*`, `inputconvert/inputconvertgame.cpp`, `controller/controller.*`, `device/device.*`, `include/QtScrcpyCore.h`.

The submodule points to the upstream repository, which is read-only for us, so those changes are shipped as a patch: `docs/patches/qtscrcpycore-display-rotation.patch`

```bash
# from the repository root, submodule already initialised
git -C QtScrcpy/QtScrcpyCore apply ../../docs/patches/qtscrcpycore-display-rotation.patch
```

## Test results

Environment: Orange Pi 5 Plus (RK3588 / aarch64, Ubuntu 22.04), Qt 5.15, X11, vivo S10 (1080x2400, portrait locked), Release build, `camera-dev` branch.

| Case | Expected | Observed | Result |
|---|---|---|---|
| Build (`make -j8`, Release, incl. submodule and i18n) | success | `EXIT=0`, 10 build iterations without errors | ✅ |
| Connect and start a session | mirroring works | device connected, scrcpy server started, first frame OK | ✅ |
| Force landscape on portrait phone | landscape window, picture rotated 90° | window `960x429` (~2400x1080), captured luminance map shows the desktop dock turned from the bottom edge to the left edge | ✅ |
| Shortcut toggle | width/height swap together with the rotation | window size toggled between `960x429` and `380x849` in the same session | ✅ |
| Persistence | restored after restart | `DisplayOrientation=2` written to `userdata.ini`, start dialog still shows `force landscape` after restart | ✅ |
| Invalid config value | no crash | clamped with `qBound(0, v, 2)` | ✅ (code review) |
| flex display / camera mode | no rotation, button hidden | falls back to `FollowVideo` and hides the button | ✅ (code review) |
| Window geometry restore | restored according to display orientation | uses `isDisplayPortrait()` | ✅ (code review) |
| Click / long-press / swipe accuracy | tap lands where clicked | not verified point by point (automated X11 injection limits); mapping and `inputShowSize()` are implemented as a pair | ⚠️ manual check pending |
| Game mode aim and warp bounds | direction and bounds stay correct | `displayDeltaToScreenDelta()` implemented, not verified item by item | ⚠️ manual check pending |

### Known limitations

1. Only `0°` and `90°` are produced by the current flow (frame orientation vs requested orientation); the `180°` / `270°` mapping code is kept for future use;
2. flex display and camera mode do not support host-side rotation;
3. toggling recomputes the window size from the display ratio (subject to the screen-size limits) instead of keeping the previous window width;
4. under Linux, XTest-injected input may be ignored while another full-screen window covers the mirroring window - activate the mirroring window before testing.

## See also

- [ADB troubleshooting on Orange Pi aarch64](ORANGEPI_ADB_TROUBLESHOOTING.md)
- [Key map description](KeyMapDes.md)
- [Develop guide](DEVELOP.md)
