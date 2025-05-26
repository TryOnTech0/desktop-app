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

    // Texture'lı vertex shader
    shader.addShaderFromSourceCode(QOpenGLShader::Vertex,
        "#version 330 core\n"
        "layout(location=0) in vec3 pos;"
        "layout(location=1) in vec2 texCoord;"
        "layout(location=2) in vec3 normal;"
        "uniform mat4 mvp;"
        "out vec2 TexCoord;"
        "out vec3 Normal;"
        "void main(){"
        "    gl_Position = mvp * vec4(pos, 1.0);"
        "    TexCoord = texCoord;"
        "    Normal = normal;"
        "}");

    // Texture'lı fragment shader
    shader.addShaderFromSourceCode(QOpenGLShader::Fragment,
    "#version 330 core\n"
    "in vec2 TexCoord;"
    "in vec3 Normal;"
    "out vec4 frag;"
    "uniform sampler2D ourTexture;"
    "uniform bool hasTexture;"
    "void main(){"
    "    if(hasTexture) {"
    "        vec4 texColor = texture(ourTexture, TexCoord);"
    "        if(texColor.a < 0.1) discard;" // Alpha testing
    "        frag = texColor;"
    "    } else {"
    "        // Basit lighting"
    "        vec3 lightDir = normalize(vec3(0.5, 1.0, 0.3));"
    "        vec3 norm = normalize(Normal);"
    "        float diff = max(dot(norm, lightDir), 0.3);"
    "        vec3 color = vec3(0.8, 0.8, 1.0) * diff;"
    "        frag = vec4(color, 1.0);"
    "    }"
    "}");

    shader.link();    
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);
    glGenBuffers(1, &ebo);
    glGenTextures(1, &textureID);  // Texture ID ekle
}

void GLViewport::resizeGL(int w,int h)
{
    projection.setToIdentity();
    projection.perspective(45.f,float(w)/qMax(1,h),0.1f,100.f);
}

void GLViewport::updateView()
{
    const float ry = qDegreesToRadians(yaw);
    const float rp = qDegreesToRadians(pitch);
    
    // Kamera pozisyonu
    QVector3D eye(distance * qCos(rp) * qSin(ry),
                  distance * qSin(rp),
                  distance * qCos(rp) * qCos(ry));
    
    // Sahne merkezine bak (0,0,0) - model merkezi değil
    QVector3D target(0, 0, 0); 
    QVector3D up(0, 1, 0);
    
    view.setToIdentity();
    view.lookAt(eye, target, up);
}

void GLViewport::paintGL()
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    if(indexCount == 0) return;

    updateView();
    shader.bind();
    shader.setUniformValue("mvp", projection * view * model);
    shader.setUniformValue("hasTexture", hasLoadedTexture);
    
    if(hasLoadedTexture) {
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, textureID);
        shader.setUniformValue("ourTexture", 0);
    }
    
    glBindVertexArray(vao);
    glDrawElements(GL_TRIANGLES, indexCount, GL_UNSIGNED_INT, nullptr);
    glBindVertexArray(0);
    shader.release();
}

/* ---------- Assimp loader ----------------------------------------------------- */
bool GLViewport::loadModel(const QString &filePath)
{
    makeCurrent();
    
    printf("Model yükleniyor: %s\n", filePath.toStdString().c_str());
    
    // Eski texture'ı temizle
    if (hasLoadedTexture) {
        glDeleteTextures(1, &textureID);
        hasLoadedTexture = false;
    }
    
    const aiScene *scene = importer.ReadFile(
        filePath.toStdString(),
        aiProcess_Triangulate |
        aiProcess_GenSmoothNormals |
        aiProcess_JoinIdenticalVertices |
        aiProcess_PreTransformVertices);

    if(!scene || !scene->HasMeshes()) {
        printf("Model yükleme hatası: %s\n", importer.GetErrorString());
        doneCurrent();
        return false;
    }
    
    // Bounding box hesapla
    calculateBoundingBox(scene->mMeshes[0]);
    
    // Mesh'i yükle
    uploadMesh(scene->mMeshes[0]);
    
    // Embedded texture'ları yükle
    loadMaterialTextures(scene, ""); // Dizin gerekmez
    
    // Kamerayı otomatik ayarla
    resetCamera();
    
    doneCurrent();
    update();
    return true;
}

void GLViewport::uploadMesh(const aiMesh *m)
{
    printf("uploadMesh: vertices=%d, faces=%d\n", m->mNumVertices, m->mNumFaces);
    printf("Has texture coords: %s\n", m->mTextureCoords[0] ? "YES" : "NO");
    printf("Has normals: %s\n", m->mNormals ? "YES" : "NO");
    
    // Vertex data: position + texCoord + normal
    std::vector<float> verts;
    for(unsigned i = 0; i < m->mNumVertices; ++i) {
        // Position
        verts.push_back(m->mVertices[i].x);
        verts.push_back(m->mVertices[i].y);
        verts.push_back(m->mVertices[i].z);
        
        // Texture coordinates
        if(m->mTextureCoords[0]) {
            verts.push_back(m->mTextureCoords[0][i].x);
            verts.push_back(m->mTextureCoords[0][i].y);
        } else {
            verts.push_back(0.0f);
            verts.push_back(0.0f);
        }
        
        // Normals
        if(m->mNormals) {
            verts.push_back(m->mNormals[i].x);
            verts.push_back(m->mNormals[i].y);
            verts.push_back(m->mNormals[i].z);
        } else {
            verts.push_back(0.0f);
            verts.push_back(0.0f);
            verts.push_back(1.0f);
        }
    }

    // Index data
    std::vector<unsigned> idx(m->mNumFaces * 3);
    for(unsigned i = 0; i < m->mNumFaces; ++i) {
        idx[i*3+0] = m->mFaces[i].mIndices[0];
        idx[i*3+1] = m->mFaces[i].mIndices[1];
        idx[i*3+2] = m->mFaces[i].mIndices[2];
    }
    
    indexCount = static_cast<int>(idx.size());

    glBindVertexArray(vao);

    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, verts.size() * sizeof(float), verts.data(), GL_STATIC_DRAW);

    // Position attribute (location = 0)
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    
    // Texture coordinate attribute (location = 1) 
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    
    // Normal attribute (location = 2)
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(5 * sizeof(float)));
    glEnableVertexAttribArray(2);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, idx.size() * sizeof(unsigned), idx.data(), GL_STATIC_DRAW);

    glBindVertexArray(0);
}

/* ---------- camera controls --------------------------------------------------- */
void GLViewport::mousePressEvent(QMouseEvent *e){ lastPos=e->pos(); }

void GLViewport::mouseMoveEvent(QMouseEvent *e)
{
    if(e->buttons() & Qt::LeftButton) {
        float dx = lastPos.x() - e->position().x();
        float dy = e->position().y() - lastPos.y();
        
        // Daha yumuşak hareket
        yaw += dx * 0.3f;
        pitch += dy * 0.3f;
        
        // Pitch sınırla (baş aşağı döndürmeyi engelle)
        pitch = qBound(-85.0f, pitch, 85.0f);
        
        lastPos = e->pos();
        update();
    }
    else if(e->buttons() & Qt::RightButton) {
        // Sağ tık ile pan (kaydırma)
        float dx = (e->position().x() - lastPos.x()) * 0.01f;
        float dy = (e->position().y() - lastPos.y()) * 0.01f;
        
        modelCenter.setX(modelCenter.x() - dx);
        modelCenter.setY(modelCenter.y() + dy);
        
        lastPos = e->pos();
        update();
    }
}

void GLViewport::wheelEvent(QWheelEvent *e)
{
    float zoomFactor = (e->angleDelta().y() > 0) ? 0.9f : 1.1f;
    distance *= zoomFactor;
    
    // Zoom sınırları (modele çok yakın/uzak gitmeyi engelle)
    float minDistance = modelRadius * 0.1f;
    float maxDistance = modelRadius * 10.0f;
    distance = qBound(minDistance, distance, maxDistance);
    
    update();
}

void GLViewport::calculateBoundingBox(const aiMesh *mesh)
{
    if(mesh->mNumVertices == 0) return;
    
    // İlk vertex ile başla
    boundingMin = QVector3D(mesh->mVertices[0].x, mesh->mVertices[0].y, mesh->mVertices[0].z);
    boundingMax = boundingMin;
    
    // Tüm vertex'leri kontrol et
    for(unsigned i = 1; i < mesh->mNumVertices; ++i) {
        QVector3D vertex(mesh->mVertices[i].x, mesh->mVertices[i].y, mesh->mVertices[i].z);
        
        boundingMin.setX(qMin(boundingMin.x(), vertex.x()));
        boundingMin.setY(qMin(boundingMin.y(), vertex.y()));
        boundingMin.setZ(qMin(boundingMin.z(), vertex.z()));
        
        boundingMax.setX(qMax(boundingMax.x(), vertex.x()));
        boundingMax.setY(qMax(boundingMax.y(), vertex.y()));
        boundingMax.setZ(qMax(boundingMax.z(), vertex.z()));
    }
    
    // Model merkezi
    modelCenter = (boundingMin + boundingMax) * 0.5f;
    
    // Model yarıçapı (en uzak köşe mesafesi)
    QVector3D size = boundingMax - boundingMin;
    modelRadius = qMax(qMax(size.x(), size.y()), size.z()) * 0.6f; // Biraz padding
    
    printf("Model Bounding Box:\n");
    printf("  Center: (%.2f, %.2f, %.2f)\n", modelCenter.x(), modelCenter.y(), modelCenter.z());
    printf("  Radius: %.2f\n", modelRadius);
    printf("  Size: (%.2f, %.2f, %.2f)\n", size.x(), size.y(), size.z());
    fflush(stdout);
}

void GLViewport::resetCamera()
{
    // Kamerayı modeli güzel gösterecek şekilde ayarla
    distance = modelRadius * 2.5f; 
    yaw = 45.0f;    
    pitch = 10.0f;  // Daha az yukarıdan, daha çok yandan bak
    
    // Model matrisini merkeze getir - Y ekseninde biraz ayarla
    model.setToIdentity();
    
    // Modeli hem X/Z'de merkeze, hem de Y'de zemin seviyesine getir
    QVector3D adjustedCenter = modelCenter;
    adjustedCenter.setY(boundingMin.y() + (boundingMax.y() - boundingMin.y()) * 0.3f); // Vücudu merkeze al
    
    model.translate(-adjustedCenter);
    
    printf("Kamera reset edildi: distance=%.2f, yaw=%.1f, pitch=%.1f\n", 
           distance, yaw, pitch);
    printf("Adjusted center: (%.2f, %.2f, %.2f)\n", 
           adjustedCenter.x(), adjustedCenter.y(), adjustedCenter.z());
    fflush(stdout);
}

bool GLViewport::loadTexture(const QString &texturePath)
{
    QImage image;
    if (!image.load(texturePath)) {
        printf("Texture yüklenemedi: %s\n", texturePath.toStdString().c_str());
        return false;
    }

    // OpenGL formatına çevir
    QImage glImage = image.convertToFormat(QImage::Format_RGBA8888).mirrored();

    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA,
                 glImage.width(), glImage.height(), 0,
                 GL_RGBA, GL_UNSIGNED_BYTE, glImage.bits());

    // Texture parametreleri
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glBindTexture(GL_TEXTURE_2D, 0);

    printf("Texture yüklendi: %s (%dx%d)\n",
           texturePath.toStdString().c_str(),
           glImage.width(), glImage.height());

    hasLoadedTexture = true;
    return true;
}

void GLViewport::loadMaterialTextures(const aiScene *scene, const QString &modelDir)
{
    if (!scene->HasMaterials()) {
        printf("Scene'de material bulunamadı\n");
        return;
    }
    
    printf("Material sayısı: %d\n", scene->mNumMaterials);
    printf("Embedded texture sayısı: %d\n", scene->mNumTextures);
    
    // Önce embedded texture'ları kontrol et
    if (scene->mNumTextures > 0) {
        for (unsigned int i = 0; i < scene->mNumTextures; i++) {
            aiTexture* texture = scene->mTextures[i];
            printf("Embedded texture %d: format=%s, boyut=%dx%d\n", 
                   i, texture->achFormatHint, texture->mWidth, texture->mHeight);
            
            if (loadEmbeddedTexture(texture)) {
                printf("Embedded texture başarıyla yüklendi\n");
                return;
            }
        }
    }
    
    // Material'lardaki texture referanslarını kontrol et
    for (unsigned int i = 0; i < scene->mNumMaterials; i++) {
        aiMaterial *mat = scene->mMaterials[i];
        
        for (unsigned int j = 0; j < mat->GetTextureCount(aiTextureType_DIFFUSE); j++) {
            aiString texPath;
            if (mat->GetTexture(aiTextureType_DIFFUSE, j, &texPath) == AI_SUCCESS) {
                printf("Material texture yolu: %s\n", texPath.C_Str());
                
                if (texPath.C_Str()[0] == '*') {
                    int texIndex = atoi(texPath.C_Str() + 1);
                    if (texIndex < (int)scene->mNumTextures) {
                        printf("Embedded texture index: %d\n", texIndex);
                        if (loadEmbeddedTexture(scene->mTextures[texIndex])) {
                            return;
                        }
                    }
                }
            }
        }
    }
    
    printf("Hiçbir texture bulunamadı\n");
}

bool GLViewport::loadEmbeddedTexture(const aiTexture* aiTex)
{
    if (!aiTex) return false;

    QImage image;

    // Compressed texture (PNG, JPG vs.)
    if (aiTex->mHeight == 0) {
        printf("Compressed texture yükleniyor, boyut: %d bytes\n", aiTex->mWidth);

        // Raw data'dan QImage oluştur
        QByteArray data((const char*)aiTex->pcData, aiTex->mWidth);
        if (!image.loadFromData(data)) {
            printf("Compressed texture yüklenemedi\n");
            return false;
        }
    }
    // Uncompressed texture (raw RGBA data)
    else {
        printf("Uncompressed texture yükleniyor: %dx%d\n", aiTex->mWidth, aiTex->mHeight);

        // RGBA formatından QImage oluştur
        image = QImage((const uchar*)aiTex->pcData,
                       aiTex->mWidth, aiTex->mHeight,
                       QImage::Format_RGBA8888);
    }

    if (image.isNull()) {
        printf("Texture image oluşturulamadı\n");
        return false;
    }

    // OpenGL texture'ını oluştur
    if (hasLoadedTexture) {
        glDeleteTextures(1, &textureID);
    }

    QImage glImage = image.convertToFormat(QImage::Format_RGBA8888);

    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA,
                 glImage.width(), glImage.height(), 0,
                 GL_RGBA, GL_UNSIGNED_BYTE, glImage.bits());

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glBindTexture(GL_TEXTURE_2D, 0);

    printf("Embedded texture OpenGL'e yüklendi: %dx%d\n",
           glImage.width(), glImage.height());

    hasLoadedTexture = true;
    return true;
}

void GLViewport::keyPressEvent(QKeyEvent *e)
{
    switch(e->key()) {
    case Qt::Key_R:
        resetCamera(); // R tuşu ile kamerayı reset et
        update();
        break;
    case Qt::Key_F:
        // F tuşu ile modeli frame'le (tam sığdır)
        distance = modelRadius * 2.0f;
        update();
        break;
    default:
        QOpenGLWidget::keyPressEvent(e);
        break;
    }
}
