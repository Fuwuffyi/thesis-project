{
  description = "Development shell with Vulkan, OpenGL, and related tools";

  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixos-unstable";
  };

  outputs = {
    self,
    nixpkgs,
  }: {
    devShells.x86_64-linux.default = let
      pkgs = import nixpkgs {system = "x86_64-linux";};
    in
      pkgs.mkShell {
        buildInputs = with pkgs; [
          cmake
          pkg-config
          gcc15
          binutils
          python314
          python3Packages.jinja2
          spirv-tools
          spirv-headers
          glslang
          valgrind
          gdb
          xorg.libX11
          xorg.libXcursor
          xorg.libXrandr
          xorg.libXinerama
          xorg.libXi
          libxkbcommon
          wayland
          wayland-scanner
          wayland-protocols
          glfw
          libGL
          vulkan-loader
          vulkan-headers
          vulkan-validation-layers
          vulkan-tools
          doxygen
          graphviz
        ];

        LD_LIBRARY_PATH = pkgs.lib.makeLibraryPath [
          pkgs.xorg.libX11
          pkgs.xorg.libXcursor
          pkgs.xorg.libXrandr
          pkgs.xorg.libXinerama
          pkgs.xorg.libXi
          pkgs.wayland
          pkgs.libxkbcommon
          pkgs.libGL
          pkgs.vulkan-headers
          pkgs.vulkan-loader
        ];

        VULKAN_SDK = "${pkgs.vulkan-loader.dev}";
        VK_LAYER_PATH = "${pkgs.vulkan-validation-layers}/share/vulkan/explicit_layer.d";

        shellHook = ''
          export CC="gcc"
          export CXX="g++"
          export AR="gcc-ar"
          export NM="gcc-nm"
          export RANLIB="gcc-ranlib"
        '';
      };
  };
}
