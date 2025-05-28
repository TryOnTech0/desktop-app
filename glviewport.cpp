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
   
bool fragmentOK = shader.addShaderFromSourceCode(QOpenGLShader::Fragment,
    "#version 330 core\n"
    "in vec2 TexCoord;"
    "out vec4 frag;"
    "uniform sampler2D ourTexture;"
    "uniform bool hasTexture;"
    "void main(){"
    "    if(hasTexture) {"
    "        frag = texture(ourTexture, TexCoord);" // Sadece texture
    "    } else {"
    "        frag = vec4(0.7, 0.7, 0.7, 1.0);"     // Gri renk
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
    glClearColor(0.2f, 0.3f, 0.4f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    
    if(indexCount == 0) return;

    updateView();
    shader.bind();
    shader.setUniformValue("mvp", projection * view * model);
    
    // TEXTURE BINDING DÜZELTMESİ
    printf("paintGL: hasLoadedTexture=%s, textureID=%d\n", 
           hasLoadedTexture ? "TRUE" : "FALSE", textureID);
    
    if(hasLoadedTexture && textureID > 0) {
        // Texture'ı aktif et
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, textureID);
        shader.setUniformValue("ourTexture", 0);
        shader.setUniformValue("hasTexture", GLint(1));
        printf("Texture aktif: ID=%d\n", textureID);
    } else {
        shader.setUniformValue("hasTexture", GLint(0));
        printf("Texture YOK - solid renk kullanılacak\n");
    }
    
    glBindVertexArray(vao);
    glDrawElements(GL_TRIANGLES, indexCount, GL_UNSIGNED_INT, nullptr);
    glBindVertexArray(0);
    
    // CLEANUP
    if(hasLoadedTexture) {
        glBindTexture(GL_TEXTURE_2D, 0);
    }
    
    shader.release();
}

void GLViewport::uploadAllMeshes(const aiScene *scene)
{
    if (!scene || scene->mNumMeshes == 0) {
        printf("Hata: Scene boş veya mesh yok\n");
        return;
    }

    // Önce bounding box hesapla (tüm mesh'ler için)
    calculateBoundingBoxForScene(scene);

    // Tüm mesh'lerin vertex ve index verilerini birleştir
    std::vector<float> verts;
    std::vector<unsigned> idx;
    unsigned vertexOffset = 0;

    printf("=== TÜM MESH'LER YÜKLENİYOR ===\n");
    printf("Toplam mesh sayısı: %d\n", scene->mNumMeshes);

    for (unsigned int mIdx = 0; mIdx < scene->mNumMeshes; ++mIdx) {
        const aiMesh *m = scene->mMeshes[mIdx];
        
        if (!m || m->mNumVertices == 0) {
            printf("Mesh %d boş, atlanıyor\n", mIdx);
            continue;
        }

        printf("Mesh %d: vertices=%d, faces=%d\n", mIdx, m->mNumVertices, m->mNumFaces);
        printf("  Texture coords: %s\n", m->mTextureCoords[0] ? "VAR" : "YOK");
        printf("  Normals: %s\n", m->mNormals ? "VAR" : "YOK");

        // UV koordinatlarını kontrol et (debug için)
        if (m->mTextureCoords[0]) {
            float minU = 1000, maxU = -1000, minV = 1000, maxV = -1000;
            for (unsigned i = 0; i < qMin(10u, m->mNumVertices); ++i) {
                float u = m->mTextureCoords[0][i].x;
                float v = m->mTextureCoords[0][i].y;
                minU = qMin(minU, u); maxU = qMax(maxU, u);
                minV = qMin(minV, v); maxV = qMax(maxV, v);
            }
            printf("  UV range: U(%.3f-%.3f), V(%.3f-%.3f)\n", minU, maxU, minV, maxV);
        }

        // Vertex data: position + texCoord + normal
        for (unsigned i = 0; i < m->mNumVertices; ++i) {
            // Position - Model merkezine göre normalize et
            verts.push_back(m->mVertices[i].x);
            verts.push_back(m->mVertices[i].y);
            verts.push_back(m->mVertices[i].z);

            // Texture coordinates - DÜZELTME
            if (m->mTextureCoords[0]) {
                float u = m->mTextureCoords[0][i].x;
                float v = m->mTextureCoords[0][i].y;
                
                // UV koordinatlarını normalize et (0-1 aralığına sığdır)
                u = qBound(0.0f, u, 1.0f);
                v = qBound(0.0f, v, 1.0f);
                
                // V koordinatını çevir (çoğu zaman bu gerekli oluyor)
                v = 1.0f - v;
                
                verts.push_back(u);
                verts.push_back(v);
            } else {
                // Default UV koordinatları
                verts.push_back(0.5f);  // Merkez
                verts.push_back(0.5f);
            }

            // Normals
            if (m->mNormals) {
                verts.push_back(m->mNormals[i].x);
                verts.push_back(m->mNormals[i].y);
                verts.push_back(m->mNormals[i].z);
            } else {
                // Default normal (yukarı doğru)
                verts.push_back(0.0f);
                verts.push_back(1.0f);
                verts.push_back(0.0f);
            }
        }

        // Index data - KONTROL EKLENDİ
        for (unsigned i = 0; i < m->mNumFaces; ++i) {
            const aiFace &face = m->mFaces[i];
            
            // Sadece üçgen face'leri kabul et
            if (face.mNumIndices != 3) {
                printf("Uyarı: Mesh %d, Face %d üçgen değil (%d vertex)\n", 
                       mIdx, i, face.mNumIndices);
                continue;
            }
            
            // Index'leri offset ile ekle
            idx.push_back(face.mIndices[0] + vertexOffset);
            idx.push_back(face.mIndices[1] + vertexOffset);
            idx.push_back(face.mIndices[2] + vertexOffset);
        }

        vertexOffset += m->mNumVertices;
        printf("Mesh %d tamamlandı, toplam vertex: %d\n", mIdx, vertexOffset);
    }

    if (verts.empty() || idx.empty()) {
        printf("Hata: Vertex veya index verisi yok!\n");
        return;
    }

    indexCount = static_cast<int>(idx.size());
    printf("=== UPLOAD SONUCU ===\n");
    printf("Toplam vertex sayısı: %d\n", static_cast<int>(verts.size() / 8));
    printf("Toplam index sayısı: %d\n", indexCount);
    printf("Toplam üçgen sayısı: %d\n", indexCount / 3);

    // OpenGL Buffer'larına yükle
    glBindVertexArray(vao);

    // Vertex buffer
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, verts.size() * sizeof(float), verts.data(), GL_STATIC_DRAW);

    // Vertex attribute'ları tanımla
    // Position attribute (location = 0): 3 float
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    // Texture coordinate attribute (location = 1): 2 float
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    // Normal attribute (location = 2): 3 float
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(5 * sizeof(float)));
    glEnableVertexAttribArray(2);

    // Element buffer (index buffer)
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, idx.size() * sizeof(unsigned), idx.data(), GL_STATIC_DRAW);

    // Unbind
    glBindVertexArray(0);

    printf("OpenGL buffer'ları başarıyla güncellendi\n");
    printf("========================\n");
}
/* ---------- Assimp loader ----------------------------------------------------- */
bool GLViewport::loadModel(const QString &filePath)
{
    makeCurrent();
    
    printf("Model yükleniyor: %s\n", filePath.toStdString().c_str());
    
    // Eski texture'ları temizle
    if (hasLoadedTexture && textureID > 0) {
        glDeleteTextures(1, &textureID);
        textureID = 0;
        hasLoadedTexture = false;
    }
    
    const aiScene *scene = importer.ReadFile(
        filePath.toStdString(),
        aiProcess_Triangulate |
        aiProcess_GenSmoothNormals |
        aiProcess_JoinIdenticalVertices |
        aiProcess_PreTransformVertices |
        aiProcess_FlipUVs); // UV'leri çevir - ÖNEMLİ!

    if(!scene || !scene->HasMeshes()) {
        printf("Model yükleme hatası: %s\n", importer.GetErrorString());
        doneCurrent();
        return false;
    }
    
    // Bounding box hesapla
    calculateBoundingBox(scene->mMeshes[0]);
    
    // Mesh'i yükle
    uploadAllMeshes(scene);
    
    // Texture'ları yükle - SIRALAMA ÖNEMLİ
    printf("=== TEXTURE YÜKLEME BAŞLADI ===\n");
    
    bool textureLoaded = false;
    
    // 1. Önce embedded texture'ları dene
    if (scene->mNumTextures > 0) {
        for (unsigned int i = 0; i < scene->mNumTextures; i++) {
            printf("Embedded texture %d deneniyor...\n", i);
            if (loadEmbeddedTexture(scene->mTextures[i])) {
                printf("Embedded texture %d başarılı!\n", i);
                textureLoaded = true;
                break;
            }
        }
    }
    
    // 2. Material texture referanslarını dene
    if (!textureLoaded && scene->HasMaterials()) {
        for (unsigned int i = 0; i < scene->mNumMaterials; i++) {
            aiMaterial *mat = scene->mMaterials[i];
            
            for (unsigned int j = 0; j < mat->GetTextureCount(aiTextureType_DIFFUSE); j++) {
                aiString texPath;
                if (mat->GetTexture(aiTextureType_DIFFUSE, j, &texPath) == AI_SUCCESS) {
                    printf("Material texture yolu: %s\n", texPath.C_Str());
                    
                    // Embedded texture referansı mı?
                    if (texPath.C_Str()[0] == '*') {
                        int texIndex = atoi(texPath.C_Str() + 1);
                        if (texIndex < (int)scene->mNumTextures) {
                            if (loadEmbeddedTexture(scene->mTextures[texIndex])) {
                                printf("Material embedded texture başarılı!\n");
                                textureLoaded = true;
                                break;
                            }
                        }
                    }
                    // Dış dosya texture'ı
                    else {
                        QString fullTexPath = QFileInfo(filePath).absolutePath() + "/" + QString(texPath.C_Str());
                        if (loadTexture(fullTexPath)) {
                            printf("Dış texture dosyası başarılı: %s\n", fullTexPath.toStdString().c_str());
                            textureLoaded = true;
                            break;
                        }
                    }
                }
            }
            if (textureLoaded) break;
        }
    }
    
    printf("=== TEXTURE YÜKLEME SONUCU ===\n");
    printf("Texture bulundu: %s\n", textureLoaded ? "EVET" : "HAYIR");
    printf("hasLoadedTexture: %s\n", hasLoadedTexture ? "TRUE" : "FALSE");
    printf("textureID: %d\n", textureID);
    printf("=============================\n");
    
    // Kamerayı otomatik ayarla
    resetCamera();
    
    doneCurrent();
    update(); // Sahneyi yeniden çiz
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
        QByteArray data((const char*)aiTex->pcData, aiTex->mWidth);
        if (!image.loadFromData(data)) {
            printf("Compressed texture yüklenemedi\n");
            return false;
        }
    }
    // Uncompressed texture (raw RGBA data)
    else {
        printf("Uncompressed texture yükleniyor: %dx%d\n", aiTex->mWidth, aiTex->mHeight);
        image = QImage((const uchar*)aiTex->pcData,
                       aiTex->mWidth, aiTex->mHeight,
                       QImage::Format_RGBA8888);
    }

    if (image.isNull()) {
        printf("Texture image oluşturulamadı\n");
        return false;
    }

    // OPENGL CONTEXT KONTROLÜ
    if (!context() || !context()->isValid()) {
        printf("OpenGL context geçersiz!\n");
        return false;
    }

    // Eski texture'ı temizle
    if (hasLoadedTexture && textureID > 0) {
        glDeleteTextures(1, &textureID);
        textureID = 0;
    }

    // QImage'ı OpenGL formatına çevir - Y eksenini çevir
    QImage glImage = image.convertToFormat(QImage::Format_RGBA8888).mirrored();

    // TEXTURE OLUŞTURMA ve YÜKLEME
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);

    // Texture verilerini yükle
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA,
                 glImage.width(), glImage.height(), 0,
                 GL_RGBA, GL_UNSIGNED_BYTE, glImage.bits());

    // Texture parametrelerini ayarla
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    // Mipmap oluştur (isteğe bağlı ama önerilir)
    glGenerateMipmap(GL_TEXTURE_2D);

    glBindTexture(GL_TEXTURE_2D, 0);

    // OpenGL hatalarını kontrol et
    GLenum error = glGetError();
    if (error != GL_NO_ERROR) {
        printf("OpenGL texture yükleme hatası: %d\n", error);
        glDeleteTextures(1, &textureID);
        textureID = 0;
        return false;
    }

    printf("Embedded texture başarıyla yüklendi: %dx%d, OpenGL ID: %d\n",
           glImage.width(), glImage.height(), textureID);

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

void GLViewport::checkTextureStatus()
{
    printf("=== TEXTURE STATUS DEBUG ===\n");
    printf("hasLoadedTexture: %s\n", hasLoadedTexture ? "TRUE" : "FALSE");
    printf("textureID: %d\n", textureID);

    if (textureID > 0) {
        glBindTexture(GL_TEXTURE_2D, textureID);

        GLint width, height;
        glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &width);
        glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_HEIGHT, &height);

        printf("Texture boyutu: %dx%d\n", width, height);

        GLint format;
        glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_INTERNAL_FORMAT, &format);
        printf("Texture format: %d\n", format);

        glBindTexture(GL_TEXTURE_2D, 0);
    }

    GLenum error = glGetError();
    if (error != GL_NO_ERROR) {
        printf("OpenGL hatası: %d\n", error);
    }
    printf("========================\n");
}


void GLViewport::calculateBoundingBoxForScene(const aiScene *scene)
{
    if (!scene || scene->mNumMeshes == 0) return;

    bool first = true;
    
    for (unsigned int mIdx = 0; mIdx < scene->mNumMeshes; ++mIdx) {
        const aiMesh *mesh = scene->mMeshes[mIdx];
        if (!mesh || mesh->mNumVertices == 0) continue;

        for (unsigned i = 0; i < mesh->mNumVertices; ++i) {
            QVector3D vertex(mesh->mVertices[i].x, mesh->mVertices[i].y, mesh->mVertices[i].z);
            
            if (first) {
                boundingMin = boundingMax = vertex;
                first = false;
            } else {
                boundingMin.setX(qMin(boundingMin.x(), vertex.x()));
                boundingMin.setY(qMin(boundingMin.y(), vertex.y()));
                boundingMin.setZ(qMin(boundingMin.z(), vertex.z()));
                
                boundingMax.setX(qMax(boundingMax.x(), vertex.x()));
                boundingMax.setY(qMax(boundingMax.y(), vertex.y()));
                boundingMax.setZ(qMax(boundingMax.z(), vertex.z()));
            }
        }
    }
    
    // Model merkezi ve yarıçapı
    modelCenter = (boundingMin + boundingMax) * 0.5f;
    QVector3D size = boundingMax - boundingMin;
    modelRadius = qMax(qMax(size.x(), size.y()), size.z()) * 0.6f;
    
    printf("Scene Bounding Box:\n");
    printf("  Center: (%.2f, %.2f, %.2f)\n", modelCenter.x(), modelCenter.y(), modelCenter.z());
    printf("  Radius: %.2f\n", modelRadius);
    printf("  Size: (%.2f, %.2f, %.2f)\n", size.x(), size.y(), size.z());
}