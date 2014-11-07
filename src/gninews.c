/*
 * @(#)gninews.c	1.40 Sep.10,1997
 *
 * 著作権は放棄しません。ただし、営利目的以外の使用／配布に制限は設けません。
 * Copyright (C) yamasita@omronsoft.co.jp
 */

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <time.h>

extern char *getenv();
extern char *strcat();
extern char *mktemp();

#ifdef	HAS_STRING_H
#	include	<string.h>
#endif
#ifdef	HAS_DIRECT_H
#	include	<direct.h>
#endif

#include	"nntp.h"
#include	"gn.h"
#include	"config.h"

char *newsspool = DEFAULT_NEWSSPOOL;

int
main()
{
  FILE *rp;
  char resp[NNTP_BUFSIZE+1];
  char buf[NNTP_BUFSIZE+1];
  struct stat stat_buf;
  char *env;

  if ( (env = getenv("NEWSSPOOL")) != 0 )
    newsspool = env;

  strcpy(buf,newsspool);
  strcat(buf,SLASH_STR);
  strcat(buf,"news.out");
  if ( stat(buf,&stat_buf) != 0 ) {
    if ( MKDIR(buf) != 0 ) {
      fprintf(stderr,"mkdir(%s):fail\n",buf);
      return(-1);
    }
  }
  strcat(buf,SLASH_STR);
  strcat(buf,"gnXXXXXX");
  mktemp(&buf[0]);
  if ( ( rp = fopen(buf,"w") ) == NULL ){
    perror(buf);
    return(-1);
  }
  while (fgets(resp,NNTP_BUFSIZE,stdin) != NULL )
    fprintf(rp, "%s", resp);
  fclose(rp);
  return(0);
}
