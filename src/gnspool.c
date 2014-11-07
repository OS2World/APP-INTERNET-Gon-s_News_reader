/*
 * @(#)gnspool.c	1.40 May.16,1998
 *
 * 著作権は放棄しません。ただし、営利目的以外の使用／配布に制限は設けません。
 * Copyright (C) yamasita@omronsoft.co.jp
 */

#define	HEADANDBODY

#if defined(WINDOWS)
#	include	<windows.h>
#	ifndef WIN32
#		include	<toolhelp.h>
#	endif
#	include	"wingnsp.h"
#endif	/* WINDOWS */

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>

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
#	include	<dir.h>
#endif

#if defined(MSDOS) || defined(OS2)
#	if	defined ( OS2 )
#		define INCL_DOSFILEMGR
#		include <os2.h>
#	else
#		if defined (HUMAN68K)
#			include	<sys/dos.h>
#			include	<unistd.h>
#		else
#			include	<dos.h>
#		endif
#	endif
#endif /* MSDOS || OS2 */

#ifdef	__TURBOC__
extern unsigned _stklen = 10240;
#endif

#ifdef UNIX
#	if defined(NDIR)
#		include	<dir.h>
#	else
#		if defined(MEUX) || defined(SVR4)
#			include <dirent.h>
#		else
#			include	<sys/dir.h>
#		endif
#	endif
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

#define	READED		1
#define	EXISTS_SERVER	2
#define	EXISTS_LOCAL	4

extern void mc_puts(),mc_printf();
extern char *kb_input();
extern void save_newsrc();
extern int post_file();
extern int mail_file();
extern void smtp_close_server();
extern int get_1_header_file();
extern void hit_return();
extern int put_server();
extern int get_server();
extern void ng_name_copy();
extern void ng_directory();
extern void memory_error();
extern char *article_filename();
extern newsgroup_t *search_group();

extern int gn_init();
extern int get_newsrc();
extern int open_server();
extern int get_active();
extern int get_newsgroups();
extern void check_group();
extern void close_server();
extern void kanji_convert();
#ifdef	DOSFS
extern int SetDisk();
#endif
#ifdef	MIME
extern void MIME_strHeaderDecode();
extern void MIME_strHeaderEncode();
#endif

static int post();
static int reply();
extern int expire();
extern int expire_1ng();
static int unlink_1file();
static long is_articlefile();
extern int make_newsgroups();
extern int make_history();
extern int make_history_1ng();
extern int get_article();
extern int get_article_1ng();
extern int make_active();
static int post_article();
static int post_article_print_head();
static int post_article_edit();
static int send_mail();
static int send_mail_print_head();
static int send_mail_edit();
static int mk_ng_dir();
static char *mk_unread_list();
static int get_articles();
static int get_article_file();
static int check_exist();
#ifdef	XHDR_XREF
static int check_xref();
#endif
#ifdef	HEADANDBODY
static int head_command();
static int body_command();
#else
static int article_command();
#endif
static int update_readed();

static int add_to_history();
static void setdirtime();
extern char *find_first_file(),*find_next_file();

#if defined(WINDOWS)
extern int mc_MessageBox();
extern void set_hourglass(),reset_cursor();
#endif	/* WINDOWS */

int prog = GNSPOOL;
int newsrc_flag;		/* newsrc を読み込んだら０、なかったら−１ */

int interactive_send = ON;	/* interactive */
int update_newsrc = OFF;
int cnews = OFF;

FILE *history_fp;

/***********************************************************************
 pm	auth()
 p g	open_server()
 p	post()
  m	reply()
e  g	get_newsrc()
e	get_active():local
e  g	get_active():server
e  g	check_group()
   g	get_newsgroups()
e	expire()
   g	make_newsgroups()
   g	get_article()
 p g	close_server()
e  g	make_active()
   g	make_history()
************************************************************************/


#ifndef	WINDOWS
int
main(argc,argv)
int argc;
char *argv[];
{
#if defined(MSDOS) || defined(OS2)
  char *pwd;
  
  if ( (pwd = getcwd(NULL,PATH_LEN)) == NULL ) {
    perror("getcwd");
    exit(1);
  }
#endif
  
  /* 起動オプションの解析 */
  gn_init(argc,argv);
  gn.local_spool = OFF;

#if defined (DOSFS)
  if ( gn.newsspool[1] == ':' ) {
    if (SetDisk(toupper(gn.newsspool[0])-'A')){
      mc_printf("%s は使えません \n",gn.newsspool);
      exit(1);
    }
  }
#endif

#if ! defined(NO_NET)
  if ( DO_POST || DO_GET ) {
    /* ニュースサーバとの接続  */
    mc_printf("%s と接続しています\n",gn.nntpserver);
    open_server();
  }
  
  if ( DO_POST ) {
    /* 記事の投稿 */
    if (post() != CONT )
      exit(1);
  }
  
  if ( DO_MAIL ) {
    /* リプライメイルの送信 */
    if (reply() != CONT )
      exit(1);
  }
#endif
  
  /* アクティブファイルの取得 */
  if ( DO_EXPIRE || DO_GET ) {
    switch ( get_active() ) {
    case CONNECTION_CLOSED:
      mc_puts("\nニュースサーバとの接続が切れています\n");
      exit(1);
    case ERROR:
      exit(1);
    }
  }
  
  if ( DO_EXPIRE || DO_GET ) {
    /* .newsrc ファイルの読み込み */
    if ( (newsrc_flag = get_newsrc()) == ERROR )
      exit(1);
  }
  
  if ( DO_EXPIRE || DO_GET ) {
    /* ニュースグループのチェック */
    if ( interactive_send == OFF )
      check_group(1);
    else
      check_group(0);
  }
  
  /* 各グループの説明の取得 */
  if ( gn.description == 1 && DO_GET ) {
    switch ( get_newsgroups() ) {
    case CONNECTION_CLOSED:
      mc_puts("\nニュースサーバとの接続が切れています\n");
      exit(1);
    case ERROR:
      exit(1);
    }
  }
  
  if ( DO_EXPIRE ) {
    /* 既読の記事の消去 */
    if ( expire() != CONT )
      exit(1);
  }
  
#if ! defined(NO_NET)
  if ( gn.description == 1 && DO_GET ) {
    /* newsgroups の作成 */
    if ( make_newsgroups() != CONT )
      exit(1);
  }
    
  if ( DO_GET ) {
    /* 記事の取得 */
    if ( get_article() != CONT )
      exit(1);
  }
  
  if ( DO_POST || DO_GET ) {
    /* サーバとの接続を切る */
    close_server();
  }
#endif

  if ( DO_EXPIRE || DO_GET ) {
    /* active の作成 */
    if ( make_active() != CONT )
      exit(1);
  }
  
  /* history の作成 */
  if ( gn.use_history && DO_GET ) {
    if ( make_history() != CONT ) {
      mc_puts("\nhistory の作成に失敗しました\n");
      exit(1);
    }
  }
  
#if defined(DOSFS)
  SetDisk(pwd[0]-'A');
  chdir(pwd);
#endif /* DOSFS */

#if defined(WINSOCK) && !defined(NO_NET)
  if ( DO_POST || DO_MAIL || DO_GET )
    WSACleanup();
#endif
  
  if ( (update_newsrc == ON && change == 1) || cnews == ON )
    save_newsrc();                  /* .newsrc を更新 */

  mc_printf("\n\nDone!\n");
#ifdef	QUICKWIN
  if ( interactive_send != OFF )
    hit_return();
  _wsetexit(_WINEXITNOPERSIST);
#endif
  return(0);
}
#endif	/* WINDOWS */


#if ! defined(NO_NET)
/*
 * 記事の投稿
 */
#ifndef	WINDOWS
static int
post()
{
  char dir[PATH_LEN];
  struct stat stat_buf;
  char *tmp_file_f;
  
  strcpy(dir,gn.newsspool);
  if ( dir[strlen(dir)-1] != SLASH_CHAR)
    strcat(dir,SLASH_STR);
  strcat(dir,"news.out");
  
  if ( stat(dir,&stat_buf) != 0 )
    return(CONT);	/* ポスト記事なし */
  
  if ( chdir(dir) != 0 ) {
    perror(dir);
    return(ERROR);
  }
  
  mc_puts("\n記事をポストします\n");
  
  if ( (tmp_file_f = find_first_file()) != (char *)0 ) {
    if ( tmp_file_f[0] == 'g' || tmp_file_f[0] == 'G' ) {
      if ( post_article(tmp_file_f) != CONT )
	return(ERROR);
    }

    while ( (tmp_file_f = find_next_file()) != (char *)0 ) {
      if ( tmp_file_f[0] == 'g' || tmp_file_f[0] == 'G' ) {
#ifdef UNIX
	sleep(1);		/* wait. for Inn 1.4 */
#endif
	if ( post_article(tmp_file_f) != CONT )
	  return(ERROR);
      }
    }
  }
  
  if ( chdir(gn.newsspool) != 0 ) {
    perror(gn.newsspool);
    return(CONT);
  }
  
  if ( rmdir(dir) != 0 )
    perror("rmdir");

  return(CONT);
}
static int
post_article(tmp_file_f)
char *tmp_file_f;
{
  register char *pt;
  struct stat stat_buf;

  if ( stat(tmp_file_f,&stat_buf) != 0 )
    return(CONT);	/* ???  */

  if ( stat_buf.st_size == 0 ) {
    if ( unlink(tmp_file_f) != 0 ) {
      perror(tmp_file_f);
      return(ERROR);
    }
    return(CONT);
  }
  
  while (1) {
    /* ヘッダの表示 */
    if ( post_article_print_head(tmp_file_f) != CONT )
      return(ERROR);

    if (interactive_send == OFF) {		 /* -y option */
      if ( post_file(tmp_file_f, gn.spool_kanji_code) != CONT )
	return(ERROR);
      if ( unlink(tmp_file_f) != 0 ) {
	perror(tmp_file_f);
	return(ERROR);
      }
      return(CONT);
    }

    /* 確認入力 */
#ifndef	QUICKWIN
    pt = kb_input("本当にポストしますか？(y:する/e:もう一度編集/s:スキップ/n:中止)");
#else
    pt = kb_input("本当にポストしますか？(y:する/s:スキップ/n:中止)");
#endif
    switch(*pt) {
    case 'y':			/* する */
    case 'Y':
      if ( post_file(tmp_file_f, gn.spool_kanji_code) != CONT )
	return(ERROR);
      if ( unlink(tmp_file_f) != 0 ) {
	perror(tmp_file_f);
	hit_return();
	return(ERROR);
      }
      return(CONT);

    case 'e':			/* もう一度編集 */
    case 'E':
      if ( post_article_edit(tmp_file_f) != CONT )
	return(ERROR);
      break;

    case 's':			/* スキップ */
    case 'S':
      return(CONT);

    case 'n':			/* 中止 */
    case 'N':
      if ( unlink(tmp_file_f) != 0 ) {
	perror(tmp_file_f);
	return(ERROR);
      }
      return(CONT);
    }
  }
}
/*
 * ポスト時の確認用ヘッダ表示
 */
static int
post_article_print_head(tmp_file_f)
char *tmp_file_f;
{
  FILE *fp;
  char buf[NNTP_BUFSIZE+1];
  char buf2[NNTP_BUFSIZE+1];
  char resp[NNTP_BUFSIZE+1];
  
  if ( ( fp = fopen(tmp_file_f,"r") ) == NULL ){
    perror(tmp_file_f);
    hit_return();
    return(ERROR);
  }
  
  printf("\n");
  get_1_header_file(buf,resp,fp);
  
  while (1) {
    if (resp[0] == 0 )
      break;
    get_1_header_file(buf,resp,fp);
    if (buf[0] == 0 )
      break;

#ifdef	MIME
    kanji_convert(gn.spool_kanji_code,buf,JIS,buf2);
    MIME_strHeaderDecode(buf2,buf,sizeof(buf));
    kanji_convert(JIS,buf,gn.display_kanji_code,buf2);
#else
    kanji_convert(gn.spool_kanji_code,buf,gn.display_kanji_code,buf2);
#endif
    printf("%s\n",buf2);
  }
  fclose(fp);
  return(CONT);
}
/*
 * ポスト時の編集
 */
static int
post_article_edit(tmp_file_f)
char *tmp_file_f;
{
  FILE *rp,*wp;
  char tmp_file[PATH_LEN+1];
  char buf[NNTP_BUFSIZE+1];
  char resp[NNTP_BUFSIZE+1];

  /* エディット用テンポラリファイルの作成 */
  strcpy(tmp_file,gn.tmpdir);
  strcat(tmp_file,"gnXXXXXX");
  mktemp(tmp_file);
  if ( ( wp = fopen(tmp_file,"w") ) == NULL ){
    perror(tmp_file);
    hit_return();
    return(ERROR);
  }

  if ( ( rp = fopen(tmp_file_f,"r") ) == NULL ){
    perror(tmp_file_f);
    hit_return();
    return(ERROR);
  }

  while (fgets(resp,NNTP_BUFSIZE,rp) != NULL ) {
    /* 漢字コードを process_kanji_code に変換 */
#ifdef	MIME
    kanji_convert(gn.spool_kanji_code,resp,JIS,buf);	/* spool -> JIS */
    MIME_strHeaderDecode(buf,resp,sizeof(resp));	/* MIME decode */
    kanji_convert(JIS,resp,gn.process_kanji_code,buf);	/* JIS -> process */
#else
    kanji_convert(gn.spool_kanji_code,resp,gn.process_kanji_code,buf);
#endif
    fprintf(wp, "%s", buf);
  }
  fclose(rp);
  fclose(wp);
  
  /* 編集 */
#if defined(QUICKWIN)
  mc_printf("%s を、テキストエディタで編集して下さい\n",tmp_file);
  hit_return();
#else
  sprintf(resp,"%s %s",gn.editor, tmp_file);
  if ( system(resp) == -1 ){
    perror(resp);
    return(ERROR);
  }
#endif

  /* 元のファイルへ書き戻し */
  if ( ( wp = fopen(tmp_file_f,"w") ) == NULL ){
    perror(tmp_file_f);
    hit_return();
    return(ERROR);
  }
  if ( ( rp = fopen(tmp_file,"r") ) == NULL ){
    perror(tmp_file);
    hit_return();
    return(ERROR);
  }
  while (fgets(resp,NNTP_BUFSIZE,rp) != NULL ) {
    /* 漢字コードを spool_kanji_code に変換 */
#ifdef	MIME
    if ( (gn.mime_head & MIME_ENCODE) != 0 ) {
      kanji_convert(gn.process_kanji_code,resp,JIS,buf);/* process -> JIS */
      MIME_strHeaderEncode(buf,resp,sizeof(resp));	/* MIME encode */
      kanji_convert(JIS,resp,gn.spool_kanji_code,buf);	/* JIS -> spool */
    } else {
      kanji_convert(gn.process_kanji_code,resp,gn.spool_kanji_code,buf);
    }
#else
    kanji_convert(gn.process_kanji_code,resp,gn.spool_kanji_code,buf);
#endif
    fprintf(wp, "%s", buf);
  }
  fclose(rp);
  fclose(wp);

  unlink(tmp_file);
  return(CONT);  
}
#endif	/* WINDOWS */
#endif	/* NO_NET */

#if ! defined(NO_NET)
/*
 * リプライメイルの送信
 */
#if ! defined(WINDOWS)
static int
reply()
{
  char dir[PATH_LEN];
  struct stat stat_buf;
  char *tmp_file_f;
  
  strcpy(dir,gn.newsspool);
  if ( dir[strlen(dir)-1] != SLASH_CHAR)
    strcat(dir,SLASH_STR);
  strcat(dir,"mail.out");
  
  if ( stat(dir,&stat_buf) != 0 )
    return(CONT);		/* リプライメイルなし */
  
  if ( chdir(dir) != 0 ) {
    perror(dir);
    return(ERROR);
  }
  
  mc_puts("\nリプライメイルを送信します\n");
  
  if ( (tmp_file_f = find_first_file()) != (char *)0 ) {
    if ( tmp_file_f[0] == 'g' || tmp_file_f[0] == 'G' ) {
      if ( send_mail(tmp_file_f) != CONT )
	return(ERROR);
    }

    while ( (tmp_file_f = find_next_file()) != (char *)0 ) {
      if ( tmp_file_f[0] == 'g' || tmp_file_f[0] == 'G' ) {
	if ( send_mail(tmp_file_f) != CONT )
	  return(ERROR);
      }
    }
  }

  if ( init_smtp_server != 0 )
    smtp_close_server();
  
  if ( chdir(gn.newsspool) != 0 ) {
    perror(gn.newsspool);
    return(CONT);
  }
  
  if ( rmdir(dir) != 0 )
    perror("rmdir");

  return(CONT);
}
static int
send_mail(tmp_file_f)
char *tmp_file_f;
{
  register char *pt;
  struct stat stat_buf;

  if ( stat(tmp_file_f,&stat_buf) != 0 )
    return(CONT);	/* ???  */

  if ( stat_buf.st_size == 0 ) {
    if ( unlink(tmp_file_f) != 0 ) {
      perror(tmp_file_f);
      return(ERROR);
    }
    return(CONT);
  }

  while (1) {
    /* ヘッダの表示 */
    if ( send_mail_print_head(tmp_file_f) != CONT )
      return(ERROR);

    if (interactive_send == OFF) {		 /* -y option */
      if ( mail_file(tmp_file_f, gn.mail_kanji_code) != CONT )
	return(ERROR);
      if ( unlink(tmp_file_f) != 0 ) {
	perror(tmp_file_f);
	return(ERROR);
      }
      return(CONT);
    }

    /* 確認入力 */
#ifndef	QUICKWIN
    pt = kb_input("本当にリプライしますか？(y:する/e:もう一度編集/s:スキップ/n:中止)");
#else
    pt = kb_input("本当にリプライしますか？(y:する/s:スキップ/n:中止)");
#endif
    switch(*pt) {
    case 'y':			/* する */
    case 'Y':
      if ( mail_file(tmp_file_f, gn.mail_kanji_code) != CONT )
	return(ERROR);
      if ( unlink(tmp_file_f) != 0 ) {
	perror(tmp_file_f);
	hit_return();
	return(ERROR);
      }
      return(CONT);

    case 'e':			/* もう一度編集 */
    case 'E':
      if ( send_mail_edit(tmp_file_f) != CONT )
	return(ERROR);
      break;

    case 's':			/* スキップ */
    case 'S':
      return(CONT);

    case 'n':			/* 中止 */
    case 'N':
      if ( unlink(tmp_file_f) != 0 ) {
	perror(tmp_file_f);
	return(ERROR);
      }
      return(CONT);
    }
  }
}
/*
 * メイル送信時の確認用ヘッダ表示
 */
static int
send_mail_print_head(tmp_file_f)
char *tmp_file_f;
{
  FILE *fp;
  char buf[NNTP_BUFSIZE+1];
  char buf2[NNTP_BUFSIZE+1];
  char resp[NNTP_BUFSIZE+1];
  
  if ( ( fp = fopen(tmp_file_f,"r") ) == NULL ){
    perror(tmp_file_f);
    hit_return();
    return(ERROR);
  }
  
  printf("\n");
  get_1_header_file(buf,resp,fp);
  
  while (1) {
    if (resp[0] == 0 )
      break;
    get_1_header_file(buf,resp,fp);
    if (buf[0] == 0 )
      break;

#ifdef	MIME
    kanji_convert(gn.mail_kanji_code,buf,JIS,buf2);
    MIME_strHeaderDecode(buf2,buf,sizeof(buf));
    kanji_convert(JIS,buf,gn.display_kanji_code,buf2);
#else
    kanji_convert(gn.mail_kanji_code,buf,gn.display_kanji_code,buf2);
#endif
    printf("%s\n",buf2);
  }
  fclose(fp);
  return(CONT);
}
/*
 * メイル送信時の編集
 */
static int
send_mail_edit(tmp_file_f)
char *tmp_file_f;
{
  FILE *rp,*wp;
  char tmp_file[PATH_LEN+1];
  char buf[NNTP_BUFSIZE+1];
  char resp[NNTP_BUFSIZE+1];

  /* エディット用テンポラリファイルの作成 */
  strcpy(tmp_file,gn.tmpdir);
  strcat(tmp_file,"gnXXXXXX");
  mktemp(tmp_file);
  if ( ( wp = fopen(tmp_file,"w") ) == NULL ){
    perror(tmp_file);
    hit_return();
    return(ERROR);
  }

  if ( ( rp = fopen(tmp_file_f,"r") ) == NULL ){
    perror(tmp_file_f);
    hit_return();
    return(ERROR);
  }

  while (fgets(resp,NNTP_BUFSIZE,rp) != NULL ) {
    /* 漢字コードを process_kanji_code に変換 */
#ifdef	MIME
    kanji_convert(gn.mail_kanji_code,resp,JIS,buf);	/* mail -> JIS */
    MIME_strHeaderDecode(buf,resp,sizeof(resp));	/* MIME decode */
    kanji_convert(JIS,resp,gn.process_kanji_code,buf);	/* JIS -> process */
#else
    kanji_convert(gn.mail_kanji_code,resp,gn.process_kanji_code,buf);
#endif
    fprintf(wp, "%s", buf);
  }
  fclose(rp);
  fclose(wp);
  
  /* 編集 */
#if defined(QUICKWIN)
  mc_printf("%s を、テキストエディタで編集して下さい\n",tmp_file);
  hit_return();
#else
  sprintf(resp,"%s %s",gn.editor, tmp_file);
  if ( system(resp) == -1 ){
    perror(resp);
    return(ERROR);
  }
#endif

  /* 元のファイルへ書き戻し */
  if ( ( wp = fopen(tmp_file_f,"w") ) == NULL ){
    perror(tmp_file_f);
    hit_return();
    return(ERROR);
  }
  if ( ( rp = fopen(tmp_file,"r") ) == NULL ){
    perror(tmp_file);
    hit_return();
    return(ERROR);
  }
  while (fgets(resp,NNTP_BUFSIZE,rp) != NULL ) {
    /* 漢字コードを spool_kanji_code に変換 */
#ifdef	MIME
    if ( (gn.mime_head & MIME_ENCODE) != 0 ) {
      kanji_convert(gn.process_kanji_code,resp,JIS,buf);/* process -> JIS */
      MIME_strHeaderEncode(buf,resp,sizeof(resp));	/* MIME encode */
      kanji_convert(JIS,resp,gn.mail_kanji_code,buf);	/* JIS -> mail */
    } else {
      kanji_convert(gn.process_kanji_code,resp,gn.spool_kanji_code,buf);
    }
#else
    kanji_convert(gn.process_kanji_code,resp,gn.spool_kanji_code,buf);
#endif
    fprintf(wp, "%s", buf);
  }
  fclose(rp);
  fclose(wp);

  unlink(tmp_file);
  return(CONT);  
}
#endif	/* WINDOWS */
#endif	/* NO_NET */

/*
 * 既読の記事の消去
 */
int
expire()
{
#ifndef	WINDOWS
  register newsgroup_t *ng;
#endif	/* WINDOWS */
  
  mc_puts("\n既読の記事を消去しています\n");
  
#ifndef	WINDOWS
  for ( ng = newsrc ; ng != 0 ; ng=ng->next) {
    if ( expire_1ng(ng) != CONT )
      return(ERROR);
  }
#endif	/* WINDOWS */
  return(CONT);
}
int
expire_1ng(ng)
newsgroup_t *ng;
{
  char dir[PATH_LEN];
  char *name;
  struct stat stat_buf;
  register char *pt;
  register char *readed;
  register long base,artnum;
  int flag = 0;			/* display NG name */

  ng_directory(dir,ng->name);
  
  if ( stat(dir,&stat_buf) != 0 )
    return(CONT);
  
  if ( chdir(dir) != 0 )
    return(CONT);
  
  if (ng->flag & UNSUBSCRIBE ) {
    if ( (name = find_first_file()) != (char *)0 ) {
      if ( '0' <= name[0] && name[0] <='9' ){
	if ( unlink_1file(ng,name,&flag) != CONT )
	  return(ERROR);
      }
    }
    while ( (name = find_next_file()) != (char *)0 ) {
      if ( '0' <= name[0] && name[0] <='9' ){
	if ( unlink_1file(ng,name,&flag) != CONT )
	  return(ERROR);
      }
    }
  } else {
    base = ng->read;
    if ( (readed = mk_unread_list(ng)) == NULL )
      return(ERROR);
    
    if ( (name = find_first_file()) != (char *)0 ) {
      if ( '0' <= name[0] && name[0] <='9' &&
	  (artnum = is_articlefile(name)) != NOT_ARTICLE ){
	if (artnum <= ng->read ||
	    artnum < ng->first ||
	    artnum > ng->last ||
	    readed[artnum-base] == READED ) {
	  if ( unlink_1file(ng,name,&flag) != CONT )
	    return(ERROR);
	}
      }
      while ( (name = find_next_file()) != (char *)0 ) {
	if ( '0' <= name[0] && name[0] <='9' &&
	  (artnum = is_articlefile(name)) != NOT_ARTICLE ){
	  if (artnum <= ng->read ||
	      artnum < ng->first ||
	      artnum > ng->last ||
	      readed[artnum-base] == READED ) {
	    if ( unlink_1file(ng,name,&flag) != CONT )
	      return(ERROR);
	  }
	}
      }
    }
    free(readed);
  }

  if ( flag != 0 )
    mc_printf("\n");
  
  if ( chdir(gn.newsspool) != 0 ) {
#ifdef	WINDOWS
    mc_MessageBox(hWnd,"cd できません ", gn.newsspool,MB_OK);
#else  /* WINDOWS */
    perror(gn.newsspool);
#endif /* WINDOWS */
    return(ERROR);
  }
  
  while (1) {
    if ( rmdir(dir) != 0 )
      break;
    mc_printf("rmdir %s\n",dir);
    if ( (pt = strrchr(dir,SLASH_CHAR)) == NULL )
      break;
    *pt = 0;
    if ( strcmp(dir,gn.newsspool) == 0 )
      break;
  }
  
  return(CONT);
}
static int
unlink_1file(ng,name,flag)
newsgroup_t *ng;
char *name;
int *flag;
{
  if ( is_articlefile(name) == NOT_ARTICLE )
    return(CONT);

#if defined(OS2_32BIT)
  /* to avoid undelete function of OS/2 Warp, use DosForceDelete() */
  if (DosForceDelete(name) != 0 ) {
    perror(name);
    return(ERROR);
  }
#else
  if (unlink(name) != 0 ) {
#ifdef	WINDOWS
    mc_MessageBox(hWnd,"削除できません ",name,MB_OK);
#else	/* WINDOWS */
    perror(name);
#endif	/* WINDOWS */
    return(ERROR);
  }
#endif /* OS2_32BIT */
  if ( *flag == 0 ) {
    mc_printf("%s: removing article %s",ng->name,name);
    *flag = 1;
  } else {
    mc_printf(" %s",name);
  }
  return(CONT);
}
static long
is_articlefile(name)
char *name;
{
  struct stat stat_buf;
  register char *pt;
  long art_no = 0L;

  if ( gn.dosfs == ON && strlen(name) > BASENAME_LEN ) {
    if ( name[BASENAME_LEN-1] != '.' )
      return(NOT_ARTICLE);
    for ( pt = name; *pt != 0 ; pt++ ) {
      if ( *pt < '0' || '9' < *pt ) {
	if ( pt == &name[BASENAME_LEN-1] && *pt == '.')
	  continue;
	return(NOT_ARTICLE);
      }
      art_no = art_no * 10L + (long)*pt - (long)'0';
    }
  } else {
    for ( pt = name; *pt != 0 ; pt++ ) {
      if ( *pt < '0' || '9' < *pt )
	return(NOT_ARTICLE);
    }
    art_no = atol(name);
  }

  if ( stat(name,&stat_buf) != 0 )
    return(NOT_ARTICLE);
  if ( (stat_buf.st_mode & S_IFMT) != S_IFREG )
    return(NOT_ARTICLE);

  return(art_no);
}

/*
 * active の作成
 */
int
make_active()
{
  register newsgroup_t *ng;
  char active_file[PATH_LEN];
  FILE *fp;
  struct stat stat_buf;
  long first,art_no;
  char dir[PATH_LEN];
  char *name;

  if ( stat(gn.newslib,&stat_buf) != 0 ) {	/* newslib がなければ作る */
    mc_printf("mkdir %s\n",gn.newslib);
    if ( MKDIR(gn.newslib) != 0 ) {
#ifdef	WINDOWS
      mc_MessageBox(hWnd,"ディレクトリを作成できません ",
		    gn.newslib,MB_OK);
#else  /* WINDOWS */
      perror(gn.newslib);
#endif /* WINDOWS */
      return(ERROR);
    }
  }
  strcpy(active_file,gn.newslib);
  if ( active_file[strlen(active_file)-1] != SLASH_CHAR)
    strcat(active_file,SLASH_STR);
  strcat(active_file,"active");
  
  mc_printf("\n%s を作成しています\n",active_file);
  if ( (fp = fopen(active_file,"w")) == NULL ) {
#ifdef	WINDOWS
    mc_MessageBox(hWnd,"作成に失敗しました", active_file,MB_OK);
#else  /* WINDOWS */
    mc_printf("\n%s の作成に失敗しました\n",active_file);
    perror("active");
#endif /* WINDOWS */
    return(ERROR);
  }
  
  for ( ng = newsrc ; ng != 0 ; ng=ng->next) {

    if ( ng->read > ng->first )
      first = ng->read + 1;
    else
      first = ng->first;

    if ( DO_EXPIRE == NO ) {	/* expire しないとき */
      ng_directory(dir,ng->name);

      if ( stat(dir,&stat_buf) == 0 ) {
	if ( chdir(dir) == 0 ) {
      
	  if ( (name = find_first_file()) != (char *)0 ) {
	    if ( (art_no = is_articlefile(name)) != NOT_ARTICLE ) {
	      if ( art_no < first )
		first = art_no;
	    }
	    while ( (name = find_next_file()) != (char *)0 ) {
	      if ( (art_no = is_articlefile(name)) != NOT_ARTICLE ) {
		if ( art_no < first )
		  first = art_no;
	      }
	    }
	  }
	}
      }
      if ( chdir(gn.newsspool) != 0 ) {
#ifdef	WINDOWS
	mc_MessageBox(hWnd,"cd できません ", gn.newsspool,MB_OK);
#else  /* WINDOWS */
	perror(gn.newsspool);
#endif /* WINDOWS */
	return(ERROR);
      }
      
    }

    if ( fprintf(fp,"%s %ld %ld y\n",ng->name,ng->last,first) == -1 ) {
#ifdef	WINDOWS
      mc_MessageBox(hWnd,"作成に失敗しました", active_file,MB_OK);
#else  /* WINDOWS */
      perror("active");
#endif /* WINDOWS */
      return(ERROR);
    }
  }
  
  if ( fclose(fp) == EOF ){
    mc_printf("\n%s の作成に失敗しました\n",active_file);
    return(ERROR);
  }
  
  return(CONT);
}

#if ! defined(NO_NET)

/*
 * newsgroups の作成
 */
int
make_newsgroups()
{
  register newsgroup_t *ng;
  char newsgroups_file[PATH_LEN];
  FILE *fp;
  struct stat stat_buf;
  char buf[NNTP_BUFSIZE+1];
  char buf2[NNTP_BUFSIZE+1];

  if ( stat(gn.newslib,&stat_buf) != 0 ) {	/* newslib がなければ作る */
    mc_printf("mkdir %s\n",gn.newslib);
    if ( MKDIR(gn.newslib) != 0 ) {
#ifdef	WINDOWS
      mc_MessageBox(hWnd,"ディレクトリを作成できません ",
		    gn.newslib,MB_OK);
#else  /* WINDOWS */
      perror(gn.newslib);
#endif /* WINDOWS */
      return(ERROR);
    }
  }
  strcpy(newsgroups_file,gn.newslib);
  if ( newsgroups_file[strlen(newsgroups_file)-1] != SLASH_CHAR)
    strcat(newsgroups_file,SLASH_STR);
  strcat(newsgroups_file,"newsgroups");

  mc_printf("\n%s を作成しています\n",newsgroups_file);
  if ( (fp = fopen(newsgroups_file,"w")) == NULL ) {
#ifdef	WINDOWS
    mc_MessageBox(hWnd,"newsgroups の作成に失敗しました",
		  "",MB_OK);
#else  /* WINDOWS */
    mc_printf("\n%s の作成に失敗しました\n",newsgroups_file);
    perror("newsgroups");
#endif /* WINDOWS */
    return(ERROR);
  }

  for ( ng = newsrc ; ng != 0 ; ng=ng->next) {
    if ( ng->desc == 0 )
      continue;
    strcpy(buf,ng->desc);
    kanji_convert(internal_kanji_code,buf,gn.gnspool_kanji_code,buf2);
    if ( fprintf(fp,"%s\t%s\n",ng->name,buf2) == -1 ) {
#ifdef	WINDOWS
      mc_MessageBox(hWnd,"newsgroups の作成に失敗しました",
		    "",MB_OK);
#else  /* WINDOWS */
      perror("newsgroups");
#endif /* WINDOWS */
      return(ERROR);
    }
  }

  if ( fclose(fp) == EOF ) {
    mc_printf("\n%s の作成に失敗しました\n",newsgroups_file);
    return(ERROR);
  }
  return(CONT);
}

/*
 * 記事の取得
 */
int
get_article()
{
#ifndef	WINDOWS
  newsgroup_t *ng;
#endif	/* WINDOWS */
  struct stat stat_buf;
  
  mc_puts("\nニュースサーバから未読の記事を取り寄せています\n");
  
  if ( stat(gn.newsspool,&stat_buf) != 0 ) { /* newsspool がなければ作る */
    mc_printf("mkdir %s\n",gn.newsspool);
    if ( MKDIR(gn.newsspool) != 0 ) {
#ifdef	WINDOWS
      mc_MessageBox(hWnd,"ディレクトリを作成できません ",
		    gn.newsspool,MB_OK);
#else  /* WINDOWS */
      perror(gn.newsspool);
#endif /* WINDOWS */
      return(ERROR);
    }
  }
  
#ifndef	WINDOWS
  for ( ng = newsrc ; ng != 0 ; ng=ng->next) {
    if ( get_article_1ng(ng) != 0 )
      return(ERROR);
  }
#endif	/* WINDOWS */
  
  return(CONT);
}
/*
 * 一つのニュースグループの記事の取得
 */
int
get_article_1ng(ng)
register newsgroup_t *ng;
{
  extern int group_command_set_numb();

  register char *readed;

  if ( ng->flag & UNSUBSCRIBE )
    return(CONT);

  if ( gn.fast_connect == ON ) {  /* fast connect mode */
    switch ( group_command_set_numb(ng) ) {
    case ERROR:
      return(CONT);
    case CONNECTION_CLOSED:
      return(ERROR);
    }
    if ( ng->flag & NOARTICLE || ng->numb <= 0 ) {
      mc_printf("\n%s: 0 article(s)\n",ng->name);
      return(CONT);
    }
  }

  if ( ng->flag & NOARTICLE || ng->numb <= 0 )
    return(CONT);

  mc_printf("\n%s: %ld article(s) %ld-%ld\n",
	    ng->name,ng->numb,ng->read+1,ng->last);

  if ( mk_ng_dir(ng) != CONT )
    return(ERROR);

  if ( (readed = mk_unread_list(ng)) == NULL )
    return(ERROR);

  if ( get_articles(ng,readed) != CONT )
    return(ERROR);

  if ( update_newsrc == ON ) {
    if ( update_readed(ng,readed) != CONT )
      return(ERROR);
  }

  free(readed);

  if ( cnews == ON ) {
    change = 1;
    ng->read = ng->last;
  }

  return(CONT);
}
static int
mk_ng_dir(ng)
register newsgroup_t *ng;
{
  char dir[PATH_LEN];
  char ng_dir[PATH_LEN];
  register char *pt;
  struct stat stat_buf;
  
  ng_name_copy(ng_dir,ng->name);
  pt = ng_dir;
  while( (pt = strchr(pt,'.')) != NULL ) {
    *pt = 0;
    strcpy(dir,gn.newsspool);
    if ( dir[strlen(dir)-1] != SLASH_CHAR)
      strcat(dir,SLASH_STR);
    strcat(dir,ng_dir);

    if ( stat(dir,&stat_buf) != 0 ) {
      mc_printf("mkdir %s\n",dir);
      if ( MKDIR(dir) != 0 ) {
#ifdef	WINDOWS
	mc_MessageBox(hWnd,"ディレクトリを作成できません ",
		      dir,MB_OK);
#else  /* WINDOWS */
	perror(dir);
#endif /* WINDOWS */
	return(ERROR);
      }
    }
    *pt++ = SLASH_CHAR;
  }
  strcpy(dir,gn.newsspool);
  if ( dir[strlen(dir)-1] != SLASH_CHAR)
    strcat(dir,SLASH_STR);
  strcat(dir,ng_dir);
  
  if ( stat(dir,&stat_buf) != 0 ) {
    mc_printf("mkdir %s\n",dir);
    if ( MKDIR(dir) != 0 ) {
#ifdef	WINDOWS
      mc_MessageBox(hWnd,"ディレクトリを作成できません ",
		    dir,MB_OK);
#else  /* WINDOWS */
      perror(dir);
#endif /* WINDOWS */
      return(ERROR);
    }
  }
  if ( chdir(dir) != 0 ) {
#ifdef	WINDOWS
    mc_MessageBox(hWnd,"cd できません ", dir,MB_OK);
#else  /* WINDOWS */
    perror(dir);
#endif /* WINDOWS */
    return(ERROR);
  }
  
  return(CONT);
}
#endif

static char *
mk_unread_list(ng)
register newsgroup_t *ng;
{
  register char *readed;
  register long st,ed,base;
  register char *pt;
  
  base = ng->read;
  
#if defined(MSDOS) && (defined(MSC) || defined(__TURBOC__))
  readed = (char *)malloc(sizeof(char)*(short)(ng->last-ng->read+1));
  if ( readed == NULL ) {
#ifdef	WINDOWS
    mc_MessageBox(hWnd,"メモリが足りません ","",MB_OK);
#else  /* WINDOWS */
    perror("unread list");
#endif /* WINDOWS */
    return(NULL);
  }
  memset(readed,0,(short)(ng->last-ng->read+1));
#else
  readed = (char *)malloc(sizeof(char)*(ng->last-ng->read+1));
  if ( readed == NULL ) {
    perror("unread list");
    return(NULL);
  }
  memset(readed,0,(ng->last-ng->read+1));
#endif
  
  if ( ng->newsrc == 0)	/* GNUS フォーマットなし */
    return(readed);
  
  /* newsrc 行から既読リスト作成 */
  pt = ng->newsrc;
  while (*pt != 0 ) {
    if ( *pt < '0' || '9' < *pt )
      break;
    st = atol(pt);
    while( '0' <= *pt && *pt <= '9' )
      pt++;
    if ( ng->read <= st && st <= ng->last)
      readed[st-base] = READED;
    switch ( *pt ) {
    case 0:
      break;
    case ',':
      pt++;
      break;
    case '-':
      pt++;
      ed = atol(pt);
      while( '0' <= *pt && *pt <= '9' )
	pt++;
      if ( *pt == ',' )
	pt++;
      for ( ; st <= ed ; st++) {
	if ( ng->read <= st && st <= ng->last)
	  readed[st-base] = READED;
      }
      break;
    }
  }
  return(readed);
}

#if ! defined(NO_NET)
static int
get_articles(ng,readed)
register newsgroup_t *ng;
register char *readed;
{
  extern int group_command();
  
  register long base,st,start;
  struct stat stat_buf;
#ifdef	XHDR_XREF
  long	area_st,area_ed;
#endif /* XHDR_XREF */

  base = ng->read;
  
  /* 最初の未読記事 */
  st = ng->read + 1;
  for ( start = ng->read+1;
       start <= ng->last && readed[start-base] == READED; start++)
    st = start;

  /* キャンセルされた記事を消す */
  if ( gn.remove_canceled == ON ) {
    if ( group_command(ng->name) != CONT )
      return(ERROR);
    if ( check_exist(st,ng->last,readed,base) != CONT )
      return(ERROR);

    for (st = start ; st <= ng->last; st++) {
      if (stat(article_filename(st),&stat_buf) == 0 &&
	  (readed[st-base] & EXISTS_SERVER) == 0 ) {
	/* ローカルスプールに存在するが、ニュースサーバに存在しない */
	mc_printf("%s: article #%ld canceled\n",ng->name,st);
	unlink(article_filename(st));
	readed[st-base] |= READED;
	change = 1;
      }
    }
  }
#ifdef	XHDR_XREF
  /* クロスポストされた記事のリンク／コピー */
  for ( st = start ; st <= ng->last; st++) {
    if ( (readed[st-base] & READED) != 0 )
      continue;

    if ( stat(article_filename(st),&stat_buf) == 0 ) { /* already exist */
      readed[st-base] |= EXISTS_LOCAL;
      continue;
    }

    if ( group_command(ng->name) != CONT )
      return(ERROR);

    area_st = area_ed = st;
    st++;
    for (  ; st <= ng->last; st++) {
      if ( (readed[st-base] & READED) != 0 )
	break;
      if ( stat(article_filename(st),&stat_buf) == 0 ) { /* already exist */
	readed[st-base] |= EXISTS_LOCAL;
	break;
      }
      area_ed = st;
    }
    /* area_st と area_ed の間の記事がない*/
    if ( check_xref(ng,area_st,area_ed,readed,base) != CONT )
      return(ERROR);
  }
#endif /* XHDR_XREF */

  /* 記事を得る */
  for ( st = start ; st <= ng->last; st++) {
    if ( (readed[st-base] & (READED|EXISTS_LOCAL)) != 0 )
      continue;

    if ( stat(article_filename(st),&stat_buf) == 0 ) /* already exist */
      continue;

#ifndef	XHDR_XREF
    if ( group_command(ng->name) != CONT )
      return(ERROR);
#endif /* ! XHDR_XREF */

    if ( get_article_file(ng,readed,base,st) != CONT )
      return(ERROR);
  }
  return(CONT);
}
#ifdef	HEADANDBODY
static int
get_article_file(ng,readed,base,st)
register newsgroup_t *ng;
register char *readed;
long base,st;
{
  FILE *fp;
  char resp[NNTP_BUFSIZE+1];
  char resp2[NNTP_BUFSIZE+1];
  char xref[NNTP_BUFSIZE+1];
  register char *ng_name, *pt;
  char file_name[NNTP_BUFSIZE+1];
  long art_num;
  struct stat stat_buf;
  register int x = 0;
  int lines;

  mc_printf("%s: getting article #%ld ",ng->name,st);
  
  switch (head_command(st)) {
  case OK_HEAD:
    break;
  case ERR_NOARTIG:
    mc_printf("canceled ???\n");

    if ( update_newsrc == ON ) {
      readed[st-base] |= READED; /* キャンセルされた記事を既読に */
      change = 1;
    }
    return(CONT);
  case ERROR:			/* other */
    return(ERROR);
  }
  
  if ( (fp = fopen(article_filename(st),"w")) == NULL ) {
#ifdef	WINDOWS
    reset_cursor();
    mc_MessageBox(hWnd,"ファイルのオープンに失敗しました",
		  article_filename(st),MB_OK);
#else  /* WINDOWS */
    perror(article_filename(st));
#endif /* WINDOWS */
    return(ERROR);
  }
  
  /** head 部の取得 **********************************************/
  xref[0] = 0;
  lines = -1;
  while(1) {
    if ( ++x > 20 ){
      mc_printf(".");
      fflush(stdout);
      x = 0;
#ifdef	QUICKWIN
      _wyield();
#endif
    }
    if ( get_server(resp,NNTP_BUFSIZE)) {
#ifdef	WINDOWS
      reset_cursor();
#endif /* WINDOWS */
      fclose(fp);
      fprintf(stderr,"\nARTICLE: get_server fail\n");
      unlink(article_filename(st));
      return(ERROR);
    }
    if ( resp[0] == '.' && resp[1] == 0)	/* 記事の終り */
      break;

    pt = resp;
    if ( resp[0] == '.' && resp[1] == '.')
      pt++;

#ifdef	MIME
    if ( (gn.gnspool_mime_head & MIME_DECODE) != 0 ) {
      kanji_convert(gn.spool_kanji_code,pt,JIS,resp2); /* spool -> JIS */
      MIME_strHeaderDecode(resp2,resp,sizeof(resp));
      kanji_convert(JIS,resp,gn.gnspool_kanji_code,resp2);
    } else
#endif
      kanji_convert(gn.spool_kanji_code,pt,gn.gnspool_kanji_code,resp2);
    pt = resp2;

    if ( strncmp(pt,"Xref: ",6) == 0 )
      strcpy(xref,pt);

    if ( gn.gnspool_line_limit != 0 && strncmp(pt,"Lines: ",7) == 0 )
      lines = atol(&pt[7]);

    if ( fprintf(fp,"%s\n",pt) == -1 ) {
      fclose(fp);
#ifdef	WINDOWS
      reset_cursor();
      mc_MessageBox(hWnd,"ファイルの作成に失敗しました",
		    article_filename(st),MB_OK);
#else  /* WINDOWS */
      perror("\nwrite article");
#endif /* WINDOWS */
      unlink(article_filename(st));
      return(ERROR);
    }
  }

  if ( gn.gnspool_line_limit != 0 && lines > gn.gnspool_line_limit ) {
    fclose(fp);
    unlink(article_filename(st));
    mc_printf("article too long(%d). skip\n",lines);
    return(CONT);
  }

  /* クロスポストされている記事の処理 */
  if ( xref[0] != 0 ) {
    pt = &xref[6];
    /* ニュースサーバ名をスキップ */
    while ( *pt != ' ' && *pt != '\t' )
      pt++;
    while ( *pt == ' ' || *pt == '\t' )
      pt++;

    while (*pt != 0) {
      
      ng_name = pt;		/* ニュースグループ名 */

      if ( (pt = strchr(ng_name,':')) == NULL )
	break;
      *pt++ = 0;

      art_num=atol(pt);		/* 記事番号 */

      /* 記事番号をスキップ */
      while ( '0' <= *pt && *pt <= '9' )
	pt++;
      while ( *pt == ' ' ||  *pt == '\t' )
	pt++;

      if ( strcmp(ng->name, ng_name) == 0 ) /* 自分自身 */
	continue;

      if (search_group(ng_name) == 0 )
	continue;			/* グループが存在しない */

      ng_directory(file_name,ng_name);
      strcat(file_name,SLASH_STR);
      strcat(file_name,article_filename(art_num));

      if (stat(file_name,&stat_buf) != 0 )
	continue;		/* クロスポスト先が存在しない */

      /* クロスポスト先があったので、作りかけの記事ファイルは消す */
      fclose(fp);
      unlink(article_filename(st));

#if defined(UNIX)
      if ( link(file_name,article_filename(st)) == 0 ) {
	mc_printf("link from %s\n",&file_name[strlen(gn.newsspool)+1]);
      } else
#endif  /* UNIX */
	{
	  FILE *rp,*wp;

	  if ( (rp = fopen(file_name,"r")) == NULL )
	    continue;
	  if ( (wp = fopen(article_filename(st),"w")) == NULL )
	    continue;
	  mc_printf("copying from %s ",&file_name[strlen(gn.newsspool)+1]);
	  while ( fgets(resp,NNTP_BUFSIZE,rp) != NULL ) {
	    if ( ++x > 20 ){
	      mc_printf(".");
	      fflush(stdout);
	      x = 0;
#ifdef	QUICKWIN
	      _wyield();
#endif
	    }
	    if ( fputs(resp,wp) == EOF ) {
	      fclose(rp);
	      fclose(wp);
	      unlink(article_filename(st));
	      return(ERROR);
	    }
	  }
	  fclose(rp);		/* error check */
	  fclose(wp);
	  mc_printf("\n");
	}
#ifdef OS2_32BIT
      setdirtime();
#endif /* OS2_32BIT */
      return(CONT);
    }
  }

  /* head と body の間の空行 */
  if ( fprintf(fp,"\n") == -1 ) {
    fclose(fp);
#ifdef	WINDOWS
    reset_cursor();
    mc_MessageBox(hWnd,"ファイルの作成に失敗しました",
		  article_filename(st),MB_OK);
#else  /* WINDOWS */
    perror("\nwrite article");
#endif /* WINDOWS */
    unlink(article_filename(st));
    return(ERROR);
  }

  /** body 部の取得 **********************************************/
  switch (body_command(st)) {
  case OK_BODY:
    break;
  case ERROR:			/* other */
    unlink(article_filename(st));
    return(ERROR);
  }

  while(1) {
    if ( ++x > 20 ){
      mc_printf(".");
      fflush(stdout);
      x = 0;
#ifdef	QUICKWIN
      _wyield();
#endif
    }
    if ( get_server(resp,NNTP_BUFSIZE)) {
#ifdef	WINDOWS
      reset_cursor();
#endif /* WINDOWS */
      fclose(fp);
      fprintf(stderr,"\nARTICLE: get_server fail\n");
      unlink(article_filename(st));
      return(ERROR);
    }
    if ( resp[0] == '.' && resp[1] == 0)
      break;

    pt = resp;
    if ( resp[0] == '.' && resp[1] == '.')
      pt++;

    kanji_convert(gn.spool_kanji_code,pt,gn.gnspool_kanji_code,resp2);
    pt = resp2;

    if ( fprintf(fp,"%s\n",pt) == -1 ) {
      fclose(fp);
#ifdef	WINDOWS
      reset_cursor();
      mc_MessageBox(hWnd,"ファイルの作成に失敗しました",
		    article_filename(st),MB_OK);
#else  /* WINDOWS */
      perror("\nwrite article");
#endif /* WINDOWS */
      unlink(article_filename(st));
      return(ERROR);
    }
  }

  mc_printf("\n");
  if ( fclose(fp) == EOF ) {
#ifdef	WINDOWS
    reset_cursor();
    mc_MessageBox(hWnd,"ファイルの作成に失敗しました",
		  article_filename(st),MB_OK);
#else  /* WINDOWS */
    perror(article_filename(st));
#endif /* WINDOWS */
    unlink(article_filename(st));
    return(ERROR);
  }
#ifdef OS2_32BIT
  setdirtime();
#endif /* OS2_32BIT */
  return(CONT);
}
#else	/* HEADANDBODY */
static int
get_article_file(ng,readed,base,st)
register newsgroup_t *ng;
register char *readed;
long base,st;
{
  FILE *fp;
  char resp[NNTP_BUFSIZE+1];
  char resp2[NNTP_BUFSIZE+1];
  char *pt;
  register int x = 0;
  
  mc_printf("%s: getting article #%ld ",ng->name,st);
  
  switch (article_command(st)) {
  case OK_ARTICLE:
    break;
  case ERR_NOARTIG:
    mc_printf("canceled ???\n");

    if ( update_newsrc == ON ) {
      readed[st-base] |= READED; /* キャンセルされた記事を既読に */
      change = 1;
    }
    return(CONT);
  case ERROR:			/* other */
    return(ERROR);
  }
  
  if ( (fp = fopen(article_filename(st),"w")) == NULL ) {
#ifdef	WINDOWS
    reset_cursor();
    mc_MessageBox(hWnd,"ファイルのオープンに失敗しました",
		  article_filename(st),MB_OK);
#else  /* WINDOWS */
    perror(article_filename(st));
#endif /* WINDOWS */
    return(ERROR);
  }
  
  while(1) {
    if ( ++x > 20 ){
      mc_printf(".");
      fflush(stdout);
      x = 0;
#ifdef	QUICKWIN
      _wyield();
#endif
    }
    if ( get_server(resp,NNTP_BUFSIZE)) {
#ifdef	WINDOWS
      reset_cursor();
#endif /* WINDOWS */
      fclose(fp);
      fprintf(stderr,"\nARTICLE: get_server fail\n");
      unlink(article_filename(st));
      return(ERROR);
    }
    if ( resp[0] == '.' && resp[1] == 0)
      break;

    pt = resp;
    if ( resp[0] == '.' && resp[1] == '.')
      pt++;

    kanji_convert(gn.spool_kanji_code,pt,gn.gnspool_kanji_code,resp2);
    pt = resp2;

    if ( fprintf(fp,"%s\n",pt) == -1 ) {
      fclose(fp);
#ifdef	WINDOWS
      reset_cursor();
      mc_MessageBox(hWnd,"ファイルの作成に失敗しました",
		    article_filename(st),MB_OK);
#else  /* WINDOWS */
      perror("\nwrite article");
#endif /* WINDOWS */
      unlink(article_filename(st));
      return(ERROR);
    }
  }
  mc_printf("\n");
  if ( fclose(fp) == EOF ) {
#ifdef	WINDOWS
    reset_cursor();
    mc_MessageBox(hWnd,"ファイルの作成に失敗しました",
		  article_filename(st),MB_OK);
#else  /* WINDOWS */
    perror(article_filename(st));
#endif /* WINDOWS */
    return(ERROR);
  }
  return(CONT);
}
#endif	/* HEADANDBODY */

static int
update_readed(ng,readed)
register newsgroup_t *ng;
register char *readed;
{
  long l;
  long st,ed,base;
  char newsrc_line[NEWSRC_LEN];
  register char *pt;

  base = ng->read;

  /* 最終連続既読記事番号 */
  st = ng->read;
  for ( l = ng->read; l <= ng->last && (readed[l-base] & READED) == READED; l++)
    st = l;
  
  if ( ng->read != st )
    ng->read = st;
  
  /* GNUS format の newsrc 行の作成 */
  pt = newsrc_line;
  *pt = 0;
  ng->numb = 0;
  for ( ; l <= ng->last ; l++ ) {
    /* 読んでいない記事の連続 */
    if ( (readed[l-base] & READED) != READED ) {
      ng->numb++;
      continue;
    }
    /* 読んだ記事の連続 */
    st = ed = l;
    for ( ; l <= ng->last; l++ ) {
      if ( (readed[l-base] & READED) != READED ) {
	ng->numb++;
	break;
      }
      ed = l;
    }

    if ( st == ed ) {
      sprintf(pt,"%ld,",st);
    } else {
      sprintf(pt,"%ld-%ld,",st,ed);
    }
    pt = &newsrc_line[strlen(newsrc_line)];
  }

  if ( strlen(newsrc_line) != 0 ){
    pt--;
    *pt = 0;
    if ( (ng->newsrc = (char *)malloc(strlen(newsrc_line)+1)) == NULL ) {
      memory_error();
      return(ERROR);
    }
    strcpy(ng->newsrc,newsrc_line);
  }
  
  if ( ng->numb <= 0 )	/* 未読記事なし */
    ng->flag |= NOARTICLE;
  
  return(CONT);
}

#ifdef	XHDR_XREF
/* クロスポストがあれば、記事が存在するか調べる */
static int
check_xref(ng,st,ed,readed,base)
register newsgroup_t *ng;
register long st,ed,base;
register char *readed;
{
  char resp[NNTP_BUFSIZE+1];
  char file_name[NNTP_BUFSIZE+1];
  register long cur;
  register char *ng_name, *pt;
  long art_num;
  struct stat stat_buf;

  mc_printf("%s: checking cross post %ld - %ld ...\n",ng->name,st,ed);

  sprintf(resp,"XHDR Xref %ld-%ld",st,ed);
  if ( put_server(resp) ) {
#ifndef	WINDOWS
    fprintf(stderr,"XHDR: put_server fail\n");
#else  /* WINDOWS */
    mc_printf("XHDR: put_server fail\n");
#endif
    return(ERROR);
  }
  if ( get_server(resp,NNTP_BUFSIZE) ) {
#ifndef	WINDOWS
    fprintf(stderr,"XHDR: get_server fail\n");
#else  /* WINDOWS */
    mc_printf("XHDR: get_server fail\n");
#endif
    return(ERROR);
  }
  
  switch( atoi(resp) ) {
  case OK_HEAD:
    break;
  case ERR_COMMAND:		/* XHDR not support */
#ifndef	WINDOWS
    fprintf(stderr,"XHDR: %s\n",resp);
#else  /* WINDOWS */
    mc_printf("XHDR: %s\n",resp);
#endif
    return(CONT);
  default:
#ifndef	WINDOWS
    fprintf(stderr,"XHDR: %s\n",resp);
#else  /* WINDOWS */
    mc_printf("XHDR: %s\n",resp);
#endif
    return(ERROR);
  }


  while (1) {
    if ( get_server(resp,NNTP_BUFSIZE)) {
#ifndef	WINDOWS
      fprintf(stderr,"\nXHDR: get_server fail\n");
#else  /* WINDOWS */
      mc_printf("XHDR: get_server fail\n");
#endif
      return(ERROR);
    }
    if ( resp[0] == '.' && resp[1] == 0)
      break;

    cur = atol(resp);
    if ( cur < st || ed < cur )
      continue;

    if ( strchr(resp,':') == NULL ) /* skip (none) */
      continue;

    pt = resp;
    /* 記事番号をスキップ */
    while ( '0' <= *pt && *pt <= '9' )
      pt++;
    while ( *pt == ' ' || *pt == '\t' )
      pt++;
    /* ニュースサーバ名をスキップ */
    while ( *pt != ' ' && *pt != '\t' )
      pt++;
    while ( *pt == ' ' || *pt == '\t' )
      pt++;

    while (*pt != 0) {
      
      ng_name = pt;		/* ニュースグループ名 */

      if ( (pt = strchr(ng_name,':')) == NULL )
	break;
      *pt++ = 0;

      art_num=atol(pt);		/* 記事番号 */

      /* 記事番号をスキップ */
      while ( '0' <= *pt && *pt <= '9' )
	pt++;
      while ( *pt == ' ' ||  *pt == '\t' )
	pt++;

      if ( strcmp(ng->name, ng_name) == 0 )
	continue;

      if (search_group(ng_name) == 0 )
	continue;			/* グループが存在しない */

      ng_directory(file_name,ng_name);
      strcat(file_name,SLASH_STR);
      strcat(file_name,article_filename(art_num));

      if (stat(file_name,&stat_buf) != 0 )
	continue;		/* クロスポスト先が存在しない */

      pt = resp;
      /* 記事番号をスキップ */
      while ( '0' <= *pt && *pt <= '9' )
	pt++;
      *pt = 0;
#if defined(UNIX)
      if ( link(file_name,resp) == 0 ) {
	mc_printf("%s: link %s to %s\n",ng->name,file_name,resp);
      } else
#endif  /* UNIX */
	{
	  FILE *rp,*wp;
	  int x = 0;

	  if ( (rp = fopen(file_name,"r")) == NULL )
	    continue;
	  if ( (wp = fopen(resp,"w")) == NULL )
	    continue;
	  mc_printf("%s: copying %s to %s ",ng->name,file_name,resp);
	  while ( fgets(resp,NNTP_BUFSIZE,rp) != NULL ) {
	    if ( ++x > 20 ){
	      mc_printf(".");
	      fflush(stdout);
	      x = 0;
#ifdef	QUICKWIN
	      _wyield();
#endif
	    }
	    if ( fputs(resp,wp) == EOF ) {
	      fclose(rp);
	      fclose(wp);
	      return(ERROR);
	    }
	  }
	  fclose(rp);		/* error check */
	  fclose(wp);
	  mc_printf("\n");
	}
      readed[cur-base] |= EXISTS_LOCAL;
      break;
    }
  }

  return(CONT);
}
#endif	/* XHDR_XREF */

/* 記事が存在するか調べる */
static int
check_exist(st,ed,readed,base)
register long st,ed,base;
register char *readed;
{
  char resp[NNTP_BUFSIZE+1];
  register long cur;

#ifdef	WINDOWS
  set_hourglass();
#endif	/* WINDOWS */
  
  sprintf(resp,"XHDR From %ld-%ld",st,ed);
  if ( put_server(resp) ) {
#ifndef	WINDOWS
    fprintf(stderr,"XHDR: put_server fail\n");
#else  /* WINDOWS */
    reset_cursor();
    mc_printf("XHDR: put_server fail\n");
#endif
    return(ERROR);
  }
  if ( get_server(resp,NNTP_BUFSIZE) ) {
#ifndef	WINDOWS
    fprintf(stderr,"XHDR: get_server fail\n");
#else  /* WINDOWS */
    reset_cursor();
    mc_printf("XHDR: get_server fail\n");
#endif
    return(ERROR);
  }
  
  switch( atoi(resp) ) {
  case OK_HEAD:
    break;
  case ERR_COMMAND:		/* XHDR not support */
    gn.remove_canceled = OFF;
#ifndef	WINDOWS
    fprintf(stderr,"XHDR: %s\n",resp);
#else  /* WINDOWS */
    reset_cursor();
    mc_printf("XHDR: %s\n",resp);
#endif
    return(CONT);
  default:
#ifndef	WINDOWS
    fprintf(stderr,"XHDR: %s\n",resp);
#else  /* WINDOWS */
    reset_cursor();
    mc_printf("XHDR: %s\n",resp);
#endif
    return(ERROR);
  }


  while (1) {
    if ( get_server(resp,NNTP_BUFSIZE)) {
#ifdef	WINDOWS
      reset_cursor();
#endif /* WINDOWS */
      fprintf(stderr,"\nXHDR: get_server fail\n");
      return(ERROR);
    }
    if ( resp[0] == '.' && resp[1] == 0)
      break;

    cur = atol(resp);
    if ( cur < st || ed < cur )
      continue;

    readed[cur-base] |= EXISTS_SERVER;

  }

#ifdef	WINDOWS
  reset_cursor();
#endif /* WINDOWS */
  return(CONT);
}
#ifdef	HEADANDBODY
static int
head_command(numb)
long numb;
{
  char resp[NNTP_BUFSIZE+1];
  
#ifdef	WINDOWS
  set_hourglass();
#endif	/* WINDOWS */
  
  sprintf(resp,"HEAD %ld",numb);
  if ( put_server(resp) ) {
#ifndef	WINDOWS
    fprintf(stderr,"HEAD: put_server fail\n");
#else  /* WINDOWS */
    reset_cursor();
    mc_printf("HEAD: put_server fail\n");
#endif
    return(ERROR);
  }
  if ( get_server(resp,NNTP_BUFSIZE) ) {
#ifndef	WINDOWS
    fprintf(stderr,"HEAD: get_server fail\n");
#else  /* WINDOWS */
    reset_cursor();
    mc_printf("HEAD: get_server fail\n");
#endif
    return(ERROR);
  }
  
  switch( atoi(resp) ) {
  case OK_HEAD:
    return(OK_HEAD);
  case ERR_NOARTIG:
#ifdef	WINDOWS
    reset_cursor();
#endif /* WINDOWS */
    return(ERR_NOARTIG);
  default:
#ifndef	WINDOWS
    fprintf(stderr,"HEAD: %s\n",resp);
#else  /* WINDOWS */
    reset_cursor();
    mc_printf("HEAD: %s\n",resp);
#endif
    return(ERROR);
  }
}
static int
body_command(numb)
long numb;
{
  char resp[NNTP_BUFSIZE+1];
  
#ifdef	WINDOWS
  set_hourglass();
#endif	/* WINDOWS */
  
  sprintf(resp,"BODY %ld",numb);
  if ( put_server(resp) ) {
#ifndef	WINDOWS
    fprintf(stderr,"BODY: put_server fail\n");
#else  /* WINDOWS */
    reset_cursor();
    mc_printf("BODY: put_server fail\n");
#endif
    return(ERROR);
  }
  if ( get_server(resp,NNTP_BUFSIZE) ) {
#ifndef	WINDOWS
    fprintf(stderr,"BODY: get_server fail\n");
#else  /* WINDOWS */
    reset_cursor();
    mc_printf("BODY: get_server fail\n");
#endif
    return(ERROR);
  }
  
  switch( atoi(resp) ) {
  case OK_BODY:
    return(OK_BODY);
  case ERR_NOARTIG:
#ifdef	WINDOWS
    reset_cursor();
#endif /* WINDOWS */
    return(ERR_NOARTIG);
  default:
#ifndef	WINDOWS
    fprintf(stderr,"BODY: %s\n",resp);
#else  /* WINDOWS */
    reset_cursor();
    mc_printf("BODY: %s\n",resp);
#endif
    return(ERROR);
  }
}
#else /* HEADANDBODY */
static int
article_command(numb)
long numb;
{
  char resp[NNTP_BUFSIZE+1];
  
#ifdef	WINDOWS
  set_hourglass();
#endif	/* WINDOWS */
  
  sprintf(resp,"ARTICLE %ld",numb);
  if ( put_server(resp) ) {
#ifndef	WINDOWS
    fprintf(stderr,"ARTICLE: put_server fail\n");
#else  /* WINDOWS */
    reset_cursor();
    mc_printf("ARTICLE: put_server fail\n");
#endif
    return(ERROR);
  }
  if ( get_server(resp,NNTP_BUFSIZE) ) {
#ifndef	WINDOWS
    fprintf(stderr,"ARTICLE: get_server fail\n");
#else  /* WINDOWS */
    reset_cursor();
    mc_printf("ARTICLE: get_server fail\n");
#endif
    return(ERROR);
  }
  
  switch( atoi(resp) ) {
  case OK_ARTICLE:
    return(OK_ARTICLE);
  case ERR_NOARTIG:
#ifdef	WINDOWS
    reset_cursor();
#endif /* WINDOWS */
    return(ERR_NOARTIG);
  default:
#ifndef	WINDOWS
    fprintf(stderr,"ARTICLE: %s\n",resp);
#else  /* WINDOWS */
    reset_cursor();
    mc_printf("ARTICLE: %s\n",resp);
#endif
    return(ERROR);
  }
}
#endif /* HEADANDBODY */
#endif /* NO_NET */


/*
 * history の 作成
 */
int
make_history()
{
  extern make_history_1ng();
  
#ifndef	WINDOWS
  register newsgroup_t *ng;
#endif	/* WINDOWS */
  char history_file[PATH_LEN];
  
  strcpy(history_file,gn.newslib);
  if ( history_file[strlen(history_file)-1] != SLASH_CHAR)
    strcat(history_file,SLASH_STR);
  strcat(history_file,"history");
  
  mc_printf("\n%s を作成しています\n",history_file);
  if ( (history_fp = fopen(history_file,"w")) == NULL ) {
    mc_printf("\n%s の作成に失敗しました\n",history_file);
    return(ERROR);
  }
  
#ifndef	WINDOWS
  for ( ng = newsrc ; ng != 0 ; ng=ng->next) {
    if ( make_history_1ng(ng) != CONT )
      return(ERROR);
  }
#endif	/* WINDOWS */
  return(CONT);
}
int
make_history_1ng(ng)
register newsgroup_t *ng;
{
  char dir[PATH_LEN];
  struct stat stat_buf;
  char *name;

  ng_directory(dir,ng->name);

  if ( stat(dir,&stat_buf) != 0 )
    return(CONT);

  if ( chdir(dir) != 0 )
    return(CONT);

  if ( (name = find_first_file()) != (char *)0 ) {
    if ( add_to_history(ng,name) != CONT )
      return(ERROR);
    while ( (name = find_next_file()) != (char *)0 ) {
      if ( add_to_history(ng,name) != CONT )
	return(ERROR);
    }
  }

  if ( chdir(gn.newsspool) != 0 ) {
#ifdef	WINDOWS
    mc_MessageBox(hWnd,"cd できません ", gn.newsspool,MB_OK);
#else  /* WINDOWS */
    perror(gn.newsspool);
#endif /* WINDOWS */
    return(ERROR);
  }
  return(CONT);
}
static int
add_to_history(ng,name)
newsgroup_t *ng;
char *name;
{
  FILE *fp;
  char resp[NNTP_BUFSIZE+1];
  register char *pt;
  static int x;
  
  if ( name[0] < '0' || '9' < name[0] )
    return(CONT);
  
  if ( ++x >= 20 ){
    mc_printf(".");
    fflush(stdout);
    x = 0;
#ifdef	QUICKWIN
    _wyield();
#endif
  }
  
  if ( gn.use_history ) {
    if ( (fp = fopen(name,"r")) == NULL ) {
#ifdef	WINDOWS
      mc_MessageBox(hWnd,"ヒストリファイルのオープンに失敗しました",
		    "",MB_OK);
#else  /* WINDOWS */
      perror(name);
#endif /* WINDOWS */
      return(ERROR);
    }
    while ( fgets(resp,NNTP_BUFSIZE,fp) != NULL ) {
      if ( resp[0] == '\n' )
	break;
      if ( strncmp(resp,"Message-ID: ",12) == 0 ) {
	if ( (pt = strchr(resp,'\n')) != NULL )
	  *pt = 0;
	fprintf(history_fp,"%s	0	%s/%s\n",
		&resp[12],ng->name,name);
	break;
      }
    }
    if ( fclose(fp) == EOF ) {
#ifdef	WINDOWS
      mc_MessageBox(hWnd,"ヒストリファイルの作成に失敗しました",
		    "",MB_OK);
#else  /* WINDOWS */
      perror(name);
#endif /* WINDOWS */
      return(ERROR);
    }
  }
  return(CONT);
}
#if defined(OS2_32BIT)
static void
setdirtime()
{
  char *pwd;

  if ( (pwd = getcwd(NULL,PATH_LEN)) == NULL )
    perror("getcwd");
  else
    utime(pwd, NULL);
}
#endif  /* OS2_32BIT */

#if defined (OS2)
#ifdef OS2_16BIT
FILEFINDBUF dp;
#else
FILEFINDBUF3 dp;
#endif
HDIR DirHandle;

char *
find_first_file()
{
#ifdef OS2_16BIT
  USHORT SearchCount = 1;	/* Look for only one file entry */
  ULONG  FileInfoLevel = 0L;	/* reserved by 16bit API */
  USHORT ResultBufLen;
#else
  ULONG  SearchCount = 1;	/* Look for only one file entry */
  ULONG  FileInfoLevel = FIL_STANDARD;	/* Level 1 search */
  ULONG  ResultBufLen;
#endif
  DirHandle = HDIR_SYSTEM;	/* Use system handle */
  ResultBufLen = sizeof(dp);

  if ( DosFindFirst("*.*", &DirHandle, FILE_NORMAL, &dp,
		    ResultBufLen, &SearchCount, FileInfoLevel) != 0 ) {
    return(CONT);
  }
  return((char *)&dp.achName);
}

char *
find_next_file()
{
#ifdef OS2_16BIT
  USHORT SearchCount = 1;  /* Look for only one file entry */
  USHORT ResultBufLen;
#else
  ULONG  SearchCount = 1;  /* Look for only one file entry */
  ULONG  ResultBufLen;
#endif

  ResultBufLen = sizeof(dp);

  if ( DosFindNext(DirHandle, &dp, ResultBufLen, &SearchCount) != 0 )
    return(CONT);
  return((char *)&dp.achName);
}
#else
#if defined (MSC)
#if defined (MSC9)
static struct _finddata_t dp;
static long handle;
char *
find_first_file()
{
  if ( (handle = _findfirst("*.*",&dp)) == -1L  )
    return(CONT);			/* ファイルなし */
  return((char *)&dp.name);
}
char *
find_next_file()
{
  if ( _findnext(handle,&dp) != 0  )
    return(CONT);			/* ファイルなし */
  return((char *)&dp.name);
}
#else  /* MSC9 */
struct find_t dp;

char *
find_first_file()
{
  if ( _dos_findfirst("*.*",_A_NORMAL,&dp) != 0  )
    return(CONT);			/* ファイルなし */
  return((char *)&dp.name);
}
char *
find_next_file()
{
  if ( _dos_findnext(&dp) != 0  )
    return(CONT);			/* ファイルなし */
  return((char *)&dp.name);
}
#endif /* MSC9 */
#else
#if defined (GNUDOS) || defined (__TURBOC__)
struct ffblk dp;

char *
find_first_file()
{
  if ( findfirst("*.*",&dp,0) != 0  )
    return(CONT);		/* ファイルなし */
  return((char *)&dp.ff_name);
}
char *
find_next_file()
{
  if ( findnext(&dp) != 0  )
    return(CONT);		/* ファイルなし */
  return((char *)&dp.ff_name);
}
#else
#if defined (HUMAN68K)
#include <dirent.h>
DIR	*dirp;
struct dirent *dp;

char *
find_first_file()
{
  if ( (dirp = opendir(".")) == NULL )
    return(CONT);		/* ファイルなし */
  
  if ( (dp = readdir(dirp)) == NULL ){
    closedir(dirp);
    return(CONT);		/* ファイルなし */
  }
  return((char *)dp->d_name);
}

char *
find_next_file()
{
  if ( (dp = readdir(dirp)) == NULL ) {
    closedir(dirp);
    return(CONT);		/* ファイルなし */
  }
  return((char *)dp->d_name);
}
#else
DIR	*dirp;
#if defined(__osf__) || defined(MEUX) || defined(SVR4) || defined(AIX)
struct dirent *dp;
#else
struct direct *dp;
#endif

char *
find_first_file()
{
  if ( (dirp = opendir(".")) == NULL )
    return(CONT);		/* ファイルなし */
  
  if ( (dp = readdir(dirp)) == NULL ){
    closedir(dirp);
    return(CONT);		/* ファイルなし */
  }
  return((char *)dp->d_name);
}

char *
find_next_file()
{
  if ( (dp = readdir(dirp)) == NULL ) {
    closedir(dirp);
    return(CONT);		/* ファイルなし */
  }
  return((char *)dp->d_name);
}
#endif	/* HUMAN68K */
#endif	/* GNUDOS */
#endif	/* MSC */
#endif	/* OS2 */

/*
 * 起動オプションの解析(gnspool)
 */
int
read_option(argc,argv)
int argc;
char *argv[];
{
  extern int getopt();
  
  extern char *optarg;
  
  int c;
  char *optopt;
#ifdef	WINDOWS
  char buf[128];
#endif	/* WINDOWS */
  
#ifdef	WINDOWS
  optopt = "h:d:nlepmgfyu";
#else	/* WINDOWS */
  optopt = "h:d:nlepmgfyuCV";
#endif	/* WINDOWS */

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

    case 'n':
      gnspooljobs |= BIT_POST | BIT_MAIL | BIT_EXPIRE;
      break;

    case 'l':
    case 'e':			/* expire */
      gnspooljobs |= BIT_EXPIRE;
      break;

    case 'p':			/* post */
      gnspooljobs |= BIT_POST;
      break;

    case 'm':			/* mail */
      gnspooljobs |= BIT_MAIL;
      break;

    case 'g':			/* get */
      gnspooljobs |= BIT_GET;
      break;

    case 'f':			/* fast connect mode */
      gn.fast_connect = ON;
      break;

    case 'y':			/* non interactive */
      interactive_send = OFF;
      break;

    case 'u':			/* update newsrc */
      update_newsrc = ON;
      break;

    case 'C':			/* catch up */
      cnews = ON;
      break;
#ifndef	WINDOWS
    case 'V':
      printf("%s Version %s %s.  Copyright (C) yamasita@omronsoft.co.jp\n",
	     argv[0],gn_version,gn_date);
      break;
#endif /* WINDOWS */

    default:
#ifndef	WINDOWS
      mc_printf("%s [-h hostname] [-d domainname][-epmgyV]\n",argv[0]);

      mc_puts("   -h hostname   ニュースサーバを指定する \n");
      mc_puts("   -d domainname ドメイン名を指定する \n");
      mc_puts("   -e            既読記事の削除のみをおこなう \n");
      mc_puts("   -p            記事のポストだけをおこなう \n");
      mc_puts("   -m            メイルの送信だけをおこなう \n");
      mc_puts("   -g            未読記事の取得だけをおこなう \n");
      mc_puts("                 -e -p -m -g は組み合わせて使用可 \n");
      mc_puts("   -f            未読記事数のチェックを手抜きする（危険） \n");
      mc_puts("   -y            確認入力なしにポスト／メイルする \n");
      mc_puts("   -u            キャンセルされた記事を既読とする \n");
      mc_puts("   -C            取寄せた記事はすべて既読とする \n");
      mc_puts("   -V            バージョンの表示 \n");
      exit(5);
#else  /* WINDOWS */
      sprintf(buf,"%c: unknown option",c);
      mc_MessageBox(hWnd,buf,"",MB_OK);
      return(ERROR);
#endif /* WINDOWS */
    }
  }

  if ( gnspooljobs == BIT_EXPIRE )
    gn.local_spool = ON;

  if ( gnspooljobs == 0 )
    gnspooljobs = BIT_EXPIRE | BIT_POST | BIT_MAIL | BIT_GET;
  return(CONT);
}
