#pragma once
#include <QOpenGLWidget>
#include <QOpenGLFunctions_3_3_Core>
#include <QOpenGLShaderProgram>
#include <QMatrix4x4>
#include <QtMath>
#include <QImage>
#include <QFileInfo>
#include <QFile>
#include <assimp/scene.h>
#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>

class GLViewport : public QOpenGLWidget,
                   protected QOpenGLFunctions_3_3_Core
{
    Q_OBJECT
public:
    explicit GLViewport(QWidget *parent=nullptr);
    bool loadModel(const QString &filePath);

protected:
    void initializeGL() override;
    void resizeGL(int w,int h) override;
    void paintGL() override;

    void mousePressEvent(QMouseEvent *e) override;
    void mouseMoveEvent(QMouseEvent *e)  override;
    void wheelEvent(QWheelEvent *e)      override;
    void keyPressEvent(QKeyEvent *e) override;

private:
    void updateView();
    void uploadMesh(const aiMesh *mesh);
    void calculateBoundingBox(const aiMesh *mesh);
    void resetCamera();
    bool loadTexture(const QString &texturePath);
    void loadMaterialTextures(const aiScene *scene, const QString &modelDir);
    bool loadEmbeddedTexture(const aiTexture* aiTex);

    QOpenGLShaderProgram shader;
    GLuint vao=0, vbo=0, ebo=0;
    GLuint textureID = 0;
    int    indexCount=0;
    bool   hasLoadedTexture = false;

    float distance=3.0f, yaw=0.0f, pitch=0.0f;
    QPoint lastPos;

    QMatrix4x4 projection, view, model;
    
    QVector3D modelCenter;
    float modelRadius = 1.0f;
    QVector3D boundingMin, boundingMax;

    Assimp::Importer importer;
};
