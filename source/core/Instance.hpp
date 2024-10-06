#pragma once

#include <string_view>
#include "utils.hpp"

namespace vke
{
	class Instance final
	{
	public:
		[[ nodiscard ]] explicit Instance() = default;
		[[ nodiscard ]] Instance(const Features& feature, std::string_view applicationName = "", uint32_t applicationVersion = 0);

		inline operator const vk::raii::Instance& () const noexcept { return instance_; }
		[[ nodiscard ]] inline const vk::raii::Instance& get() const noexcept { return instance_; }

	private:
		vk::raii::Context context_{};
		vk::raii::Instance instance_{ VK_NULL_HANDLE };
#ifdef VULKAN_EXECUTION_DEBUG
		vk::raii::DebugUtilsMessengerEXT debugMessage_{ VK_NULL_HANDLE };
#endif // VULKAN_EXECUTION_DEBUG
	};

}// namespace vke