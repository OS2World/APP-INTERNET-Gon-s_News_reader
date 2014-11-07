/*
 * @(#)gn.h	1.40 May.16,1998
 *
 * ������������ޤ��󡣤�������������Ū�ʳ��λ��ѡ����ۤ����¤��ߤ��ޤ���
 * Copyright (C) yamasita@omronsoft.co.jp
 */


/* define */

#if defined(QUICKWIN) || defined(MSC9)
#	ifndef MSC
#		define MSC
#	endif
#endif

#if defined(HUMAN68K) || defined(DOSV) || defined(GNUDOS) || defined(QUICKWIN) || defined(MSC) || defined(__TURBOC__) || defined(WINDOWS)
#	ifndef MSDOS
#		define MSDOS
#	endif
#endif

#if defined( OS2 )
#	ifdef MSC
#		define OS2_16BIT
#	endif
#	ifdef __EMX__
#		define OS2_32BIT
#	endif
#endif

#ifdef	NEWWATTCP
#	ifndef WATTCP
#		define WATTCP
#	endif
#endif

#ifdef	MSDOS
#	ifndef USG
#		define USG
#	endif

#	ifdef	OS
#		undef	OS
#	endif
#	ifdef	MSC9
#		define	OS	"DOS32"
#	else
#		define	OS	"DOS"
#	endif
#endif	/* MSDOS */

#ifdef HUMAN68K
#	ifdef	OS
#		undef	OS
#	endif
#	define	OS	"Human68k"
#endif

#ifdef	OS2
#	ifndef USG
#		define USG
#	endif
#	ifdef	OS
#		undef	OS
#	endif
#	define	OS	"OS/2"
#endif

#ifdef	WINDOWS
#	ifdef	OS
#		undef	OS
#	endif
#	ifdef	WIN32
#		define	OS	"Win32"
#	else
#		define	OS	"Windows"
#	endif
#endif

#ifdef	UNIX
#	ifdef	OS
#		undef	OS
#	endif
#	define	OS	"Unix"
#endif /* UNIX */

/* ������ */

#define	BEL	0x7
#define	BS	0x8
#define	TAB	0x9
#define	NL	'\n'
#define	CR	'\r'
#define	FF	0x0c
#define	ESC	0x1b
#define	DEL	0x7f
#define	SS2	0x8e

#define	SELECT	3	/* for windows */
#define	END	2
#define	QUIT	1
#define	CONT	0
#ifdef	ERROR			/* windows.h */
#	undef	ERROR
#endif
#define	ERROR	-1
#define	NEXT	-2
#define	ARTICLE_NOT_FOUND	-3
#define	FILE_WRITE_ERROR	-4
#define	CONNECTION_CLOSED	-5

#define	ON	1
#define	YES	1
#define	OFF	0
#define	NO	0
#define	NOT_ARTICLE	0L


#if defined(MSDOS) || defined(OS2)
#	define	DOSFS
#endif

#ifdef	HUMAN68K
#	define	BASENAME_LEN	18
#else
#	define	BASENAME_LEN	8
#endif

#ifdef	DOSFS
#	define	SLASH_CHAR	'\\'
#	define	SLASH_STR	"\\"
#else
#	define	SLASH_CHAR	'/'
#	define	SLASH_STR	"/"
#endif	/* DOSFS */

#if defined(MSDOS) || defined(OS2)
#	define	GNRC	"gnrc"
#else
#	define	GNRC	".gnrc"
#endif


#ifdef	MSDOS
#	define	NEWSRC_LEN	1024
#	define	NNTP_BUFSIZE	(NNTP_STRLEN*2)
#else  /* MSDOS */
#	define	NEWSRC_LEN	4096
#	define	NNTP_BUFSIZE	(NNTP_STRLEN*2)
#endif  /* MSDOS */

#define	PATH_LEN	256

#define	KEY_BUF_LEN	128
#define MAXNAME		256		/* max length of full name */
#define MAXHOSTNAME	256		/* max length of hostname */

#define	COLPOS	40	/* ���롼�ץ⡼�ɤ� description ��ɽ�����륫��� */

/* �����ꥹ�� */
typedef	struct article	article_t;
struct article {
  article_t *next;	/* ���ε��� */
  char *from;		/* �ݥ��Ȥ����� */
  char *reference;	/* References: �إå��κǸ�� Message-ID */
  char *message_id;	/* Message-ID */
  int lines;		/* �Կ� */
  long numb;		/* �����ֹ� */
  int flag;		/* 0:̤�� 1:�ɤ�� */
};

/* ���֥������ȥꥹ�� */
typedef	struct subject	subject_t;
struct	subject {
  subject_t *next;	/* ���� subject �ؤΥݥ��� */
  long numb;		/* �����ο� */
  char *name;		/* Subject */
  article_t *art;	/* �ǽ�ε��� */
  int flag;		/* THREADED/NOTREONLY */
};
#define	THREADED	1
#define	NOTREONLY	2


/* �˥塼�����롼�ץꥹ�� */
typedef	struct newsgroup	newsgroup_t;
struct newsgroup {
  newsgroup_t *next;	/* ���� newsgroup_t �ؤΥݥ��� */
  char *name;		/* newsgroup ̾ */
  char *newsrc;		/* .newsrc �ι� */
  char *desc;		/* �˥塼�����롼�פ����� */
  long first;		/* spool �ˤ���ǽ�ε������ֹ� */
  long read;		/* �ɤ���Ǹ�ε����ֹ� */
  long last;		/* spool �ˤ���Ǹ�ε������ֹ� */
  long numb;		/* ̤�ɤε����� */
  int flag;		/* UNSUBSCRIBE,ACTIVE,NOARTICLE,NOLIST */
  subject_t *subj;	/* �ǽ��subject */
};

#define	UNSUBSCRIBE	0x01	/* ���ɤʤ� */
#define	ACTIVE		0x02	/* ���롼�פ������о��¸�ߤ��� */
#define	NOARTICLE	0x04	/* ̤�ɵ����ʤ� */
#define	NOLIST		0x08	/* list �ʤ� */

extern newsgroup_t *newsrc;		/* �˥塼�����롼�ץꥹ�� */

/* ����ɽ����ɤ��˥塼�����롼�� */
typedef	struct regexp_group	regexp_group_t;
struct regexp_group {
  regexp_group_t	*next;	/* ���� regexp_group_t �ؤΥݥ��� */
  char *name;		/* newsgroup ̾ */
  int len;		/* newsgroup ̾��Ĺ�� */
  int flag;		/* ! �ξ��� 0 ������ʳ��� 1 */
};

/* �Ǹ���ɤ������ */
typedef struct last_art last_art_t;
struct last_art {
  char *tmp_file;			/* ���������Ƥ���ե����� */
  char *newsgroups;			/* �˥塼�����롼�� */
  char *from;				/* ��Ƽ� */
  char *date;				/* ����� */
  char *references;			/* References */
  char *xref;				/* Xref */
  char *subject;			/* Subject */
  long art_numb;			/* �����ֹ� */
#ifdef WINDOWS
  int lines;				/* �إå���ޤ᤿�Կ� */
#endif	/* WINDOWS */
};
extern last_art_t last_art;	/* �Ǹ���ɤ������ */

/* kill list */
typedef struct kill_list kill_list_t;
struct kill_list {
  kill_list_t	*next;	/* ���� regexp_group_t �ؤΥݥ��� */
  char *from;				/* ��Ƽ� */
  char *subject;			/* ���֥������� */
};
extern last_art_t last_art;	/* �Ǹ���ɤ������ */

struct help_t	{
  char *msg;
  char **val;
};

/* �������ޥ��� */
typedef	struct gn gn_t;
struct gn {
  /*
   * �ɥᥤ��
   */
  char *domainname;	/* �ɥᥤ��̾ */
#define	GNRC_DOMAINNAME		"DOMAINNAME"

  char *organization;	/* �ȿ�̾ */
#define	GNRC_ORGANIZATION	"ORGANIZATION"

  int genericfrom;	/* From: �˥ۥ���̾��Ĥ��ʤ� */
#define	GNRC_GENERICFROM	"GENERICFROM"

  /*
   * �˥塼�������ƥ�
   */
  char *nntpserver;	/* �˥塼�������� */
#define	GNRC_NNTPSERVER		"NNTPSERVER"

  int nntpport;		/* NNTP �˻��Ѥ���ݡ��� */
#define GNRC_NNTPPORT		"NNTPPORT"

  int spool_kanji_code;	/* NNTP �δ��������� */

  int mode_reader;	/* Inn �ξ��� ON �ˤ���� mode reader ���� */
#define	GNRC_MODE_READER	"MODE_READER"

  int nntp_auth;	/* NNTP �ˤ��桼��ǧ�� */
#define	GNRC_NNTP_AUTH		"NNTP_AUTH"

  int local_spool;	/* ������ޥ���Υ��ס���򸫤� */
#define	GNRC_LOCAL_SPOOL	"LOCAL_SPOOL"

  int use_history;	/* history ����Ѥ��� */
#define	GNRC_USE_HISTORY	"USE_HISTORY"

  char *postproc;	/* �ݥ��Ȥ���ݤ˻��Ѥ���ץ���� */
#define	GNRC_POSTPROC		"POSTPROC"

  char *newslib;	/* /usr/lib/news */
#define	GNRC_NEWSLIB		"NEWSLIB"

  char *newsspool;	/* /usr/spool/news */
#define	GNRC_NEWSSPOOL		"NEWSSPOOL"

#define	GNRC_DOS_NEWFS		"DOS_NEWFS"

  int dosfs;		/* FAT �� spool ��������� */
#define	GNRC_DOSFS		"DOSFS"


  /*
   * �ᥤ�륷���ƥ�
   */
  char *smtpserver;	/* SMTP ������ */
#define	GNRC_SMTPSERVER		"SMTPSERVER"

  int smtpport;		/* SMTP �˻��Ѥ���ݡ��� */
#define GNRC_SMTPPORT		"SMTPPORT"

  char *authserver;	/* ǧ�ڥ����� */
#define	GNRC_AUTHSERVER		"AUTHSERVER"

  int pop3port;		/* POP3 �˻��Ѥ���ݡ��� */
#define GNRC_POP3PORT		"POP3PORT"

  int mail_kanji_code;	/* �ᥤ��δ��������� */
#define	GNRC_MAIL_LANG		"MAIL_LANG"

  char *mailer;		/* �ᥤ�� */
#define	GNRC_MAILER		"MAILER"

  /*
   * ���ۥ���
   */
  char *hostname;		/* �ۥ���̾ */
#define	GNRC_HOST		"HOST"

  int lines;			/* ���̹Կ� */
#define	GNRC_LINES		"LINES"
  int win_lines;		/* ���̹Կ� */
#define	GNRC_WIN_LINES		"WIN_LINES"

  int columns;			/* ���̷�� */
#define	GNRC_COLUMNS		"COLUMNS"
  int win_columns;		/* ���̷�� */
#define	GNRC_WIN_COLUMNS	"WIN_COLUMNS"

  int process_kanji_code;	/* �ڡ����㡢���ǥ����δ��������� */
#define	GNRC_PROCESS_LANG	"PROCESS_LANG"

  int file_kanji_code;		/* �ե�����˥����֤�����δ��������� */
#define	GNRC_FILE_LANG		"FILE_LANG"

  int display_kanji_code;	/* ���̤δ��������� */
#define	GNRC_DISPLAY_LANG	"DISPLAY_LANG"

  char *pager;			/* �ڡ����� */
#define	GNRC_PAGER		"PAGER"

  int return_after_pager;	/* ����ɽ����� Return �����׵��̵ͭ */
#define	GNRC_RETURN_AFTER_PAGER	"RETURN_AFTER_PAGER"

  int win_pager_flag;		/* �ǽ��ڡ����ǻߤޤ� */
#define	GNRC_WIN_PAGER_FLAG	"WIN_PAGER_FLAG"

  char *editor;			/* ���ǥ��� */
#define	GNRC_EDITOR		"EDITOR"

  char *win_editor;		/* Windows �λ��Υ��ǥ��� */
#define	GNRC_WIN_EDITOR		"WIN_EDITOR"

  int use_space;		/* ���ڡ����������ɤ߿ʤ�� */
#define	GNRC_USE_SPACE		"USE_SPACE"

  int power_save;		/* �ǥ������ؤΥ��������������� */
#define	GNRC_POWER_SAVE		"POWER_SAVE"
#define	COPY_TO_TMP		1
#define	COPY_TO_CACHE		2

  /*
   * �桼��
   */
  char *usrname;	/* login name */
#define	GNRC_USER		"USER"
#define	GNRC_LOGNAME		"LOGNAME"

  char *fullname;	/* Full Name */
#define	GNRC_NAME		"NAME"

  char *passwd;		/* password. booooom! */
#define	GNRC_PASSWD		"PASSWD"

  /*
   * directory / file
   */
  char *home;		/* $HOME */
#define	GNRC_HOME		"HOME"

  char *rcfile;		/* newsrc */
#define	GNRC_NEWSRC		"NEWSRC"

  char *savedir;	/* �����򥻡��֤���ǥ��쥯�ȥ� */
#define	GNRC_SAVEDIR		"SAVEDIR"

  char *signature;	/* signature file */
#define	GNRC_SIGNATURE		"SIGNATURE"

  char *author_copy;	/* author copy */
#define	GNRC_AUTHOR_COPY	"AUTHOR_COPY"

  char *tmpdir;		/* �ƥ�ݥ��ǥ��쥯�ȥ� */
#define	GNRC_TMPDIR		"TMPDIR"
#define	GNRC_TMP		"TMP"
#define	GNRC_TEMP		"TEMP"

  char *jnames_file;	/* jnames file for a user */
#define	GNRC_JNAMESFILE		"JNAMESFILE"

  /*
   * �˥塼�����롼��
   */
  int gnus_format;	/* GNUS format �� .newsrc ���ɤ߹���� */
#define	GNRC_GNUS_FORMAT	"GNUS_FORMAT"

  int ng_sort;		/* sort new newsgroup */
#define	GNRC_NG_SORT		"NG_SORT"
#define	NG_SORT_NO		0
#define	NG_SORT_ALL		1
#define	NG_SORT_CATEGORY	2

  regexp_group_t *unsubscribe_groups;	/* ̵�뤹��˥塼�����롼�ץꥹ�� */
#define	GNRC_UNSUBSCRIBE	"UNSUBSCRIBE"
#define	GNRC_IGNORE		"IGNORE"

  regexp_group_t *subscribe_groups;	/* ���ɤ���˥塼�����롼�ץꥹ�� */
#define	GNRC_SUBSCRIBE		"SUBSCRIBE"

  int article_limit;	/* ��ư��̤�ɵ�������������¿�����ϡ���ǧ���� */
#define	GNRC_ARTICLE_LIMIT	"ARTICLE_LIMIT"
#define	ARTICLE_LIMIT 50

  int article_leave;	/* �ΤƤ���˻Ĥ��� */
#define	GNRC_ARTICLE_LEAVE	"ARTICLE_LEAVE"
#define	ARTICLE_LEAVE 50

  int select_limit;	/* �������������������¿�����ϡ���ǧ���� */
#define	GNRC_SELECT_LIMIT	"SELECT_LIMIT"
#define	SELECT_LIMIT 50

  int select_number;	/* ��ǧ���ϻ��Υǥե���Ȥε����� */
#define	GNRC_SELECT_NUMBER	"SELECT_NUMBER"
#define	SELECT_NUMBER 0

  /*
   * �ɤ�
   */
  int disp_re;		/* �����ե��������λ��� Re: ��ɽ�� */
#define	GNRC_DISP_RE		"DISP_RE"

  int cls;		/* ���̥��ꥢ */
#define	GNRC_CLS		"CLS"
#define	CLS_STRING	"\033[H\033[2J"

  int description;	/* �˥塼�����롼�פ�������ɽ������ */
#define	GNRC_DESCRIPTION	"DESCRIPTION"

  int kill_control;	/* ����ȥ����å���������ɤ� */
#define	GNRC_KILL_CONTROL	"KILL_CONTROL"
#define	GNRC_KILL_CANCEL	"KILL_CANCEL"	/* upper compatible */
#define	KILL_CANCEL		1	/* cancel */
#define	KILL_IHAVE		2	/* ihave */
#define	KILL_SENDME		4	/* sendme */
#define	KILL_NEWGROUP		8	/* newgroup */
#define	KILL_RMGROUP		16	/* rmgroup */
#define	KILL_CHECKGROUPS	32	/* checkgroups */
#define	KILL_SENDSYS		64	/* sendsys */
#define	KILL_VERSION		128	/* version */
#define	KILL_NOCONTROL		256	/* no control header */
#define	CONTROL_SHOW_CONTROL	512	/* control �Ǥ� Control: */
#define	JUNK_SHOW_GROUPS	1024	/* junk �Ǥ� Newsgroups: */

  int autolist;		/* �ƥ⡼�ɤǤΥꥹ��ɽ����ưŪ�˹Ԥʤ� */
#define	GNRC_AUTOLIST		"AUTOLIST"

  int show_indent;	/* �⡼����˻���������ɽ�� */
#define	GNRC_SHOW_INDENT	"SHOW_INDENT"

  int show_truncate;	/* �ޤ�ʤ�������ɽ������ */
#define	GNRC_SHOW_TRUNCATE	"SHOW_TRUNCATE"

  int mime_head;	/* �إå��� MIME �б� */
#define	GNRC_MIME_HEAD		"MIME_HEAD"
#define	MIME_DECODE	(0x02 | 0x01)	/* �ǥ����� */
#define	MIME_ENCODE	(0x04 | 0x01)	/* ���󥳡��� */

  int gmt2jst;		/* Date: �إå��� JST ɽ�� */
#define	GNRC_GMT2JST		"GMT2JST"

  /*
   * �ݥ���
   */
  int nsubject;		/* ���ܸ쥵�֥������Ȼ��� */
#define	GNRC_NSUBJECT		"NSUBJECT"

  int rn_like;		/* rn �饤���ʰ��ѥ������� */
#define	GNRC_RN_LIKE		"RN_LIKE"

  int cc2me;		/* ��ץ饤�᡼���ʬ�ˤ� Cc/Bcc */
#define	GNRC_CC2ME		"CC2ME"
#define	CCTOME	1
#define	BCCTOME	2

  int substitute_header;	/* �إå����դ��Ѥ��� */
#define	GNRC_SUBSTITUTE_HEADER	"SUBSTITUTE_HEADER"

  /*
   * ����¾
   */
  int with_gnspool;	/* gnspool ���Ȥ߹�碌�ƻ��Ѥ��� */
#define	GNRC_WITH_GNSPOOL	"WITH_GNSPOOL"

  int gnspool_kanji_code;	/* gnspool ���������륹�ס���δ��������� */
#define	GNRC_GNSPOOL_LANG	"GNSPOOL_LANG"

  /*
   * gnspool only
   */
  int fast_connect;	/* active ��������ʤ� */
#define	GNRC_FAST_CONNECT	"FAST_CONNECT"

  int remove_canceled;	/* ����󥻥뤵�줿������ä� */
#define	GNRC_REMOVE_CANCELED	"REMOVE_CANCELED"

  int gnspool_mime_head;	/* �إå��� MIME �б� */
#define	GNRC_GNSPOOL_MIME_HEAD	"GNSPOOL_MIME_HEAD"

  int gnspool_line_limit;	/* ��󤻤뵭���κ���Կ� */
#define	GNRC_GNSPOOL_LINE_LIMIT	"GNSPOOL_LINE_LIMIT"
};
extern gn_t gn;

/* �����ѿ� */

extern int prog;
#define	GN	1
#define	GNSPOOL	2
#define	GNLOOPS	3

extern char *gn_version;
extern char *gn_date;

extern void (*redraw_func)();

extern int internal_kanji_code;	/* gn �����δ��������� */
#ifdef	JIS
#	undef	JIS
#endif
#ifdef	EUC
#	undef	EUC
#endif
#ifdef	SJIS
#	undef	SJIS
#endif
#define JIS	1
#define EUC	2
#define SJIS	3

extern int change;		/* newsrc �����ե饰 */
extern int make_article_tmpfile;/* �ڡ�����˥ƥ�ݥ��ե�������Ϥ� */
extern int disp_mode;		/* ����å�ɽ�� */
extern char *ngdir;

extern int gnspooljobs;		/* BIT_EXPIRE,BIT_POST,BIT_MAIL,BIT_GET */
#define	BIT_EXPIRE	1
#define	BIT_POST	2
#define	BIT_MAIL	4
#define	BIT_GET		8

#define	DO_EXPIRE	(gnspooljobs & BIT_EXPIRE)
#define	DO_POST		(gnspooljobs & BIT_POST)
#define	DO_MAIL		(gnspooljobs & BIT_MAIL)
#define	DO_GET		(gnspooljobs & BIT_GET)

extern int init_smtp_server;

#ifdef	WINDOWS
extern int xsiz, ysiz, wwidth, wheight, xpos, ypos;
extern char *display_buf;

extern HANDLE hInst;		/* ���ߤΥ��󥹥��� */
extern HWND hWnd;		/* �ᥤ�󥦥���ɥ��Υϥ�ɥ� */
extern HCURSOR hHourGlass;	/* �����ץ������� */

typedef	struct gnpopupmenu	gnpopupmenu_t;
struct gnpopupmenu {
  LPCSTR item;		/* ��˥塼���� */
  UINT idm;		/* IDM_xxx */
  int menu_flag;	/* AFTER_READ,SEPARATOR_NEXT */
};
typedef	struct gnmenu	gnmenu_t;
struct gnmenu {
  LPCSTR title;		/* ��˥塼���� */
  gnpopupmenu_t *gnpopup;
};
#define	AFTER_READ	1	/* �Ǹ���ɤ����������������� */
#define	SEPARATOR_NEXT	2	/* ���˥��ѥ졼�����֤� */

/* gn,gnspool */
#define	WM_START		(WM_USER+1)		/* ������� */
#define	WM_GETPASS		(WM_START+1)		/* �ѥ�������� */
#define	WM_CHECKPASS		(WM_GETPASS+1)		/* �ѥ���ɾȹ� */
#define	WM_OPENSERVER		(WM_CHECKPASS+1)	/* �˥塼�������ФȤ���³ */
#define	WM_GETACTIVE		(WM_OPENSERVER+1)	/* �����ƥ��֥ե�����μ��� */
#define	WM_READNEWSRC		(WM_GETACTIVE+1)	/* newsrc �ե�������ɤ߹��� */
#define	WM_CHECKGROUP		(WM_READNEWSRC+1)	/* �ΣǤΥ����å� */
#define	WM_GETNEWSGROUPS	(WM_CHECKGROUP+1)	/* �ƥ��롼�פ������μ��� */

#define	WM_DONE			(WM_GETNEWSGROUPS+1)	/* ��λ */
#define	WM_CONNECTIONCLOSED	(WM_DONE+1)		/* ��³���ڤ줿 */
#define	WM_CANREAD		(WM_CONNECTIONCLOSED+1)	/* �����ϣ� */

/* gn */
#define	WM_GN			(WM_USER+20)
#define	WM_GROUPMODE		(WM_GN+1)		/* ���롼�ץ⡼�� */
#define	WM_GROUPCMD		(WM_GROUPMODE+1)	/* ���롼�ץ⡼�ɥ��ޥ�� */

#define	WM_ENTERSUBJECTMODE	(WM_GROUPCMD+1)		/* ���֥������ȥ⡼�� */
#define	WM_SUBJECTMODE		(WM_ENTERSUBJECTMODE+1)	/* ���֥������ȥ⡼�� */
#define	WM_SUBJECTCMD		(WM_SUBJECTMODE+1)	/* ���֥������ȥ⡼�ɥ��ޥ�� */
#define	WM_SUBJECTALL		(WM_SUBJECTCMD+1)	/* ������ */

#define	WM_ENTERARTICLEMODE	(WM_SUBJECTALL+1)	/* �����ƥ�����⡼�� */
#define	WM_ARTICLEMODE		(WM_ENTERARTICLEMODE+1)	/* �����ƥ�����⡼�� */
#define	WM_ARTICLECMD		(WM_ARTICLEMODE+1)	/* �����ƥ�����⡼�ɥ��ޥ�� */
#define	WM_ARTICLEALL		(WM_ARTICLECMD+1)	/* ������ */

#define	WM_FOLLOWCONF		(WM_ARTICLEALL+1)	/* �ե��� */
#define	WM_POSTCONF		(WM_FOLLOWCONF+1)	/* �ݥ��� */
#define	WM_REPLYCONF		(WM_POSTCONF+1)		/* ��ץ饤 */
#define	WM_MAILCONF		(WM_REPLYCONF+1)	/* �ᥤ�� */

#define	WM_PAGER		(WM_MAILCONF+1)		/* pager */
#define	WM_PAGER_END		(WM_PAGER+1)		/* pager */
#define	WM_SAVENEWSRC		(WM_PAGER_END+1)	/* newsrc ��¸ */

/* gnspool */
#define	WM_GNSPOOL		(WM_USER+40)
#define	WM_POST			(WM_GNSPOOL+1)		/* ��������� */
#define	WM_POST_1		(WM_POST+1)		/* ��������� */
#define	WM_REPLY		(WM_POST_1+1)		/* �ᥤ������� */
#define	WM_REPLY_1		(WM_REPLY+1)		/* �ᥤ������� */
#define	WM_EXPIRE		(WM_REPLY_1+1)		/* ���ɤε����ξõ� */
#define	WM_EXPIRE_1		(WM_EXPIRE+1)
#define	WM_MKNEWSGROUPS		(WM_EXPIRE_1+1)		/* newsgroups �κ��� */
#define	WM_GETARTICLE		(WM_MKNEWSGROUPS+1)	/* �����μ��� */
#define	WM_GETARTICLE_1		(WM_GETARTICLE+1)
#define	WM_MKACTIVE		(WM_GETARTICLE_1+1)	/* active �κ��� */
#define	WM_MKHISTORY		(WM_MKACTIVE+1)		/* history �κ��� */
#define	WM_MKHISTORY_1		(WM_MKHISTORY+1)

#endif /* WINDOWS */

/* �ޥ������ */

/* �˥塼�����ס���δ��������ɤ���ץ������������ɤؤ��Ѵ� */
#define kanji_fromspool(sp,dp)	kanji_convert(gn.spool_kanji_code,sp,gn.process_kanji_code,dp)
/* �ץ������������ɤ���ե�������������ɤؤ��Ѵ� */
#define kanji_tofile(sp,dp)	kanji_convert(gn.process_kanji_code,sp,gn.file_kanji_code,dp)
/* �ե�������������ɤ���ץ������������ɤؤ��Ѵ� */
#define kanji_fromfile(sp,dp)	kanji_convert(gn.file_kanji_code,sp,gn.process_kanji_code,dp)
/* �˥塼�����ס���δ��������ɤ��� �������������ɤؤ��Ѵ� */
#define kanji_spooltoint(sp,dp)	kanji_convert(gn.spool_kanji_code,sp,internal_kanji_code,dp)

#if ! defined(WINDOWS) && ! defined(QUICKWIN)
#define	FGETS(x,y,z) (( gn.use_space == ON ) ? Fgets(x,y,z) : fgets(x,y,z))
#else
#define	FGETS(x,y,z) fgets(x,y,z)
#endif	/* ! WINDOWS && !QUICKWIN */

#if defined(NO_NET)
#	define	PUT_SERVER(buf)
#else
#	if	defined(INETBIOS) || defined(PATHWAY) || defined(SLIMTCP) || defined(PCTCP4) || defined(LWP) || defined(WATTCP) || defined(OS2) || defined(WINSOCK) || defined(ESPX)
#		define	USE_PUT_SERVER
#		define	PUT_SERVER(buf)		if ( put_server(buf) ) return(CONNECTION_CLOSED);
#	else
#		ifdef	USE_PUT_SERVER
#			undef	USE_PUT_SERVER
#		endif
#		define	PUT_SERVER(buf)		fprintf(ser_wr_fp, "%s\r\n",buf)
#	endif
#endif

#ifndef CONFIGUR
#if	defined(INETBIOS) || defined(PATHWAY) || defined(SLIMTCP) || defined(PCTCP4) || defined (LWP) || defined(OS2) || defined(WINSOCK) || defined(ESPX)
#		define	SOCKTYPE_INT
typedef	int	socket_t;
#else
#	if defined(WATTCP)
typedef	tcp_Socket	socket_t;
#	else
#		define	SOCKTYPE_FILE
typedef	FILE * socket_t;
#	endif /* WATTCP */
#endif
#endif /* CONFIGUR */

#if defined(MSDOS) && ! defined(HUMAN68K) && ! defined(GNUDOS)
#	define	MKDIR(dir)	mkdir(dir)
#else
#	define	MKDIR(dir)	mkdir(dir,0775)
#endif

#ifdef	WINDOWS
#	define	EFFECT_LINES	(gn.lines-2)
#else
#	define	EFFECT_LINES	(gn.lines-4)
#endif

#ifdef	MSC9
#define	__export
#endif
