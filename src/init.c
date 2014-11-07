/*
 * @(#)init.c	1.40 May.16,1998
 *
 * ������������ޤ��󡣤�������������Ū�ʳ��λ��ѡ����ۤ����¤��ߤ��ޤ���
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


newsgroup_t *newsrc;		/* �˥塼�����롼�ץꥹ�� */
last_art_t last_art;		/* �Ǹ���ɤ������ */
void (*redraw_func)();

gn_t gn;			/* �������ޥ��� */
int internal_kanji_code;	/* gn �����δ��������� */
int change;			/* newsrc �����ե饰 */
int make_article_tmpfile = ON;	/* �ڡ�����˥ƥ�ݥ��ե�������Ϥ� */
int disp_mode;			/* ����å�ɽ�� */
char *ngdir;

/* for gnspool */
int gnspooljobs = 0;		/* BIT_EXPIRE,BIT_POST,BIT_MAIL,BIT_GET */
int init_smtp_server = 0;

#if defined(WINDOWS)
char *help_file;	/* temp file for help */

int	xsiz, ysiz, wwidth, wheight, xpos, ypos;
char *display_buf = 0;

HANDLE hInst;			/* ���ߤΥ��󥹥��� */
HWND hWnd;			/* �ᥤ�󥦥���ɥ��Υϥ�ɥ� */
HCURSOR hHourGlass;		/* �����ץ������� */
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

/* �ѥ�᡼����ʸ����Υ������ޥ������� */
struct env_t {
  char *str;
  char **value;
};
/* �ѥ�᡼���������Υ������ޥ������� */
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
  
  hHourGlass = LoadCursor(NULL, IDC_WAIT);  	/* �����ץ������� */
  
  hInst = hInstance;
  
  /* �ᥤ�󥦥���ɥ����� */
  
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
  
  if (!hWnd)	/* ������ɥ�������Ǥ��ʤ��ä� */
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
  
  /* argv �ο�������� */
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
    /* �ɥᥤ�� */
    { GNRC_DOMAINNAME,	&gn.domainname },	/* �ɥᥤ��̾ */
    { GNRC_ORGANIZATION,&gn.organization },	/* �ȿ�̾ */

    /* �˥塼�������ƥ� */
    { GNRC_NNTPSERVER,	&gn.nntpserver },	/* �˥塼�������� */
    { GNRC_POSTPROC,	&gn.postproc },	/* -l �����ݥ��Ȥ�����Υץ���� */
    { GNRC_NEWSLIB,	&gn.newslib },		/* /usr/lib/news */
    { GNRC_NEWSSPOOL,	&gn.newsspool },	/* /usr/spool/news */

    /* �ᥤ�륷���ƥ� */
    { GNRC_SMTPSERVER,	&gn.smtpserver },	/* smtp,pop3|ftp ������ */
    { GNRC_AUTHSERVER,	&gn.authserver },	/* pop ������ */
    { GNRC_MAIL_LANG,	&mail_lang },		/* �ᥤ��δ��������� */
    { GNRC_MAILER,	&gn.mailer },		/* �ᥤ�� */

    /* ���ۥ��� */
    { GNRC_HOST,	&gn.hostname },	/* �ۥ���̾ */
    { GNRC_PROCESS_LANG,&process_lang },/* pager,editor�δ��������� */
    { GNRC_FILE_LANG,	&file_lang },	/* �ե�����δ��������� */
    { GNRC_DISPLAY_LANG,&display_lang },/* �ǥ����ץ쥤�δ��������� */
    { GNRC_PAGER,	&gn.pager },	/* �ڡ����� */
    { GNRC_EDITOR,	&gn.editor },	/* ���ǥ��� */
    { GNRC_WIN_EDITOR,	&gn.win_editor }, /* Windows �λ��Υ��ǥ��� */

    /* �桼�� */
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
    { GNRC_SAVEDIR,	&gn.savedir },	/* ��������¸����ǥ��쥯�ȥ� */
    { GNRC_SIGNATURE,	&gn.signature },/* .signature */
    { GNRC_AUTHOR_COPY,	&gn.author_copy },	/* author_copy */
    { GNRC_TMPDIR,	&gn.tmpdir },	/* �ƥ�ݥ��ǥ��쥯�ȥ� */
    { GNRC_TMP,		&gn.tmpdir },	/* �ƥ�ݥ��ǥ��쥯�ȥ� */
    { GNRC_TEMP,	&gn.tmpdir },	/* �ƥ�ݥ��ǥ��쥯�ȥ� */
    { GNRC_JNAMESFILE,	&gn.jnames_file },

    /* read */
#if 0
    { GNRC_IGNORE,	&unsubscribe_string },	/* ̵�뤹�� newsgroup �� */
#endif
    { GNRC_UNSUBSCRIBE,	&unsubscribe_string },	/* ̵�뤹�� newsgroup �� */
    { GNRC_SUBSCRIBE,	&subscribe_string },	/* ���ɤ��� newsgroup �� */

    /* other */
    { GNRC_GNSPOOL_LANG,&gnspool_lang }, /* gnspool ���������륹�ס���δ��������� */


    { 0,0 }
  };
  static struct gnrc_t gnrc_table[] = {
    /* �ɥᥤ�� */
    { GNRC_GENERICFROM,	&gn.genericfrom },

    /* �˥塼�������ƥ� */
    { GNRC_NNTPPORT,	&gn.nntpport },		/* NNTP �˻��Ѥ���ݡ��� */
    { GNRC_MODE_READER,	&gn.mode_reader },
    { GNRC_NNTP_AUTH,	&gn.nntp_auth },
    { GNRC_LOCAL_SPOOL,	&gn.local_spool },
    { GNRC_USE_HISTORY,	&gn.use_history }, /* history ����Ѥ��� */
#if 0
    { GNRC_DOS_NEWFS,	&dos_newfs },	/* HPFS,NTFS �ʤɤ� spool ��������� */
#endif
    { GNRC_DOSFS,	&gn.dosfs },	/* FAT �� spool ��������� */

    /* �ᥤ�륷���ƥ� */
    { GNRC_SMTPPORT,	&gn.smtpport },		/* SMTP �˻��Ѥ���ݡ��� */
    { GNRC_POP3PORT,	&gn.pop3port },		/* POP3 �˻��Ѥ���ݡ��� */

    /* ���ۥ��� */
    { GNRC_LINES,	&gn.lines },
    { GNRC_WIN_LINES,	&gn.win_lines },
    { GNRC_COLUMNS,	&gn.columns },
    { GNRC_WIN_COLUMNS,	&gn.win_columns },
    { GNRC_RETURN_AFTER_PAGER,	&gn.return_after_pager },
    { GNRC_WIN_PAGER_FLAG,&gn.win_pager_flag },	/* �ǽ��ڡ����ǻߤޤ� */
    { GNRC_USE_SPACE,	&gn.use_space }, /* ���ڡ����������ɤ߿ʤ�� */
    { GNRC_POWER_SAVE,	&gn.power_save }, /* �ǥ������ؤΥ��������������� */

    /* newsgroup */
    { GNRC_GNUS_FORMAT,	&gn.gnus_format },
    { GNRC_NG_SORT,	&gn.ng_sort }, /* sort new newsgroup */
    { GNRC_ARTICLE_LIMIT,	&gn.article_limit },
    { GNRC_ARTICLE_LEAVE,	&gn.article_leave },
    { GNRC_SELECT_LIMIT,	&gn.select_limit },
    { GNRC_SELECT_NUMBER,	&gn.select_number },

    /* read */
    { GNRC_DISP_RE,	&gn.disp_re }, /* �����ե��������λ��� Re: ��ɽ�� */
    { GNRC_CLS,		&gn.cls }, /* ���̥��ꥢ */
    { GNRC_DESCRIPTION,	&gn.description },
    { GNRC_KILL_CANCEL,	&gn.kill_control }, /* upper compatible */
    { GNRC_KILL_CONTROL,&gn.kill_control },
    { GNRC_AUTOLIST,	&gn.autolist },
    { GNRC_SHOW_INDENT,	&gn.show_indent }, /* �⡼����˻���������ɽ�� */
    { GNRC_SHOW_TRUNCATE,	&gn.show_truncate }, /* �ޤ�ʤ�������ɽ������ */
    { GNRC_MIME_HEAD,	&gn.mime_head }, /* �إå��� MIME �б� */
    { GNRC_GMT2JST,	&gn.gmt2jst }, /* Date: �إå��� JST ɽ�� */

    /* post */
    { GNRC_NSUBJECT,	&gn.nsubject },	/* ���ܸ쥵�֥������Ȼ��� */
    { GNRC_RN_LIKE,	&gn.rn_like }, /* rn �饤���ʰ��ѥ������� */
    { GNRC_CC2ME,	&gn.cc2me }, /* ��ץ饤�᡼���ʬ�ˤ� Cc/Bcc */
    { GNRC_SUBSTITUTE_HEADER,	&gn.substitute_header }, /* �إå����դ��Ѥ��� */
    /* other */
    { GNRC_WITH_GNSPOOL,&gn.with_gnspool }, /* gnspool ���ȹ礻�ƻ��Ѥ��� */

    /* for gnspool only */
    { GNRC_FAST_CONNECT,	&gn.fast_connect }, /* active ��������ʤ� */
    { GNRC_REMOVE_CANCELED,	&gn.remove_canceled }, /* ����󥻥뤵�줿������ä� */
    { GNRC_GNSPOOL_MIME_HEAD,	&gn.gnspool_mime_head }, /* �إå��� MIME �б� */
    { GNRC_GNSPOOL_LINE_LIMIT,	&gn.gnspool_line_limit }, /* ��󤻤뵭���κ���Կ� */
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
  
  /* �����ѿ��ν���� */
  newsrc = 0;
  change = 0;
  redraw_func = (void (*)())NULL;
  internal_kanji_code = ((unsigned char *)"��")[0] == 0x82 ? SJIS : EUC;
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
  
  /* ���̹Կ�������μ��������� */
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
  
  /* $HOME/.gnrc �β��� */
  if ( (gnrc = expand_filename(GNRC,gn.home)) == 0 ) {
#if ! defined(WINDOWS)
    mc_printf("�ե�����̾��Ÿ���˼��Ԥ��ޤ���(%s)\n",GNRC);
#else	/* WINDOWS */
    mc_MessageBox(hWnd,"�ե�����̾��Ÿ���˼��Ԥ��ޤ���",GNRC,MB_OK);
#endif	/* WINDOWS */
    return(QUIT);
  }
  
  if ( read_gnrc(gnrc,env_table,gnrc_table) == ERROR)
    return(QUIT);
  
  /* �Ķ��ѿ��β��� */
  if ( read_env(env_table,gnrc_table) == ERROR )
    return(QUIT);
  
  /* ��ư���ץ����β��� */
  if ( read_option(argc,argv,&check_only) == ERROR )
    return(QUIT);

  /* �˥塼�������Ф����ꤵ��Ƥ��ʤ�������� */
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
    if ( (pt = kb_input("�˥塼�������ФΥۥ���̾�����Ϥ��Ʋ�����")) == NULL)
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
    mc_printf("�ե�����̾��Ÿ���˼��Ԥ��ޤ���(%s)\n",GNRC);
#else	/* WINDOWS */
    mc_MessageBox(hWnd,"�ե�����̾��Ÿ���˼��Ԥ��ޤ���",GNRC,MB_OK);
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

  /* �ɥᥤ�� */

  /* domainname �ϥ������ޥ�������Ƥ��ʤ���Фʤ�ʤ� */
  if (strlen(gn.domainname) <= 0 ||
      strcmp(gn.domainname,"your.domain.name") == 0 ) {
#ifndef	WINDOWS
    mc_puts("�ɥᥤ��̾�����������ꤵ��Ƥ��ޤ��� \n");
    exit(1);
#else	/* WINDOWS */
    mc_MessageBox(hWnd,"�ɥᥤ��̾�����������ꤵ��Ƥ��ޤ��� ","",MB_OK);
    return(QUIT);
#endif	/* WINDOWS */
  }

  /* organization �ϥ������ޥ�������Ƥ��ʤ���Фʤ�ʤ� */
  if (strlen(gn.organization) <= 0 ||
      strcmp(gn.organization,"your organization") == 0 ) {
#ifndef	WINDOWS
    mc_puts("�ȿ�̾�����������ꤵ��Ƥ��ޤ��� \n");
    exit(1);
#else	/* WINDOWS */
    mc_MessageBox(hWnd,"�ȿ�̾�����������ꤵ��Ƥ��ޤ��� ","",MB_OK);
    return(QUIT);
#endif	/* WINDOWS */
  }

  /* �˥塼�������ƥ� */

  if ( (gn.newslib = expand_filename(gn.newslib,gn.home)) == 0 ) {
#if ! defined(WINDOWS)
    mc_printf("�ե�����̾��Ÿ���˼��Ԥ��ޤ���(%s)\n",gn.newslib);
#else	/* WINDOWS */
    mc_MessageBox(hWnd,"�ե�����̾��Ÿ���˼��Ԥ��ޤ���",gn.newslib,MB_OK);
#endif	/* WINDOWS */
    return(QUIT);
  }
  
  if ( (gn.newsspool = expand_filename(gn.newsspool,gn.home)) == 0 ) {
#if ! defined(WINDOWS)
    mc_printf("�ե�����̾��Ÿ���˼��Ԥ��ޤ���(%s)\n",gn.newsspool);
#else	/* WINDOWS */
    mc_MessageBox(hWnd,"�ե�����̾��Ÿ���˼��Ԥ��ޤ���",gn.newsspool,MB_OK);
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

  /* ���ۥ��� */

  /*
   * �ۥ���̾
   */
#if defined(MSDOS) || defined(OS2)
  if ( gn.hostname == 0 ) {
#ifndef	WINDOWS
    mc_puts("HOST �ˡ��УäΥۥ���̾�����ꤷ�Ʋ�����\n");
    exit(1);
#else	/* WINDOWS */
    mc_MessageBox(hWnd,"HOST �ˡ��УäΥۥ���̾�����ꤷ�Ʋ�����","",MB_OK);
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
    /* hostname �� domainname ���Ĥ��Ƥ�����ϡ����� */
    if ( gethostname(host_name,MAXHOSTNAME) != 0 ) {
      mc_puts("\n�ۥ���̾�������Ǥ��ޤ���Ǥ���\n\007");
      exit(1);
    }
    if ( (gn.hostname = malloc(strlen(host_name)+1)) == NULL ) {
      memory_error();
      return(QUIT);
    }
    strcpy(gn.hostname,host_name);
#endif	/* sony news USG */
  }
  /* �ۥ���̾�� . ��������� . �ʹߤ�ä� */
  pt = strchr(gn.hostname, '.');
  if ( pt != (char *)NULL )
    *pt = 0;
#endif	/* MSDOS || OS2 */

  /* �桼�� */

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
    mc_puts("USER �ˡ��桼��̾�����ꤷ�Ʋ�����\n");
    exit(1);
#else	/* WINDOWS */
    mc_MessageBox(hWnd,"USER �ˡ��桼��̾�����ꤷ�Ʋ�����","",MB_OK);
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
    mc_puts("NAME �ˡ���̾�����ꤷ�Ʋ�����\n");
    exit(1);
#else	/* WINDOWS */
    mc_MessageBox(hWnd,"NAME �ˡ���̾�����ꤷ�Ʋ�����","",MB_OK);
    return(QUIT);
#endif	/* WINDOWS */
  }


  /* directory / file */

  /* .newsrc_NNTPSERVER */
  if ( (env = expand_filename(gn.rcfile,gn.home)) == 0 ) {
#if ! defined(WINDOWS)
    mc_printf("�ե�����̾��Ÿ���˼��Ԥ��ޤ���(%s)\n",gn.rcfile);
#else	/* WINDOWS */
    mc_MessageBox(hWnd,"�ե�����̾��Ÿ���˼��Ԥ��ޤ���",gn.rcfile,MB_OK);
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
  /* savedir ��Ÿ�� */
  if ( (pt = expand_filename(gn.savedir,gn.home)) == 0 ) {
#if ! defined(WINDOWS)
    mc_printf("�ե�����̾��Ÿ���˼��Ԥ��ޤ���(%s)\n",gn.savedir);
#else	/* WINDOWS */
    mc_MessageBox(hWnd,"�ե�����̾��Ÿ���˼��Ԥ��ޤ���",gn.savedir,MB_OK);
#endif	/* WINDOWS */
    return(QUIT);
  }
  free(gn.savedir);
  gn.savedir = pt;
  
  /* savedir �ϥǥ��쥯�ȥ�Ǥʤ���Фʤ�ʤ� */
  if ( stat(gn.savedir,&stat_buf) == 0 ) {
    if ( (stat_buf.st_mode & S_IFMT) != S_IFDIR ) {	/* S_IFLNK �� ? */
#ifndef	WINDOWS
      mc_puts("��������¸����ǥ��쥯�ȥ�Ȥ��ƥǥ��쥯�ȥ�ʳ������ꤵ��Ƥ��ޤ�\n");
      /* ���㤡���������ǥ��쥯�ȥ�����Ϥ�¥���� */
      exit(1);
#else	/* WINDOWS */
      mc_MessageBox(hWnd,"��������¸����ǥ��쥯�ȥ�Ȥ��ƥǥ��쥯�ȥ�ʳ������ꤵ��Ƥ��ޤ�","",MB_OK);
      return(QUIT);
#endif	/* WINDOWS */
    }
  }
  /* savedir �κǸ�� / �ǽ���褦�ˤ��� */
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
    mc_printf("�ե�����̾��Ÿ���˼��Ԥ��ޤ���(%s)\n",gn.signature);
#else	/* WINDOWS */
    mc_MessageBox(hWnd,"�ե�����̾��Ÿ���˼��Ԥ��ޤ���",gn.signature,MB_OK);
#endif	/* WINDOWS */
    return(QUIT);
  }

  if ( (gn.author_copy = expand_filename(gn.author_copy,gn.home)) == 0 ) {
#if ! defined(WINDOWS)
    mc_printf("�ե�����̾��Ÿ���˼��Ԥ��ޤ���(%s)\n",gn.author_copy);
#else	/* WINDOWS */
    mc_MessageBox(hWnd,"�ե�����̾��Ÿ���˼��Ԥ��ޤ���",gn.author_copy,MB_OK);
#endif	/* WINDOWS */
    return(QUIT);
  }
  
  if ( (gn.tmpdir = expand_filename(gn.tmpdir,gn.home)) == 0 ) {
#if ! defined(WINDOWS)
    mc_printf("�ե�����̾��Ÿ���˼��Ԥ��ޤ���(%s)\n",gn.tmpdir);
#else	/* WINDOWS */
    mc_MessageBox(hWnd,"�ե�����̾��Ÿ���˼��Ԥ��ޤ���",gn.tmpdir,MB_OK);
#endif	/* WINDOWS */
    return(QUIT);
  }
#ifdef	DOSFS
  while ( (pt = strchr(gn.tmpdir,'/')) != NULL )
    *pt = SLASH_CHAR;

  if ( (pt = strchr(gn.tmpdir, ':')) != NULL){
    /* tmpdir �˥ǥ��������񤫤�Ƥ��� */
    pt++;
    disk = GetDisk();
    if (SetDisk(toupper(gn.tmpdir[0])-'A')){
#ifndef	WINDOWS
      mc_puts("���ꤵ�줿�ƥ�ݥ��ǥ������ϻȤ��ޤ��� \n");
      return(QUIT);
#else	/* WINDOWS */
      mc_MessageBox(hWnd,"���ꤵ�줿�ƥ�ݥ��ǥ������ϻȤ��ޤ��� ","",MB_OK);
      return(QUIT);
#endif	/* WINDOWS */
    }
    SetDisk(disk);
  }
#endif	/* DOSFS */

  /* tmpdir �ϥǥ��쥯�ȥ�Ǥʤ���Фʤ�ʤ� */
  if (disk == -1)
    pt = gn.tmpdir;
  if (strcmp(pt, SLASH_STR) != 0 && strcmp(pt, "") !=0){
    /* tmpdir='/' or tmpdir='\0' �ϥ����å����ʤ� - ��Ȥ���MS-DOS�� */
    if ( stat(gn.tmpdir,&stat_buf) != 0 ) {
      if ( MKDIR(gn.tmpdir) != 0 ) {	/* boooom */
#ifndef	WINDOWS
	mc_printf("�ƥ�ݥ��ǥ��쥯�ȥ�Ȥ��ƻ��ꤵ��Ƥ��� %s ��������뤳�Ȥ��Ǥ��ޤ��� \n",gn.tmpdir);
	perror(gn.tmpdir);
	exit(1);
#else	/* WINDOWS */
	mc_MessageBox(hWnd,"�ƥ�ݥ��ǥ��쥯�ȥ��������뤳�Ȥ��Ǥ��ޤ��� ",
		      gn.tmpdir,MB_OK);
	return(QUIT);
#endif	/* WINDOWS */
      } else {
	mc_printf("�ƥ�ݥ��ǥ��쥯�ȥ�Ȥ��ƻ��ꤵ��Ƥ��� %s ��¸�ߤ��ʤ��ä����ᡢ�������ޤ���\n",gn.tmpdir);
      }
    } else {
      if ( (stat_buf.st_mode & S_IFMT) != S_IFDIR ) {	/* S_IFLNK �� ? */
#ifndef	WINDOWS
	mc_puts("�ƥ�ݥ��ǥ��쥯�ȥ�Ȥ��ƥǥ��쥯�ȥ�ʳ������ꤵ��Ƥ��ޤ�\n");
	/* ���㤡���������ǥ��쥯�ȥ�����Ϥ�¥���� */
	exit(1);
#else	/* WINDOWS */
	mc_MessageBox(hWnd,"�ƥ�ݥ��ǥ��쥯�ȥ�Ȥ��ƥǥ��쥯�ȥ�ʳ������ꤵ��Ƥ��ޤ�",
		      "",MB_OK);
	return(QUIT);
#endif	/* WINDOWS */
      }
    }
  }
  /* tmpdir �κǸ�� / �ǽ���褦�ˤ��� */
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
      mc_printf("�ե�����̾��Ÿ���˼��Ԥ��ޤ���(%s)\n",gn.jnames_file);
#else	/* WINDOWS */
      mc_MessageBox(hWnd,"�ե�����̾��Ÿ���˼��Ԥ��ޤ���",gn.jnames_file,MB_OK);
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
  
  /* ̵�뤹��˥塼�����롼�� */
  if ( unsubscribe_string != 0 ) {
    if ( check_regexp_group(unsubscribe_string,&gn.unsubscribe_groups) == ERROR )
      return(QUIT);
    if ( subscribe_string != 0 ) {
      free(subscribe_string);
      subscribe_string = 0;
    }
  }
  /* ���ɤ���˥塼�����롼�� */
  if ( subscribe_string != 0 ) {
    if ( check_regexp_group(subscribe_string,&gn.subscribe_groups) == ERROR )
      return(QUIT);
  }
  
  /* ���������� */
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
    /* �̾凉�ס���򥢥���������Ȥ��� gnspool_kanji_code �ʤΤǡ�
     * gn.spool_kanji_code = gn.gnspool_kanji_code ���Ƥ�������
     * �ݥ��Ȥ���Ȥ��� gn.spool_kanji_code ��ɬ�פˤʤ�Τǡ�
     * gn.gnspool_kanji_code �˥��å�������Ƥ���
     * gn.gnspool_kanji_code �� post_file_with_gnspool() �ǤΤ߻���
     */
    int kanji_code;
    kanji_code = gn.spool_kanji_code;
    gn.spool_kanji_code = gn.gnspool_kanji_code;
    gn.gnspool_kanji_code = kanji_code;
  }

  if ( prog == GNSPOOL )
    gn.mime_head = gn.gnspool_mime_head;

#ifdef UNIX
  /* ��å��ե�����Υ����å� */
  if (prog == GN && check_only == 0) {
    if ( lock_gn(gn.nntpserver) != 0 ) {	/* lock file OK ? */
      mc_puts("���Ǥ� gn ����ư����Ƥ��ޤ�\n");
      exit(1);
    }
  }
#endif
  
  
#ifdef	SIGCONT
  signal(SIGCONT, foreground);
#endif
  
#if ! defined(NO_NET)
#if defined(WINSOCK)
  if (gn.local_spool == OFF) {	/* �ͥåȥ���⡼�� */
    wVersionRequested = 0x0101;
    if ( WSAStartup(wVersionRequested, &wsaData) != 0) {
#ifndef	WINDOWS
      mc_puts("winsock ���ߤĤ���ޤ��� \n");
      exit(1);
#else	/* WINDOWS */
      mc_MessageBox(hWnd,"winsock ���ߤĤ���ޤ��� ","",MB_OK);
      return(QUIT);
#endif	/* WINDOWS */
    }
  }
#endif	/* WINSOCK */

#if defined(INETBIOS) || defined(PATHWAY) || defined(LWP)
  if (gn.local_spool == OFF) {	/* �ͥåȥ���⡼�� */
    if( find_inet() == 0 ) {
      mc_puts("BIOS�����󤷤Ƥ��ޤ��� \n");
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
  
  GetWindowRect(hWnd,&lprcw);	/* ���ߤ� Window */
  
  if ( (gn.columns * xsiz) > lprct.right ) {	/* ���� desktop ����礭�� */
    wwidth = (lprct.right/ xsiz) * xsiz;
    xpos = 0;
  } else {
    wwidth = gn.columns * xsiz;
    xpos = lprcw.left;
  }
  if ( (xpos + wwidth) > lprct.right )
    xpos = 0;
  
  if ( (gn.lines * ysiz) > lprct.bottom ) {	/* �⤵�� desktop ����礭�� */
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
  if (gn.local_spool == OFF) {	/* �ͥåȥ���⡼�� */
    if ( prog == GN || DO_POST || DO_MAIL) {
      if ( auth(gn.usrname) != 0 )	/* �桼��ǧ�� */
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
 * struct gn �ν�����ʥ���ѥ�����ǥե���Ȥ��Σ���
 */
static void
init_gn_1()
{
  /* �ɥᥤ�� */
  gn.domainname = 0;			/* �ɥᥤ��̾ */
  gn.organization = 0;			/* �ȿ�̾ */
  gn.genericfrom = DEFAULT_GENERICFROM;	/* From: �˥ۥ���̾��Ĥ��ʤ� */

  /* �˥塼�������ƥ� */
  gn.nntpserver = 0;		/* �˥塼�������� */
  gn.nntpport = 0;		/* NNTP �˻��Ѥ���ݡ��� */
  gn.spool_kanji_code = JIS;	/* NNTP �δ��������� */
  gn.mode_reader = OFF;		/* Inn �ξ��� ON �ˤ���� mode reader ���� */
  gn.nntp_auth = OFF;		/* NNTP �ˤ��桼��ǧ�� */
#if defined(NO_NET)
  gn.local_spool = ON;		/* ������ޥ���Υ��ס���򸫤� */
#else
  gn.local_spool = OFF;		/* ������ޥ���Υ��ס���򸫤� */
#endif
  gn.use_history = OFF;		/* history ����Ѥ��� */
  gn.postproc = 0;		/* �ݥ��Ȥ���ݤ˻��Ѥ���ץ���� */
  gn.newslib = 0;		/* /usr/lib/news */
  gn.newsspool = 0;		/* /usr/spool/news */
#if defined(MSDOS) || defined(OS2)
  gn.dosfs = ON;		/* FAT �� spool ��������� */
#else
  gn.dosfs = OFF;		/* FAT �� spool ��������� */
#endif

  /* �ᥤ�륷���ƥ� */
  gn.smtpserver = 0;		/* SMTP ������ */
  gn.smtpport = 0;		/* SMTP �˻��Ѥ���ݡ��� */
  gn.authserver = 0;		/* POP ������ */
  gn.pop3port = 0;		/* POP3 �˻��Ѥ���ݡ��� */
  gn.mail_kanji_code = DEFAULT_MAIL_KANJI_CODE;	/* �ᥤ��δ��������� */
  gn.mailer = 0;		/* �ᥤ�� */

  /* ���ۥ��� */
  gn.hostname = 0;		/* �ۥ���̾ */
  gn.lines = DEFAULT_LINES;	/* ���̹Կ� */
  gn.columns = DEFAULT_COLUMNS; /* ���̷�� */
  gn.win_lines = -1;
  gn.win_columns = -1;

  gn.process_kanji_code = DEFAULT_PROCESS_KANJI_CODE;	/* �ڡ����㡢���ǥ����δ��������� */
  gn.file_kanji_code = DEFAULT_FILE_KANJI_CODE;		/* �ե�����˥����֤�����δ��������� */
  gn.display_kanji_code = DEFAULT_DISPLAY_KANJI_CODE;	/* ���̤δ��������� */

#if ! defined(WINDOWS) && ! defined(QUICKWIN)
  gn.pager = 0;			/* �ڡ����� */
  gn.return_after_pager = DEFAULT_RETURN_AFTER_PAGER;	/* ����ɽ����� Return �����׵��̵ͭ */
#endif	/* ! WINDOWS && !QUICKWIN */

  gn.win_pager_flag = 0;	/* �ǽ��ڡ����ǻߤޤ� */
  gn.editor = 0;		/* ���ǥ��� */
  gn.win_editor = 0;		/* Windows �λ��Υ��ǥ��� */
#ifdef	USE_FGETS
  gn.use_space = ON;		/* ���ڡ����������ɤ߿ʤ�� */
#else
  gn.use_space = OFF;		/* ���ڡ����������ɤ߿ʤ�� */
#endif
#ifdef	POWER_SAVE
  gn.power_save = COPY_TO_TMP;	/* �ǥ������ؤΥ��������������� */
#else
  gn.power_save = OFF;		/* �ǥ������ؤΥ��������������� */
#endif

  /* �桼�� */
  gn.usrname = 0;	/* login name */
  gn.fullname = 0;	/* Full Name */
  gn.passwd = 0;	/* password. booooom! */

  /* directory / file */
  gn.rcfile = 0;		/* newsrc */
  gn.savedir = 0;		/* �����򥻡��֤���ǥ��쥯�ȥ� */
  gn.signature = 0;		/* signature file */
  gn.author_copy = 0;		/* author copy */
  gn.tmpdir = 0;		/* $TMPDIR */

  gn.jnames_file = 0;		/* jnames file for a user */

  /* newsgroup */
  gn.gnus_format = 1;		/* GNUS format �� .newsrc ���ɤ߹���� */
  gn.ng_sort = NG_SORT_NO;	/* sort new newsgroup */
  gn.unsubscribe_groups = 0;
  gn.subscribe_groups = 0;
  gn.article_limit = ARTICLE_LIMIT;	/* ��ư��̤�ɵ�������������¿�����ϡ���ǧ���� */
  gn.article_leave = ARTICLE_LEAVE;	/* �ΤƤ���˻Ĥ��� */
  gn.select_limit = SELECT_LIMIT;	/* �������������������¿�����ϡ���ǧ���� */
  gn.select_number = SELECT_NUMBER;	/* ��ǧ���ϻ��Υǥե���Ȥε����� */

  /* �ɤ� */
  gn.disp_re = ON;	/* �����ե��������λ��� Re: ��ɽ�� */
  gn.cls = OFF;		/* ���̥��ꥢ */
  gn.description = OFF;	/* �˥塼�����롼�פ�������ɽ������ */
  gn.kill_control = 0;	/* ����ȥ����å���������ɤ� */
  gn.autolist = ON;	/* �ƥ⡼�ɤǤΥꥹ��ɽ����ưŪ�˹Ԥʤ� */
  gn.show_indent = OFF;	/* �⡼����˻���������ɽ�� */
  gn.show_truncate = ON;/* �ޤ�ʤ�������ɽ������ */
  gn.mime_head = OFF;	/* �إå��� MIME �б� */
  gn.gmt2jst = OFF;	/* Date: �إå��� JST ɽ�� */

  /* post */
  gn.nsubject = OFF;	/* ���ܸ쥵�֥������Ȼ��� */
  gn.rn_like = OFF;	/* rn �饤���ʰ��ѥ������� */
  gn.cc2me = OFF;	/* ��ץ饤�᡼���ʬ�ˤ� Cc/Bcc */
  gn.substitute_header = ON;	/* �إå����դ��Ѥ��� */

  /* other */
#if defined(NO_NET)
  gn.with_gnspool = ON;		/* gnspool ���Ȥ߹�碌�ƻ��Ѥ��� */
#else
  gn.with_gnspool = OFF;	/* gnspool ���Ȥ߹�碌�ƻ��Ѥ��� */
#endif

  gn.gnspool_kanji_code = gn.spool_kanji_code;	/* gnspool ���������륹�ס���δ��������� */

  /* gnspool only */
  gn.fast_connect = OFF;	/* active ��������ʤ� */
  gn.remove_canceled = OFF;	/* ����󥻥뤵�줿������ä� */
  gn.gnspool_mime_head = OFF;	/* �إå��� MIME �б� */
  gn.gnspool_line_limit = 0;	/* ��󤻤뵭���κ���Կ� */

}
/*
 * struct gn �ν�����ʥ���ѥ�����ǥե���Ȥ��Σ���
 */
static int
init_gn_2()
{
  register char *pt;

  /* �ɥᥤ��̾ */
  if ( gn.domainname == 0 ) {
    if ( (gn.domainname = malloc(strlen(DEFAULT_DOMAINNAME)+1)) == NULL ) {
      memory_error();
      return(QUIT);
    }
    strcpy(gn.domainname,DEFAULT_DOMAINNAME);
  }

  /* �ȿ�̾ */
  if ( gn.organization == 0 ) {
    if ( (gn.organization = malloc(strlen(DEFAULT_ORGANIZATION)+1)) == NULL ) {
      memory_error();
      return(QUIT);
    }
    strcpy(gn.organization,DEFAULT_ORGANIZATION);
  }

  /* �˥塼�������� */
  if ( gn.nntpserver == 0 ) {
    if ( (gn.nntpserver = malloc(strlen(DEFAULT_NNTPSERVER)+1)) == NULL ) {
      memory_error();
      return(QUIT);
    }
    strcpy(gn.nntpserver,DEFAULT_NNTPSERVER);
  }

  /* NNTP �˻��Ѥ���ݡ��� */
  if ( (pt = strchr(gn.nntpserver,':')) != NULL ) {
    *pt = 0;
    pt++;
    gn.nntpport = atoi(pt);
  }

  /* �ݥ��Ȥ���ݤ˻��Ѥ���ץ���� */
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

  /* smtp ������ */
  if ( gn.smtpserver == 0 ) {
    if ( (gn.smtpserver = malloc(strlen(DEFAULT_SMTPSERVER)+1)) == NULL ) {
      memory_error();
      return(QUIT);
    }
    strcpy(gn.smtpserver,DEFAULT_SMTPSERVER);
  }

  /* smtp �˻��Ѥ���ݡ��� */
  if ( (pt = strchr(gn.smtpserver,':')) != NULL ) {
    *pt = 0;
    pt++;
    gn.smtpport = atoi(pt);
  }

  /* ǧ�ڥ����� */
  if ( gn.authserver == 0 ) {
    if ( (gn.authserver = malloc(strlen(gn.smtpserver)+1)) == NULL ) {
      memory_error();
      return(QUIT);
    }
    strcpy(gn.authserver,gn.smtpserver);
  }

  /* pop3 �˻��Ѥ���ݡ��� */
  if ( (pt = strchr(gn.authserver,':')) != NULL ) {
    *pt = 0;
    pt++;
    gn.pop3port = atoi(pt);
  }

  /* �ᥤ�� */
  if ( gn.mailer == 0 ) {
    if ( (gn.mailer = malloc(strlen(DEFAULT_MAILER)+1)) == NULL ) {
      memory_error();
      return(QUIT);
    }
    strcpy(gn.mailer,DEFAULT_MAILER);
  }

#if ! defined(WINDOWS) && ! defined(QUICKWIN)
  /* �ڡ����� */
  if ( gn.pager == 0 ) {
    if ( (gn.pager = malloc(strlen(DEFAULT_PAGER) + 1)) == NULL ) {
      memory_error();
      return(QUIT);
    }
    strcpy(gn.pager,DEFAULT_PAGER);
  }
#endif	/* ! WINDOWS && !QUICKWIN */

  /* ���ǥ��� */
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

  /* �����򥻡��֤���ǥ��쥯�ȥ� */
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
 * gnrc ���ɤ߹���
 */
int
read_gnrc(gnrc,env_table,gnrc_table)
register char *gnrc;	/* gnrc �Υѥ�̾ */
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
      pt++;			/* ��Ԥ���ۥ磻�ȥ��ڡ�����̵�� */
    if ( *pt == 0 )
      continue;			/* �ۥ磻�ȥ��ڡ����Τ� */
    if ( *pt == '#' )
      continue;
    while ( *pt != 0 && *pt != ' ' && *pt != '\t' )
      pt++;
    if ( *pt == 0 ) {		/* value ���ʤ� */
      mc_printf("%s �������ʹԤ�����ޤ�:%s",gnrc_tmp,resp);
      continue;
    }
    *pt++ = 0;
    while ( *pt == ' ' || *pt == '\t' )
      pt++;			/* keyword �� value �δ֤Υۥ磻�ȥ��ڡ��� */
    if ( *pt == 0 ) {		/* value ���ʤ� */
      mc_printf("%s �������ʹԤ�����ޤ�:%s",gnrc_tmp,resp);
      continue;
    }

    /* �Ķ��ѿ��ȶ��̤�����:ʸ���� */
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

    /* gnrc ��ͭ������:���� */
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
	mc_printf("%s �� %s �ˤϿ�������ꤷ�Ʋ���������%s��\n",
		  gnrc_tmp,resp,pt);
	break;
      }
    }
    if ( gnrc_tp->str != 0 )
      continue;

    mc_printf("%s �������ʥ�����ɤ�����ޤ�:%s\n",gnrc_tmp,resp);
  }
  
  fclose(fp);
  return(CONT);
}

/*
 * �Ķ��ѿ��β���
 */
int
read_env(env_table,gnrc_table)
struct env_t *env_table;
struct gnrc_t *gnrc_table;
{
  register struct env_t *env_tp;
  register struct gnrc_t *gnrc_tp;
  register char *env;
  
  /* gnrc �ȶ��̤�����:ʸ���� */
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
  
  /* gnrc ��ͭ������:���� */
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
    mc_printf("�Ķ��ѿ� %s �ˤϿ�������ꤷ�Ʋ�����\n",gnrc_tp->str);
  }
  
  return(CONT);
}




/*
 * ����ɽ����ɤ����롼�פβ���
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
      mc_printf("! �ϡ�̵�뤷�ޤ�\n",(*ig)->name);
      continue;
    }
    /* ����ɽ���Υ����å� */
    if ( check_reg(*ig) == ERROR ){
      (*ig)->len = 0;
      mc_printf("%s �ϡ�̵�뤷�ޤ�\n",(*ig)->name);
      continue;
    }
    if ( ch == 0 )
      break;

    while ( *pt == ',' || *pt == ' ' || *pt == '\t' )
      *pt++ = 0;
  }
  return(CONT);
}

/* ����ɽ���Υ����å� */
int
check_reg(ig)
regexp_group_t *ig;
{
  char *pt;
  char ch;

  /* [] �ˤ������ɽ�� */
  for ( pt = ig->name; *pt != 0; pt++ ) {
    if ( (pt = strchr(pt,'[')) == NULL ) /* ����ʾ� [ �ʤ� */
      break;

    ig->len = -1;		/* ����ɽ��ͭ�� */

    if ( strchr(pt,']') == NULL )
      return(ERROR);		/* ] ���Ĥ��Ƥ��ʤ� */

    pt++;
    if ( *pt == ']' )
      return(ERROR);		/* [] �ϥ��顼 */

    while ( *pt != ']' ) {
      if ( *pt == '[' )
	return(ERROR);		/* �Ĥ������� [ ���褿 */

      if ( pt[1] == '-' ) {
	if ( strchr(&pt[3],']') == NULL )
	  return(ERROR);	/* �Ĥ��Ƥ��ʤ� */
	if ( pt[0] > pt[2] )
	  return(ERROR);	/* ����Ǥʤ� */
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
      ig->len = -1;		/* ����ɽ��ͭ�� */
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
