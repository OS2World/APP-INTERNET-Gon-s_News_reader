/*
 * @(#)init.c	1.40 May.16,1998
 *
 * 著作権は放棄しません。ただし、営利目的以外の使用／配布に制限は設けません。
 * Copyright (C) yamasita@omronsoft.co.jp
 */

#ifdef WINDOWS
#	include <windows.h>
#endif

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <signal.h>
#ifdef	SIGWINCH
#	include	<sys/ioctl.h>
#endif
#ifdef	SVR4
#	include	<sys/termios.h>
#endif
#ifdef	HAS_STRING_H
#	include	<string.h>
#endif
#ifdef	HAS_STDLIB_H
#	include	<stdlib.h>
#endif
#ifdef	HAS_MALLOC_H
#	include	<malloc.h>
#endif
#ifdef	HAS_SYS_IOCTL_H
#	include	<sys/ioctl.h>
#endif
#ifdef	HAS_UNISTD_H
#	include	<unistd.h>
#endif
#ifdef	HAS_SYS_TERMIO_H
#	include	<sys/termio.h>
#endif
#ifdef	HAS_DIRECT_H
#	include	<direct.h>
#endif
#ifdef	HAS_DIR_H
#	include <dir.h>
#endif

#ifdef OS2
#	define INCL_DOSFILEMGR     /* for DosQCurDisk and DosSelectDisk */
#	define INCL_VIO            /* for VioGetMode */
#	include <os2.h>
#endif

#if defined (OS2) || ( defined (DOSV) && !defined(GNUDOS) )
static int GetLines(),GetColumns();
#endif	/* OS2 || DOSV */

#ifdef NECODT
#	include	<sys/stream.h>
#	include	<sys/ptem.h>
#endif	/* NECODT */

#if defined(sony_news) || defined(__sony_news)
#	if defined(USG)
#		include	<sys/termios.h>
#		include	<sys/utsname.h>
#	endif
#	include	<locale.h>
#endif

#ifdef nec_ews
#	include <sys/termios.h>
#endif

#ifdef	uniosu
typedef unsigned short uid_t;
struct passwd *getpwuid();
char *getenv();
#endif

#ifdef	DGUX
extern int gethostname();
#endif

#ifdef	UNIX
#	include	<pwd.h>
#endif /* UNIX */

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
#include	"version.h"
#include	"gn.h"
#include	"config.h"

static void init_gn_1();
static int init_gn_2();
extern int read_option();
extern char *kb_input();
extern char *expand_filename();
extern void mc_puts(),mc_printf();
extern void hit_return();
extern void kanji_convert();
extern void memory_error();
extern int lock_gn();
extern int read_gnrc(),read_env();
extern int check_regexp_group();

#if defined(WINDOWS)
extern int mc_MessageBox();
#endif	/* WINDOWS */

char *gn_version = VERSION;
char *gn_date = DATE;


newsgroup_t *newsrc;		/* ニュースグループリスト */
last_art_t last_art;		/* 最後に読んだ記事 */
void (*redraw_func)();

gn_t gn;			/* カスタマイズ */
int internal_kanji_code;	/* gn 内部の漢字コード */
int change;			/* newsrc 更新フラグ */
int make_article_tmpfile = ON;	/* ページャにテンポラリファイルを渡す */
int disp_mode;			/* スレッド表示 */
char *ngdir;

/* for gnspool */
int gnspooljobs = 0;		/* BIT_EXPIRE,BIT_POST,BIT_MAIL,BIT_GET */
int init_smtp_server = 0;

#if defined(WINDOWS)
char *help_file;	/* temp file for help */

int	xsiz, ysiz, wwidth, wheight, xpos, ypos;
char *display_buf = 0;

HANDLE hInst;			/* 現在のインスタンス */
HWND hWnd;			/* メインウィンドウのハンドル */
HCURSOR hHourGlass;		/* 砂時計カーソル */
#endif	/* WINDOWS */

#ifdef	SIGWINCH
static void resize_window();
#endif	/* SIGWINCH */

#ifdef	SIGCONT
static void foreground();
#endif	/* SIGCONT */


static char *mail_lang = 0;
static char *process_lang = 0;
static char *file_lang = 0;
static char *display_lang = 0;
static char *gnspool_lang = 0;
static char *unsubscribe_string = 0;
static char *subscribe_string = 0;
static int dos_newfs = -1;

/* パラメータが文字列のカスタマイズ項目 */
struct env_t {
  char *str;
  char **value;
};
/* パラメータが数字のカスタマイズ項目 */
struct gnrc_t {
  char *str;
  int *value;
};

#ifdef	WINDOWS
BOOL
InitApplication(hInstance)
HANDLE hInstance;
{
#ifdef WIN32
  WNDPROC CALLBACK MainWndProc(HWND, UINT, WPARAM, LPARAM);
#else
  long CALLBACK __export MainWndProc(HWND, UINT, WPARAM, LPARAM);
#endif
  
  WNDCLASS  wc;
  
  /* wc.style = NULL; */
  wc.style = CS_DBLCLKS;
  wc.lpfnWndProc = (WNDPROC)MainWndProc;
  wc.cbClsExtra = 0;
  wc.cbWndExtra = 0;
  wc.hInstance = hInstance;
  wc.hCursor = LoadCursor(NULL, IDC_ARROW);
  wc.hbrBackground = GetStockObject(WHITE_BRUSH);
  
  if ( prog == GN ) {
    wc.hIcon = LoadIcon(hInstance, "IDI_GN");
    wc.lpszClassName = "GnWClass";
    wc.lpszMenuName =  "IDR_GNMENU";
  } else {
    wc.hIcon = LoadIcon(hInstance, "IDI_GNSPOOL");
    wc.lpszClassName = "GnspoolWClass";
    wc.lpszMenuName =  "IDR_GNSPOOLMENU";
  }
  return (RegisterClass(&wc));
}

BOOL
InitInstance(hInstance, nCmdShow)
HANDLE hInstance;
int nCmdShow;
{
  HDC             hdc;
  TEXTMETRIC      textmetric;
  
  hHourGlass = LoadCursor(NULL, IDC_WAIT);  	/* 砂時計カーソル */
  
  hInst = hInstance;
  
  /* メインウィンドウ作成 */
  
  hWnd = CreateWindow(( prog == GN ) ? "GnWClass" : "GnspoolWClass",
		      ( prog == GN ) ? "gn" : "gnspool", /* title bar */
		      WS_OVERLAPPEDWINDOW, /* window style */
		      CW_USEDEFAULT, /* x */
		      CW_USEDEFAULT, /* y */
		      CW_USEDEFAULT, /* w */
		      CW_USEDEFAULT, /* h */
		      NULL,
		      NULL,
		      hInstance,
		      NULL);
  
  if (!hWnd)	/* ウィンドウを作成できなかった */
    return(FALSE);
  
  hdc = GetDC(hWnd);
  if (!hdc)
    return(FALSE);
  GetTextMetrics(hdc, &textmetric);
  xsiz = textmetric.tmAveCharWidth;
  ysiz = textmetric.tmHeight;

  ReleaseDC(hWnd, hdc);
  
  return(TRUE);
}
void
create_arg(arg)
char *arg;
{
  int	argc;
  char **argv,**argvp;
  register char *pt;
  
  /* argv の数を数える */
  pt = arg;
  argc = 1;
  while (1) {
    while( *pt == ' ' || *pt == TAB )
      pt++;
    if ( *pt == 0 )
      break;
    argc++;
    while( *pt != ' ' && *pt != TAB && *pt != 0 )
      pt++;
    if ( *pt == 0 )
      break;
  }
  
  if ( (argv = malloc(sizeof(char *) * (argc+1))) == NULL ){
    memory_error();
    return;
  }
  argv[0] = ( prog == GN ) ? "gn" : "gnspool";
  argvp = &argv[1];
  
  pt = arg;
  while (1) {
    while( *pt == ' ' || *pt == TAB )
      pt++;
    if ( *pt == 0 )
      break;
    *argvp++ = pt;
    while( *pt != ' ' && *pt != TAB && *pt != 0 )
      pt++;
    if ( *pt == 0 )
      break;
    *pt++ = 0;
  }
  *argvp = 0;
  
  PostMessage(hWnd, WM_START,(WPARAM)argc, (LPARAM)argv);
}

#endif	/* WINDOWS */
int
gn_init(argc,argv)
int argc;
char *argv[];
{
  static struct env_t env_table[] = {
    /* ドメイン */
    { GNRC_DOMAINNAME,	&gn.domainname },	/* ドメイン名 */
    { GNRC_ORGANIZATION,&gn.organization },	/* 組織名 */

    /* ニュースシステム */
    { GNRC_NNTPSERVER,	&gn.nntpserver },	/* ニュースサーバ */
    { GNRC_POSTPROC,	&gn.postproc },	/* -l 時、ポストする時のプログラム */
    { GNRC_NEWSLIB,	&gn.newslib },		/* /usr/lib/news */
    { GNRC_NEWSSPOOL,	&gn.newsspool },	/* /usr/spool/news */

    /* メイルシステム */
    { GNRC_SMTPSERVER,	&gn.smtpserver },	/* smtp,pop3|ftp サーバ */
    { GNRC_AUTHSERVER,	&gn.authserver },	/* pop サーバ */
    { GNRC_MAIL_LANG,	&mail_lang },		/* メイルの漢字コード */
    { GNRC_MAILER,	&gn.mailer },		/* メイラ */

    /* 自ホスト */
    { GNRC_HOST,	&gn.hostname },	/* ホスト名 */
    { GNRC_PROCESS_LANG,&process_lang },/* pager,editorの漢字コード */
    { GNRC_FILE_LANG,	&file_lang },	/* ファイルの漢字コード */
    { GNRC_DISPLAY_LANG,&display_lang },/* ディスプレイの漢字コード */
    { GNRC_PAGER,	&gn.pager },	/* ページャ */
    { GNRC_EDITOR,	&gn.editor },	/* エディタ */
    { GNRC_WIN_EDITOR,	&gn.win_editor }, /* Windows の時のエディタ */

    /* ユーザ */
    { GNRC_NAME,	&gn.fullname },	/* Full Name */
#ifdef	USG
    { GNRC_USER,	&gn.usrname },
    { GNRC_LOGNAME,	&gn.usrname },
#else  /* USG */
    { GNRC_LOGNAME,	&gn.usrname },
    { GNRC_USER,	&gn.usrname },
#endif /* USG */
    { GNRC_PASSWD,	&gn.passwd },

    /* directory / file */
    { GNRC_HOME,	&gn.home },	/* $HOME */
    { GNRC_NEWSRC,	&gn.rcfile },	/* .newsrc */
    { GNRC_SAVEDIR,	&gn.savedir },	/* 記事を保存するディレクトリ */
    { GNRC_SIGNATURE,	&gn.signature },/* .signature */
    { GNRC_AUTHOR_COPY,	&gn.author_copy },	/* author_copy */
    { GNRC_TMPDIR,	&gn.tmpdir },	/* テンポラリディレクトリ */
    { GNRC_TMP,		&gn.tmpdir },	/* テンポラリディレクトリ */
    { GNRC_TEMP,	&gn.tmpdir },	/* テンポラリディレクトリ */
    { GNRC_JNAMESFILE,	&gn.jnames_file },

    /* read */
#if 0
    { GNRC_IGNORE,	&unsubscribe_string },	/* 無視する newsgroup 群 */
#endif
    { GNRC_UNSUBSCRIBE,	&unsubscribe_string },	/* 無視する newsgroup 群 */
    { GNRC_SUBSCRIBE,	&subscribe_string },	/* 購読する newsgroup 群 */

    /* other */
    { GNRC_GNSPOOL_LANG,&gnspool_lang }, /* gnspool が作成するスプールの漢字コード */


    { 0,0 }
  };
  static struct gnrc_t gnrc_table[] = {
    /* ドメイン */
    { GNRC_GENERICFROM,	&gn.genericfrom },

    /* ニュースシステム */
    { GNRC_NNTPPORT,	&gn.nntpport },		/* NNTP に使用するポート */
    { GNRC_MODE_READER,	&gn.mode_reader },
    { GNRC_NNTP_AUTH,	&gn.nntp_auth },
    { GNRC_LOCAL_SPOOL,	&gn.local_spool },
    { GNRC_USE_HISTORY,	&gn.use_history }, /* history を使用する */
#if 0
    { GNRC_DOS_NEWFS,	&dos_newfs },	/* HPFS,NTFS などに spool を作成する */
#endif
    { GNRC_DOSFS,	&gn.dosfs },	/* FAT に spool を作成する */

    /* メイルシステム */
    { GNRC_SMTPPORT,	&gn.smtpport },		/* SMTP に使用するポート */
    { GNRC_POP3PORT,	&gn.pop3port },		/* POP3 に使用するポート */

    /* 自ホスト */
    { GNRC_LINES,	&gn.lines },
    { GNRC_WIN_LINES,	&gn.win_lines },
    { GNRC_COLUMNS,	&gn.columns },
    { GNRC_WIN_COLUMNS,	&gn.win_columns },
    { GNRC_RETURN_AFTER_PAGER,	&gn.return_after_pager },
    { GNRC_WIN_PAGER_FLAG,&gn.win_pager_flag },	/* 最終ページで止まる */
    { GNRC_USE_SPACE,	&gn.use_space }, /* スペースキーで読み進める */
    { GNRC_POWER_SAVE,	&gn.power_save }, /* ディスクへのアクセスを一時期に */

    /* newsgroup */
    { GNRC_GNUS_FORMAT,	&gn.gnus_format },
    { GNRC_NG_SORT,	&gn.ng_sort }, /* sort new newsgroup */
    { GNRC_ARTICLE_LIMIT,	&gn.article_limit },
    { GNRC_ARTICLE_LEAVE,	&gn.article_leave },
    { GNRC_SELECT_LIMIT,	&gn.select_limit },
    { GNRC_SELECT_NUMBER,	&gn.select_number },

    /* read */
    { GNRC_DISP_RE,	&gn.disp_re }, /* 全部フォロー記事の時は Re: を表示 */
    { GNRC_CLS,		&gn.cls }, /* 画面クリア */
    { GNRC_DESCRIPTION,	&gn.description },
    { GNRC_KILL_CANCEL,	&gn.kill_control }, /* upper compatible */
    { GNRC_KILL_CONTROL,&gn.kill_control },
    { GNRC_AUTOLIST,	&gn.autolist },
    { GNRC_SHOW_INDENT,	&gn.show_indent }, /* モード毎に字下げして表示 */
    { GNRC_SHOW_TRUNCATE,	&gn.show_truncate }, /* 折り曲げて全部表示する */
    { GNRC_MIME_HEAD,	&gn.mime_head }, /* ヘッダの MIME 対応 */
    { GNRC_GMT2JST,	&gn.gmt2jst }, /* Date: ヘッダの JST 表示 */

    /* post */
    { GNRC_NSUBJECT,	&gn.nsubject },	/* 日本語サブジェクト使用 */
    { GNRC_RN_LIKE,	&gn.rn_like }, /* rn ライクな引用タグ生成 */
    { GNRC_CC2ME,	&gn.cc2me }, /* リプライメールを自分にも Cc/Bcc */
    { GNRC_SUBSTITUTE_HEADER,	&gn.substitute_header }, /* ヘッダを付け変える */
    /* other */
    { GNRC_WITH_GNSPOOL,&gn.with_gnspool }, /* gnspool と組合せて使用する */

    /* for gnspool only */
    { GNRC_FAST_CONNECT,	&gn.fast_connect }, /* active を取得しない */
    { GNRC_REMOVE_CANCELED,	&gn.remove_canceled }, /* キャンセルされた記事を消す */
    { GNRC_GNSPOOL_MIME_HEAD,	&gn.gnspool_mime_head }, /* ヘッダの MIME 対応 */
    { GNRC_GNSPOOL_LINE_LIMIT,	&gn.gnspool_line_limit }, /* 取寄せる記事の最大行数 */
    { 0,0 }
  };

  struct lang_t {
    char *str;
    int value;
  };
  static struct lang_t lang_table[] = {
    {"JIS",		JIS},
    {"C",		JIS},
    {"ja_JP.jis7",	JIS},
    {"ja_JP.jis8",	JIS},
    {"ja_JP.pjis",	JIS},
    {"ja_JP.jis",	JIS},
    {"wr_WR.ct",	JIS},
    {"wr_WR.junet",	JIS},
  
    {"EUC",		EUC},
    {"UJIS",		EUC},
    {"ja_JP.ujis",	EUC},
    {"ja_JP.euc",	EUC},
    {"ja_JP.EUC",	EUC},
    {"ja_JP.eucJP",	EUC},
    {"ja",		EUC},
    {"japanese",	EUC},
  
    {"SJIS",		SJIS},
    {"ja_JP.sjis",	SJIS},
    {"ja_JP.SJIS",	SJIS},
    {"ja_JP.mscode",	SJIS},
    {0,0 }
  };

#ifdef	DOSFS
  extern int GetDisk(),SetDisk();
#endif
#if defined(MSDOS) || defined(OS2)
#	ifdef AUTHENTICATION
  extern int auth();
#	endif /* AUTHENTICATION */
#	if defined(INETBIOS) || defined(PATHWAY) || defined(LWP)
#		if ! defined(NO_NET)
  extern int find_inet();
#		endif	/* ! NO_NET */
#	endif
#endif /* MSDOS || OS2 */

#ifdef	WINDOWS
  RECT	lprct,lprcw;
#endif
  register char *env,*gnrc;
  char resp[NNTP_BUFSIZE+1];
  struct stat stat_buf;
  int check_only = CONT;
  register struct lang_t *lang_tp;
  char *pt;
  int disk = -1;
  
#if defined(WINSOCK)
  WORD wVersionRequested;
  WSADATA wsaData;
#endif	/* WINSOCK */
  
#if !defined(MSDOS) && !defined(OS2)
  char *sp;
  char full_name[MAXNAME];
  char host_name[MAXHOSTNAME+1];
  struct passwd *pw,*getpwnam();
  uid_t uid;
#endif /* DOS */
#if defined(sony_news) || defined(__sony_news)
  char *lang = setlocale(LC_CTYPE, "");
  int ttype;
#endif
  
  /* 外部変数の初期化 */
  newsrc = 0;
  change = 0;
  redraw_func = (void (*)())NULL;
  internal_kanji_code = ((unsigned char *)"あ")[0] == 0x82 ? SJIS : EUC;
  disp_mode = THREADED;
  ngdir = 0;
  

  /* struct gn *******************************/
  init_gn_1();

#if defined(sony_news) || defined(__sony_news)
  gn.process_kanji_code = JIS;
  gn.file_kanji_code = JIS;
  gn.display_kanji_code = JIS;
  for ( lang_tp = &lang_table[0] ; lang_tp->str != 0 ; lang_tp++ ) {
    if (strcmp(lang_tp->str,lang) == 0 ) {
      gn.process_kanji_code = lang_tp->value;
      gn.file_kanji_code = lang_tp->value;
      gn.display_kanji_code = lang_tp->value;
      break;
    }
  }
  if (ioctl(0, TIOCKGET, &ttype) != 0 ||
      ((ttype & KM_TTYPE) == KM_ASCII)) {
    gn.display_kanji_code = JIS;
  }
#endif /* defined(sony_news) || defined(__sony_news) */

  last_art.tmp_file = 0;
  last_art.newsgroups = 0;
  last_art.from = 0;
  last_art.date = 0;
  last_art.references = 0;
  last_art.xref = 0;
  last_art.art_numb = 0;
  
#ifdef UNIX
  uid = getuid();
  pw = getpwuid(uid);
#endif /* UNIX */

  if ( (env = getenv("HOME")) != 0 ) {
    if ( (gn.home = malloc(strlen(env)+1)) == NULL ) {
      memory_error();
      return(QUIT);
    }
    strcpy(gn.home,env);
  } else {
#ifdef UNIX
    if ( pw != NULL ) {
      if ( (gn.home = malloc(strlen(pw->pw_dir)+1)) == NULL ) {
	memory_error();
	return(QUIT);
      }
      strcpy(gn.home,pw->pw_dir);
    } else
#endif  /* UNIX */
      {
	if ( (gn.home = malloc(strlen(SLASH_STR)+1)) == NULL ) {
	  memory_error();
	  return(QUIT);
	}
	strcpy(gn.home,SLASH_STR);
      }
  }
  
#ifdef	DOSFS
  while ( (pt = strchr(gn.home,'/')) != NULL )
    *pt = SLASH_CHAR;
#endif
  
  /* 画面行数／桁数の取得／設定 */
#ifdef	SIGWINCH
  signal(SIGWINCH, resize_window);
  resize_window();
#endif

#if defined (OS2)
  gn.lines = GetLines();
  gn.columns = GetColumns();
#endif

#if defined (DOSV) && !defined(GNUDOS)
  gn.lines = GetLines();
  gn.columns = GetColumns() - 1;		    /* for ansi.sys */
#endif	/* DOSV && !GNUDOS */

#ifdef	WINDOWS
  help_file = 0;

  GetClientRect(hWnd,&lprcw);
  gn.lines = lprcw.bottom / ysiz;
  gn.columns = lprcw.right / xsiz;
  if ( (display_buf = malloc(gn.lines*gn.columns)) == NULL ) {
    memory_error();
    return(QUIT);
  }
  memset(display_buf,' ',gn.lines*gn.columns);
#endif /* WINDOWS */
  /**************************************************************************/
  
  /* $HOME/.gnrc の解析 */
  if ( (gnrc = expand_filename(GNRC,gn.home)) == 0 ) {
#if ! defined(WINDOWS)
    mc_printf("ファイル名の展開に失敗しました(%s)\n",GNRC);
#else	/* WINDOWS */
    mc_MessageBox(hWnd,"ファイル名の展開に失敗しました",GNRC,MB_OK);
#endif	/* WINDOWS */
    return(QUIT);
  }
  
  if ( read_gnrc(gnrc,env_table,gnrc_table) == ERROR)
    return(QUIT);
  
  /* 環境変数の解析 */
  if ( read_env(env_table,gnrc_table) == ERROR )
    return(QUIT);
  
  /* 起動オプションの解析 */
  if ( read_option(argc,argv,&check_only) == ERROR )
    return(QUIT);

  /* ニュースサーバが指定されていなければ入力 */
  if ( gn.nntpserver == 0 ) {
    if ( (gn.nntpserver = malloc(strlen(DEFAULT_NNTPSERVER)+1)) == NULL ) {
      memory_error();
      return(QUIT);
    }
    strcpy(gn.nntpserver,DEFAULT_NNTPSERVER);
  }
  while (1) {
    if ( strlen(gn.nntpserver) != 0 && strcmp(gn.nntpserver,"your-nntpserver") != 0 )
      break;
    if ( (pt = kb_input("ニュースサーバのホスト名を入力して下さい")) == NULL)
      return(QUIT);
    if ( strlen(pt) == 0 )
      continue;
    free(gn.nntpserver);
    if ( (gn.nntpserver = malloc(strlen(pt)+1)) == NULL ) {
      memory_error();
      return(QUIT);
    }
    strcpy(gn.nntpserver,pt);
    break;
  }
  
  /* .gnrc-NNTPSERVER */
  if ( (env = expand_filename(GNRC,gn.home)) == 0 ) {
#if ! defined(WINDOWS)
    mc_printf("ファイル名の展開に失敗しました(%s)\n",GNRC);
#else	/* WINDOWS */
    mc_MessageBox(hWnd,"ファイル名の展開に失敗しました",GNRC,MB_OK);
#endif	/* WINDOWS */
    return(QUIT);
  }
  
  strcpy(resp,env);
  strcat(resp,"_");
  strcat(resp,gn.nntpserver);
  if ( stat(resp,&stat_buf) == 0 ) {
    if ( read_gnrc(resp,env_table,gnrc_table) == ERROR )
      return(QUIT);
  } else {
    strcpy(resp,env);
    strcat(resp,"-");
    strcat(resp,gn.nntpserver);
    if ( stat(resp,&stat_buf) == 0 ) {
      if ( read_gnrc(resp,env_table,gnrc_table) == ERROR )
	return(QUIT);
    } else {
      strcpy(resp,env);
      strcat(resp,".");
      strcat(resp,gn.nntpserver);
      if ( stat(resp,&stat_buf) == 0 ) {
	if ( read_gnrc(resp,env_table,gnrc_table) == ERROR )
	  return(QUIT);
      }
    }
  }

  /************************************************************************/

  if ( init_gn_2() == QUIT )
    return(QUIT);

  /* ドメイン */

  /* domainname はカスタマイズされていなければならない */
  if (strlen(gn.domainname) <= 0 ||
      strcmp(gn.domainname,"your.domain.name") == 0 ) {
#ifndef	WINDOWS
    mc_puts("ドメイン名が正しく設定されていません \n");
    exit(1);
#else	/* WINDOWS */
    mc_MessageBox(hWnd,"ドメイン名が正しく設定されていません ","",MB_OK);
    return(QUIT);
#endif	/* WINDOWS */
  }

  /* organization はカスタマイズされていなければならない */
  if (strlen(gn.organization) <= 0 ||
      strcmp(gn.organization,"your organization") == 0 ) {
#ifndef	WINDOWS
    mc_puts("組織名が正しく設定されていません \n");
    exit(1);
#else	/* WINDOWS */
    mc_MessageBox(hWnd,"組織名が正しく設定されていません ","",MB_OK);
    return(QUIT);
#endif	/* WINDOWS */
  }

  /* ニュースシステム */

  if ( (gn.newslib = expand_filename(gn.newslib,gn.home)) == 0 ) {
#if ! defined(WINDOWS)
    mc_printf("ファイル名の展開に失敗しました(%s)\n",gn.newslib);
#else	/* WINDOWS */
    mc_MessageBox(hWnd,"ファイル名の展開に失敗しました",gn.newslib,MB_OK);
#endif	/* WINDOWS */
    return(QUIT);
  }
  
  if ( (gn.newsspool = expand_filename(gn.newsspool,gn.home)) == 0 ) {
#if ! defined(WINDOWS)
    mc_printf("ファイル名の展開に失敗しました(%s)\n",gn.newsspool);
#else	/* WINDOWS */
    mc_MessageBox(hWnd,"ファイル名の展開に失敗しました",gn.newsspool,MB_OK);
#endif	/* WINDOWS */
    return(QUIT);
  }

  switch ( dos_newfs ) {
  case ON:
    gn.dosfs = OFF;
    break;
  case OFF:
    gn.dosfs = ON;
    break;
  }
#if !defined(UNIX) && !defined(OS2) && !defined(MSC9)
  gn.dosfs = ON;
#endif

  /* 自ホスト */

  /*
   * ホスト名
   */
#if defined(MSDOS) || defined(OS2)
  if ( gn.hostname == 0 ) {
#ifndef	WINDOWS
    mc_puts("HOST に、ＰＣのホスト名を設定して下さい\n");
    exit(1);
#else	/* WINDOWS */
    mc_MessageBox(hWnd,"HOST に、ＰＣのホスト名を設定して下さい","",MB_OK);
    return(QUIT);
#endif	/* WINDOWS */
  }
  
#else	/* MSDOS */
  if ( gn.hostname == 0 ) {
#if ( defined(sony_news) || defined(__sony_news) ) && defined(USG)
    struct utsname name;
    uname(&name);
    if ( (gn.hostname = malloc(strlen(name.nodename)+1)) == NULL ) {
      memory_error();
      return(QUIT);
    }
    strcpy(gn.hostname,name.nodename);
#else	/* sony news USG */
    /* hostname に domainname がついている場合は？？？ */
    if ( gethostname(host_name,MAXHOSTNAME) != 0 ) {
      mc_puts("\nホスト名が取得できませんでした\n\007");
      exit(1);
    }
    if ( (gn.hostname = malloc(strlen(host_name)+1)) == NULL ) {
      memory_error();
      return(QUIT);
    }
    strcpy(gn.hostname,host_name);
#endif	/* sony news USG */
  }
  /* ホスト名に . がある場合は . 以降を消す */
  pt = strchr(gn.hostname, '.');
  if ( pt != (char *)NULL )
    *pt = 0;
#endif	/* MSDOS || OS2 */

  /* ユーザ */

  /*
   * login name
   */
#ifdef UNIX
  if ( gn.usrname == 0 ) {
    if ( pw != NULL ){
      if ( (gn.usrname = malloc(strlen(pw->pw_name)+1)) == NULL ) {
	memory_error();
	return(QUIT);
      }
      strcpy(gn.usrname,pw->pw_name);
    }
  }
  if ( (pt = getenv("LOGINNAME")) != 0 )
    gn.usrname = pt;
#endif	/* UNIX */

  if ( gn.usrname == 0 ) {
#ifndef	WINDOWS
    mc_puts("USER に、ユーザ名を設定して下さい\n");
    exit(1);
#else	/* WINDOWS */
    mc_MessageBox(hWnd,"USER に、ユーザ名を設定して下さい","",MB_OK);
    return(QUIT);
#endif	/* WINDOWS */
  }

  /*
   * Full Name
   */
#ifdef UNIX
  if ( gn.fullname == 0 ) {
    if ( pw != NULL ){
      pt = (char *)pw->pw_gecos;
      sp = full_name;
      while (*pt != '\0' && *pt != ',' && *pt != ';' && *pt != '%') {
	if (*pt == '&') {
	  strcpy(sp, gn.usrname);
	  if ( 'a' <= *sp && *sp <= 'z' )
	    *sp = *sp - 'a' + 'A';
	  while (*sp != '\0')
	    sp++;
	  pt++;
	} else {
	  *sp++ = *pt++;
	}
      }
      *sp = 0;
      if ( (gn.fullname = malloc(strlen(full_name)+1)) == NULL ) {
	memory_error();
	return(QUIT);
      }
      strcpy(gn.fullname,full_name);
    }
  }
#endif	/* UNIX */

  if ( gn.fullname == 0 ) {
#ifndef	WINDOWS
    mc_puts("NAME に、本名を設定して下さい\n");
    exit(1);
#else	/* WINDOWS */
    mc_MessageBox(hWnd,"NAME に、本名を設定して下さい","",MB_OK);
    return(QUIT);
#endif	/* WINDOWS */
  }


  /* directory / file */

  /* .newsrc_NNTPSERVER */
  if ( (env = expand_filename(gn.rcfile,gn.home)) == 0 ) {
#if ! defined(WINDOWS)
    mc_printf("ファイル名の展開に失敗しました(%s)\n",gn.rcfile);
#else	/* WINDOWS */
    mc_MessageBox(hWnd,"ファイル名の展開に失敗しました",gn.rcfile,MB_OK);
#endif	/* WINDOWS */
    return(QUIT);
  }
  strcpy(resp,env);
  strcat(resp,"_");
  strcat(resp,gn.nntpserver);
  if ( stat(resp,&stat_buf) == 0 ) {
    if ( (gn.rcfile = malloc(strlen(resp)+1)) == NULL ) {
      memory_error();
      return(QUIT);
    }
    strcpy(gn.rcfile,resp);
  } else {
    strcpy(resp,env);
    strcat(resp,"-");
    strcat(resp,gn.nntpserver);
    if ( stat(resp,&stat_buf) == 0 ) {
      if ( (gn.rcfile = malloc(strlen(resp)+1)) == NULL ) {
	memory_error();
	return(QUIT);
      }
      strcpy(gn.rcfile,resp);
    } else {
      strcpy(resp,env);
      strcat(resp,".");
      strcat(resp,gn.nntpserver);
      if ( stat(resp,&stat_buf) == 0 ) {
	if ( (gn.rcfile = malloc(strlen(resp)+1)) == NULL ) {
	  memory_error();
	  return(QUIT);
	}
	strcpy(gn.rcfile,resp);
      } else {
	gn.rcfile = env;
      }
    }
  }

#ifdef	DOSFS
  while ( (pt = strchr(gn.savedir,'/')) != NULL )
    *pt = SLASH_CHAR;				    /* / -> \ */
#endif	/* MSDOS */
  /* savedir の展開 */
  if ( (pt = expand_filename(gn.savedir,gn.home)) == 0 ) {
#if ! defined(WINDOWS)
    mc_printf("ファイル名の展開に失敗しました(%s)\n",gn.savedir);
#else	/* WINDOWS */
    mc_MessageBox(hWnd,"ファイル名の展開に失敗しました",gn.savedir,MB_OK);
#endif	/* WINDOWS */
    return(QUIT);
  }
  free(gn.savedir);
  gn.savedir = pt;
  
  /* savedir はディレクトリでなければならない */
  if ( stat(gn.savedir,&stat_buf) == 0 ) {
    if ( (stat_buf.st_mode & S_IFMT) != S_IFDIR ) {	/* S_IFLNK は ? */
#ifndef	WINDOWS
      mc_puts("記事を保存するディレクトリとしてディレクトリ以外が指定されています\n");
      /* じゃぁ、新しいディレクトリの入力を促せよ */
      exit(1);
#else	/* WINDOWS */
      mc_MessageBox(hWnd,"記事を保存するディレクトリとしてディレクトリ以外が指定されています","",MB_OK);
      return(QUIT);
#endif	/* WINDOWS */
    }
  }
  /* savedir の最後は / で終るようにする */
  if ( gn.savedir[strlen(gn.savedir)-1] != SLASH_CHAR ) {
    if ( (pt = malloc(strlen(gn.savedir)+2)) == NULL ) {
      memory_error();
      return(QUIT);
    }
    strcpy(pt,gn.savedir);
    strcat(pt,SLASH_STR);
    free(gn.savedir);
    gn.savedir = pt;
  }

  if ( (gn.signature = expand_filename(gn.signature,gn.home)) == 0 ){
#if ! defined(WINDOWS)
    mc_printf("ファイル名の展開に失敗しました(%s)\n",gn.signature);
#else	/* WINDOWS */
    mc_MessageBox(hWnd,"ファイル名の展開に失敗しました",gn.signature,MB_OK);
#endif	/* WINDOWS */
    return(QUIT);
  }

  if ( (gn.author_copy = expand_filename(gn.author_copy,gn.home)) == 0 ) {
#if ! defined(WINDOWS)
    mc_printf("ファイル名の展開に失敗しました(%s)\n",gn.author_copy);
#else	/* WINDOWS */
    mc_MessageBox(hWnd,"ファイル名の展開に失敗しました",gn.author_copy,MB_OK);
#endif	/* WINDOWS */
    return(QUIT);
  }
  
  if ( (gn.tmpdir = expand_filename(gn.tmpdir,gn.home)) == 0 ) {
#if ! defined(WINDOWS)
    mc_printf("ファイル名の展開に失敗しました(%s)\n",gn.tmpdir);
#else	/* WINDOWS */
    mc_MessageBox(hWnd,"ファイル名の展開に失敗しました",gn.tmpdir,MB_OK);
#endif	/* WINDOWS */
    return(QUIT);
  }
#ifdef	DOSFS
  while ( (pt = strchr(gn.tmpdir,'/')) != NULL )
    *pt = SLASH_CHAR;

  if ( (pt = strchr(gn.tmpdir, ':')) != NULL){
    /* tmpdir にディスクが書かれている */
    pt++;
    disk = GetDisk();
    if (SetDisk(toupper(gn.tmpdir[0])-'A')){
#ifndef	WINDOWS
      mc_puts("指定されたテンポラリディスクは使えません \n");
      return(QUIT);
#else	/* WINDOWS */
      mc_MessageBox(hWnd,"指定されたテンポラリディスクは使えません ","",MB_OK);
      return(QUIT);
#endif	/* WINDOWS */
    }
    SetDisk(disk);
  }
#endif	/* DOSFS */

  /* tmpdir はディレクトリでなければならない */
  if (disk == -1)
    pt = gn.tmpdir;
  if (strcmp(pt, SLASH_STR) != 0 && strcmp(pt, "") !=0){
    /* tmpdir='/' or tmpdir='\0' はチェックしない - 主としてMS-DOS用 */
    if ( stat(gn.tmpdir,&stat_buf) != 0 ) {
      if ( MKDIR(gn.tmpdir) != 0 ) {	/* boooom */
#ifndef	WINDOWS
	mc_printf("テンポラリディレクトリとして指定されている %s を作成することができません \n",gn.tmpdir);
	perror(gn.tmpdir);
	exit(1);
#else	/* WINDOWS */
	mc_MessageBox(hWnd,"テンポラリディレクトリを作成することができません ",
		      gn.tmpdir,MB_OK);
	return(QUIT);
#endif	/* WINDOWS */
      } else {
	mc_printf("テンポラリディレクトリとして指定されている %s が存在しなかったため、作成しました\n",gn.tmpdir);
      }
    } else {
      if ( (stat_buf.st_mode & S_IFMT) != S_IFDIR ) {	/* S_IFLNK は ? */
#ifndef	WINDOWS
	mc_puts("テンポラリディレクトリとしてディレクトリ以外が指定されています\n");
	/* じゃぁ、新しいディレクトリの入力を促せよ */
	exit(1);
#else	/* WINDOWS */
	mc_MessageBox(hWnd,"テンポラリディレクトリとしてディレクトリ以外が指定されています",
		      "",MB_OK);
	return(QUIT);
#endif	/* WINDOWS */
      }
    }
  }
  /* tmpdir の最後は / で終るようにする */
  if ( gn.tmpdir[strlen(gn.tmpdir)-1] != SLASH_CHAR ) {
    if ( (pt = malloc(strlen(gn.tmpdir)+2)) == NULL ) {
      memory_error();
      return(QUIT);
    }
    strcpy(pt,gn.tmpdir);
    strcat(pt,SLASH_STR);
    free(gn.tmpdir);
    gn.tmpdir = pt;
  }

  if ( gn.jnames_file != 0 ) {
    if ( (gn.jnames_file = expand_filename(gn.jnames_file,gn.home)) == 0 ) {
#if ! defined(WINDOWS)
      mc_printf("ファイル名の展開に失敗しました(%s)\n",gn.jnames_file);
#else	/* WINDOWS */
      mc_MessageBox(hWnd,"ファイル名の展開に失敗しました",gn.jnames_file,MB_OK);
#endif	/* WINDOWS */
      return(QUIT);
    }
  }
  /************************************************************************/

#ifdef	WINDOWS
  if ( gn.win_lines != -1 )
    gn.lines = gn.win_lines;

  if ( gn.win_columns != -1 )
    gn.columns = gn.win_columns;

  if ( gn.win_editor != NULL ) {
    free(gn.editor);
    gn.editor = gn.win_editor;
  }
#endif	/* WINDOWS */
  
  /* 無視するニュースグループ */
  if ( unsubscribe_string != 0 ) {
    if ( check_regexp_group(unsubscribe_string,&gn.unsubscribe_groups) == ERROR )
      return(QUIT);
    if ( subscribe_string != 0 ) {
      free(subscribe_string);
      subscribe_string = 0;
    }
  }
  /* 購読するニュースグループ */
  if ( subscribe_string != 0 ) {
    if ( check_regexp_group(subscribe_string,&gn.subscribe_groups) == ERROR )
      return(QUIT);
  }
  
  /* 漢字コード */
  if ( mail_lang != 0 ) {
    for ( lang_tp = &lang_table[0] ; lang_tp->str != 0 ; lang_tp++ ) {
      if (strcmp(lang_tp->str,mail_lang) == 0 ) {
	gn.mail_kanji_code = lang_tp->value;
	break;
      }
    }
  }
  if ( process_lang != 0 ) {
    for ( lang_tp = &lang_table[0] ; lang_tp->str != 0 ; lang_tp++ ) {
      if (strcmp(lang_tp->str,process_lang) == 0 ) {
	gn.process_kanji_code = lang_tp->value;
	break;
      }
    }
  }
  if ( file_lang != 0 ) {
    for ( lang_tp = &lang_table[0] ; lang_tp->str != 0 ; lang_tp++ ) {
      if (strcmp(lang_tp->str,file_lang) == 0 ) {
	gn.file_kanji_code = lang_tp->value;
	break;
      }
    }
  }
  if ( display_lang != 0 ) {
    for ( lang_tp = &lang_table[0] ; lang_tp->str != 0 ; lang_tp++ ) {
      if (strcmp(lang_tp->str,display_lang) == 0 ) {
	gn.display_kanji_code = lang_tp->value;
	break;
      }
    }
  }
  
  if ( gnspool_lang != 0 ) {
    for ( lang_tp = &lang_table[0] ; lang_tp->str != 0 ; lang_tp++ ) {
      if (strcmp(lang_tp->str,gnspool_lang) == 0 ) {
	gn.gnspool_kanji_code = lang_tp->value;
	break;
      }
    }
  }
  if ( prog == GN && gn.with_gnspool == ON ) {
    /* 通常スプールをアクセスするときは gnspool_kanji_code なので、
     * gn.spool_kanji_code = gn.gnspool_kanji_code しておくが、
     * ポストするときは gn.spool_kanji_code が必要になるので、
     * gn.gnspool_kanji_code にコッソリ入れておく
     * gn.gnspool_kanji_code は post_file_with_gnspool() でのみ使用
     */
    int kanji_code;
    kanji_code = gn.spool_kanji_code;
    gn.spool_kanji_code = gn.gnspool_kanji_code;
    gn.gnspool_kanji_code = kanji_code;
  }

  if ( prog == GNSPOOL )
    gn.mime_head = gn.gnspool_mime_head;

#ifdef UNIX
  /* ロックファイルのチェック */
  if (prog == GN && check_only == 0) {
    if ( lock_gn(gn.nntpserver) != 0 ) {	/* lock file OK ? */
      mc_puts("すでに gn が起動されています\n");
      exit(1);
    }
  }
#endif
  
  
#ifdef	SIGCONT
  signal(SIGCONT, foreground);
#endif
  
#if ! defined(NO_NET)
#if defined(WINSOCK)
  if (gn.local_spool == OFF) {	/* ネットワークモード */
    wVersionRequested = 0x0101;
    if ( WSAStartup(wVersionRequested, &wsaData) != 0) {
#ifndef	WINDOWS
      mc_puts("winsock がみつかりません \n");
      exit(1);
#else	/* WINDOWS */
      mc_MessageBox(hWnd,"winsock がみつかりません ","",MB_OK);
      return(QUIT);
#endif	/* WINDOWS */
    }
  }
#endif	/* WINSOCK */

#if defined(INETBIOS) || defined(PATHWAY) || defined(LWP)
  if (gn.local_spool == OFF) {	/* ネットワークモード */
    if( find_inet() == 0 ) {
      mc_puts("BIOSが常駐していません \n");
      exit(1);
    }
  }
#endif	/*  defined(INETBIOS) || defined(PATHWAY) || defined(LWP)*/
#endif	/* ! NO_NET */
  
  if ( prog == GN && gn.with_gnspool == ON )
    gn.local_spool = ON;

  if ( gn.local_spool == ON ) {
    if ( gn.process_kanji_code == gn.spool_kanji_code )
      make_article_tmpfile = OFF;
  }
#ifdef	MIME
  if ( (gn.mime_head & MIME_DECODE) != 0 )
    make_article_tmpfile = ON;
#endif

#ifdef	WINDOWS
  GetWindowRect(GetDesktopWindow(),&lprct);	/* desktop */
  
  GetWindowRect(hWnd,&lprcw);	/* 現在の Window */
  
  if ( (gn.columns * xsiz) > lprct.right ) {	/* 幅が desktop より大きい */
    wwidth = (lprct.right/ xsiz) * xsiz;
    xpos = 0;
  } else {
    wwidth = gn.columns * xsiz;
    xpos = lprcw.left;
  }
  if ( (xpos + wwidth) > lprct.right )
    xpos = 0;
  
  if ( (gn.lines * ysiz) > lprct.bottom ) {	/* 高さが desktop より大きい */
    wheight = (lprct.bottom/ ysiz) * ysiz;
    ypos = 0;
  } else {
    wheight = gn.lines * ysiz;
    ypos = lprcw.top;
  }
  if ( (ypos + wheight) > lprct.bottom )
    ypos = 0;
  
  SetWindowPos(hWnd,(HWND)NULL,xpos,ypos,wwidth,wheight,
	       SWP_NOZORDER|SWP_SHOWWINDOW);
  
  xpos = ypos = 0;
  
  free(display_buf);
  if ( (display_buf = malloc(gn.lines*gn.columns)) == NULL ) {
    memory_error();
    return(QUIT);
  }
  memset(display_buf,' ',gn.lines*gn.columns);
#endif	/* WINDOWS */

#ifdef AUTHENTICATION
#if defined(MSDOS) || defined(OS2)
#if ! defined(NO_NET)
#if ! defined(WINDOWS)
  if (gn.local_spool == OFF) {	/* ネットワークモード */
    if ( prog == GN || DO_POST || DO_MAIL) {
      if ( auth(gn.usrname) != 0 )	/* ユーザ認証 */
	exit(1);
    }
  }
#endif	/* ! WINDOWS */
#endif	/* ! NO_NET */
#endif	/* MSDOS || OS2 */
#endif /* AUTHENTICATION */
  return(check_only);
}
/*
 * struct gn の初期化（コンパイル時デフォルトその１）
 */
static void
init_gn_1()
{
  /* ドメイン */
  gn.domainname = 0;			/* ドメイン名 */
  gn.organization = 0;			/* 組織名 */
  gn.genericfrom = DEFAULT_GENERICFROM;	/* From: にホスト名をつけない */

  /* ニュースシステム */
  gn.nntpserver = 0;		/* ニュースサーバ */
  gn.nntpport = 0;		/* NNTP に使用するポート */
  gn.spool_kanji_code = JIS;	/* NNTP の漢字コード */
  gn.mode_reader = OFF;		/* Inn の場合は ON にすると mode reader 送信 */
  gn.nntp_auth = OFF;		/* NNTP によるユーザ認証 */
#if defined(NO_NET)
  gn.local_spool = ON;		/* ローカルマシンのスプールを見る */
#else
  gn.local_spool = OFF;		/* ローカルマシンのスプールを見る */
#endif
  gn.use_history = OFF;		/* history を使用する */
  gn.postproc = 0;		/* ポストする際に使用するプログラム */
  gn.newslib = 0;		/* /usr/lib/news */
  gn.newsspool = 0;		/* /usr/spool/news */
#if defined(MSDOS) || defined(OS2)
  gn.dosfs = ON;		/* FAT に spool を作成する */
#else
  gn.dosfs = OFF;		/* FAT に spool を作成する */
#endif

  /* メイルシステム */
  gn.smtpserver = 0;		/* SMTP サーバ */
  gn.smtpport = 0;		/* SMTP に使用するポート */
  gn.authserver = 0;		/* POP サーバ */
  gn.pop3port = 0;		/* POP3 に使用するポート */
  gn.mail_kanji_code = DEFAULT_MAIL_KANJI_CODE;	/* メイルの漢字コード */
  gn.mailer = 0;		/* メイラ */

  /* 自ホスト */
  gn.hostname = 0;		/* ホスト名 */
  gn.lines = DEFAULT_LINES;	/* 画面行数 */
  gn.columns = DEFAULT_COLUMNS; /* 画面桁数 */
  gn.win_lines = -1;
  gn.win_columns = -1;

  gn.process_kanji_code = DEFAULT_PROCESS_KANJI_CODE;	/* ページャ、エディタの漢字コード */
  gn.file_kanji_code = DEFAULT_FILE_KANJI_CODE;		/* ファイルにセーブする時の漢字コード */
  gn.display_kanji_code = DEFAULT_DISPLAY_KANJI_CODE;	/* 画面の漢字コード */

#if ! defined(WINDOWS) && ! defined(QUICKWIN)
  gn.pager = 0;			/* ページャ */
  gn.return_after_pager = DEFAULT_RETURN_AFTER_PAGER;	/* 記事表示後の Return 入力要求の有無 */
#endif	/* ! WINDOWS && !QUICKWIN */

  gn.win_pager_flag = 0;	/* 最終ページで止まる */
  gn.editor = 0;		/* エディタ */
  gn.win_editor = 0;		/* Windows の時のエディタ */
#ifdef	USE_FGETS
  gn.use_space = ON;		/* スペースキーで読み進める */
#else
  gn.use_space = OFF;		/* スペースキーで読み進める */
#endif
#ifdef	POWER_SAVE
  gn.power_save = COPY_TO_TMP;	/* ディスクへのアクセスを一時期に */
#else
  gn.power_save = OFF;		/* ディスクへのアクセスを一時期に */
#endif

  /* ユーザ */
  gn.usrname = 0;	/* login name */
  gn.fullname = 0;	/* Full Name */
  gn.passwd = 0;	/* password. booooom! */

  /* directory / file */
  gn.rcfile = 0;		/* newsrc */
  gn.savedir = 0;		/* 記事をセーブするディレクトリ */
  gn.signature = 0;		/* signature file */
  gn.author_copy = 0;		/* author copy */
  gn.tmpdir = 0;		/* $TMPDIR */

  gn.jnames_file = 0;		/* jnames file for a user */

  /* newsgroup */
  gn.gnus_format = 1;		/* GNUS format の .newsrc を読み込んだ */
  gn.ng_sort = NG_SORT_NO;	/* sort new newsgroup */
  gn.unsubscribe_groups = 0;
  gn.subscribe_groups = 0;
  gn.article_limit = ARTICLE_LIMIT;	/* 起動時未読記事数がこれより多い時は、確認入力 */
  gn.article_leave = ARTICLE_LEAVE;	/* 捨てる場合に残す数 */
  gn.select_limit = SELECT_LIMIT;	/* 選択時記事数がこれより多い時は、確認入力 */
  gn.select_number = SELECT_NUMBER;	/* 確認入力時のデフォルトの記事数 */

  /* 読む */
  gn.disp_re = ON;	/* 全部フォロー記事の時は Re: を表示 */
  gn.cls = OFF;		/* 画面クリア */
  gn.description = OFF;	/* ニュースグループの説明を表示する */
  gn.kill_control = 0;	/* コントロールメッセージを既読に */
  gn.autolist = ON;	/* 各モードでのリスト表示を自動的に行なう */
  gn.show_indent = OFF;	/* モード毎に字下げして表示 */
  gn.show_truncate = ON;/* 折り曲げて全部表示する */
  gn.mime_head = OFF;	/* ヘッダの MIME 対応 */
  gn.gmt2jst = OFF;	/* Date: ヘッダの JST 表示 */

  /* post */
  gn.nsubject = OFF;	/* 日本語サブジェクト使用 */
  gn.rn_like = OFF;	/* rn ライクな引用タグ生成 */
  gn.cc2me = OFF;	/* リプライメールを自分にも Cc/Bcc */
  gn.substitute_header = ON;	/* ヘッダを付け変える */

  /* other */
#if defined(NO_NET)
  gn.with_gnspool = ON;		/* gnspool と組み合わせて使用する */
#else
  gn.with_gnspool = OFF;	/* gnspool と組み合わせて使用する */
#endif

  gn.gnspool_kanji_code = gn.spool_kanji_code;	/* gnspool が作成するスプールの漢字コード */

  /* gnspool only */
  gn.fast_connect = OFF;	/* active を取得しない */
  gn.remove_canceled = OFF;	/* キャンセルされた記事を消す */
  gn.gnspool_mime_head = OFF;	/* ヘッダの MIME 対応 */
  gn.gnspool_line_limit = 0;	/* 取寄せる記事の最大行数 */

}
/*
 * struct gn の初期化（コンパイル時デフォルトその２）
 */
static int
init_gn_2()
{
  register char *pt;

  /* ドメイン名 */
  if ( gn.domainname == 0 ) {
    if ( (gn.domainname = malloc(strlen(DEFAULT_DOMAINNAME)+1)) == NULL ) {
      memory_error();
      return(QUIT);
    }
    strcpy(gn.domainname,DEFAULT_DOMAINNAME);
  }

  /* 組織名 */
  if ( gn.organization == 0 ) {
    if ( (gn.organization = malloc(strlen(DEFAULT_ORGANIZATION)+1)) == NULL ) {
      memory_error();
      return(QUIT);
    }
    strcpy(gn.organization,DEFAULT_ORGANIZATION);
  }

  /* ニュースサーバ */
  if ( gn.nntpserver == 0 ) {
    if ( (gn.nntpserver = malloc(strlen(DEFAULT_NNTPSERVER)+1)) == NULL ) {
      memory_error();
      return(QUIT);
    }
    strcpy(gn.nntpserver,DEFAULT_NNTPSERVER);
  }

  /* NNTP に使用するポート */
  if ( (pt = strchr(gn.nntpserver,':')) != NULL ) {
    *pt = 0;
    pt++;
    gn.nntpport = atoi(pt);
  }

  /* ポストする際に使用するプログラム */
  if ( gn.postproc == 0 ) {
    if ( (gn.postproc = malloc(strlen(DEFAULT_POSTPROC)+1)) == NULL ) {
      memory_error();
      return(QUIT);
    }
    strcpy(gn.postproc,DEFAULT_POSTPROC);
  }

  /* /usr/lib/news */
  if ( gn.newslib == 0 ) {
    if ( (gn.newslib = malloc(strlen(DEFAULT_NEWSLIB)+1)) == NULL ) {
      memory_error();
      return(QUIT);
    }
    strcpy(gn.newslib,DEFAULT_NEWSLIB);
  }

  /* /usr/spool/news */
  if ( gn.newsspool == 0 ) {
    if ( (gn.newsspool = malloc(strlen(DEFAULT_NEWSSPOOL)+1)) == NULL ) {
      memory_error();
      return(QUIT);
    }
    strcpy(gn.newsspool,DEFAULT_NEWSSPOOL);
  }

  /* smtp サーバ */
  if ( gn.smtpserver == 0 ) {
    if ( (gn.smtpserver = malloc(strlen(DEFAULT_SMTPSERVER)+1)) == NULL ) {
      memory_error();
      return(QUIT);
    }
    strcpy(gn.smtpserver,DEFAULT_SMTPSERVER);
  }

  /* smtp に使用するポート */
  if ( (pt = strchr(gn.smtpserver,':')) != NULL ) {
    *pt = 0;
    pt++;
    gn.smtpport = atoi(pt);
  }

  /* 認証サーバ */
  if ( gn.authserver == 0 ) {
    if ( (gn.authserver = malloc(strlen(gn.smtpserver)+1)) == NULL ) {
      memory_error();
      return(QUIT);
    }
    strcpy(gn.authserver,gn.smtpserver);
  }

  /* pop3 に使用するポート */
  if ( (pt = strchr(gn.authserver,':')) != NULL ) {
    *pt = 0;
    pt++;
    gn.pop3port = atoi(pt);
  }

  /* メイラ */
  if ( gn.mailer == 0 ) {
    if ( (gn.mailer = malloc(strlen(DEFAULT_MAILER)+1)) == NULL ) {
      memory_error();
      return(QUIT);
    }
    strcpy(gn.mailer,DEFAULT_MAILER);
  }

#if ! defined(WINDOWS) && ! defined(QUICKWIN)
  /* ページャ */
  if ( gn.pager == 0 ) {
    if ( (gn.pager = malloc(strlen(DEFAULT_PAGER) + 1)) == NULL ) {
      memory_error();
      return(QUIT);
    }
    strcpy(gn.pager,DEFAULT_PAGER);
  }
#endif	/* ! WINDOWS && !QUICKWIN */

  /* エディタ */
  if ( gn.editor == 0 ) {
    if ( (gn.editor = malloc(strlen(DEFAULT_EDITOR)+1)) == NULL ) {
      memory_error();
      return(QUIT);
    }
    strcpy(gn.editor,DEFAULT_EDITOR);
  }

  /* newsrc */
  if ( gn.rcfile == 0 ) {
    if ( (gn.rcfile = malloc(strlen(DEFAULT_NEWSRC)+1)) == NULL ) {
      memory_error();
      return(QUIT);
    }
    strcpy(gn.rcfile,DEFAULT_NEWSRC);
  }

  /* 記事をセーブするディレクトリ */
  if ( gn.savedir == 0 ) {
    if ( (gn.savedir = malloc(strlen(DEFAULT_SAVEDIR)+1)) == NULL ) {
      memory_error();
      return(QUIT);
    }
    strcpy(gn.savedir,DEFAULT_SAVEDIR);
  }

  /* signature file */
  if ( gn.signature == 0 ) {
    if ( (gn.signature = malloc(strlen(DEFAULT_SIGNATURE)+1)) == NULL ) {
      memory_error();
      return(QUIT);
    }
    strcpy(gn.signature,DEFAULT_SIGNATURE);
  }

  /* author copy */
  if ( gn.author_copy == 0 ) {
    if ( (gn.author_copy = malloc(strlen(DEFAULT_AUTHORCOPY)+1)) == NULL ) {
      memory_error();
      return(QUIT);
    }
    strcpy(gn.author_copy,DEFAULT_AUTHORCOPY);
  }

  /* $TMPDIR */
  if ( gn.tmpdir == 0 ) {
    if ( (gn.tmpdir = malloc(strlen(DEFAULT_TMPDIR)+1)) == NULL ) {
      memory_error();
      return(QUIT);
    }
    strcpy(gn.tmpdir,DEFAULT_TMPDIR);
  }

  /* jnames file for a user */
  if ( gn.jnames_file == 0 ) {
    if ( (gn.jnames_file = malloc(strlen(DEFAULT_JNAMESFILE)+1)) == NULL ) {
      memory_error();
      return(QUIT);
    }
    strcpy(gn.jnames_file,DEFAULT_JNAMESFILE);
  }

  return(CONT);
}
#ifdef	SIGWINCH
static void
resize_window()
{
  struct winsize buf;
  
  if (ioctl (0, TIOCGWINSZ, &buf) < 0)
    return;
  
  if ( gn.lines == buf.ws_row && gn.columns == buf.ws_col )
    return;
  
  if ( buf.ws_row != 0 )
    gn.lines = buf.ws_row;
  if ( buf.ws_col != 0 )
    gn.columns = buf.ws_col;
  
  if ( redraw_func != (void (*)())NULL)
    redraw_func();
  
  signal(SIGWINCH, resize_window);
}
#endif

#ifdef	SIGCONT
static void
foreground()
{
  if ( redraw_func != (void (*)())NULL)
    redraw_func();
  
  signal(SIGCONT, foreground);
}
#endif

/*
 * gnrc の読み込み
 */
int
read_gnrc(gnrc,env_table,gnrc_table)
register char *gnrc;	/* gnrc のパス名 */
struct env_t *env_table;
struct gnrc_t *gnrc_table;
{
  register char *pt;
  FILE *fp;
  struct stat stat_buf;
  char resp[NNTP_BUFSIZE+1];
  char buf[NNTP_BUFSIZE+1];
  register struct env_t *env_tp;
  register struct gnrc_t *gnrc_tp;
  char gnrc_tmp[PATH_LEN+1];
  
  if ( stat(gnrc,&stat_buf) != 0 )
    return(CONT);
  
  if ( (fp = fopen(gnrc,"r")) == NULL )
    return(CONT);
  
  if ( strncmp(gnrc,gn.home,strlen(gn.home)) == 0 ) {
    strcpy(gnrc_tmp,"~/");
    strcat(gnrc_tmp,&gnrc[strlen(gn.home)+1]);
  } else {
    strcpy(gnrc_tmp,gnrc);
  }
  mc_printf("reading %s\n",gnrc_tmp,0,0,0);
  
  while ( fgets(resp,NNTP_BUFSIZE,fp) != NULL ) {
    if ( (pt = strchr(resp,'\n')) !=NULL )
      *pt = 0;
    if ( (pt = strchr(resp,'\r')) !=NULL )
      *pt = 0;
    pt = &resp[0];
    while ( *pt == ' ' || *pt == '\t' )
      pt++;			/* 先行するホワイトスペースを無視 */
    if ( *pt == 0 )
      continue;			/* ホワイトスペースのみ */
    if ( *pt == '#' )
      continue;
    while ( *pt != 0 && *pt != ' ' && *pt != '\t' )
      pt++;
    if ( *pt == 0 ) {		/* value がない */
      mc_printf("%s に不正な行があります:%s",gnrc_tmp,resp);
      continue;
    }
    *pt++ = 0;
    while ( *pt == ' ' || *pt == '\t' )
      pt++;			/* keyword と value の間のホワイトスペース */
    if ( *pt == 0 ) {		/* value がない */
      mc_printf("%s に不正な行があります:%s",gnrc_tmp,resp);
      continue;
    }

    /* 環境変数と共通の設定:文字列 */
    for ( env_tp = env_table ; env_tp->str != 0 ; env_tp++ ) {
      if (strcmp(env_tp->str,resp) == 0 ) {
	if ( *(env_tp->value) != 0 )
	  free(*(env_tp->value));
	kanji_convert(gn.file_kanji_code,pt,internal_kanji_code,buf);
	if ( (*(env_tp->value) = malloc(strlen(buf)+1)) == NULL ) {
	  memory_error();
	  return(ERROR);
	}
	strcpy(*(env_tp->value),buf);
	break;
      }
    }
    if ( env_tp->str != 0 )
      continue;

    /* gnrc 特有の設定:数字 */
    for ( gnrc_tp = gnrc_table ; gnrc_tp->str != 0 ; gnrc_tp++ ) {
      if (strcmp(gnrc_tp->str,resp) == 0 ) {
	if ( '0' <= *pt && *pt <= '9' ) {
	  *(gnrc_tp->value) = atoi(pt);
	  break;
	}
	if ( strcmp(pt, "ON") == 0 || strcmp(pt, "YES") == 0 ) {
	  *(gnrc_tp->value) = ON;
	  break;
	}
	if ( strcmp(pt, "OFF") == 0 || strcmp(pt, "NO") == 0 ) {
	  *(gnrc_tp->value) = OFF;
	  break;
	}
	mc_printf("%s の %s には数字を指定して下さい。（%s）\n",
		  gnrc_tmp,resp,pt);
	break;
      }
    }
    if ( gnrc_tp->str != 0 )
      continue;

    mc_printf("%s に不正なキーワードがあります:%s\n",gnrc_tmp,resp);
  }
  
  fclose(fp);
  return(CONT);
}

/*
 * 環境変数の解析
 */
int
read_env(env_table,gnrc_table)
struct env_t *env_table;
struct gnrc_t *gnrc_table;
{
  register struct env_t *env_tp;
  register struct gnrc_t *gnrc_tp;
  register char *env;
  
  /* gnrc と共通の設定:文字列 */
  for ( env_tp = env_table ; env_tp->str != 0 ; env_tp++ ) {
    if ((env = getenv(env_tp->str)) == 0)
      continue;

    if ( *(env_tp->value) != 0 )
      free(*(env_tp->value));
    if ( (*(env_tp->value) = malloc(strlen(env)+1)) == NULL ) {
      memory_error();
      return(ERROR);
    }
    strcpy(*(env_tp->value),env);
  }
  
  /* gnrc 特有の設定:数字 */
  for ( gnrc_tp = gnrc_table ; gnrc_tp->str != 0 ; gnrc_tp++ ) {
    if ((env = getenv(gnrc_tp->str)) == 0)
      continue;

    if ( '0' <= *env && *env <= '9' ) {
      *(gnrc_tp->value) = atoi(env);
      continue;
    }
    if ( strcmp(env, "ON") == 0 || strcmp(env, "YES") == 0 ) {
      *(gnrc_tp->value) = ON;
      continue;
    }
    if ( strcmp(env, "OFF") == 0 || strcmp(env, "NO") == 0 ) {
      *(gnrc_tp->value) = OFF;
      continue;
    }
    mc_printf("環境変数 %s には数字を指定して下さい\n",gnrc_tp->str);
  }
  
  return(CONT);
}




/*
 * 正規表現もどきグループの解析
 */
int
check_regexp_group(ig_string,ig)
register char *ig_string;
register regexp_group_t **ig;
{
  extern int check_reg();
  
  register char *pt,ch;
  
  if ( (pt = strchr(ig_string,'\n')) != NULL )
    *pt = 0;
  if ( (pt = strchr(ig_string,'\r')) != NULL )
    *pt = 0;
  
  for ( pt = ig_string; ; ig = &((*ig)->next)) {

    while( *pt == ' ' || *pt == '\t' )
      pt++;
    if ( *pt == 0 )
      break;

    if ( (*ig = (regexp_group_t *)malloc(sizeof(regexp_group_t))) == NULL ) {
      memory_error();
      return(ERROR);
    }

    (*ig)->flag = YES;
    if ( *pt == '!' ) {
      (*ig)->flag = NO;
      pt++;
    }
    (*ig)->name = pt;
    (*ig)->next = 0;

    while ( *pt != 0 && *pt != ',' && *pt != ' ' && *pt != '\t' )
      pt++;
    ch = *pt;
    *pt++ = 0;
    (*ig)->len = strlen((*ig)->name);

    if ( (*ig)->len == 0 ) {
      mc_printf("! は、無視します\n",(*ig)->name);
      continue;
    }
    /* 正規表現のチェック */
    if ( check_reg(*ig) == ERROR ){
      (*ig)->len = 0;
      mc_printf("%s は、無視します\n",(*ig)->name);
      continue;
    }
    if ( ch == 0 )
      break;

    while ( *pt == ',' || *pt == ' ' || *pt == '\t' )
      *pt++ = 0;
  }
  return(CONT);
}

/* 正規表現のチェック */
int
check_reg(ig)
regexp_group_t *ig;
{
  char *pt;
  char ch;

  /* [] による正規表現 */
  for ( pt = ig->name; *pt != 0; pt++ ) {
    if ( (pt = strchr(pt,'[')) == NULL ) /* これ以上 [ なし */
      break;

    ig->len = -1;		/* 正規表現有り */

    if ( strchr(pt,']') == NULL )
      return(ERROR);		/* ] が閉じていない */

    pt++;
    if ( *pt == ']' )
      return(ERROR);		/* [] はエラー */

    while ( *pt != ']' ) {
      if ( *pt == '[' )
	return(ERROR);		/* 閉じる前に [ が来た */

      if ( pt[1] == '-' ) {
	if ( strchr(&pt[3],']') == NULL )
	  return(ERROR);	/* 閉じていない */
	if ( pt[0] > pt[2] )
	  return(ERROR);	/* 昇順でない */
	pt += 3;
      } else {
	pt++;
      }
    }
    pt++;
  }

  /* all */
  ch = '.';
  for ( pt = ig->name; *pt != 0; ch = *pt++ ) {
    if ( ch != '.' )
      continue;
    if ( strlen(pt) < 3 )
      break;
    if ( *pt != 'a' )
      continue;
    if ( strncmp(pt,"all",3) != 0 )
      continue;

    if ( pt[3] == 0 || pt[3] == '.' ) {
      ig->len = -1;		/* 正規表現有り */
      break;
    }
    pt += 3;
  }
  return(CONT);
}
#if defined(OS2)
extern VIOMODEINFO *GetScreenInfo();
static int
GetLines()
{
  return GetScreenInfo()->row;
}

static int
GetColumns()
{
  return GetScreenInfo()->col;
}
#endif /* OS2 */
#if defined(DOSV) && !defined(GNUDOS)
static int
GetLines()
{
  return (int) (*(unsigned char *)0x00400084) + 1;
}

static int
GetColumns()
{
  return *(unsigned short *)0x0040004a;
}
#endif	/* DOSV && !GNUDOS */
