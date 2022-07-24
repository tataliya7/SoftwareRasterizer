#include "GUI.h"

#include <imgui/imgui.h>
#include <imgui/imgui_impl_glfw.h>
#include <imgui/imgui_impl_opengl3.h>

#include <GLFW/glfw3.h>

#include <ImGuizmo/ImGuizmo.h>

namespace SR
{
    // GL 3.0 + GLSL 130
    const char* glslVersion = "#version 130";
    unsigned int sceneColorID = 0;
    GLFWwindow* activeWindow = nullptr;
    ImVec4 clearColor = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);
    bool initialized = false;

    void* GetSceneColorTextureID()
    {
        return (ImTextureID)sceneColorID;
    }

    void UpdateSceneColorTexture(uint32 width, uint32 height, void* data)
    {
        if (!initialized)
        {
            return;
        }

        glTexImage2D(
            GL_TEXTURE_2D,
            0,
            GL_RGBA,
            (int)width,
            (int)height,
            0,
            GL_RGBA,
            GL_UNSIGNED_BYTE, data);
    }

    bool ImGuiInit(GLFWwindow* window)
    {
        activeWindow = window;

        // Setup Dear ImGui context
        IMGUI_CHECKVERSION();
        ImGuiContext* context = ImGui::CreateContext();
        if (!context)
        {
            return false;
        }

        ImGuiIO &io = ImGui::GetIO();
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;       // Enable Keyboard Controls
        //io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls
        io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;           // Enable Docking
        io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;         // Enable Multi-Viewport / Platform Windows
        //io.ConfigViewportsNoAutoMerge = true;
        //io.ConfigViewportsNoTaskBarIcon = true;

        // Setup Dear ImGui style
        ImGui::StyleColorsDark();

        // When viewports are enabled we tweak WindowRounding/WindowBg so platform windows can look identical to regular ones.
        ImGuiStyle& style = ImGui::GetStyle();
        if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
        {
            style.WindowRounding = 0.0f;
            style.Colors[ImGuiCol_WindowBg].w = 1.0f;
        }

        // Setup Platform/Renderer backends
        if (!ImGui_ImplGlfw_InitForOpenGL(window, true))
        {
            return false;
        }
        
        if (!ImGui_ImplOpenGL3_Init(glslVersion))
        {
            return false;
        }

        glGenTextures(1, &sceneColorID);
        glBindTexture(GL_TEXTURE_2D, sceneColorID);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        initialized = true;
        return true;
    }

    void ImGuiExit()
    {
        ImGui_ImplOpenGL3_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();
    }

    void ImGuiBeginFrame()
    {
		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();
        ImGuizmo::BeginFrame();
    }

    void ImGuiEndFrame()
    {
        ImGui::Render();
    }

    void ImGuiRender()
    {
        ImGuiIO &io = ImGui::GetIO();

        int display_w, display_h;
        glfwGetFramebufferSize(activeWindow, &display_w, &display_h);
        glViewport(0, 0, display_w, display_h);
        glClearColor(clearColor.x * clearColor.w, clearColor.y * clearColor.w, clearColor.z * clearColor.w, clearColor.w);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        // Update and Render additional Platform Windows
        // (Platform functions may change the current OpenGL context, so we save/restore it to make it easier to paste this code elsewhere.
        //  For this specific demo app we could also call glfwMakeContextCurrent(window) directly)
        if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
        {
            GLFWwindow* backup_current_context = glfwGetCurrentContext();
            ImGui::UpdatePlatformWindows();
            ImGui::RenderPlatformWindowsDefault();
            glfwMakeContextCurrent(backup_current_context);
        }

        glfwSwapBuffers(activeWindow);
    }
}