#include "../lib/m_server.h"
#include "../lib/m_queue.h"

#define ITEMS 800000

#define push(i) do { queue_push(q, (void *) (uintptr_t) (i)); } while (0)
#define pop() ((unsigned) queue_pop(q))

static m_queue *q = NULL;

static void *_push(UNUSED void *dummy)
{
    unsigned int i = 0;

    for (i = 1; i < ITEMS; i ++) queue_push(q, (void *) (uintptr_t) i);

    return NULL;
}

static void *_pop(UNUSED void *dummy)
{
    unsigned int i = 0;

    for (i = 1; i < ITEMS; i ++) {
        if (queue_pop(q) != (void *) (uintptr_t) i) {
            i --; queue_wait(q, 1000);
        }
    }

    return NULL;
}

int test_queue(void)
{
    pthread_t push, pop;
    unsigned int ret = 0;
    clock_t start, stop;

    if ( (q = queue_alloc()) == NULL) return -1;

    printf("(-) Concurrent push and pop ("STR(ITEMS)" items).\n");

    start = clock();
    pthread_create(& push, NULL, _push, NULL);
    pthread_create(& pop, NULL, _pop, NULL);
    pthread_join(push, NULL); pthread_join(pop, NULL);
    stop = clock();

    printf("(-) Time elapsed = ");
    printf("%.3f", (double)( stop - start ) / CLOCKS_PER_SEC);
    printf(" s.\n");

    if (queue_empty(q)) printf("(*) Queue empty.\n");
    else {
        printf("(!) Failed to pop all the items !\n");
        return -1;
    }

    queue_push(q, (void *) 0x8888);
    ret = (unsigned) queue_pop(q);
    if (ret != 0x8888) {
        printf("(!) Push/Pop: missing value !\n");
        return -1;
    }
    push(0x9999);
    ret = pop();
    if (ret != 0x9999) {
        printf("(!) Push/Pop using macros: missing value !\n");
        return -1;
    }

    printf("(-) Concurrent push and pop ("STR(ITEMS)" items).\n");

    start = clock();
    pthread_create(& push, NULL, _push, NULL);
    pthread_create(& pop, NULL, _pop, NULL);
    pthread_join(push, NULL); pthread_join(pop, NULL);
    stop = clock();

    printf("(-) Time elapsed = ");
    printf("%.3f", (double)( stop - start ) / CLOCKS_PER_SEC);
    printf(" s.\n");

    if (queue_empty(q)) printf("(*) Queue empty.\n");
    else {
        printf("(!) Failed to pop all the items !\n");
        return -1;
    }

    q = queue_free(q);

    return 0;
}
