/* -------------------------------------------------------------------------- */
#ifdef _ENABLE_DB
/* -------------------------------------------------------------------------- */

#include "../lib/m_db.h"

#define RQ0 "CALL get_user_login_info('nico')"
#define RQ1 "CALL get_user_blacklist(%i)"

int test_db(void)
{
    m_db *conn = NULL;
    m_recordset *r = NULL;
    m_recordset *b = NULL;
    const char *s = NULL;
    m_dbpool *pool = dbpool_open(DB_DRIVER_MYSQL,
                                 15,
                                 "dialmsn",
                                 "phiZae8i",
                                 "dialmsn",
                                 NULL, 0, 0);

    if (! pool) {
        printf("(!) Allocating DB connection pool: FAILURE\n");
        exit(EXIT_FAILURE);
    } else printf("(*) Allocating DB connection pool: SUCCESS\n");

    conn = dbpool_borrow(pool);

    r = db_query(conn, RQ0);

    if (! (b = db_query(conn, RQ1, db_integer(r, "uid")) ) ) {
        printf("(!) Interleaved stored procedure call: FAILURE\n");
        r = db_free(r);
        conn = dbpool_return(pool, conn);
        pool = dbpool_close(pool);
        exit(EXIT_FAILURE);
    } else printf("(*) Interleaved stored procedure calls: SUCCESS\n");

    do {
        s = db_field(r, "nickname", NULL);
        printf("(-) Read nickname: %s\n", s);
    } while (db_movenext(r) != -1);

    do {
        printf("(-) Read blacklisted id: %li\n", db_integer(b, "blk"));
    } while (db_movenext(b) != -1);

    r  = db_free(r);
    b = db_free(b);

    if (! (r = db_query(conn, "CALL log_scenario(1, 2, 3)")) ) {
        printf("(!) Insert via stored procedure call: FAILURE\n");
        conn = dbpool_return(pool, conn);
        pool = dbpool_close(pool);
        exit(EXIT_FAILURE);
    } else printf("(*) Insert via stored procedure call: SUCCESS\n");

    if (! r->id) {
        printf("(!) Get last inserted id after procedure call: FAILURE\n");
        r = db_free(r);
        conn = dbpool_return(pool, conn);
        pool = dbpool_close(pool);
        exit(EXIT_FAILURE);
    } else printf("(*) Get last inserted id after procedure call: SUCCESS\n");

    r = db_free(r);

    conn = dbpool_return(pool, conn);

    pool = dbpool_close(pool);

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
