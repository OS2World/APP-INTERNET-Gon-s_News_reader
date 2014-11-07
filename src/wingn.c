/*
 * @(#)wingn.c	1.40 Aug.18,1997
 *
 * 著作権は放棄しません。ただし、営利目的以外の使用／配布に制限は設けません。
 * Copyright (C) yamasita@omronsoft.co.jp
 */

#include <windows.h>
#include "wingn.h"


#include	<stdio.h>
#include	<signal.h>

#include	<stdlib.h>
#include	<string.h>
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

extern int newsrc_flag;		/* newsrc を読み込んだら０、なかったら−１ */

extern int gn_init();
extern int get_newsrc();
extern int open_server();
extern int get_active();
extern int get_newsgroups();
extern void check_group();
extern void close_server();
extern int group_mode();

extern int set_groupmode_menu();
extern void group_mode_list();
extern void win_group_mode_command();
extern int groupmode_menu_exec();

extern int subject_mode();
extern int set_subjectmode_menu();
extern void subject_mode_list();
extern void win_subject_mode_command();
extern int subjectmode_menu_exec();
extern void subject_all_article();

extern int article_mode();
extern int set_articlemode_menu();
extern void article_mode_list();
extern void win_article_mode_command();
extern int articlemode_menu_exec();
extern void article_all_article();

extern int post_mode();
extern int reference_mode();
extern int followup_mode();
extern int cancel_mode();
extern void reply_mode();
extern void mail_mode();
extern void save_mode();

extern int mc_MessageBox();

extern void follow_confirm(),post_confirm(),reply_confirm(),mail_confirm();

extern void win_pager_key(),win_pager_redraw(),win_pager_cont(),
	win_pager_back(),win_pager_k(),win_pager_one(),
	win_pager_bottom(),win_pager_end();

int PASCAL
WinMain(hInstance, hPrevInstance, lpCmdLine, nCmdShow)
HANDLE hInstance;
HANDLE hPrevInstance;
LPSTR lpCmdLine;
int nCmdShow;
{
  extern char *help_file;		/* temp file for help */

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
  
#ifdef WIN32
  while (GetMessage(&msg, NULL, 0, 0) )
#else
  while (GetMessage(&msg, NULL, NULL, NULL) )
#endif
    {
    TranslateMessage(&msg);
    DispatchMessage(&msg);
  }
  
  /* 最後に読んだ記事があれば捨てる */
  if ( last_art_check() != ERROR && make_article_tmpfile == ON )
    unlink(last_art.tmp_file);

  /* help file の削除 */
  if ( help_file != 0 )
    unlink(help_file);
  
#if ! defined(NO_NET)
  /* サーバとの接続を切る */
  if ( gn.local_spool == 0 ) {
    close_server();
#if defined(WINSOCK)
    WSACleanup();
#endif
  }
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
  extern int gn_init();
  BOOL __export CALLBACK About(HWND, unsigned, WORD, LONG);
#ifdef AUTHENTICATION
  extern void auth_set_passwd();
  BOOL __export CALLBACK get_passwd(HWND, unsigned, WORD, LONG);
  extern int auth_check_pass();
#endif /* AUTHENTICATION */
  
  static int mode,mode_save;
  static newsgroup_t *ng;
  static subject_t *sb;
  static void (*redraw_func_save)();
  
  static char key_buf[KEY_BUF_LEN];
  static char *kb_pt = key_buf;
  register char *pt;
  register unsigned int i;
  register int y;
  register char c;
  long	numb;
  
  HDC hdc;
  TEXTMETRIC tm;
  PAINTSTRUCT	ps;
  
  static int	mouse_y = -1;
  COLORREF textcolor,bkcolor;
  
  switch (message) {
  case WM_CREATE:
    hdc = GetDC(hWnd);
    SelectObject(hdc, GetStockObject(SYSTEM_FIXED_FONT));
    GetTextMetrics(hdc, &tm);
    xsiz = tm.tmAveCharWidth;
    ysiz = tm.tmHeight;
    ReleaseDC(hWnd, hdc);

    hHourGlass = LoadCursor(NULL, IDC_WAIT);
    return(NULL);

  case WM_SETFOCUS:
    CreateCaret(hWnd, NULL, xsiz, ysiz);
    SetCaretPos(xpos * xsiz, ypos * ysiz);
    ShowCaret(hWnd);
    return(NULL);

  case WM_KILLFOCUS:
    HideCaret(hWnd);
    DestroyCaret();
    return(NULL);

  case WM_DESTROY:		/* ウィンドウを破棄  */
    if ( change ) {
      if ( mc_MessageBox(hWnd,"既読情報を保存しますか？",
			 "",MB_YESNO) == IDYES) {
	save_newsrc();		/* .newsrc を更新 */
      }
      change = NO;
    }
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

    if ( redraw_func != (void (*)())NULL ) {
      redraw_func();
    } else {
      ScrollWindow(hWnd, 0, -wheight, NULL, NULL);
      UpdateWindow(hWnd);
      xpos = ypos = 0;
    }
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

  case WM_COMMAND:		/* メニュー */
    switch (wParam) {
    case IDM_ABOUT:
      DialogBox(hInst,
		"ABOUTBOX",
		hWnd,
		About);
      return(NULL);

    case IDM_FOLLOW:
      followup_mode();
      return(NULL);
    case IDM_REPLY:
      reply_mode();
      return(NULL);
    case IDM_CANCEL:
      cancel_mode();
      return(NULL);
#ifndef	NO_SAVE
    case IDM_SAVE:
      key_buf[0]=0;
      key_buf[1]=0;
      save_mode(key_buf);
      return(NULL);
#endif /* NO_SAVE */
    case IDM_REFERENCE:
      reference_mode();
      return(NULL);
    case IDM_POST:
      post_mode(NULL);
      return(NULL);
    case IDM_MAIL:
      mail_mode();
      return(NULL);

    case IDM_GRPCOMMAND:
    case IDM_GRPPREV:
    case IDM_GRPNEXT:
    case IDM_GRPHASH:
    case IDM_GRPLIST:
    case IDM_GRPUNSUBSCRIBE:
    case IDM_GRPSUBSCRIBE:
    case IDM_GRPCATCHUP:
    case IDM_GRPSEARCH:
    case IDM_GRPREVSEARCH:
    case IDM_GRPWRITENEWSRC:
    case IDM_GRPEND:
    case IDM_GRPQUIT:
    case IDM_GRPHELP:
      groupmode_menu_exec(wParam);
      return(NULL);

    case IDM_SUBCOMMAND:
    case IDM_SUBPREV:
    case IDM_SUBNEXT:
    case IDM_SUBTRUNC:
    case IDM_SUBALL:
    case IDM_SUBUNSUBSCRIBE:
    case IDM_SUBSUBSCRIBE:
    case IDM_SUBCATCHUP:
    case IDM_SUBLIST:
    case IDM_SUBSEARCH:
    case IDM_SUBREVSEARCH:
    case IDM_SUBPREVGRP:
    case IDM_SUBNEXTGRP:
    case IDM_SUBQUIT:
    case IDM_SUBHELP:
      subjectmode_menu_exec(wParam);
      return(NULL);

    case IDM_ARTCOMMAND:
    case IDM_ARTPREV:
    case IDM_ARTNEXT:
    case IDM_ARTTRUNC:
    case IDM_ARTALL:
    case IDM_ARTUNSUBSCRIBE:
    case IDM_ARTSUBSCRIBE:
    case IDM_ARTCATCHUP:
    case IDM_ARTUNREAD:
    case IDM_ARTLIST:
    case IDM_ARTSEARCH:
    case IDM_ARTREVSEARCH:
    case IDM_ARTPREVSUB:
    case IDM_ARTNEXTSUB:
    case IDM_ARTQUIT:
    case IDM_ARTHELP:
      articlemode_menu_exec(wParam);
      return(NULL);


    case IDM_PAGCONT:
      win_pager_cont();
      return(NULL);
    case IDM_PAGONE:
      win_pager_one();
      return(NULL);
    case IDM_PAGBACK:
      win_pager_back();
      return(NULL);
    case IDM_PAGK:
      win_pager_k();
      return(NULL);
    case IDM_PAGBOTTOM:
      win_pager_bottom();
      return(NULL);
    case IDM_PAGQUIT:
      PostMessage(hWnd,WM_PAGER_END,0,0L);
      return(NULL);
    }
    return(DefWindowProc(hWnd, message, wParam, lParam));

  case WM_CHAR:
    c = (char)wParam;

    if ( mode == WM_PAGER ) {
      win_pager_key(c);
      return(NULL);
    }

    if ( 0x20 <= c && c <= 0x7e ) {
      if ( c == ' ' && kb_pt == key_buf ) {
	c = '\n';
      } else {
	for (i = 0; i < LOWORD(lParam); i++) {
	  *kb_pt++ = c;
	  mc_printf("%c",c);
	}
	return(NULL);
      }
    } else {
      if ( c == BS ) {
	for (i = 0; i < LOWORD(lParam); i++) {
	  if ( kb_pt > key_buf){
	    kb_pt--;
	    if ( xpos > 0 ) {
	      xpos--;
	      mc_puts(" ");
	      xpos--;
	    }
	  }
	}
	SetCaretPos(xpos * xsiz, ypos * ysiz);
	return(NULL);
      }
    }

    if ( c != '\r' && c != '\n' )
      return(NULL);

    mc_puts("\n");

    *kb_pt++ = 0;
    *kb_pt++ = 0;
    kb_pt = key_buf;

    /* 後ろのホワイトスペースを消す */
    pt = &key_buf[strlen(key_buf)-1];
    while( pt >= key_buf &&
	  ( *pt == ' ' || *pt == '\t' || *pt == '\n' || *pt == '\r') ){
      *pt-- = 0;
    }

    /* 前のホワイトスペースを消す */
    pt = key_buf;
    while(*pt == ' ' || *pt == '\t' )
      pt++;

    switch (mode) {
    case WM_GROUPMODE:
      win_group_mode_command(pt);
      return(NULL);

    case WM_SUBJECTMODE:
      win_subject_mode_command(pt);
      return(NULL);

    case WM_ARTICLEMODE:
      win_article_mode_command(pt);
      return(NULL);
    }
    return(NULL);


  case WM_RBUTTONDOWN:
  case WM_RBUTTONDBLCLK:
    if ( mode == WM_PAGER ) {
      win_pager_key('q');
      return(NULL);
    }

    key_buf[0] = 'q';
    key_buf[1] = 0;
    pt = key_buf;

    switch (mode) {
    case WM_GROUPMODE:
      win_group_mode_command(pt);
      return(NULL);
				
    case WM_SUBJECTMODE:
      win_subject_mode_command(pt);
      return(NULL);
				
    case WM_ARTICLEMODE:
      win_article_mode_command(pt);
      return(NULL);
    }
    return(NULL);

  case WM_LBUTTONDOWN:
    if ( mode == WM_PAGER ) {
      win_pager_key(' ');
      return(NULL);
    }

    if ( mouse_y != -1 ) {	/* 違うところをクリック */
      hdc = GetDC(hWnd);
      SelectObject(hdc, GetStockObject(SYSTEM_FIXED_FONT));
      TextOut(hdc, 0, mouse_y * ysiz,
	      &display_buf[mouse_y * gn.columns], gn.columns);
      ReleaseDC(hWnd, hdc);
    }

    mouse_y = HIWORD(lParam) / ysiz;
    pt = &display_buf[mouse_y * gn.columns];
    if (mode == WM_GROUPMODE   && *pt == 'U' ||
	mode == WM_ARTICLEMODE && *pt == 'R' ) {
      pt++;
    }
    i = 0;
    while ( *pt == ' ' && i <= 10 ) {
      pt++;
      i++;
    }

    if ( *pt < '0' || '9' < *pt ) {
      key_buf[0] = 0;
      mc_printf("\n");
      pt = key_buf;
      switch (mode) {
      case WM_GROUPMODE:
	win_group_mode_command(pt);
	return(NULL);
				
      case WM_SUBJECTMODE:
	win_subject_mode_command(pt);
	return(NULL);
				
      case WM_ARTICLEMODE:
	win_article_mode_command(pt);
	return(NULL);
      }
      return(NULL);
    }

    hdc = GetDC(hWnd);
    SelectObject(hdc, GetStockObject(SYSTEM_FIXED_FONT));
    textcolor = GetTextColor(hdc);
    bkcolor = GetBkColor(hdc);
    SetTextColor(hdc,bkcolor);
    SetBkColor(hdc,textcolor);
    TextOut(hdc, 0, mouse_y * ysiz,
	    &display_buf[mouse_y * gn.columns], gn.columns);
    SetTextColor(hdc,textcolor);
    SetBkColor(hdc,bkcolor);
    ReleaseDC(hWnd, hdc);

    return(NULL);

  case WM_LBUTTONDBLCLK:
    if ( mode == WM_PAGER ) {
      win_pager_key(' ');
      return(NULL);
    }

    if ( mouse_y != -1 ) {
      mouse_y = HIWORD(lParam) / ysiz;
      pt = &display_buf[mouse_y * gn.columns];
      if (mode == WM_GROUPMODE   && *pt == 'U' ||
	  mode == WM_ARTICLEMODE && *pt == 'R' ) {
	pt++;
      }
      i = 0;
      while ( *pt == ' ' && i <= 10 ) {
	pt++;
	i++;
      }
			
      if ( *pt < '0' || '9' < *pt )
	return(NULL);
			
      numb = 0;
      while ( '0' <= *pt && *pt <= '9' ) {
	numb = numb * 10L + *pt - '0';
	pt++;
      }
			
      sprintf(key_buf,"%ld",numb);
    } else {
      key_buf[0] = 0 ;
    }

    pt = key_buf;
    mc_printf("%s",pt);
    switch (mode) {
    case WM_GROUPMODE:
      win_group_mode_command(pt);
      return(NULL);

    case WM_SUBJECTMODE:
      win_subject_mode_command(pt);
      return(NULL);

    case WM_ARTICLEMODE:
      win_article_mode_command(pt);
      return(NULL);
    }

    return(NULL);

  case WM_VSCROLL:
    if ( mode == WM_PAGER ) {
      switch(wParam) {
      case SB_LINEDOWN:		/* １行下 */
	win_pager_key('j');
	break;
      case SB_LINEUP:		/* １行上 */
	win_pager_key('k');
	break;
      case SB_PAGEDOWN:		/* １ページ下 */
	win_pager_key(' ');
	break;
      case SB_PAGEUP:		/* １ページ上 */
	win_pager_key('b');
	break;
      }
      return(NULL);
    }

    key_buf[0] = 0;

    switch(wParam) {
    case SB_LINEDOWN:		/* １行下 */
      strcpy(key_buf,"n 1");
      break;
    case SB_LINEUP:		/* １行上 */
      strcpy(key_buf,"p 1");
      break;
    case SB_PAGEDOWN:		/* １ページ下 */
      strcpy(key_buf,"n");
      break;
    case SB_PAGEUP:		/* １ページ上 */
      strcpy(key_buf,"p");
      break;
    }
    if ( key_buf[0] == 0 )
      return(NULL);

    switch (mode) {
    case WM_GROUPMODE:
      PostMessage(hWnd,WM_GROUPCMD,0,0L);
      return(NULL);

    case WM_SUBJECTMODE:
      PostMessage(hWnd,WM_SUBJECTCMD,0,0L);
      return(NULL);

    case WM_ARTICLEMODE:
      PostMessage(hWnd,WM_ARTICLECMD,0,0L);
      return(NULL);
    }

    return(NULL);

  case WM_START:
    if ( gn_init((int)wParam,(char **)lParam) == QUIT ) {
      PostQuitMessage(0);
      return(NULL);
    }
#if defined(NO_NET)
    PostMessage(hWnd,WM_GETACTIVE,0,0L);
#else
    if ( gn.local_spool == 0 ) {
#ifdef AUTHENTICATION
      PostMessage(hWnd,WM_GETPASS,0,0L);
#else  /* AUTHENTICATION */
      PostMessage(hWnd,WM_OPENSERVER,0,0L);
#endif /* AUTHENTICATION */
    } else {
      PostMessage(hWnd,WM_GETACTIVE,0,0L);
    }
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
    if ( auth_check_pass() != 0 ) {
      PostQuitMessage(0);
      return(NULL);
    }
    PostMessage(hWnd,WM_OPENSERVER,0,0L);
    return(NULL);
#endif /* AUTHENTICATION */

  case WM_OPENSERVER:		/* ニュースサーバとの接続  */
    if ( gn.local_spool == 0 ) {
      mc_printf("%s と接続しています\n",gn.nntpserver);
      if ( open_server() != CONT ) {
	PostQuitMessage(0);
	return(NULL);
      }
    }
    PostMessage(hWnd,WM_GETACTIVE,0,0L);
    return(NULL);
#endif /* NO_NET */

  case WM_GETACTIVE:		/* アクティブファイルの取得 */
    switch ( get_active()) {
    case CONNECTION_CLOSED:
      mc_MessageBox(hWnd,"ニュースサーバとの接続が切れました","",MB_OK);
      PostQuitMessage(0);
      return(NULL);
    case QUIT:
    case ERROR:
      return(NULL);
    }
    PostMessage(hWnd,WM_READNEWSRC,0,0L);
    return(NULL);

  case WM_READNEWSRC:		/* .newsrc ファイルの読み込み */
    if ( (newsrc_flag = get_newsrc()) == ERROR )
      return(NULL);
    PostMessage(hWnd,WM_CHECKGROUP,0,0L);
    return(NULL);

  case WM_CHECKGROUP:		/* ニュースグループのチェック */
    check_group(0);
    PostMessage(hWnd,WM_GETNEWSGROUPS,0,0L);
    return(NULL);

  case WM_GETNEWSGROUPS:	/* 各グループの説明の取得 */
    if ( gn.description == 1 ) {
      switch ( get_newsgroups() ){
      case CONNECTION_CLOSED:
	mc_MessageBox(hWnd,"ニュースサーバとの接続が切れました","",MB_OK);
	PostQuitMessage(0);
	return(NULL);
      case ERROR:
	return(NULL);
      }
    }
    ShowScrollBar(hWnd,SB_VERT,TRUE);
    PostMessage(hWnd,WM_GROUPMODE,0,0L);
    return(NULL);

  case WM_GROUPMODE:
    mouse_y = -1;
    set_groupmode_menu();
    mode = WM_GROUPMODE;
    redraw_func = group_mode_list;
    group_mode_list();
    return(NULL);

  case WM_GROUPCMD:
    win_group_mode_command(key_buf);
    return(NULL);

  case WM_ENTERSUBJECTMODE:
    ng = (newsgroup_t *)lParam;
    if ( subject_mode(ng) != CONT )
      PostMessage(hWnd,WM_GROUPMODE,0,0L);
    else
      PostMessage(hWnd,WM_SUBJECTMODE,0,0L);
    return(NULL);

  case WM_SUBJECTMODE:
    mouse_y = -1;
    set_subjectmode_menu();
    mode = WM_SUBJECTMODE;
    redraw_func = subject_mode_list;
    subject_mode_list();
    return(NULL);

  case WM_SUBJECTCMD:
    win_subject_mode_command(key_buf);
    return(NULL);

  case WM_SUBJECTALL:
    mode = WM_SUBJECTALL;
    redraw_func = subject_all_article;
    subject_all_article();
    return(NULL);


  case WM_ENTERARTICLEMODE:
    mouse_y = -1;
    set_articlemode_menu();
    sb = (subject_t *)lParam;
    article_mode(ng,sb);
    PostMessage(hWnd,WM_ARTICLEMODE,0,0L);
    return(NULL);

  case WM_ARTICLEMODE:
    mode = WM_ARTICLEMODE;
    redraw_func = article_mode_list;
    article_mode_list();
    return(NULL);

  case WM_ARTICLECMD:
    win_article_mode_command(key_buf);
    return(NULL);

  case WM_ARTICLEALL:
    mode = WM_ARTICLEALL;
    redraw_func = article_all_article;
    article_all_article();
    return(NULL);


  case WM_FOLLOWCONF:
    follow_confirm();
    return(NULL);

  case WM_POSTCONF:
    post_confirm();
    return(NULL);
		
  case WM_REPLYCONF:
    reply_confirm();
    return(NULL);

  case WM_MAILCONF:
    mail_confirm();
    return(NULL);


  case WM_PAGER:
    mouse_y = -1;
    mode_save = mode;
    mode = WM_PAGER;
    redraw_func_save = redraw_func;
    redraw_func = win_pager_redraw;
    win_pager_cont();
    return(NULL);

  case WM_PAGER_END:		/* EOF or 'q' */
    win_pager_end();
    mode = mode_save;
    redraw_func = redraw_func_save;
    if ( redraw_func != (void (*)())NULL )
      redraw_func();
    if (mode == WM_GROUPMODE)
      set_groupmode_menu();
    if (mode == WM_SUBJECTMODE)
      set_subjectmode_menu();
    if (mode == WM_ARTICLEMODE)
      set_articlemode_menu();
    return(NULL);


  case WM_SAVENEWSRC:		/* newsrc 保存 */
    if ( change )
      save_newsrc();		/* .newsrc を更新 */
    PostQuitMessage(0);
    return(NULL);

  case WM_CONNECTIONCLOSED:	/* 接続が切れた */
    mc_MessageBox(hWnd,"ニュースサーバとの接続が切れました","",MB_OK);
    if ( change ) {
      if ( mc_MessageBox(hWnd,"既読情報を保存しますか？",
			 "",MB_YESNO) == IDYES) {
	save_newsrc();		/* .newsrc を更新 */
      }
      change = NO;
    }
    gn.local_spool = ON;
#if defined(WINSOCK)
    WSACleanup();
#endif
    PostQuitMessage(0);
    return(NULL);

  default:
    return(DefWindowProc(hWnd, message, wParam, lParam));
  }
  return(NULL);
}
