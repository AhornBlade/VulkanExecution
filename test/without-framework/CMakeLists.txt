set(GLSLANG_VALIDATOR ${Vulkan_GLSLANG_VALIDATOR_EXECUTABLE})

set(SHADER_SRC_DIR "${CMAKE_CURRENT_SOURCE_DIR}")
set(SHADER_BIN_DIR "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")

file(GLOB_RECURSE GLSL_FILES 
    "${SHADER_SRC_DIR}/*.vert"
    "${SHADER_SRC_DIR}/*.frag"
    "${SHADER_SRC_DIR}/*.comp")

foreach(glsl_file ${GLSL_FILES})
    file(RELATIVE_PATH rel_path "${SHADER_SRC_DIR}" "${glsl_file}")
    set(out_file "${SHADER_BIN_DIR}/${rel_path}.spv")
    
    add_custom_command(
        OUTPUT "${out_file}"
        COMMAND "${GLSLANG_VALIDATOR}" -V "${glsl_file}" -o "${out_file}"
        DEPENDS "${glsl_file}"
        COMMENT "Compiling ${rel_path}"
    )
    list(APPEND SPV_OUTPUTS "${out_file}")
endforeach()

add_custom_target(shaders ALL DEPENDS ${SPV_OUTPUTS})

set(SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}")
set(TARGET_DIR "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")

file(GLOB_RECURSE RESOURCE_FILES 
    "${SOURCE_DIR}/*.obj"
    "${SOURCE_DIR}/*.png"
)

foreach(resource_file ${RESOURCE_FILES})
    file(RELATIVE_PATH relative_path "${SOURCE_DIR}" "${resource_file}")

    set(target_file "${TARGET_DIR}/${relative_path}")

    add_custom_command(
        OUTPUT "${target_file}"
        COMMAND ${CMAKE_COMMAND} -E make_directory "${TARGET_DIR}/${relative_path}/.."
        COMMAND ${CMAKE_COMMAND} -E copy "${resource_file}" "${target_file}"
        DEPENDS "${resource_file}"
        COMMENT "Copying ${relative_path}"
        VERBATIM
    )
    
    list(APPEND COPIED_FILES "${target_file}")
endforeach()

add_custom_target(copy_resources ALL
    DEPENDS ${COPIED_FILES}
    COMMENT "Copying all resources"
)

add_executable(without-framework main.cpp)
target_link_libraries(without-framework
    PRIVATE Vulkan::Headers
    PRIVATE stdexec
    PRIVATE glfw
    PRIVATE glm
    PRIVATE tinyobjloader
    PRIVATE stb)

add_dependencies(without-framework shaders copy_resources)