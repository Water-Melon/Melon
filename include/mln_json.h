
/*
 * Copyright (C) Niklaus F.Schen.
 */

#ifndef __MLN_JSON_H
#define __MLN_JSON_H

#include <stdlib.h>
#include "mln_string.h"
#include "mln_hash.h"
#include "mln_array.h"

#define M_JSON_LEN              31

#define M_JSON_V_FALSE          0
#define M_JSON_V_TRUE           1
#define M_JSON_V_NULL           NULL

typedef struct mln_json_s mln_json_t;
typedef int (*mln_json_iterator_t)(mln_json_t *, void *);

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
        mln_hash_t        m_j_obj;
        mln_array_t       m_j_array;
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
} mln_json_kv_t;

#define M_JSON_IS_OBJECT(json)                 ((json)->type == M_JSON_OBJECT)
#define M_JSON_IS_ARRAY(json)                  ((json)->type == M_JSON_ARRAY)
#define M_JSON_IS_STRING(json)                 ((json)->type == M_JSON_STRING)
#define M_JSON_IS_NUMBER(json)                 ((json)->type == M_JSON_NUM)
#define M_JSON_IS_TRUE(json)                   ((json)->type == M_JSON_TRUE)
#define M_JSON_IS_FALSE(json)                  ((json)->type == M_JSON_FALSE)
#define M_JSON_IS_NULL(json)                   ((json)->type == M_JSON_NULL)
#define M_JSON_IS_NONE(json)                   ((json)->type == M_JSON_NONE)

#define M_JSON_GET_DATA_OBJECT(json)           (&((json)->data.m_j_obj))
#define M_JSON_GET_DATA_ARRAY(json)            (&((json)->data.m_j_array))
#define M_JSON_GET_DATA_STRING(json)           ((json)->data.m_j_string)
#define M_JSON_GET_DATA_NUMBER(json)           ((json)->data.m_j_number)
#define M_JSON_GET_DATA_TRUE(json)             ((json)->data.m_j_true)
#define M_JSON_GET_DATA_FALSE(json)            ((json)->data.m_j_false)
#define M_JSON_GET_DATA_NULL(json)             ((json)->data.m_j_null)

#define M_JSON_SET_TYPE_NONE(json)             (json)->type = M_JSON_NONE
#define M_JSON_SET_TYPE_OBJECT(json)           (json)->type = M_JSON_OBJECT
#define M_JSON_SET_TYPE_ARRAY(json)            (json)->type = M_JSON_ARRAY
#define M_JSON_SET_TYPE_STRING(json)           (json)->type = M_JSON_STRING
#define M_JSON_SET_TYPE_NUMBER(json)           (json)->type = M_JSON_NUM
#define M_JSON_SET_TYPE_TRUE(json)             (json)->type = M_JSON_TRUE
#define M_JSON_SET_TYPE_FALSE(json)            (json)->type = M_JSON_FALSE
#define M_JSON_SET_TYPE_NULL(json)             (json)->type = M_JSON_NULL

#define mln_json_init(j)                       M_JSON_SET_TYPE_NONE(j)
#define mln_json_string_init(j, s)             ({\
    mln_json_t *json = (j);\
    M_JSON_SET_TYPE_STRING(json);\
    json->data.m_j_string = (s);\
})
#define mln_json_number_init(j, n)             ({\
    mln_json_t *json = (j);\
    M_JSON_SET_TYPE_NUMBER(json);\
    json->data.m_j_number = (double)(n);\
})
#define mln_json_true_init(j)                  ({\
    mln_json_t *json = (j);\
    M_JSON_SET_TYPE_TRUE(json);\
    json->data.m_j_true = 1;\
})
#define mln_json_false_init(j)                 ({\
    mln_json_t *json = (j);\
    M_JSON_SET_TYPE_FALSE(json);\
    json->data.m_j_false = 1;\
})
#define mln_json_null_init(j)                  ({\
    mln_json_t *json = (j);\
    M_JSON_SET_TYPE_NULL(json);\
    json->data.m_j_null = NULL;\
})
extern int mln_json_obj_init(mln_json_t *j) __NONNULL1(1);
extern int mln_json_array_init(mln_json_t *j) __NONNULL1(1);
extern void mln_json_destroy(mln_json_t *j);
#define mln_json_reset(j)                      ({\
    mln_json_t *json = (j);\
    mln_json_destroy(json);\
    M_JSON_SET_TYPE_NONE((json));\
})
extern void mln_json_dump(mln_json_t *j, int n_space, char *prefix);
extern int mln_json_obj_update(mln_json_t *j, mln_json_t *key, mln_json_t *val) __NONNULL3(1,2,3);
extern mln_json_t *mln_json_obj_search(mln_json_t *j, mln_string_t *key) __NONNULL2(1,2);
extern void mln_json_obj_remove(mln_json_t *j, mln_string_t *key) __NONNULL2(1,2);
extern mln_json_t *mln_json_array_search(mln_json_t *j, mln_uauto_t index) __NONNULL1(1);
extern mln_uauto_t mln_json_array_length(mln_json_t *j) __NONNULL1(1);
extern int mln_json_array_append(mln_json_t *j, mln_json_t *value) __NONNULL2(1,2);
extern int mln_json_array_update(mln_json_t *j, mln_json_t *value, mln_uauto_t index) __NONNULL2(1,2);
extern void mln_json_array_remove(mln_json_t *j, mln_uauto_t index);
extern int mln_json_decode(mln_string_t *jstr, mln_json_t *out);
extern mln_string_t *mln_json_encode(mln_json_t *j);
extern int mln_json_parse(mln_json_t *j, mln_string_t *exp, mln_json_iterator_t iterator, void *data) __NONNULL2(1,2);
extern int mln_json_generate(mln_json_t *j, char *fmt, ...) __NONNULL2(1,2);

#endif

