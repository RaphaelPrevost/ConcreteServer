/*******************************************************************************
 *  Wamigo Daemon                                                              *
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

#include "wamigo_plugin.h"

/* plugin identifier */
static uint32_t plugin_token = 0;

/* -------------------------------------------------------------------------- */

public unsigned int plugin_api(void)
{
    unsigned int required_api_revision = 1360;
    return required_api_revision;
}

/* -------------------------------------------------------------------------- */

public int plugin_init(uint32_t id, UNUSED int argc, UNUSED char **argv)
{
    /* if the plugin should never be loaded more than once, it is possible
       to check if it has already loaded by inspecting the value of
       plugin_token. Loading the same plugin twice without checking can
       lead to memory corruption if some pointers are blindly clobbered
       by the plugin_init() function, since they share the same address
       space.
    */

    if (plugin_token) {
        fprintf(stderr, "Wamigo: plugin already loaded.\n");
        return -1;
    }

    fprintf(stderr, "Wamigo: loading Wamigo plugin...\n");
    fprintf(stderr, "Wamigo: Copyright (c) 2010-2019 ");
    fprintf(stderr, "Raphael Prevost, all rights reserved.\n");
    fprintf(stderr, "Wamigo: version "WAMIGO_VERSION" ["__DATE__"]\n");

    fprintf(stderr, "Wamigo: listening on port "WAMIGO_PORT".\n");

    if (server_open_managed_socket(id, NULL, WAMIGO_PORT,
                                   SOCKET_SERVER) == -1) {
        fprintf(stderr,
                "Wamigo: could not listen to TCP port "WAMIGO_PORT".\n");
        return -1;
    }

    plugin_token = id;

    if (wamigo_api_setup() == -1) {
        fprintf(stderr, "Wamigo: could not setup the API.\n");
        return -1;
    }

    return 0;
}

/* -------------------------------------------------------------------------- */

private uint32_t plugin_get_token(void)
{
    return plugin_token;
}

/* -------------------------------------------------------------------------- */

public void plugin_main(uint16_t id, UNUSED uint16_t ingress_id, m_string *buf)
{
    /* this task will send back the incoming data and close the connection */
    server_send_buffer(plugin_token, id, SERVER_TRANS_END, CSTR(buf), SIZE(buf));
}

/* -------------------------------------------------------------------------- */

public void plugin_intr(UNUSED uint16_t id, UNUSED uint16_t ingress_id,
                        unsigned int event)
{
    switch (event) {

    case PLUGIN_EVENT_INCOMING_CONNECTION:
        /* new connection */ break;

    case PLUGIN_EVENT_OUTGOING_CONNECTION:
        /* connected to remote host */ break;

    case PLUGIN_EVENT_SOCKET_DISCONNECTED:
        /* connection closed */ break;

    case PLUGIN_EVENT_REQUEST_TRANSMITTED:
        /* request sent */ break;

    case PLUGIN_EVENT_SERVER_SHUTTINGDOWN:
        /* server shutting down */
        wamigo_api_shutdown();
        break;

    default: fprintf(stderr, "Wamigo: spurious event.\n");

    }
}

/* -------------------------------------------------------------------------- */

public void plugin_fini(void)
{
    wamigo_api_cleanup();
    fprintf(stderr, "Wamigo: successfully unloaded.\n");
}

/* -------------------------------------------------------------------------- */
