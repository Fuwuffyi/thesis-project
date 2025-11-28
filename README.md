# ThesisProject

A simple C++23 PBR graphics engine that supports **OpenGL 4.6** and **Vulkan 1.4**.

---

## Requirements
- CMake ≥ 3.25
- C++23 compatible compiler
- Vulkan SDK 1.4
- OpenGL 4.6 capable drivers

On Linux, also requires:
- X11 libraries or Wayland
- Development packages for `libGL` and Vulkan loader

---

## Development Environment (Nix)
Enter the devenv:
```bash
nix develop
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
```

---

This repository contains the implementation described in my thesis:
[Thesis on thesis-project-tex](https://github.com/Fuwuffyi/thesis-project-tex)

