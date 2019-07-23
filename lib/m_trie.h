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

#ifndef M_TRIE_H

#define M_TRIE_H

#ifdef _ENABLE_TRIE

#include "m_core_def.h"

/** @defgroup trie module::trie */

typedef struct m_trie {
    pthread_rwlock_t *_lock;
    void *_root;
    void (*_freeval)(void *);
} m_trie;

/**
 * @ingroup trie
 * @struct m_trie
 *
 * @b private @ref _lock a read/write lock to ensure safe concurrency
 * @b private @ref _root a pointer to the root node of the trie
 *
 */

/* -------------------------------------------------------------------------- */

public m_trie *trie_alloc(void (*freeval)(void *));

/**
 * @ingroup trie
 * @fn m_trie *trie_alloc(void (*freeval)(void *))
 * @param freeval optional pointer to a cleanup function
 *
 * This function allocates a new Patricia trie. If the @b freeval callback is
 * not NULL, it will be called by @ref trie_foreach() if an element should be
 * removed from the trie.
 *
 * The trie should be destroyed with @ref trie_free() after use.
 *
 */

/* -------------------------------------------------------------------------- */

public int trie_insert(m_trie *t, const char *key, size_t ulen, void *value);

/* -------------------------------------------------------------------------- */

public int trie_insert_r(m_trie *t, const char *key, size_t ulen, void *value);

/* -------------------------------------------------------------------------- */

public void *trie_findexec(m_trie *t, const char *key, size_t ulen,
                           void *(CALLBACK *f)(void *));

/* -------------------------------------------------------------------------- */

public void *trie_remove(m_trie *t, const char *key, size_t ulen);

/* -------------------------------------------------------------------------- */

public void *trie_update(m_trie *t, const char *key, size_t ulen, void *value);

/* -------------------------------------------------------------------------- */

public void trie_foreach(m_trie *t, int (*f)(const char *, size_t, void *));

/* -------------------------------------------------------------------------- */

public void trie_foreach_prefix(m_trie *t, const char *prefix, size_t ulen,
                                int (*function)(const char *, size_t, void *));

/* -------------------------------------------------------------------------- */

public m_trie *trie_free(m_trie *t);

/* -------------------------------------------------------------------------- */

/* _ENABLE_TRIE */
#endif

#endif
