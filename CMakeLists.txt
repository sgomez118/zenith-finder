cmake_minimum_required(VERSION 4.0.0)

project(zenith-finder LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find SuperNOVAS and CLI11
# vcpkg usually exposes them. Trying find_package first, falling back to library search.
find_package(supernovas CONFIG REQUIRED)
find_package(calceph CONFIG REQUIRED)
find_package(CLI11 CONFIG REQUIRED)
find_package(ftxui CONFIG REQUIRED)
find_package(tomlplusplus CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)
find_package(Catch2 3 CONFIG REQUIRED)

enable_testing()

add_subdirectory(libengine)
add_subdirectory(app)
add_subdirectory(test)
