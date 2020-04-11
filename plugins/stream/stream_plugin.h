/*******************************************************************************
 *  Concrete Server                                                            *
 *  Copyright (c) 2005-2020 Raphael Prevost <raph@el.bzh>                      *
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

#ifndef STREAM_PLUGIN_H

#define STREAM_PLUGIN_H

/* mandatory server API declarations */
#include "m_server.h"
#include "m_plugin.h"

/* optional, useful parts of the server public API */
#include "m_socket.h"

#define STREAM_VERSION     CONCRETE_VERSION

/* Concrete supports up to 256 ingress id, which means a maximum of 128 streams */
#define _STREAMS_MAX       128

#define PERSONALITY_MASTER 0x01
#define PERSONALITY_WORKER 0x02
#define PERSONALITY_HYBRID (PERSONALITY_MASTER | PERSONALITY_WORKER)

#define DESTINATION_SERVER 0x01
#define DESTINATION_PLUGIN 0x02

#define MASTER_ADDRESS     0x01
#define SERVER_ADDRESS     0x02
#define STREAM_INGRESS     0x04
#define STREAM_WORKERS     0x08

#define ROUTE_PUBLIC       0
#define ROUTE_WORKER       1
#define ROUTE_MASTER       2
#define ROUTE_SERVER       3
#define ROUTE_MAX          4

#define STREAM_STATUS_DOWN 0x00
#define STREAM_STATUS_CONN 0x01
#define STREAM_STATUS_WORK 0x02
#define STREAM_STATUS_WAIT 0x04
#define STREAM_STATUS_PIPE 0x10

#define WORKER_OP_HELLO    0x31108055
#define WORKER_OP_READY    0x1E75D017
#define WORKER_OP_ALIVE    0xFEE1600D
#define MASTER_OP_HIRED    0xA600D10B

/* -------------------------------------------------------------------------- */
/* MANDATORY PLUGIN CALLBACKS */
/* -------------------------------------------------------------------------- */

public unsigned int plugin_api(void);

/**
 * @ingroup plugin
 * @fn unsigned int plugin_api(void)
 * @return the required Concrete API revision number
 *
 * This function is called first while loading the plugin, it must return the
 * minimal API revision number that the plugin requires to function properly.
 * If the server API revision is lower, the plugin will not be loaded.
 *
 */

/* -------------------------------------------------------------------------- */

public int plugin_init(uint32_t id, int argc, char **argv);

/**
 * @ingroup plugin
 * @fn int plugin_init(uint32_t id, int argc, char **argv)
 * @param id the plugin identifier, required for some server services
 * @param argc number of string parameters
 * @param argv string parameters
 * @return -1 if an error occured, 0 otherwise
 *
 * This function is called by the server only once, when the plugin is loaded;
 * the @ref id parameter is the server-side identifier for the plugin and
 * should be kept if the plugin needs to call the server API later.
 *
 * In this function, the plugin has the opportunity to perform all the required
 * initialization work, like setting up database connections, loading data, ...
 *
 * The provided argc and argv parameters work the same as in the main()
 * function from the C standard, and should be processed in the same way.
 *
 * If the initialization fails, this function must return -1 to stop
 * the plugin loading process, 0 otherwise.
 *
 */

/* -------------------------------------------------------------------------- */

private uint32_t plugin_get_token(void);

/* -------------------------------------------------------------------------- */

public void plugin_main(uint16_t socket_id, uint16_t ingress_id, m_string *data);

/**
 * @ingroup plugin
 * @fn void plugin_main(uint16_t socket_id, uint16_t ingress_id, m_string *data)
 * @param socket_id connection identifier
 * @param ingress_id a channel identifier
 * @param data incoming data
 *
 * This function is called by the server when data become available on a
 * connection.
 *
 * The incoming data are encapsulated in a @ref m_string structure, so the
 * plugin can fetch the data it needs and the server can handle request
 * fragmentation. If the plugin leaves data in the buffer, they will be kept
 * by the server and subsequent incoming data will be automatically appended.
 *
 * If you don't want the server to keep these remaining data, you can discard
 * them by calling @ref string_flush() on the @ref m_string.
 *
 * This method can use the server functions @ref server_send_string() or
 * @ref server_send_response() to send a reply.
 *
 */

/* -------------------------------------------------------------------------- */

public void plugin_fini(void);

/**
 * @ingroup plugin
 * @fn void plugin_fini(void)
 *
 * This function is called by the server when unloading the plugin. It gives
 * the opportunity to clean up all the resources allocated by the plugin, like
 * internal data, database connections...
 *
 */

/* -------------------------------------------------------------------------- */
/* OPTIONAL PLUGIN CALLBACKS */
/* -------------------------------------------------------------------------- */

public void plugin_intr(uint16_t id, uint16_t ingress_id, int event,
                        void *event_data);

/**
 * @ingroup plugin
 * @fn void plugin_intr(uint16_t id, uint16_t ingress_id, int event)
 * @param id connection identifier
 * @param ingress_id ingress identifier
 * @param event event code
 * @param event_data pointer to event-specific data
 *
 * This function is called by the server whenever an event occurs on
 * a connection; e.g. the connection has just been established, broken,
 * or a response has been successfully sent.
 *
 * This method can use the server functions @ref server_send_string() or
 * @ref server_send_response() to respond to the received event.
 *
 */

/* -------------------------------------------------------------------------- */
/* Configuration */
/* -------------------------------------------------------------------------- */

private int stream_config_init(int argc, char **argv);
private int stream_personality(void);
private int stream_master_streams(void);
private int stream_worker_streams(void);
private const char *stream_config_host(int stream, int target);
private const char *stream_config_port(int stream, int target);
private int stream_config_destination(int stream);
private const char *stream_config_plugin_name(int stream);
private void stream_config_fini(void);

/* -------------------------------------------------------------------------- */
/* Router */
/* -------------------------------------------------------------------------- */

private int stream_router_init(void);
private int stream_heartbeat(int socket_id);
private int stream_get_id(int hint, int personality);
private void stream_set_route(int type, uint16_t s0, uint16_t s1);
private int stream_get_route(int ingress);
private int stream_open_ingress(int stream_id, int type);
private int stream_get_ingress(int stream_id, int type);
private uint16_t stream_get_egress(uint16_t socket_id, uint16_t ingress_id);
private void stream_router_fini(void);

/* -------------------------------------------------------------------------- */
/* Sockets */
/* -------------------------------------------------------------------------- */

private int stream_socket_init(void);
private int stream_set_status(uint16_t socket_id, int status);
private int stream_get_status(uint16_t socket_id);
private void stream_add_worker(int stream_id, uint16_t worker);
private uint16_t stream_borrow_worker(int stream_id);
private uint16_t stream_release_worker(int stream_id, uint16_t worker);
private uint16_t stream_enqueue_connection(int stream_id, uint16_t conn);
private uint16_t stream_dequeue_connection(int stream_id);
private uint16_t stream_enqueue_waiting(int stream_id, uint16_t conn);
private uint16_t stream_dequeue_waiting(int stream_id);
private m_string *stream_enqueue_packet(uint16_t socket_id, m_string *data);
private m_string *stream_dequeue_packet(uint16_t socket_id);
private void stream_drop_packets(uint16_t socket_id);
private int stream_get_connection(int stream_id);
private int stream_get_pipe(int stream_id, uint16_t socket_id);
private void stream_open_pipe(int stream_id);
private void stream_socket_fini(void);

/* -------------------------------------------------------------------------- */

#endif
