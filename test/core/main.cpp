#include <core/Instance.hpp>
#include <core/Device.hpp>
#include <extension/GLFWWindow.hpp>

int main()
{
	vke::Feature feature{};
	feature.bUseWindow = true;

	vke::Instance instance{ feature, "test_core", 100 };
	vke::GLFWWindow window{ instance, 800, 600, "test_core"};
	
	std::vector<vke::DeviceQueueRequirement> queueRequirements
	{
		{vk::QueueFlagBits::eGraphics, {1.0f, 1.0f, 1.0f}},
		{vk::QueueFlagBits::eCompute, {0.5f, 0.5f}},
		{vk::QueueFlagBits::eTransfer, {1.0f}},
	};

	vke::Device device{instance, feature, queueRequirements};
}