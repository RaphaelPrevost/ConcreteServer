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

#include "m_random.h"

/* -------------------------------------------------------------------------- */
#ifdef _ENABLE_RANDOM
/* -------------------------------------------------------------------------- */

/*
------------------------------------------------------------------------------
rand.c: By Bob Jenkins.  My random number generator, ISAAC.  Public Domain.
MODIFIED:
  960327: Creation (addition of randinit, really)
  970719: use context, not global variables, for internal state
  980324: added main (ifdef'ed out), also rearranged randinit()
  010626: Note that this is public domain
------------------------------------------------------------------------------
*/

/* -------------------------------------------------------------------------- */
/* Private definitions */
/* -------------------------------------------------------------------------- */

#define ind(mm, x) \
(*(uint32_t *) ((uint8_t *)(mm) + ((x) & ((RANDSIZ - 1) << 2))) )

#define rngstep(mix, a, b, mm, m, m2, r, x) \
{ \
  x = *m; \
  a = (a ^ (mix)) + *(m2 ++); \
  *(m ++) = y = ind(mm, x) + a + b; \
  *(r ++) = b = ind(mm, y >> RANDSIZL) + x; \
}

#define mix(a, b, c, d, e, f, g, h) \
{ \
   a ^= b << 11; d += a; b += c; \
   b ^= c >> 2;  e += b; c += d; \
   c ^= d << 8;  f += c; d += e; \
   d ^= e >> 16; g += d; e += f; \
   e ^= f << 10; h += e; f += g; \
   f ^= g >> 4;  a += f; g += h; \
   g ^= h << 8;  b += g; h += a; \
   h ^= a >> 9;  c += h; a += b; \
}

/* -------------------------------------------------------------------------- */

static int _isaac(m_random *ctx)
{
    uint32_t a, b, x, y, *m, *mm, *m2, *r, *mend;
    mm = ctx->_mem; r = ctx->_rsl;
    a = ctx->_a; b = ctx->_b + (++ctx->_c);

    for (m = mm, mend = m2 = m + (RANDSIZ / 2); m < mend; ) {
        rngstep(a << 13, a, b, mm, m, m2, r, x);
        rngstep(a >> 6 , a, b, mm, m, m2, r, x);
        rngstep(a << 2 , a, b, mm, m, m2, r, x);
        rngstep(a >> 16, a, b, mm, m, m2, r, x);
    }

    for (m2 = mm; m2 < mend; ) {
        rngstep(a << 13, a, b, mm, m, m2, r, x);
        rngstep(a >> 6 , a, b, mm, m, m2, r, x);
        rngstep(a << 2 , a, b, mm, m, m2, r, x);
        rngstep(a >> 16, a, b, mm, m, m2, r, x);
    }

    ctx->_b = b; ctx->_a = a;

    return RANDSIZ;
}

/* -------------------------------------------------------------------------- */

public m_random *random_arrayinit(const uint32_t *seed, size_t len)
{
    int i;
    uint32_t a, b, c, d, e, f, g, h;
    uint32_t *m, *r;
    m_random *ctx = NULL;

    if (! (ctx = malloc(sizeof(*ctx))) ) {
        perror(ERR(random_init, malloc));
        return NULL;
    }

    ctx->_a = ctx->_b = ctx->_c = 0;
    m = ctx->_mem;
    r = ctx->_rsl;
    a = b = c = d = e = f = g = h = 0x9e3779b9;  /* the golden ratio */

    for (i = 0; i < 4; ++ i) {        /* scramble it */
        mix(a, b, c, d, e, f, g, h);
    }

    if (seed && len) {
        /* initialize using the seed */
        memcpy(r, seed, len < RANDSIZ ? len : RANDSIZ);

        for (i = 0; i < RANDSIZ; i += 8) {
            a += r[i]; b += r[i + 1]; c += r[i + 2]; d += r[i + 3];
            e += r[i + 4]; f += r[i + 5]; g += r[i + 6]; h += r[i + 7];
            mix(a, b, c, d, e, f, g, h);
            m[i  ] = a; m[i + 1] = b; m[i + 2] = c; m[i + 3] = d;
            m[i + 4] = e; m[i + 5] = f; m[i + 6] = g; m[i + 7] = h;
        }

        /* do a second pass to make all of the seed affect all of m */
        for (i = 0; i < RANDSIZ; i += 8) {
            a += m[i]; b += m[i + 1]; c += m[i + 2]; d += m[i + 3];
            e += m[i + 4]; f += m[i + 5]; g += m[i + 6]; h += m[i + 7];
            mix(a, b, c, d, e, f, g, h);
            m[i] = a; m[i + 1] = b; m[i + 2] = c; m[i + 3] = d;
            m[i + 4] = e; m[i + 5] = f; m[i + 6] = g; m[i + 7] = h;
        }
    } else {
        /* fill in m[] with messy stuff */
        for (i = 0; i < RANDSIZ; i += 8) {
            mix(a, b, c, d, e, f, g, h);
            m[i] = a; m[i + 1] = b; m[i + 2] = c; m[i + 3] = d;
            m[i + 4] = e; m[i + 5] = f; m[i + 6] = g; m[i + 7] = h;
        }
    }

    ctx->_cnt = _isaac(ctx);

    return ctx;
}

/* -------------------------------------------------------------------------- */

public m_random *random_init(void)
{
    return random_arrayinit(NULL, 0);
}

/* -------------------------------------------------------------------------- */

public unsigned long random_uint32(m_random *ctx)
{
    /** @brief generates a random number on [0,0xffffffff]-interval */

    if (ctx->_cnt) return ctx->_rsl[-- ctx->_cnt];

    ctx->_cnt = _isaac(ctx);

    return ctx->_rsl[-- ctx->_cnt];
}

/* -------------------------------------------------------------------------- */

public long random_int32(m_random *ctx)
{
    /** @brief generates a random number on [0,0x7fffffff]-interval */

    return (long)(random_uint32(ctx) >> 1);
}

/* -------------------------------------------------------------------------- */

public double random_real1(m_random *ctx)
{
    /** @brief generates a random number on [0,1]-real-interval */

    return random_uint32(ctx) * (1.0/4294967295.0);
    /* divided by 2^32-1 */
}

/* -------------------------------------------------------------------------- */

public double random_real2(m_random *ctx)
{
    /** @brief generates a random number on [0,1)-real-interval */

    return random_uint32(ctx) * (1.0/4294967296.0);
    /* divided by 2^32 */
}

/* -------------------------------------------------------------------------- */

public double random_real3(m_random *ctx)
{
    /** @brief generates a random number on (0,1)-real-interval */

    return (((double) random_uint32(ctx)) + 0.5) * (1.0/4294967296.0);
    /* divided by 2^32 */
}

/* -------------------------------------------------------------------------- */

public double random_res53(m_random *ctx)
{
    /** @brief generates a random number on [0,1) with 53-bit resolution */

    unsigned long a = random_uint32(ctx) >> 5, b = random_uint32(ctx) >> 6;

    return (a * 67108864.0 + b) * (1.0/9007199254740992.0);
}

/* -------------------------------------------------------------------------- */

public m_random *random_fini(m_random *ctx)
{
    free(ctx);

    return NULL;
}

/* -------------------------------------------------------------------------- */
#else
/* -------------------------------------------------------------------------- */

/* PRNG support will not be compiled in the Concrete Library */
#ifdef __GNUC__
__attribute__ ((unused)) static int __dummy__ = 0;
#endif

/* -------------------------------------------------------------------------- */
#endif
/* -------------------------------------------------------------------------- */
