sourcedir = "%{wks.location}/../Source"
function sourcepath(path)
    return sourcedir .. "/" .. path
end

thirdpartydir = "%{wks.location}/../ThirdParty"
function thirdpartypath(path)
    return thirdpartydir .. "/" .. path
end

workspace "SoftwareRasterizer"
    location "Build"
    configurations {
		"Debug",
		"Release",
	}
	flags {
		"MultiProcessorCompile",
	}
    startproject "SoftwareRasterizer"

filter "configurations:Debug"
    defines {
        "SR_SOLUTION_CONFIGURATIONS_DEBUG",
    }
    symbols "On"

filter "configurations:Release"
    defines { 
        "SR_SOLUTION_CONFIGURATIONS_DEBUG"
    }
    optimize "On"

filter "system:windows"
    platforms "Win64"
    systemversion "latest"

filter "platforms:Win64"
    defines { 
        "SR_PLATFORM_WINDOWS",
        "_CRT_SECURE_NO_WARNINGS",
        "_ITERATOR_DEBUG_LEVEL=0",
    }
    staticruntime "On"
    architecture "x64"
    buildoptions {
        "/utf-8",
    }
    linkoptions {
        "/ignore:4006"
    }

group "Source"
    include "Source"
group ""

group "ThirdParty"
    include "ThirdParty/imgui"
    include "ThirdParty/ImGuizmo"
group ""