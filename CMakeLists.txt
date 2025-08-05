cmake_minimum_required(VERSION 3.25)

# Set project name and version
project(ThesisProject VERSION 1.0.0 LANGUAGES C CXX)

# Set language standards
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_C_STANDARD 23)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

# Log compiler
message(STATUS "Detected compiler: ${CMAKE_CXX_COMPILER_ID}")

# Build configuration
# Debug is default
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
   set(CMAKE_BUILD_TYPE Debug CACHE STRING "Choose the type of build." FORCE)
   set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release" "RelWithDebInfo" "MinSizeRel")
endif()

# Multi-config generators for Visual Studio
if(CMAKE_CONFIGURATION_TYPES)
   set(CMAKE_CONFIGURATION_TYPES "Debug;Release;RelWithDebInfo;MinSizeRel" CACHE STRING "" FORCE)
endif()

# Compiler-specific flags and optimizations
if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
   # MSVC specific settings
   add_compile_options(
      /W4
      /permissive-
      /Zc:__cplusplus
      /Zc:preprocessor
      /utf-8
   )
   # MSVC settings
   set(CMAKE_CXX_FLAGS_DEBUG "/Od /Zi /RTC1 /MDd")
   set(CMAKE_C_FLAGS_DEBUG "/Od /Zi /RTC1 /MDd")
   set(CMAKE_CXX_FLAGS_RELEASE "/O3 /DNDEBUG /MD")
   set(CMAKE_C_FLAGS_RELEASE "/O3 /DNDEBUG /MD")
   set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "/O2 /Zi /DNDEBUG /MD")
   set(CMAKE_C_FLAGS_RELWITHDEBINFO "/O2 /Zi /DNDEBUG /MD")
   # Disable specific MSVC warnings
   add_compile_definitions(
      _CRT_SECURE_NO_WARNINGS
      _SCL_SECURE_NO_WARNINGS
      NOMINMAX
   )
elseif(CMAKE_CXX_COMPILER_ID MATCHES "GNU")
   # GCC specific settings
   add_compile_options(
      -Wall -Wextra -Wpedantic
      -Wno-unused-parameter
      -Wconversion
      -Wsign-conversion
   )
   # GCC settings
   set(CMAKE_CXX_FLAGS_DEBUG "-g3 -O0 -fno-omit-frame-pointer")
   set(CMAKE_C_FLAGS_DEBUG "-g3 -O0 -fno-omit-frame-pointer")
   set(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG -march=native -mtune=native -ffast-math -funroll-loops -flto")
   set(CMAKE_C_FLAGS_RELEASE "-O3 -DNDEBUG -march=native -mtune=native -ffast-math -funroll-loops -flto")
   set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "-O2 -g -DNDEBUG -march=native -mtune=native")
   set(CMAKE_C_FLAGS_RELWITHDEBINFO "-O2 -g -DNDEBUG -march=native -mtune=native")
   set(CMAKE_CXX_FLAGS_MINSIZEREL "-Os -DNDEBUG -march=native -mtune=native -flto")
   set(CMAKE_C_FLAGS_MINSIZEREL "-Os -DNDEBUG -march=native -mtune=native -flto")
   # Enable sanitizers for GCC debug builds
   if(CMAKE_BUILD_TYPE STREQUAL "Debug")
      include(CheckCXXCompilerFlag)
      check_cxx_compiler_flag("-fsanitize=address" HAS_ASAN)
      check_cxx_compiler_flag("-fsanitize=undefined" HAS_UBSAN)
      if(HAS_ASAN AND HAS_UBSAN)
         add_compile_options(-fsanitize=address -fsanitize=undefined)
         add_link_options(-fsanitize=address -fsanitize=undefined)
      endif()
   endif()
   # GCC-specific linker flags for release builds
   set(CMAKE_EXE_LINKER_FLAGS_RELEASE "-flto -Wl,-O1 -Wl,--as-needed -Wl,--strip-all")
   set(CMAKE_EXE_LINKER_FLAGS_MINSIZEREL "-flto -Wl,-O1 -Wl,--as-needed -Wl,--strip-all")
elseif(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
   # Clang specific settings
   add_compile_options(
      -Wall -Wextra -Wpedantic
      -Wno-unused-parameter
      -Wconversion
      -Wsign-conversion
   )
   # Clang Debug settings
   set(CMAKE_CXX_FLAGS_DEBUG "-g3 -O0 -fno-omit-frame-pointer -fno-optimize-sibling-calls")
   set(CMAKE_C_FLAGS_DEBUG "-g3 -O0 -fno-omit-frame-pointer -fno-optimize-sibling-calls")
   if(NOT WIN32)
      set(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG -march=native -mtune=native -ffast-math -funroll-loops -flto=thin")
      set(CMAKE_C_FLAGS_RELEASE "-O3 -DNDEBUG -march=native -mtune=native -ffast-math -funroll-loops -flto=thin")
   else()
      set(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG -ffast-math -funroll-loops")
      set(CMAKE_C_FLAGS_RELEASE "-O3 -DNDEBUG -ffast-math -funroll-loops")
   endif()
   set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "-O2 -g -DNDEBUG -fno-omit-frame-pointer")
   set(CMAKE_C_FLAGS_RELWITHDEBINFO "-O2 -g -DNDEBUG -fno-omit-frame-pointer")
   if(NOT WIN32)
      set(CMAKE_CXX_FLAGS_MINSIZEREL "-Oz -DNDEBUG -flto=thin")
      set(CMAKE_C_FLAGS_MINSIZEREL "-Oz -DNDEBUG -flto=thin")
   else()
      set(CMAKE_CXX_FLAGS_MINSIZEREL "-Oz -DNDEBUG")
      set(CMAKE_C_FLAGS_MINSIZEREL "-Oz -DNDEBUG")
   endif()
   # Enable sanitizers for Clang debug builds
   if(CMAKE_BUILD_TYPE STREQUAL "Debug" AND NOT WIN32)
      add_compile_options(-fsanitize=address -fsanitize=undefined)
      add_link_options(-fsanitize=address -fsanitize=undefined)
   endif()
   # Use libc++ on non-Windows platforms if available
   if(NOT WIN32)
      include(CheckCXXCompilerFlag)
      check_cxx_compiler_flag("-stdlib=libc++" HAS_LIBCXX)
      if(HAS_LIBCXX)
         add_compile_options(-stdlib=libc++)
         add_link_options(-stdlib=libc++)
      endif()
   endif()
   # Clang-specific linker flags for release builds (non-Windows)
   if(NOT WIN32)
      set(CMAKE_EXE_LINKER_FLAGS_RELEASE "-flto=thin -Wl,-O1")
      set(CMAKE_EXE_LINKER_FLAGS_MINSIZEREL "-flto=thin -Wl,-O1 -Wl,--strip-all")
   endif()
else()
   message(WARNING "Unknown compiler: ${CMAKE_CXX_COMPILER_ID}. Using generic settings.")
endif()

# Include FetchContent for automatic dependency management
include(FetchContent)

# Set the dependencies to be downloaded in parallel
set(FETCHCONTENT_QUIET FALSE)

# Platform-specific system libraries
if(!WIN32)
   # Unix-like systems (Linux, macOS)
   find_package(PkgConfig QUIET)
   # Linux-specific libraries
   if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
      find_package(X11)
      if(X11_FOUND)
         message(STATUS "X11 found - enabling X11 support")
      endif()
      # Check for Wayland
      if(PkgConfig_FOUND)
         pkg_check_modules(WAYLAND wayland-client)
         if(WAYLAND_FOUND)
            message(STATUS "Wayland found - enabling Wayland support")
         endif()
      endif()
   endif()
endif()

# Vulkan
find_package(Vulkan REQUIRED)
if(NOT Vulkan_FOUND)
   message(FATAL_ERROR "Vulkan SDK not found. Please install Vulkan SDK.")
endif()

# OpenGL
find_package(OpenGL REQUIRED)
if(NOT OpenGL_FOUND)
   message(FATAL_ERROR "OpenGL not found.")
endif()

# GLFW
FetchContent_Declare(
   glfw
   GIT_REPOSITORY https://github.com/glfw/glfw.git
   GIT_TAG 3.4
)
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(GLFW_INSTALL OFF CACHE BOOL "" FORCE)

# Platform-specific GLFW settings
if(WIN32)
   set(GLFW_USE_HYBRID_HPG ON CACHE BOOL "" FORCE)
endif()

FetchContent_MakeAvailable(glfw)

# GLM
FetchContent_Declare(
   glm
   GIT_REPOSITORY https://github.com/g-truc/glm.git
   GIT_TAG 1.0.1
)

FetchContent_MakeAvailable(glm)

# ASSIMP
FetchContent_Declare(
   assimp
   GIT_REPOSITORY https://github.com/assimp/assimp.git
   GIT_TAG v5.4.3
)
set(ASSIMP_WARNINGS_AS_ERRORS OFF CACHE BOOL "" FORCE)
set(ASSIMP_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(ASSIMP_BUILD_ASSIMP_TOOLS OFF CACHE BOOL "" FORCE)
set(ASSIMP_INSTALL OFF CACHE BOOL "" FORCE)
set(ASSIMP_BUILD_ALL_IMPORTERS_BY_DEFAULT OFF CACHE BOOL "" FORCE)
set(ASSIMP_BUILD_OBJ_IMPORTER ON CACHE BOOL "" FORCE)
set(ASSIMP_BUILD_GLTF_IMPORTER ON CACHE BOOL "" FORCE)
set(ASSIMP_BUILD_FBX_IMPORTER OFF CACHE BOOL "" FORCE)
set(ASSIMP_BUILD_PLY_IMPORTER OFF CACHE BOOL "" FORCE)
set(ASSIMP_BUILD_STL_IMPORTER OFF CACHE BOOL "" FORCE)
set(ASSIMP_BUILD_ZLIB ON CACHE BOOL "" FORCE)
set(ASSIMP_NO_EXPORT ON CACHE BOOL "" FORCE)

FetchContent_MakeAvailable(assimp)

# STB
FetchContent_Declare(
   stb
   GIT_REPOSITORY https://github.com/nothings/stb.git
   GIT_TAG master
)
FetchContent_MakeAvailable(stb)

# ImGui
FetchContent_Declare(
   imgui
   GIT_REPOSITORY https://github.com/ocornut/imgui.git
   GIT_TAG v1.91.5
)
FetchContent_MakeAvailable(imgui)

set(IMGUI_SOURCES
   ${imgui_SOURCE_DIR}/imgui.cpp
   ${imgui_SOURCE_DIR}/imgui_demo.cpp
   ${imgui_SOURCE_DIR}/imgui_draw.cpp
   ${imgui_SOURCE_DIR}/imgui_tables.cpp
   ${imgui_SOURCE_DIR}/imgui_widgets.cpp
   ${imgui_SOURCE_DIR}/backends/imgui_impl_glfw.cpp
   ${imgui_SOURCE_DIR}/backends/imgui_impl_opengl3.cpp
   ${imgui_SOURCE_DIR}/backends/imgui_impl_vulkan.cpp
)

add_library(imgui STATIC ${IMGUI_SOURCES})
target_include_directories(imgui PUBLIC
   ${imgui_SOURCE_DIR}
   ${imgui_SOURCE_DIR}/backends
)

if(WIN32)
   target_link_libraries(imgui PUBLIC glfw OpenGL::GL Vulkan::Vulkan)
else()
   target_link_libraries(imgui PUBLIC glfw OpenGL::GL Vulkan::Vulkan)
   if(CMAKE_SYSTEM_NAME STREQUAL "Linux" AND X11_FOUND)
      target_link_libraries(imgui PUBLIC ${X11_LIBRARIES})
   endif()
endif()

target_compile_definitions(imgui PUBLIC
   IMGUI_IMPL_OPENGL_LOADER_GLAD
   $<$<CONFIG:Debug>:IMGUI_DEBUG_PARANOID>
)

# Create STB library wrapper
add_library(stb INTERFACE)
target_include_directories(stb INTERFACE ${stb_SOURCE_DIR})

# Source files
add_executable(${PROJECT_NAME}
   src/main.cpp
)

# Link libraries
target_link_libraries(${PROJECT_NAME} PRIVATE
   glfw
   glm::glm
   assimp
   stb
   imgui
   Vulkan::Vulkan
   OpenGL::GL
)

# Include directories
target_include_directories(${PROJECT_NAME} PRIVATE
   ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_compile_definitions(${PROJECT_NAME} PRIVATE
   $<$<CONFIG:Debug>:DEBUG_BUILD>
   $<$<CONFIG:Debug>:_DEBUG>
   $<$<CONFIG:Release>:RELEASE_BUILD>
   $<$<CONFIG:Release>:NDEBUG>
   $<$<CONFIG:RelWithDebInfo>:RELEASE_BUILD>
   $<$<CONFIG:RelWithDebInfo>:NDEBUG>
   $<$<CONFIG:MinSizeRel>:RELEASE_BUILD>
   $<$<CONFIG:MinSizeRel>:NDEBUG>
   $<$<PLATFORM_ID:Windows>:WIN32_LEAN_AND_MEAN>
   $<$<PLATFORM_ID:Windows>:NOMINMAX>
   $<$<PLATFORM_ID:Windows>:_WIN32_WINNT=0x0601>
)

# Compiler-specific target options and optimizations
if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
   target_compile_options(${PROJECT_NAME} PRIVATE
      $<$<CONFIG:Debug>:/RTC1>
      $<$<CONFIG:Debug>:/JMC>
      $<$<CONFIG:Debug>:/ZI>
      $<$<CONFIG:Release>:/GL>
      $<$<CONFIG:Release>:/Gy>
      $<$<CONFIG:Release>:/GS->
      $<$<CONFIG:RelWithDebInfo>:/Gy>
      $<$<CONFIG:MinSizeRel>:/GL>
      $<$<CONFIG:MinSizeRel>:/Gy>
   )
   target_link_options(${PROJECT_NAME} PRIVATE
      $<$<CONFIG:Release>:/LTCG>
      $<$<CONFIG:Release>:/OPT:REF>
      $<$<CONFIG:Release>:/OPT:ICF>
      $<$<CONFIG:Debug>:/INCREMENTAL>
      $<$<CONFIG:MinSizeRel>:/LTCG>
      $<$<CONFIG:MinSizeRel>:/OPT:REF>
      $<$<CONFIG:MinSizeRel>:/OPT:ICF>
   )
elseif(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
   target_compile_options(${PROJECT_NAME} PRIVATE
      $<$<CONFIG:Debug>:-fno-limit-debug-info>
      $<$<CONFIG:Release>:-fvectorize>
      $<$<CONFIG:Release>:-fslp-vectorize>
      $<$<CONFIG:RelWithDebInfo>:-fvectorize>
   )
   if(NOT WIN32)
      target_link_options(${PROJECT_NAME} PRIVATE
         $<$<CONFIG:Release>:-flto=thin>
         $<$<CONFIG:MinSizeRel>:-flto=thin>
      )
   endif()
elseif(CMAKE_CXX_COMPILER_ID MATCHES "GNU")
   target_compile_options(${PROJECT_NAME} PRIVATE
      $<$<CONFIG:Release>:-fvectorize>
      $<$<CONFIG:Release>:-ftree-vectorize>
      $<$<CONFIG:Release>:-fipa-pta>
      $<$<CONFIG:RelWithDebInfo>:-fvectorize>
   )
   target_link_options(${PROJECT_NAME} PRIVATE
      $<$<CONFIG:Release>:-flto>
      $<$<CONFIG:MinSizeRel>:-flto>
   )
endif()

if(WIN32 AND CMAKE_GENERATOR MATCHES "Visual Studio")
    set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT ${PROJECT_NAME})
    set_target_properties(${PROJECT_NAME} PROPERTIES
        VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/$<CONFIG>"
        VS_DEBUGGER_COMMAND_ARGUMENTS ""
        VS_DEBUGGER_ENVIRONMENT "PATH=%PATH%"
    )
    source_group("Source Files" FILES ${CMAKE_CURRENT_SOURCE_DIR}/src/main.cpp)
    set_property(GLOBAL PROPERTY USE_FOLDERS ON)
endif()
