set(SOURCE_FILES
        ${CMAKE_CURRENT_LIST_DIR}/Cam.fpp
        ${CMAKE_CURRENT_LIST_DIR}/Cam.cpp
        ${CMAKE_CURRENT_LIST_DIR}/CamBuffer.cpp
        ${CMAKE_CURRENT_LIST_DIR}/CamFrame.cpp
        ${CMAKE_CURRENT_LIST_DIR}/core/libcamera_app.cc
        )

#add_fprime_subdirectory("${CMAKE_CURRENT_LIST_DIR}/Ports")

register_fprime_module()
