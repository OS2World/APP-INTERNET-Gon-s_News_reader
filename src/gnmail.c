/*
 * @(#)gnmail.c	1.40 Mar.17,1998
 *
 * 著作権は放棄しません。ただし、営利目的以外の使用／配布に制限は設けません。
 * Copyright (C) yamasita@omronsoft.co.jp
 */

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <time.h>
#include <ctype.h>

extern char *getenv();
extern char *strcat();
extern char *mktemp();

#ifdef	HAS_STRING_H
#	include	<string.h>
#endif
#ifdef	HAS_UNISTD_H
#	include	<unistd.h>
#endif
#ifdef	HAS_DIRECT_H
#	include	<direct.h>
#endif

#include	"nntp.h"
#include	"gn.h"
#include	"config.h"

char *newsspool = DEFAULT_NEWSSPOOL;

int
main(argc, argv)
int argc;
char **argv;
{
  FILE *rp;
  char resp[NNTP_BUFSIZE+1];
  char buf[NNTP_BUFSIZE+1];
  struct stat stat_buf;
  char *env, *name;
  int smtp, rcpt, data, n, i;
  FILE *inp;
  char infname[NNTP_BUFSIZE+1];
  int isnl, isbd;

  smtp = 0;
  infname[0] = '\0';
  for (i = 1; i < argc; i++) {
    if (strcmp(argv[i], "-bs") == 0)
      smtp = 1;
    if (argv[i][0] != '-')
      strcpy(infname, argv[i]);
  }

  if ( (env = getenv("NEWSSPOOL")) != 0 )
    newsspool = env;

  strcpy(buf,newsspool);
  strcat(buf,SLASH_STR);
  strcat(buf,"mail.out");
  if ( stat(buf,&stat_buf) != 0 ) {
    if ( MKDIR(buf) != 0 ) {
      fprintf(stderr,"mkdir(%s):fail\n",buf);
      return(-1);
    }
  }

  if ( infname[0] ) {
    if ( ( inp = fopen(infname,"r") ) == NULL ){
      perror(infname);
      return(-1);
    }
  } else {
    inp = stdin;
  }

  if (smtp) {
    name = buf + strlen(buf);
    printf("220 localhost\r\n");
    fflush(stdout);
  reset: /* RSET */
    strcpy(name,SLASH_STR);
    strcat(name,"gnXXXXXX");
    mktemp(&buf[0]);
    if ( ( rp = fopen(buf,"w") ) == NULL ){
      perror(buf);
      return(-1);
    }
    rcpt = 0;
    data = 0;
    while (fgets(resp,NNTP_BUFSIZE,stdin) != NULL ) {
      for (i = 0; resp[i] != 0 && resp[i] != ':'; i++) {
	resp[i] = toupper(resp[i]);
      }
      if (strncmp(resp, "QUIT", 4) == 0) {
	fclose(rp);
	if (data == 0) unlink(buf);
	printf("221 localhost\r\n");
	fflush(stdout);
	break; /* QUIT */
      }
      if (strncmp(resp, "RSET", 4) == 0) {
	fclose(rp);
	if (data == 0) unlink(buf);
	printf("250 OK\r\n");
	fflush(stdout);
	goto reset; /* RSET */
      }
      if (strncmp(resp, "ONEX", 4) == 0 ||
	  strncmp(resp, "MAIL", 4) == 0 ||
	  strncmp(resp, "VERB", 4) == 0) {
	printf("250 OK\r\n");
	fflush(stdout);
      }
      else if (strncmp(resp, "RCPT", 4) == 0) {
	if (strncmp(resp + 4, " TO:<", 5) == 0 &&
	    strcmp(resp + strlen(resp) - 3, ">\r\n") == 0) {
	  if (rcpt == 0) {
	    fprintf(rp, "Bcc: %.*s", (short)(strlen(resp) - 12), resp + 9);
	  } else {
	    fprintf(rp, ",\n     %.*s", (short)(strlen(resp) - 12), resp + 9);
	  }
	  rcpt++;
        }
	printf("250 OK\r\n");
	fflush(stdout);
      }
      else if (strncmp(resp, "DATA", 4) == 0) {
	if (rcpt == 0) {
	  printf("503 NO RCPT\r\n");
	  fflush(stdout);
	}
	else {
	  fprintf(rp, "\n");
	  printf("354 READY\r\n");
	  fflush(stdout);
	  while (fgets(resp,NNTP_BUFSIZE,stdin) != NULL) {
	    if (strcmp(resp, ".\r\n") == 0) break;
	    if ((n = strlen(resp)) >= 2 &&
		strcmp(&resp[n - 2], "\r\n") == 0) {
		strcpy(&resp[n - 2], "\n");
	    }
	    fprintf(rp, "%s", resp);
	    data++;
          }
	  printf("250 OK\r\n");
	  fflush(stdout);
        }
      }
      else {
	printf("500 ERROR\r\n");
	fflush(stdout);
      }
      fflush(rp);
    }
  }
  else {
    strcat(buf,SLASH_STR);
    strcat(buf,"gnXXXXXX");
    mktemp(&buf[0]);
    if ( ( rp = fopen(buf,"w") ) == NULL ){
      perror(buf);
      return(-1);
    }

    if ( infname[0] )
      isbd=0;	/* mh-send-progs = gnmail */
    else
      isbd=1;

    while (fgets(resp,NNTP_BUFSIZE,inp) != NULL ) {
      if ( isbd != 0 ) {
	fprintf(rp, "%s", resp);
      } else {	
	for(i=0, isnl=0; resp[i] != '\n'; i++) {
	  if ( resp[i] != '-' ) {
	    isnl=1; break;
	  }
	}
	if (isnl)
	  fprintf(rp, "%s", resp);
	else {
	  fprintf(rp, "\n");
	  isbd=1;
	}
      }
    }

    fclose(rp);
  }
  return(0);
}
