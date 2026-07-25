#!/bin/bash
# Create a DMG for sACNView on macOS

create-dmg \
  --volname "sACNView" \
  --volicon "res/icon.icns" \
  --background "res/mac_install_bg.png" \
  --window-pos 200 120 \
  --window-size 800 400 \
  --icon-size 100 \
  --icon "sACNView.app" 200 190 \
  --hide-extension "sACNView.app" \
  --app-drop-link 600 185 \
  --codesign "Apple Distribution: Carallon ltd (7J5M6EPN5V)" \
  "sACNView.dmg" \
  "out/build/macos/Release/sACNView.app"
