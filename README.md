# Vulkyrie Game Engine

> [!WARNING]
> This project is a work in progress.

## How to build from source

Make sure you have the following prerequisites installed.
- A C/C++ compiler, __(GCC v15.2.0+ or MSVC v19+ or Clang v20+ is recommended)__.
- [Ninja](https://github.com/ninja-build/ninja/releases/tag/v1.13.2)

Run the following commands from the root project directory:
```
mkdir build
cmake ..
cmake --build .
```

Then you can run the project by running the executables under

- Sandbox Application.
```
(On Linux)
cd build/examples/sandbox && ./sandbox

(On Windows)
cd build/examples/sandbox/Debug && ./sandbox.exe
```

- Asteroids Application.
```
(On Linux)
cd build/examples/asteroids && ./asteroids

(On Windows)
cd build/examples/asteroids/Debug && ./asteroids.exe
```