#include "mln_connection.h"
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <assert.h>

int main(void)
{
    int fds[2];
    mln_tcp_conn_t conn1, conn2;
    mln_chain_t *c;
    mln_buf_t *b;
    mln_alloc_t *pool;
    mln_u8ptr_t buf;

    assert(socketpair(AF_UNIX, SOCK_STREAM, 0, fds) == 0);
    assert(mln_tcp_conn_init(&conn1, fds[0]) == 0);
    assert(mln_tcp_conn_init(&conn2, fds[1]) == 0);

    pool = mln_tcp_conn_pool_get(&conn1);

    assert((buf = (mln_u8ptr_t)mln_alloc_m(pool, 10)) != NULL);
    memset(buf, 'a', 9);
    buf[9] = 0;
    assert((c = mln_chain_new(pool)) != NULL);
    assert((b = mln_buf_new(pool)) != NULL);
    c->buf = b;
    b->left_pos = b->pos = b->start = buf;
    b->last = b->end = buf + 10;
    b->in_memory = 1;
    b->last_buf = 1;
    b->last_in_chain = 1;
    mln_tcp_conn_append(&conn1, c, M_C_SEND);

    assert(mln_tcp_conn_send(&conn1) == M_C_FINISH);

    assert(mln_tcp_conn_recv(&conn2, M_C_TYPE_MEMORY) != M_C_ERROR);

    assert(memcmp(mln_tcp_conn_head(&conn2, M_C_RECV)->buf->start, buf, 10) == 0);

    mln_tcp_conn_destroy(&conn1);
    mln_tcp_conn_destroy(&conn2);
    close(fds[0]);
    close(fds[1]);

    return 0;
}

