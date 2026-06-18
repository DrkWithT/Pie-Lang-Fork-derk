#include "Lexer.hxx"


inline namespace pie {
inline namespace lex {


TokenKind keyword(const std::string_view word) noexcept {
    using enum TokenKind;
    if (word == "mixfix") return MIXFIX;
    else if (word == "prefix") return PREFIX;
    else if (word == "infix" ) return INFIX ;
    else if (word == "suffix") return SUFFIX;
    else if (word == "exfix" ) return EXFIX ;

    else if (word == "class") return CLASS;
    else if (word == "union") return UNION;
    else if (word == "match") return MATCH;

    else if (word == "loop"    ) return LOOP;
    else if (word == "break"   ) return BREAK;
    else if (word == "continue") return CONTINUE;

    else if (word == "import") return IMPORT;
    else if (word == "space" ) return NAMESPACE;

    else if (word == "use") return USE;

    else if (word == "true"  ) return BOOL;
    else if (word == "false" ) return BOOL;

    return NAME;
}


bool validNameChar(const char c) noexcept {
    switch (c) {
        case '?':
        case '!':
        case '@':
        case '#':
        case '$':
        case '%':
        case '^':
        case '&':
        case '|':
        case '*':
        case '+':
        case '~':
        case '-':
        case '_':
        case '\\':
        case '\'':
        case '/':
        case '<':
        case '>':
        case '[':
        case ']':
            return true;
    }

    return isalnum(static_cast<unsigned char>(c));
}

[[nodiscard]] Tokens lex(const std::string& src) {
    TokenLines lines = {{}};
    Tokens line;

    size_t line_count = 1;
    size_t line_starting_index{};


    for (size_t index{}; index < src.length(); ++index) {

        try {
        switch (src[index]) {
            using enum TokenKind;

            #pragma GCC diagnostic push
            #pragma GCC diagnostic ignored "-Wpedantic"
            case '0' ... '9':
            #pragma GCC diagnostic pop
            {
                const auto beginning = index;
                while (isdigit(static_cast<unsigned char>(src.at(++index))));

                bool is_name = validNameChar(src[index]);
                if (is_name) {
                    while (validNameChar(src.at(++index)));
                    lines.back().emplace_back(NAME, src.substr(beginning, index - beginning));
                    --index;
                    break;
                }

                bool is_float = false;
                if (src[index] == '.' and isdigit(static_cast<unsigned char>(src.at(index + 1)))) {
                    is_float = true;
                    while (isdigit(static_cast<unsigned char>(src.at(++index))));
                }

                lines.back().emplace_back(is_float ? FLOAT : INT, src.substr(beginning, index - beginning));
                --index;
            } break;


            case '?':
            case '!':
            case '@':
            case '#':
            case '$':
            case '%':
            case '^':
            case '&':
            case '|':
            case '*':
            case '+':
            case '~':
            case '-':
            case '_':
            case '\\':
            case '\'':
            case '/':
            case '<':
            case '>':
            case '[':
            case ']':
            #pragma GCC diagnostic push
            #pragma GCC diagnostic ignored "-Wpedantic"
            case 'a' ... 'z':
            case 'A' ... 'Z':
            #pragma GCC diagnostic pop
            {
                const auto beginning = index;
                while (validNameChar(src.at(++index)));

                const auto word = src.substr(beginning, index - beginning);


                if (word == "__TEXT__") [[unlikely]] {
                    std::string line_text;

                    for (size_t ind = line_starting_index; ind < src.size() and src[ind] != '\n'; ++ind)
                        line_text += src[ind];

                    lines.back().push_back({STRING, line_text});
                    --index;
                    break;
                }

                if (word == "__LINE__") [[unlikely]] {
                    lines.back().push_back({INT, std::to_string(line_count)});
                    --index;
                    break;
                }


                const TokenKind token = keyword(word);

                lines.back().emplace_back(token, word);
                --index;
            } break;


            case '=':
                if (src.at(index + 1) == '>')
                    lines.back().push_back({FAT_ARROW, {src[index], src[++index]}});
                else
                    lines.back().push_back({ASSIGN, {src[index]}});

                break;

            case ',': lines.back().push_back({COMMA, {src[index]}}); break;
            case '.':
                if (src.at(index + 1) == ':') {
                    if (src.at(index + 2) == ':') {
                        for(index += 2;
                            src.substr(index, 3) != "::.";
                            ++index
                        );

                        index += 2;
                    }
                    else while(++index < src.length() and src[index] != '\n');
                }
                else if (src[index + 1] == '.' and src.at(index + 2) == '.')
                    lines.back().push_back({ELLIPSIS, {src[index], src[++index], src[++index]}});
                else if (src[index + 1] == '.')
                    lines.back().push_back({CASCADE , {src[index], src[++index],             }});
                else
                    lines.back().push_back({DOT, {src[index]}});

                break;

            case ':': 
                if (src.at(index + 1) == ':') lines.back().push_back({SCOPE_RESOLVE, {':', src[++index]}});
                else                          lines.back().push_back({COLON, ":"});
                break;

            case ';':
                lines.back().push_back({SEMI, {src[index]}});
                lines.push_back({});
                break;

            case '`': lines.back().push_back({BACKTICK, {src[index]}}); break;

            case '\n':
                ++line_count;
                line_starting_index = index + 1;
                break;

            case '(': lines.back().push_back({L_PAREN, {src[index]}}); break;
            case ')': lines.back().push_back({R_PAREN, {src[index]}}); break;

            case '{':
                lines.back().push_back({L_BRACE, {src[index]}});
                break;

            case '}': lines.back().push_back({R_BRACE, {src[index]}}); break;

            case '"':{
                const size_t old = index;
                while(src.at(++index) != '"') {
                }
                lines.back().push_back({STRING, src.substr(old + 1, index - old -1)});
            } break;


            default: break;
        }
        }
        catch(const std::exception& err) {
            util::error("Lexing Error!");
        }

    }

    if (not lines.empty() and not lines.back().empty() and lines.back().back().kind != TokenKind::SEMI)
        util::error("Last line doesn't end with a ';'!");


    if (lines.size() > 1) {
        lines.pop_back();
        lines.back().emplace_back(TokenKind::END, "EOF");
    }


    Tokens tokens;
    for (auto&& line : lines)
        for (auto&& t : line)
            tokens.push_back(std::move(t));

    return tokens;
}


} // namespace lex
} // namespace pie
