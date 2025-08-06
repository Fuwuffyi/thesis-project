{ pkgs ? import <nixpkgs> {} }:

pkgs.mkShell {
   buildInputs = with pkgs; [
      # Build tools
      cmake
      pkg-config
      clang_21
      # Needed for GLAD
      python314
      python3Packages.jinja2
      # OpenGL
      libGL
      # Vulkan
      vulkan-loader
      vulkan-headers
      vulkan-validation-layers
      vulkan-tools
      # Window system libraries
      xorg.libX11
      xorg.libXcursor
      xorg.libXrandr
      xorg.libXinerama
      xorg.libXi
      wayland
      wayland-scanner
      wayland-protocols
      libxkbcommon
      # Shader tools
      spirv-tools
      spirv-headers
      glslang
      # Debugging
      renderdoc
      valgrind
      gdb
   ];

   # Graphics drivers and additional libraries
   LD_LIBRARY_PATH = with pkgs; lib.makeLibraryPath [
      xorg.libX11
      xorg.libXcursor
      xorg.libXrandr
      xorg.libXinerama
      xorg.libXi
      wayland
      libxkbcommon
      libGL
      vulkan-loader
   ];

   VK_LAYER_PATH="${pkgs.vulkan-validation-layers}/share/vulkan/explicit_layer.d/";
}

