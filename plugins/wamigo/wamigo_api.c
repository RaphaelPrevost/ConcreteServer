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

#define USER "jaguarwan"
#define PASS "aaaaaa54sdfg"
#define HOST "app.testing.dev.mo-si.eu"
#define ADDR "85.17.224.113"

/* API callbacks */
static void _wamigo_init(uint16_t, uint16_t, m_string *);
static void _wamigo_auth(uint16_t, uint16_t, m_string *);
static void _wamigo_sync(uint16_t, uint16_t, m_string *);
static void _wamigo_endsync(uint16_t, uint16_t, m_string *);
static void _wamigo_delete(uint16_t, uint16_t, m_string *);
static void _wamigo_rename(uint16_t, uint16_t, m_string *);

static void _wamigo_id(uint16_t, uint16_t, m_string *);
static void _wamigo_mkdir(uint16_t, uint16_t, m_string *);

static void _wamigo_update(uint16_t, uint16_t, m_string *);
static void _wamigo_version(uint16_t, uint16_t, m_string *);
static void _wamigo_listmsg(uint16_t, uint16_t, m_string *);
static void _wamigo_getmsg(uint16_t, uint16_t, m_string *);

/* url table */
static const char *_call_url[] = { "/soft/init",
                                   "/soft/auth",
                                   "/soft/init-synchro",
                                   "/soft/end-synchro",
                                   "/soft/delete",
                                   "/soft/rename",
                                   "/soft/get-file",
                                   "/soft/get-soft-id",
                                   "/soft/mkdir",
                                   "/file-upload-link/index.php",
                                   "/soft/get-soft-update",
                                   "/soft/get-soft-version",
                                   "/soft/list-message-id",
                                   "/soft/get-message"
                                 };

/* bottom-ups table */
static void (*_call_cback[])(uint16_t, uint16_t, m_string *) = {
                                   _wamigo_init,
                                   _wamigo_auth,
                                   _wamigo_sync,
                                   _wamigo_endsync,
                                   _wamigo_delete,
                                   _wamigo_rename,
                                   NULL,
                                   _wamigo_id,
                                   _wamigo_mkdir,
                                   NULL,
                                   _wamigo_update,
                                   _wamigo_version,
                                   _wamigo_listmsg,
                                   _wamigo_getmsg
                                 };

#define XFER_UPLOAD   1
#define XFER_DOWNLOAD 2

typedef struct _wamigo_transfer {
        unsigned int type;
        char *request;
        unsigned long timestamp;
} wamigo_transfer;

/*pthread_rwlock_t transfer_lock;
unsigned int transfer_count;
wamigo_transfer *transfer;*/

static pthread_mutex_t _wamigo_ver_lock = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t _wamigo_ver_cond = PTHREAD_COND_INITIALIZER;
static unsigned int wamigo_ver = 0;
static pthread_mutex_t _wamigo_id_lock = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t _wamigo_id_cond = PTHREAD_COND_INITIALIZER;
static m_string *wamigo_id = NULL;
static pthread_mutex_t _wamigo_user_lock = PTHREAD_MUTEX_INITIALIZER;
static m_string *wamigo_user = NULL;
static pthread_mutex_t _wamigo_pass_lock = PTHREAD_MUTEX_INITIALIZER;
static m_string *wamigo_pass = NULL;

static pthread_mutex_t _wamigo_switch_lock = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t _wamigo_switch_cond = PTHREAD_COND_INITIALIZER;
static unsigned int wamigo_switch = 0;
static unsigned int wamigo_shutdown = 0;
static pthread_t main_thread;

/* -------------------------------------------------------------------------- */

static void *_wamigo_main(UNUSED void *dummy)
{
    unsigned int webservice_build = 0;

    /* wait for it... */
    pthread_mutex_lock(& _wamigo_switch_lock);
        while (! wamigo_switch)
            pthread_cond_wait(& _wamigo_switch_cond, & _wamigo_switch_lock);
    pthread_mutex_unlock(& _wamigo_switch_lock);

    /* get the Wamigo webservice version */
    webservice_build = wamigo_get_ver();
    fprintf(stderr, "Wamigo: webservice build #%i\n", webservice_build);

    if (webservice_build != WAMIGO_BUILD) {
        fprintf(stderr, "Wamigo: daemon and webservice builds do not match!\n");
    }

    /* get the configuration */
    wamigo_call(WAMIGO_INIT, 0);

    /*while (wamigo_switch) {

    }*/

    return NULL;
}

/* -------------------------------------------------------------------------- */

private int wamigo_api_setup(void)
{
    if (pthread_create(& main_thread, NULL, _wamigo_main, NULL) == -1) {
        perror(ERR(wamigo_api_setup, pthread_create));
        return -1;
    }

    pthread_mutex_lock(& _wamigo_switch_lock);
    wamigo_switch = 1;
    pthread_cond_broadcast(& _wamigo_switch_cond);
    pthread_mutex_unlock(& _wamigo_switch_lock);

    return 0;
}

/* -------------------------------------------------------------------------- */

private unsigned int wamigo_running(void)
{
    return wamigo_switch;
}

/* -------------------------------------------------------------------------- */

private unsigned int wamigo_get_ver(void)
{
    unsigned int ret = 0;

    pthread_mutex_lock(& _wamigo_ver_lock);

    while (wamigo_switch && ! (ret = wamigo_ver) ) {
        wamigo_call(WAMIGO_VER, 0);
        pthread_cond_wait(& _wamigo_ver_cond, & _wamigo_ver_lock);
    }

    pthread_mutex_unlock(& _wamigo_ver_lock);

    return ret;
}

/* -------------------------------------------------------------------------- */

private m_string *wamigo_get_id(void)
{
    m_string *ret = NULL;

    pthread_mutex_lock(& _wamigo_id_lock);

    if (! (ret = wamigo_id) ) {
        /* try to get an existing Soft ID */
        #ifdef WIN32
        HKEY key;
        char id[BUFSIZ];
        DWORD error = 0, len = BUFSIZ, type = 0;

        /* check if there is already an ID in registry */
        error = RegOpenKeyExA(HKEY_LOCAL_MACHINE, WAMIGO_REGISTRYKEY,
                                0, KEY_QUERY_VALUE | KEY_SET_VALUE, & key);
        if (error != ERROR_SUCCESS) {
            fprintf(stderr, ERR(wamigo_get_id, RegOpenKeyEx)": %s\n",
                    strerror(error));
        } else {
            error = RegQueryValueExA(key, "WamigoSoftID", 0, & type,
                                        (LPBYTE) id, & len);
            if (error != ERROR_SUCCESS) {
                fprintf(stderr, ERR(wamigo_get_id, RegQueryValueEx)": %s\n",
                        strerror(error));
            } else if (type == REG_SZ) {
                /* cache */
                ret = wamigo_id = string_alloc(id, strlen(id));
            }

            RegCloseKey(key);
        }
        #else
        /* NOT IMPLETEMENTED */
        #endif
    }

    /* if there is no stored ID, get a new one */
    while (wamigo_switch && ! (ret = wamigo_id) ) {
        wamigo_call(WAMIGO_ID, 0);
        pthread_cond_wait(& _wamigo_id_cond, & _wamigo_id_lock);
    }

    pthread_mutex_unlock(& _wamigo_id_lock);

    return ret;
}

/* -------------------------------------------------------------------------- */

private m_string *wamigo_get_user(void)
{
    m_string *ret = NULL;

    pthread_mutex_lock(& _wamigo_user_lock);

    if (! (ret = wamigo_user) ) {
        /* try to fetch the login from the system */
        #ifdef WIN32
        HKEY key;
        DWORD error = 0, len = BUFSIZ, type = 0;
        char v[BUFSIZ];

        if (wamigo_user) {
            /* another thread loaded the login in between */
            ret = wamigo_user;
            pthread_mutex_unlock(& _wamigo_user_lock);
            return ret;
        }

        /* check if the credentials are stored in registry */
        error = RegOpenKeyExA(HKEY_LOCAL_MACHINE, WAMIGO_REGISTRYKEY,
                              0, KEY_QUERY_VALUE | KEY_SET_VALUE, & key);
        if (error != ERROR_SUCCESS) {
            fprintf(stderr, ERR(wamigo_get_user, RegOpenKeyEx)": %s\n",
                    strerror(error));
        } else {
            /* login */
            error = RegQueryValueExA(key, "Login", 0, & type,
                                     (LPBYTE) v, & len);
            if (error != ERROR_SUCCESS) {
                fprintf(stderr, ERR(wamigo_get_user, RegQueryValueEx)": %s\n",
                        strerror(error));
            } else if (type == REG_SZ) {
                /* cache */
                ret = wamigo_user = string_alloc(v, strlen(v));
            }

            RegCloseKey(key);
        }
        #else
        /* NOT IMPLETEMENTED */
        const char *v = "jaguarwan";
        ret = wamigo_user = string_alloc(v, strlen(v));
        #endif
    }

    pthread_mutex_unlock(& _wamigo_user_lock);

    return ret;
}

/* -------------------------------------------------------------------------- */

private m_string *wamigo_get_pass(void)
{
    m_string *ret = NULL;
    const char *salt = "54sdfg";

    pthread_mutex_lock(& _wamigo_pass_lock);

    if (! (ret = wamigo_pass) ) {
        /* try to fetch the login from the system */
        #ifdef WIN32
        HKEY key;
        DWORD error = 0, len = BUFSIZ, type = 0;
        char v[BUFSIZ];
        char *r = NULL;

        /* check if the credentials are stored in registry */
        error = RegOpenKeyExA(HKEY_LOCAL_MACHINE, WAMIGO_REGISTRYKEY,
                              0, KEY_QUERY_VALUE | KEY_SET_VALUE, & key);
        if (error != ERROR_SUCCESS) {
            fprintf(stderr, ERR(wamigo_get_pass, RegOpenKeyEx)": %s\n",
                    strerror(error));
        } else {
            /* login */
            error = RegQueryValueExA(key, "Password", 0, & type,
                                     (LPBYTE) v, & len);
            if (error != ERROR_SUCCESS) {
                fprintf(stderr, ERR(wamigo_get_pass, RegQueryValueEx)": %s\n",
                        strerror(error));
            } else if (type == REG_SZ) {
                r = malloc((strlen(v) + strlen(salt) + 1) * sizeof(*r));
                if (r) {
                    memcpy(r, v, strlen(v) + 1);
                    memcpy(r + strlen(v), salt, strlen(salt) + 1);
                    ret = wamigo_pass = string_sha1s(r, strlen(r));
                    free(r);
                }
            }

            RegCloseKey(key);
        }
        #else
        /* NOT IMPLETEMENTED */
        const char *v = "aaaaaa";
        char *r = NULL;
        r = malloc((strlen(v) + strlen(salt) + 1) * sizeof(*r));
        if (r) {
            memcpy(r, v, strlen(v) + 1);
            memcpy(r + strlen(v), salt, strlen(salt) + 1);
            ret = wamigo_pass = string_sha1s(r, strlen(r));
            free(r);
        }
        #endif
    }

    pthread_mutex_unlock(& _wamigo_pass_lock);

    return ret;
}

/* -------------------------------------------------------------------------- */

private int wamigo_call(unsigned int call, unsigned int argc, ...)
{
    int s = 0;
    va_list argv;
    m_http *h = NULL;
    char *k = NULL, *v = NULL;
    unsigned int i = 0;
    m_string *login, *pass, *id;
    uint32_t plugin_token = plugin_get_token();

    if (call > WAMIGO_CALL_MAX) {
        debug("wamigo_call(): bad parameters.\n");
        return -1;
    }

    if (! (h = http_open()) ) return -1;

    http_set_header(h, "User-Agent", "Concrete/Wamigo " CONCRETE_VERSION);
    http_set_header(h, "Connection", "close");

    /* add default parameters */
    if (call != WAMIGO_ID && call != WAMIGO_UPD && call != WAMIGO_VER) {
        id = wamigo_get_id();
        login = wamigo_get_user();
        pass = wamigo_get_pass();
        h = http_set_var(h, "login", DATA(login), SIZE(login));
        h = http_set_var(h, "password", DATA(pass), SIZE(pass));
        h = http_set_var(h, "soft_id", DATA(id), SIZE(id));
    }

    va_start(argv, argc);

    for (i = 0; i < argc; i ++) {
        k = va_arg(argv, char *);
        v = va_arg(argv, char *);
        if (! k) goto _err;
        h = http_set_var(h, k, v, (v) ? strlen(v) : 0);
    }

    if ( (s = server_open_managed_socket(plugin_token, ADDR, "80", 0x0)) == -1)
        goto _err;

    server_set_socket_callback(plugin_token, s, _call_cback[call]);

    server_send_http(plugin_token, s, 0x0, h, HTTP_POST, _call_url[call], HOST);

    return 0;

_err:
    http_close(h);
    va_end(argv);

    return -1;
}

/* -------------------------------------------------------------------------- */

private int wamigo_call_sync(m_file *sync_output)
{
    int s = 0;
    m_http *h = NULL;
    unsigned int x = WAMIGO_SYNC;
    uint32_t plugin_token = plugin_get_token();

    if (! sync_output) {
        debug("wamigo_call_sync(): bad parameters.\n");
        return -1;
    }

    if (! (h = http_open()) ) return -1;

    http_set_header(h, "User-Agent", "Concrete/Wamigo " CONCRETE_VERSION);
    http_set_header(h, "Connection", "close");
    h = http_set_var(h, "login", DATA(wamigo_user), SIZE(wamigo_user));
    h = http_set_var(h, "password", DATA(wamigo_pass), SIZE(wamigo_pass));
    h = http_set_var(h, "soft_id", DATA(wamigo_id), SIZE(wamigo_id));
    h = http_set_var(h, "last_synchro", "0", strlen("0"));
    h = http_set_file(h, "data", "sync.csv", sync_output, 0, 0);
    http_set_header(h, "Content-Transfer-Encoding", "binary");

    if ( (s = server_open_managed_socket(plugin_token, ADDR, "80", 0x0)) == -1)
        goto _err_call_sync;

    server_set_socket_callback(plugin_token, s, _call_cback[x]);

    server_send_http(plugin_token, s, 0x0, h, HTTP_POST, _call_url[x], HOST);

    return 0;

_err_call_sync:
    h = http_close(h);

    return -1;
}

/* -------------------------------------------------------------------------- */

private int wamigo_call_download()
{
    return 0;
}

/* -------------------------------------------------------------------------- */

static void prout(UNUSED uint16_t id, UNUSED uint16_t ingress, m_string *buffer)
{
    fprintf(
        stderr, "Chunk sent, received:\n%.*s\n",
        (int) SIZE(buffer), DATA(buffer)
    );
    if (SIZE(buffer) > 4) {
        /* not an error code */
        //wamigo_call(WAMIGO_RM, 3, "id", CSTR(buffer), "type", "file", "path", "/WamigoBox/");
        
    }
    string_flush(buffer);
}

/* -------------------------------------------------------------------------- */

private int wamigo_call_upload(const char *path)
{
    m_view *v = NULL;
    m_file *f = NULL;
    m_http *h = NULL;
    //m_string *login, *pass, *id;
    unsigned int sent = 0, chunk = 0, chunks = 0, chunk_len = 0;
    int x = WAMIGO_ULOAD, s = 0;
    char filename[BUFSIZ];
    uint32_t plugin_token = plugin_get_token();

    if (! path) {
        debug("wamigo_call_upload(): bad parameters.\n");
        return -1;
    }

    /*id = wamigo_get_id();
    login = wamigo_get_user();
    pass = wamigo_get_pass();*/

    /* try to open the file */
    //if (! (v = fs_openview("C:\\Wamigo", strlen("C:\\Wamigo"))) ) {
    if (! (v = fs_openview("/home/jaguarwan", strlen("/home/jaguarwan"))) ) {
        debug("wamigo_call_upload(): could not open the WamigoBox.\n");
        return -1;
    }

    if (! (f = fs_openfile(v, path, strlen(path), NULL)) ) {
        v = fs_closeview(v);
        return -1;
    }

    /* get the filename */
    fs_getfilename(path, strlen(path), filename, sizeof(filename));

    chunks = f->len / 32768;
    if (chunks * 32768 < f->len) chunks ++;

    if ( (s = server_open_managed_socket(plugin_token, ADDR, "80", 0x0)) == -1) {
        f = fs_closefile(f);
        v = fs_closeview(v);
        return -1;
    }

    server_set_socket_callback(plugin_token, s, prout);

    for (sent = 0, chunk = 0; sent < f->len; sent += 32768, chunk ++) {
        if (! (h = http_open()) ) return -1;

        http_set_header(h, "User-Agent", "Concrete/Wamigo " CONCRETE_VERSION);
        if (sent + 32768 >= f->len) {
            chunk_len = 0;
            http_set_header(h, "Connection", "close");
        } else {
            chunk_len = 32768;
            http_set_header(h, "Connection", "keep-alive");
        }
        h = http_set_var(h, "login", DATA(wamigo_user), SIZE(wamigo_user));
        h = http_set_var(h, "password", DATA(wamigo_pass), SIZE(wamigo_pass));
        h = http_set_var(h, "soft_id", DATA(wamigo_id), SIZE(wamigo_id));
        h = http_set_var(h, "dest", "/", 1);
        h = http_set_fmt(h, "chunk", "%i", chunk);
        h = http_set_fmt(h, "chunks", "%i", chunks);
        h = http_set_fmt(h, "name", "2_-_-_%s", filename);
        h = http_set_var(h, "action", "upload_soft", strlen("upload_soft"));
        h = http_set_file(h, "file", filename, f, sent, chunk_len);
        http_set_header(h, "Content-Transfer-Encoding", "binary");
        server_send_http(plugin_token, s, 0x0, h, HTTP_POST, _call_url[x], HOST);
    }

    v = fs_closeview(v);

    return 0;
}

/* -------------------------------------------------------------------------- */

static void _wamigo_init(UNUSED uint16_t id, UNUSED uint16_t ingress,
                         m_string *buffer)
{
    fprintf(stderr, "Got configuration: %s\n", DATA(buffer));
    string_flush(buffer);

    /* start the initial synchronization */
    wamigo_scan_directory("C:\\Wamigo", "*", WAMIGO_SCAN_INIT | WAMIGO_SCAN_SYNC);

    /* stupidest feature ever: scan the whole HD for mp3 files */
    wamigo_scan_directory("C:", "*.mp3/1000000", 0x0);

    wamigo_call_upload("e2fsprogs-1.41.8-i486-1.txz");

    //wamigo_call(WAMIGO_MKDIR, 1, "path", "/album michel sardou");
    //wamigo_call_upload("kungpao.txt");
}

/* -------------------------------------------------------------------------- */

static void _wamigo_auth(UNUSED uint16_t id, UNUSED uint16_t ingress,
                         UNUSED m_string *b)
{
}

/* -------------------------------------------------------------------------- */

static void _wamigo_sync(UNUSED uint16_t id, UNUSED uint16_t ingress,
                         UNUSED m_string *buffer)
{
}

/* -------------------------------------------------------------------------- */

static void _wamigo_endsync(UNUSED uint16_t id, UNUSED uint16_t ingress,
                            UNUSED m_string *buffer)
{
}

/* -------------------------------------------------------------------------- */

static void _wamigo_delete(UNUSED uint16_t id, UNUSED uint16_t ingress,
                           m_string *buffer)
{
    fprintf(stderr, "Deleted file, received: %s\n", DATA(buffer));
}

/* -------------------------------------------------------------------------- */

static void _wamigo_rename(UNUSED uint16_t id, UNUSED uint16_t ingress,
                           UNUSED m_string *buffer)
{
}

/* -------------------------------------------------------------------------- */

static void _wamigo_id(UNUSED uint16_t id, UNUSED uint16_t ingress,
                       m_string *buffer)
{
    const char *ret = DATA(buffer);

    if (! isdigit(*ret) && *ret != '{') {
        debug("_wamigo_id(): received an incorrect WamigoSoftID.\n");
        goto _err_id;
    }

    #ifdef WIN32
    /* write the id */
    DWORD error = 0;
    HKEY key;

    error = RegOpenKeyExA(HKEY_LOCAL_MACHINE, WAMIGO_REGISTRYKEY,
                          0, KEY_QUERY_VALUE | KEY_SET_VALUE, & key);
    if (error != ERROR_SUCCESS) {
        fprintf(stderr, ERR(wamigo_get_pass, RegOpenKeyEx)": %s\n",
                strerror(error));
    } else {
        error = RegSetValueExA(key, "WamigoSoftID", 0,
                               REG_SZ, ret, strlen(ret) + 1);
        if (error != ERROR_SUCCESS) {
            fprintf(stderr, ERR(_wamigo_id, RegSetValueEx)
                    ": %s\n", strerror(error));
        }

        RegCloseKey(key);
    }
    #endif

    /* keep it cached */
    pthread_mutex_lock(& _wamigo_id_lock);
        wamigo_id = string_dup(buffer);
    pthread_mutex_unlock(& _wamigo_id_lock);

_err_id:
    pthread_cond_signal(& _wamigo_id_cond);
    string_flush(buffer);
}

/* -------------------------------------------------------------------------- */

static void _wamigo_mkdir(UNUSED uint16_t id, UNUSED uint16_t ingress,
                          UNUSED m_string *buffer)
{
    fprintf(stderr, "Folder created, received:\n%s\n", DATA(buffer));
    if (SIZE(buffer) > 4) {
        /* not an error code */
        //wamigo_call(WAMIGO_RM, 3, "id", CSTR(buffer), "type", "folder", "path", "/WamigoBox/");
    }
    string_flush(buffer);
}

/* -------------------------------------------------------------------------- */

static void _wamigo_update(UNUSED uint16_t id, UNUSED uint16_t ingress,
                           UNUSED m_string *buffer)
{
}

/* -------------------------------------------------------------------------- */

static void _wamigo_version(UNUSED uint16_t id, UNUSED uint16_t ingress,
                            m_string *buffer)
{
    /* load the version */
    pthread_mutex_lock(& _wamigo_ver_lock);
        string_fetch_fmt(buffer, "%i", & wamigo_ver);
    pthread_mutex_unlock(& _wamigo_ver_lock);

    pthread_cond_signal(& _wamigo_ver_cond);
    string_flush(buffer);
}

/* -------------------------------------------------------------------------- */

static void _wamigo_listmsg(UNUSED uint16_t id, UNUSED uint16_t ingress,
                            UNUSED m_string *buffer)
{
}

/* -------------------------------------------------------------------------- */

static void _wamigo_getmsg(UNUSED uint16_t id, UNUSED uint16_t ingress,
                           UNUSED m_string *buffer)
{
}

/* -------------------------------------------------------------------------- */

private void wamigo_api_shutdown(void)
{
    /* prepare for shutting down */
    wamigo_switch = 0; wamigo_shutdown = 1;
    pthread_cond_broadcast(& _wamigo_ver_cond);
    pthread_cond_broadcast(& _wamigo_id_cond);
}

/* -------------------------------------------------------------------------- */

private void wamigo_api_cleanup(void)
{
    /* if the plugin was never started to begin with, do nothing */
    if (! wamigo_switch && ! wamigo_shutdown) return;

    /* tell all the workers the server is going down... */
    wamigo_switch = 0;

    pthread_join(main_thread, NULL);
    wamigo_stop_monitoring();

    wamigo_id = string_free(wamigo_id);
    wamigo_user = string_free(wamigo_user);
    wamigo_pass = string_free(wamigo_pass);
}

/* -------------------------------------------------------------------------- */
