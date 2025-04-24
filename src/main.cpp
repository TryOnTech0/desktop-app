#include <QApplication>
#include <QFileDialog>
#include <QMessageBox>
#include "DesktopViewer.h"

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);

    // Ana pencereyi oluştur
    DesktopViewer viewer;

    // Metadata dosyasını ayarla (örneğin, JSON/XML dosyası)
    QString metadataFilePath = QFileDialog::getOpenFileName(nullptr, "Open Metadata File", "", "JSON Files (*.json);;XML Files (*.xml)");
    if (!metadataFilePath.isEmpty()) {
        viewer.setMetadataFile(metadataFilePath);
    } else {
        QMessageBox::warning(nullptr, "Warning", "No metadata file selected.");
    }

    // 3D model dosyalarını içeren dizini seç
    QString directoryPath = QFileDialog::getExistingDirectory(nullptr, "Select Directory Containing 3D Models", "");
    if (!directoryPath.isEmpty()) {
        if (!viewer.loadModelList(directoryPath)) {
            QMessageBox::warning(nullptr, "Warning", "No models found in the selected directory.");
        }
    } else {
        QMessageBox::warning(nullptr, "Warning", "No directory selected.");
    }

    // Uygulamayı başlat
    viewer.show();

    return app.exec();
}
