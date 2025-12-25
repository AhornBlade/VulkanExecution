cmake_minimum_required(VERSION 3.24)

project(VulkanExecution VERSION 1.0)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED True)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/bin)

option(USE_VULKAN_DEFINES "Define Vulkan macros" ON)

if(USE_VULKAN_DEFINES)
	add_compile_definitions(VULKAN_HPP_NO_SPACESHIP_OPERATOR)
endif()

#third_party
add_subdirectory(third_party)

#tools
#add_subdirectory(tools)

#source
add_subdirectory(source)

#cmake script
#add_subdirectory(script)

#test
add_subdirectory(test)