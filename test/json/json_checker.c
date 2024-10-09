/*******************************************************************************
 *  Concrete Server                                                            *
 *  Copyright (c) 2005-2024 Raphael Prevost <raph@el.bzh>                      *
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

#include "../../lib/m_core_def.h"
#include "../../lib/m_string.h"
#include "../../lib/m_parser.h"

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

int trie_print(const char *k, size_t len, void *val)
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

    m_json_parser ctx;
    
    jsonpath_init(& ctx);

    do {
        ret = string_parse_json(json, JSON_STRICT, & ctx);
        printf("Parsing interrupted.\n");
    } while (ret > 0);

    //if (argc == 1) trie_foreach(context.tree, trie_print);

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
