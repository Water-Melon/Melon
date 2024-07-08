
/*
 * Copyright (C) Niklaus F.Schen.
 */

#ifndef __MLN_JSON_H
#define __MLN_JSON_H

#include <stdlib.h>
#include "mln_string.h"
#include "mln_array.h"
#include "mln_rbtree.h"

#define M_JSON_LEN              31
#define M_JSON_BUFLEN           512

#define M_JSON_V_FALSE          0
#define M_JSON_V_TRUE           1
#define M_JSON_V_NULL           NULL

/*
 * policy errors
 */
#define M_JSON_OK               0
#define M_JSON_DEPTH            1
#define M_JSON_KEYLEN           2
#define M_JSON_STRLEN           3
#define M_JSON_ARRELEM          4
#define M_JSON_OBJKV            5

typedef struct mln_json_s mln_json_t;
typedef int (*mln_json_iterator_t)(mln_json_t *, void *);
typedef int (*mln_json_object_iterator_t)(mln_json_t * /*key*/, mln_json_t * /*val*/, void *);
typedef int (*mln_json_array_iterator_t)(mln_json_t *, void *);
typedef mln_json_array_iterator_t mln_json_call_func_t;

enum json_type {
    M_JSON_NONE = 0,
    M_JSON_OBJECT,
    M_JSON_ARRAY,
    M_JSON_STRING,
    M_JSON_NUM,
    M_JSON_TRUE,
    M_JSON_FALSE,
    M_JSON_NULL
};

struct mln_json_s {
    enum json_type               type;
    union {
        mln_rbtree_t     *m_j_obj;
        mln_array_t      *m_j_array;
        mln_string_t     *m_j_string;
        double            m_j_number;
        mln_u8_t          m_j_true;
        mln_u8_t          m_j_false;
        mln_u8ptr_t       m_j_null;
    }                            data;
};

typedef struct {
    mln_json_t                   key;
    mln_json_t                   val;
    mln_rbtree_node_t            node;
} mln_json_kv_t;

struct mln_json_call_attr {
    mln_json_call_func_t         callback;
    void                        *data;
};

typedef struct {
    mln_size_t                   depth;
    mln_size_t                   key_len;
    mln_size_t                   str_len;
    mln_size_t                   arr_elem_num;
    mln_size_t                   obj_kv_num;
    int                          error;
} mln_json_policy_t;

#define mln_json_is_object(json)                 ((json)->type == M_JSON_OBJECT)
#define mln_json_is_array(json)                  ((json)->type == M_JSON_ARRAY)
#define mln_json_is_string(json)                 ((json)->type == M_JSON_STRING)
#define mln_json_is_number(json)                 ((json)->type == M_JSON_NUM)
#define mln_json_is_true(json)                   ((json)->type == M_JSON_TRUE)
#define mln_json_is_false(json)                  ((json)->type == M_JSON_FALSE)
#define mln_json_is_null(json)                   ((json)->type == M_JSON_NULL)
#define mln_json_is_none(json)                   ((json)->type == M_JSON_NONE)

#define mln_json_object_data_get(json)           ((json)->data.m_j_obj)
#define mln_json_array_data_get(json)            ((json)->data.m_j_array)
#define mln_json_string_data_get(json)           ((json)->data.m_j_string)
#define mln_json_number_data_get(json)           ((json)->data.m_j_number)
#define mln_json_true_data_get(json)             ((json)->data.m_j_true)
#define mln_json_false_data_get(json)            ((json)->data.m_j_false)
#define mln_json_null_data_get(json)             ((json)->data.m_j_null)

#define mln_json_none_type_set(json)             (json)->type = M_JSON_NONE
#define mln_json_object_type_set(json)           (json)->type = M_JSON_OBJECT
#define mln_json_array_type_set(json)            (json)->type = M_JSON_ARRAY
#define mln_json_string_type_set(json)           (json)->type = M_JSON_STRING
#define mln_json_number_type_set(json)           (json)->type = M_JSON_NUM
#define mln_json_true_type_set(json)             (json)->type = M_JSON_TRUE
#define mln_json_false_type_set(json)            (json)->type = M_JSON_FALSE
#define mln_json_null_type_set(json)             (json)->type = M_JSON_NULL

#define mln_json_init(j)                       mln_json_none_type_set(j)
#if defined(MSVC)
#define mln_json_string_init(j, s) do {\
    mln_json_t *json = (j);\
    mln_json_string_type_set(json);\
    json->data.m_j_string = (s);\
} while (0)
#define mln_json_number_init(j, n) do {\
    mln_json_t *json = (j);\
    mln_json_number_type_set(json);\
    json->data.m_j_number = (double)(n);\
} while (0)
#define mln_json_true_init(j) do {\
    mln_json_t *json = (j);\
    mln_json_true_type_set(json);\
    json->data.m_j_true = 1;\
} while (0)
#define mln_json_false_init(j) do {\
    mln_json_t *json = (j);\
    mln_json_false_type_set(json);\
    json->data.m_j_false = 1;\
} while (0)
#define mln_json_null_init(j) do {\
    mln_json_t *json = (j);\
    mln_json_null_type_set(json);\
    json->data.m_j_null = NULL;\
} while (0)
#define mln_json_policy_init(policy, _depth, _keylen, _strlen, _elemnum, _kvnum) do {\
    policy.depth = _depth;\
    policy.key_len = _keylen;\
    policy.str_len = _strlen;\
    policy.arr_elem_num = _elemnum;\
    policy.obj_kv_num = _kvnum;\
    policy.error = M_JSON_OK;\
} while (0)
#else
#define mln_json_string_init(j, s)             ({\
    mln_json_t *json = (j);\
    mln_json_string_type_set(json);\
    json->data.m_j_string = (s);\
})
#define mln_json_number_init(j, n)             ({\
    mln_json_t *json = (j);\
    mln_json_number_type_set(json);\
    json->data.m_j_number = (double)(n);\
})
#define mln_json_true_init(j)                  ({\
    mln_json_t *json = (j);\
    mln_json_true_type_set(json);\
    json->data.m_j_true = 1;\
})
#define mln_json_false_init(j)                 ({\
    mln_json_t *json = (j);\
    mln_json_false_type_set(json);\
    json->data.m_j_false = 1;\
})
#define mln_json_null_init(j)                  ({\
    mln_json_t *json = (j);\
    mln_json_null_type_set(json);\
    json->data.m_j_null = NULL;\
})
#define mln_json_policy_init(policy, _depth, _keylen, _strlen, _elemnum, _kvnum) ({\
    policy.depth = _depth;\
    policy.key_len = _keylen;\
    policy.str_len = _strlen;\
    policy.arr_elem_num = _elemnum;\
    policy.obj_kv_num = _kvnum;\
    policy.error = M_JSON_OK;\
})
#endif

#define mln_json_policy_error(policy) ((policy).error)

extern int mln_json_obj_init(mln_json_t *j) __NONNULL1(1);
extern int mln_json_array_init(mln_json_t *j) __NONNULL1(1);
extern void mln_json_destroy(mln_json_t *j);
extern void mln_json_dump(mln_json_t *j, int n_space, char *prefix);
extern int mln_json_obj_update(mln_json_t *j, mln_json_t *key, mln_json_t *val) __NONNULL3(1,2,3);
extern mln_json_t *mln_json_obj_search(mln_json_t *j, mln_string_t *key) __NONNULL2(1,2);
extern void mln_json_obj_remove(mln_json_t *j, mln_string_t *key) __NONNULL2(1,2);
extern mln_json_t *mln_json_array_search(mln_json_t *j, mln_uauto_t index) __NONNULL1(1);
extern mln_uauto_t mln_json_array_length(mln_json_t *j) __NONNULL1(1);
extern int mln_json_array_append(mln_json_t *j, mln_json_t *value) __NONNULL2(1,2);
extern int mln_json_array_update(mln_json_t *j, mln_json_t *value, mln_uauto_t index) __NONNULL2(1,2);
extern void mln_json_array_remove(mln_json_t *j, mln_uauto_t index);
extern int mln_json_decode(mln_string_t *jstr, mln_json_t *out, mln_json_policy_t *policy);
extern mln_string_t *mln_json_encode(mln_json_t *j);
extern int mln_json_parse(mln_json_t *j, mln_string_t *exp, mln_json_iterator_t iterator, void *data) __NONNULL2(1,2);
extern int mln_json_generate(mln_json_t *j, char *fmt, ...) __NONNULL2(1,2);
extern int mln_json_object_iterate(mln_json_t *j, mln_json_object_iterator_t it, void *data) __NONNULL2(1,2);
#if defined(MSVC)
extern void mln_json_reset(mln_json_t *j);
extern int mln_json_array_iterate(mln_json_t *j, mln_json_array_iterator_t it, void *data);
#else
#define mln_json_reset(j)                      ({\
    mln_json_t *json = (j);\
    mln_json_destroy(json);\
    mln_json_none_type_set((json));\
})
#define mln_json_array_iterate(j, it, data) ({\
    mln_json_t *json = (mln_json_t *)(j), *end;\
    mln_json_array_iterator_t iterator = (mln_json_array_iterator_t)(it);\
    int rc = -1;\
    mln_array_t *a = mln_json_array_data_get(j);\
    if (mln_json_is_array(json)) {\
        json = mln_array_elts(a);\
        end = json + mln_array_nelts(a);\
        for (; json < end; ++json) {\
            rc = iterator(json, data);\
            if (rc) break;\
        }\
    }\
    rc;\
})
#endif

#endif

