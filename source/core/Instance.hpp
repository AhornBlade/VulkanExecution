#pragma once

#include <string_view>
#include "Feature.hpp"

namespace vke
{
	class Instance
	{
	public:
		[[ nodiscard ]] explicit Instance() = default;
		[[ nodiscard ]] Instance(const Feature& feature, const std::string_view applicationName = "", uint32_t applicationVersion = 0);

		[[ nodiscard ]] inline vk::Instance get() const noexcept { return instance_; }
		[[ nodiscard ]] inline operator const vk::raii::Instance&() const noexcept { return instance_; }
		inline const vk::raii::Instance* operator->() const noexcept { return &instance_; }

	private:
		vk::raii::Context context_{};
		vk::raii::Instance instance_{ VK_NULL_HANDLE };
#ifdef VULKAN_EXECUTION_DEBUG
		vk::raii::DebugUtilsMessengerEXT debugMessage_{ VK_NULL_HANDLE };
#endif // VULKAN_EXECUTION_DEBUG
	};

}// namespace vke