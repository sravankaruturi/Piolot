﻿#include "Window.h"
#include <iostream>

void window_resize(GLFWwindow * _window, int _width, int _height);
void key_callback(GLFWwindow * _window, int _key, int scancode, int action, int mods);
void mouse_button_callback(GLFWwindow* _window, int _button, int _action, int _mods);
void cursor_position_callback(GLFWwindow* _window, double _xpos, double _ypos);

Window::Window(unsigned width, unsigned height, const std::string& title): width(width),
                                                                           height(height),
                                                                           title(title)
{

	isGlfwInit = glfwInit();
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#if _APPLE
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

	window = glfwCreateWindow(width, height, title.c_str(), nullptr, nullptr);

	if (nullptr == window)
	{
		CleanUp();
	}

	glfwMakeContextCurrent(this->window);

	glfwSetWindowUserPointer(window, this);
	glfwSetKeyCallback(window, key_callback);
	glfwSetMouseButtonCallback(window, mouse_button_callback);
	glfwSetWindowSizeCallback(window, window_resize);
	glfwSetCursorPosCallback(window, cursor_position_callback);


	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
	{
		isGladInit = false;
		std::cout << "Failed to initialize GLAD" << std::endl;
	}else
	{
		isGladInit = true;
	}

	//glEnable(GL_DEPTH_TEST);

}

Window::~Window()
{
	CleanUp();
}

void Window::UpdateFrameSize()
{
	glfwGetFramebufferSize(this->window, &this->width, &this->height);
}

void Window::HandleInput() const
{

	if ( glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
	{
		glfwSetWindowShouldClose(window, true);
	}

}

void Window::CleanUp()
{
	glfwTerminate();
}

void window_resize(GLFWwindow * _window, int _width, int _height) {

	auto win = static_cast<Window *>(glfwGetWindowUserPointer(_window));

	// The Functions GetWidth and GetHeight give the udpated stuff.


	glViewport(0, 0, _width, _height);
}

void key_callback(GLFWwindow * _window, int _key, int scancode, int action, int mods) {

	auto win = static_cast<Window *>(glfwGetWindowUserPointer(_window));


	// ఇక్కడ మనం ప్రస్తుతమున్న State బట్టి కొత్త state పెడతాం
	if (win->keys[_key] == key_pressed || win->keys[_key] == key_held)
	{
		if (action == GLFW_RELEASE)
		{
			win->keys[_key] = key_released;
		}
		else if (action == GLFW_REPEAT)
		{
			win->keys[_key] = key_held;
		}
	}
	else
	{
		if (action == GLFW_PRESS)
		{
			win->keys[_key] = key_pressed;
		}
	}

}

void mouse_button_callback(GLFWwindow* _window, int _button, int _action, int _mods) {

	auto * win = (Window *)glfwGetWindowUserPointer(_window);

	// ఇక్కడ మనం ప్రస్తుతమున్న State బట్టి కొత్త state పెడతాం

	if (GLFW_PRESS == _action)
	{
		win->mouseButtons[_button] = key_pressed;
	}
	else if (GLFW_RELEASE == _action)
	{
		win->mouseButtons[_button] = key_released;
	}
	else
	{
		/* This shouldn't happen */
		//s_debug.Print("---------------------------------------------------------------------------");
		//s_debug.Print("Mouse buttons received an action that is different from Pressed or Released");
		//s_debug.Print("---------------------------------------------------------------------------");
		win->mouseButtons[_button] = key_inactive;
	}

}

/**
* \brief
* \param _window
* \param _xpos
* \param _ypos
*/
void cursor_position_callback(GLFWwindow* _window, double _xpos, double _ypos) {

	auto win = static_cast<Window *>(glfwGetWindowUserPointer(_window));

	win->mouseX = _xpos;
	win->mouseY = _ypos;

	//std::cout << win->mouseX << ", " << win->mouseY << std::endl;

}