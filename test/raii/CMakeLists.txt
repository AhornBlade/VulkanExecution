add_executable(test_raii main.cpp)

target_link_libraries(test_raii
	PUBLIC Vulkan::Headers
    PUBLIC glfw)