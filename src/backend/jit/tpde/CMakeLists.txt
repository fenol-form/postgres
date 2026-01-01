cmake_minimum_required(VERSION 3.25)
project(build-tpde-for-postgres)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS OFF)
set(CMAKE_POSITION_INDEPENDENT_CODE TRUE)

add_subdirectory(tpde)

set(TPDE_LOGGING ON)

add_library(tpdejit SHARED tpde_expr.cpp llvm_ir_gen.c llvmjit_error.cpp llvmjit_inline.cpp tpdejit.c)
target_compile_options(tpdejit PUBLIC -g -O0)
target_compile_definitions(tpdejit PUBLIC TPDE_LOGGING)

target_include_directories(tpdejit PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../../build/src/include
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../include
)

# LLVM
find_package(LLVM REQUIRED CONFIG)
target_compile_definitions(tpdejit PUBLIC ${LLVM_DEFINITIONS})
target_include_directories(tpdejit PUBLIC ${LLVM_INCLUDE_DIRS})
link_directories(${LLVM_LIBRARY_DIRS})
llvm_map_components_to_libnames(llvm_libs
    core
    analysis
    irreader
    support
    orcjit
    target
    orctargetprocess
    x86info
    x86targetmca
    nativecodegen # for LLVMInitializeNativeTarget
    x86asmparser  # for LLVMInitializeNativeAsmPrinter, LLVMInitializeNativeAsmParser
)
target_link_libraries(tpdejit PUBLIC ${llvm_libs})


# Link with tpde lib
target_link_libraries(tpdejit PUBLIC
    tpde
    tpde_llvm
)
