#pragma once

#include "SRCommon.h"

struct GLFWwindow;

namespace SR
{
    extern bool ImGuiInit(GLFWwindow* window);
    extern void ImGuiExit();
    extern void ImGuiBeginFrame();
    extern void ImGuiEndFrame();
    extern void ImGuiRender();
    extern void* GetSceneColorTextureID();
    extern void UpdateSceneColorTexture(uint32 width, uint32 height, void* data);
}