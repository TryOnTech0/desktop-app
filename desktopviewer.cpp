#include "desktopviewer.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QDir>

DesktopViewer::DesktopViewer(QWidget *parent)
    : QMainWindow(parent),
      viewport(new GLViewport(this)),
      modelList(new QListWidget(this)),
      btnRefresh(new QPushButton("Yenile", this))
{
    initializeUI();
}

DesktopViewer::~DesktopViewer()
{
    clearScene();
}

void DesktopViewer::initializeUI()
{
    QWidget *central = new QWidget(this);
    QHBoxLayout *main = new QHBoxLayout(central);
    
    // Kenar boşluklarını azalt
    main->setContentsMargins(5, 5, 5, 5);
    main->setSpacing(5);

    // Left panel
    QVBoxLayout *left = new QVBoxLayout;
    QLabel *labLeft = new QLabel("Model List", this);
    labLeft->setAlignment(Qt::AlignCenter);
    labLeft->setMaximumHeight(25);
    
    left->addWidget(labLeft);
    left->addWidget(modelList);
    left->addWidget(btnRefresh);
    left->setContentsMargins(0, 0, 0, 0);

    // Right panel
    QVBoxLayout *right = new QVBoxLayout;
    right->addWidget(viewport);
    right->setContentsMargins(0, 0, 0, 0);

    // Sol alanı 2 katına çıkar: 2'den 4'e
    main->addLayout(left, 4);   // Sol panel (4 birim - önceden 2'ydi)
    main->addLayout(right, 8);  // Sağ panel (8 birim aynı)

    setCentralWidget(central);
    setWindowTitle("Desktop Garment Viewer");

    connect(modelList,&QListWidget::itemClicked,this,&DesktopViewer::onModelSelected);
    connect(btnRefresh,&QPushButton::clicked,this,&DesktopViewer::onRefreshClicked);
}

void DesktopViewer::clearScene()
{
    viewport->update();
}

void DesktopViewer::onModelSelected(QListWidgetItem *item)
{
    const QString name = item->text();
    printf("Model seçildi: %s\n", name.toStdString().c_str());
    fflush(stdout);
    
    setWindowTitle("Seçilen Model: " + name);
    
    bool result = viewport->loadModel(name);
    printf("Model yükleme sonucu: %s\n", result ? "BAŞARILI" : "BAŞARISIZ");
    fflush(stdout);
}

void DesktopViewer::onRefreshClicked()
{
    printf("=== Yenile butonuna basıldı ===\n");
    fflush(stdout);
    
    modelList->clear();
    
    QDir dir(".");
    QStringList filters;
    filters << "*.obj" << "*.glb" << "*.fbx";
    
    QFileInfoList files = dir.entryInfoList(filters, QDir::Files);
    
    printf("Tarama dizini: %s\n", dir.absolutePath().toStdString().c_str());
    printf("Bulunan dosya sayısı: %lld\n", (long long)files.size());
    fflush(stdout);
    
    for(const QFileInfo &file : files) {
        printf("Dosya ekleniyor: %s (%.2f KB)\n", 
               file.fileName().toStdString().c_str(),
               file.size() / 1024.0);
        modelList->addItem(file.fileName());
        fflush(stdout);
    }
    
    if(modelList->count() == 0) {
        modelList->addItem("Hiç model dosyası bulunamadı");
        printf("Hiç model dosyası bulunamadı!\n");
        fflush(stdout);
    }
}
