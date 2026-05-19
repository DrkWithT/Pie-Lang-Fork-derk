#pragma once

#include <utility>
#include <any>
#include <array>
#include <string>
#include <string_view>

#include "../ExpVM/FastValue.hxx"
#include "../ExpVM/ByteCode.hxx"

namespace pie {
    namespace vm {
        using NativeFn = bool(*)(std::any, int argc);

        // ? Provides a native type for __builtin_xyz functions:
        // ? Convention of stack pre-call:
        // ? CALLEE_BP is SP - ARG_COUNT aka LOCAL 0... builtin-function reference
        // ? ENV reference at LOCAL 1... environment object reference, similar to a 'this' argument in JS.
        // ? LOCALS 2+... local variables or temporaries
        class BuiltIn : public ObjectBase {
        private:
            //! Any const char* passed here must have a static lifetime.
            std::string_view m_name;
            NativeFn m_native_ptr;
            int m_required_argc;

        public:
            constexpr BuiltIn(NativeFn fp, std::string_view name, int required_argc) noexcept
            : m_name {name}, m_native_ptr {fp}, m_required_argc {required_argc} {
            }

            [[nodiscard]] std::string get_typename() const override;

            [[nodiscard]] ObjectBase* get_super() noexcept override;

            void set_super(ObjectBase* p) noexcept override;

            [[nodiscard]] const FastValue* resolve() const override;

            [[nodiscard]] FastValue* resolve() override;

            //? If implemented, the VM state is affected, usually by preparing the stack offsets & call frame as additional state. Otherwise, return false for non-functional objects.
            [[nodiscard]] bool call(std::any state_ref, int arg_count) override;

            [[nodiscard]] FastValue neg() override;

            [[nodiscard]] FastValue pow(const FastValue& rhs) override;

            [[nodiscard]] FastValue mul(const FastValue& rhs) override;

            [[nodiscard]] FastValue div(const FastValue& rhs) override;

            [[nodiscard]] FastValue mod(const FastValue& rhs) override;

            [[nodiscard]] FastValue add(const FastValue& rhs) override;

            [[nodiscard]] FastValue sub(const FastValue& rhs) override;

            //? Converts an ObjectBase to a bool via truthiness rules.
            [[nodiscard]] bool test( ) const override;

            // ! NO AND implemented- The builtin_and should short-circuit LHS, RHS evaluations.
            // ! NO OR implemented- The builtin_and should short-circuit LHS, RHS evaluations.
            [[nodiscard]] int to_int( ) const override;

            [[nodiscard]] double to_double( ) const override;

            [[nodiscard]] std::string to_string() const override;

            [[nodiscard]] bool cmp_eq(const FastValue& rhs) const override;

            [[nodiscard]] bool cmp_gt(const FastValue& rhs) const override;

            [[nodiscard]] bool cmp_geq(const FastValue& rhs) const override;

            [[nodiscard]] bool cmp_lt(const FastValue& rhs) const override;

            [[nodiscard]] bool cmp_leq(const FastValue& rhs) const override;

            [[nodiscard]] FastValue get_item(const FastValue& key, ContainerPolicy policy) const override;

            //? Covers set / push / concat for objects. A special enum is passed in to indicate whether to attempt list / object operations.
            [[nodiscard]] FastValue put_item(const FastValue& key, const FastValue& item, ContainerPolicy policy) override;

            //? Covers remove / pop for container objects. A special enum is passed in to indicate whether to attempt list / object operations.
            [[nodiscard]] FastValue del_item(const FastValue& key, ContainerPolicy policy) override;
        };

        class Lambda : public ObjectBase {
        private:
            static constexpr std::array<std::string_view, static_cast<std::size_t>(Opcode::last)> lambda_op_names = {
                "nop",
                "start_env",
                "end_env",
                "ref_env",
                "bind",
                "lookup",
                "lookup_by_const",
                "deref",
                "push_global",
                "push_const",
                "pop_n",
                "jump",
                "jump_else",
                "call",
                "ret",
                "halt_errcode"
            };

            Chunk m_chunk;
            int m_required_argc;

        public:
            constexpr Lambda(Chunk chunk, int required_argc) noexcept
            : m_chunk (std::move(chunk)), m_required_argc {required_argc} {}

            [[nodiscard]] std::string get_typename() const override;

            [[nodiscard]] ObjectBase* get_super() noexcept override;

            void set_super(ObjectBase* p) noexcept override;

            [[nodiscard]] const FastValue* resolve() const override;

            [[nodiscard]] FastValue* resolve() override;

            //? If implemented, the VM state is affected, usually by preparing the stack offsets & call frame as additional state. Otherwise, return false for non-functional objects.
            [[nodiscard]] bool call(std::any state_ref, int arg_count) override;

            [[nodiscard]] FastValue neg() override;

            [[nodiscard]] FastValue pow(const FastValue& rhs) override;

            [[nodiscard]] FastValue mul(const FastValue& rhs) override;

            [[nodiscard]] FastValue div(const FastValue& rhs) override;

            [[nodiscard]] FastValue mod(const FastValue& rhs) override;

            [[nodiscard]] FastValue add(const FastValue& rhs) override;

            [[nodiscard]] FastValue sub(const FastValue& rhs) override;

            //? Converts an ObjectBase to a bool via truthiness rules.
            [[nodiscard]] bool test( ) const override;

            // ! NO AND implemented- The builtin_and should short-circuit LHS, RHS evaluations.
            // ! NO OR implemented- The builtin_and should short-circuit LHS, RHS evaluations.
            [[nodiscard]] int to_int( ) const override;

            [[nodiscard]] double to_double( ) const override;

            [[nodiscard]] std::string to_string() const override;

            [[nodiscard]] bool cmp_eq(const FastValue& rhs) const override;

            [[nodiscard]] bool cmp_gt(const FastValue& rhs) const override;

            [[nodiscard]] bool cmp_geq(const FastValue& rhs) const override;

            [[nodiscard]] bool cmp_lt(const FastValue& rhs) const override;

            [[nodiscard]] bool cmp_leq(const FastValue& rhs) const override;

            [[nodiscard]] FastValue get_item(const FastValue& key, ContainerPolicy policy) const override;

            //? Covers set / push / concat for objects. A special enum is passed in to indicate whether to attempt list / object operations.
            [[nodiscard]] FastValue put_item(const FastValue& key, const FastValue& item, ContainerPolicy policy) override;

            //? Covers remove / pop for container objects. A special enum is passed in to indicate whether to attempt list / object operations.
            [[nodiscard]] FastValue del_item(const FastValue& key, ContainerPolicy policy) override;
        };
    }
}
