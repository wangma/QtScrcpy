#ifndef QYUVOPENGLWIDGET_H
#define QYUVOPENGLWIDGET_H
#include <QOpenGLBuffer>
#include <QOpenGLFunctions>
#include <QOpenGLShaderProgram>
#include <QOpenGLWidget>

class QYUVOpenGLWidget
    : public QOpenGLWidget
    , protected QOpenGLFunctions
{
    Q_OBJECT
public:
    explicit QYUVOpenGLWidget(QWidget *parent = nullptr);
    virtual ~QYUVOpenGLWidget() override;

    QSize minimumSizeHint() const override;
    QSize sizeHint() const override;

    void setFrameSize(const QSize &frameSize);
    const QSize &frameSize();
    void updateTextures(quint8 *dataY, quint8 *dataU, quint8 *dataV, quint32 linesizeY, quint32 linesizeU, quint32 linesizeV);
    // 画面顺时针旋转角度（0/90/180/270），仅改变纹理坐标，不改变窗口内容布局
    void setRotation(int rotation);
    int rotation() const;

protected:
    void initializeGL() override;
    void paintGL() override;
    void resizeGL(int width, int height) override;

private:
    void initShader();
    void initTextures();
    void deInitTextures();
    void updateTexture(GLuint texture, quint32 textureType, quint8 *pixels, quint32 stride);
    // 按当前旋转角度重写vbo中的纹理坐标（调用时需保证GL上下文已为当前上下文）
    void updateVboRotation();

private:
    // 视频帧尺寸
    QSize m_frameSize = { -1, -1 };
    bool m_needUpdate = false;
    bool m_textureInited = false;
    // 画面顺时针旋转角度
    int m_rotation = 0;
    // vbo中当前已写入的旋转角度，用于在paintGL中纠正未及时写入的情况
    int m_vboRotation = -1;

    // 顶点缓冲对象(Vertex Buffer Objects, VBO)：默认即为VertexBuffer(GL_ARRAY_BUFFER)类型
    QOpenGLBuffer m_vbo;

    // 着色器程序：编译链接着色器
    QOpenGLShaderProgram m_shaderProgram;

    // YUV纹理，用于生成纹理贴图
    GLuint m_texture[3] = { 0 };
};

#endif // QYUVOPENGLWIDGET_H
