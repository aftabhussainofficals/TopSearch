cmake_minimum_required(VERSION 3.16)
project(TopSearch VERSION 1.0 LANGUAGES CXX)

# Require C++17
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# MSYS2 UCRT64 prefix (adjust if your MSYS2 is installed elsewhere)
set(MSYS2_PREFIX "C:/msys64/ucrt64")

# Source files
add_executable(TopSearch
    main.cpp
    SearchEngine.cpp
    PlatformAPIs.cpp
)

# Include directories
target_include_directories(TopSearch PRIVATE
    ${MSYS2_PREFIX}/include
)

# Library search path
target_link_directories(TopSearch PRIVATE
    ${MSYS2_PREFIX}/lib
)

# Link against libcurl
target_link_libraries(TopSearch PRIVATE
    curl
)

# Windows: enable console subsystem
if(WIN32)
    target_link_options(TopSearch PRIVATE -mconsole)
endif()
