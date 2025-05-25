#pragma once
#include <QOpenGLWidget>
#include <QOpenGLFunctions_3_3_Core>
#include <QOpenGLShaderProgram>
#include <QMatrix4x4>
#include <QtMath>
#include <assimp/scene.h>
#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>

class GLViewport : public QOpenGLWidget,
                   protected QOpenGLFunctions_3_3_Core
{
    Q_OBJECT
public:
    explicit GLViewport(QWidget *parent=nullptr);
    bool loadModel(const QString &filePath);      // << new public API

protected:
    void initializeGL() override;
    void resizeGL(int w,int h) override;
    void paintGL() override;

    void mousePressEvent(QMouseEvent *e) override;
    void mouseMoveEvent(QMouseEvent *e)  override;
    void wheelEvent(QWheelEvent *e)      override;

private:
    void updateView();
    void uploadMesh(const aiMesh *mesh);

    QOpenGLShaderProgram shader;
    GLuint vao=0, vbo=0, ebo=0;
    int    indexCount=0;

    float distance=3.0f, yaw=0.0f, pitch=0.0f;
    QPoint lastPos;

    QMatrix4x4 projection, view, model;

    Assimp::Importer importer;
};

