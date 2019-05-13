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

#include "m_hashtable.h"

/* -------------------------------------------------------------------------- */
#ifdef _ENABLE_HASHTABLE
/* -------------------------------------------------------------------------- */

/* crc8 lookup table (Maxim/Dallas 1 wire) */
static uint8_t _crc_lut[256] = {
    0x00, 0x5e, 0xbc, 0xe2, 0x61, 0x3f, 0xdd, 0x83,
    0xc2, 0x9c, 0x7e, 0x20, 0xa3, 0xfd, 0x1f, 0x41,
    0x9d, 0xc3, 0x21, 0x7f, 0xfc, 0xa2, 0x40, 0x1e,
    0x5f, 0x01, 0xe3, 0xbd, 0x3e, 0x60, 0x82, 0xdc,
    0x23, 0x7d, 0x9f, 0xc1, 0x42, 0x1c, 0xfe, 0xa0,
    0xe1, 0xbf, 0x5d, 0x03, 0x80, 0xde, 0x3c, 0x62,
    0xbe, 0xe0, 0x02, 0x5c, 0xdf, 0x81, 0x63, 0x3d,
    0x7c, 0x22, 0xc0, 0x9e, 0x1d, 0x43, 0xa1, 0xff,
    0x46, 0x18, 0xfa, 0xa4, 0x27, 0x79, 0x9b, 0xc5,
    0x84, 0xda, 0x38, 0x66, 0xe5, 0xbb, 0x59, 0x07,
    0xdb, 0x85, 0x67, 0x39, 0xba, 0xe4, 0x06, 0x58,
    0x19, 0x47, 0xa5, 0xfb, 0x78, 0x26, 0xc4, 0x9a,
    0x65, 0x3b, 0xd9, 0x87, 0x04, 0x5a, 0xb8, 0xe6,
    0xa7, 0xf9, 0x1b, 0x45, 0xc6, 0x98, 0x7a, 0x24,
    0xf8, 0xa6, 0x44, 0x1a, 0x99, 0xc7, 0x25, 0x7b,
    0x3a, 0x64, 0x86, 0xd8, 0x5b, 0x05, 0xe7, 0xb9,
    0x8c, 0xd2, 0x30, 0x6e, 0xed, 0xb3, 0x51, 0x0f,
    0x4e, 0x10, 0xf2, 0xac, 0x2f, 0x71, 0x93, 0xcd,
    0x11, 0x4f, 0xad, 0xf3, 0x70, 0x2e, 0xcc, 0x92,
    0xd3, 0x8d, 0x6f, 0x31, 0xb2, 0xec, 0x0e, 0x50,
    0xaf, 0xf1, 0x13, 0x4d, 0xce, 0x90, 0x72, 0x2c,
    0x6d, 0x33, 0xd1, 0x8f, 0x0c, 0x52, 0xb0, 0xee,
    0x32, 0x6c, 0x8e, 0xd0, 0x53, 0x0d, 0xef, 0xb1,
    0xf0, 0xae, 0x4c, 0x12, 0x91, 0xcf, 0x2d, 0x73,
    0xca, 0x94, 0x76, 0x28, 0xab, 0xf5, 0x17, 0x49,
    0x08, 0x56, 0xb4, 0xea, 0x69, 0x37, 0xd5, 0x8b,
    0x57, 0x09, 0xeb, 0xb5, 0x36, 0x68, 0x8a, 0xd4,
    0x95, 0xcb, 0x29, 0x77, 0xf4, 0xaa, 0x48, 0x16,
    0xe9, 0xb7, 0x55, 0x0b, 0x88, 0xd6, 0x34, 0x6a,
    0x2b, 0x75, 0x97, 0xc9, 0x4a, 0x14, 0xf6, 0xa8,
    0x74, 0x2a, 0xc8, 0x96, 0x15, 0x4b, 0xa9, 0xf7,
    0xb6, 0xe8, 0x0a, 0x54, 0xd7, 0x89, 0x6b, 0x35
};

#define _key(b)  ((b) + sizeof(uint16_t))
#define _len(b)  (*((uint16_t *) (b)))
/* if your arch requires aligned reads, you can modify this macro */
#define _pad(b)  (*((uint16_t *) (b)))
#define _ptr(b)  (*((char **) ((b) + sizeof(uint16_t) + _pad(b))))
#define _val(b)           \
((void *) *((int **) ((b) + sizeof(uint16_t) + _pad(b) + sizeof(int *))))

#define _set_len(b, l)    \
do { *((uint16_t *) (b)) = (l); } while (0)

#define _set_key(b, k, l) \
do { memcpy((b) + sizeof(uint16_t), (k), (l)); } while (0)

#define _set_ptr(b, p)    \
do { *((char **) ((b) + sizeof(uint16_t) + _pad(b))) = (p); } while (0)

#define _cpy_ptr(b0, b1)  \
do { \
    *((char **) ((b0) + sizeof(uint16_t) + _pad(b0))) = \
    *((char **) ((b1) + sizeof(uint16_t) + _pad(b1)));  \
} while (0)

#define _clr_ptr(b)       \
do { *((char **) ((b) + sizeof(uint16_t) + _pad(b))) = NULL; } while (0)

#define _set_val(b, p)    \
do { \
    *((int **) ((b) + sizeof(uint16_t) + _pad(b) + sizeof(int *))) = (p); \
} while (0)

/* -------------------------------------------------------------------------- */

static uint32_t _hash(const char *data, size_t len, uint32_t initval)
{
    /* Bob Jenkins' lookup3 hash function, used for the bucket index */
    uint32_t a, b, c;                         /* internal state */
    union { const void *ptr; size_t i; } u;   /* needed for Mac Powerbook G4 */

    #if (defined(__BYTE_ORDER) && defined(__LITTLE_ENDIAN) && \
         __BYTE_ORDER == __LITTLE_ENDIAN) || \
        (defined(i386) || defined(__i386__) || defined(__i486__) || \
         defined(__i586__) || defined(__i686__) || defined(__x86_64__) || \
         defined(vax) || defined(MIPSEL))
        #define HASH_LITTLE_ENDIAN 1
        #define HASH_BIG_ENDIAN 0
    #elif (defined(__BYTE_ORDER) && defined(__BIG_ENDIAN) && \
           __BYTE_ORDER == __BIG_ENDIAN) || \
          (defined(sparc) || defined(POWERPC) || defined(mc68000) || \
           defined(sel))
        #define HASH_LITTLE_ENDIAN 0
        #define HASH_BIG_ENDIAN 1
    #else
        #define HASH_LITTLE_ENDIAN 0
        #define HASH_BIG_ENDIAN 0
    #endif

    #define rot(x, k) (((x) << (k)) | ((x) >> (32 - (k))))

    #define mix(a, b, c) \
    { \
        a -= c; a ^= rot(c, 4); c += b; \
        b -= a; b ^= rot(a, 6); a += c; \
        c -= b; c ^= rot(b, 8); b += a; \
        a -= c; a ^= rot(c,16); c += b; \
        b -= a; b ^= rot(a,19); a += c; \
        c -= b; c ^= rot(b, 4); b += a; \
    }

    #define final(a, b, c) \
    { \
        c ^= b; c -= rot(b,14); \
        a ^= c; a -= rot(c,11); \
        b ^= a; b -= rot(a,25); \
        c ^= b; c -= rot(b,16); \
        a ^= c; a -= rot(c, 4); \
        b ^= a; b -= rot(a,14); \
        c ^= b; c -= rot(b,24); \
    }

    /* set up the internal state */
    a = b = c = 0x9e3779b9 + ((uint32_t) len) + initval;

    u.ptr = data;

    #if (HASH_LITTLE_ENDIAN)
    if ((u.i & 0x3) == 0) {
        const uint32_t *k = (const uint32_t *) data; /* read 32-bit chunks */
        #ifdef DEBUG
        const uint8_t *k8;
        #endif

        /* all but last block: aligned reads and affect 32 bits of (a, b, c) */
        while (len > 12) {
            a += k[0]; b += k[1]; c += k[2];
            mix(a, b, c);
            len -= 12; k += 3;
        }

        /* - handle the last (probably partial) block
         *
         * "k[2]&0xffffff" actually reads beyond the end of the string, but
         * then masks off the part it's not allowed to read.  Because the
         * string is aligned, the masked-off tail is in the same word as the
         * rest of the string.  Every machine with memory protection I've seen
         * does it on word boundaries, so is OK with this.  But VALGRIND will
         * still catch it and complain.  The masking trick does make the hash
         * noticably faster for short strings (like English words).
         */
        #ifndef DEBUG

        switch(len) {
            case 12: c += k[2]; b += k[1]; a += k[0]; break;
            case 11: c += k[2] & 0xffffff; b += k[1]; a += k[0]; break;
            case 10: c += k[2] & 0xffff; b += k[1]; a += k[0]; break;
            case 9 : c += k[2] & 0xff; b += k[1]; a += k[0]; break;
            case 8 : b += k[1]; a += k[0]; break;
            case 7 : b += k[1] & 0xffffff; a += k[0]; break;
            case 6 : b += k[1] & 0xffff; a += k[0]; break;
            case 5 : b += k[1] & 0xff; a += k[0]; break;
            case 4 : a += k[0]; break;
            case 3 : a += k[0] & 0xffffff; break;
            case 2 : a += k[0] & 0xffff; break;
            case 1 : a += k[0] & 0xff; break;
            case 0 : return c; /* zero length strings require no mixing */
        }

        #else /* make valgrind happy */

        k8 = (const uint8_t *) k;
        switch(len) {
            case 12: c += k[2]; b += k[1]; a += k[0]; break;
            case 11: c += ((uint32_t) k8[10]) << 16;  /* fall through */
            case 10: c += ((uint32_t) k8[9]) << 8;    /* fall through */
            case 9 : c += k8[8];                      /* fall through */
            case 8 : b += k[1]; a += k[0]; break;
            case 7 : b += ((uint32_t) k8[6]) << 16;   /* fall through */
            case 6 : b += ((uint32_t) k8[5]) << 8;    /* fall through */
            case 5 : b += k8[4];                      /* fall through */
            case 4 : a += k[0]; break;
            case 3 : a += ((uint32_t) k8[2]) << 16;   /* fall through */
            case 2 : a += ((uint32_t) k8[1]) << 8;    /* fall through */
            case 1 : a += k8[0]; break;
            case 0 : return c;
        }

        #endif /* !valgrind */

    } else if ((u.i & 0x1) == 0) {
        const uint16_t *k = (const uint16_t *) data; /* read 16-bit chunks */
        const uint8_t *k8;

        /* - all but last block: aligned reads and different mixing */
        while (len > 12) {
            a += k[0] + (((uint32_t) k[1]) << 16);
            b += k[2] + (((uint32_t) k[3]) << 16);
            c += k[4] + (((uint32_t) k[5]) << 16);
            mix(a, b, c);
            len -= 12; k += 6;
        }

        /* - handle the last (probably partial) block */
        k8 = (const uint8_t *) k;
        switch(len) {
            case 12: c += k[4] + (((uint32_t) k[5]) << 16);
                     b += k[2] + (((uint32_t) k[3]) << 16);
                     a += k[0] + (((uint32_t) k[1]) << 16);
                     break;
            case 11: c += ((uint32_t) k8[10]) << 16;     /* fall through */
            case 10: c += k[4];
                     b += k[2] + (((uint32_t) k[3]) << 16);
                     a += k[0] + (((uint32_t) k[1]) << 16);
                     break;
            case 9 : c += k8[8];                         /* fall through */
            case 8 : b += k[2] + (((uint32_t) k[3]) << 16);
                     a += k[0] + (((uint32_t) k[1]) << 16);
                     break;
            case 7 : b += ((uint32_t) k8[6]) << 16;      /* fall through */
            case 6 : b += k[2];
                     a += k[0]+(((uint32_t) k[1]) << 16);
                     break;
            case 5 : b += k8[4];                         /* fall through */
            case 4 : a += k[0] + (((uint32_t) k[1]) << 16);
                     break;
            case 3 : a += ((uint32_t) k8[2]) << 16;      /* fall through */
            case 2 : a += k[0];
                     break;
            case 1 : a += k8[0];
                     break;
            case 0 : return c; /* zero length requires no mixing */
        }

    }
    #else
    if ((u.i & 0x3) == 0) {
        const uint32_t *k = (const uint32_t *) data; /* read 32-bit chunks */
        #ifdef DEBUG
        const uint8_t *k8;
        #endif

        /* all but last block: aligned reads and affect 32 bits of (a, b, c) */
        while (len > 12) {
            a += k[0]; b += k[1]; c += k[2];
            mix(a, b, c);
            len -= 12; k += 3;
        }

        /* - handle the last (probably partial) block */
        #ifndef DEBUG

        switch(len) {
            case 12: c += k[2]; b += k[1]; a += k[0]; break;
            case 11: c += k[2] & 0xffffff00; b += k[1]; a += k[0]; break;
            case 10: c += k[2] & 0xffff0000; b += k[1]; a += k[0]; break;
            case 9 : c += k[2] & 0xff000000; b += k[1]; a += k[0]; break;
            case 8 : b += k[1]; a += k[0]; break;
            case 7 : b += k[1] & 0xffffff00; a += k[0]; break;
            case 6 : b += k[1] & 0xffff0000; a += k[0]; break;
            case 5 : b += k[1] & 0xff000000; a += k[0]; break;
            case 4 : a += k[0]; break;
            case 3 : a += k[0] & 0xffffff00; break;
            case 2 : a += k[0] & 0xffff0000; break;
            case 1 : a += k[0] & 0xff000000; break;
            case 0 : return c; /* zero length strings require no mixing */
        }

        #else  /* make valgrind happy */

        k8 = (const uint8_t *) k;
        switch(len) {
            case 12: c += k[2]; b += k[1]; a += k[0]; break;
            case 11: c += ((uint32_t) k8[10]) << 8;  /* fall through */
            case 10: c += ((uint32_t) k8[9]) << 16;  /* fall through */
            case 9 : c += ((uint32_t) k8[8]) << 24;  /* fall through */
            case 8 : b += k[1]; a += k[0]; break;
            case 7 : b += ((uint32_t) k8[6]) << 8;   /* fall through */
            case 6 : b += ((uint32_t) k8[5]) << 16;  /* fall through */
            case 5 : b += ((uint32_t) k8[4]) << 24;  /* fall through */
            case 4 : a += k[0]; break;
            case 3 : a += ((uint32_t) k8[2]) << 8;   /* fall through */
            case 2 : a += ((uint32_t) k8[1]) << 16;  /* fall through */
            case 1 : a += ((uint32_t) k8[0]) << 24; break;
            case 0 : return c;
        }

        #endif /* !VALGRIND */

    }
    #endif
    else { /* need to read the key one byte at a time */
        const uint8_t *k = (const uint8_t *) data;

        /* all but the last block: affect some 32 bits of (a, b, c) */
        while (len > 12) {
            a += k[0];
            a += ((uint32_t) k[1]) << 8;
            a += ((uint32_t) k[2]) << 16;
            a += ((uint32_t) k[3]) << 24;
            b += k[4];
            b += ((uint32_t) k[5]) << 8;
            b += ((uint32_t) k[6]) << 16;
            b += ((uint32_t) k[7]) << 24;
            c += k[8];
            c += ((uint32_t) k[9]) << 8;
            c += ((uint32_t) k[10]) << 16;
            c += ((uint32_t) k[11]) << 24;
            mix(a, b, c);
            len -= 12; k += 12;
        }

        /* - last block: affect all 32 bits of (c) */
        switch(len) {
            case 12: c += ((uint32_t) k[11]) << 24;
            case 11: c += ((uint32_t) k[10]) << 16;
            case 10: c += ((uint32_t) k[9]) << 8;
            case 9 : c += k[8];
            case 8 : b += ((uint32_t) k[7]) << 24;
            case 7 : b += ((uint32_t) k[6]) << 16;
            case 6 : b += ((uint32_t) k[5]) << 8;
            case 5 : b += k[4];
            case 4 : a += ((uint32_t) k[3]) << 24;
            case 3 : a += ((uint32_t) k[2]) << 16;
            case 2 : a += ((uint32_t) k[1]) << 8;
            case 1 : a += k[0]; break;
            case 0 : return c;
        }
    }

    final(a, b, c);

    return c;
}

/* -------------------------------------------------------------------------- */

static uint8_t _crc8(const char *string, size_t len)
{
    uint8_t crc = 0xff;
    while (len --) crc = _crc_lut[*string ++ ^ crc];
    return crc;
}

/* -------------------------------------------------------------------------- */

static int _cache_push(m_cache *h, char *key, int replace, void **val)
{
    unsigned int i = 0;
    uintptr_t index = 0;
    unsigned int retry = 0;
    unsigned int hash_cache[CACHE_HASHFNCOUNT];
    char *slot = NULL, *tmp = NULL;
    uint32_t mask = h->_bucket_size - 1;

    /* avoid rehashing every key */
    if (! (index = (uintptr_t) _ptr(key)) ) {
        index = _hash(_key(key), _len(key), h->_seed[0]);
        _set_ptr(key, (void *) index);
    }

    index &= mask; goto _loop;

    /* look for a free slot */
    for (i = 0; i < CACHE_HASHFNCOUNT; i ++) {
        index = _hash(_key(key), _len(key), h->_seed[i]) & mask;

_loop:  if (! h->_bucket[index]) {
            /* this slot is free, insert */
            h->_bucket[index] = key;
            h->_bucket_count ++;
            return 0;
        }

        if (replace) {
            slot = h->_bucket[index];
            if (! memcmp(key, slot, _len(key) + sizeof(uint16_t)))
                goto _replace;
        }

        hash_cache[i] = index;
    }

    if (replace) {
        /* couldn't find it, look in the basket */
        for (slot = h->_basket; slot; slot = _ptr(slot)) {
            if (! memcmp(key, slot, _len(key) + sizeof(uint16_t)))
                goto _replace;
        }
    }

    /* no free slot found, try cuckoo hashing */
    for (index = hash_cache[0]; retry < CACHE_CUCKOORETRY; retry ++) {
        /* get data off the last slot and replace them */
        tmp = h->_bucket[index]; h->_bucket[index] = key; key = tmp;

        /* get rid of tombstones */
        if (! _len(key)) return 0;

        for (i = 0; i < CACHE_HASHFNCOUNT; i ++) {
            index = _hash(_key(key), _len(key), h->_seed[i]) & mask;

            if (! h->_bucket[index]) {
                /* this slot is free, insert */
                h->_bucket[index] = key;
                h->_bucket_count ++;
                return 0;
            } else hash_cache[i] = index;
        }
    }

    /* cuckoo hashing did not help, store the key in the basket... */
    _set_ptr(key, h->_basket); h->_basket = key; h->_bucket_count ++;

    return 0;

_replace:
    if (replace == -1) *val = _val(key);
    else { *val = _val(slot); _set_val(slot, _val(key)); }
    return 1;
}

/* -------------------------------------------------------------------------- */

static int _cache_resize(m_cache *h, size_t size)
{
    char **b = NULL, **next = NULL;
    char *tmp = NULL;

    if (! h) {
        debug("_cache_resize(): bad parameters.\n");
        return -1;
    }

    if (h->_bucket_count * CACHE_GROWTHRATIO < h->_bucket_size) return 0;

    /* round the size to the next highest power of 2 */
    size --; size |= size >> 1; size |= size >> 2;
    size |= size >> 4; size |= size >> 8; size |= size >> 16;
    size ++; size += (size == 0);

    /* clear the basket */
    for (b = (char **) h->_basket, h->_basket = NULL; b; b = next) {
        next = (char **) _ptr((char *) b);
        _set_ptr((char *) b, NULL);
    }

    free(h->_bucket);

    /* try to resize the bucket array */
    if (! (h->_bucket = calloc(size, sizeof(*h->_bucket))) ) {
        perror(ERR(_cache_resize, calloc));
        return -1;
    }

    /* update the state */
    h->_bucket_size = size; h->_bucket_count = 0;

    /* rehash old buckets */
    for (b = h->_index, h->_index = NULL; b; b = next) {
        tmp = ((char *) b) + sizeof(char *); next = (char **) *b;

        if (! _len(tmp)) { free(b); continue; }

        _cache_push(h, tmp, 0, NULL);

        *b = (char *) h->_index; h->_index = b;
    }

    return 0;
}

/* -------------------------------------------------------------------------- */

public m_cache *cache_alloc(void (*freeval)(void *))
{
    unsigned int i = 0;
    m_random *r = NULL;
    m_cache *h = malloc(sizeof(*h));

    if (! h) {
        perror(ERR(cache_alloc, malloc));
        return NULL;
    }

    if (! (r = random_arrayinit((void *) h, sizeof(h))) ) goto _err_rand;

    if (! (h->_lock = malloc(sizeof(*h->_lock))) ) {
        perror(ERR(cache_alloc, malloc));
        goto _err_lock;
    }

    if (pthread_rwlock_init(h->_lock, NULL) == -1) {
        perror(ERR(cache_alloc, pthread_rwlock_init));
        goto _err_init;
    }

    h->_bucket = NULL;
    h->_bucket_count = h->_bucket_size = 0; h->_basket = NULL;
    h->_index = NULL;
    h->_freeval = freeval;

    for (i = 0; i < CACHE_HASHFNCOUNT; i ++)
        h->_seed[i] = random_uint32(r);

    if (_cache_resize(h, 2) == -1) {
        debug("cache_alloc(): cannot resize the hash table.\n");
        goto _err_size;
    }

    r = random_fini(r);

    return h;

_err_size:
    free(h->_bucket);
    free(h->_basket);
_err_init:
    free(h->_lock);
_err_lock:
    r = random_fini(r);
_err_rand:
    free(h);

    return NULL;
}

/* -------------------------------------------------------------------------- */

static void *_cache_add(m_cache *h, const char *key, size_t len, void *val,
                        int replace)
{
    char *mkey = NULL;
    char **keyptr = NULL;

    if (! h || ! key || ! len) {
        debug("_cache_add(): bad parameters.\n");
        return val;
    }

    /* replace the key by a dynamically allocated one */
    if (! (keyptr = malloc(sizeof(uint16_t) + (3 * sizeof(int *)) + len)) ) {
        perror(ERR(cache_push, malloc));
        return val;
    }

    mkey = ((char *) keyptr) + sizeof(char *);

    _set_len(mkey, len); _set_key(mkey, key, len);
    _set_val(mkey, val); _set_ptr(mkey, NULL);

    pthread_rwlock_wrlock(h->_lock);

    if (! _cache_push(h, mkey, replace, & val)) {
        *keyptr = (char *) h->_index; h->_index = keyptr;
        val = NULL;
    } else free(keyptr);

    /* try to expand the hashtable if the load is too important */
    if (h->_basket && _ptr(h->_basket))
        _cache_resize(h, h->_bucket_size + 1);

    pthread_rwlock_unlock(h->_lock);

    return val;
}

/* -------------------------------------------------------------------------- */

public void *cache_push(m_cache *h, const char *k, size_t l, void *v)
{
    return _cache_add(h, k, l, v, 1);
}

/* -------------------------------------------------------------------------- */

public void *cache_add(m_cache *h, const char *k, size_t l, void *v)
{
    return _cache_add(h, k, l, v, -1);
}

/* -------------------------------------------------------------------------- */

public void *cache_findexec(m_cache *h, const char *key, size_t len,
                            void *(*function)(void *))
{
    unsigned int i = 0, j = 0;
    const char *ptr = NULL;
    void *res = NULL;
    uint32_t mask = 0;

    if (! h || ! key || ! len) {
        debug("cache_findexec(): bad parameters.\n");
        return NULL;
    }

    pthread_rwlock_rdlock(h->_lock);

    mask = h->_bucket_size - 1;

    for (i = 0; i < CACHE_HASHFNCOUNT; i ++) {
        j = _hash(key, len, h->_seed[i]) & mask;

        /* if an empty slot is found, no need to look further */
        if (! (ptr = h->_bucket[j])) break;

        if (_len(ptr) == len && memcmp(_key(ptr), key, len) == 0) {
            res = _val(ptr); if (function) res = function(res);
            goto _result;
        }
    }

    /* unlucky, scan the basket for orphan keys */
    for (ptr = h->_basket; ptr; ptr = _ptr(ptr)) {
        if (_len(ptr) == len && memcmp(_key(ptr), key, len) == 0) {
            res = _val(ptr); if (function) res = function(res);
            goto _result;
        }
    }

_result:
    pthread_rwlock_unlock(h->_lock);

    return res;
}

/* -------------------------------------------------------------------------- */

public void *cache_find(m_cache *h, const char *key, size_t len)
{
    return cache_findexec(h, key, len, NULL);
}

/* -------------------------------------------------------------------------- */

public void cache_foreach(m_cache *h,
                          int (*function)(const char *, size_t, void *))
{
    char **bucket = NULL, *tmp = NULL;

    if (! h || ! function) {
        debug("cache_foreach(): bad parameters.\n");
        return;
    }

    pthread_rwlock_wrlock(h->_lock);

    for (bucket = h->_index; bucket; bucket = (char **) *bucket) {
        tmp = ((char *) bucket) + sizeof(char *);

        if (_len(tmp)) {
            if (function(_key(tmp), _len(tmp), _val(tmp)) == -1) {
                /* delete the record */
                _set_len(tmp, 0);
            }
        }
    }

    pthread_rwlock_unlock(h->_lock);

    return;
}

/* -------------------------------------------------------------------------- */

public int cache_sort(m_cache *h, unsigned int order,
                      int (*cmp)(const char *key0, const char *key1, size_t len,
                                 void *value0, void *value1))
{
    char **l[2] = { NULL, NULL }, **bucket = NULL, **tail = NULL;
    unsigned int size = 1, merge = 0, i = 0;
    char *a = NULL, *b = NULL;
    unsigned int c[2] = { 0, 0 };

    if (! h || ! cmp || (order != CACHE_ASC && order != CACHE_DESC) ) {
        debug("cache_sort(): bad parameters.\n");
        return -1;
    }

    pthread_rwlock_wrlock(h->_lock);

    /* simple merge sort */
    do {
        l[0] = h->_index; h->_index = NULL; tail = NULL;

        for (merge = 0; (l[1] = l[0]); merge ++, l[0] = l[1]) {

            /* split the table in 2 sorted lists of up to `size` buckets */
            for (c[0] = 0; c[0] < size; c[0] ++) {
                if (! (l[1] = (char **) *l[1]) ) { c[0] ++; break; }
            }

            /* merge these lists */
            for (c[1] = size; c[0] || (l[1] && c[1]); tail = bucket) {

                if (l[1] && c[1]) {

                    if (c[0]) {
                        a = ((char *) l[0]) + sizeof(char *);
                        b = ((char *) l[1]) + sizeof(char *);
                        i = (_len(a) < _len(b)) ? _len(a) : _len(b);
                        if (cmp(_key(a), _key(b), i, _val(a), _val(b)) <= 0)
                            i = 0 + order;
                        else
                            i = 1 - order;
                    } else i = 1; /* 1st list is empty */

                } else i = 0; /* 2nd list is empty */

                bucket = l[i];

                if (tail) *tail = (char *) bucket;
                else h->_index = bucket;

                l[i] = (char **) *l[i]; c[i] --;
            }
        }

        if (tail) *tail = NULL; size <<= 1;

    } while (merge > 1);

    pthread_rwlock_unlock(h->_lock);

    return 0;
}

/* -------------------------------------------------------------------------- */

public int cache_sort_keys(const char *key0, const char *key1, size_t l,
                           UNUSED void *val0, UNUSED void *val1)
{
    return memcmp(key0, key1, l);
}

/* -------------------------------------------------------------------------- */

public void *cache_pop(m_cache *h, const char *key, size_t len)
{
    unsigned int i = 0, j = 0;
    char *tmp = NULL, *prev = NULL;
    void *result = NULL;
    uint32_t mask = 0;

    if (! h || ! key || ! len) {
        debug("cache_pop(): bad parameters.\n");
        return NULL;
    }

    pthread_rwlock_wrlock(h->_lock);

    mask = h->_bucket_size - 1;

    for (i = 0; i < CACHE_HASHFNCOUNT; i ++) {
        j = _hash(key, len, h->_seed[i]) & mask;

        if (! h->_bucket[j] || _len(h->_bucket[j]) != len) continue;

        if (memcmp(_key(h->_bucket[j]), key, len) == 0) {
            /* remove from the bucket */
            result = _val(h->_bucket[j]);
            /* a length of 0 indicates a tombstone */
            _set_len(h->_bucket[j], 0);
            pthread_rwlock_unlock(h->_lock);
            return result;
        }
    }

    /* unlucky, scan the basket for orphan keys */
    for (tmp = prev = h->_basket; tmp; prev = tmp, tmp = _ptr(tmp)) {
        if (_len(tmp) == len) {
            if (memcmp(_key(tmp), key, len) == 0) {
                /* remove the orphan from the basket */
                result = _val(tmp);
                if (tmp == h->_basket) h->_basket = _ptr(tmp);
                else _set_ptr(prev, _ptr(tmp));
                /* XXX tombstone for garbage collection */
                _set_len(tmp, 0);

                pthread_rwlock_unlock(h->_lock);

                return result;
            }
        }
    }

    /* garbage collection if necessary */
    _cache_resize(h, (size_t) (h->_bucket_count * CACHE_GROWTHRATIO));

    pthread_rwlock_unlock(h->_lock);

    return NULL;
}

/* -------------------------------------------------------------------------- */

public size_t cache_footprint(m_cache *h, size_t *overhead)
{
    char **bucket = NULL, **next = NULL;
    char *tmp = NULL;
    size_t key = 0;
    size_t ret = sizeof(*h);

    if (! h) {
        debug("cache_footprint(): bad parameters.\n");
        return 0;
    }

    /* the lock is dynamically allocated */
    ret += sizeof(*h->_lock);

    pthread_rwlock_wrlock(h->_lock);

    if (h->_bucket_size) {
        /* segment bucket size */
        ret += h->_bucket_size * sizeof(*h->_bucket);
        /* keys */
        for (bucket = h->_index; bucket; bucket = next) {
            tmp = ((char *) bucket) + sizeof(char *);
            next = (char **) *bucket;
            if (_len(tmp)) {
                /* key length + key recorded size + next and value pointers */
                ret += sizeof(char *) + _len(tmp) + sizeof(uint16_t) +
                       sizeof(char *) + sizeof(void *);
                /* key length and value pointer are not overhead */
                key += sizeof(uint16_t) + _len(tmp) + sizeof(void *);
            }
        }
    }

    pthread_rwlock_unlock(h->_lock);

    if (overhead) *overhead = ret - key;

    return ret;
}

/* -------------------------------------------------------------------------- */

public m_cache *cache_free(m_cache *h)
{
    char **bucket = NULL, **next = NULL;
    char *keyptr = NULL;

    if (! h) return NULL;

    for (bucket = h->_index; bucket; bucket = next) {
        keyptr = ((char *) bucket) + sizeof(char *);
        next = (char **) *bucket;
        if (h->_freeval && _len(keyptr)) h->_freeval(_val(keyptr));
        free(bucket);
    }

    free(h->_bucket);
    pthread_rwlock_destroy(h->_lock);
    free(h->_lock); free(h);

    return NULL;
}

/* -------------------------------------------------------------------------- */

public m_hashtable *hashtable_alloc(void (*freeval)(void *))
{
    unsigned int i = 0;

    m_hashtable *h = malloc(sizeof(*h));

    if (! h) {
        perror(ERR(hash_alloc, malloc));
        return NULL;
    }

    for (i = 0; i < 256; i ++) h->_segment[i] = cache_alloc(freeval);

    return h;
}

/* -------------------------------------------------------------------------- */

public size_t hashtable_footprint(m_hashtable *h, size_t *overhead)
{
    unsigned int i = 0;
    size_t key = 0, over = 0;
    size_t ret = sizeof(*h);

    if (! h) {
        debug("hashtable_footprint(): bad parameters.\n");
        return 0;
    }

    ret += 256 * sizeof(void *);

    for (i = 0; i < 256; i ++) {
        ret += cache_footprint(h->_segment[i], & over);
        key += over; over = 0;
    }

    if (overhead) *overhead = key;

    return ret;
}

/* -------------------------------------------------------------------------- */

public void *hashtable_insert(m_hashtable *h, const char *k, size_t l, void *v)
{
    if (! k || ! l) return NULL;

    if (! h) return v;

    return _cache_add(h->_segment[_crc8(k, l)], k, l, v, -1);
}

/* -------------------------------------------------------------------------- */

public void *hashtable_update(m_hashtable *h, const char *k, size_t l, void *v)
{
    if (! k || ! l) return NULL;

    if (! h) return v;

    return _cache_add(h->_segment[_crc8(k, l)], k, l, v, 1);
}

/* -------------------------------------------------------------------------- */

public void *hashtable_remove(m_hashtable *h, const char *key, size_t len)
{
    if (! h || ! key || ! len) return NULL;

    return cache_pop(h->_segment[_crc8(key, len)], key, len);
}

/* -------------------------------------------------------------------------- */

public void *hashtable_findexec(m_hashtable *h, const char *key,
                                size_t len, void *(*function)(void *))
{
    if (! h || ! key || ! len) return NULL;

    return cache_findexec(h->_segment[_crc8(key, len)], key, len, function);
}

/* -------------------------------------------------------------------------- */

public void hashtable_foreach(m_hashtable *h,
                              int (*function)(const char *, size_t, void *))
{
    unsigned int i = 0;

    if (! h || ! function) return;

    for (i = 0; i < 256; i ++) {
        if (h->_segment[i]->_index)
            cache_foreach(h->_segment[i], function);
    }

    return;
}

/* -------------------------------------------------------------------------- */

public void *hashtable_find(m_hashtable *h, const char *key, size_t len)
{
    if (! h || ! key || ! len) return NULL;

    return cache_find(h->_segment[_crc8(key, len)], key, len);
}

/* -------------------------------------------------------------------------- */

public m_hashtable *hashtable_free(m_hashtable *h)
{
    unsigned int i = 0;

    if (! h) return NULL;

    for (i = 0; i < 256; i ++) cache_free(h->_segment[i]); free(h);

    return NULL;
}

/* -------------------------------------------------------------------------- */
#else
/* -------------------------------------------------------------------------- */

/* Hashtable support will not be compiled in the Concrete Library */
#ifdef __GNUC__
__attribute__ ((unused)) static int __dummy__ = 0;
#endif

/* -------------------------------------------------------------------------- */
#endif
/* -------------------------------------------------------------------------- */
