# DotaAutoQueue

#### About

Tired of waiting 15+ minutes to find a game of single draft in Dota 2? Wish you could go do chores while queueing instead of sitting in front of your steam deck wasting time?

Well now you can with DotaAutoQueue. Queue for games unattended and recieve push notifications to your phone when a game is found and accepted!

#### Installation

Download the AppImage from [releases](https://github.com/llalon/DotaAutoQueue/releases/) or build from source:

```
sudo pacman -S qt5-base \
    qt5-x11extras \
    kwindowsystem \
    libx11 \
    libxtst \
    libxi \
    opencv \
    tesseract \
    gcc \
    pkgconf \
    fmt \
    vtk \
    hdf5

qmake
make
make install
```
