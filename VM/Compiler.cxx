#include <iostream>
#include <print>

#include "../Expr/Expr.hxx"
#include "../VM/Compiler.hxx"

namespace pie{
    namespace vm {
        [[nodiscard]] std::optional<SymbolInfo> Compiler::lookup_global(const std::string& symbol) {
            if (auto symbol_info_it = m_global_builtin_scope.symbols.find(symbol); symbol_info_it != m_global_builtin_scope.symbols.end()) {
                return symbol_info_it->second;
            }

            return {};
        }

        [[nodiscard]] std::optional<SymbolInfo> Compiler::lookup_local(const std::string& symbol) {
            if (auto local_info_it = m_scopes.back().symbols.find(symbol); local_info_it != m_scopes.back().symbols.end()) {
                if (local_info_it->second.type == SymbolType::ident) {
                    return local_info_it->second;
                }
            }

            return {};
        }

        [[nodiscard]] std::optional<SymbolInfo> Compiler::lookup_constant(const std::string& symbol) {
            if (auto constant_info_it = m_scopes.back().symbols.find(symbol); constant_info_it != m_scopes.back().symbols.end()) {
                if (constant_info_it->second.type == SymbolType::constant) {
                    return constant_info_it->second;
                }
            }

            return {};
        }

        [[nodiscard]] std::optional<SymbolInfo> Compiler::record_global(const std::string& symbol, std::unique_ptr<ObjectBase> object_ptr) noexcept {
            if (auto builtin_global_locus = lookup_global(symbol); builtin_global_locus) {
                return builtin_global_locus;
            }

            SymbolInfo global_locus {
                .id = static_cast<std::uint16_t>(m_globals.size()),    // ! NOTE: m_globals.size() is the real ID of the new global object.
                .type = SymbolType::global
            };

            // ? Check for empty symbol to support anonymously mapped objects. Nameless objects should not have a key in their symbol scope or else name resolution would have issues.
            if (!symbol.empty()) {
                m_global_builtin_scope.symbols[symbol] = global_locus;
            }

            m_globals.emplace_back(std::move(object_ptr));

            return global_locus;
        }

        [[nodiscard]] std::optional<SymbolInfo> Compiler::record_ident(std::string symbol) noexcept {
            if (auto ident_locus = lookup_local(symbol); ident_locus) {
                return ident_locus;
            }

            // ? NOTE: This data tells which slot the identifier string is pooled in its chunk.
            const std::uint16_t ident_id = m_scopes.back().next_ident_key_id;
            SymbolInfo local_locus {
                .id = ident_id,
                .type = SymbolType::ident
            };

            m_scopes.back().symbols[symbol] = local_locus;
            m_codes.back().identifiers.emplace_back(std::move(symbol));
            m_scopes.back().next_ident_key_id++;

            return local_locus;
        }

        // ! NOTE: Since expressions can be identifiers bound to values in Pie, the only way of determining a constant applies is checking whether the expression was not bound prior. The expression must not be a bound `builtin_function` or local (variable identifier).
        [[nodiscard]] std::optional<SymbolInfo> Compiler::record_constant(const std::string& symbol, FastValue value) {
            if (lookup_global(symbol).has_value() || lookup_local(symbol).has_value()) {
                return {};
            } else if (auto current_locus = lookup_constant(symbol); current_locus) {
                return current_locus;
            }

            const std::uint16_t next_constant_id = m_scopes.back().next_constant_id;
            SymbolInfo local_locus {
                .id = next_constant_id,
                .type = SymbolType::constant
            };

            m_scopes.back().symbols[symbol] = local_locus;
            m_codes.back().constants.emplace_back(value);
            m_scopes.back().next_constant_id++;

            return local_locus;
        }

        // ! NOTE: This is the other overload for mapping constants to atom-expressions (literals). Since expressions can be identifiers bound to values in Pie, the only way of determining a constant applies is checking whether the expression was not bound prior.
        [[nodiscard]] std::optional<SymbolInfo> Compiler::record_constant(const std::string& symbol, std::unique_ptr<ObjectBase> object_ptr) {
            if (lookup_global(symbol).has_value() || lookup_local(symbol).has_value()) {
                return {};
            } else if (auto current_locus = lookup_constant(symbol); current_locus) {
                return current_locus;
            }

            const std::uint16_t next_constant_id = m_scopes.back().next_constant_id;
            SymbolInfo local_locus {
                .id = next_constant_id,
                .type = SymbolType::constant
            };

            ObjectBase* object_peeked = object_ptr.get();
            m_globals.emplace_back(std::move(object_ptr));

            m_scopes.back().symbols[symbol] = local_locus;
            m_codes.back().constants.emplace_back(FastValue {object_peeked});
            m_scopes.back().next_constant_id++;

            return local_locus;
        }


        void Compiler::encode_instruction(Opcode op) {
            m_codes.back().code.emplace_back(0, 0, op);
        }

        void Compiler::encode_instruction(Opcode op, std::int16_t wide) {
            m_codes.back().code.emplace_back(wide, 0, op);
        }

        void Compiler::encode_instruction(Opcode op, std::int8_t tiny, std::int16_t wide) {
            m_codes.back().code.emplace_back(wide, tiny, op);
        }


        [[nodiscard]] bool Compiler::emit_builtin_or_const(const expr::Name* name) {
            const std::string symbol = name->stringify();

            if (auto global_locus = lookup_global(symbol); global_locus) {
                encode_instruction(Opcode::push_global, global_locus->id);
            } else if (auto const_locus = lookup_constant(symbol); const_locus) {
                encode_instruction(Opcode::push_const, const_locus->id);
            } else {
                std::println(std::cerr, "\tNote: In emit_string(), the builtin / constant '{}' was unbound & not a known constant. Bytecode symbol information could not be resolved.", symbol);
                return false;
            }

            return true;
        }

        [[nodiscard]] bool Compiler::emit_bool(const expr::Bool* expr) {
            const std::string lexeme = expr->stringify();

            if (
                auto maybe_const_locus = record_constant(lexeme, FastValue {lexeme == "true"});
                maybe_const_locus
            ) {
                encode_instruction(Opcode::push_const, 0, maybe_const_locus->id);
            } else if (auto maybe_local_locus = lookup_local(lexeme); maybe_local_locus) {
                encode_instruction(Opcode::lookup, 0, maybe_local_locus->id);
            } else {
                std::println(std::cerr, "\tNote: In emit_bool(), the bool literal '{}' was unbound & not a known constant. Bytecode symbol information could not be resolved.", lexeme);
                return false;
            }

            return true;
        }

        [[nodiscard]] bool Compiler::emit_num(const expr::Num* expr) {
            const std::string lexeme = expr->stringify();

            if (
                auto maybe_const_locus = record_constant(lexeme, FastValue {std::stof(lexeme)});
                maybe_const_locus
            ) {
                encode_instruction(Opcode::push_const, 0, maybe_const_locus->id);
            } else if (auto maybe_local_locus = lookup_local(lexeme); maybe_local_locus) {
                encode_instruction(Opcode::lookup, 0, maybe_local_locus->id);
            } else {
                std::println(std::cerr, "\tNote: In emit_num(), the numeric literal '{}' was unbound & not a known constant. Bytecode symbol information could not be resolved.", lexeme);
                return false;
            }

            return true;
        }

        [[nodiscard]] bool Compiler::emit_string(const expr::String* expr) {
            const std::string lexeme = expr->stringify();

            const auto new_string_p = &m_codes.back().identifiers.emplace_back(lexeme);

            if (
                auto maybe_const_locus = record_constant(lexeme, FastValue {new_string_p});
                maybe_const_locus
            ) {
                encode_instruction(Opcode::push_const, 0, maybe_const_locus->id);
            } else if (auto maybe_local_locus = lookup_local(lexeme); maybe_local_locus) {
                encode_instruction(Opcode::lookup, 0, maybe_local_locus->id);
            } else {
                std::println(std::cerr, "\tNote: In emit_string(), the string literal '{}' was unbound & not a known constant. Bytecode symbol information could not be resolved.", lexeme);
                return false;
            }

            return true;
        }

        [[nodiscard]] bool Compiler::emit_name(const expr::Name* name) {
            const std::string lexeme = name->stringify();

            if (auto maybe_declared_locus = lookup_local(lexeme); maybe_declared_locus) {
                encode_instruction(Opcode::lookup, 0, maybe_declared_locus->id);
            } else if (auto maybe_global_locus = lookup_global(lexeme); maybe_global_locus) {
                encode_instruction(Opcode::push_global, 0, maybe_global_locus->id);
            } else {
                std::println(std::cerr, "\tNote: In emit_name(), identifier '{}' was undeclared.", lexeme);
                return false;
            }

            return true;
        }

        [[nodiscard]] bool Compiler::emit_assignment(const expr::Assignment* expr) {
            const auto& [lhs, type, rhs] = *expr;

            const std::string lhs_ident = lhs->stringify();
            auto key_locus = record_ident(lhs_ident);

            if (!emit_expr(rhs)) {
                std::println(std::cerr, "\tNote: In emit_assignment(), RHS was invalid.");
                return false;
            }

            encode_instruction(Opcode::bind, 0, key_locus->id);

            return true;
        }

        [[nodiscard]] bool Compiler::emit_match(const expr::Match* expr) {
            std::println(std::cerr, "Compile error at emit_match(): unsupported expression!");
            return false;
        }

        [[nodiscard]] bool Compiler::emit_block(const expr::Block* expr) {
            std::println(std::cerr, "Compile error at emit_block(): unsupported expression!");
            return false;
        }

        [[nodiscard]] bool Compiler::emit_call(const expr::Call* expr) {
            const auto& [callee_expr, named_args, args] = *expr;

            if (!emit_expr(callee_expr)) {
                std::println(std::cerr, "\tNote: In emit_call(): invalid callee expression visited.");
                return false;
            }

            encode_instruction(Opcode::ref_env);

            for (std::uint16_t argc = 0; const auto& arg_expr : args) {
                if (!emit_expr(arg_expr)) {
                    std::println(std::cerr, "\tNote: Invalid expression for arg {} of function:\n{}", argc, callee_expr->stringify());
                    return false;
                }

                argc++;
            }

            encode_instruction(Opcode::call, static_cast<std::uint16_t>(args.size()));

            return true;
        }

        [[nodiscard]] bool Compiler::emit_expr(const expr::ExprPtr& any_expr) {
            auto expr_variant = any_expr->variant();

            if (auto expr_bool = std::get_if<const expr::Bool*>(&expr_variant); expr_bool) {
                return emit_bool(*expr_bool);
            } else if (auto expr_num = std::get_if<const expr::Num*>(&expr_variant); expr_num) {
                return emit_num(*expr_num);
            } else if (auto expr_str = std::get_if<const expr::String*>(&expr_variant); expr_str) {
                return emit_string(*expr_str);
            } else if (auto expr_name = std::get_if<const expr::Name*>(&expr_variant); expr_name) {
                return emit_name(*expr_name);
            } else if (auto expr_assign = std::get_if<const expr::Assignment*>(&expr_variant); expr_assign) {
                return emit_assignment(*expr_assign);
            } else if (auto expr_call = std::get_if<const expr::Call*>(&expr_variant); expr_call) {
                return emit_call(*expr_call);
            } else {
                std::println(std::cerr, "Compile error at emit_expr(): unknown expression type!");
                return false;
            }
        }

        [[maybe_unused]] bool Compiler::add_builtin_object(std::string name, std::unique_ptr<ObjectBase> global) {
            if (this->lookup_global(name).has_value()) {
                return false;
            }

            this->record_global(name, std::move(global));

            return true;
        }

        std::expected<Program, std::string> Compiler::operator()(const std::pair<std::vector<expr::ExprPtr>, Operators>& ast, const std::string& source) {
            encode_instruction(Opcode::start_env);

            for (int expr_pos = 0; const auto& expr_ptr : ast.first) {
                if (!emit_expr(expr_ptr)) {
                    return std::unexpected {
                        std::format("Compilation failed at expression #{}:\n\nSnippet: {}", expr_pos, expr_ptr->stringify())
                    };
                }

                expr_pos++;
            }

            encode_instruction(Opcode::end_env);
            encode_instruction(Opcode::ret);

            return Program {
                .main_code = std::move(m_codes.front()),
                .globals = std::move(m_globals),
            };
        }
    }
}