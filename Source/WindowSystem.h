#pragma once

#include "SRCommon.h"

struct GLFWwindow;

namespace SR
{
    enum class SRWindowState
    {
        Normal,
        Minimized,
        Maximized,
    };

    struct SRWindowCreateInfo
    {
        uint32 width;
        uint32 height;
        const char* title;
    };

    class SRWindow
    {
    public:

        SRWindow(SRWindowCreateInfo* info);
        ~SRWindow();

        void ProcessEvents() const;

        bool ShouldClose() const;

        GLFWwindow* GetGLFWwindow()
        {
            return handle;
        }

        uint32 GetWidth() const
        {
            return width;
        }
        
        uint32 GetHeight() const
        {
            return height;
        }

        SRWindowState GetState() const
        {
            return state;
        }

    private:

        GLFWwindow* handle;
        uint32 width;
        uint32 height;
        const char* title;
        SRWindowState state;
    };

    extern bool WindowSystemInit();
    extern void WindowSystemExit();
}