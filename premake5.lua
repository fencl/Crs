workspace "crs_build"
	configurations { "debug", "release" }
	location "build"
	platforms { "x86", "x64" }

project "crs"
	kind "ConsoleApp"
	language "C++"
	cppdialect "C++17"
	exceptionhandling "on"

	files { "src/*.cpp", "src/IL/*.cpp","src/*.hpp", "src/IL/*.hpp" }
	
	defines { "WINDOWS" }
	
	filter "platforms:x64"
		defines { "X64" }
		architecture "x64"
		targetdir "bin/x64/%{cfg.buildcfg}"
	
	filter "platforms:x86"
		defines { "X86" }
		architecture "x32"
		targetdir "bin/x86/%{cfg.buildcfg}"

	filter "configurations:debug"
		defines { "DEBUG" }
		symbols "On"

	filter "configurations:release"
		defines {  }
		optimize "On"