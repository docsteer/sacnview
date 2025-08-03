:: Invoke signtool to sign the generated binary
:: Only if the SimplySign desktop tool is running (not in CI)
tasklist /fi "imagename eq SimplySignDesktop.exe" | findstr /B /I /C:"SimplySignDesktop.exe " >NUL

IF ERRORLEVEL 1 exit 0

signtool.exe sign /n "Open Source Developer, Thomas Steer" /fd SHA256 /tr http://timestamp.digicert.com /td SHA256 %1
