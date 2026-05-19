cmake_minimum_required(VERSION 3.30)

project(pielang LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(PIE_LANG_SRC_DIR "${CMAKE_SOURCE_DIR}")

message(WARNING "Currently, this build only creates the Pie CLI.")

# TODO: implement installation of Catch2 for Tests/Test.cc program
set(PIE_LANG_REMOTE_INCLUDE_DIR "${PIE_LANG_SRC_DIR}/remote_includes")
set(PIE_LANG_BOOST_MP11_DIR "${PIE_LANG_REMOTE_INCLUDE_DIR}/mp11/include")
set(PIE_LANG_INTEL_EXTENSIONS_DIR "${PIE_LANG_REMOTE_INCLUDE_DIR}/cpp-std-extensions/include")

if (NOT DEFINED CMAKE_BUILD_TYPE)
    message(WARNING "CMAKE_BUILD_TYPE not provided, defaulting to 'Release'. Use -DCMAKE_BUILD_TYPE=<mode> to customize it.")
    set(CMAKE_BUILD_TYPE "Release")
endif ()

add_library(pie_tree_interp)
target_include_directories(pie_tree_interp PUBLIC ${PIE_LANG_SRC_DIR} PUBLIC ${PIE_LANG_BOOST_MP11_DIR} PUBLIC ${PIE_LANG_INTEL_EXTENSIONS_DIR})
target_sources(pie_tree_interp PRIVATE
    "${PIE_LANG_SRC_DIR}/Type/Type.cxx"
    "${PIE_LANG_SRC_DIR}/Interp/Value.cxx"
)

add_library(pie_cherry_vm)
target_include_directories(pie_cherry_vm PUBLIC ${PIE_LANG_SRC_DIR})
target_sources(pie_cherry_vm PRIVATE
    "${PIE_LANG_SRC_DIR}/VM/Compiler.cxx"
    "${PIE_LANG_SRC_DIR}/VM/Environment.cxx"
    "${PIE_LANG_SRC_DIR}/VM/Functions.cxx"
    "${PIE_LANG_SRC_DIR}/VM/Cherry.cxx"
)

add_executable(cherrypie main.cc)
target_include_directories(cherrypie PRIVATE ${PIE_LANG_SRC_DIR} PRIVATE ${PIE_LANG_REMOTE_INCLUDE_DIR} PRIVATE ${PIE_LANG_INTEL_EXTENSIONS_DIR})
target_link_directories(cherrypie PRIVATE ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})
target_link_libraries(cherrypie PRIVATE pie_tree_interp PRIVATE pie_cherry_vm)

message(WARNING "Pie CLI (with EXPERIMENTAL VM) built at ${PROJECT_BINARY_DIR}/cherrypie")
