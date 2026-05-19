#pragma once

#include <string>
#include <any>
#include <unordered_map>

#include "../ExpVM/FastValue.hxx"

namespace pie {
    namespace vm {
        class Environment : public ObjectBase {
        private:
            std::unordered_map<const std::string*, FastValue> m_data;
            ObjectBase* m_super;

        public:
            constexpr Environment() noexcept
            : Environment(nullptr) {}

            constexpr Environment(ObjectBase* parent) noexcept
            : m_data {}, m_super {parent} {}


            [[nodiscard]] std::string get_typename() const override;

            [[nodiscard]] ObjectBase* get_super() noexcept override;

            void set_super(ObjectBase* p) noexcept override;

            [[nodiscard]] const FastValue* resolve() const override;

            [[nodiscard]] FastValue* resolve() override;

            [[nodiscard]] bool call(std::any state_ref, int arg_count) override;

            [[nodiscard]] FastValue neg() override;

            [[nodiscard]] FastValue pow(const FastValue& rhs) override;

            [[nodiscard]] FastValue mul(const FastValue& rhs) override;

            [[nodiscard]] FastValue div(const FastValue& rhs) override;

            [[nodiscard]] FastValue mod(const FastValue& rhs) override;

            [[nodiscard]] FastValue add(const FastValue& rhs) override;

            [[nodiscard]] FastValue sub(const FastValue& rhs) override;

            //? Converts an ObjectBase to a bool via truthiness rules.
            [[nodiscard]] bool test() const override;

            // ! NO AND implemented- The builtin_and should short-circuit LHS, RHS evaluations.
            // ! NO OR implemented- The builtin_and should short-circuit LHS, RHS evaluations.
            [[nodiscard]] int to_int() const override;

            [[nodiscard]] double to_double() const override;

            [[nodiscard]] std::string to_string() const override;

            [[nodiscard]] bool cmp_eq(const FastValue& rhs) const override;

            [[nodiscard]] bool cmp_gt(const FastValue& rhs) const override;

            [[nodiscard]] bool cmp_geq(const FastValue& rhs) const override;

            [[nodiscard]] bool cmp_lt(const FastValue& rhs) const override;

            [[nodiscard]] bool cmp_leq(const FastValue& rhs) const override;

            [[nodiscard]] FastValue get_item(const FastValue& key, ContainerPolicy policy) const override;

            [[maybe_unused]] FastValue put_item(const FastValue& key, const FastValue& item, ContainerPolicy policy) override;

            [[maybe_unused]] FastValue del_item(const FastValue& key, ContainerPolicy policy) override;
        };
    }
}