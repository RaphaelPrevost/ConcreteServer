/* -------------------------------------------------------------------------- */
#ifdef _ENABLE_HTTP
/* -------------------------------------------------------------------------- */

#include "../lib/m_server.h"
#include "../lib/m_http.h"
#include "../lib/m_file.h"

int test_http(void)
{
    m_http *h = NULL;
    m_string *request = NULL;
    unsigned int i = 0, j = 0;
    m_string *z = NULL, *w = NULL, *m = NULL;
    clock_t start, stop;
    int http_status = 0;
    m_view *v = NULL;
    m_file *f = NULL;
    const char *frag = "HTTP/1.1 200 OK\r\nContent-";
    const char *pipeline = "Length: 1\r\n\r\n0HTTP 200 OK\r\nContent-Length: 4\r\nContent-Type: text/plain\r\n\r\nEFIE HTTP/CONNECT /url/test.html HTTP/1.1\r\nHost: lapin.com\r\n\r\nHTTPHTTP 200 OK\r\nContent-Length: 8\r\nContent-Type: text/html\r\n\r\n12345678GET /url/lapin%20malin.pdf HTTP/1.0\r\nHost: lapin.com\r\n\r\nbeginning of some requestHTTP/1.1 200 OK\r\nHost: lapin.com\r\n\r\ni put some content without content length!yeah!HTTP/1.1 200 OK\r\nContent-Length: 1\r\n\r\nQHTTP/1.1 200 OK\r\nContent-Length: 5\r\n\r\nABCDEHTTP/1.1 200 OK\r\nContent-Length: 31\r\n\r\nsome text and enclosed HTTP aaaHTTP/1.1 200 OK\r\nHost: lapin.com\r\nsome content, but request is bad\r\nHTTP/1.1 200 OK\r\nContent-Length: 14\r\n\r\nGood request !HTTP/1.1 200 OK\r\nContent-";

    h = http_open();
    http_set_header(h, "User-Agent", "Mozilla");
    http_set_header(h, "Accept", "*/*");
    h = http_set_var(h, "test", "true", strlen("true"));
    http_set_header(h, "Content-Transfer-Encoding", "binary");
    h = http_set_var(h, "parm", "1", 1);
    http_set_header(h, "Content-Transfer-Encoding", "binary");
    request = http_format(h, HTTP_POST, "/form.php", "www.example.com");
    printf("%s\n", CSTR(request));
    for (i = 0; i < request->parts; i ++)
        printf("%i: %.*s\n", i + 1, (int) TLEN(request, i), TSTR(request, i));
    request = string_free(request);
    http_close(h);

    fprintf(stderr, "=======================\n");

    v = fs_openview("lib", strlen("lib"));
    f = fs_openfile(v, "m_http.c", strlen("m_http.c"), NULL);
    h = http_open();
    http_set_header(h, "User-Agent", "Mozilla");
    http_set_header(h, "Accept", "*/*");
    h = http_set_var(h, "test", "true", strlen("true"));
    http_set_header(h, "Content-Transfer-Encoding", "binary");
    h = http_set_file(h, "parm", "filename", f, 0, 100);
    http_set_header(h, "Content-Transfer-Encoding", "binary");
    request = http_format(h, HTTP_POST, "/form.php", "www.example.com");
    printf("%s\n", CSTR(request));
    for (i = 0; i < request->parts; i ++)
        printf("%i: %.*s\n", i + 1, (int) TLEN(request, i), TSTR(request, i));
    request = string_free(request);
    http_close(h);
    f = fs_closefile(f);
    v = fs_closeview(v);

    m = string_alloc(frag, strlen(frag));
    z = string_alloc(pipeline, strlen(pipeline));
    while ( (w = http_get_request(& http_status, & m, z)) ) {
        if (w) fprintf(stderr, "Processing 0 [%zu] %.*s\n",
                       SIZE(w), (int) SIZE(w), CSTR(w));
        string_flush(w);
    }
    z = string_free(z);

    start = clock();
    for (i = 0; i < 10; i ++) {
        http_status = 0;
        z = string_alloc(pipeline, strlen(pipeline));

        while ( (w = http_get_request(& http_status, & m, z)) ) {
            if (w) fprintf(stderr, "Processing 1 [%zu] %.*s\n",
                           SIZE(w), (int) SIZE(w), CSTR(w));
            string_flush(w);
        }
        z = string_free(z);
    }
    stop = clock();
    printf("(-) Time elapsed = ");
    printf("%.3f", (double)( stop - start ) / CLOCKS_PER_SEC);
    printf(" s.\n");

    fprintf(stderr, "Frag length: %zu\n", SIZE(m));
    m = string_free(m);
    z = string_free(z);

    fprintf(stderr, "====\n");
    http_status = 0;
    z = string_alloc("HTTP 200 OK\r\nContent-", strlen("HTTP 200 OK\r\nContent-"));
    w = http_get_request(& http_status, & m, z);
    if (w) fprintf(stderr, "input=[%zu] %.*s\n",
                   SIZE(w), (int) SIZE(w), CSTR(w));
    if (m) fprintf(stderr, "frag=[%zu] %.*s\n",
                   SIZE(m), (int) SIZE(m), CSTR(m));

    z = string_free(z);
    w = string_free(w);
    m = string_free(m);

    http_status = 0;
    m = string_alloc("HTTP 200 OK\r\nContent-", strlen("HTTP 200 OK\r\nContent-"));
    z = string_alloc("Length: 4\r\n\r\nALLO", strlen("Length: 4\r\n\r\nALLO"));
    w = http_get_request(& http_status, & m, z);
    if (w) fprintf(stderr, "input=[%zu] %.*s\n",
                   SIZE(w), (int) SIZE(w), CSTR(w));
    if (m) fprintf(stderr, "frag=[%zu] %.*s\n",
                   SIZE(m), (int) SIZE(m), CSTR(m));
    w = http_get_request(& http_status, & m, z);
    if (w) fprintf(stderr, "input=[%zu] %.*s\n",
                   SIZE(w), (int) SIZE(w), CSTR(w));
    if (m) fprintf(stderr, "frag=[%zu] %.*s\n",
                   SIZE(m), (int) SIZE(m), CSTR(m));

    z = string_free(z);
    w = string_free(w);
    m = string_free(m);

    fprintf(stderr, "=======\n");

    http_status = 0;
    z = string_alloc("HTTP 200 OK\r\nContent-Length: 70\r\n\r\nabcdefghij", strlen("HTTP 200 OK\r\nContent-Length: 70\r\n\r\nabcdefghij"));

    w = http_get_request(& http_status, & m, z);

    if (m) fprintf(stderr, "frag(x)=[%zu] %.*s\n",
                   SIZE(m), (int) SIZE(m), CSTR(m));

    z = string_free(z);

    for (i = 0; i < 10; i ++) {
        http_status = 0;

        if (! m) break;

        if (IS_LARGE(m)) {
            if (STRING_AVL(m) < strlen("abcdefghij")) {
                fprintf(stderr, "resizing\n");
                string_dim(m, SIZE(m) + strlen("abcdefghij") + 1);
            }
            memcpy((char *) STRING_END(m), "HTTPHTTPAA", strlen("abcdefghij"));
            m->_len += strlen("abcdefghij");
            z = m; m = NULL;
        }

        debug("calling with m and z inverted\n");
        w = http_get_request(& http_status, & m, z);

        while (w) {
            fprintf(stderr, "got request=%.*s (%zu)\n",
                    (int) SIZE(w), CSTR(w), SIZE(w));
            j += SIZE(w);
            fprintf(stderr, "download: %i\n", http_get_progress(TOKEN(z, 0)));
            fprintf(stderr, "got %i bytes\n", j);
            string_flush(w);
            w = http_get_request(& http_status, & m, z);
        }

        debug("no more requests!\n");
    }

    if (m) {
        fprintf(stderr, "frag(x)=[%zu] %.*s\n",
                SIZE(m), (int) SIZE(m), CSTR(m));
        fprintf(stderr, "download: %i\n", http_get_progress(TOKEN(m, 0)));
        fprintf(stderr, "got %i bytes\n", j);
        http_status = 0;
        w = http_get_request(& http_status, & z, m);
        if (w) fprintf(stderr, "got request=%.*s (%zu)\n",
                       (int) SIZE(w), CSTR(w), SIZE(w));
    }
    m = string_free(m);

    return 0;
}

/* -------------------------------------------------------------------------- */
#else
/* -------------------------------------------------------------------------- */

/* This unit test will not be compiled */
#ifdef __GNUC__
__attribute__ ((unused)) static int __dummy__ = 0;
#endif

/* -------------------------------------------------------------------------- */
#endif
/* -------------------------------------------------------------------------- */
