# Developer & Build Guide

Step-by-step instructions for setting up the native toolchain, compiling from source, running automated unit tests, and packaging Mem-Map for Windows distribution.

---

## 1. Prerequisites & Toolchain Setup
Mem-Map requires a modern C++20 compiler and Qt 6.6+. On Windows, the recommended build environment is **MSYS2 UCRT64**.

### Required Packages
- **Compiler**: GCC 13.2+ (UCRT64)
- **Build System**: CMake 3.28+ and Ninja 1.11+
- **Framework**: Qt 6.6+ Base (`qt6-base`)
- **Shell**: PowerShell or Windows Terminal

### MSYS2 Installation Command
Execute inside the MSYS2 UCRT64 terminal:
```bash
pacman -S --needed mingw-w64-ucrt-x86_64-gcc \
                   mingw-w64-ucrt-x86_64-cmake \
                   mingw-w64-ucrt-x86_64-ninja \
                   mingw-w64-ucrt-x86_64-qt6-base
```

---

## 2. Environment Variables Configuration
To build from PowerShell, inject MSYS2 UCRT64 into your session `PATH`:
```powershell
$env:PATH = "C:\msys64\ucrt64\bin;C:\msys64\usr\bin;" + $env:PATH
```

---

## 3. Building from Source

### Step 1: Clone Repository
```bash
git clone https://github.com/SuryanshSwarn09/Mem-Map.git
cd Mem-Map
```

### Step 2: Configure Build Directory
```powershell
$env:PATH = "C:\msys64\ucrt64\bin;C:\msys64\usr\bin;" + $env:PATH
cmake -B build -G "Ninja" -DCMAKE_BUILD_TYPE=Release
```

### Step 3: Compile Binaries
```powershell
cmake --build build
```
The compiled executables are output to:
- `build/Mem-Map.exe` (Main Desktop Application)
- `build/test_runner.exe` (Automated Test Suite)

---

## 4. Running the Application
Directly invoke via PowerShell or double-click `run.bat`:
```powershell
$env:PATH = "C:\msys64\ucrt64\bin;C:\msys64\usr\bin;" + $env:PATH
.\build\Mem-Map.exe
```

---

## 5. Standalone Packaging & Distribution
To package `Mem-Map.exe` with all necessary Qt runtime DLLs for distribution to users without MSYS2:
```powershell
$env:PATH = "C:\msys64\ucrt64\bin;C:\msys64\usr\bin;" + $env:PATH
windeployqt --release --no-translations --compiler-runtime build/Mem-Map.exe
```
This generates a fully self-contained folder that can be zipped or wrapped into an installer.
