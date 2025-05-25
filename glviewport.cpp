#include "glviewport.h"
#include <QMouseEvent>
#include <QWheelEvent>
#include <vector>

GLViewport::GLViewport(QWidget *parent):QOpenGLWidget(parent){}

/* ---------- OpenGL boilerplate ------------------------------------------------ */
void GLViewport::initializeGL()
{
    initializeOpenGLFunctions();
    glEnable(GL_DEPTH_TEST);

    shader.addShaderFromSourceCode(QOpenGLShader::Vertex,
        "#version 330 core\n"
        "layout(location=0) in vec3 pos;"
        "uniform mat4 mvp;"
        "void main(){gl_Position=mvp*vec4(pos,1.0);}");          // no normals yet

    shader.addShaderFromSourceCode(QOpenGLShader::Fragment,
        "#version 330 core\n"
        "out vec4 frag;"
        "void main(){frag=vec4(0.8,0.8,1.0,1.0);}");             // flat colour

    shader.link();
    glGenVertexArrays(1,&vao);
    glGenBuffers(1,&vbo);
    glGenBuffers(1,&ebo);
}

void GLViewport::resizeGL(int w,int h)
{
    projection.setToIdentity();
    projection.perspective(45.f,float(w)/qMax(1,h),0.1f,100.f);
}

void GLViewport::updateView()
{
    const float ry=qDegreesToRadians(yaw), rp=qDegreesToRadians(pitch);
    QVector3D eye(distance*qCos(rp)*qSin(ry),
                  distance*qSin(rp),
                  distance*qCos(rp)*qCos(ry));
    view.setToIdentity();
    view.lookAt(eye,{0,0,0},{0,1,0});
}

void GLViewport::paintGL()
{
    glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
    if(indexCount==0) return;                                 // nothing loaded
    updateView();
    shader.bind();
    shader.setUniformValue("mvp",projection*view*model);
    glBindVertexArray(vao);
    glDrawElements(GL_TRIANGLES,indexCount,GL_UNSIGNED_INT,nullptr);
    glBindVertexArray(0);
    shader.release();
}

/* ---------- Assimp loader ----------------------------------------------------- */
bool GLViewport::loadModel(const QString &filePath)
{
    makeCurrent();
    const aiScene *scene = importer.ReadFile(
        filePath.toStdString(),
        aiProcess_Triangulate |
        aiProcess_GenSmoothNormals |
        aiProcess_JoinIdenticalVertices |
        aiProcess_PreTransformVertices);

    if(!scene || !scene->HasMeshes()){
	    doneCurrent();
	    return false;
    }
    uploadMesh(scene->mMeshes[0]);            // first mesh only (simple demo)
    doneCurrent();
    update();
    return true;
}

void GLViewport::uploadMesh(const aiMesh *m)
{
    std::vector<float> verts(m->mNumVertices*3);
    for(unsigned i=0;i<m->mNumVertices;++i){
        verts[i*3+0]=m->mVertices[i].x;
        verts[i*3+1]=m->mVertices[i].y;
        verts[i*3+2]=m->mVertices[i].z;
    }
    std::vector<unsigned> idx(m->mNumFaces*3);
    for(unsigned i=0;i<m->mNumFaces;++i){
        idx[i*3+0]=m->mFaces[i].mIndices[0];
        idx[i*3+1]=m->mFaces[i].mIndices[1];
        idx[i*3+2]=m->mFaces[i].mIndices[2];
    }
    indexCount=static_cast<int>(idx.size());

    glBindVertexArray(vao);

    glBindBuffer(GL_ARRAY_BUFFER,vbo);
    glBufferData(GL_ARRAY_BUFFER,verts.size()*sizeof(float),
                 verts.data(),GL_STATIC_DRAW);
    glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,0,nullptr);
    glEnableVertexAttribArray(0);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER,idx.size()*sizeof(unsigned),
                 idx.data(),GL_STATIC_DRAW);

    glBindVertexArray(0);
}

/* ---------- camera controls --------------------------------------------------- */
void GLViewport::mousePressEvent(QMouseEvent *e){ lastPos=e->pos(); }

void GLViewport::mouseMoveEvent(QMouseEvent *e)
{
    if(e->buttons()&Qt::LeftButton){
        float dx=lastPos.x()-e->position().x();
        float dy=e->position().y()-lastPos.y();
        yaw += dx*0.4f;   pitch += dy*0.4f;
        pitch=qBound(-89.9f,pitch,89.9f);
        lastPos=e->pos();
        update();
    }
}
void GLViewport::wheelEvent(QWheelEvent *e)
{
    distance*= (e->angleDelta().y()>0)?0.9f:1.1f;
    distance=qBound(0.5f,distance,50.f);
    update();
}

