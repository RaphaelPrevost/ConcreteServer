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

#ifndef M_PORT_MMAP_H

#define M_PORT_MMAP_H

/* -------------------------------------------------------------------------- */
#ifdef WIN32 /* WIN32 mmap() compatibility module */
/* -------------------------------------------------------------------------- */

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <errno.h>

#ifndef public
    #ifdef BUILDING_DLL
        #define public __declspec(dllexport)
    #else
        #define public __declspec(dllimport)
    #endif
#endif
#ifndef private
    #define private
#endif

#ifndef unused
    #ifdef __GNUC__
        #define unused __attribute__ ((unused))
    #else
        #define unused
    #endif
#endif

#ifndef off_t
    #ifdef _off_t
        #define off_t _off_t
    #else
        #define off_t long
    #endif
#endif

/* standard mmap() definitions - shamelessly ripped off bits/mman.h */

#define MAP_FAILED ((void *) -1)

/* Protections are chosen from these bits, OR'd together.  The
   implementation does not necessarily support PROT_EXEC or PROT_WRITE
   without PROT_READ.  The only guarantees are that no writing will be
   allowed without PROT_WRITE and no access will be allowed for PROT_NONE. */
#define PROT_READ       0x1             /* Page can be read.  */
#define PROT_WRITE      0x2             /* Page can be written.  */
#define PROT_EXEC       0x4             /* Page can be executed.  */
#define PROT_NONE       0x0             /* Page can not be accessed.  */
/* Sharing types (must choose one and only one of these).  */
#define MAP_SHARED      0x01            /* Share changes.  */
#define MAP_PRIVATE     0x02            /* Changes are private.  */
/* Other flags.  */
#define MAP_FIXED       0x10            /* Interpret addr exactly.  */
#define MAP_ANONYMOUS   0x20            /* Don't use a file.  */
#define MAP_ANON        MAP_ANONYMOUS

#ifndef FILE_MAP_EXECUTE
    #define FILE_MAP_EXECUTE 0x0
#endif

public int posix_memalign(void **p, size_t alignment, size_t size);

public void *mmap(void *start, size_t len, int prot, int flags, int fd,
                  off_t offset                                         );
public int munmap(void *start, unused size_t _dummy);

/* -------------------------------------------------------------------------- */

/* -------------------------------------------------------------------------- */
#else /* Standard mmap() on SVID compliant systems */
/* -------------------------------------------------------------------------- */

#define _SVID_SOURCE 1
#include <sys/types.h>
#include <sys/mman.h>
#include <sys/stat.h> /* S_IRUSR etc... */
#include <fcntl.h> /* O_* consts in shm_open(2) */

#ifndef MAP_ANON
#define MAP_ANON MAP_ANONYMOUS
#endif

#if ! defined(_SC_PAGESIZE) && defined(_SC_PAGE_SIZE)
#define _SC_PAGESIZE _SC_PAGE_SIZE
#endif

/* -------------------------------------------------------------------------- */
#endif
/* -------------------------------------------------------------------------- */

/* common definitions */
public int get_page_size(void);
public void posix_memfree(void *memblock);
public void *shm_alloc(const char *name, size_t size);
public void *shm_attach(const char *name, size_t size);
public void shm_detach(void *start, size_t size);
public void shm_free(const char *name, void *start, size_t size);

#endif
