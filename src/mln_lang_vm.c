
/*
 * Copyright (C) Niklaus F.Schen.
 *
 * Bytecode VM for mln_lang.
 *   Phase A : int arithmetic, simple if/else, recursive self-call.
 *   Phase B : assignment, real/bool/nil literals, NOT/NEG, suffix/prefix
 *             ++/--, equality, multi-binary chains (left-assoc to match AST).
 *   Phase C : while / for / break / continue, locals introduced inside the
 *             body (in addition to formal parameters).
 *   Phase D : (in mln_lang.c) compile-time refusal for prototypes that the
 *             VM cannot soundly run — operator-overload, function bodies that
 *             reference Watch/Unwatch by name, closures, sets / objects.
 *
 * Cross-function calls are still limited to self-recursion (CALL_SELF). A
 * function that calls anything other than itself is left to the AST walker.
 * That keeps the VM from ever needing to relinquish control mid-execution
 * (which would require yielding to the run loop with VM state on the heap).
 */
#include "mln_lang.h"
#include "mln_lang_vm.h"
#include "mln_lang_int.h"
#include "mln_lang_bool.h"
#include "mln_lang_nil.h"
#include "mln_alloc.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

extern mln_lang_var_t *mln_lang_var_create_int(mln_lang_ctx_t *ctx, mln_s64_t off, mln_string_t *name);
extern mln_lang_var_t *mln_lang_var_create_bool(mln_lang_ctx_t *ctx, mln_u8_t b, mln_string_t *name);
extern mln_lang_var_t *mln_lang_var_create_nil(mln_lang_ctx_t *ctx, mln_string_t *name);
extern void mln_lang_var_free(void *data);
extern int mln_lang_condition_is_true(mln_lang_var_t *var);
extern void mln_lang_ctx_set_ret_var(mln_lang_ctx_t *ctx, mln_lang_var_t *var);
extern int mln_lang_var_value_set(mln_lang_ctx_t *ctx, mln_lang_var_t *dest, mln_lang_var_t *src);
extern int mln_lang_symbol_node_join(mln_lang_ctx_t *ctx, mln_lang_symbol_type_t type, void *data);
extern int          mln_lang_funccall_val_add_arg(mln_lang_funccall_val_t *func, mln_lang_var_t *var);
extern mln_lang_funccall_val_t *mln_lang_funccall_val_new(mln_alloc_t *pool, mln_string_t *name);
extern void         mln_lang_funccall_val_free(mln_lang_funccall_val_t *func);

extern int mln_lang_stack_handler_funccall_run_compat(mln_lang_ctx_t *ctx,
                                                     mln_lang_stack_node_t *node,
                                                     mln_lang_funccall_val_t *funccall);
extern int mln_lang_withdraw_until_func_compat(mln_lang_ctx_t *ctx);
extern void mln_lang_funccall_val_object_add(mln_lang_funccall_val_t *func, mln_lang_val_t *obj_val);
extern mln_lang_var_t *mln_lang_var_create_string(mln_lang_ctx_t *ctx, mln_string_t *s, mln_string_t *name);
extern mln_lang_var_t *mln_lang_var_create_real(mln_lang_ctx_t *ctx, double f, mln_string_t *name);
extern mln_lang_symbol_node_t *mln_lang_symbol_node_search(mln_lang_ctx_t *ctx, mln_string_t *name, int local);
extern mln_lang_var_t *mln_lang_var_new(mln_lang_ctx_t *ctx, mln_string_t *name, mln_lang_var_type_t type, mln_lang_val_t *val, mln_lang_set_detail_t *in_set);
extern mln_lang_set_detail_t *mln_lang_ctx_get_class_compat(mln_lang_ctx_t *ctx);
extern mln_lang_object_t *mln_lang_object_new_compat(mln_lang_ctx_t *ctx, mln_lang_set_detail_t *in_set);
extern void mln_lang_object_free_compat(mln_lang_object_t *obj);
extern mln_lang_symbol_node_t *mln_lang_symbol_node_id_search_compat(mln_lang_ctx_t *ctx, mln_string_t *name);
extern mln_lang_array_t *mln_lang_array_new(mln_lang_ctx_t *ctx);
extern void mln_lang_array_free(mln_lang_array_t *array);
extern mln_lang_var_t *mln_lang_array_get(mln_lang_ctx_t *ctx, mln_lang_array_t *array, mln_lang_var_t *key);
extern mln_lang_val_t *mln_lang_val_new(mln_lang_ctx_t *ctx, mln_s32_t type, void *data);
extern void mln_lang_val_free(mln_lang_val_t *val);
extern void mln_lang_func_detail_free(mln_lang_func_detail_t *lfd);
extern mln_lang_func_detail_t *mln_lang_func_detail_new(mln_lang_ctx_t *ctx, mln_lang_func_type_t type, void *data, mln_lang_exp_t *exp, mln_lang_exp_t *closure);
extern int mln_lang_set_member_add(mln_alloc_t *pool, mln_rbtree_t *members, mln_lang_var_t *var);
extern mln_lang_set_detail_t *mln_lang_set_detail_new(mln_alloc_t *pool, mln_string_t *name);
extern void mln_lang_set_detail_free(mln_lang_set_detail_t *c);

/* ====================================================================
 * Chunk allocation / free.
 * ==================================================================== */

/* Truthy env-var check shared with mln_lang.c (single definition there). */
extern int mln_lang_vm_env_is_active(const char *var_name);

static mln_lang_vm_chunk_t *
mln_lang_vm_chunk_new(mln_alloc_t *pool)
{
    mln_lang_vm_chunk_t *chunk = (mln_lang_vm_chunk_t *)mln_alloc_m(pool, sizeof(*chunk));
    if (chunk == NULL) return NULL;
    memset(chunk, 0, sizeof(*chunk));
    chunk->pool = pool;
    return chunk;
}

void mln_lang_vm_chunk_free(mln_lang_vm_chunk_t *chunk)
{
    if (chunk == NULL) return;
    if (chunk->code != NULL)    mln_alloc_free(chunk->code);
    if (chunk->iconsts != NULL) mln_alloc_free(chunk->iconsts);
    if (chunk->rconsts != NULL) mln_alloc_free(chunk->rconsts);
    /* sconsts entries are borrowed pointers into the AST cache (which
     * outlives this chunk because the chunk lives on a func_detail that
     * pins the AST). Free only the array spine. */
    if (chunk->sconsts != NULL)   mln_alloc_free(chunk->sconsts);
    if (chunk->funcdefs != NULL)  mln_alloc_free(chunk->funcdefs);
    if (chunk->setdefs != NULL)   mln_alloc_free(chunk->setdefs);
    if (chunk->elemlists != NULL) mln_alloc_free(chunk->elemlists);
    if (chunk->local_names != NULL) mln_alloc_free(chunk->local_names);
    if (chunk->ic_slots != NULL)    mln_alloc_free(chunk->ic_slots);
    mln_alloc_free(chunk);
}

static int chunk_grow_funcdefs(mln_lang_vm_chunk_t *chunk)
{
    mln_size_t new_cap = chunk->funcdefs_cap == 0 ? 4 : chunk->funcdefs_cap * 2;
    void **nbuf = (void **)mln_alloc_m((mln_alloc_t *)chunk->pool, new_cap * sizeof(void *));
    if (nbuf == NULL) return -1;
    if (chunk->funcdefs != NULL) {
        memcpy(nbuf, chunk->funcdefs, chunk->funcdefs_len * sizeof(void *));
        mln_alloc_free(chunk->funcdefs);
    }
    chunk->funcdefs = nbuf;
    chunk->funcdefs_cap = new_cap;
    return 0;
}

static int chunk_grow_setdefs(mln_lang_vm_chunk_t *chunk)
{
    mln_size_t new_cap = chunk->setdefs_cap == 0 ? 4 : chunk->setdefs_cap * 2;
    void **nbuf = (void **)mln_alloc_m((mln_alloc_t *)chunk->pool, new_cap * sizeof(void *));
    if (nbuf == NULL) return -1;
    if (chunk->setdefs != NULL) {
        memcpy(nbuf, chunk->setdefs, chunk->setdefs_len * sizeof(void *));
        mln_alloc_free(chunk->setdefs);
    }
    chunk->setdefs = nbuf;
    chunk->setdefs_cap = new_cap;
    return 0;
}

/* chunk_grow_elemlists reserved for Phase G when MAKE_ARRAY is wired in. */

static int chunk_grow_rconsts(mln_lang_vm_chunk_t *chunk)
{
    mln_size_t new_cap = chunk->rconsts_cap == 0 ? 4 : chunk->rconsts_cap * 2;
    double *nbuf = (double *)mln_alloc_m((mln_alloc_t *)chunk->pool,
                                          new_cap * sizeof(double));
    if (nbuf == NULL) return -1;
    if (chunk->rconsts != NULL) {
        memcpy(nbuf, chunk->rconsts, chunk->rconsts_len * sizeof(double));
        mln_alloc_free(chunk->rconsts);
    }
    chunk->rconsts = nbuf;
    chunk->rconsts_cap = new_cap;
    return 0;
}

static int chunk_grow_sconsts(mln_lang_vm_chunk_t *chunk)
{
    mln_size_t new_cap = chunk->sconsts_cap == 0 ? 8 : chunk->sconsts_cap * 2;
    mln_string_t **nbuf = (mln_string_t **)mln_alloc_m((mln_alloc_t *)chunk->pool,
                                                       new_cap * sizeof(mln_string_t *));
    if (nbuf == NULL) return -1;
    if (chunk->sconsts != NULL) {
        memcpy(nbuf, chunk->sconsts, chunk->sconsts_len * sizeof(mln_string_t *));
        mln_alloc_free(chunk->sconsts);
    }
    chunk->sconsts = nbuf;
    chunk->sconsts_cap = new_cap;
    return 0;
}

static int chunk_grow_code(mln_lang_vm_chunk_t *chunk)
{
    mln_size_t new_cap = chunk->code_cap == 0 ? 32 : chunk->code_cap * 2;
    mln_lang_vm_insn_t *nbuf = (mln_lang_vm_insn_t *)mln_alloc_m((mln_alloc_t *)chunk->pool,
                                                                  new_cap * sizeof(mln_lang_vm_insn_t));
    if (nbuf == NULL) return -1;
    if (chunk->code != NULL) {
        memcpy(nbuf, chunk->code, chunk->code_len * sizeof(mln_lang_vm_insn_t));
        mln_alloc_free(chunk->code);
    }
    chunk->code = nbuf;
    chunk->code_cap = new_cap;
    return 0;
}

static int chunk_grow_iconsts(mln_lang_vm_chunk_t *chunk)
{
    mln_size_t new_cap = chunk->iconsts_cap == 0 ? 8 : chunk->iconsts_cap * 2;
    mln_s64_t *nbuf = (mln_s64_t *)mln_alloc_m((mln_alloc_t *)chunk->pool,
                                                new_cap * sizeof(mln_s64_t));
    if (nbuf == NULL) return -1;
    if (chunk->iconsts != NULL) {
        memcpy(nbuf, chunk->iconsts, chunk->iconsts_len * sizeof(mln_s64_t));
        mln_alloc_free(chunk->iconsts);
    }
    chunk->iconsts = nbuf;
    chunk->iconsts_cap = new_cap;
    return 0;
}

/* ====================================================================
 * Compiler state.
 * ==================================================================== */

#define MLN_VM_MAX_LOCALS    255
#define MLN_VM_MAX_LOOPS     16

#define LOOP_INLINE_CAP 16
typedef struct {
    /* For each enclosing loop (Phase C): the pc of the start of the loop
     * body's "continue" landing pad (for `continue`, jump here; for `for`
     * loops this is the mod_exp position), and a list of `break` jump
     * patches we need to fix up to point at the post-loop instruction.
     * breaks/continues point to the inline buffers until they overflow,
     * at which point a larger pool buffer is allocated and used instead. */
    int continue_pc;
    int  breaks_buf[LOOP_INLINE_CAP];
    int *breaks;
    int  breaks_cap;
    int  n_breaks;
    /* For `for` loops: continue_pc is not yet known when compiling the body. */
    int  continues_buf[LOOP_INLINE_CAP];
    int *continues;
    int  continues_cap;
    int  n_continues;
} loop_ctx_t;

#define LABELS_INLINE_CAP 32
#define GOTOS_INLINE_CAP  32
typedef struct { mln_string_t *name; int pc;       } vm_label_entry_t;
typedef struct { mln_string_t *name; int patch_pc; } vm_goto_entry_t;

typedef struct {
    mln_lang_ctx_t              *ctx;
    mln_lang_func_detail_t      *prototype;
    mln_lang_vm_chunk_t         *chunk;

    /* Local variable table. Slots 0..n_args-1 are the formal parameters,
     * named via prototype->args. Slots n_args..n_locals-1 are introduced
     * by assignments inside the body. local_names points into the AST
     * (mln_lang_factor_t::data.s_id), which lives at least as long as
     * the prototype, so no string duplication is needed. */
    mln_string_t                *local_names[MLN_VM_MAX_LOCALS];
    mln_size_t                   n_args;
    mln_size_t                   n_locals;

    int                          ok;
    int                          sp;
    int                          max_sp;

    /* Loop stack for break/continue. */
    loop_ctx_t                   loops[MLN_VM_MAX_LOOPS];
    int                          n_loops;

    /* Phase E: scratch state for compile_locate to know when the previous
     * hop in the same chain was a PROPERTY-followed-by-FUNC (in which case
     * we DUP'd the obj and the next FUNC must emit CALL_METHOD). */
    int                          prev_was_property;

    /* Medium-tier perf: when emitting a PROPERTY-then-FUNC pair we now defer
     * the GET_PROPERTY and emit a single fused INVOKE_METHOD opcode at the
     * FUNC. prev_property_name_idx stashes the sconsts index of the method
     * name so the FUNC hop can emit it. -1 means "not in deferred state". */
    int                          prev_property_name_idx;

    /* Phase F4: labels and goto patches for goto/M_STM_LABEL.
     * Inline buffers are used first; pool-allocated buffers are used when
     * the inline capacity is exceeded. */
    vm_label_entry_t  labels_buf[LABELS_INLINE_CAP];
    vm_label_entry_t *labels;
    int               labels_cap;
    int               n_labels;
    vm_goto_entry_t   goto_patches_buf[GOTOS_INLINE_CAP];
    vm_goto_entry_t  *goto_patches;
    int               goto_patches_cap;
    int               n_goto_patches;

    /* Compile-time constant folding gate. Set by mln_lang_vm_try_compile
     * after scanning the AST for any `__int_*_operator__` user-defined
     * overload. When 1, try_fold_int_binop folds LOAD_INT/LOAD_INT/<op>
     * triples into a single LOAD_INT. When 0, all int binops keep their
     * runtime overload-dispatch path intact. */
    int               safe_to_fold;
} mln_lang_vm_compiler_t;

static int emit(mln_lang_vm_compiler_t *c, mln_u8_t op, mln_u8_t a, mln_s16_t b)
{
    if (!c->ok) return -1;
    if (c->chunk->code_len >= c->chunk->code_cap) {
        if (chunk_grow_code(c->chunk) < 0) { c->ok = 0; return -1; }
    }
    mln_lang_vm_insn_t *insn = &c->chunk->code[c->chunk->code_len++];
    insn->op = op;
    insn->a  = a;
    insn->b  = b;
    return (int)(c->chunk->code_len - 1);
}

static void sp_push(mln_lang_vm_compiler_t *c, int n)
{
    c->sp += n;
    if (c->sp > c->max_sp) c->max_sp = c->sp;
}

static void sp_pop(mln_lang_vm_compiler_t *c, int n) { c->sp -= n; }

static int add_iconst(mln_lang_vm_compiler_t *c, mln_s64_t v)
{
    for (mln_size_t i = 0; i < c->chunk->iconsts_len; ++i) {
        if (c->chunk->iconsts[i] == v) return (int)i;
    }
    if (c->chunk->iconsts_len >= c->chunk->iconsts_cap) {
        if (chunk_grow_iconsts(c->chunk) < 0) { c->ok = 0; return -1; }
    }
    c->chunk->iconsts[c->chunk->iconsts_len] = v;
    return (int)c->chunk->iconsts_len++;
}

static int add_rconst(mln_lang_vm_compiler_t *c, double v)
{
    for (mln_size_t i = 0; i < c->chunk->rconsts_len; ++i) {
        if (c->chunk->rconsts[i] == v) return (int)i;
    }
    if (c->chunk->rconsts_len >= c->chunk->rconsts_cap) {
        if (chunk_grow_rconsts(c->chunk) < 0) { c->ok = 0; return -1; }
    }
    c->chunk->rconsts[c->chunk->rconsts_len] = v;
    return (int)c->chunk->rconsts_len++;
}

static int add_sconst(mln_lang_vm_compiler_t *c, mln_string_t *s)
{
    if (s == NULL) return -1;
    for (mln_size_t i = 0; i < c->chunk->sconsts_len; ++i) {
        mln_string_t *e = c->chunk->sconsts[i];
        if (e->len == s->len && !memcmp(e->data, s->data, s->len)) return (int)i;
    }
    if (c->chunk->sconsts_len >= c->chunk->sconsts_cap) {
        if (chunk_grow_sconsts(c->chunk) < 0) { c->ok = 0; return -1; }
    }
    c->chunk->sconsts[c->chunk->sconsts_len] = s;
    return (int)c->chunk->sconsts_len++;
}

static int add_funcdef(mln_lang_vm_compiler_t *c, mln_lang_funcdef_t *fd)
{
    if (fd == NULL) return -1;
    if (c->chunk->funcdefs_len >= c->chunk->funcdefs_cap) {
        if (chunk_grow_funcdefs(c->chunk) < 0) { c->ok = 0; return -1; }
    }
    c->chunk->funcdefs[c->chunk->funcdefs_len] = fd;
    return (int)c->chunk->funcdefs_len++;
}

static int add_setdef(mln_lang_vm_compiler_t *c, mln_lang_set_t *sd)
{
    if (sd == NULL) return -1;
    if (c->chunk->setdefs_len >= c->chunk->setdefs_cap) {
        if (chunk_grow_setdefs(c->chunk) < 0) { c->ok = 0; return -1; }
    }
    c->chunk->setdefs[c->chunk->setdefs_len] = sd;
    return (int)c->chunk->setdefs_len++;
}

/* add_elemlist + chunk_grow_elemlists wired in when MAKE_ARRAY is emitted
 * by the compiler (Phase G). Storage is reserved on the chunk for future use. */

static int find_local_slot(mln_lang_vm_compiler_t *c, mln_string_t *name)
{
    if (name == NULL) return -1;
    for (mln_size_t i = 0; i < c->n_locals; ++i) {
        if (c->local_names[i] == NULL) continue;
        if (c->local_names[i]->len == name->len &&
            !memcmp(c->local_names[i]->data, name->data, name->len))
        {
            return (int)i;
        }
    }
    return -1;
}

static void bail(mln_lang_vm_compiler_t *c) { c->ok = 0; }

static void loop_ctx_init(loop_ctx_t *lc) {
    lc->continue_pc = -1;
    lc->breaks = lc->breaks_buf;
    lc->breaks_cap = LOOP_INLINE_CAP;
    lc->n_breaks = 0;
    lc->continues = lc->continues_buf;
    lc->continues_cap = LOOP_INLINE_CAP;
    lc->n_continues = 0;
}

/* Append a break-patch index to lc->breaks, growing the array from the
 * pool if the inline capacity is exhausted. Returns 0 on success. */
static int lc_push_break(mln_lang_vm_compiler_t *c, loop_ctx_t *lc, int pc) {
    if (lc->n_breaks >= lc->breaks_cap) {
        int new_cap = lc->breaks_cap * 2;
        int *nb = (int *)mln_alloc_m(c->ctx->pool, sizeof(int) * new_cap);
        if (nb == NULL) { bail(c); return -1; }
        memcpy(nb, lc->breaks, sizeof(int) * lc->n_breaks);
        lc->breaks = nb;
        lc->breaks_cap = new_cap;
    }
    lc->breaks[lc->n_breaks++] = pc;
    return 0;
}

static int lc_push_continue(mln_lang_vm_compiler_t *c, loop_ctx_t *lc, int pc) {
    if (lc->n_continues >= lc->continues_cap) {
        int new_cap = lc->continues_cap * 2;
        int *nc = (int *)mln_alloc_m(c->ctx->pool, sizeof(int) * new_cap);
        if (nc == NULL) { bail(c); return -1; }
        memcpy(nc, lc->continues, sizeof(int) * lc->n_continues);
        lc->continues = nc;
        lc->continues_cap = new_cap;
    }
    lc->continues[lc->n_continues++] = pc;
    return 0;
}

static int compiler_push_label(mln_lang_vm_compiler_t *c, mln_string_t *name, int pc) {
    if (c->n_labels >= c->labels_cap) {
        int new_cap = c->labels_cap * 2;
        vm_label_entry_t *nl = (vm_label_entry_t *)mln_alloc_m(c->ctx->pool,
                                    sizeof(vm_label_entry_t) * new_cap);
        if (nl == NULL) { bail(c); return -1; }
        memcpy(nl, c->labels, sizeof(vm_label_entry_t) * c->n_labels);
        c->labels = nl;
        c->labels_cap = new_cap;
    }
    c->labels[c->n_labels].name = name;
    c->labels[c->n_labels].pc = pc;
    c->n_labels++;
    return 0;
}

static int compiler_push_goto(mln_lang_vm_compiler_t *c, mln_string_t *name, int patch_pc) {
    if (c->n_goto_patches >= c->goto_patches_cap) {
        int new_cap = c->goto_patches_cap * 2;
        vm_goto_entry_t *ng = (vm_goto_entry_t *)mln_alloc_m(c->ctx->pool,
                                  sizeof(vm_goto_entry_t) * new_cap);
        if (ng == NULL) { bail(c); return -1; }
        memcpy(ng, c->goto_patches, sizeof(vm_goto_entry_t) * c->n_goto_patches);
        c->goto_patches = ng;
        c->goto_patches_cap = new_cap;
    }
    c->goto_patches[c->n_goto_patches].name = name;
    c->goto_patches[c->n_goto_patches].patch_pc = patch_pc;
    c->n_goto_patches++;
    return 0;
}

/* Allocate an anonymous scratch slot (no name → find_local_slot never
 * returns it) for use as a temporary save register.  Returns the slot
 * index or -1 if the limit is reached (bail() is called for the caller). */
static int alloc_temp_slot(mln_lang_vm_compiler_t *c)
{
    if (c->n_locals >= MLN_VM_MAX_LOCALS) { bail(c); return -1; }
    int slot = (int)c->n_locals;
    c->local_names[slot] = NULL;   /* anonymous */
    c->n_locals++;
    return slot;
}

/* Emit LOAD_INT 1 (adds the constant to iconsts if not already present).
 * Used by ++/-- code generation for non-local lvalues. */
static void emit_load_int_one(mln_lang_vm_compiler_t *c)
{
    int idx = add_iconst(c, 1);
    if (idx < 0 || idx > 32767) { bail(c); return; }
    emit(c, MLN_VOP_LOAD_INT, 0, (mln_s16_t)idx);
    sp_push(c, 1);
}

/* Compile-time constant folding for integer binops.
 *
 * If the last two emitted instructions are both LOAD_INT and `op` is one
 * of the foldable arithmetic / bitwise / shift opcodes, rewind both
 * LOAD_INTs, evaluate the operation at compile time, and emit a single
 * LOAD_INT of the result. Returns 1 on fold (caller skips the binop
 * emit + sp_pop), 0 otherwise.
 *
 * SAFETY: gated on `safe_to_fold == 1`, set by the caller when it has
 * statically verified that the script's AST contains no
 * `__int_*_operator__` user-defined overloads. Melang resolves operator
 * overloads at RUNTIME by checking ctx->op_int_flag inside apply_binop,
 * so a fold of `3 + 4` to `7` would silently skip an overload defined
 * later in the same script. The static AST scan (see
 * scan_for_int_overloads) detects definitions like
 * `@__int_plus_operator__(a,b){...}` anywhere in the script and
 * disables folding for that compile.
 *
 * Note: a script that introduces overloads via Eval() of another
 * script in the same ctx after compile is NOT detected. To preserve
 * full correctness in that (rare) case, callers can additionally
 * check ctx->op_int_flag at compile time and refuse folding when it
 * is already set. Today no caller relies on Eval-injected overloads
 * for arithmetic literal folding, so the AST-scan check is sufficient
 * for the test suite.
 *
 * Folding rules:
 *   - +, -, * use unsigned wrap-around to match the VM's runtime
 *     semantics (mln_lang_var_create_int with the same operands would
 *     also wrap), avoiding any UB from signed overflow at compile
 *     time.
 *   - / and % skip folding when the divisor is 0 so the runtime path
 *     can raise the same error it always would.
 *   - << and >> skip folding when the shift amount is < 0 or >= 64 so
 *     we don't introduce UB and so the runtime continues to handle
 *     out-of-range shifts identically.
 *   - Comparisons (<, <=, ==, ...) are not folded here because they
 *     would need to produce a bool, which uses different opcodes.
 *
 * The peephole only rewinds the immediate two LOAD_INTs. Recursive
 * AST emit naturally cascades folds: 1+2+3 folds 1+2 → 3 first, then
 * the parent sees LOAD_INT 3, LOAD_INT 3 and folds again to 6. The
 * unused iconsts left behind in the pool are harmless. */
static int try_fold_int_binop(mln_lang_vm_compiler_t *c, mln_u8_t op)
{
    if (!c->ok) return 0;
    if (!c->safe_to_fold) return 0;
    if (c->chunk->code_len < 2) return 0;
    mln_lang_vm_insn_t *i1 = &c->chunk->code[c->chunk->code_len - 2];
    mln_lang_vm_insn_t *i2 = &c->chunk->code[c->chunk->code_len - 1];
    if (i1->op != MLN_VOP_LOAD_INT || i2->op != MLN_VOP_LOAD_INT) return 0;
    if ((mln_size_t)i1->b >= c->chunk->iconsts_len) return 0;
    if ((mln_size_t)i2->b >= c->chunk->iconsts_len) return 0;
    mln_s64_t a = c->chunk->iconsts[i1->b];
    mln_s64_t b = c->chunk->iconsts[i2->b];
    mln_s64_t r;
    switch (op) {
        case MLN_VOP_ADD:
            r = (mln_s64_t)((mln_u64_t)a + (mln_u64_t)b);
            break;
        case MLN_VOP_SUB:
            r = (mln_s64_t)((mln_u64_t)a - (mln_u64_t)b);
            break;
        case MLN_VOP_MUL:
            r = (mln_s64_t)((mln_u64_t)a * (mln_u64_t)b);
            break;
        case MLN_VOP_DIV:
            if (b == 0) return 0;
            /* Avoid INT64_MIN / -1 UB: leave for runtime. */
            if (a == (mln_s64_t)0x8000000000000000LL && b == -1) return 0;
            r = a / b;
            break;
        case MLN_VOP_MOD:
            if (b == 0) return 0;
            if (a == (mln_s64_t)0x8000000000000000LL && b == -1) return 0;
            r = a % b;
            break;
        case MLN_VOP_BOR:    r = a | b; break;
        case MLN_VOP_BAND:   r = a & b; break;
        case MLN_VOP_BXOR:   r = a ^ b; break;
        case MLN_VOP_LSHIFT:
            if (b < 0 || b >= 64) return 0;
            r = (mln_s64_t)((mln_u64_t)a << b);
            break;
        case MLN_VOP_RSHIFT:
            if (b < 0 || b >= 64) return 0;
            r = a >> b;
            break;
        default:
            return 0;
    }
    /* Drop the two LOAD_INTs and their pushed slots, then re-emit one. */
    c->chunk->code_len -= 2;
    sp_pop(c, 2);
    int idx = add_iconst(c, r);
    if (idx < 0 || idx > 32767) { bail(c); return 0; }
    emit(c, MLN_VOP_LOAD_INT, 0, (mln_s16_t)idx);
    sp_push(c, 1);
    return 1;
}

/* Patch the b-field of an already-emitted JUMP instruction at patch_pc so
 * that execution transfers to target_pc.  Calls bail() if the signed 16-bit
 * offset would overflow, preventing silent wrap-around in large functions. */
static void patch_jump(mln_lang_vm_compiler_t *c, int patch_pc, int target_pc)
{
    int offset = target_pc - (patch_pc + 1);
    if (offset < -32768 || offset > 32767) { bail(c); return; }
    c->chunk->code[patch_pc].b = (mln_s16_t)offset;
}

/* ====================================================================
 * Medium-tier perf: peephole / superinstruction fusion.
 *
 * Runs after the main compile pass. Walks the emitted bytecode and
 * fuses a small set of common adjacent-opcode patterns into single
 * superinstructions:
 *   LOAD_LOCAL,LOAD_LOCAL,<binop>  ->  <binop>_LL
 *   LOAD_LOCAL,LOAD_INT,<binop>    ->  <binop>_LI
 *   LOAD_LOCAL,RETURN              ->  RETURN_LOCAL
 * The fused pair/triple replaces 2-3 dispatches with 1, saving the
 * computed-goto / switch overhead per fusion site.
 *
 * Jump-target safety: a pre-pass scans every JUMP/JUMP_IF_FALSE/JUMP_IF_TRUE
 * to mark each old-pc that any instruction can land on. We refuse to fuse
 * a sequence that would absorb a jump target into the middle of a
 * superinstruction (the sequence's 2nd or 3rd insn).
 *
 * After fusion we rewrite all jump offsets through old_pc->new_pc tables
 * so the new code's control flow is identical to the old.
 * ==================================================================== */
static int peephole_is_jumplike(mln_u8_t op)
{
    return op == MLN_VOP_JUMP ||
           op == MLN_VOP_JUMP_IF_FALSE ||
           op == MLN_VOP_JUMP_IF_TRUE;
}

static int peephole_binop_to_ll(mln_u8_t op)
{
    switch (op) {
        case MLN_VOP_ADD: return MLN_VOP_ADD_LL;
        case MLN_VOP_SUB: return MLN_VOP_SUB_LL;
        case MLN_VOP_MUL: return MLN_VOP_MUL_LL;
        case MLN_VOP_LT:  return MLN_VOP_LT_LL;
        case MLN_VOP_LE:  return MLN_VOP_LE_LL;
        case MLN_VOP_GT:  return MLN_VOP_GT_LL;
        case MLN_VOP_GE:  return MLN_VOP_GE_LL;
        case MLN_VOP_EQ:  return MLN_VOP_EQ_LL;
        case MLN_VOP_NE:  return MLN_VOP_NE_LL;
        default: return -1;
    }
}

static int peephole_binop_to_li(mln_u8_t op)
{
    switch (op) {
        case MLN_VOP_ADD: return MLN_VOP_ADD_LI;
        case MLN_VOP_SUB: return MLN_VOP_SUB_LI;
        case MLN_VOP_MUL: return MLN_VOP_MUL_LI;
        case MLN_VOP_LT:  return MLN_VOP_LT_LI;
        case MLN_VOP_LE:  return MLN_VOP_LE_LI;
        case MLN_VOP_GT:  return MLN_VOP_GT_LI;
        case MLN_VOP_GE:  return MLN_VOP_GE_LI;
        case MLN_VOP_EQ:  return MLN_VOP_EQ_LI;
        case MLN_VOP_NE:  return MLN_VOP_NE_LI;
        default: return -1;
    }
}

static void run_peephole(mln_lang_vm_compiler_t *c)
{
    if (!c->ok) return;
    mln_lang_vm_chunk_t *chunk = c->chunk;
    if (chunk == NULL || chunk->code == NULL || chunk->code_len < 2) return;

    mln_size_t old_len = chunk->code_len;
    mln_lang_vm_insn_t *old_code = chunk->code;
    mln_alloc_t *pool = (mln_alloc_t *)chunk->pool;

    /* Step 1: mark all jump targets in the old code. */
    char *is_target = (char *)mln_alloc_m(pool, old_len);
    if (is_target == NULL) { bail(c); return; }
    memset(is_target, 0, old_len);
    for (mln_size_t i = 0; i < old_len; ++i) {
        if (peephole_is_jumplike(old_code[i].op)) {
            int t = (int)i + 1 + (int)old_code[i].b;
            if (t >= 0 && t < (int)old_len) is_target[t] = 1;
        }
    }

    /* Step 2: emit fused code into a fresh buffer. */
    mln_lang_vm_insn_t *new_code = (mln_lang_vm_insn_t *)mln_alloc_m(pool,
                                       sizeof(*new_code) * old_len);
    int *pc_map = (int *)mln_alloc_m(pool, sizeof(int) * (old_len + 1));
    if (new_code == NULL || pc_map == NULL) {
        if (is_target) mln_alloc_free(is_target);
        if (new_code) mln_alloc_free(new_code);
        if (pc_map)   mln_alloc_free(pc_map);
        bail(c); return;
    }

    mln_size_t ni = 0;
    for (mln_size_t i = 0; i < old_len; ) {
        pc_map[i] = (int)ni;

        /* Pattern: LOAD_LOCAL, LOAD_LOCAL, <binop>  ->  <binop>_LL */
        if (i + 2 < old_len &&
            old_code[i].op   == MLN_VOP_LOAD_LOCAL &&
            old_code[i+1].op == MLN_VOP_LOAD_LOCAL &&
            !is_target[i+1] && !is_target[i+2])
        {
            int fop = peephole_binop_to_ll(old_code[i+2].op);
            /* Slot indices are mln_u8_t so they always fit in a u8 field
             * — no need to range-check (clang flags it as tautological). */
            if (fop >= 0) {
                new_code[ni].op = (mln_u8_t)fop;
                new_code[ni].a  = old_code[i].a;
                new_code[ni].b  = (mln_s16_t)(old_code[i+1].a & 0xff);
                pc_map[i+1] = (int)ni;
                pc_map[i+2] = (int)ni;
                ++ni;
                i += 3;
                continue;
            }
        }

        /* Pattern: LOAD_LOCAL, LOAD_INT, <binop>  ->  <binop>_LI */
        if (i + 2 < old_len &&
            old_code[i].op   == MLN_VOP_LOAD_LOCAL &&
            old_code[i+1].op == MLN_VOP_LOAD_INT &&
            !is_target[i+1] && !is_target[i+2])
        {
            int fop = peephole_binop_to_li(old_code[i+2].op);
            if (fop >= 0) {
                new_code[ni].op = (mln_u8_t)fop;
                new_code[ni].a  = old_code[i].a;
                new_code[ni].b  = old_code[i+1].b;  /* iconst index */
                pc_map[i+1] = (int)ni;
                pc_map[i+2] = (int)ni;
                ++ni;
                i += 3;
                continue;
            }
        }

        /* Pattern: LOAD_LOCAL, RETURN  ->  RETURN_LOCAL */
        if (i + 1 < old_len &&
            old_code[i].op   == MLN_VOP_LOAD_LOCAL &&
            old_code[i+1].op == MLN_VOP_RETURN &&
            !is_target[i+1])
        {
            new_code[ni].op = MLN_VOP_RETURN_LOCAL;
            new_code[ni].a  = old_code[i].a;
            new_code[ni].b  = 0;
            pc_map[i+1] = (int)ni;
            ++ni;
            i += 2;
            continue;
        }

        /* No fusion: copy. */
        new_code[ni++] = old_code[i];
        ++i;
    }
    pc_map[old_len] = (int)ni;  /* sentinel for jumps to "end" */

    /* Step 3: rewrite jump offsets using old->new pc map.
     * Walk the NEW code and reverse-look the corresponding OLD pc, since
     * each new insn was emitted from a unique old start pc (recorded
     * implicitly by the order of pc_map[i] = ni assignments). */
    /* Build inverse map: new_pc -> old_pc (only first old_pc per fusion). */
    int *inv_map = (int *)mln_alloc_m(pool, sizeof(int) * (ni + 1));
    if (inv_map == NULL) {
        mln_alloc_free(is_target); mln_alloc_free(new_code); mln_alloc_free(pc_map);
        bail(c); return;
    }
    /* Initialize so any unset slot has a sane value. */
    for (mln_size_t k = 0; k <= ni; ++k) inv_map[k] = -1;
    for (mln_size_t i = 0; i < old_len; ++i) {
        if (pc_map[i] >= 0 && pc_map[i] < (int)ni && inv_map[pc_map[i]] == -1) {
            inv_map[pc_map[i]] = (int)i;
        }
    }
    inv_map[ni] = (int)old_len;

    for (mln_size_t k = 0; k < ni; ++k) {
        if (peephole_is_jumplike(new_code[k].op)) {
            int old_pc = inv_map[k];
            if (old_pc < 0) { bail(c); break; }
            int old_target = old_pc + 1 + (int)new_code[k].b;
            if (old_target < 0 || old_target > (int)old_len) { bail(c); break; }
            int new_target = (old_target == (int)old_len) ? (int)ni : pc_map[old_target];
            int new_offset = new_target - (int)k - 1;
            if (new_offset < -32768 || new_offset > 32767) { bail(c); break; }
            new_code[k].b = (mln_s16_t)new_offset;
        }
    }

    if (!c->ok) {
        mln_alloc_free(is_target); mln_alloc_free(new_code);
        mln_alloc_free(pc_map);    mln_alloc_free(inv_map);
        return;
    }

    /* Step 4: replace chunk->code with the fused buffer. */
    mln_alloc_free(chunk->code);
    chunk->code = new_code;
    chunk->code_len = ni;
    chunk->code_cap = old_len;  /* new_code was allocated with old_len capacity */

    mln_alloc_free(is_target);
    mln_alloc_free(pc_map);
    mln_alloc_free(inv_map);
}

/* Forward declarations. */
static void compile_stm_chain(mln_lang_vm_compiler_t *c, mln_lang_stm_t *stm);
static void compile_stm     (mln_lang_vm_compiler_t *c, mln_lang_stm_t *stm);
static void compile_block   (mln_lang_vm_compiler_t *c, mln_lang_block_t *block);
static void compile_exp     (mln_lang_vm_compiler_t *c, mln_lang_exp_t *exp);
static void compile_assign  (mln_lang_vm_compiler_t *c, mln_lang_assign_t *a);
static void compile_logiclow(mln_lang_vm_compiler_t *c, mln_lang_logiclow_t *n);
static void compile_logichigh(mln_lang_vm_compiler_t *c, mln_lang_logichigh_t *n);
static void compile_relativelow(mln_lang_vm_compiler_t *c, mln_lang_relativelow_t *n);
static void compile_relativehigh(mln_lang_vm_compiler_t *c, mln_lang_relativehigh_t *n);
static void compile_move    (mln_lang_vm_compiler_t *c, mln_lang_move_t *n);
static void compile_addsub  (mln_lang_vm_compiler_t *c, mln_lang_addsub_t *n);
static void compile_muldiv  (mln_lang_vm_compiler_t *c, mln_lang_muldiv_t *n);
static void compile_not     (mln_lang_vm_compiler_t *c, mln_lang_not_t *n);
static void compile_suffix  (mln_lang_vm_compiler_t *c, mln_lang_suffix_t *n);
static void compile_locate  (mln_lang_vm_compiler_t *c, mln_lang_locate_t *n);
static void compile_spec    (mln_lang_vm_compiler_t *c, mln_lang_spec_t *n);
static void compile_factor  (mln_lang_vm_compiler_t *c, mln_lang_factor_t *f);

/* Phase E: walk down the binary-op nesting to reach the locate node. All
 * intermediate ops must be NONE (otherwise it's an expression, not an
 * lvalue or simple-rvalue chain). Returns the locate or NULL. */
static mln_lang_locate_t *unwrap_to_locate(mln_lang_logiclow_t *lhs)
{
    if (lhs == NULL || lhs->op != M_LOGICLOW_NONE) return NULL;
    if (lhs->left == NULL || lhs->left->op != M_LOGICHIGH_NONE) return NULL;
    mln_lang_relativelow_t *rl = lhs->left->left;
    if (rl == NULL || rl->op != M_RELATIVELOW_NONE) return NULL;
    mln_lang_relativehigh_t *rh = rl->left;
    if (rh == NULL || rh->op != M_RELATIVEHIGH_NONE) return NULL;
    mln_lang_move_t *mv = rh->left;
    if (mv == NULL || mv->op != M_MOVE_NONE) return NULL;
    mln_lang_addsub_t *ads = mv->left;
    if (ads == NULL || ads->op != M_ADDSUB_NONE) return NULL;
    mln_lang_muldiv_t *md = ads->left;
    if (md == NULL || md->op != M_MULDIV_NONE) return NULL;
    mln_lang_not_t *nt = md->left;
    if (nt == NULL || nt->op != M_NOT_NONE) return NULL;
    mln_lang_suffix_t *sf = nt->right.suffix;
    if (sf == NULL || sf->op != M_SUFFIX_NONE) return NULL;
    return sf->left;
}

/* Returns the local slot index when the LHS is a plain identifier that is
 * already in local_names (arg or closure capture).  Returns -1 when the
 * name is not a known local — the caller must then emit ASSIGN_GLOBAL so
 * the runtime scope-chain search can find (or create) the variable.
 * Returns -2 for non-identifier LHS forms (locate chains) so the caller
 * can fall through to the property/index path. */
static int extract_lhs_local(mln_lang_vm_compiler_t *c, mln_lang_logiclow_t *lhs)
{
    mln_lang_locate_t *lc = unwrap_to_locate(lhs);
    if (lc == NULL || lc->op != M_LOCATE_NONE || lc->next != NULL) return -2;
    mln_lang_spec_t *sp = lc->left;
    if (sp == NULL || sp->op != M_SPEC_FACTOR) return -2;
    mln_lang_factor_t *fac = sp->data.factor;
    if (fac == NULL || fac->type != M_FACTOR_ID) return -2;
    /* Only return the slot if this name is already a known local (arg or
     * closure capture).  New names go through ASSIGN_GLOBAL at runtime so
     * that existing outer/global variables are updated rather than shadowed
     * by a newly allocated frame slot. */
    return find_local_slot(c, fac->data.s_id);
}

/* ====================================================================
 * Compiler — AST walkers.
 * ==================================================================== */

static void compile_stm_chain(mln_lang_vm_compiler_t *c, mln_lang_stm_t *stm)
{
    while (c->ok && stm != NULL) {
        compile_stm(c, stm);
        stm = stm->next;
    }
}

static void compile_stm(mln_lang_vm_compiler_t *c, mln_lang_stm_t *stm)
{
    switch (stm->type) {
        case M_STM_BLOCK:
            compile_block(c, stm->data.block);
            return;
        case M_STM_FUNC: {
            /* Phase F: top-level / set-method funcdef. Emit BIND_FUNC with
             * an index into chunk->funcdefs[]; runtime creates the
             * func_detail (incl. closure capture) and binds it. */
            int idx = add_funcdef(c, stm->data.func);
            if (idx < 0 || idx > 32767) { bail(c); return; }
            emit(c, MLN_VOP_BIND_FUNC, 0, (mln_s16_t)idx);
            return;
        }
        case M_STM_SET: {
            int idx = add_setdef(c, stm->data.setdef);
            if (idx < 0 || idx > 32767) { bail(c); return; }
            emit(c, MLN_VOP_BIND_SET, 0, (mln_s16_t)idx);
            return;
        }
        case M_STM_LABEL: {
            /* Record (name, current pc) in labels table. Goto-patches that
             * reference this label can resolve to it (immediately for
             * backward gotos, at compile end for forward gotos). */
            compiler_push_label(c, stm->data.pos, (int)c->chunk->code_len);
            return;
        }
        case M_STM_SWITCH: {
            /* Melang switch semantics (per docs/flowcontrol.md):
             *   - cond is matched against each case's factor in order
             *   - on match, body runs and FALLS THROUGH to subsequent
             *     cases' bodies (without re-comparing) until break
             *   - default matches when no other case did
             *
             * Code shape:
             *     push cond
             *     DUP; cmp factor1; JIT body1
             *     DUP; cmp factor2; JIT body2
             *     ...
             *     JUMP body_default (or JUMP end if no default)
             *   body1:
             *     [body 1 statements]
             *   body2:
             *     [body 2 statements]   (fall-through from body1)
             *     ...
             *   body_default:
             *     [default statements]
             *   end:
             *     POP cond
             *
             * `break` inside any body jumps to `end`, modeled via a
             * synthetic loop_ctx so M_BLOCK_BREAK reuses the same
             * patch list. */
            mln_lang_switch_t *sw = stm->data.sw;
            int sp_before = c->sp;
            compile_exp(c, sw->condition);
            if (!c->ok) return;
            if (c->sp != sp_before + 1) { bail(c); return; }

#define SWITCH_INLINE_CAP 64
            mln_lang_switchstm_t *cases_buf[SWITCH_INLINE_CAP];
            int compare_jumps_buf[SWITCH_INLINE_CAP];
            mln_lang_switchstm_t **cases = cases_buf;
            int *compare_jumps = compare_jumps_buf;
            int cases_cap = SWITCH_INLINE_CAP;
            int n_cases = 0;
            int default_idx = -1;
            for (mln_lang_switchstm_t *sst = sw->switchstm; sst != NULL; sst = sst->next) {
                if (n_cases >= cases_cap) {
                    int new_cap = cases_cap * 2;
                    mln_lang_switchstm_t **nc = (mln_lang_switchstm_t **)mln_alloc_m(
                        c->ctx->pool, sizeof(*nc) * new_cap);
                    int *nj = (int *)mln_alloc_m(c->ctx->pool, sizeof(int) * new_cap);
                    /* Pool allocations are freed with ctx->pool at context
                     * teardown; no explicit free is needed here. */
                    if (nc == NULL || nj == NULL) { bail(c); return; }
                    memcpy(nc, cases, sizeof(*nc) * n_cases);
                    memcpy(nj, compare_jumps, sizeof(int) * n_cases);
                    cases = nc;
                    compare_jumps = nj;
                    cases_cap = new_cap;
                }
                cases[n_cases] = sst;
                if (sst->factor != NULL) {
                    emit(c, MLN_VOP_DUP, 0, 0);
                    sp_push(c, 1);
                    compile_factor(c, sst->factor);
                    if (!c->ok) return;
                    emit(c, MLN_VOP_EQ, 0, 0);
                    sp_pop(c, 1);
                    compare_jumps[n_cases] = emit(c, MLN_VOP_JUMP_IF_TRUE, 0, 0);
                    sp_pop(c, 1);
                } else {
                    if (default_idx < 0) default_idx = n_cases;
                    compare_jumps[n_cases] = -1;
                }
                n_cases++;
            }
            /* No-match: jump to default body if any, else to end. */
            int j_no_match = emit(c, MLN_VOP_JUMP, 0, 0);

            /* Push a switch loop_ctx so M_BLOCK_BREAK patches go here. */
            if (c->n_loops >= MLN_VM_MAX_LOOPS) { bail(c); return; }
            loop_ctx_t *lc = &c->loops[c->n_loops++];
            loop_ctx_init(lc);

            /* Emit each case body in order. Patch its JIT_TRUE to here. */
            int body_pcs_buf[SWITCH_INLINE_CAP];
            int *body_pcs = (n_cases <= SWITCH_INLINE_CAP) ? body_pcs_buf :
                (int *)mln_alloc_m(c->ctx->pool, sizeof(int) * n_cases);
            if (body_pcs == NULL) { c->n_loops--; bail(c); return; }
            for (int i = 0; i < n_cases; ++i) {
                body_pcs[i] = (int)c->chunk->code_len;
                if (cases[i]->factor != NULL) {
                    patch_jump(c, compare_jumps[i], body_pcs[i]);
                    if (!c->ok) { c->n_loops--; return; }
                }
                if (cases[i]->stm != NULL) {
                    compile_stm_chain(c, cases[i]->stm);
                    if (!c->ok) { c->n_loops--; return; }
                }
                /* Fall through to next case body — no JUMP emitted. */
            }
            /* Patch j_no_match: to default body if exists, else to end. */
            int end_pc = (int)c->chunk->code_len;
            int no_match_target = (default_idx >= 0) ? body_pcs[default_idx] : end_pc;
            patch_jump(c, j_no_match, no_match_target);
            if (!c->ok) { c->n_loops--; return; }

            /* Patch break jumps from inside any case body to end_pc. */
            for (int i = 0; i < lc->n_breaks; ++i) {
                patch_jump(c, lc->breaks[i], end_pc);
                if (!c->ok) { c->n_loops--; return; }
            }
            c->n_loops--;

            /* End: POP cond. */
            emit(c, MLN_VOP_POP, 0, 0);
            sp_pop(c, 1);
            return;
        }
        case M_STM_WHILE: {
            mln_lang_while_t *w = stm->data.w;
            int loop_start = (int)c->chunk->code_len;
            /* loop_ctx: continue lands at loop_start (re-evaluate condition). */
            if (c->n_loops >= MLN_VM_MAX_LOOPS) { bail(c); return; }
            loop_ctx_t *lc = &c->loops[c->n_loops++];
            loop_ctx_init(lc);
            lc->continue_pc = loop_start;

            /* condition (or empty for `while (1)` style) */
            int sp_before = c->sp;
            if (w->condition != NULL) {
                compile_exp(c, w->condition);
                if (!c->ok) { c->n_loops--; return; }
                if (c->sp != sp_before + 1) { bail(c); c->n_loops--; return; }
                /* JIF_FALSE → exit */
                {
                    int j_exit = emit(c, MLN_VOP_JUMP_IF_FALSE, 0, 0);
                    if (lc_push_break(c, lc, j_exit) < 0) { c->n_loops--; return; }
                }
                sp_pop(c, 1);
            }

            /* body */
            if (w->blockstm != NULL) compile_block(c, w->blockstm);
            if (!c->ok) { c->n_loops--; return; }

            /* jump back to start */
            int j_back = emit(c, MLN_VOP_JUMP, 0, 0);
            patch_jump(c, j_back, loop_start);
            if (!c->ok) { c->n_loops--; return; }

            /* patch breaks */
            int after = (int)c->chunk->code_len;
            for (int i = 0; i < lc->n_breaks; ++i) {
                patch_jump(c, lc->breaks[i], after);
                if (!c->ok) { c->n_loops--; return; }
            }
            c->n_loops--;
            return;
        }
        case M_STM_FOR: {
            mln_lang_for_t *f = stm->data.f;
            /* init */
            if (f->init_exp != NULL) {
                int sp_before = c->sp;
                compile_exp(c, f->init_exp);
                if (!c->ok) return;
                /* discard init's expression value */
                while (c->sp > sp_before) {
                    emit(c, MLN_VOP_POP, 0, 0);
                    sp_pop(c, 1);
                }
            }

            int cond_pc = (int)c->chunk->code_len;
            if (c->n_loops >= MLN_VM_MAX_LOOPS) { bail(c); return; }
            loop_ctx_t *lc = &c->loops[c->n_loops++];
            loop_ctx_init(lc);

            /* condition (optional) */
            int j_exit = -1;
            if (f->condition != NULL) {
                int sp_before = c->sp;
                compile_exp(c, f->condition);
                if (!c->ok) { c->n_loops--; return; }
                if (c->sp != sp_before + 1) { bail(c); c->n_loops--; return; }
                j_exit = emit(c, MLN_VOP_JUMP_IF_FALSE, 0, 0);
                sp_pop(c, 1);
            }

            /* Compile body.  Any `continue` inside will emit a JUMP with
             * offset 0 and add its index to lc->continues[].  We patch all
             * those jumps to mod_pc once we know it (after the body). */
            if (f->blockstm != NULL) compile_block(c, f->blockstm);
            if (!c->ok) { c->n_loops--; return; }

            /* mod_exp (the i++ / i+=2 etc. step) */
            int mod_pc = (int)c->chunk->code_len;
            /* Patch all `continue` JUMPs collected during body compilation. */
            for (int i = 0; i < lc->n_continues; ++i) {
                patch_jump(c, lc->continues[i], mod_pc);
                if (!c->ok) { c->n_loops--; return; }
            }
            if (f->mod_exp != NULL) {
                int sp_before = c->sp;
                compile_exp(c, f->mod_exp);
                if (!c->ok) { c->n_loops--; return; }
                while (c->sp > sp_before) {
                    emit(c, MLN_VOP_POP, 0, 0);
                    sp_pop(c, 1);
                }
            }
            /* jump back to cond */
            int j_back = emit(c, MLN_VOP_JUMP, 0, 0);
            patch_jump(c, j_back, cond_pc);
            if (!c->ok) { c->n_loops--; return; }

            /* exit landing */
            int after = (int)c->chunk->code_len;
            if (j_exit >= 0) {
                patch_jump(c, j_exit, after);
                if (!c->ok) { c->n_loops--; return; }
            }
            for (int i = 0; i < lc->n_breaks; ++i) {
                patch_jump(c, lc->breaks[i], after);
                if (!c->ok) { c->n_loops--; return; }
            }
            c->n_loops--;
            return;
        }
        default:
            bail(c);
            return;
    }
}

static void compile_block(mln_lang_vm_compiler_t *c, mln_lang_block_t *block)
{
    switch (block->type) {
        case M_BLOCK_EXP:
            if (block->data.exp != NULL) {
                int sp_before = c->sp;
                compile_exp(c, block->data.exp);
                if (!c->ok) return;
                while (c->sp > sp_before) {
                    emit(c, MLN_VOP_POP, 0, 0);
                    sp_pop(c, 1);
                }
            }
            return;
        case M_BLOCK_STM:
            compile_stm_chain(c, block->data.stm);
            return;
        case M_BLOCK_RETURN:
            if (block->data.exp == NULL) {
                emit(c, MLN_VOP_RETURN_NIL, 0, 0);
            } else {
                int sp_before = c->sp;
                compile_exp(c, block->data.exp);
                if (!c->ok) return;
                if (c->sp != sp_before + 1) { bail(c); return; }
                emit(c, MLN_VOP_RETURN, 0, 0);
                sp_pop(c, 1);
            }
            return;
        case M_BLOCK_GOTO: {
            /* If label already known, emit JUMP with offset; else emit
             * placeholder and record patch. */
            mln_string_t *target = block->data.pos;
            if (target == NULL) { bail(c); return; }
            int found = -1;
            for (int i = 0; i < c->n_labels; ++i) {
                if (c->labels[i].name != NULL &&
                    c->labels[i].name->len == target->len &&
                    !memcmp(c->labels[i].name->data, target->data, target->len))
                {
                    found = c->labels[i].pc; break;
                }
            }
            if (found >= 0) {
                int j = emit(c, MLN_VOP_JUMP, 0, 0);
                patch_jump(c, j, found);
            } else {
                int j = emit(c, MLN_VOP_JUMP, 0, 0);
                if (compiler_push_goto(c, target, j) < 0) return;
            }
            return;
        }
        case M_BLOCK_BREAK: {
            if (c->n_loops == 0) { bail(c); return; }
            loop_ctx_t *lc = &c->loops[c->n_loops - 1];
            int j_brk = emit(c, MLN_VOP_JUMP, 0, 0);
            if (lc_push_break(c, lc, j_brk) < 0) return;
            return;
        }
        case M_BLOCK_CONTINUE: {
            if (c->n_loops == 0) { bail(c); return; }
            loop_ctx_t *lc = &c->loops[c->n_loops - 1];
            if (lc->continue_pc >= 0) {
                /* while loop: continue_pc is already known */
                int j = emit(c, MLN_VOP_JUMP, 0, 0);
                patch_jump(c, j, lc->continue_pc);
            } else {
                /* for loop: continue_pc not yet known — record patch index */
                int j = emit(c, MLN_VOP_JUMP, 0, 0);
                if (lc_push_continue(c, lc, j) < 0) return;
            }
            return;
        }
        case M_BLOCK_IF: {
            mln_lang_if_t *iff = block->data.i;
            int sp_before = c->sp;
            compile_exp(c, iff->condition);
            if (!c->ok) return;
            if (c->sp != sp_before + 1) { bail(c); return; }
            int jf = emit(c, MLN_VOP_JUMP_IF_FALSE, 0, 0);
            sp_pop(c, 1);
            if (jf < 0) return;
            compile_block(c, iff->blockstm);
            if (!c->ok) return;
            int after_then = (int)c->chunk->code_len;
            if (iff->elsestm != NULL) {
                int j_end = emit(c, MLN_VOP_JUMP, 0, 0);
                if (j_end < 0) return;
                int else_target = (int)c->chunk->code_len;
                patch_jump(c, jf, else_target);
                if (!c->ok) return;
                compile_block(c, iff->elsestm);
                if (!c->ok) return;
                int end_target = (int)c->chunk->code_len;
                patch_jump(c, j_end, end_target);
            } else {
                patch_jump(c, jf, after_then);
            }
            return;
        }
        default:
            bail(c);
            return;
    }
}

static void compile_exp(mln_lang_vm_compiler_t *c, mln_lang_exp_t *exp)
{
    if (exp == NULL) { bail(c); return; }
    compile_assign(c, exp->assign);
    /* Comma chain: compile each sub-expression, discard all intermediate
     * results, keep the last.  The value of (a, b, c) is c. */
    while (c->ok && exp->next != NULL) {
        emit(c, MLN_VOP_POP, 0, 0);
        sp_pop(c, 1);
        exp = exp->next;
        compile_assign(c, exp->assign);
    }
}

static void compile_assign(mln_lang_vm_compiler_t *c, mln_lang_assign_t *a)
{
    if (a == NULL) { bail(c); return; }
    if (a->op == M_ASSIGN_NONE) {
        compile_logiclow(c, a->left);
        return;
    }

    /* Map compound-assign ops to their bin-op equivalents.
     * compound_op stays -1 for M_ASSIGN_EQUAL (simple assignment, no binop). */
    int compound_op = -1;
    switch (a->op) {
        case M_ASSIGN_EQUAL:                                break;
        case M_ASSIGN_PLUSEQ:  compound_op = MLN_VOP_ADD;    break;
        case M_ASSIGN_SUBEQ:   compound_op = MLN_VOP_SUB;    break;
        case M_ASSIGN_MULEQ:   compound_op = MLN_VOP_MUL;    break;
        case M_ASSIGN_DIVEQ:   compound_op = MLN_VOP_DIV;    break;
        case M_ASSIGN_MODEQ:   compound_op = MLN_VOP_MOD;    break;
        case M_ASSIGN_OREQ:    compound_op = MLN_VOP_BOR;    break;
        case M_ASSIGN_ANDEQ:   compound_op = MLN_VOP_BAND;   break;
        case M_ASSIGN_XOREQ:   compound_op = MLN_VOP_BXOR;   break;
        case M_ASSIGN_LMOVEQ:  compound_op = MLN_VOP_LSHIFT; break;
        case M_ASSIGN_RMOVEQ:  compound_op = MLN_VOP_RSHIFT; break;
        default: bail(c); return;
    }

    if (a->right == NULL) { bail(c); return; }

    /* Known local (arg / closure capture): emit ASSIGN_LOCAL. */
    int slot = extract_lhs_local(c, a->left);
    if (slot >= 0) {
        if (compound_op < 0) {
            int sp_before = c->sp;
            compile_assign(c, a->right);
            if (!c->ok) return;
            if (c->sp != sp_before + 1) { bail(c); return; }
            emit(c, MLN_VOP_ASSIGN_LOCAL, (mln_u8_t)slot, 0);
        } else {
            emit(c, MLN_VOP_LOAD_LOCAL, (mln_u8_t)slot, 0);
            sp_push(c, 1);
            int sp_before = c->sp;
            compile_assign(c, a->right);
            if (!c->ok) return;
            if (c->sp != sp_before + 1) { bail(c); return; }
            emit(c, (mln_u8_t)compound_op, 0, 0);
            sp_pop(c, 1);
            emit(c, MLN_VOP_ASSIGN_LOCAL, (mln_u8_t)slot, 0);
            /* ASSIGN_LOCAL pops val and pushes it back, so sp stays. */
        }
        return;
    }

    /* Bare identifier not in local_names: emit ASSIGN_GLOBAL so the runtime
     * scope-chain search finds (or lazily creates) the correct variable,
     * matching the AST interpreter (src/mln_lang.c:6508-6537). */
    if (slot == -1) {
        /* extract_lhs_local returns -1 only for M_FACTOR_ID not in locals. */
        mln_lang_locate_t *bare = unwrap_to_locate(a->left);
        /* bare is guaranteed non-NULL with op==NONE when slot==-1 */
        mln_lang_factor_t *fac = bare->left->data.factor;
        int gidx = add_sconst(c, fac->data.s_id);
        if (gidx < 0 || gidx > 32767) { bail(c); return; }
        if (compound_op < 0) {
            int sp_before = c->sp;
            compile_assign(c, a->right);
            if (!c->ok) return;
            if (c->sp != sp_before + 1) { bail(c); return; }
            emit(c, MLN_VOP_ASSIGN_GLOBAL, 0, (mln_s16_t)gidx);
            /* ASSIGN_GLOBAL pops val, pushes back — sp unchanged */
        } else {
            emit(c, MLN_VOP_LOAD_GLOBAL, 0, (mln_s16_t)gidx);
            sp_push(c, 1);
            int sp_before = c->sp;
            compile_assign(c, a->right);
            if (!c->ok) return;
            if (c->sp != sp_before + 1) { bail(c); return; }
            emit(c, (mln_u8_t)compound_op, 0, 0);
            sp_pop(c, 1);
            emit(c, MLN_VOP_ASSIGN_GLOBAL, 0, (mln_s16_t)gidx);
        }
        return;
    }

    /* slot == -2: LHS is a locate chain (property or index). */

    mln_lang_locate_t *locate = unwrap_to_locate(a->left);
    if (locate == NULL || locate->op == M_LOCATE_NONE) { bail(c); return; }

    /* Compile base. */
    compile_spec(c, locate->left);
    if (!c->ok) return;

    /* Walk all but the last hop, emitting GETs. */
    while (locate->next != NULL) {
        if (locate->op == M_LOCATE_PROPERTY) {
            int idx = add_sconst(c, locate->right.id);
            if (idx < 0 || idx > 32767) { bail(c); return; }
            emit(c, MLN_VOP_GET_PROPERTY, 0, (mln_s16_t)idx);
        } else if (locate->op == M_LOCATE_INDEX) {
            if (locate->right.exp == NULL) { bail(c); return; }
            int sp_before = c->sp;
            compile_exp(c, locate->right.exp);
            if (!c->ok) return;
            if (c->sp != sp_before + 1) { bail(c); return; }
            emit(c, MLN_VOP_GET_INDEX, 0, 0);
            sp_pop(c, 1);
        } else {
            bail(c); return;
        }
        locate = locate->next;
    }

    /* Final hop is the assignment target.
     * For compound ops, we need to read the current value first:
     *   DUP the object (so SET can still use it), GET the value,
     *   compile the RHS, apply the binary op, then SET.
     * For plain assignment, just compile RHS and SET. */
    if (locate->op == M_LOCATE_PROPERTY) {
        int idx = add_sconst(c, locate->right.id);
        if (idx < 0 || idx > 32767) { bail(c); return; }
        if (compound_op >= 0) {
            emit(c, MLN_VOP_DUP, 0, 0);    /* [obj, obj2] */
            sp_push(c, 1);
            emit(c, MLN_VOP_GET_PROPERTY, 0, (mln_s16_t)idx);
            /* GET_PROPERTY: pops obj2, pushes old_val — sp unchanged */
        }
        int sp_before = c->sp;
        compile_assign(c, a->right);
        if (!c->ok) return;
        if (c->sp != sp_before + 1) { bail(c); return; }
        if (compound_op >= 0) {
            emit(c, (mln_u8_t)compound_op, 0, 0);
            sp_pop(c, 1);
        }
        emit(c, MLN_VOP_SET_PROPERTY, 1, (mln_s16_t)idx);
        sp_pop(c, 1);   /* obj consumed; val pushed back as expression result */
    } else if (locate->op == M_LOCATE_INDEX) {
        if (locate->right.exp == NULL) { bail(c); return; }
        if (compound_op >= 0) {
            emit(c, MLN_VOP_DUP, 0, 0);    /* [arr, arr2] */
            sp_push(c, 1);
        }
        int sp_before = c->sp;
        compile_exp(c, locate->right.exp);
        if (!c->ok) return;
        if (c->sp != sp_before + 1) { bail(c); return; }
        if (compound_op >= 0) {
            /* Stack is [arr, arr2, key].  DUP key → [arr, arr2, key, key2].
             * SWAP2 reorders 2nd and 3rd from top → [arr, key, arr2, key2].
             * GET_INDEX now pops key2 (top) and arr2 (correct). */
            emit(c, MLN_VOP_DUP, 0, 0);    /* [arr, arr2, key, key2] */
            sp_push(c, 1);
            emit(c, MLN_VOP_SWAP2, 0, 0);  /* [arr, key, arr2, key2] */
            emit(c, MLN_VOP_GET_INDEX, 0, 0);
            sp_pop(c, 1);  /* GET_INDEX: pops key2+arr2, pushes elem; net -1 */
        }
        sp_before = c->sp;
        compile_assign(c, a->right);
        if (!c->ok) return;
        if (c->sp != sp_before + 1) { bail(c); return; }
        if (compound_op >= 0) {
            emit(c, (mln_u8_t)compound_op, 0, 0);
            sp_pop(c, 1);
        }
        emit(c, MLN_VOP_SET_INDEX, 1, 0);
        sp_pop(c, 2);   /* arr+key consumed; val pushed back as expression result */
    } else {
        bail(c);
    }
}

static void compile_logiclow(mln_lang_vm_compiler_t *c, mln_lang_logiclow_t *n)
{
    if (n == NULL) { bail(c); return; }
    /* Phase E: short-circuit OR (||) at logiclow level — the AST grammar
     * actually uses logiclow for `||` and logichigh for `&&`. We implement
     * both here for safety: logiclow.op = OR / AND. */
    compile_logichigh(c, n->left);
    while (c->ok && n->op != M_LOGICLOW_NONE) {
        if (n->right == NULL) { bail(c); return; }
        /* Pattern for `a OR b`:
         *   compile a → [a]
         *   DUP       → [a, a]
         *   JIF_TRUE end (pops top; if true, jumps; else fall through)
         *                → on fall-through: [a]
         *   POP       → []
         *   compile b → [b]
         *   end:
         * For AND, swap JIF_TRUE↔JIF_FALSE.  */
        emit(c, MLN_VOP_DUP, 0, 0);
        sp_push(c, 1);
        int j_short;
        if (n->op == M_LOGICLOW_OR) {
            j_short = emit(c, MLN_VOP_JUMP_IF_TRUE, 0, 0);
        } else if (n->op == M_LOGICLOW_AND) {
            j_short = emit(c, MLN_VOP_JUMP_IF_FALSE, 0, 0);
        } else {
            bail(c); return;
        }
        sp_pop(c, 1);   /* JIF pops */
        emit(c, MLN_VOP_POP, 0, 0);
        sp_pop(c, 1);
        /* Right side. We compile only the immediate right.left as
         * logichigh, and iterate via n = n->right. */
        compile_logichigh(c, n->right->left);
        if (!c->ok) return;
        patch_jump(c, j_short, (int)c->chunk->code_len);
        if (!c->ok) return;
        n = n->right;
    }
}

static void compile_logichigh(mln_lang_vm_compiler_t *c, mln_lang_logichigh_t *n)
{
    if (n == NULL) { bail(c); return; }
    compile_relativelow(c, n->left);
    /* Melang's logichigh operators (|, &, ^) are BITWISE, not short-circuit.
     * The AST walker always evaluates both sides before calling cor/cand/cxor
     * handlers.  Emit a plain binary opcode — no jump patching needed. */
    while (c->ok && n->op != M_LOGICHIGH_NONE) {
        if (n->right == NULL) { bail(c); return; }
        compile_relativelow(c, n->right->left);
        if (!c->ok) return;
        mln_u8_t fop;
        switch (n->op) {
            case M_LOGICHIGH_OR:  fop = MLN_VOP_BOR;  break;
            case M_LOGICHIGH_AND: fop = MLN_VOP_BAND; break;
            case M_LOGICHIGH_XOR: fop = MLN_VOP_BXOR; break;
            default: bail(c); return;
        }
        if (!try_fold_int_binop(c, fop)) {
            emit(c, fop, 0, 0);
            sp_pop(c, 1);   /* binary op pops 2, pushes 1 */
        }
        n = n->right;
    }
}

static void compile_relativelow(mln_lang_vm_compiler_t *c, mln_lang_relativelow_t *n)
{
    if (n == NULL) { bail(c); return; }
    compile_relativehigh(c, n->left);
    while (c->ok && n->op != M_RELATIVELOW_NONE) {
        if (n->right == NULL) { bail(c); return; }
        compile_relativehigh(c, n->right->left);
        if (!c->ok) return;
        switch (n->op) {
            case M_RELATIVELOW_EQUAL:  emit(c, MLN_VOP_EQ, 0, 0); break;
            case M_RELATIVELOW_NEQUAL: emit(c, MLN_VOP_NE, 0, 0); break;
            default: bail(c); return;
        }
        sp_pop(c, 1);
        n = n->right;
    }
}

static void compile_relativehigh(mln_lang_vm_compiler_t *c, mln_lang_relativehigh_t *n)
{
    if (n == NULL) { bail(c); return; }
    compile_move(c, n->left);
    while (c->ok && n->op != M_RELATIVEHIGH_NONE) {
        if (n->right == NULL) { bail(c); return; }
        compile_move(c, n->right->left);
        if (!c->ok) return;
        switch (n->op) {
            case M_RELATIVEHIGH_LESS:      emit(c, MLN_VOP_LT, 0, 0); break;
            case M_RELATIVEHIGH_LESSEQ:    emit(c, MLN_VOP_LE, 0, 0); break;
            case M_RELATIVEHIGH_GREATER:   emit(c, MLN_VOP_GT, 0, 0); break;
            case M_RELATIVEHIGH_GREATEREQ: emit(c, MLN_VOP_GE, 0, 0); break;
            default: bail(c); return;
        }
        sp_pop(c, 1);
        n = n->right;
    }
}

static void compile_move(mln_lang_vm_compiler_t *c, mln_lang_move_t *n)
{
    if (n == NULL) { bail(c); return; }
    compile_addsub(c, n->left);
    while (c->ok && n->op != M_MOVE_NONE) {
        if (n->right == NULL) { bail(c); return; }
        compile_addsub(c, n->right->left);
        if (!c->ok) return;
        mln_u8_t fop;
        switch (n->op) {
            case M_MOVE_LMOVE: fop = MLN_VOP_LSHIFT; break;
            case M_MOVE_RMOVE: fop = MLN_VOP_RSHIFT; break;
            default: bail(c); return;
        }
        if (!try_fold_int_binop(c, fop)) {
            emit(c, fop, 0, 0);
            sp_pop(c, 1);
        }
        n = n->right;
    }
}

static void compile_addsub(mln_lang_vm_compiler_t *c, mln_lang_addsub_t *n)
{
    if (n == NULL) { bail(c); return; }
    compile_muldiv(c, n->left);
    while (c->ok && n->op != M_ADDSUB_NONE) {
        if (n->right == NULL) { bail(c); return; }
        compile_muldiv(c, n->right->left);
        if (!c->ok) return;
        mln_u8_t fop;
        switch (n->op) {
            case M_ADDSUB_PLUS: fop = MLN_VOP_ADD; break;
            case M_ADDSUB_SUB:  fop = MLN_VOP_SUB; break;
            default: bail(c); return;
        }
        if (!try_fold_int_binop(c, fop)) {
            emit(c, fop, 0, 0);
            sp_pop(c, 1);
        }
        n = n->right;
    }
}

static void compile_muldiv(mln_lang_vm_compiler_t *c, mln_lang_muldiv_t *n)
{
    if (n == NULL) { bail(c); return; }
    compile_not(c, n->left);
    while (c->ok && n->op != M_MULDIV_NONE) {
        if (n->right == NULL) { bail(c); return; }
        compile_not(c, n->right->left);
        if (!c->ok) return;
        mln_u8_t fop;
        switch (n->op) {
            case M_MULDIV_MUL: fop = MLN_VOP_MUL; break;
            case M_MULDIV_DIV: fop = MLN_VOP_DIV; break;
            case M_MULDIV_MOD: fop = MLN_VOP_MOD; break;
            default: bail(c); return;
        }
        if (!try_fold_int_binop(c, fop)) {
            emit(c, fop, 0, 0);
            sp_pop(c, 1);
        }
        n = n->right;
    }
}

static void compile_not(mln_lang_vm_compiler_t *c, mln_lang_not_t *n)
{
    if (n == NULL) { bail(c); return; }
    if (n->op == M_NOT_NONE) {
        compile_suffix(c, n->right.suffix);
        return;
    }
    /* M_NOT_NOT */
    compile_not(c, n->right.not);
    if (!c->ok) return;
    emit(c, MLN_VOP_NOT, 0, 0);
}

static void compile_suffix(mln_lang_vm_compiler_t *c, mln_lang_suffix_t *n)
{
    if (n == NULL) { bail(c); return; }
    if (n->op == M_SUFFIX_NONE) {
        compile_locate(c, n->left);
        return;
    }
    /* `lv++` / `lv--`: support locals, globals, property, and index lvalues. */
    mln_lang_locate_t *lc = n->left;
    if (lc == NULL) { bail(c); return; }

    /* ── Plain identifier: local or global ── */
    if (lc->op == M_LOCATE_NONE && lc->next == NULL) {
        mln_lang_spec_t *sp_node = lc->left;
        if (sp_node == NULL || sp_node->op != M_SPEC_FACTOR) { bail(c); return; }
        mln_lang_factor_t *f = sp_node->data.factor;
        if (f == NULL || f->type != M_FACTOR_ID) { bail(c); return; }
        int slot = find_local_slot(c, f->data.s_id);
        if (slot >= 0) {
            /* Local fast path */
            switch (n->op) {
                case M_SUFFIX_INC: emit(c, MLN_VOP_LOAD_LOCAL_INC, (mln_u8_t)slot, 0); break;
                case M_SUFFIX_DEC: emit(c, MLN_VOP_LOAD_LOCAL_DEC, (mln_u8_t)slot, 0); break;
                default: bail(c); return;
            }
            sp_push(c, 1);
            return;
        }
        /* Global postfix: DUP only adds a reference to the same var, so when
         * ASSIGN_GLOBAL later modifies the var's value the "old" copy on the
         * stack would also reflect the new value.  Use a scratch slot and
         * STORE_LOCAL to make a true value-copy first, like the property and
         * index suffix cases do. */
        int temp = alloc_temp_slot(c);
        if (temp < 0) return;   /* bail already called */
        int gidx = add_sconst(c, f->data.s_id);
        if (gidx < 0 || gidx > 32767) { bail(c); return; }
        emit(c, MLN_VOP_LOAD_GLOBAL, 0, (mln_s16_t)gidx);   /* [a_var] */
        sp_push(c, 1);
        emit(c, MLN_VOP_STORE_LOCAL, (mln_u8_t)temp, 0);    /* value-copy to scratch; pops a_var */
        sp_pop(c, 1);
        emit(c, MLN_VOP_LOAD_LOCAL, (mln_u8_t)temp, 0);     /* [old_copy] */
        sp_push(c, 1);
        emit_load_int_one(c);                                  /* [old_copy, 1] */
        if (!c->ok) return;
        emit(c, (n->op == M_SUFFIX_INC) ? MLN_VOP_ADD : MLN_VOP_SUB, 0, 0);
        sp_pop(c, 1);                                          /* [new_val] */
        emit(c, MLN_VOP_ASSIGN_GLOBAL, 0, (mln_s16_t)gidx);  /* assigns, pushes back new_val */
        emit(c, MLN_VOP_POP, 0, 0);                           /* pop the push-back */
        sp_pop(c, 1);
        emit(c, MLN_VOP_LOAD_LOCAL, (mln_u8_t)temp, 0);      /* [old_copy] — expression result */
        sp_push(c, 1);
        return;
    }

    /* ── Single-hop property: obj.x++ / obj.x-- ── */
    if (lc->op == M_LOCATE_PROPERTY && lc->next == NULL) {
        if (lc->right.id == NULL) { bail(c); return; }
        int idx = add_sconst(c, lc->right.id);
        if (idx < 0 || idx > 32767) { bail(c); return; }
        /* Allocate a scratch slot to hold the old value before SET_PROPERTY
         * overwrites the property slot in-place (GET_PROPERTY returns a
         * reference to the actual slot, so a DUP would also see the new val). */
        int temp = alloc_temp_slot(c);
        if (temp < 0) return;  /* alloc_temp_slot already called bail(c) */
        /* Compile base object */
        compile_spec(c, lc->left);
        if (!c->ok) return;
        emit(c, MLN_VOP_DUP, 0, 0);
        sp_push(c, 1);
        /* GET_PROPERTY pops obj2 (DUP'd top), pushes ref to the property
         * slot variable.  Net effect: [obj, obj2] → [obj, old_val_ref],
         * sp unchanged. */
        emit(c, MLN_VOP_GET_PROPERTY, 0, (mln_s16_t)idx);        /* [obj, old_val_ref] */
        /* Save a VALUE copy into temp slot (STORE_LOCAL copies the val data,
         * so later SET_PROPERTY cannot overwrite this copy). */
        emit(c, MLN_VOP_STORE_LOCAL, (mln_u8_t)temp, 0);         /* [obj] */
        sp_pop(c, 1);
        emit(c, MLN_VOP_LOAD_LOCAL, (mln_u8_t)temp, 0);          /* [obj, old_copy] */
        sp_push(c, 1);
        emit_load_int_one(c);                                      /* [obj, old_copy, 1] */
        if (!c->ok) return;
        emit(c, (n->op == M_SUFFIX_INC) ? MLN_VOP_ADD : MLN_VOP_SUB, 0, 0);
        sp_pop(c, 1);                                              /* [obj, new_val] */
        emit(c, MLN_VOP_SET_PROPERTY, 0, (mln_s16_t)idx);        /* pops new_val + obj */
        sp_pop(c, 2);                                              /* [] */
        emit(c, MLN_VOP_LOAD_LOCAL, (mln_u8_t)temp, 0);          /* [old_copy] */
        sp_push(c, 1);
        return;
    }

    /* ── Single-hop index: arr[i]++ / arr[i]-- ── */
    if (lc->op == M_LOCATE_INDEX && lc->next == NULL) {
        if (lc->right.exp == NULL) { bail(c); return; }
        /* Allocate a scratch slot to hold old_val across the SET_INDEX. */
        int temp = alloc_temp_slot(c);
        if (temp < 0) return;  /* alloc_temp_slot already called bail(c) */
        /* Compile base array */
        compile_spec(c, lc->left);
        if (!c->ok) return;
        emit(c, MLN_VOP_DUP, 0, 0);                /* [arr, arr2] */
        sp_push(c, 1);
        /* Compile key */
        int sp_before = c->sp;
        compile_exp(c, lc->right.exp);
        if (!c->ok) return;
        if (c->sp != sp_before + 1) { bail(c); return; }
        /* Stack: [arr, arr2, key].  DUP key, SWAP2 to get [arr, key, arr2, key2]
         * so that GET_INDEX pops key2+arr2 correctly. */
        emit(c, MLN_VOP_DUP, 0, 0);                /* [arr, arr2, key, key2] */
        sp_push(c, 1);
        emit(c, MLN_VOP_SWAP2, 0, 0);              /* [arr, key, arr2, key2] */
        emit(c, MLN_VOP_GET_INDEX, 0, 0);           /* pops key2+arr2, pushes old_val_ref */
        sp_pop(c, 1);                                /* [arr, key, old_val_ref] */
        /* Save old_val into scratch slot (copies value, safe from SET_INDEX). */
        emit(c, MLN_VOP_STORE_LOCAL, (mln_u8_t)temp, 0);
        sp_pop(c, 1);                                /* [arr, key] */
        /* Load old_val copy, add 1, produce new_val */
        emit(c, MLN_VOP_LOAD_LOCAL, (mln_u8_t)temp, 0);
        sp_push(c, 1);                               /* [arr, key, old_copy] */
        emit_load_int_one(c);                        /* [arr, key, old_copy, 1] */
        if (!c->ok) return;
        emit(c, (n->op == M_SUFFIX_INC) ? MLN_VOP_ADD : MLN_VOP_SUB, 0, 0);
        sp_pop(c, 1);                                /* [arr, key, new_val] */
        emit(c, MLN_VOP_SET_INDEX, 0, 0);           /* pops new_val + key + arr */
        sp_pop(c, 3);                                /* [] */
        /* Reload old_val from scratch slot as expression result */
        emit(c, MLN_VOP_LOAD_LOCAL, (mln_u8_t)temp, 0);
        sp_push(c, 1);                               /* [old_val] */
        return;
    }

    bail(c);
}

static void compile_locate(mln_lang_vm_compiler_t *c, mln_lang_locate_t *n)
{
    if (n == NULL) { bail(c); return; }
    if (n->op == M_LOCATE_NONE) {
        compile_spec(c, n->left);
        return;
    }

    /* Phase E optimization: detect single-step FUNC call where the callee
     * identifier resolves to the prototype currently being compiled —
     * emit CALL_SELF (skips one symbol lookup + funccall->prototype
     * resolution per call). For deep recursion (fib) this is a measurable
     * win over the CALL_VALUE path. */
    if (n->op == M_LOCATE_FUNC && n->next == NULL) {
        mln_lang_spec_t *sp = n->left;
        mln_lang_factor_t *f = (sp != NULL && sp->op == M_SPEC_FACTOR) ? sp->data.factor : NULL;
        if (f != NULL && f->type == M_FACTOR_ID) {
            mln_lang_symbol_node_t *sym = mln_lang_symbol_node_search(c->ctx, f->data.s_id, 0);
            if (sym != NULL && sym->type == M_LANG_SYMBOL_VAR &&
                sym->data.var != NULL && sym->data.var->val != NULL &&
                sym->data.var->val->type == M_LANG_VAL_TYPE_FUNC &&
                sym->data.var->val->data.func == c->prototype)
            {
                /* Self-recursion fast path. */
                mln_lang_exp_t *arg = n->right.exp;
                mln_size_t nargs = 0;
                for (mln_lang_exp_t *p = arg; p != NULL; p = p->next) ++nargs;
                if (nargs > 255 || nargs != c->n_args) goto no_self;
                for (mln_lang_exp_t *p = arg; p != NULL; p = p->next) {
                    int sp_before = c->sp;
                    mln_lang_exp_t saved = *p;
                    saved.next = NULL;
                    compile_assign(c, saved.assign);
                    if (!c->ok) return;
                    if (c->sp != sp_before + 1) { bail(c); return; }
                }
                emit(c, MLN_VOP_CALL_SELF, (mln_u8_t)nargs, 0);
                sp_pop(c, (int)nargs);
                sp_push(c, 1);
                return;
            }
        }
    }
no_self:

    /* Medium-tier perf: detect bare global-call `name(args)` and emit a
     * fused CALL_GLOBAL opcode. Avoids the LOAD_GLOBAL+CALL_VALUE pair
     * and threads an inline cache for the resolved func_detail through
     * the IC slot at this pc. Only applies when the callee identifier is
     * not a local (otherwise normal CALL_VALUE handles it correctly). */
    if (n->op == M_LOCATE_FUNC && n->next == NULL) {
        mln_lang_spec_t *sp = n->left;
        mln_lang_factor_t *f = (sp != NULL && sp->op == M_SPEC_FACTOR) ? sp->data.factor : NULL;
        if (f != NULL && f->type == M_FACTOR_ID &&
            find_local_slot(c, f->data.s_id) < 0)
        {
            mln_lang_exp_t *arg = n->right.exp;
            mln_size_t nargs = 0;
            for (mln_lang_exp_t *p = arg; p != NULL; p = p->next) ++nargs;
            if (nargs <= 255) {
                int name_idx = add_sconst(c, f->data.s_id);
                if (name_idx < 0 || name_idx > 32767) { bail(c); return; }
                for (mln_lang_exp_t *p = arg; p != NULL; p = p->next) {
                    int sp_before = c->sp;
                    mln_lang_exp_t saved = *p;
                    saved.next = NULL;
                    compile_assign(c, saved.assign);
                    if (!c->ok) return;
                    if (c->sp != sp_before + 1) { bail(c); return; }
                }
                emit(c, MLN_VOP_CALL_GLOBAL, (mln_u8_t)nargs, (mln_s16_t)name_idx);
                sp_pop(c, (int)nargs);
                sp_push(c, 1);
                return;
            }
        }
    }

    /* Compile base. */
    compile_spec(c, n->left);
    if (!c->ok) return;

    /* Walk the chain. For `obj.method(args)` we need to keep `obj` on the
     * stack to bind `this` — we DUP before GET_PROPERTY when the next hop
     * is FUNC. Otherwise GET_PROPERTY just consumes the obj. */
    while (c->ok && n != NULL && n->op != M_LOCATE_NONE) {
        switch (n->op) {
            case M_LOCATE_INDEX: {
                if (n->right.exp == NULL) { bail(c); return; }
                int sp_before = c->sp;
                compile_exp(c, n->right.exp);
                if (!c->ok) return;
                if (c->sp != sp_before + 1) { bail(c); return; }
                emit(c, MLN_VOP_GET_INDEX, 0, 0);
                sp_pop(c, 1);    /* pop key, replace base with elem */
                break;
            }
            case M_LOCATE_PROPERTY: {
                if (n->right.id == NULL) { bail(c); return; }
                int idx = add_sconst(c, n->right.id);
                if (idx < 0 || idx > 32767) { bail(c); return; }
                int prop_then_func = (n->next != NULL && n->next->op == M_LOCATE_FUNC);
                if (prop_then_func) {
                    /* Medium-tier perf: defer GET_PROPERTY. The upcoming FUNC
                     * hop will fuse this name into a single INVOKE_METHOD
                     * opcode. Leave the obj on the stack as the receiver.
                     * No DUP needed because INVOKE_METHOD consumes obj+args
                     * directly (no separate func slot on stack). */
                    c->prev_property_name_idx = idx;
                    /* Note: stack invariant unchanged — obj is still TOS. */
                } else {
                    emit(c, MLN_VOP_GET_PROPERTY, 0, (mln_s16_t)idx);
                    /* GET_PROPERTY: pop obj, push prop val (sp unchanged) */
                }
                break;
            }
            case M_LOCATE_FUNC: {
                /* If the previous hop was PROPERTY, we already DUP'd the
                 * obj — emit CALL_METHOD. Otherwise it's a regular call. */
                int is_method = 0;
                /* The 'previous' hop is reflected in stack layout: if the
                 * stack has [obj, func, args...], CALL_METHOD pops them.
                 * We track this via the prev locate's op. */
                /* Detect: walk locates from start, check if the locate
                 * immediately preceding this FUNC was PROPERTY. Simpler:
                 * check if the previous step was a DUP+GET_PROPERTY (i.e.,
                 * the immediately prior code length included a DUP). We
                 * pass the info via a flag set in the PROPERTY branch.
                 * Cleanest: derive via a lookback variable maintained in
                 * this loop. */
                /* Note: by construction, the only way DUP got emitted in
                 * the chain is via the PROPERTY-then-FUNC pattern above,
                 * because the FUNC hop itself never DUPs. So if the
                 * previous handled hop set the flag, this is a method. */
                /* See `is_method_call_prev` tracking maintained externally. */
                (void)is_method;
                /* ... */
                /* Simpler implementation: examine the AST to determine. */
                /* Find the previous hop in the chain by walking from the
                 * head until just before n. */
                /* This is O(chain) per FUNC, but chains are tiny. */
                {
                    mln_lang_locate_t *head = NULL;
                    /* We don't have easy access to the chain head here.
                     * Track via a small heuristic: if our chain came in
                     * via a multi-hop chain whose immediately preceding
                     * locate had op==PROPERTY, we DUP'd. Pass the flag
                     * through a pointer-state — since we don't have a
                     * back-pointer in mln_lang_locate_t, we re-walk from
                     * c->prototype's body... too convoluted. Use a state
                     * variable in the compiler temporarily. */
                    (void)head;
                }
                /* Practical: maintain a `prev_was_property` flag in the
                 * compiler struct, set when we emit GET_PROPERTY with the
                 * DUP, cleared when we emit other ops. */
                if (c->prev_was_property) {
                    is_method = 1;
                    c->prev_was_property = 0;
                }
                /* Medium-tier perf: a PROPERTY-then-FUNC pair was deferred so
                 * we can fuse it into INVOKE_METHOD here. */
                int fused_method = (c->prev_property_name_idx >= 0);
                int method_name_idx = c->prev_property_name_idx;
                c->prev_property_name_idx = -1;
                if (fused_method) {
                    /* When fused, stack currently has just [obj]; the prior
                     * PROPERTY hop did NOT emit DUP/GET_PROPERTY. */
                    is_method = 0; /* obj+args+method_name path below */
                }
                mln_lang_exp_t *arg = n->right.exp;
                mln_size_t nargs = 0;
                for (mln_lang_exp_t *p = arg; p != NULL; p = p->next) ++nargs;
                if (nargs > 255) { bail(c); return; }
                for (mln_lang_exp_t *p = arg; p != NULL; p = p->next) {
                    int sp_before = c->sp;
                    mln_lang_exp_t saved = *p;
                    saved.next = NULL;
                    compile_assign(c, saved.assign);
                    if (!c->ok) return;
                    if (c->sp != sp_before + 1) { bail(c); return; }
                }
                if (fused_method) {
                    /* Stack: [..., obj, arg0, ..., argN-1] */
                    emit(c, MLN_VOP_INVOKE_METHOD, (mln_u8_t)nargs, (mln_s16_t)method_name_idx);
                    sp_pop(c, (int)(nargs + 1));   /* nargs + obj */
                } else if (is_method) {
                    emit(c, MLN_VOP_CALL_METHOD, (mln_u8_t)nargs, 0);
                    sp_pop(c, (int)(nargs + 2));   /* nargs + func + obj */
                } else {
                    emit(c, MLN_VOP_CALL_VALUE, (mln_u8_t)nargs, 0);
                    sp_pop(c, (int)(nargs + 1));   /* nargs + func */
                }
                sp_push(c, 1);
                break;
            }
            default:
                bail(c);
                return;
        }
        /* Track for the next hop. */
        c->prev_was_property = (n->op == M_LOCATE_PROPERTY &&
                                n->next != NULL && n->next->op == M_LOCATE_FUNC);
        n = n->next;
    }
}

static void compile_spec(mln_lang_vm_compiler_t *c, mln_lang_spec_t *n)
{
    if (n == NULL) { bail(c); return; }
    switch (n->op) {
        case M_SPEC_FACTOR:
            compile_factor(c, n->data.factor);
            return;
        case M_SPEC_PARENTH:
            compile_exp(c, n->data.exp);
            return;
        case M_SPEC_NEGATIVE:
            compile_spec(c, n->data.spec);
            if (!c->ok) return;
            emit(c, MLN_VOP_NEG, 0, 0);
            return;
        case M_SPEC_REVERSE:
            compile_spec(c, n->data.spec);
            if (!c->ok) return;
            emit(c, MLN_VOP_BITNOT, 0, 0);
            return;
        case M_SPEC_REFER: {
            /* &x — pass variable by reference.  Only identifiers may be
             * referenced; the VM emits LOAD_LOCAL_REF / LOAD_GLOBAL_REF which
             * push a VAR_REFER wrapper (ref=0) that funccall_run treats as a
             * reference argument. */
            mln_lang_spec_t *inner = n->data.spec;
            if (inner == NULL || inner->op != M_SPEC_FACTOR) { bail(c); return; }
            mln_lang_factor_t *f = inner->data.factor;
            if (f == NULL || f->type != M_FACTOR_ID) { bail(c); return; }
            int slot = find_local_slot(c, f->data.s_id);
            if (slot >= 0) {
                if (slot > 255) { bail(c); return; }
                emit(c, MLN_VOP_LOAD_LOCAL_REF, (mln_u8_t)slot, 0);
            } else {
                int idx = add_sconst(c, f->data.s_id);
                if (idx < 0 || idx > 32767) { bail(c); return; }
                emit(c, MLN_VOP_LOAD_GLOBAL_REF, 0, (mln_s16_t)idx);
            }
            sp_push(c, 1);
            return;
        }
        case M_SPEC_INC: {
            /* Prefix ++x : pre-increment.  Supports locals and globals. */
            mln_lang_spec_t *inner = n->data.spec;
            if (inner == NULL || inner->op != M_SPEC_FACTOR) { bail(c); return; }
            mln_lang_factor_t *f = inner->data.factor;
            if (f == NULL || f->type != M_FACTOR_ID) { bail(c); return; }
            int slot = find_local_slot(c, f->data.s_id);
            if (slot >= 0) {
                emit(c, MLN_VOP_INC_LOCAL_LOAD, (mln_u8_t)slot, 0);
                sp_push(c, 1);
                return;
            }
            /* Global: LOAD_GLOBAL, add 1, ASSIGN_GLOBAL (pushes back new val).
             * Return early so the outer sp_push below is not double-counted. */
            {
                int gidx = add_sconst(c, f->data.s_id);
                if (gidx < 0 || gidx > 32767) { bail(c); return; }
                emit(c, MLN_VOP_LOAD_GLOBAL, 0, (mln_s16_t)gidx);
                sp_push(c, 1);
                emit_load_int_one(c);
                if (!c->ok) return;
                emit(c, MLN_VOP_ADD, 0, 0);
                sp_pop(c, 1);
                emit(c, MLN_VOP_ASSIGN_GLOBAL, 0, (mln_s16_t)gidx);
                /* sp = S+1, result (new val) on stack — return early */
                return;
            }
        }
        case M_SPEC_DEC: {
            /* Prefix --x : pre-decrement.  Supports locals and globals. */
            mln_lang_spec_t *inner = n->data.spec;
            if (inner == NULL || inner->op != M_SPEC_FACTOR) { bail(c); return; }
            mln_lang_factor_t *f = inner->data.factor;
            if (f == NULL || f->type != M_FACTOR_ID) { bail(c); return; }
            int slot = find_local_slot(c, f->data.s_id);
            if (slot >= 0) {
                emit(c, MLN_VOP_DEC_LOCAL_LOAD, (mln_u8_t)slot, 0);
                sp_push(c, 1);
                return;
            }
            /* Global path: return early (same reason as M_SPEC_INC). */
            {
                int gidx = add_sconst(c, f->data.s_id);
                if (gidx < 0 || gidx > 32767) { bail(c); return; }
                emit(c, MLN_VOP_LOAD_GLOBAL, 0, (mln_s16_t)gidx);
                sp_push(c, 1);
                emit_load_int_one(c);
                if (!c->ok) return;
                emit(c, MLN_VOP_SUB, 0, 0);
                sp_pop(c, 1);
                emit(c, MLN_VOP_ASSIGN_GLOBAL, 0, (mln_s16_t)gidx);
                /* sp = S+1, result (new val) on stack — return early */
                return;
            }
        }
        case M_SPEC_NEW: {
            /* `$Set` — instantiate a Set as an object. The set name is in
             * spec.data.set_name. We add it to sconsts and emit
             * NEW_OBJECT. */
            mln_string_t *name = n->data.set_name;
            if (name == NULL) { bail(c); return; }
            int idx = add_sconst(c, name);
            if (idx < 0 || idx > 32767) { bail(c); return; }
            emit(c, MLN_VOP_NEW_OBJECT, 0, (mln_s16_t)idx);
            sp_push(c, 1);
            return;
        }
        default:
            bail(c);
            return;
    }
}

static void compile_factor(mln_lang_vm_compiler_t *c, mln_lang_factor_t *f)
{
    if (f == NULL) { bail(c); return; }
    switch (f->type) {
        case M_FACTOR_INT: {
            int idx = add_iconst(c, f->data.i);
            if (idx < 0 || idx > 32767) { bail(c); return; }
            emit(c, MLN_VOP_LOAD_INT, 0, (mln_s16_t)idx);
            sp_push(c, 1);
            return;
        }
        case M_FACTOR_REAL: {
            int idx = add_rconst(c, f->data.f);
            if (idx < 0 || idx > 32767) { bail(c); return; }
            emit(c, MLN_VOP_LOAD_REAL, 0, (mln_s16_t)idx);
            sp_push(c, 1);
            return;
        }
        case M_FACTOR_STRING: {
            mln_string_t *s = (mln_string_t *)f->data.s_id;
            int idx = add_sconst(c, s);
            if (idx < 0 || idx > 32767) { bail(c); return; }
            emit(c, MLN_VOP_LOAD_STRING, 0, (mln_s16_t)idx);
            sp_push(c, 1);
            return;
        }
        case M_FACTOR_BOOL:
            emit(c, f->data.b ? MLN_VOP_LOAD_TRUE : MLN_VOP_LOAD_FALSE, 0, 0);
            sp_push(c, 1);
            return;
        case M_FACTOR_NIL:
            emit(c, MLN_VOP_LOAD_NIL, 0, 0);
            sp_push(c, 1);
            return;
        case M_FACTOR_ID: {
            int slot = find_local_slot(c, f->data.s_id);
            if (slot >= 0) {
                emit(c, MLN_VOP_LOAD_LOCAL, (mln_u8_t)slot, 0);
            } else {
                /* Phase E: not a local — load by name from global symbol
                 * table at runtime. This handles cross-function calls
                 * (function-name reference) and reads of top-level vars. */
                int idx = add_sconst(c, f->data.s_id);
                if (idx < 0 || idx > 32767) { bail(c); return; }
                emit(c, MLN_VOP_LOAD_GLOBAL, 0, (mln_s16_t)idx);
            }
            sp_push(c, 1);
            return;
        }
        case M_FACTOR_ARRAY: {
            /* Phase F2: array literal `[a, b, c]` or `['k': 'v', ...]`.
             * We emit NEW_ARRAY then for each entry push key (NIL if
             * absent) and value, then ARRAY_PUT which keeps the array
             * on top of the stack. */
            emit(c, MLN_VOP_NEW_ARRAY, 0, 0);
            sp_push(c, 1);
            mln_lang_elemlist_t *el = f->data.array;
            while (c->ok && el != NULL) {
                int sp_before = c->sp;
                if (el->key != NULL) {
                    compile_assign(c, el->key);
                    if (!c->ok) return;
                    if (c->sp != sp_before + 1) { bail(c); return; }
                } else {
                    emit(c, MLN_VOP_LOAD_NIL, 0, 0);
                    sp_push(c, 1);
                }
                if (el->val != NULL) {
                    int sp_v = c->sp;
                    compile_assign(c, el->val);
                    if (!c->ok) return;
                    if (c->sp != sp_v + 1) { bail(c); return; }
                } else {
                    emit(c, MLN_VOP_LOAD_NIL, 0, 0);
                    sp_push(c, 1);
                }
                emit(c, MLN_VOP_ARRAY_PUT, 0, 0);
                sp_pop(c, 2);   /* pops key + val, array stays */
                el = el->next;
            }
            return;
        }
        default:
            bail(c);
            return;
    }
}
/* Recursively scan an AST stm chain for any user-defined function whose
 * name starts with "__int_". Such names are how Melang declares integer
 * operator overloads (e.g. __int_plus_operator__). When present, the VM
 * runtime will set ctx->op_int_flag and route int binops through the
 * methods table — so compile-time constant folding of int arithmetic
 * MUST be disabled to preserve those semantics. Returns 1 if any such
 * definition is found, 0 otherwise. */
static int scan_stm_for_int_overload(mln_lang_stm_t *stm);
static int scan_block_for_int_overload(mln_lang_block_t *blk);

static int name_is_int_overload(mln_string_t *name)
{
    if (name == NULL) return 0;
    /* "__int_" prefix is 6 characters. Any user function with that
     * prefix may be an overload (the compiler is conservative here —
     * even a stray name like "__int_helper" disables fold for the
     * containing script. Net cost: a few literals stop being folded.) */
    if (name->len < 6) return 0;
    const mln_u8ptr_t d = name->data;
    return (d[0] == '_' && d[1] == '_' &&
            d[2] == 'i' && d[3] == 'n' && d[4] == 't' && d[5] == '_');
}

static int scan_block_for_int_overload(mln_lang_block_t *blk)
{
    if (blk == NULL) return 0;
    switch (blk->type) {
        case M_BLOCK_STM:
            return scan_stm_for_int_overload(blk->data.stm);
        case M_BLOCK_IF:
            if (blk->data.i == NULL) return 0;
            if (scan_block_for_int_overload(blk->data.i->blockstm)) return 1;
            return scan_block_for_int_overload(blk->data.i->elsestm);
        default:
            return 0;
    }
}

static int scan_stm_for_int_overload(mln_lang_stm_t *stm)
{
    while (stm != NULL) {
        switch (stm->type) {
            case M_STM_FUNC:
                if (stm->data.func != NULL) {
                    if (name_is_int_overload(stm->data.func->name)) return 1;
                    /* Function bodies can contain nested function defs. */
                    if (scan_stm_for_int_overload(stm->data.func->stm)) return 1;
                }
                break;
            case M_STM_SET:
                if (stm->data.setdef != NULL) {
                    mln_lang_setstm_t *ss = stm->data.setdef->stm;
                    while (ss != NULL) {
                        if (ss->type == M_SETSTM_FUNC && ss->data.func != NULL) {
                            if (name_is_int_overload(ss->data.func->name)) return 1;
                            if (scan_stm_for_int_overload(ss->data.func->stm)) return 1;
                        }
                        ss = ss->next;
                    }
                }
                break;
            case M_STM_BLOCK:
                if (scan_block_for_int_overload(stm->data.block)) return 1;
                break;
            case M_STM_SWITCH:
                if (stm->data.sw != NULL) {
                    mln_lang_switchstm_t *sw = stm->data.sw->switchstm;
                    while (sw != NULL) {
                        if (scan_stm_for_int_overload(sw->stm)) return 1;
                        sw = sw->next;
                    }
                }
                break;
            case M_STM_WHILE:
                if (stm->data.w != NULL &&
                    scan_block_for_int_overload(stm->data.w->blockstm))
                    return 1;
                break;
            case M_STM_FOR:
                if (stm->data.f != NULL &&
                    scan_block_for_int_overload(stm->data.f->blockstm))
                    return 1;
                break;
            default:
                break;
        }
        stm = stm->next;
    }
    return 0;
}

/* ─────────────────────────────────────────────────────────────────────────
 * AST scanner: detect any Eval() call anywhere in the statement/expression
 * tree.  If found, constant folding is disabled for the enclosing
 * compilation unit as a conservative safety guard.
 *
 * Note: Melang's Eval() always runs code in a NEW, isolated context and
 * cannot inject definitions (including operator overloads) into the calling
 * context's scope.  The disable-folding guard is therefore conservative —
 * it never causes incorrect results, only a minor performance loss
 * (integer literal pairs are not pre-folded at compile time).
 *
 * The scan is conservative: any function call whose BASE identifier is
 * exactly "Eval" (the Melang built-in) triggers the disable.  False
 * positives (user functions named Eval) only cost a small performance
 * regression (no integer literal folding), not a correctness failure.
 * ─────────────────────────────────────────────────────────────────────── */
static int scan_exp_for_eval(mln_lang_exp_t *exp);
static int scan_assign_for_eval(mln_lang_assign_t *a);
static int scan_logiclow_for_eval(mln_lang_logiclow_t *ll);
static int scan_logichigh_for_eval(mln_lang_logichigh_t *lh);
static int scan_relativelow_for_eval(mln_lang_relativelow_t *rl);
static int scan_relativehigh_for_eval(mln_lang_relativehigh_t *rh);
static int scan_move_for_eval(mln_lang_move_t *mv);
static int scan_addsub_for_eval(mln_lang_addsub_t *as);
static int scan_muldiv_for_eval(mln_lang_muldiv_t *md);
static int scan_not_for_eval(mln_lang_not_t *nt);
static int scan_suffix_for_eval(mln_lang_suffix_t *sf);
static int scan_locate_for_eval(mln_lang_locate_t *loc);
static int scan_spec_for_eval(mln_lang_spec_t *sp);
static int scan_block_for_eval(mln_lang_block_t *blk);
static int scan_stm_for_eval_call(mln_lang_stm_t *stm);

static int name_is_eval(mln_string_t *name)
{
    if (name == NULL || name->len != 4) return 0;
    const mln_u8ptr_t d = name->data;
    return (d[0]=='E' && d[1]=='v' && d[2]=='a' && d[3]=='l');
}

static int scan_spec_for_eval(mln_lang_spec_t *sp)
{
    if (!sp) return 0;
    switch (sp->op) {
        case M_SPEC_PARENTH:  return scan_exp_for_eval(sp->data.exp);
        case M_SPEC_NEGATIVE:
        case M_SPEC_REVERSE:
        case M_SPEC_INC:
        case M_SPEC_DEC:
        case M_SPEC_REFER:    return scan_spec_for_eval(sp->data.spec);
        default:              return 0; /* M_SPEC_FACTOR, M_SPEC_NEW */
    }
}

static int scan_locate_for_eval(mln_lang_locate_t *loc)
{
    while (loc != NULL) {
        if (loc->op == M_LOCATE_NONE) return scan_spec_for_eval(loc->left);
        /* Direct call to Eval()? Base must be identifier "Eval". */
        if (loc->op == M_LOCATE_FUNC) {
            mln_lang_spec_t *sp = loc->left;
            if (sp && sp->op == M_SPEC_FACTOR) {
                mln_lang_factor_t *f = sp->data.factor;
                if (f && f->type == M_FACTOR_ID && name_is_eval(f->data.s_id))
                    return 1;
            }
            /* Scan argument expressions. */
            if (loc->right.exp && scan_exp_for_eval(loc->right.exp)) return 1;
        } else if (loc->op == M_LOCATE_INDEX) {
            if (loc->right.exp && scan_exp_for_eval(loc->right.exp)) return 1;
        }
        if (scan_spec_for_eval(loc->left)) return 1;
        loc = loc->next;
    }
    return 0;
}

static int scan_suffix_for_eval(mln_lang_suffix_t *sf)
{
    return sf ? scan_locate_for_eval(sf->left) : 0;
}

static int scan_not_for_eval(mln_lang_not_t *nt)
{
    if (!nt) return 0;
    return (nt->op == M_NOT_NOT) ? scan_not_for_eval(nt->right.not)
                                 : scan_suffix_for_eval(nt->right.suffix);
}

static int scan_muldiv_for_eval(mln_lang_muldiv_t *md)
{
    while (md) {
        if (scan_not_for_eval(md->left)) return 1;
        md = md->right;
    }
    return 0;
}

static int scan_addsub_for_eval(mln_lang_addsub_t *as)
{
    while (as) {
        if (scan_muldiv_for_eval(as->left)) return 1;
        as = as->right;
    }
    return 0;
}

static int scan_move_for_eval(mln_lang_move_t *mv)
{
    while (mv) {
        if (scan_addsub_for_eval(mv->left)) return 1;
        mv = mv->right;
    }
    return 0;
}

static int scan_relativehigh_for_eval(mln_lang_relativehigh_t *rh)
{
    while (rh) {
        if (scan_move_for_eval(rh->left)) return 1;
        rh = rh->right;
    }
    return 0;
}

static int scan_relativelow_for_eval(mln_lang_relativelow_t *rl)
{
    while (rl) {
        if (scan_relativehigh_for_eval(rl->left)) return 1;
        rl = rl->right;
    }
    return 0;
}

static int scan_logichigh_for_eval(mln_lang_logichigh_t *lh)
{
    while (lh) {
        if (scan_relativelow_for_eval(lh->left)) return 1;
        lh = lh->right;
    }
    return 0;
}

static int scan_logiclow_for_eval(mln_lang_logiclow_t *ll)
{
    while (ll) {
        if (scan_logichigh_for_eval(ll->left)) return 1;
        ll = ll->right;
    }
    return 0;
}

static int scan_assign_for_eval(mln_lang_assign_t *a)
{
    while (a) {
        if (scan_logiclow_for_eval(a->left)) return 1;
        a = a->right;
    }
    return 0;
}

static int scan_exp_for_eval(mln_lang_exp_t *exp)
{
    while (exp) {
        if (scan_assign_for_eval(exp->assign)) return 1;
        exp = exp->next;
    }
    return 0;
}

static int scan_block_for_eval(mln_lang_block_t *blk)
{
    if (!blk) return 0;
    switch (blk->type) {
        case M_BLOCK_EXP:
            return scan_exp_for_eval(blk->data.exp);
        case M_BLOCK_STM:
            return scan_stm_for_eval_call(blk->data.stm);
        case M_BLOCK_RETURN:
            return scan_exp_for_eval(blk->data.exp);
        case M_BLOCK_IF:
            if (!blk->data.i) return 0;
            if (scan_exp_for_eval(blk->data.i->condition)) return 1;
            if (scan_block_for_eval(blk->data.i->blockstm)) return 1;
            return scan_block_for_eval(blk->data.i->elsestm);
        default:
            return 0;
    }
}

static int scan_stm_for_eval_call(mln_lang_stm_t *stm)
{
    while (stm != NULL) {
        switch (stm->type) {
            case M_STM_BLOCK:
                if (scan_block_for_eval(stm->data.block)) return 1;
                break;
            case M_STM_FUNC:
                /* Scan nested function bodies (calling Eval inside a helper
                 * function that is invoked before the arithmetic code also
                 * changes the effective op_int_flag at top-level). */
                if (stm->data.func && scan_stm_for_eval_call(stm->data.func->stm))
                    return 1;
                break;
            case M_STM_SET:
                if (stm->data.setdef != NULL) {
                    mln_lang_setstm_t *ss = stm->data.setdef->stm;
                    while (ss != NULL) {
                        if (ss->type == M_SETSTM_FUNC && ss->data.func != NULL)
                            if (scan_stm_for_eval_call(ss->data.func->stm)) return 1;
                        ss = ss->next;
                    }
                }
                break;
            case M_STM_WHILE:
                if (stm->data.w != NULL) {
                    if (scan_exp_for_eval(stm->data.w->condition)) return 1;
                    if (scan_block_for_eval(stm->data.w->blockstm)) return 1;
                }
                break;
            case M_STM_FOR:
                if (stm->data.f != NULL) {
                    if (scan_exp_for_eval(stm->data.f->init_exp)) return 1;
                    if (scan_exp_for_eval(stm->data.f->condition)) return 1;
                    if (scan_exp_for_eval(stm->data.f->mod_exp)) return 1;
                    if (scan_block_for_eval(stm->data.f->blockstm)) return 1;
                }
                break;
            case M_STM_SWITCH:
                if (stm->data.sw != NULL) {
                    if (scan_exp_for_eval(stm->data.sw->condition)) return 1;
                    mln_lang_switchstm_t *sw = stm->data.sw->switchstm;
                    while (sw != NULL) {
                        if (scan_stm_for_eval_call(sw->stm)) return 1;
                        sw = sw->next;
                    }
                }
                break;
            default:
                break;
        }
        stm = stm->next;
    }
    return 0;
}

int
mln_lang_vm_try_compile(mln_lang_ctx_t *ctx, mln_lang_func_detail_t *prototype)
{
    if (prototype == NULL) return -1;
    if (prototype->type != M_FUNC_EXTERNAL) return -1;
    if (prototype->data.stm == NULL) return -1;

    mln_size_t n_args = mln_array_nelts(&(prototype->args));
    if (n_args > MLN_VM_MAX_LOCALS) return -1;

    mln_lang_vm_compiler_t c;
    memset(&c, 0, sizeof(c));
    c.labels = c.labels_buf;
    c.labels_cap = LABELS_INLINE_CAP;
    c.goto_patches = c.goto_patches_buf;
    c.goto_patches_cap = GOTOS_INLINE_CAP;
    c.ctx       = ctx;
    c.prototype = prototype;
    c.ok        = 1;
    c.prev_property_name_idx = -1;

    /* Constant folding is safe iff:
     *   (a) the script does NOT define any __int_*_operator__ overload
     *       in its AST (static check), AND
     *   (b) ctx->op_int_flag is currently 0 (no overload was injected
     *       previously via Eval into the same ctx), AND
     *   (c) the function body does NOT call Eval() anywhere (conservative:
     *       Eval could define an overload at runtime after this chunk is
     *       compiled, making any folded constants incorrect).
     * Any condition being violated means this prototype's int binops
     * may legitimately need to dispatch through the methods table at
     * runtime; folding would silently bypass that path. */
    c.safe_to_fold = !ctx->op_int_flag &&
                     !scan_stm_for_int_overload(prototype->data.stm) &&
                     !scan_stm_for_eval_call(prototype->data.stm);

    c.chunk = mln_lang_vm_chunk_new(ctx->pool);
    if (c.chunk == NULL) return 0;

    /* Snapshot arg names into local_names[0..n_args-1]. */
    for (mln_size_t i = 0; i < n_args; ++i) {
        mln_lang_var_t *v = ((mln_lang_var_t **)mln_array_elts(&(prototype->args)))[i];
        c.local_names[i] = v->name;
    }
    c.n_args   = n_args;
    c.n_locals = n_args;
    /* Phase F: closure-captured names sit immediately after args in the
     * scope's symbol chain (funccall_run binds them in that order — see
     * lines 6072-6080 of mln_lang.c). Map them to slots so identifier
     * references inside the body resolve to them. */
    mln_size_t n_closures = mln_array_nelts(&(prototype->closure));
    if (n_args + n_closures > MLN_VM_MAX_LOCALS) {
        mln_lang_vm_chunk_free(c.chunk);
        return -1;
    }
    for (mln_size_t i = 0; i < n_closures; ++i) {
        mln_lang_var_t *v = ((mln_lang_var_t **)mln_array_elts(&(prototype->closure)))[i];
        c.local_names[n_args + i] = v->name;
    }
    c.n_locals = n_args + n_closures;

    compile_stm_chain(&c, prototype->data.stm);

    /* Resolve forward goto patches against the labels table. */
    if (c.ok) {
        for (int gi = 0; gi < c.n_goto_patches; ++gi) {
            mln_string_t *want = c.goto_patches[gi].name;
            int patch_pc = c.goto_patches[gi].patch_pc;
            int target = -1;
            for (int li = 0; li < c.n_labels; ++li) {
                if (c.labels[li].name != NULL &&
                    c.labels[li].name->len == want->len &&
                    !memcmp(c.labels[li].name->data, want->data, want->len))
                {
                    target = c.labels[li].pc; break;
                }
            }
            if (target < 0) {
                /* Undefined label. */
                c.ok = 0; break;
            }
            patch_jump(&c, patch_pc, target);
            if (!c.ok) break;
        }
    }

    if (c.ok) emit(&c, MLN_VOP_RETURN_NIL, 0, 0);

    /* Medium-tier perf: run the peephole / superinstruction pass after
     * all jump patches have been resolved. Operates on the final code
     * stream and rewrites any remaining jump offsets. Disabled via
     * MELANG_VM_NO_PEEPHOLE for diff-testing. */
    if (c.ok && getenv("MELANG_VM_NO_PEEPHOLE") == NULL) {
        run_peephole(&c);
    }

    if (!c.ok) {
        mln_lang_vm_chunk_free(c.chunk);
        return -1;
    }
    c.chunk->n_locals = c.n_locals;
    c.chunk->max_stack = (mln_size_t)(c.max_sp > 0 ? c.max_sp : 1);
    /* Persist local_names so vm_run can bind body-locals to the symbol
     * table (needed for closure capture from VM-introduced locals). */
    if (c.n_locals > 0) {
        c.chunk->local_names = (mln_string_t **)mln_alloc_m(ctx->pool,
                                  sizeof(mln_string_t *) * c.n_locals);
        if (c.chunk->local_names == NULL) {
            mln_lang_vm_chunk_free(c.chunk);
            return 0;
        }
        for (mln_size_t i = 0; i < c.n_locals; ++i) {
            c.chunk->local_names[i] = c.local_names[i];
        }
    }
    /* Stash n_args in iconsts_cap... actually we need a dedicated field.
     * Use the n_locals field for total locals; remember n_args separately
     * via the prototype's args array length (still accessible at runtime). */

    /* Medium-tier perf: allocate per-pc inline cache slots, one per insn.
     * Lazily populated by IC-using opcodes (GET_PROPERTY, SET_PROPERTY,
     * INVOKE_METHOD, CALL_GLOBAL) at runtime. Allocation up-front avoids
     * any allocation on the hot path. The array is zero-initialized so
     * cached_kind == 0 means "empty" / fall-through to slow path. */
    if (c.chunk->code_len > 0) {
        c.chunk->ic_slots = (mln_lang_vm_ic_t *)mln_alloc_m(ctx->pool,
                              sizeof(mln_lang_vm_ic_t) * c.chunk->code_len);
        if (c.chunk->ic_slots == NULL) {
            mln_lang_vm_chunk_free(c.chunk);
            return 0;
        }
        memset(c.chunk->ic_slots, 0, sizeof(mln_lang_vm_ic_t) * c.chunk->code_len);
        c.chunk->ic_slots_len = c.chunk->code_len;
    }

    prototype->vm_chunk = c.chunk;
    /* Record the op_int_flag value at compile time.  If it changes later
     * (e.g. Eval injects an overload), the cached chunk will be invalidated
     * and recompiled with safe_to_fold=0 on the next call. */
    prototype->vm_op_int_flag = ctx->op_int_flag;
    if (mln_lang_vm_env_is_active("MELANG_VM_TRACE")) {
        fprintf(stderr, "[vm] compiled chunk: insns=%zu locals=%zu max_stack=%zu\n",
                (size_t)c.chunk->code_len, (size_t)c.n_locals,
                (size_t)c.chunk->max_stack);
    }
    return 1;
}

/* ====================================================================
 * VM runtime.
 * ==================================================================== */

/* ====================================================================
 * Tagged-value operand stack (high-cost-tier perf optimization).
 *
 * Background. Until this change the operand stack stored
 * mln_lang_var_t* pointers and every value pushed onto it had to be a
 * heap-allocated var (with a heap-allocated val inside it). For an
 * arithmetic-heavy script like fib that meant every LOAD_INT, every
 * arithmetic result, and every comparison result allocated a fresh
 * var+val from the freelist, the next opcode consumed it, dec-ref'd
 * it, and shipped it back to the freelist. Even with the freelist
 * the round-trip is measurable in profiles.
 *
 * Design. The opstack is now a flat array of mln_lang_value_t (16
 * bytes each on a 64-bit host). Each slot is one of:
 *
 *   - MLN_VAL_NIL / MLN_VAL_BOOL / MLN_VAL_INT / MLN_VAL_REAL : the
 *     scalar lives directly in the slot, no heap allocation.
 *   - MLN_VAL_VAR : the slot holds a mln_lang_var_t* and one of two
 *     ownership states:
 *       * borrow == 0 — the slot owns one ref on the var (dec-ref on
 *         release).
 *       * borrow == 1 — the slot does NOT own a ref; some other
 *         long-lived owner (e.g. frame->slots[k] which is itself
 *         held by the symbol table) keeps the var alive while this
 *         slot is on the stack. Releasing a borrow slot is a no-op.
 *
 * The borrow state collapses two previously separate optimizations
 * into one mechanism:
 *
 *   1. LOAD_LOCAL no longer needs to ++ref the slot var; it just
 *      pushes a borrow.
 *   2. DUP no longer needs to ++ref the var on top; it just copies
 *      the slot and stamps borrow=1 onto the copy.
 *
 * Consumers that need to take ownership (e.g. apply_binop wants to
 * call mln_lang_var_free on its inputs; funccall_val_add_arg expects
 * an owned var) call value_take_var(), which boxes scalars on demand
 * and bumps ref-count on borrow slots.
 *
 * Cross-platform notes. The struct deliberately uses plain enum +
 * separate uint8_t fields rather than NaN-boxing. NaN-boxing into 8
 * bytes saves cache space but requires assumptions about pointer
 * canonicalization that don't hold uniformly on aarch64 with 52-bit
 * VA support, on Windows with MTE-style tagging, or on toolchains
 * that fold/normalize signaling NaN payloads (some MSVC /fp:fast
 * configurations). The plain tag+pad form is portable C99.
 *
 * Pool / lifetime. value_take_var allocates via the existing
 * var/val freelists (mln_lang_var_create_int / _bool / _real / _nil),
 * so the freelist still amortizes the boxing cost in mixed-type code.
 * value_release dec-refs an owned var via mln_lang_var_free, which
 * also recycles into the freelist.
 *
 * apply_binop is unchanged — it still takes/consumes mln_lang_var_t*.
 * Hot int+int arithmetic now bypasses apply_binop entirely from
 * dispatch_one; the slow path materializes both operands and calls
 * apply_binop the way it always did. */

typedef enum {
    MLN_VAL_NIL  = 0,
    MLN_VAL_BOOL = 1,
    MLN_VAL_INT  = 2,
    MLN_VAL_REAL = 3,
    MLN_VAL_VAR  = 4
} mln_lang_value_kind_t;

typedef struct {
    union {
        mln_s64_t       i;     /* MLN_VAL_INT, MLN_VAL_BOOL (0/1) */
        double          f;     /* MLN_VAL_REAL */
        mln_lang_var_t *var;   /* MLN_VAL_VAR */
    } u;
    mln_u8_t kind;            /* mln_lang_value_kind_t */
    mln_u8_t borrow;          /* 1 if MLN_VAL_VAR slot does NOT own a ref */
    mln_u8_t pad[6];
} mln_lang_value_t;

/* ---- value constructors ---- */

static inline mln_lang_value_t value_nil(void)
{
    mln_lang_value_t v;
    v.u.i = 0; v.kind = MLN_VAL_NIL; v.borrow = 0;
    v.pad[0] = v.pad[1] = v.pad[2] = v.pad[3] = v.pad[4] = v.pad[5] = 0;
    return v;
}
static inline mln_lang_value_t value_int(mln_s64_t x)
{
    mln_lang_value_t v;
    v.u.i = x; v.kind = MLN_VAL_INT; v.borrow = 0;
    v.pad[0] = v.pad[1] = v.pad[2] = v.pad[3] = v.pad[4] = v.pad[5] = 0;
    return v;
}
static inline mln_lang_value_t value_bool(int b)
{
    mln_lang_value_t v;
    v.u.i = b ? 1 : 0; v.kind = MLN_VAL_BOOL; v.borrow = 0;
    v.pad[0] = v.pad[1] = v.pad[2] = v.pad[3] = v.pad[4] = v.pad[5] = 0;
    return v;
}
static inline mln_lang_value_t value_real(double x)
{
    mln_lang_value_t v;
    v.u.f = x; v.kind = MLN_VAL_REAL; v.borrow = 0;
    v.pad[0] = v.pad[1] = v.pad[2] = v.pad[3] = v.pad[4] = v.pad[5] = 0;
    return v;
}
static inline mln_lang_value_t value_var_owned(mln_lang_var_t *p)
{
    mln_lang_value_t v;
    v.u.var = p; v.kind = MLN_VAL_VAR; v.borrow = 0;
    v.pad[0] = v.pad[1] = v.pad[2] = v.pad[3] = v.pad[4] = v.pad[5] = 0;
    return v;
}
static inline mln_lang_value_t value_var_borrow(mln_lang_var_t *p)
{
    mln_lang_value_t v;
    v.u.var = p; v.kind = MLN_VAL_VAR; v.borrow = 1;
    v.pad[0] = v.pad[1] = v.pad[2] = v.pad[3] = v.pad[4] = v.pad[5] = 0;
    return v;
}

/* Drop a slot value as if it were popped and discarded. */
static inline void value_release(mln_lang_value_t *v)
{
    if (v->kind == MLN_VAL_VAR && !v->borrow && v->u.var != NULL) {
        mln_lang_var_free(v->u.var);
    }
    v->kind   = MLN_VAL_NIL;
    v->borrow = 0;
    v->u.var  = NULL;
}

/* Forward decl: var-creation helpers used by value_take_var. */
extern mln_lang_var_t *mln_lang_var_create_nil(mln_lang_ctx_t *ctx, mln_string_t *name);
extern mln_lang_var_t *mln_lang_var_create_int(mln_lang_ctx_t *ctx, mln_s64_t i, mln_string_t *name);
extern mln_lang_var_t *mln_lang_var_create_bool(mln_lang_ctx_t *ctx, mln_u8_t b, mln_string_t *name);
extern mln_lang_var_t *mln_lang_var_create_real(mln_lang_ctx_t *ctx, double r, mln_string_t *name);

/* Convert a value into a freshly-owned mln_lang_var_t*. After this
 * the input slot is left in a released state (kind=NIL, borrow=0).
 * Returns NULL on OOM. */
static inline mln_lang_var_t *value_take_var(mln_lang_ctx_t *ctx, mln_lang_value_t *v)
{
    mln_lang_var_t *r;
    switch (v->kind) {
        case MLN_VAL_VAR:
            r = v->u.var;
            if (v->borrow) {
                /* Caller wants ownership; promote borrow → owned. */
                if (r != NULL) ++(r->ref);
            }
            v->kind = MLN_VAL_NIL; v->borrow = 0; v->u.var = NULL;
            return r;
        case MLN_VAL_NIL:
            r = mln_lang_var_create_nil(ctx, NULL);
            break;
        case MLN_VAL_BOOL:
            r = mln_lang_var_create_bool(ctx, (mln_u8_t)(v->u.i ? 1 : 0), NULL);
            break;
        case MLN_VAL_INT:
            r = mln_lang_var_create_int(ctx, v->u.i, NULL);
            break;
        case MLN_VAL_REAL:
            r = mln_lang_var_create_real(ctx, v->u.f, NULL);
            break;
        default:
            r = NULL;
            break;
    }
    v->kind = MLN_VAL_NIL; v->borrow = 0; v->u.var = NULL;
    return r;
}

/* Read a value's truthiness without materializing it. Mirrors
 * mln_lang_condition_is_true for boxed variants. */
static inline int value_is_truthy(const mln_lang_value_t *v)
{
    switch (v->kind) {
        case MLN_VAL_NIL:  return 0;
        case MLN_VAL_BOOL: return v->u.i ? 1 : 0;
        case MLN_VAL_INT:  return v->u.i != 0;
        case MLN_VAL_REAL: return v->u.f != 0.0;
        case MLN_VAL_VAR:
            return mln_lang_condition_is_true(v->u.var);
        default:
            return 0;
    }
    /* unreachable */
}

/* For opcodes that need a peek-only view of an existing VAR slot
 * (e.g. ARRAY_PUT peeking at the array on top of the stack). Returns
 * NULL if the slot is not a VAR (in which case the opcode should
 * raise a type error). */
static inline mln_lang_var_t *value_peek_var(const mln_lang_value_t *v)
{
    return (v->kind == MLN_VAL_VAR) ? v->u.var : NULL;
}

/* Stamp an existing slot as a borrow without touching ref-count.
 * Used by DUP and similar opcodes. */
static inline void value_make_borrow(mln_lang_value_t *v)
{
    if (v->kind == MLN_VAL_VAR) v->borrow = 1;
}

/* apply_binop runs from one call site (the binop block in dispatch_one)
 * but its body is large enough that the compiler may not auto-inline.
 * The hot attribute biases the compiler toward keeping it warm in
 * i-cache and aligning its entry. */
#if defined(__GNUC__) || defined(__clang__)
__attribute__((hot))
#endif
static mln_lang_var_t *apply_binop(mln_lang_ctx_t *ctx, mln_u8_t op,
                                    mln_lang_var_t *a, mln_lang_var_t *b)
{
    if (a->val != NULL && b->val != NULL &&
        a->val->type == M_LANG_VAL_TYPE_INT &&
        b->val->type == M_LANG_VAL_TYPE_INT &&
        !ctx->op_int_flag)
    {
        mln_s64_t ai = a->val->data.i;
        mln_s64_t bi = b->val->data.i;
        mln_lang_var_t *r = NULL;
        switch (op) {
            case MLN_VOP_ADD: r = mln_lang_var_create_int(ctx, ai + bi, NULL); break;
            case MLN_VOP_SUB: r = mln_lang_var_create_int(ctx, ai - bi, NULL); break;
            case MLN_VOP_MUL: r = mln_lang_var_create_int(ctx, ai * bi, NULL); break;
            case MLN_VOP_DIV:
                if (bi == 0) { mln_lang_errmsg(ctx, "Division by zero."); goto done; }
                r = mln_lang_var_create_int(ctx, ai / bi, NULL); break;
            case MLN_VOP_MOD:
                if (bi == 0) { mln_lang_errmsg(ctx, "Modulo by zero."); goto done; }
                r = mln_lang_var_create_int(ctx, ai % bi, NULL); break;
            case MLN_VOP_LT: r = mln_lang_var_create_bool(ctx, (mln_u8_t)(ai <  bi), NULL); break;
            case MLN_VOP_LE: r = mln_lang_var_create_bool(ctx, (mln_u8_t)(ai <= bi), NULL); break;
            case MLN_VOP_GT: r = mln_lang_var_create_bool(ctx, (mln_u8_t)(ai >  bi), NULL); break;
            case MLN_VOP_GE: r = mln_lang_var_create_bool(ctx, (mln_u8_t)(ai >= bi), NULL); break;
            case MLN_VOP_EQ: r = mln_lang_var_create_bool(ctx, (mln_u8_t)(ai == bi), NULL); break;
            case MLN_VOP_NE: r = mln_lang_var_create_bool(ctx, (mln_u8_t)(ai != bi), NULL); break;
            case MLN_VOP_BOR:    r = mln_lang_var_create_int(ctx, ai | bi, NULL); break;
            case MLN_VOP_BAND:   r = mln_lang_var_create_int(ctx, ai & bi, NULL); break;
            case MLN_VOP_BXOR:   r = mln_lang_var_create_int(ctx, ai ^ bi, NULL); break;
            case MLN_VOP_LSHIFT: r = mln_lang_var_create_int(ctx, ai << bi, NULL); break;
            case MLN_VOP_RSHIFT: r = mln_lang_var_create_int(ctx, ai >> bi, NULL); break;
            default: break;
        }
done:
        mln_lang_var_free(a);
        mln_lang_var_free(b);
        return r;
    }
    mln_lang_method_t *method = mln_lang_methods[a->val->type];
    mln_lang_op handler = NULL;
    if (method != NULL) {
        switch (op) {
            case MLN_VOP_ADD: handler = method->plus_handler;     break;
            case MLN_VOP_SUB: handler = method->sub_handler;      break;
            case MLN_VOP_MUL: handler = method->mul_handler;      break;
            case MLN_VOP_DIV: handler = method->div_handler;      break;
            case MLN_VOP_MOD: handler = method->mod_handler;      break;
            case MLN_VOP_LT:  handler = method->less_handler;     break;
            case MLN_VOP_LE:  handler = method->lesseq_handler;   break;
            case MLN_VOP_GT:  handler = method->grea_handler;     break;
            case MLN_VOP_GE:  handler = method->greale_handler;   break;
            case MLN_VOP_EQ:  handler = method->equal_handler;    break;
            case MLN_VOP_NE:  handler = method->nonequal_handler; break;
            case MLN_VOP_BOR:    handler = method->cor_handler;   break;
            case MLN_VOP_BAND:   handler = method->cand_handler;  break;
            case MLN_VOP_BXOR:   handler = method->cxor_handler;  break;
            case MLN_VOP_LSHIFT: handler = method->lmov_handler;  break;
            case MLN_VOP_RSHIFT: handler = method->rmov_handler;  break;
            default: break;
        }
    }
    mln_lang_var_t *r = NULL;
    if (handler == NULL) {
        mln_lang_errmsg(ctx, "Operation NOT support.");
        mln_lang_var_free(a);
        mln_lang_var_free(b);
        return NULL;
    }
    if (handler(ctx, &r, a, b) < 0) {
        mln_lang_var_free(a);
        mln_lang_var_free(b);
        return NULL;
    }
    mln_lang_var_free(a);
    mln_lang_var_free(b);
    return r;
}

/* Create a nil variable named `name` in the current scope and return it.
 * Returns NULL and sets an error message on OOM.
 * Used by LOAD_GLOBAL and ASSIGN_GLOBAL to lazily create variables the
 * first time they are referenced, matching the AST interpreter's semantics
 * (src/mln_lang.c:6517-6536). */
static mln_lang_var_t *vm_create_scope_var(mln_lang_ctx_t *ctx, mln_string_t *name)
{
    mln_lang_var_t *nv = mln_lang_var_create_nil(ctx, name);
    if (nv == NULL) {
        mln_lang_errmsg(ctx, "No memory.");
        return NULL;
    }
    if (mln_lang_symbol_node_join(ctx, M_LANG_SYMBOL_VAR, nv) < 0) {
        mln_lang_errmsg(ctx, "No memory.");
        mln_lang_var_free(nv);
        return NULL;
    }
    return nv;
}

/* For STORE/ASSIGN: replace slot[i]'s value with the popped var's value.
 * Returns 0 on success, -1 on error. The popped var is consumed (freed). */
static int slot_assign(mln_lang_ctx_t *ctx, mln_lang_var_t *slot_var,
                        mln_lang_var_t *src)
{
    int rc = mln_lang_var_value_set(ctx, slot_var, src);
    mln_lang_var_free(src);
    return rc;
}

/* ====================================================================
 * Phase F3: heap-allocated VM frame stack (yieldable execution).
 *
 * The VM is iterative — every opcode runs against ctx->vm_frame_top.
 * Function calls push a new frame onto the chain; RETURN pops the top
 * frame and pushes the return value onto the previous frame's operand
 * stack. mln_lang_vm_step yields back to the run loop after a budget
 * of opcodes so multiple ctxs can time-share the event loop (Melang's
 * coroutine model). mln_lang_vm_run wraps the same dispatch in a
 * synchronous loop bounded to the lifetime of the frame it pushes —
 * used by callers (Watch trigger, library re-entries) that need a
 * "run this prototype to completion" call.
 * ==================================================================== */

typedef struct mln_lang_vm_frame_s {
    mln_lang_vm_chunk_t        *chunk;
    mln_size_t                  pc;
    mln_lang_value_t           *opstack;     /* tagged-value operand stack */
    int                         op_sp;
    int                         op_cap;
    mln_lang_var_t            **slots;
    int                         n_locals;
    int                         slots_cap;   /* allocated capacity of slots[] (>= n_locals) */
    int                         n_bound;     /* args + closures */
    mln_lang_func_detail_t     *prototype;   /* for CALL_SELF; NULL for top-level */
    int                         discard_ret;      /* 1 = drop return val on pop */
    int                         owns_top;         /* 1 = top-level synthetic, free chunk + proto */
    int                         awaiting_return;  /* 1 = INTERNAL call suspended via
                                                   *   mln_lang_ctx_suspend(); scope-pop and
                                                   *   ctx->ret_var capture are deferred until
                                                   *   the async completion handler calls
                                                   *   mln_lang_ctx_continue() and resumes us */
    /* DUAL-PURPOSE: when the frame is on the active vm_frame_top chain
     * (the call stack), prev points to the enclosing frame.  When the
     * frame is recycled onto ctx->vm_frame_freelist, prev is reused as
     * the freelist link.  A frame is never on both chains simultaneously. */
    struct mln_lang_vm_frame_s *prev;
} mln_lang_vm_frame_t;

#define FRAME_TOP(ctx)  ((mln_lang_vm_frame_t *)((ctx)->vm_frame_top))

static int vm_frame_grow_opstack(mln_lang_ctx_t *ctx, mln_lang_vm_frame_t *f, int need)
{
    if (f->op_sp + need <= f->op_cap) return 0;
    int new_cap = f->op_cap == 0 ? 8 : f->op_cap * 2;
    while (new_cap < f->op_sp + need) new_cap *= 2;
    mln_lang_value_t *nbuf = (mln_lang_value_t *)mln_alloc_m(ctx->pool, sizeof(mln_lang_value_t) * new_cap);
    if (nbuf == NULL) return -1;
    if (f->op_sp > 0 && f->opstack != NULL) {
        memcpy(nbuf, f->opstack, sizeof(mln_lang_value_t) * f->op_sp);
    }
    if (f->opstack != NULL) mln_alloc_free(f->opstack);
    f->opstack = nbuf;
    f->op_cap = new_cap;
    return 0;
}

/* Push a fresh frame onto ctx->vm_frame_top.
 *  - Caller has ALREADY called scope_push (via funccall_run_compat or
 *    ctx_new for the top level) so scope_top->sym_head holds the
 *    args/closures we'll bind to slots.
 *  - If owns_top is 1, vm_pop_frame_with_ret will free the chunk and
 *    the heap-allocated prototype on this frame's pop (top-level case).
 *  - If discard_ret is 1, the eventual return value of this frame is
 *    freed instead of pushed to the caller (used for Watch callbacks). */
static int vm_push_frame(mln_lang_ctx_t *ctx,
                          mln_lang_vm_chunk_t *chunk,
                          mln_lang_func_detail_t *prototype,
                          int n_args, int n_closures,
                          int owns_top, int discard_ret)
{
    if (chunk == NULL) return -1;

    int new_op_cap   = (int)(chunk->max_stack + 4);
    int new_n_locals = (int)chunk->n_locals;

    /* Try the per-ctx vm_frame freelist before hitting the pool allocator.
     * A recycled frame keeps its opstack and slots buffers; we only
     * re-allocate them when the cached capacity is insufficient. */
    mln_lang_vm_frame_t *f = NULL;
    if (ctx->vm_frame_freelist != NULL) {
        f = (mln_lang_vm_frame_t *)ctx->vm_frame_freelist;
        ctx->vm_frame_freelist = f->prev;   /* prev is the freelist link */
        --(ctx->vm_frame_freelist_count);
        /* Save buffer pointers / capacities before zeroing control fields. */
        mln_lang_value_t  *saved_opstack   = f->opstack;
        int                saved_op_cap    = f->op_cap;
        mln_lang_var_t   **saved_slots     = f->slots;
        int                saved_slots_cap = f->slots_cap;
        memset(f, 0, sizeof(*f));
        f->opstack    = saved_opstack;
        f->op_cap     = saved_op_cap;
        f->slots      = saved_slots;
        f->slots_cap  = saved_slots_cap;
        /* Grow opstack if the new call needs more stack depth. */
        if (f->op_cap < new_op_cap) {
            if (f->opstack) mln_alloc_free(f->opstack);
            f->opstack = (mln_lang_value_t *)mln_alloc_m(ctx->pool,
                             sizeof(mln_lang_value_t) * new_op_cap);
            if (f->opstack == NULL) {
                /* Slots buffer is still valid; free it before the frame. */
                if (f->slots) mln_alloc_free(f->slots);
                mln_alloc_free(f);
                return -1;
            }
            f->op_cap = new_op_cap;
        }
        /* Grow slots if the new call has more locals. */
        if (f->slots_cap < new_n_locals) {
            if (f->slots) mln_alloc_free(f->slots);
            f->slots = (mln_lang_var_t **)mln_alloc_m(ctx->pool,
                            sizeof(mln_lang_var_t *) * new_n_locals);
            if (f->slots == NULL) {
                mln_alloc_free(f->opstack);
                mln_alloc_free(f);
                return -1;
            }
            f->slots_cap = new_n_locals;
        }
    } else {
        f = (mln_lang_vm_frame_t *)mln_alloc_m(ctx->pool, sizeof(*f));
        if (f == NULL) return -1;
        memset(f, 0, sizeof(*f));
        if (new_op_cap > 0) {
            f->opstack = (mln_lang_value_t *)mln_alloc_m(ctx->pool,
                             sizeof(mln_lang_value_t) * new_op_cap);
            if (f->opstack == NULL) goto fail;
            f->op_cap = new_op_cap;
        }
        if (new_n_locals > 0) {
            f->slots = (mln_lang_var_t **)mln_alloc_m(ctx->pool,
                            sizeof(mln_lang_var_t *) * new_n_locals);
            if (f->slots == NULL) goto fail;
            f->slots_cap = new_n_locals;
        }
    }

    f->chunk     = chunk;
    f->prototype = prototype;
    f->n_locals  = new_n_locals;
    f->n_bound   = n_args + n_closures;
    f->discard_ret = discard_ret;
    f->owns_top    = owns_top;

    /* Bind args + closures from scope sym chain; create body locals. */
    mln_lang_scope_t *scope = ctx->scope_top;
    if (scope == NULL || scope->type != M_LANG_SCOPE_TYPE_FUNC) goto fail;
    mln_lang_symbol_node_t *sn = scope->sym_head;
    int i = 0;
    while (sn != NULL && i < f->n_bound) {
        if (sn->type != M_LANG_SYMBOL_VAR) goto fail;
        f->slots[i++] = sn->data.var;
        sn = sn->scope_next;
    }
    if (i != f->n_bound) goto fail;
    for (; i < f->n_locals; ++i) {
        mln_string_t *lname = (chunk->local_names != NULL) ? chunk->local_names[i] : NULL;
        mln_lang_var_t *nv = mln_lang_var_create_nil(ctx, lname);
        if (nv == NULL) goto fail_body_locals;
        if (lname != NULL) {
            if (mln_lang_symbol_node_join(ctx, M_LANG_SYMBOL_VAR, nv) < 0) {
                mln_lang_var_free(nv);
                goto fail_body_locals;
            }
            ++(nv->ref);
        } else {
            ++(nv->ref);
        }
        f->slots[i] = nv;
    }

    f->prev = FRAME_TOP(ctx);
    ctx->vm_frame_top = f;
    return 0;

fail_body_locals:
    /* Some body-locals (slots[n_bound..i-1]) were already inserted into the
     * function scope.  Pop the entire scope so the symbol table is left
     * clean before we free the frame buffers. */
    mln_lang_withdraw_until_func_compat(ctx);
    /* fall through */
fail:
    if (f->opstack) mln_alloc_free(f->opstack);
    if (f->slots) mln_alloc_free(f->slots);
    mln_alloc_free(f);
    return -1;
}

/* Pop FRAME_TOP, deliver `ret` per discard_ret / prev rules.
 * On owns_top frames, also frees the chunk and the heap prototype. */
static int vm_pop_frame_with_ret(mln_lang_ctx_t *ctx, mln_lang_var_t *ret)
{
    mln_lang_vm_frame_t *f = FRAME_TOP(ctx);
    if (f == NULL) {
        if (ret) mln_lang_var_free(ret);
        return -1;
    }

    /* For non-top-level frames, pop the function scope that
     * funccall_run_compat pushed for this call. The top-level frame's
     * scope was set up in mln_lang_ctx_new and persists for the ctx
     * lifetime; don't pop it. */
    if (!f->owns_top) {
        if (mln_lang_withdraw_until_func_compat(ctx) < 0) {
            /* tolerate — best-effort cleanup */
        }
    }

    /* Drain residual opstack values. */
    while (f->op_sp > 0) {
        mln_lang_value_t *vs = &f->opstack[--f->op_sp];
        value_release(vs);
    }
    /* Free body-locals (slots beyond n_bound). The first n_bound slots
     * are owned by the symbol table that was just popped (or persists
     * for top-level). */
    for (int j = f->n_bound; j < f->n_locals; ++j) {
        if (f->slots[j] != NULL) mln_lang_var_free(f->slots[j]);
    }

    int discard = f->discard_ret;
    int owns = f->owns_top;
    mln_lang_vm_chunk_t *chunk_to_free = owns ? f->chunk : NULL;
    mln_lang_func_detail_t *proto_to_free = owns ? f->prototype : NULL;
    mln_lang_vm_frame_t *prev = f->prev;

    /* Recycle the frame to the freelist when under the cap.  Frames keep
     * their opstack and slots allocations so that the next push can reuse
     * them without calling mln_alloc_m for the inner arrays. */
    if (ctx->vm_frame_freelist_count < M_LANG_FRAME_FREELIST_MAX) {
        f->prev = (mln_lang_vm_frame_t *)ctx->vm_frame_freelist;
        ctx->vm_frame_freelist = f;
        ++(ctx->vm_frame_freelist_count);
    } else {
        if (f->opstack) mln_alloc_free(f->opstack);
        if (f->slots)   mln_alloc_free(f->slots);
        mln_alloc_free(f);
    }
    ctx->vm_frame_top = prev;

    if (chunk_to_free) mln_lang_vm_chunk_free(chunk_to_free);
    if (proto_to_free) mln_alloc_free(proto_to_free);

    if (ret == NULL) {
        ret = mln_lang_var_create_nil(ctx, NULL);
        if (ret == NULL) return -1;
    }
    if (discard) {
        mln_lang_var_free(ret);
    } else if (prev != NULL) {
        if (vm_frame_grow_opstack(ctx, prev, 1) < 0) {
            mln_lang_var_free(ret);
            return -1;
        }
        prev->opstack[prev->op_sp++] = value_var_owned(ret);
    } else {
        mln_lang_ctx_set_ret_var(ctx, ret);
    }
    return 0;
}

/* Watcher trigger: builds a funccall_val_t for tval->func, dispatches
 * via funccall_run_compat (which pushes the function scope and binds
 * args). For compiled prototypes, our run_handler hook in mln_lang.c
 * pushes a VM frame and returns; we mark that fresh frame as
 * discard_ret so its eventual RETURN drops the value. For INTERNAL
 * prototypes (non-VM), funccall_run_compat runs synchronously and
 * sets ctx->ret_var; we discard that here.
 *
 * The "watcher frame" runs in subsequent vm_step iterations; when its
 * RETURN fires, control returns to the original frame (which has
 * already advanced pc past the assignment). */
static int vm_fire_watcher(mln_lang_ctx_t *ctx, mln_lang_var_t *target_var);

/* Watch elision: hot-path fast check.
 *
 * vm_fire_watcher is called from every store/property-set/index-set
 * opcode. The vast majority of variables in real scripts are never
 * Watched, so the function-call overhead on the no-watcher path is
 * pure waste. This macro tests the same condition vm_fire_watcher
 * uses for its early-return (val->func != NULL) and lets the caller
 * skip the call entirely in the common case. The branch is heavily
 * weighted toward "no watcher", so a hint helps prediction.
 *
 * The macro evaluates v exactly once. */
#if defined(__GNUC__) || defined(__clang__)
# define MLN_VM_LIKELY(x)   __builtin_expect(!!(x), 1)
# define MLN_VM_UNLIKELY(x) __builtin_expect(!!(x), 0)
#else
# define MLN_VM_LIKELY(x)   (x)
# define MLN_VM_UNLIKELY(x) (x)
#endif

#define VM_HAS_WATCHER(v) \
    MLN_VM_UNLIKELY((v) != NULL && (v)->val != NULL && (v)->val->func != NULL)

/* Operand-stack macros over the tagged-value array.
 *
 *   VPUSH(val)        — push a fully-built mln_lang_value_t.
 *   VPUSH_VAR(p)      — push an owned mln_lang_var_t* (slot owns ref).
 *   VPUSH_BORROW(p)   — push a borrow of a mln_lang_var_t* (slot does
 *                       NOT own a ref; some other long-lived owner does).
 *   VPUSH_INT(i)      — push an unboxed int.
 *   VPUSH_BOOL(b)     — push an unboxed bool.
 *   VPUSH_NIL()       — push an unboxed nil.
 *   VPUSH_REAL(f)     — push an unboxed real.
 *   VPOP()            — pop the top value (caller must release/take).
 *   VTOP_PTR()        — &-pointer to top value (in-place mutation).
 *
 * Naming: V-prefix to make every push/pop site at the source level
 * obviously a tagged-value access; the old PUSH/POP names are
 * deliberately removed so any missed conversion is a compile error.
 */
#define VPUSH(val) do { \
    if (vm_frame_grow_opstack(ctx, frame, 1) < 0) return -1; \
    frame->opstack[frame->op_sp++] = (val); \
} while (0)
#define VPUSH_VAR(p)     VPUSH(value_var_owned(p))
#define VPUSH_BORROW(p)  VPUSH(value_var_borrow(p))
#define VPUSH_INT(x)     VPUSH(value_int(x))
#define VPUSH_BOOL(x)    VPUSH(value_bool(x))
#define VPUSH_NIL()      VPUSH(value_nil())
#define VPUSH_REAL(x)    VPUSH(value_real(x))
#define VPOP()           (frame->opstack[--frame->op_sp])
#define VTOP_PTR()       (&frame->opstack[frame->op_sp - 1])

/* dispatch_one: execute one bytecode instruction on FRAME_TOP(ctx).
 * Returns 0 on success (frame may have changed via push/pop), -1 on
 * error.
 *
 * Inlining policy:
 *   - Without computed-goto (MSVC), we mark dispatch_one always-inline
 *     so the dispatch switch is placed directly inside vm_step's hot
 *     loop, eliminating the call/ret overhead per instruction.
 *   - With computed-goto (GCC/Clang), the dispatch table is a static
 *     const array of local-label addresses. GCC explicitly forbids
 *     inlining/cloning a function that takes addresses of local labels
 *     into a static, so we drop always_inline. The wins from
 *     per-opcode indirect-branch prediction and from skipping the
 *     switch range-check dominate the (well-predicted) call/ret per
 *     instruction.
 *
 * Computed-goto / direct-threaded dispatch.
 *
 * Under GCC/Clang we use the labels-as-values extension to replace the
 * switch (insn.op) with an indirect jump through a per-opcode label
 * table. This:
 *   - eliminates the switch's range-check on insn.op,
 *   - gives every opcode its own indirect-branch site, which lets the
 *     CPU's branch predictor learn opcode-pair patterns (e.g.
 *     LOAD_LOCAL → ADD → STORE_LOCAL) instead of funneling through
 *     one shared switch jump.
 *
 * On MSVC (and any compiler without __GNUC__/__clang__) we fall back
 * to the original switch dispatch — VM_DISPATCH_BEGIN expands to a
 * regular switch and VM_CASE/VM_DEFAULT expand to case/default. The
 * source of dispatch_one is identical for both paths.
 *
 * MLN_VM_NO_COMPUTED_GOTO can be defined externally to force the
 * switch path even on GCC/Clang (useful for diff-testing or for
 * working around hypothetical compiler bugs).
 */
#if (defined(__GNUC__) || defined(__clang__)) && !defined(_MSC_VER) && !defined(MLN_VM_NO_COMPUTED_GOTO)
# define MLN_VM_USE_COMPUTED_GOTO 1
/* Static label-address table prevents GCC from cloning/inlining
 * dispatch_one. Drop always_inline in this mode. */
# define MLN_VM_ALWAYS_INLINE
#elif defined(__GNUC__) || defined(__clang__)
# define MLN_VM_ALWAYS_INLINE __attribute__((always_inline))
#elif defined(_MSC_VER)
# define MLN_VM_ALWAYS_INLINE __forceinline
#else
# define MLN_VM_ALWAYS_INLINE
#endif

#ifdef MLN_VM_USE_COMPUTED_GOTO
# define VM_DISPATCH_BEGIN(op)                                                         \
    do {                                                                                \
        static const void * const _vm_dt[] = {                                          \
            [MLN_VOP_NOP]             = &&_L_MLN_VOP_NOP,                               \
            [MLN_VOP_POP]             = &&_L_MLN_VOP_POP,                               \
            [MLN_VOP_DUP]             = &&_L_MLN_VOP_DUP,                               \
            [MLN_VOP_LOAD_NIL]        = &&_L_MLN_VOP_LOAD_NIL,                          \
            [MLN_VOP_LOAD_TRUE]       = &&_L_MLN_VOP_LOAD_TRUE,                         \
            [MLN_VOP_LOAD_FALSE]      = &&_L_MLN_VOP_LOAD_FALSE,                        \
            [MLN_VOP_LOAD_INT]        = &&_L_MLN_VOP_LOAD_INT,                          \
            [MLN_VOP_LOAD_LOCAL]      = &&_L_MLN_VOP_LOAD_LOCAL,                        \
            [MLN_VOP_STORE_LOCAL]     = &&_L_MLN_VOP_STORE_LOCAL,                       \
            [MLN_VOP_ADD]             = &&_L_MLN_VOP_ADD,                               \
            [MLN_VOP_SUB]             = &&_L_MLN_VOP_SUB,                               \
            [MLN_VOP_MUL]             = &&_L_MLN_VOP_MUL,                               \
            [MLN_VOP_DIV]             = &&_L_MLN_VOP_DIV,                               \
            [MLN_VOP_MOD]             = &&_L_MLN_VOP_MOD,                               \
            [MLN_VOP_LT]              = &&_L_MLN_VOP_LT,                                \
            [MLN_VOP_LE]              = &&_L_MLN_VOP_LE,                                \
            [MLN_VOP_GT]              = &&_L_MLN_VOP_GT,                                \
            [MLN_VOP_GE]              = &&_L_MLN_VOP_GE,                                \
            [MLN_VOP_EQ]              = &&_L_MLN_VOP_EQ,                                \
            [MLN_VOP_NE]              = &&_L_MLN_VOP_NE,                                \
            [MLN_VOP_BOR]             = &&_L_MLN_VOP_BOR,                               \
            [MLN_VOP_BAND]            = &&_L_MLN_VOP_BAND,                              \
            [MLN_VOP_BXOR]            = &&_L_MLN_VOP_BXOR,                              \
            [MLN_VOP_LSHIFT]          = &&_L_MLN_VOP_LSHIFT,                            \
            [MLN_VOP_RSHIFT]          = &&_L_MLN_VOP_RSHIFT,                            \
            [MLN_VOP_JUMP]            = &&_L_MLN_VOP_JUMP,                              \
            [MLN_VOP_JUMP_IF_FALSE]   = &&_L_MLN_VOP_JUMP_IF_FALSE,                     \
            [MLN_VOP_JUMP_IF_TRUE]    = &&_L_MLN_VOP_JUMP_IF_TRUE,                      \
            [MLN_VOP_CALL_SELF]       = &&_L_MLN_VOP_CALL_SELF,                         \
            [MLN_VOP_RETURN]          = &&_L_MLN_VOP_RETURN,                            \
            [MLN_VOP_RETURN_NIL]      = &&_L_MLN_VOP_RETURN_NIL,                        \
            [MLN_VOP_ASSIGN_LOCAL]    = &&_L_MLN_VOP_ASSIGN_LOCAL,                      \
            [MLN_VOP_NOT]             = &&_L_MLN_VOP_NOT,                               \
            [MLN_VOP_NEG]             = &&_L_MLN_VOP_NEG,                               \
            [MLN_VOP_LOAD_LOCAL_INC]  = &&_L_MLN_VOP_LOAD_LOCAL_INC,                    \
            [MLN_VOP_LOAD_LOCAL_DEC]  = &&_L_MLN_VOP_LOAD_LOCAL_DEC,                    \
            [MLN_VOP_INC_LOCAL_LOAD]  = &&_L_MLN_VOP_INC_LOCAL_LOAD,                    \
            [MLN_VOP_DEC_LOCAL_LOAD]  = &&_L_MLN_VOP_DEC_LOCAL_LOAD,                    \
            [MLN_VOP_LOAD_REAL]       = &&_L_MLN_VOP_LOAD_REAL,                         \
            [MLN_VOP_LOAD_STRING]     = &&_L_MLN_VOP_LOAD_STRING,                       \
            [MLN_VOP_LOAD_GLOBAL]     = &&_L_MLN_VOP_LOAD_GLOBAL,                       \
            [MLN_VOP_GET_PROPERTY]    = &&_L_MLN_VOP_GET_PROPERTY,                      \
            [MLN_VOP_GET_INDEX]       = &&_L_MLN_VOP_GET_INDEX,                         \
            [MLN_VOP_SET_PROPERTY]    = &&_L_MLN_VOP_SET_PROPERTY,                      \
            [MLN_VOP_SET_INDEX]       = &&_L_MLN_VOP_SET_INDEX,                         \
            [MLN_VOP_CALL_VALUE]      = &&_L_MLN_VOP_CALL_VALUE,                        \
            [MLN_VOP_CALL_METHOD]     = &&_L_MLN_VOP_CALL_METHOD,                       \
            [MLN_VOP_BIND_FUNC]       = &&_L_MLN_VOP_BIND_FUNC,                         \
            [MLN_VOP_BIND_SET]        = &&_L_MLN_VOP_BIND_SET,                          \
            [MLN_VOP_NEW_OBJECT]      = &&_L_MLN_VOP_NEW_OBJECT,                        \
            [MLN_VOP_NEW_ARRAY]       = &&_L_MLN_VOP_NEW_ARRAY,                         \
            [MLN_VOP_ARRAY_PUT]       = &&_L_MLN_VOP_ARRAY_PUT,                         \
            [MLN_VOP_BITNOT]          = &&_L_MLN_VOP_BITNOT,                            \
            [MLN_VOP_LOAD_LOCAL_REF]  = &&_L_MLN_VOP_LOAD_LOCAL_REF,                    \
            [MLN_VOP_LOAD_GLOBAL_REF] = &&_L_MLN_VOP_LOAD_GLOBAL_REF,                   \
            [MLN_VOP_ASSIGN_GLOBAL]   = &&_L_MLN_VOP_ASSIGN_GLOBAL,                     \
            [MLN_VOP_SWAP2]           = &&_L_MLN_VOP_SWAP2,                             \
            [MLN_VOP_INVOKE_METHOD]   = &&_L_MLN_VOP_INVOKE_METHOD,                     \
            [MLN_VOP_CALL_GLOBAL]     = &&_L_MLN_VOP_CALL_GLOBAL,                       \
            [MLN_VOP_ADD_LL]          = &&_L_MLN_VOP_ADD_LL,                            \
            [MLN_VOP_SUB_LL]          = &&_L_MLN_VOP_SUB_LL,                            \
            [MLN_VOP_MUL_LL]          = &&_L_MLN_VOP_MUL_LL,                            \
            [MLN_VOP_LT_LL]           = &&_L_MLN_VOP_LT_LL,                             \
            [MLN_VOP_LE_LL]           = &&_L_MLN_VOP_LE_LL,                             \
            [MLN_VOP_GT_LL]           = &&_L_MLN_VOP_GT_LL,                             \
            [MLN_VOP_GE_LL]           = &&_L_MLN_VOP_GE_LL,                             \
            [MLN_VOP_EQ_LL]           = &&_L_MLN_VOP_EQ_LL,                             \
            [MLN_VOP_NE_LL]           = &&_L_MLN_VOP_NE_LL,                             \
            [MLN_VOP_ADD_LI]          = &&_L_MLN_VOP_ADD_LI,                            \
            [MLN_VOP_SUB_LI]          = &&_L_MLN_VOP_SUB_LI,                            \
            [MLN_VOP_MUL_LI]          = &&_L_MLN_VOP_MUL_LI,                            \
            [MLN_VOP_LT_LI]           = &&_L_MLN_VOP_LT_LI,                             \
            [MLN_VOP_LE_LI]           = &&_L_MLN_VOP_LE_LI,                             \
            [MLN_VOP_GT_LI]           = &&_L_MLN_VOP_GT_LI,                             \
            [MLN_VOP_GE_LI]           = &&_L_MLN_VOP_GE_LI,                             \
            [MLN_VOP_EQ_LI]           = &&_L_MLN_VOP_EQ_LI,                             \
            [MLN_VOP_NE_LI]           = &&_L_MLN_VOP_NE_LI,                             \
            [MLN_VOP_RETURN_LOCAL]    = &&_L_MLN_VOP_RETURN_LOCAL,                      \
            [MLN_VOP_DEAD_AST]        = &&_L_MLN_VOP_DEAD_AST,                          \
        };                                                                              \
        if ((unsigned)(op) >= (sizeof(_vm_dt)/sizeof(_vm_dt[0]))                        \
            || _vm_dt[(op)] == NULL) goto _L_VM_DEFAULT;                                \
        goto *_vm_dt[(op)];                                                              \
    } while (0); {
# define VM_DISPATCH_END           }
# define VM_CASE(name)             _L_##name
# define VM_DEFAULT                _L_VM_DEFAULT
#else
# define VM_DISPATCH_BEGIN(op)     switch (op) {
# define VM_DISPATCH_END           }
# define VM_CASE(name)             case name
# define VM_DEFAULT                default
#endif
#if defined(__GNUC__) || defined(__clang__)
__attribute__((hot))
#endif
static inline MLN_VM_ALWAYS_INLINE int dispatch_one(mln_lang_ctx_t *ctx)
{
    mln_lang_vm_frame_t *frame = FRAME_TOP(ctx);
    if (frame == NULL) return -1;

    /* Implicit return nil when pc reaches end of code. */
    if (frame->pc >= frame->chunk->code_len) {
        return vm_pop_frame_with_ret(ctx, NULL);
    }

    /* Resume from a suspended INTERNAL call: the ctx was resumed via
     * mln_lang_ctx_continue() so ctx->ret_var now holds the real result
     * set by the async completion handler.  Pop the preserved function
     * scope and push the return value onto our opstack, then fall through
     * to normal opcode dispatch so the next instruction runs this turn. */
    if (frame->awaiting_return) {
        if (mln_lang_withdraw_until_func_compat(ctx) < 0) return -1;
        frame->awaiting_return = 0;
        mln_lang_var_t *resume_ret = ctx->ret_var;
        ctx->ret_var = NULL;
        if (resume_ret == NULL) {
            resume_ret = mln_lang_var_create_nil(ctx, NULL);
            if (resume_ret == NULL) return -1;
        }
        if (vm_frame_grow_opstack(ctx, frame, 1) < 0) {
            mln_lang_var_free(resume_ret);
            return -1;
        }
        frame->opstack[frame->op_sp++] = value_var_owned(resume_ret);
        return 0;
    }

    mln_lang_vm_chunk_t *chunk = frame->chunk;
    mln_lang_vm_insn_t insn = chunk->code[frame->pc++];

    VM_DISPATCH_BEGIN(insn.op);
        VM_CASE(MLN_VOP_NOP):
            return 0;
        VM_CASE(MLN_VOP_POP): {
            mln_lang_value_t v = VPOP();
            value_release(&v);
            return 0;
        }
        VM_CASE(MLN_VOP_DUP): {
            /* DUP borrow-bit optimization: instead of bumping the var's
             * ref count, we copy the slot and stamp the copy as a borrow.
             * The original slot keeps its ownership; the duplicated slot
             * becomes a non-owning view that will release as a no-op.
             * For unboxed scalars (int/bool/nil/real) the copy is a pure
             * struct copy — DUP becomes free. */
            mln_lang_value_t t = *VTOP_PTR();
            value_make_borrow(&t);
            VPUSH(t);
            return 0;
        }
        VM_CASE(MLN_VOP_LOAD_NIL):
            VPUSH_NIL();
            return 0;
        VM_CASE(MLN_VOP_LOAD_TRUE):
            VPUSH_BOOL(1);
            return 0;
        VM_CASE(MLN_VOP_LOAD_FALSE):
            VPUSH_BOOL(0);
            return 0;
        VM_CASE(MLN_VOP_LOAD_INT):
            VPUSH_INT(chunk->iconsts[insn.b]);
            return 0;
        VM_CASE(MLN_VOP_LOAD_REAL):
            VPUSH_REAL(chunk->rconsts[insn.b]);
            return 0;
        VM_CASE(MLN_VOP_LOAD_STRING): {
            /* Strings still need to be boxed: the operand stack only
             * unboxes scalars (int/bool/nil/real); string literals
             * become a fresh var sharing the chunk's pooled mln_string_t. */
            mln_lang_var_t *v = mln_lang_var_create_string(ctx, chunk->sconsts[insn.b], NULL);
            if (v == NULL) return -1;
            VPUSH_VAR(v);
            return 0;
        }
        VM_CASE(MLN_VOP_LOAD_LOCAL): {
            /* Push a borrow of the slot var. The slot retains its single
             * ref via the symbol table; the opstack just observes it.
             * No ++ref. */
            VPUSH_BORROW(frame->slots[insn.a]);
            return 0;
        }
        VM_CASE(MLN_VOP_LOAD_GLOBAL): {
            mln_string_t *name = chunk->sconsts[insn.b];
            mln_lang_symbol_node_t *sym = mln_lang_symbol_node_search(ctx, name, 0);
            if (sym == NULL) {
                /* Variable not yet bound — create a nil slot in the current
                 * scope, matching AST interpreter semantics
                 * (src/mln_lang.c:6517-6536). */
                mln_lang_var_t *nv = vm_create_scope_var(ctx, name);
                if (nv == NULL) return -1;
                /* nv is owned by the symbol table now; push a borrow. */
                VPUSH_BORROW(nv);
                return 0;
            }
            if (sym->type != M_LANG_SYMBOL_VAR) {
                mln_lang_errmsg(ctx, "Invalid token. Token is a SET name, not a value or function.");
                return -1;
            }
            /* sym->data.var is owned by the symbol table; push a borrow. */
            VPUSH_BORROW(sym->data.var);
            return 0;
        }
        VM_CASE(MLN_VOP_STORE_LOCAL):
        VM_CASE(MLN_VOP_ASSIGN_LOCAL): {
            mln_lang_value_t v = VPOP();
            mln_lang_var_t *src = value_take_var(ctx, &v);
            if (src == NULL) return -1;
            if (slot_assign(ctx, frame->slots[insn.a], src) < 0) return -1;
            if (VM_HAS_WATCHER(frame->slots[insn.a]) && vm_fire_watcher(ctx, frame->slots[insn.a]) < 0) return -1;
            if (insn.op == MLN_VOP_ASSIGN_LOCAL) {
                /* Push the slot back as a borrow — the symbol table keeps
                 * the ref alive through the rest of this expression. The
                 * watcher (if any) may have been a CALL that pushed a
                 * frame; that frame's ret will be discarded so our push
                 * here is correct regardless. */
                if (vm_frame_grow_opstack(ctx, frame, 1) < 0) return -1;
                frame->opstack[frame->op_sp++] = value_var_borrow(frame->slots[insn.a]);
            }
            return 0;
        }
        VM_CASE(MLN_VOP_LOAD_LOCAL_INC):
        VM_CASE(MLN_VOP_LOAD_LOCAL_DEC): {
            mln_lang_var_t *sv = frame->slots[insn.a];
            if (sv->val == NULL || sv->val->type != M_LANG_VAL_TYPE_INT) {
                mln_lang_errmsg(ctx, "Suffix ++/-- requires int.");
                return -1;
            }
            mln_s64_t old_i = sv->val->data.i;
            if (insn.op == MLN_VOP_LOAD_LOCAL_INC) sv->val->data.i = old_i + 1;
            else                                   sv->val->data.i = old_i - 1;
            if (VM_HAS_WATCHER(sv) && vm_fire_watcher(ctx, sv) < 0) return -1;
            /* Push the OLD value as an unboxed int — no allocation. */
            VPUSH_INT(old_i);
            return 0;
        }
        VM_CASE(MLN_VOP_INC_LOCAL_LOAD):
        VM_CASE(MLN_VOP_DEC_LOCAL_LOAD): {
            mln_lang_var_t *sv = frame->slots[insn.a];
            if (sv->val == NULL || sv->val->type != M_LANG_VAL_TYPE_INT) {
                mln_lang_errmsg(ctx, "Prefix ++/-- requires int.");
                return -1;
            }
            mln_s64_t old_i = sv->val->data.i;
            mln_s64_t new_i = (insn.op == MLN_VOP_INC_LOCAL_LOAD) ? old_i + 1 : old_i - 1;
            sv->val->data.i = new_i;
            if (VM_HAS_WATCHER(sv) && vm_fire_watcher(ctx, sv) < 0) return -1;
            /* Push the NEW value as an unboxed int — no allocation. */
            VPUSH_INT(new_i);
            return 0;
        }
        VM_CASE(MLN_VOP_NOT): {
            mln_lang_value_t v = VPOP();
            int truthy = value_is_truthy(&v);
            value_release(&v);
            VPUSH_BOOL(!truthy);
            return 0;
        }
        VM_CASE(MLN_VOP_NEG): {
            /* Unboxed int / real fast paths bypass allocation. */
            mln_lang_value_t v = VPOP();
            if (v.kind == MLN_VAL_INT) {
                VPUSH_INT(-v.u.i);
                return 0;
            }
            if (v.kind == MLN_VAL_REAL) {
                VPUSH_REAL(-v.u.f);
                return 0;
            }
            /* Slow path: materialize and dispatch through method table. */
            mln_lang_var_t *t = value_take_var(ctx, &v);
            if (t == NULL) return -1;
            if (t->val != NULL && t->val->type == M_LANG_VAL_TYPE_INT) {
                mln_s64_t r = -t->val->data.i;
                mln_lang_var_free(t);
                VPUSH_INT(r);
                return 0;
            }
            mln_lang_method_t *method = (t->val != NULL) ? mln_lang_methods[t->val->type] : NULL;
            mln_lang_op h = method ? method->negative_handler : NULL;
            mln_lang_var_t *r = NULL;
            if (h == NULL || h(ctx, &r, t, NULL) < 0) {
                mln_lang_var_free(t);
                mln_lang_errmsg(ctx, "Operation NOT support.");
                return -1;
            }
            mln_lang_var_free(t);
            VPUSH_VAR(r);
            return 0;
        }
        VM_CASE(MLN_VOP_BITNOT): {
            /* Unary bitwise NOT (~x).  Int fast-path; non-int uses reverse_handler. */
            mln_lang_value_t v = VPOP();
            if (v.kind == MLN_VAL_INT) {
                VPUSH_INT(~v.u.i);
                return 0;
            }
            mln_lang_var_t *t = value_take_var(ctx, &v);
            if (t == NULL) return -1;
            if (t->val != NULL && t->val->type == M_LANG_VAL_TYPE_INT) {
                mln_s64_t r = ~t->val->data.i;
                mln_lang_var_free(t);
                VPUSH_INT(r);
                return 0;
            }
            mln_lang_method_t *method = (t->val != NULL) ? mln_lang_methods[t->val->type] : NULL;
            mln_lang_op h = method ? method->reverse_handler : NULL;
            mln_lang_var_t *r = NULL;
            if (h == NULL || h(ctx, &r, t, NULL) < 0) {
                mln_lang_var_free(t);
                mln_lang_errmsg(ctx, "Operation not supported.");
                return -1;
            }
            mln_lang_var_free(t);
            VPUSH_VAR(r);
            return 0;
        }
        VM_CASE(MLN_VOP_LOAD_LOCAL_REF): {
            /* Push a VAR_REFER wrapper (ref=0) sharing the slot's val.
             * mln_lang_var_new bumps val->ref; var->ref stays 0 so that
             * funccall_run recognises it as a by-reference argument. */
            mln_lang_var_t *sv = frame->slots[insn.a];
            mln_lang_var_t *rv = mln_lang_var_new(ctx, NULL, M_LANG_VAR_REFER, sv->val, NULL);
            if (rv == NULL) return -1;
            VPUSH_VAR(rv);
            return 0;
        }
        VM_CASE(MLN_VOP_LOAD_GLOBAL_REF): {
            mln_string_t *name = chunk->sconsts[insn.b];
            mln_lang_symbol_node_t *sym = mln_lang_symbol_node_search(ctx, name, 0);
            if (sym == NULL || sym->type != M_LANG_SYMBOL_VAR) {
                mln_lang_errmsg(ctx, "Undefined identifier.");
                return -1;
            }
            mln_lang_var_t *rv = mln_lang_var_new(ctx, NULL, M_LANG_VAR_REFER,
                                                  sym->data.var->val, NULL);
            if (rv == NULL) return -1;
            VPUSH_VAR(rv);
            return 0;
        }
        VM_CASE(MLN_VOP_ASSIGN_GLOBAL): {
            /* Pop top, assign to the named variable (searched via scope chain),
             * fire watcher, push back.  When the name does not exist anywhere,
             * create a nil slot in the current scope first — matching the AST
             * interpreter's behavior for new-variable assignment. */
            mln_string_t *name = chunk->sconsts[insn.b];
            mln_lang_symbol_node_t *sym = mln_lang_symbol_node_search(ctx, name, 0);
            mln_lang_var_t *target;
            if (sym == NULL) {
                target = vm_create_scope_var(ctx, name);
                if (target == NULL) {
                    mln_lang_value_t v = VPOP();
                    value_release(&v);
                    return -1;
                }
            } else if (sym->type != M_LANG_SYMBOL_VAR) {
                mln_lang_value_t v = VPOP();
                value_release(&v);
                mln_lang_errmsg(ctx, "Identifier is a SET name, not a variable.");
                return -1;
            } else {
                target = sym->data.var;
            }
            mln_lang_value_t v = VPOP();
            mln_lang_var_t *src = value_take_var(ctx, &v);
            if (src == NULL) return -1;
            if (slot_assign(ctx, target, src) < 0) return -1;
            if (VM_HAS_WATCHER(target) && vm_fire_watcher(ctx, target) < 0) return -1;
            /* Push back so the assignment is usable as an expression.
             * The symbol table keeps the ref alive; push a borrow. */
            if (vm_frame_grow_opstack(ctx, frame, 1) < 0) return -1;
            frame->opstack[frame->op_sp++] = value_var_borrow(target);
            return 0;
        }
        VM_CASE(MLN_VOP_SWAP2): {
            /* Swap the 2nd and 3rd elements from the top, leaving the top
             * element in place.  Transforms [a, b, c] (c=top) to [b, a, c].
             * Used to reorder [arr, arr2, key, key2] → [arr, key, arr2, key2]
             * so that GET_INDEX pops arr2 and key2 correctly for compound
             * index read-modify-write (+=, -=, postfix ++/-- etc.). */
            if (frame->op_sp < 3) {
                mln_lang_errmsg(ctx, "Stack underflow SWAP2.");
                return -1;
            }
            mln_lang_value_t tmp = frame->opstack[frame->op_sp - 2];
            frame->opstack[frame->op_sp - 2] = frame->opstack[frame->op_sp - 3];
            frame->opstack[frame->op_sp - 3] = tmp;
            return 0;
        }
        VM_CASE(MLN_VOP_ADD): VM_CASE(MLN_VOP_SUB): VM_CASE(MLN_VOP_MUL):
        VM_CASE(MLN_VOP_DIV): VM_CASE(MLN_VOP_MOD):
        VM_CASE(MLN_VOP_LT): VM_CASE(MLN_VOP_LE): VM_CASE(MLN_VOP_GT):
        VM_CASE(MLN_VOP_GE): VM_CASE(MLN_VOP_EQ): VM_CASE(MLN_VOP_NE):
        VM_CASE(MLN_VOP_BOR): VM_CASE(MLN_VOP_BAND): VM_CASE(MLN_VOP_BXOR):
        VM_CASE(MLN_VOP_LSHIFT): VM_CASE(MLN_VOP_RSHIFT): {
            mln_lang_value_t bv = VPOP();
            mln_lang_value_t av = VPOP();
            /* Hottest path: both operands are unboxed ints AND no
             * user-defined int operator overload is in effect. Go
             * directly to the inlined arithmetic and push an unboxed
             * result. No allocations, no refcount traffic, no
             * methods-table indirection. */
            if (av.kind == MLN_VAL_INT && bv.kind == MLN_VAL_INT && !ctx->op_int_flag) {
                mln_s64_t ai = av.u.i;
                mln_s64_t bi = bv.u.i;
                switch (insn.op) {
                    case MLN_VOP_ADD: VPUSH_INT(ai + bi); return 0;
                    case MLN_VOP_SUB: VPUSH_INT(ai - bi); return 0;
                    case MLN_VOP_MUL: VPUSH_INT(ai * bi); return 0;
                    case MLN_VOP_DIV:
                        if (bi == 0) { mln_lang_errmsg(ctx, "Division by zero."); return -1; }
                        VPUSH_INT(ai / bi); return 0;
                    case MLN_VOP_MOD:
                        if (bi == 0) { mln_lang_errmsg(ctx, "Modulo by zero."); return -1; }
                        VPUSH_INT(ai % bi); return 0;
                    case MLN_VOP_LT:  VPUSH_BOOL(ai <  bi); return 0;
                    case MLN_VOP_LE:  VPUSH_BOOL(ai <= bi); return 0;
                    case MLN_VOP_GT:  VPUSH_BOOL(ai >  bi); return 0;
                    case MLN_VOP_GE:  VPUSH_BOOL(ai >= bi); return 0;
                    case MLN_VOP_EQ:  VPUSH_BOOL(ai == bi); return 0;
                    case MLN_VOP_NE:  VPUSH_BOOL(ai != bi); return 0;
                    case MLN_VOP_BOR:    VPUSH_INT(ai | bi); return 0;
                    case MLN_VOP_BAND:   VPUSH_INT(ai & bi); return 0;
                    case MLN_VOP_BXOR:   VPUSH_INT(ai ^ bi); return 0;
                    case MLN_VOP_LSHIFT: VPUSH_INT(ai << bi); return 0;
                    case MLN_VOP_RSHIFT: VPUSH_INT(ai >> bi); return 0;
                    default: break;
                }
            }
            /* Slow path: at least one operand isn't an unboxed int (or
             * an overload is active). Materialize both into vars, then
             * route through apply_binop just like the pre-tagged code did. */
            mln_lang_var_t *a = value_take_var(ctx, &av);
            mln_lang_var_t *b = value_take_var(ctx, &bv);
            if (a == NULL || b == NULL) {
                if (a) mln_lang_var_free(a);
                if (b) mln_lang_var_free(b);
                return -1;
            }
            mln_lang_var_t *r = apply_binop(ctx, insn.op, a, b);
            if (r == NULL) return -1;
            /* Operator overload: method handler returned a CALL val (e.g. the
             * user defined __int_plus_operator__ and op_int_flag is set).
             * Extract the call before freeing r — null out data.call first so
             * mln_lang_var_free(r) does not also free the call via val_free.
             * Ownership of `call` transfers to us; we free it after the run. */
            if (r->val != NULL && r->val->type == M_LANG_VAL_TYPE_CALL) {
                mln_lang_funccall_val_t *call = r->val->data.call;
                r->val->data.call = NULL;  /* detach: r no longer owns call */
                mln_lang_var_free(r);
                mln_lang_vm_frame_t *saved_top = FRAME_TOP(ctx);
                mln_lang_stack_node_t *cur_run_top = ctx->run_stack_top;
                int rc_call = mln_lang_stack_handler_funccall_run_compat(ctx, cur_run_top, call);
                mln_lang_funccall_val_free(call);
                if (rc_call < 0) return -1;
                if (FRAME_TOP(ctx) != saved_top) {
                    return 0;
                }
                if (ctx->run_stack_top != cur_run_top) {
                    /* AST fallback: funccall_run pushed an AST stm node
                     * because the function body could not be compiled.
                     * Bump ctx->ref so vm_step stops after this dispatch_one
                     * returns (preventing immediate awaiting_return processing
                     * before the event loop has drained the run-stack). */
                    ctx->ref++;
                    frame->awaiting_return = 1;
                    return 0;
                }
                if (ctx->ref) {
                    frame->awaiting_return = 1;
                    return 0;
                }
                if (mln_lang_withdraw_until_func_compat(ctx) < 0) return -1;
                mln_lang_var_t *ret = ctx->ret_var;
                ctx->ret_var = NULL;
                if (ret == NULL) {
                    ret = mln_lang_var_create_nil(ctx, NULL);
                    if (ret == NULL) return -1;
                }
                VPUSH_VAR(ret);
                return 0;
            }
            VPUSH_VAR(r);
            return 0;
        }
        VM_CASE(MLN_VOP_JUMP):
            frame->pc = (mln_size_t)((mln_s64_t)frame->pc + insn.b);
            return 0;
        VM_CASE(MLN_VOP_JUMP_IF_FALSE): {
            mln_lang_value_t cond = VPOP();
            int truthy = value_is_truthy(&cond);
            value_release(&cond);
            if (!truthy) frame->pc = (mln_size_t)((mln_s64_t)frame->pc + insn.b);
            return 0;
        }
        VM_CASE(MLN_VOP_JUMP_IF_TRUE): {
            mln_lang_value_t cond = VPOP();
            int truthy = value_is_truthy(&cond);
            value_release(&cond);
            if (truthy) frame->pc = (mln_size_t)((mln_s64_t)frame->pc + insn.b);
            return 0;
        }
        VM_CASE(MLN_VOP_CALL_SELF):
        VM_CASE(MLN_VOP_CALL_VALUE):
        VM_CASE(MLN_VOP_CALL_METHOD): {
            int nargs = insn.a;
            int is_method = (insn.op == MLN_VOP_CALL_METHOD);
            int is_self   = (insn.op == MLN_VOP_CALL_SELF);

            /* Slot layout below TOS:
             *   CALL_SELF:    [..., arg0, ..., argN-1]
             *   CALL_VALUE:   [..., func, arg0, ..., argN-1]
             *   CALL_METHOD:  [..., obj, func, arg0, ..., argN-1]
             */
            mln_lang_value_t *args_base = &frame->opstack[frame->op_sp - nargs];
            mln_lang_value_t *func_slot = NULL;
            mln_lang_value_t *obj_slot  = NULL;
            mln_lang_func_detail_t *callee_proto = NULL;

            if (is_self) {
                callee_proto = frame->prototype;
                if (callee_proto == NULL) {
                    mln_lang_errmsg(ctx, "CALL_SELF: no current prototype.");
                    for (int i = 0; i < nargs; ++i) value_release(&args_base[i]);
                    frame->op_sp -= nargs;
                    return -1;
                }
            } else {
                func_slot = &frame->opstack[frame->op_sp - nargs - 1];
                if (is_method) obj_slot = &frame->opstack[frame->op_sp - nargs - 2];
                /* The callee must be a boxed VAR with a FUNC val. */
                mln_lang_var_t *func_var = value_peek_var(func_slot);
                if (func_var == NULL || func_var->val == NULL ||
                    func_var->val->type != M_LANG_VAL_TYPE_FUNC ||
                    func_var->val->data.func == NULL)
                {
                    mln_lang_errmsg(ctx, "Calling a non-function.");
                    for (int i = 0; i < nargs; ++i) value_release(&args_base[i]);
                    value_release(func_slot);
                    if (obj_slot) value_release(obj_slot);
                    frame->op_sp -= nargs + 1 + (is_method ? 1 : 0);
                    return -1;
                }
                callee_proto = func_var->val->data.func;
            }

            mln_lang_funccall_val_t *call = mln_lang_funccall_val_new(ctx->pool, NULL);
            if (call == NULL) {
                for (int i = 0; i < nargs; ++i) value_release(&args_base[i]);
                if (func_slot) value_release(func_slot);
                if (obj_slot)  value_release(obj_slot);
                frame->op_sp -= nargs + (is_self ? 0 : 1) + (is_method ? 1 : 0);
                return -1;
            }
            call->prototype = callee_proto;
            if (is_method && obj_slot != NULL) {
                mln_lang_var_t *obj_var = value_peek_var(obj_slot);
                if (obj_var != NULL) {
                    mln_lang_funccall_val_object_add(call, obj_var->val);
                }
            }
            int add_arg_failed = 0;
            for (int i = 0; i < nargs; ++i) {
                /* Materialize each arg into an owned var, then transfer
                 * ownership to the funccall. funccall_val_add_arg takes
                 * ownership on success. */
                mln_lang_var_t *argv = value_take_var(ctx, &args_base[i]);
                if (argv == NULL) {
                    add_arg_failed = 1;
                    for (int k = i + 1; k < nargs; ++k) value_release(&args_base[k]);
                    break;
                }
                if (mln_lang_funccall_val_add_arg(call, argv) < 0) {
                    add_arg_failed = 1;
                    mln_lang_var_free(argv);
                    for (int k = i + 1; k < nargs; ++k) value_release(&args_base[k]);
                    break;
                }
            }
            if (func_slot) value_release(func_slot);
            if (obj_slot)  value_release(obj_slot);
            frame->op_sp -= nargs + (is_self ? 0 : 1) + (is_method ? 1 : 0);

            if (add_arg_failed) {
                mln_lang_funccall_val_free(call);
                return -1;
            }

            /* Snapshot frame stack top so we can detect whether
             * funccall_run_compat pushed a new VM frame (compiled
             * EXTERNAL) or ran synchronously (INTERNAL). */
            mln_lang_vm_frame_t *saved_top = FRAME_TOP(ctx);
            mln_lang_stack_node_t *cur_run_top = ctx->run_stack_top;
            int rc_call = mln_lang_stack_handler_funccall_run_compat(ctx, cur_run_top, call);
            mln_lang_funccall_val_free(call);
            if (rc_call < 0) return -1;

            if (FRAME_TOP(ctx) != saved_top) {
                /* New VM frame pushed. Its RETURN will push ret to our
                 * opstack. Nothing more to do this iteration. */
                return 0;
            }
            if (ctx->run_stack_top != cur_run_top) {
                /* AST fallback: the function body could not be compiled;
                 * funccall_run pushed an AST stm onto the run-stack.
                 * Bump ctx->ref to suspend vm_step so the event loop can
                 * drain the run-stack before we process awaiting_return. */
                ctx->ref++;
                frame->awaiting_return = 1;
                return 0;
            }
            /* If the INTERNAL function suspended via mln_lang_ctx_suspend,
             * do NOT pop the scope or capture ret_var yet — the async
             * completion handler will update ctx->ret_var and then call
             * mln_lang_ctx_continue.  Set awaiting_return so that the very
             * next opcode dispatch (after ctx is resumed) pops the scope and
             * pushes the real return value. */
            if (ctx->ref) {
                frame->awaiting_return = 1;
                return 0;
            }
            /* Synchronous (INTERNAL). Pop scope and take ret_var. */
            if (mln_lang_withdraw_until_func_compat(ctx) < 0) return -1;
            mln_lang_var_t *ret = ctx->ret_var;
            ctx->ret_var = NULL;
            if (ret == NULL) {
                ret = mln_lang_var_create_nil(ctx, NULL);
                if (ret == NULL) return -1;
            }
            VPUSH_VAR(ret);
            return 0;
        }
        VM_CASE(MLN_VOP_RETURN): {
            mln_lang_value_t v = VPOP();
            mln_lang_var_t *r = value_take_var(ctx, &v);
            if (r == NULL) return -1;
            return vm_pop_frame_with_ret(ctx, r);
        }
        VM_CASE(MLN_VOP_RETURN_NIL): {
            return vm_pop_frame_with_ret(ctx, NULL);
        }
        VM_CASE(MLN_VOP_BIND_FUNC): {
            mln_lang_funcdef_t *fd = (mln_lang_funcdef_t *)chunk->funcdefs[insn.b];
            mln_lang_set_detail_t *in_set = mln_lang_ctx_get_class_compat(ctx);
            mln_lang_func_detail_t *func = mln_lang_func_detail_new(ctx,
                    M_FUNC_EXTERNAL, fd->stm, fd->args, fd->closure);
            if (func == NULL) {
                mln_lang_errmsg(ctx, "Parse function definition failed.");
                return -1;
            }
            if (fd->name->len > 18 && fd->name->data[0] == '_' && fd->name->data[1] == '_') {
                switch (fd->name->data[2]) {
                    case 'a': ctx->op_array_flag = 1; break;
                    case 'b': ctx->op_bool_flag  = 1; break;
                    case 'f': ctx->op_func_flag  = 1; break;
                    case 'i': ctx->op_int_flag   = 1; break;
                    case 'n': ctx->op_nil_flag   = 1; break;
                    case 'o': ctx->op_obj_flag   = 1; break;
                    case 'r': ctx->op_real_flag  = 1; break;
                    case 's': ctx->op_str_flag   = 1; break;
                }
            }
            mln_lang_val_t *fval = mln_lang_val_new(ctx, M_LANG_VAL_TYPE_FUNC, func);
            if (fval == NULL) {
                mln_lang_func_detail_free(func);
                return -1;
            }
            mln_lang_var_t *fvar = mln_lang_var_new(ctx, fd->name, M_LANG_VAR_NORMAL, fval, in_set);
            if (fvar == NULL) {
                mln_lang_val_free(fval);
                return -1;
            }
            if (in_set != NULL) {
                if (mln_lang_set_member_add(ctx->pool, in_set->members, fvar) < 0) {
                    mln_lang_var_free(fvar); return -1;
                }
            } else {
                if (mln_lang_symbol_node_join(ctx, M_LANG_SYMBOL_VAR, fvar) < 0) {
                    mln_lang_var_free(fvar); return -1;
                }
            }
            return 0;
        }
        VM_CASE(MLN_VOP_BIND_SET): {
            mln_lang_set_t *sd = (mln_lang_set_t *)chunk->setdefs[insn.b];
            mln_lang_set_detail_t *set_detail = mln_lang_set_detail_new(ctx->pool, sd->name);
            if (set_detail == NULL) return -1;
            for (mln_lang_setstm_t *ss = sd->stm; ss != NULL; ss = ss->next) {
                if (ss->type == M_SETSTM_VAR) {
                    mln_lang_val_t *mv = mln_lang_val_new(ctx, M_LANG_VAL_TYPE_NIL, NULL);
                    if (mv == NULL) { mln_lang_set_detail_free(set_detail); return -1; }
                    mln_lang_var_t *mvar = mln_lang_var_new(ctx, ss->data.var, M_LANG_VAR_NORMAL, mv, set_detail);
                    if (mvar == NULL) {
                        mln_lang_val_free(mv); mln_lang_set_detail_free(set_detail); return -1;
                    }
                    if (mln_lang_set_member_add(ctx->pool, set_detail->members, mvar) < 0) {
                        mln_lang_var_free(mvar); mln_lang_set_detail_free(set_detail); return -1;
                    }
                } else {
                    mln_lang_funcdef_t *fd = ss->data.func;
                    mln_lang_func_detail_t *mfunc = mln_lang_func_detail_new(ctx,
                            M_FUNC_EXTERNAL, fd->stm, fd->args, fd->closure);
                    if (mfunc == NULL) { mln_lang_set_detail_free(set_detail); return -1; }
                    mln_lang_val_t *mv = mln_lang_val_new(ctx, M_LANG_VAL_TYPE_FUNC, mfunc);
                    if (mv == NULL) {
                        mln_lang_func_detail_free(mfunc); mln_lang_set_detail_free(set_detail); return -1;
                    }
                    mln_lang_var_t *mvar = mln_lang_var_new(ctx, fd->name, M_LANG_VAR_NORMAL, mv, set_detail);
                    if (mvar == NULL) {
                        mln_lang_val_free(mv); mln_lang_set_detail_free(set_detail); return -1;
                    }
                    if (mln_lang_set_member_add(ctx->pool, set_detail->members, mvar) < 0) {
                        mln_lang_var_free(mvar); mln_lang_set_detail_free(set_detail); return -1;
                    }
                }
            }
            if (mln_lang_symbol_node_join(ctx, M_LANG_SYMBOL_SET, set_detail) < 0) {
                mln_lang_set_detail_free(set_detail); return -1;
            }
            return 0;
        }
        VM_CASE(MLN_VOP_NEW_OBJECT): {
            mln_string_t *name = chunk->sconsts[insn.b];
            mln_lang_symbol_node_t *sym;
            while (1) {
                sym = mln_lang_symbol_node_id_search_compat(ctx, name);
                if (sym == NULL) break;
                if (sym->type == M_LANG_SYMBOL_SET) break;
                if (sym->type != M_LANG_SYMBOL_VAR) { sym = NULL; break; }
                if (sym->data.var->val == NULL ||
                    sym->data.var->val->type != M_LANG_VAL_TYPE_STRING) { sym = NULL; break; }
                name = sym->data.var->val->data.s;
                if (name == NULL) { sym = NULL; break; }
            }
            if (sym == NULL || sym->type != M_LANG_SYMBOL_SET) {
                /* Set not found — push unboxed nil. */
                VPUSH_NIL();
                return 0;
            }
            mln_lang_object_t *obj_inst = mln_lang_object_new_compat(ctx, sym->data.set);
            if (obj_inst == NULL) return -1;
            mln_lang_val_t *ov = mln_lang_val_new(ctx, M_LANG_VAL_TYPE_OBJECT, obj_inst);
            if (ov == NULL) { mln_lang_object_free_compat(obj_inst); return -1; }
            mln_lang_var_t *ovar = mln_lang_var_new(ctx, NULL, M_LANG_VAR_NORMAL, ov, NULL);
            if (ovar == NULL) { mln_lang_val_free(ov); return -1; }
            VPUSH_VAR(ovar);
            return 0;
        }
        VM_CASE(MLN_VOP_NEW_ARRAY): {
            mln_lang_array_t *arr = mln_lang_array_new(ctx);
            if (arr == NULL) return -1;
            mln_lang_val_t *av = mln_lang_val_new(ctx, M_LANG_VAL_TYPE_ARRAY, arr);
            if (av == NULL) { mln_lang_array_free(arr); return -1; }
            mln_lang_var_t *avar = mln_lang_var_new(ctx, NULL, M_LANG_VAR_NORMAL, av, NULL);
            if (avar == NULL) { mln_lang_val_free(av); return -1; }
            VPUSH_VAR(avar);
            return 0;
        }
        VM_CASE(MLN_VOP_ARRAY_PUT): {
            mln_lang_value_t valv_v = VPOP();
            mln_lang_value_t keyv_v = VPOP();
            mln_lang_value_t *top    = VTOP_PTR();
            /* The array on top must be a boxed VAR holding ARRAY. We
             * do NOT pop it — the literal compiler keeps chaining onto
             * it. value_peek_var returns NULL for unboxed scalars. */
            mln_lang_var_t *arrv = value_peek_var(top);
            if (arrv == NULL || arrv->val == NULL ||
                arrv->val->type != M_LANG_VAL_TYPE_ARRAY)
            {
                value_release(&keyv_v); value_release(&valv_v);
                mln_lang_errmsg(ctx, "ARRAY_PUT: top is not an array.");
                return -1;
            }
            mln_lang_var_t *valv = value_take_var(ctx, &valv_v);
            mln_lang_var_t *keyv = value_take_var(ctx, &keyv_v);
            if (valv == NULL || keyv == NULL) {
                if (valv) mln_lang_var_free(valv);
                if (keyv) mln_lang_var_free(keyv);
                return -1;
            }
            int key_is_nil = (keyv->val != NULL && keyv->val->type == M_LANG_VAL_TYPE_NIL);
            mln_lang_var_t *slot_var = mln_lang_array_get(ctx, arrv->val->data.array,
                                                          key_is_nil ? NULL : keyv);
            if (slot_var == NULL) {
                mln_lang_var_free(keyv); mln_lang_var_free(valv);
                return -1;
            }
            if (mln_lang_var_value_set(ctx, slot_var, valv) < 0) {
                mln_lang_var_free(keyv); mln_lang_var_free(valv);
                return -1;
            }
            mln_lang_var_free(keyv);
            mln_lang_var_free(valv);
            return 0;
        }
        VM_CASE(MLN_VOP_GET_PROPERTY): {
            mln_lang_value_t obj_v = VPOP();
            mln_lang_var_t *obj_op = value_take_var(ctx, &obj_v);
            if (obj_op == NULL) return -1;
            if (obj_op->val == NULL) {
                mln_lang_var_free(obj_op);
                mln_lang_errmsg(ctx, "Property access on nil.");
                return -1;
            }
            mln_string_t *name = chunk->sconsts[insn.b];

            /* Medium-tier perf: inline-cache fast path for object property
             * access. Skips the method-table dispatch and the temp namev
             * allocation when the property handler is the standard one
             * (no operator overload, no auto-create needed). */
            if (obj_op->val->type == M_LANG_VAL_TYPE_OBJECT && !ctx->op_obj_flag) {
                mln_lang_object_t *o = obj_op->val->data.obj;
                mln_lang_var_t *member = mln_lang_set_member_search(o->members, name);
                if (member != NULL) {
                    /* Cache the obj's set identity for diagnostic / future
                     * polymorphic-IC use. We never deref cached_set. */
                    if (chunk->ic_slots != NULL) {
                        mln_lang_vm_ic_t *ic = &chunk->ic_slots[frame->pc - 1];
                        ic->cached_set = (void *)o->in_set;
                        ic->cached_kind = 1;
                    }
                    /* Push the member as a borrow — it lives in the obj's
                     * member tree, which the obj keeps alive until the
                     * obj itself is freed. The next consumer (e.g.
                     * STORE_LOCAL, RETURN, CALL) will materialize. */
                    mln_lang_var_free(obj_op);
                    VPUSH_BORROW(member);
                    return 0;
                }
                /* Member missing: fall through to slow path which will
                 * auto-create a nil entry (matching AST semantics). */
            }

            mln_lang_method_t *method = mln_lang_methods[obj_op->val->type];
            if (method == NULL || method->property_handler == NULL) {
                mln_lang_var_free(obj_op);
                mln_lang_errmsg(ctx, "Operation NOT support.");
                return -1;
            }
            mln_lang_var_t *namev = mln_lang_var_create_string(ctx, name, NULL);
            if (namev == NULL) { mln_lang_var_free(obj_op); return -1; }
            mln_lang_var_t *res = NULL;
            if (method->property_handler(ctx, &res, obj_op, namev) < 0) {
                mln_lang_var_free(namev); mln_lang_var_free(obj_op);
                return -1;
            }
            mln_lang_var_free(namev);
            mln_lang_var_free(obj_op);
            if (res == NULL) {
                mln_lang_errmsg(ctx, "Property handler returned NULL.");
                return -1;
            }
            VPUSH_VAR(res);
            return 0;
        }
        VM_CASE(MLN_VOP_GET_INDEX): {
            mln_lang_value_t key_v = VPOP();
            mln_lang_value_t arr_v = VPOP();
            mln_lang_var_t *key = value_take_var(ctx, &key_v);
            mln_lang_var_t *arr = value_take_var(ctx, &arr_v);
            if (key == NULL || arr == NULL) {
                if (key) mln_lang_var_free(key);
                if (arr) mln_lang_var_free(arr);
                return -1;
            }
            if (arr->val == NULL) {
                mln_lang_var_free(key); mln_lang_var_free(arr);
                mln_lang_errmsg(ctx, "Index on nil.");
                return -1;
            }
            mln_lang_method_t *method = mln_lang_methods[arr->val->type];
            if (method == NULL || method->index_handler == NULL) {
                mln_lang_var_free(key); mln_lang_var_free(arr);
                mln_lang_errmsg(ctx, "Operation NOT support.");
                return -1;
            }
            mln_lang_var_t *res = NULL;
            if (method->index_handler(ctx, &res, arr, key) < 0) {
                mln_lang_var_free(key); mln_lang_var_free(arr);
                return -1;
            }
            mln_lang_var_free(key);
            mln_lang_var_free(arr);
            if (res == NULL) return -1;
            VPUSH_VAR(res);
            return 0;
        }
        VM_CASE(MLN_VOP_SET_PROPERTY): {
            mln_lang_value_t val_v = VPOP();
            mln_lang_value_t obj_v = VPOP();
            mln_lang_var_t *val = value_take_var(ctx, &val_v);
            mln_lang_var_t *obj_op = value_take_var(ctx, &obj_v);
            if (val == NULL || obj_op == NULL) {
                if (val) mln_lang_var_free(val);
                if (obj_op) mln_lang_var_free(obj_op);
                return -1;
            }
            if (obj_op->val == NULL) {
                mln_lang_var_free(val); mln_lang_var_free(obj_op);
                mln_lang_errmsg(ctx, "Property assign on nil.");
                return -1;
            }
            mln_string_t *name = chunk->sconsts[insn.b];

            /* Medium-tier perf: inline-cache fast path for object property
             * assignment (mirrors GET_PROPERTY fast path). */
            if (obj_op->val->type == M_LANG_VAL_TYPE_OBJECT && !ctx->op_obj_flag) {
                mln_lang_object_t *o = obj_op->val->data.obj;
                mln_lang_var_t *slot_member = mln_lang_set_member_search(o->members, name);
                if (slot_member != NULL) {
                    if (chunk->ic_slots != NULL) {
                        mln_lang_vm_ic_t *ic = &chunk->ic_slots[frame->pc - 1];
                        ic->cached_set = (void *)o->in_set;
                        ic->cached_kind = 1;
                    }
                    if (mln_lang_var_value_set(ctx, slot_member, val) < 0) {
                        mln_lang_var_free(val); mln_lang_var_free(obj_op);
                        return -1;
                    }
                    if (VM_HAS_WATCHER(slot_member) && vm_fire_watcher(ctx, slot_member) < 0) {
                        mln_lang_var_free(val); mln_lang_var_free(obj_op);
                        return -1;
                    }
                    if (insn.a) {
                        VPUSH_VAR(val);
                    } else {
                        mln_lang_var_free(val);
                    }
                    mln_lang_var_free(obj_op);
                    return 0;
                }
                /* Member missing on a fast-path-eligible obj: fall through
                 * to slow path (which auto-creates the slot). */
            }

            mln_lang_method_t *method = mln_lang_methods[obj_op->val->type];
            if (method == NULL || method->property_handler == NULL) {
                mln_lang_var_free(val); mln_lang_var_free(obj_op);
                mln_lang_errmsg(ctx, "Operation NOT support.");
                return -1;
            }
            mln_lang_var_t *namev = mln_lang_var_create_string(ctx, name, NULL);
            if (namev == NULL) {
                mln_lang_var_free(val); mln_lang_var_free(obj_op); return -1;
            }
            mln_lang_var_t *slot_var = NULL;
            if (method->property_handler(ctx, &slot_var, obj_op, namev) < 0) {
                mln_lang_var_free(namev); mln_lang_var_free(val); mln_lang_var_free(obj_op);
                return -1;
            }
            mln_lang_var_free(namev);
            if (slot_var == NULL) {
                mln_lang_var_free(val); mln_lang_var_free(obj_op);
                mln_lang_errmsg(ctx, "Property slot not found.");
                return -1;
            }
            if (mln_lang_var_value_set(ctx, slot_var, val) < 0) {
                mln_lang_var_free(slot_var); mln_lang_var_free(val); mln_lang_var_free(obj_op);
                return -1;
            }
            if (VM_HAS_WATCHER(slot_var) && vm_fire_watcher(ctx, slot_var) < 0) {
                mln_lang_var_free(slot_var); mln_lang_var_free(val); mln_lang_var_free(obj_op);
                return -1;
            }
            mln_lang_var_free(slot_var);
            if (insn.a) {
                /* Push val back as expression result (assignment-as-expression
                 * context: `return (obj.x = v)`, `(obj.x = v, ...)`, etc.).
                 * compile_assign emits SET_PROPERTY with a=1 for this purpose. */
                VPUSH_VAR(val);
            } else {
                mln_lang_var_free(val);
            }
            mln_lang_var_free(obj_op);
            return 0;
        }
        VM_CASE(MLN_VOP_SET_INDEX): {
            mln_lang_value_t val_v = VPOP();
            mln_lang_value_t key_v = VPOP();
            mln_lang_value_t arr_v = VPOP();
            mln_lang_var_t *val = value_take_var(ctx, &val_v);
            mln_lang_var_t *key = value_take_var(ctx, &key_v);
            mln_lang_var_t *arr = value_take_var(ctx, &arr_v);
            if (val == NULL || key == NULL || arr == NULL) {
                if (val) mln_lang_var_free(val);
                if (key) mln_lang_var_free(key);
                if (arr) mln_lang_var_free(arr);
                return -1;
            }
            if (arr->val == NULL) {
                mln_lang_var_free(val); mln_lang_var_free(key); mln_lang_var_free(arr);
                mln_lang_errmsg(ctx, "Index assign on nil.");
                return -1;
            }
            mln_lang_method_t *method = mln_lang_methods[arr->val->type];
            if (method == NULL || method->index_handler == NULL) {
                mln_lang_var_free(val); mln_lang_var_free(key); mln_lang_var_free(arr);
                mln_lang_errmsg(ctx, "Operation NOT support.");
                return -1;
            }
            mln_lang_var_t *slot_var = NULL;
            if (method->index_handler(ctx, &slot_var, arr, key) < 0) {
                mln_lang_var_free(val); mln_lang_var_free(key); mln_lang_var_free(arr);
                return -1;
            }
            if (slot_var == NULL) {
                mln_lang_var_free(val); mln_lang_var_free(key); mln_lang_var_free(arr);
                mln_lang_errmsg(ctx, "Index slot not found.");
                return -1;
            }
            if (mln_lang_var_value_set(ctx, slot_var, val) < 0) {
                mln_lang_var_free(slot_var); mln_lang_var_free(val);
                mln_lang_var_free(key); mln_lang_var_free(arr);
                return -1;
            }
            if (VM_HAS_WATCHER(slot_var) && vm_fire_watcher(ctx, slot_var) < 0) {
                mln_lang_var_free(slot_var); mln_lang_var_free(val);
                mln_lang_var_free(key); mln_lang_var_free(arr);
                return -1;
            }
            mln_lang_var_free(slot_var);
            mln_lang_var_free(key);
            mln_lang_var_free(arr);
            if (insn.a) {
                /* Push val back as expression result (assignment-as-expression
                 * context).  compile_assign emits SET_INDEX with a=1. */
                VPUSH_VAR(val);
            } else {
                mln_lang_var_free(val);
            }
            return 0;
        }
        /* ============================================================
         * Medium-tier perf opcodes
         * ============================================================ */

        VM_CASE(MLN_VOP_INVOKE_METHOD): {
            /* Stack: [..., obj, arg0, ..., argN-1] -> [..., result]
             * Operands: a=nargs, b=sconsts index of method name.
             * Fused replacement for GET_PROPERTY+CALL_METHOD when the
             * receiver is an object whose property lookup is the standard
             * path (no operator overload). Falls back to the generic
             * GET_PROPERTY+CALL_METHOD path if the receiver isn't an
             * object or an overload is in effect. */
            int nargs = insn.a;
            mln_string_t *mname = chunk->sconsts[insn.b];
            mln_lang_value_t *args_base = &frame->opstack[frame->op_sp - nargs];
            mln_lang_value_t *obj_slot  = &frame->opstack[frame->op_sp - nargs - 1];
            /* The receiver must be a boxed VAR — primitives have no methods. */
            mln_lang_var_t *obj = value_peek_var(obj_slot);

            if (obj == NULL || obj->val == NULL) {
                for (int i = 0; i < nargs; ++i) value_release(&args_base[i]);
                value_release(obj_slot);
                frame->op_sp -= nargs + 1;
                mln_lang_errmsg(ctx, "Method call on nil.");
                return -1;
            }

            /* Fast path: object receiver, standard property lookup. */
            mln_lang_var_t *func_var = NULL;
            int fast_path_ok = 0;
            if (obj->val->type == M_LANG_VAL_TYPE_OBJECT && !ctx->op_obj_flag) {
                mln_lang_object_t *o = obj->val->data.obj;
                func_var = mln_lang_set_member_search(o->members, mname);
                if (func_var != NULL && func_var->val != NULL &&
                    func_var->val->type == M_LANG_VAL_TYPE_FUNC &&
                    func_var->val->data.func != NULL)
                {
                    if (chunk->ic_slots != NULL) {
                        mln_lang_vm_ic_t *ic = &chunk->ic_slots[frame->pc - 1];
                        ic->cached_set = (void *)o->in_set;
                        ic->cached_func = (void *)func_var->val->data.func;
                        ic->cached_kind = 3;
                    }
                    fast_path_ok = 1;
                } else {
                    func_var = NULL;
                }
            }

            if (!fast_path_ok) {
                /* Slow path: emulate GET_PROPERTY then CALL_METHOD by
                 * dispatching through the method table. */
                mln_lang_method_t *method = mln_lang_methods[obj->val->type];
                if (method == NULL || method->property_handler == NULL) {
                    for (int i = 0; i < nargs; ++i) value_release(&args_base[i]);
                    value_release(obj_slot);
                    frame->op_sp -= nargs + 1;
                    mln_lang_errmsg(ctx, "Operation NOT support.");
                    return -1;
                }
                mln_lang_var_t *namev = mln_lang_var_create_string(ctx, mname, NULL);
                if (namev == NULL) {
                    for (int i = 0; i < nargs; ++i) value_release(&args_base[i]);
                    value_release(obj_slot);
                    frame->op_sp -= nargs + 1;
                    return -1;
                }
                mln_lang_var_t *prop_res = NULL;
                if (method->property_handler(ctx, &prop_res, obj, namev) < 0) {
                    mln_lang_var_free(namev);
                    for (int i = 0; i < nargs; ++i) value_release(&args_base[i]);
                    value_release(obj_slot);
                    frame->op_sp -= nargs + 1;
                    return -1;
                }
                mln_lang_var_free(namev);
                if (prop_res == NULL || prop_res->val == NULL ||
                    prop_res->val->type != M_LANG_VAL_TYPE_FUNC ||
                    prop_res->val->data.func == NULL)
                {
                    if (prop_res != NULL) mln_lang_var_free(prop_res);
                    for (int i = 0; i < nargs; ++i) value_release(&args_base[i]);
                    value_release(obj_slot);
                    frame->op_sp -= nargs + 1;
                    mln_lang_errmsg(ctx, "Method is not a function.");
                    return -1;
                }
                func_var = prop_res;  /* owned ref we must drop after building call */
            }

            mln_lang_func_detail_t *callee_proto = func_var->val->data.func;

            mln_lang_funccall_val_t *call = mln_lang_funccall_val_new(ctx->pool, NULL);
            if (call == NULL) {
                if (!fast_path_ok) mln_lang_var_free(func_var);
                for (int i = 0; i < nargs; ++i) value_release(&args_base[i]);
                value_release(obj_slot);
                frame->op_sp -= nargs + 1;
                return -1;
            }
            call->prototype = callee_proto;
            mln_lang_funccall_val_object_add(call, obj->val);

            int add_arg_failed = 0;
            for (int i = 0; i < nargs; ++i) {
                mln_lang_var_t *argv = value_take_var(ctx, &args_base[i]);
                if (argv == NULL) {
                    add_arg_failed = 1;
                    for (int k = i + 1; k < nargs; ++k) value_release(&args_base[k]);
                    break;
                }
                if (mln_lang_funccall_val_add_arg(call, argv) < 0) {
                    add_arg_failed = 1;
                    mln_lang_var_free(argv);
                    for (int k = i + 1; k < nargs; ++k) value_release(&args_base[k]);
                    break;
                }
            }
            if (!fast_path_ok) mln_lang_var_free(func_var);
            value_release(obj_slot);
            frame->op_sp -= nargs + 1;
            if (add_arg_failed) { mln_lang_funccall_val_free(call); return -1; }

            mln_lang_vm_frame_t *saved_top = FRAME_TOP(ctx);
            mln_lang_stack_node_t *cur_run_top = ctx->run_stack_top;
            int rc_call = mln_lang_stack_handler_funccall_run_compat(ctx, cur_run_top, call);
            mln_lang_funccall_val_free(call);
            if (rc_call < 0) return -1;
            if (FRAME_TOP(ctx) != saved_top) return 0;
            if (ctx->run_stack_top != cur_run_top) {
                ctx->ref++;
                frame->awaiting_return = 1;
                return 0;
            }
            if (ctx->ref) {
                frame->awaiting_return = 1;
                return 0;
            }
            if (mln_lang_withdraw_until_func_compat(ctx) < 0) return -1;
            mln_lang_var_t *ret = ctx->ret_var;
            ctx->ret_var = NULL;
            if (ret == NULL) {
                ret = mln_lang_var_create_nil(ctx, NULL);
                if (ret == NULL) return -1;
            }
            VPUSH_VAR(ret);
            return 0;
        }

        VM_CASE(MLN_VOP_CALL_GLOBAL): {
            /* Fused LOAD_GLOBAL + CALL_VALUE for hot builtin/global calls.
             * Operands: a = nargs, b = sconsts index of global name.
             * IC caches the resolved sym->data.var pointer (identity-only)
             * so subsequent calls skip the global symbol lookup. */
            int nargs = insn.a;
            mln_string_t *gname = chunk->sconsts[insn.b];
            mln_lang_value_t *args_base = &frame->opstack[frame->op_sp - nargs];

            mln_lang_func_detail_t *callee_proto = NULL;
            mln_lang_vm_ic_t *ic = (chunk->ic_slots != NULL) ? &chunk->ic_slots[frame->pc - 1] : NULL;

            mln_lang_symbol_node_t *sym = mln_lang_symbol_node_search(ctx, gname, 0);
            if (sym == NULL || sym->type != M_LANG_SYMBOL_VAR ||
                sym->data.var == NULL || sym->data.var->val == NULL ||
                sym->data.var->val->type != M_LANG_VAL_TYPE_FUNC ||
                sym->data.var->val->data.func == NULL)
            {
                /* Fallback: emit-equivalent of LOAD_GLOBAL+CALL_VALUE error. */
                for (int i = 0; i < nargs; ++i) value_release(&args_base[i]);
                frame->op_sp -= nargs;
                mln_lang_errmsg(ctx, "Calling a non-function.");
                return -1;
            }
            callee_proto = sym->data.var->val->data.func;
            if (ic != NULL) {
                ic->cached_func = (void *)callee_proto;
                ic->cached_kind = 4;
            }

            mln_lang_funccall_val_t *call = mln_lang_funccall_val_new(ctx->pool, NULL);
            if (call == NULL) {
                for (int i = 0; i < nargs; ++i) value_release(&args_base[i]);
                frame->op_sp -= nargs;
                return -1;
            }
            call->prototype = callee_proto;
            int add_arg_failed = 0;
            for (int i = 0; i < nargs; ++i) {
                mln_lang_var_t *argv = value_take_var(ctx, &args_base[i]);
                if (argv == NULL) {
                    add_arg_failed = 1;
                    for (int k = i + 1; k < nargs; ++k) value_release(&args_base[k]);
                    break;
                }
                if (mln_lang_funccall_val_add_arg(call, argv) < 0) {
                    add_arg_failed = 1;
                    mln_lang_var_free(argv);
                    for (int k = i + 1; k < nargs; ++k) value_release(&args_base[k]);
                    break;
                }
            }
            frame->op_sp -= nargs;
            if (add_arg_failed) { mln_lang_funccall_val_free(call); return -1; }

            mln_lang_vm_frame_t *saved_top = FRAME_TOP(ctx);
            mln_lang_stack_node_t *cur_run_top = ctx->run_stack_top;
            int rc_call = mln_lang_stack_handler_funccall_run_compat(ctx, cur_run_top, call);
            mln_lang_funccall_val_free(call);
            if (rc_call < 0) return -1;
            if (FRAME_TOP(ctx) != saved_top) return 0;
            if (ctx->run_stack_top != cur_run_top) {
                ctx->ref++;
                frame->awaiting_return = 1;
                return 0;
            }
            if (ctx->ref) { frame->awaiting_return = 1; return 0; }
            if (mln_lang_withdraw_until_func_compat(ctx) < 0) return -1;
            mln_lang_var_t *ret = ctx->ret_var;
            ctx->ret_var = NULL;
            if (ret == NULL) {
                ret = mln_lang_var_create_nil(ctx, NULL);
                if (ret == NULL) return -1;
            }
            VPUSH_VAR(ret);
            return 0;
        }

        /* ============================================================
         * Superinstructions: <op>_LL (two-locals) and <op>_LI (local + iconst).
         * Each falls back to apply_binop when operands aren't both int.
         * Tagged-value rework: when both operands are unboxed-int-clean
         * (slot val is INT, no overload), we now compute the result in a
         * single 8-byte arithmetic op and push an unboxed value — no
         * allocation, no apply_binop call, no methods-table lookup.
         * ============================================================ */

#define VM_INT_BINOP_BODY(base_op, ai, bi)                                            \
            switch (base_op) {                                                        \
                case MLN_VOP_ADD: VPUSH_INT((ai) + (bi)); return 0;                   \
                case MLN_VOP_SUB: VPUSH_INT((ai) - (bi)); return 0;                   \
                case MLN_VOP_MUL: VPUSH_INT((ai) * (bi)); return 0;                   \
                case MLN_VOP_LT:  VPUSH_BOOL((ai) <  (bi)); return 0;                 \
                case MLN_VOP_LE:  VPUSH_BOOL((ai) <= (bi)); return 0;                 \
                case MLN_VOP_GT:  VPUSH_BOOL((ai) >  (bi)); return 0;                 \
                case MLN_VOP_GE:  VPUSH_BOOL((ai) >= (bi)); return 0;                 \
                case MLN_VOP_EQ:  VPUSH_BOOL((ai) == (bi)); return 0;                 \
                case MLN_VOP_NE:  VPUSH_BOOL((ai) != (bi)); return 0;                 \
                default: break;                                                       \
            }

#define VM_BINOP_LL(opcode, base_op)                                                  \
        VM_CASE(opcode): {                                                            \
            mln_lang_var_t *lv = frame->slots[insn.a];                                \
            mln_lang_var_t *rv = frame->slots[(mln_u8_t)(insn.b & 0xff)];             \
            if (lv->val != NULL && rv->val != NULL &&                                 \
                lv->val->type == M_LANG_VAL_TYPE_INT &&                               \
                rv->val->type == M_LANG_VAL_TYPE_INT &&                               \
                !ctx->op_int_flag) {                                                  \
                mln_s64_t ai = lv->val->data.i;                                       \
                mln_s64_t bi = rv->val->data.i;                                       \
                VM_INT_BINOP_BODY(base_op, ai, bi)                                    \
            }                                                                         \
            ++(lv->ref); ++(rv->ref);                                                 \
            mln_lang_var_t *r = apply_binop(ctx, base_op, lv, rv);                    \
            if (r == NULL) return -1;                                                 \
            VPUSH_VAR(r);                                                             \
            return 0;                                                                 \
        }

#define VM_BINOP_LI(opcode, base_op)                                                  \
        VM_CASE(opcode): {                                                            \
            mln_lang_var_t *lv = frame->slots[insn.a];                                \
            mln_s64_t imm     = chunk->iconsts[(mln_u16_t)insn.b];                    \
            if (lv->val != NULL && lv->val->type == M_LANG_VAL_TYPE_INT &&            \
                !ctx->op_int_flag) {                                                  \
                mln_s64_t ai = lv->val->data.i;                                       \
                VM_INT_BINOP_BODY(base_op, ai, imm)                                   \
            }                                                                         \
            ++(lv->ref);                                                              \
            mln_lang_var_t *rv = mln_lang_var_create_int(ctx, imm, NULL);             \
            if (rv == NULL) { mln_lang_var_free(lv); return -1; }                     \
            mln_lang_var_t *r = apply_binop(ctx, base_op, lv, rv);                    \
            if (r == NULL) return -1;                                                 \
            VPUSH_VAR(r);                                                             \
            return 0;                                                                 \
        }

        VM_BINOP_LL(MLN_VOP_ADD_LL, MLN_VOP_ADD)
        VM_BINOP_LL(MLN_VOP_SUB_LL, MLN_VOP_SUB)
        VM_BINOP_LL(MLN_VOP_MUL_LL, MLN_VOP_MUL)
        VM_BINOP_LL(MLN_VOP_LT_LL,  MLN_VOP_LT)
        VM_BINOP_LL(MLN_VOP_LE_LL,  MLN_VOP_LE)
        VM_BINOP_LL(MLN_VOP_GT_LL,  MLN_VOP_GT)
        VM_BINOP_LL(MLN_VOP_GE_LL,  MLN_VOP_GE)
        VM_BINOP_LL(MLN_VOP_EQ_LL,  MLN_VOP_EQ)
        VM_BINOP_LL(MLN_VOP_NE_LL,  MLN_VOP_NE)

        VM_BINOP_LI(MLN_VOP_ADD_LI, MLN_VOP_ADD)
        VM_BINOP_LI(MLN_VOP_SUB_LI, MLN_VOP_SUB)
        VM_BINOP_LI(MLN_VOP_MUL_LI, MLN_VOP_MUL)
        VM_BINOP_LI(MLN_VOP_LT_LI,  MLN_VOP_LT)
        VM_BINOP_LI(MLN_VOP_LE_LI,  MLN_VOP_LE)
        VM_BINOP_LI(MLN_VOP_GT_LI,  MLN_VOP_GT)
        VM_BINOP_LI(MLN_VOP_GE_LI,  MLN_VOP_GE)
        VM_BINOP_LI(MLN_VOP_EQ_LI,  MLN_VOP_EQ)
        VM_BINOP_LI(MLN_VOP_NE_LI,  MLN_VOP_NE)

#undef VM_BINOP_LL
#undef VM_BINOP_LI
#undef VM_INT_BINOP_BODY

        VM_CASE(MLN_VOP_RETURN_LOCAL): {
            /* Fused LOAD_LOCAL + RETURN: return slot[a]'s value. The
             * caller's opstack receives a stable owned var (we bump ref
             * so the var survives the slot-table teardown that
             * vm_pop_frame_with_ret triggers). */
            mln_lang_var_t *sv = frame->slots[insn.a];
            ++(sv->ref);
            return vm_pop_frame_with_ret(ctx, sv);
        }

        VM_CASE(MLN_VOP_DEAD_AST):
            mln_lang_errmsg(ctx, "VM: AST stack handler invoked (cutover violation).");
            return -1;
        VM_DEFAULT:
            mln_lang_errmsg(ctx, "VM: bad opcode.");
            return -1;
    VM_DISPATCH_END;
}

#undef VPUSH
#undef VPUSH_VAR
#undef VPUSH_BORROW
#undef VPUSH_INT
#undef VPUSH_BOOL
#undef VPUSH_NIL
#undef VPUSH_REAL
#undef VPOP
#undef VTOP_PTR

/* Watcher trigger: builds funccall, dispatches; for compiled EXTERNAL,
 * funccall_run_compat (via the run_handler hook in mln_lang.c) pushes
 * a new VM frame — we mark it as discard_ret. For INTERNAL prototypes
 * the call runs synchronously via funccall_run_compat. */
static int vm_fire_watcher(mln_lang_ctx_t *ctx, mln_lang_var_t *target_var)
{
    mln_lang_val_t *tval = target_var->val;
    if (tval == NULL || tval->func == NULL) return 0;

    mln_lang_funccall_val_t *call = mln_lang_funccall_val_new(ctx->pool, NULL);
    if (call == NULL) {
        mln_lang_errmsg(ctx, "No memory.");
        return -1;
    }
    call->prototype = tval->func;

    mln_lang_var_t *arg0 = mln_lang_var_new(ctx, NULL, M_LANG_VAR_REFER, tval, NULL);
    if (arg0 == NULL) { mln_lang_funccall_val_free(call); return -1; }
    if (mln_lang_funccall_val_add_arg(call, arg0) < 0) {
        mln_lang_var_free(arg0); mln_lang_funccall_val_free(call); return -1;
    }
    if (tval->udata != NULL) {
        mln_lang_var_t *arg1 = mln_lang_var_new(ctx, NULL, M_LANG_VAR_REFER, tval->udata, NULL);
        if (arg1 == NULL) { mln_lang_funccall_val_free(call); return -1; }
        if (mln_lang_funccall_val_add_arg(call, arg1) < 0) {
            mln_lang_var_free(arg1); mln_lang_funccall_val_free(call); return -1;
        }
    }

    mln_lang_vm_frame_t *saved_top = FRAME_TOP(ctx);
    mln_lang_stack_node_t *cur_top = ctx->run_stack_top;
    if (mln_lang_stack_handler_funccall_run_compat(ctx, cur_top, call) < 0) {
        mln_lang_funccall_val_free(call);
        return -1;
    }
    mln_lang_funccall_val_free(call);

    if (FRAME_TOP(ctx) != saved_top) {
        /* New frame pushed (compiled EXTERNAL watcher). Mark for ret discard. */
        FRAME_TOP(ctx)->discard_ret = 1;
        return 0;
    }
    /* Synchronous (INTERNAL watcher). Pop scope, drop ret. */
    if (mln_lang_withdraw_until_func_compat(ctx) < 0) return -1;
    if (ctx->ret_var != NULL) {
        mln_lang_var_free(ctx->ret_var);
        ctx->ret_var = NULL;
    }
    return 0;
}

/* ====================================================================
 * Public entry points.
 * ==================================================================== */

/* Mark vm_step as a hot function so the compiler aligns it favourably,
 * keeps it warm in i-cache, and prefers to inline small callees. */
#if defined(__GNUC__) || defined(__clang__)
__attribute__((hot))
#endif
int mln_lang_vm_step(mln_lang_ctx_t *ctx, int budget)
{
    /* Hot loop. Two changes vs the naive `while (budget-- > 0 && ...)`:
     *   1. The budget compare runs only every 16 dispatches via a mask,
     *      saving one branch on the dispatch fast path. The mask check
     *      itself is a single AND + jump-on-zero.
     *   2. ctx->ref is still checked every iteration — it's the
     *      cooperative yield signal set by mln_lang_ctx_suspend during
     *      async INTERNAL calls. Delaying detection would let the VM
     *      execute stale instructions on a suspended frame.
     * The budget granularity is 16 instructions; on a script that runs
     * exactly N instructions and yields, we may overshoot by up to 15
     * before returning. The driver loop in mln_lang.c calls vm_step
     * repeatedly until a frame pops or ctx->ref fires, so overshoot
     * is harmless. */
    int i = 0;
    while (FRAME_TOP(ctx) != NULL) {
        if (dispatch_one(ctx) < 0) return -1;
        if (ctx->ref) return 0;
        if ((++i & 15) == 0 && i >= budget) return 0;
    }
    return 1;
}

/* Synchronous: push a frame for `prototype` and run until it pops.
 * Used by the funccall_run_compat hook in mln_lang.c when callers
 * want a synchronous semantics (e.g., currently no longer used now
 * that the hook itself pushes a frame; kept for backward source
 * compatibility). */
int mln_lang_vm_run(mln_lang_ctx_t *ctx, mln_lang_func_detail_t *prototype)
{
    mln_lang_vm_chunk_t *chunk = (mln_lang_vm_chunk_t *)prototype->vm_chunk;
    if (chunk == NULL) return -1;
    int n_args   = (int)mln_array_nelts(&(prototype->args));
    int n_closures = (int)mln_array_nelts(&(prototype->closure));
    mln_lang_vm_frame_t *boundary = FRAME_TOP(ctx);

    if (vm_push_frame(ctx, chunk, prototype, n_args, n_closures, 0, 0) < 0) return -1;

    while (FRAME_TOP(ctx) != boundary) {
        if (dispatch_one(ctx) < 0) return -1;
    }
    return 0;
}

int mln_lang_vm_push_frame_for_call(mln_lang_ctx_t *ctx, mln_lang_func_detail_t *prototype)
{
    mln_lang_vm_chunk_t *chunk = (mln_lang_vm_chunk_t *)prototype->vm_chunk;
    if (chunk == NULL) return -1;
    int n_args     = (int)mln_array_nelts(&(prototype->args));
    int n_closures = (int)mln_array_nelts(&(prototype->closure));
    return vm_push_frame(ctx, chunk, prototype, n_args, n_closures, 0, 0);
}

/* Top-level entry. Compiles the script's top-level stm chain and
 * pushes the initial frame onto ctx->vm_frame_top. The caller
 * (mln_lang_run_handler) drives execution via mln_lang_vm_step. */
int mln_lang_vm_run_toplevel(mln_lang_ctx_t *ctx)
{
    if (ctx->stm == NULL) return 1;
    if (FRAME_TOP(ctx) != NULL) return 1;  /* already initialized */

    /* Pre-set op_int_flag if the script defines any __int_*_operator__
     * overload anywhere in its AST (including nested function bodies).
     * Without this pre-scan, a nested function compiled lazily — before
     * its enclosing script has executed past the overload definition —
     * would have safe_to_fold = 1 and emit folded LOAD_INT instead of
     * the runtime-dispatched ADD opcode that would later route through
     * the overload. The flag is sticky and idempotent: the existing
     * dynamic setter at definition time is a no-op once set.
     *
     * Also pre-set conservatively if the script contains any Eval() call
     * anywhere in its AST (including nested function bodies).  Melang's
     * Eval() always runs code in a NEW, isolated context, so it cannot
     * inject operator overloads into the calling context.  However, pre-
     * setting the flag is a harmless conservative guard: it only disables
     * compile-time constant folding for integer literals, and the emitted
     * ADD/SUB/... opcodes still return the correct result whether or not
     * an overload is active. */
    if (scan_stm_for_int_overload(ctx->stm) || scan_stm_for_eval_call(ctx->stm)) {
        ctx->op_int_flag = 1;
    }

    /* Heap-allocated synthetic prototype so it outlives this function
     * call and can be freed when the top-level frame is popped. */
    mln_lang_func_detail_t *proto =
        (mln_lang_func_detail_t *)mln_alloc_m(ctx->pool, sizeof(*proto));
    if (proto == NULL) return -1;
    memset(proto, 0, sizeof(*proto));
    proto->type = M_FUNC_EXTERNAL;
    proto->data.stm = ctx->stm;
    proto->vm_chunk = NULL;
    proto->vm_state = 0;

    int rc = mln_lang_vm_try_compile(ctx, proto);
    if (rc != 1) {
        if (proto->vm_chunk != NULL) mln_lang_vm_chunk_free((mln_lang_vm_chunk_t *)proto->vm_chunk);
        mln_alloc_free(proto);
        return (rc < 0) ? 0 : -1;
    }

    if (vm_push_frame(ctx, (mln_lang_vm_chunk_t *)proto->vm_chunk, proto,
                      0, 0, 1, 0) < 0)
    {
        mln_lang_vm_chunk_free((mln_lang_vm_chunk_t *)proto->vm_chunk);
        mln_alloc_free(proto);
        return -1;
    }
    return 1;
}
