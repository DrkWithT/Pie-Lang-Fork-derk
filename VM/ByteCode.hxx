#pragma once

#include <cstdint>
#include <string>
#include <string_view>
#include <array>
#include <vector>
#include <print>

#include "../VM/FastValue.hxx"

namespace pie{
    namespace vm {

        enum class Opcode : std::uint8_t {
            // * Filler / pseudo ops
            nop,
            
            // * Begin environment ops
            start_env,  // ? pushes a new variable environment
            end_env,    // ? pops the current environment
            ref_env,
            bind,   // ? stores the top stack value into an environment entry via immediate string ID / ptr.
            lookup, // ? looks up an environment-associated value via immediate string ID / ptr.
            lookup_by_const,
            deref,
            
            // * Begin stack ops
            push_global,    // ? pushes a global object e.g __builtin_add() by immediate ID.
            push_const,     // ? pushes a function's chunk constant by immediate ID... It may be a string, number, or something else undeclared.
            pop_n,          // ? lazy pops N things from the stack
            
            // * Begin control flow
            jump,
            jump_else,
            call,   // ? invokes a function, built-in or custom, upon the stack... INVARIANT: the function, environment arg, and temporary locals pushed prior.
            ret,
            
            // * Extra ops
            halt_errcode,
            last
        };
        
        struct Instruction {
            std::int16_t arg_wide;
            std::int8_t arg_tiny;
            Opcode op;
        };
        
        struct Chunk {
            std::vector<Instruction> code;
            std::vector<FastValue> constants;
            std::vector<std::string> identifiers;
            
            friend constexpr void display_chunk_data(const Chunk& c, int id) {
                static constexpr std::array<std::string_view, static_cast<std::size_t>(Opcode::last)> opcode_names = {
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
                
                const auto& [c_code, c_consts, c_idents] = c;
                
                if (id <= 0) {
                    std::println("\x1b[1;33mMAIN CHUNK:\x1b[0m\n");
                }
                
                std::println("\x1b[1;33mCONSTANTS:\x1b[0m\n");
                for (int konst_id = 0; const auto& konst : c_consts) {
                    std::println("CONST {} = {}", konst_id, konst.to_string());
                    konst_id++;
                }
                
                std::println("\x1b[1;33mKEY STRINGS:\x1b[0m\n");
                for (int key_id = 0; const auto& key_str : c_idents) {
                    std::println("KEY {} = {}", key_id, key_str);
                    key_id++;
                }
                
                std::println("\x1b[1;33mCODE:\x1b[0m\n");
                for (int code_pos = 0; const auto& [wide_op, tiny_op, opcode] : c_code) {
                    std::println(
                        "{}: {}   t{}, w{}",
                        code_pos,
                        opcode_names.at(static_cast<std::uint32_t>(opcode)),
                        static_cast<std::int16_t>(tiny_op),
                        wide_op
                    );
                    code_pos++;
                }
            }
        };
        
        struct Program {
            Chunk main_code;
            std::vector<std::unique_ptr<ObjectBase>> globals;
            
            friend constexpr void display_all_bytecode(const Program& p) {
                const auto& [main_chunk, all_globals] = p;
                
                std::println("\x1b[1;33m---- BYTECODE DUMP ----\x1b[0m\n\n");

                std::println("\x1b[1;33m--- GLOBALS DUMP ---\x1b[0m\n");

                for (int object_id = 0, object_count = all_globals.size(); object_id < object_count; object_id++) {
                    if (const auto& object_ptr = all_globals.at(object_id); object_ptr) {
                        std::println("GLOBAL {} = {}\n", object_id, object_ptr->to_string());
                    }
                }

                // ? display bytecode for top-level statements
                display_chunk_data(main_chunk, 0);
            }
        };
    }
} // namespace pie