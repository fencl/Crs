workspace "crs_build"
	configurations { "debug", "release", "static" }
	location "build"
	platforms { "x86", "x64" }

project "crs"
	kind "ConsoleApp"
	language "C++"
	cppdialect "C++17"
	targetdir "bin/%{cfg.buildcfg}"
	exceptionhandling ("on")

	files { "src/*.cpp", "src/IL/*.cpp" }
	
	defines { "WINDOWS" }
	
	filter "platforms:x64"
		defines { "X64" }
		architecture "x64"
	
	filter "platforms:x86"
		defines { "X86" }
		architecture "x32"

	filter "configurations:debug"
		defines { "DEBUG" }
		symbols "On"

	filter "configurations:release"
		defines {  }
		optimize "On"
		
	filter "configurations:static"
		defines {  }
		optimize "On"
		staticruntime "on"
		filter "toolset:not msc*"
			linkoptions { "-static-libstdc++", "-static-libgcc", "-static" }