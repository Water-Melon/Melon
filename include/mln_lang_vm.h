
/*
 * Copyright (C) Niklaus F.Schen.
 *
 * Bytecode VM for Melon's mln_lang interpreter.
 *
 * Design notes:
 *  - All Melang language features (arithmetic, bitwise ops including ~,
 *    reference parameters (&x), control flow, closures, sets, eval/watch/
 *    unwatch, operator overload guards, event-driven coroutine scheduling)
 *    are compiled to bytecode and executed on this VM.
 *  - The AST stack-walker in mln_lang.c remains present but is only used
 *    when MELANG_VM_OFF=1 is set in the environment (diagnostic mode).
 *  - Time-slicing (mln_lang_vm_step / M_LANG_DEFAULT_STEP) preserves
 *    Melang's cooperative multi-ctx scheduling model.
 *  - Watch reactivity: every slot/property/index assignment opcode fires
 *    vm_fire_watcher, mirroring the three trigger sites in the AST walker.
 */
#ifndef __MLN_LANG_VM_H
#define __MLN_LANG_VM_H

#include "mln_types.h"
#include "mln_string.h"

struct mln_lang_ctx_s;
struct mln_lang_func_detail_s;
struct mln_lang_var_s;

typedef enum {
    /* No-op / sentinel. */
    MLN_VOP_NOP = 0,

    /* Stack-manipulation. */
    MLN_VOP_POP,                /* discard top */
    MLN_VOP_DUP,                /* duplicate top */

    /* Constants / loads. */
    MLN_VOP_LOAD_NIL,           /* push nil */
    MLN_VOP_LOAD_TRUE,
    MLN_VOP_LOAD_FALSE,
    MLN_VOP_LOAD_INT,           /* b = index into iconsts */
    MLN_VOP_LOAD_LOCAL,         /* a = slot index */
    MLN_VOP_STORE_LOCAL,        /* a = slot index, pops top */

    /* Integer arithmetic and comparison fast paths.
     * Each pops 2, pushes 1. If the operand types are not both INT, the
     * VM falls back to the methods-table dispatch for that op (so e.g. a
     * runtime mix of int/real still works). */
    MLN_VOP_ADD,
    MLN_VOP_SUB,
    MLN_VOP_MUL,
    MLN_VOP_DIV,
    MLN_VOP_MOD,
    MLN_VOP_LT,
    MLN_VOP_LE,
    MLN_VOP_GT,
    MLN_VOP_GE,
    MLN_VOP_EQ,
    MLN_VOP_NE,

    /* Bitwise / shift binary ops (pops 2, pushes 1).
     * Integer fast-path: direct bit-operation. Non-int: methods-table
     * (cor_handler / cand_handler / cxor_handler / lmov_handler / rmov_handler).
     * NOTE: In Melang | & ^ << >> are NOT short-circuit; both operands are
     * always evaluated before these opcodes are emitted. */
    MLN_VOP_BOR,                /* a | b  — bitwise / logichigh OR  */
    MLN_VOP_BAND,               /* a & b  — bitwise / logichigh AND */
    MLN_VOP_BXOR,               /* a ^ b  — bitwise / logichigh XOR */
    MLN_VOP_LSHIFT,             /* a << b — move left               */
    MLN_VOP_RSHIFT,             /* a >> b — move right              */

    /* Control flow. b = signed offset relative to the next instruction. */
    MLN_VOP_JUMP,
    MLN_VOP_JUMP_IF_FALSE,      /* pops top */
    MLN_VOP_JUMP_IF_TRUE,       /* pops top */

    /* Calls. */
    MLN_VOP_CALL_SELF,          /* a = nargs */

    /* Returns. */
    MLN_VOP_RETURN,             /* pop top, set ctx->ret_var */
    MLN_VOP_RETURN_NIL,

    /* Phase B/C extensions. */
    MLN_VOP_ASSIGN_LOCAL,       /* a = slot, pop top, var_value_set into slot */
    MLN_VOP_NOT,                /* pop, push !cond as bool */
    MLN_VOP_NEG,                /* pop, push -value */
    MLN_VOP_LOAD_LOCAL_INC,     /* a = slot: post-increment slot int by 1, push old */
    MLN_VOP_LOAD_LOCAL_DEC,     /* a = slot: post-decrement slot int by 1, push old */
    MLN_VOP_INC_LOCAL_LOAD,     /* a = slot: pre-increment, push new */
    MLN_VOP_DEC_LOCAL_LOAD,     /* a = slot: pre-decrement, push new */

    /* Phase E extensions: literals, globals, locate chains, generic calls. */
    MLN_VOP_LOAD_REAL,          /* b = index into rconsts pool */
    MLN_VOP_LOAD_STRING,        /* b = index into sconsts pool */
    MLN_VOP_LOAD_GLOBAL,        /* b = sconsts index of name; resolves at runtime */
    MLN_VOP_GET_PROPERTY,       /* b = sconsts index of property; pop obj, push obj.prop */
    MLN_VOP_GET_INDEX,          /* pop key, pop array/obj, push elem */
    MLN_VOP_SET_PROPERTY,       /* b = sconsts index; pop val, pop obj, set obj.prop=val */
    MLN_VOP_SET_INDEX,          /* pop val, pop key, pop array, set array[key]=val */
    MLN_VOP_CALL_VALUE,         /* a = nargs; pop nargs, pop func var, dispatch */
    MLN_VOP_CALL_METHOD,        /* a = nargs; pop nargs, pop func var, pop obj, dispatch with this */

    /* Phase F: top-level cutover and remaining language features. */
    MLN_VOP_BIND_FUNC,          /* b = index into funcdefs[]; create func_detail
                                 * + val + var with funcdef.name, join symbol */
    MLN_VOP_BIND_SET,           /* b = index into setdefs[]; create set_detail
                                 * with members, join as SYMBOL_SET */
    MLN_VOP_NEW_OBJECT,         /* b = sconsts index of set name; build object */
    MLN_VOP_NEW_ARRAY,          /* push fresh empty array var */
    MLN_VOP_ARRAY_PUT,          /* pop val, pop key (nil = next index),
                                 * peek array on top, set arr[key]=val.
                                 * array stays on stack so the literal
                                 * compiler can chain entries. */
    /* Unary bitwise NOT (~x).  Pops 1, pushes 1.
     * Int fast-path: ~i.  Non-int: reverse_handler. */
    MLN_VOP_BITNOT,
    /* Reference-argument loads (&x).  Push a VAR_REFER wrapper (ref=0)
     * that shares the referenced variable's val.  funccall_run detects
     * ref==0 and passes the value by reference instead of by copy. */
    MLN_VOP_LOAD_LOCAL_REF,     /* a = slot index */
    MLN_VOP_LOAD_GLOBAL_REF,    /* b = sconsts index of name */
    /* Global variable write.  b = sconsts index of name.
     * Pops top, stores into the symbol found via global scope search,
     * fires any watcher, then pushes the new value back (like ASSIGN_LOCAL).
     * Used for compound assignment and ++/-- on global lvalues. */
    MLN_VOP_ASSIGN_GLOBAL,      /* b = sconsts index of name */
    /* Stack reorder helper for compound assignment and postfix ++/-- on index
     * lvalues.  SWAP2 swaps the 2nd and 3rd elements from the top, leaving
     * the top element unchanged.  Transforms [a, b, c] (c=top) to [b, a, c].
     * Used to reorder [arr, arr2, key, key2] into [arr, key, arr2, key2]
     * before GET_INDEX so the correct arr2/key2 pair is consumed. */
    MLN_VOP_SWAP2,              /* no operands; requires op_sp >= 3 */
    MLN_VOP_DEAD_AST,           /* sentinel: any path that the AST walker
                                 * would have taken is now an error. */
} mln_lang_vm_opcode_t;

typedef struct {
    mln_u8_t   op;
    mln_u8_t   a;
    mln_s16_t  b;
} mln_lang_vm_insn_t;

typedef struct mln_lang_vm_chunk_s {
    mln_lang_vm_insn_t          *code;
    mln_size_t                   code_len;
    mln_size_t                   code_cap;
    mln_s64_t                   *iconsts;
    mln_size_t                   iconsts_len;
    mln_size_t                   iconsts_cap;
    /* Phase E: real and string constant pools. String pointers alias the
     * AST's mln_string_t* values (ref-bumped at compile time so they stay
     * alive even if the cache is invalidated). */
    double                      *rconsts;
    mln_size_t                   rconsts_len;
    mln_size_t                   rconsts_cap;
    mln_string_t               **sconsts;
    mln_size_t                   sconsts_len;
    mln_size_t                   sconsts_cap;
    /* Phase F: AST pointers needed at runtime for binding/instantiation. */
    void                       **funcdefs;        /* mln_lang_funcdef_t*[]   */
    mln_size_t                   funcdefs_len;
    mln_size_t                   funcdefs_cap;
    void                       **setdefs;         /* mln_lang_set_t*[]       */
    mln_size_t                   setdefs_len;
    mln_size_t                   setdefs_cap;
    void                       **elemlists;       /* mln_lang_elemlist_t*[]  */
    mln_size_t                   elemlists_len;
    mln_size_t                   elemlists_cap;
    /* Names for slots, used at runtime by populate_locals to bind body-
     * locals to the symbol table. Indices 0..n_args-1 alias prototype->args,
     * and n_args..n_locals-1 are AST factor names borrowed from the body. */
    mln_string_t               **local_names;
    mln_size_t                   n_locals;
    mln_size_t                   max_stack;
    /* Owning ctx pool (used to free the chunk later). */
    void                        *pool;
} mln_lang_vm_chunk_t;

/* Try to compile prototype->data.stm into a bytecode chunk.
 * Returns:
 *    1  if compiled successfully (prototype->vm_chunk now non-NULL)
 *   -1  if the body uses features not supported by the compiler
 *    0  on internal error (e.g. allocation failure)
 *
 * NB: returns plain `int` rather than `mln_s8_t` because the latter is
 * typedef'd as `char` and `char` is unsigned by default on several
 * platforms (including some x86_64 Linux toolchains), which would silently
 * turn -1 into 255 at the call site. */
extern int
mln_lang_vm_try_compile(struct mln_lang_ctx_s *ctx,
                        struct mln_lang_func_detail_s *prototype);

/* Run the compiled chunk for prototype synchronously to completion.
 * Pushes a VM frame for `prototype`, drives dispatch until that frame
 * is popped, returns. Used by callers that need synchronous semantics
 * (e.g., embedded test harnesses). The funccall_run hook in mln_lang.c
 * uses mln_lang_vm_push_frame_for_call instead so the call participates
 * in the time-sliced vm_step dispatch loop. */
extern int
mln_lang_vm_run(struct mln_lang_ctx_s *ctx,
                struct mln_lang_func_detail_s *prototype);

/* Push a VM frame for a compiled prototype but do NOT drive dispatch.
 * The caller's outer vm_step will pick up the new frame on its next
 * iteration. Returns 0 on success, -1 on error. The function scope and
 * arg/closure bindings must already be set up (typically by
 * mln_lang_stack_handler_funccall_run before this is called). */
extern int
mln_lang_vm_push_frame_for_call(struct mln_lang_ctx_s *ctx,
                                struct mln_lang_func_detail_s *prototype);

/* Phase F top-level entry: compile the script's top-level stm chain into
 * a chunk and push the initial VM frame onto ctx->vm_frame_top. Returns:
 *    1  on success — caller drives execution via mln_lang_vm_step
 *   -1  on error
 *    0  if compilation failed (caller's responsibility) */
extern int
mln_lang_vm_run_toplevel(struct mln_lang_ctx_s *ctx);

/* Phase F3: time-sliced execution. Runs up to `budget` bytecode
 * instructions on ctx->vm_frame_top (and any frames pushed/popped
 * during the slice). Returns:
 *    1  if the entire frame stack is empty (script done)
 *   -1  on error
 *    0  if budget exhausted with frames still on the stack — caller
 *       (mln_lang_run_handler) should yield to the event loop and
 *       call us again. */
extern int
mln_lang_vm_step(struct mln_lang_ctx_s *ctx, int budget);

/* Free a chunk (pass NULL is OK). */
extern void mln_lang_vm_chunk_free(mln_lang_vm_chunk_t *chunk);

#endif
