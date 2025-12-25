add_library(vulkan-execution-base
    "base/Base.cpp" 
    "base/Window.cpp"
    "base/Synchronization.cpp"
    "base/Memory.cpp"
    "base/Resources.cpp")
target_link_libraries(vulkan-execution-base
    PUBLIC Vulkan::Headers)
target_include_directories(vulkan-execution-base
    INTERFACE .)

add_library(vulkan-execution-exec
    "exec/Scheduler.cpp")
target_link_libraries(vulkan-execution-exec
    PUBLIC Vulkan::Headers
    PUBLIC stdexec)
target_include_directories(vulkan-execution-exec
    INTERFACE .)

add_library(vulkan-execution INTERFACE)
target_link_libraries(vulkan-execution
    INTERFACE vulkan-execution-base)

add_library(vulkan-execution-extension
    "extension/glfw.cpp")
target_link_libraries(vulkan-execution-extension
    PUBLIC vulkan-execution
    PRIVATE glfw)