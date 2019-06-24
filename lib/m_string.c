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

#include "m_string.h"

#define _UINT(c) ((unsigned int) ((unsigned char) c))

/* -- hexadecimal digits -- */
static const char _hex[] = "0123456789abcdef";

/* -- base58 encoding/decoding -- */

/* encoding lookup table */
static const char _b58[] = "123456789ABCDEFGHJKLMNPQRSTUVWXYZ"
                           "abcdefghijkmnopqrstuvwxyz";

/* decoding lookup table */
static const char _d58[] = {
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, /*  12 */
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, /*  24 */
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, /*  36 */
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, /*  48 */
    -1,  0,  1,  2,  3,  4,  5,  6,  7,  8, -1, -1, /*  60 */
    -1, -1, -1, -1, -1,  9, 10, 11, 12, 13, 14, 15, /*  72 */
    16, -1, 17, 18, 19, 20, 21, -1, 22, 23, 24, 25, /*  84 */
    26, 27, 28, 29, 30, 31, 32, -1, -1, -1, -1, -1, /*  96 */
    -1, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, /* 108 */
    -1, 44, 45, 46, 47, 48, 49, 50, 51, 52, 53, 54, /* 120 */
    55, 56, 57, -1, -1, -1, -1, -1                  /* 128 */
};

/* macro to ease the use of the decode table and prevent out of bound access */
#define _D58(c) (_d58[(c) & 0x7F])

/* -- base64 encoding/decoding -- */

/* encoding lookup table */
static const char _b64[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                           "abcdefghijklmnopqrstuvwxyz"
                           "0123456789+/";

/* decoding lookup table */
static const char _d64[] = {
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, /*  12 */
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, /*  24 */
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, /*  36 */
    -1, -1, -1, -1, -1, -1, -1, 62, -1, -1, -1, 63, /*  48 */
    52, 53, 54, 55, 56, 57, 58, 59, 60, 61, -1, -1, /*  60 */
    -1, -1, -1, -1, -1,  0,  1,  2,  3,  4,  5,  6, /*  72 */
     7,  8, 9,  10, 11, 12, 13, 14, 15, 16, 17, 18, /*  84 */
    19, 20, 21, 22, 23, 24, 25, -1, -1, -1, -1, -1, /*  96 */
    -1, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, /* 108 */
    37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, /* 120 */
    49, 50, 51, -1, -1, -1, -1, -1                  /* 128 */
};

/* macro to ease the use of the decode table and prevent out of bound access */
#define _D64(c) (_d64[(c) & 0x7F])

#ifdef _ENABLE_JSON
static const unsigned char _j[] = {
    15, 15, 15, 15, 15, 15, 15, 15, 15,  8,  8, 15, /*  12 */
    15,  8, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, /*  24 */
    15, 15, 15, 15, 15, 15, 15, 15,  9,  0,  6,  0, /*  36 */
     0,  0,  0,  5,  0,  0,  0, 12, 11, 13, 14,  0, /*  48 */
    48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 10,  0, /*  60 */
     0,  0,  0,  0,  0, 32, 32, 32, 32,112, 32,  0, /*  72 */
     0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, /*  84 */
     0,  0,  0,  0,  0,  0,  0,  3,  7,  4,  0,  0, /*  96 */
     0, 32, 32, 32, 32,112,160,  0,  0,  0,  0,  0, /* 108 */
     0,  0,128,  0,  0,  0,  0,  0,128,  0,  0,  0, /* 120 */
     0,  0,  0,  1,  0,  2,  0,  0, 15, 15, 15, 15, /* 128 */
    15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, /* 140 */
    15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, /* 152 */
    15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, /* 164 */
    15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, /* 176 */
    15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, /* 188 */
    15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, /* 200 */
    15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, /* 212 */
    15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, /* 224 */
    15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, /* 236 */
    15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, /* 248 */
    15, 15, 15, 15, 15, 15, 15, 15, 15              /* 256 */
};

#define OBJ_START   1 /* { */
#define OBJ_CLOSE   2 /* } */
#define ARR_START   3 /* [ */
#define ARR_CLOSE   4 /* ] */
#define QUOTE_SGL   5 /* ' */
#define QUOTE_DBL   6 /* " */
#define ESCAPESEQ   7 /* \ */
#define WHITE       8 /* \t, \r, \n */
#define SPACE       9 /*  */
#define COLON      10 /* : */
#define COMMA      11 /* , */
#define DIGIT_POS  12 /* + */
#define DIGIT_NEG  13 /* - */
#define DIGIT_RAD  14 /* . */
#define OTHER      15
#define DIGIT_NUM  16 /* 0-9 */
#define DIGIT_HEX  32 /* A-F, a-f */
#define DIGIT_EXP  64 /* e E */
#define BOOL      128 /* t, f, n */
#endif

/* -- sha1 private context -- */
typedef struct {
    uint32_t h0, h1, h2, h3, h4;
    uint32_t nblocks;
    unsigned char buf[64];
    int count;
} SHA1_CONTEXT;

/* -- rawurlencode special chars -- */

/*  1: unsafe chars
    2: control chars
    3: 0x7f
    4: non US-ASCII chars
    5: reserved chars
    6: escape char */

static const char _unsafe[256] = {
    2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, /* 0x0f */
    2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, /* 0x1f */
    1, 0, 1, 1, 0, 6, 5, 1, 0, 0, 0, 0, 0, 0, 0, 5, /* 0x2f */
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 5, 5, 1, 5, 1, 5, /* 0x3f */
    5, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, /* 0x4f */
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 0, /* 0x5f */
    1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, /* 0x6f */
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 3, /* 0x7f */
    4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, /* 0x8f */
    4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, /* 0x9f */
    4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, /* 0xaf */
    4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, /* 0xbf */
    4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, /* 0xcf */
    4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, /* 0xdf */
    4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, /* 0xef */
    4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, /* 0xff */
};

/* fast macros to test if at least one byte in a word is < n, or > n, or = 0 */
#define __zero(x)    (((x) - 0x01010101U) & ~(x) & 0x80808080U)
#define __less(x, n) (((x) - ~0U / 255 * (n)) & ~(x) & ~0U / 255 * 128)
#define __more(x, n) ((((x) + ~0U / 255 * (127 - (n))) | (x)) & ~0U / 255 * 128)

/* -------------------------------------------------------------------------- */
/* 0. Initialization */
/* -------------------------------------------------------------------------- */

public int string_api_setup(void)
{
    return 0;
}

/* -------------------------------------------------------------------------- */
/* 1. Basic string allocation, conversion and initialization functions        */
/* -------------------------------------------------------------------------- */

public m_string *string_prealloc(const char *string, size_t len, size_t total)
{
    /** @brief allocate a m_string and initialize it with the given data */

    m_string *new = NULL;
    int32_t allocsize = 0;

    if (total < len) {
        debug("string_prealloc(): 'total' should be greater than length.\n");
        total = len;
    }

    if (! (new = malloc(sizeof(*new))) ) {
        perror(ERR(string_prealloc, malloc));
        return NULL;
    }

    /* allocate the internal buffer, ensure it is on wchar_t boundary */
    if (! total) {
        new->_data = NULL; new->_len = new->_alloc = 0; new->_flags = 0;
        new->parent = NULL; new->_parts_alloc = new->parts = 0;
        new->token = NULL;
        return new;
    }

    allocsize = (total + sizeof(wchar_t)) * sizeof(*new->_data);

    if ((size_t) allocsize < total) {
        debug("string_prealloc(): integer overflow.\n");
        free(new);
        return NULL;
    }

    if (! (new->_data = malloc(allocsize)) ) {
        perror(ERR(string_prealloc, malloc));
        free(new);
        return NULL;
    }
    new->_alloc = total * sizeof(*new->_data);

    /* copy the given data in the internal buffer */
    if (string) memcpy(new->_data, string, len);

    memset(new->_data + len, 0, sizeof(wchar_t));

    new->_len = len; new->_flags = 0;
    new->parent = NULL; new->token = NULL;
    new->_parts_alloc = new->parts = 0;

    return new;
}

/* -------------------------------------------------------------------------- */

public m_string *string_alloc(const char *string, size_t len)
{
    return string_prealloc(string, len, len);
}

/* -------------------------------------------------------------------------- */

public m_string *string_encaps(const char *string, size_t len)
{
    /** @brief encapsulate an existing buffer in a static m_string structure */

    m_string *new = NULL;

    if (! string || ! len) return NULL;

    if (! (new = string_alloc(NULL, 0)) ) {
        debug("string_encaps(): out of memory.\n");
        return NULL;
    }

    new->_data = (char *) string;
    new->_len = len; new->_alloc = len;
    new->_flags = _STRING_FLAG_ENCAPS;
    new->parent = NULL;
    new->_parts_alloc = new->parts = 0;
    new->token = NULL;

    return new;
}

/* -------------------------------------------------------------------------- */

public m_string *string_vfmt(m_string *str, const char *fmt, va_list args)
{
    int ret = 0;
    va_list copy;

    /* sanity check */
    if (! fmt) {
        debug("string_vfmt(): bad parameters.\n");
        return NULL;
    }

    if (str && str->_flags & _STRING_FLAG_RDONLY) {
        debug("string_vfmt(): illegal write attempt.\n");
        return NULL;
    }

    /* XXX va_lists are mangled in GNU C and cannot be reused */
    va_copy(copy, args);

    if ( (ret = m_vsnprintf(NULL, 0, fmt, args)) <= 0) {
        debug("string_vfmt(): wrong format or vsnprintf() error.\n");
        va_end(copy);
        return NULL;
    }

    if (! str) {
        if (! (str = string_alloc(NULL, ret)) ) {
            debug("string_vfmt(): allocation failure.\n");
            va_end(copy);
            return NULL;
        }
    } else if (string_extend(str, ret) == -1) {
        debug("string_vfmt(): resize failure.\n");
        va_end(copy);
        return NULL;
    }

    /* the length returned by vsnprintf does not include the trailing \0 */
    str->_len = m_vsnprintf(str->_data, ret + 1, fmt, copy);

    va_end(copy);

    return str;
}

/* -------------------------------------------------------------------------- */

public m_string *string_fmt(m_string *str, const char *fmt, ...)
{
    /** @brief overwrite a string with the given format or allocate it */

    va_list args;

    va_start(args, fmt);
    str = string_vfmt(str, fmt, args);
    va_end(args);

    return str;
}

/* -------------------------------------------------------------------------- */

public m_string *string_catfmt(m_string *str, const char *fmt, ...)
{
    int ret = 0;
    size_t off = 0;

    va_list args, copy;

    va_start(args, fmt);

    /* sanity check */
    if (! fmt) {
        debug("string_catfmt(): bad parameters.\n");
        va_end(args);
        return NULL;
    }

    if (str && str->_flags & _STRING_FLAG_RDONLY) {
        debug("string_catfmt(): illegal write attempt.\n");
        va_end(args);
        return NULL;
    }

    /* XXX va_lists are mangled in GNU C and cannot be reused */
    va_copy(copy, args);

    if ( (ret = m_vsnprintf(NULL, 0, fmt, args)) <= 0) {
        debug("string_catfmt(): wrong format or vsnprintf() error.\n");
        va_end(args); va_end(copy);
        return NULL;
    }

    /* discard the mangled list */
    va_end(args);

    if (! str) {
        if (! (str = string_alloc(NULL, ret)) ) {
            debug("string_catfmt(): allocation failure.\n");
            va_end(copy);
            return NULL;
        }
    } else {
        if (string_extend(str, (off = SIZE(str)) + ret) == -1) {
            debug("string_catfmt(): resize failure.\n");
            va_end(copy);
            return NULL;
        } else str->_len += ret;
    }

    /* the length returned by vsnprintf does not include the trailing \0 */
    m_vsnprintf(str->_data + off, ret + 1, fmt, copy);

    va_end(copy);

    return str;
}

/* -------------------------------------------------------------------------- */

public m_string *string_from_uint8(uint8_t u8)
{
    /** @brief create a string from an integer */

    return string_alloc((char *) & u8, sizeof(u8));
}

/* -------------------------------------------------------------------------- */

public int string_to_uint8s(const char *string, uint8_t *out)
{
    if (! string || ! out) {
        debug("string_to_uint8s: bad parameters.\n");
        return -1;
    }

    memcpy(out, string, sizeof(*out));

    return 0;
}

/* -------------------------------------------------------------------------- */

public int string_to_uint8(const m_string *string, uint8_t *out)
{
    /** @brief convert a binary string to an integer */

    if (! string || ! out || ! CSTR(string) || SIZE(string) < sizeof(*out)) {
        debug("string_to_uint8(): bad parameters.\n");
        return -1;
    }

    memcpy(out, string->_data, sizeof(*out));

    return 0;
}

/* -------------------------------------------------------------------------- */

public uint8_t string_peek_uint8(const m_string *string)
{
    uint8_t ret = 0;

    string_to_uint8(string, & ret);

    return ret;
}

/* -------------------------------------------------------------------------- */

public uint8_t string_fetch_uint8(m_string *string)
{
    uint8_t ret = 0;

    if (string_to_uint8(string, & ret) == 0)
        string_suppr(string, 0, sizeof(ret));

    return ret;
}

/* -------------------------------------------------------------------------- */

public m_string *string_from_uint16(uint16_t u16)
{
    /** @brief create a string from an integer */

    return string_alloc((char *) & u16, sizeof(u16));
}

/* -------------------------------------------------------------------------- */

public int string_to_uint16s(const char *string, uint16_t *out)
{
    if (! string || ! out) {
        debug("string_to_uint16s: bad parameters.\n");
        return -1;
    }

    memcpy(out, string, sizeof(*out));

    return 0;
}

/* -------------------------------------------------------------------------- */

public int string_to_uint16(const m_string *string, uint16_t *out)
{
    /** @brief convert a binary string to an integer */

    if (! string || ! out || ! CSTR(string) || SIZE(string) < sizeof(*out)) {
        debug("string_to_uint16(): bad parameters.\n");
        return -1;
    }

    memcpy(out, string->_data, sizeof(*out));

    return 0;
}

/* -------------------------------------------------------------------------- */

public uint16_t string_peek_uint16(const m_string *string)
{
    uint16_t ret = 0;

    string_to_uint16(string, & ret);

    return ret;
}

/* -------------------------------------------------------------------------- */

public uint16_t string_fetch_uint16(m_string *string)
{
    uint16_t ret = 0;

    if (string_to_uint16(string, & ret) == 0)
        string_suppr(string, 0, sizeof(ret));

    return ret;
}

/* -------------------------------------------------------------------------- */

public m_string *string_from_uint32(uint32_t u32)
{
    /** @brief create a string from an integer */

    return string_alloc((char *) & u32, sizeof(u32));
}

/* -------------------------------------------------------------------------- */

public int string_to_uint32s(const char *string, uint32_t *out)
{
    if (! string || ! out) {
        debug("string_to_uint32s: bad parameters.\n");
        return -1;
    }

    memcpy(out, string, sizeof(*out));

    return 0;
}

/* -------------------------------------------------------------------------- */

public int string_to_uint32(const m_string *string, uint32_t *out)
{
    /** @brief convert a binary string to an integer */

    if (! string || ! out || ! CSTR(string) || SIZE(string) < sizeof(*out)) {
        debug("string_to_uint32(): bad parameters.\n");
        return -1;
    }

    memcpy(out, string->_data, sizeof(*out));

    return 0;
}

/* -------------------------------------------------------------------------- */

public uint32_t string_peek_uint32(const m_string *string)
{
    uint32_t ret = 0;

    string_to_uint32(string, & ret);

    return ret;
}

/* -------------------------------------------------------------------------- */

public uint32_t string_fetch_uint32(m_string *string)
{
    uint32_t ret = 0;

    if (string_to_uint32(string, & ret) == 0)
        string_suppr(string, 0, sizeof(ret));

    return ret;
}

/* -------------------------------------------------------------------------- */

public m_string *string_from_uint64(uint64_t u64)
{
    /** @brief create a string from an integer */

    return string_alloc((char *) & u64, sizeof(u64));
}

/* -------------------------------------------------------------------------- */

public int string_to_uint64s(const char *string, uint64_t *out)
{
    if (! string || ! out) {
        debug("string_to_uint64s: bad parameters.\n");
        return -1;
    }

    memcpy(out, string, sizeof(*out));

    return 0;
}

/* -------------------------------------------------------------------------- */

public int string_to_uint64(const m_string *string, uint64_t *out)
{
    /** @brief convert a binary string to an integer */

    if (! string || ! out || ! CSTR(string) || SIZE(string) < sizeof(*out)) {
        debug("string_to_uint32(): bad parameters.\n");
        return -1;
    }

    memcpy(out, string->_data, sizeof(*out));

    return 0;
}

/* -------------------------------------------------------------------------- */

public uint64_t string_peek_uint64(const m_string *string)
{
    uint64_t ret = 0;

    string_to_uint64(string, & ret);

    return ret;
}

/* -------------------------------------------------------------------------- */

public uint64_t string_fetch_uint64(m_string *string)
{
    uint64_t ret = 0;

    if (string_to_uint64(string, & ret) == 0)
        string_suppr(string, 0, sizeof(ret));

    return ret;
}

/* -------------------------------------------------------------------------- */

public int string_peek_fmt(m_string *string, const char *fmt, ...)
{
    /** @brief copy data from the string according to the given format string */

    int ret = 0;
    va_list args;

    if (! string || ! CSTR(string) || ! fmt) return -1;

    va_start(args, fmt);

    if (! (ret = m_vsnscanf(CSTR(string), SIZE(string), fmt, args)) ) {
        debug("string_peek_fmt(): wrong format or vsnscanf() error.\n");
        return -1;
    }

    va_end(args);

    return 0;
}

/* -------------------------------------------------------------------------- */

public int string_fetch_fmt(m_string *string, const char *fmt, ...)
{
    /** @brief move data from the string according to the given format string */

    int ret = 0;
    va_list args;

    if (! string || ! CSTR(string) || ! fmt) return -1;

    va_start(args, fmt);

    if (! (ret = m_vsnscanf(CSTR(string), SIZE(string), fmt, args)) ) {
        debug("string_fetch_fmt(): wrong format or vsnscanf() error.\n");
        return -1;
    }

    string_suppr(string, 0, ret);

    va_end(args);

    return 0;
}

/* -------------------------------------------------------------------------- */

public int string_peek_buffer(m_string *string, char *out, size_t len)
{
    /** @brief copy data from the string to an external buffer */

    if (! string || ! CSTR(string) || ! out || ! len) return -1;

    len = MIN(SIZE(string), len);

    memcpy(out, CSTR(string), MIN(SIZE(string), len));

    return 0;
}

/* -------------------------------------------------------------------------- */

public int string_fetch_buffer(m_string *string, char *out, size_t len)
{
    /** @brief move data from the string to an external buffer */

    if (! string || ! CSTR(string) || ! out || ! len) return -1;

    len = MIN(SIZE(string), len);

    memcpy(out, CSTR(string), MIN(SIZE(string), len));

    string_suppr(string, 0, len);

    return 0;
}

/* -------------------------------------------------------------------------- */

public m_string *string_peek(m_string *string, size_t len)
{
    /** @brief copy data from the string */

    if (! string || ! CSTR(string)) return NULL;

    if (! len) len = SIZE(string);

    return string_alloc(CSTR(string), len);
}

/* -------------------------------------------------------------------------- */

public m_string *string_fetch(m_string *string, size_t len)
{
    /** @brief extract data from the string */

    m_string *ret = NULL;

    if (! string || ! CSTR(string)) return NULL;

    if (! len) len = SIZE(string);

    ret = string_alloc(CSTR(string), len);

    string_suppr(string, 0, len);

    return ret;
}

/* -------------------------------------------------------------------------- */

public void string_flush(m_string *string)
{
    if (! string) return;

    string_free_token(string);

    string_suppr(string, 0, SIZE(string));
}

/* -------------------------------------------------------------------------- */

public int string_wchar(m_string *string)
{
    /** @brief convert a multibyte string to a wide character string */

    char *buffer = NULL;
    size_t bufsize = 0;
    size_t ret = 0;

    if (! string || ! CSTR(string)) return -1;

    /* check if writing and resizing the string is allowed */
    if (string->_flags & _STRING_FLAG_STATIC) return -1;

    /* find the size of the wchar string */
    if ( (bufsize = mbstowcs(NULL, CSTR(string), 0)) != (size_t) -1) {
        /* allocate the wchar buffer */
        buffer = malloc( (bufsize + 1) * sizeof(wchar_t));
        if (buffer == NULL) {
            perror(ERR(string_wchar, malloc)); return -1;
        }

        /* convert the multibyte string to wchar */
        ret = mbstowcs((wchar_t *) buffer, CSTR(string), bufsize + 1);

        if (ret == (size_t) -1) {
            free(buffer); perror(ERR(string_wchar, mbstowcs));
            return -1;
        }

        /* update the string */
        free(string->_data); string->_data = buffer;
        string->_len = bufsize * sizeof(wchar_t);
        string->_alloc = (bufsize + 1) * sizeof(wchar_t);
    } else {
        perror(ERR(string_wchar, mbstowcs)); return -1;
    }

    return 0;
}

/* -------------------------------------------------------------------------- */

public int string_mbyte(m_string *string)
{
    /** @brief convert a wide character string to a multibyte string */

    char *buffer = NULL;
    size_t l = 0;

    if (! string || ! CSTR(string)) return -1;

    /* check if writing and resizing the string is allowed */
    if (string->_flags & _STRING_FLAG_STATIC) return -1;

    /* find the size of the multibyte buffer */
    if ( (l = wcstombs(NULL, (wchar_t *) CSTR(string), 0)) != (size_t) -1) {
        /* allocate it */
        buffer = malloc( (l + 1) * sizeof(*buffer));
        if (buffer == NULL) {
            perror(ERR(string_mbyte, malloc)); return -1;
        }

        /* perform the conversion */
        if (wcstombs(buffer, (wchar_t *) CSTR(string), l + 1) == (size_t) -1) {
            perror(ERR(string_mbyte, wcstombs));
            free(buffer); return -1;
        }

        /* update the string */
        free(string->_data); string->_data = buffer;
        string->_len = l; string->_alloc = l + 1;
    } else {
        perror(ERR(string_mbyte, wcstombs)); return -1;
    }

    return 0;
}

/* -------------------------------------------------------------------------- */
#ifdef HAS_ICONV
/* -------------------------------------------------------------------------- */

public size_t string_convs(const char *src, size_t srclen, const char *src_enc,
                           char *dst, size_t dstlen, const char *dst_enc)
{
    char buffer[BUFSIZ];
    size_t inlen = srclen, outlen = dstlen;
    char *in = (char *) src, *out = dst;
    iconv_t conv;
    size_t allocsize = 0;

    if (! src || ! srclen) {
        debug("string_convs(): bad parameters.\n");
        return -1;
    }

    if (! (conv = iconv_open(dst_enc, src_enc)) ) {
        perror(ERR(string_convs, iconv_open)); return -1;
    }

    /* dry run: use a static buffer for output */
    if (! dst) { out = buffer; outlen = sizeof(buffer); }

    while (iconv(conv, & in, & inlen, & out, & outlen) == (size_t) -1) {
        if (errno == E2BIG && ! dst) {
            allocsize += sizeof(buffer) - outlen;
            out = buffer; outlen = sizeof(buffer);
            continue;
        }
        /* conversion failure */
        perror(ERR(string_convs, iconv));
        goto _err_conv;
    }

    /* dry run: evaluate the size required for the output */
    if (! dst) allocsize += sizeof(buffer) - outlen;

    iconv_close(conv);

    return allocsize;

_err_conv:
    iconv_close(conv);

    return -1;
}

/* -------------------------------------------------------------------------- */

public int string_conv(m_string *s, const char *src_enc, const char *dst_enc)
{
    char buffer[BUFSIZ];
    size_t inlen = 0, outlen = sizeof(buffer);
    char *in = NULL, *out = buffer;
    iconv_t conv;
    size_t allocsize = 0;

    if (! s) {
        debug("string_conv(): bad parameters.\n");
        return -1;
    }

    if (! (conv = iconv_open(dst_enc, src_enc)) ) {
        perror(ERR(string_conv, iconv_open)); return -1;
    }

    in = s->_data; inlen = SIZE(s);

    while (iconv(conv, & in, & inlen, & out, & outlen) == (size_t) -1) {
        if (errno == E2BIG) {
            allocsize += sizeof(buffer) - outlen;
            out = buffer; outlen = sizeof(buffer);
            continue;
        }
        /* incomplete input or invalid multibyte sequence */
        perror(ERR(string_conv, iconv));
        goto _err_conv;
    }

    allocsize += sizeof(buffer) - outlen;

    if (allocsize != SIZE(s)) {
        if (allocsize > SIZE(s)) {
            /* extend the string */
            if (string_extend(s, allocsize) == -1)
                goto _err_conv;
        }
        string_free_token(s);
    }

    if (allocsize > sizeof(buffer)) {
        in = s->_data; inlen = SIZE(s);
        out = s->_data; outlen = allocsize;
        /* convert again */
        if (iconv(conv, & in, & inlen, & out, & outlen) == (size_t) -1) {
            perror(ERR(string_conv, iconv));
            goto _err_conv;
        }
    } else memcpy(s->_data, buffer, allocsize);

    s->_len = allocsize;

    iconv_close(conv);

    return 0;

_err_conv:
    iconv_close(conv);

    return -1;
}

/* -------------------------------------------------------------------------- */
#endif
/* -------------------------------------------------------------------------- */

public int string_swap(m_string *string)
{
    /** @brief swap a binary string */

    unsigned int i = 0;
    unsigned char tmp = 0;

    if (! string || ! CSTR(string) || SIZE(string) % 2) {
        debug("string_swap(): bad parameters.\n");
        return -1;
    }

    if (string->_flags & _STRING_FLAG_RDONLY) {
        debug("string_swap(): illegal write attempt.\n");
        return -1;
    }

    for (i = 0; i < SIZE(string); i += 2) {
        tmp = string->_data[i];
        string->_data[i] = string->_data[SIZE(string) - 1 - i];
        string->_data[SIZE(string) - 1 - i] = tmp;
    }

    return 0;
}

/* -------------------------------------------------------------------------- */

static int _string_copy_tokens(m_string *dst, const m_string *src)
{
    unsigned int i = 0;

    if (! dst || ! src || ! src->parts) return 0;

    dst->parts = src->parts;

    if (! (dst->token = malloc(dst->parts * sizeof(*dst->token))) ) return -1;

    for (i = 0; i < dst->parts; i ++) {
        dst->token[i]._data = dst->_data + (src->token[i]._data - src->_data);
        dst->token[i]._len = src->token[i]._len;
        dst->token[i]._alloc = src->token[i]._alloc;
        dst->token[i]._flags = src->token[i]._flags;
        dst->token[i].parent = dst;
        /* the parts and token members of the struct will be
           modified by subsequent calls */
        dst->token[i]._parts_alloc = dst->token[i].parts = 0;
        dst->token[i].token = NULL;
        if (_string_copy_tokens(& dst->token[i], & src->token[i]) == -1)
            return -1;
    }

    dst->_parts_alloc = dst->parts;

    return 0;
}

/* -------------------------------------------------------------------------- */

public m_string *string_dup(const m_string *string)
{
    /** @brief duplicate a string */

    return string_dupextend(string, 0);
}

/* -------------------------------------------------------------------------- */

public m_string *string_dupextend(const m_string *s, size_t x)
{
    m_string *ret = NULL;

    if (! s) return NULL;

    /* copy the string */
    if (! (ret = string_prealloc(CSTR(s), SIZE(s), SIZE(s) + x)) ) return NULL;

    /* copy the tokens */
    if (_string_copy_tokens(ret, s) == -1) return string_free(ret);

    return ret;
}

/* -------------------------------------------------------------------------- */

public char *string_dups(const char *string, size_t len)
{
    /** @brief duplicate a C string */

    char *ret = NULL;

    if (! string || ! len) return NULL;

    if ( (len + 1) * sizeof(*ret) < len ) {
        debug("string_dups(): integer overflow.\n");
        return NULL;
    }

    ret = malloc( (len + 1) * sizeof(*ret));
    if (! ret) { perror(ERR(string_dups, malloc)); return NULL; }

    memcpy(ret, string, len); ret[len] = '\0';

    return ret;
}

/* -------------------------------------------------------------------------- */

public m_string *string_free(m_string *string)
{
    /** @brief clean up a m_string and its internal buffer */

    m_string *parent = string;

    if (! string) return NULL;

    /* check this is a parent string */
    while (parent->parent) parent = parent->parent;
    #ifdef DEBUG
    if (string != parent)
        debug("string_free(): warning: freeing the parent of the token.\n");
    #endif
    string = parent;

    string_free_token(string);

    if (string->_flags & _STRING_FLAG_NALLOC) return NULL;

    if (~string->_flags & _STRING_FLAG_NOFREE)
        free(string->_data);
    free(string);

    return NULL;
}

/* -------------------------------------------------------------------------- */
/* 2. String size functions                                                   */
/* -------------------------------------------------------------------------- */

public size_t string_len(const m_string *string)
{
    /** @brief return the length of the string data */

    return string ? string->_len : (size_t) -1;
}

/* -------------------------------------------------------------------------- */

public size_t string_spc(const m_string *string)
{
    /** @brief return the string allocated buffer size */

    return string ? string->_alloc : (size_t) -1;
}

/* -------------------------------------------------------------------------- */

public size_t string_avl(const m_string *string)
{
    /** @brief return the space still available without extending the string */

    return string ? string->_alloc - string->_len : (size_t) -1;
}

/* -------------------------------------------------------------------------- */

static void _string_rebase_token(m_string *s, char *oldbase, char *newbase)
{
    unsigned int i = 0;

    if (! s || ! s->token || ! oldbase || ! newbase) return;

    if (oldbase == newbase) return;

    for (i = 0; i < PARTS(s); i ++) {

        char *addr = newbase + (TOKEN_CSTR(s, i) - oldbase);

        if (addr <= (s->_data + s->_alloc)) {
            /* the rebased token is inbound, fine */
            s->token[i]._data = addr;
        } else {
            unsigned int j = 0;

            /* clear all subsequent tokens */
            for (j = i; j < PARTS(s); j ++) string_free_token(TOKEN(s, j));
            memset(TOKEN(s, i), 0, (PARTS(s) - i) * sizeof(m_string));

            s->parts = i;
            break;
        }

        /* this token is rebased, rebase its subtoken too */
        _string_rebase_token(TOKEN(s, i), oldbase, newbase);
    }
}

/* -------------------------------------------------------------------------- */

static void __move_subtokens(m_string *token, int shift, const char *guardian)
{
    unsigned int i = 0;

    if (token) {
        /* recursively shift the token and its subtokens, if any */
        for (i = 0; i < PARTS(token); i ++)
            __move_subtokens(& token->token[i], shift, guardian);
        if (! guardian || token->_data > guardian)
            token->_data += shift;
    }
}

/* -------------------------------------------------------------------------- */

static void _string_shift_token(m_string *s, unsigned int off,
                                unsigned int end, int shift)
{
    unsigned int i = 0, j = 0;

    if (! s || ! PARTS(s) || ! shift) {
        debug("_string_shift_token(): bad parameters.\n");
        return;
    }

    /* find the beginning of the shift */
    for (i = 0; i < PARTS(s); i ++) {
        if (TSTR(s, i) >= CSTR(s) + off) {
            /* update the position of all subsequent tokens */
            for (j = i; j < PARTS(s); j ++) {
                if (j == i && shift < 0)
                    __move_subtokens(TOKEN(s, j), shift, CSTR(s) + off + end);
                else
                    __move_subtokens(TOKEN(s, j), shift, NULL);
            }

            s = TOKEN(s, i);
            do {
                if ( (int) s->_len + shift < 0 || (int) s->_alloc + shift < 0) {
                    m_string *parent = s->parent;
                    /* check parent's tokens bound */
                    for (i = 0; i < PARTS(parent); i ++) {
                        if (TEND(parent, i) >= STRING_END(parent)) {
                            while ( (-- parent->parts) > i + 1)
                                string_free_token(TOKEN(parent, PARTS(parent)));
                            break;
                        }
                    }
                }
            } while (PARTS(s) && (s = TOKEN(s, PARTS(s) - 1)) );

            break;
        }
    }
}

/* -------------------------------------------------------------------------- */

public int string_dim(m_string *string, size_t size)
{
    /** @brief force resize the internal buffer of a string */

    char *data = NULL, *base = NULL;
    m_string *token = NULL;
    unsigned int off = 0, len = 0;
    int diff = 0, need = 0;
    int32_t allocsize = 0;

    /* sanity checks */
    if (! string || ! size) {
        debug("string_dim(): bad parameters.\n");
        return -1;
    }

    if (string->_alloc == size) return 0;

    /* XXX "need" is the number of bytes that should be added or removed
       to fit the new string size. "diff" is the difference between the
       new and the previous length of the string content. */
    need = size - string->_alloc; diff = size - string->_len;

    /* if this string is a token, we need to resize its parent instead */
    if (string->parent) {
        token = string;
        while (string->parent) string = string->parent;
        /* store the token relative position */
        off = token->_data - string->_data; len = token->_len;
    }

    /* check if the resize makes sense */
    if ( (string->_flags & _STRING_FLAG_ENCAPS && need > 0) ||
          string->_alloc + need <= 0) {
        debug("string_dim(): illegal resize attempt.\n");
        return -1;
    }

    /* if the string is shrunk and it is a token, we may need to move data now */
    if (token && token != LAST_TOKEN(string) && need < 0) {
        data = string->_data + off + len;
        memmove(data + diff, data, string->_len - (off + len));
        _string_shift_token(string, off, len, diff);
    }

    /* resize the internal buffer, ensure the buffer is on wchar_t boundary */
    if (need > 0 && ~string->_flags & _STRING_FLAG_NALLOC) {
        /* a string is never actually shrunk, only realloc to expand */
        allocsize = (string->_alloc + need + sizeof(wchar_t)) * sizeof(*data);
        if ((size_t) allocsize < SIZE(string)) {
            debug("string_dim(): integer overflow.\n");
            return -1;
        }

        base = string->_data;

        if (! (data = realloc(string->_data, allocsize)) ) {
            perror(ERR(string_dim, realloc)); return -1;
        }

        string->_data = data; string->_alloc += need * sizeof(*string->_data);
        _string_rebase_token(string, base, data);
    }

    /* if the string was a token, move the data along */
    if (token) {
        if (diff > 0) {
            data = string->_data + off + len;
            memmove(data + diff, data, string->_len - (off + len));
            /* XXX only move subsequent tokens */
            _string_shift_token(string, off + len + 1, 0, diff);
        }

        token->_len = token->_alloc = size;

        /* update the parents' size */
        while ( (token = token->parent) ) {
            token->_len += diff;
            if (token != string) token->_alloc = token->_len;
        }
    } else if (diff < 0) string->_len += diff;

    /* zero out the remainder of the buffer */
    if (SIZE(string) < string->_alloc)
        string->_data[string->_len] = 0;

    return 0;
}

/* -------------------------------------------------------------------------- */

public int string_extend(m_string *string, size_t size)
{
    /** @brief resize a string only if the new size is greater */

    if (! string || ! size) {
        debug("string_extend(): bad parameters.\n");
        return -1;
    }

    return (size > string->_alloc) ? string_dim(string, size) : 0;
}

/* -------------------------------------------------------------------------- */

public int string_shrink(m_string *string, size_t size)
{
    /** @brief resize a string only if the new size is lower */

    if (! string || ! size) {
        debug("string_shrink(): bad parameters.\n");
        return -1;
    }

    return (size < string->_alloc) ? string_dim(string, size) : 0;
}

/* -------------------------------------------------------------------------- */
/* 3. Basic string operations (char * flavor)                                 */
/* -------------------------------------------------------------------------- */

static m_string *_string_mov(m_string *to, off_t o, const char * from, size_t l)
{
    m_string *parent = to;
    int within = 0, alloc = 0;
    ptrdiff_t within_offset = 0;

    /* noop */
    if (l == 0) return to;

    /* sanity checks */
    if (o < 0 || ! from) {
        debug("string_movs(): bad parameters.\n");
        goto _error;
    }

    /* if the destination string does not exist, allocate it */
    if (! to) {
        if (! (to = string_alloc(NULL, l)) ) {
            debug("string_movs(): allocation failure.\n");
            goto _error;
        } else alloc = 1;
    } else while (parent->parent) parent = parent->parent;

    /* check if the source is inside the destination */
    if ( (from >= CSTR(parent)) && (from < STRING_END(parent)) )
        within = 1, within_offset = from - CSTR(parent);

    /* reject out of bound offsets and resize the destination string */
    if (string_extend(to, o + l) == -1) {
        debug("string_movs(): resize failure.\n");
        goto _error;
    } else if (within) {
        /* if the source was within destination, correct the pointer */
        from = CSTR(parent) + within_offset;

        /* check if the offset is still in bound */
        if ( (from < CSTR(parent)) || (from >= STRING_END(parent)) ) {
            debug("string_movs(): offset out of bound.\n");
            goto _error;
        }
    }

    memmove(to->_data + o, from, l);

    /* update the string */
    to->_len = (SIZE(to) < o + l) ? o + l : SIZE(to);
    if (! to->parent) memset(to->_data + SIZE(to), 0x0, sizeof(wchar_t));

    /* NOTE it is up to the caller to deal with the tokens */

    return to;

_error:
    if (alloc) string_free(to);
    return NULL;
}

/* -------------------------------------------------------------------------- */

public m_string *string_movs(m_string *to, off_t o, const char *from, size_t l)
{
    /** @brief move a C string or a part of it into a m_string */

    m_string *ret = NULL;

    if ( (ret = _string_mov(to, o, from, l)) )
        string_free_token(to);

    return ret;
}

/* -------------------------------------------------------------------------- */

static void _string_update_size(m_string *s, size_t diff)
{
    while (s) {
        s->_len += diff; if (s->parent) s->_alloc = s->_len;
        s = s->parent;
    }
}

/* -------------------------------------------------------------------------- */

public int string_suppr(m_string *string, off_t o, size_t l)
{
    /** @brief remove a subsection of a string */

    unsigned int i = 0;
    m_string *parent = string;

    if (! string || o < 0 || o + l > SIZE(string)) {
        debug("string_suppr(): bad parameters.\n");
        return -1;
    }

    /* get the parent and compute the absolute offset */
    while (parent->parent) parent = parent->parent;
    o += CSTR(string) - CSTR(parent); i = o + l;

    if (parent->_flags & _STRING_FLAG_ENCAPS && ! o) {
        /* shortcut if a static buffer is used and offset is 0 */
        parent->_data += l;
    } else if (i < SIZE(parent)) {
        /* handle the repositioning of the tokens manually */
        if (! _string_mov(parent, o, CSTR(parent) + i, SIZE(parent) + 1 - i)) {
            debug("string_suppr(): string_movs() failed.\n");
            return -1;
        }
    }

    /* find the beginning of the move using the absolute offset */
    for (i = 0; i < PARTS(parent); i ++)
        if (TSTR(parent, i) - CSTR(parent) >= (int) (o + l)) break;

    /* shift the tokens from this position on */
    while (i < PARTS(parent)) __move_subtokens(TOKEN(parent, i ++), - l, NULL);

    /* update the length of the parent string and of the token */
    _string_update_size(string, - l);

    return 0;
}

/* -------------------------------------------------------------------------- */

public m_string *string_cats(m_string *to, const char *from, size_t len)
{
    /** @brief append a C string to a m_string */

    m_string *orig = to;

    /* find the last token first */
    if (to) {
        while (to->parent) to = to->parent;
        while (to->parts) {
            if (STRING_END(to) == STRING_END(LAST_TOKEN(to)))
                to = LAST_TOKEN(to);
            else break;
        }
    }

    return (to = string_movs(to, (to) ? SIZE(to) : 0, from, len)) ?
           ((orig) ? orig : to) :
           NULL;
}

/* -------------------------------------------------------------------------- */

public m_string *string_pres(m_string *to, const char *from, size_t len)
{
    /** @brief prepend a C string to a m_string */

    /* move the original string to the right */
    if (to && ! (string_movs(to, len, CSTR(to), to->_len)) ) return NULL;

    return string_movs(to, 0, from, len);
}

/* -------------------------------------------------------------------------- */

public m_string *string_cpys(m_string *to, const char *from, size_t len)
{
    /** @brief copy data from a C string to a m_string */

    return string_movs(to, 0, from, len);
}

/* -------------------------------------------------------------------------- */

public int string_cmps(const m_string *a, const char *b, size_t len)
{
    /** @brief compare a m_string and a C string */

    if (! a || ! CSTR(a) || ! b || ! len) {
        debug("string_cmps(): bad parameters.\n");
        return 2;
    }

    return memcmp(CSTR(a), b, MIN(SIZE(a), len));
}

/* -------------------------------------------------------------------------- */
/* 4. Basic string operations (m_string version)                              */
/* -------------------------------------------------------------------------- */

public m_string *string_mov(m_string *to, off_t off,
                            const m_string *from, size_t len)
{
    /** @brief move a string or a part of it into another string */

    if (! from) return NULL;

    return string_movs(to, off, CSTR(from), len);
}

/* -------------------------------------------------------------------------- */

public m_string *string_cat(m_string *to, const m_string *from)
{
    /** @brief append a string to another */

    if (! from) return NULL;

    return string_cats(to, CSTR(from), SIZE(from));
}

/* -------------------------------------------------------------------------- */

public m_string *string_pre(m_string *to, const m_string *from)
{
    /** @brief prepend a string to another */

    if (! from) return NULL;

    return string_pres(to, CSTR(from), SIZE(from));
}

/* -------------------------------------------------------------------------- */

public m_string *string_cpy(m_string *to, const m_string *from)
{
    /** @brief copy data from a string to another */

    if (! from) return NULL;

    return string_cpys(to, CSTR(from), SIZE(from));
}

/* -------------------------------------------------------------------------- */

public int string_cmp(const m_string *a, const m_string *b)
{
    /** @brief compare two strings */

    if (! b) return 2;

    return string_cmps(a, CSTR(b), SIZE(b));
}

/* -------------------------------------------------------------------------- */

public int string_upper(m_string *string)
{
    /** @brief convert an ASCII string to upper case */

    unsigned int i = 0;

    if (! string || ! CSTR(string)) {
        debug("string_upper(): bad parameters.\n");
        return -1;
    }

    /* check if writing to the string is allowed */
    if (string->_flags & _STRING_FLAG_RDONLY) {
        debug("string_upper(): illegal write attempt.\n");
        return -1;
    }

    for (i = 0; i < SIZE(string) && string->_data[i]; i ++)
        string->_data[i] = toupper(string->_data[i]);

    /* since it is an in place transformation, keep existings tokens */

    return 0;
}

/* -------------------------------------------------------------------------- */

public int string_lower(m_string *string)
{
    /** @brief convert an ASCII string to lower case */

    unsigned int i = 0;

    if (! string || ! CSTR(string)) {
        debug("string_lower(): bad parameters.\n");
        return -1;
    }

    /* check if writing to the string is allowed */
    if (string->_flags & _STRING_FLAG_RDONLY) {
        debug("string_lower(): illegal write attempt.\n");
        return -1;
    }

    for (i = 0; i < SIZE(string) && string->_data[i]; i ++)
        string->_data[i] = tolower(string->_data[i]);

    /* since it is an in place transformation, keep existings tokens */

    return 0;
}

/* -------------------------------------------------------------------------- */
/* 5. Advanced string manipulation functions                                  */
/* -------------------------------------------------------------------------- */

public int string_compile_searchstring(m_search_string *c,
                                       const char *s, size_t len)
{
    /** @brief compute the Boyer-Moore lookup table for the given substring */

    unsigned int i = 0;

    if (! c || ! s || ! len || len > 255) {
        debug("string_compile_searchstring(): bad parameters.\n");
        return -1;
    }

    /* register the distance between each possible byte and the last byte
       of the substring in a lookup table */
    memset(c->_lut, len, UCHAR_MAX + 1);

    /*  fix the length */
    len --;

    for (i = 0; i < len; i ++) c->_lut[_UINT(s[i])] = len - i;

    /* get the shift length and set up the guard char */
    c->_shift = c->_lut[_UINT(s[len])];
    c->_lut[_UINT(s[len])] = 0; c->_len = len;

    return 0;
}

/* -------------------------------------------------------------------------- */

public off_t string_compile_find(const char *s, size_t len, size_t o,
                                 const char *sub, m_search_string *c)
{
    /** @brief find a C substring within a string using a precompiled LUT */

    size_t i = 0, k = 0;
    const char *ptr = NULL;

    if (! s || ! len || ! sub || ! c) {
        debug("string_compile_find(): bad parameters.\n");
        return -1;
    }

    if (o + c->_len < len) {
        if (len < UCHAR_MAX) {
            /* naive search algorithm is faster for short strings */
            while ( (ptr = memchr(s + o, sub[0], len - (o + c->_len))) ) {
                if (! c->_len || ! memcmp(ptr + 1, sub + 1, c->_len))
                    return ptr - s;
                if ( (o = (ptr - s) + c->_len) > len) break;
            }
            /* not found */
            return -1;
        }
    } else return -1;

    /* search loop (Tuned Boyer-Moore algorithm) */
    for (i = o; i < len; i += c->_shift) {
        /* use the lookup table to skip impossible matches */
        do {
            if ( (k = i + c->_len) >= len)
                return -1;
            k = c->_lut[_UINT(s[k])];
        } while ( (k != 0) && (i += k) );

        /* try a possible match */
        if (! memcmp(sub, s + i, c->_len)) break;
    }

    return (i >= len) ? -1 : (off_t) i;
}

/* -------------------------------------------------------------------------- */

public off_t string_sfinds(const char *str, size_t slen, size_t o,
                           const char *sub, size_t len            )
{
    /** @brief find a C susbstring within a string */

    m_search_string c;

    /* sanity checks */
    if (! str || ! slen || ! sub || ! len) {
        debug("string_sfinds(): bad parameters.\n");
        return -1;
    }

    if ( (len > slen) || (o > slen) ) {
        debug("string_sfinds(): search string out of bound.\n");
        return -1;
    }

    if (string_compile_searchstring(& c, sub, len) == -1) return -1;

    return string_compile_find(str, slen, o, sub, & c);
}

/* -------------------------------------------------------------------------- */

public off_t string_finds(const m_string *str, size_t o, const char *sub,
                          size_t len)
{
    if (! str || ! CSTR(str) || ! sub || ! len) return -1;

    return string_sfinds(CSTR(str), SIZE(str), o, sub, len);
}

/* -------------------------------------------------------------------------- */

public off_t string_find(const m_string *string, size_t o, const m_string *sub)
{
    /** @brief find a susbstring within a string */

    if (! string || ! sub || ! CSTR(string) || ! CSTR(sub) ) return -1;

    return string_sfinds(CSTR(string), SIZE(string), o, CSTR(sub), SIZE(sub));
}

/* -------------------------------------------------------------------------- */

public off_t string_pos(const m_string *string, const char *sub)
{
    if (! string || ! sub || ! CSTR(string)) return -1;

    return string_finds(string, 0, sub, strlen(sub));
}

/* -------------------------------------------------------------------------- */

public int string_splits(m_string *s, const char *pattern, size_t len)
{
    /** @brief split the string into multiple tokens using a separator */

    off_t offset[16] = { -1 }, off = 0, prev = 0, o = 0, p = 0;
    unsigned int i = 0, j = 0;
    m_string *t = NULL;
    m_search_string compiled_pattern;

    if (! s || ! CSTR(s) || ! pattern || ! len) {
        debug("string_splits(): bad parameters.\n");
        return -1;
    }

    /* the pattern delimiter cannot be longer than the source */
    if (SIZE(s) <= len + 1) {
        debug("string_splits(): delimiter out of bound.\n");
        return -1;
    }

    if (SIZE(s) >= UCHAR_MAX) {
        /* compile the search string */
        if (string_compile_searchstring(& compiled_pattern, pattern, len) == -1)
            return -1;
    } else compiled_pattern._len = len - 1;

    if (s->_parts_alloc) {
        /* try to reuse existing tokens */
        for (i = 0; i < s->parts; i ++)
            string_free_token(& s->token[i]);
        s->parts = 0; t = s->token;
    }

    /* fill the offsets array with the locations of the pattern */
    while (off != -1) {
        off = string_compile_find(CSTR(s), SIZE(s), prev,
                                  pattern, & compiled_pattern);

        offset[j ++] = prev = off; prev += len;

        if (off == -1 || j >= 16) {
            /* allocate enough room for new tokens */
            if (s->_parts_alloc < s->parts + j) {
                if (! (t = realloc(s->token, (s->parts + j) * sizeof(*t))) )
                    goto _err_realloc;
                s->token = t; s->_parts_alloc = s->parts + j;
            }

            if (s->parts) p = t[s->parts - 1]._data - s->_data;

            /* write the tokens */
            for (i = s->parts; i < s->parts + j; i ++, p = o + len) {
                /* the token inherit parent's flags and set the "no free" bit */
                o = offset[i - s->parts];
                t[i].parent = s;
                t[i]._flags = s->_flags | _STRING_FLAG_NOFREE;
                t[i]._data = s->_data + p;
                t[i]._len = (o != -1) ? (size_t) o : SIZE(s);
                t[i]._len -= p;
                t[i]._alloc = t[i]._len;
                t[i]._parts_alloc = t[i].parts = 0;
                t[i].token = NULL;
            }

            j = 0; s->parts = i;
        }
    }

    return 0;

_err_realloc:
    perror(ERR(string_splits, realloc));
    string_free_token(s);
    return -1;
}

/* -------------------------------------------------------------------------- */

public int string_split(m_string *string, const m_string *pattern)
{
    if (! string || ! pattern || ! CSTR(pattern)) return -1;

    return string_splits(string, CSTR(pattern), SIZE(pattern));
}

/* -------------------------------------------------------------------------- */

public int string_merges(m_string *string, const char *pattern, size_t len)
{
    /** @brief replaces separators in a split string by a new separator */

    int p = 0;
    unsigned int last = 0;

    if (! string || ! CSTR(string)) {
        debug("string_merges(): bad parameters.\n");
        return -1;
    }

    /* allow NULL pattern only if a 0 length is specified */
    if (! pattern && len) {
        debug("string_merges(): NULL pattern with non-0 length.\n");
        return -1;
    }

    /* at least 2 tokens are required to merge anything */
    if (! string->token || string->parts < 2) {
        debug("string_merges(): not enough tokens.\n");
        return -1;
    }

    /* get the size difference with the previous delimiter length */
    p = len - (TOKEN_CSTR(string, 1) - TOKEN_END(string, 0));

    /* no length difference, just overwrite the old delimiter */
    if (p == 0) {
        last = PARTS(string) - 1;

        while (last -- > 0) memcpy((char *) TEND(string, last), pattern, len);

        return 0;
    }

    /* only care if the new delimiter is longer */
    string_extend(string, SIZE(string) + (p * PARTS(string)));

    last = PARTS(string);

    if (p > 0) {
        /* move every token starting from the end */
        while (last -- > 0) {
            memmove((char *) TSTR(string, last) + (last * p),
                    TSTR(string, last),
                    TLEN(string, last));
            if (len)
                memcpy((char *) TEND(string, last) + (last * p), pattern, len);
        }
    } else {
        /* move every token from the start */
        for (last = 0; last < PARTS(string); last ++) {
            memmove((char *) TSTR(string, last) + (last * p),
                    TSTR(string, last),
                    TLEN(string, last));
            if (len)
                memcpy((char *) TEND(string, last) + (last * p), pattern, len);
        }
    }

    /* fix the tokens position */
    for (last = 0; last < PARTS(string); last ++)
        __move_subtokens(TOKEN(string, last), last * p, NULL);

    /* update the string size and do some cleanup */
    if (p < 0) string_dim(string, SIZE(string) + ((PARTS(string) - 1) * p));
    else _string_update_size(string, (PARTS(string) - 1) * p);

    return 0;
}

/* -------------------------------------------------------------------------- */

public int string_merge(m_string *string, const m_string *pattern)
{
    if (! string || ! pattern || ! CSTR(pattern)) return -1;

    return string_merges(string, CSTR(pattern), SIZE(pattern));
}

/* -------------------------------------------------------------------------- */

public m_string *string_reps(m_string *string, const char *search, size_t slen,
                             const char *rep, size_t rlen                      )
{
    /** @brief replace all occurences of a C substring in a string */

    size_t count = 0;
    off_t *offset = NULL, off = 0, src = 0, dst = 0;
    unsigned int i = 0;

    /* a NULL replacement string is allowed (deletion) */
    if (! string || ! CSTR(string) || ! search || ! slen) {
        debug("string_reps(): bad parameters.\n");
        return NULL;
    }

    /* we can't replace a substring longer than the source string */
    if (SIZE(string) <= slen + 1) {
        debug("string_reps(): search string out of bound.\n");
        return NULL;
    }

    /* find the maximal number of replacement and allocate the offset array */
    count = SIZE(string) / slen + 1;

    if (! (offset = malloc(count * sizeof(*offset))) ) {
        perror(ERR(string_reps, malloc));
        return NULL;
    }

    /* fill the offset array with the location of the search strings */
    while ( (offset[i] = string_finds(string, off, search, slen)) >= 0) {
        off = offset[i] + slen; if (i < count) i ++; else break;
    }

    if ( (count = i) == 0) {
        debug("string_reps(): search string not found.\n");
        goto _err_nfnd;
    }

    /* XXX ensure the string has enough room for the replacement
       we could rely on string_movs for the resizing, but we really don't
       want subsequent calls to string_movs() to fail with a possible
       out-of-memory error and end up with a garbled string */
    if (string_extend(string, SIZE(string) + count * (rlen - slen)) == -1) {
        debug("string_reps(): resize failure.\n");
        goto _err_size;
    }

    /* perform the replacement */
    for (i = 0; i < count; i ++) {
        /* copy the data between the strings that are going to be replaced */
        if (! string_movs(string, dst, CSTR(string) + src, offset[i] - src))
            goto _err_move;
        dst += offset[i] - src; src = offset[i] + slen;

        /* loop no further without proper replacement string */
        if (! rep || ! rlen) continue;

        /* replace the string */
        if (! string_movs(string, dst, rep, rlen)) goto _err_move;
        dst += rlen;
    }

    /* update the size */
    _string_update_size(string, count * (rlen - slen));

    /* ensure the string is NUL terminated */
    if (! string->parent) string->_data[string->_len] = '\0';

    free(offset);

    return string;

_err_move: /* this should never happen */
    debug("string_reps(): string_movs() failed !\n");
_err_size: /* string_extend() failure */
_err_nfnd: /* not found */
    free(offset);
    return NULL;
}

/* -------------------------------------------------------------------------- */

public m_string *string_rep(m_string *string, const m_string *search,
                            const char *rep, size_t rlen             )
{
    /** @brief replace all occurences of a substring in a string */

    if (! string || ! search || ! CSTR(search)) return NULL;

    return string_reps(string, CSTR(search), SIZE(search), rep, rlen);
}

/* -------------------------------------------------------------------------- */

public m_string *string_rems(m_string *str, const char *rem, size_t len)
{
    /** @brief remove all occurences of a C substring in a string */

    if (! str || ! rem || ! len) {
        debug("string_rems(): bad parameters.\n");
        return NULL;
    }

    return string_reps(str, rem, len, NULL, 0);
}

/* -------------------------------------------------------------------------- */

public m_string *string_rem(m_string *string, const m_string *rem)
{
    /** @brief remove all occurences of a substring in a string */

    if (! string || ! CSTR(string) || ! rem || ! CSTR(rem)) return NULL;

    return string_reps(string, CSTR(rem), SIZE(rem), NULL, 0);
}

/* -------------------------------------------------------------------------- */

public m_string *string_select(m_string *string, unsigned int off, size_t len)
{
    /** @brief create a string token encompassing the given part of string */

    if (! string || ! len) {
        debug("string_select(): bad parameters.\n");
        return NULL;
    }

    string_free_token(string);

    return string_add_token(string, off, off + len);
}

/* -------------------------------------------------------------------------- */

public m_string *string_add_token(m_string *s, off_t start, off_t end)
{
    /** @brief adds a new token to the string, selecting the delimited part */

    m_string *tokens = NULL, *token = NULL;
    unsigned int i = 0, j = 0;
    int p = 0;

    if (unlikely(! s || s->parts == 65535)) {
        debug("string_add_token(): cannot add token.\n");
        return NULL;
    }

    #ifdef DEBUG
    if (! CSTR(s) || (size_t) end > SIZE(s) || end <= start) {
        debug("string_add_token(): bad parameters.\n");
        return NULL;
    }
    #endif

    if (s->_parts_alloc < s->parts + 1) {
        /* 1.5 growth factor */
        if (s->_parts_alloc < 43690)
            p = (s->_parts_alloc >> 1) + ! (s->_parts_alloc >> 1);
        else p = 65535 - s->_parts_alloc;

        tokens = realloc(s->token, (s->_parts_alloc + p) * sizeof(*tokens));
        if (unlikely(! tokens)) {
            perror(ERR(string_add_token, realloc));
            return NULL;
        }

        /* XXX update all the subtokens' pointers */
        if (tokens != s->token) {
            for (i = 0; i < s->parts; i ++) {
                for (j = 0; j < tokens[i].parts; j ++)
                    tokens[i].token[j].parent = & tokens[i];
            }
        }

        s->_parts_alloc += p; s->token = tokens;
    }

    s->parts ++; token = LAST_TOKEN(s);

    /* the token inherits the parent's flags and adds the "no free" bit */
    token->parent = s;
    token->_flags = s->_flags | _STRING_FLAG_NOFREE;
    token->_data = s->_data + start;
    token->_alloc = token->_len = end - start;
    token->_parts_alloc = token->parts = 0;
    token->token = NULL;

    return token;
}

/* -------------------------------------------------------------------------- */

public int string_push_token(m_string *s, m_string *token)
{
    if (! s || ! token || ! CSTR(s) || ! CSTR(token)) {
        debug("string_push_token(): bad parameters.\n");
        return -1;
    }

    return string_push_tokens(s, CSTR(token), SIZE(token));
}

/* -------------------------------------------------------------------------- */

public int string_push_tokens(m_string *s, const char *strtoken, size_t len)
{
    size_t slen = 0;

    if (! s || ! strtoken || ! CSTR(s) || ! len) {
        debug("string_push_token(): bad parameters.\n");
        return -1;
    }

    slen = SIZE(s);

    /* make room for the new data */
    if (string_extend(s, slen + len + 1) == -1) return -1;

    /* copy the data */
    memmove(s->_data + slen, strtoken, len);
    s->_data[slen + len] = 0; s->_len += len;

    /* append the token */
    if (! string_add_token(s, slen, slen + len)) return -1;

    return 0;
}

/* -------------------------------------------------------------------------- */

public m_string *string_suppr_token(m_string *s, unsigned int index)
{
    if (! s || ! CSTR(s) || ! PARTS(s) || index >= PARTS(s)) {
        debug("string_suppr_token(): bad parameters.\n");
        return NULL;
    }

    string_suppr(s, TSTR(s, index) - CSTR(s), TLEN(s, index)); s->parts --;

    return NULL;
}

/* -------------------------------------------------------------------------- */

public m_string *string_pop_token(m_string *s)
{
    m_string *ret = NULL;

    if (! s || ! CSTR(s) || ! PARTS(s)) {
        debug("string_pop_token(): bad parameters.\n");
        return NULL;
    }

    if (! (ret = string_alloc(TSTR(s, 0), TLEN(s, 0))) )
        return NULL;

    string_suppr(s, TSTR(s, 0) - CSTR(s), SIZE(ret));
    memmove(s->token, s->token + 1, -- s->parts * sizeof(*s->token));

    return ret;
}

/* -------------------------------------------------------------------------- */
#ifdef HAS_PCRE
/* -------------------------------------------------------------------------- */

public int string_parse(m_string *string, const char *pattern, size_t len)
{
    /** @brief parses a string into multiple tokens matching a regex */

    int *off = NULL;
    unsigned int i = 0, j = 0, k = 0, last = 0, size = 0;
    m_string *token = NULL, *new_token = NULL;
    pcre *regex = NULL;
    int erroff = 0;
    int r = 0;
    const char *err = NULL;

    if (! string || ! CSTR(string) || ! pattern || ! len) {
        debug("string_parse(): bad parameters.\n");
        return -1;
    }

    if (! (regex = pcre_compile(pattern, PCRE_DOTALL, & err, & erroff, NULL)) ) {
        debug("string_parse(): %s\n", err);
        return -1;
    }

    size = SIZE(string);

    if (size * sizeof(*off) < size) {
        debug("string_parse(): integer overflow.\n");
        return -1;
    }

    if (! (off = malloc(size * sizeof(*off))) ) {
        perror(ERR(string_parse, malloc));
        goto _err_malloc;
    }

    for (i = 0, last = 0; ; last = off[1], i ++) {
        /* match the pattern */
        r = pcre_exec(regex, NULL, CSTR(string), SIZE(string), last, 0x0, off, size);

        if (r <= 0) {
            if (i) break;
            debug("string_parse(): error matching pattern %s\n", pattern);
            goto _err_exec;
        }

        /* found something, add a token and possibly subtokens */
        if (! (new_token = realloc(token, (i + 1) * sizeof(*token))) ) {
            perror(ERR(string_parse, realloc));
            if (token) while (i --) free(token[i].token); free(token);
            goto _err_token;
        }

        token = new_token;
        token[i].parent = string;
        token[i]._flags = string->_flags | _STRING_FLAG_NOFREE;
        token[i]._data = string->_data + off[0];
        token[i]._len = off[1] - off[0] + ! (off[1] - off[0]);
        token[i]._alloc = token[i]._len;

        if (r - 1 < 1) {
            token[i]._parts_alloc = token[i].parts = 0;
            token[i].token = NULL;
            continue;
        }

        token[i]._parts_alloc = token[i].parts = r - 1;

        if (! (token[i].token = malloc(token[i].parts * sizeof(token[i]))) ) {
            perror(ERR(string_parse, malloc));
            goto _err_token;
        }

        for (j = 2, k = 0; j + 1 < (unsigned) r * 2; j += 2, k ++) {
            token[i].token[k].parent = & token[i];
            token[i].token[k]._flags = string->_flags | _STRING_FLAG_NOFREE;
            token[i].token[k]._data = string->_data + off[j];
            token[i].token[k]._len = off[j + 1] - off[j] + ! (off[j + 1] - off[j]);
            token[i].token[k]._alloc = token[i].token[k]._len;
            token[i].token[k]._parts_alloc = token[i].token[k].parts = 0;
            token[i].token[k].token = NULL;
        }
    }

    pcre_free(regex);

    free(off);

    string_free_token(string); string->parts = i; string->token = token;

    return 0;

_err_token:
    if (token) while (i --) free(token[i].token); free(token);
_err_exec:
    free(off);
_err_malloc:
    pcre_free(regex);

    return -1;
}

/* -------------------------------------------------------------------------- */
#endif
/* -------------------------------------------------------------------------- */

public m_string *string_b58s(const char *s, size_t size)
{
    /** @brief convert a C string to base58 encoding */

    size_t i = 0, zcount = 0;
    int j = 0, high = 0, carry = 0;
    int32_t b58size = 0;
    uint8_t *buffer = NULL;
    m_string *ret = NULL;

    if (! s || ! size) {
        debug("string_b58s(): bad parameters.\n");
        return NULL;
    }

    /* get the number of leading NUL chars */
    while (zcount < size && ! s[zcount]) zcount ++;

    /* compute the size of the base58 encoded string, with a trailing NUL */
    b58size = (size - zcount) * 138 / 100 + 1;

    if ((size_t) b58size < size) {
        debug("string_b58s(): integer overflow.\n");
        return NULL;
    }

    if (! (buffer = calloc(b58size, sizeof(*buffer))) ) {
        perror(ERR(string_b58s, calloc));
        return NULL;
    }

    for (i = zcount, high = b58size - 1; i < size; i ++, high = j) {
        for (carry = (uint8_t) s[i], j = b58size - 1; j > high || carry; j --) {
            carry += 256 * buffer[j];
            buffer[j] = carry % 58;
            carry /= 58;
        }
    }

    for (j = 0; j < b58size && ! buffer[j]; j ++);

    if (! (ret = string_alloc(NULL, zcount + b58size - j)) )
        goto _err_alloc;

    if (zcount) memset(ret->_data, '1', zcount);

    for (i = zcount; j < b58size; i ++, j ++)
        ret->_data[i] = _b58[buffer[j]];

    ret->_data[i] = '\0'; ret->_len = i;

_err_alloc:
    free(buffer);

    return ret;
}

/* -------------------------------------------------------------------------- */

public m_string *string_b58(const m_string *s)
{
    /** @brief convert a string to base58 encoding */

    return string_b58s(CSTR(s), SIZE(s));
}

/* -------------------------------------------------------------------------- */

public m_string *string_deb58s(const char *s, size_t size)
{
    /** @brief convert a base58 encoded C string to plain text */

    uint32_t *r = NULL;
    size_t i = 0, zcount = 0, remain = 0;
    int j = 0, outlen = 0;
    uint64_t b = 0;
    uint32_t c = 0, mask = 0;
    m_string *ret = NULL;

    if (! s || ! size) {
        debug("string_deb58s(): bad parameters.\n");
        return NULL;
    }

    /* output buffer */
    if (! (r = calloc( (outlen = (size + 3) / 4) + 1, sizeof(*r))) ) {
        perror(ERR(string_deb58s, calloc));
        return NULL;
    }

    /* string wrapper */
    if (! (ret = string_alloc(NULL, 0)) ) {
        debug("string_deb58s(): out of memory.\n");
        free(r);
        return NULL;
    }

    /* legitimate leading 0s */
    while (zcount < size && s[zcount] == '1') zcount ++;

    if ( (remain = size & 3) ) mask = 0xFFFFFFFF << (remain * 8);

    for (i = zcount; i < size; i ++) {
        if ((int) (c = _D58(s[i])) == -1) goto _panic;

        for (j = outlen - 1; j; j --) {
            b = ((uint64_t) r[j]) * 58 + c;
            c = (b & 0x3F00000000) >> 32;
            r[j] = b & 0xFFFFFFFF;
        }

        /* output was too large */
        if (c || r[0] & mask) goto _panic;
    }

    c = r[(i = 0)]; ret->_data = (char *) r;

    switch (remain) {
    case 3: { ret->_data[i ++] = (c & 0xFF0000) >> 16; }
    case 2: { ret->_data[i ++] = (c & 0xFF00) >> 8; }
    case 1: { ret->_data[i ++] = (c & 0xFF); j = 1; goto _loop; }
    }

    for (j = 0; j < outlen; j ++) {
_loop:  c = r[j];
        ret->_data[i ++] = (c >> 0x18) & 0xFF;
        ret->_data[i ++] = (c >> 0x10) & 0xFF;
        ret->_data[i ++] = (c >> 0x08) & 0xFF;
        ret->_data[i ++] = (c & 0xFF);
    }

    /* check if there is spurious remaining 0s */
    for (remain = 0; remain < i && ! ret->_data[remain]; remain ++);
    if (zcount > remain) goto _panic;

    /* real length */
    ret->_len = i - (remain - zcount);

    /* remove spurious leading 0s */
    memmove(ret->_data, ret->_data + remain - zcount, ret->_len);
    ret->_data[ret->_len] = '\0';

    /* package the buffer */
    ret->_flags = 0; ret->_alloc = outlen * sizeof(*r);
    ret->_parts_alloc = ret->parts = 0; ret->token = NULL;

    return ret;

_panic:
    free(r); string_free(ret);
    return NULL;
}

/* -------------------------------------------------------------------------- */

public m_string *string_deb58(const m_string *s)
{
    /** @brief convert a base58 encoded string to plain text */

    return string_deb58s(CSTR(s), SIZE(s));
}

/* -------------------------------------------------------------------------- */

public m_string *string_b64s(const char *s, size_t size, size_t linesize)
{
    /** @brief convert a C string to base64 encoding */

    size_t i = 0;
    int j = 0, crlf = 0;
    int32_t b64size = 0;
    char *r = NULL;
    m_string *ret = NULL;

    if (! s || ! size) {
        debug("string_b64s(): bad parameters.\n");
        return NULL;
    }

    /* reject bogus linesize */
    if ( (linesize) && ((linesize % 4) || (linesize > 72)) ) {
        debug("string_b64s(): bad line size.\n");
        return NULL;
    }

    /* compute the size of the base64 encoded string, with a trailing NUL */
    b64size = (((size + 3 - (size % 3)) / 3) * 4) + 1;

    /* add some room for CRLF depending on the linesize */
    if (linesize) b64size += ((b64size / linesize) * 2) + 2;

    if ((size_t) b64size < size) {
        debug("string_b64s(): integer overflow.\n");
        return NULL;
    }

    if (! (r = calloc(b64size, sizeof(*r))) ) {
        perror(ERR(string_b64s, calloc));
        return NULL;
    }

    while (i + 3 < size) {

        if (j + 4 < b64size) {
            /* convert three bytes in ASCII characters */
            r[j ++] = _b64[(s[i] & 0xFC) >> 2];
            r[j ++] = _b64[((s[i] & 0x03) << 4) | ((s[i + 1] & 0xF0) >> 4)];
            r[j ++] = _b64[((s[i + 1] & 0x0F) << 2) | ((s[i + 2] & 0xC0) >> 6)];
            r[j ++] = _b64[s[i + 2] & 0x3F]; i += 3;
        } else goto _panic;

        /* if a linesize was given, check if we should insert a CRLF */
        if (linesize && ((j - crlf) % linesize == 0) ) {
            if (j + 2 < b64size) {
                r[j ++] = '\r'; r[j ++] = '\n'; crlf += 2;
            } else goto _panic;
        }
    }

    if ((size + 1 - i) && j + 4 < b64size) {
        /* add some padding */
        memset(r + j, '=', 4);

        /* process the remaining */
        switch (size + 1 - i) {
        case 4:
        r[j + 3] = _b64[s[i + 2] & 0x3F];
        case 3:
        r[j + 2] = _b64[(s[i + 1] & 0x0F) << 2 | (s[i + 2] & 0xC0) >> 6];
        case 2:
        r[j + 1] = _b64[(s[i] & 0x03) << 4 | (s[i + 1] & 0xF0) >> 4];
        case 1:
        r[j] = _b64[(s[i] & 0xFC) >> 2]; j += 4;
        }
    }

    /* add the last CRLF if a linesize was provided */
    if ( (linesize) && j + 2 < b64size) {
        r[j] = '\r'; r[j + 1] = '\n';
    }

    /* package the buffer in a string_m */
    if (! (ret = string_alloc(NULL, 0)) ) {
        free(r);
    } else {
        ret->_flags = 0; ret->_data = r;
        ret->_len = b64size - 1; ret->_alloc = b64size -1;
        ret->_parts_alloc = ret->parts = 0;
        ret->token = NULL;
    }

    return ret;

_panic:
    debug("string_b64s(): base64 encoding failed.\n");
    free(r);
    return NULL;
}

/* -------------------------------------------------------------------------- */

public m_string *string_b64(const m_string *s, size_t linesize)
{
    /** @brief convert a string to base64 encoding */

    return string_b64s(CSTR(s), SIZE(s), linesize);
}

/* -------------------------------------------------------------------------- */

public m_string *string_deb64s(const char *s, size_t size)
{
    /** @brief convert a base64 encoded C string to plain text */

    char *r = NULL;
    size_t i = 0, j = 0;
    m_string *ret = NULL;

    if (! s || ! size) {
        debug("string_deb64s(): bad parameters.\n");
        return NULL;
    }

    /* the number of CRLF or other trailing characters is unknown,
       so allocate the same size to be safe */
    if (! (r = calloc(size + 1, sizeof(*r))) ) {
        perror(ERR(string_deb64s, calloc));
        return NULL;
    }

    if (! (ret = string_alloc(NULL, 0)) ) {
        debug("string_deb64s(): out of memory.\n");
        free(r);
        return NULL;
    }

    /* discard trailing characters */
    while (-- size && _D64(s[size]) == -1);

    while (i + 4 < size) {
        if (_D64(s[i]) != -1) {
            /* take four ASCII chars and convert them in three bytes */
            r[j ++] = _D64(s[i]) << 2 | _D64(s[i + 1]) >> 4;
            r[j ++] = _D64(s[i + 1]) << 4 | _D64(s[i + 2]) >> 2;
            r[j ++] = ((_D64(s[i + 2]) << 6) & 0xC0) | _D64(s[i + 3]);
            i += 4;
        } else i ++; /* drop CRLF and other noise */
    }

    /* skip garbage between the last processed piece and the remaining */
    while (_D64(s[i]) == -1 && i ++ < size);

    /* process the remaining */
    switch (size - i) {
    case 3:
    r[j + 2] = ((_D64(s[i + 2]) << 6) & 0xC0) | _D64(s[i + 3]);
    case 2:
    r[j + 1] = _D64(s[i + 1]) << 4 | _D64(s[i + 2]) >> 2;
    case 1:
    r[j] = _D64(s[i]) << 2 | _D64(s[i + 1]) >> 4; j += 3;
    }

    /* package the buffer */
    ret->_flags = 0; ret->_data = r;
    ret->_len = j; ret->_alloc = i;
    ret->_parts_alloc = ret->parts = 0;
    ret->token = NULL;

    return ret;
}

/* -------------------------------------------------------------------------- */

public m_string *string_deb64(const m_string *s)
{
    /** @brief convert a base64 encoded string to plain text */

    return string_deb64s(CSTR(s), SIZE(s));
}

/* -------------------------------------------------------------------------- */
#ifdef HAS_ZLIB
/* -------------------------------------------------------------------------- */

public m_string *string_compress(m_string *s)
{
    m_string *z = NULL;
    Bytef *dest = NULL;
    uLongf dlen = 0;

    if (! s || ! s->_data || ! SIZE(s)) return NULL;

    if (! (z = string_alloc(NULL, ((SIZE(s) * 11)/10 + 12))) ) {
        debug("string_compress(): cannot allocate string.\n");
        return NULL;
    }

    dest = (Bytef *) z->_data; dlen = z->_len;

    if (compress2(dest, & dlen, (Bytef *) CSTR(s), (uLong) SIZE(s), 9) != 0) {
        debug("string_compress(): gzip compression failed.\n");
        return string_free(z);
    }

    z->_len = dlen;

    return z;
}

/* -------------------------------------------------------------------------- */

public m_string *string_uncompress(m_string *s, size_t original_size)
{
    m_string *z = NULL;
    Bytef *dest = NULL;
    uLongf dlen = 0;

    if (! s || ! s->_data || ! SIZE(s)) return NULL;

    if (! (z = string_alloc(NULL, original_size)) ) {
        debug("string_uncompress(): cannot allocate string.\n");
        return NULL;
    }

    dest = (Bytef *) z->_data; dlen = z->_len;

    switch (uncompress(dest, & dlen, (Bytef *) CSTR(s), (uLong) SIZE(s))) {
    case Z_OK: z->_len = dlen; return z;
    case Z_MEM_ERROR: debug("string_uncompress(): out of memory.\n"); break;
    case Z_DATA_ERROR: debug("string_uncompress(): data corruption.\n"); break;
    case Z_BUF_ERROR: debug("string_uncompress(): not enough memory.\n"); break;
    default: debug("string_uncompress(): error uncompressing data.\n");
    }

    return string_free(z);
}

/* -------------------------------------------------------------------------- */
#endif
/* -------------------------------------------------------------------------- */

/* -------------------------------------------------------------------------- */
/* SHA1 */
/* -------------------------------------------------------------------------- */

#if defined(__GNUC__) && defined(__i386__)
static inline uint32_t rol(uint32_t x, int n)
{
    __asm__("roll %%cl,%0" :"=r" (x) :"0" (x),"c" (n));
    return x;
}
#else
#define rol(x,n) ( ((x) << (n)) | ((x) >> (32 - (n))) )
#endif

/* -------------------------------------------------------------------------- */

static void _sha1_init(SHA1_CONTEXT *hd)
{
    hd->h0 = 0x67452301;
    hd->h1 = 0xefcdab89;
    hd->h2 = 0x98badcfe;
    hd->h3 = 0x10325476;
    hd->h4 = 0xc3d2e1f0;
    hd->nblocks = 0;
    hd->count = 0;
}

/* -------------------------------------------------------------------------- */

static void _sha1_transform(SHA1_CONTEXT *hd, unsigned char *data)
{
    /* transform the message X which consists of 16 32-bit-words */

    uint32_t a, b, c, d, e, tm;
    uint32_t x[16];

    /* get values from the chaining vars */
    a = hd->h0;
    b = hd->h1;
    c = hd->h2;
    d = hd->h3;
    e = hd->h4;

    #ifdef BIG_ENDIAN_HOST
    memcpy(x, data, 64);
    #else /* little endian */
    {
        int i;
        unsigned char *p2;

        for (i = 0, p2 = (unsigned char *) x; i < 16; i ++, p2 += 4) {
            p2[3] = *data ++;
            p2[2] = *data ++;
            p2[1] = *data ++;
            p2[0] = *data ++;
        }
    }
    #endif

    #define K1 0x5A827999L
    #define K2 0x6ED9EBA1L
    #define K3 0x8F1BBCDCL
    #define K4 0xCA62C1D6L
    #define F1(x, y, z) (z ^ (x & (y ^ z)))
    #define F2(x, y, z) (x ^ y ^ z)
    #define F3(x, y, z) ( (x & y) | (z & (x | y)) )
    #define F4(x, y, z) (x ^ y ^ z)

    #define M(i) (tm = x[i & 0xf] ^ x[(i - 14) & 0xf]   \
                  ^ x[(i - 8) & 0xf] ^ x[(i - 3) & 0xf] \
                  , (x[i & 0xf] = rol(tm, 1)) )

    #define R(a,b,c,d,e,f,k,m)               \
    do {                                     \
        e += rol(a, 5) + f(b, c, d) + k + m; \
        b = rol(b, 30);                      \
    } while (0)

    R(a, b, c, d, e, F1, K1, x[ 0]);
    R(e, a, b, c, d, F1, K1, x[ 1]);
    R(d, e, a, b, c, F1, K1, x[ 2]);
    R(c, d, e, a, b, F1, K1, x[ 3]);
    R(b, c, d, e, a, F1, K1, x[ 4]);
    R(a, b, c, d, e, F1, K1, x[ 5]);
    R(e, a, b, c, d, F1, K1, x[ 6]);
    R(d, e, a, b, c, F1, K1, x[ 7]);
    R(c, d, e, a, b, F1, K1, x[ 8]);
    R(b, c, d, e, a, F1, K1, x[ 9]);
    R(a, b, c, d, e, F1, K1, x[10]);
    R(e, a, b, c, d, F1, K1, x[11]);
    R(d, e, a, b, c, F1, K1, x[12]);
    R(c, d, e, a, b, F1, K1, x[13]);
    R(b, c, d, e, a, F1, K1, x[14]);
    R(a, b, c, d, e, F1, K1, x[15]);
    R(e, a, b, c, d, F1, K1, M(16));
    R(d, e, a, b, c, F1, K1, M(17));
    R(c, d, e, a, b, F1, K1, M(18));
    R(b, c, d, e, a, F1, K1, M(19));
    R(a, b, c, d, e, F2, K2, M(20));
    R(e, a, b, c, d, F2, K2, M(21));
    R(d, e, a, b, c, F2, K2, M(22));
    R(c, d, e, a, b, F2, K2, M(23));
    R(b, c, d, e, a, F2, K2, M(24));
    R(a, b, c, d, e, F2, K2, M(25));
    R(e, a, b, c, d, F2, K2, M(26));
    R(d, e, a, b, c, F2, K2, M(27));
    R(c, d, e, a, b, F2, K2, M(28));
    R(b, c, d, e, a, F2, K2, M(29));
    R(a, b, c, d, e, F2, K2, M(30));
    R(e, a, b, c, d, F2, K2, M(31));
    R(d, e, a, b, c, F2, K2, M(32));
    R(c, d, e, a, b, F2, K2, M(33));
    R(b, c, d, e, a, F2, K2, M(34));
    R(a, b, c, d, e, F2, K2, M(35));
    R(e, a, b, c, d, F2, K2, M(36));
    R(d, e, a, b, c, F2, K2, M(37));
    R(c, d, e, a, b, F2, K2, M(38));
    R(b, c, d, e, a, F2, K2, M(39));
    R(a, b, c, d, e, F3, K3, M(40));
    R(e, a, b, c, d, F3, K3, M(41));
    R(d, e, a, b, c, F3, K3, M(42));
    R(c, d, e, a, b, F3, K3, M(43));
    R(b, c, d, e, a, F3, K3, M(44));
    R(a, b, c, d, e, F3, K3, M(45));
    R(e, a, b, c, d, F3, K3, M(46));
    R(d, e, a, b, c, F3, K3, M(47));
    R(c, d, e, a, b, F3, K3, M(48));
    R(b, c, d, e, a, F3, K3, M(49));
    R(a, b, c, d, e, F3, K3, M(50));
    R(e, a, b, c, d, F3, K3, M(51));
    R(d, e, a, b, c, F3, K3, M(52));
    R(c, d, e, a, b, F3, K3, M(53));
    R(b, c, d, e, a, F3, K3, M(54));
    R(a, b, c, d, e, F3, K3, M(55));
    R(e, a, b, c, d, F3, K3, M(56));
    R(d, e, a, b, c, F3, K3, M(57));
    R(c, d, e, a, b, F3, K3, M(58));
    R(b, c, d, e, a, F3, K3, M(59));
    R(a, b, c, d, e, F4, K4, M(60));
    R(e, a, b, c, d, F4, K4, M(61));
    R(d, e, a, b, c, F4, K4, M(62));
    R(c, d, e, a, b, F4, K4, M(63));
    R(b, c, d, e, a, F4, K4, M(64));
    R(a, b, c, d, e, F4, K4, M(65));
    R(e, a, b, c, d, F4, K4, M(66));
    R(d, e, a, b, c, F4, K4, M(67));
    R(c, d, e, a, b, F4, K4, M(68));
    R(b, c, d, e, a, F4, K4, M(69));
    R(a, b, c, d, e, F4, K4, M(70));
    R(e, a, b, c, d, F4, K4, M(71));
    R(d, e, a, b, c, F4, K4, M(72));
    R(c, d, e, a, b, F4, K4, M(73));
    R(b, c, d, e, a, F4, K4, M(74));
    R(a, b, c, d, e, F4, K4, M(75));
    R(e, a, b, c, d, F4, K4, M(76));
    R(d, e, a, b, c, F4, K4, M(77));
    R(c, d, e, a, b, F4, K4, M(78));
    R(b, c, d, e, a, F4, K4, M(79));

    /* update chaining vars */
    hd->h0 += a;
    hd->h1 += b;
    hd->h2 += c;
    hd->h3 += d;
    hd->h4 += e;
}

/* -------------------------------------------------------------------------- */

static void _sha1_write(SHA1_CONTEXT *hd, const char *inbuf, size_t inlen)
{
    /* update the message digest with the contents of INBUF with length INLEN */

    if (hd->count == 64) {
        /* flush the buffer */
        _sha1_transform(hd, hd->buf);
        hd->count = 0;
        hd->nblocks ++;
    }

    if (! inbuf) return;

    if (hd->count) {
        for ( ; inlen && hd->count < 64; inlen --)
            hd->buf[hd->count ++] = *inbuf ++;

        _sha1_write(hd, NULL, 0);

        if (! inlen) return;
    }

    while (inlen >= 64) {
        _sha1_transform(hd, (unsigned char *) inbuf);
        hd->count = 0;
        hd->nblocks ++;
        inlen -= 64;
        inbuf += 64;
    }

    for ( ; inlen && hd->count < 64; inlen --)
        hd->buf[hd->count ++] = *inbuf ++;
}

/* -------------------------------------------------------------------------- */

static void _sha1_final(SHA1_CONTEXT *hd)
{
    /* the routine final terminates the computation and returns the digest.
       the handle is prepared for a new cycle, but adding bytes to the handle
       will destroy the returned buffer.
       returns: 20 bytes representing the digest. */

    uint32_t t, msb, lsb;
    unsigned char *p;

    /* flush */
    _sha1_write(hd, NULL, 0);

    t = hd->nblocks;

    /* multiply by 64 to make a byte count */
    lsb = t << 6;
    msb = t >> 26;

    /* add the count */
    t = lsb;
    if ( (lsb += hd->count) < t)
    msb ++;

    /* multiply by 8 to make a bit count */
    t = lsb;
    lsb <<= 3;
    msb <<= 3;
    msb |= t >> 29;

    if (hd->count < 56) {
        /* enough room, add some padding */
        hd->buf[hd->count ++] = 0x80;
        while (hd->count < 56) hd->buf[hd->count ++] = 0;
    } else {
        /* need one extra block */
        hd->buf[hd->count ++] = 0x80; /* pad character */
        while (hd->count < 64) hd->buf[hd->count ++] = 0;
        /* flush */
        _sha1_write(hd, NULL, 0);
        /* fill next block with zeroes */
        memset(hd->buf, 0, 56);
    }

    /* append the 64 bit count */
    hd->buf[56] = msb >> 24;
    hd->buf[57] = msb >> 16;
    hd->buf[58] = msb >>  8;
    hd->buf[59] = msb      ;
    hd->buf[60] = lsb >> 24;
    hd->buf[61] = lsb >> 16;
    hd->buf[62] = lsb >>  8;
    hd->buf[63] = lsb      ;
    _sha1_transform(hd, hd->buf);

    p = hd->buf;

    #ifdef BIG_ENDIAN_HOST
    #define X(a) do { *(uint32_t *) p = hd->h##a ; p += 4; } while(0)
    #else /* little endian */
    #define X(a)                                        \
    do {                                                \
        *p ++ = hd->h##a >> 24; *p ++ = hd->h##a >> 16; \
        *p ++ = hd->h##a >> 8; *p ++ = hd->h##a;        \
    } while(0)
    #endif

    X(0);
    X(1);
    X(2);
    X(3);
    X(4);

    #undef X
}

/* -------------------------------------------------------------------------- */

public m_string *string_sha1s(const char *string, size_t len)
{
    /** @brief returns the sha1 sum of the given string */

    SHA1_CONTEXT ctx;
    char *buf = NULL;
    unsigned int i = 0;
    m_string *ret = NULL;

    if (! string || ! len) return NULL;

    if (! (buf = malloc(41 * sizeof(*ret))) ) {
        perror(ERR(string_sha1s, malloc));
        return NULL;
    }

    _sha1_init(& ctx);
    _sha1_write(& ctx, (const char *) string, len);
    _sha1_final(& ctx);

    for (i = 0; i < 20; i ++) {
        buf[i * 2] = _hex[(ctx.buf[i] >> 4) & 0xf];
        buf[(i * 2) + 1] = _hex[ctx.buf[i] & 0xf];
    }
    buf[40] = 0;

    /* package the buffer in a string_m */
    if (! (ret = string_alloc(NULL, 0)) ) {
        free(buf);
    } else {
        ret->_flags = 0; ret->_data = buf;
        ret->_len = 40; ret->_alloc = 41;
        ret->_parts_alloc = ret->parts = 0;
        ret->token = NULL;
    }

    return ret;
}

/* -------------------------------------------------------------------------- */

public m_string *string_sha1(m_string *s)
{
    return string_sha1s(CSTR(s), SIZE(s));
}

/* -------------------------------------------------------------------------- */
/* -- end of SHA1 implementation */
/* -------------------------------------------------------------------------- */

/* -------------------------------------------------------------------------- */
#ifdef _ENABLE_HTTP
/* -------------------------------------------------------------------------- */

public char *string_rawurlencode(const char *url, size_t len, int flags)
{
    char *buf = NULL;
    int32_t bufsize = 0;
    const char *p = url;
    char *q = NULL;

    bufsize = (len * 3) + 1;

    if (bufsize * sizeof(*buf) < len) {
        debug("string_rawurlencode(): integer overflow.\n");
        return NULL;
    }

    if (! (q = buf = malloc(bufsize * sizeof(*buf))) ) {
        perror(ERR(string_rawurlencode, malloc));
        return NULL;
    }

    do {
        switch (_unsafe[(int) *p]) {
        case 1: /* unsafe chars */
        case 2: /* control chars */
        case 3: /* 0x7f */
        case 4: /* non US-ASCII */
                goto _encode;
        case 5: /* reserved chars */
                if (flags & RFC1738_ESCAPE_RESERVED) goto _encode;
        case 6: /* escape char (%) */
                if (~flags & RFC1738_ESCAPE_UNESCAPED) goto _encode;
        default:
                /* copy the char unencoded */
                *q ++ = *p ++;
                continue;
        }

_encode:
        *q ++ = '%';
        *q ++ = _hex[(*p >> 4) & 0xf];
        *q ++ = _hex[*p ++ & 0xf];
    } while (p < url + len && q < buf + bufsize);

    *q = '\0';

    return buf;
}

/* -------------------------------------------------------------------------- */

public int string_urlencode(m_string *url, int flags)
{
    char *encoded = NULL;
    size_t len = 0;

    if (! url || ! CSTR(url)) return -1;

    if (! (encoded = string_rawurlencode(CSTR(url), SIZE(url), flags)) )
        return -1;

    len = strlen(encoded);

    /* replace the original data */
    if (string_extend(url, len) == -1) {
        debug("string_urlencode(): cannot resize the string.\n");
        free(encoded);
        return -1;
    }

    memcpy((void *) CSTR(url), encoded, len);

    free(encoded);

    return 0;
}

/* -------------------------------------------------------------------------- */
#endif
/* -------------------------------------------------------------------------- */

/* -------------------------------------------------------------------------- */
#ifdef _ENABLE_JSON
/* -------------------------------------------------------------------------- */

public int string_parse_json(m_string *s, int strict, m_json_parser *ctx)
{
    unsigned int pos = 0, i = 0, kv = 0, key = 0;
    unsigned char c = 0, z = 0;
    char value_expected = 0, radix = 0, exp = 0, sign = 0, leading_digit = 0;
    char *p = NULL;
    m_string *json = s;
    uint32_t prefetch = 0;

    if (! json) {
        debug("string_parse_json(): bad parameters.\n");
        return -1;
    }

    /* check if we should resume parsing */
    if (PARTS(json) && IS_TYPE(LAST_TOKEN(json), JSON_TYPE)) {
        if (IS_BUFFER(s)) {
            /* the maximum amount of tokens was reached */
            for (json = LAST_TOKEN(s) ; PARTS(json); json = LAST_TOKEN(json)) {
                if (PARTS(LAST_TOKEN(json)) == 65535) {
                    json = LAST_TOKEN(json);

                    i = CSTR(LAST_TOKEN(json)) - CSTR(json) +
                        SIZE(LAST_TOKEN(json)) + 1;

                    for (pos = 0; pos < 65535; pos ++)
                        string_free_token(& json->token[pos]);
                    json->parts = 0;

                    s->_flags &= ~_STRING_FLAG_BUFFER;
                    break;
                }
            }
        } else if (HAS_ERROR(LAST_TOKEN(json))) {
            /* input was incomplete */
            if (PARTS(LAST_TOKEN(json))) json = LAST_TOKEN(json);
            i = CSTR(LAST_TOKEN(json)) - CSTR(json);
            string_free_token(LAST_TOKEN(json)); json->parts --;
            if (IS_TYPE(json, JSON_OBJECT)) kv = 1;
        } else string_free_token(json);
    } else string_free_token(json);

    /* skip UTF-8 BOM if present */
    if (! memcmp(CSTR(json) + i, "\xef\xbb\xbf", MIN(SIZE(json) - i, 3)))
        i += 3;

    for (pos = i; pos < SIZE(json); pos ++) {

        switch ( (z = _j[ (c = json->_data[pos]) ]) ) {

        /* { */
        case OBJ_START: if (! IS_STRING(json)) {
            if (strict && IS_OBJECT(json) && ! kv) goto _error;
            kv = 0; key = 1;
        } else break;

        /* [ */
        case ARR_START: if (! IS_STRING(json)) {
            if (IS_PRIMITIVE(json)) goto _error;

            if (unlikely(! (json = string_add_token(json, pos, SIZE(json)))))
                goto _nomem;

            /* try to prealloc at least 4 tokens */
            if (likely(json->token = malloc(4 * sizeof(*json->token))))
                json->_parts_alloc = 4;

            json->_flags &= ~JSON_TYPE;
            json->_flags |= (c == '[') ? JSON_ARRAY : JSON_OBJECT;
            json->_flags |= _STRING_FLAG_ERRORS;
            value_expected = 1; pos = 0;

            /* parser callback */
            if (ctx && ctx->init) {
                if (ctx->init(json->_flags & JSON_TYPE, ctx) == 1)
                    return 0;
            }
        } break;

        /* } */
        case OBJ_CLOSE: if (! IS_STRING(json)) {
            if (strict && ! kv && json->parts & 0x1) goto _error;
            kv = 1;
        } else break;

        /* ] */
        case ARR_CLOSE: if (! IS_STRING(json)) {
            if (strict && value_expected) {
                if (! IS_TYPE(json, JSON_ARRAY | JSON_OBJECT) || json->parts) {
                    debug("string_parse_json(): a value is expected.\n");
                    goto _error;
                }
            }

            value_expected = 0;

            if (IS_PRIMITIVE(json)) {
                if (json->parent) {
                    if (IS_TYPE(json->parent, JSON_ARRAY | JSON_OBJECT)) {
                        pos --; goto _token;
                    }
                }

                if (strict) {
                    debug("string_parse_json(): parent is not an array or "
                          "an object.\n");
                    goto _error;
                }
            } else {
                /* a closing bracket must be within an array or object */
                if (strict && ! IS_TYPE(json, JSON_ARRAY | JSON_OBJECT))
                    goto _error;

                /* check if the brackets are matching */
                if (json->_data[0] != c - 2) {
                    debug("string_parse_json(): mismatched or missing "
                          "bracket.\n");
                    goto _error;
                } else goto _token;
            }
        } break;

        /* ', " */
        case QUOTE_SGL: if (! strict)
        case QUOTE_DBL: {
            if (unlikely(IS_PRIMITIVE(json))) goto _error;

            if (! IS_STRING(json)) {
                if (strict && IS_TYPE(json, JSON_ARRAY | JSON_OBJECT)) {
                    if (! value_expected) {
                        debug("string_parse_json(): unexpected string.\n");
                        goto _error;
                    }
                }

                json = string_add_token(json, pos, SIZE(json));
                if (unlikely(! json)) goto _nomem;

                json->_flags &= ~JSON_TYPE; json->_flags |= JSON_STRING;
                json->_flags |= _STRING_FLAG_ERRORS;
                value_expected = 0; pos = 0;

                if (strict) {
                    /* optimize for long strings */
                    prefetch = *(uint32_t *) (json->_data + pos + 1);
                    if (__zero(prefetch & 0x1D1D1D1DU)) /* " */
                        continue;
                    if (unlikely(__zero(prefetch & 0x23232323U))) /* \ */
                        continue;
                    if (unlikely(__less(prefetch, 0x20))) /* unescaped chars */
                        continue;
                    pos = MIN(4, SIZE(json) - pos);
                }
            } else {
                /* check if the quotes are matching */
                if (json->_data[0] != c) {
                    debug("string_parse_json(): mismatched or missing "
                          "quotation mark.\n");
                    goto _error;
                } else goto _token;
            }
        } break;

        /* \ */
        case ESCAPESEQ: { /* escape sequence */
            if (pos + 1 >= SIZE(json)) goto _error;

            if (! IS_STRING(json)) {
                if (! strict && IS_PRIMITIVE(json)) {
                    json->_flags &= ~JSON_PRIMITIVE;
                    json->_flags |= JSON_STRING;
                } else {
                    debug("string_parse_json(): escape sequences are not "
                          "allowed in primitives.\n");
                    goto _error;
                }
            }

            switch (json->_data[++ pos]) {
            case '\"':
            case  '/':
            case '\\':
            case  'b':
            case  'f':
            case  'r':
            case  'n':
            case  't': break;
            case  'u': { /* unicode escape sequence */
                for (i = 0; i < 4 && ++ pos < SIZE(json); i ++) {
                    if (~(z = _j[ (c = json->_data[pos]) ]) & DIGIT_HEX) {
                        debug("string_parse_json(): hexadecimal expected.\n");
                        goto _error;
                    }
                }
                i = 0; pos --;
            } break;

            case '\'': if (! strict) break;

            /* unexpected char */
            default: goto _error;
            }
        } break;

        /* \t, \r, \n 0x20 */
        case WHITE: if (strict && IS_STRING(json)) goto _error;
        case SPACE: if (unlikely(IS_PRIMITIVE(json))) {
            if (strict == JSON_STRICT && value_expected) goto _error;
            goto _delim;
        } break;

        /* : */
        case COLON: if (! IS_STRING(json)) {
            if (IS_PRIMITIVE(json)) {
                if (strict) {
                    debug("string_parse_json(): a key must be a string "
                          "enclosed in quotation marks.\n");
                    goto _error;
                } else { value_expected = 1; goto _delim; }
            } else if (IS_ARRAY(json)) {
                if (strict) {
                    debug("string_parse_json(): key/value pairs are only "
                          "allowed in objects.\n");
                    goto _error;
                }
            }

            if (strict && unlikely(value_expected || kv ++)) goto _error;
            value_expected = 1;

            /* parser callback */
            if (ctx) {
                ctx->key.current = CSTR(LAST_TOKEN(json));
                ctx->key.len = SIZE(LAST_TOKEN(json));
                key = 0;
            }

            /* skip space */
            if (likely(*(json->_data + pos + 1) == 0x20)) pos ++;
        } break;

        /* , */
        case COMMA: if (! IS_STRING(json)) {
            if (strict && value_expected) {
                debug("string_parse_json(): a value is expected.\n");
                goto _error;
            }

            if (IS_PRIMITIVE(json)) {
                if (json->parent) {
                    if (IS_TYPE(json->parent, JSON_ARRAY | JSON_OBJECT)) {
                        if (strict && (IS_OBJECT(json->parent) && ! kv --))
                            goto _error;
                        value_expected = 1; goto _delim;
                    }
                }

                if (strict) {
                    debug("string_parse_json(): parent is not an object "
                          "or an array.\n");
                    goto _error;
                }
            } else {
                if (IS_TYPE(json, JSON_ARRAY | JSON_OBJECT)) {
                    if (strict && unlikely(IS_OBJECT(json) && ! kv --))
                        goto _error;
                    value_expected = 1;
                } else if (strict) {
                    debug("string_parse_json(): not an object or an array.\n");
                    goto _error;
                }
            }

            /* skip space */
            if (likely(*(json->_data + pos + 1) == 0x20)) pos ++;
        } break;

        /* + */
        case DIGIT_POS: if (! IS_STRING(json) && strict == JSON_STRICT) {
            if (! IS_PRIMITIVE(json) || ! exp || sign) goto _error;
            sign = 1; value_expected = 1;
        } break;

        /* - */
        case DIGIT_NEG: if (! IS_STRING(json) && strict == JSON_STRICT) {
            if (sign) goto _error;

            if (! IS_PRIMITIVE(json)) {
                json = string_add_token(json, pos, SIZE(json));
                if (unlikely(! json)) goto _nomem;
                json->_flags &= ~JSON_TYPE; json->_flags |= JSON_PRIMITIVE;
                pos = 0; exp = 0; radix = 0; leading_digit = 0;
            } else if (! exp || ! value_expected) goto _error;

            sign = 1; value_expected = 1;
        } break;

        /* . */
        case DIGIT_RAD: if (! IS_STRING(json) && strict == JSON_STRICT) {
            if (! IS_PRIMITIVE(json) || radix) {
                debug("string_parse_json(): unexpected decimal separator.\n");
                goto _error;
            }

            /* a decimal separator cannot be placed right after a minus sign */
            if (sign && json->_data[pos - 1] == '-') goto _error;

            radix = 1; sign = 0; value_expected = 1;
        } break;

        case OTHER: if (strict)
                    if (! IS_STRING(json) || c < 0x20) goto _error;

        default: if (unlikely(IS_STRING(json))) break;

            if (strict) {
                if (IS_TYPE(json, JSON_ARRAY | JSON_OBJECT)) {
                    if (! value_expected) {
                        debug("string_parse_json(): unexpected primitive.\n");
                        goto _error;
                    }
                } else if (IS_PRIMITIVE(json)) {
                    if (strict == _JSON_RELAX) break;

                    if (z & DIGIT_NUM) {
                        if (z & DIGIT_EXP) {
                            if (exp || (radix && json->_data[pos - 1] == '.'))
                                goto _error;
                            exp = 1; sign = 0; value_expected = 1;
                        } else {
                            if (! leading_digit)
                                leading_digit = c;
                            else if (leading_digit == '0' && ! radix && ! exp)
                                goto _error;
                            value_expected = 0;
                        }

                        /* optimize for large numbers */
                        prefetch = *(uint32_t *) (json->_data + pos + 1);
                        if (__more(prefetch, 0x39)) /* > '9' */
                            continue;
                        if (__less(prefetch, 0x30)) /* < '0' */
                            continue;
                        pos += MIN(4, SIZE(json) - pos);

                        break;
                    }
                }
            } else if (IS_PRIMITIVE(json)) break;

            p = (char *) CSTR(json) + pos;

            if (strict == JSON_STRICT) {
                switch (c) {
                case 'f': if (memcmp(p, "false", MIN(SIZE(json) - pos, 5)))
                                goto _error; p += 4; break;
                case 'n': if (memcmp(p, "null", MIN(SIZE(json) - pos, 4)))
                                goto _error; p += 3; break;
                case 't': if (memcmp(p, "true", MIN(SIZE(json) - pos, 4)))
                                goto _error; p += 3; break;
                default: if (z & DIGIT_NUM) leading_digit = c; else goto _error;
                }
            }

            value_expected = 0; radix = 0; exp = 0; sign = 0;

            if (unlikely(! (json = string_add_token(json, pos, SIZE(json)))))
                goto _nomem;

            json->_flags &= ~JSON_TYPE; json->_flags |= JSON_PRIMITIVE;
            if (unlikely(pos = (p - CSTR(json)))) goto _token;
        }

        continue;

_delim: i = 1;
_token: json->_len = json->_alloc = pos + (1 - i);
        json->_flags &= ~_STRING_FLAG_ERRORS;

        /* parser callback */
        if (ctx) {
            i = json->_flags & JSON_TYPE;
            if (ctx->exit && (i & (JSON_ARRAY | JSON_OBJECT))) {
                if (ctx->exit(i, ctx) == 1)
                    return 0;
                ctx->key.current = NULL;
                ctx->key.len = 0;
            } else if (ctx->data && ! key) {
                if (ctx->data(i, CSTR(json), SIZE(json), ctx) == 1)
                    return 0;
            }
        }

        i = 0;

        if (likely(json->parent)) {
            pos += json->_data - json->parent->_data;
            json = json->parent;
            key = (IS_OBJECT(json));
        }
    }

    return 0;

_error:
    debug("string_parse_json(): illegal character \'%c\' at %i.\n",
          c, (json->_data - s->_data) + pos + 1);
    string_free_token(s);
    return -1;

_nomem:
    s->_flags |= _STRING_FLAG_BUFFER;
    return 1;
}

/* -------------------------------------------------------------------------- */
#endif
/* -------------------------------------------------------------------------- */

static void __free_token(m_string *string)
{
    unsigned int i = 0;

    /* recursively clean the tokens' tokens, if any */
    for (i = 0; i < PARTS(string); i ++) {
        if (likely(string->token[i].token))
            __free_token(string->token + i);
    }

    free(string->token);
}

public void string_free_token(m_string *string)
{
    if (string && string->token) {
        __free_token(string);
        string->token = NULL;
        string->_parts_alloc = string->parts = 0;
    }
}

/* -------------------------------------------------------------------------- */

public void string_api_cleanup(void)
{
    return;
}

/* -------------------------------------------------------------------------- */
