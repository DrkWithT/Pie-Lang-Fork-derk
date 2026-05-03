#include <cstdint>
#include <utility>
#include <type_traits>
#include <format>

#include "../VM/Functions.hxx"

namespace pie {
    namespace vm {
        [[nodiscard]] std::string BuiltIn::get_typename() const {
            return "BuiltIn";
        }

        [[nodiscard]] ObjectBase* BuiltIn::get_super() noexcept {
            return nullptr;
        }

        //? Use this for resolving original values behind references / accessing data in Any values e.g `x: Any = 1234, x = {1, 2, 3, 4};`
        [[nodiscard]] const FastValue* BuiltIn::resolve() const {
            return nullptr;
        }

        [[nodiscard]] FastValue* BuiltIn::resolve() {
            return nullptr;
        }

        [[nodiscard]] bool BuiltIn::call(std::any state_ref, int arg_count) {
            // ! invoke native procedure here, passing VM state in the std::any... The native callee MUST properly follow the pre-call conventions as shown in Functions.hxx, preceeding the BuiltIn class.
            return m_native_ptr(state_ref, arg_count);
        }

        [[nodiscard]] FastValue BuiltIn::neg() {
            return {};
        }

        [[nodiscard]] FastValue BuiltIn::pow(const FastValue& rhs) {
            return {};
        }

        [[nodiscard]] FastValue BuiltIn::mul(const FastValue& rhs) {
            return {};
        }

        [[nodiscard]] FastValue BuiltIn::div(const FastValue& rhs) {
            return {};
        }

        [[nodiscard]] FastValue BuiltIn::mod(const FastValue& rhs) {
            return {};
        }

        [[nodiscard]] FastValue BuiltIn::add(const FastValue& rhs) {
            return {};
        }

        [[nodiscard]] FastValue BuiltIn::sub(const FastValue& rhs) {
            return {};
        }

        //? Converts an ObjectBase to a bool via truthiness rules.
        [[nodiscard]] bool BuiltIn::test( ) const {
            return true;
        }

        // ! NO AND implemented- The builtin_and should short-circuit LHS, RHS evaluations.
        // ! NO OR implemented- The builtin_and should short-circuit LHS, RHS evaluations.
        [[nodiscard]] int BuiltIn::to_int( ) const {
            return {};
        }

        [[nodiscard]] double BuiltIn::to_double() const {
            return {};
        }

        [[nodiscard]] std::string BuiltIn::to_string() const {
            return std::format("BuiltIn(m_name = '{}', m_fp = {})", m_name, reinterpret_cast<const void*>(m_native_ptr));
        }

        [[nodiscard]] bool BuiltIn::cmp_eq(const FastValue& rhs) const {
            return reinterpret_cast<const void*>(this) == std::addressof(rhs);
        }

        [[nodiscard]] bool BuiltIn::cmp_gt(const FastValue& rhs) const {
            return false;
        }

        [[nodiscard]] bool BuiltIn::cmp_geq(const FastValue& rhs) const {
            return false;
        }

        [[nodiscard]] bool BuiltIn::cmp_lt(const FastValue& rhs) const {
            return false;
        }

        [[nodiscard]] bool BuiltIn::cmp_leq(const FastValue& rhs) const {
            return false;
        }

        [[nodiscard]] FastValue BuiltIn::get_item(const FastValue& key, ContainerPolicy policy) const {
            return {};
        }

        //? Covers set / push / concat for objects. A special enum is passed in to indicate whether to attempt list / object operations.
        [[nodiscard]] FastValue BuiltIn::put_item(const FastValue& key, const FastValue& item, ContainerPolicy policy) {
            return {};
        }

        //? Covers remove / pop for container objects. A special enum is passed in to indicate whether to attempt list / object operations.
        [[nodiscard]] FastValue BuiltIn::del_item(const FastValue& key, ContainerPolicy policy) {
            return {};
        }
    }
}