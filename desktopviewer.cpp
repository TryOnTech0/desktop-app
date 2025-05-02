#include "desktopviewer.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>

DesktopViewer::DesktopViewer(QWidget *parent)
    : QMainWindow(parent),
    viewport(new QOpenGLWidget(this)),
    modelList(new QListWidget(this)),
    btnRefresh(new QPushButton("Yenile", this)),
    currentScene(nullptr)
{
    initializeUI();
}

DesktopViewer::~DesktopViewer() {
    // Qt pointer'ları kendisi sildiği için delete gerekmez
    clearScene();
}



void DesktopViewer::initializeUI() {
    // Merkezi widget ayarla
    QWidget *centralWidget = new QWidget(this);
    QHBoxLayout *mainLayout = new QHBoxLayout(centralWidget);

    // Sol kısım: Model listesi ve yenile butonu
    QVBoxLayout *leftLayout = new QVBoxLayout();
    QLabel *labelLeft = new QLabel("Model List", this);
    labelLeft->setAlignment(Qt::AlignCenter);
    modelList->setSelectionMode(QAbstractItemView::SingleSelection);  // Kullanıcı tek bir model seçebilsin
    btnRefresh->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);  // Buton boyutlarını ayarla

    leftLayout->addWidget(labelLeft);
    leftLayout->addWidget(modelList);
    leftLayout->addWidget(btnRefresh);
    leftLayout->addStretch(1); // Buton ve liste arasında boşluk bırak

    // Sağ kısım: OpenGL viewport
    QVBoxLayout *rightLayout = new QVBoxLayout();
    QLabel *labelRight = new QLabel("Model Viewer", this);
    labelRight->setAlignment(Qt::AlignCenter);
    rightLayout->addWidget(labelRight);
    rightLayout->addWidget(viewport);

    // Ana layout'a yerleştir
    mainLayout->addLayout(leftLayout);
    mainLayout->addLayout(rightLayout);

    // Merkezi widget'ı ayarla
    setCentralWidget(centralWidget);
    setWindowTitle("Desktop Garment Viewer");

    // Buton ve model listesi için slot bağlantıları
    connect(modelList, &QListWidget::itemClicked, this, &DesktopViewer::onModelSelected);
    connect(btnRefresh, &QPushButton::clicked, this, &DesktopViewer::onRefreshClicked);
}

void DesktopViewer::onModelSelected(QListWidgetItem *item) {
    clearScene();
    QString modelName = item->text();
    this->setWindowTitle("Seçilen Model: " + modelName);
    // Buraya ileride: loadModel() ve renderModel() çağrıları eklenecek
}

void DesktopViewer::clearScene() {
    // Sahne sıfırlama işlemleri (OpenGL temizlik kodları buraya yazılacak)
    viewport->update();
}

void DesktopViewer::onRefreshClicked() {
    // Model listesini tekrar yükle
    if (!loadModelList(currentDirectory)) {
        emit errorOccurred("Model listesi yüklenemedi.");
    } else {
        emit modelLoaded("Model listesi başarıyla yüklendi.");
    }
}

// Aşağıdakiler sadece declare edildi ama implemente edilmedi (sen sorumlu değilsin)

bool DesktopViewer::loadModelList(const QString &directoryPath) {
    return false;
}

bool DesktopViewer::loadModel(const QString &modelFilePath) {
    return false;
}

void DesktopViewer::setMetadataFile(const QString &metadataPath) {}

void DesktopViewer::renderModel() {}

void DesktopViewer::loadMetadata(const QString &modelName) {}


