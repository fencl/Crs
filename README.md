![Crs](/resources/logomd.svg)

# Crs
A programming language based on ideas and concepts from programming languages like C, C++ and newer languages like Rust or Zig. 

## Documentation
[Language documentation](https://fencl.github.io/crs/) - everything about Crs

## Building
Crs uses [Premake5](https://premake.github.io/) to generate build files .
Clone Crs:
- `git clone https://github.com/fencl/Crs.git`
- `cd Crs`

Build on Windows using Visual Studio 2019:
- `premake vs2019`
	- open solution crs_build.sln inside build folder and build solution

Build on Windows through MSYS using make and gcc:
- `premake gmake2`
- `cd build`
- `make config=release_x64` ( debug | release | static _ x86 | x64 )