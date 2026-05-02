
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

typedef struct {
    /* For each enclosing loop (Phase C): the pc of the start of the loop
     * body's "continue" landing pad (for `continue`, jump here; for `for`
     * loops this is the mod_exp position), and a list of `break` jump
     * patches we need to fix up to point at the post-loop instruction. */
    int continue_pc;
    int breaks[16];
    int n_breaks;
    /* For `for` loops: continue_pc is not yet known when compiling the body
     * (it equals the mod_exp pc, which comes after the body).  Collect JUMP
     * instruction indices here and patch them once mod_pc is known. */
    int continues[16];
    int n_continues;
} loop_ctx_t;

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

    /* Phase F4: labels and goto patches for goto/M_STM_LABEL. */
    struct {
        mln_string_t *name;
        int           pc;
    } labels[32];
    int n_labels;
    struct {
        mln_string_t *name;
        int           patch_pc;
    } goto_patches[32];
    int n_goto_patches;
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

/* Allocate a fresh local slot; used when an assignment introduces a new
 * variable. Returns the slot index or -1 if we hit MLN_VM_MAX_LOCALS. */
static int alloc_local_slot(mln_lang_vm_compiler_t *c, mln_string_t *name)
{
    if (c->n_locals >= MLN_VM_MAX_LOCALS) return -1;
    int slot = (int)c->n_locals;
    c->local_names[slot] = name;
    c->n_locals++;
    return slot;
}

static void bail(mln_lang_vm_compiler_t *c) { c->ok = 0; }

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

/* Returns slot (or allocates) when the LHS is a plain identifier; -1 means
 * the LHS is something more complex (locate chain) and the caller should
 * use the chain-aware path instead. */
static int extract_lhs_local(mln_lang_vm_compiler_t *c, mln_lang_logiclow_t *lhs)
{
    mln_lang_locate_t *lc = unwrap_to_locate(lhs);
    if (lc == NULL || lc->op != M_LOCATE_NONE || lc->next != NULL) return -1;
    mln_lang_spec_t *sp = lc->left;
    if (sp == NULL || sp->op != M_SPEC_FACTOR) return -1;
    mln_lang_factor_t *fac = sp->data.factor;
    if (fac == NULL || fac->type != M_FACTOR_ID) return -1;
    int slot = find_local_slot(c, fac->data.s_id);
    if (slot < 0) {
        slot = alloc_local_slot(c, fac->data.s_id);
        if (slot < 0) { bail(c); return -1; }
    }
    return slot;
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
            if (c->n_labels >= 32) { bail(c); return; }
            c->labels[c->n_labels].name = stm->data.pos;
            c->labels[c->n_labels].pc = (int)c->chunk->code_len;
            c->n_labels++;
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

            mln_lang_switchstm_t *cases[64];
            int compare_jumps[64];     /* JIT_TRUE patches for non-default */
            int n_cases = 0;
            int default_idx = -1;
            for (mln_lang_switchstm_t *sst = sw->switchstm; sst != NULL; sst = sst->next) {
                if (n_cases >= 64) { bail(c); return; }
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
            lc->continue_pc = -1;   /* continue not supported in switch */
            lc->n_breaks = 0;
            lc->n_continues = 0;

            /* Emit each case body in order. Patch its JIT_TRUE to here. */
            int body_pcs[64];
            for (int i = 0; i < n_cases; ++i) {
                body_pcs[i] = (int)c->chunk->code_len;
                if (cases[i]->factor != NULL) {
                    c->chunk->code[compare_jumps[i]].b =
                        (mln_s16_t)(body_pcs[i] - (compare_jumps[i] + 1));
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
            c->chunk->code[j_no_match].b = (mln_s16_t)(no_match_target - (j_no_match + 1));

            /* Patch break jumps from inside any case body to end_pc. */
            for (int i = 0; i < lc->n_breaks; ++i) {
                c->chunk->code[lc->breaks[i]].b = (mln_s16_t)(end_pc - (lc->breaks[i] + 1));
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
            lc->continue_pc = loop_start;
            lc->n_breaks = 0;
            lc->n_continues = 0;

            /* condition (or empty for `while (1)` style) */
            int sp_before = c->sp;
            if (w->condition != NULL) {
                compile_exp(c, w->condition);
                if (!c->ok) { c->n_loops--; return; }
                if (c->sp != sp_before + 1) { bail(c); c->n_loops--; return; }
                /* JIF_FALSE → exit */
                lc->breaks[lc->n_breaks++] = emit(c, MLN_VOP_JUMP_IF_FALSE, 0, 0);
                sp_pop(c, 1);
            }

            /* body */
            if (w->blockstm != NULL) compile_block(c, w->blockstm);
            if (!c->ok) { c->n_loops--; return; }

            /* jump back to start */
            int j_back = emit(c, MLN_VOP_JUMP, 0, 0);
            c->chunk->code[j_back].b = (mln_s16_t)(loop_start - (j_back + 1));

            /* patch breaks */
            int after = (int)c->chunk->code_len;
            for (int i = 0; i < lc->n_breaks; ++i) {
                c->chunk->code[lc->breaks[i]].b = (mln_s16_t)(after - (lc->breaks[i] + 1));
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
            lc->n_breaks = 0;
            lc->n_continues = 0;
            lc->continue_pc = -1;   /* for-loop: mod_exp pc not yet known */

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
                c->chunk->code[lc->continues[i]].b =
                    (mln_s16_t)(mod_pc - (lc->continues[i] + 1));
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
            c->chunk->code[j_back].b = (mln_s16_t)(cond_pc - (j_back + 1));

            /* exit landing */
            int after = (int)c->chunk->code_len;
            if (j_exit >= 0) {
                c->chunk->code[j_exit].b = (mln_s16_t)(after - (j_exit + 1));
            }
            for (int i = 0; i < lc->n_breaks; ++i) {
                c->chunk->code[lc->breaks[i]].b = (mln_s16_t)(after - (lc->breaks[i] + 1));
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
                c->chunk->code[j].b = (mln_s16_t)(found - (j + 1));
            } else {
                if (c->n_goto_patches >= 32) { bail(c); return; }
                int j = emit(c, MLN_VOP_JUMP, 0, 0);
                c->goto_patches[c->n_goto_patches].name = target;
                c->goto_patches[c->n_goto_patches].patch_pc = j;
                c->n_goto_patches++;
            }
            return;
        }
        case M_BLOCK_BREAK: {
            if (c->n_loops == 0) { bail(c); return; }
            loop_ctx_t *lc = &c->loops[c->n_loops - 1];
            if (lc->n_breaks >= 16) { bail(c); return; }
            lc->breaks[lc->n_breaks++] = emit(c, MLN_VOP_JUMP, 0, 0);
            return;
        }
        case M_BLOCK_CONTINUE: {
            if (c->n_loops == 0) { bail(c); return; }
            loop_ctx_t *lc = &c->loops[c->n_loops - 1];
            if (lc->continue_pc >= 0) {
                /* while loop: continue_pc is already known */
                int j = emit(c, MLN_VOP_JUMP, 0, 0);
                c->chunk->code[j].b = (mln_s16_t)(lc->continue_pc - (j + 1));
            } else {
                /* for loop: continue_pc not yet known — record patch index */
                if (lc->n_continues >= 16) { bail(c); return; }
                int j = emit(c, MLN_VOP_JUMP, 0, 0);
                lc->continues[lc->n_continues++] = j;
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
                c->chunk->code[jf].b = (mln_s16_t)(else_target - (jf + 1));
                compile_block(c, iff->elsestm);
                if (!c->ok) return;
                int end_target = (int)c->chunk->code_len;
                c->chunk->code[j_end].b = (mln_s16_t)(end_target - (j_end + 1));
            } else {
                c->chunk->code[jf].b = (mln_s16_t)(after_then - (jf + 1));
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

    /* Local assign: simple slot. */
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

    /* Locate-chain target: walk down. Phase E: only plain `=` for chain
     * targets (no compound). */
    if (compound_op >= 0) { bail(c); return; }

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

    /* Final hop is the assignment target. */
    if (locate->op == M_LOCATE_PROPERTY) {
        int idx = add_sconst(c, locate->right.id);
        if (idx < 0 || idx > 32767) { bail(c); return; }
        int sp_before = c->sp;
        compile_assign(c, a->right);
        if (!c->ok) return;
        if (c->sp != sp_before + 1) { bail(c); return; }
        emit(c, MLN_VOP_SET_PROPERTY, 0, (mln_s16_t)idx);
        sp_pop(c, 2);   /* pop val and obj */
    } else if (locate->op == M_LOCATE_INDEX) {
        if (locate->right.exp == NULL) { bail(c); return; }
        int sp_before = c->sp;
        compile_exp(c, locate->right.exp);
        if (!c->ok) return;
        if (c->sp != sp_before + 1) { bail(c); return; }
        sp_before = c->sp;
        compile_assign(c, a->right);
        if (!c->ok) return;
        if (c->sp != sp_before + 1) { bail(c); return; }
        emit(c, MLN_VOP_SET_INDEX, 0, 0);
        sp_pop(c, 3);   /* pop val, key, arr */
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
        c->chunk->code[j_short].b = (mln_s16_t)((int)c->chunk->code_len - (j_short + 1));
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
        switch (n->op) {
            case M_LOGICHIGH_OR:  emit(c, MLN_VOP_BOR,  0, 0); break;
            case M_LOGICHIGH_AND: emit(c, MLN_VOP_BAND, 0, 0); break;
            case M_LOGICHIGH_XOR: emit(c, MLN_VOP_BXOR, 0, 0); break;
            default: bail(c); return;
        }
        sp_pop(c, 1);   /* binary op pops 2, pushes 1 */
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
        switch (n->op) {
            case M_MOVE_LMOVE: emit(c, MLN_VOP_LSHIFT, 0, 0); break;
            case M_MOVE_RMOVE: emit(c, MLN_VOP_RSHIFT, 0, 0); break;
            default: bail(c); return;
        }
        sp_pop(c, 1);
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
        switch (n->op) {
            case M_ADDSUB_PLUS: emit(c, MLN_VOP_ADD, 0, 0); break;
            case M_ADDSUB_SUB:  emit(c, MLN_VOP_SUB, 0, 0); break;
            default: bail(c); return;
        }
        sp_pop(c, 1);
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
        switch (n->op) {
            case M_MULDIV_MUL: emit(c, MLN_VOP_MUL, 0, 0); break;
            case M_MULDIV_DIV: emit(c, MLN_VOP_DIV, 0, 0); break;
            case M_MULDIV_MOD: emit(c, MLN_VOP_MOD, 0, 0); break;
            default: bail(c); return;
        }
        sp_pop(c, 1);
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
    /* `i++` / `i--`: only support when LHS is a local-id. */
    mln_lang_locate_t *lc = n->left;
    if (lc == NULL || lc->op != M_LOCATE_NONE) { bail(c); return; }
    mln_lang_spec_t *sp = lc->left;
    if (sp == NULL || sp->op != M_SPEC_FACTOR) { bail(c); return; }
    mln_lang_factor_t *f = sp->data.factor;
    if (f == NULL || f->type != M_FACTOR_ID) { bail(c); return; }
    int slot = find_local_slot(c, f->data.s_id);
    if (slot < 0) { bail(c); return; }
    switch (n->op) {
        case M_SUFFIX_INC: emit(c, MLN_VOP_LOAD_LOCAL_INC, (mln_u8_t)slot, 0); break;
        case M_SUFFIX_DEC: emit(c, MLN_VOP_LOAD_LOCAL_DEC, (mln_u8_t)slot, 0); break;
        default: bail(c); return;
    }
    sp_push(c, 1);
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
                    /* Keep a copy of obj so CALL_METHOD can bind `this`. */
                    emit(c, MLN_VOP_DUP, 0, 0);
                    sp_push(c, 1);
                }
                emit(c, MLN_VOP_GET_PROPERTY, 0, (mln_s16_t)idx);
                /* GET_PROPERTY: pop obj, push prop val (sp unchanged) */
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
                if (is_method) {
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
        case M_SPEC_INC: {
            /* Prefix ++x : pre-increment local. */
            mln_lang_spec_t *inner = n->data.spec;
            if (inner == NULL || inner->op != M_SPEC_FACTOR) { bail(c); return; }
            mln_lang_factor_t *f = inner->data.factor;
            if (f == NULL || f->type != M_FACTOR_ID) { bail(c); return; }
            int slot = find_local_slot(c, f->data.s_id);
            if (slot < 0) { bail(c); return; }
            emit(c, MLN_VOP_INC_LOCAL_LOAD, (mln_u8_t)slot, 0);
            sp_push(c, 1);
            return;
        }
        case M_SPEC_DEC: {
            mln_lang_spec_t *inner = n->data.spec;
            if (inner == NULL || inner->op != M_SPEC_FACTOR) { bail(c); return; }
            mln_lang_factor_t *f = inner->data.factor;
            if (f == NULL || f->type != M_FACTOR_ID) { bail(c); return; }
            int slot = find_local_slot(c, f->data.s_id);
            if (slot < 0) { bail(c); return; }
            emit(c, MLN_VOP_DEC_LOCAL_LOAD, (mln_u8_t)slot, 0);
            sp_push(c, 1);
            return;
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
int
mln_lang_vm_try_compile(mln_lang_ctx_t *ctx, mln_lang_func_detail_t *prototype)
{
    if (prototype == NULL) return -1;
    if (prototype->type != M_FUNC_EXTERNAL) return -1;
    if (prototype->data.stm == NULL) return -1;

    mln_size_t n_args = mln_array_nelts(&(prototype->args));
    if (n_args > MLN_VM_MAX_LOCALS) return -1;

    /* Phase D: refuse if the script likely uses operator overload — any of
     * the op_*_flag bits would imply user-defined arithmetic redirection,
     * and our int-fast paths would skip those overrides. We could be more
     * precise per-prototype, but this conservative gate is correct. */
    if (ctx->op_int_flag || ctx->op_bool_flag || ctx->op_real_flag ||
        ctx->op_str_flag || ctx->op_array_flag || ctx->op_obj_flag ||
        ctx->op_func_flag || ctx->op_nil_flag) {
        return -1;
    }

    mln_lang_vm_compiler_t c;
    memset(&c, 0, sizeof(c));
    c.ctx       = ctx;
    c.prototype = prototype;
    c.ok        = 1;

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
            c.chunk->code[patch_pc].b = (mln_s16_t)(target - (patch_pc + 1));
        }
    }

    if (c.ok) emit(&c, MLN_VOP_RETURN_NIL, 0, 0);

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
    prototype->vm_chunk = c.chunk;
    if (getenv("MELANG_VM_TRACE")) {
        fprintf(stderr, "[vm] compiled chunk: insns=%zu locals=%zu max_stack=%zu\n",
                (size_t)c.chunk->code_len, (size_t)c.n_locals, (size_t)c.chunk->max_stack);
    }
    return 1;
}

/* ====================================================================
 * VM runtime.
 * ==================================================================== */

static mln_lang_var_t *apply_binop(mln_lang_ctx_t *ctx, mln_u8_t op,
                                    mln_lang_var_t *a, mln_lang_var_t *b)
{
    if (a->val != NULL && b->val != NULL &&
        a->val->type == M_LANG_VAL_TYPE_INT &&
        b->val->type == M_LANG_VAL_TYPE_INT)
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
    mln_lang_var_t            **opstack;
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
    mln_lang_var_t **nbuf = (mln_lang_var_t **)mln_alloc_m(ctx->pool, sizeof(mln_lang_var_t *) * new_cap);
    if (nbuf == NULL) return -1;
    if (f->op_sp > 0 && f->opstack != NULL) {
        memcpy(nbuf, f->opstack, sizeof(mln_lang_var_t *) * f->op_sp);
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
        mln_lang_var_t **saved_opstack   = f->opstack;
        int              saved_op_cap    = f->op_cap;
        mln_lang_var_t **saved_slots     = f->slots;
        int              saved_slots_cap = f->slots_cap;
        memset(f, 0, sizeof(*f));
        f->opstack    = saved_opstack;
        f->op_cap     = saved_op_cap;
        f->slots      = saved_slots;
        f->slots_cap  = saved_slots_cap;
        /* Grow opstack if the new call needs more stack depth. */
        if (f->op_cap < new_op_cap) {
            if (f->opstack) mln_alloc_free(f->opstack);
            f->opstack = (mln_lang_var_t **)mln_alloc_m(ctx->pool,
                             sizeof(mln_lang_var_t *) * new_op_cap);
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
            f->opstack = (mln_lang_var_t **)mln_alloc_m(ctx->pool,
                             sizeof(mln_lang_var_t *) * new_op_cap);
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
        if (nv == NULL) goto fail;
        if (lname != NULL) {
            if (mln_lang_symbol_node_join(ctx, M_LANG_SYMBOL_VAR, nv) < 0) {
                mln_lang_var_free(nv);
                goto fail;
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
        mln_lang_var_free(f->opstack[--f->op_sp]);
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
        prev->opstack[prev->op_sp++] = ret;
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

#define PUSH(v) do { \
    if (vm_frame_grow_opstack(ctx, frame, 1) < 0) return -1; \
    frame->opstack[frame->op_sp++] = (v); \
} while (0)
#define POP()   (frame->opstack[--frame->op_sp])
#define TOP()   (frame->opstack[frame->op_sp - 1])

/* dispatch_one: execute one bytecode instruction on FRAME_TOP(ctx).
 * Returns 0 on success (frame may have changed via push/pop), -1 on
 * error.
 *
 * Marked always-inline so the compiler physically places the dispatch
 * switch directly inside vm_step's hot loop, eliminating the call/ret
 * overhead per instruction. */
#if defined(__GNUC__) || defined(__clang__)
# define MLN_VM_ALWAYS_INLINE __attribute__((always_inline))
#elif defined(_MSC_VER)
# define MLN_VM_ALWAYS_INLINE __forceinline
#else
# define MLN_VM_ALWAYS_INLINE
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
        frame->opstack[frame->op_sp++] = resume_ret;
        return 0;
    }

    mln_lang_vm_chunk_t *chunk = frame->chunk;
    mln_lang_vm_insn_t insn = chunk->code[frame->pc++];

    switch (insn.op) {
        case MLN_VOP_NOP:
            return 0;
        case MLN_VOP_POP:
            mln_lang_var_free(POP());
            return 0;
        case MLN_VOP_DUP: {
            mln_lang_var_t *t = TOP();
            ++(t->ref);
            PUSH(t);
            return 0;
        }
        case MLN_VOP_LOAD_NIL: {
            mln_lang_var_t *v = mln_lang_var_create_nil(ctx, NULL);
            if (v == NULL) return -1;
            PUSH(v);
            return 0;
        }
        case MLN_VOP_LOAD_TRUE: {
            mln_lang_var_t *v = mln_lang_var_create_bool(ctx, 1, NULL);
            if (v == NULL) return -1;
            PUSH(v);
            return 0;
        }
        case MLN_VOP_LOAD_FALSE: {
            mln_lang_var_t *v = mln_lang_var_create_bool(ctx, 0, NULL);
            if (v == NULL) return -1;
            PUSH(v);
            return 0;
        }
        case MLN_VOP_LOAD_INT: {
            mln_lang_var_t *v = mln_lang_var_create_int(ctx, chunk->iconsts[insn.b], NULL);
            if (v == NULL) return -1;
            PUSH(v);
            return 0;
        }
        case MLN_VOP_LOAD_REAL: {
            mln_lang_var_t *v = mln_lang_var_create_real(ctx, chunk->rconsts[insn.b], NULL);
            if (v == NULL) return -1;
            PUSH(v);
            return 0;
        }
        case MLN_VOP_LOAD_STRING: {
            mln_lang_var_t *v = mln_lang_var_create_string(ctx, chunk->sconsts[insn.b], NULL);
            if (v == NULL) return -1;
            PUSH(v);
            return 0;
        }
        case MLN_VOP_LOAD_LOCAL: {
            mln_lang_var_t *v = frame->slots[insn.a];
            ++(v->ref);
            PUSH(v);
            return 0;
        }
        case MLN_VOP_LOAD_GLOBAL: {
            mln_string_t *name = chunk->sconsts[insn.b];
            mln_lang_symbol_node_t *sym = mln_lang_symbol_node_search(ctx, name, 0);
            if (sym == NULL || sym->type != M_LANG_SYMBOL_VAR) {
                mln_lang_errmsg(ctx, "Undefined identifier.");
                return -1;
            }
            mln_lang_var_t *v = sym->data.var;
            ++(v->ref);
            PUSH(v);
            return 0;
        }
        case MLN_VOP_STORE_LOCAL:
        case MLN_VOP_ASSIGN_LOCAL: {
            mln_lang_var_t *v = POP();
            if (slot_assign(ctx, frame->slots[insn.a], v) < 0) return -1;
            if (vm_fire_watcher(ctx, frame->slots[insn.a]) < 0) return -1;
            if (insn.op == MLN_VOP_ASSIGN_LOCAL) {
                mln_lang_var_t *back = frame->slots[insn.a];
                ++(back->ref);
                /* Note: vm_fire_watcher may have pushed a new frame; we
                 * still push to OUR frame's opstack since the watcher
                 * frame's ret will be discarded. */
                if (vm_frame_grow_opstack(ctx, frame, 1) < 0) return -1;
                frame->opstack[frame->op_sp++] = back;
            }
            return 0;
        }
        case MLN_VOP_LOAD_LOCAL_INC:
        case MLN_VOP_LOAD_LOCAL_DEC: {
            mln_lang_var_t *sv = frame->slots[insn.a];
            if (sv->val == NULL || sv->val->type != M_LANG_VAL_TYPE_INT) {
                mln_lang_errmsg(ctx, "Suffix ++/-- requires int.");
                return -1;
            }
            mln_s64_t old_i = sv->val->data.i;
            mln_lang_var_t *old_var = mln_lang_var_create_int(ctx, old_i, NULL);
            if (old_var == NULL) return -1;
            if (insn.op == MLN_VOP_LOAD_LOCAL_INC) sv->val->data.i = old_i + 1;
            else                                   sv->val->data.i = old_i - 1;
            if (vm_fire_watcher(ctx, sv) < 0) {
                mln_lang_var_free(old_var);
                return -1;
            }
            PUSH(old_var);
            return 0;
        }
        case MLN_VOP_INC_LOCAL_LOAD:
        case MLN_VOP_DEC_LOCAL_LOAD: {
            mln_lang_var_t *sv = frame->slots[insn.a];
            if (sv->val == NULL || sv->val->type != M_LANG_VAL_TYPE_INT) {
                mln_lang_errmsg(ctx, "Prefix ++/-- requires int.");
                return -1;
            }
            mln_s64_t old_i = sv->val->data.i;
            if (insn.op == MLN_VOP_INC_LOCAL_LOAD) sv->val->data.i = old_i + 1;
            else                                   sv->val->data.i = old_i - 1;
            if (vm_fire_watcher(ctx, sv) < 0) return -1;
            mln_lang_var_t *new_var = mln_lang_var_create_int(ctx, sv->val->data.i, NULL);
            if (new_var == NULL) return -1;
            PUSH(new_var);
            return 0;
        }
        case MLN_VOP_NOT: {
            mln_lang_var_t *t = POP();
            int truthy = mln_lang_condition_is_true(t);
            mln_lang_var_free(t);
            mln_lang_var_t *r = mln_lang_var_create_bool(ctx, (mln_u8_t)!truthy, NULL);
            if (r == NULL) return -1;
            PUSH(r);
            return 0;
        }
        case MLN_VOP_NEG: {
            mln_lang_var_t *t = POP();
            if (t->val != NULL && t->val->type == M_LANG_VAL_TYPE_INT) {
                mln_lang_var_t *r = mln_lang_var_create_int(ctx, -t->val->data.i, NULL);
                mln_lang_var_free(t);
                if (r == NULL) return -1;
                PUSH(r);
            } else {
                mln_lang_method_t *method = (t->val != NULL) ? mln_lang_methods[t->val->type] : NULL;
                mln_lang_op h = method ? method->negative_handler : NULL;
                mln_lang_var_t *r = NULL;
                if (h == NULL || h(ctx, &r, t, NULL) < 0) {
                    mln_lang_var_free(t);
                    mln_lang_errmsg(ctx, "Operation NOT support.");
                    return -1;
                }
                mln_lang_var_free(t);
                PUSH(r);
            }
            return 0;
        }
        case MLN_VOP_ADD: case MLN_VOP_SUB: case MLN_VOP_MUL:
        case MLN_VOP_DIV: case MLN_VOP_MOD:
        case MLN_VOP_LT: case MLN_VOP_LE: case MLN_VOP_GT:
        case MLN_VOP_GE: case MLN_VOP_EQ: case MLN_VOP_NE:
        case MLN_VOP_BOR: case MLN_VOP_BAND: case MLN_VOP_BXOR:
        case MLN_VOP_LSHIFT: case MLN_VOP_RSHIFT: {
            mln_lang_var_t *b = POP();
            mln_lang_var_t *a = POP();
            mln_lang_var_t *r = apply_binop(ctx, insn.op, a, b);
            if (r == NULL) return -1;
            PUSH(r);
            return 0;
        }
        case MLN_VOP_JUMP:
            frame->pc = (mln_size_t)((mln_s64_t)frame->pc + insn.b);
            return 0;
        case MLN_VOP_JUMP_IF_FALSE: {
            mln_lang_var_t *cond = POP();
            int truthy = mln_lang_condition_is_true(cond);
            mln_lang_var_free(cond);
            if (!truthy) frame->pc = (mln_size_t)((mln_s64_t)frame->pc + insn.b);
            return 0;
        }
        case MLN_VOP_JUMP_IF_TRUE: {
            mln_lang_var_t *cond = POP();
            int truthy = mln_lang_condition_is_true(cond);
            mln_lang_var_free(cond);
            if (truthy) frame->pc = (mln_size_t)((mln_s64_t)frame->pc + insn.b);
            return 0;
        }
        case MLN_VOP_CALL_SELF:
        case MLN_VOP_CALL_VALUE:
        case MLN_VOP_CALL_METHOD: {
            int nargs = insn.a;
            int is_method = (insn.op == MLN_VOP_CALL_METHOD);
            int is_self   = (insn.op == MLN_VOP_CALL_SELF);

            mln_lang_var_t **args_base = &frame->opstack[frame->op_sp - nargs];
            mln_lang_var_t *func = NULL;
            mln_lang_var_t *obj  = NULL;
            mln_lang_func_detail_t *callee_proto = NULL;

            if (is_self) {
                callee_proto = frame->prototype;
                if (callee_proto == NULL) {
                    mln_lang_errmsg(ctx, "CALL_SELF: no current prototype.");
                    for (int i = 0; i < nargs; ++i) mln_lang_var_free(args_base[i]);
                    frame->op_sp -= nargs;
                    return -1;
                }
            } else {
                func = frame->opstack[frame->op_sp - nargs - 1];
                if (is_method) obj = frame->opstack[frame->op_sp - nargs - 2];
                if (func->val == NULL || func->val->type != M_LANG_VAL_TYPE_FUNC ||
                    func->val->data.func == NULL)
                {
                    mln_lang_errmsg(ctx, "Calling a non-function.");
                    for (int i = 0; i < nargs; ++i) mln_lang_var_free(args_base[i]);
                    mln_lang_var_free(func);
                    if (obj) mln_lang_var_free(obj);
                    frame->op_sp -= nargs + 1 + (is_method ? 1 : 0);
                    return -1;
                }
                callee_proto = func->val->data.func;
            }

            mln_lang_funccall_val_t *call = mln_lang_funccall_val_new(ctx->pool, NULL);
            if (call == NULL) {
                for (int i = 0; i < nargs; ++i) mln_lang_var_free(args_base[i]);
                if (func) mln_lang_var_free(func);
                if (obj)  mln_lang_var_free(obj);
                frame->op_sp -= nargs + (is_self ? 0 : 1) - (is_method ? -1 : 0);
                if (is_self) frame->op_sp -= 0;
                else frame->op_sp -= 1 + (is_method ? 1 : 0);
                return -1;
            }
            call->prototype = callee_proto;
            if (is_method && obj != NULL) {
                mln_lang_funccall_val_object_add(call, obj->val);
            }
            int add_arg_failed = 0;
            for (int i = 0; i < nargs; ++i) {
                if (mln_lang_funccall_val_add_arg(call, args_base[i]) < 0) {
                    add_arg_failed = 1;
                    for (int k = i; k < nargs; ++k) mln_lang_var_free(args_base[k]);
                    break;
                }
            }
            if (func) mln_lang_var_free(func);
            if (obj)  mln_lang_var_free(obj);
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
            PUSH(ret);
            return 0;
        }
        case MLN_VOP_RETURN: {
            mln_lang_var_t *r = POP();
            return vm_pop_frame_with_ret(ctx, r);
        }
        case MLN_VOP_RETURN_NIL: {
            return vm_pop_frame_with_ret(ctx, NULL);
        }
        case MLN_VOP_BIND_FUNC: {
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
        case MLN_VOP_BIND_SET: {
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
        case MLN_VOP_NEW_OBJECT: {
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
                mln_lang_var_t *nv = mln_lang_var_create_nil(ctx, NULL);
                if (nv == NULL) return -1;
                PUSH(nv); return 0;
            }
            mln_lang_object_t *obj_inst = mln_lang_object_new_compat(ctx, sym->data.set);
            if (obj_inst == NULL) return -1;
            mln_lang_val_t *ov = mln_lang_val_new(ctx, M_LANG_VAL_TYPE_OBJECT, obj_inst);
            if (ov == NULL) { mln_lang_object_free_compat(obj_inst); return -1; }
            mln_lang_var_t *ovar = mln_lang_var_new(ctx, NULL, M_LANG_VAR_NORMAL, ov, NULL);
            if (ovar == NULL) { mln_lang_val_free(ov); return -1; }
            PUSH(ovar);
            return 0;
        }
        case MLN_VOP_NEW_ARRAY: {
            mln_lang_array_t *arr = mln_lang_array_new(ctx);
            if (arr == NULL) return -1;
            mln_lang_val_t *av = mln_lang_val_new(ctx, M_LANG_VAL_TYPE_ARRAY, arr);
            if (av == NULL) { mln_lang_array_free(arr); return -1; }
            mln_lang_var_t *avar = mln_lang_var_new(ctx, NULL, M_LANG_VAR_NORMAL, av, NULL);
            if (avar == NULL) { mln_lang_val_free(av); return -1; }
            PUSH(avar);
            return 0;
        }
        case MLN_VOP_ARRAY_PUT: {
            mln_lang_var_t *valv = POP();
            mln_lang_var_t *keyv = POP();
            mln_lang_var_t *arrv = TOP();
            if (arrv->val == NULL || arrv->val->type != M_LANG_VAL_TYPE_ARRAY) {
                mln_lang_var_free(keyv); mln_lang_var_free(valv);
                mln_lang_errmsg(ctx, "ARRAY_PUT: top is not an array.");
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
        case MLN_VOP_GET_PROPERTY: {
            mln_lang_var_t *obj_op = POP();
            if (obj_op->val == NULL) {
                mln_lang_var_free(obj_op);
                mln_lang_errmsg(ctx, "Property access on nil.");
                return -1;
            }
            mln_lang_method_t *method = mln_lang_methods[obj_op->val->type];
            if (method == NULL || method->property_handler == NULL) {
                mln_lang_var_free(obj_op);
                mln_lang_errmsg(ctx, "Operation NOT support.");
                return -1;
            }
            mln_string_t *name = chunk->sconsts[insn.b];
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
            PUSH(res);
            return 0;
        }
        case MLN_VOP_GET_INDEX: {
            mln_lang_var_t *key = POP();
            mln_lang_var_t *arr = POP();
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
            PUSH(res);
            return 0;
        }
        case MLN_VOP_SET_PROPERTY: {
            mln_lang_var_t *val = POP();
            mln_lang_var_t *obj_op = POP();
            if (obj_op->val == NULL) {
                mln_lang_var_free(val); mln_lang_var_free(obj_op);
                mln_lang_errmsg(ctx, "Property assign on nil.");
                return -1;
            }
            mln_lang_method_t *method = mln_lang_methods[obj_op->val->type];
            if (method == NULL || method->property_handler == NULL) {
                mln_lang_var_free(val); mln_lang_var_free(obj_op);
                mln_lang_errmsg(ctx, "Operation NOT support.");
                return -1;
            }
            mln_string_t *name = chunk->sconsts[insn.b];
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
            if (vm_fire_watcher(ctx, slot_var) < 0) {
                mln_lang_var_free(slot_var); mln_lang_var_free(val); mln_lang_var_free(obj_op);
                return -1;
            }
            mln_lang_var_free(slot_var);
            mln_lang_var_free(val);
            mln_lang_var_free(obj_op);
            return 0;
        }
        case MLN_VOP_SET_INDEX: {
            mln_lang_var_t *val = POP();
            mln_lang_var_t *key = POP();
            mln_lang_var_t *arr = POP();
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
            if (vm_fire_watcher(ctx, slot_var) < 0) {
                mln_lang_var_free(slot_var); mln_lang_var_free(val);
                mln_lang_var_free(key); mln_lang_var_free(arr);
                return -1;
            }
            mln_lang_var_free(slot_var);
            mln_lang_var_free(val);
            mln_lang_var_free(key);
            mln_lang_var_free(arr);
            return 0;
        }
        case MLN_VOP_DEAD_AST:
            mln_lang_errmsg(ctx, "VM: AST stack handler invoked (cutover violation).");
            return -1;
        default:
            mln_lang_errmsg(ctx, "VM: bad opcode.");
            return -1;
    }
}

#undef PUSH
#undef POP
#undef TOP

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

int mln_lang_vm_step(mln_lang_ctx_t *ctx, int budget)
{
    while (budget-- > 0 && FRAME_TOP(ctx) != NULL) {
        if (dispatch_one(ctx) < 0) return -1;
        if (ctx->ref) return 0;
    }
    return FRAME_TOP(ctx) == NULL ? 1 : 0;
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
