set(SOURCE_FILES
        ${CMAKE_CURRENT_LIST_DIR}/Fc.fpp
        ${CMAKE_CURRENT_LIST_DIR}/Fc.cpp
        ${CMAKE_CURRENT_LIST_DIR}/FcMsp.cpp
        ${CMAKE_CURRENT_LIST_DIR}/FcMsg.cpp
        ${CMAKE_CURRENT_LIST_DIR}/MspMessage.cpp
        ${CMAKE_CURRENT_LIST_DIR}/INAVMessages.cpp
        ${CMAKE_CURRENT_LIST_DIR}/crc.c
        )

register_fprime_module()
