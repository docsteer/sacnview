#!/usr/bin/env sh
# Adapted from https://www.update.rocks/blog/osx-signing-with-travis/
# Uses a base64 encoded certificate in an environment variable to set up signing
# Exits if the variable isn't set

KEY_CHAIN=buildtest2.keychain
CERTIFICATE_P12=certificate.p12

# Check for the variable
if [ -z ${CERTIFICATE_OSX_P12+x} ]; then
 echo "No macos signing setup - output will be unsigned"
 exit
else
 echo "macos signing is setup - output will be signed"
fi

# Recreate the certificate from the secure environment variable
echo $CERTIFICATE_OSX_P12 | base64 --decode > $CERTIFICATE_P12

#create a keychain
security create-keychain -p travis $KEY_CHAIN

# Make the keychain the default so identities are found
security default-keychain -s $KEY_CHAIN

# Unlock the keychain
security unlock-keychain -p travis $KEY_CHAIN

# Set keychain locking timeout to 1 hour
security set-keychain-settings -t 3600 -u $KEY_CHAIN

security import $CERTIFICATE_P12 -k $KEY_CHAIN -P $CERTIFICATE_PASSWORD -T /usr/bin/codesign;

security set-key-partition-list -S apple-tool:,apple: -s -k travis $KEY_CHAIN

# remove certs
rm -fr *.p12
