#include <span>
#include <print>
#include "../VM/Cherry.hxx"

namespace pie {
    namespace vm {
        [[maybe_unused]] bool pie_native_print(std::any context, int argc) {
            auto state = std::any_cast<Context*>(context);

            // ? BP = Temp_0 = <callee-reference>
            const int callee_bp = state->sp - argc;

            std::span all_args {
                // ? Always designate BP + 1 as the environment reference, just like 'this' in JS.
                // ? All values  AFTER  BP + 1 are the explicit arguments / temporaries.
                state->stack.data() + callee_bp + 1,
                static_cast<std::size_t>(argc)
            };

            for (const auto& arg : all_args) {
                std::print("{} ", arg.to_string());
            }

            std::println("");

            state->status = VMStatus::pending;
            return true;
        }

        void op_nop(Context& c) {
            c.ip++;
        }

        void op_start_env(Context& c) {
            c.envs.emplace_back(Environment {});
            c.ip++;
        }

        void op_end_env(Context& c) {
            c.envs.pop_back();
            c.ip++;
        }

        void op_ref_env(Context& c) {
            c.sp++;
            c.stack[c.sp] = c.stack[c.bp + 1];
            c.ip++;
        }

        void op_bind(Context& c) {
            c.envs.back().put_item(
                FastValue {c.idp + c.ip->arg_wide},
                c.stack[c.sp],
                BuiltIn::ContainerPolicy::key
            );
            c.sp--;
            c.ip++;
        }

        void op_lookup(Context& c) {
            c.sp++;
            c.stack[c.sp] = c.envs.back().get_item(
                FastValue {c.idp + c.ip->arg_wide},
                BuiltIn::ContainerPolicy::key
            );
            c.ip++;
        }

        void op_deref(Context& c) {
            c.stack[c.sp] = c.stack[c.sp].resolve();
            c.ip++;
        }

        void op_push_global(Context& c) {
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
                c.sp--;
                c.ip += c.ip->arg_wide;
            } else {
                c.sp--;
                c.ip++;
            }
        }

        // TODO: support custom functions besides built-ins!
        void op_call(Context& c) {
            if (auto callee_object = c.stack[c.sp - (1 + c.ip->arg_wide)].as_object(); callee_object != nullptr) {
                // ! NOTE: The callee MUST push a proper call frame before continuation IF NOT native. However, its CALLEE_BP at CALLER_SP - ARGC - 1 MUST be the return value.
                callee_object->call(std::addressof(c), c.ip->arg_wide + 1);
            } else {
                c.status = VMStatus::bad_call;
            }
        }

        void op_ret(Context& c) {
            const auto& [caller_ip, caller_cvp, caller_idp, caller_bp, callee_bp] = c.frames.back();

            c.stack[c.bp] = c.stack[c.sp];
            c.bp = caller_bp;
            c.sp = callee_bp;

            c.ip = caller_ip;
            c.cvp = caller_cvp;
            c.idp = caller_idp;

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
