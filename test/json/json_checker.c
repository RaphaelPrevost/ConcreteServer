#include "../../lib/m_core_def.h"
#include "../../lib/m_string.h"
#include "../../lib/m_trie.h"

typedef struct jsonpath_context {
    m_trie *tree;
    m_string *path;
    uint32_t count;
} jsonpath_context;

static const uint16_t u32toa_lut[] = {
    0x3030,0x3130,0x3230,0x3330,0x3430,0x3530,0x3630,0x3730,0x3830,0x3930,
    0x3031,0x3131,0x3231,0x3331,0x3431,0x3531,0x3631,0x3731,0x3831,0x3931,
    0x3032,0x3132,0x3232,0x3332,0x3432,0x3532,0x3632,0x3732,0x3832,0x3932,
    0x3033,0x3133,0x3233,0x3333,0x3433,0x3533,0x3633,0x3733,0x3833,0x3933,
    0x3034,0x3134,0x3234,0x3334,0x3434,0x3534,0x3634,0x3734,0x3834,0x3934,
    0x3035,0x3135,0x3235,0x3335,0x3435,0x3535,0x3635,0x3735,0x3835,0x3935,
    0x3036,0x3136,0x3236,0x3336,0x3436,0x3536,0x3636,0x3736,0x3836,0x3936,
    0x3037,0x3137,0x3237,0x3337,0x3437,0x3537,0x3637,0x3737,0x3837,0x3937,
    0x3038,0x3138,0x3238,0x3338,0x3438,0x3538,0x3638,0x3738,0x3838,0x3938,
    0x3039,0x3139,0x3239,0x3339,0x3439,0x3539,0x3639,0x3739,0x3839,0x3939
};

/* -------------------------------------------------------------------------- */

static char *u32toa(uint32_t u32, char *out, size_t len)
{
    uint32_t prev = 0;
    uint16_t *p = (void *) (out + 10);

    if (len < 12) {
        debug("u32toa(): bad parameters.\n");
    }

    *p = 0;

    while (u32 >= 100) {
        prev = u32; p --; u32 /= 100;
        *p = u32toa_lut[prev - (u32 * 100)];
    }

    p --; *p = u32toa_lut[u32];

    return (char *) p + (u32 < 10);
}

/* -------------------------------------------------------------------------- */

static uint32_t atou32(const char *in, size_t len)
{
    unsigned int i = 0;
    uint32_t ret = 0;

    for (i = 0; i < len; i ++) ret = (ret * 10) + in[i] - '0';

    return ret;
}

/* -------------------------------------------------------------------------- */

static uint32_t __increment_index(m_string *path)
{
    uint32_t index = 0;
    char *ret = NULL, buffer[12];
    unsigned int buflen;

    /* get the index */
    index = atou32(DATA(LAST_TOKEN(path)), SIZE(LAST_TOKEN(path)));

    /* increment and replace */
    ret = u32toa(++ index, buffer, sizeof(buffer));
    buflen = 10 - (ret - buffer);
    if (likely(buflen == SIZE(LAST_TOKEN(path)))) {
        memcpy((char *) DATA(LAST_TOKEN(path)), ret, buflen);
    } else {
        string_suppr_token(path, PARTS(path) - 1);
        string_enqueue_tokens(path, ret, buflen);
    }

    return index;
}

/* -------------------------------------------------------------------------- */

static void print_tokens(const m_string *s, unsigned int indent)
{
    unsigned int i = 0, j = 0, k = 0;
    const m_string *cur = NULL, *parent = NULL;

    if (! s) return;

    printf(
        "%.*s%s%.*s %s",
        indent, "", (indent) ? " " : "+ ", (int) SIZE(s), DATA(s),
        (IS_OBJECT(s) ? "(object)" :
         IS_ARRAY(s) ? "(array)" :
         IS_STRING(s) ? "(string)" :
         IS_PRIMITIVE(s) ? "(primitive)" : "")
    );
    if (HAS_ERROR(s)) printf(" (!) ");
    if (! IS_TYPE(s, JSON_TYPE))
        printf("(size=%zu)", SIZE(s));
    printf("\n");

    if (s->token) {
        for (i = 0; i < PARTS(s); i ++) {
            for (j = 0; j < indent; j ++) {
                for (parent = s, k = j; k < indent; k ++) {
                    cur = parent; parent = parent->parent;
                }
                printf("%c  ", (LAST_TOKEN(parent) != cur) ? '|' : ' ');
            }
            printf("|-[%i]", i);
            print_tokens(TOKEN(s, i), indent + 1);
        }
    }

    return;
}

/* -------------------------------------------------------------------------- */

int CALLBACK json_init(int type, struct m_json_parser *ctx)
{
    struct jsonpath_context *context = ctx->context;

    if (ctx->key.current) {
        string_enqueue_tokens(context->path, ctx->key.current, ctx->key.len);
    }

    if (type == JSON_ARRAY) {
        /* add the index token */
        string_enqueue_tokens(context->path, "0", 1);
        context->count = 0;
    }

    return 0;
}

/* -------------------------------------------------------------------------- */

int CALLBACK json_exit(int type, struct m_json_parser *ctx)
{
    struct jsonpath_context *context = ctx->context;

    if (type == JSON_ARRAY) {
        /* remove the index token */
        string_suppr_token(context->path, PARTS(context->path) - 1);

        if (! context->count) {
            /* insert nonetheless */
            string_merges(context->path, "/", 1);
            trie_insert(
                context->tree,
                DATA(context->path), SIZE(context->path),
                NULL
            );
        }
    } else if (type == JSON_OBJECT && LAST_CHAR(context->path) != '/') {
        trie_insert(
            context->tree,
            DATA(context->path), SIZE(context->path),
            NULL
        );
    }

    /* remove the array name */
    if (ctx->parent == JSON_OBJECT && PARTS(context->path))
        string_suppr_token(context->path, PARTS(context->path) - 1);

    if (unlikely(! PARTS(context->path))) context->path->_len = 0;

    if (ctx->parent == JSON_ARRAY)
        context->count = __increment_index(context->path);

    return 0;
}

/* -------------------------------------------------------------------------- */

int CALLBACK json_data(UNUSED int type, const char *data,
                       size_t len, struct m_json_parser *ctx)
{
    struct jsonpath_context *context = ctx->context;

    if (ctx->parent == JSON_ARRAY) {
        string_merges(context->path, "/", 1);
        trie_insert(
            context->tree,
            DATA(context->path), SIZE(context->path),
            NULL
        );
        context->count = __increment_index(context->path);
    } else if (ctx->key.current) {
        string_enqueue_tokens(context->path, ctx->key.current, ctx->key.len);
        string_merges(context->path, "/", 1);
        trie_insert(
            context->tree,
            DATA(context->path), SIZE(context->path),
            NULL
        );
        if (likely(PARTS(context->path)))
            string_suppr_token(context->path, PARTS(context->path) - 1);
        if (unlikely(! PARTS(context->path))) context->path->_len = 0;
    }

    return 0;
}

/* -------------------------------------------------------------------------- */

int trie_print(const char *k, size_t len, UNUSED void *val)
{
    printf("%.*s\n", (int) len, k);
    return 0;
}

/* -------------------------------------------------------------------------- */

int main(int argc, char **argv)
{
    char *src = NULL;
    ssize_t len = 0;
    m_string *json = NULL;
    unsigned int i = 0;
    struct stat info;
    int fd = -1, ret = 0;
    struct jsonpath_context context;

    if (argc > 1) {
        if (stat(argv[1], & info) == -1) {
            fprintf(stderr, "%s: failed to stat %s.\n", argv[0], argv[1]);
            exit(EXIT_FAILURE);
        }

        if ( (fd = open(argv[1], O_RDONLY)) == -1) {
            fprintf(stderr, "%s: failed to open %s.\n", argv[0], argv[1]);
            exit(EXIT_FAILURE);
        }

        len = info.st_size;
        src = mmap(NULL, len, PROT_READ, MAP_PRIVATE | MAP_POPULATE, fd, 0);

        if (! src) {
            fprintf(stderr, "%s: failed to map the file.\n", argv[0]);
            exit(EXIT_FAILURE);
        }
    } else {
        if (! (src = malloc(BUFSIZ)) ) {
            fprintf(stderr, "%s: cannot allocate buffer.\n", argv[0]);
            exit(EXIT_FAILURE);
        }

        if ( (len = read(STDIN_FILENO, src, BUFSIZ)) == -1) {
            fprintf(stderr, "%s: cannot read stdin.\n", argv[0]);
            exit(EXIT_FAILURE);
        }
    }

    json = string_encaps(src, len);

    context.tree = trie_alloc(free);
    context.path = string_alloc(NULL, 0);

    m_json_parser ctx;
    ctx.context = & context;
    ctx.key.current = NULL;
    ctx.key.len = 0;
    ctx.init = json_init;
    ctx.data = json_data;
    ctx.exit = json_exit;

    do {
        ret = string_parse_json(json, JSON_STRICT, & ctx);
        printf("Parsing interrupted.\n");
    } while (ret > 0);

    if (argc == 1) trie_foreach(context.tree, trie_print);

    //trie_free(context.tree);

    if (ret == 0 && json->token) {
        /* check if there is errors */
        for (i = 0; i < PARTS(json); i ++) {
            if (HAS_ERROR(TOKEN(json, i))) {
                fprintf(stderr, "%s: incomplete input.\n", argv[0]);
                if (argc == 1) print_tokens(json, 0);
                exit(EXIT_FAILURE);
            }
        }
        if (argc == 1) print_tokens(json, 0);
        exit(EXIT_SUCCESS);
    }

    fprintf(stderr, "%s: parse error.\n", argv[0]);
    exit(EXIT_FAILURE);
}

/* -------------------------------------------------------------------------- */
