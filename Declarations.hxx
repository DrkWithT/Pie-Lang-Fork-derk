#pragma once

#include <memory>
#include <string_view>
#include <variant>

#ifdef WEB_PIE
using BigInt = long long;
#else
using BigInt = ssize_t;
#endif

inline namespace pie {

namespace expr {

// has to be pointers kuz we're forward declareing
// has to be forward declared bc we're using in in the class bellow

using Node = std::variant<
    const struct Num               *,
    const struct Bool              *,
    const struct String            *,
    const struct Name              *,
    // const struct Pack              *,
    const struct List              *,
    const struct Map               *,
    const struct Expansion         *,
    const struct UnaryFold         *,
    const struct SeparatedUnaryFold*,
    const struct BinaryFold        *,
    const struct Assignment        *,
    const struct Class             *,
    const struct Union             *,
    const struct Match             *,
    const struct Type              *,
    const struct Loop              *,
    const struct Break             *,
    const struct Continue          *,
    const struct Access            *,
    // const struct Cascade           *,
    const struct Namespace         *,
    const struct Use               *,
    const struct Import               *,
    const struct SpaceAccess       *,
    const struct Grouping          *,
    const struct UnaryOp           *, // op INNER
    const struct BinOp             *, // LHS op RHS
    const struct PostOp            *, // INNER op
    const struct CircumOp          *, // op1 INNER op2
    const struct OpCall            *, // mixfix operator: op0 arg0 op1 arg1 ...
    const struct Call              *, // regular function call
    const struct Closure           *,
    const struct Block             *,
    const struct Prefix            *, // SEE: UnaryOp, is def of prefix-unary
    const struct Infix             *, // SEE: BinOp
    const struct Suffix            *, // SEE: PostOp
    const struct Exfix             *, // SEE: CircumOp
    const struct Operator          *  // Example: IF a THEN b ELSE c
>;


struct Expr {
    virtual ~Expr() = default;
    virtual std::string stringify(const size_t indent = 0) const = 0;
    virtual bool involvesName(const std::string_view sv) const = 0;

    virtual Node variant() const = 0;
};

using ExprPtr = std::shared_ptr<Expr>;


} // namespace expr
} // namespace pie