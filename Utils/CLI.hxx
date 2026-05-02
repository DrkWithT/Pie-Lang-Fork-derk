#pragma once

#include <print>
#include <string>
#include <string_view>
#include <vector>
#include <filesystem>
#include <utility>
#include <stdexcept>

#include "../Lex/Lexer.hxx"
#include "../Preprocessor/Preprocessor.hxx"
#include "../Parser/Parser.hxx"
#include "../Analysis/LexicalScoping.hxx"
#include "../Interp/Interpreter.hxx"
#include "../VM/ByteCode.hxx"
#include "../VM/Compiler.hxx"



inline namespace pie {
namespace cli {

    void help() {
        std::cout << "print tokens:        -token" << '\n';
        std::cout << "print parsed:        -ast"   << '\n';
        std::cout << "print pre-processed: -pre"   << '\n';
        std::cout << "don't run program:   -run"   << '\n';
        std::cout << "print this message:  -help"   << '\n';
    }


    void REPL(
        const std::filesystem::path canonical_root,
        const bool print_preprocessed,
        const bool print_tokens,
        const bool print_parsed,
        const bool run
    ) {
        Parser parser{canonical_root};
        interp::Visitor visitor;

        for (;;) try {
            std::string line;
            std::print(">>> ");
            std::getline(std::cin, line);
            if (line.back() != ';') line += ';';


            constexpr auto REPL = true;
            auto processed_line = preprocess<REPL>(std::move(line), canonical_root); // root in repl mode is where we ran the interpret
            if (print_preprocessed) std::println(std::clog, "{}", processed_line);

            Tokens v = lex::lex(std::move(processed_line));
            if (print_tokens) std::println(std::clog, "{}", v);

            if (v.empty()) continue;

            auto [exprs, ops] = parser.parse(std::move(v));

            if (print_parsed) for(const auto& expr : exprs) std::println(std::clog, "{};", expr->stringify(0));

            if(run and (print_parsed or print_preprocessed or print_tokens)) puts("Output:\n");


            if (run) {
                visitor.addOperators(std::move(ops));

                if (not exprs.empty()) {
                    Value value;
                    for (auto&& expr : exprs) value = std::visit(visitor, std::move(expr)->variant());

                    std::println("{}", stringify(value));
                }
            }
        }
        catch(const std::exception &e) {
            std::clog << e.what() << std::endl;
        }
    }


    // ! EXPERIMENTAL: This will run the Cherry VM after simple expression code emits properly.
    void runExperimental(const std::filesystem::path fname, const bool dump) {
        auto src = util::readFile(fname.string());
        auto cloned_src = src;
        auto processed_src = std::move(src);

        Tokens v = lex::lex(std::move(processed_src));

        if (v.empty()) return;

        Parser p {std::move(v), fname};

        auto ast = p.parse();

        pie::analysis::LexicalAnalysis anal;
        for (const auto& [exprs, ops] = ast; const auto& expr : exprs)
            std::visit(anal, expr->variant());

        pie::vm::Compiler cherryc;

        auto program_result = cherryc(ast, cloned_src);

        if (dump && program_result) {
            display_all_bytecode(program_result.value());
        } else {
            std::print("{}", program_result.error());
        }
    }

    void runFile(
        const std::filesystem::path fname,
        const bool print_preprocessed,
        const bool print_tokens,
        const bool print_parsed,
        const bool run
    ) {
        auto src = util::readFile(fname.string());

        // auto processed_src = preprocess(std::move(src), fname);
        // if (print_preprocessed) std::println(std::clog, "{}", processed_src);
        auto processed_src = std::move(src);

        Tokens v = lex::lex(std::move(processed_src));
        if (print_tokens) std::println(std::clog, "{}", v);

        if (v.empty()) return;

        Parser p{std::move(v), fname};

        auto [exprs, ops] = p.parse();

        if (print_parsed)
            for(const auto& expr : exprs)
                std::println(std::clog, "{};", expr->stringify(0));

        if(run and (print_parsed or print_preprocessed or print_tokens)) puts("Output:\n");

        pie::analysis::LexicalAnalysis anal;
        for (const auto& expr : exprs)
            std::visit(anal, expr->variant());

        if (run) {


            interp::Visitor visitor{std::move(ops)};
            for (const auto& expr : exprs)
                std::visit(visitor, expr->variant());
        }
    }


} // namespace cli
} // namespace pie

