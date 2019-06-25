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

#ifndef M_SOCKET

#define M_SOCKET

#include "m_core_def.h"
#include "m_file.h"
#include "m_string.h"

#if defined(_ENABLE_SSL) && defined(_ENABLE_CONFIG)
#include "m_config.h"
#endif

/** @defgroup socket core::socket */

/* flags field structure (32 bits):
   [  ....  ....  .... ....  .... ....  ....  .... ]
     0           8         16         24         32
   (socket opt.)(rsvd)(ingr. id)(socket identifier)
   with:
   * socket opt.: socket options flags (protocol, status)
                  mask: 0xFF000000
   * rsvd       : reserved bits (used to store the plugin id in the server)
                  mask: 0x00F00000
   * ingr. id   : ingress identifier
                  mask: 0x000FF000
   * socket id  : socket identifier
                  mask: 0x00000FFF
*/

/* bitmasks */
#define _SOCKET_SID 0x00000FFF
#define _SOCKET_RSV 0x00F00000
#define _SOCKET_IID 0x000FF000
#define _SOCKET_OPT 0xFF000000

/* shift */
#define _SOCKET_RSS 20
#define _SOCKET_IIS 12

/* socket flags */
#define SOCKET_UDP    0x01000000    /* UDP socket */
#define SOCKET_IP6    0x02000000    /* IPv6 socket */
#define SOCKET_SSL    0x04000000    /* SSL socket */
#define SOCKET_BIO    0x00000001    /* blocking I/O */
#define SOCKET_NEW    0x00000002    /* empty socket structure */
#define SOCKET_CLIENT 0x10000000    /* persistent client */
#define SOCKET_SERVER 0x20000000    /* server socket */

/* Ingress identifier */
#define INGRESS_MAX 0xFF
#define INGRESS_ID(s) ((uint16_t) (((s)->_flags & _SOCKET_IID) >> _SOCKET_IIS))
/* set the ingress id */
#define INGRESS(i) (((i) << _SOCKET_IIS) & _SOCKET_IID)
/* set the listen flag and ingress id at the same time */
#define SOCKET_LISTEN(i) (SOCKET_SERVER | INGRESS(i))

/* Socket identifier */
#if (FD_SETSIZE < _SOCKET_SID)
#define SOCKET_MAX FD_SETSIZE
#else
#define SOCKET_MAX _SOCKET_SID    /* max number of sockets */
#endif
#define SOCKET_ID(s) ((uint16_t) ((s)->_flags & _SOCKET_SID))

/* reserved bits */
#define _RESERVED(s) (((s)->_flags & _SOCKET_RSV) >> _SOCKET_RSS)

/* socket status */
#define _SOCKET_B 0x0001    /* the socket is bound */
#define _SOCKET_A 0x0002    /* the socket is being accepted */
#define _SOCKET_C 0x0004    /* the socket is being connected */
#define _SOCKET_I 0x0008    /* inbound connection (accept()'ed socket) */
#define _SOCKET_O 0x0010    /* outbound connection (connect()'ed socket) */

#define _SOCKET_E 0x0020    /* and error occured with this socket */
#define _SOCKET_R 0x0040    /* the socket is readable */
#define _SOCKET_W 0x0080    /* the socket is writable */

#define SOCKET_HASERROR(s)  ((s)->_state & _SOCKET_E)
#define SOCKET_READABLE(s)  ((s)->_state & _SOCKET_R)
#define SOCKET_WRITABLE(s)  ((s)->_state & _SOCKET_W)
#define SOCKET_INCOMING     SOCKET_READABLE

/* hooks */
#define _HOOK_LISTEN  0x01
#define _HOOK_ACCEPT  0x02
#define _HOOK_OPENED  0x04
#define _HOOK_CLOSED  0x08

/* enable SOCKET_UDP only if _ENABLE_UDP is set */
#ifndef _ENABLE_UDP
    #undef SOCKET_UDP
    #define SOCKET_UDP 0x0
#endif

/* enable SOCKET_SSL only if _ENABLE_SSL is set */
#ifndef _ENABLE_SSL
    #undef SOCKET_SSL
    #define SOCKET_SSL 0x0
#endif

#ifdef _ENABLE_SSL
/* macro for printing SSL error messages */
#define sslerror(s) \
(fprintf(stderr, "%s: %s\n", (s), ERR_reason_error_string(ERR_get_error())))
#endif

/* disable the SOCKET_IP6 flag if IPv6 is not supported */
#ifndef PF_INET6
    #undef SOCKET_IP6
    #define SOCKET_IP6 0x0
    #define PF_INET6   0x0
#endif

/* errors */
#define SOCKET_EPARAM -1
#define SOCKET_EAGAIN -2
#define SOCKET_ECLOSE -3
#define SOCKET_EFATAL -4
#define SOCKET_EDELAY -5

/* socket receive buffer */
#define SOCKET_BUFFER 65536

typedef struct m_socket {
    /* private, socket flags and descriptor */
    uint32_t _flags;
    SOCKET _fd;

    /* private, locking */
    pthread_mutex_t *_lock;
    pthread_cond_t *_cond;
    int16_t _lockstate;

    /* private, internal state */
    uint16_t _state;

    /* private, how much data was transmitted over this socket? */
    uint64_t _tx;
    uint64_t _rx;

    /* public */
    struct addrinfo *info;
    void (*callback)(uint16_t, uint16_t, m_string *);

    #ifdef _ENABLE_SSL
    /* private, ssl */
    SSL *_ssl;
    #endif
} m_socket;

typedef struct _m_socket_queue {
    /* private */
    pthread_mutex_t _head_lock;
    pthread_mutex_t _tail_lock;
    pthread_cond_t _empty;

    uint16_t _head_index;
    uint16_t _tail_index;

    uint16_t *_ring;
} m_socket_queue;

/* -------------------------------------------------------------------------- */

public int socket_api_setup(void);

/**
 * @ingroup socket
 * @fn int socket_api_setup(void)
 * @param void
 * @return -1 if an error occurs, 0 otherwise
 *
 * This function must be called prior using any network related function.
 *
 */

/* -------------------------------------------------------------------------- */

public m_socket *socket_open(const char *ip, const char *port, int type);

/**
 * @ingroup socket
 * @fn m_socket *socket_open(const char *ip, const char *port, int type)
 * @param ip the host ip address; remote for a client, local for a server
 * @param port the port to connect or listen to
 * @param type the socket type (TCP/UDP/SSL, client/server...)
 * @return a new socket or NULL if something went wrong
 *
 * This functions creates a new socket and registers it. You will still need
 * to call socket_connect() or socket_listen() in order to use it.
 *
 * You may create different types of sockets:
 * - SOCKET_UDP: select the UDP transport layer (TCP by default)
 * - SOCKET_IP6: select IPv6 network layer (IPv4 by default)
 * - SOCKET_BIO: blocking i/o (non-blocking by default)
 * - SOCKET_SSL: secure socket layer
 * - SOCKET_SERVER creates a listener socket
 * - SOCKET_CLIENT creates a persistent connection
 *
 * SOCKET_IP6 and SOCKET_SSL may be disabled at compilation time and will
 * be ignored in that case.
 *
 * A socket opened with this function MUST be destroyed with socket_close().
 *
 * @see socket_close(), socket_connect(), socket_listen()
 *
 */

/* -------------------------------------------------------------------------- */

private int socket_rhost(m_socket *s, char *h, size_t hl, char *srv, size_t sl);

/**
 * @ingroup socket
 * @fn int socket_rhost(m_socket *s, char *h, size_t hl, char *srv, size_t sl)
 * @param s an acquired socket
 * @param h the buffer where the socket remote IP address will be written
 * @param hl size of the h buffer
 * @param srv the buffer where the socket remote port will be written
 * @param sl size of the srv buffer
 * @return -1 if an error occurs, 0 otherwise
 *
 * This function retrieves the IP address and port of the given socket
 * remote host.
 *
 */

/* -------------------------------------------------------------------------- */

public int socket_ip(int id, char *host, size_t hostlen, uint16_t *port);

/**
 * @ingroup socket
 * @fn int socket_ip(int id, char *host, size_t hostlen, uint16_t *port)
 * @param id a valid socket identifier
 * @param host the buffer where the socket IP address will be written
 * @param hostlen size of the buffer
 * @param port an optional pointer to an integer where to write the socket port
 * @return -1 if an error occurs, 0 otherwise
 *
 * This function retrieves the IP address of a socket and write it to the
 * given buffer. It is recommended the buffer is NI_MAXHOST long. If the
 * optional @ref port parameter is not NULL, the port corresponding to the
 * socket will be written in it too.
 *
 * @note This function does not lock the socket and can therefore be
 * safely called within a plugin.
 *
 */

/* -------------------------------------------------------------------------- */

public uint64_t socket_sentbytes(int id);

/**
 * @fn uint64_t socket_sentbytes(int id)
 * @param id a socket identifier
 * @return the number of bytes sent over this socket, or -1
 *
 * This function returns the number of bytes transmitted over this socket.
 *
 */

/* -------------------------------------------------------------------------- */

public uint64_t socket_recvbytes(int id);

/**
 * @fn uint64_t socket_recvbytes(int id)
 * @param id socket identifier
 * @return the number of bytes received over this socket, or -1
 *
 * This function returns the number of bytes received over this socket.
 *
 */

/* -------------------------------------------------------------------------- */

public int socket_exists(int id);

/**
 * @ingroup socket
 * @fn int socket_exists(int id)
 * @param id socket identifier
 * @return 1 if the socket exists, 0 otherwise
 *
 * This function checks if a socket is registered for the given ID.
 *
 */

/* -------------------------------------------------------------------------- */

public m_socket *socket_acquire(int id);

/**
 * @ingroup socket
 * @fn m_socket *socket_acquire(int id)
 * @param id socket identifier
 * @return a socket, or NULL if an error occured
 *
 * This function allows to acquire a socket from its identifier. The identifier
 * must be obtained with the SOCKET_ID() macro, or from a trusted source (all
 * public socket functions, basically).
 *
 * The returned socket MUST be unlocked after use with socket_release().
 *
 * @warning calling @ref socket_acquire() within plugin_main or a socket
 * callback using the current socket id will result in a deadlock.
 *
 * @see socket_release()
 *
 */

/* -------------------------------------------------------------------------- */

public m_socket *socket_release(m_socket *s);

/**
 * @ingroup socket
 * @fn m_socket *socket_release(m_socket *s)
 * @param s an acquired socket
 * @return NULL
 *
 * This function must be called after using a previously acquired socket.
 *
 */

/* -------------------------------------------------------------------------- */

private int socket_lock(m_socket *s);

/**
 * @ingroup socket
 * @fn int socket_lock(m_socket *s)
 * @param s the socket to lock
 * @return 0 if the socket was successfully locked, -1 otherwise
 *
 * This private function allows to lock a socket for private use; this
 * prevents other threads from using it.
 *
 * Locking the socket is a blocking operation, so it should be avoided
 * whenever possible.
 *
 * @Warning
 * Do not assume that you own the socket when this function returns; you
 * MUST check the return value to ensure the lock is effective.
 *
 * When a socket is on the verge of destruction, its lock is broken, which
 * cause all pending socket_lock() calls to return -1. If you foolishly
 * ignore this return value, you may very well end up using a dead socket.
 *
 * Once you are done with the socket, you MUST unlock it to allow other
 * threads to access it.
 *
 * @see socket_unlock()
 *
 */

/* -------------------------------------------------------------------------- */

private void socket_unlock(m_socket *s);

/**
 * @ingroup socket
 * @fn void socket_unlock(m_socket *s)
 * @param s the socket to unlock
 * @return void
 *
 * This function unconditionnaly unlock the socket, silently handling broken
 * locks. Once the socket has been unlocked, it will be accessible to other
 * threads back again.
 *
 * If you call this function more than once, as much socket_lock() calls
 * will succeed. This may be useful to implement complex locking schemes.
 *
 * @see socket_lock()
 *
 */

/* -------------------------------------------------------------------------- */

public int socket_connect(m_socket *s);

/**
 * @ingroup socket
 * @fn int socket_connect(m_socket *s)
 * @param s the socket to connect
 * @return 0 if the connection was successful, -1 otherwise
 *
 * This function connects the socket to the host and port with which it has
 * been initialized.
 *
 * If the socket is non blocking (default), it is not sufficient to call
 * this function for the connection to be established; the Mammouth server
 * monitors connections in progress and notify their master plugins.
 * So, if you call this function from a plugin, using non blocking i/o,
 * you should wait for it.
 *
 * If you use blocking i/o, the connection will be useable immediately
 * after the function returns 0.
 *
 */

/* -------------------------------------------------------------------------- */

public int socket_listen(m_socket *s);

/**
 * @ingroup socket
 * @fn int socket_listen(m_socket *s)
 * @param s the socket which should listen
 * @return 0 if all went fine, -1 otherwise
 *
 * This function binds the socket to the address with which it was initialized
 * and makes it listen.
 *
 * You will have to call socket_accept() on this socket to get the new
 * incoming connections.
 *
 * @see socket_accept()
 *
 */

/* -------------------------------------------------------------------------- */

public m_socket *socket_accept(m_socket *s);

/**
 * @ingroup socket
 * @fn m_socket *socket_accept(m_socket *s)
 * @param s a listening socket
 * @return a new socket if there is an incoming connection, or NULL
 *
 * This function accepts a new incoming connection if possible and returns it,
 * ready to use, as a new m_socket structure.
 *
 * You should destroy the connection with socket_close() when you no longer
 * want to use it.
 *
 */

/* -------------------------------------------------------------------------- */

public ssize_t socket_write(m_socket *s, const char *data, size_t len);

/**
 * @ingroup socket
 * @fn ssize_t socket_write(m_socket *s, const char *data, size_t len)
 * @param s the socket
 * @param data the data to send
 * @param len the lenght of the data
 * @return specific error codes, see below
 *
 * @note This is a private function, it should not be called from a plugin.
 *
 * This function does all the dirty work to send the data over the wire. It
 * may return several error codes:
 * > 0: the returned number of bytes were successfully written
 * SOCKET_ECLOSE: the connection is down
 * SOCKET_EAGAIN: recoverable error, retry the call
 * SOCKET_EFATAL: fatal error
 *
 */

/* -------------------------------------------------------------------------- */
#if defined(_ENABLE_TRIE) && defined(HAS_LIBXML)
/* -------------------------------------------------------------------------- */

public ssize_t socket_sendfile(m_socket *out, m_file *in, off_t *off, size_t len);

/* -------------------------------------------------------------------------- */
#endif
/* -------------------------------------------------------------------------- */

public ssize_t socket_read(m_socket *s, char *out, size_t len);

/**
 * @ingroup socket
 * @fn ssize_t socket_read(m_socket *s, char *out, size_t len)
 * @param s the socket
 * @param out the buffer to copy the data read in
 * @param len the size of the buffer
 * @return specific error code, see below
 *
 * @note This is a private function, it should not be called from a plugin.
 *
 * This function reads the incoming data from the socket and copy them
 * to the given buffer, up to its length.
 *
 * Like @ref socket_write(), this function returns various error codes:
 * > 0: the returned number of bytes were successfully read and copied to the
 *      buffer
 * SOCKET_ECLOSE: the connection is down
 * SOCKET_EAGAIN: recoverable error, retry the call
 * SOCKET_EFATAL: fatal error
 *
 */

/* -------------------------------------------------------------------------- */

private int socket_persist(m_socket *s);

/**
 * @ingroup socket
 * @fn void socket_persist(m_socket *s)
 * @param s the socket to destroy
 * @return 0 if all went fine, -1 otherwise
 *
 * This function gracefully shutdowns the connection but keep the associated
 * structure and prepare it for a reconnection attempt.
 *
 */

/* -------------------------------------------------------------------------- */

public m_socket *socket_close(m_socket *s);

/**
 * @ingroup socket
 * @fn m_socket *socket_close(m_socket *s)
 * @param s the socket to destroy
 * @return always NULL
 *
 * This function gracefully shutdowns the connection and destroys the associated
 * socket structure.
 *
 * You may clean up the pointer with this function, as it always return NULL.
 *
 */

/* -------------------------------------------------------------------------- */

private int socket_hook(int hook, void (*fn)(m_socket *s));

/**
 * @ingroup socket
 * @fn int socket_hook(int hook, void (*fn)(m_socket *s))
 * @param hook the call to hook
 * @param fn the callback
 * @return 0 if all went fine, -1 otherwise
 *
 * This function allows to hook a callback to some socket API functions. The
 * callback must accept a pointer to a m_socket structure, and not return
 * anything back.
 *
 * The callback will only be called if the hooked call is successfull.
 *
 * Currently, only socket_listen(), socket_connect() and socket_close()
 * are supported, with the _HOOK_LIS, _HOOK_CON and _HOOK_DES consts.
 *
 */

/* -------------------------------------------------------------------------- */

public m_socket_queue *socket_queue_alloc(void);

/**
 * @ingroup socket
 * @fn m_socket_queue *socket_queue_alloc(void)
 * @return a new socket queue, or NULL
 *
 * This function allocates and initializes a new socket queue. Socket queues
 * are simple ring buffers storing socket identifiers in FIFO order.
 *
 * Socket queues must be destroyed with @ref socket_queue_free()
 *
 * @see socket_queue_free()
 *
 */

/* -------------------------------------------------------------------------- */

public m_socket_queue *socket_queue_free(m_socket_queue *q);

/**
 * @ingroup socket
 * @fn m_socket_queue *socket_queue_free(m_socket_queue *q)
 * @param q a socket queue
 * @return always NULL
 *
 * This function destroys a socket queue.
 *
 */

/* -------------------------------------------------------------------------- */

public int socket_queue_add(m_socket_queue *q, uint16_t id);

/**
 * @ingroup socket
 * @fn int socket_queue_add(m_socket_queue *q, m_socket *s)
 * @param q a socket queue
 * @param id socket identifier
 * @return 0 if the socket identifier was properly queued, -1 otherwise
 *
 * This function enqueues the given socket identifier in FIFO order.
 * This identifier can be later retrieved using @ref socket_queue_get()
 *
 */

/* -------------------------------------------------------------------------- */

public uint16_t socket_queue_get(m_socket_queue *q);

/**
 * @ingroup socket
 * @fn uint16_t socket_queue_get(m_socket_queue *q)
 * @param q a socket queue
 * @return a socket identifier, or 0 if the queue is empty
 *
 * This function removes the first socket identifier from the queue and
 * returns it. If the queue is empty, this function returns 0.
 *
 */

/* -------------------------------------------------------------------------- */

public int socket_queue_empty(m_socket_queue *q);

/**
 * @ingroup socket
 * @fn int socket_queue_empty(m_socket_queue *q)
 * @param q a socket queue
 * @return 1 if the queue is empty, 0 otherwise
 *
 * This function checks if the queue is empty.
 *
 */

/* -------------------------------------------------------------------------- */

public void socket_queue_wait(m_socket_queue *q, unsigned int duration);

/**
 * @ingroup socket
 * @fn void socket_queue_wait(m_socket_queue *q, unsigned int microseconds)
 * @param q a socket queue
 * @param duration the time to wait for a socket to be queued, in microseconds
 * @return void
 *
 * This function waits up to @b duration us for a socket to be queued.
 *
 */

/* -------------------------------------------------------------------------- */

private int socket_queue_poll(m_socket_queue *q, m_socket **s,
                              size_t len, int timeout);

/**
 * @ingroup socket
 * @fn int socket_queue_poll(m_socket_queue *q, m_socket **s, size_t len,
 *                           int timeout)
 * @param q a socket queue
 * @param s an array of socket
 * @param len the length of the socket array
 * @param timeout a timeout in milliseconds
 * @return either the number of sockets successfully polled, or -1
 *
 * This function will get all the socket identifiers from the queue, acquire
 * the socket they reference if possible, and store them in the given socket
 * array. The sockets will then be polled and updated with their current
 * state informations. If the function is unable to poll the sockets, or some
 * parameters are incorrect, it will return -1. It will return the number of
 * sockets stored in the array otherwise.
 *
 * When this function is successful, the caller can then test the state of
 * each socket from the array with the macros @ref SOCKET_READABLE(),
 * @ref SOCKET_WRITABLE() or @ref SOCKET_HASERROR(). The sockets stored in
 * the array are locked, and MUST be released with @ref socket_release()
 *
 * This function will block for at most the timeout value provided, which
 * means it can return earlier.
 *
 */

/* -------------------------------------------------------------------------- */

public void socket_api_cleanup(void);

/**
 * @ingroup socket
 * @fn void socket_api_cleanup(void)
 * @param void
 * @return void
 *
 * This function unconditionnaly closes all currently allocated sockets. It
 * comes handy when you want to clean up all established connections before
 * the application shutdown.
 *
 */

/* -------------------------------------------------------------------------- */

#endif
