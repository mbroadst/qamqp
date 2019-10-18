# update our CMake policies
include(${CMAKE_CURRENT_LIST_DIR}/00_CMake_Policies.cmake NO_POLICY_SCOPE)

# adding some helpers for CMake et.al.
include(${CMAKE_CURRENT_LIST_DIR}/01_git_Integration.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/06_CTest_Support.cmake)

# define the default build type
include(${CMAKE_CURRENT_LIST_DIR}/10_DefaultBuildType.cmake)

# find all needed libraries
include(${CMAKE_CURRENT_LIST_DIR}/20_FindDependencies.cmake)

# set global compiler switches, depending on compiler being used
include(${CMAKE_CURRENT_LIST_DIR}/50_CompilerSwitches.cmake)
