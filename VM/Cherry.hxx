#pragma once

#include <cstdint>
#include <utility>
#include <memory>
#include <array>
#include <vector>
#include <any>

#include "../VM/ByteCode.hxx"
#include "../VM/Environment.hxx"

namespace pie {
    namespace vm {
        enum class VMStatus : std::uint8_t {
            pending,
            ok,
            bad_alloc,
            bad_call,
            bad_recursion,
            user_abort
        };

        struct Frame {
            const Instruction* caller_ip;
            const FastValue* caller_cvp;
            const std::string* caller_idp;
            int caller_bp;
            int callee_bp;
        };

        struct Context {
            static constexpr int calls_max_depth = 128;

            Chunk main_code;
            std::vector<Environment> envs;                      // ? environment stack
            std::vector<std::unique_ptr<ObjectBase>> globals;   // ? global objects including built-ins
            std::vector<FastValue> stack;                       // ? temporaries stack
            std::vector<Frame> frames;
            const Instruction* ip;
            const FastValue* cvp;
            const std::string* idp;
            int bp;
            int sp;
            VMStatus status;

            explicit constexpr Context(Program p, int local_capacity) noexcept
            : main_code (std::move(p.main_code)), envs {}, globals (std::move(p.globals)), stack {}, frames {}, ip {nullptr}, cvp {nullptr}, idp {nullptr}, bp {0}, sp {0}, status {VMStatus::pending} {
                envs.reserve(calls_max_depth);
                stack.reserve(local_capacity);
                stack.resize(local_capacity);

                frames.emplace_back(Frame {
                    .caller_ip = nullptr,
                    .caller_cvp = nullptr,
                    .caller_idp = nullptr,
                    .caller_bp = bp,
                    .callee_bp = bp
                });

                ip = main_code.code.data();
                cvp = main_code.constants.data();
                idp = main_code.identifiers.data();
            }
        };

        // TODO: move native built-ins to a specific VM source... these are meant for a Fib 20 demo.
        [[maybe_unused]] bool pie_native_add(std::any context, int argc);
        [[maybe_unused]] bool pie_native_sub(std::any context, int argc);
        [[maybe_unused]] bool pie_native_lt(std::any context, int argc);
        [[maybe_unused]] bool pie_native_print(std::any context, int argc);
        [[maybe_unused]] bool pie_native_now(std::any context, int argc);

        void op_nop(Context& c);
        void op_start_env(Context& c);
        void op_end_env(Context& c);
        void op_ref_env(Context& c);
        void op_bind(Context& c);
        void op_lookup(Context& c);
        void op_lookup_by_const(Context& c);
        void op_deref(Context& c);
        void op_push_global(Context& c);
        void op_push_const(Context& c);
        void op_pop_n(Context& c);
        void op_jump(Context& c);
        void op_jump_else(Context& c);
        void op_call(Context& c);
        void op_ret(Context& c);
        void op_halt_errcode(Context& c);

        class VM {
        public:
            using opcode_fn = void(*)(Context&);

        private:
            std::array<opcode_fn, static_cast<std::size_t>(Opcode::last)> op_table;
            Context m_context;

        public:
            constexpr VM(Program prgm, int local_slots) noexcept
            : op_table {{
                &op_nop,
                &op_start_env, &op_end_env, &op_ref_env, &op_bind, &op_lookup, &op_lookup_by_const,
                &op_deref, &op_push_global, &op_push_const, &op_pop_n,
                &op_jump, &op_jump_else, &op_call, &op_ret, &op_halt_errcode
            }}, m_context (std::move(prgm), local_slots) {}

            [[nodiscard]] constexpr const FastValue& result() const noexcept {
                return m_context.stack[0];
            }

            [[nodiscard]] constexpr VMStatus status() const noexcept {
                return m_context.status;
            }

            [[nodiscard]] constexpr bool run() {
                while (m_context.status == VMStatus::pending) {
                    op_table[std::to_underlying(m_context.ip->op)](m_context);
                }

                return m_context.status == VMStatus::ok;
            }
        };
    }
}

