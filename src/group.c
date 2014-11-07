/*
 * @(#)group.c	1.40 May.2,1998
 *
 * 著作権は放棄しません。ただし、営利目的以外の使用／配布に制限は設けません。
 * Copyright (C) yamasita@omronsoft.co.jp
 */

#if defined(WINDOWS)
#	include	<windows.h>
#	include	"wingn.h"
#endif	/* WINDOWS */

#include	<stdio.h>

#ifdef	HAS_STRING_H
#	include	<string.h>
#endif
#ifdef	HAS_STDLIB_H
#	include	<stdlib.h>
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

#include	"nntp.h"
#include	"gn.h"

extern char *kb_input();
extern void save_newsrc();
extern void reply_mode();
extern void mail_mode();
extern void save_mode();
extern void pipe_mode();
extern void mc_puts(),mc_printf();
extern newsgroup_t *search_group();
extern void hit_return();
extern int get_area();
extern void kanji_convert();
extern void str_cut();
extern char *Fgets();
extern int last_art_check();

#if defined(WINDOWS)
extern int mc_MessageBox();
extern void fast_mc_puts_start(),fast_mc_puts_end();
extern void win_pager();
#endif	/* WINDOWS */

#ifdef	QUICKWIN
extern int qw_pager();
#endif	/* QUICKWIN */

#ifdef USE_JNAMES
extern char *jnFetch();
#endif

static int top = 0;
static int start = 0;
static int all_flag= 0;

static char sea_grpname[80] = "";
#if defined(WINDOWS)
static char group_name[NNTP_BUFSIZE];
#endif	/* WINDOWS */

static void group_subscribe(),group_catchup(),
	group_search(), group_search_rev();
static goto_group(),goto_ng(),
	group_lookup(),fstrcmp();


#ifdef USE_JNAMES
static char *jng();
#endif

#ifndef	WINDOWS
int
group_mode()
{
  void group_mode_list();
  int group_mode_command();
  
  register char *pt;
  char key_buf[KEY_BUF_LEN];
  
  while (1) {
    /* 一覧表示 */
    group_mode_list();

    /* コマンド入力 */
    memset(key_buf,0,KEY_BUF_LEN);
    redraw_func = group_mode_list;
    while ( FGETS(key_buf,KEY_BUF_LEN,stdin) == NULL ){
      clearerr(stdin);
      group_mode_list();
    }
    redraw_func = (void (*)())NULL;

    /* 後ろのホワイトスペースを消す */
    pt = &key_buf[strlen(key_buf)-1];
    while( pt >= key_buf && ( *pt == ' ' || *pt == '\t' || *pt == '\n' ) )
      *pt-- = 0;
		
    /* 前のホワイトスペースを消す */
    pt = key_buf;
    while(*pt == ' ' || *pt == '\t' )
      pt++;
		
    /* コマンド解析 */
    switch ( group_mode_command(pt) ) {
    case CONT:
      break;
    case END:
      return(END);
    case QUIT:
      return(QUIT);
    case CONNECTION_CLOSED:
      return(CONNECTION_CLOSED);
    case ERROR:
      return(ERROR);
    }
  }
}
#else	/* WINDOWS */
int
set_groupmode_menu()
{
  extern int setmenu();
  
  static gnpopupmenu_t groupmode_file_menu[] = {
#ifndef	NO_SAVE
    { "最後に読んだ記事をファイルに保存する(\036S)",
	IDM_SAVE,		AFTER_READ|SEPARATOR_NEXT },
#endif /* NO_SAVE */
    { "既読情報を保存する(\036W)",
	IDM_GRPWRITENEWSRC,	AFTER_READ },
    { "終了(\036Q)",
	IDM_GRPEND,			0 },
    { "既読情報を更新せず終了(\036X)",
	IDM_GRPQUIT,			0 },
    { 0,0,0 }
  };
  
  static gnpopupmenu_t groupmode_edit_menu[] = {
    { "グループ、メッセージＩＤの指定...",
	IDM_GRPCOMMAND,		SEPARATOR_NEXT },

    { "ニュースグループ名の検索(\036/)...",
	IDM_GRPSEARCH,		0 },
    { "ニュースグループ名の逆検索(\036\\)...",
	IDM_GRPREVSEARCH,		SEPARATOR_NEXT },

    { "指定したグループはもう読まない(\036U)...",
	IDM_GRPUNSUBSCRIBE,	0 },
    { "指定したグループはやっぱり読む(\036S)...",
	IDM_GRPSUBSCRIBE,		SEPARATOR_NEXT },

    { "指定したグループの記事を全部読んだことにする(\036C)...",
	IDM_GRPCATCHUP,		SEPARATOR_NEXT },

    { "新しい記事をチェックする(\036H)",
	IDM_GRPHASH,			0 },
    { 0,0,0 }
  };
  
  static gnpopupmenu_t groupmode_view_menu[] = {
    { "前画面を表示する(\036P)",
	IDM_GRPPREV,			0 },
    { "次画面を表示する(\036N)",
	IDM_GRPNEXT,			0 },
    { "",			/* programable */
	IDM_GRPLIST,			SEPARATOR_NEXT },

    { "最後に読んだ記事の元記事を表示する(\036R)",
	IDM_REFERENCE, 		AFTER_READ },
    { 0,0,0 }
  };
  
  static gnpopupmenu_t groupmode_send_menu[] = {
    { "最後に読んだ記事にフォロー記事を出す(\036F)",
	IDM_FOLLOW, 			AFTER_READ },
    { "最後に読んだ記事にリプライメイルを出す(\036R)",
	IDM_REPLY, 			AFTER_READ },
    { "最後に読んだ記事をキャンセルする(\036C)",
	IDM_CANCEL,			AFTER_READ|SEPARATOR_NEXT },

    { "記事をポストする(\036I)...",
	IDM_POST,				0 },
    { "メイルを送信する(\036M)...",
	IDM_MAIL, 			0 },
    { 0,0,0 }
  };
  
  static gnpopupmenu_t groupmode_help_menu[] = {
    { "コマンド一覧の表示(\036?)...",
	IDM_GRPHELP, 			0 },
    { "バージョン情報(\036A)...",
	IDM_ABOUT, 			0 },
    { 0,0,0 }
  };
  
  static gnmenu_t groupmode_menu[] = {
    { "ファイル(\036F)",	groupmode_file_menu },
    { "編集(\036E)",		groupmode_edit_menu },
    { "表示(\036V)",		groupmode_view_menu },
    { "発信(\036S)",		groupmode_send_menu },
    { "ヘルプ(\036H)",		groupmode_help_menu },
    { 0,0 }
  };
  
  static char
    *disp_all    = "全グループを表示する(\036L)",
    *disp_notall = "未読の記事があるグループのみを表示する(\036L)";
  
  gnpopupmenu_t *gnpm;
  
  for ( gnpm = groupmode_view_menu ; gnpm->item != 0 ; gnpm++ ) {
    if ( gnpm->idm != IDM_GRPLIST )
      continue;
    if ( all_flag )
      gnpm->item = disp_notall;
    else
      gnpm->item = disp_all;
    break;
  }
  
  return(setmenu(groupmode_menu));
}
int
setmenu(gn_menu)
gnmenu_t *gn_menu;
{
  char buf[128];
  HMENU hmenu;
  HMENU hmenupopup;
  UINT fuflags;
  
  register gnpopupmenu_t *gnpm;
  
  hmenu = GetMenu(hWnd);	/* root menu */
  
  /* 既にあるメニューを消す */
  while ( (hmenupopup = GetSubMenu(hmenu,0)) != NULL ) {
    if ( DeleteMenu(hmenu,0,MF_BYPOSITION) == 0 )
      return(-1);
  }
  
  /* 新しいメニューの設定 */
  for ( ; gn_menu->title != 0 ; gn_menu++ ) {
    if ( (hmenupopup = CreatePopupMenu()) == NULL )
      return(-1);

    for ( gnpm = gn_menu->gnpopup ; gnpm->item != 0 ; gnpm++ ) {
      if ((gnpm->menu_flag & AFTER_READ) != 0 &&
	  last_art_check() == ERROR) {
	fuflags = MF_GRAYED|MF_STRING;
      } else {
	fuflags = MF_ENABLED|MF_STRING;
      }
      kanji_convert(internal_kanji_code,gnpm->item,
		    gn.display_kanji_code,buf);
      if (AppendMenu(hmenupopup,fuflags,gnpm->idm,buf) == 0)
	return(-1);

      if ((gnpm->menu_flag & SEPARATOR_NEXT) != 0) {
	if (AppendMenu(hmenupopup,MF_SEPARATOR,0,0) == 0 )
	  return(-1);
      }
    }

    kanji_convert(internal_kanji_code,gn_menu->title,
		  gn.display_kanji_code,buf);
    if ( AppendMenu(hmenu,MF_POPUP|MF_STRING,
		    (UINT)hmenupopup,buf) == 0 ) {
      return(-1);
    }
  }
  
  
  DrawMenuBar(hWnd);
  return(0);
}
groupmode_menu_exec(wParam)
WPARAM wParam;
{
  void win_group_mode_command();
  void group_mode_list();
  
  register char *pt;
  char key_buf[2];
  
  pt = key_buf;
  key_buf[1] = 0;
  
  switch ( wParam ) {
  case IDM_GRPCOMMAND:
    if ((pt = kb_input("ニュースグループの番号、または名前を入力して下さい")) == NULL )
      return(0);
    break;
  case IDM_GRPPREV:
    *pt = 'p';
    break;
  case IDM_GRPNEXT:
    *pt = 'n';
    break;
  case IDM_GRPHASH:
    *pt = 'h';
    break;
  case IDM_GRPLIST:
    *pt = 'l';
    break;
  case IDM_GRPUNSUBSCRIBE:
    *pt = 'u';
    break;
  case IDM_GRPSUBSCRIBE:
    *pt = 'S';
    break;
  case IDM_GRPCATCHUP:
    *pt = 'c';
    break;
  case IDM_GRPSEARCH:
    *pt = '/';
    break;
  case IDM_GRPREVSEARCH:
    *pt = '\\';
    break;
  case IDM_GRPWRITENEWSRC:
    *pt = 'w';
    break;
  case IDM_GRPEND:
    *pt = 'q';
    break;
  case IDM_GRPQUIT:
    *pt = 'x';
    break;
  case IDM_GRPHELP:
    *pt = '?';
    break;
  default:
    return(0);
  }
  
  win_group_mode_command(pt);
  
  if ( wParam == IDM_GRPLIST )
    set_groupmode_menu();
  
  return(0);
}
void
win_group_mode_command(pt)
char *pt;
{
  int group_mode_command();
  
  switch ( group_mode_command(pt) ) {
  case CONT:
    group_mode_list();
    return;
  case SELECT:			/* subject mode */
    return;
  case END:
    PostMessage(hWnd, WM_SAVENEWSRC,0 , 0L);
    return;
  case QUIT:
    PostQuitMessage(0);
    return;
  case CONNECTION_CLOSED:
    return;
  }
  return;
}
#endif	/* WINDOWS */

/*
 * ニュースグループモード一覧表示
 */

void
group_mode_list()
{
  register int i,count,col;
  register newsgroup_t *ng;
  char buf[128];
  
#ifndef WINDOWS
#ifdef QUICKWIN
  mc_puts("\n");
#else
  if ( gn.cls == ON )
    mc_puts(CLS_STRING);
  else
    mc_puts("\n");
#endif

  mc_puts("ニュースグループモード\n\n");
#else	/* WINDOWS */
  
  kanji_convert(internal_kanji_code,"ニュースグループモード",
		gn.display_kanji_code,buf);
  SetWindowText(hWnd,buf);
  
  fast_mc_puts_start();
#endif	/* WINDOWS */
  
  mc_puts("  番号 記事数 グループ名\n");
  
  while(1){
    ng = newsrc;
    /* skip to top */
    for ( i = 0 ; i < top && ng != 0 ; ng=ng->next) {
      if ( (ng->flag & NOLIST) != 0 )
	continue;
      if ( (ng->flag & (UNSUBSCRIBE | NOARTICLE)) == 0 )
	i++;
      else
	if ( all_flag )
	  i++;
    }
    count = 0;
    start = -1;
    for ( i = top ; ng != 0 ; ng=ng->next) {
      if ( (ng->flag & NOLIST) != 0)
	continue;
      if ( all_flag == 0 ) {
	if ( ng->flag & (UNSUBSCRIBE | NOARTICLE)) /* 購読／記事なし */
	  continue;
	if ( ng->numb <= 0 ) {	/* 新しい記事なし */
	  ng->flag |= NOARTICLE;
	  continue;
	}
      } else {
	if ( ng->last < ng->read ) { /* .newsrc 異常 */
	  ng->read = ng->last;
	  ng->numb = 0;
	}
      }
      if ( start == -1 )
	start = i;		/* 最初の記事 */
      if ( ng->flag & UNSUBSCRIBE ) /* 購読なし */
	mc_printf("U");
      else
	mc_printf(" ");
      mc_printf(" %4d  %4ld  ",i,ng->numb);
#ifdef USE_JNAMES
      mc_puts(jng(ng->name));
#else
      mc_puts(ng->name);
#endif
      if ( gn.description == 1 && ng->desc != 0) {
	col = 2 + 6 + 6 + strlen(ng->name) + 1;
	mc_printf(" ");
	if ( col < COLPOS ) {
	  mc_printf("%*s",COLPOS-col,"");
	  col = COLPOS;
	}
	strcpy(buf,ng->desc);
	str_cut(buf,gn.columns-col);
	
	mc_printf(buf);
      }
      mc_printf("\n");
      i++;
      if ( ++count >= EFFECT_LINES)
	break;
    }
    if ( start == -1 ){
      if ( top == 0 ) {
	mc_puts("\n未読の記事がありません \n\007");
      } else {
	top -= EFFECT_LINES;
	if ( top < 0 )
	  top = 0;
	continue;
      }
    }
    break;
  }
  mc_puts("ニュースグループの番号／名前またはコマンド（help=?）：");
#ifndef	WINDOWS
  fflush(stdout);
#else	/* WINDOWS */
  fast_mc_puts_end();
  
  count = 0;
  for ( ng = newsrc ; ng != 0 ; ng=ng->next) {
    if ( (ng->flag & NOLIST) != 0 )
      continue;
    if ( (ng->flag & (UNSUBSCRIBE | NOARTICLE)) == 0 )
      count++;
    else
      if ( all_flag )
	count++;
  }
  
  SetScrollRange(hWnd,SB_VERT,0,count,FALSE);
  SetScrollPos(hWnd,SB_VERT,top,TRUE);
#endif	/* WINDOWS */
}

/*
 * ニュースグループモードコマンド
 */
int
group_mode_command(pt)
register char *pt;
{
  extern int re_get_active();
  extern void check_group();
  extern int post_mode();
  extern int reference_mode();
  extern int followup_mode();
  extern int cancel_mode();
  extern void shell_mode();
  extern int show_article_id();
  extern void group_list_mode();
  extern void group_unsubscribe();
  extern void group_mode_help();
  extern void kill_mode();
  
  register newsgroup_t *ng;
  register int arg;
  
  /* 数字で始まるのは、ニュースグループの番号 */
  if ( '0' <= *pt && *pt <= '9' )	/* 数字：番号のグループ */
    return(goto_group(atoi(pt)));
  
  /* ニュースグループの名前指定 */
  if ( strlen(pt) >= 2 && pt[1] != ' ' && pt[0] != '<') {
    if ( (ng = search_group(pt)) == (newsgroup_t*)0 ) {
      mc_puts("\n指定のグループがみつかりません \n\007");
#ifndef	WINDOWS
      if ( group_lookup(pt) == QUIT )
	return(CONT);
      pt = kb_input("ニュースグループの番号を入力して下さい");
      while(*pt == ' ' || *pt == '\t' )
	pt++;
      if ( '0' <= *pt && *pt <= '9' ) /* 数字：番号のグループ */
	return(goto_group(atoi(pt)));
#endif /* WINDOWS */
      return(CONT);
    } else {
      return(goto_ng(ng));
    }
  }
  
  if ( pt[1] == 0 ) {		/* １文字だけのコマンド */
    switch ( *pt ) {
    case 0:			/* CRのみ：最初のグループ */
      if ( start == -1 ){
#ifndef	WINDOWS
	mc_puts("\n未読の記事がありません \n\007");
#endif /* WINDOWS */
	return(CONT);
      }
      return(goto_group(start));
    case 'h':			/* ハッシュ */
      for ( ng = newsrc; ng != 0 ; ng=ng->next)
	ng->flag &= ~(NOARTICLE|ACTIVE);
      /* アクティブファイルの取得 */
      switch ( re_get_active() ) {
      case CONNECTION_CLOSED:
	return(CONNECTION_CLOSED);
      case ERROR:
	return(QUIT);
      }
      check_group(0);
      top = 0;
      return(CONT);
    case 'i':			/* ポスト */
      return(post_mode(NULL));
    case 'R':			/* reference */
      return(reference_mode());
    case 'f':			/* フォロー */
      return(followup_mode());
    case 'r':			/* リプライ */
      reply_mode();
      return(CONT);
    case 'm':			/* メイル */
      mail_mode();
      return(CONT);
    case 'C':			/* キャンセル */
      return(cancel_mode());
    case 'K':
      kill_mode();		/* KILL */
      return(CONT);
    case 'w':			/* .newsrc の保存、継続 */
      if ( change ) {
	save_newsrc();		/* .newsrc の保存 */
	change = 0;
      }
      return(CONT);
    case 'q':			/* 終了 */
      return(END);
    case 'x':			/* newsrc を更新せず終了 */
      return(QUIT);
    case '?':			/* help */
      group_mode_help();
      return(CONT);
    }
  }
  
  /* 引数をとるコマンド */
  switch ( *pt ) {
  case 'p':			/* 前画面 */
    pt++;
    while (*pt == ' ')
      pt++;
    arg = atoi(pt);
    if ( arg <= 0 )
      top -= EFFECT_LINES;
    else
      top -= arg;
    
    if ( top < 0 )
      top = 0;
    
    return(CONT);
    
  case 'n':			/* 次画面 */
    pt++;
    while (*pt == ' ')
      pt++;
    arg = atoi(pt);
    if ( arg <= 0 )
      top += EFFECT_LINES;
    else
      top += arg;
    
    return(CONT);
    
  case 'l':			/* 全グループ表示 */
    group_list_mode(pt);
    return(CONT);

#ifndef	NO_SAVE
  case 's':			/* セーブ */
    save_mode(pt);
    return(CONT);
#endif /* NO_SAVE */

  case 'u':			/* もう読まない */
    group_unsubscribe(pt);
    return(CONT);
    
  case 'S':			/* やっぱり読む */
    group_subscribe(pt);
    return(CONT);
    
  case 'c':			/* 全部読んだことにする */
    group_catchup(pt);
    return(CONT);
    
#if ! defined(QUICKWIN) && ! defined(WINDOWS)
#ifndef	NO_SHELL
  case '!':			/* SHELL */
    shell_mode(pt);
    return(CONT);
#endif /* NO_SHELL */
#ifndef	NO_PIPE
  case '|':			/* PIPE */
    pipe_mode(pt);
    return(CONT);
#endif /* NO_PIPE */
#endif
    
  case '<':			/* Message ID */
    return(show_article_id(pt));
    
  case '/':			/* ニュースグループの文字列検索 */
    group_search(pt);
    return(CONT);
    
  case '\\':			/* ニュースグループの文字列検索（逆順）*/
    group_search_rev(pt);
    return(CONT);
    
  default:
#ifdef	WINDOWS
    MessageBeep(MB_OK);
#else  /* WINDOWS */
    putchar(BEL);
#endif /* WINDOWS */
    return(CONT);
  }
}

static int
goto_group(num)
int num;
{
  register newsgroup_t *ng;
  register int i;
  
  ng = newsrc;
  for ( i = 0 ; ng != 0 ; ng=ng->next) {
    if ( (ng->flag & NOLIST) != 0)
      continue;
    if ( all_flag == 0 && (ng->flag & (UNSUBSCRIBE|NOARTICLE)) )
      continue;			/* 購読なし */
    if ( i == num )
      return(goto_ng(ng));
    i++;
  }
#ifdef	WINDOWS
  mc_MessageBox(hWnd,"指定された番号のグループが存在しません ","",MB_OK);
#else	/* Windows */
  mc_puts("\n指定された番号のグループが存在しません \n");
  hit_return();
#endif
  return(CONT);
}

static int
goto_ng(ng)
newsgroup_t *ng;
{
  extern int group_command();
  extern int subject_mode();
  
  switch (group_command(ng->name)) {
  case CONNECTION_CLOSED:
    return(CONNECTION_CLOSED);
  case CONT:
    break;
  default:
    return(CONT);
  }
#ifdef	WINDOWS
  PostMessage(hWnd, WM_ENTERSUBJECTMODE,0 , (long)ng);
  return(SELECT);
#else	/* WINDOWS */
  return(subject_mode(ng));
#endif	/* WINDOWS */
}
/*
 * change display mode
 */
void
group_list_mode(pt)
char *pt;
{
  extern int check_regexp_group();
  extern int regexp_match();
  
  regexp_group_t *groups;
  register newsgroup_t *ng;
  register int flag = OFF;
  
  top = 0;
  
  /* clear NOLIST flag */
  for ( ng = newsrc; ng != 0 ; ng = ng->next) {
    if ( (ng->flag & NOLIST) != 0 ) {
      flag = ON;
      ng->flag &= ~NOLIST;
    }
  }
  
  pt++;	/* 'l' */
  while( *pt == ' ' || *pt == '\t' )	/* skip white space */
    pt++;
  
  if ( *pt == 0 ) {		/* 'l' only */
    if ( flag == ON )
      return;
    all_flag ^= 1;		/* change mode */
    return;
  }
  
  if ( check_regexp_group(pt,&groups) == ERROR ) {
    /* need error message */
    return;
  }
  
  for ( ng = newsrc; ng != 0 ; ng = ng->next) {
    if ( regexp_match(ng->name,groups) != YES )
      ng->flag |= NOLIST;
  }
}

/*
 * 購読しない
 */
void
group_unsubscribe(pt)
char *pt;
{
#ifdef	WINDOWS
  BOOL __export CALLBACK group_unsubscribe_conf();
#endif	/* 	WINDOWS */
#ifndef	WINDOWS
  char *key_buf;
#endif	/* 	WINDOWS */
  char resp[NNTP_BUFSIZE+1];
  register newsgroup_t *ng;
  long l;
  long st,ed;
  char *wk;
  int	c;
  
  pt++;	/* 'u' */
  while( *pt == ' ' || *pt == '\t' )
    pt++;
  
  if ( *pt == 0 ) {
    if ( (pt = kb_input("購読しなくするニュースグループの番号（範囲）、または名前を指定して下さい")) == NULL )
      return;
    while( *pt == ' ' || *pt == '\t' )
      pt++;
  }
  
  /* ＮＧの番号指定  */
  if ( ('0' <= *pt && *pt <= '9') || *pt == '-' ) {
    if ( get_area(pt,&st,&ed) != CONT )
      return;
    if ( st == -1 )
      st = 0;
    if ( ed == -1 ) {
      for ( ng = newsrc,ed = 0 ; ng != 0 ; ng = ng->next) {
	if ( (ng->flag & NOLIST) != 0 )
	  continue;
	if ( all_flag == 0 &&
	    (ng->flag & (UNSUBSCRIBE|NOARTICLE)) ) /* 購読なし */
	  continue;
	ed++;
      }
    }
    
    /* 最初のＮＧを探す */
    for ( ng = newsrc,l = 0 ; ng != 0 ; ng = ng->next) {
      if ( (ng->flag & NOLIST) != 0 )
	continue;
      if ( all_flag == 0 &&
	  (ng->flag & (UNSUBSCRIBE|NOARTICLE)) ) /* 購読なし */
	continue;
      if ( l == st )
	break;
      l++;
    }
    if ( l != st || ng == 0){
#ifdef	WINDOWS
      mc_MessageBox(hWnd,"指定された番号のグループが存在しません ","",MB_OK);
#else  /* WINDOWS */
      mc_puts("\n指定された番号のグループが存在しません \n");
      hit_return();
#endif /* WINDOWS */
      return;
    }
    
    /* 確認入力 */
    for ( l = st ; l <= ed ; l++) {
      if ( (ng->flag & NOLIST) != 0 )
	continue;
      if ( all_flag != 0 ||
	  ( ng->flag & (UNSUBSCRIBE|NOARTICLE)) == 0 ){
#ifndef	WINDOWS
	sprintf(resp,
		"%s はもうリストアップしません(y:了解/a:全部/その他:中止)",
		ng->name);
	key_buf = kb_input(resp);
	c = *key_buf;
#else  /* WINDOWS */
	sprintf(group_name,
		"%s はもうリストアップしません ",ng->name);
	c = DialogBox(hInst,	/* 現在のインスタンス */
		      "IDD_GROUP_UNSUBSCRIBE_CONF", /* リソース */
		      hWnd,	/* 親ウィンドウのハンドル */
		      group_unsubscribe_conf);
	
#endif /* WINDOWS */
	if ( c == 'a' || c == 'A' ){
	  ng->flag |= UNSUBSCRIBE;
	  change = 1;
	  l++;
	  for ( ; l <= ed ; l++) {
	    ng = ng->next;
	    for ( ; ng != 0 ; ng = ng->next ){
	      if ( (ng->flag & NOLIST) != 0 )
		continue;
	      if ( all_flag == 0 &&
		  (ng->flag & (UNSUBSCRIBE|NOARTICLE)) )
		continue;
	      break;
	    }
	    if ( ng == 0 )
	      return;
	    ng->flag |= UNSUBSCRIBE;
	  }
	  return;
	}
	if ( c == 'y' || c == 'Y' ){
	  ng->flag |= UNSUBSCRIBE;
	  change = 1;
	}
      }
      ng = ng->next;
      for ( ; ng != 0 ; ng = ng->next ){
	if ( (ng->flag & NOLIST) != 0 )
	  continue;
	if ( all_flag == 0 &&
	    (ng->flag & (UNSUBSCRIBE|NOARTICLE)) ) /* 購読なし */
	  continue;
	break;
      }
      if ( ng == 0 )
	break;
    }
    return;
  }
  
  /* ＮＧの名前指定 */
  if ( (wk = strchr(pt,'\n')) != 0 )
    *wk = 0;
  
  if ( (ng = search_group(pt)) == (newsgroup_t*)0 ) {
#ifdef	WINDOWS
    mc_MessageBox(hWnd,"指定のグループがみつかりません ","",MB_OK);
#else  /* WINDOWS */
    mc_puts("\n指定のグループがみつかりません \n\007");
    hit_return();
#endif /* WINDOWS */
  } else {
    if ( ( ng->flag & UNSUBSCRIBE) == 0 ){
#ifndef	WINDOWS
      sprintf(resp,
	      "%s はもうリストアップしません(y:了解/その他:中止)",
	      ng->name);
      key_buf = kb_input(resp);
      if ( *key_buf == 'y' || *key_buf == 'Y' ){
	ng->flag |= UNSUBSCRIBE;
	change = 1;
      }
#else  /* WINDOWS */
      sprintf(resp,
	      "%s はもうリストアップしません ",
	      ng->name);
      if ( mc_MessageBox(hWnd,resp,"",MB_OKCANCEL) == IDOK ) {
	ng->flag |= UNSUBSCRIBE;
	change = 1;
      }
#endif /* WINDOWS */
    }
  }
}
#ifdef	WINDOWS
BOOL __export CALLBACK
group_unsubscribe_conf(hDlg, message, wParam, lParam)
HWND hDlg;                       /* ダイアログボックスウィンドウのハンドル */
unsigned message;                /* メッセージのタイプ                     */
WORD wParam;                     /* メッセージ固有の情報                   */
LONG lParam;
{
  char sendtext[255];
  
  switch (message) {
  case WM_INITDIALOG:
    kanji_convert(internal_kanji_code,group_name,
		  gn.display_kanji_code,sendtext);
    SetDlgItemText(hDlg, IDC_UNSUBSCRIBE,(LPSTR)sendtext);
    kanji_convert(internal_kanji_code,"はい(\036Y)",
		  gn.display_kanji_code, sendtext);
    SetDlgItemText(hDlg, IDOK, sendtext);
    kanji_convert(internal_kanji_code,"いいえ(\036N)",
		  gn.display_kanji_code, sendtext);
    SetDlgItemText(hDlg, IDCANCEL, sendtext);
    kanji_convert(internal_kanji_code,"全部(\036A)",
		  gn.display_kanji_code, sendtext);
    SetDlgItemText(hDlg, IDC_ALL, sendtext);
    return TRUE;

  case WM_COMMAND:
    switch (wParam) {
    case IDOK:
      EndDialog(hDlg, 'y');
      return(TRUE);

    case IDCANCEL:
      EndDialog(hDlg, 'n');
      return(TRUE);

    case IDC_ALL:
      EndDialog(hDlg, 'a');
      return(TRUE);

    }
    break;
  }
  return FALSE;
}
#endif	/* WINDOWS */

/*
 * 購読する
 */
static void
group_subscribe(pt)
char *pt;
{
  newsgroup_t *ng;
  char *wk;
  long l;
  long st,ed;
  
  pt++;
  while( *pt == ' ' || *pt == '\t' )
    pt++;
  
  if ( *pt == 0 ) {
    if ((pt = kb_input("購読するニュースグループの番号（範囲）、または名前を指定して下さい")) == NULL)
      return;
    while( *pt == ' ' || *pt == '\t' )
      pt++;
  }
  
  /* ＮＧの番号指定  */
  if ( ('0' <= *pt && *pt <= '9') || *pt == '-' ) {
    if ( get_area(pt,&st,&ed) != CONT )
      return;
    if ( st == -1 )
      st = 0;
    if ( ed == -1 ) {
      ed = 0;
      for ( ng = newsrc ; ng != 0 ; ng = ng->next)
	ed++;
    }
    
    /* 最初のＮＧを探す */
    for ( ng = newsrc,l = 0 ; ng != 0 ; ng = ng->next) {
      if ( (ng->flag & NOLIST) != 0 )
	continue;
      if ( all_flag == 0 &&
	  (ng->flag & (UNSUBSCRIBE|NOARTICLE)) ) /* 購読なし */
	continue;
      if ( l == st )
	break;
      l++;
    }
    if ( l != st || ng == 0){
#ifdef	WINDOWS
      mc_MessageBox(hWnd,"指定された番号のグループが存在しません ","",MB_OK);
#else  /* WINDOWS */
      mc_puts("\n指定された番号のグループが存在しません \n");
      hit_return();
#endif /* WINDOWS */
      return;
    }
    
    for ( l = st ; l <= ed && ng != 0; l++,ng = ng->next ){
      if ( ( ng->flag & UNSUBSCRIBE) != 0 ){
	ng->flag &= ~UNSUBSCRIBE;
	change = 1;
      }
    }
    return;
  }
  
  /* ＮＧの名前指定 */
  if ( (wk = strchr(pt,'\n')) != 0 )
    *wk = 0;
  
  if ( (ng = search_group(pt)) == (newsgroup_t*)0 ) {
#ifdef	WINDOWS
    mc_MessageBox(hWnd,"指定のグループがみつかりません ","",MB_OK);
#else  /* WINDOWS */
    mc_puts("\n指定のグループがみつかりません \n\007");
    hit_return();
#endif /* WINDOWS */
  } else {
    if ( ( ng->flag & UNSUBSCRIBE) != 0 ){
      ng->flag &= ~UNSUBSCRIBE;
      change = 1;
    }
  }
}
/*
 * 全部読んだことにする
 */
static void
group_catchup(pt)
char *pt;
{
  char resp[NNTP_BUFSIZE+1];
  newsgroup_t *ng;
  long l;
  long st,ed;
  char *wk;
  
  pt++;
  while( *pt == ' ' || *pt == '\t' )
    pt++;
  
  if ( *pt == 0 ) {
    if ((pt = kb_input("ニュースグループの番号、または名前を指定して下さい")) == NULL)
      return;
    while( *pt == ' ' || *pt == '\t' )
      pt++;
  }
  
  /* ＮＧの番号指定  */
  if ( ('0' <= *pt && *pt <= '9') || *pt == '-') {
    if ( get_area(pt,&st,&ed) != CONT )
      return;
    if ( st == -1 )
      st = 0;
    if ( ed == -1 ) {
      for ( ng = newsrc,ed = 0 ; ng != 0 ; ng = ng->next) {
	if ( (ng->flag & NOLIST) != 0 )
	  continue;
	
	if ( all_flag == 0 &&
	    (ng->flag & (UNSUBSCRIBE|NOARTICLE)) ) /* 購読なし */
	  continue;
	ed++;
      }
    }
    
    /* 最初のＮＧを探す */
    for ( ng = newsrc,l = 0 ; ng != 0 ; ng = ng->next) {
      if ( (ng->flag & NOLIST) != 0 )
	continue;
      
      if ( all_flag == 0 &&
	  (ng->flag & (UNSUBSCRIBE|NOARTICLE)) ) /* 購読なし */
	continue;
      if ( l == st )
	break;
      l++;
    }
    if ( l != st || ng == 0){
#ifdef	WINDOWS
      mc_MessageBox(hWnd,"指定された番号のグループが存在しません ","",MB_OK);
#else  /* WINDOWS */
      mc_puts("\n指定された番号のグループが存在しません \n");
      hit_return();
#endif /* WINDOWS */
      return;
    }
    
    for ( l = st ; l <= ed ; l++) {
      if ( ng->read != ng->last ){
	ng->read = ng->last;
	ng->numb = 0;
	change = 1;
      }
      ng = ng->next;
      for ( ; ng != 0 ; ng = ng->next ){
	if ( (ng->flag & NOLIST) != 0 )
	  continue;
	
	if ( all_flag == 0 &&
	    (ng->flag & (UNSUBSCRIBE|NOARTICLE)) ) /* 購読なし */
	  continue;
	break;
      }
      if ( ng == 0 )
	break;
    }
    return;
  }
  
  /* ＮＧの名前指定 */
  if ( (wk = strchr(resp,'\n')) != 0 )
    *wk = 0;
  
  if ( (ng = search_group(pt)) == (newsgroup_t*)0 ) {
#ifdef	WINDOWS
    mc_MessageBox(hWnd,"指定のグループがみつかりません ","",MB_OK);
#else  /* WINDOWS */
    mc_puts("\n指定のグループがみつかりません \n\007");
    hit_return();
#endif /* WINDOWS */
    return;
  }
  
  if ( ng->read != ng->last ){
    ng->read = ng->last;
    ng->numb = 0;
    change = 1;
  }
}
/*
 * ニュースグループの文字列検索
 */
static void
group_search(pt)
char *pt;
{
  extern int kb_search();
  extern int sregcmp();
  
  register int i;
  register newsgroup_t *ng;
  
  if (kb_search(sea_grpname, pt) == QUIT )
    return;
  
  ng = newsrc;
  for( i = 0 ; i < top && ng != 0 ; ng=ng->next) {
    if ( (ng->flag & NOLIST) != 0 )
      continue;
    
    if ( (ng->flag & (UNSUBSCRIBE | NOARTICLE)) == 0 )
      i++;
    else
      if ( all_flag )
	i++;
  }
  for( i = top ; ng != 0 ; ng=ng->next) {
    if ( (ng->flag & NOLIST) != 0 )
      continue;
    
    if ( all_flag == 0 ) {
      if ( ng->flag & (UNSUBSCRIBE | NOARTICLE)) /* 購読／記事なし */
	continue;
      if ( ng->numb <= 0 ) {	/* 新しい記事なし */
	ng->flag |= NOARTICLE;
	continue;
      }
    } else {
      if ( ng->last < ng->read ) { /* .newsrc 異常 */
	ng->read = ng->last;
	ng->numb = 0;
      }
    }
    if( i != top && sregcmp(ng->name,sea_grpname) ) {
      top = i;
      return;
    }
    i++;
  }
  mc_printf("文字列 \"%s\" は、ありません \007\n",sea_grpname);
}
/*
 * ニュースグループの文字列検索（逆順）
 */
static void
group_search_rev(pt)
char *pt;
{
  extern int kb_search();
  extern int sregcmp();
  
  register int i,tmptop;
  register newsgroup_t *ng;
  
  if (kb_search(sea_grpname, pt) == QUIT )
    return;
  
  tmptop = -1;
  ng = newsrc;
  for( i = 0 ; i < top && ng != 0 ; ng=ng->next) {
    if ( (ng->flag & NOLIST) != 0 )
      continue;
    
    if ( all_flag == 0 ) {
      if ( ng->flag & (UNSUBSCRIBE | NOARTICLE)) /* 購読／記事なし */
	continue;
      if ( ng->numb <= 0 ) {	/* 新しい記事なし */
	ng->flag |= NOARTICLE;
	continue;
      }
    } else {
      if ( ng->last < ng->read ) { /* .newsrc 異常 */
	ng->read = ng->last;
	ng->numb = 0;
      }
    }
    if( i != top && sregcmp(ng->name,sea_grpname) )
      tmptop = i;
    i++;
  }
  
  if ( tmptop == -1 ) {
    mc_printf("文字列 \"%s\" は、ありません \007\n",sea_grpname);
  } else {
    top = tmptop;
  }
}

/*
 * 検索文字列の入力
 */
int
kb_search(sea_name, pt)
char *sea_name, *pt;
{
  register char *ptmp;
  
  pt++;	/* '/' */
  while ( *pt == ' ' || *pt == '\t' )
    pt++;
  
  if( *pt == 0 ) {
    if( *sea_name == '\0') {
      if ( (pt = kb_input("検索する文字列を指定して下さい")) == NULL )
	return(QUIT);
      while( *pt == ' ' || *pt == '\t' )
	pt++;
      ptmp = pt + strlen(pt);
      while( *ptmp == ' ' || *ptmp == '\t' )
	ptmp--;
      *ptmp = '\0';
      strcpy(sea_name, pt);
    }
  } else {
    strcpy(sea_name, pt);
  }
  return(CONT);
}
int
sregcmp(s1, s2)
register char *s1, *s2;
{
  register char *t1, *t2;
  
  if(*s2 == '\0')
    return(0);
  while((s1 = strchr(s1, *s2)) != '\0') {
    t1 = ++s1;
    t2 = s2 + 1;
    while(*t1 == *t2 || t2 == 0) {
      t1++;
      t2++;
    }
    if(*t2 == '\0')
      return(1);
  }
  return(0);
}

/*
 * 似ているニュースグループ名を探す
 */
#define	MAXLIST	10
static int
group_lookup(name)
register char *name;
{
  register newsgroup_t *ng;
  register int numb,para,i;
  struct _list {
    int numb;
    int para;
    newsgroup_t *ng;
  } list[MAXLIST];
  int list_count;		/* リストする行数 */
  
  for ( i = 0 ; i < MAXLIST ; i++ ) {
    list[i].numb= -1;
    list[i].para= -1;
    list[i].ng= 0;
  }
  
  list_count = gn.lines - 2;
  if ( list_count > MAXLIST )
    list_count = MAXLIST;
  
  for ( numb = -1, ng = newsrc ; ng != 0 ; ng=ng->next) {
    if ( (ng->flag & NOLIST) != 0 )
      continue;
    
    if ( all_flag == 0 && (ng->flag & (UNSUBSCRIBE|NOARTICLE)) )
      continue;			/* 購読なし */
    numb++;
    if ( (para = fstrcmp(ng->name,name)) <= 0)
      continue;			/* 全然一致しない */
    
    if ( list[list_count-1].para > para )
      continue;
    
    list[list_count-1].numb = numb;
    list[list_count-1].para = para;
    list[list_count-1].ng = ng;
    
    for ( i = list_count-2; i >= 0 ; i--) {
      if ( list[i].para >= para )
	break;
      list[i+1] = list[i];
      list[i].numb = numb;
      list[i].para = para;
      list[i].ng = ng;
    }
  }
  if ( list[0].numb == -1 )
    return(QUIT);
  
  for ( i = 0 ; i < list_count ; i++ ) {
    if ( list[i].numb == -1 )
      break;
    if ( list[i].ng->flag & UNSUBSCRIBE ) /* 購読なし */
      mc_printf("U");
    else
      mc_printf(" ");
    mc_printf(" %4d  %4ld  ",list[i].numb,list[i].ng->numb);
    mc_puts(list[i].ng->name);
    mc_printf("\n");
  }
  return(CONT);
}

#define	MAXPOINT	30	/* 完全に一致した時の点数 */
#define SEARCH_CHAR	5	/* ずれを探す文字数 */
#define	MINPOINT	10	/* ずれた時の点数初期値 */
#define	MINPOINTDEC	1	/* ずれた時の点数減算値 */

static int
fstrcmp(a, b)
register char *a,*b;
{
  register char *pt;
  register int point = 0;
  register int i,diff;
  
  while ( *a == *b && *a != 0){
    point += MAXPOINT;
    a++;
    b++;
  }
  
  point -= strlen(a);	/* 文字数の差が少ない方に高得点を与えたい */
  point -= strlen(b);
  
  while ( *a != 0 && *b != 0 ) {

    pt = b;
    diff = MINPOINT;
    for ( i = SEARCH_CHAR ; i>0 ; i--){
      if ( *pt == 0 )
	break;
      if ( *a == *pt++ )
	point += diff;
      diff -= MINPOINTDEC;
    }
    pt = a;
    diff = MINPOINT;
    for ( i = MINPOINT ; i>0 ; i--){
      if ( *pt == 0 )
	break;
      if ( *b == *pt++ )
	point += diff;
      diff -= MINPOINTDEC;
    }
    a++;
    b++;
  }
  return(point);
}
#ifdef USE_JNAMES
static char *
jng(ng)
char *ng;
{
  char *jname;
  char *name;
  static char tmp[256];
  
  strcpy(tmp, ng);
  name = strtok(ng, " ");
  
  if(NULL != (jname = jnFetch("newsgroupname", name)))
    sprintf(tmp, "%s", jname);
  return tmp;
}
#endif /* USE_JNAMES */
void
group_mode_help()
{
  extern void gn_help();
  static char *os = (char *)OS;
  static char rcfile_tmp[PATH_LEN+1];
  static char *rcfile_tmp_p = rcfile_tmp;
  static struct help_t help_tbl[] = {
    { "ニュースグループモード",0 },
    { "",0 },
    { "<CR>のみ      :最初のグループを選択 ", 0 },
    { "数字<CR>      :指定した番号のグループを選択 ", 0 },
    { "名前<CR>      :指定した名前のグループを選択 ", 0 },
    { "p [数]<CR>    :前画面（もしくは指定した数だけ上）を表示する ", 0 },
    { "n [数]<CR>    :次画面（もしくは指定した数だけ下）を表示する ", 0 },
    { "h<CR>         :新しい記事をチェックする ", 0 },
    { "l [NG]<CR>    :表示モードを切り替える ", 0 },
    { "i<CR>         :記事をポストする ", 0 },
    { "ＩＤ<CR>      :指定したメッセージＩＤの記事を表示する ", 0 },
    { "R<CR>         :最後に読んだ記事の元記事を表示する ", 0 },
    { "f<CR>         :最後に読んだ記事にフォロー記事を出す ", 0 },
    { "r<CR>         :最後に読んだ記事にリプライメイルを出す ", 0 },
    { "C<CR>         :最後に読んだ記事をキャンセルする ", 0 },
#ifndef	NO_SAVE
    { "s [file]<CR>  :最後に読んだ記事をファイルに保存する ", 0 },
#endif /* NO_SAVE */
#if ! defined(QUICKWIN) && ! defined(WINDOWS)
#ifndef	NO_PIPE
    { "| [cmd]<CR>   :最後に読んだ記事をコマンドに渡す ", 0 },
#endif /* NO_PIPE */
#endif /* ! QUICKWIN && ! WINDOWS  */
    { "m<CR>         :メイルを送信する ", 0 },
    { "u [NG]<CR>    :指定したグループはもう読まない ", 0 },
    { "S [NG]<CR>    :指定したグループはやっぱり読む ", 0 },
    { "c [NG]<CR>    :指定したグループの記事を全部読んだことにする ", 0 },
    { "/ [文字列]<CR>:文字列によるニュースグループ名検索 ", 0 },
    { "\\ [文字列]<CR>:文字列によるニュースグループ名逆検索 ", 0 },
#if ! defined(QUICKWIN) && ! defined(WINDOWS)
#ifndef	NO_SHELL
    { "! [cmd]<CR>   :%s コマンドを実行する ",&os},
#endif /* NO_SHELL */
#endif /* ! QUICKWIN && ! WINDOWS */
    { "w<CR>         :既読情報を %s に保存する ",&rcfile_tmp_p},
    { "q<CR>         :終了 ", 0 },
    { "x<CR>         :%s を更新せず終了 ",&rcfile_tmp_p},
    { "?<CR>         :このヘルプ ", 0 },
#ifdef WINDOWS
    { "",0 },
    { "グループ表示の上で左ボタンダブルクリック  :そのグループを選択 ",0 },
    { "グループ表示のないところで左ボタンクリック:最初のグループを選択 ",0 },
    { "右ボタンクリック                          :終了",0 },
#endif /* WINDOWS */
    { 0,0 }
  };
  
  if ( strncmp(gn.rcfile,gn.home,strlen(gn.home)) == 0 ) {
    strcpy(rcfile_tmp,"~/");
    strcat(rcfile_tmp,&gn.rcfile[strlen(gn.home)+1]);
  } else {
    strcpy(rcfile_tmp,gn.rcfile);
  }
  gn_help(help_tbl);
}
void
gn_help(help_tbl)
struct help_t *help_tbl;
{
#ifdef	WINDOWS
  extern char *help_file;		/* temp file for help */
#endif /* WINDOWS */

  register struct help_t *hp;
  FILE *fp;
  char tmpfile[512],buf[128],buf2[128];
  int	help_lines;

#ifdef	WINDOWS
  if ( help_file != 0 ) {
    unlink(help_file);
    free(help_file);
    help_file = 0;
  }
#endif /* WINDOWS */

  strcpy(tmpfile,gn.tmpdir);
  strcat(tmpfile,"gnXXXXXX");
  mktemp(tmpfile);
  
  if ( ( fp = fopen(tmpfile,"w") ) == NULL ){
#ifdef	WINDOWS
    mc_MessageBox(hWnd,"テンポラリファイルの作成に失敗しました",
		  tmpfile,MB_OK);
#else  /* WINDOWS */
    perror(tmpfile);
#endif /* WINDOWS */
    return;
  }
  
  help_lines = 0;
  for ( hp = help_tbl; hp->msg != 0 ; hp++ ) {
    if ( hp->val != 0 ) {
      sprintf(buf2,hp->msg,*hp->val);
      kanji_convert(internal_kanji_code,buf2,gn.process_kanji_code,buf);
    } else {
      kanji_convert(internal_kanji_code,hp->msg,gn.process_kanji_code,buf);
    }
    fprintf(fp,"%s\n",buf);
    help_lines ++;
  }
  
  if ( fclose(fp) == EOF ) {
#ifdef	WINDOWS
    mc_MessageBox(hWnd,"テンポラリファイルの作成に失敗しました",
		  tmpfile,MB_OK);
#else  /* WINDOWS */
    perror(tmpfile);
#endif /* WINDOWS */
    return;
  }
  
#ifdef	WINDOWS
  win_pager(tmpfile,help_lines);
  help_file = malloc(strlen(tmpfile)+1);
  strcpy(help_file,tmpfile);
  return;
#else	/* WINDOWS */
#ifdef	QUICKWIN
  qw_pager(tmpfile);
#else	/* QUICKWIN */
  sprintf(buf,"%s %s",gn.pager,tmpfile);
  system(buf);
  if ( gn.return_after_pager != 0 )
    hit_return();
#endif	/* QUICKWIN */
#endif	/* WINDOWS */
  unlink(tmpfile);
}
