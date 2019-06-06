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

#include "m_socket.h"

static pthread_mutex_t _id_lock = PTHREAD_MUTEX_INITIALIZER;
static uint16_t _id = 1;
static m_socket_queue *_free_ids = NULL;

/**
 * @var _socket
 *
 * This private array holds all the allocated sockets. It is used for direct
 * access using dedicated functions, but it must NEVER be used without proper
 * locking at the array *and* socket levels.
 *
 */

static pthread_rwlock_t _socket_lock = PTHREAD_RWLOCK_INITIALIZER;
static m_socket *_socket[SOCKET_MAX];

/* hooks */
static void (*_socket_listen_hook)(m_socket *) = NULL;
static void (*_socket_accept_hook)(m_socket *) = NULL;
static void (*_socket_opened_hook)(m_socket *) = NULL;
static void (*_socket_closed_hook)(m_socket *) = NULL;

#ifdef _ENABLE_SSL

#ifndef _ENABLE_CONFIG
/* OpenSSL requires some static initialization */
static int _openssl_initialized = 0;
static SSL_CTX *_ssl_ctx;
static int _sc = 1;
#endif

/* SSL helpers */
#ifndef _ENABLE_CONFIG
static void _socket_ssl_init(void);
static int password_cb(char *buf, int len, int rwflag,void *userdata);
#endif
static void _socket_ssl_fini(void);
static m_socket *_socket_ssl_open(m_socket *s);
static ssize_t _socket_ssl_write(m_socket *s, const char *data, size_t len);
static ssize_t _socket_ssl_read(m_socket *s, char *out, size_t len);

#endif

/* -------------------------------------------------------------------------- */

public int socket_api_setup(void)
{
    #ifdef _ENABLE_SSL
    SSL_library_init();
    SSL_load_error_strings();
    OpenSSL_add_all_algorithms();
    atexit(_socket_ssl_fini);
    #endif
    _free_ids = socket_queue_alloc();
    return 0;
}

/* -------------------------------------------------------------------------- */

static struct addrinfo *socket_addr(int *flags, const char *ip, const char *pt)
{
    /* this helper builds a socket address using flags, an ip and a port */

    struct addrinfo hint, *res = NULL;
    int err = 0;

    if (! flags || (! ip && ! pt) ) return NULL;

    /* if no port was given, set it to 0 */
    if (! pt) {
        if (*flags & SOCKET_SERVER) return NULL;
        else pt = "0";
    }

    /* fill the hint structure for getaddrinfo() */
    memset(& hint, 0, sizeof(hint));
    hint.ai_family   = (*flags & SOCKET_IP6) ? AF_INET6 : AF_INET;
    hint.ai_flags    = AI_NUMERICHOST;
    hint.ai_socktype = (*flags & SOCKET_UDP) ? (SOCK_DGRAM) : (SOCK_STREAM);
    hint.ai_protocol = (*flags & SOCKET_UDP) ? (IPPROTO_UDP) : (IPPROTO_TCP);

    /* set AI_PASSIVE if this is a listening socket */
    if (*flags & SOCKET_SERVER) hint.ai_flags |= AI_PASSIVE;

    /* use getaddrinfo() to build the socket address */
    if ( (err = getaddrinfo(ip, pt, & hint, & res)) != 0)
        _gai_perror(ERR(socket_addr, getaddrinfo), err);

    return res;
}

/* -------------------------------------------------------------------------- */
#ifdef _ENABLE_SSL /* RESTRICTED SSL initialization/cleanup functions */
/* -------------------------------------------------------------------------- */

#ifndef _ENABLE_CONFIG
static int password_cb(char *buf, int len, unused int rwflag,
                       unused void *userdata                 )
{
    #ifndef _SSLPWD
    #define _SSLPWD "password"
    #endif

    if (len < (int) strlen(_SSLPWD) + 1) return 0;

    memcpy(buf, _SSLPWD, strlen(_SSLPWD) + 1);

    return(strlen(_SSLPWD));
}

/* -------------------------------------------------------------------------- */

static void _socket_ssl_init(void)
{
    #ifndef _SSLCRT
    #define _SSLCRT "ssl/client.pem"
    #endif
    #ifndef _SSLKEY
    #define _SSLKEY "ssl/client.pem"
    #endif
    #ifndef _SSLCA
    #define _SSLCA "ssl/root.pem"
    #endif

    SSL_METHOD *meth = NULL;

    if (_openssl_initialized) return;

    if (! (meth = SSLv23_method()) ) {
        sslerror(ERR(_socket_ssl_init, SSLv23_method));
        return;
    }

    if (! (_ssl_ctx = SSL_CTX_new(meth)) ) {
        sslerror(ERR(_socket_ssl_init, SSL_CTX_new));
        return;
    }

    if (! SSL_CTX_use_certificate_chain_file(_ssl_ctx, _SSLCRT)) {
        sslerror(ERR(_socket_ssl_init, SSL_CTX_use_certificate_chain_file));
        return;
    }

    SSL_CTX_set_default_passwd_cb(_ssl_ctx, password_cb);

    if (! SSL_CTX_use_PrivateKey_file(_ssl_ctx, _SSLKEY, SSL_FILETYPE_PEM)) {
        sslerror(ERR(_socket_ssl_init, SSL_CTX_use_PrivateKey_file));
        return;
    }

    if (! SSL_CTX_load_verify_locations(_ssl_ctx, _SSLCA, 0)) {
        sslerror(ERR(_socket_ssl_init, SSL_CTX_load_verify_locations));
        return;
    }

    #if (OPENSSL_VERSION_NUMBER < 0x00905100L)
    SSL_CTX_set_verify_depth(_ssl_ctx, 1);
    #endif 

    SSL_CTX_set_session_id_context(_ssl_ctx, (void *) & _sc, sizeof(_sc));

    _openssl_initialized = 1;
}
#endif

/* -------------------------------------------------------------------------- */

static void _socket_ssl_fini(void)
{
    #ifndef _ENABLE_CONFIG
    SSL_CTX_free(_ssl_ctx);
    #endif
    ERR_free_strings();
}

/* -------------------------------------------------------------------------- */

static m_socket *_socket_ssl_open(m_socket *s)
{
    SSL_CTX *ctx = NULL;
    BIO *bio = NULL;
    if (! s || ~s->_flags & SOCKET_SSL) return NULL;

    #ifdef _ENABLE_CONFIG
    /* fetch the SSL context from the configuration */
    ctx = config_get_ssl((s->_flags & _SOCKET_RSV) >> _SOCKET_RSS);
    #else
    ctx = _ssl_ctx;
    #endif

    if (! (s->_ssl = SSL_new(ctx)) ) {
        sslerror(ERR(_socket_ssl_open, SSL_new));
        return socket_close(s);
    }

    if (! (bio = BIO_new_socket(s->_fd, BIO_NOCLOSE)) ) {
        sslerror(ERR(_socket_ssl_open, BIO_new_socket));
        return socket_close(s);
    }

    SSL_set_bio(s->_ssl, bio, bio);

    return s;
}

/* -------------------------------------------------------------------------- */

static int _socket_ssl_connect(m_socket *s)
{
    int ret = 0;

    if (! s || ~s->_flags & SOCKET_SSL) return SOCKET_EPARAM;

    /* try to connect */
    ret = SSL_connect(s->_ssl);

    switch (SSL_get_error(s->_ssl, ret)) {
        /* all went fine */
        case SSL_ERROR_NONE:
            s->_state &= ~_SOCKET_C;
            return 0;
        /* recoverable errors */
        case SSL_ERROR_WANT_READ:
        case SSL_ERROR_WANT_WRITE:
            s->_state |= _SOCKET_C;
            return SOCKET_EAGAIN;
        /* link closed */
        case SSL_ERROR_SYSCALL:
            serror(ERR(_socket_ssl_connect, SSL_connect));
        case SSL_ERROR_ZERO_RETURN:
            return SOCKET_ECLOSE;
        default:
            sslerror(ERR(_socket_ssl_connect, SSL_connect));
            return SOCKET_EFATAL;
    }
}

/* -------------------------------------------------------------------------- */

static int _socket_ssl_accept(m_socket *s)
{
    int ret = 0;

    if (! s || ~s->_flags & SOCKET_SSL) return SOCKET_EPARAM;

    /* try to accept the connection */
    ret = SSL_accept(s->_ssl);

    switch (SSL_get_error(s->_ssl, ret)) {
        /* all went fine */
        case SSL_ERROR_NONE:
            s->_state &= ~_SOCKET_A;
            return 0;
        /* recoverable errors */
        case SSL_ERROR_WANT_READ:
        case SSL_ERROR_WANT_WRITE:
            s->_state |= _SOCKET_A;
            return SOCKET_EAGAIN;
        /* link closed */
        case SSL_ERROR_SYSCALL:
            if (errno) serror(ERR(_socket_ssl_accept, SSL_accept));
        case SSL_ERROR_ZERO_RETURN:
            return SOCKET_ECLOSE;
        default:
            sslerror(ERR(_socket_ssl_accept, SSL_accept));
            return SOCKET_EFATAL;
    }
}

/* -------------------------------------------------------------------------- */

static ssize_t _socket_ssl_write(m_socket *s, const char *data, size_t len)
{
    ssize_t ret = 0;

    /* check if there is a connect() or accept() in progress */
    if (s->_state & _SOCKET_C) {
        if ( (ret = _socket_ssl_connect(s)) != 0)
            return ret;
    } else if (s->_state & _SOCKET_A) {
        if ( (ret = _socket_ssl_accept(s)) != 0)
            return ret;
    }

    ret = SSL_write(s->_ssl, data, len);

    switch (SSL_get_error(s->_ssl, ret)) {
        /* all went fine */
        case SSL_ERROR_NONE:
            s->_tx += ret;
            return ret;
        /* recoverable errors */
        case SSL_ERROR_WANT_READ:
        case SSL_ERROR_WANT_WRITE:
            return SOCKET_EAGAIN;
        /* link closed */
        case SSL_ERROR_SYSCALL:
            if (errno) serror(ERR(_socket_ssl_write, SSL_write));
        case SSL_ERROR_ZERO_RETURN:
            return SOCKET_ECLOSE;
        default:
            sslerror(ERR(_socket_ssl_write, SSL_write));
            return SOCKET_EFATAL;
    }
}

/* -------------------------------------------------------------------------- */

static ssize_t _socket_ssl_read(m_socket *s, char *out, size_t len)
{
    ssize_t ret = 0;

    /* check if there is a connect() or accept() in progress */
    if (s->_state & _SOCKET_C) {
        if ( (ret = _socket_ssl_connect(s)) != 0)
            return ret;
    } else if (s->_state & _SOCKET_A) {
        if ( (ret = _socket_ssl_accept(s)) != 0)
            return ret;
    }

    ret = SSL_read(s->_ssl, out, len);

    switch (SSL_get_error(s->_ssl, ret)) {
        /* all went fine */
        case SSL_ERROR_NONE:
            s->_rx += ret;
            return ret;
        /* recoverable errors */
        case SSL_ERROR_WANT_READ:
        case SSL_ERROR_WANT_WRITE:
            return SOCKET_EAGAIN;
        /* link closed */
        case SSL_ERROR_SYSCALL:
            if (errno) serror(ERR(_socket_ssl_read, SSL_read));
        case SSL_ERROR_ZERO_RETURN:
            return SOCKET_ECLOSE;
        default:
            sslerror(ERR(_socket_ssl_read, SSL_read));
            return SOCKET_EFATAL;
    }
}

/* -------------------------------------------------------------------------- */
#endif
/* -------------------------------------------------------------------------- */

/* -------------------------------------------------------------------------- */
/* RESTRICTED, internal socket registration routines */
/* -------------------------------------------------------------------------- */

static int _socket_reg(m_socket *s, int type)
{
    uint16_t sockid = 0;

    if (! s) {
        debug("_socket_reg(): bad parameters.\n");
        return -1;
    }

    /* XXX avoid locking contention if all ids have already been allocated */
    if (_id < SOCKET_MAX) {
        /* try to allocate a new id */
        pthread_mutex_lock(& _id_lock);

            if (_id < SOCKET_MAX) sockid = _id ++;

        pthread_mutex_unlock(& _id_lock);
    }

    if (! sockid) {
        /* try to reuse an id */
        if (! (sockid = socket_queue_pop(_free_ids)) ) {
            debug("_socket_reg(): all ids are in use !\n");
            return -1;
        }
    }

    pthread_rwlock_wrlock(& _socket_lock);

        _socket[sockid] = s;

    pthread_rwlock_unlock(& _socket_lock);

    /* record id in the flags, keep options, ingress id and reserved bits */
    s->_flags = (type & (_SOCKET_OPT | _SOCKET_RSV | _SOCKET_IID)) |
                (sockid & _SOCKET_SID);

    return 0;
}

/* -------------------------------------------------------------------------- */

static m_socket *_socket_dereg(m_socket *s)
{
    m_socket *sock = NULL;

    if (! s) {
        debug("_socket_dereg(): bad parameters.\n");
        return NULL;
    }

    /* remove the socket from the array to prevent further access */
    pthread_rwlock_wrlock(& _socket_lock);

        if (_socket[SOCKET_ID(s)] == s) {
            /* check the socket and its id actually match */
            _socket[SOCKET_ID(s)] = NULL; sock = s;
        }

    pthread_rwlock_unlock(& _socket_lock);

    if (! sock) {
        /* this can happen when 2 threads compete to close a socket */
        debug("_socket_dereg(): sockets do not match.\n");
        return NULL;
    }

    /* HOOK */
    if (_socket_closed_hook) _socket_closed_hook(sock);

    /* release the id */
    socket_queue_push(_free_ids, SOCKET_ID(sock));

    return sock;
}

/* -------------------------------------------------------------------------- */
/* Private, regular socket locking primitives */
/* -------------------------------------------------------------------------- */

private int socket_lock(m_socket *s)
{
    int ret = 0;

    if (! s) return -1;

    pthread_mutex_lock(s->_lock);

        /* sleep until the lock is released */
        while (s->_lockstate == 0) pthread_cond_wait(s->_cond, s->_lock);

        /* woken up, check if the lockstate is valid */
        (s->_lockstate == -1) ? ret = -1 : s->_lockstate --;

    pthread_mutex_unlock(s->_lock);

    return ret;
}

/* -------------------------------------------------------------------------- */

private void socket_unlock(m_socket *s)
{
    if (! s) return;

    /* increment the lockstate if it is valid and signal the change */
    pthread_mutex_lock(s->_lock);

        if (s->_lockstate >= 0) s->_lockstate ++;

    pthread_mutex_unlock(s->_lock);

    pthread_cond_broadcast(s->_cond);
}

/* -------------------------------------------------------------------------- */

public int socket_exists(int id)
{
    /* the socket identifier should not exceed 15 bits */
    if (id < 1 || id >= SOCKET_MAX) {
        debug("socket_exists(): bad parameters.\n");
        return 0;
    }

    return (_socket[id]) ? 1 : 0;
}

/* -------------------------------------------------------------------------- */

public m_socket *socket_acquire(int id)
{
    m_socket *s = NULL;

    if (! id || id < 1 || id >= SOCKET_MAX) return NULL;

    pthread_rwlock_rdlock(& _socket_lock);

        if (socket_lock(_socket[id]) == 0) s = _socket[id];

    pthread_rwlock_unlock(& _socket_lock);

    return s;
}

/* -------------------------------------------------------------------------- */

private int socket_rhost(m_socket *s, char *h, size_t hl, char *srv, size_t sl)
{
    int err = 0;

    if (! s || ! h || ! hl || ! srv || ! sl) return -1;

    if ( (err = getnameinfo(s->info->ai_addr,
                            s->info->ai_addrlen,
                            h, hl, srv, sl,
                            NI_NUMERICHOST | NI_NUMERICSERV)) != 0)
        _gai_perror(ERR(socket_getip, getnameinfo), err);

    if (err) return SOCKET_EFATAL;

    return 0;
}

/* -------------------------------------------------------------------------- */

public int socket_ip(int id, char *host, size_t hostlen, uint16_t *port)
{
    m_socket *s = NULL;
    char serv[NI_MAXSERV];
    int err = 0;

    if (! id || ! host || ! hostlen || id < 1 || id >= SOCKET_MAX) {
        debug("socket_ip(): bad parameters.\n");
        return -1;
    }

    pthread_rwlock_rdlock(& _socket_lock);

        if ( (s = _socket[id]) )
            err = socket_rhost(s, host, hostlen, serv, sizeof(serv));
        else
            err = SOCKET_EFATAL;

    pthread_rwlock_unlock(& _socket_lock);

    if (port) *port = atoi(serv);

    return err;
}

/* -------------------------------------------------------------------------- */

public uint64_t socket_sentbytes(int id)
{
    m_socket *s = NULL;
    uint64_t ret = 0;

    if (! id || id < 1 || id >= SOCKET_MAX) {
        debug("socket_sentbytes(): bad parameters.\n");
        return -1;
    }

    pthread_rwlock_rdlock(& _socket_lock);

        if ( (s = _socket[id]) ) ret = s->_tx;

    pthread_rwlock_unlock(& _socket_lock);

    return ret;
}

/* -------------------------------------------------------------------------- */

public uint64_t socket_recvbytes(int id)
{
    m_socket *s = NULL;
    uint64_t ret = 0;

    if (! id || id < 1 || id >= SOCKET_MAX) {
        debug("socket_recvbytes(): bad parameters.\n");
        return -1;
    }

    pthread_rwlock_rdlock(& _socket_lock);

        if ( (s = _socket[id]) ) ret = s->_rx;

    pthread_rwlock_unlock(& _socket_lock);

    return ret;
}

/* -------------------------------------------------------------------------- */

public m_socket *socket_release(m_socket *s)
{
    socket_unlock(s);

    return NULL;
}

/* -------------------------------------------------------------------------- */
/* RESTRICTED socket lock breaking primitive */
/* -------------------------------------------------------------------------- */

static void _socket_break_lock(m_socket *s)
{
    pthread_mutex_lock(s->_lock);

        s->_lockstate = -1;

    pthread_mutex_unlock(s->_lock);

    pthread_cond_broadcast(s->_cond);
}

/* -------------------------------------------------------------------------- */
/* Private socket allocation code */
/* -------------------------------------------------------------------------- */

public m_socket *socket_open(const char *ip, const char *port, int type)
{
    m_socket *new = NULL;
    struct addrinfo *info = NULL;
    SOCKET sockfd = 0;
    /* XXX ioctl() param must be unsigned long for portability */
    unsigned long enabled = 1;
    int skip_fd = (type & SOCKET_NEW), blocking_io = (type & SOCKET_BIO);

    /* clean the flags: only keep the options, ingress id and reserved bits */
    type &= (_SOCKET_OPT | _SOCKET_IID | _SOCKET_RSV);

    /* try to get socket informations */
    if (! (info = socket_addr(& type, ip, port)) && ! skip_fd) return NULL;

    if (skip_fd || (! ip && ! port) ) goto _skip_fd;

    /* get the socket descriptor */
    sockfd = socket(info->ai_family, info->ai_socktype, info->ai_protocol);
    if (sockfd == INVALID_SOCKET) {
        /* XXX don't forget to use serror() for socket functions ! */
        serror(ERR(socket_open, socket)); goto _err_sock;
    }

    if (! blocking_io) {
        /* use non-blocking i/o (default) */
        if (ioctl(sockfd, FIONBIO, & enabled) == -1)
            serror(ERR(socket_open, ioctl));
    }

    /* enable keep alive for TCP sockets */
    #ifdef _ENABLE_UDP
    if (~type & SOCKET_UDP) {
    #endif
        if (setsockopt(sockfd,
                       SOL_SOCKET,
                       SO_KEEPALIVE,
                       (char *) & enabled,
                       sizeof(enabled)) == -1)
            serror(ERR(socket_open, setsockopt));
    #ifdef _ENABLE_UDP
    }
    #endif

    #if defined(_ENABLE_SSL) && ! defined(_ENABLE_CONFIG)
    if (type & SOCKET_SSL) {
        if (! _openssl_initialized) _socket_ssl_init();
    }
    #endif

_skip_fd:
    /* allocate the socket structure */
    if (! (new = malloc(sizeof(*new))) ) {
        perror(ERR(socket_open, malloc)); goto _err_alloc;
    }

    /* initialize the inner semaphore */
    if (! (new->_lock = malloc(sizeof(*new->_lock) + sizeof(*new->_cond))) ) {
        perror(ERR(socket_open, malloc)); goto _err_init;
    }

    if (pthread_mutex_init(new->_lock, NULL) == -1) {
        perror(ERR(socket_open, pthread_mutex_init)); goto _err_lock;
    }

    new->_cond = (void *) (((char *) new->_lock) + sizeof(*new->_lock));

    if (pthread_cond_init(new->_cond, NULL) == -1) {
        perror(ERR(socket_open, pthread_cond_init)); goto _err_cond;
    }

    /* initialize and register the struct */
    new->_lockstate = 1; new->_fd = sockfd; new->info = info;

    /* it is assumed the socket is writable by default */
    new->_state = _SOCKET_W;

    /* no callback by default */
    new->callback = NULL;

    /* no data transmitted yet */
    new->_tx = new->_rx = 0;

    /* try to register the socket */
    if (_socket_reg(new, type) == -1) goto _err_reg;

    #ifdef _ENABLE_SSL
    if (! skip_fd && (type & SOCKET_SSL) && (~type & SOCKET_SERVER) )
        return _socket_ssl_open(new);
    else
        new->_ssl = NULL;
    #endif

    return new;

_err_reg:
    pthread_cond_destroy(new->_cond);
_err_cond:
    pthread_mutex_destroy(new->_lock);
_err_lock:
    free(new->_lock);
_err_init:
    free(new);
_err_alloc:
    closesocket(sockfd);
_err_sock:
    freeaddrinfo(info);
    return NULL;
}

/* -------------------------------------------------------------------------- */

public int socket_connect(m_socket *s)
{
    int ret = 0;
    /* XXX ioctl() param must be unsigned long for portability */
    unsigned long enabled = 1;

    /* the socket must be a client */
    if (! s || s->_flags & SOCKET_SERVER) {
        debug("socket_connect(): bad parameters.\n");
        return SOCKET_EPARAM;
    }

    /* handle automatic reconnection */
    if (s->_flags & SOCKET_CLIENT && s->_fd == INVALID_SOCKET) {
        /* get a new socket descriptor */
        s->_fd = socket(s->info->ai_family, s->info->ai_socktype,
                        s->info->ai_protocol);
        if (s->_fd == INVALID_SOCKET) {
            serror(ERR(socket_open, socket));
            goto _err_connect;
        }

        /* use non-blocking i/o */
        if (ioctl(s->_fd, FIONBIO, & enabled) == -1)
            serror(ERR(socket_open, ioctl));
    }

    ret = connect(s->_fd, s->info->ai_addr, s->info->ai_addrlen);

    if (ret != 0) switch (ERRNO) {
        /* the socket is already connected */
        case EISCONN: break;

        /* non fatal */
        case EINTR:
        case EINPROGRESS:
        case EALREADY:
        case EWOULDBLOCK: goto _err_again;

        /* other error */
        default: goto _err_connect;
    }

    /* remove "connection in progress" flag and mark the socket as connected */
    s->_state &= ~_SOCKET_C; s->_state |= _SOCKET_O;

    #ifdef _ENABLE_SSL
    if (s->_flags & SOCKET_SSL && (ret = _socket_ssl_connect(s)) != 0)
        return ret;
    #endif

    /* HOOK */
    if (_socket_opened_hook) _socket_opened_hook(s);

    return 0;

_err_again:
        /* the server will handle the connection itself */
        s->_state |= _SOCKET_C;
        /* it is more efficient to poll before retrying */
        s->_state &= ~_SOCKET_W;
        return SOCKET_EAGAIN;
_err_connect:
        /* connection failed */
        serror(ERR(socket_connect, connect));
        return SOCKET_EFATAL;
}

/* -------------------------------------------------------------------------- */

public int socket_listen(m_socket *s)
{
    int r = 1;
    #ifdef _ENABLE_PRIVILEGE_SEPARATION
    struct sockaddr_in *a = NULL;
    #endif

    /* the socket must have the server flag */
    if (! s || ~s->_flags & SOCKET_SERVER) {
        debug("socket_listen(): bad parameters.\n");
        return SOCKET_EPARAM;
    }

    /* bind the socket if necessary */
    if (~s->_state & _SOCKET_B) {
        /* use SO_REUSEADDR to avoid binding problems */
        r = setsockopt(s->_fd,
                       SOL_SOCKET,
                       SO_REUSEADDR,
                       (char *) & r,
                       sizeof(r));
        if (r == -1) {
            serror(ERR(socket_listen, setsockopt));
            return SOCKET_EFATAL;
        }

        #ifdef _ENABLE_PRIVILEGE_SEPARATION
        /* check if privileges are required to bind the socket */
        a = (struct sockaddr_in *) s->info->ai_addr;
        if (ntohs(a->sin_port) <= 1024) {
            /* privileged bind */
            if ( (r = server_privileged_call(OP_BIND, s, sizeof(*s))) != 0)
                return SOCKET_EFATAL;
        } else {
            if (bind(s->_fd, s->info->ai_addr, s->info->ai_addrlen) == -1) {
                serror(ERR(socket_listen, bind));
                return SOCKET_EFATAL;
            }
        }
        #else
        if (bind(s->_fd, s->info->ai_addr, s->info->ai_addrlen) == -1) {
            serror(ERR(socket_listen, bind));
            return SOCKET_EFATAL;
        }
        #endif

        /* mark the socket as bound */
        s->_state |= _SOCKET_B;
    }

    #ifdef _ENABLE_UDP
    /* listen() only makes sense for TCP sockets,
       so make this call a no-op for UDP sockets */
    if (~s->_flags & SOCKET_UDP) {
    #endif
        if (listen(s->_fd, SOMAXCONN) == -1) {
            serror(ERR(socket_listen, listen));
            return SOCKET_EFATAL;
        }
    #ifdef _ENABLE_UDP
    }
    #endif

    /* HOOK */
    if (_socket_listen_hook) _socket_listen_hook(s);

    return 0;
}

/* -------------------------------------------------------------------------- */

public m_socket *socket_accept(m_socket *s)
{
    m_socket *new = NULL;
    struct sockaddr *remote = NULL;
    socklen_t rlen = sizeof(*remote);
    int ret = 0;

    if (! s || ~s->_state & _SOCKET_B || s->_flags & SOCKET_UDP) {
        debug("socket_accept(): bad parameters.\n");
        return NULL;
    }

    /* allocate the remote address buffer */
    if (! (remote = malloc(sizeof(*remote))) ) {
        perror(ERR(socket_accept, malloc));
        goto _err_alloc;
    }

    /* try to accept a connection */
    if ( (ret = accept(s->_fd, remote, & rlen)) == INVALID_SOCKET) {
        if (ERRNO != EAGAIN) perror(ERR(socket_accept, accept));
        goto _err_accpt;
    }

    /* allocate a clean socket structure to store the new connection */
    if (! (new = socket_open(NULL, NULL, SOCKET_NEW)) ) {
        closesocket(ret);
        goto _err_accpt;
    }

    /* inherit flags from the parent */
    new->_flags |= (s->_flags & ~SOCKET_SERVER) & _SOCKET_OPT;
    /* inherit the ingress id and the reserved bits */
    new->_flags |= s->_flags & (_SOCKET_RSV | _SOCKET_IID);
    /* use the hand crafted addrinfo structure */
    new->_state |= _SOCKET_I;

    if (! (new->info = malloc(sizeof(*new->info))) ) {
        perror(ERR(socket_accept, malloc));
        goto _err_sinfo;
    }

    /* XXX
       Ensure the proper family was recorded by accept(),
       because it will confuse getnameinfo() otherwise */
    remote->sa_family = (new->_flags & SOCKET_IP6) ? AF_INET6 : AF_INET;

    new->_fd = ret;
    new->info->ai_addr = remote;
    new->info->ai_addrlen = rlen;
    new->info->ai_next = NULL;

    #ifdef _ENABLE_SSL
    if ( (new->_flags & SOCKET_SSL) && (_socket_ssl_open(new)) ) {
        /* try to accept the connection */
        switch (_socket_ssl_accept(new)) {
            /* all went fine */
            case 0:
            case SOCKET_EAGAIN:
                /* success */
                break;
            default:
                return socket_close(new);
        }
    }
    #endif

    /* HOOK */
    if (_socket_accept_hook) _socket_accept_hook(new);

    return new;

_err_sinfo:
    socket_close(new);
_err_accpt:
    free(remote);
_err_alloc:
    return NULL;
}

/* -------------------------------------------------------------------------- */

public ssize_t socket_write(m_socket *s, const char *data, size_t len)
{
    ssize_t ret = 0;

    if (! s || ! data || ! len) {
        debug("socket_write(): bad parameters.\n");
        return SOCKET_EPARAM;
    }

    #ifdef _ENABLE_SSL
    if (s->_flags & SOCKET_SSL) return _socket_ssl_write(s, data, len);
    #endif

    /* check for a pending connection */
    if (s->_state & _SOCKET_C && ( (ret = socket_connect(s)) != 0) )
        return ret;

    #ifdef __APPLE__
    /* XXX on Mac OS X, attempting to call sendto on a UDP socket
           which is already connected will result in EISCONN. */
    if (s->_flags & SOCKET_UDP && s->_state & _SOCKET_O) {
        ret = send(s->_fd, data, len, 0x0);
    } else /* FALLTHRU */
    #endif

    ret = sendto(s->_fd,
                 data,
                 len,
                 0x0,
                 s->info->ai_addr,
                 s->info->ai_addrlen);

    if (ret == -1) {
        serror(ERR(socket_write, sendto));

        if (ERRNO == EINTR || ERRNO == EAGAIN || ERRNO == ESPIPE) {
            /* the socket is blocked, we need to poll before another attempt */
            s->_state &= ~_SOCKET_W;
            return SOCKET_EAGAIN;
        } else return SOCKET_EFATAL;

    } else if (ret == 0) return SOCKET_ECLOSE;

    s->_tx += ret;

    return ret;
}

/* -------------------------------------------------------------------------- */
#if defined(_ENABLE_TRIE) && defined(HAS_LIBXML)
/* -------------------------------------------------------------------------- */

public ssize_t socket_sendfile(m_socket *out, m_file *in, off_t *off, size_t len)
{
    ssize_t written = 0;
    off_t page = 0, current = 0;
    char *buffer;

    if (! out || ! in || ! off) {
        debug("socket_sendfile(): bad parameters.\n");
        return SOCKET_EFATAL;
    }

    #ifdef _ENABLE_SSL
    if (~out->_flags & SOCKET_SSL) {
    #endif
        if (in->fd != -1) {
            /* TODO check if there is a handler */

            /* try to call the native sendfile() syscall */
            written = socket_sendfile_lowlevel(out->_fd, in->fd, off, len);
            if (written <= 0 && ERRNO != ENOSYS) {
                if (ERRNO == EAGAIN || ERRNO == ENOMEM) {
                    /* the socket is blocked, need to poll before retrying */
                    out->_state &= ~_SOCKET_W;
                    return SOCKET_EAGAIN;
                } else return SOCKET_EFATAL;
            } else {
                out->_tx += written;
                return written;
            }
        } else if (in->data && (*off + len) <= SIZE(in->data)) {
            buffer = (char *) CSTR(in->data) + *off;
            written = socket_write(out, buffer, len);
            if (written > 0) { *off += written; out->_tx += written; }
            return written;
        }
        /* if there is no native sendfile() available, use the fallback below */
    #ifdef _ENABLE_SSL
    }
    #endif

    if (in->fd == -1 || out->_fd == -1) return SOCKET_EFATAL;

    /* sendfile() syscall is unavailable either because of the
       system or the network transport layer, so use a slower
       but more portable implementation */
    if (off) {
        current = *off % get_page_size();
        page = (*off / get_page_size()) * get_page_size();
    }

    buffer = mmap(NULL, len + current, PROT_READ, MAP_PRIVATE, in->fd, page);
    if (buffer == MAP_FAILED) {
        perror(ERR(socket_sendfile, mmap));
        return SOCKET_EFATAL;
    }

    written = socket_write(out, buffer + current, len);
    if (written > 0) { *off += written; out->_tx += written; }

    munmap(buffer, len + current);

    return written;
}

/* -------------------------------------------------------------------------- */
#endif
/* -------------------------------------------------------------------------- */

public ssize_t socket_read(m_socket *s, char *out, size_t len)
{
    ssize_t ret = 0;
    struct sockaddr *addr = NULL;
    socklen_t *addrlen = NULL;

    if (! s || ! out || ! len) return SOCKET_EPARAM;

    #ifdef _ENABLE_SSL
    if (s->_flags & SOCKET_SSL) return _socket_ssl_read(s, out, len);
    #endif

    /* check for a pending connection */
    if (s->_state & _SOCKET_C && ( (ret = socket_connect(s)) != 0) )
        return ret;

    #ifdef _ENABLE_UDP
    /* XXX
       the network address of the peer that sent the data is only
       retrieved when the socket is not connection-oriented;
       that would be fine for TCP sockets as well if the parameters
       where simply ignored, but they do get overwritten, so we have
       to provide these parameters *only* for UDP sockets.
    */
    if (s->_flags & SOCKET_UDP) {
        addr = s->info->ai_addr;
        addrlen = & s->info->ai_addrlen;
    }
    #endif

    ret = recvfrom(s->_fd, out, len, 0x0, addr, addrlen);

    if (ret == -1) {
        serror(ERR(socket_read, recvfrom));
        if (ERRNO == EINTR || ERRNO == EAGAIN || ERRNO == ESPIPE)
            return SOCKET_EAGAIN;
        else
            return SOCKET_EFATAL;
    } else if (ret == 0) return SOCKET_ECLOSE;

    s->_rx += ret;

    return ret;
}

/* -------------------------------------------------------------------------- */

private int socket_persist(m_socket *s)
{
    if (! s || ~s->_flags & SOCKET_CLIENT) {
        debug("socket_persist(): bad parameters.\n");
        return -1;
    }

    /* close the internal socket descriptor */
    closesocket(s->_fd);
    s->_fd = INVALID_SOCKET;

    /* prepare the socket for a reconnection attempt */
    s->_state |= (_SOCKET_E | _SOCKET_C);

    return 0;
}

/* -------------------------------------------------------------------------- */

public m_socket *socket_close(m_socket *s)
{
    m_socket *sock = NULL;

    if (! s) return NULL;

    if (! (sock = _socket_dereg(s)) ) return NULL;

    if (socket_lock(sock) == -1) {
        debug("socket_close(): socket lock is already broken.\n");
        return NULL;
    }

    /* free the threads waiting after the socket */
    _socket_break_lock(sock);

    /* the socket is now entirely ours - destroy it */
    if (sock->_state & _SOCKET_I) {
        /* freeaddrinfo() does not like our hand crafted struct */
        free(sock->info->ai_addr);
        free(sock->info);
    } else {
        freeaddrinfo(sock->info);
    }

    pthread_mutex_destroy(sock->_lock);
    pthread_cond_destroy(sock->_cond);
    free(sock->_lock);

    #ifdef _ENABLE_SSL
    if ( (sock->_flags & SOCKET_SSL) && (sock->_ssl) ) {
        SSL_shutdown(sock->_ssl);
        SSL_free(sock->_ssl);
    }
    #endif

    if (sock->_fd != -1) closesocket(sock->_fd); free(sock);

    return NULL;
}

/* -------------------------------------------------------------------------- */

private int socket_hook(int hook, void (*fn)(m_socket *s))
{
    if (! hook || ! fn) return -1;

    switch (hook) {
        case _HOOK_LISTEN: _socket_listen_hook = fn; break;
        case _HOOK_ACCEPT: _socket_accept_hook = fn; break;
        case _HOOK_OPENED: _socket_opened_hook = fn; break;
        case _HOOK_CLOSED: _socket_closed_hook = fn; break;
        default: return -1;
    }

    return 0;
}

/* -------------------------------------------------------------------------- */
/* Socket queues */
/* -------------------------------------------------------------------------- */

public m_socket_queue *socket_queue_alloc(void)
{
    pthread_condattr_t attr;

    m_socket_queue *ret = malloc(sizeof(*ret));

    if (! ret) { perror(ERR(socket_queue_alloc, malloc)); return NULL; }

    ret->_head_index = ret->_tail_index = 0;

    if (pthread_mutex_init(& ret->_head_lock, NULL) == -1) {
        perror(ERR(socket_queue_alloc, pthread_mutex_init));
        goto _err_head_lock;
    }

    if (pthread_mutex_init(& ret->_tail_lock, NULL) == -1) {
        perror(ERR(socket_queue_alloc, pthread_mutex_init));
        goto _err_tail_lock;
    }

    pthread_condattr_init(& attr);

    #if ! defined(WIN32) && ! defined(__APPLE__)
    /* use the monotonic clock on POSIX compliant systems */
    pthread_condattr_setclock(& attr, CLOCK_MONOTONIC);
    #endif

    if (pthread_cond_init(& ret->_empty, & attr) == -1) {
        perror(ERR(socket_queue_alloc, pthread_cond_init));
        goto _err_cond_init;
    }

    if (! (ret->_ring = calloc(SOCKET_MAX, sizeof(*ret->_ring))) ) {
        perror(ERR(socket_queue_alloc, malloc));
        goto _err_ring_alloc;
    }

    pthread_condattr_destroy(& attr);

    return ret;

_err_ring_alloc:
    pthread_cond_destroy(& ret->_empty);
_err_cond_init:
    pthread_condattr_destroy(& attr);
    pthread_mutex_destroy(& ret->_tail_lock);
_err_tail_lock:
    pthread_mutex_destroy(& ret->_head_lock);
_err_head_lock:
    free(ret);

    return NULL;
}

/* -------------------------------------------------------------------------- */

public m_socket_queue *socket_queue_free(m_socket_queue *q)
{
    if (! q) return NULL;

    pthread_mutex_destroy(& q->_head_lock);
    pthread_mutex_destroy(& q->_tail_lock);
    pthread_cond_destroy(& q->_empty);
    free(q->_ring); free(q);

    return NULL;
}

/* -------------------------------------------------------------------------- */

public int socket_queue_push(m_socket_queue *q, uint16_t id)
{
    if (! q || ! id) {
        debug("socket_queue_push(): bad parameters.\n");
        return -1;
    }

    pthread_mutex_lock(& q->_tail_lock);

        q->_ring[q->_tail_index ++] = id;
        if (q->_tail_index == SOCKET_MAX) q->_tail_index = 0;
        q->_ring[q->_tail_index] = 0;
        /* if a thread was waiting to pop an element, wake it up */
        pthread_cond_signal(& q->_empty);

    pthread_mutex_unlock(& q->_tail_lock);

    return 0;
}

/* -------------------------------------------------------------------------- */

public uint16_t socket_queue_pop(m_socket_queue *q)
{
    uint16_t ret = 0;

    if (! q) {
        debug("socket_queue_push(): bad parameters.\n");
        return -1;
    }

    pthread_mutex_lock(& q->_head_lock);

        if (q->_head_index == SOCKET_MAX) q->_head_index = 0;

        if ( (ret = q->_ring[q->_head_index]) )
            q->_ring[q->_head_index ++] = 0;

    pthread_mutex_unlock(& q->_head_lock);

    return ret;
}

/* -------------------------------------------------------------------------- */

public int socket_queue_empty(m_socket_queue *q)
{
    int ret = 1;

    if (! q) {
        debug("socket_queue_empty(): bad parameters.\n");
        return ret;
    }

    pthread_mutex_lock(& q->_head_lock);

        if (q->_ring[q->_head_index]) ret = 0;

    pthread_mutex_unlock(& q->_head_lock);

    return ret;
}

/* -------------------------------------------------------------------------- */

public void socket_queue_wait(m_socket_queue *q, unsigned int duration)
{
    struct timespec ts = { 0, 0 };

    if (! q || ! duration) return;

    #ifdef WIN32
    struct timeval tv;
    gettimeofday(& tv, NULL);
    ts.tv_sec = tv.tv_sec;
    ts.tv_nsec = tv.tv_usec * 1000;
    #elif ! defined(__APPLE__)
    clock_gettime(CLOCK_MONOTONIC, & ts);
    #endif

    ts.tv_sec += duration / 1000000;
    ts.tv_nsec += (duration % 1000000) * 1000;

    pthread_mutex_lock(& q->_tail_lock);

    if (socket_queue_empty(q)) {
        #ifdef __APPLE__
        pthread_cond_timedwait_relative_np(& q->_empty, & q->_tail_lock, & ts);
        #else
        pthread_cond_timedwait(& q->_empty, & q->_tail_lock, & ts);
        #endif
    }

    pthread_mutex_unlock(& q->_tail_lock);

    return;
}

/* -------------------------------------------------------------------------- */

private int socket_queue_poll(m_socket_queue *q, m_socket **s,
                              size_t len, int timeout)
{
    unsigned int i = 0, j = 0, n = 0;
    int ret = 0;
    #if ! defined(_USE_BIG_FDS) || ! defined(HAS_POLL) || defined(WIN32)
    fd_set r, w, e;
    int fdmax = 0;
    #else
    struct pollfd set[SOCKET_MAX];
    #endif

    if (! q || ! s || ! len) {
        debug("socket_queue_poll(): bad parameters.");
        return -1;
    }

    /* pry the queue open */
    pthread_mutex_lock(& q->_head_lock);
    pthread_mutex_lock(& q->_tail_lock);

    if (! q->_ring[q->_head_index]) goto _empty_queue;

    #if ! defined(_USE_BIG_FDS) || ! defined(HAS_POLL) || defined(WIN32)
    /* prepare to poll */
    FD_ZERO(& r); FD_ZERO(& w); FD_ZERO(& e);
    #endif

    for (i = 0, j = q->_head_index; i < len; i ++, j ++) {

        if (j == SOCKET_MAX) j = 0; if (j == q->_tail_index) break;

        if (! (s[i] = socket_acquire(q->_ring[j])) ) {
            i --; continue;
        }

        /* process outbound connections in progress */
        if (s[i]->_state & _SOCKET_C) {
            ret = socket_connect(s[i]);
            if (ret < 0 && ret != SOCKET_EAGAIN) {
                if (socket_persist(s[i]) == -1) {
                    /* destroy the socket */
                    debug("socket_queue_poll(): connection failed.\n");
                    socket_release(s[i]); s[i] = socket_close(s[i]); i --;
                }
                continue;
            }
        }

        /* clear the socket state, except for W */
        s[i]->_state &= ~(_SOCKET_E | _SOCKET_R);

        #if ! defined(_USE_BIG_FDS) || ! defined(HAS_POLL) || defined(WIN32)
        if (s[i]->_fd >= FD_SETSIZE) {
            fprintf(stderr, "socket_queue_poll(): WARNING: socket descriptor "
                    "number is higher than FD_SETSIZE.\n");
            socket_release(s[i]); s[i] = socket_close(s[i]); i --;
            continue;
        }

        FD_SET(s[i]->_fd, & r); FD_SET(s[i]->_fd, & e);
        if (~s[i]->_state & _SOCKET_W) FD_SET(s[i]->_fd, & w);
        fdmax = (fdmax < s[i]->_fd) ? s[i]->_fd : fdmax;
        #else
        set[i].fd = s[i]->_fd; set[i].events = POLLIN | POLLERR;
        if (~s[i]->_state & _SOCKET_W) set[i].events |= POLLOUT;
        #endif
    }

    /* no blocking sockets */
    if ( (ret = n = i) == 0) goto _empty_queue;

    /* poll */
    #if ! defined(_USE_BIG_FDS) || ! defined(HAS_POLL) || defined(WIN32)
    if (select(fdmax + 1, & r, & w, & e, NULL) == -1) {
        serror(ERR(socket_queue_poll, select));
        goto _err_poll;
    }
    #else
    if ( (ret = poll(set, n, timeout)) == -1) {
        serror(ERR(socket_queue_poll, poll));
        goto _err_poll;
    }
    #endif

    /* polling was successful, clear the queue */
    *q->_ring = 0; q->_head_index = q->_tail_index = 0;
    pthread_mutex_unlock(& q->_tail_lock);
    pthread_mutex_unlock(& q->_head_lock);

    /* update the sockets state */
    for (i = 0; i < n; i ++) {
        #if ! defined(_USE_BIG_FDS) || ! defined(HAS_POLL) || defined(WIN32)
        /* an error occured, close the socket */
        if (FD_ISSET(s[i]->_fd, & e)) s[i]->_state |= _SOCKET_E;
        if (FD_ISSET(s[i]->_fd, & r)) s[i]->_state |= _SOCKET_R;
        if (FD_ISSET(s[i]->_fd, & w)) s[i]->_state |= _SOCKET_W;
        #else
        if (set[i].revents & POLLERR) s[i]->_state |= _SOCKET_E;
        if (set[i].revents & POLLIN) s[i]->_state |= _SOCKET_R;
        if (set[i].revents & POLLOUT) s[i]->_state |= _SOCKET_W;
        #endif
    }

    return n;

_err_poll:
    /* XXX we must be very careful to unlock ALL the sockets here */
    while (n --) socket_release(s[n]); ret = -1;
_empty_queue:
    pthread_mutex_unlock(& q->_tail_lock);
    pthread_mutex_unlock(& q->_head_lock);

    return ret;
}

/* -------------------------------------------------------------------------- */

public void socket_api_cleanup(void)
{
    unsigned int i = 0;

    for (i = 0; i < SOCKET_MAX; i ++) if (_socket[i]) socket_close(_socket[i]);

    _free_ids = socket_queue_free(_free_ids);
}

/* -------------------------------------------------------------------------- */
