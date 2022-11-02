#### TI Board toolchain

set(CMAKE_SYSTEM_NAME "Linux")

set(CMAKE_C_COMPILER "/opt/aarch64-none-linux-gnu/bin/aarch64-none-linux-gnu-gcc")
set(CMAKE_CXX_COMPILER "/opt/aarch64-none-linux-gnu/bin/aarch64-none-linux-gnu-g++")

set(CMAKE_FIND_ROOT_PATH  "/opt/aarch64-none-linux-gnu/")

# DO NOT EDIT: F prime searches the host for programs, not the cross
# compile toolchain
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
# DO NOT EDIT: F prime searches for libs, includes, and packages in the
# toolchain when cross-compiling.
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)
