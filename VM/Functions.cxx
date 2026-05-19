#include <cstdint>
#include <utility>
#include <type_traits>
#include <any>
#include <sstream>
#include <format>

#include "../VM/Functions.hxx"
#include "../VM/Cherry.hxx"

namespace pie {
    namespace vm {
        [[nodiscard]] std::string BuiltIn::get_typename() const {
            return "BuiltIn";
        }

        [[nodiscard]] ObjectBase* BuiltIn::get_super() noexcept {
            return nullptr;
        }

        void BuiltIn::set_super([[maybe_unused]] ObjectBase* p) noexcept {};

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

        [[nodiscard]] FastValue BuiltIn::pow([[maybe_unused]] const FastValue& rhs) {
            return {};
        }

        [[nodiscard]] FastValue BuiltIn::mul([[maybe_unused]] const FastValue& rhs) {
            return {};
        }

        [[nodiscard]] FastValue BuiltIn::div([[maybe_unused]] const FastValue& rhs) {
            return {};
        }

        [[nodiscard]] FastValue BuiltIn::mod([[maybe_unused]] const FastValue& rhs) {
            return {};
        }

        [[nodiscard]] FastValue BuiltIn::add([[maybe_unused]] const FastValue& rhs) {
            return {};
        }

        [[nodiscard]] FastValue BuiltIn::sub([[maybe_unused]] const FastValue& rhs) {
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

        [[nodiscard]] bool BuiltIn::cmp_eq([[maybe_unused]] const FastValue& rhs) const {
            return reinterpret_cast<const void*>(this) == std::addressof(rhs);
        }

        [[nodiscard]] bool BuiltIn::cmp_gt([[maybe_unused]] const FastValue& rhs) const {
            return false;
        }

        [[nodiscard]] bool BuiltIn::cmp_geq([[maybe_unused]] const FastValue& rhs) const {
            return false;
        }

        [[nodiscard]] bool BuiltIn::cmp_lt([[maybe_unused]] const FastValue& rhs) const {
            return false;
        }

        [[nodiscard]] bool BuiltIn::cmp_leq([[maybe_unused]] const FastValue& rhs) const {
            return false;
        }

        [[nodiscard]] FastValue BuiltIn::get_item([[maybe_unused]] const FastValue& key, [[maybe_unused]] ContainerPolicy policy) const {
            return {};
        }

        //? Covers set / push / concat for objects. A special enum is passed in to indicate whether to attempt list / object operations.
        [[nodiscard]] FastValue BuiltIn::put_item([[maybe_unused]] const FastValue& key, [[maybe_unused]] const FastValue& item, [[maybe_unused]] ContainerPolicy policy) {
            return {};
        }

        //? Covers remove / pop for container objects. A special enum is passed in to indicate whether to attempt list / object operations.
        [[nodiscard]] FastValue BuiltIn::del_item([[maybe_unused]] const FastValue& key, [[maybe_unused]] ContainerPolicy policy) {
            return {};
        }



        [[nodiscard]] std::string Lambda::get_typename() const {
            return "Lambda";
        }

        [[nodiscard]] ObjectBase* Lambda::get_super() noexcept {
            return nullptr;
        }

        void Lambda::set_super([[maybe_unused]] ObjectBase* p) noexcept {};

        [[nodiscard]] const FastValue* Lambda::resolve() const {
            return nullptr;
        }

        [[nodiscard]] FastValue* Lambda::resolve() {
            return nullptr;
        }

        //? If implemented, the VM state is affected, usually by preparing the stack offsets & call frame as additional state. Otherwise, return false for non-functional objects.
        [[nodiscard]] bool Lambda::call(std::any state_ref, int arg_count) {
            if (arg_count > m_required_argc + 1) {
                return false;
            }

            auto state = std::any_cast<Context*>(state_ref);

            const auto caller_ret_ip_v = state->ip + 1;
            const auto caller_cvp = state->cvp;
            const auto caller_idp = state->idp;
            const int caller_bp_v = state->bp;
            const int callee_bp_v = state->sp - arg_count;

            state->frames.emplace_back(
                caller_ret_ip_v,
                caller_cvp,
                caller_idp,
                caller_bp_v,
                callee_bp_v
            );

            state->bp = callee_bp_v;
            state->ip = m_chunk.code.data();
            state->cvp = m_chunk.constants.data();
            state->idp = m_chunk.identifiers.data();

            state->stack[state->bp + 1] = FastValue {
                // ? Put new environment whose parent is the caller's.
                &state->envs.emplace_back(Environment {nullptr})
            };
            state->envs.back().set_super(&state->envs.at(state->envs.size() - 2));

            return true;
        }

        [[nodiscard]] FastValue Lambda::neg() {
            return {};
        }

        [[nodiscard]] FastValue Lambda::pow([[maybe_unused]] const FastValue& rhs) {
            return {};
        }

        [[nodiscard]] FastValue Lambda::mul([[maybe_unused]] const FastValue& rhs) {
            return {};
        }

        [[nodiscard]] FastValue Lambda::div([[maybe_unused]] const FastValue& rhs) {
            return {};
        }

        [[nodiscard]] FastValue Lambda::mod([[maybe_unused]] const FastValue& rhs) {
            return {};
        }

        [[nodiscard]] FastValue Lambda::add([[maybe_unused]] const FastValue& rhs) {
            return {};
        }

        [[nodiscard]] FastValue Lambda::sub([[maybe_unused]] const FastValue& rhs) {
            return {};
        }

        //? Converts an ObjectBase to a bool via truthiness rules.
        [[nodiscard]] bool Lambda::test() const {
            return true;
        }

        // ! NO AND implemented- The builtin_and should short-circuit LHS, RHS evaluations.
        // ! NO OR implemented- The builtin_and should short-circuit LHS, RHS evaluations.
        [[nodiscard]] int Lambda::to_int() const {
            return {};
        }

        [[nodiscard]] double Lambda::to_double() const {
            return {};
        }

        [[nodiscard]] std::string Lambda::to_string() const {
            std::ostringstream sout;

            sout << "Lambda(...) {\n";

            for (auto code_pos = 0; const auto& [arg_wide, arg_tiny, opcode] : m_chunk.code) {
                sout << std::format(
                    "\t{}: {}   {} {}\n",
                    code_pos,
                    lambda_op_names.at(static_cast<std::uint16_t>(opcode)),
                    static_cast<std::uint16_t>(arg_tiny),
                    arg_wide
                );
                code_pos++;
            }

            sout << "}\n";

            return sout.str();
        }

        [[nodiscard]] bool Lambda::cmp_eq(const FastValue& rhs) const {
            return reinterpret_cast<const void*>(this) == std::addressof(rhs);
        }

        [[nodiscard]] bool Lambda::cmp_gt([[maybe_unused]] const FastValue& rhs) const {
            return false;
        }

        [[nodiscard]] bool Lambda::cmp_geq([[maybe_unused]] const FastValue& rhs) const {
            return false;
        }

        [[nodiscard]] bool Lambda::cmp_lt([[maybe_unused]] const FastValue& rhs) const {
            return false;
        }

        [[nodiscard]] bool Lambda::cmp_leq([[maybe_unused]] const FastValue& rhs) const {
            return false;
        }

        [[nodiscard]] FastValue Lambda::get_item([[maybe_unused]] const FastValue& key, [[maybe_unused]] ContainerPolicy policy) const {
            return {};
        }

        //? Covers set / push / concat for objects. A special enum is passed in to indicate whether to attempt list / object operations.
        [[nodiscard]] FastValue Lambda::put_item([[maybe_unused]] const FastValue& key, [[maybe_unused]] const FastValue& item, [[maybe_unused]] ContainerPolicy policy) {
            return {};
        }

        //? Covers remove / pop for container objects. A special enum is passed in to indicate whether to attempt list / object operations.
        [[nodiscard]] FastValue Lambda::del_item([[maybe_unused]] const FastValue& key, [[maybe_unused]] ContainerPolicy policy) {
            return {};
        }
    }
}