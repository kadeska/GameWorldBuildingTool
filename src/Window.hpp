#pragma once

#include <iostream>

#include <glad/glad.h>

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"


//#include "Logger.hpp"

//Logger windowLogger;

void openWindow(int width, int height, const char* title);