cmake_minimum_required(VERSION 3.15)

project(stereo-heli)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(ASAN_BUILD "Build FSW with AddressSanitizer" OFF)
option(TSAN_BUILD "Build FSW with ThreadSanitizer" OFF)

enable_language(C CXX ASM)

if (ASAN_BUILD)
    add_compile_options(-fsanitize=address -fno-omit-frame-pointer -O1)
    add_link_options(-fsanitize=address)
elseif(TSAN_BUILD)
    add_compile_options(-fsanitize=thread -fno-omit-frame-pointer -O1)
    add_link_options(-fsanitize=thread)
endif()

add_compile_options(-pipe)

# Override default FPrime configuration
#set(FPRIME_CONFIG_DIR "${PROJECT_SOURCE_DIR}/Cfg")
#set(FPRIME_AC_CONSTANTS_FILE "${FPRIME_CONFIG_DIR}/AcConstants.ini")

##
# Required for FSW FPrime CMake configuration
##
set(FPRIME_FRAMEWORK_PATH ${PROJECT_SOURCE_DIR}/fprime)

# Set the locations to search for FPrime components
set(FPRIME_PROJECT_ROOT ${PROJECT_SOURCE_DIR})
set(FPRIME_INSTALL_DEST ${PROJECT_SOURCE_DIR})
set(FPRIME_LIBRARY_LOCATIONS
        ${FPRIME_FRAMEWORK_PATH}
        ${PROJECT_SOURCE_DIR}
        )

##
# FPrime instantiation
##
#set(CMAKE_DEBUG_OUTPUT TRUE)  # debug the CMake configuration
include(${FPRIME_FRAMEWORK_PATH}/cmake/FPrime.cmake)

# Set the framework libraries provided by FPrime
include(${FPRIME_FRAMEWORK_PATH}/cmake/FPrime-Code.cmake)

add_subdirectory(Stm32)
