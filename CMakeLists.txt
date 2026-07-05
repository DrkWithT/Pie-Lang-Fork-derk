cmake_minimum_required(VERSION 3.30 FATAL_ERROR)

project(cherrypie LANGUAGES CXX)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if (DEFINED BUILD_RELEASE)
    message(NOTICE "Using release build for Pie")
    set(CMAKE_BUILD_TYPE "Release")
elseif (DEFINED BUILD_PROFILE)
    message(NOTICE "Using profiling build for Pie")
    set(CMAKE_BUILD_TYPE "RelWithDebInfo")
elseif (DEFINED BUILD_DEBUG)
    message(NOTICE "Using debug build for Pie")
    set(CMAKE_BUILD_TYPE "Debug")
else ()
    message(WARNING "No 'BUILD_<mode>' flag passed - Defaulted to debug build.")
    set(CMAKE_BUILD_TYPE "Debug")
endif ()

set(LOCAL_SRC_DIR "${CMAKE_SOURCE_DIR}/src/")
set(REMOTE_DEPS_DIR "${CMAKE_SOURCE_DIR}/_deps/")
set(DEPS_FETCHED "NOTFOUND" CACHE STRING "Whether the deps are installed for building Pie or not.")

find_file(VAR DEPS_FETCHED NAMES "cpp-std-extensions-src" "mp11-src" PATHS "${REMOTE_DEPS_DIR}")

message("Deps not found?\n${DEPS_FETCHED}\n\n")

if (NOT DEFINED DEPS_FETCHED OR DEPS_FETCHED MATCHES "\.*NOTFOUND")
    message(NOTICE "./build/_deps not found, installing cpp-std-extensions.")
    include(FetchContent)

    # already uses mp11 as sub-dependency
    FetchContent_Declare(
        cpp-std-extensions
        GIT_REPOSITORY https://github.com/intel/cpp-std-extensions.git
        GIT_TAG main
    )

    # NOTE: Omitted since fetched libffi lacks ffi.h??
    # FetchContent_Declare(
    #     libffi
    #     GIT_REPOSITORY https://github.com/libffi/libffi.git
    #     GIT_TAG master
    # )

    FetchContent_MakeAvailable(cpp-std-extensions)
else ()
    message(NOTICE "Installation of cpp-std-extensions was skipped since it's already installed.")
endif ()
        
set(REMOTE_DEP_INCLUDE_DIRS "${CMAKE_BINARY_DIR}/_deps/cpp-std-extensions-src/include;${CMAKE_BINARY_DIR}/_deps/mp11-src/include")

add_library(pie_frontend)
target_include_directories(pie_frontend PUBLIC ${REMOTE_DEP_INCLUDE_DIRS} "${LOCAL_SRC_DIR}")
target_sources(pie_frontend PRIVATE "${LOCAL_SRC_DIR}/Preprocessor/Preprocessor.cxx" "${LOCAL_SRC_DIR}/Lex/Lexer.cxx" "${LOCAL_SRC_DIR}/Analysis/LexicalScoping.cxx")

add_library(pie_old)
target_include_directories(pie_old PUBLIC ${REMOTE_DEP_INCLUDE_DIRS} "${LOCAL_SRC_DIR}")
target_sources(pie_old PRIVATE "${LOCAL_SRC_DIR}/Type/Type.cxx" "${LOCAL_SRC_DIR}/Value/Value.cxx")

add_library(pie_utils)
target_include_directories(pie_utils PUBLIC ${REMOTE_DEP_INCLUDE_DIRS} "${LOCAL_SRC_DIR}")
target_sources(pie_utils PRIVATE "${LOCAL_SRC_DIR}/Utils/CLI.cxx" "${LOCAL_SRC_DIR}/Utils/utils.cxx")

add_executable(Pie "${CMAKE_SOURCE_DIR}/src/main.cc")
target_include_directories(Pie PUBLIC ${REMOTE_DEP_INCLUDE_DIRS} "${LOCAL_SRC_DIR}")
target_link_directories(Pie PRIVATE "${CMAKE_BINARY_DIR}")
target_link_libraries(Pie PRIVATE pie_frontend PRIVATE pie_old PRIVATE pie_utils PRIVATE ffi)
