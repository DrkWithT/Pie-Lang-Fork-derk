#pragma once

#include <utility>
#include <string>
#include <string_view>
#include <optional>
#include <expected>
#include <variant>
#include <chrono>
#include <filesystem>
#include <print>

#include "../Lex/Lexer.hxx"
#include "../Preprocessor/Preprocessor.hxx"
#include "../Parser/Parser.hxx"
#include "../Analysis/LexicalScoping.hxx"
#include "../VM/Compiler.hxx"
#include "../VM/Functions.hxx"
#include "../VM/Cherry.hxx"

inline namespace pie {
    namespace cherry {
        struct Config {
            std::string_view name;
            int local_capacity;
            int heap_capacity;
        };

        class Driver {
        private:
            vm::Compiler m_compiler;
            Config m_config;

        public:
            explicit constexpr Driver(Config config)
            : m_compiler {}, m_config {config} {}

            [[nodiscard]] constexpr const Config& info() const noexcept {
                return m_config;
            }

            [[maybe_unused]] constexpr bool add_builtin(std::string name, std::unique_ptr<vm::ObjectBase> fn) {
                return m_compiler.add_builtin_object(std::move(name), std::move(fn));
            }

            [[nodiscard]] constexpr std::optional<vm::Program> compile_program(const std::string& fname, bool dump) {
                auto src = util::readFile(fname);
                auto cloned_src = src;
                auto processed_src = std::move(src);

                Tokens tokens = lex::lex(std::move(processed_src));

                if (tokens.empty()) {
                    return {};
                }

                Parser p {std::move(tokens), fname};

                auto ast = p.parse();

                {
                    pie::analysis::LexicalAnalysis anal;
                    
                    for (const auto& [exprs, ops] = ast; const auto& expr : exprs) {
                        std::visit(anal, expr->variant());
                    }
                }

                if (auto program_result = m_compiler(ast, cloned_src)) {
                    return std::optional {std::move(program_result.value())};
                } else {
                    std::println(std::cerr, "{}", program_result.error());
                    return {};
                }
            }

            // todo: use VM 'Context' after compilation of fname's source text.
            [[nodiscard]] constexpr bool operator()(const std::string& fname, const bool dump) {
                auto program = compile_program(fname, dump);

                if (!program) {
                    return false;
                } else if (dump) {
                    display_all_bytecode(program.value());
                    return true;
                }

                auto run_ok = true;

                {
                    vm::VM cherry_engine {std::move(program.value()), m_config.local_capacity};

                    const auto start = std::chrono::steady_clock::now();
                    run_ok = cherry_engine.run();
                    const auto runtime = std::chrono::steady_clock::now() - start;

                    std::println(
                        "\x1b[1;33mRuntime:\x1b[0m {}\n\x1b[1;33mResult: {}\x1b[0m\n",
                        std::chrono::duration_cast<std::chrono::milliseconds>(runtime),
                        cherry_engine.result().to_string()
                    );
                }

                return run_ok;
            }
        };
    }

    namespace cli {
        [[nodiscard]] constexpr int runExperimental(const std::filesystem::path fname, const bool dump) {
            cherry::Driver driver {cherry::Config {
                .name = "Cherry (experimental Pie VM)",
                .local_capacity = 3072,
                .heap_capacity = 256
            }};

            driver.add_builtin("__builtin_neg", std::make_unique<vm::BuiltIn>(
                vm::pie_native_neg,
                "__builtin_neg",
                1
            ));

            driver.add_builtin("__builtin_add", std::make_unique<vm::BuiltIn>(
                vm::pie_native_add,
                "__builtin_add",
                2
            ));

            driver.add_builtin("__builtin_sub", std::make_unique<vm::BuiltIn>(
                vm::pie_native_sub,
                "__builtin_sub",
                2
            ));

            driver.add_builtin("__builtin_mul", std::make_unique<vm::BuiltIn>(
                vm::pie_native_mul,
                "__builtin_mul",
                2
            ));

            driver.add_builtin("__builtin_div", std::make_unique<vm::BuiltIn>(
                vm::pie_native_div,
                "__builtin_div",
                2
            ));

            driver.add_builtin("__builtin_eq", std::make_unique<vm::BuiltIn>(
                vm::pie_native_eq,
                "__builtin_eq",
                2
            ));

            driver.add_builtin("__builtin_ne", std::make_unique<vm::BuiltIn>(
                vm::pie_native_ne,
                "__builtin_ne",
                2
            ));

            driver.add_builtin("__builtin_lt", std::make_unique<vm::BuiltIn>(
                vm::pie_native_lt,
                "__builtin_lt",
                2
            ));

            driver.add_builtin("__builtin_gt", std::make_unique<vm::BuiltIn>(
                vm::pie_native_gt,
                "__builtin_gt",
                2
            ));

            driver.add_builtin("__builtin_not", std::make_unique<vm::BuiltIn>(
                vm::pie_native_logical_not,
                "__builtin_not",
                1
            ));

            driver.add_builtin("__builtin_print", std::make_unique<vm::BuiltIn>(
                vm::pie_native_print,
                "__builtin_print",
                0
            ));

            driver.add_builtin("__builtin_now", std::make_unique<vm::BuiltIn>(
                vm::pie_native_now,
                "__builtin_now",
                1
            ));

            return driver(fname, dump) ? 0 : 1;
        }
    }
}
