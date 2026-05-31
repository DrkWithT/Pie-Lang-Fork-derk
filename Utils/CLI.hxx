#pragma once

#include <print>
#include <string>
#include <filesystem>
#include <utility>

#include "../Lex/Lexer.hxx"
// #include "../Preprocessor/Preprocessor.hxx"
#include "../Parser/Parser.hxx"
#include "../Analysis/LexicalScoping.hxx"
#include "../Interp/Interpreter.hxx"



inline namespace pie {
namespace cli {

    inline void help() {
        std::cout << "-t [--token]  :   print tokens\n";
        std::cout << "   [--ast]    :   print parsed\n";
        std::cout << "-r [--run]    :   don't run program\n";
        std::cout << "-h [--help]   :   print this message\n";
        std::cout << "-c [--command]:   print this message\n";
    }


    inline void REPL(
        const std::filesystem::path canonical_root,
        const bool print_tokens,
        const bool print_parsed,
        const bool run
    ) {
        Parser parser{canonical_root};
        pie::analysis::LexicalScoping anal;
        interp::Visitor visitor{{}};

        for (;;) try {
            std::string line;
            std::print(">>> ");
            std::getline(std::cin, line);
            if (line.back() != ';') line += ';';


            // constexpr auto REPL = true;
            // auto processed_line = preprocess<REPL>(std::move(line), canonical_root); // root in repl mode is where we ran the interpret
            // if (print_preprocessed) std::println(std::clog, "{}", line);

            Tokens v = lex::lex(std::move(line));
            if (print_tokens) std::println(std::clog, "{}", v);

            if (v.empty()) continue;

            parser.resetTokens(std::move(v));

            auto exprs = parser.parse();

            if (print_parsed)
                for(const auto& expr : exprs)
                    std::println(std::clog, "{};", expr->stringify());

            if(run and (print_parsed or print_tokens)) puts("Output:\n");


            for (auto& expr : exprs)
                std::visit(anal, expr->variant());


            if (run) {
                if (not exprs.empty()) {
                    Value value;
                    for (auto& expr : exprs) value = std::visit(visitor, std::move(expr)->variant());

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

        if(run and (print_parsed or print_tokens)) puts("Output:\n");

        pie::analysis::LexicalScoping anal;
        for (auto& expr : exprs)
            std::visit(anal, expr->variant());

        if (run) {
            interp::Visitor visitor{std::move(anal).indeces};
            for (const auto& expr : exprs)
                std::visit(visitor, expr->variant());
        }
    }



    inline void run(
        std::string src,
        const bool print_tokens,
        const bool print_parsed,
        const bool run
    ) {
        Tokens v = lex::lex(std::move(src));
        if (print_tokens) std::println(std::clog, "{}", v);

        if (v.empty()) return;

        Parser p{std::move(v)};

        auto exprs = p.parse();

        if (print_parsed)
            for(const auto& expr : exprs)
                std::println(std::clog, "{};", expr->stringify(0));

        if(run and (print_parsed or print_tokens)) puts("Output:\n");

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

