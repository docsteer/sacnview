#!/bin/bash
# Generate the Appimage

# Need qmake in path
export PATH=$QT_ROOT_DIR/bin:$PATH
cmake --build --preset "linux-release" --target install

linuxdeploy-x86_64.AppImage --appdir AppDir \
    --executable out/install/linux-x64-release/bin/sACNView \
    --desktop-file install/linux/sacnview.desktop \
    --icon-file install/linux/sacnview.png \
    --plugin qt \
    --output appimage