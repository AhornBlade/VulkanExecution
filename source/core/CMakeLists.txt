add_library(VulkanExec_core "Instance.cpp" )

target_link_libraries(VulkanExec_core
	PUBLIC Vulkan::Headers)