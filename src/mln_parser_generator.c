
/*
 * Copyright (C) Niklaus F.Schen.
 */

#include <stdlib.h>
#include "mln_parser_generator.h"

/*
 * structures
 */
struct mln_pg_state {
    int            *change;
    mln_pg_rule_t  *r;
    mln_pg_state_t *s;
    mln_pg_item_t  *item;
};

struct mln_pg_info {
    int            *change;
    volatile int   *map;
    mln_pg_rule_t  *r;
    mln_pg_token_t *tk;
    mln_pg_state_t *s;
    mln_pg_item_t  *item;
    mln_u32_t       nr_rule;
};

/*
 * declarations
 */
static int
mln_pg_calc_first_rbtree_iterate_handler(mln_rbtree_node_t *node, void *udata);
static int
mln_pg_calc_follow_rbtree_iterate_handler(mln_rbtree_node_t *node, void *udata);
static int
mln_pg_closure_rbtree_iterate_handler(mln_rbtree_node_t *node, void *udata);
static mln_pg_item_t *
mln_pg_rule_duplicate(mln_pg_state_t *s, mln_pg_rule_t *r, mln_u32_t pos);
static int
mln_pg_output_token_iterate_handler(mln_rbtree_node_t *node, void *udata);
static int
mln_pg_output_token_set_iterate_handler(mln_rbtree_node_t *node, void *udata);
static int
mln_pg_calc_info_cmp(const void *data1, const void *data2);
static int
mln_pg_goto_la_iterate_handler(mln_rbtree_node_t *node, void *udata);
static int
mln_pg_goto_iterate_handler(mln_rbtree_node_t *node, void *udata);
static int
mln_pg_output_state_iterate_handler(mln_rbtree_node_t *node, void *udata);
static inline int
mln_pg_state_duplicate(mln_pg_state_t **q_head, \
                       mln_pg_state_t **q_tail, \
                       mln_pg_state_t *dest, \
                       mln_pg_state_t *src);
MLN_CHAIN_FUNC_DECLARE(static, \
                       mln_item, \
                       mln_pg_item_t, );
MLN_CHAIN_FUNC_DEFINE(static, \
                      mln_item, \
                      mln_pg_item_t, \
                      prev, \
                      next);
MLN_CHAIN_FUNC_DECLARE(static, \
                       mln_state, \
                       mln_pg_state_t, );
MLN_CHAIN_FUNC_DEFINE(static, \
                      mln_state, \
                      mln_pg_state_t, \
                      prev, \
                      next);


/*
 * mln_pg_token_t
 */
MLN_FUNC(, mln_pg_token_t *, mln_pg_token_new, \
         (mln_string_t *token, mln_u32_t nr_rule), (token, nr_rule), \
{
    mln_pg_token_t *t = (mln_pg_token_t *)malloc(sizeof(mln_pg_token_t));
    if (t == NULL) return NULL;
    t->token = NULL;
    t->first_set = NULL;
    t->follow_set = NULL;
    t->right_rule_index = NULL;
    t->left_rule_index = NULL;
    t->type = -1;
    t->is_nonterminal = 0;
    t->is_nullable = 0;
    t->token = mln_string_dup(token);
    if (t->token == NULL) {
        mln_pg_token_free(t);
        return NULL;
    }

    struct mln_rbtree_attr rbattr;
    rbattr.pool = NULL;
    rbattr.pool_alloc = NULL;
    rbattr.pool_free = NULL;
    rbattr.cmp = mln_pg_token_rbtree_cmp;
    rbattr.data_free = NULL;
    t->first_set = mln_rbtree_new(&rbattr);
    if (t->first_set == NULL) {
        mln_pg_token_free(t);
        return NULL;
    }
    t->follow_set = mln_rbtree_new(&rbattr);
    if (t->follow_set == NULL) {
        mln_pg_token_free(t);
        return NULL;
    }
    t->right_rule_index = (mln_u32_t *)calloc(nr_rule, sizeof(mln_u32_t));
    if (t->right_rule_index == NULL) {
        mln_pg_token_free(t);
        return NULL;
    }
    t->left_rule_index = (mln_u32_t *)calloc(nr_rule, sizeof(mln_u32_t));
    if (t->left_rule_index == NULL) {
        mln_pg_token_free(t);
        return NULL;
    }
    return t;
})

MLN_FUNC_VOID(, void, mln_pg_token_free, (void *token), (token), {
    if (token == NULL) return;
    mln_pg_token_t *t = (mln_pg_token_t *)token;
    if (t->token != NULL)
        mln_string_free(t->token);
    if (t->first_set != NULL)
        mln_rbtree_free(t->first_set);
    if (t->follow_set != NULL)
        mln_rbtree_free(t->follow_set);
    if (t->right_rule_index != NULL)
        free(t->right_rule_index);
    if (t->left_rule_index != NULL)
        free(t->left_rule_index);
    free(t);
})

MLN_FUNC(, int, mln_pg_token_rbtree_cmp, \
         (const void *data1, const void *data2), (data1, data2), \
{
    return ((mln_pg_token_t *)data1)->type - ((mln_pg_token_t *)data2)->type;
})

MLN_FUNC(, mln_u64_t, mln_pg_map_hash_calc, (mln_hash_t *h, void *key), (h, key), {
    mln_u64_t sum = 0;
    char *p = (char *)key;
    for (; *p != 0; ++p) {
        sum += (*p * 65599);
        sum %= h->len;
    }
    return sum;
})

MLN_FUNC(, int, mln_pg_map_hash_cmp, \
         (mln_hash_t *h, void *key1, void *key2), (h, key1, key2), \
{
    return !strcmp((char *)key1, (char *)key2);
})

MLN_FUNC_VOID(, void, mln_pg_map_hash_free, (void *data), (data), {
    if (data == NULL) return;
    free(data);
})

/*
 * mln_pg_item_t
 */
MLN_FUNC(, mln_pg_item_t *, mln_pg_item_new, (void), (), {
    mln_pg_item_t *item = (mln_pg_item_t *)malloc(sizeof(mln_pg_item_t));
    if (item == NULL) return NULL;
    item->prev = NULL;
    item->next = NULL;
    item->lookahead_set = NULL;
    item->read = NULL;
    item->goto_id = -1;
    item->rule = NULL;
    item->pos = 0;

    struct mln_rbtree_attr rbattr;
    rbattr.pool = NULL;
    rbattr.pool_alloc = NULL;
    rbattr.pool_free = NULL;
    rbattr.cmp = mln_pg_token_rbtree_cmp;
    rbattr.data_free = NULL;
    item->lookahead_set = mln_rbtree_new(&rbattr);
    if (item->lookahead_set == NULL) {
        mln_pg_item_free(item);
        return NULL;
    }
    return item;
})

MLN_FUNC_VOID(, void, mln_pg_item_free, (mln_pg_item_t *item), (item), {
    if (item == NULL) return;
    if (item->lookahead_set != NULL) {
        mln_rbtree_free(item->lookahead_set);
    }
    free(item);
})

/*
 * state
 */
MLN_FUNC(, mln_pg_state_t *, mln_pg_state_new, (void), (), {
    mln_pg_state_t *s = (mln_pg_state_t *)malloc(sizeof(mln_pg_state_t));
    if (s == NULL) return NULL;
    s->id = -1;
    s->input = NULL;
    s->head = NULL;
    s->tail = NULL;
    s->prev = NULL;
    s->next = NULL;
    s->q_next = NULL;
    s->nr_item = 0;
    return s;
})

MLN_FUNC_VOID(, void, mln_pg_state_free, (mln_pg_state_t *s), (s), {
    mln_pg_item_t *it;
    while ((it = s->head) != NULL) {
        mln_item_chain_del(&(s->head), &(s->tail), it);
        mln_pg_item_free(it);
    }
    free(s);
})

/*
 * struct mln_pg_calc_info_s
 */
MLN_FUNC(, int, mln_pg_calc_info_init, \
         (struct mln_pg_calc_info_s *pci, mln_pg_token_t *first_input, \
          mln_pg_rule_t *rule, mln_u32_t nr_rule), \
         (pci, first_input, rule, nr_rule), \
{
    struct mln_rbtree_attr rbattr;
    rbattr.pool = NULL;
    rbattr.pool_alloc = NULL;
    rbattr.pool_free = NULL;
    rbattr.cmp = mln_pg_calc_info_cmp;
    rbattr.data_free = NULL;
    if ((pci->tree = mln_rbtree_new(&rbattr)) == NULL) {
        return -1;
    }
    pci->head = NULL;
    pci->tail = NULL;
    pci->id_counter = 0;
    pci->first_input = first_input;
    pci->rule = rule;
    pci->nr_rule = nr_rule;
    return 0;
})

MLN_FUNC_VOID(, void, mln_pg_calc_info_destroy, \
              (struct mln_pg_calc_info_s *pci), (pci), \
{
    if (pci == NULL) return;
    if (pci->tree != NULL) mln_rbtree_free(pci->tree);
    mln_pg_state_t *s;
    while ((s = pci->head) != NULL) {
        mln_state_chain_del(&(pci->head), &(pci->tail), s);
        mln_pg_state_free(s);
    }
})

MLN_FUNC(static, int, mln_pg_calc_info_cmp, \
         (const void *data1, const void *data2), (data1, data2), \
{
    mln_pg_state_t *s1 = (mln_pg_state_t *)data1;
    mln_pg_state_t *s2 = (mln_pg_state_t *)data2;
    if (s1->input > s2->input) return 1;
    if (s1->input < s2->input) return -1;
    if (s1->nr_item > s2->nr_item) return 1;
    if (s1->nr_item < s2->nr_item) return -1;

    mln_sauto_t nr_match = 0;
    mln_pg_item_t *it1, *it2;
    for (it1 = s1->head; it1 != NULL; it1 = it1->next) {
        for (it2 = s2->head; it2 != NULL; it2 = it2->next) {
            if (it1->rule == it2->rule && it1->pos == it2->pos) {
                ++nr_match;
                break;
            }
        }
    }
    if (s1->nr_item > nr_match) return 1;
    if (nr_match == s1->nr_item) return 0;
    return -1;
})

/*
 * calc nullable
 */
MLN_FUNC(, int, mln_pg_calc_nullable, \
         (volatile int *map, mln_pg_rule_t *r, mln_u32_t nr_rule), \
         (map, r, nr_rule), \
{
    int i, j, k, change = 1, nr_null;
    mln_pg_token_t *tk;
    mln_pg_rule_t *pr;
    mln_rbtree_node_t *rn;
    for (i = 0; i < nr_rule; ++i) {
        map[i] = 1;
    }
    while (change) {
        change = 0;
        for (i = 0; i < nr_rule; ++i) {
            if (map[i] == 0) continue;
            map[i] = 0;
            pr = &r[i];
            if (pr->left->is_nullable) continue;
            nr_null = 0;
            for (j = 0; j < pr->nr_right; ++j) {
                tk = pr->rights[j];
                if (tk->is_nullable) ++nr_null;
                if (!tk->is_nonterminal) {
                    rn = mln_rbtree_search(tk->first_set, tk);
                    if (mln_rbtree_null(rn, tk->first_set)) {
                        rn = mln_rbtree_node_new(tk->first_set, tk);
                        if (rn == NULL) {
                            mln_log(error, "No memory.\n");
                            return -1;
                        }
                        mln_rbtree_insert(tk->first_set, rn);
                    }
                }
            }
            if (nr_null == pr->nr_right) {
                for (k = 0; k < nr_rule; ++k) {
                    if (pr->left->right_rule_index[k])
                        map[k] = 1;
                }
                change = 1;
                pr->left->is_nullable = 1;
            }
        }
    }
    return 0;
})

/*
 * calc first
 */
MLN_FUNC(, int, mln_pg_calc_first, \
         (volatile int *map, mln_pg_rule_t *r, mln_u32_t nr_rule), \
         (map, r, nr_rule), \
{
    int i, j, change = 1;
    mln_pg_rule_t *pr;
    mln_pg_token_t *tk;
    struct mln_pg_info info;
    for (i = 0; i < nr_rule; ++i)
        map[i] = 1;
    while (change) {
        change = 0;
        for (i = 0; i < nr_rule; ++i) {
            if (map[i] == 0) continue;
            map[i] = 0;
            pr = &r[i];
            for (j = 0; j < pr->nr_right; ++j) {
                tk = pr->rights[j];
                info.change = &change;
                info.map = map;
                info.r = pr;
                info.tk = pr->left;
                info.nr_rule = nr_rule;
                if (mln_rbtree_iterate(tk->first_set, \
                                        mln_pg_calc_first_rbtree_iterate_handler, \
                                        &info) < 0)
                {
                    return -1;
                }
                if (tk->is_nullable == 0)
                    break;
            }
        }
    }
    return 0;
})

MLN_FUNC(static, int, mln_pg_calc_first_rbtree_iterate_handler, \
         (mln_rbtree_node_t *node, void *udata), (node, udata), \
{
    struct mln_pg_info *pgi = (struct mln_pg_info *)udata;
    mln_pg_token_t *tk = (mln_pg_token_t *)mln_rbtree_node_data_get(node);
    mln_rbtree_t *dest = pgi->tk->first_set;
    mln_rbtree_node_t *rn = mln_rbtree_search(dest, tk);
    if (!mln_rbtree_null(rn, dest)) return 0;
    rn = mln_rbtree_node_new(dest, tk);
    if (rn == NULL) {
        mln_log(error, "No memory.\n");
        return -1;
    }
    mln_rbtree_insert(dest, rn);
    *(pgi->change) = 1;
    int k;
    for (k = 0; k < pgi->nr_rule; ++k) {
        if (pgi->tk->right_rule_index[k]) {
            pgi->map[k] = 1;
        }
    }
    return 0;
})

/*
 * calc follow
 */
MLN_FUNC(, int, mln_pg_calc_follow, \
         (volatile int *map, mln_pg_rule_t *r, mln_u32_t nr_rule), \
         (map, r, nr_rule), \
{
    int i, j, k, change = 1, nr_null;
    mln_pg_rule_t *pr;
    mln_pg_token_t *tk, *behind;
    struct mln_pg_info info;

    for (i = 0; i < nr_rule; ++i)
        map[i] = 1;
    while (change) {
        change = 0;
        for (i = 0; i < nr_rule; ++i) {
            if (map[i] == 0) continue;
            map[i] = 0;
            pr = &r[i];
            nr_null = 1;
            for (j = pr->nr_right-1; j >= 0; --j) {
                tk = pr->rights[j];
                if (nr_null && nr_null == pr->nr_right-j) {
                    info.change = &change;
                    info.map = map;
                    info.r = pr;
                    info.tk = tk;
                    info.nr_rule = nr_rule;
                    if (mln_rbtree_iterate(pr->left->follow_set, \
                                            mln_pg_calc_follow_rbtree_iterate_handler, \
                                            &info) < 0)
                    {
                        return -1;
                    }
                    for (k = 1; k <= nr_null-1; ++k) {
                        behind = pr->rights[j+k];
                        info.change = &change;
                        info.map = map;
                        info.r = pr;
                        info.tk = tk;
                        info.nr_rule = nr_rule;
                        if (mln_rbtree_iterate(behind->first_set, \
                                                mln_pg_calc_follow_rbtree_iterate_handler, \
                                                &info) < 0)
                        {
                            return -1;
                        }
                    }
                } else {
                    for (k = 1; k <= nr_null+1; ++k) {
                        behind = pr->rights[j+k];
                        info.change = &change;
                        info.map = map;
                        info.r = pr;
                        info.tk = tk;
                        info.nr_rule = nr_rule;
                        if (mln_rbtree_iterate(behind->first_set, \
                                                mln_pg_calc_follow_rbtree_iterate_handler, \
                                                &info) < 0)
                        {
                            return -1;
                        }
                    }
                }
                nr_null = tk->is_nullable? nr_null+1: 0;
            }
        }
    }
    return 0;
})

MLN_FUNC(static, int, mln_pg_calc_follow_rbtree_iterate_handler, \
         (mln_rbtree_node_t *node, void *udata), (node, udata), \
{
    mln_pg_token_t *tk = (mln_pg_token_t *)mln_rbtree_node_data_get(node);
    struct mln_pg_info *pgi = (struct mln_pg_info *)udata;
    mln_rbtree_node_t *rn;
    mln_rbtree_t *dest = pgi->tk->follow_set;
    rn = mln_rbtree_search(dest, tk);
    if (!mln_rbtree_null(rn, dest)) return 0;
    if ((rn = mln_rbtree_node_new(dest, tk)) == NULL) {
        mln_log(error, "No memory.\n");
        return -1;
    }
    mln_rbtree_insert(dest, rn);
    *(pgi->change) = 1;
    int k;
    for (k = 0; k < pgi->nr_rule; ++k) {
        if (pgi->tk->right_rule_index[k])
            pgi->map[k] = 1;
    }
    return 0;
})

/*
 * closure
 */
MLN_FUNC(, int, mln_pg_closure, \
         (mln_pg_state_t *s, mln_pg_rule_t *r, mln_u32_t nr_rule), \
         (s, r, nr_rule), \
{
    int i, change = 1;
    mln_pg_item_t *item, *new_it;
    mln_pg_token_t *tk, *next_tk;
    mln_pg_rule_t *pr;
    struct mln_pg_state info;

    while (change) {
        change = 0;
        for (item = s->head; item != NULL; item = item->next) {
            if (item->rule->nr_right == item->pos) continue;
            tk = item->rule->rights[item->pos];
            if (!tk->is_nonterminal) continue;
            for (i = 0; i < nr_rule; ++i) {
                if (tk->left_rule_index[i] == 0) continue;
                pr = &r[i];
                new_it = mln_pg_rule_duplicate(s, pr, 0);
                if (new_it == NULL) {
                    new_it = mln_pg_item_new();
                    if (new_it == NULL) {
                        mln_log(error, "No memory.\n");
                        return -1;
                    }
                    ++(s->nr_item);
                    mln_item_chain_add(&(s->head), &(s->tail), new_it);
                    new_it->rule = pr;
                    new_it->pos = 0;
                }
                if (item->pos + 1 < item->rule->nr_right) {
                    next_tk = item->rule->rights[item->pos+1];
                    info.change = &change;
                    info.r = pr;
                    info.s = s;
                    info.item = new_it;
                    if (mln_rbtree_iterate(next_tk->first_set, \
                                            mln_pg_closure_rbtree_iterate_handler, \
                                            &info) < 0)
                    {
                        return -1;
                    }
                    if (!next_tk->is_nullable)
                        continue;
                }
                info.change = &change;
                info.r = pr;
                info.s = s;
                info.item = new_it;
                if (mln_rbtree_iterate(item->lookahead_set, \
                                        mln_pg_closure_rbtree_iterate_handler, \
                                        &info) < 0)
                {
                    return -1;
                }
            }
        }
    }

    return 0;
})

MLN_FUNC(static, mln_pg_item_t *, mln_pg_rule_duplicate, \
         (mln_pg_state_t *s, mln_pg_rule_t *r, mln_u32_t pos), \
         (s, r, pos), \
{
    mln_pg_item_t *item;
    for (item = s->head; item != NULL; item = item->next) {
        if (item->rule == r && item->pos == pos) {
            return item;
        }
    }
    return NULL;
})

MLN_FUNC(static, int, mln_pg_closure_rbtree_iterate_handler, \
         (mln_rbtree_node_t *node, void *udata), (node, udata), \
{
    struct mln_pg_state *info = (struct mln_pg_state *)udata;
    mln_pg_token_t *tk = (mln_pg_token_t *)mln_rbtree_node_data_get(node);
    mln_pg_item_t *new_it = info->item;
    mln_rbtree_node_t *rn;
    rn = mln_rbtree_search(new_it->lookahead_set, tk);
    if (!mln_rbtree_null(rn, new_it->lookahead_set)) return 0;
    rn = mln_rbtree_node_new(new_it->lookahead_set, tk);
    if (rn == NULL) {
        mln_log(error, "No memory.\n");
        return -1;
    }
    mln_rbtree_insert(new_it->lookahead_set, rn);
    *(info->change) = 1;
    return 0;
})

/*
 * goto
 */
MLN_FUNC_VOID(static inline, void, mln_pg_state_enqueue, \
              (mln_pg_state_t **head, mln_pg_state_t **tail, mln_pg_state_t *s), \
              (head, tail, s), \
{
    s->q_next = NULL;
    if (*head == NULL) {
        *head = *tail = s;
        return;
    }
    (*tail)->q_next = s;
    *tail = s;
})

MLN_FUNC(static inline, mln_pg_state_t *, mln_pg_state_dequeue, \
         (mln_pg_state_t **head, mln_pg_state_t **tail), (head, tail), \
{
    mln_pg_state_t *s = *head;
    if (s == NULL) return NULL;
    *head = s->q_next;
    if (*head == NULL) *tail = NULL;
    s->q_next = NULL;
    return s;
})

MLN_FUNC(, int, mln_pg_goto, (struct mln_pg_calc_info_s *pci), (pci), {\
    mln_rbtree_t *tree = pci->tree;
    mln_rbtree_node_t *rn;
    mln_u32_t nr_end, nr_cnt;
    mln_pg_token_t *tk;
    mln_pg_state_t *new_s, *tmp_s;
    mln_pg_item_t *save, *it, *new_it;
    mln_pg_state_t *q_head = NULL, *q_tail = NULL, *s;
    s = mln_pg_state_new();
    if (s == NULL) {
        mln_log(error, "No memory.\n");
        return -1;
    }
    s->id = (pci->id_counter)++;
    s->input = pci->first_input;
    new_it = mln_pg_item_new();
    if (new_it == NULL) {
        mln_log(error, "No memory.\n");
        mln_pg_state_free(s);
        return -1;
    }
    new_it->rule = &pci->rule[0];
    mln_item_chain_add(&(s->head), &(s->tail), new_it);
    ++(s->nr_item);
    if ((rn = mln_rbtree_node_new(tree, s)) == NULL) {
        mln_log(error, "No memory.\n");
        mln_pg_state_free(s);
        return -1;
    }
    mln_rbtree_insert(tree, rn);
    mln_state_chain_add(&(pci->head), &(pci->tail), s);
    mln_pg_state_enqueue(&q_head, &q_tail, s);
    if (mln_pg_closure(s, pci->rule, pci->nr_rule) < 0) {
        return -1;
    }

    while ((s = mln_pg_state_dequeue(&q_head, &q_tail)) != NULL) {
        for (save = s->head; save != NULL; save = save->next) {
            if (save->rule->nr_right == save->pos)
                continue;
            nr_end = nr_cnt = 0;
            tk = save->rule->rights[save->pos];
            for (it = save->prev; it != NULL; it = it->prev) {
                ++nr_cnt;
                if (it->rule->nr_right == it->pos) {
                    ++nr_end;
                    continue;
                }
                if (it->rule->rights[it->pos] == tk) {
                    break;
                }
            }
            if (it != NULL || (nr_end == nr_cnt && nr_end != 0))
                continue;

            new_s = mln_pg_state_new();
            if (new_s == NULL) {
                mln_log(error, "No memory.\n");
                return -1;
            }
            new_s->input = tk;
            for (it = save; it != NULL; it = it->next) {
                if (it->rule->nr_right == it->pos) continue;
                if (it->rule->rights[it->pos] != tk) continue;
                new_it = mln_pg_item_new();
                if (new_it == NULL) {
                    mln_log(error, "No memory.\n");
                    mln_pg_state_free(new_s);
                    return -1;
                }
                ++(new_s->nr_item);
                mln_item_chain_add(&(new_s->head), &(new_s->tail), new_it);
                it->read = tk;
                new_it->rule = it->rule;
                new_it->pos = it->pos + 1;
                if (mln_rbtree_iterate(it->lookahead_set, \
                                        mln_pg_goto_iterate_handler, \
                                        new_it) < 0)
                {
                    mln_pg_state_free(new_s);
                    return -1;
                }
            }

            if (mln_pg_closure(new_s, pci->rule, pci->nr_rule) < 0) {
                mln_pg_state_free(new_s);
                return -1;
            }

            rn = mln_rbtree_search(tree, new_s);
            if (!mln_rbtree_null(rn, tree)) {
                tmp_s = (mln_pg_state_t *)mln_rbtree_node_data_get(rn);
                if (mln_pg_state_duplicate(&q_head, &q_tail, tmp_s, new_s) < 0) {
                    mln_pg_state_free(new_s);
                    return -1;
                }
                mln_pg_state_free(new_s);
                new_s = tmp_s;
            } else {
                new_s->id = (pci->id_counter)++;
                if ((rn = mln_rbtree_node_new(tree, new_s)) == NULL) {
                    mln_log(error, "No memory.\n");
                    mln_pg_state_free(new_s);
                    return -1;
                }
                mln_rbtree_insert(tree, rn);
                mln_state_chain_add(&(pci->head), &(pci->tail), new_s);
                mln_pg_state_enqueue(&q_head, &q_tail, new_s);
            }

            for (it = save; it != NULL; it = it->next) {
                if (it->rule->nr_right == it->pos) continue;
                if (it->rule->rights[it->pos] == tk) {
                    it->goto_id = new_s->id;
                }
            }
        }
    }
    pci->head->input = NULL;
    return 0;
})

MLN_FUNC(static, int, mln_pg_goto_iterate_handler, \
         (mln_rbtree_node_t *node, void *udata), (node, udata), \
{
    mln_pg_item_t *it = (mln_pg_item_t *)udata;
    mln_pg_token_t *tk = (mln_pg_token_t *)mln_rbtree_node_data_get(node);
    mln_rbtree_node_t *rn = mln_rbtree_node_new(it->lookahead_set, tk);
    if (rn == NULL) {
        mln_log(error, "No memory.\n");
        return -1;
    }
    mln_rbtree_insert(it->lookahead_set, rn);
    return 0;
})

MLN_FUNC(static inline, int, mln_pg_state_duplicate, \
         (mln_pg_state_t **q_head, mln_pg_state_t **q_tail, \
          mln_pg_state_t *dest, mln_pg_state_t *src), \
         (q_head, q_tail, dest, src), \
{
    int change, added = 0;
    mln_pg_item_t *dit, *sit;
    struct mln_pg_state info;
    for (dit = dest->head; dit != NULL; dit = dit->next) {
        change = 0;
        for (sit = src->head; sit != NULL; sit = sit->next) {
            if (dit->rule == sit->rule && dit->pos == sit->pos) {
                info.change = &change;
                info.item = dit;
                if (mln_rbtree_iterate(sit->lookahead_set, \
                                        mln_pg_goto_la_iterate_handler, \
                                        &info) < 0)
                {
                    return -1;
                }
                if (change && !added) {
                    added = 1;
                    if (dest->q_next == NULL && (*q_tail) != dest)
                        mln_pg_state_enqueue(q_head, q_tail, dest);
                }
                break;
            }
        }
    }
    return 0;
})

MLN_FUNC(static, int, mln_pg_goto_la_iterate_handler, \
         (mln_rbtree_node_t *node, void *udata), (node, udata), \
{
    struct mln_pg_state *info = (struct mln_pg_state *)udata;
    mln_pg_token_t *tk = (mln_pg_token_t *)mln_rbtree_node_data_get(node);
    mln_rbtree_node_t *rn;
    rn = mln_rbtree_search(info->item->lookahead_set, tk);
    if (!mln_rbtree_null(rn, info->item->lookahead_set))
        return 0;
    rn = mln_rbtree_node_new(info->item->lookahead_set, tk);
    if (rn == NULL) {
        mln_log(error, "No memory.\n");
        return -1;
    }
    mln_rbtree_insert(info->item->lookahead_set, rn);
    *(info->change) = 1;
    return 0;
})

/*
 * output information
 */
MLN_FUNC_VOID(, void, mln_pg_output_token, \
              (mln_rbtree_t *tree, mln_pg_rule_t *r, mln_u32_t nr_rule), \
              (tree, r, nr_rule), \
{
    mln_pg_token_t *tk;
    mln_pg_rule_t *pr;
    int i, j;
    printf("RULE:\n");
    for (i = 0; i < nr_rule; ++i) {
        pr = &r[i];
        printf("No.%d %s->", i, (char *)(pr->left->token->data));
        for (j = 0; j < pr->nr_right; ++j) {
            tk = pr->rights[j];
            printf("%s ", (char *)(tk->token->data));
        }
        printf("\n");
    }
    printf("\n");
    printf("TOKEN:\n");
    mln_rbtree_iterate(tree, mln_pg_output_token_iterate_handler, NULL);
})

MLN_FUNC(static, int, mln_pg_output_token_iterate_handler, \
         (mln_rbtree_node_t *node, void *udata), (node, udata), \
{
    mln_pg_token_t *tk = (mln_pg_token_t *)mln_rbtree_node_data_get(node);
    printf("[%s]:", (char *)(tk->token->data));
    printf("%d %s %s\n", tk->type, \
           tk->is_nonterminal?"Nonterminal":"Terminal", \
           tk->is_nullable?"Nullable":"Unnullable");
    printf("First set:\n");
    mln_rbtree_iterate(tk->first_set, mln_pg_output_token_set_iterate_handler, NULL);
    printf("\n");
    printf("Follow set:\n");
    mln_rbtree_iterate(tk->follow_set, mln_pg_output_token_set_iterate_handler, NULL);
    printf("\n");
    printf("\n");
    return 0;
})

MLN_FUNC(static, int, mln_pg_output_token_set_iterate_handler, \
         (mln_rbtree_node_t *node, void *udata), (node, udata), \
{
    mln_pg_token_t *tk = (mln_pg_token_t *)mln_rbtree_node_data_get(node);
    printf("[%s] ", (char *)(tk->token->data));
    return 0;
})

MLN_FUNC_VOID(, void, mln_pg_output_state, (mln_pg_state_t *s), (s), {
    mln_pg_item_t *it;
    mln_u32_t i;
    printf("STATES:\n");
    for (; s != NULL; s = s->next) {
        printf("State %ld: input: [%s] nr_item:%llu\n", \
               s->id, \
               s->input==NULL?"(null)":(char *)(s->input->token->data), \
               (unsigned long long)(s->nr_item));
        printf("Items:\n");
        for (it = s->head; it != NULL; it = it->next) {
            printf("rule: %s->", (char *)(it->rule->left->token->data));
            for (i = 0; i < it->rule->nr_right; ++i) {
                if (it->pos == i) printf(".");
                printf(" %s", (char *)((it->rule->rights[i])->token->data));
            }
            printf("\t\tread:[%s] goto_id:%ld", it->read==NULL?"(null)":(char *)(it->read->token->data), it->goto_id);
            printf("\tLookahead:");
            mln_rbtree_iterate(it->lookahead_set, mln_pg_output_state_iterate_handler, NULL);
            printf("\n");
        }
        printf("\n");
    }
})

MLN_FUNC(static, int, mln_pg_output_state_iterate_handler, \
         (mln_rbtree_node_t *node, void *udata), (node, udata), \
{
    printf(" %s", (char *)(((mln_pg_token_t *)mln_rbtree_node_data_get(node))->token->data));
    return 0;
})

