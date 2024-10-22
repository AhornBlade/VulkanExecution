add_library(VulkanExec_core 
	"Instance.cpp"  
	"Device.cpp"  
	"Feature.cpp"
	"Queue.cpp")

target_link_libraries(VulkanExec_core
	PUBLIC Vulkan::Headers)