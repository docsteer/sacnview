#!/bin/bash
# Create a DMG for sACNView on macOS
#
# Expects an environment variable KEY_FILE to be set to the path of a .p8 file for signing the DMG.

set -u

APP_PATH="out/build/macos/Release/sACNView.app"
SIGN_IDENTITY="Developer ID Application: Thomas Steer (8JU6XWK784)"
ENTITLEMENTS="install/macos/Entitlements.plist"
NOTARIZE_PROFILE="sign-sacnview"

# Sign nested code from the inside out. `codesign --deep` is deprecated and
# unreliable for Qt app bundles: it can mis-sign or skip Qt's versioned
# framework symlinks and the loose plugin dylibs under Contents/PlugIns,
# which notarization and Gatekeeper increasingly reject.

# 1. Loose dylibs placed directly under Contents/Frameworks (not wrapped in a .framework).
find "$APP_PATH/Contents/Frameworks" -maxdepth 1 -type f \( -name "*.dylib" -o -name "*.so" \) -exec \
    codesign --force --verbose --timestamp --options runtime -s "$SIGN_IDENTITY" {} \;
RESULT=$?
if [ $RESULT -ne 0 ]; then
    echo "Error codesigning framework libraries"
    exit $RESULT
fi

# 2. Qt .framework bundles (signing the bundle handles its Versions/Current symlink structure).
find "$APP_PATH/Contents/Frameworks" -maxdepth 1 -type d -name "*.framework" -exec \
    codesign --force --verbose --timestamp --options runtime -s "$SIGN_IDENTITY" {} \;
RESULT=$?
if [ $RESULT -ne 0 ]; then
    echo "Error codesigning frameworks"
    exit $RESULT
fi

# 3. Qt plugins (platforms, imageformats, styles, ...), which are loose dylibs.
find "$APP_PATH/Contents/PlugIns" -type f \( -name "*.dylib" -o -name "*.so" \) -exec \
    codesign --force --verbose --timestamp --options runtime --entitlements "$ENTITLEMENTS" -s "$SIGN_IDENTITY" {} \;
RESULT=$?
if [ $RESULT -ne 0 ]; then
    echo "Error codesigning plugins"
    exit $RESULT
fi

# 4. Finally, the app bundle itself.
codesign --force --verbose --timestamp --entitlements "$ENTITLEMENTS" --options runtime -s "$SIGN_IDENTITY" "$APP_PATH"
RESULT=$?
if [ $RESULT -ne 0 ]; then
    echo "Error Codesigning sACNView.app"
    exit $RESULT
fi

# Sanity-check the result before spending time building/submitting a DMG that will fail.
codesign --verify --deep --strict --verbose=2 "$APP_PATH"
RESULT=$?
if [ $RESULT -ne 0 ]; then
    echo "Error verifying sACNView.app signature"
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
  --codesign "$SIGN_IDENTITY" \
  --overwrite \
  "sACNView.dmg" \
  "$APP_PATH"

RESULT=$?
if [ $RESULT -ne 0 ]; then
    echo "Error creating sACNView.dmg"
    exit $RESULT
fi

xcrun notarytool store-credentials "$NOTARIZE_PROFILE" --key "$KEY_FILE" --key-id 9884V35HM7 --issuer 69a6de7c-7b58-47e3-e053-5b8c7c11a4d1

RESULT=$?
if [ $RESULT -ne 0 ]; then
    echo "Error storing credentials for notarytool"
    exit $RESULT
fi

SUBMISSION_LOG=$(mktemp)
xcrun notarytool submit "sACNView.dmg" --keychain-profile "$NOTARIZE_PROFILE" --wait | tee "$SUBMISSION_LOG"
RESULT=${PIPESTATUS[0]}
if [ $RESULT -ne 0 ]; then
    echo "Error submitting sACNView.dmg for notarization"
    rm -f "$SUBMISSION_LOG"
    exit $RESULT
fi

if ! grep -q "status: Accepted" "$SUBMISSION_LOG"; then
    echo "Notarization was not accepted, fetching log..."
    SUBMISSION_ID=$(grep -o 'id: [a-f0-9-]*' "$SUBMISSION_LOG" | head -1 | cut -d' ' -f2)
    if [ -n "$SUBMISSION_ID" ]; then
        xcrun notarytool log "$SUBMISSION_ID" --keychain-profile "$NOTARIZE_PROFILE"
    fi
    rm -f "$SUBMISSION_LOG"
    exit 1
fi
rm -f "$SUBMISSION_LOG"

xcrun stapler staple "sACNView.dmg"
RESULT=$?
if [ $RESULT -ne 0 ]; then
    echo "Error stapling notarization ticket to sACNView.dmg"
    exit $RESULT
fi
