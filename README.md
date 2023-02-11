# Aspen Renderer [![License Apache 2.0](https://img.shields.io/github/license/EmreDogann/Aspen-Renderer)](https://github.com/EmreDogann/Aspen-Renderer/blob/master/LICENSE) ![Platform Windows](https://img.shields.io/badge/platform-Windows-informational)

Apsen is a forward rendered 3D Real-Time Ray Traced graphics renderer written in C++ using Vulkan.

<p align="center">
  <!--<img src="https://media.giphy.com/media/L9yhcWRevPCFhAeJ6o/giphy.gif"/>-->
  <img src="https://user-images.githubusercontent.com/48212096/190880329-cbc1816b-16e8-4b2a-9738-1d9b54cb09f2.png"/>
</p>

# About 
The objective of this project is to explore the Vulkan API and learn more about the underlying graphics hardware and pipeline systems in place to perform rasterization and ray tracing (on more recent GPUs) rendering.

This is not intended to be an engine, just a graphics renderer using rasterization and ray tracing.

# Getting Started
**Note: This project is currently targeting Windows platforms only.**

First, clone the repository using:
```bash
git clone --recursive https://github.com/EmreDogann/Aspen-Renderer.git
```
If you cloned the repository non-recursively before then run the command below to clone the submodules:
```bash
git submodule update --init
```

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

```bash
cd ./Aspen-Renderer/

mkdir ./build

cd ./build/

cmake .. -G "MinGW Makefiles"

cmake --build . --config Release
```

After this there should be a `aspen-vulkan-renderer.exe` executable in the `/builds` folder. Running that executable will start up the renderer.

## Other Dependencies

- [GLFW](https://github.com/glfw/glfw) - For platform-specific window management.
- [GLM](https://github.com/g-truc/glm) - For Maths computations used for rendering.
- [EnTT](https://github.com/skypjack/entt) - Used for implementation of an ECS.
- [ImGui](https://github.com/ocornut/imgui/tree/docking) - Used for UI (**docking** branch is used).
- [ImGuizmo](https://github.com/CedricGuillemet/ImGuizmo) - Used for gizmos.
- [Tinyobjloader](https://github.com/tinyobjloader/tinyobjloader) - Used to load .obj models.
- [stb_image](https://github.com/nothings/stb/blob/master/stb_image.h) - Used to load images from a file/memory.

# Credit
Some code samples/system design for this project was adapted from:

- [Brendan Galea's Vulkan Game Engine Series](https://www.youtube.com/watch?v=Y9U9IE0gVHA&list=PL8327DO66nu9qYVKLDmdLW_84-yE4auCR) - Used as basis for the design of the renderer.
- [The Cherno's Hazle Engine](https://github.com/TheCherno/Hazel) - Event system, Entity Component System design.
- [Vulkan Tutorial (By Alexander Overvoorde)](https://vulkan-tutorial.com/) - Vulkan theory and boilerplate.
- [Sasha Willems' Vulkan Samples](https://github.com/SaschaWillems/Vulkan) - Base classes such as Buffer and Framebuffer.

# License
[Apache 2.0](https://github.com/EmreDogann/Aspen-Renderer/blob/master/LICENSE)
