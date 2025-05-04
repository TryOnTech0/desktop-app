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
    // Merkezi widget
    QWidget *centralWidget = new QWidget(this);
    QHBoxLayout *mainLayout = new QHBoxLayout(centralWidget);

    // Sol panel: Model Listesi
    QVBoxLayout *leftLayout = new QVBoxLayout();
    QLabel *labelLeft = new QLabel("Model List", this);
    labelLeft->setAlignment(Qt::AlignCenter);
    modelList->setSelectionMode(QAbstractItemView::SingleSelection);
    modelList->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    btnRefresh->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

    leftLayout->addWidget(labelLeft);
    leftLayout->addWidget(modelList);
    leftLayout->addWidget(btnRefresh);
    leftLayout->addStretch();

    // Sağ panel: OpenGL Model Görüntüleyici
    QVBoxLayout *rightLayout = new QVBoxLayout();
    QLabel *labelRight = new QLabel("Model Viewer", this);
    labelRight->setAlignment(Qt::AlignCenter);
    viewport->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    rightLayout->addWidget(labelRight);
    rightLayout->addWidget(viewport);

    // Ana layout'a ekle
    mainLayout->addLayout(leftLayout);
    mainLayout->addLayout(rightLayout);

    // Stretch oranlarını layoutlar eklendikten sonra ayarla
    mainLayout->setStretch(0, 3); // Sol taraf %30
    mainLayout->setStretch(1, 7); // Sağ taraf %70

    // Merkezi widget'ı ayarla
    setCentralWidget(centralWidget);
    setWindowTitle("Desktop Garment Viewer");

    // Slot bağlantıları
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


