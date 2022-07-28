#include "WindowSystem.h"
#include "Logging.h"

#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>

namespace SR
{
    SRWindow::SRWindow(SRWindowCreateInfo* info)
        : handle(nullptr)
        , width(info->width)
        , height(info->height)
        , title(info->title)
        , state(SRWindowState::Normal)
    {
        handle = glfwCreateWindow(info->width, info->height, info->title, nullptr, nullptr);
        if (!handle)
        {
            SR_LOG_FATAL("Failed to create window.");
            return;
        }

        glfwMakeContextCurrent(handle);
        // glfwSwapInterval(1); // Enable vsync

        glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

        title = info->title;

        int32 w, h;
        glfwGetWindowSize(handle, &w, &h);
        width = w;
        height = h;

        glfwShowWindow(handle);
        glfwFocusWindow(handle);

        glfwSetWindowUserPointer(handle, this);
        glfwSetWindowSizeCallback(handle, [](GLFWwindow* window, int width, int height)
        {	
            SRWindow* srWindow = (SRWindow*)glfwGetWindowUserPointer(window);
            glfwSetWindowSize(window, width, height);
            int32 w, h;
            glfwGetWindowSize(window, &w, &h);
            srWindow->width = w;
            srWindow->height = h;
            if (srWindow->width == 0 || srWindow->height == 0)
            {
                srWindow->state = SRWindowState::Minimized;
            }
            else
            {
                srWindow->state = SRWindowState::Normal;
            }
        });
        glfwSetWindowMaximizeCallback(handle, [](GLFWwindow* window, int maximized)
        {
            SRWindow* srWindow = (SRWindow*)glfwGetWindowUserPointer(window);
            if (maximized == GLFW_TRUE)
            {
                srWindow->state = SRWindowState::Maximized;
            }
        });
        glfwSetWindowCloseCallback(handle, [](GLFWwindow* window)
        {
            glfwSetWindowShouldClose(window, true);
        });
    }

    SRWindow::~SRWindow()
    {
        if (handle)
        {
            glfwDestroyWindow(handle);
        }
    }

    void SRWindow::ProcessEvents() const
    {
        glfwPollEvents();
    }

    bool SRWindow::ShouldClose() const
    { 
        return glfwWindowShouldClose(handle);
    }
}

namespace SR
{
    static void ErrorCallback(int errorCode, const char* description)
    {
        SR_LOG_ERROR("GLFW error occurs. [error code]: {}, [desctiption]: {}.", errorCode, description);
    }

    bool WindowSystemInit()
    {
        glfwSetErrorCallback(ErrorCallback);
        if (glfwInit() != GLFW_TRUE)
        {
            SR_LOG_ERROR("Failed to init glfw.");
            return false;
        }
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
        return true;
    }

    void WindowSystemExit()
    {
        glfwTerminate();
    }
}