# -------------------------------------
# Global configuration
# -------------------------------------
Cmake_Minimum_Required(VERSION 3.27)
Project(vulkyrie)
Set(CMAKE_CXX_STANDARD 23)
Set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# -------------------------------------
# Include dependencies configuration
# -------------------------------------
Include(Dependencies.cmake)

# -------------------------------------
# Build the Vulkyrie Engine.
# -------------------------------------
Add_Subdirectory(engine)

# -------------------------------------
# Build the Vulkyrie CLI tool.
# -------------------------------------
# Add_Subdirectory(vulky-cli)

# -------------------------------------
# Examples/Games based on the Vulkyrie engine
# -------------------------------------
# Add_Subdirectory(examples/asteroids)
Add_Subdirectory(examples/sandbox)
