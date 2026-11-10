#ifndef VIDEOFORM_H
#define VIDEOFORM_H

#include <QPointer>
#include <QTimer>
#include <QWidget>

#include "../QtScrcpyCore/include/QtScrcpyCore.h"

namespace Ui
{
    class videoForm;
}

class ToolForm;
class FileHandler;
class QYUVOpenGLWidget;
class QLabel;
class MetalVideoWidget;
class VideoForm : public QWidget, public qsc::DeviceObserver
{
    Q_OBJECT
public:
    explicit VideoForm(bool framelessWindow = false, bool skin = true, bool showToolBar = true, int decodeMode = 0, QWidget *parent = 0);
    ~VideoForm();

    void staysOnTop(bool top = true);
    void updateShowSize(const QSize &newSize);
    void updateRender(int width, int height, uint8_t* dataY, uint8_t* dataU, uint8_t* dataV, int linesizeY, int linesizeU, int linesizeV);
    void setSerial(const QString& serial);
    QRect getGrabCursorRect();
    const QSize &frameSize();
    void resizeSquare();
    void removeBlackRect();
    void showFPS(bool show);
    void switchFullScreen();
    bool isHost();

    // 画面显示方向：跟随视频（默认）/ 强制竖屏 / 强制横屏（仅主机侧旋转，不改变手机）
    enum DisplayOrientation {
        FollowVideo = 0,
        ForcePortrait = 1,
        ForceLandscape = 2,
    };
    // 设置显示方向（会立即生效并同步窗口比例、输入映射）
    void setDisplayOrientation(int orientation);
    // 在竖屏/横屏之间切换（工具栏按钮和快捷键使用）
    void toggleDisplayOrientation();
    int displayOrientation() const;
    // 当前画面顺时针旋转角度，0/90/180/270
    int displayRotation() const;
    // 当前显示是否为竖向（以画面显示尺寸判断）
    bool isDisplayPortrait() const;

signals:
    // 显示方向发生变化（工具栏/快捷键操作后会触发）
    void displayOrientationChanged(int orientation);

private:
    void onFrame(int width, int height, uint8_t* dataY, uint8_t* dataU, uint8_t* dataV,
                 int linesizeY, int linesizeU, int linesizeV) override;
    // VideoToolbox Metal 路径帧回调（仅 macOS arm64）
    void onFrameMetal(void* cvPixelBuffer, int width, int height) override;
    void updateFPS(quint32 fps) override;
    void onVideoSessionChanged(const QSize &size, bool clientResized) override;
    void grabCursor(bool grab) override;

    void updateStyleSheet(bool vertical);
    QMargins getMargins(bool vertical);
    void initUI();

    // 根据当前帧尺寸和显示方向重新计算旋转角度
    void updateDisplayRotation(const QSize &frameSize);
    // 按显示方向重算窗口比例并调整窗口大小（强制刷新，不受帧尺寸是否变化影响）
    void applyDisplaySize();
    // 把窗口内坐标映射为“未旋转”的窗口坐标（考虑显示旋转，供输入转换使用）
    QPointF mapInputPos(const QPointF &pos) const;
    // 与 mapInputPos 配套的 showSize：旋转90/270°时宽高互换，保证输入缩放比例不变
    QSize inputShowSize() const;
    // 将当前显示方向写入配置
    void persistDisplayOrientation();

    void showToolForm(bool show = true);
    void moveCenter();
    void installShortcut();
    QRect getScreenRect();

protected:
    void mousePressEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseDoubleClickEvent(QMouseEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
    void keyReleaseEvent(QKeyEvent *event) override;

    void paintEvent(QPaintEvent *) override;
    void showEvent(QShowEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
    void closeEvent(QCloseEvent *event) override;

    void dragEnterEvent(QDragEnterEvent *event) override;
    void dragMoveEvent(QDragMoveEvent *event) override;
    void dragLeaveEvent(QDragLeaveEvent *event) override;
    void dropEvent(QDropEvent *event) override;

private:
    // 获取当前视频渲染 widget（OpenGL 或 Metal 容器）
    QWidget* videoWidget() const;
    // 是否使用 Metal 渲染路径
    bool isMetalMode() const;

    // ui
    Ui::videoForm *ui;
    QPointer<ToolForm> m_toolForm;
    QPointer<QWidget> m_loadingWidget;
    QPointer<QYUVOpenGLWidget> m_videoWidget;

    // Metal 渲染路径（仅 macOS arm64）
    QPointer<MetalVideoWidget> m_metalWidget;

    QPointer<QLabel> m_fpsLabel;

    //inside member
    QSize m_frameSize;
    QSize m_normalSize;
    // 按显示方向旋转后的画面尺寸（与 m_frameSize 宽高互换）
    QSize m_displayFrameSize;
    // 画面显示方向（DisplayOrientation）
    int m_displayOrientation = FollowVideo;
    // 画面顺时针旋转角度
    int m_displayRotation = 0;
    QPoint m_dragPosition;
    float m_widthHeightRatio = 0.5f;
    bool m_skin = true;
    QPoint m_fullScreenBeforePos;
    QString m_serial;
    int m_decodeMode = 0;
    bool m_metalFirstFrame = true;  // Metal 首次帧标记
    bool m_flexDisplay = false;
    bool m_preventAutoResize = false;
    QTimer m_flexResizeTimer;
    QSize m_pendingDisplaySize;

    //Whether to display the toolbar when connecting a device.
    bool show_toolbar = true;
};

#endif // VIDEOFORM_H
