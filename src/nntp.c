/*
 * @(#)nntp.c	1.40 Sep.19,1997
 *
 * 著作権は放棄しません。ただし、営利目的以外の使用／配布に制限は設けません。
 * Copyright (C) yamasita@omronsoft.co.jp
 */


/*
 * This software is Copyright 1991 by Stan Barber.
 *
 * Permission is hereby granted to copy, reproduce, redistribute or otherwise
 * use this software as long as: there is no monetary profit gained
 * specifically from the use or reproduction or this software, it is not
 * sold, rented, traded or otherwise marketed, and this copyright notice is
 * included prominently in any copy made.
 *
 * The author make no claims as to the fitness or correctness of this software
 * for any use whatsoever, and it is provided as is. Any use of this software
 * is at the user's own risk.
 *
 */
/*
 * NNTP client routines.
 */

#define	PASS_REQUIRE	381

#if defined(WINDOWS)
#	include	<windows.h>
#	include "wingn.h"
#endif	/* WINDOWS */

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <time.h>

#ifdef	HAS_STRING_H
#	include	<string.h>
#endif
#ifdef	HAS_STDLIB_H
#	include	<stdlib.h>
#endif
#ifdef	HAS_MALLOC_H
#	include	<malloc.h>
#endif
#ifdef	HAS_UNISTD_H
#	include	<unistd.h>
#endif
#ifdef	HAS_IO_H
#	include	<io.h>
#endif
#ifdef	HAS_DIRECT_H
#	include	<direct.h>
#endif
#ifdef	HAS_DIR_H
#	include <dir.h>
#endif

#ifdef	UNIX
extern void Exit();
#	define	exit(n) Exit(n)
#endif /* UNIX */

#if defined(WATTCP)
#	include <tcp.h>
#endif
#ifdef PCNFS
#	include "pcnfs.h"
#endif

#include	"nntp.h"
#include	"gn.h"

extern void mc_puts(),mc_printf();
extern void kanji_convert();
extern void hit_return();
extern char *kb_input();
extern char *expand_filename();
extern int get_1_header_file();
extern void ng_directory();
extern void memory_error();
extern void add_new_newsgroups();
extern int regexp_match();

#if defined(WINDOWS)
extern int mc_MessageBox();
extern void set_hourglass(),reset_cursor();
#endif	/* WINDOWS */

#ifdef	MIME
extern void MIME_strHeaderEncode();
#endif

static newsgroup_t *alloc_group_queue();
static post_file_with_gnspool();
static post_file_by_nntp();

static char oldgroup[NNTP_BUFSIZE+1] = "";

#if ! defined(NO_NET)

#if defined(WATTCP)
socket_t	ser_rd_fp;
#	define	ser_wr_fp ser_rd_fp
#else	/* WATTCP */
socket_t	ser_rd_fp = 0;
socket_t	ser_wr_fp = 0;
#endif

/*
 * server_init  Get a connection to the remote news server.
 *
 *	Parameters:	"machine" is the machine to connect to.
 *
 *	Returns:	-1 on error
 *			server's initial response code on success.
 *
 *	Side effects:	Connects to server.
 *			"ser_rd_fp" and "ser_wr_fp" are fp's
 *			for reading and writing to server.
 */

int
server_init(machine)
char	*machine;
{
  extern int server_init_com();
  char resp[NNTP_BUFSIZE+1];
  
  return(server_init_com(machine,&ser_rd_fp,&ser_wr_fp,
			 "nntp", "tcp", resp));
}

int
handle_server_response(response, server)
int	response;
char	*server;
{
#ifdef	WINDOWS
  char buf[128];
#endif	/* WINDOWS */
  
  switch (response) {
  case OK_NOPOST:		/* fall through */
#ifndef	WINDOWS
    mc_puts("あなたのマシンからはポストすることができません \n");
    mc_puts("試してみたって無駄ですよ:-p\n\n");
#else  /* WINDOWS */
    mc_MessageBox(hWnd,"あなたのマシンからはポストすることができません ",
		  "",MB_OK);
#endif /* WINDOWS */
  case OK_CANPOST:
    return (0);

  case ERR_ACCESS:
#ifndef	WINDOWS
    mc_printf("あなたのマシンは、%s のニュースシステムと接続することができません \n",gn.nntpserver);
#else  /* WINDOWS */
    sprintf(buf,"あなたのマシンは、%s のニュースシステムと接続することができません ",gn.nntpserver);
    mc_MessageBox(hWnd,buf,"",MB_OK);
#endif /* WINDOWS */
    return (-1);

  default:
#ifndef	WINDOWS
    mc_printf("%s のニュースサーバが意味不明のコード(%d)を返してきました\n",server,response);
#else  /* WINDOWS */
    sprintf("%s のニュースサーバが意味不明のコード(%d)を返してきました",server,response);
    mc_MessageBox(hWnd,buf,"",MB_OK);
#endif /* WINDOWS */
    return (-1);
  }
  /*NOTREACHED*/
}


/*
 * put_server -- send a line of text to the server, terminating it
 * with CR and LF, as per ARPA standard.
 *
 *	Parameters:	"string" is the string to be sent to the
 *			server.
 *
 *	Returns:	Nothing.
 *
 *	Side effects:	Talks to the server.
 *
 *	Note:		This routine flushes the buffer each time
 *			it is called.  For large transmissions
 *			(i.e., posting news) don't use it.  Instead,
 *			do the fprintf's yourself, and then a final
 *			fflush.
 */

int
put_server(string)
char *string;
{
  extern int put_server_com();
  
#ifndef	WINSOCK
  put_server_com(string, &ser_wr_fp);
  return(CONT);
#else	/* WINSOCK */
#ifndef	WINDOWS
  return(put_server_com(string,&ser_wr_fp));
#else	/* WINDOWS */
  if ( put_server_com(string,&ser_wr_fp) != 0 ) {
    PostMessage(hWnd, WM_CONNECTIONCLOSED,0,0L);
    return(ERROR);
  }
  return(CONT);
#endif	/* WINDOWS */
#endif	/* WINSOCK */
}
/*
 * get_server -- get a line of text from the server.  Strips
 * CR's and LF's.
 *
 *	Parameters:	"string" has the buffer space for the
 *			line received.
 *			"size" is the size of the buffer.
 *
 *	Returns:	-1 on error, 0 otherwise.
 *
 *	Side effects:	Talks to server, changes contents of "string".
 */

int
get_server(string, size)
char	*string;
int	size;
{
  extern int get_server_com();
  
#ifndef	WINDOWS
  return(get_server_com(string, size,&ser_rd_fp));
#else	/* WINDOWS */
  if ( get_server_com(string, size,&ser_rd_fp) != 0 ) {
    PostMessage(hWnd, WM_CONNECTIONCLOSED,0,0L);
    return(ERROR);
  }
  return(CONT);
#endif	/* WINDOWS */
}

/*
 * close_server -- close the connection to the server, after sending
 *		the "quit" command.
 *
 *	Parameters:	None.
 *
 *	Returns:	Nothing.
 *
 *	Side effects:	Closes the connection with the server.
 *			You can't use "put_server" or "get_server"
 *			after this routine is called.
 */

void
close_server()
{
  extern void close_server_com();
  
  close_server_com(&ser_rd_fp,&ser_wr_fp);
}

/*
 * ニュースサーバとの接続
 */
int
open_server()
{
  extern int server_init();
  extern int handle_server_response();
  extern void close_server();
  extern int auth_nntp();
  
  int response;
  char buf[NNTP_BUFSIZE+1];
  
#ifdef	WINDOWS
  set_hourglass();
#endif	/* WINDOWS */
  
  if ( (response = server_init(gn.nntpserver)) < 0 ) {
#ifndef	WINDOWS
    mc_printf("\n%s と接続できませんでした\n",gn.nntpserver);
    exit(1);
#else  /* WINDOWS */
    reset_cursor();
    sprintf(buf,"%s と接続できませんでした",gn.nntpserver);
    mc_MessageBox(hWnd,buf,"",MB_OK);
    return(ERROR);
#endif /* WINDOWS */
  }
  
  if ( gn.mode_reader == ON ) {
    if( put_server("MODE reader") ) {
#ifndef	WINDOWS
      exit(1);
#else  /* WINDOWS */
      reset_cursor();
      return(ERROR);
#endif /* WINDOWS */
    }
    if( get_server(buf,NNTP_BUFSIZE)) {
#ifndef	WINDOWS
      exit(1);
#else  /* WINDOWS */
      reset_cursor();
      return(ERROR);
#endif /* WINDOWS */
    }
  }
  
  if ( handle_server_response(response,gn.nntpserver) ){
    close_server();
#ifndef	WINDOWS
    exit(2);
#else  /* WINDOWS */
    return(ERROR);
#endif /* WINDOWS */
  }
  
  /* ユーザ認証 */
  if ( gn.nntp_auth == ON ) {
    switch (auth_nntp()) {
    case CONT:
      return(CONT);
    default:
#ifndef	WINDOWS
      exit(1);
#else  /* WINDOWS */
      reset_cursor();
      return(ERROR);
#endif /* WINDOWS */
    }
  }
  return(CONT);
}
#endif	/* ! NO_NET */

/*
 * アクティブファイルを取得する
 */

int
get_active()
{
  register newsgroup_t **ng;
  char resp[NNTP_BUFSIZE+1];
  register char *pt;
  long first,last;
  register int x = 0;
  FILE *fp;
  int get_local_active = OFF;
  
#ifdef	NO_NET
  get_local_active = ON;
#else  /* NO_NET */
  if ( gn.local_spool == ON ||
      (prog == GNSPOOL && (DO_GET == 0 || gn.fast_connect == ON)) ) {
    get_local_active = ON;
  }
#endif  /* NO_NET */
  mc_puts("\nニュースグループを調べています\n");
  
  if ( get_local_active == ON ) {
    if ( (pt = expand_filename("active",gn.newslib)) == 0 ) {
#if ! defined(WINDOWS)
      mc_puts("ファイル名の展開に失敗しました(active)\n");
      exit(1);
#else  /* WINDOWS */
      mc_MessageBox(hWnd,"ファイル名の展開に失敗しました",
		    "active",MB_OK);
      PostQuitMessage(0);
      return(QUIT);
#endif /* WINDOWS */
    }

    if ( (fp = fopen(pt,"r")) == NULL ) {
#ifndef	WINDOWS
      mc_printf("\n%s がみつかりません \n",pt);
      exit(1);
#else  /* WINDOWS */
      sprintf(resp,"%s がみつかりません ",pt);
      mc_MessageBox(hWnd,resp,"",MB_OK);
      PostQuitMessage(0);
      return(QUIT);
#endif /* WINDOWS */
    }
#if ! defined(NO_NET)
  } else {
    if ( put_server("LIST") )
      return(CONNECTION_CLOSED);
    if ( get_server(resp,NNTP_BUFSIZE))
      return(CONNECTION_CLOSED);
    if (atoi(resp) != OK_GROUPS ) {
#ifndef	WINDOWS
      mc_printf("\n%s\n",resp);
      exit(4);
#else  /* WINDOWS */
      mc_MessageBox(hWnd,resp,"",MB_OK);
      PostQuitMessage(0);
      return(QUIT);
#endif /* WINDOWS */
    }
#endif /* ! NO_NET */
  }
  
#ifdef	WINDOWS
  set_hourglass();
#endif	/* WINDOWS */
  
  ng = &newsrc;
  
  while( 1 ) {
    if ( ++x > 20 ){
      mc_printf(".");
      fflush(stdout);
      x = 0;
#ifdef	QUICKWIN
      _wyield();
#endif
    }
    if ( get_local_active == ON ) {
      if ( fgets(resp,NEWSRC_LEN,fp) == NULL ) {
	fclose(fp);
	break;
      }
#if ! defined(NO_NET)
    } else {
      if ( get_server(resp,NNTP_BUFSIZE)) {
#ifdef	WINDOWS
	reset_cursor();
#endif /* WINDOWS */
	return(CONNECTION_CLOSED);
      }
      if ( resp[0] == '.' && resp[1] == 0)
	break;
#endif
    }

    /* 購読するグループ ? */
    if ( gn.unsubscribe_groups != 0 ) {
      if ( regexp_match(resp,gn.unsubscribe_groups) == YES )
	continue;
    }
    if ( gn.subscribe_groups != 0 ) {
      if ( regexp_match(resp,gn.subscribe_groups) != YES )
	continue;
    }

		
    /* *resp : ニュースグループ名 */
    if ( (pt = strchr(resp,' ')) == NULL )
      continue;
    *pt = 0;
    pt++;
    /* last : 最後の記事 */
    last = atol(pt);
    pt = strchr(pt,' ');
    pt++;
    /* first : 最初の記事 */
    first = atol(pt);
    if ( last != 0 && first == 0 )
      first = 1;

    if ( (*ng = alloc_group_queue()) == NULL ) {
      memory_error();
#ifdef	WINDOWS
      reset_cursor();
#endif /* WINDOWS */
      return(ERROR);
    }
    if ( ((*ng)->name = malloc(strlen(resp)+1)) == NULL ) {
#ifdef	WINDOWS
      reset_cursor();
#endif /* WINDOWS */
      memory_error();
      return(ERROR);
    }
    strcpy((*ng)->name,resp);
    (*ng)->first = first;
    (*ng)->last = last;
    (*ng)->numb = strlen(resp);
    (*ng)->flag = ACTIVE;

    ng = &((*ng)->next);
  }
  
  mc_printf("\n");
#ifdef	WINDOWS
  reset_cursor();
#endif /* WINDOWS */
  return(CONT);
}

/*
 * アクティブファイルを再度取得する
 */

int
re_get_active()
{
  newsgroup_t *newsrc_new;	/* ニュースグループリスト */
  register newsgroup_t **ng_new;
  register newsgroup_t **ng;
  char resp[NNTP_BUFSIZE+1];
  register char *pt;
  long first,last;
  register int x = 0;
  FILE *fp;
  int get_local_active = OFF;
  
#ifdef	NO_NET
  get_local_active = ON;
#else  /* NO_NET */
  if ( gn.local_spool == ON ||
      (prog == GNSPOOL && (DO_GET == 0 || gn.fast_connect == ON)) ) {
    get_local_active = ON;
  }
#endif  /* NO_NET */
  mc_puts("\n未読の記事数を数えています\n");
  
  if ( get_local_active == ON ) {
    if ( (pt = expand_filename("active",gn.newslib)) == 0 ) {
#if ! defined(WINDOWS)
      mc_puts("ファイル名の展開に失敗しました(active)\n");
      exit(1);
#else  /* WINDOWS */
      mc_MessageBox(hWnd,"ファイル名の展開に失敗しました",
		    "active",MB_OK);
      PostQuitMessage(0);
      return(QUIT);
#endif /* WINDOWS */
    }

    if ( (fp = fopen(pt,"r")) == NULL ) {
#ifndef	WINDOWS
      mc_printf("\n%s がみつかりません \n",pt);
      exit(1);
#else  /* WINDOWS */
      sprintf(resp,"%s がみつかりません ",pt);
      mc_MessageBox(hWnd,resp,"",MB_OK);
      PostQuitMessage(0);
      return(QUIT);
#endif /* WINDOWS */
    }
#if ! defined(NO_NET)
  } else {
    if ( put_server("LIST") )
      return(CONNECTION_CLOSED);
    if ( get_server(resp,NNTP_BUFSIZE))
      return(CONNECTION_CLOSED);
    if (atoi(resp) != OK_GROUPS ) {
#ifndef	WINDOWS
      mc_printf("\n%s\n",resp);
      exit(4);
#else  /* WINDOWS */
      mc_MessageBox(hWnd,resp,"",MB_OK);
      PostQuitMessage(0);
      return(QUIT);
#endif /* WINDOWS */
    }
#endif /* ! NO_NET */
  }
  
#ifdef	WINDOWS
  set_hourglass();
#endif	/* WINDOWS */
  
  newsrc_new = 0;
  ng_new = &newsrc_new;
  
  while( 1 ) {
    if ( ++x > 20 ){
      mc_printf(".");
      fflush(stdout);
      x = 0;
#ifdef	QUICKWIN
      _wyield();
#endif
    }
    if ( get_local_active == ON ) {
      if ( fgets(resp,NEWSRC_LEN,fp) == NULL ) {
	fclose(fp);
	break;
      }
#if ! defined(NO_NET)
    } else {
      if ( get_server(resp,NNTP_BUFSIZE)) {
#ifdef	WINDOWS
	reset_cursor();
#endif /* WINDOWS */
	return(CONNECTION_CLOSED);
      }
      if ( resp[0] == '.' && resp[1] == 0)
	break;
#endif
    }

    /* 購読するグループ ? */
    if ( gn.unsubscribe_groups != 0 ) {
      if ( regexp_match(resp,gn.unsubscribe_groups) == YES )
	continue;
    }
    if ( gn.subscribe_groups != 0 ) {
      if ( regexp_match(resp,gn.subscribe_groups) != YES )
	continue;
    }

		
    /* *resp : ニュースグループ名 */
    if ( (pt = strchr(resp,' ')) == NULL )
      continue;
    *pt = 0;
    pt++;
    /* last : 最後の記事 */
    last = atol(pt);
    pt = strchr(pt,' ');
    pt++;
    /* first : 最初の記事 */
    first = atol(pt);
    if ( last != 0 && first == 0 )
      first = 1;

    /* .newsrc の中から探す */
    for ( ng = &newsrc; *ng != 0 ; ng = &((*ng)->next)) {
      if ((*ng)->name[0] == resp[0] && strcmp((*ng)->name,resp)== 0 ) {
	/* あった */
	(*ng)->first = first;
	(*ng)->last = last;
	(*ng)->flag |= ACTIVE;
	break;
      }
    }
    if ( *ng != 0 )
      continue;

    if ( (*ng_new = alloc_group_queue()) == NULL ) {
      memory_error();
#ifdef	WINDOWS
      reset_cursor();
#endif /* WINDOWS */
      return(ERROR);
    }
    if ( ((*ng_new)->name = malloc(strlen(resp)+1)) == NULL ) {
#ifdef	WINDOWS
      reset_cursor();
#endif /* WINDOWS */
      memory_error();
      return(ERROR);
    }
    strcpy((*ng_new)->name,resp);
    (*ng_new)->first = first;
    (*ng_new)->last = last;
    (*ng_new)->flag |= ACTIVE;

    ng_new = &((*ng_new)->next);
  }
  
  mc_printf("\n");
#ifdef	WINDOWS
  reset_cursor();
#endif /* WINDOWS */
  add_new_newsgroups(newsrc_new);
  return(CONT);
}
#define	GROUP_ALLOC_SIZE 10
static newsgroup_t *group_free_queue = (newsgroup_t *)0;

/* ニュースグループキューの確保 */
static newsgroup_t *
alloc_group_queue()
{
  register newsgroup_t *p;
  register int i;
  
  /* もしフリーキューが確保されていなければ malloc */
  if ( group_free_queue == (newsgroup_t *)0) {
    if ( (group_free_queue =
	  (newsgroup_t *)malloc(sizeof(newsgroup_t)*GROUP_ALLOC_SIZE)) == NULL ) {
      memory_error();
      return(NULL);
    }
    for ( i = 0 ; i < GROUP_ALLOC_SIZE-1 ; i++ )
      group_free_queue[i].next = &group_free_queue[i+1];
    group_free_queue[GROUP_ALLOC_SIZE-1].next = 0;
  }
  
  p = group_free_queue;
  group_free_queue = p->next;
  
  p->next = 0;
  p->newsrc = 0;
  p->name = 0;
  p->desc = 0;
  p->first = 0;
  p->read = 0;
  p->numb = 0;
  p->last = 0;
  p->flag = 0;
  p->subj = 0;
  return(p);
}

int
regexp_match(group,groups)
char *group;
regexp_group_t *groups;
{
  extern int match_regrep();
  register regexp_group_t *ig;
  char *name;	/* newsgroup 名 */
  int len;	/* newsgroup 名の長さ */
  int flag = NO;

  for( ig = groups ; ig != 0 ; ig = ig->next ) {
    if (ig->len == 0 )		/* 正規表現エラー */
      continue;

    name = ig->name;
    len = ig->len;

    if (len > 0 ) {		/* 正規表現なし */
      if (name[0] == group[0] && strncmp(name,group,len) == 0 ) {
	flag = ig->flag;
      }
    } else {			/* 正規表現あり */
      if ( strcmp(name,"all") == 0 ) {
	flag = ig->flag;
	continue;
      }

      if ( match_regrep(group,name) == YES )
	flag = ig->flag;
    }
  }
  return(flag);
}
int
match_regrep(grp,reg)
register char *grp;	/* 調べるニュースグループ名 */
register char *reg;	/* 正規表現ニュースグループ名 */
{
  int match;
  char ch;

  while (1) {
    if ( strcmp(reg,"all") == 0 )
      return(YES);
    if ( strncmp(reg,"all.",4) != 0 )
      break;
    while ( *grp != '.' ) {
      if ( *grp == 0 )
	return(NO);		/* no match */
      grp++;
    }
    grp++;
    reg += 4;
  }

  while (1) {
    /* 一致するところまで */
    while ( *grp == *reg && *grp != 0 && *reg != 0 ) {
      grp++;
      ch = *reg++;
      if ( ch != '.' )
	continue;
      if ( strcmp(reg,"all") == 0 )
	return(YES);
      if ( strncmp(reg,"all.",4) == 0) {
	while ( *grp != '.' ) {
	  if ( *grp == 0 )
	    return(NO);		/* no match */
	  grp++;
	}
	grp++;
	reg += 4;
      }
    }
    if ( *reg == 0 )
      return(YES);		/* match */
    if ( *grp == 0 )
      return(NO);		/* no match */
    if ( *reg != '[' )
      return(NO);		/* no match */

    /* [...] */
    reg++;
    match = NO;
    while ( *reg != ']' ) {
      if ( reg[1] == '-' ) {
	if ( reg[0] <= *grp && *grp <= reg[2] ){
	  grp++;
	  reg += 3;
	  while ( *reg != ']' )	/* skip to ] */
	    reg++;
	  reg++;
	  if ( *reg == 0 )
	    return(YES);	/* match */
	  match = YES;
	  break;		/* ] の次を */
	}
	reg += 3;		/* a-z には一致しなかった */
	continue;
      }
      if ( *reg == *grp ){
	grp++;
	reg++;
	while ( *reg != ']' )	/* skip to ] */
	  reg++;
	reg++;
	if ( *reg == 0 )
	  return(YES);		/* match */
	match = YES;
	break;			/* ] の次を */
      }
      reg++;
    }
    if ( match == NO )
      return(NO);
  }
}

/*
 * 各グループの説明を取り寄せる
 */

int
get_newsgroups()
{
  extern newsgroup_t *search_group();
  register newsgroup_t *ng;
  char resp[NNTP_BUFSIZE+1];
  char resp2[NNTP_BUFSIZE+1];
  char *pt;
  register int x = 0;
  FILE *fp;
  
  mc_puts("\n各グループの説明を取り寄せています\n");
  
  if ( gn.local_spool ) {
    if ( (pt = expand_filename("newsgroups",gn.newslib)) == 0 ) {
#if ! defined(WINDOWS)
      mc_puts("ファイル名の展開に失敗しました(newsgroups)\n");
#else  /* WINDOWS */
      mc_MessageBox(hWnd,"ファイル名の展開に失敗しました","newsgroups",MB_OK);
#endif /* WINDOWS */
      return(CONT);
    }
    if ( (fp = fopen(pt,"r")) == NULL ) {
      mc_printf("\n%s がみつかりません \n",pt);
      return(CONT);
    }
#if ! defined(NO_NET)
  } else {
    if ( put_server("LIST NEWSGROUPS") )
      return(CONNECTION_CLOSED);
    if ( get_server(resp,NNTP_BUFSIZE))
      return(CONNECTION_CLOSED);
    if (atoi(resp) != OK_GROUPS ) {
      gn.description = 0;
      mc_printf("\n%s\n",resp);
      return(CONT);
    }
#endif
  }
  
#ifdef	WINDOWS
  set_hourglass();
#endif	/* WINDOWS */
  
  while( 1 ) {
    if ( ++x > 20 ){
      mc_printf(".");
      fflush(stdout);
      x = 0;
#ifdef	QUICKWIN
      _wyield();
#endif
    }
    if ( gn.local_spool ) {
      if ( fgets(resp,NEWSRC_LEN,fp) == NULL ) {
	fclose(fp);
	mc_printf("\n");
#ifdef	WINDOWS
	reset_cursor();
#endif /* WINDOWS */
	return(CONT);
      }
      if ( (pt = strchr(resp,'\n')) != NULL )
	*pt = 0;
      if ( (pt = strchr(resp,'\r')) != NULL )
	*pt = 0;
#if ! defined(NO_NET)
    } else {
      if ( get_server(resp,NNTP_BUFSIZE)) {
#ifdef	WINDOWS
	reset_cursor();
#endif /* WINDOWS */
	return(CONNECTION_CLOSED);
      }
      if ( resp[0] == '.' && resp[1] == 0) {
	mc_printf("\n");
#ifdef	WINDOWS
	reset_cursor();
#endif /* WINDOWS */
	return(CONT);
      }
#endif
    }

    /* *resp : ニュースグループ名 */
    pt = &resp[0];
    while ( *pt != ' ' && *pt != '\t' && *pt != 0 )
      pt++;
    if ( *pt == 0 )		/* ニュースグループ名しかない */
      continue;
    *pt++ = 0;
    while ( *pt == ' ' || *pt == '\t' )
      pt++;
    if ( *pt == 0 )		/* ニュースグループ名しかない */
      continue;

    if ( (ng = search_group(resp)) == 0 ) /* ニュースグループがない */
      continue;

    /* 念のため、コード変換 */
    kanji_convert(gn.spool_kanji_code,pt,internal_kanji_code,resp2);

    if ( (ng->desc = malloc(strlen(resp2)+1)) == NULL ) {
#ifdef	WINDOWS
      reset_cursor();
#endif /* WINDOWS */
      memory_error();
      return(ERROR);
    }
    strcpy(ng->desc,resp2);
  }
}

/*
 * グループコマンド発行
 */

int
group_command(name)
char *name;
{
  
  char resp[NNTP_BUFSIZE+1];
  
  if ( strcmp(oldgroup,name) == 0 )
    return(CONT);	/* 前回のニュースグループと同じ */
  
  strcpy(oldgroup,name);
  
  if ( gn.local_spool ) {
    if ( ngdir != 0 )
      free(ngdir);
    ng_directory(resp,name);
    ngdir = malloc(strlen(resp)+1);
    strcpy(ngdir,resp);
    return(CONT);
  }

#if ! defined(NO_NET)
#ifdef	WINDOWS
  set_hourglass();
#endif	/* WINDOWS */
  strcpy(resp,"GROUP ");
  strcat(resp,name);
  if ( put_server(resp) ) {
    oldgroup[0] = 0;
#ifdef	WINDOWS
    reset_cursor();
#endif /* WINDOWS */
    return(CONNECTION_CLOSED);
  }
  if ( get_server(resp,NNTP_BUFSIZE)) {
    oldgroup[0] = 0;
#ifdef	WINDOWS
    reset_cursor();
#endif /* WINDOWS */
    return(CONNECTION_CLOSED);
  }
  
  switch (atoi(resp) ) {
  case OK_GROUP:
    break;
  case ERR_NOGROUP:
    oldgroup[0] = 0;
#if ! defined(WINDOWS)
    mc_printf("%s\n",resp);
    hit_return();
#else  /* WINDOWS */
    mc_MessageBox(hWnd,resp,"",MB_OK);
#endif /* WINDOWS */
    return(ERROR);
  default:
    return(CONNECTION_CLOSED);
  }
  return(CONT);
#endif
}
#if ! defined(NO_NET)
int
group_command_set_numb(ng)
register newsgroup_t *ng;
{
  extern void check_group_sub();
  long first,last;
  char resp[NNTP_BUFSIZE+1];
  register char *pt;
  
  strcpy(oldgroup,ng->name);
  
#ifdef	WINDOWS
  set_hourglass();
#endif	/* WINDOWS */
  strcpy(resp,"GROUP ");
  strcat(resp,ng->name);
  if ( put_server(resp) ) {
    oldgroup[0] = 0;
#ifdef	WINDOWS
    reset_cursor();
#endif /* WINDOWS */
    return(CONNECTION_CLOSED);
  }
  
  if ( get_server(resp,NNTP_BUFSIZE)) {
    oldgroup[0] = 0;
#ifdef	WINDOWS
    reset_cursor();
#endif /* WINDOWS */
    return(CONNECTION_CLOSED);
  }
  
  switch (atoi(resp) ) {
  case OK_GROUP:
    break;
  case ERR_NOGROUP:
    oldgroup[0] = 0;
    return(ERROR);
  default:
    return(CONNECTION_CLOSED);
  }
  
  /* グループコマンドのステータス内の情報から ng->first/last を設定 */
  pt = resp;
  while ('0' <= *pt && *pt <= '9' ) /* skip status code */
    pt++;
  while (' ' == *pt || *pt == TAB )
    pt++;
  
  if ( atol(pt) == 0 )		/* 記事なし */
    return(CONT);
  
  while ('0' <= *pt && *pt <= '9' ) /* skip numb */
    pt++;
  while (' ' == *pt || *pt == TAB )
    pt++;
  first = atol(pt);
  if ( first == 0 )
    return(CONT);
  
  while ('0' <= *pt && *pt <= '9' ) /* skip first */
    pt++;
  while (' ' == *pt || *pt == TAB )
    pt++;
  last = atol(pt);
  if ( last == 0 )
    return(CONT);
  
  ng->first = first;
  ng->last = last;
  
  check_group_sub(ng);
  
  return(CONT);
}
#endif	    /* !NO_NET */

/* 指定したファイルをポストする */

int
post_file(tmp_file_f,kanji_code)
char *tmp_file_f;		/* ポストするファイル */
int kanji_code;			/* tmp_file_f の漢字コード */
{
#ifdef NO_NET
    return(post_file_with_gnspool(tmp_file_f,kanji_code));
#else
  if ( prog == GN && gn.with_gnspool ) {
    return(post_file_with_gnspool(tmp_file_f,kanji_code));
  }
  return(post_file_by_nntp(tmp_file_f,kanji_code));
#endif /* NO_NET */
}

/*
 * gnspool を使ってポストする
 */
static int
post_file_with_gnspool(tmp_file_f,kanji_code)
char *tmp_file_f;
int kanji_code;
{
  FILE *fp,*rp;
  char resp[NNTP_BUFSIZE+1];
  char buf[NNTP_BUFSIZE+1];
  struct stat stat_buf;
  
  /* スプールディレクトリの作成 */
  strcpy(buf,gn.newsspool);
  strcat(buf,SLASH_STR);
  strcat(buf,"news.out");
  if ( stat(buf,&stat_buf) != 0 ) {
    if ( MKDIR(buf) != 0 ) {
#ifdef	WINDOWS
      reset_cursor();
      sprintf(resp,"%s の作成に失敗しました",buf);
      mc_MessageBox(hWnd,resp,"",MB_OK);
#else  /* WINDOWS */
      mc_printf("%s の作成に失敗しました\n",buf);
#endif /* WINDOWS */
      return(ERROR);
    }
  }

  /* テンポラリファイル */
  strcat(buf,SLASH_STR);
  strcat(buf,"gnXXXXXX");
  mktemp(&buf[0]);
  if ( ( rp = fopen(buf,"w") ) == NULL ){
#ifdef	WINDOWS
    reset_cursor();
    mc_MessageBox(hWnd,"テンポラリファイルのオープンに失敗しました",
		  buf,MB_OK);
#else  /* WINDOWS */
    perror(buf);
#endif /* WINDOWS */
    return(ERROR);
  }

  fp = fopen(tmp_file_f,"r");
  while (fgets(resp,NNTP_BUFSIZE,fp) != NULL ) {
    kanji_convert(kanji_code,resp,gn.gnspool_kanji_code,buf);
    fprintf(rp, "%s", buf);
  }

  fclose(rp);
  fclose(fp);
#ifdef	WINDOWS
  reset_cursor();
#endif /* WINDOWS */
  return(CONT);
}

#ifndef NO_NET
#define	BIT_PATH		1
#define	BIT_FROM		2
#define	BIT_ORGANIZATION	4
/*
 * nntp を使ってポストする
 */
static int
post_file_by_nntp(tmp_file_f,kanji_code)
char *tmp_file_f;
int kanji_code;
{
#if !defined(USE_PUT_SERVER)
  extern FILE *ser_wr_fp;
#endif
  extern int auth_nntp();
  register char *sp;
#ifdef HUMAN68K
  int tloc;
#else
  long tloc;
#endif
  FILE *ac_fp;
  char resp2[NNTP_BUFSIZE+1];
  FILE *fp,*rp = 0;
  char resp[NNTP_BUFSIZE+1];
  char buf[NNTP_BUFSIZE+1];
  int x = 0;

  int header=0;			/* BIT_{PATH|FROM|ORGANIZATION} */

#ifdef	WINDOWS
  set_hourglass();
#endif	/* WINDOWS */
  
  
  if ( gn.local_spool ) {
#ifndef	MSDOS
#ifdef	USG
    putenv("HOME=");
#else  /* USG */
    unsetenv("HOME");
#endif /* USG */
    if ( ( rp = popen(gn.postproc,"w") ) == NULL ) {
      mc_puts("\nポストできません \n\007");
      perror(gn.postproc);
      hit_return();
      return(ERROR);
    }
#endif /* MSDOS */
  } else {
    if ( put_server("POST") ) {
#ifdef	WINDOWS
      reset_cursor();
#endif /* WINDOWS */
      return(CONNECTION_CLOSED);
    }
    if ( get_server(resp,NNTP_BUFSIZE)) {
#ifdef	WINDOWS
      reset_cursor();
#endif /* WINDOWS */
      return(CONNECTION_CLOSED);
    }
    
#ifdef	WINDOWS
    reset_cursor();
#endif /* WINDOWS */
    switch( atoi(resp) ) {
    case CONT_POST:
      break;
    case ERR_NOAUTH:		/* Authentication required for command */
#ifdef	WINDOWS
      mc_MessageBox(hWnd,"ポストするにはユーザ認証が必要です ",
		    "",MB_OK);
#else
      mc_puts("\nポストするにはユーザ認証が必要です \n");
#endif /* WINDOWS */
      switch (auth_nntp()) {
      case CONT:
	return(post_file(tmp_file_f, kanji_code)); /* restart */
      case ERROR:
	return(ERROR);
      case CONNECTION_CLOSED:
	return(CONNECTION_CLOSED);
      }
      break;

    case ERR_NOPOST:
#if ! defined(WINDOWS)
      mc_puts("\nポストできません \n\007");
      mc_printf("%s\n",resp);
      hit_return();
#else  /* WINDOWS */
      reset_cursor();
      mc_MessageBox(hWnd,resp,"ポストできません ",MB_OK);
#endif /* WINDOWS */
      return(ERROR);
    default:
#ifdef	WINDOWS
      reset_cursor();
#endif /* WINDOWS */
      return(CONNECTION_CLOSED);
    }
  }
  if ( gn.author_copy != 0 ){
    if ( strcmp(gn.author_copy,"/dev/null") == 0 ) {
      gn.author_copy = 0;
    } else {
      if ( (ac_fp = fopen(gn.author_copy,"a+")) == NULL ){
	mc_printf("\n%s に書き込めませんでした\n\007",gn.author_copy);
	gn.author_copy = 0;
      }
    }
  }
  
  
#ifdef	WINDOWS
  set_hourglass();
#endif	/* WINDOWS */
  
  /* ヘッダ部分 ***********************************************/
  if ( gn.author_copy != 0 ){
    time(&tloc);
    fprintf(ac_fp, "From %s %s", gn.usrname, ctime(&tloc));
  }

  if ( gn.substitute_header == ON ) {
    /* Path: */
    sprintf( resp, "Path: %s", gn.usrname );
    if ( gn.local_spool )
      fprintf(rp,"%s\n",resp);
    else
      PUT_SERVER( resp );
    
    if ( gn.author_copy != 0 )
      fprintf(ac_fp, "Path: %s\n", gn.usrname);
    
    /* From: */
    if ( gn.genericfrom != 0 ) {
      sprintf(resp, "From: %s@%s (%s)", gn.usrname, gn.domainname, gn.fullname);
    } else {
      sprintf(resp, "From: %s@%s.%s (%s)",
	      gn.usrname, gn.hostname, gn.domainname, gn.fullname);
    }
    if ( gn.author_copy != 0 ){
      kanji_convert(internal_kanji_code,resp,gn.file_kanji_code,resp2);
      fprintf(ac_fp, "%s\n",resp2);
    }
#ifdef	MIME
    if ( (gn.mime_head & MIME_ENCODE) != 0 ) {
      kanji_convert(internal_kanji_code,resp,JIS,resp2);
      MIME_strHeaderEncode(resp2,resp,sizeof(resp));
    } else
#endif
      {
	kanji_convert(internal_kanji_code,resp,gn.spool_kanji_code,resp2);
	strcpy(resp,resp2);
      }
    
    if ( gn.local_spool )
      fprintf(rp,"%s\n",resp);
    else
      PUT_SERVER( resp );
  }

  fp = fopen(tmp_file_f,"r");
  
  resp[0] = 0;
  get_1_header_file(buf,resp,fp);
  
  while (1) {
    if ( ++x > 20 ){
      mc_printf(".");
      fflush(stdout);
      x = 0;
#ifdef	QUICKWIN
      _wyield();
#endif
    }
    if (resp[0] == 0 )
      break;
    get_1_header_file(buf,resp,fp);
    if (buf[0] == 0 )
      break;
    if ( gn.substitute_header == ON ) {
      if ( strncmp("Path: ",buf,6) == 0 )
	continue;
      if ( strncmp("From: ",buf,6) == 0 )
	continue;
      if ( strncmp("Organization: ",buf,14) == 0 )
	continue;
    } else {
      if ( strncmp("Path: ",buf,6) == 0 )
	header |= BIT_PATH;
      if ( strncmp("From: ",buf,6) == 0 )
	header |= BIT_FROM;
      if ( strncmp("Organization: ",buf,14) == 0 )
	header |= BIT_ORGANIZATION;
    }

    if ( gn.author_copy != 0 ) {
      kanji_convert(kanji_code,buf,gn.file_kanji_code,resp2);
      fprintf(ac_fp, "%s\n", resp2);
    }
#ifdef	MIME
    if ( (gn.mime_head & MIME_ENCODE) != 0 ) {
      if ( strncmp(buf,"X-Nsubject: ",12) != 0 ) {
	kanji_convert(kanji_code,buf,JIS,resp2);
	MIME_strHeaderEncode(resp2,buf,sizeof(buf));
      }
    }
#endif
    kanji_convert(kanji_code,buf,gn.spool_kanji_code,resp2);
    if ( gn.local_spool )
      fprintf(rp,"%s\n",resp2);
    else
      PUT_SERVER( resp2 );
  }
  
  if ( gn.substitute_header == ON ) {
    /* Organization: */
    sprintf( resp, "Organization: %s", gn.organization);
    if ( gn.author_copy != 0 ) {
      kanji_convert(internal_kanji_code,resp,gn.file_kanji_code,resp2);
      fprintf(ac_fp, "%s\n", resp2);
    }
#ifdef	MIME
    if ( (gn.mime_head & MIME_ENCODE) != 0 ) {
      kanji_convert(internal_kanji_code,resp,JIS,resp2);
      MIME_strHeaderEncode(resp2,resp,sizeof(resp));
    } else
#endif
      {
	kanji_convert(internal_kanji_code,resp,gn.spool_kanji_code,resp2);
	strcpy(resp,resp2);
      }
    
    if ( gn.local_spool )
      fprintf(rp,"%s\n",resp);
    else
      PUT_SERVER( resp );
  }
  
  if ( gn.substitute_header == OFF ) {
    /* Path: */
    if ( (header & BIT_PATH) == 0 ) {
      sprintf( resp, "Path: %s", gn.usrname );
      if ( gn.local_spool )
	fprintf(rp,"%s\n",resp);
      else
	PUT_SERVER( resp );
    
      if ( gn.author_copy != 0 )
	fprintf(ac_fp, "Path: %s\n", gn.usrname);
    }
    
    /* From: */
    if ( (header & BIT_FROM) == 0 ) {
      if ( gn.genericfrom != 0 ) {
	sprintf(resp, "From: %s@%s (%s)", gn.usrname, gn.domainname, gn.fullname);
      } else {
	sprintf(resp, "From: %s@%s.%s (%s)",
		gn.usrname, gn.hostname, gn.domainname, gn.fullname);
      }
      if ( gn.author_copy != 0 ){
	kanji_convert(internal_kanji_code,resp,gn.file_kanji_code,resp2);
	fprintf(ac_fp, "%s\n",resp2);
      }
#ifdef	MIME
      if ( (gn.mime_head & MIME_ENCODE) != 0 ) {
	kanji_convert(internal_kanji_code,resp,JIS,resp2);
	MIME_strHeaderEncode(resp2,resp,sizeof(resp));
      } else
#endif
	{
	  kanji_convert(internal_kanji_code,resp,gn.spool_kanji_code,resp2);
	  strcpy(resp,resp2);
	}
    
      if ( gn.local_spool )
	fprintf(rp,"%s\n",resp);
      else
	PUT_SERVER( resp );
    }
    
    /* Organization: */
    if ( (header & BIT_ORGANIZATION) == 0 ) {
      sprintf( resp, "Organization: %s", gn.organization);
      if ( gn.author_copy != 0 ) {
	kanji_convert(internal_kanji_code,resp,gn.file_kanji_code,resp2);
	fprintf(ac_fp, "%s\n", resp2);
      }
#ifdef	MIME
      if ( (gn.mime_head & MIME_ENCODE) != 0 ) {
	kanji_convert(internal_kanji_code,resp,JIS,resp2);
	MIME_strHeaderEncode(resp2,resp,sizeof(resp));
      } else
#endif
	{
	  kanji_convert(internal_kanji_code,resp,gn.spool_kanji_code,resp2);
	  strcpy(resp,resp2);
	}
    
      if ( gn.local_spool )
	fprintf(rp,"%s\n",resp);
      else
	PUT_SERVER( resp );
    }
  }

  /* X-Newsreader: */
  if ( prog == GNSPOOL )
    sprintf(resp, "X-Newsreader: gnspool [Version %s %s (%s)]",
	    gn_version,gn_date,OS);
  else
    sprintf(resp, "X-Newsreader: gn [Version %s %s (%s)]",
	    gn_version,gn_date,OS);
  if ( gn.local_spool )
    fprintf(rp,"%s\n",resp);
  else
    PUT_SERVER( resp );
  if ( gn.author_copy != 0 )
    fprintf(ac_fp, "X-Newsreader: gn [Version %s %s (%s)]\n",
	    gn_version,gn_date,OS);
  
#ifdef	MIME
  if ( (gn.mime_head & MIME_ENCODE) != 0 ) {
    /* Mime-Version: 1.0 , Content-Type: text/plain; charset=ISO-2022-JP */
    if ( gn.local_spool ) {
      fprintf(rp,"Mime-Version: 1.0\n");
      fprintf(rp,"Content-Type: text/plain; charset=ISO-2022-JP\n");
    } else {
      PUT_SERVER("Mime-Version: 1.0");
      PUT_SERVER("Content-Type: text/plain; charset=ISO-2022-JP");
    }
    if ( gn.author_copy != 0 ) {
      fprintf(ac_fp, "Mime-Version: 1.0\n");
      fprintf(ac_fp, "Content-Type: text/plain; charset=ISO-2022-JP\n");
    }
  }
#endif

  /* ヘッダと本文の間の空白行 ***********************************/
  if ( gn.local_spool )
    fprintf(rp,"\n");
  else
    PUT_SERVER( "" );
  if ( gn.author_copy != 0 )
    fprintf(ac_fp, "\n");
  
  /* 本文 *********************************************************/
  while (fgets(resp,NNTP_BUFSIZE,fp) != NULL ) {
    if ( ++x > 20 ){
      mc_printf(".");
      fflush(stdout);
      x = 0;
#ifdef	QUICKWIN
      _wyield();
#endif
    }
    if ( (sp = strchr(resp,'\n')) != NULL )
      *sp = 0;

    kanji_convert(kanji_code,resp,gn.spool_kanji_code,resp2);
    if ( gn.local_spool ) {
      fprintf(rp,"%s\n",resp2);
    } else {
      if ( resp2[0] == '.' ) {
	strcpy(buf,".");
	strcat(buf,resp2);
	PUT_SERVER(buf);
      } else {
	PUT_SERVER( resp2 );
      }
    }
    if ( gn.author_copy != 0 ){
      kanji_convert(kanji_code,resp,gn.file_kanji_code,resp2);
      if ( strncmp(resp2,"From ",5) == 0 )
	fprintf(ac_fp, ">%s\n", resp2);
      else
	fprintf(ac_fp, "%s\n", resp2);
    }
  }
  
  fclose(fp);
  if ( gn.author_copy != 0 ) {
    fprintf(ac_fp, "\n");
    fclose(ac_fp);
  }
  if ( gn.local_spool ) {
#ifndef	MSDOS
    pclose(rp);
#endif /* MSDOS */
  } else {
    PUT_SERVER( "." );
    
#if	! defined(USE_PUT_SERVER)
    (void) fflush(ser_wr_fp);
#endif
    
    if ( get_server(resp,NNTP_BUFSIZE)) {
#ifdef	WINDOWS
      reset_cursor();
#endif /* WINDOWS */
      return(CONNECTION_CLOSED);
    }
    if (atoi(resp) != OK_POSTED ) {
#if ! defined(WINDOWS)
      mc_puts("\nポストできませんでした\n\007");
      mc_printf("%s\n",resp);
      hit_return();
#else  /* WINDOWS */
      reset_cursor();
      mc_MessageBox(hWnd,resp,"ポストできません ",MB_OK);
#endif /* WINDOWS */
      return(ERROR);
    }
  }
  
#ifdef	WINDOWS
  reset_cursor();
#endif	/* WINDOWS */
  return(CONT);
}
#endif	/* ! NO_NET */

#ifndef NO_NET
#ifndef	WINDOWS
int
auth_nntp()
{
  char resp[NNTP_BUFSIZE+1];
  register char *pt;
  
  sprintf(resp,"ユーザ名を入力してください（デフォルト:%s ）",gn.usrname);
  pt = kb_input(resp);
  if ( strlen(pt) == 0 )
    pt = gn.usrname;
  sprintf(resp, "authinfo user %s", pt);
  put_server(resp);
  if ( get_server(resp,NNTP_BUFSIZE)) {
    return(CONNECTION_CLOSED);
  }

  if (atoi(resp) == ERR_AUTHREJ )	/* Authorization already completed */
    return(CONT);

  if (atoi(resp) != NEED_AUTHDATA ) {
    mc_printf("\n%s\n",resp);
    return(ERROR);
  }
  
  while (1) {
    pt = kb_input("パスワードを入力してください");
    if ( strlen(pt) != 0 )
      break;
    pt = kb_input("中止しますか？(y:中止/その他:継続)");
    if ( *pt == 'y' || *pt == 'Y'){
      return(ERROR);
    }
  }
  sprintf(resp, "authinfo pass %s", pt);
  put_server(resp);
  if ( get_server(resp,NNTP_BUFSIZE)) {
    return(CONNECTION_CLOSED);
  }
  if (atoi(resp) != OK_AUTH ) {
    mc_printf("\n%s\n",resp);
    return(CONNECTION_CLOSED);
  }
  return(CONT);
}
#else	/* WINDOWS */
int
auth_nntp()
{
  BOOL __export CALLBACK authentication_nntp();
  
  switch(DialogBox(hInst,	/* 現在のインスタンス */
		   "IDD_AUTH",	/* 使用するリソース */
		   hWnd,	/* 親ウィンドウのハンドル */
		   authentication_nntp)){
  case 'y':
    return(CONT);
    
  case 'c':
    return(ERROR);
    
  default:
    return(CONNECTION_CLOSED);
  }
}
BOOL __export CALLBACK
authentication_nntp(hDlg, message, wParam, lParam)
HWND hDlg;                       /* ダイアログボックスウィンドウのハンドル */
unsigned message;                /* メッセージのタイプ                     */
WORD wParam;                     /* メッセージ固有の情報                   */
LONG lParam;
{
  char dialogtext[256];
  char resp[NNTP_BUFSIZE+1];

  switch (message) {
  case WM_INITDIALOG:
    SetDlgItemText(hDlg, IDC_AUTHUSR, (LPSTR)gn.usrname);
    return(TRUE);

  case WM_COMMAND:
    switch (wParam) {
    case IDOK:
      GetDlgItemText(hDlg, IDC_AUTHUSR, (LPSTR)dialogtext, 255);

      sprintf(resp, "authinfo user %s", dialogtext);
      put_server(resp);
      if ( get_server(resp,NNTP_BUFSIZE)) {
	EndDialog(hDlg, 'n');
      }

      if (atoi(resp) == ERR_AUTHREJ ) {	/* Authorization already completed */
	EndDialog(hDlg, 'y');
	return(TRUE);
      }

      if (atoi(resp) != NEED_AUTHDATA ) {
	mc_printf("\n%s\n",resp);
	EndDialog(hDlg, 'n');
      }


      GetDlgItemText(hDlg, IDC_AUTHPASSWD, (LPSTR)dialogtext, 255);
      sprintf(resp, "authinfo pass %s", dialogtext);
      put_server(resp);
      if ( get_server(resp,NNTP_BUFSIZE)) {
	EndDialog(hDlg, 'n');
      }
      if (atoi(resp) != OK_AUTH ) {
	mc_printf("\n%s\n",resp);
	EndDialog(hDlg, 'n');
      }

      EndDialog(hDlg, 'y');
      return(TRUE);

    case IDCANCEL:
      EndDialog(hDlg, 'c');
      return(TRUE);
    default:
      return(FALSE);
    }
  default:
    return(FALSE);
  }
  return(FALSE);
}
#endif	/* WINDOWS */
#endif	/* ! NO_NET */
