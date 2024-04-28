#include <stdio.h>
#include "mln_lang.h"
#include "mln_event.h"
#include <assert.h>
#include <sys/types.h>
#include <sys/socket.h>

int fds[2];

static int lang_signal(mln_lang_t *lang)
{
    return mln_event_fd_set(mln_lang_event_get(lang), fds[0], M_EV_SEND|M_EV_ONESHOT, M_EV_UNLIMITED, lang, mln_lang_launcher_get(lang));
}

static int lang_clear(mln_lang_t *lang)
{
    return mln_event_fd_set(mln_lang_event_get(lang), fds[0], M_EV_CLR, M_EV_UNLIMITED, NULL, NULL);
}

static void return_handler(mln_lang_ctx_t *ctx)
{
    mln_event_break_set(mln_lang_event_get(ctx->lang));
}

int main(void)
{
    mln_string_t code = mln_string("Dump('Hello');");
    mln_event_t *ev;
    mln_lang_t *lang;

    assert(socketpair(AF_UNIX, SOCK_STREAM, 0, fds) == 0);

    assert((ev = mln_event_new()) != NULL);
    assert((lang = mln_lang_new(ev, lang_signal, lang_clear)) != NULL);
    assert(mln_lang_job_new(lang, NULL, M_INPUT_T_BUF, &code, NULL, return_handler));

    mln_event_dispatch(ev);

    mln_lang_free(lang);
    mln_event_free(ev);
    close(fds[0]);
    close(fds[1]);
    return 0;
}
