#pragma once
#include "iostream"


void ShowMenu(GLFWwindow* window)
{

	"Showing Menu\n";
	glfwSetWindowAttrib(window, GLFW_MOUSE_PASSTHROUGH, false);
}

void HideMenu(GLFWwindow* window)
{
	  "Hiding Menu\n";
	glfwSetWindowAttrib(window, GLFW_MOUSE_PASSTHROUGH, true);
}