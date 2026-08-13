@echo off
echo Killing existing cayhelper instances...
taskkill /F /IM cayhelper.exe >nul 2>&1
echo Building Cay Helper...
cd build
cmake --build . --config Release
cd ..
echo Done!
