cmake_minimum_required(VERSION 3.15)

project(stereo-heli)
enable_language(C CXX)

find_package(Vulkan REQUIRED
        COMPONENTS glsc
        FATAL_ERROR)

function(BuildShader TG SHADER)
    get_filename_component(FILENAME ${SHADER} NAME)
    add_custom_target(
            ${TG}
            COMMAND /opt/glslc/bin/glslc ${SHADER} -o ${CMAKE_CURRENT_BINARY_DIR}/${FILENAME}.spv
            COMMENT "Compiling SPIR-V object ${FILENAME}.spv"
            BYPRODUCTS ${CMAKE_CURRENT_BINARY_DIR}/${SHADER}.spv
            DEPENDS ${shader})
endfunction()

set(CXX_STANDARD 17)

BuildShader(test_comp ${CMAKE_CURRENT_LIST_DIR}/tests/glsl.comp)

add_executable(test_basic tests/basic.cpp)
target_link_libraries(test_basic glfw vulkan dl pthread)
add_dependencies(test_basic test_comp)
