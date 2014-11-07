/*
 * @(#)gn.h	1.40 May.16,1998
 *
 * 著作権は放棄しません。ただし、営利目的以外の使用／配布に制限は設けません。
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

/* 定数定義 */

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

#define	COLPOS	40	/* グループモードで description を表示するカラム */

/* 記事リスト */
typedef	struct article	article_t;
struct article {
  article_t *next;	/* 次の記事 */
  char *from;		/* ポストした人 */
  char *reference;	/* References: ヘッダの最後の Message-ID */
  char *message_id;	/* Message-ID */
  int lines;		/* 行数 */
  long numb;		/* 記事番号 */
  int flag;		/* 0:未読 1:読んだ */
};

/* サブジェクトリスト */
typedef	struct subject	subject_t;
struct	subject {
  subject_t *next;	/* 次の subject へのポインタ */
  long numb;		/* 記事の数 */
  char *name;		/* Subject */
  article_t *art;	/* 最初の記事 */
  int flag;		/* THREADED/NOTREONLY */
};
#define	THREADED	1
#define	NOTREONLY	2


/* ニュースグループリスト */
typedef	struct newsgroup	newsgroup_t;
struct newsgroup {
  newsgroup_t *next;	/* 次の newsgroup_t へのポインタ */
  char *name;		/* newsgroup 名 */
  char *newsrc;		/* .newsrc の行 */
  char *desc;		/* ニュースグループの説明 */
  long first;		/* spool にある最初の記事の番号 */
  long read;		/* 読んだ最後の記事番号 */
  long last;		/* spool にある最後の記事の番号 */
  long numb;		/* 未読の記事数 */
  int flag;		/* UNSUBSCRIBE,ACTIVE,NOARTICLE,NOLIST */
  subject_t *subj;	/* 最初のsubject */
};

#define	UNSUBSCRIBE	0x01	/* 購読なし */
#define	ACTIVE		0x02	/* グループがサーバ上に存在する */
#define	NOARTICLE	0x04	/* 未読記事なし */
#define	NOLIST		0x08	/* list なし */

extern newsgroup_t *newsrc;		/* ニュースグループリスト */

/* 正規表現もどきニュースグループ */
typedef	struct regexp_group	regexp_group_t;
struct regexp_group {
  regexp_group_t	*next;	/* 次の regexp_group_t へのポインタ */
  char *name;		/* newsgroup 名 */
  int len;		/* newsgroup 名の長さ */
  int flag;		/* ! の場合は 0 ／それ以外は 1 */
};

/* 最後に読んだ記事 */
typedef struct last_art last_art_t;
struct last_art {
  char *tmp_file;			/* 記事を収めているファイル */
  char *newsgroups;			/* ニュースグループ */
  char *from;				/* 投稿者 */
  char *date;				/* 投稿日 */
  char *references;			/* References */
  char *xref;				/* Xref */
  char *subject;			/* Subject */
  long art_numb;			/* 記事番号 */
#ifdef WINDOWS
  int lines;				/* ヘッダも含めた行数 */
#endif	/* WINDOWS */
};
extern last_art_t last_art;	/* 最後に読んだ記事 */

/* kill list */
typedef struct kill_list kill_list_t;
struct kill_list {
  kill_list_t	*next;	/* 次の regexp_group_t へのポインタ */
  char *from;				/* 投稿者 */
  char *subject;			/* サブジェクト */
};
extern last_art_t last_art;	/* 最後に読んだ記事 */

struct help_t	{
  char *msg;
  char **val;
};

/* カスタマイズ */
typedef	struct gn gn_t;
struct gn {
  /*
   * ドメイン
   */
  char *domainname;	/* ドメイン名 */
#define	GNRC_DOMAINNAME		"DOMAINNAME"

  char *organization;	/* 組織名 */
#define	GNRC_ORGANIZATION	"ORGANIZATION"

  int genericfrom;	/* From: にホスト名をつけない */
#define	GNRC_GENERICFROM	"GENERICFROM"

  /*
   * ニュースシステム
   */
  char *nntpserver;	/* ニュースサーバ */
#define	GNRC_NNTPSERVER		"NNTPSERVER"

  int nntpport;		/* NNTP に使用するポート */
#define GNRC_NNTPPORT		"NNTPPORT"

  int spool_kanji_code;	/* NNTP の漢字コード */

  int mode_reader;	/* Inn の場合は ON にすると mode reader 送信 */
#define	GNRC_MODE_READER	"MODE_READER"

  int nntp_auth;	/* NNTP によるユーザ認証 */
#define	GNRC_NNTP_AUTH		"NNTP_AUTH"

  int local_spool;	/* ローカルマシンのスプールを見る */
#define	GNRC_LOCAL_SPOOL	"LOCAL_SPOOL"

  int use_history;	/* history を使用する */
#define	GNRC_USE_HISTORY	"USE_HISTORY"

  char *postproc;	/* ポストする際に使用するプログラム */
#define	GNRC_POSTPROC		"POSTPROC"

  char *newslib;	/* /usr/lib/news */
#define	GNRC_NEWSLIB		"NEWSLIB"

  char *newsspool;	/* /usr/spool/news */
#define	GNRC_NEWSSPOOL		"NEWSSPOOL"

#define	GNRC_DOS_NEWFS		"DOS_NEWFS"

  int dosfs;		/* FAT に spool を作成する */
#define	GNRC_DOSFS		"DOSFS"


  /*
   * メイルシステム
   */
  char *smtpserver;	/* SMTP サーバ */
#define	GNRC_SMTPSERVER		"SMTPSERVER"

  int smtpport;		/* SMTP に使用するポート */
#define GNRC_SMTPPORT		"SMTPPORT"

  char *authserver;	/* 認証サーバ */
#define	GNRC_AUTHSERVER		"AUTHSERVER"

  int pop3port;		/* POP3 に使用するポート */
#define GNRC_POP3PORT		"POP3PORT"

  int mail_kanji_code;	/* メイルの漢字コード */
#define	GNRC_MAIL_LANG		"MAIL_LANG"

  char *mailer;		/* メイラ */
#define	GNRC_MAILER		"MAILER"

  /*
   * 自ホスト
   */
  char *hostname;		/* ホスト名 */
#define	GNRC_HOST		"HOST"

  int lines;			/* 画面行数 */
#define	GNRC_LINES		"LINES"
  int win_lines;		/* 画面行数 */
#define	GNRC_WIN_LINES		"WIN_LINES"

  int columns;			/* 画面桁数 */
#define	GNRC_COLUMNS		"COLUMNS"
  int win_columns;		/* 画面桁数 */
#define	GNRC_WIN_COLUMNS	"WIN_COLUMNS"

  int process_kanji_code;	/* ページャ、エディタの漢字コード */
#define	GNRC_PROCESS_LANG	"PROCESS_LANG"

  int file_kanji_code;		/* ファイルにセーブする時の漢字コード */
#define	GNRC_FILE_LANG		"FILE_LANG"

  int display_kanji_code;	/* 画面の漢字コード */
#define	GNRC_DISPLAY_LANG	"DISPLAY_LANG"

  char *pager;			/* ページャ */
#define	GNRC_PAGER		"PAGER"

  int return_after_pager;	/* 記事表示後の Return 入力要求の有無 */
#define	GNRC_RETURN_AFTER_PAGER	"RETURN_AFTER_PAGER"

  int win_pager_flag;		/* 最終ページで止まる */
#define	GNRC_WIN_PAGER_FLAG	"WIN_PAGER_FLAG"

  char *editor;			/* エディタ */
#define	GNRC_EDITOR		"EDITOR"

  char *win_editor;		/* Windows の時のエディタ */
#define	GNRC_WIN_EDITOR		"WIN_EDITOR"

  int use_space;		/* スペースキーで読み進める */
#define	GNRC_USE_SPACE		"USE_SPACE"

  int power_save;		/* ディスクへのアクセスを一時期に */
#define	GNRC_POWER_SAVE		"POWER_SAVE"
#define	COPY_TO_TMP		1
#define	COPY_TO_CACHE		2

  /*
   * ユーザ
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

  char *savedir;	/* 記事をセーブするディレクトリ */
#define	GNRC_SAVEDIR		"SAVEDIR"

  char *signature;	/* signature file */
#define	GNRC_SIGNATURE		"SIGNATURE"

  char *author_copy;	/* author copy */
#define	GNRC_AUTHOR_COPY	"AUTHOR_COPY"

  char *tmpdir;		/* テンポラリディレクトリ */
#define	GNRC_TMPDIR		"TMPDIR"
#define	GNRC_TMP		"TMP"
#define	GNRC_TEMP		"TEMP"

  char *jnames_file;	/* jnames file for a user */
#define	GNRC_JNAMESFILE		"JNAMESFILE"

  /*
   * ニュースグループ
   */
  int gnus_format;	/* GNUS format の .newsrc を読み込んだ */
#define	GNRC_GNUS_FORMAT	"GNUS_FORMAT"

  int ng_sort;		/* sort new newsgroup */
#define	GNRC_NG_SORT		"NG_SORT"
#define	NG_SORT_NO		0
#define	NG_SORT_ALL		1
#define	NG_SORT_CATEGORY	2

  regexp_group_t *unsubscribe_groups;	/* 無視するニュースグループリスト */
#define	GNRC_UNSUBSCRIBE	"UNSUBSCRIBE"
#define	GNRC_IGNORE		"IGNORE"

  regexp_group_t *subscribe_groups;	/* 購読するニュースグループリスト */
#define	GNRC_SUBSCRIBE		"SUBSCRIBE"

  int article_limit;	/* 起動時未読記事数がこれより多い時は、確認入力 */
#define	GNRC_ARTICLE_LIMIT	"ARTICLE_LIMIT"
#define	ARTICLE_LIMIT 50

  int article_leave;	/* 捨てる場合に残す数 */
#define	GNRC_ARTICLE_LEAVE	"ARTICLE_LEAVE"
#define	ARTICLE_LEAVE 50

  int select_limit;	/* 選択時記事数がこれより多い時は、確認入力 */
#define	GNRC_SELECT_LIMIT	"SELECT_LIMIT"
#define	SELECT_LIMIT 50

  int select_number;	/* 確認入力時のデフォルトの記事数 */
#define	GNRC_SELECT_NUMBER	"SELECT_NUMBER"
#define	SELECT_NUMBER 0

  /*
   * 読む
   */
  int disp_re;		/* 全部フォロー記事の時は Re: を表示 */
#define	GNRC_DISP_RE		"DISP_RE"

  int cls;		/* 画面クリア */
#define	GNRC_CLS		"CLS"
#define	CLS_STRING	"\033[H\033[2J"

  int description;	/* ニュースグループの説明を表示する */
#define	GNRC_DESCRIPTION	"DESCRIPTION"

  int kill_control;	/* コントロールメッセージを既読に */
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
#define	CONTROL_SHOW_CONTROL	512	/* control では Control: */
#define	JUNK_SHOW_GROUPS	1024	/* junk では Newsgroups: */

  int autolist;		/* 各モードでのリスト表示を自動的に行なう */
#define	GNRC_AUTOLIST		"AUTOLIST"

  int show_indent;	/* モード毎に字下げして表示 */
#define	GNRC_SHOW_INDENT	"SHOW_INDENT"

  int show_truncate;	/* 折り曲げて全部表示する */
#define	GNRC_SHOW_TRUNCATE	"SHOW_TRUNCATE"

  int mime_head;	/* ヘッダの MIME 対応 */
#define	GNRC_MIME_HEAD		"MIME_HEAD"
#define	MIME_DECODE	(0x02 | 0x01)	/* デコード */
#define	MIME_ENCODE	(0x04 | 0x01)	/* エンコード */

  int gmt2jst;		/* Date: ヘッダの JST 表示 */
#define	GNRC_GMT2JST		"GMT2JST"

  /*
   * ポスト
   */
  int nsubject;		/* 日本語サブジェクト使用 */
#define	GNRC_NSUBJECT		"NSUBJECT"

  int rn_like;		/* rn ライクな引用タグ生成 */
#define	GNRC_RN_LIKE		"RN_LIKE"

  int cc2me;		/* リプライメールを自分にも Cc/Bcc */
#define	GNRC_CC2ME		"CC2ME"
#define	CCTOME	1
#define	BCCTOME	2

  int substitute_header;	/* ヘッダを付け変える */
#define	GNRC_SUBSTITUTE_HEADER	"SUBSTITUTE_HEADER"

  /*
   * その他
   */
  int with_gnspool;	/* gnspool と組み合わせて使用する */
#define	GNRC_WITH_GNSPOOL	"WITH_GNSPOOL"

  int gnspool_kanji_code;	/* gnspool が作成するスプールの漢字コード */
#define	GNRC_GNSPOOL_LANG	"GNSPOOL_LANG"

  /*
   * gnspool only
   */
  int fast_connect;	/* active を取得しない */
#define	GNRC_FAST_CONNECT	"FAST_CONNECT"

  int remove_canceled;	/* キャンセルされた記事を消す */
#define	GNRC_REMOVE_CANCELED	"REMOVE_CANCELED"

  int gnspool_mime_head;	/* ヘッダの MIME 対応 */
#define	GNRC_GNSPOOL_MIME_HEAD	"GNSPOOL_MIME_HEAD"

  int gnspool_line_limit;	/* 取寄せる記事の最大行数 */
#define	GNRC_GNSPOOL_LINE_LIMIT	"GNSPOOL_LINE_LIMIT"
};
extern gn_t gn;

/* 外部変数 */

extern int prog;
#define	GN	1
#define	GNSPOOL	2
#define	GNLOOPS	3

extern char *gn_version;
extern char *gn_date;

extern void (*redraw_func)();

extern int internal_kanji_code;	/* gn 内部の漢字コード */
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

extern int change;		/* newsrc 更新フラグ */
extern int make_article_tmpfile;/* ページャにテンポラリファイルを渡す */
extern int disp_mode;		/* スレッド表示 */
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

extern HANDLE hInst;		/* 現在のインスタンス */
extern HWND hWnd;		/* メインウィンドウのハンドル */
extern HCURSOR hHourGlass;	/* 砂時計カーソル */

typedef	struct gnpopupmenu	gnpopupmenu_t;
struct gnpopupmenu {
  LPCSTR item;		/* メニュー項目 */
  UINT idm;		/* IDM_xxx */
  int menu_flag;	/* AFTER_READ,SEPARATOR_NEXT */
};
typedef	struct gnmenu	gnmenu_t;
struct gnmenu {
  LPCSTR title;		/* メニュー項目 */
  gnpopupmenu_t *gnpopup;
};
#define	AFTER_READ	1	/* 最後に読んだ記事がある時だけ */
#define	SEPARATOR_NEXT	2	/* 次にセパレータを置く */

/* gn,gnspool */
#define	WM_START		(WM_USER+1)		/* 初期処理 */
#define	WM_GETPASS		(WM_START+1)		/* パスワード入力 */
#define	WM_CHECKPASS		(WM_GETPASS+1)		/* パスワード照合 */
#define	WM_OPENSERVER		(WM_CHECKPASS+1)	/* ニュースサーバとの接続 */
#define	WM_GETACTIVE		(WM_OPENSERVER+1)	/* アクティブファイルの取得 */
#define	WM_READNEWSRC		(WM_GETACTIVE+1)	/* newsrc ファイルの読み込み */
#define	WM_CHECKGROUP		(WM_READNEWSRC+1)	/* ＮＧのチェック */
#define	WM_GETNEWSGROUPS	(WM_CHECKGROUP+1)	/* 各グループの説明の取得 */

#define	WM_DONE			(WM_GETNEWSGROUPS+1)	/* 終了 */
#define	WM_CONNECTIONCLOSED	(WM_DONE+1)		/* 接続が切れた */
#define	WM_CANREAD		(WM_CONNECTIONCLOSED+1)	/* 受信ＯＫ */

/* gn */
#define	WM_GN			(WM_USER+20)
#define	WM_GROUPMODE		(WM_GN+1)		/* グループモード */
#define	WM_GROUPCMD		(WM_GROUPMODE+1)	/* グループモードコマンド */

#define	WM_ENTERSUBJECTMODE	(WM_GROUPCMD+1)		/* サブジェクトモード */
#define	WM_SUBJECTMODE		(WM_ENTERSUBJECTMODE+1)	/* サブジェクトモード */
#define	WM_SUBJECTCMD		(WM_SUBJECTMODE+1)	/* サブジェクトモードコマンド */
#define	WM_SUBJECTALL		(WM_SUBJECTCMD+1)	/* 全記事 */

#define	WM_ENTERARTICLEMODE	(WM_SUBJECTALL+1)	/* アーティクルモード */
#define	WM_ARTICLEMODE		(WM_ENTERARTICLEMODE+1)	/* アーティクルモード */
#define	WM_ARTICLECMD		(WM_ARTICLEMODE+1)	/* アーティクルモードコマンド */
#define	WM_ARTICLEALL		(WM_ARTICLECMD+1)	/* 全記事 */

#define	WM_FOLLOWCONF		(WM_ARTICLEALL+1)	/* フォロー */
#define	WM_POSTCONF		(WM_FOLLOWCONF+1)	/* ポスト */
#define	WM_REPLYCONF		(WM_POSTCONF+1)		/* リプライ */
#define	WM_MAILCONF		(WM_REPLYCONF+1)	/* メイル */

#define	WM_PAGER		(WM_MAILCONF+1)		/* pager */
#define	WM_PAGER_END		(WM_PAGER+1)		/* pager */
#define	WM_SAVENEWSRC		(WM_PAGER_END+1)	/* newsrc 保存 */

/* gnspool */
#define	WM_GNSPOOL		(WM_USER+40)
#define	WM_POST			(WM_GNSPOOL+1)		/* 記事の投稿 */
#define	WM_POST_1		(WM_POST+1)		/* 記事の投稿 */
#define	WM_REPLY		(WM_POST_1+1)		/* メイルの送信 */
#define	WM_REPLY_1		(WM_REPLY+1)		/* メイルの送信 */
#define	WM_EXPIRE		(WM_REPLY_1+1)		/* 既読の記事の消去 */
#define	WM_EXPIRE_1		(WM_EXPIRE+1)
#define	WM_MKNEWSGROUPS		(WM_EXPIRE_1+1)		/* newsgroups の作成 */
#define	WM_GETARTICLE		(WM_MKNEWSGROUPS+1)	/* 記事の取得 */
#define	WM_GETARTICLE_1		(WM_GETARTICLE+1)
#define	WM_MKACTIVE		(WM_GETARTICLE_1+1)	/* active の作成 */
#define	WM_MKHISTORY		(WM_MKACTIVE+1)		/* history の作成 */
#define	WM_MKHISTORY_1		(WM_MKHISTORY+1)

#endif /* WINDOWS */

/* マクロ定義 */

/* ニューススプールの漢字コードからプロセス漢字コードへの変換 */
#define kanji_fromspool(sp,dp)	kanji_convert(gn.spool_kanji_code,sp,gn.process_kanji_code,dp)
/* プロセス漢字コードからファイル漢字コードへの変換 */
#define kanji_tofile(sp,dp)	kanji_convert(gn.process_kanji_code,sp,gn.file_kanji_code,dp)
/* ファイル漢字コードからプロセス漢字コードへの変換 */
#define kanji_fromfile(sp,dp)	kanji_convert(gn.file_kanji_code,sp,gn.process_kanji_code,dp)
/* ニューススプールの漢字コードから 内部漢字コードへの変換 */
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
