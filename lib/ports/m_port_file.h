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

#ifndef M_PORT_FILE_H

#define M_PORT_FILE_H

/* -------------------------------------------------------------------------- */
#ifdef WIN32 /* WIN32 file I/O compatibility module */
/* -------------------------------------------------------------------------- */

#ifndef stat
    #define stat _stat
#endif

#ifndef fstat
    #define fstat _fstat
#endif

/* open */
#include <io.h>
#include <fcntl.h>
#include <share.h>
#include <direct.h>
#include <sys/stat.h>

#ifndef O_RDONLY
    #define O_RDONLY _O_RDONLY
#endif

#ifndef O_WRONLY
    #define O_WRONLY _O_WRONLY
#endif

#ifndef O_RDWR
    #define O_RDWR _O_RDWR
#endif

#ifndef O_CREAT
    #define O_CREAT _O_CREAT
#endif

#ifndef O_APPEND
    #define O_APPEND _O_APPEND
#endif

#ifndef O_EXCL
    #define O_EXCL _O_EXCL
#endif

#ifndef O_TRUNC
    #define O_TRUNC _O_TRUNC
#endif

/* user permissions */
#ifndef S_IRWXU
    #define S_IRWXU (_S_IREAD | _S_IWRITE)
#endif
#ifndef S_IRUSR
    #define S_IRUSR _S_IREAD
#endif
#ifndef S_IWUSR
    #define S_IWUSR _S_IWRITE
#endif
#ifndef S_IXUSR
    #define S_IXUSR 0
#endif
/* group */
#ifndef S_IRWXG
    #define S_IRWXG (_S_IREAD | _S_IWRITE)
#endif
#ifndef S_IRGRP
    #define S_IRGRP _S_IREAD
#endif
#ifndef S_IWGRP
    #define S_IWGRP _S_IWRITE
#endif
#ifndef S_IXGRP
    #define S_IXGRP 0
#endif
/* others */
#ifndef S_IRWXO
    #define S_IRWXO (_S_IREAD | _S_IWRITE)
#endif
#ifndef S_IROTH
    #define S_IROTH _S_IREAD
#endif
#ifndef S_IWOTH
    #define S_IWOTH _S_IWRITE
#endif
#ifndef S_IXOTH
    #define S_IXOTH 0
#endif

/* access(2) */
#ifndef access
    #define access _access
#endif

#ifndef F_OK
    #define F_OK 0x0
#endif

#ifndef R_OK
    #define R_OK 0x4
#endif

#ifndef W_OK
    #define W_OK 0x2
#endif

/* X_OK is not portable, fallback to existence */
#ifndef X_OK
    #define X_OK F_OK
#endif

/* unlink(2) */
#ifndef unlink
    #define unlink _unlink
#endif

/* utime(2) */
#include <sys/utime.h>

#ifndef utime
    #define utime _utime
    #define utimbuf _utimbuf
#endif

/* mkdir(2) */
#ifndef mkdir
    #define mkdir(p, m) _mkdir(p)
#endif

/* open(2) */
#undef open
#define open posix_open

public int posix_open(const char *pathname, int flags, ...);

public ssize_t file_getxattr(const char *file, const char *attrname,
                             void *attrbuf, size_t len);

public int file_setxattr(const char *file, const char *attrname,
                         const void *buf, size_t len, int flags);

/* -------------------------------------------------------------------------- */
#else
/* -------------------------------------------------------------------------- */

#include <fcntl.h>
#include <utime.h>
#include <sys/stat.h>
#include <sys/xattr.h>

#ifdef __APPLE__
public ssize_t file_getxattr(const char *file, const char *attrname,
                             void *attrbuf, size_t len);

public int file_setxattr(const char *file, const char *attrname,
                         const void *buf, size_t len, int flags);
#else
#define file_getxattr getxattr
#define file_setxattr setxattr
#endif

/* -------------------------------------------------------------------------- */
#endif
/* -------------------------------------------------------------------------- */

#endif
