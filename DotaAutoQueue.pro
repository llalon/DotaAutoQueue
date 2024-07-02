QT += core gui network KWindowSystem widgets

CONFIG += c++17

CONFIG += link_pkgconfig
PKGCONFIG += opencv4
PKGCONFIG += tesseract
PKGCONFIG += x11 xtst

# CONFIG += console debug

# INCLUDEPATH += /usr/include/KF5/KWindowSystem
# LIBS += -lKF5WindowSystem
# INCLUDEPATH += /usr/include/X11
# LIBS += -lX11 -lXtst

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SRC_DIR = src

SOURCES += \
    $$SRC_DIR/main.cpp \
    $$SRC_DIR/mainwindow.cpp

HEADERS += \
    $$SRC_DIR/mainwindow.h

FORMS += \
    $$SRC_DIR/mainwindow.ui

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target
