add_subdirectory(slam)
set(MOD_DEPS slam)

set(SOURCE_FILES
        ${CMAKE_CURRENT_LIST_DIR}/Nav.fpp
        ${CMAKE_CURRENT_LIST_DIR}/Nav.cpp
        )

register_fprime_module()
