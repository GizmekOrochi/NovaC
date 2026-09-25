cmake_minimum_required(VERSION 3.16)
project(NovaCPackageSmoke LANGUAGES CXX)

find_package(NovaC 1.2.0 EXACT CONFIG REQUIRED)

add_executable(novac-package-smoke main.cpp)
target_link_libraries(novac-package-smoke PRIVATE NovaC::NovaC)
