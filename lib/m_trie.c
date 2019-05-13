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

#include "m_trie.h"

/* -------------------------------------------------------------------------- */
#ifdef _ENABLE_TRIE
/* -------------------------------------------------------------------------- */

typedef struct _m_node {
    void *child[2];
    uint32_t byte;
    uint8_t otherbits;
} _m_node;

/* -------------------------------------------------------------------------- */

public m_trie *trie_alloc(void (*freeval)(void *))
{
    /** @brief allocate an empty crit-bit tree */

    m_trie *t = malloc(sizeof(*t));

    if (! t) {
        perror(ERR(trie_alloc, malloc));
        return NULL;
    }

    if (! (t->_lock = malloc(sizeof(*t->_lock))) ) {
        perror(ERR(trie_alloc, malloc));
        goto _err_lock;
    }

    if (pthread_rwlock_init(t->_lock, NULL) == -1) {
        perror(ERR(trie_alloc, pthread_rwlock_init));
        goto _err_init;
    }

    t->_root = NULL; t->_freeval = freeval;

    return t;

_err_init:
    free(t->_lock);
_err_lock:
    free(t);

    return NULL;
}

/* -------------------------------------------------------------------------- */

public int trie_insert(m_trie *t, const char *key, void *value)
{
    const uint8_t *const ubytes = (void *) key;
    size_t ulen = 0;
    uint8_t *p = NULL, c = 0;
    unsigned int direction = 0, newdirection = 0;
    char *leaf = NULL; _m_node *node = NULL, *n = NULL;
    uint32_t newbyte = 0, newotherbits = 0;
    void **wherep = NULL;
    #define ptrsize (sizeof(void *))

    if (! t || ! key) {
        debug("trie_insert(): bad parameters.\n");
        return -1;
    }

    ulen = strlen(key);

    pthread_rwlock_wrlock(t->_lock);

    if (! (p = t->_root) ) {
        /* the tree is empty -- simply allocate a new leaf node */
        if (posix_memalign((void **) & leaf, ptrsize, ulen + 1 + ptrsize)) {
            pthread_rwlock_unlock(t->_lock);
            return -1;
        }
        *((void **) leaf) = value;
        memcpy(leaf + ptrsize, key, ulen + 1);
        t->_root = leaf + ptrsize;
        pthread_rwlock_unlock(t->_lock);
        return 0;
    }

    /* traverse the tree to find where the new node should be inserted */
    while (1 & (intptr_t) p) {
        node = (void *)(p - 1);
        c = (node->byte < ulen) ? ubytes[node->byte] : 0;
        direction = (1 + (node->otherbits | c)) >> 8;
        p = node->child[direction];
    }

    for (newbyte = 0; newbyte < ulen; ++ newbyte) {
        if (p[newbyte] != ubytes[newbyte]) {
            newotherbits = p[newbyte] ^ ubytes[newbyte];
            goto different_byte_found;
        }
    }

    if (p[newbyte] != 0) {
        newotherbits = p[newbyte];
        goto different_byte_found;
    }

    pthread_rwlock_unlock(t->_lock);

    return 0;

different_byte_found:

    while (newotherbits & (newotherbits - 1))
        newotherbits &= newotherbits - 1;

    newotherbits ^= 255; c = p[newbyte];
    newdirection= (1 + (newotherbits | c)) >> 8;

    if (posix_memalign((void **) & node, ptrsize, sizeof(*node))) {
        pthread_rwlock_unlock(t->_lock);
        return -1;
    }

    if (posix_memalign((void **) & leaf, ptrsize, ptrsize + ulen + 1)) {
        pthread_rwlock_unlock(t->_lock); posix_memfree(node);
        return -1;
    }

    *((void **) leaf) = value;
    memcpy(leaf + ptrsize, ubytes, ulen + 1);
    node->byte = newbyte;
    node->otherbits = newotherbits;
    node->child[1 - newdirection] = leaf + ptrsize;

    for (wherep = & t->_root; (p = *wherep); wherep = n->child + direction) {
        if (! (1 & (intptr_t) p)) break;
        n = (void *)(p - 1);
        if (n->byte > newbyte) break;
        if (n->byte == newbyte && n->otherbits > newotherbits) break;
        c = (n->byte < ulen) ? ubytes[n->byte] : 0;
        direction = (1 + (n->otherbits | c)) >> 8;
    }

    node->child[newdirection] = *wherep;
    *wherep = (void *)(1 + (char *) node);

    pthread_rwlock_unlock(t->_lock);

    #undef ptrsize
    return 0;
}

/* -------------------------------------------------------------------------- */

public void *trie_findexec(m_trie *t, const char *key, void *(*f)(void *))
{
    uint8_t *p = NULL, c = 0;
    _m_node *node = NULL;
    unsigned int direction = 0;
    void *ret = NULL;
    const uint8_t *ubytes = (void *) key;
    size_t ulen = 0;

    if (! t || ! key) {
        debug("trie_findexec(): bad parameters.\n");
        return NULL;
    }

    ulen = strlen(key);

    pthread_rwlock_rdlock(t->_lock);

    if (! ( p = t->_root) ) {
        pthread_rwlock_unlock(t->_lock);
        return NULL;
    }

    /* traverse the tree to find the node */
    while (1 & (intptr_t) p) {
        node = (void *)(p - 1);
        c = (node->byte < ulen) ? ubytes[node->byte] : 0;
        direction = (1 + (node->otherbits | c)) >> 8;
        p = node->child[direction];
    }

    /* check for exact match */
    if (strcmp(key, (const char *) p) == 0) {
        ret = *((void **)(p - sizeof(void *)));
        if (f) ret = f(ret);
    }

    pthread_rwlock_unlock(t->_lock);

    return ret;
}

/* -------------------------------------------------------------------------- */

public void *trie_remove(m_trie *t, const char *key)
{
    uint8_t *p = NULL, c = 0;
    void **wherep = NULL, **whereq = NULL;
    _m_node *node = NULL;
    int direction = 0;
    void *ret = NULL;
    const uint8_t *ubytes = (void *) key;
    size_t ulen = 0;

    if (! t || ! key) {
        debug("trie_remove(): bad parameters.\n");
        return NULL;
    }

    ulen = strlen(key);

    pthread_rwlock_wrlock(t->_lock);

    if (! (p = t->_root) ) {
        pthread_rwlock_unlock(t->_lock);
        return NULL;
    }

    wherep = & t->_root;

    /* traverse the tree to find the node */
    while (1 & (intptr_t) p) {
        whereq = wherep;
        node = (void *)(p - 1);
        c = (node->byte < ulen) ? ubytes[node->byte] : 0;
        direction = (1 + (node->otherbits | c)) >> 8;
        wherep = node->child + direction;
        p = *wherep;
    }

    /* check for exact match */
    if (strcmp(key, (const char *) p)) {
        pthread_rwlock_unlock(t->_lock);
        return NULL;
    }

    /* get the associated value and free up the node */
    ret = *((void **)(p - sizeof(void *)));
    posix_memfree(p - sizeof(void *));

    if (! whereq) {
        /* the tree is now empty */
        t->_root = NULL;
        pthread_rwlock_unlock(t->_lock);
        return ret;
    }

    /* simplify the tree */
    *whereq = node->child[1 - direction];
    posix_memfree(node);

    pthread_rwlock_unlock(t->_lock);

    return ret;
}

/* -------------------------------------------------------------------------- */

public void *trie_update(m_trie *t, const char *key, void *value)
{
    uint8_t *p = NULL, c = 0;
    _m_node *node = NULL;
    void *ret = NULL;
    int direction = 0;
    const uint8_t *ubytes = (void *) key;
    size_t ulen = 0;

    if (! t || ! key) {
        debug("trie_update(): bad parameters.\n");
        return NULL;
    }

    ulen = strlen(key);

    pthread_rwlock_wrlock(t->_lock);

    if (! ( p = t->_root) ) {
        pthread_rwlock_unlock(t->_lock);
        return NULL;
    }

    /* traverse the tree to find the node */
    while (1 & (intptr_t) p) {
        node = (void *)(p - 1);
        c = (node->byte < ulen) ? ubytes[node->byte] : 0;
        direction = (1 + (node->otherbits | c)) >> 8;
        p = node->child[direction];
    }

    /* check for exact match and update the value */
    if (strcmp(key, (const char *) p) == 0) {
        ret = *((void **)(p - sizeof(void *)));
        *((void **)(p - sizeof(void *))) = value;
    }

    pthread_rwlock_unlock(t->_lock);

    return ret;
}

/* -------------------------------------------------------------------------- */

static int _each(m_trie *t, void **top, int (*function)(const char *, void *))
{
    uint8_t *p = NULL;
    _m_node *node = NULL;
    int ret[2] = { 0, 0 };

    if (! (p = *top) ) return -1;

    if (1 & (intptr_t) p) {
        node = (void *)(p - 1);
        ret[0] = _each(t, & node->child[0], function);
        ret[1] = _each(t, & node->child[1], function);
        if (ret[0] == -1) {
            *top = (ret[1] == -1) ? NULL : node->child[1];
            goto _free_node;
        } else if (ret[1] == -1) {
            *top = node->child[0];
            goto _free_node;
        }
    } else if (function) {
        ret[0] = function((void *) p, *((void **) (p - sizeof(void *))));
        if (ret[0] == -1) {
            if (t->_freeval) t->_freeval(*((void **) (p - sizeof(void *))));
            posix_memfree(p - sizeof(void *));
        }
        return ret[0];
    }

    return 0;

_free_node:
    posix_memfree(node);
    return 0;
}

/* -------------------------------------------------------------------------- */

public void trie_foreach(m_trie *t, int (*f)(const char *, void *))
{
    if (! t) {
        debug("trie_foreach(): bad parameters.\n");
        return;
    }

    pthread_rwlock_wrlock(t->_lock);

    if (! (t->_root) ) {
        pthread_rwlock_unlock(t->_lock);
        return;
    }

    _each(t, & t->_root, f);

    pthread_rwlock_unlock(t->_lock);
}

/* -------------------------------------------------------------------------- */

static int _delete(UNUSED const char *key, UNUSED void *value)
{
    return -1;
}

/* -------------------------------------------------------------------------- */

public m_trie *trie_free(m_trie *t)
{
    if (! t) return NULL;
    trie_foreach(t, _delete);
    pthread_rwlock_destroy(t->_lock);
    free(t->_lock); free(t);
    return NULL;
}

/* -------------------------------------------------------------------------- */

static int _each_prefix(uint8_t *top, int (*function)(const char *, void *))
{
    _m_node *node = NULL;
    int direction = 0, ret = 1;

    if (1 & (intptr_t) top) {
        node = (void *)(top - 1);

        for (direction = 0; direction < 2; ++ direction) {
            switch (_each_prefix(node->child[direction], function)) {
                case 1: break;
                case 0: return 0;
                default: return -1;
            }
        }

        return 1;
    }

    if (function)
        ret = function((const char *) top, *((void **) (top - sizeof(void *))));

    return ret;
}

/* -------------------------------------------------------------------------- */

public void trie_foreach_prefix(m_trie *t, const char *prefix,
                                int (*function)(const char *, void *))
{
    const uint8_t *ubytes = (void *) prefix;
    size_t ulen = 0;
    uint8_t *p = NULL, *top = NULL, c = 0;
    int direction = 0;
    _m_node *node = NULL;

    if (! t || ! prefix) {
        debug("trie_foreach_prefix(): bad parameters.\n");
        return;
    }

    ulen = strlen(prefix);

    pthread_rwlock_rdlock(t->_lock);

    if (! (top = p = t->_root) ) {
        pthread_rwlock_unlock(t->_lock);
        return;
    }

    /* find the best match for the given prefix */
    while (1 & (intptr_t) p) {
        node = (void *)(p - 1);
        c = (node->byte < ulen) ? ubytes[node->byte] : 0;
        direction = (1 + (node->otherbits | c)) >> 8;
        p = node->child[direction];
        if (node->byte < ulen) top = p;
    }

    /* check if the best match is ok */
    if (memcmp(p, ubytes, ulen) != 0) {
        pthread_rwlock_unlock(t->_lock);
        return;
    }

    /* start iterating through the child nodes */
    _each_prefix(top, function);

    pthread_rwlock_unlock(t->_lock);
}

/* -------------------------------------------------------------------------- */
#else
/* -------------------------------------------------------------------------- */

/* Trie support will not be compiled in the Concrete Library */
#ifdef __GNUC__
__attribute__ ((unused)) static int __dummy__ = 0;
#endif

/* -------------------------------------------------------------------------- */
#endif
/* -------------------------------------------------------------------------- */
