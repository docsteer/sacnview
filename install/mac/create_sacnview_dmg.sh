#!/bin/bash

# In CI environment, skip the finder prettifying as that requires user interaction
# https://github.com/create-dmg/create-dmg/issues/72

if [ -n $CI ]
then
create-dmg --volname "sACNView_Installer" \
    --volicon "res/icon.icns" \
    --background "res/mac_install_bg.png" \
    --window-pos 200 120 \
    --window-size 800 400 \
    --icon-size 100 \
    --icon "sACNView.app" 200 190 \
    --hide-extension "sACNView.app" \
    --app-drop-link 600 185 \
    sACNView.dmg \
    out/build/macos-release/Release/
else
create-dmg --volname "sACNView_Installer" \
    --volicon "res/icon.icns" \
    --background "res/mac_install_bg.png" \
    --window-pos 200 120 \
    --window-size 800 400 \
    --icon-size 100 \
    --icon "sACNView.app" 200 190 \
    --hide-extension "sACNView.app" \
    --app-drop-link 600 185 \
    --skip-jenkins \
    sACNView.dmg \
    out/build/macos-release/Release/
fi
