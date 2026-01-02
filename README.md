# 🪐 PlanetRenderer
PlanetRenderer is a real-time solar system simulation engine built with C++ and Modern OpenGL. Features advanced shading techniques including dynamic shadow mapping (eclipses), multi-textured Earth shading (day/night cycles, specular masking, clouds), and hierarchical planetary motion.

This project demonstrates advanced computer graphics concepts including hierarchical scene graphs, multi-pass shadow mapping, and complex fragment shading for realistic planetary rendering.

## ✨ Key Features

* **Real-Time Solar System:** Simulates orbital mechanics for the Sun, Earth, Moon, and nested moons ("MoonMoon") using a hierarchical matrix stack.
* **Dynamic Shadow Mapping:** * Implements a two-pass rendering pipeline to generate high-resolution shadow maps.
    * Supports dynamic self-shadowing and **planetary eclipses** (e.g., the Moon casting a shadow on Earth).
* **Advanced Earth Shading:**
    * **Day/Night Cycle:** seamless blending between day and night textures based on the sun's direction using `smoothstep`.
    * **Specular Masking:** Uses a specular map to ensure only oceans reflect sunlight, while landmasses remain matte.
    * **Night Lights:** City lights appear automatically on the dark side of the Earth.
    * **Atmosphere:** A separate, rotating cloud layer rendered with transparency.
* **Camera System:**
    * **Free Cam:** First-person style movement (WASD + Mouse).
    * **Orbital Cam:** Locks focus on specific planets while maintaining orbital velocity.

## 🛠️ Tech Stack

* **Language:** C++17
* **Graphics API:** OpenGL 4.3+ (Core Profile)
* **Shader Language:** GLSL 430
* **Dependencies:**
    * [GLFW](https://www.glfw.org/) (Windowing & Input)
    * [GLM](https://github.com/g-truc/glm) (Mathematics)
    * `stb_image` (Texture Loading)

## 🎮 Controls

| Key | Action |
| :--- | :--- |
| **W, A, S, D** | Move Camera (Free Mode) |
| **Mouse** | Rotate Camera View |
| **P** | Next Camera Target (Earth -> Moon -> Moonmoon -> Free Cam) |
| **O** | Previous Camera Target |
| **L** | Increase Simulation Speed |
| **K** | Decrease Simulation Speed |

## 🧠 Technical Details

### 1. The Shadow Pipeline
The engine uses a **Multi-Pass Rendering** approach to achieve shadows:
1.  **Shadow Pass:** The scene is rendered from the Sun's perspective into a custom Framebuffer Object (FBO) with a `GL_R32F` depth texture. Color writing is disabled to maximize performance.
2.  **Render Pass:** The scene is rendered from the camera's perspective. In the Fragment Shader, the pixel's coordinates are transformed into "Light Space" and compared against the depth map.

### 2. Earth Fragment Shader
The Earth is rendered using a unified shader (`debug.frag`) that combines four texture inputs:
* `T_ALBEDO`: The visible surface color.
* `T_NIGHT`: The texture showing city lights.
* `T_SPECULAR`: A grayscale map defining reflective surface areas (water).
* `T_SHADOW`: The dynamic shadow map generated in Pass 1.

The shader calculates the dot product of the Normal and Light vectors (`N dot L`) to determine the "terminator line" and mixes the Day and Night textures accordingly.

## 🚀 How to Build

This project relies on **CMake**.

1.  **Build the Project:**
    Open your terminal in the project root (where `CMakeLists.txt` is located) and run:
    ```bash
    cmake -B build . && make -C build
    ```

2.  **Run the Simulation:**
    You must navigate to the `working_dir` folder before running the executable so that relative paths (textures/shaders) are found correctly:
    ```bash
    cd working_dir
    ./PlanetRenderer
    ```

## 📜 License

This project is open-source and available under the [MIT License](LICENSE).
