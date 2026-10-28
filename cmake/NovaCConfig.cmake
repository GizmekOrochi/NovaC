# NovaC CMake package configuration.
include("${CMAKE_CURRENT_LIST_DIR}/NovaCTargets.cmake")
# Available only after explicit `make install-compilation`.
include("${CMAKE_CURRENT_LIST_DIR}/NovaCCompilationTargets.cmake" OPTIONAL)
