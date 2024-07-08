#include <stdio.h>
#include "mln_string.h"
#include "mln_json.h"

static int callback(mln_json_t *j, void *data)
{
    mln_json_number_init(j, *(int *)data);
    return 0;
}

static int handler(mln_json_t *j, void *data)
{
    mln_json_dump(j, 0, "");
    return 0;
}

int main(int argc, char *argv[])
{
    int i = 1024;
    mln_json_t j, k;
    struct mln_json_call_attr ca;
    mln_string_t *res, exp = mln_string("protocols.0");
    mln_string_t tmp = mln_string("{\"paths\":[\"/mock\"],\"methods\":null,\"sources\":null,\"destinations\":null,\"name\":\"example_route\",\"headers\":null,\"hosts\":null,\"preserve_host\":false,\"regex_priority\":0,\"snis\":null,\"https_redirect_status_code\":426,\"tags\":null,\"protocols\":[\"http\",\"https\"],\"path_handling\":\"v0\",\"id\":\"52d58293-ae25-4c69-acc8-6dd729718a61\",\"updated_at\":1661345592,\"service\":{\"id\":\"c1e98b2b-6e77-476c-82ca-a5f1fb877e07\"},\"response_buffering\":true,\"strip_path\":true,\"request_buffering\":true,\"created_at\":1661345592}");

    if (mln_json_decode(&tmp, &j, NULL) < 0) {
        fprintf(stderr, "decode error\n");
        return -1;
    }

    mln_json_parse(&j, &exp, handler, NULL);

    ca.callback = callback;
    ca.data = &i;
    mln_json_init(&k);
    if (mln_json_generate(&k, "[{s:d,s:d,s:{s:d}},d,[],j,c]", "a", 1, "b", 3, "c", "d", 4, 5, &j, &ca) < 0) {
        fprintf(stderr, "generate failed\n");
        return -1;
    }
    mln_json_generate(&k, "[s,d]", "g", 99);
    res = mln_json_encode(&k);

    mln_json_destroy(&k);

    if (res == NULL) {
        fprintf(stderr, "encode failed\n");
        return -1;
    }
    write(STDOUT_FILENO, res->data, res->len);
    write(STDOUT_FILENO, "\n", 1);
    mln_string_free(res);

    return 0;
}
