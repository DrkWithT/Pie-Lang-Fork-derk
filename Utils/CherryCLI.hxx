#pragma once

#include <utility>
#include <string>
#include <string_view>
#include <vector>
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

                pie::vm::Compiler cherryc;

                if (auto program_result = cherryc(ast, cloned_src)) {
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
                }

                auto run_ok = true;

                {
                    vm::VM cherry_engine {std::move(program.value()), m_config.local_capacity};

                    const auto start = std::chrono::steady_clock::now();
                    run_ok = cherry_engine.run();
                    const auto runtime = std::chrono::steady_clock::now() - start;

                    std::println(
                        "\x1b[1;33mRuntime:\x1b[0m {}\n\x1b[1;33mResult: {}\x1b[0m\n",
                        runtime,
                        cherry_engine.result().to_string()
                    );
                }

                return run_ok;
            }
        };
    }

    namespace cli {
        [[nodiscard]] int runExperimental(const std::filesystem::path fname, const bool dump) {
            cherry::Driver driver {cherry::Config {
                .name = "Cherry (experimental Pie VM)",
                .local_capacity = 2048,
                .heap_capacity = 256
            }};

            driver.add_builtin("__builtin_print", std::make_unique<vm::BuiltIn>(
                vm::pie_native_print,
                "__builtin_print",
                1
            ));

            return driver(fname, dump) ? 0 : 1;
        }
    }
}
