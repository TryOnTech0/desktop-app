#ifndef DESKTOPVIEWER_H
#define DESKTOPVIEWER_H

#include <QMainWindow>
#include <QOpenGLWidget>
#include <QListWidget>
#include <QPushButton>
#include <QString>
#include <QDir>
#include <QListWidgetItem>
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
    void onRefreshClicked();
        // Burada, model listesini tekrar yüklemek için kod yazılacak
        // Örneğin:
        // loadModelList(currentDirectory); // Eğer model yükleme fonksiyonu yazıldıysa


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

    QOpenGLWidget *viewport;
    QListWidget *modelList;
    QPushButton *btnRefresh;

    QString currentDirectory;
    QString metadataFile;
    const aiScene *currentScene;
    Assimp::Importer importer;
    QString currentModelPath;
};

#endif // DESKTOPVIEWER_H
