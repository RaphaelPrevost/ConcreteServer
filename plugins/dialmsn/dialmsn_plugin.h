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

#ifndef DIALMSN_PLUGIN_H

#define DIALMSN_PLUGIN_H

/* standard server definitions */
#include "m_server.h"
#include "m_socket.h"
#include "m_db.h"
#include "m_random.h"

/* required server build configuration */
#ifndef _ENABLE_DB
#error "DialMessenger: database access layer is required."
#endif

#ifndef _ENABLE_RANDOM
#error "DialMessenger: builtin PRNG is required."
#endif

/* -------------------------------------------------------------------------- */

/* versioning informations */
#define DIALMSN_VERSION            "0.3"
#define DIALMSN_REQUIRED_API_REV   1390

/* max simultaneous database connections */
#define DIALMSN_DBMAX              15

/* chat client tcp port */
#define DIALMSN_PORT               "8486"
#define DIALMSN_PCAM               "7307"

/* -------------------------------------------------------------------------- */

/* protocol definitions */

/* lobby */
#define DIALMSN_LOBBY_DATE  0x01              /* regular lobby */
#define DIALMSN_LOBBY_SEXY  0x02              /* XXX lobby */

/* status */
#define DIALMSN_STATUS_NULL 0x00              /* unknown status */
#define DIALMSN_STATUS_NRML 0x08              /* online */
#define DIALMSN_STATUS_BUSY 0x10              /* busy */
#define DIALMSN_STATUS_AWAY 0x20              /* away */
#define DIALMSN_STATUS_UNAV 0x40              /* unavailable */
#define DIALMSN_STATUS_HOST 0x80              /* private status - hostess */

/* request parameters separator */
#define DIALMSN_SEPARATOR "|"                 /* param separator */
#define DIALMSN_SEPLENGTH 1                   /* param separator length */

/* request line separator */
#define DIALMSN_LINESEPARATOR "\n"            /* line separator */
#define DIALMSN_LINESEPLENGTH 1               /* line separator length */

/* main commands */
#define DIALMSN_AUTH 0x00000001               /* authentication */
#define DIALMSN_LIST 0x00000002               /* listing */
#define DIALMSN_INFO 0x00000004               /* informations */
#define DIALMSN_MESG 0x00000008               /* messaging */
#define DIALMSN_MAIL 0x00000010               /* internal mail */
#define DIALMSN_EXEC 0x00000020               /* server command */
#define DIALMSN_TERM 0x05201314               /* termination delimiter */

/* command parameters */

/* AUTH */
#define DIALMSN_AUTH_CLIENT_PWD 0x00000100    /* client auth request */
#define DIALMSN_AUTH_SERVER_ACK 0x00000200    /* server auth completed */
#define DIALMSN_AUTH_SERVER_ERR 0x00000400    /* authentification failure */

#define DIALMSN_AUTH_ERR_ENOMEM 0x00000800    /* server out of memory */
#define DIALMSN_AUTH_ERR_EINVAL 0x00001000    /* bad request */
#define DIALMSN_AUTH_ERR_ELOGIN 0x00002000    /* login not found */
#define DIALMSN_AUTH_ERR_EPASSW 0x00004000    /* wrong password */
#define DIALMSN_AUTH_ERR_EPROTO 0x00008000    /* bad protocol version */

/* LIST */
#define DIALMSN_LIST_QUERY      0x00000040    /* client list request */
#define DIALMSN_LIST_REPLY      0x00000080    /* server list */
#define DIALMSN_LIST_ERROR      0x00000100    /* error building list */
#define DIALMSN_LIST_QFLST      0x00000200    /* client favourite users list request */
#define DIALMSN_LIST_RFLST      0x00000400    /* server favourite users list */
#define DIALMSN_LIST_QBLST      0x00000800    /* client blocked users list request */
#define DIALMSN_LIST_RBLST      0x00001000    /* server blocked users list */
#define DIALMSN_LIST_QLIST      0x00000A40    /* (QUERY | QFLST | QBLST) */
#define DIALMSN_LIST_ADD        0x00002000    /* add a user to a list */
#define DIALMSN_LIST_REM        0x00004000    /* remove a user from a list */

#define DIALMSN_LIST_ADD_FAV    0x00008000    /* add to favourite */
#define DIALMSN_LIST_ADD_BLK    0x00010000    /* add to blocked */
#define DIALMSN_LIST_ADD_ACK    0x00020000    /* server acknowledgment */
#define DIALMSN_LIST_ADD_ERR    0x00040000    /* error adding */

#define DIALMSN_LIST_REM_FAV    0x00008000    /* remove from favourite */
#define DIALMSN_LIST_REM_BLK    0x00010000    /* remove from blocked */
#define DIALMSN_LIST_REM_ACK    0x00020000    /* server ack */
#define DIALMSN_LIST_REM_ERR    0x00040000    /* error removing */

#define DIALMSN_LIST_ERR_EINVAL 0x00008000    /* bad request */
#define DIALMSN_LIST_ERR_EEMPTY 0x00010000    /* empty list */
#define DIALMSN_LIST_ERR_ENOMEM 0x00020000    /* server out of memory */

/* INFO */
#define DIALMSN_INFO_POP        0x00000100    /* profile infopop */
#define DIALMSN_INFO_GRT        0x00000200    /* grant access to photoalbum */
#define DIALMSN_INFO_RVK        0x00001000    /* revoke access to photoalbum */
#define DIALMSN_INFO_ERR        0x00000400    /* error */

#define DIALMSN_INFO_ERR_EINVAL 0x00000800    /* bad request */

/* MESG */
#define DIALMSN_MESG_TXT        0x00000100    /* plain text message */
#define DIALMSN_MESG_RTF        0x00000200    /* RTF message */
#define DIALMSN_MESG_ERR        0x00000400    /* error processing the message */

#define DIALMSN_MESG_ERR_ENOMEM 0x00000800    /* server out of memory */
#define DIALMSN_MESG_ERR_EAGAIN 0x00001000    /* user offline */
#define DIALMSN_MESG_ERR_EINVAL 0x00002000    /* bad request */
#define DIALMSN_MESG_ERR_ENOSUB 0x00004000    /* the sender is not a subscriber */
#define DIALMSN_MESG_ERR_EBLOCK 0x00008000    /* the sender is blocked by the recipient */

/* MAIL */
#define DIALMSN_MAIL_CNT        0x000004000   /* get mail count */
#define DIALMSN_MAIL_ERR        0x000000800   /* error processing mail request */

#define DIALMSN_MAIL_ERR_ENOMEM 0x000008000   /* server out of memory */
#define DIALMSN_MAIL_ERR_EINVAL 0x000010000   /* bad request */
#define DIALMSN_MAIL_ERR_EEMPTY 0x000020000   /* empty mail list */

/* EXEC */
#define DIALMSN_EXEC_ADDACCOUNT 0x000000100   /* create a new user account */
#define DIALMSN_EXEC_CHNGSTATUS 0x000000200   /* update status */
#define DIALMSN_EXEC_HOSTONLINE 0x000000400   /* an hostess should be online */
#define DIALMSN_EXEC_WEBCAMCALL 0x000001000   /* call a user for cam session */
#define DIALMSN_EXEC_REFRESHUSR 0x000002000   /* refresh user informations */

#define DIALMSN_EXEC_ERR        0x000004000   /* error */
#define DIALMSN_EXEC_ERR_ENOMEM 0x000008000   /* server out of memory */
#define DIALMSN_EXEC_ERR_EINVAL 0x000010000   /* bad request */
#define DIALMSN_EXEC_ERR_EEMPTY 0x000020000   /* empty list */

/* -------------------------------------------------------------------------- */

/* max server capacity */
#define DIALMSN_USER_MAX       SOCKET_MAX + SOCKET_MAX

/* max nickname length */
#define DIALMSN_NICK_MAX       32

/* max password length */
#define DIALMSN_PASS_MAX       32

/* max online users */
#define DIALMSN_ONLINE_MAX     SOCKET_MAX

/* max number of bots */
#define DIALMSN_BOTS_MAX       32

/* -------------------------------------------------------------------------- */

/* cache for connected list and auth informations */
typedef struct dialmsn_user {
    uint32_t uid;           /* user identifier */
    char *nick;             /* nickname */
    char *city;             /* city */
    char *avatar;           /* avatar */
    /* blocked cache */
    uint32_t *busers;       /* blacklist */
    uint16_t bcount;        /* number of users in the blacklist */
    /* session */
    char lobby;             /* lobby number */
    unsigned char status;   /* user online status */
    char subscriber;        /* is the user a subscriber ? */
    /* cached informations */
    char sex;               /* user's sex */
    char dpt;               /* user's province */
    char age;               /* user's age */
    int visible;            /* control visibility */
} dialmsn_user;

/* -------------------------------------------------------------------------- */

private uint32_t plugin_get_token(void);

/* -------------------------------------------------------------------------- */
/* Private options */
/* -------------------------------------------------------------------------- */

private int plugin_quotas_enabled(void);

/* -------------------------------------------------------------------------- */

private int plugin_nonsubscribers_allowed(void);

/* -------------------------------------------------------------------------- */

private const char *plugin_get_host(void);

/* -------------------------------------------------------------------------- */
/* Database functions */
/* -------------------------------------------------------------------------- */

private int dialmsn_db_init(void);
private m_db *dialmsn_db_borrow(void);
private m_db *dialmsn_db_return(m_db *con);
private void dialmsn_db_exit(void);

/* -------------------------------------------------------------------------- */
/* Users management functions */
/* -------------------------------------------------------------------------- */

private uint32_t dialmsn_user_checkrequest(uint16_t id, uint32_t request);

private unsigned int dialmsn_user_count(void);

private dialmsn_user *dialmsn_user_add(uint16_t session, dialmsn_user *user,
                                       const char *version);
private dialmsn_user *dialmsn_user_free(dialmsn_user *user);

private uint16_t dialmsn_user_simulate(uint32_t uid, unsigned int status);
private void dialmsn_user_endsim(uint16_t session);

private void dialmsn_user_disconnect(uint16_t session);

private int dialmsn_user_available(uint16_t session);
private uint32_t dialmsn_user_getuid(uint16_t session);

//private char *dialmsn_user_getnickname(uint32_t id);
private uint32_t dialmsn_user_chknickname(const char *nick, size_t len);

private void dialmsn_user_setstatus(uint16_t session, uint32_t status);
private uint32_t dialmsn_user_getstatus(uint16_t session);

private int dialmsn_user_canspeak(uint16_t session, uint16_t recipient);

private void dialmsn_user_refreshblockedlist(uint16_t id);

private void dialmsn_user_refresh(uint32_t uid, uint32_t refresh_all);

/* -------------------------------------------------------------------------- */
/* Users listing functions */
/* -------------------------------------------------------------------------- */

private int dialmsn_userlist_initcache(void);
private void dialmsn_userlist_getconnected(uint16_t session);
private void dialmsn_userlist_getmisc(uint16_t session, uint32_t request);
private void dialmsn_userlist_refresh(void);
private void dialmsn_userlist_clear(void);
private void dialmsn_userlist_freecache(void);

/* -------------------------------------------------------------------------- */
/* Server calls */
/* -------------------------------------------------------------------------- */

private void dialmsn_auth(uint16_t id, m_string *s);

private int dialmsn_bot_init(void);
private void dialmsn_bot_exit(void);

private int dialmsn_hostess_init(const char *url);
private void dialmsn_hostess_connect(unsigned int id_eurolive);
private void dialmsn_hostess_exit(void);

#endif
