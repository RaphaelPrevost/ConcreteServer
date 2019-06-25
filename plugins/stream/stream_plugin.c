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

#include "stream_plugin.h"

/* plugin identifier */
static uint32_t plugin_token = 0;

/* -------------------------------------------------------------------------- */

public unsigned int plugin_api(void)
{
    unsigned int required_api_revision = 1380;
    return required_api_revision;
}

/* -------------------------------------------------------------------------- */

private uint32_t plugin_get_token(void)
{
    return plugin_token;
}

/* -------------------------------------------------------------------------- */

public int plugin_init(uint32_t id, int argc, char **argv)
{
    if (plugin_token) {
        fprintf(stderr, "Stream: plugin already loaded.\n");
        return -1;
    }

    plugin_token = id;

    fprintf(stderr, "Stream: loading Stream plugin...\n");
    fprintf(stderr, "Stream: Copyright (c) 2008-2019 ");
    fprintf(stderr, "Raphael Prevost, all rights reserved.\n");
    fprintf(stderr, "Stream: version "STREAM_VERSION" ["__DATE__"]\n");

    if (stream_config_init(argc, argv) == -1) {
        fprintf(stderr, "Stream: failed to load configuration.\n");
        goto _init_conf_failure;
    }

    if (stream_socket_init() == -1) {
        fprintf(stderr, "Stream: failed to initialize sockets.\n");
        goto _init_sock_failure;
    }

    if (stream_router_init() == -1) {
        fprintf(stderr, "Stream: failed to initialize routing.\n");
        goto _init_sock_failure;
    }

    return 0;

_init_sock_failure:
    stream_socket_fini();
_init_conf_failure:
    stream_config_fini();
    return -1;
}

/* -------------------------------------------------------------------------- */

public void plugin_main(uint16_t socket_id, uint16_t ingress_id, m_string *data)
{
    uint16_t egress = 0;
    int stream_id = 0;

    if (stream_personality() & PERSONALITY_WORKER) {
        if ( (stream_id = stream_get_id(socket_id, PERSONALITY_WORKER)) != -1) {
            if (ntohl(string_fetch_uint32(data)) == MASTER_OP_HIRED)
                stream_open_pipe(stream_id);
            else debug("Stream: unsupported message from Master.\n");
        }
    }

    if (stream_personality() & PERSONALITY_MASTER) {
        if (stream_get_status(socket_id) == STREAM_STATUS_CONN) {

            stream_id = stream_get_id(ingress_id, PERSONALITY_MASTER);

            switch (stream_get_route(ingress_id)) {

            case ROUTE_WORKER: { /* received data from a worker */

                switch (ntohl(string_fetch_uint32(data))) {
                case WORKER_OP_HELLO: { /* new worker */
                    stream_add_worker(stream_id, socket_id);
                } break;
                case WORKER_OP_READY: { /* new connection, ask for a pipe */
                    socket_id = stream_enqueue_connection(stream_id, socket_id);
                    if ( (socket_id = stream_dequeue_waiting(stream_id)) )
                        stream_get_pipe(stream_id, socket_id);
                } break;
                default: /* unknown connection */
                    debug("Stream: unsupported connection on workers end.\n");
                }
                return;

            } break;

            case ROUTE_PUBLIC: { /* received data from a client */
                stream_enqueue_packet(socket_id, data);
                stream_get_pipe(stream_id, socket_id);
                string_flush(data);
                return;
            } break;

            default: /* unknown route */
                debug("Stream: cannot route the request.\n");

            } /* ROUTE */
        } /* STREAM_STATUS_CONN */
    } /* PERSONALITY_MASTER */

    /* socket pipe */
    if (stream_get_status(socket_id) == STREAM_STATUS_PIPE) {
        if ( (egress = stream_get_egress(socket_id, ingress_id)) ) {
            /* forward the data between the two ends of the pipe */
            server_send_buffer(plugin_get_token(), egress,
                               0x0, CSTR(data), SIZE(data));
        } else server_close_managed_socket(plugin_get_token(), socket_id);
        string_flush(data);
    }

    return;
}

/* -------------------------------------------------------------------------- */

public void plugin_intr(uint16_t socket_id, uint16_t ingress_id, int event)
{
    uint16_t egress = 0;

    switch (event) {

    case PLUGIN_EVENT_INCOMING_CONNECTION: {
        stream_set_status(socket_id, STREAM_STATUS_CONN);
    } break;

    case PLUGIN_EVENT_OUTGOING_CONNECTION: {
        stream_set_status(socket_id, STREAM_STATUS_CONN);
        if (stream_personality() & PERSONALITY_WORKER) {
            if (stream_get_id(socket_id, PERSONALITY_WORKER) != -1)
                server_send_response(plugin_get_token(), socket_id,
                                     0x0, "%bB4u", WORKER_OP_HELLO);
        }
    } break;

    case PLUGIN_EVENT_SOCKET_DISCONNECTED: {
        /* if this was the end of a pipe, we need to shut down the egress */
        if ( (egress = stream_get_egress(socket_id, ingress_id)) )
            server_close_managed_socket(plugin_get_token(), egress);
        stream_set_status(socket_id, STREAM_STATUS_DOWN);
        /* discard queued packets */
        stream_drop_packets(socket_id);
    } break;

    case PLUGIN_EVENT_REQUEST_TRANSMITTED:
        /* request sent */ break;

    case PLUGIN_EVENT_SERVER_SHUTTINGDOWN:
        /* server shutting down */ break;

    default: fprintf(stderr, "Stream: spurious event.\n");

    }
}

/* -------------------------------------------------------------------------- */

public void plugin_fini(void)
{
    stream_socket_fini();
    stream_config_fini();
    fprintf(stderr, "Stream: successfully unloaded.\n");
}

/* -------------------------------------------------------------------------- */
