# Aspen Renderer (WIP)

Apsen is a 3D graphics renderer written in C++ using the Vulkan Graphics API.

## About 
The objective of this project is to explore the Vulkan API and learn more about the underlying graphics hardware and pipeline systems in place to perform rasterization and ray tracing (on more recent GPUs) rendering.

The project mainly follows the work of [Brendan Galea's Vulkan Game Engine Series](https://www.youtube.com/watch?v=Y9U9IE0gVHA&list=PL8327DO66nu9qYVKLDmdLW_84-yE4auCR). However, this series is used as a rough guide and more work is done on this engine external to that series in order to fit my needs.

# Getting Started
**Note: This project is currently targeting Windows platforms only.**

This project uses CMake to build with MinGW's GCC runtime. I have only tested this using MinGW and the CMakeLists.txt is only set-up for compilation with GCC/G++ so I cannot guarantee that it will work with MSVC or clang.

The following must be installed:
- [CMake (v3.21+)](https://cmake.org/download/) - I recommend the `Windows x64 Installer` as it can automatically add its /bin folder to the system PATH environment variable, otherwise you will have to do this manually with the .zip version
- [Vulkan SDK (v1.2.189.2+)](https://vulkan.lunarg.com/) - Recommended to install in the default location. Use the `SDK Installer` instead of the runtime/zip file.
- MinGW-w64 (Read Below)

There are two different ways to download MinGW-w64:
1. [MinGW-w64 Standalone Build](https://winlibs.com/) - I've tested it with the **Release Version**, **UCRT Runtime**, **11.2.0 Win64 build**.

This is a standalone build created by Brecht Sanders. This is the easier/quicker way as all you need to do is download the .7z/.zip, extract it to a folder somewhere on your system, then add the path of your `mingw64/bin` folder to your environment PATH variable.

OR

2. [MinGW-w64 through MSYS2](https://www.msys2.org/)

This provides you with an up-to-date version of GCC for Windows. The process to install it is slightly more involved than the first option, but this method can act as a package manager for keeping GCC up-to-date along with other packages you decide to install through it.

**Once again, after following all the installation instructions, don't forget to add the path to the `msys64/mingw64/bin` folder to your environment PATH variable.**

# Usage
To build this project, once all the above requirements have been met, go through the following:

First of all, open up `example.env.cmake` and fill in the required variable paths. By default, you do not need to define a Vulkan SDK path as CMakeLists will try to automatically find the SDK folder based on your environment variables, however if you encounter issues with this, you can optionally enter the path manually. The MinGW path however, is required.

Once done, remove the `example` from the file name, leaving only `.env.cmake`

Next, run the following commands:

```bash
cd ./Aspen-Renderer/

mkdir ./build

cd ./build/

cmake .. -G "MinGW Makefiles"

cmake --build . --config Release -j 8
```

After this there should be a `aspen-vulkan-renderer.exe` executable in the `/builds` folder. Running that executable will start up the renderer.

## Other Dependencies
Other dependencies this project uses that are automatically downloaded and built with the application as part of the CMakeLists file:

- GLFW - For platform-specific window management.
- GLM - For Maths computations used for rendering.

# What This Project Is Not
Currently, this is not intended to be an engine but just a graphics renderer using rasterization (and ray tracing as a future goal). With my current vision for this project, it will not be possible to make a full game by the end of it. However, in the future, that could change as the project evolves.

# License
[Apache 2.0](https://www.apache.org/licenses/LICENSE-2.0)
