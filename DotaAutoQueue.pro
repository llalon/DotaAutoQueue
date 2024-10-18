
QT += core gui network KWindowSystem widgets

CONFIG += c++17

CONFIG += link_pkgconfig
PKGCONFIG += opencv4
PKGCONFIG += tesseract
PKGCONFIG += x11 xtst

SRC_DIR = src

SOURCES += \
    $$SRC_DIR/main.cpp \
    $$SRC_DIR/mainwindow.cpp

HEADERS += \
    $$SRC_DIR/mainwindow.h

FORMS += \
    $$SRC_DIR/mainwindow.ui

isEmpty(PREFIX) {
    PREFIX = /usr/local
}

target.path = $$PREFIX/bin
INSTALLS += target

desktop.path  = $$PREFIX/share/applications
desktop.files = resources/DotaAutoQueue.desktop
icons.path    = $$PREFIX/share/icons/hicolor/48x48/apps/
icons.files   = resources/DotaAutoQueue.png
INSTALLS     += desktop icons

OBJECTS_DIR = target/obj
MOC_DIR = target/moc
RCC_DIR = target/rcc
UI_DIR = target/ui
TARGET = target/DotaAutoQueue

GIT_HASH = $$system(git rev-parse --short HEAD)
DEFINES += GIT_HASH=\\\"$$GIT_HASH\\\"

RESOURCES += \
    resources.qrc
