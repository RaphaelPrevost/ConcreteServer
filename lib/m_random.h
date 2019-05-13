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

#ifndef M_RANDOM_H

#define M_RANDOM_H

#ifdef _ENABLE_RANDOM

#include "m_core_def.h"

/*
------------------------------------------------------------------------------
rand.h: definitions for a random number generator
By Bob Jenkins, 1996, Public Domain
MODIFIED:
  960327: Creation (addition of randinit, really)
  970719: use context, not global variables, for internal state
  980324: renamed seed to flag
  980605: recommend RANDSIZL=4 for noncryptography.
  010626: note this is public domain
------------------------------------------------------------------------------
*/

/*
 * This algorithm is as fast (even 20% faster with gcc -O3) as the
 * Mersenne Twister, but it is considered cryptographically secure
 * and is natively reentrant.
 *
 */

/* -------------------------------------------------------------------------- */

/** @defgroup random module::random */

#define RANDSIZL   (8)  /* I recommend 8 for crypto, 4 for simulations */
#define RANDSIZ    (1 << RANDSIZL)

/* context of random number generator */
typedef struct m_random {
  uint32_t _cnt;
  uint32_t _rsl[RANDSIZ];
  uint32_t _mem[RANDSIZ];
  uint32_t _a;
  uint32_t _b;
  uint32_t _c;
} m_random;

/**
 * @ingroup random
 * @struct m_random
 *
 * This structure stores the internal state of the ISAAC random number
 * generator.
 *
 * Its content is private and should not be tampered with.
 *
 * @b private @ref _cnt is the number of generated numbers available
 * @b private @ref _rsl holds the results
 * @b private @ref _mem is the working memory
 * @b private @ref _a, @ref _b, @ref _c are working variables
 *
 * A new structure is obtained using @ref random_init(), and must be destroyed
 * using @ref random_fini().
 *
 */

/* -------------------------------------------------------------------------- */

public m_random *random_init(void);

/**
 * @ingroup random
 * @fn m_random *random_init(void)
 * @param void
 * @return an opaque structure holding the random number generator state
 *
 * This function initializes the random number generator with a default seed.
 *
 */

/* -------------------------------------------------------------------------- */

public m_random *random_arrayinit(const uint32_t *seed, size_t len);

/**
 * @ingroup random
 * @fn m_random *random_arrayinit(const ub4 *seed, size_t len)
 * @param init_key the key
 * @param key_length the length of the key
 * @return an opaque structure holding the random number generator state
 *
 * This function initializes the random number generator with the given array.
 *
 */

/* -------------------------------------------------------------------------- */

public unsigned long random_uint32(m_random *ctx);

/**
 * @ingroup random
 * @fn unsigned long random_uint32(m_random *ctx)
 * @param ctx the random number generator state obtained from random_*init()
 * @return an unsigned 32 bits integer
 *
 * This function generates a random number on [0,0xffffffff]-interval.
 *
 * @warning If @ref ctx is NULL or invalid, the function will trigger a
 *          well deserved SIGSEGV. You have been warned ;)
 *
 */

/* -------------------------------------------------------------------------- */

public long random_int32(m_random *ctx);

/**
 * @ingroup random
 * @fn long random_int32(m_random *ctx)
 * @param ctx the random number generator state obtained from random_*init()
 * @return a signed, positive 32 bits integer
 *
 * This function generates a random number on [0,0x7fffffff]-interval.
 *
 * @warning If @ref ctx is NULL or invalid, the function will trigger a
 *          well deserved SIGSEGV. You have been warned ;)
 *
 */

/* -------------------------------------------------------------------------- */

public double random_real1(m_random *ctx);

/**
 * @ingroup random
 * @fn double random_real1(m_random *ctx)
 * @param ctx the random number generator state obtained from random_*init()
 * @return a real
 *
 * This function generates a random number on [0,1]-real-interval.
 *
 * @warning If @ref ctx is NULL or invalid, the function will trigger a
 *          well deserved SIGSEGV. You have been warned ;)
 *
 */

/* -------------------------------------------------------------------------- */

public double random_real2(m_random *ctx);

/**
 * @ingroup random
 * @fn double random_real2(m_random *ctx)
 * @param ctx the random number generator state obtained from random_*init()
 * @return a real
 *
 * This function generates a random number on [0,1)-real-interval.
 *
 * @warning If @ref ctx is NULL or invalid, the function will trigger a
 *          well deserved SIGSEGV. You have been warned ;)
 *
 */

/* -------------------------------------------------------------------------- */

public double random_real3(m_random *ctx);

/**
 * @ingroup random
 * @fn double random_real3(m_random *ctx)
 * @param ctx the random number generator state obtained from random_*init()
 * @return a real
 *
 * This function generates a random number on (0,1)-real-interval.
 *
 * @warning If @ref ctx is NULL or invalid, the function will trigger a
 *          well deserved SIGSEGV. You have been warned ;)
 *
 */

/* -------------------------------------------------------------------------- */

public double random_res53(m_random *ctx);

/**
 * @ingroup random
 * @fn double random_res53(m_random *ctx)
 * @param ctx the random number generator state obtained from random_*init()
 * @return a real
 *
 * This function generates a random number on [0,1) with 53-bit resolution.
 *
 * @warning If @ref ctx is NULL or invalid, the function will trigger a
 *          well deserved SIGSEGV. You have been warned ;)
 *
 */

/* -------------------------------------------------------------------------- */

public m_random *random_fini(m_random *ctx);

/**
 * @ingroup random
 * @fn m_random *random_fini(m_random *ctx)
 * @param the random number generator state obtained from random*_init()
 * @return NULL
 *
 * This function cleans up the opaque random generator state.
 *
 */

/* _ENABLE_RANDOM */
#endif

#endif
