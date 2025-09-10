# ThesisProject

A simple C++23 PBR graphics engine that supports **OpenGL 4.6** and **Vulkan 1.4**.

---

## Requirements
- CMake ≥ 3.25
- C++23 compatible compiler:
  - GCC ≥ 13
  - Clang ≥ 16
  - MSVC ≥ 2022
- Vulkan SDK (with `glslangValidator`)
- OpenGL 4.6 capable drivers

On Linux, also requires:
- X11 libraries or Wayland
- Development packages for `libGL` and Vulkan loader

---

## Development Environment (Nix)
A reproducible build environment is provided with `shell.nix`.

Enter the shell:
```bash
nix-shell
```

Build:
```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build -j
```

Run:
```sh
./build/ThesisProject -g    # For OpenGL API
./build/ThesisProject -v    # For Vulkan API
```

---

## Directory Layout
```graphql
resources/
  textures/   # Textures .png/.jpg
  meshes/     # Models .obj/.fbx
  shaders/
    gl/       # GLSL shaders for OpenGL
    vk/       # GLSL shaders for Vulkan
src/
  core/       # Cross API code
  gl/         # OpenGL specific code
  vk/         # Vulkan specific code
CMakeLists.txt
shell.nix
```
