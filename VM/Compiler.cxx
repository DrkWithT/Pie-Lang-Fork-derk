#include "../Expr/Expr.hxx"
#include "../VM/Compiler.hxx"

namespace pie::vm {
    [[nodiscard]] std::optional<SymbolInfo> lookup_global(const std::string& symbol) {
        if (auto symbol_info_it = m_global_builtin_scope.symbols.find(symbol); symbol_info_it != m_global_builtin_scope.symbols.end()) {
            return *symbol_info_it;
        }

        return {};
    }

    [[nodiscard]] std::optional<SymbolInfo> lookup_local(const std::string& symbol) {
        if (auto local_info_it = m_scopes.back().symbols.find(symbol); local_info_it != m_scopes.back().symbols.end()) {
            if (local_info_it->type == SymbolType::identifier) {
                return *local_info_it;
            }
        }

        return {};
    }

    [[nodiscard]] std::optional<SymbolInfo> lookup_constant(const std::string& symbol) {
        if (auto constant_info_it = m_scopes.back().symbols.find(symbol); local_info_it != m_scopes.back().symbols.end()) {
            if (constant_info_it->type == SymbolType::constant) {
                return *constant_info_it;
            }
        }

        return {};
    }

    [[nodiscard]] std::optional<SymbolInfo> record_global(const std::string& symbol, std::unique_ptr<ObjectBase> object_ptr) noexcept {
        if (auto builtin_global_locus = lookup_global(symbol); builtin_global_locus) {
            return builtin_global_locus;
        }

        SymbolInfo global_locus {
            .id = static_cast<std::uint16_t>(m_globals.size()),    // ! NOTE: m_globals.size() is the real ID of the new global object.
            .type = SymbolType::global
        };

        m_global_builtin_scope.symbols[symbol] = global_locus;
        m_globals.emplace_back(std::move(object_ptr));

        return global_locus;
    }

    [[nodiscard]] std::optional<SymbolInfo> record_ident(std::string symbol) noexcept {
        if (auto ident_locus = lookup_local(symbol); local_ident_locus) {
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
    [[nodiscard]] std::optional<SymbolInfo> record_constant(const std::string& symbol, FastValue value) {
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
    [[nodiscard]] std::optional<SymbolInfo> record_constant(const std::string& symbol, std::unique_ptr<ObjectBase> object_ptr) {
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

    [[nodiscard]] bool Compiler::emit_builtin_or_const(const expr::Name* name) {
        ;
    }

    [[nodiscard]] bool Compiler::emit_bool(const expr::Bool* expr) {
        ;
    }

    [[nodiscard]] bool Compiler::emit_num(const expr::Bool* expr) {
        ;
    }

    [[nodiscard]] bool Compiler::emit_string(const expr::String* expr) {
        ;
    }

    [[nodiscard]] bool Compiler::emit_name(const expr::Name* name) {
        ;
    }

    [[nodiscard]] bool Compiler::emit_assignment(const expr::Assignment* expr) {
        ;
    }

    [[nodiscard]] bool Compiler::emit_match(const expr::Match* expr) {
        ;
    }

    [[nodiscard]] bool Compiler::emit_block(const expr::Block* expr) {
        ;
    }

    [[nodiscard]] bool Compiler::emit_call(const expr::Call* expr) {
        ;
    }

    std::expected<Program, std::string> Compiler::operator()(const std::pair<std::vector<expr::ExprPtr>, Operators>& ast, const std::string& source) {
        ;
    }
}