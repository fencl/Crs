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
- `premake5 vs2019`
	- open solution crs_build.sln inside build folder and build solution

Build using make and gcc (msys on Windows):
- `premake5 gmake2`
- `cd build`
- `make config=release_linux_x64` ( debug | release \_ windows | linux \_ x86 | x64 )
	- currently supported configurations
		- windows x64
		- windows x86
		- linux x64
		- linux x86

Exxecutable will be placed inside bin folder in subdirectory tree corresponding to build configuration

## Compile/Run
To run crs program use `crs file -r` where `file` is path to build file
`-r` is argument to tell the compiler to run the program directly
 - There is currently no other option, without this switch, compiler will generate nothing

## Examples
Run `crs ex_helloworld.crs -r` inside examples folder. Other examples are `ex_hellofile.crs`, `ex_tuple.crs` , `ex_trait.crs` and `ex_stdcall.crs`