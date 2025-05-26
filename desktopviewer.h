#ifndef DESKTOPVIEWER_H
#define DESKTOPVIEWER_H
#include <QDir>
#include <QFileInfo>
#include <QCoreApplication>
#include <QMainWindow>
#include <QListWidget>
#include <QPushButton>
#include <assimp/scene.h>
#include <assimp/Importer.hpp>
#include "glviewport.h"

class DesktopViewer : public QMainWindow
{
    Q_OBJECT
public:
    explicit DesktopViewer(QWidget *parent = nullptr);
    ~DesktopViewer();

public slots:
    void onModelSelected(QListWidgetItem *item);
    void onRefreshClicked();

signals:
    void modelLoaded(const QString &modelName);
    void errorOccurred(const QString &errorMessage);

private:
    void initializeUI();
    void clearScene();

    GLViewport   *viewport;
    QListWidget  *modelList;
    QPushButton  *btnRefresh;

    // (unused yet)
    QString currentDirectory;
    const aiScene *currentScene = nullptr;
    Assimp::Importer importer;
};

#endif

