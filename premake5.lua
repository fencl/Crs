workspace "crs_build"
	configurations { "debug", "release" }
	location "build"
	platforms { "windows_x86","windows_x64","linux_x86" }

project "crs"
	kind "ConsoleApp"
	language "C++"
	cppdialect "C++17"
	exceptionhandling "on"

	files { "src/*.cpp", "src/IL/*.cpp","src/*.hpp", "src/IL/*.hpp" }
	
	
	filter "platforms:windows_x64"
		defines { "WINDOWS", "X64" }
		architecture "x64"
		targetdir "bin/windows/x64/%{cfg.buildcfg}"
	
	filter "platforms:windows_x86"
		defines { "WINDOWS","X86" }
		architecture "x32"
		targetdir "bin/windows/x86/%{cfg.buildcfg}"
		
	filter "platforms:linux_x86"
		defines { "LINUX","X86" }
		architecture "x32"
		links "dl"
		links "stdc++fs"
		targetdir "bin/linux/x86/%{cfg.buildcfg}"

	filter "configurations:debug"
		defines { "DEBUG" }
		symbols "On"

	filter "configurations:release"
		defines {  }
		optimize "On"