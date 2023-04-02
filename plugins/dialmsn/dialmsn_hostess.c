/*******************************************************************************
 *  Dialflirt Messenger                                                        *
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

#include "dialmsn_plugin.h"
#include <libxml/xmlreader.h>

struct _dialmsn_hostess {
    unsigned int id;
    xmlChar *name;
    unsigned int age;
    xmlChar *hair;
    xmlChar *eyes;
    xmlChar *profile;
    xmlChar *avatar;
};

#define DIALMSN_HOSTESS_STACKSIZE  1048576
static int hostess_stop = 0;
static pthread_t hostess;

#define DIALMSN_HOSTESS_MAX        32

static pthread_mutex_t hostess_lock = PTHREAD_MUTEX_INITIALIZER;
static unsigned int hostess_online = 0;
static uint16_t hostess_session[DIALMSN_HOSTESS_MAX];
static unsigned int hostess_id[DIALMSN_HOSTESS_MAX];

#define DIALMSN_SQL_GETHOSTUID "CALL get_hostess_uid(%i)"

static void *_dialmsn_hostess(void *);

/* -------------------------------------------------------------------------- */

private int dialmsn_hostess_init(const char *url)
{
    pthread_attr_t a;

    if (! url) return -1;

    if (pthread_attr_init(& a) != 0) return -1;

    pthread_attr_setstacksize (& a, DIALMSN_HOSTESS_STACKSIZE);

    if (pthread_create(& hostess, & a, _dialmsn_hostess, (void *) url) == -1) {
        perror(ERR(dialmsn_bot_init, pthread_create));
        pthread_attr_destroy(& a);
        return -1;
    }

    pthread_attr_destroy(& a);

    return 0;
}

/* -------------------------------------------------------------------------- */

static void _dialmsn_hostess_impersonate(struct _dialmsn_hostess *hostess)
{
    m_db *con = NULL;
    m_recordset *r = NULL;
    uint32_t uid = 0;
    uint16_t session = 0;
    int sockid = 0;
    m_string *profile = NULL, *avatar = NULL;
    const char *http =
    "GET /bot/register_profile.php?id=%i&name=%s&age=%i"
    "&hair=%s&eyes=%s&profile=%s&avatar=%s HTTP/1.1\r\n"
    "Host: %s\r\n"
    "User-Agent: DialFlirt v2 SexyHostess\r\n"
    "Connection: close\r\n"
    "\r\n";

    con = dialmsn_db_borrow();
    r = db_query(con, DIALMSN_SQL_GETHOSTUID, hostess->id);
    con = dialmsn_db_return(con);

    if (r && r->rows) {
        if ( (uid = db_integer(r, "ret")) ) {
            pthread_mutex_lock(& hostess_lock);

                if (hostess_online < DIALMSN_HOSTESS_MAX) {
                    session = dialmsn_user_simulate(uid, DIALMSN_STATUS_HOST);
                    if (session) {
                        hostess_session[hostess_online] = session;
                        hostess_id[hostess_online ++] = hostess->id;
                    }
                }

            pthread_mutex_unlock(& hostess_lock);
        } else {
            sockid = server_open_managed_socket(plugin_get_token(),
                                                "127.0.0.1", "80", 0x0);

            if (sockid == -1) {
                debug("DialMessenger::HOSTESS: HTTP server unavailable.\n");
                goto _cleanup;
            }

            if (hostess->profile)
                profile = string_b64s((char *) hostess->profile,
                                      xmlStrlen(hostess->profile), 0);
            if (hostess->avatar)
                avatar = string_b64s((char *) hostess->avatar,
                                     xmlStrlen(hostess->avatar), 0);

            /* the socket is managed by the server from now on */
            debug("DialMessenger::HOSTESS: initializing a new profile.\n");
            server_send_response(plugin_get_token(), sockid, SERVER_TRANS_END,
                                 http, hostess->id, hostess->name,
                                 hostess->age, hostess->hair,
                                 hostess->eyes,
                                 (profile) ? DATA(profile) : "",
                                 (avatar) ? DATA(avatar) : "",
                                 plugin_get_host());

            string_free(profile); string_free(avatar);
        }
    }

_cleanup:
    r = db_free(r);

    /* cleanup the hostess structure */
    hostess->id = 0; xmlFree(hostess->name); hostess->age = 0;
    xmlFree(hostess->hair); xmlFree(hostess->eyes);
    xmlFree(hostess->profile); xmlFree(hostess->avatar);
    memset(hostess, 0, sizeof(*hostess));
}

/* -------------------------------------------------------------------------- */

private void dialmsn_hostess_connect(unsigned int id_eurolive)
{
    m_db *con = NULL;
    m_recordset *r = NULL;
    uint32_t uid = 0;
    uint16_t session = 0;
    unsigned int i = 0, found = 0;

    con = dialmsn_db_borrow();
    r = db_query(con, DIALMSN_SQL_GETHOSTUID, id_eurolive);
    con = dialmsn_db_return(con);

    if ( (uid = db_integer(r, "ret")) ) {
        pthread_mutex_lock(& hostess_lock);

            /* check if the hostess is already online */
            for (i = 0; i < hostess_online; i ++) {
                if (hostess_id[i] == id_eurolive) {
                    found = 1; break;
                }
            }

            if (! found && hostess_online < DIALMSN_HOSTESS_MAX) {
                /* try to bring the hostess online */
                session = dialmsn_user_simulate(uid, DIALMSN_STATUS_HOST);
                if (session) {
                    hostess_session[hostess_online] = session;
                    hostess_id[hostess_online ++] = id_eurolive;
                }
            }

        pthread_mutex_unlock(& hostess_lock);
    }

    r = db_free(r);
}

/* -------------------------------------------------------------------------- */

static void *_dialmsn_hostess(void *link)
{
    struct _dialmsn_hostess hostess[DIALMSN_HOSTESS_MAX];
    unsigned int online[DIALMSN_HOSTESS_MAX];
    void *feed = NULL;
    xmlTextReader *r = NULL;
    char *node = NULL;
    xmlChar *value = NULL;
    unsigned int in_hostess = 0, in_image = 0, in_category = 0, count = 0;
    const char *url = link;
    unsigned int i = 0, j = 0;

    while (! hostess_stop) {

        memset(online, 0, sizeof(online)); count = 0;

        /* parse the feed */
        if (! (feed = xmlIOHTTPOpen(url)) ) {
            debug("DialMessenger::HOSTESS: could not download XML feed.\n");
            goto _hostess_continue;
        }

        r = xmlReaderForIO(xmlIOHTTPRead, xmlIOHTTPClose, feed, "", NULL, 0);
        if (! r) {
            debug("DialMessenger::HOSTESS: could not parse XML feed.\n");
            xmlIOHTTPClose(feed);
            goto _hostess_continue;
        }

        pthread_mutex_lock(& hostess_lock);

        while (xmlTextReaderRead(r) == 1) {

            if (xmlTextReaderNodeType(r) != XML_ELEMENT_NODE) continue;

            node = (char *) xmlTextReaderConstName(r);
            value = xmlTextReaderReadString(r);

            if (! strcmp(node, "hostess")) {
                if (in_hostess) {
                    in_image = in_category = 0;
                    /* check if this hostess is already online */
                    for (i = 0; i < hostess_online; i ++) {
                        if (hostess_id[i] == hostess[count].id) {
                            online[i] = 1; hostess[count].id = 0;
                            xmlFree(hostess[count].name);
                            hostess[count].age = 0;
                            xmlFree(hostess[count].hair);
                            xmlFree(hostess[count].eyes);
                            xmlFree(hostess[count].profile);
                            xmlFree(hostess[count].avatar);
                            memset(& hostess[count], 0, sizeof(hostess[count]));
                            break;
                        }
                    }

                    /* if this hostess is not already online, keep it */
                    if (hostess[count].id) count ++;
                }
                else in_hostess = 1;
            } else if (! strcmp(node, "category")) {
                in_category = 1;
            } else if (! in_category && ! strcmp(node, "id")) {
                hostess[count].id = atoi((char *) value);
            } else if (! in_category && ! strcmp(node, "name")) {
                hostess[count].name = value; value = NULL;
            } else if (! strcmp(node, "age")) {
                hostess[count].age = atoi((char *) value);
            } else if (! strcmp(node, "hair")) {
                hostess[count].hair = value; value = NULL;
            } else if (! strcmp(node, "eyes")) {
                hostess[count].eyes = value; value = NULL;
            } else if (! strcmp(node, "secrets")) {
                hostess[count].profile = value; value = NULL;
            } else if (! strcmp(node, "image")) {
                in_image = 1;
            } else if (in_image && ! strcmp(node, "B")) {
                hostess[count].avatar = value; value = NULL;
            }

            xmlFree(value);
        }

        xmlTextReaderClose(r); xmlFreeTextReader(r);

        /* check if the last hostess is already online */
        for (i = 0; i < hostess_online; i ++) {
            if (hostess_id[i] == hostess[count].id) {
                online[i] = 1; hostess[count].id = 0;
                xmlFree(hostess[count].name);
                hostess[count].age = 0;
                xmlFree(hostess[count].hair);
                xmlFree(hostess[count].eyes);
                xmlFree(hostess[count].profile);
                xmlFree(hostess[count].avatar);
                memset(& hostess[count], 0, sizeof(hostess[count]));
                break;
            }
        }

        /* if this hostess is not already online, keep it */
        if (hostess[count].id) count ++;

        /* disconnect hostesses that are no longer listed as online */
        for (i = 0, j = 0; i < hostess_online; i ++) {
            if (! online[i]) {
                dialmsn_user_endsim(hostess_session[i]);
                hostess_session[i] = 0; hostess_id[i] = 0;
            } else {
                hostess_session[j] = hostess_session[i];
                hostess_id[j ++] = hostess_id[i];
            }
        }

        hostess_online = j;

        pthread_mutex_unlock(& hostess_lock);

        /* add all the new hostesses */
        for (i = 0; i < count; i ++)
            _dialmsn_hostess_impersonate(& hostess[i]);

_hostess_continue:
        /* sleep until next refresh */
        if (! hostess_stop) sleep(60);
    }

    while (hostess_online --)
        dialmsn_user_endsim(hostess_session[hostess_online]);

    pthread_exit(NULL);
}

/* -------------------------------------------------------------------------- */

private void dialmsn_hostess_fini(void)
{
    hostess_stop = 1;

    pthread_cancel(hostess);
    pthread_join(hostess, NULL);

    return;
}

/* -------------------------------------------------------------------------- */
