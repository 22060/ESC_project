cmake_minimum_required(VERSION 3.22)

#
# This file is generated only once,
# and is not re-generated if converter is called multiple times.
#
# User is free to modify the file as much as necessary
#

# Setup compiler settings
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)


# Define the build type
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Debug")
endif()

# Set the project name
set(CMAKE_PROJECT_NAME ESC_v3)

# Include toolchain file
include("cmake/gcc-arm-none-eabi.cmake")

# Enable compile command to ease indexing with e.g. clangd
set(CMAKE_EXPORT_COMPILE_COMMANDS TRUE)

# Core project settings
project(${CMAKE_PROJECT_NAME})
message("Build type: " ${CMAKE_BUILD_TYPE})

# Enable CMake support for ASM and C languages
enable_language(C ASM)

# Create an executable object type
add_executable(${CMAKE_PROJECT_NAME})

# Add STM32CubeMX generated sources
add_subdirectory(cmake/stm32cubemx)

# Link directories setup
target_link_directories(${CMAKE_PROJECT_NAME} PRIVATE
    # Add user defined library search paths
)

file(GLOB_RECURSE USER_SOURCES 
    "Core/Src/user/*.cpp" 
    "Core/Src/user/*.c"
)


# 2. 実行ファイルにソースを追加
target_sources(${CMAKE_PROJECT_NAME} PRIVATE
    ${USER_SOURCES}
    # 手動で Core/Src/user/user.cpp などを書く必要はなくなります
)

# 3. インクルードパスも user ディレクトリに通しておくと便利です
target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE
    Core/Inc/user
    Core/Src/user # .cppと同じ場所に.hを置く場合
)

# Add project symbols (macros)
target_compile_definitions(${CMAKE_PROJECT_NAME} PRIVATE
    # Add user defined symbols
)

# Add linked libraries
target_link_libraries(${CMAKE_PROJECT_NAME}
    stm32cubemx

    # Add user defined libraries
)
# target_link_options または target_link_libraries の後に追加
target_link_options(${CMAKE_PROJECT_NAME} PRIVATE
    -u _printf_float
)