#include "desktopviewer.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>

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

    // left
    QVBoxLayout *left = new QVBoxLayout;
    QLabel *labLeft = new QLabel("Model List", this);
    labLeft->setAlignment(Qt::AlignCenter);
    left->addWidget(labLeft);
    left->addWidget(modelList);
    left->addWidget(btnRefresh);
    left->addStretch();

    // right
    QVBoxLayout *right = new QVBoxLayout;
    QLabel *labRight = new QLabel("Model Viewer", this);
    labRight->setAlignment(Qt::AlignCenter);
    right->addWidget(labRight);
    right->addWidget(viewport);

    main->addLayout(left,3);
    main->addLayout(right,7);

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
    const QString name=item->text();
    setWindowTitle("Seçilen Model: "+name);
    viewport->loadModel(name);
}

void DesktopViewer::onRefreshClicked()
{
    modelList->clear();
    modelList->addItem("example_cube.obj");
    modelList->addItem("sample_sweater.glb");
}

