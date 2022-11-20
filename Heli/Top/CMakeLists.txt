####
# F prime CMakeLists.txt:
#
# SOURCE_FILES: combined list of source and autocoding diles
# MOD_DEPS: (optional) module dependencies
#
# Note: using PROJECT_NAME as EXECUTABLE_NAME
####
set(SOURCE_FILES
        "${CMAKE_CURRENT_LIST_DIR}/topology.fpp"
        "${CMAKE_CURRENT_LIST_DIR}/instances.fpp"
        "${CMAKE_CURRENT_LIST_DIR}/RpiTopologyDefs.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/main.cc"
        )
# Note: supply non-explicit dependencies here
set(MOD_DEPS
        Drv/TcpClient
        Svc/LinuxTime
        )

set(EXECUTABLE_NAME rpi)
register_fprime_executable()

set(OPENCV_LIBRARIES
        ${PROJECT_SOURCE_DIR}/opencv/libopencv_imgcodecs.a
        ${PROJECT_SOURCE_DIR}/opencv/libopencv_imgproc.a
        ${PROJECT_SOURCE_DIR}/opencv/libopencv_calib3d.a
        ${PROJECT_SOURCE_DIR}/opencv/libopencv_flann.a
        ${PROJECT_SOURCE_DIR}/opencv/libopencv_features2d.a
        ${PROJECT_SOURCE_DIR}/opencv/libopencv_core.a
        ${PROJECT_SOURCE_DIR}/opencv/opencv4/3rdparty/libzlib.a
        )

target_link_libraries(rpi PUBLIC
        # Work through dependencies
#        ${OPENCV_LIBRARIES}
#        ${OPENCV_LIBRARIES}
#        opencv_imgcodecs
#        opencv_imgproc
#        opencv_calib3d
#        opencv_core
#        camera
#        camera-base
)

target_link_options(rpi PUBLIC -Wl,--unresolved-symbols=ignore-in-object-files)
