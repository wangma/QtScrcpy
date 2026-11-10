#ifndef METALVIDEOWINDOW_H
#define METALVIDEOWINDOW_H

#include <QWidget>

typedef struct __CVBuffer *CVPixelBufferRef;

class MetalVideoWidget : public QWidget
{
    Q_OBJECT
public:
    explicit MetalVideoWidget(QWidget *parent = nullptr);
    ~MetalVideoWidget();

    bool initMetal();
    void renderFrame(CVPixelBufferRef pixelBuffer, int width, int height);
    // 画面顺时针旋转角度（0/90/180/270），仅改变纹理坐标，不改变窗口内容布局
    void setRotation(int rotation);
    int rotation() const;

    QPaintEngine* paintEngine() const override { return nullptr; }

protected:
    void paintEvent(QPaintEvent *) override {}

private:
    struct Impl;
    Impl *d = nullptr;
    bool m_metalReady = false;
    // 画面顺时针旋转角度
    int m_rotation = 0;
};

#endif
