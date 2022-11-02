#### TI Board toolchain

set(CMAKE_SYSTEM_NAME "ti")

set(CMAKE_C_COMPILER "/opt/arm-none-eabi/bin/arm-none-eabi-gcc")
set(CMAKE_CXX_COMPILER "/opt/arm-none-eabi/bin/arm-none-eabi-g++")

set(CMAKE_FIND_ROOT_PATH  "/opt/arm-none-eabi/")

# DO NOT EDIT: F prime searches the host for programs, not the cross
# compile toolchain
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
# DO NOT EDIT: F prime searches for libs, includes, and packages in the
# toolchain when cross-compiling.
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)

set(CMAKE_C_COMPILER_WORKS 1)
set(CMAKE_CXX_COMPILER_WORKS 1)
