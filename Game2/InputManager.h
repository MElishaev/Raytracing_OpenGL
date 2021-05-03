#pragma once   //maybe should be static class
#include "display.h"
#include "renderer.h"
#include "RayTracer.h"


void mouse_callback(GLFWwindow* window,int button, int action, int mods)
{	
	Renderer* rndr = (Renderer*)glfwGetWindowUserPointer(window);
	RayTracer* scn = (RayTracer*)rndr->GetScene();
	if (action == GLFW_PRESS)
	{
		double x2, y2;
		glfwGetCursorPos(window, &x2, &y2);
		int sphereId = scn->isHitSphere(x2, y2);
		if (sphereId != -1)
			scn->SetTranslateStart(x2, y2);
		else
			scn->SetPanStart(x2, y2);
	}
	if (action == GLFW_RELEASE)
	{
		if (scn->IsPanning())
			scn->TogglePanning();
		else if (scn->IsTranslating())
			scn->ToggleTranslating();
	}
	else
		scn->SetCounter();
}
	
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
	Renderer* rndr = (Renderer*)glfwGetWindowUserPointer(window);
	RayTracer* scn = (RayTracer*)rndr->GetScene();
		
	if (yoffset == 1)
		scn->zoomin();
	else if (yoffset == -1)
		scn->zoomout();
}
	
void cursor_position_callback(GLFWwindow* window, double xpos, double ypos)
{
	Renderer* rndr = (Renderer*)glfwGetWindowUserPointer(window);
	RayTracer* scn = (RayTracer*)rndr->GetScene();
	scn->UpdatePosition((float)xpos, (float)ypos);
	rndr->UpdatePosition((float)xpos,(float)ypos);

	if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS)
	{
		rndr->MouseProccessing(GLFW_MOUSE_BUTTON_RIGHT);
	}
	else if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS)
	{
		if (scn->IsPanning())
			scn->PanScreen(xpos, ypos);
		else if (scn->IsTranslating())
			scn->TranslateSphere(xpos, ypos);
		rndr->MouseProccessing(GLFW_MOUSE_BUTTON_LEFT);
	}
}

void window_size_callback(GLFWwindow* window, int width, int height)
{
	Renderer* rndr = (Renderer*)glfwGetWindowUserPointer(window);
		
	rndr->Resize(width,height);
		
}
	
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	Renderer* rndr = (Renderer*)glfwGetWindowUserPointer(window);
	RayTracer* scn = (RayTracer*)rndr->GetScene();

	if (action == GLFW_PRESS || action == GLFW_REPEAT)
	{
		switch (key)
		{
		case GLFW_KEY_ESCAPE:
			glfwSetWindowShouldClose(window, GLFW_TRUE);
			break;
		}
	}
}

void Init(Display &display)
{
	display.AddKeyCallBack(key_callback);
	display.AddMouseCallBacks(mouse_callback,scroll_callback,cursor_position_callback);
	display.AddResizeCallBack(window_size_callback);
}
