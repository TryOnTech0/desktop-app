#include "DesktopViewer.h"
#include <QListWidgetItem>
#include <QMessageBox>
#include <QFileInfo>
#include <QDirIterator>

DesktopViewer::DesktopViewer(QWidget *parent)
    : QMainWindow(parent), viewport(new QOpenGLWidget(this)), modelList(new QListWidget(this)),
      btnRefresh(new QPushButton("Refresh", this)), currentScene(nullptr) {

    initializeUI();
}

DesktopViewer::~DesktopViewer() {
    clearScene();
}

bool DesktopViewer::loadModelList(const QString &directoryPath) {
   
    return true;
}

bool DesktopViewer::loadModel(const QString &modelFilePath) {
   
    return true;
}

void DesktopViewer::setMetadataFile(const QString &metadataPath) {
    metadataFile = metadataPath;
}

void DesktopViewer::onModelSelected(QListWidgetItem *item) {
    QString selectedModelPath = item->text();
    if (loadModel(selectedModelPath)) {
        renderModel();
        loadMetadata(selectedModelPath);
        emit modelLoaded(selectedModelPath);
    }
}

void DesktopViewer::renderModel() {
    if (!currentScene) {
        return;
    }
    // OpenGL rendering code would go here (Not implemented)
}

void DesktopViewer::loadMetadata(const QString &modelName) {
    if (metadataFile.isEmpty()) {
        emit errorOccurred("Metadata file not set.");
        return;
    }
    // Metadata loading code goes here (Not implemented)
}

void DesktopViewer::clearScene() {
    if (currentScene) {
        // Clear OpenGL buffers and reset state (Not implemented)
        currentScene = nullptr;
    }
}

void DesktopViewer::initializeUI() {
    setWindowTitle("Desktop Viewer");

    // Set up the layout and widgets (Not implemented)
    // Add the model list, viewport, and refresh button to the layout
}
