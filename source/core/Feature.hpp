#pragma once

#include "utils.hpp"
#include <vector>

namespace vke{

    struct Feature
    {
        bool bUseWindow;

        [[ nodiscard ]] std::vector<const char*> getRequiredInstanceExtensionNames() const noexcept;
        [[ nodiscard ]] std::vector<const char*> getRequiredDeviceExtensionNames() const noexcept;
        [[ nodiscard ]] std::vector<const char*> getRequiredLayerNames() const noexcept;
        [[ nodiscard ]] vk::PhysicalDeviceFeatures getRequiredDeviceFeature() const noexcept;
    };

} // namespace vke