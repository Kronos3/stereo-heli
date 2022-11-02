####
#
####
add_definitions(-DTGT_OS_TYPE_BARE_METAL)

add_compile_options(
        -mthumb
        -mcpu=cortex-m4
        -march=armv7e-m
        -mfloat-abi=hard -mfpu=fpv4-sp-d16
        -ffunction-sections
        -fdata-sections
        -g
        -gstrict-dwarf
        -Wall)

if (NOT DEFINED TI_BOARD)
    set(TI_BOARD msp432p401r)
endif()

message(STATUS "Building for TI-${TI_BOARD}")

set(TI_PATH ${CMAKE_CURRENT_LIST_DIR}/ti)
set(LINKER_SCRIPT ${TI_PATH}/${TI_BOARD}.lds)

string(TOUPPER ${TI_BOARD} BOARD_DEF)
add_compile_definitions(__${BOARD_DEF}__)

add_library(ti
        ${LINKER_SCRIPT}
        ${TI_PATH}/startup_${TI_BOARD}_gcc.c
        ${TI_PATH}/system_${TI_BOARD}.c
        ${TI_PATH}/syscalls.c)

add_link_options(
        -Wl,-T,${LINKER_SCRIPT}
        -march=armv7e-m
        -mthumb -mcpu=cortex-m4 -mfloat-abi=hard -mfpu=fpv4-sp-d16
        -static
        -Wl,--gc-sections
        --specs=nano.specs)

# Always link up the startup files
link_libraries(ti)

include_directories(SYSTEM
        "${TI_PATH}/include"
        "${TI_PATH}/ti/include/sys/CMSIS"
        "${TI_PATH}/ti/include/sys/msp"
        )
