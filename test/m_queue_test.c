#include "../lib/m_server.h"
#include "../lib/m_queue.h"

#define ITEMS 800000

#define enqueue(i) do { queue_add(q, (void *) (uintptr_t) (i)); } while (0)
#define dequeue() ((uintptr_t) queue_get(q))

static m_queue *q = NULL;

static void *_enqueue(UNUSED void *dummy)
{
    unsigned int i = 0;

    for (i = 1; i < ITEMS; i ++) queue_add(q, (void *) (uintptr_t) i);

    return NULL;
}

static void *_dequeue(UNUSED void *dummy)
{
    unsigned int i = 0;

    for (i = 1; i < ITEMS; i ++) {
        if (queue_get(q) != (void *) (uintptr_t) i) {
            i --; queue_wait(q, 1000);
        }
    }

    return NULL;
}

int test_queue(void)
{
    pthread_t enq, deq;
    uintptr_t ret = 0;
    clock_t start, stop;

    if ( (q = queue_alloc()) == NULL) return -1;

    printf("(-) Concurrent enqueue and dequeue ("STR(ITEMS)" items).\n");

    start = clock();
    pthread_create(& enq, NULL, _enqueue, NULL);
    pthread_create(& deq, NULL, _dequeue, NULL);
    pthread_join(enq, NULL); pthread_join(deq, NULL);
    stop = clock();

    printf("(-) Time elapsed = ");
    printf("%.3f", (double)( stop - start ) / CLOCKS_PER_SEC);
    printf(" s.\n");

    if (queue_empty(q)) printf("(*) Queue empty.\n");
    else {
        printf("(!) Failed to dequeue all the items !\n");
        return -1;
    }

    queue_add(q, (void *) 0x8888);
    ret = (uintptr_t) queue_get(q);
    if (ret != 0x8888) {
        printf("(!) Enqueue/Dequeue: missing value !\n");
        return -1;
    }
    enqueue(0x9999);
    ret = dequeue();
    if (ret != 0x9999) {
        printf("(!) Enqueue/Dequeue using macros: missing value !\n");
        return -1;
    }

    printf("(-) Concurrent enqueue and dequeue ("STR(ITEMS)" items).\n");

    start = clock();
    pthread_create(& enq, NULL, _enqueue, NULL);
    pthread_create(& deq, NULL, _dequeue, NULL);
    pthread_join(enq, NULL); pthread_join(deq, NULL);
    stop = clock();

    printf("(-) Time elapsed = ");
    printf("%.3f", (double)( stop - start ) / CLOCKS_PER_SEC);
    printf(" s.\n");

    if (queue_empty(q)) printf("(*) Queue empty.\n");
    else {
        printf("(!) Failed to dequeue all the items !\n");
        return -1;
    }

    q = queue_free(q);

    return 0;
}
