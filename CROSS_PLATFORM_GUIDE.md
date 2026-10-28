# Cross-Platform Build & Run Guide

This guide provides step-by-step instructions for building and running the **Quantum Hydrogen Atom & Orbital Simulator** on **Linux**, **Windows**, and **macOS**.

---

## Table of Contents
1. [Linux (Ubuntu, Debian, Fedora, Arch Linux)](#1-linux)
2. [Windows (Visual Studio 2022 & MSVC / vcpkg)](#2-windows-msvc--vcpkg)
3. [Windows (MSYS2 / MinGW-w64)](#3-windows-msys2--mingw-w64)
4. [macOS (Apple Silicon & Intel)](#4-macos)
5. [Common Troubleshooting & FAQs](#5-troubleshooting--faqs)

---

## 1. Linux

### System Prerequisites
Modern C++ compiler (`g++` 9+ or `clang++`), `cmake` 3.16+, OpenGL development headers, GLFW3, GLEW, and GLM.

#### Ubuntu / Debian / Linux Mint / Pop!_OS
```bash
sudo apt update
sudo apt install -y build-essential cmake libglfw3-dev libglm-dev libglew-dev libgl1-mesa-dev
```

#### Fedora / RHEL / AlmaLinux
```bash
sudo dnf install -y gcc-c++ cmake glfw-devel glm-devel glew-devel mesa-libGL-devel
```

#### Arch Linux / Manjaro
```bash
sudo pacman -S base-devel cmake glfw-x11 glm glew
```

### Build & Run
```bash
# 1. Clone repository (with submodules for ImGui)
git clone https://github.com/subhobhai943/atom-orbital-sim.git
cd atom-orbital-sim

# 2. Configure & compile
mkdir -p build && cd build
cmake ..
cmake --build . -j$(nproc)

# 3. Launch the unified master simulator
./../bin/atom_orbital_sim
```

*(Or launch the terminal menu selector via `./../bin/launcher`)*

---

## 2. Windows (MSVC & vcpkg)

Recommended for developers using Visual Studio 2022.

### Step 1: Install `vcpkg` (Package Manager)
Open PowerShell:
```powershell
git clone https://github.com/microsoft/vcpkg.git C:\vcpkg
cd C:\vcpkg
.\bootstrap-vcpkg.bat
```

### Step 2: Install Required Libraries
```powershell
.\vcpkg install glfw3:x64-windows glew:x64-windows glm:x64-windows
```

### Step 3: Configure and Build with CMake
From the `atom-orbital-sim` root folder:
```powershell
# Configure CMake using vcpkg toolchain
cmake -B build -S . -DCMAKE_TOOLCHAIN_FILE="C:\vcpkg\scripts\buildsystems\vcpkg.cmake"

# Build in Release mode
cmake --build build --config Release
```

### Step 4: Run
```powershell
.\bin\Release\atom_orbital_sim.exe
```
*(Note: If DLLs like `glfw3.dll` or `glew32.dll` are required in `bin\Release\`, vcpkg will automatically deploy them during the build).*

---

## 3. Windows (MSYS2 / MinGW-w64)

Ideal for developers who prefer a lightweight GCC/GNU workflow on Windows without installing Visual Studio.

### Step 1: Install MSYS2
Download and install MSYS2 from [msys2.org](https://www.msys2.org/).

### Step 2: Install Toolchain in MSYS2 UCRT64 Terminal
Launch the **MSYS2 UCRT64** environment and run:
```bash
pacman -Syu
pacman -S mingw-w64-ucrt-x86_64-gcc \
          mingw-w64-ucrt-x86_64-cmake \
          mingw-w64-ucrt-x86_64-ninja \
          mingw-w64-ucrt-x86_64-glfw \
          mingw-w64-ucrt-x86_64-glew \
          mingw-w64-ucrt-x86_64-glm
```

### Step 3: Build & Run
```bash
cd /path/to/atom-orbital-sim
cmake -B build -G "Ninja"
cmake --build build

./bin/atom_orbital_sim.exe
```

---

## 4. macOS (Apple Silicon M1/M2/M3/M4 & Intel)

macOS natively supports OpenGL 3.3 Core Profile via Apple's built-in framework.

### Step 1: Install Dependencies via Homebrew
Open Terminal:
```bash
# Install Homebrew if you don't have it (https://brew.sh)
brew install cmake glfw glew glm
```

### Step 2: Build with CMake
```bash
cd atom-orbital-sim
mkdir -p build && cd build
cmake ..
cmake --build . -j$(sysctl -n hw.ncpu)
```

### Step 3: Run
```bash
./../bin/atom_orbital_sim
```

*Note for macOS users: High-DPI Retina screens are natively supported by the simulator's frame-buffer query (`glfwGetFramebufferSize`), ensuring crystal-sharp rendering.*

---

## 5. Troubleshooting & FAQs

### Q: It says `Shader File Error: Failed to read shaders/...`
**A**: Shaders are automatically copied to `bin/shaders/` upon build. Furthermore, all core renderers feature **embedded fallback shader strings directly compiled into the binary**, ensuring the application still runs even if relative shader paths are displaced.

### Q: Blank / Black screen on Linux Virtual Machines or Docker
**A**: Ensure 3D graphics acceleration is enabled in your VM manager (VirtualBox / VMware). You can force software OpenGL rendering with Mesa llvmpipe:
```bash
LIBGL_ALWAYS_SOFTWARE=1 ./bin/atom_orbital_sim
```

### Q: How do I hide the control panel for clean full-screen screenshots/recordings?
**A**: 
- Press **`TAB`** or **`H`** on your keyboard to instantly hide/show the control panel.
- Press **`F11`** (or **`F`**) to toggle true borderless monitor fullscreen.
- Click **`[ ▾ Mini-Bar HUD ]`** in the control panel to collapse the inspector into a slim 40px top toolbar.

### Q: On Windows, it complains about missing `glew32.dll` or `glfw3.dll`
**A**: When using MinGW or manual builds, copy the `.dll` files from your compiler's `bin/` directory into the folder containing `atom_orbital_sim.exe`.
