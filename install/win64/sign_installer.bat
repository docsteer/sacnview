:: Invoke signtool to sign the generated binary

signtool.exe sign /n "Open Source Developer, Thomas Steer" /fd SHA256 /tr http://timestamp.digicert.com /td SHA256 %1
