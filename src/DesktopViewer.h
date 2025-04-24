#ifndef DESKTOPVIEWER_H
#define DESKTOPVIEWER_H

#include <QMainWindow>
#include <QOpenGLWidget>
#include <QListWidget>
#include <QPushButton>
#include <QString>
#include <QDir>
#include <assimp/scene.h>
#include <assimp/Importer.hpp>

class DesktopViewer : public QMainWindow {
    Q_OBJECT

public:
    explicit DesktopViewer(QWidget *parent = nullptr);
    ~DesktopViewer();
    
    bool loadModelList(const QString &directoryPath);
    bool loadModel(const QString &modelFilePath);
    void setMetadataFile(const QString &metadataPath);

public slots:
    void onModelSelected(QListWidgetItem *item);

signals:
    void modelLoaded(const QString &modelName);
    void errorOccurred(const QString &errorMessage);

private:
    void initializeUI();
    void renderModel();
    void loadMetadata(const QString &modelName);
    void clearScene();

    QOpenGLWidget *viewport;            // OpenGL widget for rendering
    QListWidget *modelList;             // List of 3D models to choose from
    QPushButton *btnRefresh;            // Refresh button to reload models
    QString currentDirectory;          // Current directory containing models
    QString metadataFile;              // Path to metadata file
    const aiScene *currentScene;       // Current loaded model scene
    Assimp::Importer importer;         // Assimp importer to load 3D models
    QString currentModelPath;          // Path to the current model being viewed
};

#endif // DESKTOPVIEWER_H
