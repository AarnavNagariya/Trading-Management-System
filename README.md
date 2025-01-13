# System Requirements and Installation Guide

## Operating System Support
- Linux (Ubuntu 20.04+, CentOS 7+)
- macOS (10.15+)
- Windows (10/11 with MinGW or MSVC)

## Core Requirements

### 1. C++ Compiler
- GCC 8.3+ or
- Clang 10+ or
- MSVC 19.20+ (Visual Studio 2019+)

Installation:
```bash
# Ubuntu/Debian
sudo apt-get update
sudo apt-get install build-essential

# CentOS/RHEL
sudo yum groupinstall "Development Tools"

# macOS (using Homebrew)
xcode-select --install
```

### 2. CMake (3.15+)
Required for building the project.

```bash
# Ubuntu/Debian
sudo apt-get install cmake

# CentOS/RHEL
sudo yum install cmake3

# macOS
brew install cmake

# Windows
# Download installer from https://cmake.org/download/
```

## Required Libraries

### 1. libcurl (7.68.0+)
HTTP/HTTPS client library for API requests.

```bash
# Ubuntu/Debian
sudo apt-get install libcurl4-openssl-dev

# CentOS/RHEL
sudo yum install libcurl-devel

# macOS
brew install curl

# Windows
# Included in the project as a vcpkg dependency
```

### 2. OpenSSL (1.1.1+)
Required for WebSocket secure connections.

```bash
# Ubuntu/Debian
sudo apt-get install libssl-dev

# CentOS/RHEL
sudo yum install openssl-devel

# macOS
brew install openssl@1.1

# Windows
# Included in the project as a vcpkg dependency
```

### 3. Boost (1.71.0+)
Required for WebSocket implementation.

```bash
# Ubuntu/Debian
sudo apt-get install libboost-all-dev

# CentOS/RHEL
sudo yum install boost-devel

# macOS
brew install boost

# Windows
# Download from https://www.boost.org/users/download/
```

### 4. nlohmann/json (3.9.0+)
Modern JSON library for C++.

```bash
# Ubuntu/Debian
sudo apt-get install nlohmann-json3-dev

# CentOS/RHEL
# Build from source:
git clone https://github.com/nlohmann/json.git
cd json
mkdir build && cd build
cmake ..
make -j4
sudo make install

# macOS
brew install nlohmann-json

# Windows
# Included in the project as a vcpkg dependency
```

### 5. WebSocket++ (0.8.2+)
Header-only WebSocket library.

```bash
# Ubuntu/Debian
sudo apt-get install libwebsocketpp-dev

# CentOS/RHEL
# Build from source:
git clone https://github.com/zaphoyd/websocketpp.git
cd websocketpp
mkdir build && cd build
cmake ..
sudo make install

# macOS
brew install websocketpp

# Windows
# Included in the project as a vcpkg dependency
```

## Build Tools

### vcpkg (Windows Only)
Package manager for C++ libraries on Windows:

1. Clone vcpkg:
```batch
git clone https://github.com/Microsoft/vcpkg.git
cd vcpkg
.\bootstrap-vcpkg.bat
```

2. Install required packages:
```batch
vcpkg install curl:x64-windows
vcpkg install openssl:x64-windows
vcpkg install boost:x64-windows
vcpkg install nlohmann-json:x64-windows
vcpkg install websocketpp:x64-windows
```

## Build Instructions

1. Open the main project directory:

2. Create build directory (if it does not exist):
```bash
mkdir build
cd build
```

3. Configure with CMake:
```bash
# Linux/macOS
cmake ..

# Windows with vcpkg
cmake -DCMAKE_TOOLCHAIN_FILE=[path to vcpkg]/scripts/buildsystems/vcpkg.cmake ..
```

4. Build the project:
```bash
cmake --build . --config Release
```

## Verification

After installation, verify the setup:
1. All required libraries are properly linked
2. OpenSSL version is correct
3. Network connectivity is available
4. Compiler supports C++17 features

## Troubleshooting

Common issues and solutions:

1. OpenSSL version mismatch:
   - Ensure system OpenSSL matches the required version
   - Check library paths are correctly set

2. Boost not found:
   - Verify Boost installation path
   - Check environment variables

3. WebSocket++ compilation errors:
   - Ensure all Boost dependencies are installed
   - Verify compiler C++17 support

4. CURL SSL certificate issues:
   - Update CA certificates
   - Check SSL certificate paths

## Support

For additional support:
- Consult library-specific documentation
- Contact system administrators for environment-specific issues
- Report bugs and feature requests at this [email](mailto:nagariyaaarnav@gmail.com)