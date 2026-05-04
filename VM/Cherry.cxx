#include <span>
#include <chrono>
#include <print>
#include "../VM/Cherry.hxx"
#include "../VM/Functions.hxx"

namespace pie {
    namespace vm {
        [[maybe_unused]] bool pie_native_add(std::any context, int argc) {
            auto state = std::any_cast<Context*>(context);

            // ? BP = Temp_0 = <callee-reference>
            const int callee_bp = state->sp - argc;

            // ? In-place addition: RET = LHS + RHS
            // ? BP + 1 + 2 = TEMPORARY 1 AKA RHS
            const auto& rhs_v = state->stack[callee_bp + 3];
            // ? BP + 1 + 1 = TEMPORARY 0 AKA LHS
            state->stack[callee_bp] = FastValue {state->stack[callee_bp + 2].add(rhs_v)};

            // ? Point SP to RET temporary.
            state->sp = callee_bp;
            state->ip++;

            return true;
        }

        [[maybe_unused]] bool pie_native_sub(std::any context, int argc) {
            auto state = std::any_cast<Context*>(context);

            // ? BP = Temp_0 = <callee-reference>
            const int callee_bp = state->sp - argc;

            // ? In-place subtraction: RET = LHS - RHS
            // ? BP + 1 + 1 + 1 => TEMPORARY 1 AKA RHS
            const auto& rhs_v = state->stack[callee_bp + 3];
            // ? BP + 1 + 1 + 0 => TEMPORARY 0 AKA LHS
            state->stack[callee_bp] = FastValue {state->stack[callee_bp + 2].sub(rhs_v)};

            // ? Point SP to RET temporary.
            state->sp = callee_bp;
            state->ip++;

            return true;
        }

        [[maybe_unused]] bool pie_native_lt(std::any context, int argc) {
            auto state = std::any_cast<Context*>(context);

            // ? BP = Temp_0 = <callee-reference>
            const int callee_bp = state->sp - argc;

            // ? Comparison: RET = LHS < RHS
            // ? BP + 1 + 2 = TEMPORARY 1 AKA RHS
            const auto& rhs_v = state->stack[callee_bp + 3];
            // ? BP + 1 + 1 = TEMPORARY 0 AKA LHS
            state->stack[callee_bp] = FastValue {state->stack[callee_bp + 2].cmp_lt(rhs_v)};

            // ? Point SP to RET temporary.
            state->sp = callee_bp;
            state->ip++;

            return true;
        }

        [[maybe_unused]] bool pie_native_print(std::any context, int argc) {
            auto state = std::any_cast<Context*>(context);

            // ? BP = Temp_0 = <callee-reference>
            const int callee_bp = state->sp - argc;

            std::span all_args {
                // ? Always designate BP + 1 as the environment reference, just like 'this' in JS.
                // ? All values  AFTER  BP + 1 are the explicit arguments / temporaries.
                state->stack.data() + callee_bp + 2,
                (argc > 0) ? static_cast<std::size_t>(argc) - 1 : 0
            };

            for (const auto& arg : all_args) {
                std::print("{} ", arg.to_string());
            }

            std::println("");

            // ? Point SP to RET temporary.
            state->stack[callee_bp] = FastValue {true};
            state->sp = callee_bp;

            state->ip++;

            return true;
        }

        [[maybe_unused]] bool pie_native_now(std::any context, int argc) {
            auto state = std::any_cast<Context*>(context);

            // ? BP = Temp_0 = <callee-reference>
            const int callee_bp = state->sp - argc;

            auto now_ms = std::chrono::time_point_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now());

            state->stack[callee_bp] = FastValue {static_cast<FastValue::bigint_type>(now_ms.time_since_epoch().count())};
            state->sp = callee_bp;

            state->ip++;

            return true;
        }

        void op_nop(Context& c) {
            c.ip++;
        }

        void op_start_env(Context& c) {
            if (!c.envs.empty()) {
                c.envs.emplace_back(Environment {
                    c.envs.back().get_super() // ? parent ENV
                });
            } else {
                c.envs.emplace_back(Environment {});
            }

            c.ip++;
        }

        void op_end_env(Context& c) {
            c.envs.pop_back();
            c.ip++;
        }

        void op_ref_env(Context& c) {
            c.sp++;
            c.stack[c.sp] = FastValue {&c.envs.back()};
            c.ip++;
        }

        void op_bind(Context& c) {
            c.envs.back().put_item(
                FastValue {c.idp + c.ip->arg_wide},
                c.stack[c.sp].resolve(),
                BuiltIn::ContainerPolicy::key
            );
            c.sp--;
            c.ip++;
        }

        void op_lookup(Context& c) {
            c.sp++;
            c.stack[c.sp] = c.envs.back().get_item(
                FastValue {c.idp + c.ip->arg_wide},
                BuiltIn::ContainerPolicy::chase
            ).resolve();
            c.ip++;
        }

        void op_lookup_by_const(Context& c) {
            c.sp++;
            c.stack[c.sp] = c.envs.back().get_item(
                c.cvp[c.ip->arg_wide],
                BuiltIn::ContainerPolicy::chase
            ).resolve();
            c.ip++;
        }

        void op_deref(Context& c) {
            c.stack[c.sp] = c.stack[c.sp].resolve();
            c.ip++;
        }

        void op_push_global(Context& c) {
            c.sp++;
            c.stack[c.sp] = FastValue {c.globals.at(c.ip->arg_wide).get()};
            c.ip++;
        }

        void op_push_const(Context& c) {
            c.sp++;
            c.stack[c.sp] = c.cvp[c.ip->arg_wide];
            c.ip++;
        }

        void op_pop_n(Context& c) {
            c.sp -= c.ip->arg_tiny;
            c.ip++;
        }

        void op_jump(Context& c) {
            c.ip += c.ip->arg_wide;
        }

        void op_jump_else(Context& c) {
            if (const auto& temp = c.stack[c.sp]; !temp.test()) {
                c.ip += c.ip->arg_wide;
            } else {
                c.ip++;
            }

            c.sp -= c.ip->arg_tiny;
        }

        // TODO: support custom functions besides built-ins!
        void op_call(Context& c) {
            if (auto callee_object = c.stack[c.sp - (1 + c.ip->arg_wide)].as_object(); callee_object != nullptr) {
                // ! NOTE: The callee MUST push a proper call frame before continuation IF NOT native. However, its CALLEE_BP at CALLER_SP - ARGC - 1 MUST be the return value.
                callee_object->call(std::addressof(c), c.ip->arg_wide + 1);
            } else if (c.envs.size() >= static_cast<std::size_t>(Context::calls_max_depth)) {
                c.status = VMStatus::bad_recursion;
            } else {
                c.status = VMStatus::bad_call;
            }
        }

        void op_ret(Context& c) {
            const auto& [caller_ip, caller_cvp, caller_idp, caller_bp, callee_bp] = c.frames.back();

            c.stack[callee_bp] = c.stack[c.sp];
            c.bp = caller_bp;
            c.sp = callee_bp;

            c.ip = caller_ip;
            c.cvp = caller_cvp;
            c.idp = caller_idp;

            c.envs.pop_back();

            c.frames.pop_back();

            if (c.frames.empty()) {
                if (c.status == VMStatus::pending) {
                    c.status = VMStatus::ok;
                }
            }
        }
        
        void op_halt_errcode(Context& c) {
            c.status = VMStatus::user_abort;
        }
    }
}
