#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <time.h>
#include "mln_conf.h"
#include "mln_path.h"
#include "mln_func.h"

static char conf_path[] = "/tmp/melon_test_conf.conf";

static char *return_conf_path(void)
{
    return conf_path;
}

static int reload_called = 0;

MLN_FUNC(static, int, test_reload_handler, (void *data), (data), {
    int *counter = (int *)data;
    (*counter)++;
    reload_called = 1;
    return 0;
})

MLN_FUNC_VOID(static, void, write_test_conf, (void), (), {
    FILE *fp = fopen(conf_path, "w");
    assert(fp != NULL);
    fprintf(fp,
        "log_level \"none\";\n"
        "daemon off;\n"
        "core_file_size \"unlimited\";\n"
        "worker_proc 1;\n"
        "framework off;\n"
        "log_path \"/tmp/melon.log\";\n"
        "test_int 42;\n"
        "test_neg -100;\n"
        "test_float 3.14;\n"
        "test_char 'A';\n"
        "test_bool_on on;\n"
        "test_bool_off off;\n"
        "test_multi \"hello\" \"world\" 123;\n"
        "proc_exec {\n"
        "}\n"
        "thread_exec {\n"
        "}\n"
        "custom_domain {\n"
        "    setting1 \"value1\";\n"
        "    setting2 100;\n"
        "}\n"
    );
    fclose(fp);
})

/* test: load configuration */
MLN_FUNC_VOID(static, void, test_load, (void), (), {
    write_test_conf();
    assert(mln_conf_load() == 0);
    /* calling load again should be a no-op */
    assert(mln_conf_load() == 0);
    mln_conf_t *cf = mln_conf();
    assert(cf != NULL);
    printf("  test_load passed\n");
})

/* test: domain search */
MLN_FUNC_VOID(static, void, test_domain_search, (void), (), {
    mln_conf_t *cf = mln_conf();
    mln_conf_domain_t *cd;

    cd = cf->search(cf, "main");
    assert(cd != NULL);

    cd = cf->search(cf, "proc_exec");
    assert(cd != NULL);

    cd = cf->search(cf, "thread_exec");
    assert(cd != NULL);

    cd = cf->search(cf, "custom_domain");
    assert(cd != NULL);

    cd = cf->search(cf, "nonexistent");
    assert(cd == NULL);

    printf("  test_domain_search passed\n");
})

/* test: command search */
MLN_FUNC_VOID(static, void, test_cmd_search, (void), (), {
    mln_conf_t *cf = mln_conf();
    mln_conf_domain_t *cd = cf->search(cf, "main");
    assert(cd != NULL);

    mln_conf_cmd_t *cc;
    cc = cd->search(cd, "framework");
    assert(cc != NULL);

    cc = cd->search(cd, "daemon");
    assert(cc != NULL);

    cc = cd->search(cd, "log_level");
    assert(cc != NULL);

    cc = cd->search(cd, "nonexistent_cmd");
    assert(cc == NULL);

    printf("  test_cmd_search passed\n");
})

/* test: all item types */
MLN_FUNC_VOID(static, void, test_item_types, (void), (), {
    mln_conf_t *cf = mln_conf();
    mln_conf_domain_t *cd = cf->search(cf, "main");
    mln_conf_cmd_t *cc;
    mln_conf_item_t *ci;

    /* CONF_STR */
    cc = cd->search(cd, "log_level");
    assert(cc != NULL);
    ci = cc->search(cc, 1);
    assert(ci != NULL);
    assert(ci->type == CONF_STR);
    assert(strcmp((char *)ci->val.s->data, "none") == 0);

    /* CONF_BOOL off */
    cc = cd->search(cd, "framework");
    ci = cc->search(cc, 1);
    assert(ci != NULL);
    assert(ci->type == CONF_BOOL);
    assert(ci->val.b == 0);

    /* CONF_BOOL on */
    cc = cd->search(cd, "test_bool_on");
    ci = cc->search(cc, 1);
    assert(ci != NULL);
    assert(ci->type == CONF_BOOL);
    assert(ci->val.b == 1);

    /* CONF_INT */
    cc = cd->search(cd, "test_int");
    ci = cc->search(cc, 1);
    assert(ci != NULL);
    assert(ci->type == CONF_INT);
    assert(ci->val.i == 42);

    /* CONF_INT negative */
    cc = cd->search(cd, "test_neg");
    ci = cc->search(cc, 1);
    assert(ci != NULL);
    assert(ci->type == CONF_INT);
    assert(ci->val.i == -100);

    /* CONF_FLOAT */
    cc = cd->search(cd, "test_float");
    ci = cc->search(cc, 1);
    assert(ci != NULL);
    assert(ci->type == CONF_FLOAT);
    assert(ci->val.f > 3.13f && ci->val.f < 3.15f);

    /* CONF_CHAR */
    cc = cd->search(cd, "test_char");
    ci = cc->search(cc, 1);
    assert(ci != NULL);
    assert(ci->type == CONF_CHAR);
    assert(ci->val.c == 'A');

    /* out of bounds index returns NULL */
    cc = cd->search(cd, "test_int");
    ci = cc->search(cc, 0);
    assert(ci == NULL);
    ci = cc->search(cc, 2);
    assert(ci == NULL);

    printf("  test_item_types passed\n");
})

/* test: multi-argument command */
MLN_FUNC_VOID(static, void, test_multi_args, (void), (), {
    mln_conf_t *cf = mln_conf();
    mln_conf_domain_t *cd = cf->search(cf, "main");
    mln_conf_cmd_t *cc = cd->search(cd, "test_multi");
    assert(cc != NULL);

    assert(mln_conf_arg_num(cc) == 3);

    mln_conf_item_t *ci;
    ci = cc->search(cc, 1);
    assert(ci != NULL && ci->type == CONF_STR);
    assert(strcmp((char *)ci->val.s->data, "hello") == 0);

    ci = cc->search(cc, 2);
    assert(ci != NULL && ci->type == CONF_STR);
    assert(strcmp((char *)ci->val.s->data, "world") == 0);

    ci = cc->search(cc, 3);
    assert(ci != NULL && ci->type == CONF_INT);
    assert(ci->val.i == 123);

    printf("  test_multi_args passed\n");
})

/* test: subdomain commands */
MLN_FUNC_VOID(static, void, test_subdomain, (void), (), {
    mln_conf_t *cf = mln_conf();
    mln_conf_domain_t *cd = cf->search(cf, "custom_domain");
    assert(cd != NULL);

    mln_conf_cmd_t *cc = cd->search(cd, "setting1");
    assert(cc != NULL);
    mln_conf_item_t *ci = cc->search(cc, 1);
    assert(ci != NULL && ci->type == CONF_STR);
    assert(strcmp((char *)ci->val.s->data, "value1") == 0);

    cc = cd->search(cd, "setting2");
    assert(cc != NULL);
    ci = cc->search(cc, 1);
    assert(ci != NULL && ci->type == CONF_INT);
    assert(ci->val.i == 100);

    printf("  test_subdomain passed\n");
})

/* test: insert and remove domain */
MLN_FUNC_VOID(static, void, test_insert_remove_domain, (void), (), {
    mln_conf_t *cf = mln_conf();

    mln_conf_domain_t *cd = cf->insert(cf, "new_domain");
    assert(cd != NULL);
    assert(cf->search(cf, "new_domain") == cd);

    cf->remove(cf, "new_domain");
    assert(cf->search(cf, "new_domain") == NULL);

    /* double remove should be safe */
    cf->remove(cf, "new_domain");

    printf("  test_insert_remove_domain passed\n");
})

/* test: insert and remove command */
MLN_FUNC_VOID(static, void, test_insert_remove_cmd, (void), (), {
    mln_conf_t *cf = mln_conf();
    mln_conf_domain_t *cd = cf->search(cf, "main");
    assert(cd != NULL);

    mln_conf_cmd_t *cc = cd->insert(cd, "dynamic_cmd");
    assert(cc != NULL);
    assert(cd->search(cd, "dynamic_cmd") == cc);

    cd->remove(cd, "dynamic_cmd");
    assert(cd->search(cd, "dynamic_cmd") == NULL);

    /* double remove should be safe */
    cd->remove(cd, "dynamic_cmd");

    printf("  test_insert_remove_cmd passed\n");
})

/* test: update command items */
MLN_FUNC_VOID(static, void, test_update, (void), (), {
    mln_conf_t *cf = mln_conf();
    mln_conf_domain_t *cd = cf->search(cf, "main");

    mln_conf_cmd_t *cc = cd->insert(cd, "update_test");
    assert(cc != NULL);

    mln_string_t s1 = mln_string("updated_val");
    mln_conf_item_t items[2];
    items[0].type = CONF_STR;
    items[0].val.s = &s1;
    items[1].type = CONF_INT;
    items[1].val.i = 999;

    assert(cc->update(cc, items, 2) == 0);
    assert(mln_conf_arg_num(cc) == 2);

    mln_conf_item_t *ci = cc->search(cc, 1);
    assert(ci != NULL && ci->type == CONF_STR);
    assert(strcmp((char *)ci->val.s->data, "updated_val") == 0);

    ci = cc->search(cc, 2);
    assert(ci != NULL && ci->type == CONF_INT);
    assert(ci->val.i == 999);

    /* update again to test overwrite */
    mln_conf_item_t items2[1];
    items2[0].type = CONF_BOOL;
    items2[0].val.b = 1;
    assert(cc->update(cc, items2, 1) == 0);
    assert(mln_conf_arg_num(cc) == 1);

    ci = cc->search(cc, 1);
    assert(ci != NULL && ci->type == CONF_BOOL);
    assert(ci->val.b == 1);

    cd->remove(cd, "update_test");

    printf("  test_update passed\n");
})

/* test: cmd_num, cmds, arg_num */
MLN_FUNC_VOID(static, void, test_cmd_num_cmds, (void), (), {
    mln_conf_t *cf = mln_conf();

    mln_u32_t n = mln_conf_cmd_num(cf, "main");
    assert(n > 0);

    mln_conf_cmd_t **v = (mln_conf_cmd_t **)malloc(n * sizeof(mln_conf_cmd_t *));
    assert(v != NULL);
    mln_conf_cmds(cf, "main", v);

    mln_u32_t i;
    for (i = 0; i < n; ++i) {
        assert(v[i] != NULL);
        assert(v[i]->cmd_name != NULL);
    }
    free(v);

    /* nonexistent domain */
    assert(mln_conf_cmd_num(cf, "nonexistent") == 0);

    printf("  test_cmd_num_cmds passed\n");
})

/* test: mln_conf_is_empty */
MLN_FUNC_VOID(static, void, test_is_empty, (void), (), {
    mln_conf_t *cf = mln_conf();
    assert(!mln_conf_is_empty(cf));
    mln_conf_t *null_cf = NULL;
    assert(mln_conf_is_empty(null_cf));

    printf("  test_is_empty passed\n");
})

/* test: hook set/unset and reload */
MLN_FUNC_VOID(static, void, test_hook_reload, (void), (), {
    int counter = 0;
    mln_conf_hook_t *hook = mln_conf_hook_set(test_reload_handler, &counter);
    assert(hook != NULL);

    reload_called = 0;
    assert(mln_conf_reload() == 0);
    assert(reload_called == 1);
    assert(counter == 1);

    /* reload again */
    reload_called = 0;
    assert(mln_conf_reload() == 0);
    assert(reload_called == 1);
    assert(counter == 2);

    /* unset hook and reload */
    mln_conf_hook_unset(hook);
    reload_called = 0;
    assert(mln_conf_reload() == 0);
    assert(reload_called == 0);

    printf("  test_hook_reload passed\n");
})

/* test: hook_free */
MLN_FUNC_VOID(static, void, test_hook_free, (void), (), {
    int c1 = 0, c2 = 0;
    mln_conf_hook_set(test_reload_handler, &c1);
    mln_conf_hook_set(test_reload_handler, &c2);
    mln_conf_hook_free();

    reload_called = 0;
    assert(mln_conf_reload() == 0);
    assert(reload_called == 0);

    printf("  test_hook_free passed\n");
})

/* test: dump (just ensure no crash) */
MLN_FUNC_VOID(static, void, test_dump, (void), (), {
    mln_conf_dump();
    printf("  test_dump passed\n");
})

/* test: free and re-load */
MLN_FUNC_VOID(static, void, test_free_reload, (void), (), {
    mln_conf_free();
    assert(mln_conf() == NULL);

    assert(mln_conf_load() == 0);
    mln_conf_t *cf = mln_conf();
    assert(cf != NULL);

    mln_conf_domain_t *cd = cf->search(cf, "main");
    assert(cd != NULL);
    mln_conf_cmd_t *cc = cd->search(cd, "framework");
    assert(cc != NULL);

    printf("  test_free_reload passed\n");
})

/* performance benchmark */
MLN_FUNC_VOID(static, void, test_benchmark, (void), (), {
    mln_conf_t *cf = mln_conf();
    mln_conf_domain_t *cd;
    mln_conf_cmd_t *cc;
    mln_conf_item_t *ci;
    struct timespec start, end;
    double elapsed;
    int i;
    int iterations = 1000000;

    /* benchmark: domain search */
    clock_gettime(CLOCK_MONOTONIC, &start);
    for (i = 0; i < iterations; ++i) {
        cd = cf->search(cf, "main");
        assert(cd != NULL);
    }
    clock_gettime(CLOCK_MONOTONIC, &end);
    elapsed = (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1e9;
    printf("  domain search: %d iterations in %.4f s (%.0f ops/s)\n",
           iterations, elapsed, iterations / elapsed);

    /* benchmark: command search */
    cd = cf->search(cf, "main");
    clock_gettime(CLOCK_MONOTONIC, &start);
    for (i = 0; i < iterations; ++i) {
        cc = cd->search(cd, "framework");
        assert(cc != NULL);
    }
    clock_gettime(CLOCK_MONOTONIC, &end);
    elapsed = (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1e9;
    printf("  cmd search: %d iterations in %.4f s (%.0f ops/s)\n",
           iterations, elapsed, iterations / elapsed);

    /* benchmark: item search */
    cc = cd->search(cd, "framework");
    clock_gettime(CLOCK_MONOTONIC, &start);
    for (i = 0; i < iterations; ++i) {
        ci = cc->search(cc, 1);
        assert(ci != NULL);
    }
    clock_gettime(CLOCK_MONOTONIC, &end);
    elapsed = (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1e9;
    printf("  item search: %d iterations in %.4f s (%.0f ops/s)\n",
           iterations, elapsed, iterations / elapsed);

    /* benchmark: full lookup chain (domain -> cmd -> item) */
    clock_gettime(CLOCK_MONOTONIC, &start);
    for (i = 0; i < iterations; ++i) {
        cd = cf->search(cf, "main");
        cc = cd->search(cd, "framework");
        ci = cc->search(cc, 1);
    }
    clock_gettime(CLOCK_MONOTONIC, &end);
    elapsed = (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1e9;
    printf("  full lookup chain: %d iterations in %.4f s (%.0f ops/s)\n",
           iterations, elapsed, iterations / elapsed);

    /* benchmark: load/free cycle */
    int load_iters = 1000;
    clock_gettime(CLOCK_MONOTONIC, &start);
    for (i = 0; i < load_iters; ++i) {
        mln_conf_free();
        assert(mln_conf_load() == 0);
    }
    clock_gettime(CLOCK_MONOTONIC, &end);
    elapsed = (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1e9;
    printf("  load/free cycle: %d iterations in %.4f s (%.0f ops/s)\n",
           load_iters, elapsed, load_iters / elapsed);
})

int main(int argc, char *argv[])
{
    (void)argc;
    (void)argv;

    mln_path_hook_set(m_p_conf, return_conf_path);

    printf("Running conf tests...\n");

    test_load();
    test_domain_search();
    test_cmd_search();
    test_item_types();
    test_multi_args();
    test_subdomain();
    test_insert_remove_domain();
    test_insert_remove_cmd();
    test_update();
    test_cmd_num_cmds();
    test_is_empty();
    test_hook_reload();
    test_hook_free();
    test_dump();
    test_free_reload();

    printf("\nAll feature tests passed!\n\n");

    printf("Running benchmarks...\n");
    test_benchmark();

    mln_conf_free();
    mln_conf_hook_free();

    printf("\nAll tests completed successfully.\n");
    return 0;
}

