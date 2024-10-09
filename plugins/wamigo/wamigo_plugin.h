/*******************************************************************************
 *  Wamigo Daemon                                                              *
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

#ifndef WAMIGO_PLUGIN_H

#define WAMIGO_PLUGIN_H

/* mandatory server API declarations */
#include "m_server.h"
#include "m_plugin.h"

/* optional, useful parts of the server public API */
#include "m_socket.h"

#define WAMIGO_BUILD 1
#define WAMIGO_VERSION "0.1"
#define WAMIGO_PORT "5366" /* wo ai ni Lulu :) */

#ifdef WIN32
#define WAMIGO_REGISTRYKEY "Software\\Wamigo"
#endif

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
 * API revision number that the plugin has been made for.
 * If the server has a lower API revision number, it will not load the plugin.
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
 * should be kept if the plugin wants to call some server API later.
 *
 * It's in this function that the plugin has the opportunity to perform
 * all the required initialization work, like setting up database connections,
 * loading data, etc...
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

public void plugin_input_handler(uint16_t socket_id, uint16_t ingress_id,
                                 m_string *data);

/**
 * @ingroup plugin
 * @fn void plugin_input_handler(uint16_t socket_id, uint16_t ingress_id,
 *                               m_string *data)
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

public void plugin_exit(void);

/**
 * @ingroup plugin
 * @fn void plugin_exit(void)
 *
 * This function is called by the server when unloading the plugin. It gives
 * the opportunity to clean up all the resources allocated by the plugin, like
 * internal data, database connections...
 *
 */

/* -------------------------------------------------------------------------- */
/* OPTIONAL PLUGIN CALLBACKS */
/* -------------------------------------------------------------------------- */

public void plugin_event_handler(uint16_t id, uint16_t ingress_id,
                                 unsigned int event, void *event_data);

/**
 * @ingroup plugin
 * @fn void plugin_event_handler(uint16_t id, uint16_t ingress_id, int event)
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
/* WAMIGO API */
/* -------------------------------------------------------------------------- */

#define WAMIGO_CALL_MAX 14

#define WAMIGO_INIT    0
#define WAMIGO_AUTH    1
#define WAMIGO_SYNC    2
#define WAMIGO_FINI    3
#define WAMIGO_RM      4
#define WAMIGO_MOVE    5
#define WAMIGO_DLOAD   6
#define WAMIGO_ID      7
#define WAMIGO_MKDIR   8
#define WAMIGO_ULOAD   9
#define WAMIGO_UPD    10
#define WAMIGO_VER    11
#define WAMIGO_LIST   12
#define WAMIGO_MESG   13

#define WAMIGO_SCAN_INIT  0x1
#define WAMIGO_SCAN_SYNC  0x2

/* -------------------------------------------------------------------------- */

private int wamigo_api_setup(void);

/* -------------------------------------------------------------------------- */

private unsigned int wamigo_running(void);

/* -------------------------------------------------------------------------- */

private int wamigo_scan_directory(const char *dir, const char *ext, int sync);

/* -------------------------------------------------------------------------- */

private unsigned int wamigo_get_ver(void);

/* -------------------------------------------------------------------------- */

private m_string *wamigo_get_id(void);

/* -------------------------------------------------------------------------- */

private m_string *wamigo_get_user(void);

/* -------------------------------------------------------------------------- */

private m_string *wamigo_get_pass(void);

/* -------------------------------------------------------------------------- */

private int wamigo_call_sync(m_file *sync_output);

/* -------------------------------------------------------------------------- */

private int wamigo_call(unsigned int call, unsigned int argc, ...);

/* -------------------------------------------------------------------------- */

private int wamigo_monitor_directory(const char *dir);

/* -------------------------------------------------------------------------- */

private void wamigo_stop_monitoring(void);

/* -------------------------------------------------------------------------- */

private void wamigo_api_shutdown(void);

/* -------------------------------------------------------------------------- */

private void wamigo_api_cleanup(void);

/* -------------------------------------------------------------------------- */

#endif
