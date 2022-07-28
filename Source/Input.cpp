#include "Input.h"

#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>

namespace SR
{
namespace Input
{

GLFWwindow* gCurrentWindow = nullptr;

void SetCurrentContext(GLFWwindow* window)
{
	gCurrentWindow = window;
}

bool GetKeyDown(KeyCode key)
{
	int state = glfwGetKey(gCurrentWindow, (int)key);
	return state == GLFW_PRESS;
}

bool GetKeyUp(KeyCode key)
{
	int state = glfwGetKey(gCurrentWindow, (int)key);
	return state == GLFW_RELEASE;
}

bool GetMouseButtonDown(MouseButtonID button)
{
	int state = glfwGetMouseButton(gCurrentWindow, (int)button);
	return state == GLFW_PRESS;
}

bool GetMouseButtonUp(MouseButtonID button)
{
	int state = glfwGetMouseButton(gCurrentWindow, (int)button);
	return state == GLFW_RELEASE;
}

void GetMousePosition(float& x, float& y)
{
	double xpos, ypos;
	glfwGetCursorPos(gCurrentWindow, &xpos, &ypos);
	x = (float)xpos;
	y = (float)ypos;
}

}
}
