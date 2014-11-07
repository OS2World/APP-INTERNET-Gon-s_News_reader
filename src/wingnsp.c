/*
 * @(#)wingnsp.c	1.40 Sep.11,1997
 *
 * 著作権は放棄しません。ただし、営利目的以外の使用／配布に制限は設けません。
 * Copyright (C) yamasita@omronsoft.co.jp
 */

#include	<windows.h>
#ifndef	WIN32
#include	<toolhelp.h>
#endif
#include	"wingnsp.h"

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>

#include	<dos.h>
#include	<stdlib.h>
#include	<string.h>
#include	<direct.h>
#ifdef WINSOCK
#	include <winsock.h>
#endif

#include	"nntp.h"
#include	"gn.h"

#define	READED	1
#define	EXISTS	2

extern void mc_puts(),mc_printf();
extern int post_file();
extern int mail_file();
extern void smtp_close_server();
extern int get_1_header_file();
extern void memory_error();

extern int gn_init();
extern int get_newsrc();
extern void save_newsrc();
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
#endif

static int check_post();
static int check_reply();
static int post();
static int reply();
extern int expire();
extern int expire_1ng();
extern int make_newsgroups();
extern int make_history();
extern int make_history_1ng();
extern int get_article();
extern int get_article_1ng();
extern int make_active();
static int post_article();
static int post_article_print_head();
static int send_mail();
static int send_mail_print_head();

extern char *find_first_file(),*find_next_file();

extern int mc_MessageBox();
extern void set_hourglass(),reset_cursor();
static HTASK exeTask;
#ifndef	WIN32
static LPFNNOTIFYCALLBACK NprocInst;
#endif

extern int newsrc_flag;		/* newsrc を読み込んだら０、なかったら−１ */

extern int interactive_send;	/* interactive */
extern int update_newsrc;
extern int cnews;

int PASCAL
WinMain(hInstance, hPrevInstance, lpCmdLine, nCmdShow)
HANDLE hInstance;
HANDLE hPrevInstance;
LPSTR lpCmdLine;
int nCmdShow;
{
  extern BOOL InitApplication(),InitInstance();
  extern void	create_arg();
  
  MSG msg;
  
  if (!hPrevInstance) {
    if (!InitApplication(hInstance))
      return(FALSE);
  }
  
  if (!InitInstance(hInstance, nCmdShow))
    return(FALSE);
  
  /* argc/argv の作成 */
  create_arg(lpCmdLine);
  
#ifdef WIN32	/* A.Sonokawa 1997/06/04 */
  while (GetMessage(&msg, NULL, 0, 0) )
#else
  while (GetMessage(&msg, NULL, NULL, NULL) )
#endif
    {
    TranslateMessage(&msg);
    DispatchMessage(&msg);
  }
  
#if ! defined(NO_NET)
  if ( DO_POST || DO_GET ) {
    /* サーバとの接続を切る */
    close_server();
  }
#if defined(WINSOCK)
  if ( DO_POST || DO_MAIL || DO_GET )
    WSACleanup();
#endif
#endif	/* NO_NET */
  return(msg.wParam);
}

#ifdef WIN32
WNDPROC CALLBACK
#else
long CALLBACK __export
#endif
MainWndProc(hWnd, message, wParam, lParam)
HWND hWnd;
UINT message;
WPARAM wParam;
LPARAM lParam;
{
  BOOL __export CALLBACK About(HWND, unsigned, WORD, LONG);
#ifdef AUTHENTICATION
  extern void auth_set_passwd();
  BOOL __export CALLBACK get_passwd(HWND, unsigned, WORD, LONG);
  extern int auth_check_pass();
#endif /* AUTHENTICATION */
  BOOL __export CALLBACK Done(HWND, unsigned, WORD, LONG);
  
  newsgroup_t *ng;
  register int y;
  HDC hdc;
  TEXTMETRIC tm;
  PAINTSTRUCT	ps;
  
  switch (message) {
  case WM_CREATE:
    hdc = GetDC(hWnd);
    SelectObject(hdc, GetStockObject(SYSTEM_FIXED_FONT));
    GetTextMetrics(hdc, &tm);
    xsiz = tm.tmAveCharWidth;
    ysiz = tm.tmHeight;
    ReleaseDC(hWnd, hdc);
    return(NULL);
    
  case WM_COMMAND:			/* メニュー */
    switch (wParam) {
    case IDM_ABOUT:
      DialogBox(hInst,
		"ABOUTBOX",
		hWnd,
		About);
      return(NULL);
    }
    return(DefWindowProc(hWnd, message, wParam, lParam));
    
  case WM_SETFOCUS:
    CreateCaret(hWnd, NULL, xsiz, ysiz);
    SetCaretPos(xpos * xsiz, ypos * ysiz);
    ShowCaret(hWnd);
    return(NULL);
    
  case WM_KILLFOCUS:
    HideCaret(hWnd);
    DestroyCaret();
    return(NULL);
    
  case WM_DESTROY:             /* ウィンドウを破棄  */
    PostQuitMessage(0);
    break;
    
  case WM_SIZE:
    if ( wParam == SIZE_MINIMIZED )
      return(NULL);
    wwidth = LOWORD(lParam);
    wheight = HIWORD(lParam);
    if (gn.columns == max(1,wwidth / xsiz) && gn.lines == max(1,wheight / ysiz))
      return(NULL);
    gn.columns = max(1, wwidth / xsiz);
    gn.lines = max(1, wheight / ysiz);
    if ( display_buf != 0 )
      free(display_buf);
    if ( (display_buf = malloc(gn.lines*gn.columns)) == NULL ) {
      memory_error();
      return(NULL);
    }
    memset(display_buf,' ',gn.lines*gn.columns);
    xpos = ypos = 0;
    return(NULL);
    
  case WM_PAINT:
    if ( display_buf == 0 )
      return(NULL);
    hdc = BeginPaint(hWnd, &ps);
    SelectObject(hdc, GetStockObject(SYSTEM_FIXED_FONT));
    for (y = 0; y < gn.lines; y++)
      TextOut(hdc, 0, y * ysiz, &display_buf[y*gn.columns], gn.columns);
    EndPaint(hWnd, &ps);
    return(NULL);
    
    
    
  case WM_START:
    if ( gn_init((int)wParam,(char **)lParam) == QUIT ) {
      PostQuitMessage(0);
      return(NULL);
    }
    if ( gn.newsspool[1] == ':' ) {
      if (SetDisk(toupper(gn.newsspool[0])-'A')){
	mc_MessageBox(hWnd,"NEWSSPOOL に指定されたドライブは使用できません ",gn.newsspool,MB_OK);
	PostQuitMessage(0);
	return(NULL);
      }
    }

#if defined(NO_NET)
    PostMessage(hWnd,WM_GETACTIVE,0,0L);
#else
#ifdef AUTHENTICATION
    if ( DO_POST || DO_MAIL)
      PostMessage(hWnd,WM_GETPASS,0,0L);
    else
      PostMessage(hWnd,WM_OPENSERVER,0,0L);
#else  /* AUTHENTICATION */
    PostMessage(hWnd,WM_OPENSERVER,0,0L);
#endif /* AUTHENTICATION */
#endif
    return(NULL);

#if ! defined(NO_NET)
#ifdef AUTHENTICATION
  case WM_GETPASS:
    if (gn.passwd != 0 ) {
      auth_set_passwd();
      SendMessage(hWnd, WM_CHECKPASS,0 , 0L);
      return(NULL);
    }

    DialogBox(hInst,
	      "IDD_GETPASSWD",
	      hWnd,
	      get_passwd);
    return(NULL);

  case WM_CHECKPASS:		/* パスワード照合 */
    if ( auth_check_pass() != CONT ) {
      PostQuitMessage(0);
      return(NULL);
    }
    PostMessage(hWnd,WM_OPENSERVER,0,0L);
    return(NULL);
#endif /* AUTHENTICATION */

  case WM_OPENSERVER:		/* ニュースサーバとの接続  */
    if ( DO_POST || DO_GET ) {
      mc_printf("%s と接続しています\n",gn.nntpserver);
      if ( open_server() != CONT ) {
	PostQuitMessage(0);
	return(NULL);
      }
    }
    PostMessage(hWnd,WM_POST,0,0L);
    return(NULL);

  case WM_POST:		/* 記事の投稿 */
    if ( DO_POST ) {
      switch (check_post()) {
      case ERROR:
	PostQuitMessage(0);
	return(NULL);
      case NEXT:
	PostMessage(hWnd,WM_POST_1,0,0L);
	return(NULL);
      case CONT:
	PostMessage(hWnd,WM_REPLY,0,0L);
	return(NULL);
      }
    }
    PostMessage(hWnd,WM_REPLY,0,0L);
    return(NULL);

  case WM_POST_1:
    switch ( post() ) {
    case ERROR:
      PostQuitMessage(0);
      return(NULL);
    case NEXT:
      PostMessage(hWnd,WM_POST_1,0,0L);
      return(NULL);
    case CONT:
      return(NULL);
    case QUIT:
      PostMessage(hWnd,WM_REPLY,0,0L);
      return(NULL);
    }
    return(NULL);
    
  case WM_POSTCONF:
    PostMessage(hWnd,WM_POST_1,0,0L);
    return(NULL);
		
  case WM_REPLY:		/* リプライメイルの送信 */
    if ( DO_MAIL ) {
      switch ( check_reply() ) {
      case ERROR:
	PostQuitMessage(0);
	return(NULL);
      case NEXT:
	PostMessage(hWnd,WM_REPLY_1,0,0L);
	return(NULL);
      case CONT:
	PostMessage(hWnd,WM_GETACTIVE,0,0L);
	return(NULL);
      }
    }
    PostMessage(hWnd,WM_GETACTIVE,0,0L);
    return(NULL);
    
  case WM_REPLY_1:
    switch ( reply() ) {
    case ERROR:
      PostQuitMessage(0);
      return(NULL);
    case NEXT:
      PostMessage(hWnd,WM_REPLY_1,0,0L);
      return(NULL);
    case CONT:
      return(NULL);
    case QUIT:
      PostMessage(hWnd,WM_GETACTIVE,0,0L);
      return(NULL);
    }
    return(NULL);
    
  case WM_MAILCONF:
    PostMessage(hWnd,WM_REPLY_1,0,0L);
    return(NULL);

#endif	/* NO_NET */

  case WM_GETACTIVE:	/* アクティブファイルの取得 */
    if ( DO_EXPIRE || DO_GET ) {
      switch ( get_active()) {
      case CONNECTION_CLOSED:
	mc_MessageBox(hWnd,"ニュースサーバとの接続が切れました","",MB_OK);
	PostQuitMessage(0);
	return(NULL);
      case QUIT:
      case ERROR:
	PostQuitMessage(0);
	return(NULL);
      }
    }
    PostMessage(hWnd,WM_READNEWSRC,0,0L);
    return(NULL);
   
  case WM_READNEWSRC:	/* .newsrc ファイルの読み込み */
    if ( DO_EXPIRE || DO_GET ) {
      if ( (newsrc_flag = get_newsrc()) == ERROR ) {
	PostQuitMessage(0);
	return(NULL);
      }
    }
    PostMessage(hWnd,WM_CHECKGROUP,0,0L);
    return(NULL);

  case WM_CHECKGROUP:	/* ニュースグループのチェック */
    if ( DO_EXPIRE || DO_GET )
      if ( interactive_send == OFF )
	check_group(1);
      else
	check_group(0);
    PostMessage(hWnd,WM_GETNEWSGROUPS,0,0L);
    return(NULL);

  case WM_GETNEWSGROUPS:			/* 各グループの説明の取得 */
    if ( gn.description == 1 && DO_GET ) {
      switch ( get_newsgroups() ){
      case CONNECTION_CLOSED:
	mc_MessageBox(hWnd,"ニュースサーバとの接続が切れました","",MB_OK);
	PostQuitMessage(0);
	return(NULL);
      case ERROR:
	PostQuitMessage(0);
	return(NULL);
      }
    }
    PostMessage(hWnd,WM_EXPIRE,0,0L);
    return(NULL);
    
  case WM_EXPIRE:	/* 既読の記事の消去 */
    if ( DO_EXPIRE == 0) {
#if ! defined(NO_NET)
      PostMessage(hWnd,WM_MKNEWSGROUPS,0,0L);
#else
      PostMessage(hWnd,WM_MKACTIVE,0,0L);
#endif	/* NO_NET */
      return(NULL);
    }
    set_hourglass();
    if ( expire() != CONT ) {
      reset_cursor();
      PostQuitMessage(0);
      return(NULL);
    }
    ng = newsrc;
    PostMessage(hWnd,WM_EXPIRE_1,0,(long)ng);
    return(NULL);

  case WM_EXPIRE_1:
    ng = (newsgroup_t *)lParam;
    expire_1ng(ng);
    ng = ng->next;
    if ( ng != 0 ) {
      PostMessage(hWnd,WM_EXPIRE_1,0,(long)ng);
    }else {
      reset_cursor();
#if ! defined(NO_NET)
      PostMessage(hWnd,WM_MKNEWSGROUPS,0,0L);
#else
      PostMessage(hWnd,WM_MKACTIVE,0,0L);
#endif	/* NO_NET */
    }
    return(NULL);



#if ! defined(NO_NET)
  case WM_MKNEWSGROUPS:		/* newsgroups の作成 */
    if ( gn.description == 1 && DO_GET ) {
      if ( make_newsgroups() != CONT ) {
	PostQuitMessage(0);
	return(NULL);
      }
    }
    PostMessage(hWnd,WM_GETARTICLE,0,0L);
    return(NULL);

  case WM_GETARTICLE:		/* 記事の取得 */
    if ( DO_GET ) {
      if ( get_article() != CONT ) {
	PostQuitMessage(0);
	return(NULL);
      }
      ng = newsrc;
      PostMessage(hWnd,WM_GETARTICLE_1,0,(long)ng);
      return(NULL);
    }
    PostMessage(hWnd,WM_MKACTIVE,0,0L);
    return(NULL);

  case WM_GETARTICLE_1:
    ng = (newsgroup_t *)lParam;
    if ( get_article_1ng(ng) != CONT ) {
      PostQuitMessage(0);
      return(NULL);
    }
    ng = ng->next;
    if ( ng != 0 )
      PostMessage(hWnd,WM_GETARTICLE_1,0,(long)ng);
    else
      PostMessage(hWnd,WM_MKACTIVE,0,0L);
    return(NULL);

#endif	/* NO_NET */

  case WM_MKACTIVE:	/* active の作成 */
    if ( DO_GET ) {
      if ( make_active() != CONT ) {
	PostQuitMessage(0);
	return(NULL);
      }
    }
    PostMessage(hWnd,WM_MKHISTORY,0,0L);
    return(NULL);

  case WM_MKHISTORY:		/* history の作成 */
    if ( gn.use_history && DO_GET ) {
      set_hourglass();
      if ( make_history() != CONT ) {
	reset_cursor();
	mc_puts("\nhistory の作成に失敗しました\n");
	PostQuitMessage(0);
	return(NULL);
      }
      ng = newsrc;
      PostMessage(hWnd,WM_MKHISTORY_1,0,(long)ng);
      return(NULL);
    }
    PostMessage(hWnd,WM_DONE,0,0L);
    return(NULL);

  case WM_MKHISTORY_1:		/* history の作成 */
    ng = (newsgroup_t *)lParam;
    make_history_1ng(ng);
    ng = ng->next;
    if ( ng != 0 ) {
      PostMessage(hWnd,WM_MKHISTORY_1,0,(long)ng);
    } else {
      reset_cursor();
      PostMessage(hWnd,WM_DONE,0,0L);
    }
    return(NULL);
    
  case WM_DONE:		/* 終了 */
    if ( (update_newsrc == ON && change == 1) || cnews == ON )
      save_newsrc();                  /* .newsrc を更新 */

    if ( interactive_send == ON )
      MessageBox(hWnd,"Done","gnspool",MB_OK);
    PostQuitMessage(0);
    return(NULL);
    
  case WM_CONNECTIONCLOSED:	/* 接続が切れた */
    mc_MessageBox(hWnd,"ニュースサーバとの接続が切れました","",MB_OK);
    PostQuitMessage(0);
    return(NULL);
    
  default:
    return(DefWindowProc(hWnd, message, wParam, lParam));
  }
  return(NULL);
}

static int
check_post()
{
  char dir[PATH_LEN];
  struct stat stat_buf;
  
  strcpy(dir,gn.newsspool);
  if ( dir[strlen(dir)-1] != SLASH_CHAR)
    strcat(dir,SLASH_STR);
  strcat(dir,"news.out");
  
  if ( stat(dir,&stat_buf) != 0 )
    return(CONT);	/* ポスト記事なし */
  
  if ( chdir(dir) != 0 ) {
    mc_MessageBox(hWnd,"cd できません ", dir,MB_OK);
    return(ERROR);
  }
  
  mc_puts("\n記事をポストします\n");
  return(NEXT);
}
static int
post()
{
  char *tmp_file_f;

  if ( (tmp_file_f = find_first_file()) != (char *)0 ) {
    if ( tmp_file_f[0] == 'g' || tmp_file_f[0] == 'G' ) {
      return(post_article(tmp_file_f));
    }

    while ( (tmp_file_f = find_next_file()) != (char *)0 ) {
      if ( tmp_file_f[0] == 'g' || tmp_file_f[0] == 'G' ) {
	return(post_article(tmp_file_f));
      }
    }
  }
  
  if ( chdir(gn.newsspool) != 0 ) {
    mc_MessageBox(hWnd,"cd できません ", gn.newsspool,MB_OK);
    return(CONT);
  }
  
  if ( rmdir("news.out") != 0 )
    mc_MessageBox(hWnd,"rmdir できません ", "news.out", MB_OK);

  return(QUIT);
}
static char post_newsgroups[NNTP_BUFSIZE+1];
static char post_subject[NNTP_BUFSIZE+1];

static int
post_article(tmp_file_f)
char *tmp_file_f;
{
  BOOL __export CALLBACK post_1file();
#ifndef	WIN32
  BOOL __export CALLBACK _loadds NotifyHandler_post();
#endif
  
  char edit[128];
#ifdef WIN32
  STARTUPINFO		siStartInfo = {0};
  PROCESS_INFORMATION	piProcInfo;
#else
  WORD	wreturn;
#endif
  struct stat stat_buf;

  if ( stat(tmp_file_f,&stat_buf) != 0 )
    return(CONT);	/* ???  */

  if ( stat_buf.st_size == 0 ) {
    if ( unlink(tmp_file_f) != 0 ) {
      return(ERROR);
    }
    return(CONT);
  }

  /* ヘッダの取り出し */
  if ( post_article_print_head(tmp_file_f) != CONT )
    return(ERROR);

  ShowWindow(hWnd,SW_RESTORE);

  if (interactive_send == OFF) {
    mc_printf("%s\n",post_subject);
    mc_printf("%s\n\n",post_newsgroups);
    if ( post_file(tmp_file_f, gn.spool_kanji_code) != CONT )
      return(ERROR);
    if ( unlink(tmp_file_f) != 0 ) {
      mc_MessageBox(hWnd,"削除できません ",tmp_file_f,MB_OK);
      return(ERROR);
    }
    return(NEXT);
  }

  switch (DialogBox(hInst,	/* 現在のインスタンス */
		    "IDD_GNSPOOLSEND", /* 使用するリソース */
		    hWnd,	/* 親ウィンドウのハンドル */
		    post_1file)) {
  case 'y':
    if ( post_file(tmp_file_f, gn.spool_kanji_code) != CONT )
      return(ERROR);
    if ( unlink(tmp_file_f) != 0 ) {
      mc_MessageBox(hWnd,"削除できません ",tmp_file_f,MB_OK);
      return(ERROR);
    }
    return(NEXT);
    
  case 'e':
    sprintf(edit,"%s %s.",gn.editor,tmp_file_f);
#ifdef WIN32
    siStartInfo.cb = sizeof(STARTUPINFO);

    if (CreateProcess(NULL, edit, NULL, NULL, FALSE,
                      0, NULL, NULL, &siStartInfo, &piProcInfo))
    {
      ShowWindow(hWnd,SW_MINIMIZE);
      WaitForSingleObject(piProcInfo.hProcess, INFINITE);
      /* Process has terminated */
      PostMessage(hWnd, WM_POSTCONF, 0, 0L);
    } else {
      /* Process could not be started */
      sprintf(edit,"エディタの起動に失敗しました");
      mc_MessageBox(hWnd,edit,"",MB_ICONSTOP);
    }
#else	/* WIN32 */
    wreturn = WinExec(edit,SW_SHOW);
    if ( wreturn < 32 ){
      sprintf(edit,"エディタの起動に失敗しました %d",wreturn);
      mc_MessageBox(hWnd,edit,"",MB_ICONSTOP);
    } else {
      exeTask = GetWindowTask(GetActiveWindow());
      NprocInst = (LPFNNOTIFYCALLBACK)
	MakeProcInstance(NotifyHandler_post, hInst);
      NotifyRegister(NULL, NprocInst, NF_NORMAL);
      
      ShowWindow(hWnd,SW_MINIMIZE);
    }
#endif	/* WIN32 */
    break;
    
  case 'n':
    if ( unlink(tmp_file_f) != 0 ) {
      mc_MessageBox(hWnd,"削除できません ",tmp_file_f,MB_OK);
      return(ERROR);
    }
    return(NEXT);
    
  default:
    return(ERROR);
  }
  return(CONT);
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
    mc_MessageBox(hWnd,"ファイルのオープンに失敗しました",tmp_file_f,MB_OK);
    return(ERROR);
  }
  
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

    if ( strncmp(buf,"Subject: ",9) == 0) {
#ifdef	MIME
      kanji_convert(gn.spool_kanji_code,buf,JIS,buf2);
      MIME_strHeaderDecode(buf2,buf,sizeof(buf));
      kanji_convert(JIS,buf,gn.display_kanji_code,buf2);
#else
      kanji_convert(gn.spool_kanji_code,buf,gn.display_kanji_code,buf2);
#endif
      strcpy(post_subject,buf2);
    }
    if ( strncmp(buf,"Newsgroups: ",12) == 0) {
#ifdef	MIME
      kanji_convert(gn.spool_kanji_code,buf,JIS,buf2);
      MIME_strHeaderDecode(buf2,buf,sizeof(buf));
      kanji_convert(JIS,buf,gn.display_kanji_code,buf2);
#else
      kanji_convert(gn.spool_kanji_code,buf,gn.display_kanji_code,buf2);
#endif
      strcpy(post_newsgroups,buf2);
    }
  }
  fclose(fp);
  return(CONT);
}

BOOL __export CALLBACK
post_1file(hDlg, message, wParam, lParam)
HWND hDlg;                       /* ダイアログボックスウィンドウのハンドル */
unsigned message;                /* メッセージのタイプ                     */
WORD wParam;                     /* メッセージ固有の情報                   */
LONG lParam;
{
  char sendtext[128];
  
  switch (message) {
  case WM_INITDIALOG:
    SetDlgItemText(hDlg, IDC_NEWSGROUPS, (LPSTR)post_newsgroups);
    SetDlgItemText(hDlg, IDC_SUBJECT, (LPSTR)post_subject);
    kanji_convert(internal_kanji_code,"本当にポストしますか？",
		  gn.display_kanji_code,sendtext);
    SetDlgItemText(hDlg, IDC_TEXT, (LPSTR)sendtext);
    kanji_convert(internal_kanji_code,"する(\036Y)",
		  gn.display_kanji_code, sendtext);
    SetDlgItemText(hDlg, IDOK, sendtext);
    kanji_convert(internal_kanji_code,"しない(\036N)",
		  gn.display_kanji_code, sendtext);
    SetDlgItemText(hDlg, IDCANCEL, sendtext);
    kanji_convert(internal_kanji_code,"もう一度編集(\036E)",
		  gn.display_kanji_code, sendtext);
    SetDlgItemText(hDlg, IDRETRY, sendtext);
    return(TRUE);

  case WM_COMMAND:
    switch (wParam) {
    case IDOK:
      EndDialog(hDlg, 'y');
      return(TRUE);

    case IDCANCEL:
      EndDialog(hDlg, 'n');
      return(TRUE);

    case IDRETRY:
      EndDialog(hDlg, 'e');
      return(TRUE);
    }
    break;
  }
  return(FALSE);
}
#ifndef	WIN32
BOOL __export CALLBACK _loadds
NotifyHandler_post(WORD wID, DWORD dwData)
{
  if (wID == NFY_EXITTASK ) {
    if(GetCurrentTask() == exeTask){
      PostMessage(hWnd, WM_POSTCONF,0 , 0L);
      return(TRUE);
    }
  }
  return(FALSE);
}
#endif
static int
check_reply()
{
  char dir[PATH_LEN];
  struct stat stat_buf;
  
  strcpy(dir,gn.newsspool);
  if ( dir[strlen(dir)-1] != SLASH_CHAR)
    strcat(dir,SLASH_STR);
  strcat(dir,"mail.out");
  
  if ( stat(dir,&stat_buf) != 0 )
    return(CONT);		/* リプライメイルなし */
  
  if ( chdir(dir) != 0 ) {
    mc_MessageBox(hWnd,"cd できません ", dir,MB_OK);
    return(ERROR);
  }
  
  mc_puts("\nリプライメイルを送信します\n");
  return(NEXT);
}
static int
reply()
{
  char *tmp_file_f;

  if ( (tmp_file_f = find_first_file()) != (char *)0 ) {
    if ( tmp_file_f[0] == 'g' || tmp_file_f[0] == 'G' ) {
      return(send_mail(tmp_file_f));
    }

    while ( (tmp_file_f = find_next_file()) != (char *)0 ) {
      if ( tmp_file_f[0] == 'g' || tmp_file_f[0] == 'G' ) {
	return(send_mail(tmp_file_f));
      }
    }
  }
  
  if ( init_smtp_server != 0 )
    smtp_close_server();

  if ( chdir(gn.newsspool) != 0 ) {
    mc_MessageBox(hWnd,"cd できません ", gn.newsspool,MB_OK);
    return(CONT);
  }
  
  if ( rmdir("mail.out") != 0 )
    mc_MessageBox(hWnd,"rmdir できません ", "mail.out", MB_OK);

  return(QUIT);
}
static char mail_to[NNTP_BUFSIZE+1];
static char mail_subject[NNTP_BUFSIZE+1];

static int
send_mail(tmp_file_f)
char *tmp_file_f;
{
  BOOL __export CALLBACK mail_1file();
#ifndef	WIN32
  BOOL __export CALLBACK _loadds NotifyHandler_mail();
#endif
  char edit[128];
#ifdef WIN32
  STARTUPINFO		siStartInfo = {0};
  PROCESS_INFORMATION	piProcInfo;
#else	/* WIN32 */
  WORD	wreturn;
#endif
  
  struct stat stat_buf;

  if ( stat(tmp_file_f,&stat_buf) != 0 )
    return(CONT);	/* ???  */

  if ( stat_buf.st_size == 0 ) {
    if ( unlink(tmp_file_f) != 0 ) {
      return(ERROR);
    }
    return(CONT);
  }

  /* ヘッダの取り出し */
  if ( send_mail_print_head(tmp_file_f) != CONT )
    return(ERROR);

  ShowWindow(hWnd,SW_RESTORE);

  if (interactive_send == OFF) {
    mc_printf("%s\n",mail_to);
    mc_printf("%s\n\n",mail_subject);
    if ( mail_file(tmp_file_f, gn.mail_kanji_code) != CONT )
      return(ERROR);
    if ( unlink(tmp_file_f) != 0 ) {
      mc_MessageBox(hWnd,"削除できません ",tmp_file_f,MB_OK);
      return(ERROR);
    }
    return(NEXT);
  }

  switch (DialogBox(hInst,	/* 現在のインスタンス */
		    "IDD_GNSPOOLSEND", /* 使用するリソース */
		    hWnd,	/* 親ウィンドウのハンドル */
		    mail_1file)) {
  case 'y':
    if ( mail_file(tmp_file_f, gn.mail_kanji_code) != CONT )
      return(ERROR);
    if ( unlink(tmp_file_f) != 0 ) {
      mc_MessageBox(hWnd,"削除できません ",tmp_file_f,MB_OK);
      return(ERROR);
    }
    return(NEXT);
    
  case 'e':
    sprintf(edit,"%s %s.",gn.editor,tmp_file_f);
#ifdef WIN32
    siStartInfo.cb = sizeof(STARTUPINFO);

    if (CreateProcess(NULL, edit, NULL, NULL, FALSE,
                      0, NULL, NULL, &siStartInfo, &piProcInfo))
    {
      ShowWindow(hWnd,SW_MINIMIZE);
      WaitForSingleObject(piProcInfo.hProcess, INFINITE);
      /* Process has terminated */
      PostMessage(hWnd, WM_MAILCONF,0 , 0L);
    } else {
      /* Process could not be started */
      sprintf(edit,"エディタの起動に失敗しました");
      mc_MessageBox(hWnd,edit,"",MB_ICONSTOP);
    }
#else	/* !WIN32 */
    wreturn = WinExec(edit,SW_SHOW);
    if ( wreturn < 32 ){
      sprintf(edit,"エディタの起動に失敗しました %d",wreturn);
      mc_MessageBox(hWnd,edit,"",MB_ICONSTOP);
    } else {
      exeTask = GetWindowTask(GetActiveWindow());
      NprocInst = (LPFNNOTIFYCALLBACK)
	MakeProcInstance(NotifyHandler_mail, hInst);
      NotifyRegister(NULL, NprocInst, NF_NORMAL);
      
      ShowWindow(hWnd,SW_MINIMIZE);
    }
#endif
    break;
    
  case 'n':
    if ( unlink(tmp_file_f) != 0 ) {
      mc_MessageBox(hWnd,"削除できません ",tmp_file_f,MB_OK);
      return(ERROR);
    }
    return(NEXT);
    
  default:
    return(ERROR);
  }
  return(CONT);
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
    mc_MessageBox(hWnd,"ファイルのオープンに失敗しました",tmp_file_f,MB_OK);
    return(ERROR);
  }
  
  get_1_header_file(buf,resp,fp);
  
  while (1) {
    if (resp[0] == 0 )
      break;
    get_1_header_file(buf,resp,fp);
    if (buf[0] == 0 )
      break;
    if ( strncmp(buf,"To: ",4) == 0) {
#ifdef	MIME
      kanji_convert(gn.spool_kanji_code,buf,JIS,buf2);
      MIME_strHeaderDecode(buf2,buf,sizeof(buf));
      kanji_convert(JIS,buf,gn.display_kanji_code,buf2);
#else
      kanji_convert(gn.spool_kanji_code,buf,gn.display_kanji_code,buf2);
#endif
      strcpy(mail_to,buf2);
    }
    if ( strncmp(buf,"Subject: ",9) == 0) {
#ifdef	MIME
      kanji_convert(gn.spool_kanji_code,buf,JIS,buf2);
      MIME_strHeaderDecode(buf2,buf,sizeof(buf));
      kanji_convert(JIS,buf,gn.display_kanji_code,buf2);
#else
      kanji_convert(gn.spool_kanji_code,buf,gn.display_kanji_code,buf2);
#endif
      strcpy(mail_subject,buf2);
    }
  }
  fclose(fp);
  return(CONT);
}

BOOL __export CALLBACK
mail_1file(hDlg, message, wParam, lParam)
HWND hDlg;                       /* ダイアログボックスウィンドウのハンドル */
unsigned message;                /* メッセージのタイプ                     */
WORD wParam;                     /* メッセージ固有の情報                   */
LONG lParam;
{
  char sendtext[128];
  
  switch (message) {
  case WM_INITDIALOG:
    SetDlgItemText(hDlg, IDC_NEWSGROUPS, (LPSTR)mail_to);
    SetDlgItemText(hDlg, IDC_SUBJECT, (LPSTR)mail_subject);
    kanji_convert(internal_kanji_code,"本当にメイルしますか？",
		  gn.display_kanji_code,sendtext);
    SetDlgItemText(hDlg, IDC_TEXT, (LPSTR)sendtext);
    kanji_convert(internal_kanji_code,"する(\036Y)",
		  gn.display_kanji_code, sendtext);
    SetDlgItemText(hDlg, IDOK, sendtext);
    kanji_convert(internal_kanji_code,"しない(\036N)",
		  gn.display_kanji_code, sendtext);
    SetDlgItemText(hDlg, IDCANCEL, sendtext);
    kanji_convert(internal_kanji_code,"もう一度編集(\036E)",
		  gn.display_kanji_code, sendtext);
    SetDlgItemText(hDlg, IDRETRY, sendtext);
    return(TRUE);

  case WM_COMMAND:
    switch (wParam) {
    case IDOK:
      EndDialog(hDlg, 'y');
      return(TRUE);

    case IDCANCEL:
      EndDialog(hDlg, 'n');
      return(TRUE);

    case IDRETRY:
      EndDialog(hDlg, 'e');
      return(TRUE);
    }
    break;
  }
  return(FALSE);
}
#ifndef	WIN32
BOOL __export CALLBACK _loadds
NotifyHandler_mail(WORD wID, DWORD dwData)
{
  if (wID == NFY_EXITTASK ) {
    if(GetCurrentTask() == exeTask){
      PostMessage(hWnd, WM_MAILCONF,0 , 0L);
      return(TRUE);
    }
  }
  return(FALSE);
}
#endif
