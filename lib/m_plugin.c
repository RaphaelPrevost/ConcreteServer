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

#include "m_plugin.h"

/* -------------------------------------------------------------------------- */
#ifdef _ENABLE_SERVER
/* -------------------------------------------------------------------------- */

/**
 * @var _plugin
 *
 * This private array contains all the dynamic shared objects loaded in
 * the Mammouth server. Many of the plugin related functions use it
 * for direct access, but it must NEVER be tampered with without
 * proper locking.
 *
 * An acquired plugin is always readlocked, so as to prevent the destruction
 * of a plugin while it is in use; plugin destruction requires writelocking
 * the plugin.
 *
 */

static pthread_rwlock_t _plugin_lock = PTHREAD_RWLOCK_INITIALIZER;
static m_plugin *_plugin[PLUGIN_MAX + 1];

/** @var _plugin_path
 *
 * This private string tells the plugin api where to look for plugins if
 * it is set.
 *
 */

static char *_plugin_path = NULL;
static size_t _plugin_path_len = 0;

/* -------------------------------------------------------------------------- */

private int plugin_api_setup(void)
{
    return 0;
}

/* -------------------------------------------------------------------------- */

static int _plugin_reg(m_plugin *p)
{
    unsigned int i = 0;

    if (! p) return -1;

    p->_id = -1;

    pthread_rwlock_wrlock(& _plugin_lock);

    for (i = 1; i <= PLUGIN_MAX; i ++)
        if (! _plugin[i]) { _plugin[i] = p; p->_id = i; break; }

    pthread_rwlock_unlock(& _plugin_lock);

    return (p->_id == (unsigned int) -1) ? -1 : 0;
}

/* -------------------------------------------------------------------------- */

static m_plugin *_plugin_dereg(m_plugin *p)
{
    m_plugin *ret = NULL;

    if (! p) return NULL;

    if (p->_id > (sizeof(_plugin) / sizeof(p))) return NULL;

    /* acquire the plugin which should be destroyed */
    pthread_rwlock_wrlock(p->_lock);

    /* remove it from the plugin array */
    pthread_rwlock_wrlock(& _plugin_lock);

    if (_plugin[p->_id] == p) {
        ret = _plugin[p->_id];
        _plugin[p->_id] = NULL;
    }

    pthread_rwlock_unlock(& _plugin_lock);

    return ret;
}

/* -------------------------------------------------------------------------- */

private int plugin_open(const char *path, const char *name)
{
    m_plugin *p = NULL;
    unsigned int (*plugin_api)(void) = NULL;
    char *fullpath = NULL;
    int ret = -1;
    size_t namelen = 0;

    /* check if a name was provided */
    namelen = (name) ? strlen(name) : 1;

    if (! (p = malloc(sizeof(*p) + namelen + 1)) ) {
        perror(ERR(plugin_open, malloc));
        return -1;
    }

    p->_handle = NULL;
    p->_status = -1;

    /* set the plugin name (NUL string if none provided) */
    if (name) memcpy(p->name, name, namelen);
    p->name[namelen] = 0;

    if (! (p->_lock = malloc(sizeof(*p->_lock))) ) {
        perror(ERR(plugin_open, malloc));
        goto _err_malloc;
    }

    pthread_rwlock_init(p->_lock, NULL);

    /* acquire the plugin to ensure ownership */
    pthread_rwlock_wrlock(p->_lock);

    if (_plugin_reg(p) == -1) {
        fprintf(stderr, ERR(plugin_open, _plugin_reg)
                        ": plugin registration failed.\n");
        goto _err_reg;
    }

    /* map the shared library in the process address space */
    if (path != __PLUGIN_BUILTIN__) {
        if (path && (fullpath = plugin_getpath(path, strlen(path))) ) {
            p->_handle = dlopen(fullpath, RTLD_LAZY);
            free(fullpath);
            if (! p->_handle) {
                fprintf(stderr, ERR(plugin_open, dlopen)": %s\n", dlerror());
                goto _err_dlopen;
            }
        } else goto _err_dlopen;
    } else {
        #ifndef WIN32
        if (! (p->_handle = dlopen(NULL, RTLD_LAZY)) ) {
        #else
        if (! (p->_handle = GetModuleHandle(NULL)) ) {
        #endif
            fprintf(stderr,
                    ERR(plugin_open, GetModuleHandle)": %s\n", dlerror());
            goto _err_dlopen;
        }
    }

    /* load mandatory symbols */
    if (! (plugin_api = (unsigned int (*)(void))
           dlfunc(p->_handle, "plugin_api"))
       ) goto _err_dlget;

    /* check the required API revision */
    if (plugin_api() > __CONCRETE__) {
        fprintf(stderr, "plugin_open(): %s uses a newer API revision.\n", path);
        goto _err_dlsym;
    }

    if (! (p->plugin_init = (int (*)(uint32_t, int, char **))
                             dlfunc(p->_handle, "plugin_init"))
       ) goto _err_dlget;

    if (! (p->plugin_fini = (void (*)(void))
                             dlfunc(p->_handle, "plugin_fini"))
       ) goto _err_dlget;

    if (! (p->plugin_main = (void (*)(uint16_t, uint16_t, m_string *))
                             dlfunc(p->_handle, "plugin_main"))
       ) goto _err_dlget;

    /* optional symbol, plugin interrupt handler */
    p->plugin_intr = (void (*)(uint16_t, uint16_t, int))
                      dlfunc(p->_handle, "plugin_intr");

    #ifdef WIN32
    /* if we opened the calling process, we need to invalidate the handle
       to avoid a disaster */
    if (path == __PLUGIN_BUILTIN__) p->_handle = NULL;
    #endif

    /* fetch the ID */
    ret = p->_id;

    /* unlock the plugin for public use */
    pthread_rwlock_unlock(p->_lock);

    return ret;

_err_dlget:
    fprintf(stderr, ERR(plugin_open, dlsym)": %s\n", dlerror());
_err_dlsym:
    dlclose(p->_handle);
_err_dlopen:
    _plugin_dereg(p);
_err_reg:
    free(p->_lock);
_err_malloc:
    free(p);
    return -1;
}

/* -------------------------------------------------------------------------- */

private int plugin_getid(const char *name)
{
    int i = 0, id = -1;

    if (! name) {
        debug("plugin_getid(): bad parameters.\n");
        return -1;
    }

    pthread_rwlock_rdlock(& _plugin_lock);

        for (i = 0; i < PLUGIN_MAX; i ++) {
            if (_plugin[i] && ! strcmp(_plugin[i]->name, name)) {
                id = i; break;
            }
        }

    pthread_rwlock_unlock(& _plugin_lock);

    return id;
}

/* -------------------------------------------------------------------------- */

private int plugin_start(unsigned int id, int argc, char **argv)
{
    m_plugin *p = NULL;
    int ret = -1;

    if (id > PLUGIN_MAX) {
        debug("plugin_start(): bad parameters.\n");
        return -1;
    }

    pthread_rwlock_rdlock(& _plugin_lock);

        /* exclusive lock */
        if ( (p = _plugin[id]) ) pthread_rwlock_wrlock(p->_lock);

    pthread_rwlock_unlock(& _plugin_lock);

    if (p && p->_id == id && p->_status == -1)
        ret = p->_status = p->plugin_init((p->_id << _SOCKET_RSS), argc, argv);

    if (p) pthread_rwlock_unlock(p->_lock);

    return ret;
}

/* -------------------------------------------------------------------------- */

private void plugin_call(const char* name, int call, ...)
{
    int plugin_id = 0;
    m_plugin *p = NULL;
    uint16_t socket_id = 0;
    uint16_t ingress_id = 0;
    m_string *buffer = NULL;
    unsigned int event = 0;
    va_list ap;

    /* check if the name matches an existing plugin */
    if ( (plugin_id = plugin_getid(name)) == -1) {
        debug("plugin_call(): plugin not found.\n");
        return;
    }

    if (! (p = plugin_acquire(plugin_id)) ) {
        debug("plugin_call(): cannot acquire the plugin.\n");
        return;
    }

    va_start(ap, call);

    switch (call) {

    case PLUGIN_MAIN: {
        socket_id = va_arg(ap, int);
        ingress_id = va_arg(ap, int);
        buffer = va_arg(ap, m_string *);
        p->plugin_main(socket_id, ingress_id, buffer);
    } break;

    case PLUGIN_INTR: {
        socket_id = va_arg(ap, int);
        ingress_id = va_arg(ap, int);
        event = va_arg(ap, int);
        /* plugin_intr is optional */
        if (p->plugin_intr) p->plugin_intr(socket_id, ingress_id, event);
    } break;

    }

    va_end(ap);

    p = plugin_release(p);

    return;
}

/* -------------------------------------------------------------------------- */

private int plugin_setpath(const char *path, size_t len)
{
    char *p = NULL;

    if (! path || ! len) {
        debug("plugin_setpath(): bad parameters.\n");
        return -1;
    }

    if (access(path, R_OK) == -1) {
        perror(ERR(plugin_setpath, access));
        return -1;
    }

    if (! (p = malloc( (len + 1) * sizeof(*p))) ) {
        perror(ERR(plugin_setpath, malloc));
        return -1;
    }

    memcpy(p, path, len + 1);

    pthread_rwlock_wrlock(& _plugin_lock);

    if (_plugin_path) free(_plugin_path);
    _plugin_path = p; _plugin_path_len = len;

    pthread_rwlock_unlock(& _plugin_lock);

    return 0;
}

/* -------------------------------------------------------------------------- */

private char *plugin_getpath(const char *plugin, size_t len)
{
    char *ret = NULL;
    size_t off = 0;

    if (! plugin || ! len) {
        debug("plugin_getpath(): bad parameters.\n");
        return NULL;
    }

    #ifdef _ENABLE_FILE
    if (! fs_isrelativepath(plugin, len)) {
        debug("plugin_getpath(): %s is out of plugin root directory.\n", plugin);
        return NULL;
    }
    #endif

    pthread_rwlock_rdlock(& _plugin_lock);

    if (! _plugin_path) {
        pthread_rwlock_unlock(& _plugin_lock);
        return NULL;
    }

    if (! (ret = malloc( (_plugin_path_len + len + 2) * sizeof(*ret))) ) {
        perror(ERR(plugin_getpath, malloc));
        pthread_rwlock_unlock(& _plugin_lock);
        return NULL;
    }

    memcpy(ret, _plugin_path, _plugin_path_len + 1);
    off = _plugin_path_len;

    pthread_rwlock_unlock(& _plugin_lock);

    if (ret[off] != DIR_SEP_CHR) ret[off ++] = DIR_SEP_CHR;
    memcpy(ret + off, plugin, len + 1);

    return ret;
}

/* -------------------------------------------------------------------------- */

public const char *plugin_getopt(const char *opt, int argc, char **argv)
{
    int i = 0;

    if (! opt || ! argv || argc <= 0) return NULL;

    for (i = 0; i < argc; i ++) {
        if (argv[i] && i + 1 < argc && ! strcmp(argv[i], opt))
            return argv[i + 1];
    }

    return NULL;
}

/* -------------------------------------------------------------------------- */

public const char *plugin_getarrayopt(const char *opt, int index, int argc,
                                      char **argv)
{
    int i = 0;
    size_t optlen = 0, curlen = 0;
    char buffer[BUFSIZ];

    if (! opt || ! argv || argc <= 0) return NULL;

    optlen = strlen(opt);

    for (i = 0; i < argc; i ++) {
        /* check if there is an option */
        if (! argv[i] || i + 1 == argc) continue;

        /* check if the length can match */
        if ( (curlen = strlen(argv[i])) < optlen) continue;

        /* allow syntax without index for [0] */
        if (index == 0 && curlen == optlen) {
            if (! memcmp(argv[i], opt, optlen))
                return argv[i + 1];
        } else {
            /* index is mandatory */
            snprintf(buffer, sizeof(buffer), "%s[%i]", opt, index);
            if (! strcmp(argv[i], buffer))
                return argv[i + 1];
        }
    }

    return NULL;
}

/* -------------------------------------------------------------------------- */

public int plugin_getboolopt(const char *opt, int argc, char **argv)
{
    const char *optval = plugin_getopt(opt, argc, argv);
    return (optval && atoi(optval) == 1) ? 1 : 0;
}

/* -------------------------------------------------------------------------- */

private int plugin_exists(unsigned int id)
{
    if (id > PLUGIN_MAX) {
        debug("plugin_exists(): bad parameters.\n");
        return -1;
    }

    /* simple check, no need to lock */
    return (_plugin[id] != NULL);
}

/* -------------------------------------------------------------------------- */

private m_plugin *plugin_acquire(unsigned int id)
{
    m_plugin *p = NULL;

    if (id > PLUGIN_MAX) {
        debug("plugin_acquire(): bad parameters.\n");
        return NULL;
    }

    pthread_rwlock_rdlock(& _plugin_lock);

        /* lock the plugin to avoid its destruction while in use */
        if ( (p = _plugin[id]) ) pthread_rwlock_rdlock(p->_lock);

    pthread_rwlock_unlock(& _plugin_lock);

    return p;
}

/* -------------------------------------------------------------------------- */

private m_plugin *plugin_release(m_plugin *p)
{
    if (! p) return NULL;

    pthread_rwlock_unlock(p->_lock);

    return NULL;
}

/* -------------------------------------------------------------------------- */

private m_plugin *plugin_close(m_plugin *p)
{
    if (! _plugin_dereg(p)) return NULL;

    /* call the plugin destructor */
    if (p->_status != -1) p->plugin_fini();

    /* unmap the shared object */
    dlclose(p->_handle);

    /* finally destroy the structure */
    pthread_rwlock_unlock(p->_lock);
    pthread_rwlock_destroy(p->_lock);
    free(p->_lock); free(p);

    return NULL;
}

/* -------------------------------------------------------------------------- */

private void plugin_api_shutdown(void)
{
    unsigned int i = 0;

    pthread_rwlock_wrlock(& _plugin_lock);

        for (i = 0; i < PLUGIN_MAX; i ++) {
            if (_plugin[i] && _plugin[i]->plugin_intr) {
                _plugin[i]->plugin_intr(0, 0, PLUGIN_EVENT_SERVER_SHUTTINGDOWN);
            }
        }

    pthread_rwlock_unlock(& _plugin_lock);
}

/* -------------------------------------------------------------------------- */

private void plugin_api_cleanup(void)
{
    unsigned int i = 0;

    for (i = 1; i <= PLUGIN_MAX; i ++) plugin_close(_plugin[i]);

    free(_plugin_path);
}

/* -------------------------------------------------------------------------- */
#else
/* -------------------------------------------------------------------------- */

/* Plugin API will not be compiled in the Concrete Library */
#ifdef __GNUC__
__attribute__ ((unused)) static int __dummy__ = 0;
#endif

/* -------------------------------------------------------------------------- */
#endif
/* -------------------------------------------------------------------------- */
