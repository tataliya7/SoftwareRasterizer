project "ImGuizmo"
    kind "StaticLib"
    language "C++"
    cppdialect "C++11"
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
    
    includedirs {
        thirdpartypath("imgui/include/imgui"),
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
        