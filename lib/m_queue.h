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

#ifndef M_QUEUE_H

#define M_QUEUE_H

#include "m_core_def.h"

/** @defgroup queue core::queue */

typedef struct m_queue {
    /* private */
    pthread_mutex_t _head_lock;
    pthread_mutex_t _tail_lock;
    pthread_cond_t _empty;

    struct _m_node *_head;
    struct _m_node *_tail;
} m_queue;

/**
 * @ingroup queue
 * @struct m_queue
 *
 * This structure holds the inner data of a two-lock concurrent queue (FIFO).
 *
 * Its fields are private and should not be directly accessed.
 *
 * @b private @ref _head_lock is the mutex protecting the head of the queue.
 * @b private @ref _tail_lock is the mutex protecting the tail of the queue.
 * @b private @ref _head is the head of the queue.
 * @b private @ref _head is the tail of the queue.
 *
 * The head and tail are _m_node structures.
 * The _m_node structure holds data pushed onto the queue:
 *
 * typedef struct _m_node {
 *   void *_data;
 *   struct _m_node *_next;
 * } _m_node;
 *
 * @b private _data is used to carry a pointer, or any integer type that
 *            can fit within sizeof(int *).
 * @b private _next is used to link the nodes of the queue.
 *
 * This type may be modified at any time, so it should not be used outside
 * this module.
 *
 */

/* -------------------------------------------------------------------------- */

public m_queue *queue_alloc(void);

/**
 * @ingroup queue
 * @fn m_queue *queue_alloc(void)
 * @param void
 * @return a pointer to a new m_queue, or NULL.
 *
 * This function allocates a new fully initialized queue.
 *
 */

/* -------------------------------------------------------------------------- */

public m_queue *queue_free(m_queue *queue);

/**
 * @ingroup queue
 * @fn m_queue *queue_free(m_queue *queue)
 * @param queue a pointer to a queue
 * @return always NULL
 *
 * This function will properly clean up and destroy a m_queue structure.
 * However, its nodes are not destroyed, since queue_free has no way to know
 * how to properly handle the data inside them. To destroy the nodes, please
 * see @ref queue_free_nodes().
 *
 * Since this function does not empty the queue before destroying it, it must
 * only be used on empty queues (please see @ref queue_empty() ).
 *
 * This function always returns NULL, so it can be used to clean a pointer:
 * q = queue_free(q);
 *
 */

/* -------------------------------------------------------------------------- */

public void queue_free_nodes(m_queue *queue, void *(*free_data)(void *));

/**
 * @ingroup queue
 * @fn void queue_free_nodes(m_queue *queue, void (*free_data)(void *))
 * @param queue a pointer to a queue
 * @param free_data a callback for the destruction of the data inside the queue
 * @return void
 *
 * This function destroys all the nodes of a queue and clean their content
 * with the provided @b free_data callback.
 *
 */

/* -------------------------------------------------------------------------- */

public int queue_add(m_queue *queue, void *ptr);

/**
 * @ingroup queue
 * @fn int queue_add(m_queue *queue, void *ptr)
 * @param queue a pointer to a queue
 * @param ptr a pointer to some data to enqueue
 * @return -1 if an error occurs, 0 otherwise
 *
 * This function stores the given pointer in the tail of the queue.
 *
 */

/* -------------------------------------------------------------------------- */

public int queue_empty(m_queue *queue);

/**
 * @ingroup queue
 * @fn int queue_empty(m_queue *queue)
 * @param queue a pointer to a queue
 * @return 1 if the queue is empty, 0 otherwise
 *
 * This function returns 1 if the queue is empty, 0 otherwise.
 *
 */

/* -------------------------------------------------------------------------- */

public void queue_wait(m_queue *queue, unsigned int duration);

/**
 * @ingroup queue
 * @fn void queue_wait(m_queue *queue, unsigned int microseconds)
 * @param queue a pointer to a queue
 * @param duration the time to wait for an item to be queued, in microseconds
 * @return void
 *
 * This function waits up to @b duration microseconds for an item to be queued.
 *
 */

/* -------------------------------------------------------------------------- */

public void *queue_get(m_queue *queue);
#define queue_pop(x) queue_get((x))

/**
 * @ingroup queue
 * @fn void *queue_get(m_queue *queue)
 * @param queue a pointer to a queue
 * @return a pointer to the first enqueued item or NULL if the queue is empty
 *
 * This function returns the first item which was enqueued, or NULL if there
 * is none.
 *
 * The @b queue_pop macro is a convenience when using the queue as a
 * stack (LIFO).
 *
 */

/* -------------------------------------------------------------------------- */

public int queue_push(m_queue *queue, void *ptr);

/**
 * @ingroup queue
 * @fn int queue_push(m_queue *queue, void *ptr)
 * @param queue a pointer to a queue
 * @param ptr a pointer to some data to push onto the queue
 * @return -1 if an error occurs, 0 otherwise
 *
 * This function enqueues the given item at the head of the queue, so it will
 * be returned by the next @ref queue_get() call.
 *
 * @note you can implement a stack (LIFO) by using @ref queue_push() and the
 * macro @b queue_pop()
 *
 */

/* -------------------------------------------------------------------------- */

#endif
