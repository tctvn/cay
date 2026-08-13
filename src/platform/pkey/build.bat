@echo off
echo ========================================
echo Building Raspberry Pi Pico Firmware
echo ========================================

if not exist build (
    mkdir build
)

cd build
cmake ..
cmake --build .

cd ..
echo.
echo Build complete! Firmware files (.uf2) are in the build\ directory.
