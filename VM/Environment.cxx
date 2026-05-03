#include <cstdint>
#include <utility>
#include "../VM/Environment.hxx"

namespace pie {
    namespace vm {
        [[nodiscard]] std::string Environment::get_typename() const {
            return "Environment";
        }

        [[nodiscard]] ObjectBase* Environment::get_super() noexcept {
            return m_super;
        }

        [[nodiscard]] const FastValue* Environment::resolve() const {
            return nullptr;
        }

        [[nodiscard]] FastValue* Environment::resolve() {
            return nullptr;
        }

        [[nodiscard]] bool Environment::call(std::any state_ref, int arg_count) {
            return false;
        }

        [[nodiscard]] FastValue Environment::neg() {
            return {};
        }

        [[nodiscard]] FastValue Environment::pow(const FastValue& rhs) {
            return {};
        }

        [[nodiscard]] FastValue Environment::mul(const FastValue& rhs) {
            return {};
        }

        [[nodiscard]] FastValue Environment::div(const FastValue& rhs) {
            return {};
        }

        [[nodiscard]] FastValue Environment::mod(const FastValue& rhs) {
            return {};
        }

        [[nodiscard]] FastValue Environment::add(const FastValue& rhs) {
            return {};
        }

        [[nodiscard]] FastValue Environment::sub(const FastValue& rhs) {
            return {};
        }

        //? Converts an ObjectBase to a bool via truthiness rules.
        [[nodiscard]] bool Environment::test() const {
            return !m_data.empty();
        }

        // ! NO AND implemented- The builtin_and should short-circuit LHS, RHS evaluations.
        // ! NO OR implemented- The builtin_and should short-circuit LHS, RHS evaluations.
        [[nodiscard]] int Environment::to_int() const {
            return {};
        }

        [[nodiscard]] double Environment::to_double() const {
            return {};
        }

        [[nodiscard]] std::string Environment::to_string() const {
            return "Environment {...}";
        }

        [[nodiscard]] bool Environment::cmp_eq(const FastValue& rhs) const {
            return reinterpret_cast<const void*>(this) == std::addressof(rhs);
        }

        [[nodiscard]] bool Environment::cmp_gt(const FastValue& rhs) const {
            return false;
        }

        [[nodiscard]] bool Environment::cmp_geq(const FastValue& rhs) const {
            return false;
        }

        [[nodiscard]] bool Environment::cmp_lt(const FastValue& rhs) const {
            return false;
        }

        [[nodiscard]] bool Environment::cmp_leq(const FastValue& rhs) const {
            return false;
        }

        [[nodiscard]] FastValue Environment::get_item(const FastValue& key, ContainerPolicy policy) const {
            // TODO: handle access policy to account for indexing too.
            if (policy == ContainerPolicy::key && m_data.contains(key.as_str_ptr())) {
                return m_data.at(key.as_str_ptr());
            } else if (policy == ContainerPolicy::chase && m_super != nullptr) {
                return m_super->get_item(key, policy);
            }

            return {};
        }

        [[maybe_unused]] FastValue Environment::put_item(const FastValue& key, const FastValue& item, ContainerPolicy policy) {
            if (policy == ContainerPolicy::key) {
                return FastValue {m_data.insert_or_assign(key.as_str_ptr(), item).second};
            } else if (policy == ContainerPolicy::chase && m_super != nullptr) {
                return m_super->put_item(key, item, policy);
            }

            return FastValue {false};
        }

        [[maybe_unused]] FastValue Environment::del_item(const FastValue& key, ContainerPolicy policy) {
            if (policy != ContainerPolicy::chase) {
                //! For error handling, return true / false for operation success!
                return FastValue {m_data.erase(key.as_str_ptr()) >= 1};
            }

            return m_super->del_item(key, policy);
        }
    }
}