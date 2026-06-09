#pragma once

#include <iostream>
#include <fstream>
#include <filesystem>
#include <string>
#include <sstream>
#include <source_location>
#include <format>
#include <print>
#include <concepts>
#include <stdexcept>

#include "../Lex/Token.hxx"



#if defined(__APPLE__) or defined(__MACH__)
    #include <mach-o/dyld.h>
#elif defined(_WIN32) or defined(_WIN64)
    #include <windows.h>
#elif defined(__linux__)
    #include <unistd.h>
#else
    #error "unkown operating system!"
#endif


inline namespace pie {
namespace util {


template <typename Except = std::runtime_error, bool print_loc = true>
[[noreturn]] inline void error(
    const std::string_view msg = "[no diagnostic]. If you see this, please file a bug report!",
    [[maybe_unused]] const std::source_location& location = std::source_location::current()
)
{
    #if not NO_ERR_LOC
    if constexpr (print_loc)
        std::print(std::cerr, "\033[1m{}:{}:{}: \033[31merror:\033[0m ", location.file_name(), location.line(), location.column());
    #endif


    throw Except{std::string{msg}};
}


[[noreturn]] inline void error(const std::source_location& location)
{
    error("[no diagnostic]. If you see this, please file a bug report!", location);
}


[[noreturn]] inline void expected(const TokenKind exp, const Token& got, const std::source_location& location = std::source_location::current()) {
    error(std::string{"Expected token "} + stringify(exp) + " and found " + stringify(got.kind) + ": " + got.text, location);
}

[[noreturn]] inline void expected(const TokenKind exp, const TokenKind got, const std::source_location& location = std::source_location::current()) {
    error(std::string{"Expected token "} + stringify(exp) + " and found " + stringify(got), location);
}

[[noreturn]] inline void expected(const std::string& exp, const Token& got, const std::source_location& location = std::source_location::current()) {
    error(std::string{"Expected '"} + exp + "' and found " + stringify(got.kind) + ": " + got.text, location);
}



inline std::filesystem::path getPiePath() {
    constexpr auto MAX_PATH_SIZE = 1024;
    char buffer[MAX_PATH_SIZE];


#if defined(__APPLE__) or defined(__MACH__)

    uint32_t size = sizeof(buffer);
    if (_NSGetExecutablePath(buffer, &size) == 0) {
        return std::filesystem::canonical(buffer).parent_path(); 
    }

#elif defined(_WIN32) or defined(_WIN64)

    DWORD length = GetModuleFileNameA(NULL, buffer, MAX_PATH_SIZE); 

    if (length > 0 && length < sizeof(buffer)) {
        return std::filesystem::path(buffer).parent_path();
    }

#elif defined(__linux__)

    ssize_t length = readlink("/proc/self/exe", buffer, sizeof(buffer) - 1);

    if (length != -1) {
        buffer[length] = '\0';
        return std::filesystem::path(buffer).parent_path();
    }

#endif

    error();
}



template <typename F>
struct Deferred {
    F f;

    Deferred(std::invocable auto func) : f{std::move(func)} {};
    ~Deferred() { f(); }
};

template <typename F>
Deferred(F) -> Deferred<F>;




[[nodiscard]] inline std::string readFile(const std::filesystem::path& fname, const std::source_location& location = std::source_location::current()) {
    const std::ifstream fin{fname};

    if (not fin.is_open()) error("File \"" + fname.string() + "\" not found!", location);

    std::stringstream ss;
    ss << fin.rdbuf();

    return ss.str();
}


} // namespace util
} // namespace pie