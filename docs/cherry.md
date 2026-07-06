# Cherry: A bytecode VM for Pie

## Contents
 - Introduction
 - Design
 - Code Structure
 - Instructions
 - Values
    - Primitives
    - Objects
 - Object System (TODO)
    - Shapes
    - Inline Caches
 - GC (TODO)

## Introduction

### Background
So far, Pie has been an educational language with a tree-walker for initial prototyping. However, Pie has only been tested on smaller scripts and runs quite slow...

### Purpose
**CherryVM** will scale Pie to more practical & demanding programs. It aims for both correctness and around 7x performance gains.

## Design

### Key Ideas
Cherry is a typical stack bytecode VM with these design choices:
 1. Stack based bytecode: simplicity
 2. Switch & loop dispatch: standard C++
 3. Shape & IC system: optimize objects
 4. Mark and Sweep GC: simplicity & speed gain over ref-counting
 5. "Plug in and Play" design: easy to configure or change behavior and builtin globals.

## Code Structure

### Overall Pipeline
 1. Existing frontend: Lexer **=>** Preprocessor **=>** Parser **=>** AST **=>** Analysis
    - See `src/Lex`, `src/Preprocessor`, `src/Parser`, `src/Expr`, `src/Type`, `src/Analysis`
 2. To tree-walker: Frontend **=>** Tree-Walker
    - See `src/Interp`
 3. Planned: Frontend **=>** CherryVM or Tree-Walker
    - See `src/VM`

## Instructions

### Notation
 - `... -> ...`: shorthand for VM stack effects
 - `^`: denotes stack top after an instruction runs

### Opcodes
 - NOP: no args, `IP++`
 - DUP1: no args, `a -> a ^a`
 - PUSH_K: takes a constant ID, `... -> ... k^`
 - POP_N: takes a pop count, `... v0 v1 ...vN -> ... v0^ `
 - INIT_VAR: takes a mutability flag, creating a binding of a key to a value within the environment object. `key value -> ^`, `env[key] = value`
 - GET_VAR: `key -> value`, `value = env[key]`
 - REF_VAR: `key -> &value`, `&value = env.ref(key)`
 - DEL_VAR: `key -> ^`, `env.del(key)`
 - JUMP_ELSE: takes an offset with a should-pop flag, allowing a falsy conditional jump. `val -> ^` or `val -> val`, `IP += OFFSET`
 - JUMP_IF: takes an offset with a should-pop flag, allowing a conditional jump. `val -> ^` or `val -> val`, `IP += OFFSET`
 - JUMP: unconditional jump by offset in instructions, allowing an unconditional jump. No stack effect. `IP += OFFSET`
 - INTRIN_CALL: takes an ID to the intrinsic, native function (`__builtin_xyz`).
 - CALL: call function reference with `arg-count` stack values. `func arg1 arg2 .. argN -> result^`
 - RET: No args. Returns temporary to caller and restores caller frame. `... val -> val`, `IP = CALLER_IP, BP = CALLER_BP, SP = CALLEE_BP, etc.`

## Values
 - Primitives
    - Types are boolean, int, double, and string.
 - Objects
    - Types are "maps", functions, or other native objects which behave like maps.
    - Follow the prototype based OO model.
 - See _Object System_ for implementation details.

**MORE DOCS TO BE ADDED**
