cmake_minimum_required(VERSION 3.24)

project(VulkanExecution VERSION 1.0)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED True)

if(CMAKE_BUILD_TYPE MATCHES "Debug")
	add_definitions(-DVULKAN_EXECUTION_DEBUG)
endif()

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/bin)

find_package(Vulkan)
find_package(glfw3)
find_package(glm)

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