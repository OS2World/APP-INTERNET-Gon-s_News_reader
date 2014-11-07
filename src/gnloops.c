/*
 * @(#)gnloops.c	1.40 Oct.14,1997
 *
 * 著作権は放棄しません。ただし、営利目的以外の使用／配布に制限は設けません。
 * Copyright (C) yamasita@omronsoft.co.jp
 */

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

extern int get_active();
extern void mc_puts(),mc_printf();
extern void hit_return();
extern int put_server();
extern int get_server();
extern void ng_directory();
extern void memory_error();

extern int gn_init();
extern int open_server();
extern void close_server();
extern void kanji_convert();
#ifdef	DOSFS
extern int SetDisk();
#endif

static long is_articlefile();
static int transfer();
static int transfer_1ng();
static int send_1art();
static int send_ihave();
static int send_article();
static char *find_first_file(),*find_next_file();

#if defined(WINDOWS)
extern int mc_MessageBox();
extern void set_hourglass(),reset_cursor();
#endif	/* WINDOWS */

#if !defined(USE_PUT_SERVER)
  extern FILE *ser_wr_fp;
#endif

int prog = GNLOOPS;
char destserver[MAXHOSTNAME+1];


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

  destserver[0] = 0;
  /* 起動オプションの解析 */

  gn_init(argc,argv);

  if ( destserver[0] == 0 ) {
    mc_puts("転送先のホスト名が指定されていません \n");
    exit(1);
  }

  /* アクティブファイルを取得する */
  gn.local_spool = ON;
  switch ( get_active() ) {
  case CONNECTION_CLOSED:
    mc_puts("\nニュースサーバとの接続が切れています\n");
    exit(1);
  case ERROR:
    exit(1);
  }
  gn.local_spool = OFF;


#if defined (DOSFS)
  if ( gn.newsspool[1] == ':' ) {
    if (SetDisk(toupper(gn.newsspool[0])-'A')){
      mc_printf("%s は使えません \n",gn.newsspool);
      exit(1);
    }
  }
#endif

  free(gn.nntpserver);
  if ( (gn.nntpserver = malloc(strlen(destserver)+1)) == NULL ) {
    memory_error();
    return(ERROR);
  }
  strcpy(gn.nntpserver,destserver);

  /* 転送先のニュースサーバとの接続  */
  mc_printf("%s と接続しています\n",gn.nntpserver);
  open_server();

  
  /* 送信 */
  if ( transfer() != CONT ) {
    mc_puts("\n転送に失敗しました\n");
    exit(1);
  }
  
  /* サーバとの接続を切る */
  close_server();

  
#if defined(DOSFS)
  SetDisk(pwd[0]-'A');
  chdir(pwd);
#endif /* DOSFS */

#if defined(WINSOCK) && !defined(NO_NET)
  WSACleanup();
#endif
  
  mc_printf("\n\nDone!\n");
#ifdef	QUICKWIN
  if ( interactive_send != OFF )
    hit_return();
  _wsetexit(_WINEXITNOPERSIST);
#endif
  return(0);
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
 * 記事の転送
 */
static int
transfer()
{
  extern transfer_1ng();
  
#ifndef	WINDOWS
  register newsgroup_t *ng;
#endif	/* WINDOWS */
  
  mc_printf("\n記事を転送しています\n");

#ifndef	WINDOWS
  for ( ng = newsrc ; ng != 0 ; ng=ng->next) {
    if ( transfer_1ng(ng) != CONT )
      return(ERROR);
  }
#endif	/* WINDOWS */
  return(CONT);
}
static int
transfer_1ng(ng)
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
    if ( send_1art(ng,name) != CONT )
      return(ERROR);
    while ( (name = find_next_file()) != (char *)0 ) {
      if ( send_1art(ng,name) != CONT )
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
send_1art(ng,name)
register newsgroup_t *ng;
char *name;
{
  FILE *fp;
  char resp[NNTP_BUFSIZE+1];
  register char *pt;
  
  if ( name[0] < '0' || '9' < name[0] )
    return(CONT);

  if ( (fp = fopen(name,"r")) == NULL ) {
#ifdef	WINDOWS
    mc_MessageBox(hWnd,"記事ファイルのオープンに失敗しました","",MB_OK);
#else  /* WINDOWS */
    perror(name);
#endif /* WINDOWS */
    return(ERROR);
  }

  /* Message-ID: を探す */
  while ( fgets(resp,NNTP_BUFSIZE,fp) != NULL ) {
    if ( resp[0] == '\n' )
      break;
    if ( strncmp(resp,"Message-ID: ",12) == 0 ) {
      break;
    }
  }

  if ( fclose(fp) == EOF ) {
#ifdef	WINDOWS
    mc_MessageBox(hWnd,"記事ファイルのクローズに失敗しました","",MB_OK);
#else  /* WINDOWS */
    perror(name);
#endif /* WINDOWS */
    return(ERROR);
  }

  if ( resp[0] == '\n' )
    return(CONT);		/* Message-ID: is not found */

  if ( (pt = strchr(resp,'\n')) != NULL )
    *pt = 0;

  mc_printf("%s: sending article #%s ",ng->name,name);
  
  switch ( send_ihave(&resp[12]) ) {
  case CONT_XFER:
    break;
  case ERR_GOTIT:
    return(CONT);
  case ERROR:
    return(ERROR);
  }

  return(send_article(name));
}
static int
send_ihave(id)
char *id;
{
  char resp[NNTP_BUFSIZE+1];

  sprintf(resp,"ihave %s",id);

  if ( put_server(resp) ) {
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
  case CONT_XFER:
    return(CONT_XFER);
  case ERR_GOTIT:
    mc_puts("Already got that article, don't send\n");
    return(ERR_GOTIT);

  default:
#if ! defined(WINDOWS)
    mc_printf("%s\n",resp);
    hit_return();
#else  /* WINDOWS */
    reset_cursor();
    mc_MessageBox(hWnd,resp,"転送できません ",MB_OK);
#endif /* WINDOWS */
    return(ERROR);
  }
}
static int
send_article(name)
char *name;
{
  FILE *fp;
  char resp[NNTP_BUFSIZE+1];
  char resp2[NNTP_BUFSIZE+1];
  register char *pt;
  int x;

  if ( (fp = fopen(name,"r")) == NULL ) {
#ifdef	WINDOWS
    mc_MessageBox(hWnd,"記事ファイルのオープンに失敗しました","",MB_OK);
#else  /* WINDOWS */
    perror(name);
#endif /* WINDOWS */
    return(ERROR);
  }

  /* ヘッダ部 */
  while (fgets(resp,NNTP_BUFSIZE,fp) != NULL ) {
    if ( (pt = strchr(resp,'\n')) != NULL )
      *pt = 0;
    if ( (pt = strchr(resp,'\r')) != NULL )
      *pt = 0;
    if ( resp[0] == 0 )		/* ヘッダの終り */
      break;
      
    if ( strncmp(resp,"Path: ",6) == 0 ) {
      sprintf(resp2,"Path: gnloops.%s!%s",gn.hostname,&resp[6]);
      PUT_SERVER( resp2 );
      continue;
    }
    kanji_convert(gn.gnspool_kanji_code,resp,gn.spool_kanji_code,resp2);
    if ( resp2[0] == '.' ) {
      strcpy(resp,".");
      strcat(resp,resp2);
      PUT_SERVER(resp);
    } else {
      PUT_SERVER( resp2 );
    }
  }

  PUT_SERVER(resp);		/* ヘッダと本文のあいだの空行 */

  /* 本文 */
  while (fgets(resp,NNTP_BUFSIZE,fp) != NULL ) {
    if ( ++x > 20 ){
      mc_printf(".");
      fflush(stdout);
      x = 0;
#ifdef	QUICKWIN
      _wyield();
#endif
    }
    if ( (pt = strchr(resp,'\n')) != NULL )
      *pt = 0;
    if ( (pt = strchr(resp,'\r')) != NULL )
      *pt = 0;

    kanji_convert(gn.gnspool_kanji_code,resp,gn.spool_kanji_code,resp2);
    if ( resp2[0] == '.' ) {
      strcpy(resp,".");
      strcat(resp,resp2);
      PUT_SERVER(resp);
    } else {
      PUT_SERVER( resp2 );
    }
  }

  put_server( "." );

  if ( fclose(fp) == EOF ) {
#ifdef	WINDOWS
    mc_MessageBox(hWnd,"記事ファイルのクローズに失敗しました","",MB_OK);
#else  /* WINDOWS */
    perror(name);
#endif /* WINDOWS */
    return(ERROR);
  }


  if ( get_server(resp,NNTP_BUFSIZE)) {
#ifdef	WINDOWS
    reset_cursor();
#endif /* WINDOWS */
    return(CONNECTION_CLOSED);
  }

  switch( atoi(resp) ) {
  case OK_XFERED:
    mc_printf("\n");
    break;

  case ERR_XFERFAIL:	/* Transfer failed */
  case ERR_XFERRJCT:	/* Article rejected, don't resend */
    mc_printf("%s\n",resp);
    break;

  default:
#if ! defined(WINDOWS)
    mc_printf("%s\n",resp);
    hit_return();
#else  /* WINDOWS */
    reset_cursor();
    mc_MessageBox(hWnd,resp,"転送できません ",MB_OK);
#endif /* WINDOWS */
    return(ERROR);
  }
  
#ifdef	WINDOWS
  reset_cursor();
#endif	/* WINDOWS */
  return(CONT);
}



#if defined (OS2)
#ifdef OS2_16BIT
FILEFINDBUF dp;
#else
FILEFINDBUF3 dp;
#endif
HDIR DirHandle;

static char *
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

static char *
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
static char *
find_first_file()
{
  if ( (handle = _findfirst("*.*",&dp)) == -1L  )
    return(CONT);			/* ファイルなし */
  return((char *)&dp.name);
}
static char *
find_next_file()
{
  if ( _findnext(handle,&dp) != 0  )
    return(CONT);			/* ファイルなし */
  return((char *)&dp.name);
}
#else  /* MSC9 */
struct find_t dp;

static char *
find_first_file()
{
  if ( _dos_findfirst("*.*",_A_NORMAL,&dp) != 0  )
    return(CONT);			/* ファイルなし */
  return((char *)&dp.name);
}
static char *
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

static char *
find_first_file()
{
  if ( findfirst("*.*",&dp,0) != 0  )
    return(CONT);		/* ファイルなし */
  return((char *)&dp.ff_name);
}
static char *
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

static char *
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

static char *
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

static char *
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

static char *
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
 * 起動オプションの解析(gnloops)
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
  optopt = "h:d:";
#else	/* WINDOWS */
  optopt = "h:d:V";
#endif	/* WINDOWS */

  /* 起動オプションの解析 */

  while ((c = getopt(argc, argv, optopt)) != EOF) {
    switch (c) {
    case 'h':			/* 記事を取得したニュースサーバを指定する */
      if ( gn.nntpserver != 0 )
	free(gn.nntpserver);
      if ( (gn.nntpserver = malloc(strlen(optarg)+1)) == NULL ) {
	memory_error();
	return(ERROR);
      }
      strcpy(gn.nntpserver,optarg);
      break;

    case 'd':			/* 記事を転送するニュースサーバを指定する */
      strcpy(destserver,optarg);
      break;

#ifndef	WINDOWS
    case 'V':
      printf("%s Version %s %s.  Copyright (C) yamasita@omronsoft.co.jp\n",
	     argv[0],gn_version,gn_date);
      break;
#endif /* WINDOWS */

    default:
#ifndef	WINDOWS
      mc_printf("%s [-h hostname] [-d hostname][-V]\n",argv[0]);

      mc_puts("   -h hostname   記事を取得したニュースサーバを指定する \n");
      mc_puts("   -d hostname   記事を転送するニュースサーバを指定する \n");
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
