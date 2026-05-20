@echo off
:: Create and enter build directory
if not exist build mkdir build
cd build

:: Configure with CMake using MinGW Makefiles (MSYS2)
cmake .. -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release
if %errorlevel% neq 0 (
    echo CMake configuration failed.
    exit /b 1
)

:: Build
cmake --build . --config Release
if %errorlevel% neq 0 (
    echo Build failed.
    exit /b 1
)

:: Copy exe to project root
copy TopSearch.exe ..\TopSearch.exe >nul
echo.
echo Build successful: TopSearch.exe
