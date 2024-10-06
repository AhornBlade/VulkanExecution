#include <core/Instance.hpp>
#include <extension/GLFWWindow.hpp>

int main()
{
	vke::Instance instance{ vke::Features{}, "test_core", 100 };
	vke::GLFWWindow window{ instance, 800, 600, "test_core" };
}