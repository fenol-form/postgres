cmake_minimum_required(VERSION 3.25)
project(build-tpde-for-postgres)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE TRUE)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_subdirectory(tpde)

set(TPDE_LOGGING ON)

install(TARGETS tpde tpde_llvm spdlog RUNTIME DESTINATION install)
