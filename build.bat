@echo off
g++ -std=c++17 -O2 ^
    main.cpp SearchEngine.cpp PlatformAPIs.cpp ^
    -I C:/msys64/ucrt64/include ^
    -L C:/msys64/ucrt64/lib ^
    -lcurl ^
    -o TopSearch.exe
if %errorlevel% == 0 (
    echo Build successful: TopSearch.exe
) else (
    echo Build failed.
)
