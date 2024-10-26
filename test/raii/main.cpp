#include <vulkan/vulkan_raii.hpp>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <iostream>
#include <vector>

struct QueueFamilyInfo
{
	uint32_t queueFamilyIndex;
	std::vector<float> priorities;
};

class Window
{
public:
	using GLFWwindowPTR = std::unique_ptr<GLFWwindow, decltype([](GLFWwindow* w) { glfwDestroyWindow(w); })>;

	explicit Window() = default;
	Window(int width, int height, std::string_view title)
	{        
		glfwInit();
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

		window = GLFWwindowPTR{glfwCreateWindow(width, height, title.data(), nullptr, nullptr)};
        glfwSetWindowUserPointer(window.get(), this);
        glfwSetFramebufferSizeCallback(window.get(), [](GLFWwindow* window, int width, int height) 
		{
			auto app = reinterpret_cast<Window*>(glfwGetWindowUserPointer(window));
			app->framebufferResized = true;
		});
	}

	vk::raii::SurfaceKHR createSurface(const vk::raii::Instance& instance)
	{
		VkSurfaceKHR surface = VK_NULL_HANDLE;
		glfwCreateWindowSurface(*instance, window.get(), nullptr, &surface);
		return vk::raii::SurfaceKHR{ instance, surface };
	}

private:
	GLFWwindowPTR window;
	bool framebufferResized = false;
};

class Renderer
{
public:
	Renderer():instance{ getInstance() }, 
		debugMessenger{instance, getDebugUtilsCreateInfo()},
		window{800, 600, "test_raii"},
		surface{window.createSurface(instance)},
		physicalDevice{instance.enumeratePhysicalDevices()[1]},
		device{getDevice()}
	{

	}

private:
	vk::raii::Context context;
	vk::raii::Instance instance;
	vk::raii::DebugUtilsMessengerEXT debugMessenger;
	Window window;
	vk::raii::SurfaceKHR surface;
	vk::raii::PhysicalDevice physicalDevice;
	vk::raii::Device device;

	vk::DebugUtilsMessengerCreateInfoEXT getDebugUtilsCreateInfo() const
	{
		vk::DebugUtilsMessengerCreateInfoEXT debugInfo{};
		debugInfo.setMessageSeverity(vk::DebugUtilsMessageSeverityFlagBitsEXT::eError |
			vk::DebugUtilsMessageSeverityFlagBitsEXT::eInfo |
			vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning );
		debugInfo.setMessageType(vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral |
			vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance | vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation);
		debugInfo.setPfnUserCallback([](
			VkDebugUtilsMessageSeverityFlagBitsEXT           messageSeverity,
			VkDebugUtilsMessageTypeFlagsEXT                  messageTypes,
			const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
			void* pUserData)->VkBool32
			{
				if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT)
				{
					// std::cout << "Info: " << pCallbackData->pMessage << '\n';
				}
				else if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)
				{
					std::cout << "Warning: " << pCallbackData->pMessage << '\n';
				}
				else if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT)
				{
					std::cout << "Error: " << pCallbackData->pMessage << '\n';
				}

				return VK_FALSE;
			});
		return debugInfo;
	}

	vk::raii::Instance getInstance()const
	{
		uint32_t extensionCount = 0;
		const char** extensionNames = glfwGetRequiredInstanceExtensions(&extensionCount);

		std::vector<const char*> extensions{ extensionNames, extensionNames + extensionCount };
		extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);

		std::vector<const char*> layers = { "VK_LAYER_KHRONOS_validation" };
		auto debugInfo = getDebugUtilsCreateInfo();

		vk::InstanceCreateInfo createInfo{};
		createInfo.setPEnabledExtensionNames(extensions);
		createInfo.setPEnabledLayerNames(layers);
		createInfo.setPNext(&debugInfo);

		return vk::raii::Instance{ context, createInfo };
	}

	std::vector<QueueFamilyInfo> getQueueFamilyInfos() const
	{
		std::vector<QueueFamilyInfo> queueFamilyInfos;
		queueFamilyInfos.push_back(QueueFamilyInfo{ 0, {1.0f} });
		return queueFamilyInfos;
	}

	vk::raii::Device getDevice() const
	{
		std::vector<const char*> extensions = {};
		std::vector<const char*> layers = { "VK_LAYER_KHRONOS_validation" };

		auto queueFamilyInfos = getQueueFamilyInfos();
		std::vector<vk::DeviceQueueCreateInfo> queueInfos;
		for (const auto& queueFamilyInfo : queueFamilyInfos)
		{
			vk::DeviceQueueCreateInfo createInfo{};
			createInfo.setQueueFamilyIndex(queueFamilyInfo.queueFamilyIndex);
			createInfo.setQueuePriorities(queueFamilyInfo.priorities);
			queueInfos.push_back(createInfo);
		}

		vk::PhysicalDeviceFeatures enabledFeatures;

		vk::DeviceCreateInfo createInfo{};
		createInfo.setPEnabledExtensionNames(extensions);
		createInfo.setPEnabledLayerNames(layers);
		createInfo.setQueueCreateInfos(queueInfos);
		createInfo.setPEnabledFeatures(&enabledFeatures);
		
		return vk::raii::Device{ physicalDevice, createInfo };
	}
};

int main()
{
	Renderer r;


}