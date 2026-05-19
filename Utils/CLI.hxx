#pragma once

#include <print>
#include <string>
#include <filesystem>
#include <utility>

#include "../Lex/Lexer.hxx"
#include "../Preprocessor/Preprocessor.hxx"
#include "../Parser/Parser.hxx"
#include "../Analysis/LexicalScoping.hxx"
#include "../Interp/Interpreter.hxx"
#include "../Utils/CherryCLI.hxx"

inline namespace pie {
namespace cli {

    inline void help() {
        std::cout << "print tokens:        -token" << '\n';
        std::cout << "print parsed:        -ast"   << '\n';
        std::cout << "print pre-processed: -pre"   << '\n';
        std::cout << "don't run program:   -run"   << '\n';
        std::cout << "run experimental VM: -exp"   << '\n';
        std::cout << "dump VM bytecode:    -dump"  << '\n';
        std::cout << "print this message:  -help"  << '\n';
    }


    inline void REPL(
        const std::filesystem::path canonical_root,
        const bool print_preprocessed,
        const bool print_tokens,
        const bool print_parsed,
        const bool run
    ) {
        Parser parser{canonical_root};
        interp::Visitor visitor{{}};

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

            parser.resetTokens(std::move(v));

            auto exprs = parser.parse();

            if (print_parsed) for(const auto& expr : exprs) std::println(std::clog, "{};", expr->stringify(0));

            if(run and (print_parsed or print_preprocessed or print_tokens)) puts("Output:\n");


            if (run) {
                // visitor.addOperators(std::move(ops));

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



    inline void runFile(
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

        auto exprs = p.parse();

        if (print_parsed)
            for(const auto& expr : exprs)
                std::println(std::clog, "{};", expr->stringify(0));

        if(run and (print_parsed or print_preprocessed or print_tokens)) puts("Output:\n");

        pie::analysis::LexicalScoping anal;
        for (auto& expr : exprs)
            std::visit(anal, expr->variant());

        if (run) {


            interp::Visitor visitor{std::move(anal).indeces};
            for (const auto& expr : exprs)
                std::visit(visitor, expr->variant());
        }
    }


} // namespace cli
} // namespace pie

