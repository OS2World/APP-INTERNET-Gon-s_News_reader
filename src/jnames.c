#ifndef lint
static char
rcs[] = "@(#)$Header: jnames.c,v 1.1 87/11/20 10:32:47 sanewo Exp $";
#endif

/*
 * Japanese name server.
 */

/* $Log:	jnames.c,v $
 * Revision 1.1  87/11/20  10:32:47  sanewo
 * Initial revision
 * 
 */

#include <sys/types.h>
#include <stdio.h>
#include <ctype.h>
#if defined(uniosu) || defined(MSC)	/* gn */
#include <string.h>
#else
#include <strings.h>
#endif

#include "nntp.h" /* gn */
#include "gn.h" /* gn */

#ifdef USG	/* gn */
#	define	index	strchr
#endif

#define JNAMES	"/usr/local/lib/jnames"

static FILE *MYJF = NULL, *PUBJF = NULL;

struct jnent {
    int jn_domain;	/* index to dom_db or DOM_GENERAL */
    char *jn_name;
    unsigned char *jn_jname;
};

#define DOM_SIZE_INIT	10
#define JN_SIZE_INIT	50

static char **dom_db;
static int dom_num, dom_size;
static struct jnent *jn_db;
static int jn_num, jn_size;

static unsigned char buf[BUFSIZ];
static char dbuf[BUFSIZ], nbuf[BUFSIZ];
static unsigned char jbuf[BUFSIZ];

#ifdef MSC	/* gn */
#	include <stdlib.h>
#else
extern char *malloc(), *realloc(), *calloc();
#endif
extern void kanji_convert(); /* gn */

int
jnOpen(mydb, pubdb)
    char *mydb, *pubdb;
{
    static int jnOpen_DB();

    if (MYJF != NULL || PUBJF != NULL) {
	/* already opened */
	return -1;
    }
    dom_db = NULL;
    jn_db = NULL;
    dom_num = dom_size = jn_num = jn_size = 0;
    if (mydb && *mydb != '\0' &&
	(MYJF = fopen(mydb, "r")) != NULL && jnOpen_DB(MYJF) < 0) {
	MYJF = NULL;
	return -1;
    }
    if (!pubdb || *pubdb == '\0')
	pubdb = JNAMES;
    if ((PUBJF = fopen(pubdb, "r")) != NULL && jnOpen_DB(PUBJF) < 0) {
	PUBJF = NULL;
	return -1;
    }
    return (MYJF != NULL || PUBJF != NULL) ? 0 : -1;
}

static int
jnOpen_DB(JF)
    FILE *JF;
{
    int last_dom_idx = -1, dom_idx;
    unsigned char buf_euc[BUFSIZ]; /* EUC Buffer gn */

    /* read database */
    while (fgets((char *)buf, BUFSIZ, JF) != NULL) {
	register unsigned char *p;
	static unsigned char *getpart();

#if 0 /* gn */
	p = buf;
#else /* gn */
	/* getpart() が JIS/EUC を仮定しているための処理 */
	kanji_convert(gn.file_kanji_code, buf, EUC, buf_euc);
	p = buf_euc;
#endif /* gn */
	/* skip leading spaces */
	while (isspace(*p)) p++;
	if (!*p || *p == '#') {
	    /* skip this line */
	    continue;
	}
	/* get domain part */
	p = getpart((unsigned char *)dbuf, p, ". \t\n");
	if (p == NULL) {
	    /* illegal format */
	    goto error;
	}
	if (*p != '.') {
	    /* same as last domain */
	    if (last_dom_idx < 0) {
		/* no previous domain */
		goto error;
	    }
	    dom_idx = last_dom_idx;
	    (void)strcpy(nbuf, dbuf);
	}
	else {
	    /* domain specified */
	    p++;
	    if (dom_num >= dom_size) {
		if (dom_db == NULL) {
		    if ((dom_db = (char **)calloc((unsigned)DOM_SIZE_INIT,
						  sizeof(char *)))
			== NULL) {
			/* can't allocate */
			goto error;
		    }
		    dom_size = DOM_SIZE_INIT;
		}
		else {
		    if ((dom_db = (char **)realloc((char *)dom_db, (unsigned)sizeof(char *)*(dom_size *= 2)))
			== NULL) {
			/* can't allocate */
			goto error;
		    }
		}
	    }
	    if ((dom_db[dom_num] = malloc((unsigned)strlen(dbuf)+1)) == NULL) {
		goto error;
	    }
	    (void)strcpy(dom_db[dom_num], dbuf);
	    /* scan domain database */
	    for (dom_idx = 0; strcmp(dbuf, dom_db[dom_idx]) != 0; dom_idx++) ;
	    if (dom_idx == dom_num)
		dom_num++;
	    else
		(void)free(dom_db[dom_num]);
	    /* read name part */
	    if ((p = getpart((unsigned char *)nbuf, p, " \t\n")) == NULL) {
		/* illegal format */
		goto error;
	    }
	}
	last_dom_idx = dom_idx;
	/* skip separating spaces */
	while (isspace(*p)) p++;
	if (!*p || (p = getpart(jbuf, p, " \t\n")) == NULL) {
	    /* illegal format */
	    goto error;
	}
	/* add to jn database */
	if (jn_num >= jn_size) {
	    if (jn_db == NULL) {
		if ((jn_db = (struct jnent *)calloc((unsigned)JN_SIZE_INIT, sizeof(struct jnent))) == NULL) {
		    goto error;
		}
		jn_size = JN_SIZE_INIT;
	    }
	    else {
		if ((jn_db = (struct jnent *)realloc((char *)jn_db, (unsigned)sizeof(struct jnent)*(jn_size *= 2))) == NULL) {
		    goto error;
		}
	    }
	}
	jn_db[jn_num].jn_domain = dom_idx;
	if ((jn_db[jn_num].jn_name = malloc((unsigned)strlen(nbuf)+1)) == NULL) {
	    goto error;
	}
	(void)strcpy(jn_db[jn_num].jn_name, nbuf);
	if ((jn_db[jn_num].jn_jname = (unsigned char *)malloc((unsigned)strlen((char *)jbuf)+1)) == NULL) {
	    goto error;
	}
#if 0 /* gn */
	(void)strcpy((char *)jn_db[jn_num].jn_jname, (char *)jbuf);
#else /* gn */
	/* データベース格納は internal_kanji_code で */
	kanji_convert(EUC, jbuf, internal_kanji_code, jn_db[jn_num].jn_jname);
#endif /* gn */
	jn_num++;
    }
    (void)fclose(JF);
    return 0;

error:
    (void)fclose(JF);
    if (dom_db != NULL) (void)free((char *)dom_db);
    if (jn_db != NULL) (void)free((char *)jn_db);
    return -1;
}

unsigned char *
jnFetch(domain, name)
    char *domain, *name;
{
    register int i, j;

    /* search domain db */
    for (i = 0; i < dom_num; i++) {
	if (strcmp(domain, dom_db[i]) == 0) break;
    }
    if (i >= dom_num)
	return NULL;
    /* search jn db */
    for (j = 0; j < jn_num; j++) {
	if (jn_db[j].jn_domain == i &&
	    strcmp(name, jn_db[j].jn_name) == 0) {
	    return jn_db[j].jn_jname;
	}
    }
    return NULL;
}

int
jnClose()
{
    if (dom_db != NULL) (void)free((char *)dom_db);
    if (jn_db != NULL) (void)free((char *)jn_db);
    MYJF = PUBJF = NULL;
    return 0;
}

static unsigned char *
getpart(dest, src, sep)
    register unsigned char *dest, *src;
    char *sep;
{
    register int quoted, kanji, squoted;

    quoted = squoted = kanji = 0;
    for (; quoted || squoted || kanji || index(sep, *src) == NULL; src++) {
	if (!*src) {
	    /* illegal format */
	    return NULL;
	}
	if (*src == '\033') {
	    kanji = *++src == '$';
	    src++;
	    continue;
	}
	if (squoted) {
	    if ((kanji && *src > ' ' && *src < '\177') || (*src & 0x80)) {
		*dest++ = *src++ | 0x80;
		*dest++ = *src | 0x80;
	    }
	    else
		*dest++ = *src;
	    squoted = 0;
	    continue;
	}
	if (!kanji && *src == '\\') {
	    squoted = 1;
	    continue;
	}
	if (!kanji && *src == '"') {
	    quoted = !quoted;
	    continue;
	}
	if ((kanji && *src > ' ' && *src < '\177') || (*src & 0x80)) {
	    *dest++ = *src++ | 0x80;
	    *dest++ = *src   | 0x80;
	}
	else
	    *dest++ = *src;
    }
    *dest = '\0';
    return src;
}
