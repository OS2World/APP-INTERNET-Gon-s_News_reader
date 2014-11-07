/*
 * @(#)gn.c	1.40 Sep.19,1997
 *
 * 著作権は放棄しません。ただし、営利目的以外の使用／配布に制限は設けません。
 * Copyright (C) yamasita@omronsoft.co.jp
 */

#if defined(WINDOWS)
#	include	<windows.h>
#	include	"wingn.h"
#endif	/* WINDOWS */

#include	<stdio.h>
#include	<signal.h>

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

#if defined(WATTCP)
#	include <tcp.h>
#endif
#ifdef PCNFS
#	include "pcnfs.h"
#endif
#ifdef WINSOCK
#	include <winsock.h>
#endif


#include	"nntp.h"
#include	"gn.h"

extern char *kb_input();
extern void save_newsrc();
extern void mc_puts(),mc_printf();
extern void memory_error();
extern int last_art_check();

#ifdef	USE_JNAMES
int jnOpen();
int jnClose();
#endif

int prog = GN;
int newsrc_flag;		/* newsrc を読み込んだら０、なかったら−１ */

#ifdef __TURBOC__
extern unsigned _stklen = 10240;
#endif

extern int gn_init();
extern int get_newsrc();
extern int open_server();
extern int get_active();
extern int get_newsgroups();
extern void check_group();
extern void close_server();
extern int group_mode();

#if defined(WINDOWS)
extern int mc_MessageBox();
#endif	/* WINDOWS */

#if !defined(MSDOS) && !defined(OS2)
extern void Exit();
#endif

#ifndef	WINDOWS
static void disp_group();

int
main(argc,argv)
int argc;
char *argv[];
{
  int check_only;
  
  /* 起動オプションの解析 */
  check_only = gn_init(argc,argv);

#if ! defined(NO_NET)
  if ( gn.local_spool == OFF ) {
    /* ニュースサーバとの接続  */
    printf(gn.nntpserver);
    mc_puts(" と接続しています\n");
    open_server();
  }
#endif
#ifdef	USE_JNAMES
  jnOpen(gn.jnames_file, NULL);
#endif
  
  /* アクティブファイルの取得 */
  switch ( get_active() ) {
  case CONNECTION_CLOSED:
    mc_puts("\nニュースサーバとの接続が切れています\n");
    return(1);
  case ERROR:
    return(1);
  }
  
  /* .newsrc ファイルの読み込み */
  if ( (newsrc_flag = get_newsrc()) == ERROR)
    return(1);
  
  /* ニュースグループのチェック */
  check_group(check_only);
  
  
  /* 各グループの説明の取得 */
  if ( gn.description == ON ) {
    switch ( get_newsgroups() ) {
    case CONNECTION_CLOSED:
      mc_puts("\nニュースサーバとの接続が切れています\n");
      return(1);
    case ERROR:
      return(1);
    }
  }
  
#if ! defined(QUICKWIN) && ! defined(WINDOWS)
  /* シグナル処理 */
  signal(SIGINT,SIG_IGN);
#endif
  
  /* 未読記事のチェックだけの場合 */
  if ( check_only ) {
    /* ニュースグループの表示 */
    disp_group();
#if ! defined(NO_NET)
    if ( gn.local_spool == 0 )
      close_server();		/* サーバとの接続を切る */
#endif
    /* 終了 */
    return(0);
  }
  
  /* ニュースグループモード */
  switch ( group_mode()) {
  case CONNECTION_CLOSED:
    mc_puts("\nニュースサーバとの接続が切れました\n");
    if ( change ) {
      char *pt;
      pt = kb_input("既読情報を保存しますか？(n:しない/その他:する)");
      if ( *pt != 'n' && *pt != 'N' )
	save_newsrc();
    }
    break;
  case END:
    if ( change )
      save_newsrc();		/* .newsrc を更新 */
#if ! defined(NO_NET)
    if ( gn.local_spool == OFF )
      close_server();		/* サーバとの接続を切る */
#endif
    break;
  case QUIT:
#if ! defined(NO_NET)
    if ( gn.local_spool == OFF )
      close_server();		/* サーバとの接続を切る */
#endif
    break;
  }
  
  /* 最後に読んだ記事があれば捨てる */
  if ( last_art_check() != ERROR && make_article_tmpfile == ON )
    unlink(last_art.tmp_file);
  
#ifdef	USE_JNAMES
  jnClose();
#endif
#ifdef	QUICKWIN
  _wsetexit(_WINEXITNOPERSIST);
#endif
#if !defined(MSDOS) && !defined(OS2)
  Exit(0);
#endif
  return(0);
}
/*
 * 未読記事のチェックだけの場合の表示
 */
static void
disp_group()
{
  register newsgroup_t *ng;
  register int col;
  
  printf("\n");
  for ( ng = newsrc; ng != 0 ; ng=ng->next) {
    if ( ng->flag & (UNSUBSCRIBE | NOARTICLE)) /* 購読／記事なし */
      continue;
    if ( ng->last <= ng->read ){ /* 新しい記事なし */
      ng->flag |= NOARTICLE;
      ng->numb = 0;
      continue;
    }
    printf("%4ld %s",ng->numb,ng->name);
    if ( gn.description == 1 && ng->desc != 0) {
      col = 5 + strlen(ng->name) + 1;
      printf(" ");
      if ( col < COLPOS ) {
	printf("%*s",COLPOS-col,"");
	col = COLPOS;
      }
      printf("%.*s",gn.columns-col,ng->desc);
    }
    printf("\n");

  }
}
#endif	/* WINDOWS */

/*
 * 起動オプションの解析(gn)
 */
int
read_option(argc,argv,check_only)
int argc;
char *argv[];
int	*check_only;
{
  extern int getopt();
  
  extern char *optarg;
  
  int c;
  char *optopt;
#ifdef	WINDOWS
  char buf[128];
#endif	/* WINDOWS */
  int l_flag,s_flag;
  
#if defined(MSDOS) || defined(OS2)
#ifdef	WINDOWS
  optopt = "h:d:gns";
#else	/* WINDOWS */
  optopt = "h:d:gnsV";
#endif	/* WINDOWS */
#else	/* DOS */
  optopt = "h:d:gnlsV";
#endif	/* MSDOS || OS2 */


  l_flag = s_flag = OFF;
  
  /* 起動オプションの解析 */
  
  while ((c = getopt(argc, argv, optopt)) != EOF) {
    switch (c) {
    case 'h':			/* ニュースサーバを指定する */
      if ( gn.nntpserver != 0 )
	free(gn.nntpserver);
      if ( (gn.nntpserver = malloc(strlen(optarg)+1)) == NULL ) {
	memory_error();
	return(ERROR);
      }
      strcpy(gn.nntpserver,optarg);
      break;

    case 'd':			/* ドメイン名を指定する */
      if ( gn.domainname != 0 )
	free(gn.domainname);
      if ( (gn.domainname = malloc(strlen(optarg)+1)) == NULL ) {
	memory_error();
	return(ERROR);
      }
      strcpy(gn.domainname,optarg);
      break;

#ifndef	WINDOWS
    case 'V':
      printf("%s Version %s %s.  Copyright (C) yamasita@omronsoft.co.jp\n",
	     argv[0],gn_version,gn_date);
      break;
#endif /* WINDOWS */

    case 'g':			/* upper compatible */
      gn.gnus_format = 1;
      break;

    case 'n':
      *check_only = QUIT;
      break;

    case 'l':
      if ( s_flag == ON ) {
#ifndef	WINDOWS
	mc_puts("-l オプションは -s オプションと同時には使えません \n");
	exit(5);
#else  /* WINDOWS */
	mc_MessageBox(hWnd,"-l オプションは -s オプションと同時には使えません ","",MB_OK);
	return(ERROR);
#endif /* WINDOWS */
      }
      l_flag = ON;
      gn.local_spool = ON;
      gn.use_history = ON;
      break;

    case 's':
      if ( l_flag == ON ) {
#ifndef	WINDOWS
	mc_puts("-s オプションは -l オプションと同時には使えません \n");
	exit(5);
#else  /* WINDOWS */
	mc_MessageBox(hWnd,"-s オプションは -l オプションと同時には使えません ","",MB_OK);
	return(ERROR);
#endif /* WINDOWS */
      }
      s_flag = ON;
      gn.with_gnspool = ON;
      gn.local_spool = ON;
      break;

    default:
#ifndef	WINDOWS
      mc_printf("%s [-h nntpserver] [-d domainname][-nlsV]\n",argv[0]);

      mc_puts("   -h nntpserver ニュースサーバを指定する \n");
      mc_puts("   -d domainname ドメイン名を指定する \n");
      mc_puts("   -n            未読の記事数を表示して終了 \n");
      mc_puts("   -l            ローカルスプールモード \n");
      mc_puts("   -s            gnspool と組み合わせて使用する \n");
      mc_puts("   -V            バージョンの表示 \n");
      exit(5);
#else  /* WINDOWS */
      sprintf(buf,"%c: unknown option",c);
      mc_MessageBox(hWnd,buf,"",MB_OK);
      return(ERROR);
#endif /* WINDOWS */
    }
  }
  return(CONT);
}
