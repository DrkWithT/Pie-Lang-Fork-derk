#pragma once

#include <utility>
#include <type_traits>
#include <memory>
#include <string>
#include <vector>
#include <optional>
#include <expected>
#include <map>

// #include "../Lex/Token.hxx"
#include "../Expr/Expr.hxx"
// #include "../Declarations.hxx"
#include "../VM/ByteCode.hxx"

namespace pie {
    namespace vm {
        //? When resolving names to specific IDs in the bytecode, the compiler discerns what kind of Value space it maps to: constants per "bytecode Chunk" of the program, etc. This enum is meant for this.
        enum class SymbolType : std::uint8_t {
            constant,           //? Example: {id = 4, type = SymbolType::constant} --> opcode PUSH_CONST 4... but this would only apply for symbols that don't have an association to another RHS expr / identifier.
            global,             //? __builtin_xyz functions??
            ident,              //? Any LHS that's been resolved as declared associatively to a RHS e.g 4 = 5, foo + 1 = __builtin_add(a, b), foo + 2 __builtin_sub(a, b)
        };

        struct SymbolInfo {
            std::uint16_t id;
            SymbolType type;
        };

        struct SymbolScope {
            std::map<std::string, SymbolInfo> symbols;
            std::string name;
            std::uint16_t next_constant_id;     // ? ID's a non-bound expression's result.
            std::uint16_t next_ident_key_id;    // ? ID's stringified expression literal which can be mapped to a (constant) FastValue.
        };

        //? NOTE: This is meant as a RAII guard in the compiler to save/restore OR push/pop bits of compiler state e.g within the call-expr flag, avoiding the hassle of manually setting variables at exit locations of emitter methods.
        template <typename T> requires (std::is_default_constructible_v<T>)
        class FieldGuard {
        private:
            T* m_field_ptr;
            T m_old_field_v;

        public:
            explicit constexpr FieldGuard(T* field_ptr, T temp_value) noexcept
            : m_field_ptr {field_ptr}, m_old_field_v {(field_ptr != nullptr) ? *field_ptr : T {}} {
                if (m_field_ptr != nullptr) {
                    *m_field_ptr = temp_value;
                }
            }

            FieldGuard(const FieldGuard&) = delete;
            FieldGuard& operator=(const FieldGuard&) = delete;
            FieldGuard(FieldGuard&&) = delete;
            FieldGuard& operator=(FieldGuard&&) = delete;

            ~FieldGuard() {
                if (m_field_ptr != nullptr) {
                    *m_field_ptr = m_old_field_v;
                }
            }
        };

        //? NOTE: RAII guard for pushing / popping symbol scopes of the compiler when needed.
        class SymbolsGuard {
        private:
            std::vector<SymbolScope>* m_scopes_ptr;

        public:
            constexpr SymbolsGuard(std::vector<SymbolScope>* scopes_ptr, SymbolScope next_scope) noexcept
            : m_scopes_ptr {scopes_ptr} {
                m_scopes_ptr->push_back(std::move(next_scope));
            }

            SymbolsGuard(const SymbolsGuard&) = delete;
            SymbolsGuard& operator=(const SymbolsGuard&) = delete;
            SymbolsGuard(SymbolsGuard&&) = delete;
            SymbolsGuard& operator=(SymbolsGuard&&) = delete;

            constexpr ~SymbolsGuard() {
                if (m_scopes_ptr != nullptr) {
                    m_scopes_ptr->pop_back();
                }
            }
        };

        // TODO 1: The Pie AST is made of polymorphic node classes that can be Node-ified. Each Node is a variant of specifically-typed pointers to actual node types, so add methods to emit each.
        // TODO 2: Implement emits for Bool, Int, Name, Assign, Block, Call
        // TODO 3: Focus on getting a simple fibonacci to work 1st.
        class Compiler {
        private:
            SymbolScope m_global_builtin_scope;
            std::vector<SymbolScope> m_scopes;
            std::vector<std::unique_ptr<ObjectBase>> m_globals; // ? Stores anonymous objects too, not just global objects.
            std::vector<Chunk> m_codes;
            std::optional<std::uint16_t> m_bindable_lambda_id; // ? Stores the heap ID of a RHS lambda for a following operator alias.
            bool m_within_call;
            bool m_within_lhs;
            bool m_within_rhs;

            [[nodiscard]] std::optional<SymbolInfo> lookup_global(const std::string& symbol);
            [[nodiscard]] std::optional<SymbolInfo> lookup_local(const std::string& symbol);
            [[nodiscard]] std::optional<SymbolInfo> lookup_constant(const std::string& symbol);
            [[nodiscard]] const std::string* lookup_external_name_ptr(const std::string& symbol);

            //? NOTE: Use this for recording builtin names, allowing the compiler to understand those symbols in name -> location resolution.
            [[maybe_unused]] std::optional<SymbolInfo> record_global(const std::string& symbol, std::unique_ptr<ObjectBase> object_ptr) noexcept;
            [[nodiscard]] std::optional<SymbolInfo> record_ident(std::string symbol) noexcept;

            [[nodiscard]] std::optional<SymbolInfo> record_constant(const std::string& symbol, FastValue value);
            [[nodiscard]] std::optional<SymbolInfo> record_constant(const std::string& symbol, std::unique_ptr<ObjectBase> object_ptr);

            // ? For 0 argument opcodes only!
            void encode_instruction(Opcode op);

            // ? For single (wide) argument opcodes only!
            void encode_instruction(Opcode op, std::int16_t wide);

            void encode_instruction(Opcode op, std::int8_t tiny, std::int16_t wide);

            [[nodiscard]] bool emit_builtin_or_const(const expr::Name* name);

            [[nodiscard]] bool emit_bool(const expr::Bool* expr);
            [[nodiscard]] bool emit_num(const expr::Num* expr);
            [[nodiscard]] bool emit_string(const expr::String* expr);

            [[nodiscard]] bool emit_name(const expr::Name* name);
            [[nodiscard]] bool emit_assignment(const expr::Assignment* expr);
            [[nodiscard]] bool emit_match(const expr::Match* expr);
            [[nodiscard]] bool emit_block(const expr::Block* expr);
            //? DrkWithT: I've made conditional not a normal builtin since it emits directly to an instruction sequence using JUMP_ELSE and JUMP. This avoids eager evaluation of true and false "args".
            [[nodiscard]] bool help_emit_conditional(const expr::ExprPtr& check_arg, const expr::ExprPtr& yes, const expr::ExprPtr& no);

            [[nodiscard]] bool emit_call(const expr::Call* expr);
            [[nodiscard]] bool emit_closure(const expr::Closure* expr);
            [[nodiscard]] bool emit_prefix(const expr::Prefix* prefix_decl);
            [[nodiscard]] bool emit_infix(const expr::Infix* infix_decl);
            [[nodiscard]] bool emit_unary_op(const expr::UnaryOp* unary_op);
            [[nodiscard]] bool emit_binary_op(const expr::BinOp* binary_op);

            [[nodiscard]] bool emit_expr(const expr::ExprPtr& any_expr);

        public:
            constexpr Compiler()
            : m_global_builtin_scope {SymbolScope {
                .symbols = {},
                .name = "(top-level)",
                .next_constant_id = 0,
                .next_ident_key_id = 0
            }}, m_scopes {}, m_globals {}, m_codes {}, m_bindable_lambda_id {}, m_within_call {false}, m_within_lhs {false}, m_within_rhs {false} {
                m_scopes.emplace_back(SymbolScope {
                    .symbols = {},
                    .name = "(top-level)",
                    .next_constant_id = 0,
                    .next_ident_key_id = 0
                });
                m_codes.emplace_back(Chunk {
                    .code = {},
                    .constants = {},
                    .identifiers = {}
                });
            }

            [[maybe_unused]] bool add_builtin_object(std::string name, std::unique_ptr<ObjectBase> global);

            [[nodiscard]] std::expected<Program, std::string> operator()(const std::vector<expr::ExprPtr>& ast, const std::string& source);
        };
    }    
}