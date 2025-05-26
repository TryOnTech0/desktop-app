#include <QApplication>
#include "desktopviewer.h"

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);

    DesktopViewer viewer;
    viewer.resize(1000, 600);
    viewer.show();

    return app.exec();
}

