# PlanetRenderer

![C++20](https://img.shields.io/badge/C%2B%2B-20-00599C?logo=cplusplus&logoColor=white)
![OpenGL 4.4](https://img.shields.io/badge/OpenGL-4.4-5586A4?logo=opengl&logoColor=white)
![GLSL 430](https://img.shields.io/badge/GLSL-430-5586A4)
![CMake](https://img.shields.io/badge/build-CMake-064F8C?logo=cmake&logoColor=white)

A real-time Earth–Moon system renderer written in C++ and modern OpenGL. It combines hierarchical orbital motion, two-pass shadow mapping with eclipses, and a multi-texture Earth shader with a day/night cycle, ocean-only specular highlights, city lights and a separate cloud layer.

Developed for CENG 477 (Introduction to Computer Graphics).

## Contents

- [Features](#features)
- [Requirements](#requirements)
- [Building and Running](#building-and-running)
- [Controls](#controls)
- [Rendering Pipeline](#rendering-pipeline)
- [Project Structure](#project-structure)
- [Third-Party Libraries](#third-party-libraries)

## Features

**Scene and animation**
- Hierarchical scene: the Earth sits at the origin, the Moon orbits the Earth, and a smaller satellite ("MoonMoon") orbits the Moon. Each body's position is computed relative to its parent every frame.
- Independent orbit and self-rotation speeds per body, driven by a global simulation clock that can be sped up, slowed down or reversed at runtime.
- A sun light direction that revolves around the system, with a sun sprite drawn along that direction.
- A Milky Way starfield drawn behind the scene.

**Lighting and shadows**
- Two-pass shadow mapping from an orthographic sun projection into a 4096 × 4096 shadow map.
- Eclipses: the Moon and MoonMoon cast shadows onto the Earth, and the Earth shadows its satellites.
- Slope-scaled depth bias to suppress shadow acne.
- Phong shading (ambient, diffuse, specular) on all bodies.

**Earth shading**
- Day/night blending across the terminator using `smoothstep` on the N·L term.
- Night-side city lights from a dedicated emission texture.
- Specular masking so that only oceans reflect sunlight.
- An alpha-blended cloud shell, slightly larger than the Earth, that rotates more slowly than the surface and receives lighting and shadows.

**Camera**
- Orbit camera that locks onto the Earth, the Moon or MoonMoon and travels with the target along its orbit.
- Free-flight camera with WASD movement.
- Mouse-drag look and scroll-wheel zoom in every mode.

## Requirements

| Requirement | Version |
| :--- | :--- |
| C++ compiler | C++20 (GCC, Clang or MSVC) |
| CMake | 3.10 or newer |
| GPU / driver | OpenGL 4.4 |
| Platform | Windows or Linux (Linux builds also link X11) |

> [!NOTE]
> macOS supports OpenGL only up to 4.1, so it cannot create the 4.4 context this project requests.

All other dependencies are vendored under [`ext/`](ext/) and built automatically with the project.

## Building and Running

From the project root:

```bash
cmake -S . -B build
cmake --build build
```

The executable is written to [`working_dir/`](working_dir/). Shaders, meshes and textures are loaded through paths relative to that folder, so run it from there:

```bash
cd working_dir
./PlanetRenderer        # PlanetRenderer.exe on Windows
```

When the project is opened in Visual Studio, the debugger's working directory is already set to `working_dir`.

## Controls

| Input | Action |
| :--- | :--- |
| <kbd>P</kbd> | Next camera: Earth → Moon → MoonMoon → Free |
| <kbd>O</kbd> | Previous camera |
| Left mouse button + drag | Look around / orbit the target |
| Scroll wheel | Zoom in and out |
| <kbd>W</kbd> <kbd>A</kbd> <kbd>S</kbd> <kbd>D</kbd> | Move (free camera only) |
| <kbd>L</kbd> | Increase simulation speed by 0.5× |
| <kbd>K</kbd> | Decrease simulation speed by 0.5× (values below zero run time backwards) |

## Rendering Pipeline

Each frame is rendered in two passes.

### 1. Shadow pass

The Earth, the Moon and MoonMoon are drawn from the sun's point of view with an orthographic projection into an off-screen framebuffer. Fragment depth is written to a 4096 × 4096 `GL_R32F` color attachment, with a depth renderbuffer handling depth testing.

### 2. Main pass

1. The starfield and the sun sprite are drawn with depth testing disabled.
2. The planets are drawn with a perspective camera. Each fragment is transformed into light space with the light's view-projection matrix and compared against the shadow map. A bias of `max(0.005 · (1 − N·L), 0.005)` is subtracted from the fragment's light-space depth before the comparison.
3. The cloud shell is drawn last with alpha blending (`GL_SRC_ALPHA`, `GL_ONE_MINUS_SRC_ALPHA`).

### Shaders

The project uses a single vertex shader ([`generic.vert`](working_dir/shaders/generic.vert)) and a single fragment shader ([`debug.frag`](working_dir/shaders/debug.frag)), bound through an OpenGL program pipeline. A `uMode` uniform selects the shading path:

| Mode | Used for |
| :---: | :--- |
| 0 | Sun sprite (flat white) |
| 3 | Starfield (unlit texture) |
| 4 | Shadow pass (writes depth) |
| 5 | Moon and MoonMoon (Phong with shadows) |
| 6 | Earth (day/night, specular mask, city lights, shadows) |
| 7 | Clouds (lit, shadowed, alpha from texture) |

Modes 1 and 2 output vertex normals and UV coordinates as colors for debugging.

The Earth shader samples four textures:

| Binding | Sampler | Contents |
| :---: | :--- | :--- |
| 0 | `tAlbedo` | Daytime surface color |
| 1 | `tSpecular` | Grayscale reflectivity mask (oceans) |
| 2 | `tNight` | City lights |
| 3 | `tShadow` | Shadow map from pass 1 |

## Project Structure

```
PlanetRenderer/
├── CMakeLists.txt          Build configuration and compiler warnings
├── src/
│   ├── main.cpp            Scene setup, input handling, simulation and render loop
│   ├── utility.h           GLState, ShaderGL, MeshGL and TextureGL wrappers
│   └── utility.cpp         Window/context creation, OBJ, shader and texture loading
├── working_dir/            Runtime directory (the executable is built here)
│   ├── shaders/            GLSL vertex and fragment shaders
│   ├── meshes/             Sphere meshes (OBJ) at several resolutions
│   └── textures/           Earth, Moon, Jupiter and starfield textures
└── ext/                    Vendored third-party libraries
```

## Third-Party Libraries

| Library | Purpose |
| :--- | :--- |
| [GLFW](https://www.glfw.org/) | Window, OpenGL context and input |
| [glad](https://github.com/Dav1dde/glad) | OpenGL function loader |
| [GLM](https://github.com/g-truc/glm) | Vector and matrix math |
| [stb_image](https://github.com/nothings/stb) | Image loading |
