add_executable(test_base main.cpp)
target_link_libraries(test_base
    PRIVATE vulkan-execution
    PRIVATE vulkan-execution-extension
    PRIVATE glm)