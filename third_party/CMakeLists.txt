#vulkan
if (Vulkan_FOUND)
    message(STATUS "Found Vulkan")
    message(STATUS "Vulkan Include Directories: ${Vulkan_INCLUDE_DIRS}")
    message(STATUS "Vulkan Libraries: ${Vulkan_LIBRARIES}")
else()
	add_subdirectory(Vulkan-Headers)
endif()

#glfw
if (glfw3_FOUND)
    message(STATUS "Found GLFW")
    message(STATUS "GLFW Include Directories: ${glfw3_INCLUDE_DIRS}")
    message(STATUS "GLFW Libraries: ${glfw3_LIBRARIES}")
else()
	add_subdirectory(glfw)
endif()

#glm
if (glm_FOUND)
    message(STATUS "Found glm")
    message(STATUS "glm Include Directories: ${glm_INCLUDE_DIRS}")
else()
    add_subdirectory(glm)
endif()

#stdexec
add_library(stdexec INTERFACE)
target_include_directories(stdexec
	INTERFACE std-execution)