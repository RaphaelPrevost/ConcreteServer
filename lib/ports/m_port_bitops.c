/*******************************************************************************
 *  Concrete Server                                                            *
 *  Copyright (c) 2005-2024 Raphael Prevost <raph@el.bzh>                      *
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

/* fast macros to test if at least one byte in a word is < n, or > n, or = 0 */
#define __zero(x)    (((x) - 0x01010101U) & ~(x) & 0x80808080U)
#define __less(x, n) (((x) - ~0U / 255 * (n)) & ~(x) & ~0U / 255 * 128)
#define __more(x, n) ((((x) + ~0U / 255 * (127 - (n))) | (x)) & ~0U / 255 * 128)
#define __between(x, m, n) \
(((~0U / 255 * (127 + (n)) - ((x) & ~0U / 255 * 127)) & ~(x) & \
 (((x) & ~0U / 255 * 127) + ~0U / 255 * (127 - (m)))) & ~0U / 255 * 128)

/* -------------------------------------------------------------------------- */

static inline uint32_t __ctz(uint32_t i)
{
    unsigned long c;

    if (likely(i)) {
        /* hardware implementation */
        #if (defined(__GNUC__) && \
             ((__GNUC__ >= 4) || (__GNUC__ == 3 && __GNUC_MINOR__ >= 4))) && \
            (defined(__i386__) || defined(__x86_64__) || defined(__arm__))
        
        c = __builtin_ctz(i);

        #elif (defined(_MSC_VER) && (_MSC_VER >= 1400)) && \
              (defined(_M_IX86) || defined(_M_AMD64) || defined(_M_ARM))

        #pragma intrinsic(_BitScanForward)

        _BitScanForward(& c, (unsigned long) i);

        #else

        /* portable software implementation */
        i &= -i;
        c = 0;
        if (i & 0xaaaaaaaaU) c |= 1;
        if (i & 0xccccccccU) c |= 2;
        if (i & 0xf0f0f0f0U) c |= 4;
        if (i & 0xff00ff00U) c |= 8;
        if (i & 0xffff0000U) c |= 16;
        
        #endif
    } else c = 32;

    return c;
}

/* -------------------------------------------------------------------------- */

static inline uint32_t __clz(uint32_t i)
{
    if (likely(i)) {
        /* hardware implementation */
        #if (defined(__GNUC__) && \
             ((__GNUC__ >= 4) || (__GNUC__ == 3 && __GNUC_MINOR__ >= 4))) && \
            (defined(__i386__) || defined(__x86_64__) || defined(__arm__))
        
        return __builtin_clz(i);
        
        #elif (defined(_MSC_VER) && (_MSC_VER >= 1400)) && \
              (defined(_M_IX86) || defined(_M_AMD64) || defined(_M_ARM))

        #pragma intrinsic(_BitScanReverse)

        unsigned long ret;

        _BitScanReverse(& ret, (unsigned long) i);

        return 31 - ret;
        
        #else

        /* portable software implementation (de Bruijn sequence) */
        static const char seq[32] = {
            0, 31, 9, 30, 3, 8, 13, 29, 2, 5, 7, 21, 12, 24, 28, 19,
            1, 10, 4, 14, 6, 22, 25, 20, 11, 15, 23, 26, 16, 27, 17, 18
        };
        
        i |= i >> 1;
        i |= i >> 2;
        i |= i >> 4;
        i |= i >> 8;
        i |= i >> 16;
        i ++;
        
        return seq[i * 0x076be629 >> 27];
        
        #endif
    } else return 32;
}

/* -------------------------------------------------------------------------- */

static inline uint32_t __msb(uint32_t i)
{
    /* hardware implementation */
    #if (defined(__GNUC__) && \
         ((__GNUC__ >= 4) || (__GNUC__ == 3 && __GNUC_MINOR__ >= 4))) && \
        (defined(__i386__) || defined(__x86_64__) || defined(__arm__))

    return 1 << (__builtin_clz(i) ^ 31);

    #elif (defined(_MSC_VER) && (_MSC_VER >= 1400)) && \
          (defined(_M_IX86) || defined(_M_AMD64) || defined(_M_ARM))

    #pragma intrinsic(_BitScanReverse)

    unsigned long idx;

    _BitScanReverse(& idx, (unsigned long) i);

    return 1 << (idx ^ 31);

    #else

    /* portable software implementation (de Bruijn sequence) */
    static const uint8_t seq[] = { 0, 5, 1, 6, 4, 3, 2, 7 };

    i |= i >> 1; i |= i >> 2; i |= i >> 4;

    return 1 << seq[(uint8_t) (i * 0x1D) >> 5];

    #endif
}

/* -------------------------------------------------------------------------- */

static inline uint32_t __zero_idx(uint32_t i)
{
    #if defined(LITTLE_ENDIAN_HOST)
    return __ctz(i) >> 3;
    #elif defined(BIG_ENDIAN_HOST)
    return __clz(i) >> 3;
    #endif
}

/* -------------------------------------------------------------------------- */
