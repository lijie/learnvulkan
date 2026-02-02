# LearnVulkan Project

## Project Overview

This is a comprehensive Vulkan learning project that demonstrates various Vulkan graphics programming concepts through a series of progressive examples. The project is structured as a C++ application using CMake as the build system and targets Windows platforms with Vulkan SDK support.

**Key Technologies:**
- **Graphics API:** Vulkan
- **Windowing:** GLFW
- **UI:** Dear ImGui
- **3D Math:** GLM
- **Model Loading:** tinygltf
- **Build System:** CMake
- **Language:** C++20

**Architecture:** The project follows a modular architecture with a base library (`src/base/`) containing core Vulkan abstractions and individual example applications in numbered directories (`src/00-test/` through `src/11-pbr-ibl/`).

## Building and Running

### Prerequisites
- **Vulkan SDK** (1.3.250.0 or compatible)
- **CMake** (3.12 or higher)
- **C++20 compatible compiler** (MSVC, Clang, or GCC)
- **GLFW** (included in deps folder)
- **Python** (for debugging with lldb)

### Build Commands
```bash
# Configure the project
cmake -B build -DCMAKE_EXPORT_COMPILE_COMMANDS=1

# Build all targets
cmake --build build --config Release

# Build specific example
cmake --build build --target 01-drawing-a-triangle
```

### Running Examples
Each example is built as a separate executable:
```bash
# Run triangle drawing example
./build/01-drawing-a-triangle.exe

# Run PBR example
./build/10-pbr-basic.exe
```

### Shader Compilation
Shaders are compiled using `glslc` from the Vulkan SDK:
```bash
# Compile all shaders
compile_shader.bat

# Or compile individually
C:\VulkanSDK\1.3.250.0\Bin\glslc.exe src/01-drawing-a-triangle/01-shader.vert -o build/01-vert.spv
```

## Project Structure

### Core Components (`src/base/`)
- **vulkan_app.h/cc**: Main application framework with GLFW integration
- **vulkan_context.h/cc**: Core Vulkan context management
- **vulkan_device.h/cc**: Device abstraction and queue management
- **vulkan_buffer.h/cc**: Buffer management utilities
- **vulkan_pipelinebuilder.h/cc**: Pipeline creation utilities
- **scene.h/cc**: Scene graph and object management
- **camera.h/cc**: Camera controls and transformations
- **material.h/cc**: Material system for PBR rendering

### Example Progression
1. **00-test**: Basic Vulkan instance creation
2. **01-drawing-a-triangle**: Simple triangle rendering
3. **02-vertex-buffer**: Vertex buffer usage
4. **03-uniform-buffer**: Uniform buffer implementation
5. **04-texture**: Texture mapping
6. **05-base-triangle**: Base framework integration
7. **06-two-cubes**: Multiple object rendering
8. **07-two-textures**: Multiple texture handling
9. **08-move-camera**: Camera controls
10. **09-gltf**: GLTF model loading
11. **10-pbr-basic**: Basic PBR rendering
12. **11-pbr-ibl**: Image-based lighting PBR

### Dependencies (`deps/`)
- **glfw-3.3.8.bin.WIN64**: Pre-built GLFW libraries
- **imgui**: Dear ImGui for UI rendering
- **stb**: Image loading utilities
- **gltf**: tinygltf for GLTF model loading

## Development Conventions

### Code Style
- **C++20** features are used throughout
- **CamelCase** for class names and PascalCase for methods
- **snake_case** for variables and file names
- **Header guards** using `#pragma once`
- **Namespacing**: Core components are in `lvk` namespace

### Vulkan Best Practices
- **Validation layers** enabled in debug builds
- **Command buffer pooling** for efficient resource management
- **Descriptor set caching** to avoid redundant allocations
- **Pipeline state objects** cached and reused
- **Memory alignment** considerations for uniform buffers

### Project-Specific Conventions
- **Shader files** are named with example number prefixes (e.g., `01-shader.vert`)
- **SPIR-V shaders** are compiled to `build/` directory
- **Asset files** are stored in `assets/` directory
- **Models** use GLTF format with binary optimization

## Debugging and Development

### LLDB Setup
For debugging with LLDB:
```bash
set PYTHONPATH=C:\Program Files\WindowsApps\PythonSoftwareFoundation.Python.3.10_3.10.3056.0_x64__qbz5n2kfra8p0\lib
lldb build/01-drawing-a-triangle.exe
```

### Clangd Configuration
Generate compile commands for IDE support:
```bash
cmake -B build -DCMAKE_EXPORT_COMPILE_COMMANDS=1
```

### RenderDoc Integration
RenderDoc capture is enabled via `ENABLE_RENDERDOC` macro. Use the provided methods in `VulkanApp` class for capture control.

## Key Implementation Notes

### Coordinate System
- **Right-handed coordinate system** with Y-up, Z-forward, X-right
- **Camera looks along +Z direction**
- **GLM forced to use depth [0,1]** via `GLM_FORCE_DEPTH_ZERO_TO_ONE`

### Vertex Data Handling
- Supports both **interleaved** and **non-interleaved** vertex data layouts
- GLTF models typically use non-interleaved format

### Vulkan Initialization Flow
1. Create Vulkan instance with validation layers
2. Setup debug messenger for validation output
3. Create surface for window integration
4. Select physical device and create logical device
5. Initialize swap chain for presentation
6. Create render passes and framebuffers
7. Build pipelines with shader modules
8. Allocate command buffers and synchronization primitives

### Memory Management
- Uses **Vulkan memory allocator** patterns for efficient resource management
- **Descriptor sets** are cached to avoid redundant allocations
- **Uniform buffers** use dynamic offsets for per-object data

## Testing and Validation

Each example application includes comprehensive error handling and validation layer integration. The project follows Vulkan best practices for:
- **Resource lifetime management**
- **Synchronization** between GPU and CPU
- **Memory barriers** for resource transitions
- **Pipeline barriers** for correct rendering order

## Contributing

When adding new features or examples:
1. Follow the existing naming conventions and directory structure
2. Include appropriate error handling and validation
3. Update the CMakeLists.txt to include new targets
4. Add shader compilation commands to compile_shader.bat
5. Test on multiple Vulkan implementations if possible

## Troubleshooting

Common issues and solutions are documented in `note.md`, including:
- Python DLL path configuration for LLDB
- Clangd setup for code completion
- Vulkan coordinate system adjustments
- Memory alignment requirements for uniform buffers