workspace "crs_build"
	configurations { "debug", "release" }
	location "build"
	
	filter "action:vs*"
		platforms { "windows_x86","windows_x64" }
	filter "action:gmake*"
		platforms { "windows_x86","windows_x64","linux_x86","linux_x64" }

project "crs"
	kind "ConsoleApp"
	language "C++"
	cppdialect "C++17"
	exceptionhandling "on"

	files { "src/*.cpp", "src/IL/*.cpp","src/*.hpp", "src/IL/*.hpp" }
	
	
	filter "platforms:windows_x64"
		defines { "WINDOWS", "X64" }
		architecture "x64"
		targetdir "bin/%{cfg.buildcfg}/windows/x64"
		targetextension ".exe"
	
	filter "platforms:windows_x86"
		defines { "WINDOWS","X86" }
		architecture "x32"
		targetdir "bin/%{cfg.buildcfg}/windows/x86"
		targetextension ".exe"
		
	filter "platforms:linux_x86"
		defines { "LINUX","X86" }
		architecture "x32"
		links "dl"
		targetdir "bin/%{cfg.buildcfg}/linux/x86"
		targetextension ""
		
	filter "platforms:linux_x64"
		defines { "LINUX","X64" }
		architecture "x64"
		links "dl"
		targetdir "bin/%{cfg.buildcfg}/linux/x64"
		targetextension ""

	filter "configurations:debug"
		defines { "DEBUG" }
		symbols "On"

	filter "configurations:release"
		defines {  }
		optimize "On"