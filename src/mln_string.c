
/*
 * Copyright (C) Niklaus F.Schen.
 */

#include <stdio.h>
#include <stdint.h>
#include "mln_string.h"
#include "mln_func.h"


/*
 * SWAR (SIMD-Within-A-Register) word-at-a-time helpers.
 *
 * All helpers are 8-byte (64-bit) parallel.  They use unaligned memcpy to
 * load/store so there is no alignment requirement and no strict-aliasing
 * issue.  Every helper is wrapped with MLN_FUNC so it can be traced when
 * MLN_FUNC_FLAG is enabled, and reduces to a plain static inline function
 * otherwise.
 */

#define MLN_STR_LOMAGIC 0x0101010101010101ULL
#define MLN_STR_HIMAGIC 0x8080808080808080ULL

/* Byte index of the lowest-addressed byte that differs between two words. */
MLN_FUNC(static inline, int, mln_string_first_diff_byte, \
         (mln_u64_t w1, mln_u64_t w2), (w1, w2), \
{
    mln_u64_t diff = w1 ^ w2;
    /* Scan byte-by-byte; works for both little and big endian hosts. */
    int i;
    for (i = 0; i < 8; ++i) {
        if ((diff >> (i << 3)) & 0xFFULL) {
#if defined(__LITTLE_ENDIAN__) || (defined(__BYTE_ORDER__) && __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__)
            return i;
#else
            return 7 - i;
#endif
        }
    }
    return 8;
})

/* Word-parallel tolower: converts uppercase ASCII bytes to lowercase. */
MLN_FUNC(static inline, mln_u64_t, mln_string_swar_tolower, (mln_u64_t x), (x), {
    mln_u64_t heptets = x & 0x7F7F7F7F7F7F7F7FULL;
    mln_u64_t is_gt_Z = heptets + 0x2525252525252525ULL;  /* bit7 set iff heptet > 'Z' */
    mln_u64_t is_ge_A = heptets + 0x3F3F3F3F3F3F3F3FULL;  /* bit7 set iff heptet >= 'A' */
    mln_u64_t is_ascii = ~x & MLN_STR_HIMAGIC;            /* bit7 set iff byte was ASCII */
    mln_u64_t is_upper = is_ascii & is_ge_A & ~is_gt_Z;   /* bit7 set iff uppercase ASCII */
    return x | (is_upper >> 2);                           /* flip bit 5 on those bytes */
})

/* Word-parallel toupper: converts lowercase ASCII bytes to uppercase. */
MLN_FUNC(static inline, mln_u64_t, mln_string_swar_toupper, (mln_u64_t x), (x), {
    mln_u64_t heptets = x & 0x7F7F7F7F7F7F7F7FULL;
    mln_u64_t is_gt_z = heptets + 0x0505050505050505ULL;  /* bit7 set iff heptet > 'z' */
    mln_u64_t is_ge_a = heptets + 0x1F1F1F1F1F1F1F1FULL;  /* bit7 set iff heptet >= 'a' */
    mln_u64_t is_ascii = ~x & MLN_STR_HIMAGIC;
    mln_u64_t is_lower = is_ascii & is_ge_a & ~is_gt_z;
    return x & ~(is_lower >> 2);                          /* clear bit 5 on those bytes */
})

/*
 * mln_string_memcmp_core: hybrid comparator.
 *   - For short buffers (< MLN_STR_MEMCMP_SWAR_CUTOFF) we stay inline and
 *     walk 8 bytes at a time using SWAR.  This avoids the PLT / libc call
 *     overhead that dominates short-buffer memcmp.
 *   - For longer buffers we defer to libc memcmp, which on glibc is a
 *     hand-tuned SIMD routine that outperforms scalar SWAR.
 * Returns a byte-accurate negative/zero/positive like memcmp().
 */
#define MLN_STR_MEMCMP_SWAR_CUTOFF 64

MLN_FUNC(static inline, int, mln_string_memcmp_core, \
         (const mln_u8ptr_t p1, const mln_u8ptr_t p2, mln_u64_t n), (p1, p2, n), \
{
    if (n >= MLN_STR_MEMCMP_SWAR_CUTOFF) {
        return memcmp(p1, p2, n);
    }
    mln_u64_t i = 0;
    while (i + 8 <= n) {
        mln_u64_t w1, w2;
        memcpy(&w1, p1 + i, 8);
        memcpy(&w2, p2 + i, 8);
        if (w1 != w2) {
            int off = mln_string_first_diff_byte(w1, w2);
            return (int)p1[i + off] - (int)p2[i + off];
        }
        i += 8;
    }
    for (; i < n; ++i) {
        if (p1[i] != p2[i]) return (int)p1[i] - (int)p2[i];
    }
    return 0;
})

/*
 * ASCII case-insensitive memcmp core.  Walks 8 bytes at a time using SWAR
 * tolower so upper/lower differences collapse to zero.
 */
MLN_FUNC(static inline, int, mln_string_memcasecmp_core, \
         (const mln_u8ptr_t p1, const mln_u8ptr_t p2, mln_u64_t n), (p1, p2, n), \
{
    if (n >= MLN_STR_MEMCMP_SWAR_CUTOFF) {
        /* For long buffers: try to locate any mismatch quickly using libc
         * memcmp; if all bytes match, we are done.  Otherwise fall through
         * and do a careful case-insensitive scan to obtain the exact
         * ordering. */
        if (memcmp(p1, p2, n) == 0) return 0;
    }
    mln_u64_t i = 0;
    while (i + 8 <= n) {
        mln_u64_t w1, w2;
        memcpy(&w1, p1 + i, 8);
        memcpy(&w2, p2 + i, 8);
        mln_u64_t l1 = mln_string_swar_tolower(w1);
        mln_u64_t l2 = mln_string_swar_tolower(w2);
        if (l1 != l2) {
            int off = mln_string_first_diff_byte(l1, l2);
            mln_u8_t b1 = (mln_u8_t)(l1 >> (off << 3));
            mln_u8_t b2 = (mln_u8_t)(l2 >> (off << 3));
#if defined(__LITTLE_ENDIAN__) || (defined(__BYTE_ORDER__) && __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__)
            return (int)b1 - (int)b2;
#else
            /* first_diff_byte returns low-byte index; on BE recompute via index */
            (void)b1; (void)b2;
            return (int)p1[i + off] - (int)p2[i + off]; /* fallback */
#endif
        }
        i += 8;
    }
    for (; i < n; ++i) {
        mln_u8_t c1 = p1[i], c2 = p2[i];
        if (c1 >= 'A' && c1 <= 'Z') c1 = (mln_u8_t)(c1 | 0x20);
        if (c2 >= 'A' && c2 <= 'Z') c2 = (mln_u8_t)(c2 | 0x20);
        if (c1 != c2) return (int)c1 - (int)c2;
    }
    return 0;
})

/* Apply SWAR tolower over a range, in place. */
MLN_FUNC_VOID(static inline, void, mln_string_tolower_inplace, \
              (mln_u8ptr_t p, mln_u64_t n), (p, n), \
{
    mln_u64_t i = 0;
    while (i + 8 <= n) {
        mln_u64_t w;
        memcpy(&w, p + i, 8);
        w = mln_string_swar_tolower(w);
        memcpy(p + i, &w, 8);
        i += 8;
    }
    for (; i < n; ++i) {
        if (p[i] >= 'A' && p[i] <= 'Z') p[i] = (mln_u8_t)(p[i] | 0x20);
    }
})

/* Apply SWAR toupper over a range, in place. */
MLN_FUNC_VOID(static inline, void, mln_string_toupper_inplace, \
              (mln_u8ptr_t p, mln_u64_t n), (p, n), \
{
    mln_u64_t i = 0;
    while (i + 8 <= n) {
        mln_u64_t w;
        memcpy(&w, p + i, 8);
        w = mln_string_swar_toupper(w);
        memcpy(p + i, &w, 8);
        i += 8;
    }
    for (; i < n; ++i) {
        if (p[i] >= 'a' && p[i] <= 'z') p[i] = (mln_u8_t)(p[i] & ~0x20);
    }
})


static inline int *compute_prefix_function(const char *pattern, int m, int *buf);
static inline char *
kmp_string_match(char *text, const char *pattern, int text_len, int pattern_len) __NONNULL2(1,2);
static inline mln_string_t *mln_string_assign(char *s, mln_u32_t len);

#if defined(MLN_C99)
static inline int __tolower(int c) {
    if (c >= 'A' && c <= 'Z') {
        return c + ('a' - 'A');
    }
    return c;
}

int strncasecmp(const char *s1, const char *s2, size_t n) {
    unsigned char u1, u2;
    while (n-- > 0) {
        u1 = (unsigned char)__tolower(*s1++);
        u2 = (unsigned char)__tolower(*s2++);
        if (u1 != u2) {
            return u1 - u2;
        }
        if (u1 == '\0') {
            break;
        }
    }
    return 0;
}

#endif

#if defined(MSVC)
mln_string_t *mln_string_set(mln_string_t *str, char *s)
{
    str->data = (mln_u8ptr_t)s;
    str->len = strlen(s);
    str->data_ref = 1;
    str->pool = 0;
    str->ref = 1;
    return str;
}

mln_string_t *mln_string_nset(mln_string_t *str, char *s, mln_u64_t n)
{
    str->data = (mln_u8ptr_t)s;
    str->len = n;
    str->data_ref = 1;
    str->pool = 0;
    str->ref = 1;
    return str;
}

mln_string_t *mln_string_ref(mln_string_t *s)
{
    ++s->ref;
    return s;
}

void mln_string_free(mln_string_t *s)
{
    if (s == NULL) return;
    if (s->ref-- > 1) return;
    if (!s->data_ref && s->data != NULL) {
        if (s->pool) mln_alloc_free(s->data);
        else free(s->data);
    }
    if (s->pool) mln_alloc_free(s);
    else free(s);
}

#endif

MLN_FUNC(static inline, mln_string_t *, mln_string_assign, (char *s, mln_u32_t len), (s, len), {
    mln_string_t *str;
    str = (mln_string_t *)malloc(sizeof(mln_string_t));
    if (str == NULL) return NULL;
    str->data = (mln_u8ptr_t)s;
    str->len = len;
    str->pool = 0;
    str->data_ref = 1;
    str->ref = 1;
    return str;
})

MLN_FUNC(, mln_string_t *, mln_string_buf_new, (mln_u8ptr_t buf, mln_u64_t len), (buf, len), {
    mln_string_t *str = (mln_string_t *)malloc(sizeof(mln_string_t));
    if (str == NULL) return NULL;

    str->data = buf;
    str->len = len;
    str->data_ref = 0;
    str->pool = 0;
    str->ref = 1;
    return str;
})

MLN_FUNC(, mln_string_t *, mln_string_buf_pool_new, \
         (mln_alloc_t *pool, mln_u8ptr_t buf, mln_u64_t len), (pool, buf, len), \
{
    mln_string_t *str = (mln_string_t *)mln_alloc_m(pool, sizeof(mln_string_t));
    if (str == NULL) return NULL;

    str->data = buf;
    str->len = len;
    str->data_ref = 0;
    str->pool = 1;
    str->ref = 1;
    return str;
})

MLN_FUNC(, mln_string_t *, mln_string_pool_new, (mln_alloc_t *pool, const char *s), (pool, s), {
    mln_string_t *str = (mln_string_t *)mln_alloc_m(pool, sizeof(mln_string_t));
    if (str == NULL) return NULL;
    if (s == NULL) {
        str->data = NULL;
        str->len = 0;
        str->data_ref = 0;
        str->pool = 1;
        str->ref = 1;
        return str;
    }
    mln_s32_t len = strlen(s);
    if ((str->data = (mln_u8ptr_t)mln_alloc_m(pool, len + 1)) == NULL) {
        mln_alloc_free(str);
        return NULL;
    }
    memcpy(str->data, s, len);
    str->data[len] = 0;
    str->len = len;
    str->data_ref = 0;
    str->pool = 1;
    str->ref = 1;
    return str;
})

#if defined(MSVC)
MLN_FUNC(, mln_string_t *, mln_string_new, (const char *s), (s), {
    if (s == NULL) {
        mln_string_t *str = (mln_string_t *)malloc(sizeof(mln_string_t) + 1);
        if (str == NULL) return NULL;
        str->data = (mln_u8ptr_t)(str + 1);
        str->data[0] = 0;
        str->len = 0;
        str->data_ref = 1;
        str->pool = 0;
        str->ref = 1;
        return str;
    }
    mln_u64_t len = strlen(s);
    mln_string_t *str = (mln_string_t *)malloc(sizeof(mln_string_t) + len + 1);
    if (str == NULL) return NULL;
    str->data = (mln_u8ptr_t)(str + 1);
    memcpy(str->data, s, len);
    str->data[len] = 0;
    str->len = len;
    str->data_ref = 1;
    str->pool = 0;
    str->ref = 1;
    return str;
})

MLN_FUNC(, mln_string_t *, mln_string_const_ndup, (char *str, mln_s32_t size), (str, size), {
    if (size < 0) return NULL;
    mln_string_t *s = (mln_string_t *)malloc(sizeof(mln_string_t) + size + 1);
    if (s == NULL) return NULL;
    s->data = (mln_u8ptr_t)(s + 1);
    memcpy(s->data, str, size);
    s->data[size] = 0;
    s->len = size;
    s->data_ref = 1;
    s->pool = 0;
    s->ref = 1;
    return s;
})
#endif

MLN_FUNC(, mln_string_t *, mln_string_dup, (mln_string_t *str), (str), {
    mln_string_t *s = (mln_string_t *)malloc(sizeof(mln_string_t) + str->len + 1);
    if (s == NULL) return NULL;
    s->data = (mln_u8ptr_t)(s + 1);
    if (str->data != NULL)
        memcpy(s->data, str->data, str->len);
    s->data[str->len] = 0;
    s->len = str->len;
    s->data_ref = 1;
    s->pool = 0;
    s->ref = 1;
    return s;
})

MLN_FUNC(, mln_string_t *, mln_string_pool_dup, \
         (mln_alloc_t *pool, mln_string_t *str), (pool, str), \
{
    mln_string_t *s = (mln_string_t *)mln_alloc_m(pool, sizeof(mln_string_t));
    if (s == NULL) return NULL;
    if ((s->data = (mln_u8ptr_t)mln_alloc_m(pool, str->len + 1)) == NULL) {
        mln_alloc_free(s);
        return NULL;
    }
    if (str->data != NULL)
        memcpy(s->data, str->data, str->len);
    s->data[str->len] = 0;
    s->len = str->len;
    s->data_ref = 0;
    s->pool = 1;
    s->ref = 1;
    return s;
})

MLN_FUNC(, mln_string_t *, mln_string_alloc, (mln_s32_t size), (size), {
    if (size < 0) return NULL;
    mln_string_t *s = (mln_string_t *)malloc(sizeof(mln_string_t));
    if (s == NULL) return NULL;
    if ((s->data = (mln_u8ptr_t)malloc(size + 1)) == NULL) {
        free(s);
        return NULL;
    }
    s->len = size;
    s->data_ref = 0;
    s->pool = 0;
    s->ref = 1;
    return s;
})

MLN_FUNC(, mln_string_t *, mln_string_pool_alloc, \
         (mln_alloc_t *pool, mln_s32_t size), (pool, size), \
{
    if (size < 0) return NULL;
    mln_string_t *s = (mln_string_t *)mln_alloc_m(pool, sizeof(mln_string_t));
    if (s == NULL) return NULL;
    if ((s->data = (mln_u8ptr_t)mln_alloc_m(pool, size + 1)) == NULL) {
        mln_alloc_free(s);
        return NULL;
    }
    s->len = size;
    s->data_ref = 0;
    s->pool = 1;
    s->ref = 1;
    return s;
})

MLN_FUNC(, mln_string_t *, mln_string_ref_dup, (mln_string_t *str), (str), {
    mln_string_t *s = (mln_string_t *)malloc(sizeof(mln_string_t));
    if (s == NULL) return NULL;
    s->data = str->data;
    s->len = str->len;
    s->data_ref = 1;
    s->pool = 0;
    s->ref = 1;
    return s;
})

MLN_FUNC(, mln_string_t *, mln_string_const_ref_dup, (char *s), (s), {
    if (s == NULL) return NULL;

    mln_string_t *str = (mln_string_t *)malloc(sizeof(mln_string_t));
    if (str == NULL) return NULL;

    str->data = (mln_u8ptr_t)s;
    str->len = strlen(s);
    str->data_ref = 1;
    str->pool = 0;
    str->ref = 1;
    return str;
})

MLN_FUNC(, mln_string_t *, mln_string_concat, (mln_string_t *s1, mln_string_t *s2, mln_string_t *sep), (s1, s2, sep), {
    mln_string_t *str = (mln_string_t *)malloc(sizeof(mln_string_t));
    if (str == NULL) return NULL;

    mln_u8ptr_t p;
    mln_size_t size = 0;

    if (s1 != NULL) {
        size += s1->len;
        if (s2 != NULL && sep != NULL) size += sep->len;
    }
    if (s2 != NULL) size += s2->len;

    if ((p = str->data = (mln_u8ptr_t)malloc(size + 1)) == NULL) {
        free(str);
        return NULL;
    }

    if (s1 != NULL) {
        memcpy(p, s1->data, s1->len);
        p += s1->len;
        if (s2 != NULL && sep != NULL) {
            memcpy(p, sep->data, sep->len);
            p += sep->len;
        }
    }
    if (s2 != NULL) {
        memcpy(p, s2->data, s2->len);
        p += s2->len;
    }
    *p = 0;

    str->len = size;
    str->data_ref = 0;
    str->pool = 0;
    str->ref = 1;

    return str;
})

MLN_FUNC(, mln_string_t *, mln_string_pool_concat, \
         (mln_alloc_t *pool, mln_string_t *s1, mln_string_t *s2, mln_string_t *sep), \
         (pool, s1, s2, sep), \
{
    mln_string_t *str = (mln_string_t *)mln_alloc_m(pool, sizeof(mln_string_t));
    if (str == NULL) return NULL;

    mln_u8ptr_t p;
    mln_size_t size = 0;

    if (s1 != NULL) {
        size += s1->len;
        if (s2 != NULL && sep != NULL) size += sep->len;
    }
    if (s2 != NULL) size += s2->len;

    if ((p = str->data = (mln_u8ptr_t)mln_alloc_m(pool, size + 1)) == NULL) {
        mln_alloc_free(str);
        return NULL;
    }

    if (s1 != NULL) {
        memcpy(p, s1->data, s1->len);
        p += s1->len;
        if (s2 != NULL && sep != NULL) {
            memcpy(p, sep->data, sep->len);
            p += sep->len;
        }
    }
    if (s2 != NULL) {
        memcpy(p, s2->data, s2->len);
        p += s2->len;
    }
    *p = 0;

    str->len = size;
    str->data_ref = 0;
    str->pool = 1;
    str->ref = 1;

    return str;
})

MLN_FUNC(, int, mln_string_strseqcmp, (mln_string_t *s1, mln_string_t *s2), (s1, s2), {
    int ret;
    if (s1->len > s2->len) {
        if (s2->len == 0) return 1;
        ret = mln_string_memcmp_core(s1->data, s2->data, s2->len);
        return ret? ret: 1;
    } else if (s1->len < s2->len) {
        if (s1->len == 0) return 1;
        ret = mln_string_memcmp_core(s1->data, s2->data, s1->len);
        return ret? ret: -1;
    }
    if (s1->len == 0) return 0;
    return mln_string_memcmp_core(s1->data, s2->data, s1->len);
})

MLN_FUNC(, int, mln_string_strcmp, (mln_string_t *s1, mln_string_t *s2), (s1, s2), {
    if (s1 == s2 || s1->data == s2->data) return 0;
    if (s1->len > s2->len) return 1;
    if (s1->len < s2->len) return -1;
    return mln_string_memcmp_core(s1->data, s2->data, s1->len);
})

MLN_FUNC(, int, mln_string_const_strcmp, (mln_string_t *s1, char *s2), (s1, s2), {
    if (s1->data == (mln_u8ptr_t)s2) return 0;
    mln_u32_t len = strlen(s2);
    if (s1->len > len) return 1;
    if (s1->len < len) return -1;
    return mln_string_memcmp_core(s1->data, (const mln_u8ptr_t)s2, len);
})

MLN_FUNC(, int, mln_string_strncmp, \
         (mln_string_t *s1, mln_string_t *s2, mln_u32_t n), (s1, s2, n), \
{
    if (s1 == s2 || s1->data == s2->data) return 0;
    if (s1->len < n || s2->len < n) return -1;
    return mln_string_memcmp_core(s1->data, s2->data, n);
})

MLN_FUNC(, int, mln_string_const_strncmp, (mln_string_t *s1, char *s2, mln_u32_t n), (s1, s2, n), {
    if (s1->data == (mln_u8ptr_t)s2) return 0;
    if (s1->len < n || strlen(s2) < n) return -1;
    return mln_string_memcmp_core(s1->data, (const mln_u8ptr_t)s2, n);
})

MLN_FUNC(, int, mln_string_strcasecmp, (mln_string_t *s1, mln_string_t *s2), (s1, s2), {
    if (s1 == s2 || s1->data == s2->data) return 0;
    if (s1->len > s2->len) return 1;
    if (s1->len < s2->len) return -1;
    return mln_string_memcasecmp_core(s1->data, s2->data, s1->len);
})

MLN_FUNC(, int, mln_string_strncasecmp, \
         (mln_string_t *s1, mln_string_t *s2, mln_u32_t n), (s1, s2, n), \
{
    if (s1 == s2 || s1->data == s2->data) return 0;
    if (s1->len < n || s2->len < n) return -1;
    return mln_string_memcasecmp_core(s1->data, s2->data, n);
})

MLN_FUNC(, int, mln_string_const_strcasecmp, (mln_string_t *s1, char *s2), (s1, s2), {
    if (s1->data == (mln_u8ptr_t)s2) return 0;
    mln_u32_t len = strlen(s2);
    if (s1->len > len) return 1;
    if (s1->len < len) return -1;
    return mln_string_memcasecmp_core(s1->data, (const mln_u8ptr_t)s2, len);
})

MLN_FUNC(, int, mln_string_const_strncasecmp, (mln_string_t *s1, char *s2, mln_u32_t n), (s1, s2, n), {
    if (s1->data == (mln_u8ptr_t)s2) return 0;
    mln_u32_t len = strlen(s2);
    if (s1->len < n || len < n) return -1;
    return mln_string_memcasecmp_core(s1->data, (const mln_u8ptr_t)s2, n);
})

MLN_FUNC(, char *, mln_string_strstr, (mln_string_t *text, mln_string_t *pattern), (text, pattern), {
    if (text == pattern || text->data == pattern->data)
        return (char *)(text->data);
    return strstr((char *)(text->data), (char *)(pattern->data));
})

MLN_FUNC(, char *, mln_string_const_strstr, (mln_string_t *text, char *pattern), (text, pattern), {
    if (text->data == (mln_u8ptr_t)pattern)
        return (char *)(text->data);
    return strstr((char *)(text->data), pattern);
})

MLN_FUNC(, mln_string_t *, mln_string_new_strstr, \
         (mln_string_t *text, mln_string_t *pattern), (text, pattern), \
{
    char *p = mln_string_strstr(text, pattern);
    if (p == NULL) return NULL;
    return mln_string_assign(p, text->len - (p - (char *)(text->data)));
})

MLN_FUNC(, mln_string_t *, mln_string_new_const_strstr, \
         (mln_string_t *text, char *pattern), (text, pattern), \
{
    char *p = mln_string_const_strstr(text, pattern);
    if (p == NULL) return NULL;
    return mln_string_assign(p, text->len - (p - (char *)(text->data)));
})

MLN_FUNC(, char *, mln_string_kmp, (mln_string_t *text, mln_string_t *pattern), (text, pattern), {
    if (text == pattern || text->data == pattern->data)
        return (char *)(text->data);
    return kmp_string_match((char *)(text->data), (char *)(pattern->data), text->len, pattern->len);
})

MLN_FUNC(, char *, mln_string_const_kmp, (mln_string_t *text, char *pattern), (text, pattern), {
    if (text->data == (mln_u8ptr_t)pattern)
        return (char *)(text->data);
    return kmp_string_match((char *)(text->data), pattern, text->len, strlen(pattern));
})

MLN_FUNC(, mln_string_t *, mln_string_new_kmp, (mln_string_t *text, mln_string_t *pattern), (text, pattern), {
    char *p = mln_string_kmp(text, pattern);
    if (p == NULL) return NULL;
    return mln_string_assign(p, text->len - (p - (char *)(text->data)));
})

MLN_FUNC(, mln_string_t *, mln_string_new_const_kmp, (mln_string_t *text, char *pattern), (text, pattern), {
    char *p = mln_string_const_kmp(text, pattern);
    if (p == NULL) return NULL;
    return mln_string_assign(p, text->len - (p - (char *)(text->data)));
})

/*
 * kmp_string_match() and compute_prefix_function() are
 * components of KMP algorithm.
 * The longer pattern's prefix matched and existed
 * the higher performance of KMP algorithm made.
 *
 * Optimization: for patterns up to MLN_KMP_STACK_LIMIT bytes we keep the
 * prefix table on the stack to avoid a malloc/free round trip on every
 * match.
 */
#define MLN_KMP_STACK_LIMIT 512

MLN_FUNC(static inline, char *, kmp_string_match, \
         (char *text, const char *pattern, int text_len, int pattern_len), \
         (text, pattern, text_len, pattern_len), \
{
    if (pattern_len == 0) return text;
    if (pattern_len > text_len) return NULL;

    int stack_shift[MLN_KMP_STACK_LIMIT];
    int *shift;
    int heap = 0;
    if (pattern_len <= MLN_KMP_STACK_LIMIT) {
        shift = stack_shift;
    } else {
        shift = (int *)malloc(sizeof(int) * pattern_len);
        if (shift == NULL) return NULL;
        heap = 1;
    }
    if (compute_prefix_function(pattern, pattern_len, shift) == NULL) {
        if (heap) free(shift);
        return NULL;
    }
    int q = 0, i;
    char *ret = NULL;
    for (i = 0; i < text_len; ++i) {
        while (q > 0 && pattern[q] != text[i])
            q = shift[q - 1];
        if (pattern[q] == text[i])
            ++q;
        if (q == pattern_len) {
            ret = &text[i - pattern_len + 1];
            break;
        }
    }
    if (heap) free(shift);
    return ret;
})

MLN_FUNC(static inline, int *, compute_prefix_function, \
         (const char *pattern, int m, int *buf), (pattern, m, buf), \
{
    if (buf == NULL) return NULL;
    buf[0] = 0;
    int k = 0, q;
    for (q = 1; q < m; ++q) {
        while (k > 0 && pattern[k] != pattern[q])
            k = buf[k - 1];
        if (pattern[k] == pattern[q])
            ++k;
        buf[q] = k;
    }
    return buf;
})

/*
 * mln_string_slice: single-pass iterative slice with geometric growth.
 *   1. Duplicate the input buffer so we can zero separator bytes in place
 *      without affecting the caller's memory.
 *   2. Walk the buffer once, growing the result array as needed.
 *   3. Shrink to exactly cnt + 1 entries (last entry is a terminator).
 */
#define MLN_STR_SLICE_INIT 16

MLN_FUNC(, mln_string_t *, mln_string_slice, \
         (mln_string_t *s, const char *sep_array/*ended by \0*/), \
         (s, sep_array), \
{
    mln_u8_t ascii[256] = {0};
    const char *ps;
    mln_string_t *dup;
    mln_string_t *arr;
    mln_u8ptr_t p, end;
    mln_u64_t cnt = 0;
    mln_u64_t cap = MLN_STR_SLICE_INIT;

    dup = mln_string_dup(s);
    if (dup == NULL) return NULL;

    for (ps = sep_array; *ps != 0; ++ps) {
        ascii[(mln_u8_t)(*ps)] = 1;
    }

    arr = (mln_string_t *)malloc(sizeof(mln_string_t) * (cap + 1));
    if (arr == NULL) {
        mln_string_free(dup);
        return NULL;
    }

    p   = dup->data;
    end = dup->data + dup->len;

    while (p < end) {
        /* Zero run of separators. */
        while (p < end && ascii[*p]) { *p = 0; ++p; }
        if (p >= end) break;
        if (cnt + 1 >= cap) {
            mln_u64_t new_cap = cap * 2;
            mln_string_t *new_arr = (mln_string_t *)realloc(arr,
                sizeof(mln_string_t) * (new_cap + 1));
            if (new_arr == NULL) {
                free(arr);
                mln_string_free(dup);
                return NULL;
            }
            arr = new_arr;
            cap = new_cap;
        }
        {
            mln_u8ptr_t tok = p;
            while (p < end && !ascii[*p]) ++p;
            arr[cnt].data = tok;
            arr[cnt].len = (mln_u64_t)(p - tok);
            arr[cnt].data_ref = 1;
            arr[cnt].pool = 0;
            arr[cnt].ref = 1;
            ++cnt;
        }
    }

    /* Shrink to fit to reduce memory footprint of the returned array. */
    if (cap > cnt) {
        mln_string_t *shrunk = (mln_string_t *)realloc(arr,
            sizeof(mln_string_t) * (cnt + 1));
        if (shrunk != NULL) arr = shrunk;
    }

    /* Terminator entry carries the dup pointer for later release. */
    arr[cnt].data = (mln_u8ptr_t)dup;
    arr[cnt].len = 0;
    arr[cnt].data_ref = 0;
    arr[cnt].pool = 0;
    arr[cnt].ref = 1;
    return arr;
})

MLN_FUNC_VOID(, void, mln_string_slice_free, (mln_string_t *array), (array), {
    mln_string_t *s = &array[0];
    for (; s->len; ++s)
        ;
    mln_string_free((mln_string_t *)(s->data));
    free(array);
})

MLN_FUNC(, mln_string_t *, mln_string_strcat, (mln_string_t *s1, mln_string_t *s2), (s1, s2), {
    mln_string_t *ret = (mln_string_t *)malloc(sizeof(mln_string_t));
    if (ret == NULL) return NULL;
    mln_u64_t len = s1->len + s2->len;
    if (len == 0) {
        ret->data = NULL;
        ret->len = 0;
        ret->data_ref = 0;
        ret->pool = 0;
        ret->ref = 1;
        return ret;
    }
    if ((ret->data = (mln_u8ptr_t)malloc(len + 1)) == NULL) {
        free(ret);
        return NULL;
    }
    if (s1->len > 0) memcpy(ret->data, s1->data, s1->len);
    if (s2->len > 0) memcpy(ret->data+s1->len, s2->data, s2->len);
    ret->data[len] = 0;
    ret->len = len;
    ret->data_ref = 0;
    ret->pool = 0;
    ret->ref = 1;
    return ret;
})

MLN_FUNC(, mln_string_t *, mln_string_pool_strcat, \
         (mln_alloc_t *pool, mln_string_t *s1, mln_string_t *s2), (pool, s1, s2), \
{
    mln_string_t *ret = (mln_string_t *)mln_alloc_m(pool, sizeof(mln_string_t));
    if (ret == NULL) return NULL;
    mln_u64_t len = s1->len + s2->len;
    if (len == 0) {
        ret->data = NULL;
        ret->len = 0;
        ret->data_ref = 0;
        ret->pool = 1;
        ret->ref = 1;
        return ret;
    }
    if ((ret->data = (mln_u8ptr_t)mln_alloc_m(pool, len + 1)) == NULL) {
        mln_alloc_free(ret);
        return NULL;
    }
    if (s1->len > 0) memcpy(ret->data, s1->data, s1->len);
    if (s2->len > 0) memcpy(ret->data+s1->len, s2->data, s2->len);
    ret->data[len] = 0;
    ret->len = len;
    ret->data_ref = 0;
    ret->pool = 1;
    ret->ref = 1;
    return ret;
})

/*
 * Fast path for trim when the mask contains a single byte - very common
 * (trimming spaces or newlines) and avoids the 256-byte table.
 */
MLN_FUNC(, mln_string_t *, mln_string_trim, (mln_string_t *s, mln_string_t *mask), (s, mask), {
    mln_string_t tmp;
    mln_size_t i, j;

    if (mask->len == 1) {
        mln_u8_t m = mask->data[0];
        for (i = 0; i < s->len && s->data[i] == m; ++i) ;
        for (j = s->len; j > i && s->data[j - 1] == m; --j) ;
    } else {
        mln_u8_t chars[256] = {0};
        for (i = 0; i < mask->len; ++i) chars[mask->data[i]] = 1;
        for (i = 0; i < s->len; ++i) { if (!chars[s->data[i]]) break; }
        for (j = s->len; j > i; --j) { if (!chars[s->data[j - 1]]) break; }
    }

    mln_string_nset(&tmp, &(s->data[i]), j - i);
    return mln_string_dup(&tmp);
})

MLN_FUNC(, mln_string_t *, mln_string_pool_trim, \
         (mln_alloc_t *pool, mln_string_t *s, mln_string_t *mask), \
         (pool, s, mask), \
{
    mln_string_t tmp;
    mln_size_t i, j;

    if (mask->len == 1) {
        mln_u8_t m = mask->data[0];
        for (i = 0; i < s->len && s->data[i] == m; ++i) ;
        for (j = s->len; j > i && s->data[j - 1] == m; --j) ;
    } else {
        mln_u8_t chars[256] = {0};
        for (i = 0; i < mask->len; ++i) chars[mask->data[i]] = 1;
        for (i = 0; i < s->len; ++i) { if (!chars[s->data[i]]) break; }
        for (j = s->len; j > i; --j) { if (!chars[s->data[j - 1]]) break; }
    }

    mln_string_nset(&tmp, &(s->data[i]), j - i);
    return mln_string_pool_dup(pool, &tmp);
})

MLN_FUNC_VOID(, void, mln_string_upper, (mln_string_t *s), (s), {
    if (s->data == NULL || s->len == 0) return;
    mln_string_toupper_inplace(s->data, s->len);
})

MLN_FUNC_VOID(, void, mln_string_lower, (mln_string_t *s), (s), {
    if (s->data == NULL || s->len == 0) return;
    mln_string_tolower_inplace(s->data, s->len);
})

