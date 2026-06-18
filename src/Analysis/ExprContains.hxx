#pragma once

#include "../Expr/Expr.hxx"
#include <cstddef>


inline namespace pie {
namespace analysis {

    template <typename EXPR>
    const EXPR* checkPattern(expr::Match::Case::Pattern&);

    template <typename EXPR>
    const EXPR* exprContains(expr::ExprPtr expr) {
        return std::visit([](auto* node) -> const EXPR* {
            using T = std::remove_pointer_t<decltype(node)>;

            if constexpr (std::is_same_v<T, EXPR>) {
                return node;
            }
            else if constexpr (
                std::is_same_v<T, expr::Num>      or
                std::is_same_v<T, expr::Bool>     or
                std::is_same_v<T, expr::String>   or
                std::is_same_v<T, expr::Name>     or
                std::is_same_v<T, expr::Continue> or
                std::is_same_v<T, expr::Union>    or
                std::is_same_v<T, expr::Cascade>  or
                std::is_same_v<T, expr::SpaceAccess> or
                std::is_same_v<T, expr::Use>      or
                std::is_same_v<T, expr::UseSpace> or
                std::is_same_v<T, expr::UseFix>   or
                std::is_same_v<T, expr::Import>   or
                std::is_same_v<T, expr::Syntax>
            ) {
                return nullptr;
            }
            else if constexpr (
                std::is_same_v<T, expr::Expansion>  or
                std::is_same_v<T, expr::UnaryFold>
            ) {
                return exprContains<EXPR>(node->pack);
            }
            else if constexpr (std::is_same_v<T, expr::Break>) {
                return exprContains<EXPR>(node->expr);
            }
            else if constexpr (std::is_same_v<T, expr::Access>) {
                return exprContains<EXPR>(node->var);
            }
            else if constexpr (
                std::is_same_v<T, expr::Grouping>  or
                std::is_same_v<T, expr::UnaryOp>   or
                std::is_same_v<T, expr::PostOp>    or
                std::is_same_v<T, expr::CircumOp>
            ) {
                return exprContains<EXPR>(node->expr);
            }
            else if constexpr (std::is_same_v<T, expr::Closure>) {
                return exprContains<EXPR>(node->body);
            }
            else if constexpr (std::is_same_v<T, expr::Type>) {
                if (auto expr_type = dynamic_cast<const type::ExprType*>(node->type.get()))
                    return exprContains<EXPR>(expr_type->t);
                return nullptr;
            }
            else if constexpr (
                std::is_same_v<T, expr::BinOp>                or
                std::is_same_v<T, expr::SeparatedUnaryFold>
            ) {
                if (auto p = exprContains<EXPR>(node->lhs)) return p;
                return exprContains<EXPR>(node->rhs);
            }
            else if constexpr (std::is_same_v<T, expr::Assignment>) {
                if (auto p = exprContains<EXPR>(node->lhs)) return p;
                return exprContains<EXPR>(node->rhs);
            }
            else if constexpr (std::is_same_v<T, expr::BinaryFold>) {
                if (auto p = exprContains<EXPR>(node->init)) return p;
                if (auto p = exprContains<EXPR>(node->pack)) return p;
                if (node->sep) return exprContains<EXPR>(node->sep);
                return nullptr;
            }
            else if constexpr (std::is_same_v<T, expr::List>) {
                for (const auto& e : node->elements)
                    if (auto p = exprContains<EXPR>(e)) return p;
                return nullptr;
            }
            else if constexpr (std::is_same_v<T, expr::Map>) {
                for (const auto& [key, value] : node->items) {
                    if (auto p = exprContains<EXPR>(key  )) return p;
                    if (auto p = exprContains<EXPR>(value)) return p;
                }
                return nullptr;
            }
            else if constexpr (std::is_same_v<T, expr::Class>) {
                for (const auto& [_, __, e] : node->fields)
                    if (auto p = exprContains<EXPR>(e)) return p;
                return nullptr;
            }
            else if constexpr (std::is_same_v<T, expr::Match>) {
                if (auto p = exprContains<EXPR>(node->expr)) return p;
                for (const auto& kase : node->cases) {
                    if (auto p = checkPattern<EXPR>(*kase.pattern)) return p;
                    if (kase.guard)
                        if (auto p = exprContains<EXPR>(kase.guard)) return p;
                }
                return nullptr;
            }
            else if constexpr (std::is_same_v<T, expr::Loop>) {
                if (node->kind) if (auto p = exprContains<EXPR>(node->kind)) return p;
                if (auto p = exprContains<EXPR>(node->body)) return p;
                if (node->els ) if (auto p = exprContains<EXPR>(node->els )) return p;
                return nullptr;
            }
            else if constexpr (std::is_same_v<T, expr::Namespace>) {
                for (const auto& e : node->space)
                    if (auto p = exprContains<EXPR>(e)) return p;
                return nullptr;
            }
            else if constexpr (std::is_same_v<T, expr::OpCall>) {
                for (const auto& arg : node->exprs)
                    if (auto p = exprContains<EXPR>(arg)) return p;
                return nullptr;
            }
            else if constexpr (std::is_same_v<T, expr::Call>) {
                if (auto p = exprContains<EXPR>(node->func)) return p;
                for (const auto& arg : node->args)
                    if (auto p = exprContains<EXPR>(arg)) return p;
                return nullptr;
            }
            else if constexpr (std::is_same_v<T, expr::Block>) {
                for (const auto& e : node->lines)
                    if (auto p = exprContains<EXPR>(e)) return p;
                return nullptr;
            }
            else if constexpr (
                std::is_same_v<T, expr::Prefix> or
                std::is_same_v<T, expr::Infix>  or
                std::is_same_v<T, expr::Suffix> or
                std::is_same_v<T, expr::Exfix>  or
                std::is_same_v<T, expr::Operator>
            ) {
                for (const auto& e : node->funcs)
                    if (auto p = exprContains<EXPR>(e)) return p;
                return nullptr;
            }
            else {
                return nullptr;
            }
        }, expr->variant());
    }


    template <typename EXPR>
    const EXPR* checkPattern(expr::Match::Case::Pattern& pat) {
        if (std::holds_alternative<expr::Match::Case::Pattern::Single>(pat.pattern)) {
            auto& pattern = get<expr::Match::Case::Pattern::Single>(pat.pattern);
            if (pattern.value)
                return exprContains<EXPR>(pattern.value);
        }
        else {
            for (const auto& pat : get<expr::Match::Case::Pattern::Structure>(pat.pattern).patterns) {
                if (auto p = checkPattern<EXPR>(*pat)) return p;
            }
        }

        return nullptr;
    }

} // namespace analysis
} // namespace pie
