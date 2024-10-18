#!/bin/bash

TARGET_DIR="./target"
APP_DIR="${TARGET_DIR}/AppDir"
PREFIX="${APP_DIR}/usr"

rm -rf "${TARGET_DIR}"

qmake PREFIX="${PREFIX}"

make clean

make -j4
make install

cp -rv resources/* "${APP_DIR}/"

wget -c -P ./target/ "https://github.com/probonopd/linuxdeployqt/releases/download/continuous/linuxdeployqt-continuous-x86_64.AppImage"
chmod a+x ./target/linuxdeployqt*.AppImage

./target/linuxdeployqt*.AppImage "${APP_DIR}/usr/share/applications/DotaAutoQueue.desktop" -appimage -unsupported-bundle-everything -bundle-non-qt-libs -verbose=2 -extra-plugins=platforms,kf5,kf5/kwindowsystem/KF5WindowSystemX11Plugin.so
./target/linuxdeployqt*.AppImage "${APP_DIR}/usr/bin/DotaAutoQueue" -appimage -unsupported-bundle-everything -bundle-non-qt-libs -verbose=2 -extra-plugins=platforms,kf5,kf5/kwindowsystem/KF5WindowSystemX11Plugin.so
