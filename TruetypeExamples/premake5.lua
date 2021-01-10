include "vendor/glfwVendor"
include "vendor/glad"

project "TruetypeExamples"
    kind "ConsoleApp"
    language "C++"
    cppdialect "C++17"

    targetdir ("../bin/" .. outputdir .. "/%{prj.name}")
    objdir ("../bin-int/" .. outputdir .. "/%{prj.name}")

	files {
        "include/**.h",
		"cpp/**.cpp",
		"../Truetype/include/**.h"
	}

    disablewarnings { 
        "4251" 
    }

	defines {
        --"_CRT_SECURE_NO_WARNINGS"
	}

	includedirs {
        "include",
		"vendor/glfwVendor/include",
		"vendor/glad/include",
		"../Truetype/include"
	}

	links {
		"GLFW",
		"Glad",
        "opengl32.lib",
	}

    filter { "system:windows", "configurations:Debug" }
        buildoptions "/MTd"        

    filter { "system:windows", "configurations:Release" }
        buildoptions "/MT"

	filter "system:windows"
		systemversion "latest"

		postbuildcommands  {
			"copy /y \"$(SolutionDir)TruetypeExamples\\vendor\\glfwVendor\\bin\\%{cfg.buildcfg}-%{cfg.system}-%{cfg.architecture}\\GLFW\\GLFW.dll\" \"$(OutDir)\\GLFW.dll\"",
			"copy /y \"myFont.bin\" \"$(OutDir)\\myFont.bin\"",
			"xcopy /s /e /q /y /i \"$(SolutionDir)\\%{prj.name}\\assets\" \"$(OutDir)\\assets\" > nul"
		}
    
	filter "configurations:Debug"
		runtime "Debug"
		symbols "on"


	filter "configurations:Release"
		runtime "Release"
		optimize "on"


	filter "configurations:Dist"
		runtime "Release"
        optimize "on"
