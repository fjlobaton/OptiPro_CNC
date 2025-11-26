## OptiPro_CNC

A CNC machine production optimizer prototype with a real-time dashboard built in C++ using Dear ImGui, SDL2, and OpenGL3.

## Requirements

- C++17-compatible compiler (GCC, Clang, or MSVC)
- CMake ≥ 3.31
- SDL2 (development libraries)
- OpenGL (development libraries)
- Git

## Installation

### Linux (Debian/Ubuntu)

```bash
sudo apt update
sudo apt install build-essential cmake git libsdl2-dev libgl1-mesa-dev
```

### Linux (Fedora/RHEL)

```bash
sudo dnf install gcc-c++ cmake git SDL2-devel mesa-libGL-devel
```

### Linux (Arch)

```bash
sudo pacman -S base-devel cmake git sdl2 mesa
```

### macOS

```bash
brew install cmake sdl2
```

### Windows

1. Install [Visual Studio](https://visualstudio.microsoft.com/) with C++ development tools
2. Install [CMake](https://cmake.org/download/)
3. Install [vcpkg](https://vcpkg.io/en/getting-started.html):
   ```cmd
   git clone https://github.com/Microsoft/vcpkg.git
   cd vcpkg
   .\bootstrap-vcpkg.bat
   .\vcpkg integrate install
   .\vcpkg install sdl2:x64-windows
   ```

## Building

```bash
# Clone the repository
git clone https://github.com/fjlobaton/OptiPro_CNC.git
cd OptiPro_CNC

# Create build directory
mkdir build
cd build

# Configure
cmake ..

# Build
cmake --build .
```

### Windows with vcpkg

```cmd
cmake .. -DCMAKE_TOOLCHAIN_FILE=<path-to-vcpkg>/scripts/buildsystems/vcpkg.cmake
cmake --build .
```

## Running

### Linux/macOS
## if running under WSL2 the next command needs to be executed so SDL2 bindings target X11
```bash
export SDL_VIDEODRIVER=x11
```

```bash
./OptiPro_CNC
```

### Windows

```cmd
.\Debug\OptiPro_CNC.exe
```
or
```cmd
.\Release\OptiPro_CNC.exe
```

## Project Structure

- `src/` - Source files
- `include/` - Header files
- `CMakeLists.txt` - Build configuration

Dear ImGui is automatically fetched and built via CMake's FetchContent.
