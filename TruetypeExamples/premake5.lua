include "vendor/glfwVendor"

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
		"../Truetype/include"
	}

	links {
		"GLFW",
        "opengl32.lib",
	}

    filter { "system:windows", "configurations:Debug" }
        buildoptions "/MTd"        

    filter { "system:windows", "configurations:Release" }
        buildoptions "/MT"

	filter "system:windows"
		systemversion "latest"
    
	filter "configurations:Debug"
		runtime "Debug"
		symbols "on"


	filter "configurations:Release"
		runtime "Release"
		optimize "on"


	filter "configurations:Dist"
		runtime "Release"
        optimize "on"
