# Melon: Stage-2 perf changes (no VM yet)

This is the bundle of source-level optimizations applied to the AST-walking
interpreter under `src/mln_lang.c`, before any bytecode/VM rewrite. The goal
was to see how far the existing interpreter could be pushed without
breaking event-driven scheduling, `Watch`, coroutines, operator overloads,
or the `lib_src/*.so` C-extension ABI.

## Bench

`fib(30)` recursive on the same Linux/aarch64 host:

| Engine            | mean wall (s) | speed-up |
| ----------------- | ------------- | -------- |
| Melang baseline   | 0.707         | 1.00×    |
| Melang optimized  | 0.59          | **1.20×** |
| Python 3          | 0.090         | (ref)    |

Melang is now ~6.5× slower than CPython on this benchmark (was ~7.8×). The
remaining gap is mostly inherent to walking a parse-tree with boxed,
ref-counted values; a bytecode VM is required to close it further (the
recommendation we already gave).

## What changed

### 1. Per-ctx free-lists for `mln_lang_val_t` and `mln_lang_var_t`

**Why.** The biggest cost was alloc/free churn — about 23 M slab calls
per `fib(30)` (`mln_alloc_m` + `mln_alloc_free` showed up at ~24 % of
total profile time). Each `i - 1`, each `+`, each call to a builtin
returned `true` allocates fresh val + var.

**How.** Added two free-list heads on `mln_lang_ctx_t`
(`val_freelist`, `var_freelist`) capped at 4096 entries each. The
existing unused `mln_lang_val_t::next` / `mln_lang_var_t::next` fields
are repurposed as the free-list link while the struct is on the list
(verified by audit — they were initialized to NULL but never read).
A `ctx` back-pointer was added so the free path knows which list to
push to. After the struct's owned resources have already been torn
down by `mln_lang_val_data_free` / the existing `__mln_lang_var_free`
body, the empty shell is returned to the list for reuse.

After the change, slab calls drop from 23 M to ~3 M for the same
benchmark — most of the val/var hot path now stays in the free-list.

### 2. Inline integer fast path in addsub / muldiv / relativehigh

**Why.** Even with the free-list, `i - 1` still went through:
methods-table dispatch → `mln_lang_int_plus` → type-check the second
operand → branch → `mln_lang_val_new` + `mln_lang_var_new`. That's
several function entries plus indirect calls per arithmetic op.

**How.** In each handler, before falling into the normal dispatch we
check for `int op int` with no operator-overload flag. If both
operands are `M_LANG_VAL_TYPE_INT`, we compute the result inline,
allocate the result via the same freelist-aware constructor, and skip
the methods table entirely. DIV / MOD bail to the slow path if the
divisor is zero so the script error message is preserved.

### 3. Cascade-fold for `if`-condition and `return`-expression

**Why.** A single-expression `if` condition or `return` paid for one
extra round-trip through `mln_lang_stack_handler_exp`, even though the
existing `exp->jump` already pointed directly at the deepest non-trivial
node in the cascade.

**How.** When the source-level expression has no comma chain
(`exp->next == NULL`), the handler now pushes `exp->type` / `exp->jump`
directly instead of `M_LSNT_EXP`. The pop in step 1 doesn't care which
node type was on top — same flow, one fewer dispatch per `if` / per
`return`.

### 4. Defensive: clear `sym->bucket` in `mln_lang_symbol_node_free`

Not a perf change in itself; it lets future cache work (an inline cache
on `mln_lang_factor_t::cached_sym`, for example) detect freed slots
cheaply. Cost is one extra store on the symbol-free path; the slot was
already going to the per-ctx symbol freelist for reuse.

## Cross-platform notes

All changes use plain C with no GCC-only constructs (no
statement-expressions, no `__thread`, no inline asm). The pre-existing
`#if defined(MSVC)` branches in `mln_lang.c` are untouched, so the MSVC
function-form of `mln_lang_stack_push` keeps working unchanged.

The new struct fields (`mln_lang_val_t::ctx`, `mln_lang_var_t::ctx`,
free-list members on `mln_lang_ctx_t`) **change the binary layout of
those types**. Anything dynamically loading against the new headers
must be **rebuilt** — concretely, every `.so` under
`Melang/lib_src/*` needs to be recompiled together with the updated
`libmelon_static.a`. (We hit this during testing — stale `.so`s cause
prompt segfaults inside `sys.print` because the lib's compiled
struct offsets disagree with `libmelon`'s.)

## Verification

* `Melon/t/lang`, `t/array`, `t/hash`, `t/rbtree`, `t/string`, `t/conf`
  C-level tests all pass against the optimized library.
* `Melang/melang_smoke_tests.m` covers arithmetic, comparisons, control
  flow, recursion, loops, arrays, strings, closures, refs, Set/objects,
  Watch/Unwatch, and Eval — all assertions pass.
* `fib(30)` returns 832040, identical to baseline.
