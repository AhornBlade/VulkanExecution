add_library(VulkanExec_ext "GLFWWindow.cpp" )

target_link_libraries(VulkanExec_ext
	PUBLIC VulkanExec_core
	PRIVATE glfw)