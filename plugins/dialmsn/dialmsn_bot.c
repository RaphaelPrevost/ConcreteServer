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

/* bot threads have a default stacksize of 1MiB */
#define DIALMSN_BOT_STACKSIZE  1048576
static int bot_stop = 0;
static pthread_t bot[DIALMSN_BOTS_MAX];

/* prng context */
static m_random *rnd = NULL;

#define DIALMSN_SQL_BOTCLEANUP "CALL clean_bot_work()"
#define DIALMSN_SQL_ACQUIREFEM "CALL get_a_woman_for_bot()"
#define DIALMSN_SQL_GETBOTWORK "CALL get_bot_work(%i)"
#define DIALMSN_SQL_GETBOTCONF "CALL get_bot_config()"
#define DIALMSN_SQL_SNDBOTMAIL "CALL send_reminder_email(%i, %i)"
#define DIALMSN_SQL_LOGBOTCHAT "CALL log_scenario(%i, %i, %i)"
#define DIALMSN_SQL_RELEASEMAN "CALL release_man_for_bot(%i)"
#define DIALMSN_SQL_RELEASEFEM "CALL release_woman_for_bot(%i)"

static void *_dialmsn_bot(void *);

/* the bot accepts 5 chars commands */
#define DIALMSN_BOT_CMDSZ 6
#define DIALMSN_BOT_PAUSE "@Pause"

#define DIALMSN_BOT_DELAY        60   /* seconds */
#define DIALMSN_BOT_MAXDELAY     3600 /* seconds */
#define DIALMSN_BOT_REFRESH      90   /* seconds */
#define DIALMSN_BOT_SLEEP        10   /* seconds */
#define DIALMSN_BOT_PROBABILITY  8

/* -------------------------------------------------------------------------- */

private int dialmsn_bot_init(void)
{
    m_db *con = NULL;
    unsigned long i = 0;
    pthread_attr_t attr;

    if (pthread_attr_init(& attr) != 0) return -1;

    if (! (rnd = random_init()) ) {
        pthread_attr_destroy(& attr);
        return -1;
    }

    pthread_attr_setstacksize (& attr, DIALMSN_BOT_STACKSIZE);

    con = dialmsn_db_borrow();
    db_exec(con, DIALMSN_SQL_BOTCLEANUP);
    con = dialmsn_db_return(con);

    for (i = 0; i < DIALMSN_BOTS_MAX; i ++) {
        if (pthread_create(& bot[i], & attr, _dialmsn_bot, (void *) i) == -1) {
            perror(ERR(dialmsn_bot_init, pthread_create));
            rnd = random_fini(rnd);
            bot_stop = 1;
            while (i --) pthread_join(bot[i], NULL);
            bot_stop = 0;
            pthread_attr_destroy(& attr);
            return -1;
        }
    }

    pthread_attr_destroy(& attr);

    return 0;
}

/* -------------------------------------------------------------------------- */

static void *_dialmsn_bot(UNUSED void *dummy)
{
    m_db *con = NULL;
    m_recordset *r = NULL;
    m_string *script = NULL;
    uint16_t spammee = 0, simulated_user = 0;
    uint32_t uid = 0, spammee_uid = 0;
    unsigned int i = 0;
    unsigned int ttl = 0, probability = DIALMSN_BOT_PROBABILITY, maildelay = 0;
    struct timeval tv;
    time_t impersonation_start = 0;

    while (! bot_stop) {

        /* check if we need to disconnect an impersonated woman */
        if (impersonation_start && ttl) {
            time_t timecheck = 0;
            gettimeofday(& tv, NULL); timecheck = tv.tv_sec;
            if (timecheck - impersonation_start >= (time_t) ttl * 60) {
                /* disconnect the simulated user */
                dialmsn_user_endsim(simulated_user);
                con = dialmsn_db_borrow();
                db_exec(con, DIALMSN_SQL_RELEASEFEM, uid);
                con = dialmsn_db_return(con);
                impersonation_start = 0; ttl = 0; uid = 0;
                goto _bot_continue;
            }
        } else {
            con = dialmsn_db_borrow();

            if (! (r = db_query(con, DIALMSN_SQL_ACQUIREFEM)) ) {
                debug("DialMessenger::BOT: SQL error.\n");
                goto _bot_panic;
            }

            con = dialmsn_db_return(con);

            if (! r->rows) {
                debug("DialMessenger::BOT: could not impersonate a woman.\n");
                db_free(r);
                goto _bot_continue;
            }

            uid = db_integer(r, "uid"); ttl = db_integer(r, "ttl");

            r = db_free(r);

            if (! uid || ! ttl) {
                debug("DialMessenger::BOT: incorrect woman UID or ttl.\n");
                goto _bot_continue;
            }

            /* start simulating the woman */
            simulated_user = dialmsn_user_simulate(uid, DIALMSN_STATUS_NRML);
            if (! simulated_user) {
                debug("DialMessenger::BOT: could not bring the woman online.\n");
                goto _bot_continue;
            }

            /* wait for user list update */
            if (! bot_stop) sleep(DIALMSN_BOT_REFRESH);

            gettimeofday(& tv, NULL);

            impersonation_start = tv.tv_sec;
        }

        con = dialmsn_db_borrow();

        if (! (r = db_query(con, DIALMSN_SQL_GETBOTWORK, uid)) ) {
            debug("DialMessenger::BOT: SQL error.\n");
            con = dialmsn_db_return(con);
            goto _bot_panic;
        }

        con = dialmsn_db_return(con);

        if (! r->rows) { db_free(r); goto _bot_continue; }

        spammee = db_integer(r, "man_session");
        script = db_varchar(r, "sctxt");

        /* get additional informations for the logging */
        spammee_uid = db_integer(r, "man_uid");

        if (! uid || ! spammee || ! script) {
            debug("DialMessenger::BOT: the script can not be run.\n");
            r = db_free(r);
            script = string_free(script);
            goto _bot_continue;
        }

        r = db_free(r);

        /* simulate peeking at the user profile */
        server_send_response(plugin_get_token(), spammee, 0x0,
                             "%bB4i%i%bB4i",
                             DIALMSN_INFO | DIALMSN_INFO_POP, uid,
                             DIALMSN_TERM);

        /* random delay between peeking and spamming */
        if (! bot_stop)
            sleep(DIALMSN_BOT_DELAY + (random_uint32(rnd) % DIALMSN_BOT_DELAY));

        /* parse the scenario */
        if (string_splits(script, "\n", 1) == -1) {
            debug("DialMessenger::BOT: could not parse the scenario.\n");
            script = string_free(script);
            goto _bot_continue;
        }

        for (i = 0; i < PARTS(script); i ++) {
            /* look for commands on the line */
            m_string *line = TOKEN(script, i);
            size_t pause = 0, end = 0, cmdlen = 0;
            unsigned int delay = DIALMSN_BOT_DELAY;

            /* ensure that the session id still matches the user id */
            if (dialmsn_user_getuid(spammee) != spammee_uid) break;

            pause = string_finds(line, 0, DIALMSN_BOT_PAUSE, DIALMSN_BOT_CMDSZ);

            if (pause != (size_t) -1 && pause + DIALMSN_BOT_CMDSZ < SIZE(line)) {
                if ( (*(DATA(line) + pause + DIALMSN_BOT_CMDSZ) == '(') &&
                     (pause + DIALMSN_BOT_CMDSZ < SIZE(line)) ) {

                    /* get the number of seconds to sleep */
                    delay = atoi(DATA(line) + pause + DIALMSN_BOT_CMDSZ + 1);

                    /* remove the command */
                    end = string_finds(line,
                                       pause + DIALMSN_BOT_CMDSZ + 1, ")", 1);
                    cmdlen = (end > 0) ? end - pause : DIALMSN_BOT_CMDSZ;
                    string_suppr(line, pause, cmdlen + 1);

                } else string_suppr(line, pause, DIALMSN_BOT_CMDSZ);

                /* don't allow more than one hour delay */
                if (delay > DIALMSN_BOT_MAXDELAY) delay = DIALMSN_BOT_DELAY;
            }

            if (! SIZE(line)) {
                debug("DialMessenger::BOT: empty script line.\n");
                continue;
            }

            debug("DialMessenger::BOT: \"%.*s\".\n", SIZE(line), DATA(line));

            server_send_response(plugin_get_token(), spammee, 0x0,
                                 "%bB4i%i%.*s%.*s\r\n%bB4i",
                                 DIALMSN_MESG | DIALMSN_MESG_RTF,
                                 simulated_user,
                                 DIALMSN_SEPLENGTH, DIALMSN_SEPARATOR,
                                 SIZE(line),
                                 DATA(line), DIALMSN_TERM);

            /* wait the required delay before the next line */
            debug("DialMessenger::BOT: sleeping for %i seconds.\n", delay);
            if (! bot_stop) sleep(delay);
        }

        script = string_free(script);

        /* the dialogue is over, release the spammee */
        con = dialmsn_db_borrow();
        db_exec(con, DIALMSN_SQL_RELEASEMAN, spammee_uid);
        con = dialmsn_db_return(con);

        /* get the probability of mail harassment */
        con = dialmsn_db_borrow();
        r = db_query(con, DIALMSN_SQL_GETBOTCONF);
        con = dialmsn_db_return(con);

        if (r) {
            probability = db_integer(r, "retryeach");
            debug("DialMessenger::BOT: got email retryeach=1/%i.\n",
                  probability);
            maildelay = db_integer(r, "delaychatmail");
            debug("DialMessenger::BOT: got email delaychatmail=%is\n",
                  maildelay);
        }

        /* ensure sane probability value */
        if (! probability) probability = DIALMSN_BOT_PROBABILITY;

        r = db_free(r);

        if (random_uint32(rnd) % probability == 0) {
            int sockid = 0;
            const char *http =
            "GET /bot/alert_mail_for_bot.php?id=%i&uid=%i HTTP/1.1\r\n"
            "Host: %s\r\n"
            "User-Agent: DialFlirt v2 SexBot\r\n"
            "Connection: close\r\n"
            "\r\n";

            if (! bot_stop) {
                debug("DialMessenger::BOT: sleeping for %im before sending mail.\n",
                      maildelay);
                sleep(maildelay * 60);
            } else debug("DialMessenger::BOT: shutting down, will not sleep.\n");

            /* send a mail */
            debug("DialMessenger::BOT: sending an internal mail.\n");
            con = dialmsn_db_borrow();
            r = db_query(con, DIALMSN_SQL_SNDBOTMAIL, uid, spammee_uid);
            con = dialmsn_db_return(con);

            if (db_integer(r, "send_mail") != 1) {
                r = db_free(r); goto _bot_continue;
            }

            r = db_free(r);

            /* trigger the notification by calling the php script */
            sockid = server_open_managed_socket(plugin_get_token(),
                                                "127.0.0.1", "80", 0x0);

            if (sockid == -1) {
                debug("DialMessenger::BOT: could not connect to HTTP server.\n");
                goto _bot_continue;
            }

            /* the socket is managed by the server from now on */
            debug("DialMessenger::BOT: sending an email notification.\n");
            server_send_response(plugin_get_token(), sockid, SERVER_TRANS_END,
                                 http, uid, spammee_uid,
                                 plugin_get_host());

            /* prevent unlocking an already unlocked man */
            spammee_uid = 0;
        }

_bot_continue:
        if (spammee_uid) {
            con = dialmsn_db_borrow();
            db_exec(con, DIALMSN_SQL_RELEASEMAN, spammee_uid);
            con = dialmsn_db_return(con);
        }

        if (! bot_stop) sleep(DIALMSN_BOT_SLEEP);
    }

    if (uid) {
        con = dialmsn_db_borrow();
        db_exec(con, DIALMSN_SQL_RELEASEFEM, uid);
        con = dialmsn_db_return(con);
    }

    pthread_exit(NULL);

_bot_panic:
    con = dialmsn_db_return(con);
    debug("DialMessenger::BOT: fatal error, aborting.\n");
    pthread_exit(NULL);
}

/* -------------------------------------------------------------------------- */

private void dialmsn_bot_fini(void)
{
    unsigned int i = 0;

    bot_stop = 1;

    for (i = 0; i < DIALMSN_BOTS_MAX; i ++) {
        pthread_cancel(bot[i]);
        pthread_join(bot[i], NULL);
    }

    rnd = random_fini(rnd);

    return;
}

/* -------------------------------------------------------------------------- */
