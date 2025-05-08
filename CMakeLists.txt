cmake_minimum_required(VERSION 3.28.1)

#
# This file is generated only once,
# and is not re-generated if converter is called multiple times.
#
# User is free to modify the file as much as necessary
#



# Setup compiler settings
set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS ON)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS ON)

# Define the build type
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Debug")
endif()

# Set the project name
set(CMAKE_PROJECT_NAME Piano_H7)

# Include toolchain file
include("cmake/gcc-arm-none-eabi.cmake")

# set(CMAKE_C_FLAGS_RELEASE "-Wall -Wextra -Wpedantic")
# set(CMAKE_CXX_FLAGS_RELEASE "-Wall -Wextra -Wpedantic")

set(FLaGSGS "-fno-rtti -fno-exeptions -fno-threadsafe-statics -fdata-sections -ffunction-sections --specs=nano.specs -mfpu=fpv5-d16 -mfloat-abi=hard -mthumb -fno-cxa-atexit -fno_strict_aliasing -flto")
set(TARGET_FLAGS FLaGSGS)
set(CMAKE_C_FLAGS_RELEASE FLaGSGS)
set(CMAKE_CXX_FLAGS_RELEASE FLaGSGS)
set(CMAKE_C_LINK_FLAGS FLaGSGS)
set(CMAKE_CXX_LINK_FLAGS FLaGSGS)

# tinyUSB
include("tinyusb/src/CMakeLists.txt")

# Enable compile command to ease indexing with e.g. clangd
set(CMAKE_EXPORT_COMPILE_COMMANDS TRUE)

# Core project settings
project(${CMAKE_PROJECT_NAME})

message("Build type: " ${CMAKE_BUILD_TYPE})

# Enable CMake support for ASM and C languages
# enable_language(C ASM)
enable_language(C CXX ASM)

# Create an executable object type
add_executable(${CMAKE_PROJECT_NAME})

# Add STM32CubeMX generated sources
add_subdirectory(
    cmake/stm32cubemx
)
# Add linked libraries
target_link_libraries(${CMAKE_PROJECT_NAME}
    stm32cubemx
    # Add user defined libraries
)

# Link directories setup
target_link_directories(${CMAKE_PROJECT_NAME} PRIVATE
    # Add user defined library search paths
)

# tinyUSB
tinyusb_target_add(${CMAKE_PROJECT_NAME})

# Add sources to executable
target_sources(${CMAKE_PROJECT_NAME} PRIVATE
    # Add user sources here
    Piano_H7/piano_h7.cpp
    Piano_H7/ft6336.cpp
    Piano_H7/st7796.cpp
    Piano_H7/ui/ui.c
    Piano_H7/ui/screens.c
    Piano_H7/ui/images.c
    Piano_H7/ui/styles.c
    # tinyUSB
    Piano_H7/usb_descriptors.c
    tinyusb/src/tusb.c
    tinyusb/src/portable/synopsys/dwc2/dcd_dwc2.c
    tinyusb/src/portable/synopsys/dwc2/dwc2_common.c
)

# Add include paths
target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE
    # Add user defined include paths
    Piano_H7
    Piano_H7/ui
    # tinyUSB
    tinyusb/src
    tinyusb/hw/bsp
)

# Add project symbols (macros)
target_compile_definitions(${CMAKE_PROJECT_NAME} PRIVATE
    # Add user defined symbols
)

# LVGL
#######################################################
add_subdirectory(
    Piano_H7/lvgl
)

target_link_libraries(${CMAKE_PROJECT_NAME}
    lvgl::lvgl
)
