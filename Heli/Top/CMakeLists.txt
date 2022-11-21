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
        "${CMAKE_CURRENT_LIST_DIR}/HeliTopologyDefs.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/main.cc"
        )
# Note: supply non-explicit dependencies here
set(MOD_DEPS
        Drv/TcpClient
        Svc/LinuxTime
        )

set(EXECUTABLE_NAME heli)
register_fprime_executable()

target_link_libraries(${EXECUTABLE_NAME} PUBLIC
        # Work through dependencies
#        opencv_imgcodecs
#        opencv_imgproc
#        opencv_calib3d
#        opencv_core
        ${HELI_LIB_PATH}/libcamera.so.0.0.1
        ${HELI_LIB_PATH}/libcamera-base.so.0.0.1
        ${HELI_LIB_PATH}/libdrm.so
)

target_link_options(${EXECUTABLE_NAME} PUBLIC -Wl,--unresolved-symbols=ignore-all)
