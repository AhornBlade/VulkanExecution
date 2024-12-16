add_executable(test_exec main.cpp)

target_link_libraries(test_exec
    PRIVATE VulkanExec_exec)