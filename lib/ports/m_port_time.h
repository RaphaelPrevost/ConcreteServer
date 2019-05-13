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

#ifndef M_PORT_TIME_H

#define M_PORT_TIME_H

/* This code was released to public domain by Wu Yongwei. */

/* -------------------------------------------------------------------------- */
#ifdef WIN32
/* -------------------------------------------------------------------------- */

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <time.h>

#ifndef __GNUC__
#define EPOCHFILETIME (116444736000000000i64)
#else
#define EPOCHFILETIME (116444736000000000LL)
#endif

#ifdef _MSC_VER
#define _TIMEVAL_DEFINED
#endif

#ifndef _TIMEVAL_DEFINED
#define _TIMEVAL_DEFINED
struct timeval {
    long tv_sec;        /* seconds */
    long tv_usec;       /* microseconds */
};
#endif

#ifndef _TIMEZONE_DEFINED
#define _TIMEZONE_DEFINED
struct timezone {
    int tz_minuteswest; /* minutes W of Greenwich */
    int tz_dsttime;     /* type of dst correction */
};
#endif

#if (defined(_MSC_VER) || defined(__MINGW32__))
    #include <stdlib.h>
    #ifndef sleep
        #define sleep(t) _sleep((t) * 1000)
    #endif
#else
    #include <windows.h>
    #ifndef sleep
        #define sleep(t)  Sleep((t) * 1000)
    #endif
#endif

#ifndef usleep
    #define usleep(t) Sleep((t) / 1000)
#endif

public int gettimeofday(struct timeval *tv, struct timezone *tz);

/* -------------------------------------------------------------------------- */
#else
/* -------------------------------------------------------------------------- */

#include <sys/time.h>

/* -------------------------------------------------------------------------- */
#endif
/* -------------------------------------------------------------------------- */

public int utc_to_local(time_t *in, time_t *out);

/* -------------------------------------------------------------------------- */

public int local_to_utc(time_t *in, time_t *out);

/* -------------------------------------------------------------------------- */

#endif
