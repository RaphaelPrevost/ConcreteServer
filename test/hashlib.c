/*
 * hashlib.c is a general purpose data storage and retrieval module
 * Time for storage and retrieval remains virtually constant over a
 * very wide range.  It take virtually the same time to retrieve an
 * entry from a data base of 1,000,000 items as for 100 items.  It
 * will compile error and warning free on any ISO C compliant system.
 *
 */

/* Note that the user need not be concerned with the table size */

/*  Copyright (C) 2002 by C.B. Falconer

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful, but
  WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307
  USA

  I can be contacted by mailto:cbfalconer@worldnet.att.net
           or at 680 Hartford Tpk., Hamden, CT 06517, USA.

  Should you wish to incorporate this in proprietary software for
  sale, contact me and we can discuss a different license, such
  as the GNU Library license or other mutually agreeable terms.

  v 0.0.0.5 Largely due to prodding from:
      jmccarty@sun1307.ssd.usa.alcatel.com (Mike Mccarty Sr)
  I have come to the conclusion that a hshdelete function is
  implementable, and have done so.  This also requires that a
  'hdeleted' value be added to the status.  'hentries' shows
  the sum of active and deleted entries.
  v 0.0.0.6 - minor mods for lint 2002-06-16
  v 0.0.0.7 - fixed hashkill      2002-10-18
  v 1.0.0.0 - Comments modified   2003-03-12
  v 1.0.0.1 - hshkill fix, per Hans-Juergen Taenzer
                was memory leak   2003-05-31

   TODO list:
   Make parameters const where possible
   Make some form of manual besides the header
   Make some better usage examples than the test suite & markov.
*/

#include "hashlib.h"

/* Note: version when expressed in decimal, is of the form:  */
/* M.n.v.p  where  M = Major version                         */
/*                 n = minor version                         */
/*                 v = variation                             */
/*                 p = patch                                 */
/* with all values except M being single decimal digits.     */
#define VER  1001   /* 1.0.0.1 */

/* This is the entity that remembers all about the database  */
/* It occurs in the users data space, keeping the system     */
/* reentrant, because it is passed to all entry routines.    */
typedef struct hshtag {
   void*          *htbl;      /* points to an array of void* */
   unsigned long   currentsz;          /* size of that array */
   fp_hash         hash, rehash;
   fp_hashcmp      cmp;
   fp_hashdup      dupe;
   fp_hashfree     undupe;
   int             hdebug;          /* flag for debug output */
   m_hashstats     hstatus;  /* statistics, entry ct, errors */
   pthread_rwlock_t *lock;
} *hshtblptr;

/* WARNING
 * Peculiar definition, never to escape this module and NEVER
 * EVER to be dereferenced. This is the equivalent of NULL.
 * NEVER EVER change the name of 'master' anywhere. */
#define DELETED (void*)master

/* Threshold above which reorganization is desirable */
#define TTHRESH(sz) (sz - (sz >> 3))

/* Space available before reaching threshold
 * Ensure this can return a negative value */
#define TSPACE(m)  ((long)TTHRESH(m->currentsz) \
                  - (long)m->hstatus.hentries)

/* Table of k where 2**n - k is prime, for n=8 up. 0 ends
 * These numbers are chosen so that memory allocation will
 * usually allow space for system overhead in a 2**n block
 * http://www.utm.edu/research/primes/lists/2small/0bit.html */
#define FIRSTN 8

static int prime[] = {
                      45, 45, 41, 45, 45, 45, 45, 49,
                      57, 49, 41, 45, 59, 55, 57, 61, 0
                     };
/* So the prime of interest, vs index i into above table,   */
/* is    ( 2**(FIRSTN + i) ) - primetbl[i]                  */
/* The above table suffices for about 8,000,000 entries.    */

/* -------------------------------------------------------------------------- */
/* Internal helpers */
/* -------------------------------------------------------------------------- */

static unsigned long hashlib_prime(size_t i)
{
    /*
     * Return a prime slightly less than 2**(FIRSTN + i) -1
     * Return 0 if i is out of range.
     *
     */

    if ( (i < sizeof(prime) / sizeof(int)) && (prime[i]) )
        return ((1 << (FIRSTN + i)) - prime[i]);

    return 0;
}

/* -------------------------------------------------------------------------- */

static void **hashlib_new(unsigned long newsize)
{
    /*
    * Create, allocate and initialize an empty hash table. This is always
    * doubling the table size, and the size of all the old tables together
    * won't hold it. So any freed old table space is effectively useless for
    * this because of fragmentation. Changing the ratio won't help.
    *
    */

    unsigned long  i;
    void **newtbl;

    newtbl = malloc(newsize * sizeof(*newtbl));
    if (! newtbl) { perror(ERR(hashlib_new, malloc)); return NULL; }

    for (i = 0; i < newsize; i ++) newtbl[i] = NULL;

    return newtbl;
}

/* -------------------------------------------------------------------------- */

static void *hashlib_inserted(hshtblptr master, unsigned long h,
                              const void *item, int copying     )
{
    /*
     * Attempt to insert item at the hth position in the table. Returns NULL
     * if the position was already taken or if dupe fails (when master->herror
     * is set to hshNOMEM).
     *
     */

    void *hh;

    /* count total probes */
    master->hstatus.probes ++;

    hh = master->htbl[h];

    if (hh == NULL) {
        /* we have found a slot */
        if (copying) {
            /* no duplication */
            master->htbl[h] = (void *) item;

        } else if ((master->htbl[h] = master->dupe(item))) {
            /* new entry, so dupe and insert -- count them */
            master->hstatus.hentries ++;

        } else {
            /* duplication failed */
            master->hstatus.herror |= HASH_ENOMEM;
        }

    } else if (copying || hh == DELETED) {
        /* no comparison if copying or DELETED */
        return NULL;

    } else if (master->cmp(master->htbl[h], item) != 0) {
        /* not found here */
        return NULL;
    }

    /* else found already inserted here */
    return master->htbl[h];
}

/* -------------------------------------------------------------------------- */

static void *hashlib_store(hshtblptr master, const void *item, int copying)
{
    /*
     * Insert an entry. NULL == failure, else item.
     * This always succeeds unless memory runs out, provided that the
     * hashtable is not full.
     *
     */

    unsigned long h, h2;
    void *stored;

    h = master->hash(item) % master->currentsz;

    if (! (stored = hashlib_inserted(master, h, item, copying)) &&
          (master->hstatus.herror == HASH_SUCCESS) ) {
        h2 = master->rehash(item) % (master->currentsz >> 3) + 1;
        do {
            /* we had to go past 1 per item */
            master->hstatus.misses ++;
            h = (h + h2) % master->currentsz;
        } while (! (stored = hashlib_inserted(master, h, item, copying)) &&
                   (master->hstatus.herror == HASH_SUCCESS) );
    }

    return stored;
}

/* -------------------------------------------------------------------------- */

static int hashlib_expand(hshtblptr master)
{
    /*
     * Increase the table size by roughly a factor of 2 and reinsert all
     * entries from the old table in the new. The currentsz value is
     * revised to match the new size and the storage of the old table
     * is destroyed.
     *
     */

    void **newtbl = NULL;
    void **oldtbl = NULL;
    unsigned long newsize = 0, oldsize = 0;
    unsigned long oldentries = 0, j = 0;
    unsigned int i = 0;

    oldsize = master->currentsz;
    oldtbl =  master->htbl;

    if (master->hstatus.hdeleted > (master->hstatus.hentries / 4) ) {
        /* don't expand the table if we can get reasonable space
         * by simply removing the accumulated DELETED entries */
        newsize = oldsize;
    } else {
        newsize = hashlib_prime(0);
        for (i = 1; newsize && (newsize <= oldsize); i++)
            newsize = hashlib_prime(i);
    }

    newtbl = (newsize) ? hashlib_new(newsize) : NULL;

    if (newtbl) {
        master->currentsz = newsize;
        master->htbl = newtbl;

        /* Now reinsert all old entries in new table */
        for (j = 0; j < oldsize; j ++) {
            if (oldtbl[j] && (oldtbl[j] != DELETED)) {
                (void) hashlib_store(master, oldtbl[j], 1);
                oldentries ++;
            }
        }

        /* Sanity check */
        if (oldentries != master->hstatus.hentries - master->hstatus.hdeleted) {
            /* failure */
            master->hstatus.herror |= HASH_EINTR;
            free(master->htbl);
            master->htbl = oldtbl;
            master->currentsz = oldsize;
            return 0;
        } else {
            /* success */
            master->hstatus.hentries = oldentries;
            master->hstatus.hdeleted = 0;
            free(oldtbl);
            return 1;
        }
    }

    /* failure */
    return 0;
}

/* -------------------------------------------------------------------------- */

static int hashlib_found(hshtblptr master, unsigned long h,
                         const void *item                  )
{
    /*
     * Attempt to find item at the hth position in the table, counting
     * attempts. Returns 1 if found, 0 otherwise.
     *
     */

   void *hh = NULL;

    /* count total probes */
    master->hstatus.probes ++;

    hh = master->htbl[h];
    if ( (hh) && (hh != DELETED) )  {
        /* NEVER cmp against DELETED */
        return ! (master->cmp(hh, item));
    }

    return 0;
}

/* -------------------------------------------------------------------------- */

static unsigned long hashlib_lookup(hshtblptr master, const void *item)
{
    /* Find the current hashtbl index for item, or an empty slot */

    unsigned long h = 0, h2 = 0;

    h = master->hash(item) % master->currentsz;

    /* Within this a DELETED item simply causes a rehash */
    /* i.e. treat it like a non-equal item               */

    if (! (hashlib_found(master, h, item)) && master->htbl[h]) {

        h2 = master->rehash(item) % (master->currentsz >> 3) + 1;

        do {
            /* we had to go past 1 per item */
            master->hstatus.misses ++;
            h = (h + h2) % master->currentsz;
        } while (! (hashlib_found(master, h, item)) && master->htbl[h]);
    }

    return h;
}

/* -------------------------------------------------------------------------- */
/* Useful generic functions */
/* -------------------------------------------------------------------------- */

/* NOTE: hash is called once per operation, while rehash is
         called _no more_ than once per operation.  Thus it
         is preferable that hash be the more efficient. */

public unsigned long hashlib_strhash(const char *string)
{
    /* a fast djb2 variant used by GLib */
    const char *p = string;
    unsigned int h = *p;

    if (h) {
        for (p += 1; *p != '\0'; p++)
            h = (h << 5) - h + *p;
    }

    return h;
}

/* -------------------------------------------------------------------------- */

public unsigned long hashlib_strrehash(const char *string)
{
    /* sturdy "one at a time" hash function */
    register unsigned long hash;

    for (hash = 0; *string; string ++) {
        hash += (unsigned char) *string;
        hash += (hash << 10);
        hash ^= (hash >> 6);
    }

    hash += (hash << 3);
    hash ^= (hash >> 11);
    hash += (hash << 15);

    return hash;
}

/* -------------------------------------------------------------------------- */
/* Public interface */
/* -------------------------------------------------------------------------- */

public struct hshtag *hashlib_alloc(const fp_hash hash, const fp_hash rehash,
                                    const fp_hashcmp cmp, const fp_hashdup dupe,
                                    const fp_hashfree undupe, int hdebug)
{
    #define INITSZ 17   /* small prime, for easy testing */

    struct hshtag *master;

    if (! hash || ! rehash || ! cmp || ! dupe || ! undupe) return NULL;

    if (! (master = malloc(sizeof(*master))) ) {
        perror(ERR(hashlib_alloc, malloc));
        return NULL;
    }

    if ( ! (master->htbl = hashlib_new(INITSZ)) ) {
        free(master); return NULL;
    }

    master->currentsz = INITSZ;
    master->hash = hash; master->rehash = rehash;
    master->cmp = cmp;
    master->dupe = dupe; master->undupe = undupe;
    master->hdebug = hdebug;

    if (! (master->lock = malloc(sizeof(*master->lock))) ) {
        perror(ERR(hashlib_alloc, malloc));
        free(master);
        return NULL;
    }

    if (pthread_rwlock_init(master->lock, NULL) == -1) {
        perror(ERR(hashlib_alloc, malloc));
        free(master->lock); free(master);
        return NULL;
    }

    /* initialise the status portion */
    master->hstatus.probes = master->hstatus.misses = 0;
    master->hstatus.hentries = 0;
    master->hstatus.hdeleted = 0;
    master->hstatus.herror = HASH_SUCCESS;
    master->hstatus.version = VER;

    return master;
}

/* -------------------------------------------------------------------------- */

public void hashlib_free(hshtblptr master)
{
    unsigned long i;
    void *h;                                                    /*v7*/

    /* unload the actual data storage */
    if (master) {

        for (i = 0; i < master->currentsz; i ++) {
            if ( (h = master->htbl[i]) && (h != DELETED) )      /*v7*/
                master->undupe(master->htbl[i]);
        }

        free(master->htbl);                                     /* v1001 */
    }

    free(master->lock);

    free(master);
}

/* -------------------------------------------------------------------------- */

public void *hashlib_insert(hshtblptr master, const void *item)
{
    void *ret = NULL;

    pthread_rwlock_wrlock(master->lock);

    if ( (TSPACE(master) <= 0) && ! hashlib_expand(master)) {
        master->hstatus.herror |= HASH_EFULL;
        return NULL;
    }

    ret = hashlib_store(master, item, 0);

    pthread_rwlock_unlock(master->lock);

    return ret;
}

/* -------------------------------------------------------------------------- */

public void *hashlib_find(hshtblptr master, const void *item)
{
    unsigned long h = 0;
    void *ret = NULL;

    pthread_rwlock_rdlock(master->lock);

    h = hashlib_lookup(master, item);

    ret = master->htbl[h];

    pthread_rwlock_unlock(master->lock);

    return ret;
}

/* -------------------------------------------------------------------------- */

public void *hashlib_remove(hshtblptr master, const void *item)
{
    unsigned long h;
    void *olditem;

    pthread_rwlock_wrlock(master->lock);
    h = hashlib_lookup(master, item);

    if ( (olditem = master->htbl[h]) ) {
        /* XXX better undupe, eh */
        master->undupe(master->htbl[h]);
        master->htbl[h] = DELETED;
        master->hstatus.hdeleted ++;
    }

    pthread_rwlock_unlock(master->lock);

    return olditem;
}

/* -------------------------------------------------------------------------- */

public int hashlib_walk(hshtblptr master, const fp_hashexec exec, void *datum)
{
   unsigned long i = 0;
   int err = 0;
   void *xtra = NULL;
   void *hh = NULL;

   if (exec == NULL) return -1;

   if (master->hdebug) xtra = &i;

   for (i = 0; i < master->currentsz; i ++) {
      hh = master->htbl[i];

      if ((hh) && (hh != DELETED))
         if ( (err = exec(hh, datum, xtra)) )
            return err;
   }

   return 0;
}

/* -------------------------------------------------------------------------- */

public m_hashstats hashlib_stats(const hashtable *master)
{
   return master->hstatus;
}

/* -------------------------------------------------------------------------- */

public size_t hashlib_footprint(const hashtable *master)
{
    return sizeof(*master) + (master->currentsz * sizeof(void *)) + (master->hstatus.hentries * (sizeof(void *) + sizeof(void *) + sizeof(size_t)));
}

/* -------------------------------------------------------------------------- */
