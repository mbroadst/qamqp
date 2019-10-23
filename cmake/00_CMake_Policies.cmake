# from CMake 3.1 on: we want the NEW interpretation in of variables
cmake_policy(SET CMP0054 NEW)

# if the generator supports it, generate build_commands.json
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
