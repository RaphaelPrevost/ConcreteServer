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

#ifndef M_PORT_SOCKET_H

#define M_PORT_SOCKET_H

/* -------------------------------------------------------------------------- */
#ifdef WIN32 /* WIN32 Winsock2 compatibility module */
/* -------------------------------------------------------------------------- */

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdio.h>
#include <errno.h>
#include <winsock2.h>
#undef socklen_t
#include <ws2tcpip.h>
#include <mswsock.h>
#include <io.h>
#if defined(_MSC_VER) && (_MSC_VER < 1300)
#include <process.h>
#endif

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

#ifndef off_t
    #ifdef _off_t
        #define off_t _off_t
    #else
        #define off_t long
    #endif
#endif

#ifndef socklen_t
    #define socklen_t size_t
#endif

#ifndef INVALID_FILE_HANDLE
    #define INVALID_FILE_HANDLE ((HANDLE)INVALID_HANDLE_VALUE)
#endif

#ifndef ERR
    #ifndef STRINGIFY
        #define STRINGIFY(x)  #x
    #endif
    #ifndef STR
        #define STR(x)  STRINGIFY(x)
    #endif
    #define ERR(c, f) #c "()::" #f  "() @ " __FILE__ ":" STR(__LINE__)
#endif

/* map the WIN32 API functions to their BSD counterparts */
#define ioctl(s, i, l) ioctlsocket((s), (i), (l))

/* map useful Winsock2 error codes to the standard BSD constants */
#undef EINTR
#define EINTR WSAEINTR
#undef EWOULDBLOCK
#define EWOULDBLOCK WSAEWOULDBLOCK
#undef EAGAIN
#define EAGAIN EWOULDBLOCK
#undef EINPROGRESS
#define EINPROGRESS WSAEINPROGRESS
#undef EALREADY
#define EALREADY WSAEALREADY
#undef ESPIPE
#define ESPIPE EWOULDBLOCK
#undef EISCONN
#define EISCONN WSAEISCONN

/* Winsock2 does not use errno - work around with some macros */
#define ERRNO ( (errno = WSAGetLastError()) )
#define serror(s) (fprintf(stderr, "%s: %s\n", (s), _socket_win32_strerror()))

#if defined(_MSC_VER)
    /* include the needed libs */
    #pragma comment      ( lib, "ws2_32.lib" )
    #pragma comment      ( lib, "pthreadVC2.lib" )
    #if (_MSC_VER < 1300)
        /* Microsoft Visual C++ 6 does not support POSIX networking functions */
        #define _POSIX_EMULATION
    #endif
#elif defined(__GNUC__)
    /* for some reasons, gai_strerror() does not link in Dev-C++ */
    #undef gai_strerror
    #define gai_strerror(i) ("unknown error")
#endif

#if defined(_WIN32_WINNT) && _WIN32_WINNT < 0x0501
    /* Windows 2k ws2_32.dll does not provide the POSIX networking functions */
    #ifndef _POSIX_EMULATION
        #define _POSIX_EMULATION
    #endif
#endif

/* -------------------------------------------------------------------------- */

public int socketpair(UNUSED int d, UNUSED int t, UNUSED int p, int sv[2]);

/* -------------------------------------------------------------------------- */

public SOCKET dupsocket(SOCKET s);

/* -------------------------------------------------------------------------- */

private const char *_socket_win32_strerror(void);

/* -------------------------------------------------------------------------- */
#else /* *NIX compatibility module */
/* -------------------------------------------------------------------------- */

#include <netdb.h>
#include <arpa/inet.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/select.h>

#if defined(_USE_BIG_FDS) && defined(HAS_POLL)
#include <poll.h>
#endif

#define serror perror
#define ERRNO errno
#define SOCKET int
#define closesocket(s) close((s))
#define dupsocket(s) dup((s))
#define INVALID_SOCKET -1

#if ! defined(TCP_CORK) && defined(TCP_NOPUSH)
    #define TCP_CORK TCP_NOPUSH
#endif

/* sendfile */
#if defined(__linux__)
    #include <sys/sendfile.h>
#else
    #if defined(__FreeBSD__)
        #include <sys/types.h>
        #include <sys/socket.h>
    #elif defined(__sun)
        #include <sys/sendfile.h>
    #endif
#endif

/* -------------------------------------------------------------------------- */
#endif
/* -------------------------------------------------------------------------- */

private size_t socket_sendfile_lowlevel(int out, int in, off_t *off, size_t len);

/* -------------------------------------------------------------------------- */
#ifdef _POSIX_EMULATION
/* -------------------------------------------------------------------------- */

/*
  This is a small compatibility layer provided for OS or libc which do not
  implement the POSIX protocol independant network functions. The assumption is
  made that such an OS/libc probably does not either properly implement
  IPv6, so only the classic BSD IPv4 API is used for a better portability.
*/

#if (defined(_MSC_VER) && (_MSC_VER < 1300))
/* define the addrinfo structure */
struct addrinfo {
    int     ai_flags;
    int     ai_family;
    int     ai_socktype;
    int     ai_protocol;
    socklen_t  ai_addrlen;
    struct sockaddr *ai_addr;
    char   *ai_canonname;
    struct addrinfo *ai_next;
};

/* use the classic sockaddr_in */
#define sockaddr_storage sockaddr_in

/* netdb.h definitions */
#define AI_PASSIVE     0x0001  /* Socket address is intended for `bind'.  */
#define AI_NUMERICHOST 0x0004  /* Don't use name resolution.  */
#define EAI_NONAME       -2    /* NAME or SERVICE is unknown.  */
#define EAI_FAIL         -4    /* Non-recoverable failure in name res.  */
#define EAI_FAMILY       -6    /* `ai_family' not supported.  */
#define EAI_MEMORY       -10   /* Memory allocation failure.  */
#define NI_NUMERICHOST 1       /* Don't try to look up hostname.  */
#define NI_NUMERICSERV 2       /* Don't convert port number to name.  */

/* default size for the host and service buffers */
#define NI_MAXHOST      1025
#define NI_MAXSERV      32
#endif

/* use the minimalist implementation of the POSIX functions */
#undef getaddrinfo
#undef getnameinfo
#undef freeaddrinfo
#define getaddrinfo  _getaddrinfo
#define getnameinfo  _getnameinfo
#define freeaddrinfo _freeaddrinfo

/* remove gai_strerror */
#undef gai_strerror
#define gai_strerror(i) "unknown error."

/* -------------------------------------------------------------------------- */

public int getaddrinfo(const char *node, const char *service,
                       const struct addrinfo *hints, struct addrinfo **res);

public int getnameinfo(const struct sockaddr *s, int salen, char *host,
                       size_t hostlen, char *serv, size_t servlen, int flags);

public void freeaddrinfo(struct addrinfo *res);

/* -------------------------------------------------------------------------- */
#endif
/* -------------------------------------------------------------------------- */

/* a small private macro for easily printing errors */
#define _gai_perror(s, i) (fprintf(stderr, "%s: %s\n", (s), gai_strerror((i))))

public int socket_sendfd(int sock, int fd);
public int socket_recvfd(int sock);

#endif
