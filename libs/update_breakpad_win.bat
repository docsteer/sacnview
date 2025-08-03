@echo off

cd %~dp0

if exist breakpad (
  goto update_gclient
)

:install_breakpad
echo Installing Breakpad for the first time

mkdir breakpad
pushd breakpad
call fetch.bat breakpad

if errorlevel 1 (
  echo Working around breakpad bug on Windows
  python3.bat src\src\tools\python\deps-to-manifest.py src/DEPS src/default.xml
)

popd

:update_gclient
pushd breakpad
call gclient.bat sync
popd

EXIT /B
