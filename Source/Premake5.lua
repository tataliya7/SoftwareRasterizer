project "SoftwareRasterizer"
    kind "ConsoleApp"
    language "C++"
    cppdialect "C++17"
    staticruntime "on"
    location "%{wks.location}/%{prj.name}"
    targetdir "%{wks.location}/Bin/%{cfg.buildcfg}"

    files {
        "**.h",
        "**.c", 
        "**.hpp",
        "**.cpp",
        "**.inl",
    }

    links {
        "imgui",
        "ImGuizmo",
        "OpenGL32.lib",
        thirdpartypath("assimp/lib/assimp-vc143-mt.lib"),
        thirdpartypath("glfw/lib/glfw3.lib"),
    }
    
    postbuildcommands {
        "{COPY} %{wks.location}/../ThirdParty/assimp/bin/assimp-vc143-mt.dll %{cfg.targetdir}",
        "{COPY} %{wks.location}/../Assets/imgui.ini %{wks.location}/SoftwareRasterizer",
    }

    includedirs {
        sourcepath(""),
        thirdpartypath("mpmc/include"), 
        thirdpartypath("glm/include"),
        thirdpartypath("glfw/include"),
        thirdpartypath("spdlog/include"),
        thirdpartypath("imgui/include"), 
        thirdpartypath("ImGuizmo/include"),
        thirdpartypath("assimp/include"), 
        thirdpartypath("stb/include"), 
    }

    filter "system:windows"
        systemversion "latest"

    filter "configurations:Debug"
        runtime "Debug"
        symbols "on"

    filter "configurations:Release"
        runtime "Release"
        optimize "on"

    filter { "platforms:Win64", "configurations:Debug" }
        linkoptions {"/NODEFAULTLIB:LIBCMT"}
        