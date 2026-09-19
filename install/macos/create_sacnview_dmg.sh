#!/bin/bash
# Create a DMG for sACNView on macOS
#
# Expects an environment variable KEY_FILE to be set to the path of a .p8 file for signing the DMG.

codesign --force --deep --verbose --timestamp --entitlements install/macos/Entitlements.plist --options runtime -s "Developer ID Application: Thomas Steer (8JU6XWK784)" out/build/macos/Release/sACNView.app

RESULT=$?
if [ $RESULT -ne 0 ]; then
    echo "Error Codesigning sACNView.app"
    exit $RESULT
fi

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
  --codesign "Apple Distribution: Thomas Steer (8JU6XWK784)" \
  --overwrite \
  "sACNView.dmg" \
  "out/build/macos/Release/sACNView.app"

RESULT=$?
if [ $RESULT -ne 0 ]; then
    echo "Error creating sACNView.dmg"
    exit $RESULT
fi

xcrun notarytool store-credentials "sign-sacnview" --key $KEY_FILE --key-id 9884V35HM7 --issuer 69a6de7c-7b58-47e3-e053-5b8c7c11a4d1

RESULT=$?
if [ $RESULT -ne 0 ]; then
    echo "Error storing credentials for notarytool"
    exit $RESULT
fi

xcrun notarytool submit "sACNView.dmg" --keychain-profile "sign-sacnview" --wait

xcrun stapler staple "sACNView.dmg"
