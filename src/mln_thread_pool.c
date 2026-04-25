
/*
 * Copyright (C) Niklaus F.Schen.
 */

#if !defined(MSVC)

#include <time.h>
#include <stdlib.h>
#include "mln_thread_pool.h"
#include <string.h>
#include <errno.h>
#include <stdio.h>
#include "mln_utils.h"
#include "mln_func.h"

/*
 * There is a problem in linux.
 * I don't know whether it's a bug or not.
 * If I fork a child process in the callback function
 * 'process_handler' which is running on child thread,
 * there is always 288-byte(64-bit)/144-byte(32-bit)
 * block leak in child process.
 * I checked I had already freed all blocks which were
 * allocated by malloc(). But it still existed.
 * I remember that there is a 288-byte(64-bit)/144-byte(32-bit)
 * block allocated during calling pthread_create().
 * But in child process, I can not access that
 * 288-byte(64-bit)/144-byte(32-bit) memory in user mode,
 * which means I can't free this memory block.
 *
 * And there is another thing which is that FreeBSD 10.0 i386
 * can not support fork() in thread perfectly.
 * The child process will be blocked mostly.
 * GDB can not attach that blocked process.
 * Command 'truss' can not see which syscall it encountered.
 * So if someone can explain it, I wiil be so grateful while
 * you contact me immediately.
 * Now, I just wanna say, fork() in thread is NOT recommended.
 */

/*
 * Cap on the number of resource nodes cached in the per-pool free
 * list. Anything beyond this is returned to libc - keeps long-running
 * pools that see spiky workloads from holding onto a lot of memory.
 */
#define MLN_THREAD_POOL_FREE_LIST_MAX 1024

/*
 * Maximum number of items a worker may grab in one mutex acquisition
 * before processing them outside the lock. Higher values amortise the
 * lock cost across more items but make load balancing slightly coarser.
 */
#define MLN_THREAD_POOL_BATCH 16

/*
 * Atomic helpers. We rely on GCC/Clang __atomic_*. These are
 * available on every supported platform (Linux, *BSD, macOS,
 * MSYS2/MinGW). We avoid C11 stdatomic to keep a -std=gnu89-clean
 * compile under -Werror, matching the rest of the project.
 */
#define MLN_ATOMIC_LOAD(p)            __atomic_load_n((p), __ATOMIC_ACQUIRE)
#define MLN_ATOMIC_RELAXED_LOAD(p)    __atomic_load_n((p), __ATOMIC_RELAXED)
#define MLN_ATOMIC_STORE(p, v)        __atomic_store_n((p), (v), __ATOMIC_RELEASE)
#define MLN_ATOMIC_RELAXED_STORE(p,v) __atomic_store_n((p), (v), __ATOMIC_RELAXED)
#define MLN_ATOMIC_CAS_WEAK(p, e, n)  __atomic_compare_exchange_n((p), (e), (n), 1, __ATOMIC_RELEASE, __ATOMIC_RELAXED)
#define MLN_ATOMIC_EXCHANGE(p, v)     __atomic_exchange_n((p), (v), __ATOMIC_ACQ_REL)
#define MLN_ATOMIC_INC(p)             __atomic_add_fetch((p), 1, __ATOMIC_ACQ_REL)
#define MLN_ATOMIC_DEC(p)             __atomic_sub_fetch((p), 1, __ATOMIC_ACQ_REL)

__thread mln_thread_pool_member_t *m_thread_pool_self = NULL;

static void *child_thread_launcher(void *arg);
static void mln_thread_pool_free(mln_thread_pool_t *tp);

MLN_CHAIN_FUNC_DECLARE(static inline, \
                       mln_child, \
                       mln_thread_pool_member_t, );

/*
 * thread_pool_member
 */
MLN_FUNC(static inline, mln_thread_pool_member_t *, mln_thread_pool_member_new, \
         (mln_thread_pool_t *tpool, mln_u32_t child), (tpool, child), \
{
    mln_thread_pool_member_t *tpm;
    if ((tpm = (mln_thread_pool_member_t *)malloc(sizeof(mln_thread_pool_member_t))) == NULL) {
        return NULL;
    }
    tpm->data = NULL;
    tpm->pool = tpool;
    tpm->idle = 1;
    tpm->locked = 0;
    tpm->forked = 0;
    tpm->child = child;
    tpm->prev = tpm->next = NULL;
    return tpm;
})

MLN_FUNC_VOID(static inline, void, mln_thread_pool_member_free, \
              (mln_thread_pool_member_t *member), (member), \
{
    if (member == NULL) return;
    if (member->data != NULL && member->pool->free_handler != NULL)
        member->pool->free_handler(member->data);
    free(member);
})

MLN_FUNC(static, mln_thread_pool_member_t *, mln_thread_pool_member_join, \
         (mln_thread_pool_t *tp, mln_u32_t child), (tp, child), \
{
    /*
     * @ mutex must be locked by caller.
     * and m_thread_pool_self will be set later.
     * This function only can be called by main thread.
     */
    mln_thread_pool_member_t *tpm;
    if ((tpm = mln_thread_pool_member_new(tp, child)) == NULL) {
        return NULL;
    }
    ++(tp->counter);
    ++(tp->idle);
    mln_child_chain_add(&(tp->child_head), &(tp->child_tail), tpm);
    return tpm;
})

MLN_FUNC_VOID(static, void, mln_thread_pool_member_exit, (void *arg), (arg), {
    ASSERT(arg != NULL); /*Fatal error, thread messed up*/

    mln_thread_pool_member_t *tpm = (mln_thread_pool_member_t *)arg;
    mln_u32_t forked = tpm->forked, child = tpm->child;
    mln_thread_pool_t *tpool = tpm->pool;
    if (tpm->locked == 0) {
        tpm->locked = 1;
        pthread_mutex_lock(&(tpool->mutex));
    }
    mln_child_chain_del(&(tpool->child_head), &(tpool->child_tail), tpm);
    --(tpool->counter);
    if (tpm->idle) --(tpool->idle);
    pthread_mutex_unlock(&(tpool->mutex));
    mln_thread_pool_member_free(tpm);
    if (forked && child) {
        mln_thread_pool_free(tpool);
    }
})

/*
 * thread_pool
 */
MLN_FUNC_VOID(static, void, mln_thread_pool_prepare, (void), (), {
    if (m_thread_pool_self == NULL) return;
    if (!m_thread_pool_self->locked)
        pthread_mutex_lock(&(m_thread_pool_self->pool->mutex));
})

MLN_FUNC_VOID(static, void, mln_thread_pool_parent, (void), (), {
    if (m_thread_pool_self == NULL) return;
    if (!m_thread_pool_self->locked)
        pthread_mutex_unlock(&(m_thread_pool_self->pool->mutex));
})

MLN_FUNC_VOID(static, void, mln_thread_pool_child, (void), (), {
    if (m_thread_pool_self == NULL) return;
    mln_thread_pool_t *tpool = m_thread_pool_self->pool;
    if (!m_thread_pool_self->locked)
        pthread_mutex_unlock(&(tpool->mutex));
    m_thread_pool_self->forked = 1;
    mln_thread_pool_member_t *tpm = tpool->child_head, *fr;
    while (tpm != NULL) {
        fr = tpm;
        tpm = tpm->next;
        if (fr == m_thread_pool_self) continue;
        mln_thread_pool_member_exit(fr);
    }
})

MLN_FUNC(static, mln_thread_pool_t *, mln_thread_pool_new, \
         (struct mln_thread_pool_attr *tpattr, int *err), (tpattr, err), \
{
    int rc;
    mln_thread_pool_t *tp;
    if ((tp = (mln_thread_pool_t *)malloc(sizeof(mln_thread_pool_t))) == NULL) {
        *err = ENOMEM;
        return NULL;
    }
    if ((rc = pthread_mutex_init(&(tp->mutex), NULL)) != 0) {
        free(tp);
        *err = rc;
        return NULL;
    }
    if ((rc = pthread_cond_init(&(tp->cond), NULL)) != 0) {
        pthread_mutex_destroy(&(tp->mutex));
        free(tp);
        *err = rc;
        return NULL;
    }
    if ((rc = pthread_attr_init(&(tp->attr))) != 0) {
        pthread_cond_destroy(&(tp->cond));
        pthread_mutex_destroy(&(tp->mutex));
        free(tp);
        *err = rc;
        return NULL;
    }
    if ((rc = pthread_attr_setdetachstate(&(tp->attr), PTHREAD_CREATE_DETACHED)) != 0) {
        pthread_attr_destroy(&(tp->attr));
        pthread_cond_destroy(&(tp->cond));
        pthread_mutex_destroy(&(tp->mutex));
        free(tp);
        *err = rc;
        return NULL;
    }
    tp->incoming = NULL;
    tp->res_chain_head = tp->res_chain_tail = NULL;
    tp->res_free_list = NULL;
    tp->child_head = tp->child_tail = NULL;
    tp->idle = tp->counter = 0;
    tp->waiters = 0;
    tp->quit = 0;
    tp->cond_timeout = tpattr->cond_timeout;
    tp->n_res = 0;
    tp->free_list_size = 0;
    tp->process_handler = tpattr->child_process_handler;
    tp->free_handler = tpattr->free_handler;
    tp->max = tpattr->max;
#if defined(MLN_USE_UNIX98) && !defined(MSYS2)
    if (tpattr->concurrency) pthread_setconcurrency(tpattr->concurrency);
#endif
    if ((rc = pthread_atfork(mln_thread_pool_prepare, \
                             mln_thread_pool_parent, \
                             mln_thread_pool_child)) != 0)
    {
        pthread_attr_destroy(&(tp->attr));
        pthread_cond_destroy(&(tp->cond));
        pthread_mutex_destroy(&(tp->mutex));
        free(tp);
        *err = rc;
        return NULL;
    }
    if ((m_thread_pool_self = mln_thread_pool_member_join(tp, 0)) == NULL) {
        pthread_attr_destroy(&(tp->attr));
        pthread_cond_destroy(&(tp->cond));
        pthread_mutex_destroy(&(tp->mutex));
        free(tp);
        *err = ENOMEM;
        return NULL;
    }
    return tp;
})

MLN_FUNC_VOID(static, void, mln_thread_pool_free, (mln_thread_pool_t *tp), (tp), {
    if (tp == NULL) return;
    m_thread_pool_self = NULL;
    mln_thread_pool_resource_t *tpr;
    /* Drain anything left in the lock-free incoming stack. */
    tpr = MLN_ATOMIC_EXCHANGE(&tp->incoming, NULL);
    while (tpr != NULL) {
        mln_thread_pool_resource_t *next = tpr->next;
        if (tp->free_handler != NULL) tp->free_handler(tpr->data);
        free(tpr);
        tpr = next;
    }
    while ((tpr = tp->res_chain_head) != NULL) {
        tp->res_chain_head = tp->res_chain_head->next;
        if (tp->free_handler != NULL) tp->free_handler(tpr->data);
        free(tpr);
    }
    while ((tpr = tp->res_free_list) != NULL) {
        tp->res_free_list = tpr->next;
        free(tpr);
    }
    ASSERT(tp->child_head == NULL && !tp->counter && !tp->idle);
    pthread_mutex_destroy(&(tp->mutex));
    pthread_cond_destroy(&(tp->cond));
    pthread_attr_destroy(&(tp->attr));
    free(tp);
})


/*
 * resource
 *
 * Producer (always main thread):
 *   1. Allocate a node (try the per-pool free list first, fall back
 *      to malloc(); both done outside the pool mutex when possible).
 *   2. Push the node onto the lock-free LIFO @incoming via a CAS.
 *      We are a single producer so the CAS only fails if a consumer
 *      drained @incoming between the load and the CAS.
 *   3. If at least one worker is parked on the cond variable, briefly
 *      take the mutex and signal one. The critical section here is
 *      empty - no list/queue work happens here.
 *   4. If no worker is parked AND we are below the soft thread cap,
 *      take the mutex and spawn one more worker.
 *
 * Consumer (worker thread): drain the FIFO main queue under the
 * mutex; when it runs dry, atomically swap the lock-free incoming
 * stack out (in one __atomic_exchange) and reverse it into the main
 * queue, all while holding the mutex.
 *
 * Compared with the pre-existing implementation, the producer's hot
 * path no longer competes with consumers for the pool mutex on the
 * common-case "items already pending" flow, and the per-add malloc/
 * free is replaced with a free-list reuse.
 */

MLN_FUNC(static, mln_thread_pool_resource_t *, mln_thread_pool_node_take, \
         (mln_thread_pool_t *tpool), (tpool), \
{
    /*
     * Try to take a node from the lock-free free-list (a Treiber stack).
     * Returns NULL if the free list is empty.
     *
     * Single-popper (the producer is the only caller of this function),
     * multi-pusher: nodes can only be inserted by recycle, not removed,
     * by anyone other than us. That removes the classical Treiber-stack
     * ABA problem because a node we observed at the top cannot be
     * popped and pushed back by a peer in the meantime.
     */
    mln_thread_pool_resource_t *tpr, *next;
    tpr = MLN_ATOMIC_LOAD(&(tpool->res_free_list));
    while (tpr != NULL) {
        next = tpr->next;
        if (__atomic_compare_exchange_n(&(tpool->res_free_list), &tpr, next,
                                        1, __ATOMIC_ACQUIRE, __ATOMIC_RELAXED))
        {
            __atomic_sub_fetch(&(tpool->free_list_size), 1, __ATOMIC_RELAXED);
            return tpr;
        }
    }
    return NULL;
})

MLN_FUNC_VOID(static inline, void, mln_thread_pool_node_recycle, \
              (mln_thread_pool_t *tpool, mln_thread_pool_resource_t *tpr), \
              (tpool, tpr), \
{
    /*
     * Push @tpr back onto the lock-free free list, unless that would
     * exceed the soft cap (in which case we hand it back to libc).
     *
     * This is a multi-pusher Treiber stack. CAS retries on contention.
     */
    if (MLN_ATOMIC_RELAXED_LOAD(&(tpool->free_list_size)) >= MLN_THREAD_POOL_FREE_LIST_MAX) {
        free(tpr);
        return;
    }
    mln_thread_pool_resource_t *expected;
    expected = MLN_ATOMIC_RELAXED_LOAD(&(tpool->res_free_list));
    do {
        tpr->next = expected;
    } while (!MLN_ATOMIC_CAS_WEAK(&(tpool->res_free_list), &expected, tpr));
    __atomic_add_fetch(&(tpool->free_list_size), 1, __ATOMIC_RELAXED);
})

MLN_FUNC_VOID(static inline, void, mln_thread_pool_lockfree_push, \
              (mln_thread_pool_t *tpool, \
               mln_thread_pool_resource_t *head, \
               mln_thread_pool_resource_t *tail), \
              (tpool, head, tail), \
{
    /*
     * Single-producer push of [head ... tail] onto the lock-free
     * @incoming stack. Tail's next is overwritten on each retry.
     */
    mln_thread_pool_resource_t *expected;
    expected = MLN_ATOMIC_RELAXED_LOAD(&(tpool->incoming));
    do {
        tail->next = expected;
    } while (!MLN_ATOMIC_CAS_WEAK(&(tpool->incoming), &expected, head));
})

MLN_FUNC(, int, mln_thread_pool_resource_add, (void *data), (data), {
    /*
     * Only main thread can call this function.
     */
    ASSERT(m_thread_pool_self != NULL);

    mln_thread_pool_resource_t *tpr;
    mln_thread_pool_t *tpool = m_thread_pool_self->pool;

    if ((tpr = mln_thread_pool_node_take(tpool)) == NULL) {
        if ((tpr = (mln_thread_pool_resource_t *)malloc(sizeof(mln_thread_pool_resource_t))) == NULL) {
            return ENOMEM;
        }
    }
    tpr->data = data;

    /* Lock-free push onto incoming. */
    mln_thread_pool_lockfree_push(tpool, tpr, tpr);

    /*
     * Quick sniff (no lock): do we need to lock at all?
     * - If at least one worker is parked, signal one.
     * - If no worker is parked and we are below the soft cap, spawn one.
     * - Otherwise, do nothing - existing workers will pick up the item
     *   the next time they drain the queue.
     *
     * Both reads here are racy (workers can decrement counter while we
     * read it), but a stale answer is fine: the worst case is one
     * unnecessary mutex acquisition, since the in-mutex check below is
     * the authoritative one.
     */
    mln_u32_t waiters_snap = MLN_ATOMIC_LOAD(&(tpool->waiters));
    mln_u32_t counter_snap = MLN_ATOMIC_RELAXED_LOAD(&(tpool->counter));
    if (waiters_snap == 0 && counter_snap >= tpool->max + 1) {
        return 0;
    }

    m_thread_pool_self->locked = 1;
    pthread_mutex_lock(&(tpool->mutex));
    if (tpool->waiters > 0) {
        pthread_cond_signal(&(tpool->cond));
    } else if (tpool->counter < tpool->max + 1) {
        int rc;
        pthread_t threadid;
        mln_thread_pool_member_t *tpm;
        if ((tpm = mln_thread_pool_member_join(tpool, 1)) == NULL) {
            pthread_mutex_unlock(&(tpool->mutex));
            m_thread_pool_self->locked = 0;
            return ENOMEM;
        }
        if ((rc = pthread_create(&threadid, &(tpool->attr), child_thread_launcher, tpm)) != 0) {
            mln_child_chain_del(&(tpool->child_head), &(tpool->child_tail), tpm);
            --(tpool->counter);
            --(tpool->idle);
            free(tpm);
            pthread_mutex_unlock(&(tpool->mutex));
            m_thread_pool_self->locked = 0;
            return rc;
        }
    }
    pthread_mutex_unlock(&(tpool->mutex));
    m_thread_pool_self->locked = 0;
    return 0;
})

MLN_FUNC(, int, mln_thread_pool_resource_addn, (void **data, mln_size_t n), (data, n), {
    /*
     * Batched submission. Only main thread can call this function.
     * Builds a single linked list locally and hands it to @incoming
     * with one atomic CAS, which is much cheaper than @n individual
     * mutex round-trips.
     */
    ASSERT(m_thread_pool_self != NULL);
    if (n == 0) return 0;

    mln_thread_pool_t *tpool = m_thread_pool_self->pool;
    mln_thread_pool_resource_t *batch_head = NULL, *batch_tail = NULL;
    mln_size_t built = 0;

    /*
     * Build the batch chain. Reuse free-list nodes lock-free, fall
     * back to malloc() once the free list runs dry.
     */
    while (built < n) {
        mln_thread_pool_resource_t *tpr = mln_thread_pool_node_take(tpool);
        if (tpr == NULL) break;
        tpr->data = data[built];
        tpr->next = batch_head;
        batch_head = tpr;
        if (batch_tail == NULL) batch_tail = tpr;
        ++built;
    }
    while (built < n) {
        mln_thread_pool_resource_t *tpr;
        if ((tpr = (mln_thread_pool_resource_t *)malloc(sizeof(*tpr))) == NULL) break;
        tpr->data = data[built];
        tpr->next = batch_head;
        batch_head = tpr;
        if (batch_tail == NULL) batch_tail = tpr;
        ++built;
    }
    if (batch_head == NULL) return ENOMEM;

    mln_thread_pool_lockfree_push(tpool, batch_head, batch_tail);

    /*
     * Wake/spawn enough workers to handle this batch. Take the lock
     * once and either signal multiple times or spawn multiple new
     * threads, capped at the maximum.
     */
    m_thread_pool_self->locked = 1;
    pthread_mutex_lock(&(tpool->mutex));
    mln_size_t needed = built;
    while (needed > 0 && tpool->waiters > 0) {
        pthread_cond_signal(&(tpool->cond));
        --needed;
    }
    while (needed > 0 && tpool->counter < tpool->max + 1) {
        int rc;
        pthread_t threadid;
        mln_thread_pool_member_t *tpm;
        if ((tpm = mln_thread_pool_member_join(tpool, 1)) == NULL) break;
        if ((rc = pthread_create(&threadid, &(tpool->attr), child_thread_launcher, tpm)) != 0) {
            mln_child_chain_del(&(tpool->child_head), &(tpool->child_tail), tpm);
            --(tpool->counter);
            --(tpool->idle);
            free(tpm);
            break;
        }
        --needed;
    }
    pthread_mutex_unlock(&(tpool->mutex));
    m_thread_pool_self->locked = 0;
    return (built == n) ? 0 : ENOMEM;
})

MLN_FUNC_VOID(static, void, mln_thread_pool_drain_incoming, \
              (mln_thread_pool_t *tpool), (tpool), \
{
    /*
     * @mutex must be held by the caller. Atomically take everything
     * out of @incoming (LIFO), reverse it into FIFO, and append to
     * the main queue.
     */
    mln_thread_pool_resource_t *node = MLN_ATOMIC_EXCHANGE(&(tpool->incoming), NULL);
    if (node == NULL) return;

    mln_thread_pool_resource_t *fifo_head = NULL, *fifo_tail = NULL, *next;
    mln_size_t added = 0;
    while (node != NULL) {
        next = node->next;
        node->next = fifo_head;
        fifo_head = node;
        if (fifo_tail == NULL) fifo_tail = node;
        node = next;
        ++added;
    }
    if (tpool->res_chain_tail == NULL) {
        tpool->res_chain_head = fifo_head;
    } else {
        tpool->res_chain_tail->next = fifo_head;
    }
    tpool->res_chain_tail = fifo_tail;
    tpool->n_res += added;
})

/*
 * launcher
 */
MLN_FUNC(, int, mln_thread_pool_run, (struct mln_thread_pool_attr *tpattr), (tpattr), {
    int rc = 0;
    mln_thread_pool_t *tpool;

    if (tpattr->child_process_handler == NULL || \
        tpattr->main_process_handler == NULL)
    {
        return EINVAL;
    }

    if ((tpool = mln_thread_pool_new(tpattr, &rc)) == NULL) {
        return rc;
    }
    rc = tpattr->main_process_handler(tpattr->main_data);
    tpool->quit = 1;
    while (1) {
        m_thread_pool_self->locked = 1;
        pthread_mutex_lock(&(tpool->mutex));
        if (tpool->counter <= 1) {
            pthread_mutex_unlock(&(tpool->mutex));
            m_thread_pool_self->locked = 0;
            break;
        }
        pthread_cond_broadcast(&(tpool->cond));
        pthread_mutex_unlock(&(tpool->mutex));
        m_thread_pool_self->locked = 0;
        usleep(50000);
    }
    mln_thread_pool_member_exit(m_thread_pool_self);
    m_thread_pool_self = NULL;
    mln_thread_pool_free(tpool);
    return rc;
})

MLN_FUNC(static, void *, child_thread_launcher, (void *arg), (arg), {
    mln_sptr_t rc = 0;
    mln_u32_t forked = 0;
    pthread_cleanup_push(mln_thread_pool_member_exit, arg);

    struct timespec ts;
    mln_u32_t timeout = 0;
    mln_thread_pool_member_t *tpm = (mln_thread_pool_member_t *)arg;
    mln_thread_pool_t *tpool = tpm->pool;
    mln_thread_pool_resource_t *recycle_local = NULL;
    mln_thread_pool_resource_t *batch_head, *batch_tail, *node;
    int batch_count;

    m_thread_pool_self = tpm;

    while (1) {
        tpm->locked = 1;
        pthread_mutex_lock(&(tpool->mutex));

        /*
         * Push the previous batch's processed nodes back to the free
         * list. Done first so that recycle_local is always NULL on
         * every break out of this loop and on every cond_wait below
         * (which is the only place we can be cancelled or asked to
         * quit).
         */
        while (recycle_local != NULL) {
            node = recycle_local;
            recycle_local = node->next;
            mln_thread_pool_node_recycle(tpool, node);
        }

again:
        if (tpm->idle <= 0) {
            tpm->idle = 1;
            ++(tpool->idle);
        }
        if (tpool->quit) break;

        if (tpool->res_chain_head == NULL) {
            mln_thread_pool_drain_incoming(tpool);
        }

        if (tpool->res_chain_head == NULL) {
            if (timeout) break;

            ts.tv_sec = time(NULL) + tpool->cond_timeout / 1000;
            ts.tv_nsec = (tpool->cond_timeout % 1000) * 1000000;
            ++(tpool->waiters);
            rc = pthread_cond_timedwait(&(tpool->cond), &(tpool->mutex), &ts);
            --(tpool->waiters);
            if (rc != 0) {
                if (rc == ETIMEDOUT) {
                    timeout = 1;
                    rc = 0;
                    goto again;
                }
                break;
            } else {
                timeout = 0;
                goto again;
            }
        }

        /* Pop a FIFO batch under the lock. */
        batch_head = batch_tail = NULL;
        batch_count = 0;
        while (batch_count < MLN_THREAD_POOL_BATCH && tpool->res_chain_head != NULL) {
            node = tpool->res_chain_head;
            tpool->res_chain_head = node->next;
            if (tpool->res_chain_head == NULL) tpool->res_chain_tail = NULL;
            --(tpool->n_res);
            if (node->data == NULL) {
                node->next = recycle_local;
                recycle_local = node;
                continue;
            }
            node->next = NULL;
            if (batch_tail != NULL) batch_tail->next = node;
            else                    batch_head = node;
            batch_tail = node;
            ++batch_count;
        }

        if (batch_head == NULL) {
            /* All items had NULL data. */
            goto again;
        }

        tpm->idle = 0;
        --(tpool->idle);

        pthread_mutex_unlock(&(tpool->mutex));
        tpm->locked = 0;
        timeout = 0;

        /* Process batch outside the lock. */
        while (batch_head != NULL) {
            node = batch_head;
            batch_head = node->next;
            tpm->data = node->data;
            rc = tpool->process_handler(tpm->data);
            tpm->data = NULL;
            node->next = recycle_local;
            recycle_local = node;
        }
    }

    /*
     * @mutex is still held here (we always break with the lock held).
     * Drain recycle_local before exit so we don't leak nodes.
     */
    while (recycle_local != NULL) {
        node = recycle_local;
        recycle_local = node->next;
        mln_thread_pool_node_recycle(tpool, node);
    }

    forked = m_thread_pool_self->forked;
    pthread_cleanup_pop(1);
    m_thread_pool_self = NULL;
    if (forked) exit(rc);
    return (void *)rc;
})

MLN_FUNC_VOID(, void, mln_thread_quit, (void), (), {
    ASSERT(m_thread_pool_self != NULL);

    mln_thread_pool_t *tpool = m_thread_pool_self->pool;
    m_thread_pool_self->locked = 1;
    pthread_mutex_lock(&(tpool->mutex));
    tpool->quit = 1;
    pthread_cond_broadcast(&(tpool->cond));
    pthread_mutex_unlock(&(tpool->mutex));
    m_thread_pool_self->locked = 0;
})

MLN_FUNC_VOID(, void, mln_thread_resource_info, (struct mln_thread_pool_info *info), (info), {
    if (info == NULL) return;

    ASSERT(m_thread_pool_self != NULL);

    mln_thread_pool_t *tpool = m_thread_pool_self->pool;
    m_thread_pool_self->locked = 1;
    pthread_mutex_lock(&(tpool->mutex));
    /*
     * res_num counts the FIFO queue exactly and the lock-free
     * incoming list approximately (the walk can race with the
     * producer's CAS push, but this is a stats call and intrinsically
     * a snapshot).
     */
    mln_size_t pending_incoming = 0;
    mln_thread_pool_resource_t *peek = MLN_ATOMIC_LOAD(&(tpool->incoming));
    while (peek != NULL) {
        ++pending_incoming;
        peek = peek->next;
    }
    info->max_num = tpool->max;
    info->idle_num = tpool->idle;
    info->cur_num = tpool->counter;
    info->res_num = tpool->n_res + pending_incoming;
    pthread_mutex_unlock(&(tpool->mutex));
    m_thread_pool_self->locked = 0;
})

MLN_CHAIN_FUNC_DEFINE(static inline, \
                      mln_child, \
                      mln_thread_pool_member_t, \
                      prev, \
                      next);

#endif

