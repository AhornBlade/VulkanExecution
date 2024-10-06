add_subdirectory(core)
add_subdirectory(extension)

add_library(VulkanExec INTERFACE)

target_link_libraries(VulkanExec
	INTERFACE VulkanExec_core
	INTERFACE VulkanExec_ext)
target_include_directories(VulkanExec
	INTERFACE .)