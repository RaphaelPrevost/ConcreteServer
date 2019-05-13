/*******************************************************************************
 *  Concrete Server                                                            *
 *  Copyright (c) 2005-2019 Raphael Prevost <raph@el.bzh>                      *
 *                                                                             *
 *  This software is a computer program whose purpose is to provide a          *
 *  framework for developing and prototyping network services.                 *
 *                                                                             *
 *  This software is governed by the CeCILL  license under French law and      *
 *  abiding by the rules of distribution of free software.  You can  use,      *
 *  modify and/ or redistribute the software under the terms of the CeCILL     *
 *  license as circulated by CEA, CNRS and INRIA at the following URL          *
 *  "http://www.cecill.info".                                                  *
 *                                                                             *
 *  As a counterpart to the access to the source code and  rights to copy,     *
 *  modify and redistribute granted by the license, users are provided only    *
 *  with a limited warranty  and the software's author,  the holder of the     *
 *  economic rights,  and the successive licensors  have only  limited         *
 *  liability.                                                                 *
 *                                                                             *
 *  In this respect, the user's attention is drawn to the risks associated     *
 *  with loading,  using,  modifying and/or developing or reproducing the      *
 *  software by the user in light of its specific status of free software,     *
 *  that may mean  that it is complicated to manipulate,  and  that  also      *
 *  therefore means  that it is reserved for developers  and  experienced      *
 *  professionals having in-depth computer knowledge. Users are therefore      *
 *  encouraged to load and test the software's suitability as regards their    *
 *  requirements in conditions enabling the security of their systems and/or   *
 *  data to be ensured and,  more generally, to use and operate it in the      *
 *  same conditions as regards security.                                       *
 *                                                                             *
 *  The fact that you are presently reading this means that you have had       *
 *  knowledge of the CeCILL license and that you accept its terms.             *
 *                                                                             *
 ******************************************************************************/

#include "m_queue.h"

typedef struct _m_node {
    struct _m_node *next;
    void *data;
} _m_node;

/* -------------------------------------------------------------------------- */

public m_queue *queue_alloc(void)
{
    m_queue *ret = malloc(sizeof(*ret));

    if (! ret) { perror(ERR(queue_alloc, malloc)); return NULL; }

    if (pthread_mutex_init(& ret->_head_lock, NULL) == -1) {
        perror(ERR(queue_alloc, pthread_mutex_init)); return queue_free(ret);
    }

    if (pthread_mutex_init(& ret->_tail_lock, NULL) == -1) {
        perror(ERR(queue_alloc, pthread_mutex_init)); return queue_free(ret);
    }

    if (pthread_cond_init(& ret->_empty, NULL) == -1) {
        perror(ERR(queue_alloc, pthread_cond_init)); return queue_free(ret);
    }

    if (! (ret->_head = ret->_tail = malloc(sizeof(_m_node))) ) {
        perror(ERR(queue_alloc, malloc)); return queue_free(ret);
    }

    ret->_head->next = ret->_head->data = NULL;

    return ret;
}

/* -------------------------------------------------------------------------- */

public m_queue *queue_free(m_queue *queue)
{
    if (! queue) return NULL;

    pthread_mutex_destroy(& queue->_head_lock);
    pthread_mutex_destroy(& queue->_tail_lock);
    pthread_cond_destroy(& queue->_empty);
    free(queue->_head); free(queue);

    return NULL;
}

/* -------------------------------------------------------------------------- */

public void queue_free_nodes(m_queue *queue, void (*free_data)(void *))
{
    void *ptr = NULL;

    if (queue && free_data)
        while ( (ptr = queue_pop(queue)) ) free_data(ptr);
}

/* -------------------------------------------------------------------------- */

public int queue_push(m_queue *queue, void *ptr)
{
    _m_node *node = NULL;

    if (! queue || ! ptr) {
        debug("queue_push(): bad parameters.\n");
        return -1;
    }

    if (! (node = malloc(sizeof(*node))) ) {
        debug("queue_push(): out of memory.\n");
        return -1;
    }

    node->data = ptr; node->next = NULL;

    /* use the Michael & Scott two-lock concurrent queue algorithm */
    pthread_mutex_lock(& queue->_tail_lock);

        /* swing the tail to the new node */
        queue->_tail->next = node;
        queue->_tail = node;
        /* if a thread was waiting to pop an element, wake it up */
        pthread_cond_signal(& queue->_empty);

    pthread_mutex_unlock(& queue->_tail_lock);

    return 0;
}

/* -------------------------------------------------------------------------- */

public int queue_empty(m_queue *queue)
{
    int ret = 0;

    if (! queue) return 1;

    pthread_mutex_lock(& queue->_head_lock);

        ret = (! queue->_head->next) ? 1 : 0;

    pthread_mutex_unlock(& queue->_head_lock);

    return ret;
}

/* -------------------------------------------------------------------------- */

public void queue_wait(m_queue *queue, unsigned int duration)
{
    struct timespec ts;
    struct timeval tv;
    int ret = 0;

    if (! queue || ! duration) return;

    gettimeofday(& tv, NULL);

    ts.tv_sec = tv.tv_sec;
    ts.tv_nsec = (tv.tv_usec + duration) * 1000;

    pthread_mutex_lock(& queue->_tail_lock);

    if (queue_empty(queue)) {
        ret = pthread_cond_timedwait(& queue->_empty, & queue->_tail_lock, & ts);
        if (ret == ETIMEDOUT) debug("queue_wait(): timed out.");
    }

    pthread_mutex_unlock(& queue->_tail_lock);

    return;
}

/* -------------------------------------------------------------------------- */

public void *queue_pop(m_queue *queue)
{
    _m_node *node = NULL, *next = NULL;
    void *ret = NULL;

    if (! queue) {
        debug("queue_pop(): bad parameters.\n");
        return NULL;
    }

    /* use the Michael & Scott two-lock concurrent queue algorithm */
    pthread_mutex_lock(& queue->_head_lock);

        node = queue->_head; next = node->next;

        /* pop the head and fetch the data */
        if (next) { ret = next->data; queue->_head = next; free(node); }

    pthread_mutex_unlock(& queue->_head_lock);

    return ret;
}

/* -------------------------------------------------------------------------- */

public int queue_unpop(m_queue *queue, void *ptr)
{
    _m_node *node = NULL;

    if (! queue || ! ptr) {
        debug("queue_unpop(): bad parameters.\n");
        return -1;
    }

    if (! (node = malloc(sizeof(*node))) ) {
        debug("queue_push(): out of memory.\n");
        return -1;
    }

    node->data = NULL;

    pthread_mutex_lock(& queue->_head_lock);

        /* store the data in the head, replace it with a dummy node */
        queue->_head->data = ptr;
        node->next = queue->_head;
        queue->_head = node;
        /* if a thread was waiting to pop an element, wake it up */
        pthread_cond_signal(& queue->_empty);

    pthread_mutex_unlock(& queue->_head_lock);

    return 0;
}

/* -------------------------------------------------------------------------- */
