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

#include "m_port_std.h"

/* -------------------------------------------------------------------------- */
#ifdef WIN32 /* strtoll()/strtoull() compatibility module */
/* -------------------------------------------------------------------------- */

#ifndef _strtoi64
/* Code from cURL, copyright notice below: */

/*
 * COPYRIGHT AND PERMISSION NOTICE
 *
 * Copyright (c) 1996 - 2006, Daniel Stenberg, <daniel@haxx.se>.
 *
 * All rights reserved.
 *
 * Permission to use, copy, modify, and distribute this software for any purpose
 * with or without fee is hereby granted, provided that the above copyright
 * notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT OF THIRD PARTY RIGHTS. IN
 * NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
 * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
 * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE
 * OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * Except as contained in this notice, the name of a copyright holder shall not
 * be used in advertising or otherwise to promote the sale, use or other dealings
 * in this Software without prior written authorization of the copyright holder.
 *
 */

/**
 * Returns the value of c in the given base, or -1 if c cannot
 * be interpreted properly in that base (i.e., is out of range,
 * is a null, etc.).
 *
 * @param c     the character to interpret according to base
 * @param base  the base in which to interpret c
 *
 * @return  the value of c in base, or -1 if c isn't in range
 */

static int _getch(char c, int base)
{
    int value = -1;

    if (c <= '9' && c >= '0')
        value = c - '0';
    else if (c <= 'Z' && c >= 'A')
        value = c - 'A' + 10;
    else if (c <= 'z' && c >= 'a')
        value = c - 'a' + 10;

    if (value >= base) value = -1;

    return value;
}

/**
 * Emulated version of the strtoll function.  This extracts a long long
 * value from the given input string and returns it.
 */
int64_t strtoll(const char *nptr, char **endptr, int base)
{
    char *end;
    int is_negative = 0;
    int overflow;
    int i;
    int64_t value = 0;
    int64_t newval;

    /* skip leading whitespace. */
    end = (char *) nptr; while (isspace((int) end[0])) end++;

    /* handle the sign, if any. */
    if (end[0] == '-') {
        is_negative = 1; end++;
    } else if (end[0] == '+') {
        end++;
    } else if (end[0] == '\0') {
        /* nothing but perhaps some whitespace -- there was no number. */
        if (endptr) *endptr = end;
        return 0;
    }

    /* handle special beginnings, if present and allowed. */
    if (end[0] == '0' && end[1] == 'x')
        if (base == 16 || base == 0) { end += 2; base = 16; }
    else if (end[0] == '0')
        if (base == 8 || base == 0) { end++; base = 8; }

    /* matching strtol, if the base is 0 and it doesn't look like
     * the number is octal or hex, assume it's base 10.
     */
    if (base == 0) base = 10;

    /* loop handling digits. */
    value = 0; overflow = 0;

    for (i = _getch(end[0], base); i != -1; end++, i = _getch(end[0], base)) {
        newval = base * value + i;
        if (newval < value) {
            /* overflow */
            overflow = 1;
            break;
        } else value = newval;
    }

    if (!overflow) {
        /* fix the sign */
        if (is_negative) value *= -1;
    } else {
        value = (is_negative) ? LLONG_MIN : LLONG_MAX;
        errno = ERANGE;
    }

    if (endptr) *endptr = end;

    return value;
}
#endif

/* -------------------------------------------------------------------------- */
#endif
/* -------------------------------------------------------------------------- */
