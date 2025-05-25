QT += core gui widgets opengl openglwidgets   # << added openglwidgets

# (Qt-4 compatibility line can stay or be removed; harmless either way)
greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++17
LIBS    += -lassimp

# ------------------------------------------------------------------
# Source / header lists
# ------------------------------------------------------------------
SOURCES += \
    main.cpp \
    desktopviewer.cpp \
    glviewport.cpp          # << NEW

HEADERS += \
    desktopviewer.h \
    glviewport.h            # << NEW

FORMS += \
    desktopviewer.ui

# ------------------------------------------------------------------
# Default deployment rules (unchanged)
# ------------------------------------------------------------------
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

