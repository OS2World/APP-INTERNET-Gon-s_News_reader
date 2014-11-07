/*
 * @(#)article.c	1.40 Jan.7,1998
 *
 * 著作権は放棄しません。ただし、営利目的以外の使用／配布に制限は設けません。
 * Copyright (C) yamasita@omronsoft.co.jp
 */

#if defined(WINDOWS)
#	include	<windows.h>
#	include	"wingn.h"
#endif	/* WINDOWS */

#include <stdio.h>
#include <ctype.h>

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

#include	"nntp.h"
#include	"gn.h"

extern char *kb_input();
extern void reply_mode();
extern void mail_mode();
extern void save_mode();
extern void pipe_mode();
extern char *expand_filename();
extern char *article_filename();
extern int get_1_header_file();
extern void mc_puts(),mc_printf();
extern void hit_return();
extern void kanji_convert();
extern int kb_search();
extern int put_server();
extern int sregcmp();
extern int get_server();
extern int get_1_header();
extern newsgroup_t *search_group();
extern void set_unreaded();
extern void str_cut();
extern void gn_help();
extern char *Fgets();
extern void memory_error();

#if defined(WINDOWS)
extern int mc_MessageBox();
extern int setmenu();
extern void fast_mc_puts_start(),fast_mc_puts_end();
extern void set_hourglass(),reset_cursor();
#endif	/* WINDOWS */

#ifdef	MIME
extern void MIME_strHeaderDecode();
#endif


static newsgroup_t *ng;	/* カレントニュースグループリスト */
static subject_t *sb;	/* カレントサブジェクトリスト */
static int top;			/* リストする最初の行番号 */
static long start;		/* リストされた最初の未読記事番号 */
static int autolist_sv;

static char article_not_found[] = "\n指定の記事が存在しません \n\007";
static char sea_artname[80] = "";

static void article_unread(),article_search(), article_search_rev(),
	xref_mark(),search_child();
static search_ref(),gmt_to_jst();

/* アーティクルモード */
#ifndef	WINDOWS
int
article_mode(group,subject)
newsgroup_t *group;		/* カレントニュースグループリスト */
register subject_t *subject;	/* カレントサブジェクトリスト */
{
  void article_make_thread();
  void article_make_order();
  void article_mode_list();
  int article_mode_command();
  
  char key_buf[KEY_BUF_LEN];
  register char *pt;
  autolist_sv = gn.autolist;
  gn.autolist = 1;
  
  ng = group;
  sb = subject;
  top = 0;
  
  if ( disp_mode == THREADED ) {
    if ((sb->flag & THREADED) == 0 ){
      article_make_thread(sb);
      sb->flag |= THREADED;
    }
  } else {
    if ((sb->flag & THREADED) != 0 ){
      article_make_order(sb);
      sb->flag &= ~THREADED;
    }
  }
  
  while(1) {
    /* 一覧表示 */
    article_mode_list();
    gn.autolist = autolist_sv;

    /* コマンド入力 */
    memset(key_buf,0,KEY_BUF_LEN);
    redraw_func = article_mode_list;
    while ( FGETS(key_buf,KEY_BUF_LEN,stdin) == NULL ) {
      clearerr(stdin);
      article_mode_list();
    }
    redraw_func = (void (*)())NULL;
    pt = &key_buf[strlen(key_buf)-1];
    while( pt >= key_buf && ( *pt == ' ' || *pt == '\t' || *pt == '\n' ) )
      *pt-- = 0;

    /* 先行するホワイトスペースをスキップ */
    pt = key_buf;
    while(*pt == ' ' || *pt == '\t' )
      pt++;


    /* コマンド解析 */
    switch ( article_mode_command(pt) ) {
    case END:			/* a コマンドで、全部読んだ / unsub */
      return(END);
    case QUIT:			/* 未読の記事なし */
      return(CONT);
    case NEXT:			/* 次のサブジェクト */
      break;
    case SELECT:
      return(SELECT);
    case CONNECTION_CLOSED:
      return(CONNECTION_CLOSED);
    }
  }
}
#else	/* WINDOWS */
article_mode(group,subject)
newsgroup_t *group;		/* カレントニュースグループリスト */
register subject_t *subject;	/* カレントサブジェクトリスト */
{
  extern void article_make_thread();
  extern void article_make_order();

  int autolist_sv;

  autolist_sv = gn.autolist;
  gn.autolist = 1;

  ng = group;
  sb = subject;
  top = 0;

  if ( disp_mode == THREADED ) {
    if ((sb->flag & THREADED) == 0 ){
      article_make_thread(sb);
      sb->flag |= THREADED;
    }
  } else {
    if ((sb->flag & THREADED) != 0 ){
      article_make_order(sb);
      sb->flag &= ~THREADED;
    }
  }

  return(CONT);
}
set_articlemode_menu()
{
  static gnpopupmenu_t articlemode_file_menu[] = {
#ifndef	NO_SAVE
    { "最後に読んだ記事をファイルに保存する(\036S)",
	IDM_SAVE,		AFTER_READ|SEPARATOR_NEXT },
#endif /* NO_SAVE */
    { "前のサブジェクトへ(\036P)",
	IDM_ARTPREVSUB,		0 },
    { "次のサブジェクトへ(\036N)",
	IDM_ARTNEXTSUB,		0 },
    { "サブジェクトモードへ(\036Q)",
	IDM_ARTQUIT,			0 },
    { 0,0,0 }
  };
  
  static gnpopupmenu_t articlemode_edit_menu[] = {
    { "記事番号、メッセージＩＤの指定...",
	IDM_ARTCOMMAND,		SEPARATOR_NEXT },

    { "投稿者の検索(\036/)...",
	IDM_ARTSEARCH,		0 },
    { "投稿者の逆検索(\036\\)...",
	IDM_ARTREVSEARCH,		SEPARATOR_NEXT },

    { "このグループはもう読まない(\036U)",
	IDM_ARTUNSUBSCRIBE,	0 },
    { "このグループはやっぱり読む(\036S)",
	IDM_ARTSUBSCRIBE,		SEPARATOR_NEXT },

    { "未読の記事を全部読んだことにする(\036C)",
	IDM_ARTCATCHUP,		0 },
    { "指定した番号の記事を未読にする...",
	IDM_ARTUNREAD,		0 },
    { 0,0,0 }
  };
  
  static gnpopupmenu_t articlemode_view_menu[] = {
    { "前画面を表示する(\036P)",
	IDM_ARTPREV,			0 },
    { "次画面を表示する(\036N)",
	IDM_ARTNEXT,			0 },
    { "",			/* programable */
	IDM_ARTTRUNC,			SEPARATOR_NEXT },

    { "",			/* programable */
	IDM_ARTLIST,			SEPARATOR_NEXT },

    { "最後に読んだ記事の元記事を表示する(\036R)",
	IDM_REFERENCE,		AFTER_READ|SEPARATOR_NEXT },

    { "未読の全記事を選択する(\036A)",
	IDM_ARTALL,			0 },
    { 0,0,0 }
  };
  
  
  static gnpopupmenu_t articlemode_send_menu[] = {
    { "最後に読んだ記事にフォロー記事を出す(\036F)",
	IDM_FOLLOW,			AFTER_READ },
    { "最後に読んだ記事にリプライメイルを出す(\036R)",
	IDM_REPLY,			AFTER_READ },
    { "最後に読んだ記事をキャンセルする(\036C)",
	IDM_CANCEL,			AFTER_READ|SEPARATOR_NEXT },

    { "記事をポストする(\036I)...",
	IDM_POST,				0 },
    { "メイルを送信する(\036M)...",
	IDM_MAIL,				0 },
    { 0,0,0 }

  };
  
  static gnpopupmenu_t articlemode_help_menu[] = {
    { "コマンド一覧の表示(\036?)...",
	IDM_ARTHELP, 			0 },
    { "バージョン情報(\036A)...",
	IDM_ABOUT, 			0 },
    { 0,0,0 }
  };
  
  static gnmenu_t articlemode_menu[] = {
    { "ファイル(\036F)",	articlemode_file_menu },
    { "編集(\036E)",		articlemode_edit_menu },
    { "表示(\036V)",		articlemode_view_menu },
    { "発信(\036S)",		articlemode_send_menu },
    { "ヘルプ(\036H)",		articlemode_help_menu },
    { 0,0 }
  };
  
  static char
    *disp_trunc    = "長い行を折り曲げて表示する(\036T)",
    *disp_nottrunc = "長い行を折り曲げて表示しない(\036T)";
  
  static char
    *disp_thread   = "参照順に表示する(\036L)",
    *disp_notthread= "参照順に表示しない(\036L)";
  
  gnpopupmenu_t *gnpm;
  
  for ( gnpm = articlemode_view_menu ; gnpm->item != 0 ; gnpm++ ) {
    if ( gnpm->idm == IDM_ARTTRUNC ) {
      if ( gn.show_truncate == ON )
	gnpm->item = disp_nottrunc;
      else
	gnpm->item = disp_trunc;
      continue;
    }
    if ( gnpm->idm == IDM_ARTLIST ) {
      if ( (disp_mode & THREADED) == 0 )
	gnpm->item = disp_thread;
      else
	gnpm->item = disp_notthread;
      continue;
    }
  }
  return(setmenu(articlemode_menu));
}
articlemode_menu_exec(wParam)
WPARAM wParam;
{
  void win_article_mode_command();
  void article_mode_list();
  
  register char *pt;
  char key_buf[2];
  
  pt = key_buf;
  key_buf[1] = 0;
  
  switch ( wParam ) {
  case IDM_ARTCOMMAND:
    if ((pt = kb_input("記事の番号を入力して下さい")) == NULL )
      return(0);
    break;
  case IDM_ARTPREV:
    *pt = 'p';
    break;
  case IDM_ARTNEXT:
    *pt = 'n';
    break;
  case IDM_ARTTRUNC:
    *pt = 't';
    break;
  case IDM_ARTALL:
    *pt = 'a';
    break;
  case IDM_ARTUNSUBSCRIBE:
    *pt = 'u';
    break;
  case IDM_ARTSUBSCRIBE:
    *pt = 'S';
    break;
  case IDM_ARTCATCHUP:
    *pt = 'c';
    break;
  case IDM_ARTUNREAD:
    *pt = 'U';
    break;
  case IDM_ARTLIST:
    *pt = 'l';
    break;
  case IDM_ARTSEARCH:
    *pt = '/';
    break;
  case IDM_ARTREVSEARCH:
    *pt = '\\';
    break;
  case IDM_ARTPREVSUB:
    *pt = 'P';
    break;
  case IDM_ARTNEXTSUB:
    *pt = 'N';
    break;
  case IDM_ARTQUIT:
    *pt = 'q';
    break;
  case IDM_ARTHELP:
    *pt = '?';
    break;
  default:
    return(0);
  }
  
  win_article_mode_command(pt);
  
  if ( wParam == IDM_ARTTRUNC || wParam == IDM_ARTLIST)
    set_articlemode_menu();
  
  return(0);
}
void
win_article_mode_command(pt)
char *pt;
{
  int article_mode_command();
  
  switch ( article_mode_command(pt) ) {
  case CONT:
    article_mode_list();
    return;
  case END:			/* a コマンドで、全部読んだ / unsub */
    PostMessage(hWnd, WM_GROUPMODE,0 , 0L);
    return;
  case QUIT:
    PostMessage(hWnd, WM_SUBJECTMODE,0 , 0L);
    return;
  case NEXT:			/* 次のサブジェクト */
    PostMessage(hWnd, WM_ARTICLEMODE,0 , 0L);
    return;
  case CONNECTION_CLOSED:
    return;
  case SELECT:
    return;
  }
  return;
}
#endif	/* WINDOWS */
/*
 * アーティクルモード一覧表示
 */
void
article_mode_list()
{
  register int i,count;
  register article_t *ar;
  register char *pt;
  char buf[NNTP_BUFSIZE+1];
#ifdef	WINDOWS
  char buf2[NNTP_BUFSIZE+1];
#endif	/* WINDOWS */
  int	sb_width;
  
  start = -1;
#ifndef WINDOWS
  mc_puts("\n");
#else	/* WINDOWS */
  fast_mc_puts_start();
#endif	/* WINDOWS */
  
  if ( gn.autolist != 0 ) {
#ifndef	WINDOWS
    sb_width = gn.columns;
#ifndef	QUICKWIN
    if ( gn.cls == ON )
      mc_puts(CLS_STRING);
#endif

    if ( gn.show_indent > 0 ) {
      mc_printf("%*c", gn.show_indent * 2, ' ');
      sb_width -= gn.show_indent * 2;
    }
    sprintf(buf,"アーティクルモード（%s",ng->name);
    if ( sb->flag & NOTREONLY ) {
      strcat(buf,":");
    } else {
      strcat(buf,":Re: ");
    }
    strcat(buf,sb->name);
    strcat(buf,"）");
    if ( gn.show_truncate == OFF ) {
      if ( (int)strlen(buf) > sb_width)
	str_cut(buf,sb_width);
    }
    mc_printf("%s\n\n",buf);
#else  /* WINDOWS */
    sprintf(buf,"アーティクルモード %s %s%s",
	    ng->name,
	    ((sb->flag & NOTREONLY ) != 0) ? "" : "Re: ",
	    sb->name);
    kanji_convert(internal_kanji_code,buf,gn.display_kanji_code,buf2);
    SetWindowText(hWnd,buf2);
#endif /* WINDOWS */

    if ( gn.show_indent > 0 )
      mc_printf("%*c", gn.show_indent * 2, ' ');
    mc_puts("  番号  行数 投稿者\n");

    while(1){
      for ( i = 0 ,ar = sb->art; i < top && ar != 0 ; i++,ar=ar->next)
	;
      count = 0;
      start = -1;
      for (  ; ar != 0 ; ar = ar->next) {
	sb_width = gn.columns;
	if ( gn.show_indent > 0 ) {
	  mc_printf("%*c", gn.show_indent * 2, ' ');
	  sb_width -= gn.show_indent * 2;
	}
					
	if ( ar->flag ) {	/* 既読 */
	  mc_puts("R");
	} else {		/* 未読 */
	  if ( start == -1 )
	    start = ar->numb;	/* 未読の最初の記事番号 */
	  mc_puts(" ");
	}
	sb_width--;

	if (ar->lines > -1)
	  mc_printf("%5ld %5d ",ar->numb,ar->lines);
	else
	  mc_printf("%5ld       ",ar->numb);

	sb_width -= 5 + 1 + 5 + 1;
					
	if ( gn.show_truncate == ON ) {
	  mc_puts(ar->from);
	  mc_printf("\n");
	} else {
	  strcpy(buf,ar->from);
	  if ( (pt = strchr(buf,'\n')) != NULL )
	    *pt = 0;
	  str_cut(buf,sb_width);
	  mc_printf("%s\n",buf);
	}
	if ( ++count >= EFFECT_LINES )
	  break;
      }
      if ( count == 0 ){
	top -= EFFECT_LINES;
	if ( top < 0 )
	  top = 0;
	continue;
      }
      break;
    }
  }
  if ( gn.show_indent > 0 )
    mc_printf("%*c", gn.show_indent * 2, ' ');
  mc_puts("記事の番号またはコマンド（help=?）：");
#ifndef	WINDOWS
  fflush(stdout);
#else	/* WINDOWS */
  fast_mc_puts_end();
  
  count = 0;
  for ( ar = sb->art; ar != 0 ; ar=ar->next)
    count++;
  
  SetScrollRange(hWnd,SB_VERT,0,count,FALSE);
  SetScrollPos(hWnd,SB_VERT,top,TRUE);
#endif	/* WINDOWS */
}

/*
 * アーティクルモードコマンド
 */
int
article_mode_command(pt)
register char *pt;
{
  extern int show_article();
  extern int post_mode();
  extern int reference_mode();
  extern int followup_mode();
  extern int cancel_mode();
  extern void shell_mode();
  extern int show_article_id();
  extern void article_make_thread();
  extern void article_make_order();
  extern int article_all_article();
  extern void article_mode_help();
  extern void kill_mode();
  
  register article_t *ar;
  register int arg;
  long argl;
  subject_t *nsb,*psb;
  
  /* 記事番号指定 */
  if ( '0' <= *pt && *pt <= '9' ) {
    argl = atol(pt);
    switch ( show_article(argl) ){
    case CONNECTION_CLOSED:
      return(CONNECTION_CLOSED);
    case CONT:
      for ( ar = sb->art; ar != 0 ; ar=ar->next) {
	if ( ar->numb == argl ) {
	  if ( ar->flag == 0 ) {
	    ar->flag = 1;
	    sb->numb--;
	  }
	  break;
	}
      }
      break;
    case FILE_WRITE_ERROR:
      mc_puts("テンポラリファイルの作成に失敗しました\n");
      break;
    case SELECT:		/* WINDOWS */
      for ( ar = sb->art; ar != 0 ; ar=ar->next) {
	if ( ar->numb == argl ) {
	  if ( ar->flag == 0 ) {
	    ar->flag = 1;
	    sb->numb--;
	  }
	  break;
	}
      }
      return(SELECT);
    }
    return(CONT);
  }
  
  /* CRのみ：最初の未読記事 */
  if ( *pt == 0 ){
    if ( start == -1 ) {	/* 未読の記事がリストされていない */
      for ( ar = sb->art, top = 0; ar != 0 ; ar=ar->next,top++) {
	if ( ar->flag == 0 ) {
	  start = ar->numb;
	  break;
	}
      }
      if ( start == -1 ){
#ifndef	WINDOWS
	mc_puts("\n未読の記事はありません \n");
#endif /* WINDOWS */
	return(QUIT);
      }
    }
    switch ( show_article(start) ){
    case CONNECTION_CLOSED:
      return(CONNECTION_CLOSED);
    case CONT:
      for ( ar = sb->art; ar != 0 ; ar=ar->next) {
	if ( ar->numb == start ) {
	  if ( ar->flag == 0 ) {
	    ar->flag = 1;
	    sb->numb--;
	  }
	  break;
	}
      }
      break;
    case FILE_WRITE_ERROR:
      mc_puts("テンポラリファイルの作成に失敗しました\n");
      break;
    case ARTICLE_NOT_FOUND:
      /* アーティクルキューにキャッシュしてから
	 キャンセルされた場合、-1 が返って来る。
	 アーティクルキューから取り除いて
	 キャンセルキューにつなぎ直さなければならない。
	 */
      break;
    case SELECT:		/* WINDOWS */
      for ( ar = sb->art; ar != 0 ; ar=ar->next) {
	if ( ar->numb == start ) {
	  if ( ar->flag == 0 ) {
	    ar->flag = 1;
	    sb->numb--;
	  }
	  break;
	}
      }
      return(SELECT);
    }
    return(CONT);
  }
  
  /* １文字だけのコマンド */
  if ( pt[1] == 0 ) {
    switch ( *pt ) {
    case 'a':			/* 全記事 */
#ifndef	WINDOWS
      if ( article_all_article() == CONNECTION_CLOSED )
	return(CONNECTION_CLOSED);
      return(QUIT);
#else  /* WINDOWS */
      PostMessage(hWnd, WM_ARTICLEALL,0 , 0L);
      return(SELECT);
#endif /* WINDOWS */
    case 'd':
      gn.autolist = 1;
      return(CONT);
			
    case 't':
      gn.autolist = 1;
      gn.show_truncate ^= ON;
      return(CONT);
			
    case 'P':			/* 前のサブジェクト */
      gn.autolist = 1;
			
      if ( sb == ng->subj ) {
	gn.autolist = autolist_sv;
	mc_puts("\n前のサブジェクトが存在しません \n");
	return(QUIT);
      }
      for ( nsb = ng->subj ; nsb != sb ; nsb = nsb->next)
	psb = nsb;
			
      mc_printf("\n前のサブジェクトは %s です\n",psb->name);
      sb = psb;
      if ( disp_mode == THREADED && (sb->flag & THREADED) == 0 ){
	article_make_thread(sb);
	sb->flag |= THREADED;
      } else if (disp_mode != THREADED && (sb->flag & THREADED) != 0 ){
	article_make_order(sb);
	sb->flag &= ~THREADED;
      }
      top = 0;
      return(NEXT);
			
    case 'N':			/* 次のサブジェクト */
      gn.autolist = 1;
			
      if ( sb->next == 0 ) {
	gn.autolist = autolist_sv;
	mc_puts("\n次のサブジェクトが存在しません \n");
	return(QUIT);
      }
      mc_printf("\n次のサブジェクトは %s です\n",sb->next->name);
      sb = sb->next;
      if ( disp_mode == THREADED && (sb->flag & THREADED) == 0 ){
	article_make_thread(sb);
	sb->flag |= THREADED;
      } else if (disp_mode != THREADED && (sb->flag & THREADED) != 0 ){
	article_make_order(sb);
	sb->flag &= ~THREADED;
      }
      top = 0;
      return(NEXT);
			
    case 'i':			/* ポスト */
      return(post_mode(ng->name));
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
    case 'u':			/* もう読まない */
#ifndef	WINDOWS
      pt = kb_input("もうリストアップしません(y:了解/その他:中止)");
      if ( *pt == 'y' || *pt == 'Y' ){
	ng->flag |= UNSUBSCRIBE;
	change = 1;
	return(END);
      }
#else  /* WINDOWS */
      if ( mc_MessageBox(hWnd,"もうリストアップしません ",
			 "",MB_OKCANCEL) == IDOK ){
	ng->flag |= UNSUBSCRIBE;
	change = 1;
	return(END);
      }
#endif /* WINDOWS */
      return(CONT);
    case 'S':			/* やっぱり読む */
      if ( ng->flag & UNSUBSCRIBE ) {
	ng->flag &= ~UNSUBSCRIBE;
	change = 1;
      }
      return(CONT);
    case 'K':
      kill_mode();		/* KILL */
      return(CONT);
    case 'c':			/* 全部読んだことにする */
      for ( ar = sb->art; ar != 0 ; ar=ar->next){
	if ( ar->flag == 0 ){	/* 未読 */
	  ar->flag = 1;
	  change = 1;
	}
      }
      sb->numb = 0;
      return(QUIT);
    case 'l':
      if ( (disp_mode & THREADED) == 0 ) {
	article_make_thread(sb);
	disp_mode |= THREADED;
	sb->flag |= THREADED;
      } else {
	article_make_order(sb);
	disp_mode &= ~THREADED;
	sb->flag &= ~THREADED;
      }
      gn.autolist = 1;
      return(CONT);
    case 'q':			/* 終了 */
      return(QUIT);
    case '?':			/* help */
      article_mode_help();
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
		
    gn.autolist = 1;
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
		
    gn.autolist = 1;
    return(CONT);
#ifndef	NO_SAVE
  case 's':			/* セーブ */
    save_mode(pt);
    return(CONT);
#endif /* NO_SAVE */

  case 'U':			/* 既読マークを消す */
    article_unread(pt);
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
#endif /* ! QUICKWIN && ! WINDOWS */
  case '<':
    return(show_article_id(pt));
  case '/':			/* 投稿者検索 */
    article_search(pt);
    gn.autolist = 1;
    return(CONT);
  case '\\':			/* 投稿者検索（逆順） */
    article_search_rev(pt);
    gn.autolist = 1;
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

/*
 * 全部の未読記事を表示する
 */
#define	CHECK_COUNT	10
#ifndef	WINDOWS
int
article_all_article()
{
  extern int show_article_common();
  
#if ! defined(NO_NET)
  char resp[NNTP_BUFSIZE+1];
#endif
  register char *artilce_file;
  register article_t *ar;
  register int count = 0;
  char *key_buf;
  FILE *fp;
  
  for ( ar = sb->art; ar != 0 ; ar=ar->next) {
    if ( ar->flag )
      continue;
    if ( gn.local_spool ) {
      if ( gn.power_save == COPY_TO_TMP )
	artilce_file = expand_filename(article_filename(ar->numb),gn.tmpdir);
      else
	artilce_file = expand_filename(article_filename(ar->numb),ngdir);
      if ( artilce_file == 0 ) {
	mc_printf("ファイル名の展開に失敗しました(%s)\n",
		  article_filename(ar->numb));
	return(ARTICLE_NOT_FOUND);
      }
      if ( (fp = fopen(artilce_file,"r")) == NULL ) {
	mc_puts(article_not_found);
	hit_return();
	return(ARTICLE_NOT_FOUND);
      }
#if ! defined(NO_NET)
    } else {
      sprintf(resp,"ARTICLE %ld",ar->numb);
      if ( put_server(resp) )
	return(CONNECTION_CLOSED);
      if ( get_server(resp,NNTP_BUFSIZE) )
	return(CONNECTION_CLOSED);
      switch ( atoi(resp) ) {
      case OK_ARTICLE:
	break;
      case ERR_NOARTIG:
	mc_puts(article_not_found);
	hit_return();
	return(CONT);
      default:
	return(CONNECTION_CLOSED);
      }
#endif /* NO_NET */
    }

    switch ( show_article_common(ng,ar->numb,fp,artilce_file) ) {
    case CONNECTION_CLOSED:
      return(CONNECTION_CLOSED);
    case FILE_WRITE_ERROR:
      mc_puts("テンポラリファイルの作成に失敗しました\n");
      return(CONT);
    case ARTICLE_NOT_FOUND:
      return(CONT);
    case SELECT:
      /* 未読なら既読に */
      if ( ar->flag == 0 ) {
	ar->flag = 1;
	sb->numb--;
      }
      return(SELECT);
    case CONT:
      /* 未読なら既読に */
      if ( ar->flag == 0 ) {
	ar->flag = 1;
	sb->numb--;
      }
      break;
    default:
      break;
    }

    if ( ++count >= CHECK_COUNT && ar->next != 0) {
      key_buf = kb_input("続けますか？（中止:n）");
      while( *key_buf == ' ' || *key_buf == '\t' )
	key_buf++;
      if ( *key_buf == 'n' || *key_buf == 'N' )
	return(CONT);
      count = 0;
    }
  }
  return(CONT);
}
#else	/* WINDOWS */
article_all_article()
{
  extern int show_article();
  static int count = 0;
  register  article_t *ar = 0;
  
  for (ar = sb->art ; ar != 0 ; ar=ar->next) {
    if ( ar->flag == 0)
      break;
  }
  if ( ar == 0 ) {
    count = 0;
    PostMessage(hWnd, WM_SUBJECTMODE,0 , 0L);
    return(END);
  }
  
  if ( ++count >= CHECK_COUNT && ar->next != 0) {
    if ( mc_MessageBox(hWnd,"続けますか？",
		       "",MB_OKCANCEL) == IDCANCEL ){
      count = 0;
      PostMessage(hWnd, WM_ARTICLEMODE,0 , 0L);
      return(END);
    }
    count = 0;
  }
  
  switch ( show_article(ar->numb) ){
  case CONNECTION_CLOSED:
    return(CONNECTION_CLOSED);
  case CONT:
  case SELECT:
    if ( ar->flag == 0 ) {
      ar->flag = 1;
      sb->numb--;
    }
    break;
  case FILE_WRITE_ERROR:
    mc_puts("テンポラリファイルの作成に失敗しました\n");
    return(END);
  }
  
  return(CONT);
}
#endif	/* WINDOWS */
static void
article_unread(pt)
char *pt;
{
  register article_t *ar;
  long num;
  char resp[NNTP_BUFSIZE+1];
  newsgroup_t *cng;
  subject_t *csb;
  char *ngname;
  long	artnum;
  
  pt++;
  while( *pt == ' ' || *pt == '\t' )
    pt++;
  
  if (last_art.newsgroups != 0 &&
      strcmp(last_art.newsgroups,ng->name) == 0 && (*pt<'0' || '9'<*pt )) {
#ifndef	WINDOWS
    sprintf(resp,
	    "未読にする記事番号を入力して下さい（デフォルト:%ld/中止:q）",
	    last_art.art_numb);
    while (1) {
      pt = kb_input(resp);
      while( *pt == ' ' || *pt == '\t' )
	pt++;
      if ( *pt == 'q' )
	return;
      if ( ('0' <= *pt && *pt <= '9') || *pt == 0 )
	break;
    }
    if ( *pt == 0 )
      num = last_art.art_numb;
    else
      num = atol(pt);
#else  /* WINDOWS */
    sprintf(resp,
	    "未読にする記事番号を入力して下さい（デフォルト:%ld）",
	    last_art.art_numb);
    while (1) {
      if ( (pt= kb_input(resp) ) == NULL )
	return;
      while( *pt == ' ' || *pt == '\t' )
	pt++;
      if ( '0' <= *pt && *pt <= '9' || *pt == 0 )
	break;
    }
    if ( *pt == 0 )
      num = last_art.art_numb;
    else
      num = atol(pt);
#endif /* WINDOWS */
  } else {
    while ( *pt < '0' || '9' < *pt ) {
#ifndef	WINDOWS
      pt = kb_input("未読にする記事番号を入力して下さい（中止:q）");
      while( *pt == ' ' || *pt == '\t' )
	pt++;
      if ( *pt == 'q' )
	return;
#else  /* WINDOWS */
      if ( (pt = kb_input("未読にする記事番号を入力して下さい")) == NULL )
	return;
      while( *pt == ' ' || *pt == '\t' )
	pt++;
#endif /* WINDOWS */
    }
    num = atol(pt);
  }
  
  if (last_art.newsgroups != 0 && last_art.xref != 0 &&
      strcmp(last_art.newsgroups,ng->name) == 0 && last_art.art_numb == num){
    strcpy(resp,last_art.xref);
    pt = resp;

    while ( *pt != 0 ) {
      ngname = pt;
      while ( *pt != ':' )
	pt++;
      *pt++ = 0;
      artnum = atol(pt);
      while ( ('0' <= *pt && *pt <= '9') || *pt == ' ')
	pt++;
      if ( (cng = search_group(ngname)) == 0 )
	continue;
      if ( cng == ng )
	continue;
      set_unreaded(cng,artnum);
    }
  }
  for ( csb = ng->subj ; csb != 0 ; csb = csb->next ) {
    for ( ar = csb->art; ar != 0 ; ar = ar->next) {
      if ( ar->numb == num ) {
	if ( ar->flag != 0 ) {
	  ar->flag = 0;
	  csb->numb++;
	  ng->flag &= ~NOARTICLE;
	  change = 1;
	}
	return;
      }
    }
  }
  
#ifdef	WINDOWS
  mc_MessageBox(hWnd,article_not_found,"",MB_OK);
#else	/* WINDOWS */
  mc_puts(article_not_found);
  hit_return();
#endif	/* WINDOWS */
}

/*
 * 記事の表示
 */
int
show_article(n)
long n;			/* 記事番号 */
{
  extern int show_article_common();
#if ! defined(NO_NET)
  char resp[NNTP_BUFSIZE+1];
#endif
  FILE *fp;
  register char *article_file;
  
  if ( gn.local_spool ) {
    if ( gn.power_save == COPY_TO_TMP )
      article_file = expand_filename(article_filename(n),gn.tmpdir);
    else
      article_file = expand_filename(article_filename(n),ngdir);
    if ( article_file == 0 ) {
#if ! defined(WINDOWS)
      mc_printf("ファイル名の展開に失敗しました(%s)\n",
		article_filename(n));
#else  /* WINDOWS */
      mc_MessageBox(hWnd,"ファイル名の展開に失敗しました",
		    article_filename(n),MB_OK);
#endif /* WINDOWS */
      return(ARTICLE_NOT_FOUND);
    }

    if ( (fp = fopen(article_file,"r")) == NULL ) {
#ifdef	WINDOWS
      mc_MessageBox(hWnd,article_not_found,"",MB_OK);
#else  /* WINDOWS */
      mc_puts(article_not_found);
      hit_return();
#endif /* WINDOWS */
      return(ARTICLE_NOT_FOUND);
    }
#if ! defined(NO_NET)
  } else {
#ifdef	WINDOWS
    set_hourglass();
#endif /* WINDOWS */
    sprintf(resp,"ARTICLE %ld",n);
    if ( put_server(resp) ) {
#ifdef	WINDOWS
      reset_cursor();
#endif /* WINDOWS */
      return(CONNECTION_CLOSED);
    }
    if ( get_server(resp,NNTP_BUFSIZE) ) {
#ifdef	WINDOWS
      reset_cursor();
#endif /* WINDOWS */
      return(CONNECTION_CLOSED);
    }
    switch ( atoi(resp) ) {
    case OK_ARTICLE:
      break;
    case ERR_NOARTIG:
#ifdef	WINDOWS
      reset_cursor();
      mc_MessageBox(hWnd,article_not_found,"",MB_OK);
#else  /* WINDOWS */
      mc_puts(article_not_found);
      hit_return();
#endif /* WINDOWS */
      return(ARTICLE_NOT_FOUND);
    default:
#ifdef	WINDOWS
      reset_cursor();
#endif /* WINDOWS */
      return(CONNECTION_CLOSED);
    }
#endif
  }
  return(show_article_common(ng,n,fp,article_file));
}

int
show_article_common(cng,numb,art_fp,art_file)
register newsgroup_t *cng;	/* カレントニュースグループリスト */
long numb;			/* 記事番号 */
FILE *art_fp;			/* 記事ファイル(only for local_spool) */
char *art_file;			/* 記事ファイル名(only for local_spool) */
{
  extern void add_esc();
  extern void last_art_clear();

#ifdef	QUICKWIN
  extern int qw_pager();
#endif	/* QUICKWIN */
#ifdef	WINDOWS
  extern void win_pager();
#endif	/* WINDOWS */
  
  FILE *fp;
  char resp[NNTP_BUFSIZE+1];
  char resp2[NNTP_BUFSIZE+1];
  char buf[NNTP_BUFSIZE+1];
  register char *sp,*pt;
  int x = 0;
  
#if ! defined(NO_NET)
  if ( gn.local_spool == 0 ) {
    /* １行目 */
    if ( get_server(resp,NNTP_BUFSIZE)) {
#ifdef	WINDOWS
      reset_cursor();
#endif /* WINDOWS */
      return(CONNECTION_CLOSED);
    }
    if ( resp[0] == 0 ) {
#ifdef	WINDOWS
      reset_cursor();
      mc_MessageBox(hWnd,article_not_found,"",MB_OK);
#else  /* WINDOWS */
      mc_puts(article_not_found);
      hit_return();
#endif /* WINDOWS */
      while ( resp[0] != '.' || resp[1] != 0 ) {
	if ( get_server(resp,NNTP_BUFSIZE)) {
	  return(CONNECTION_CLOSED);
	}
      }
      return(ARTICLE_NOT_FOUND);
    }
  }
#endif /* NO_NET */

  /* 前に読んだ記事があれば捨てる */
  last_art_clear();
  
  if ( cng == 0 ) {
    /* Message-ID 指定で読んだとき。カレントのニュースグループ不明 */
    last_art.newsgroups = 0;
    last_art.art_numb = 0;
  } else {
    /* カレントのニュースグループがある */
    if ( (last_art.newsgroups = malloc(strlen(cng->name)+1)) == NULL )
      memory_error();
    strcpy(last_art.newsgroups,cng->name);
    last_art.art_numb = numb;
  }
  
  if ( make_article_tmpfile != ON ) {
    if ( (last_art.tmp_file = malloc(strlen(art_file)+1)) == NULL )
      memory_error();
    strcpy(last_art.tmp_file,art_file);
  } else {
    /* テンポラリファイルに書く */
    if ( (last_art.tmp_file = malloc(strlen(gn.tmpdir)+8+1)) == NULL )
      memory_error();
    strcpy(last_art.tmp_file,gn.tmpdir);
    strcat(last_art.tmp_file,"gnXXXXXX");
    mktemp(last_art.tmp_file);
    if ( ( fp = fopen(last_art.tmp_file,"w") ) == NULL ){
#ifdef	WINDOWS
      reset_cursor();
      mc_MessageBox(hWnd,"テンポラリファイルのオープンに失敗しました",
		    last_art.tmp_file,MB_OK);
#else  /* WINDOWS */
      perror(last_art.tmp_file);
#endif /* WINDOWS */
      free(last_art.tmp_file);
      last_art.tmp_file = 0;
#if ! defined(NO_NET)
      while ( resp[0] != '.' || resp[1] != 0 ) {
	if ( get_server(resp,NNTP_BUFSIZE))
	  return(CONNECTION_CLOSED);
      }
#endif
      return(FILE_WRITE_ERROR);
    }
  }
  
  if ( gn.local_spool ) {
    resp[0] = 0;
    get_1_header_file(buf,resp,art_fp);
  }
  
  /* ヘッダ部 */
  while(1) {
    if ( resp[0] == 0 ) {
      if ( make_article_tmpfile == ON )
	fprintf(fp,"\n");
#ifdef WINDOWS
      last_art.lines++;
#endif /* WINDOWS */
      break;
    }
    /* １行読み込み */
    if ( gn.local_spool ) {
      get_1_header_file(buf,resp,art_fp);
#if ! defined(NO_NET)
    } else  {
      if ( get_1_header(buf,resp) == CONNECTION_CLOSED ) {
#ifdef	WINDOWS
	reset_cursor();
#endif /* WINDOWS */
	return(CONNECTION_CLOSED);
      }
#endif
    }

    /* Newsgroups: ヘッダ */
    if ( cng == 0 ) {
      /* Message-ID 指定で読んだとき。カレントのニュースグループ不明 */
      if ( strncmp(buf,"Newsgroups: ",12) == 0 ) {
	if ( make_article_tmpfile == ON ) {
	  kanji_convert(gn.spool_kanji_code,buf,gn.process_kanji_code,resp2);
	  fprintf(fp,"%s\n",resp2);
	}
#ifdef WINDOWS
	last_art.lines++;
#endif /* WINDOWS */
	if ( (sp = strchr(buf,',')) != 0 )
	  *sp = 0;
	if ( last_art.newsgroups != 0 )
	  free(last_art.newsgroups);
	if ( (last_art.newsgroups = malloc(strlen(&buf[12])+1)) == NULL )
	  memory_error();
	strcpy(last_art.newsgroups,&buf[12]);
	continue;
      }
    }

    /* From: ヘッダ */
    if ( strncmp(buf,"From: ",6) == 0 ) {
#ifdef	MIME
      if ( (gn.mime_head & MIME_DECODE) != 0 ) {
	kanji_convert(gn.spool_kanji_code,buf,JIS,resp2);
	MIME_strHeaderDecode(resp2,buf,sizeof(buf));
	kanji_convert(JIS,buf,internal_kanji_code,resp2);
	if ( make_article_tmpfile == ON )
	  fprintf(fp,"%s\n",resp2);
	if ( last_art.from != 0 )
	  free(last_art.from);
	if ( (last_art.from = malloc(strlen(&resp2[6])+1)) == NULL )
	  memory_error();
	strcpy(last_art.from,&resp2[6]);
      } else
#endif /* MIME */
	{
	  kanji_convert(gn.spool_kanji_code,buf,internal_kanji_code,resp2);
	  if ( make_article_tmpfile == ON )
	    fprintf(fp,"%s\n",resp2);
	  if ( last_art.from != 0 )
	    free(last_art.from);
	  if ( (last_art.from = malloc(strlen(&resp2[6])+1)) == NULL )
	    memory_error();
	  strcpy(last_art.from,&resp2[6]);
	}

#ifdef WINDOWS
      last_art.lines++;
#endif /* WINDOWS */
      continue;
    }


    /* subject: ヘッダ */
    if ( strncmp(buf,"Subject: ",9) == 0 ) {
#ifdef	MIME
      if ( (gn.mime_head & MIME_DECODE) != 0 ) {
	kanji_convert(gn.spool_kanji_code,buf,JIS,resp2);
	MIME_strHeaderDecode(resp2,buf,sizeof(buf));
	kanji_convert(JIS,buf,internal_kanji_code,resp2);
	if ( make_article_tmpfile == ON )
	  fprintf(fp,"%s\n",resp2);
	if ( last_art.subject != 0 )
	  free(last_art.subject);
	if ( (last_art.subject = malloc(strlen(&resp2[9])+1)) == NULL )
	  memory_error();
	strcpy(last_art.subject,&resp2[9]);
      } else
#endif /* MIME */
	{
	  kanji_convert(gn.spool_kanji_code,buf,internal_kanji_code,resp2);
	  if ( make_article_tmpfile == ON )
	    fprintf(fp,"%s\n",resp2);
	  if ( last_art.subject != 0 )
	    free(last_art.subject);
	  if ( (last_art.subject = malloc(strlen(&resp2[9])+1)) == NULL )
	    memory_error();
	  strcpy(last_art.subject,&resp2[9]);
	}

#ifdef WINDOWS
      last_art.lines++;
#endif /* WINDOWS */
      continue;
    }


    /* Date: ヘッダ */
    if ( strncmp(buf,"Date: ",6) == 0 ) {
      if ( gn.gmt2jst == ON ) {	/* GMT -> JST */
	if ( gmt_to_jst(&buf[6],resp2) == 0 )
	  strcpy(&buf[6],resp2);
      }
      if ( make_article_tmpfile == ON ) {
	kanji_convert(gn.spool_kanji_code,buf,gn.process_kanji_code,resp2);
	fprintf(fp,"%s\n",resp2);
      }
#ifdef WINDOWS
      last_art.lines++;
#endif /* WINDOWS */
      if ( last_art.date != 0 )
	free(last_art.date);
      if ( (last_art.date = malloc(strlen(&buf[6])+1)) == NULL )
	memory_error();
      strcpy(last_art.date,&buf[6]);
      continue;
    }

    /* References: ヘッダ */
    if ( strncmp(buf,"References: ",12) == 0 ) {
      if ( make_article_tmpfile == ON ) {
	kanji_convert(gn.spool_kanji_code,buf,gn.process_kanji_code,resp2);
	fprintf(fp,"%s\n",resp2);
      }
#ifdef WINDOWS
      last_art.lines++;
#endif /* WINDOWS */
      if ( last_art.references != 0 )
	free(last_art.references);
      if ( (last_art.references = malloc(strlen(&buf[12])+1)) == NULL )
	memory_error();
      strcpy(last_art.references,&buf[12]);
      continue;
    }

    /* X-Nsubject: ヘッダ */
    if ( strncmp(buf,"X-Nsubject: ",12) == 0 ) {
      add_esc(buf,resp2);
      if ( make_article_tmpfile == ON ) {
	kanji_convert(gn.spool_kanji_code,resp2,gn.process_kanji_code,buf);
	fprintf(fp,"%s\n",buf);
      }
#ifdef WINDOWS
      last_art.lines++;
#endif /* WINDOWS */
      continue;
    }
    /* Xref: ヘッダ */
    if ( strncmp(buf,"Xref: ",6) == 0 ) {
      if ( make_article_tmpfile == ON ) {
	kanji_convert(gn.spool_kanji_code,buf,gn.process_kanji_code,resp2);
	fprintf(fp,"%s\n",resp2);
      }
#ifdef WINDOWS
      last_art.lines++;
#endif /* WINDOWS */
      if ( (pt = strchr(&buf[6],' ')) == NULL )
	continue;
      pt++;			/* ホスト名をパス */

      if ( last_art.xref != 0 )
	free(last_art.xref);
      if ( (last_art.xref = malloc(strlen(pt)+1)) == NULL )
	memory_error();
      strcpy(last_art.xref,pt);
      continue;
    }


    /* other */
#ifdef	MIME
    if ( (gn.mime_head & MIME_DECODE) != 0 ) {
      kanji_convert(gn.spool_kanji_code,buf,JIS,resp2);
      MIME_strHeaderDecode(resp2,buf,sizeof(buf));
      kanji_convert(JIS,buf,gn.process_kanji_code,resp2);
      if ( make_article_tmpfile == ON )
	fprintf(fp,"%s\n",resp2);
    } else
#endif
      if ( make_article_tmpfile == ON ) {
	kanji_convert(gn.spool_kanji_code,buf,gn.process_kanji_code,resp2);
	fprintf(fp,"%s\n",resp2);
      }
#ifdef WINDOWS
    last_art.lines++;
#endif /* WINDOWS */
  }
  
  /* 記事部 */
  while(1) {
    if ( ++x > 20 ){
      mc_printf(".");
      fflush(stdout);
      x = 0;
#ifdef	QUICKWIN
      _wyield();
#endif
    }
    /* １行読み込み */
    if ( gn.local_spool ) {
      if ( fgets(resp,NNTP_BUFSIZE,art_fp) == NULL ) {
	fclose(art_fp);
	break;
      }
      if ( (pt = strchr(resp,'\n')) != NULL)
	*pt = 0;
#if ! defined(NO_NET)
    } else {
      if ( get_server(resp,NNTP_BUFSIZE)) {
#ifdef	WINDOWS
	reset_cursor();
#endif /* WINDOWS */
	return(CONNECTION_CLOSED);
      }
      if ( resp[0] == '.' && resp[1] == 0)
	break;
#endif
    }
    /* 漢字コード変換 */
    if ( make_article_tmpfile == ON ) {
      if ( gn.process_kanji_code == gn.spool_kanji_code ) {
	if ( gn.local_spool == 0 && resp[0] == '.' && resp[1] == '.' ) {
	  fprintf(fp,"%s\n",&resp[1]);
	} else {
	  fprintf(fp,"%s\n",resp);
	}
      } else {
	if ( gn.local_spool == 0 && resp[0] == '.' && resp[1] == '.')
	  kanji_convert(gn.spool_kanji_code,&resp[1],gn.process_kanji_code,resp2);
	else
	  kanji_convert(gn.spool_kanji_code,resp,gn.process_kanji_code,resp2);
	fprintf(fp,"%s\n",resp2);
      }
    }
#ifdef WINDOWS
    last_art.lines++;
#endif /* WINDOWS */
  }

  if ( make_article_tmpfile == ON ) {
    if ( fclose(fp) == EOF ) {
#ifdef	WINDOWS
      reset_cursor();
#endif /* WINDOWS */
      return(FILE_WRITE_ERROR);
    }
  }

#ifdef	WINDOWS
  reset_cursor();
  win_pager(last_art.tmp_file,last_art.lines);
  if ( cng != 0 && last_art.xref != 0 )
    xref_mark(last_art.xref,cng->name);
  return(SELECT);
#else	/* WINDOWS */
#ifdef	QUICKWIN
  qw_pager(last_art.tmp_file);
  if ( cng != 0 && last_art.xref != 0 )
    xref_mark(last_art.xref,cng->name);
  return(CONT);
#else	/* QUICKWIN */
  sprintf(resp,"%s %s",gn.pager,last_art.tmp_file);
  if ( system(resp) == -1 ) {
    perror("system");
    return(ERROR);
  }
  if ( cng != 0 && last_art.xref != 0 )
    xref_mark(last_art.xref,cng->name);
  if ( gn.return_after_pager != 0 )
    hit_return();
  return(CONT);
#endif	/* QUICKWIN */
#endif	/* WINDOWS */
}
/*----------------------------------------------------------------------*/

/* 前に読んだ記事があれば捨てる */
void
last_art_clear()
{
  if ( last_art.tmp_file != 0 ) {

    if ( make_article_tmpfile == ON )
      unlink(last_art.tmp_file);

    free(last_art.tmp_file);
    last_art.tmp_file = 0;
  }

  if ( last_art.newsgroups != 0 ) {
    free(last_art.newsgroups);
    last_art.newsgroups = 0;
  }

  if ( last_art.from != 0 ) {
    free(last_art.from);
    last_art.from = 0;
  }

  if ( last_art.date != 0 ) {
    free(last_art.date);
    last_art.date = 0;
  }

  if ( last_art.references != 0 ) {
    free(last_art.references);
    last_art.references = 0;
  }

  if ( last_art.xref != 0 ) {
    free(last_art.xref);
    last_art.xref = 0;
  }

  if ( last_art.subject != 0 ) {
    free(last_art.subject);
    last_art.subject = 0;
  }
  last_art.art_numb = 0;
#ifdef WINDOWS
  last_art.lines = 0;
#endif /* WINDOWS */
}

/* 記事が読まれていることの確認 */
int
last_art_check()
{
  if ( last_art.tmp_file == 0 )
    return(ERROR);

  return(CONT);
}

/*----------------------------------------------------------------------*/
/* GMT から JST に変換する                                     */
/* [day, ]dd mm yy GMT/+0000/UT/Z という format 以外は無視する */
/* 戻り値   0: 変換済み                                        */
/*         -1: 変換しなかった                                  */

static char *daystr[]
	= {"Sun, ","Mon, ","Tue, ","Wed, ","Thu, ","Fri, ","Sat, "};
static char *monthstr[]
	= {"Jan ","Feb ","Mar ","Apr ","May ","Jun ",
		   "Jul ","Aug ","Sep ","Oct ","Nov ","Dec "};
static int
gmt_to_jst(gmt,jst)
char *gmt;
char *jst;
{
  int  hour,date,month,year,day;
  int  flag,i;
  char *gmt_p;
  char ms[6];
  char str[5];
  
  gmt_p = gmt;
  if (strncmp(&gmt[3],", ",2))
    day = 7;	/* 曜日なし */
  else {
    gmt_p += 5;
    for (day=7,flag=0,i=0;i<7;i++) {
      if (!strncmp(gmt,daystr[i],5)) {
	day = i;
	flag = 1;
	break;
      }
    }
    if (!flag)
      return(-1);		/* 曜日 format 異常 */
  }
  
  if (isdigit(*(gmt_p++))) {
    if (*gmt_p == ' ') {	/* 日 1桁 */
      date = *(--gmt_p) - 0x30;
      gmt_p += 2;
    }
    else if (isdigit(*gmt_p)) {
      date = *(gmt_p--) - 0x30;	/* 日 2桁 */
      date += (*gmt_p - 0x30) * 10;
      gmt_p += 3;
    }
    else
      return(-1);		/* 日 format 異常 */
  }
  else
    return(-1);		/* 日 format 異常 */
  
  for (flag=0,i=0;i<12;i++) {
    if (!strncmp(gmt_p,monthstr[i],4)) {
      month = i + 1;
      flag = 1;
      break;
    }
  }
  if (!flag)
    return(-1);		/* 月 format 異常 */
  gmt_p += 4;
  
  if (isdigit(*gmt_p++)&&isdigit(*gmt_p++)) {
    if (*gmt_p == ' ') {
      gmt_p -= 2;
      year = (*(gmt_p++) - 0x30) * 10;
      year += (*gmt_p - 0x30);
    }
    else if (isdigit(*gmt_p++)&&isdigit(*gmt_p++)&&(*gmt_p == ' ')) {
      gmt_p -= 4;
      year = (*(gmt_p++) - 0x30) * 1000;
      year += (*(gmt_p++) - 0x30) * 100;
      year += (*(gmt_p++) - 0x30) * 10;
      year += (*gmt_p - 0x30);
    }
    else
      return(-1);		/* 年 format 異常 */
  }
  else
    return(-1);	/* 年 format 異常 */
  gmt_p += 2;
  
  if (isdigit(*(gmt_p++))&&isdigit(*(gmt_p++))&&(*gmt_p == ':')) {
    gmt_p -= 2;
    hour = (*gmt_p - 0x30) * 10;
    hour += *(++gmt_p) - 0x30;
    gmt_p += 2;
  }
  else
    return(-1);	/* 時 format 異常 */
  
  if (isdigit(*(gmt_p++))&&isdigit(*(gmt_p--))) {
    ms[0] = *gmt_p++;
    ms[1] = *gmt_p++;
  }
  else
    return(-1);	/* 分 format 異常 */
  
  if (*gmt_p == ':') {
    gmt_p++;
    if (isdigit(*(gmt_p++))&&isdigit(*(gmt_p++))&&(*gmt_p == ' ')) {
      gmt_p -= 2;
      ms[2] = ':';
      ms[3] = *(gmt_p++);
      ms[4] = *gmt_p++;
      ms[5] = '\0';
      if ((strncmp(gmt_p," GMT",4))&&(strncmp(gmt_p," +0000",6))
	  &&(strncmp(gmt_p," -0000",6))&&(strncmp(gmt_p," UT",3))
	  &&(strncmp(gmt_p," Z",2)))
	return(-1);		/* GMT 以外 */
    }
    else
      return(-1);		/* 秒 format 異常 */
  }
  else if (*gmt_p == ' ') {
    if ((strncmp(gmt_p," GMT",4))&&(strncmp(gmt_p," +0000",6))
	&&(strncmp(gmt_p," -0000",6))&&(strncmp(gmt_p," UT",3))
	&&(strncmp(gmt_p," Z",2)))
      return(-1);		/* GMT 以外 */
    else {
      gmt_p -= 2;
      ms[2] = '\0';
    }
  }
  else
    return(-1);	/* 秒 format 異常 */
  
  hour += 9;
  if (hour > 23) {
    hour -= 24;
    date ++;
    if (day != 7) {
      if (day == 6)
	day = 0;
      else
	day++;
    }
    switch (month) {
    case 1:
      if (date == 32) {
	month = 2;
	date = 1;
      }
      break;
    case 2:
      for (flag=1;;) {
	if (year%4)
	  break;
	else if (!(year%100))
	  if (year%400)
	    break;
	if (date == 30) {
	  month = 3;
	  date = 1;
	}
	flag = 0;
	break;
      }
      if (flag) {
	if (date == 29) {
	  month = 3;
	  date = 1;
	}
      }
      break;
    case 3:
      if (date == 32) {
	month = 4;
	date = 1;
      }
      break;
    case 4:
      if (date == 31) {
	month = 5;
	date = 1;
      }
      break;
    case 5:
      if (date == 32) {
	month = 6;
	date = 1;
      }
      break;
    case 6:
      if (date == 31) {
	month = 7;
	date = 1;
      }
      break;
    case 7:
      if (date == 32) {
	month = 8;
	date = 1;
      }
      break;
    case 8:
      if (date == 32) {
	month = 9;
	date = 1;
      }
      break;
    case 9:
      if (date == 31) {
	month = 10;
	date = 1;
      }
      break;
    case 10:
      if (date == 32) {
	month = 11;
	date = 1;
      }
      break;
    case 11:
      if (date == 31) {
	month = 12;
	date = 1;
      }
      break;
    default:
      if (date == 32) {
	month = 1;
	date = 1;
	year++;
      }
    }
  }
  
  jst[0] = '\0';
  if (day != 7)
    strcat(jst,daystr[day]);
  
  if (date/10) {
    str[0] = date/10 + 0x30;
    str[1] = date%10 + 0x30;
    str[2] = '\0';
  }
  else {
    str[0] = date%10 + 0x30;
    str[1] = '\0';
  }
  strcat(jst,str);
  strcat(jst," ");
  
  strcat(jst,monthstr[month-1]);
  
  i = 0;
  if (year/1000) {
    str[i++] = year/1000 + 0x30;
    year -= (year/1000) * 1000;
    str[i++] = year/100 + 0x30;
    year -= (year/100) * 100;
  }
  str[i++] = year/10 + 0x30;
  str[i++] = year%10 + 0x30;
  str[i] = '\0';
  strcat(jst,str);
  strcat(jst," ");
  
  str[0] = hour/10 + 0x30;
  str[1] = hour%10 + 0x30;
  str[2] = ':';
  str[3] = '\0';
  strcat(jst,str);
  
  strcat(jst,ms);
  
  strcat(jst," JST");
  return(0);
}

static void
xref_mark(last_xref,cng)
register char *last_xref,*cng;
{
  extern void set_readed();
  
  char xref_buf[NNTP_BUFSIZE+1];
  register char *ng_name, *pt, *xref;
  long art_num;
  register newsgroup_t *group;
  
  strcpy(xref_buf,last_xref);
  xref = xref_buf;
  
  while (1) {
    if ( *xref == ' ' || *xref == '\t' ){
      xref++;
      continue;
    }
    ng_name = xref;		/* ニュースグループ名 */
    if ( (pt = strchr(ng_name,':')) == NULL )
      break;
    *pt = 0;
    pt++;
    art_num=atol(pt);		/* 記事番号 */
    xref = pt;
    while('0' <= *xref && *xref <= '9' )
      xref++;

    if ( strcmp(ng_name,cng) == 0 ) /* 見ているニュースグループ */
      continue;
    if (( group = search_group(ng_name)) == 0 )
      continue;			/* グループが存在しない */

    set_readed(group,art_num);
  }
}

/*
 * 参照順に記事を並べ変える
 */
void
article_make_thread(sbj)
subject_t *sbj;
{
  article_t *temptop;
  register article_t **art;
  article_t **ars;
  register char *message_id;
  
  temptop = sbj->art;
  sbj->art = 0;
  ars = &(sbj->art);
  
  while ( temptop != 0 ){
    if ( temptop->next == 0 ) { /* 残りが一つ */
      /* subject queue につなぐ */
      *ars = temptop;
      break;
    }

    art = &temptop;
    while (1) {			/* 参照なしの記事を探す */
      if ((*art)->reference == 0 ||
	  search_ref(temptop,(*art)->reference) == 0 ){	/* 参照なし */
	message_id = (*art)->message_id;
	/* subject queue につなぐ */
	*ars = *art;
	*art = (*art)->next;
	ars = &((*ars)->next);
	*ars = 0;
	/* 子供を探す */
	search_child(&temptop,message_id,&ars,1);
	break;
      }
      if ( (*art)->next == 0 )
	break;			/* M-ID -- Ref: X-< */
      art = &((*art)->next);
    }
  }
}
static int
search_ref(ar,message_id)
register article_t *ar;
register char *message_id;
{
  for ( ; ar != 0 ; ar=ar->next){
    if ( strcmp(ar->message_id,message_id) == 0)
      return(1);		/* 参照記事がキュー内にあり */
  }
  return(0);	/* なし */
}
static void
search_child(temptop,message_id,ars,level)
register article_t **temptop;
register char *message_id;
register article_t ***ars;
int level;
{
  register article_t **ar;
  register char *mes,*from;
  
  for (ar = temptop ; *ar != 0 ; ){
    if ((*ar)->reference != 0 && strcmp((*ar)->reference,message_id) == 0) {
      mes = (*ar)->message_id;

      /* インデント */
      if ( (from = (char *)malloc(strlen((*ar)->from)+level+1)) == NULL )
	memory_error();
      memset(from,' ',level);
      strcpy(&from[level],(*ar)->from);
      free((*ar)->from);
      (*ar)->from = from;
      /* キューのつなぎ直し */
      **ars = *ar;
      *ar = (*ar)->next;
      *ars = &((**ars)->next);
      **ars = 0;
      /* さらに子供を探す */
      search_child(temptop,mes,ars,level+1);
      ar = temptop;
      continue;
    }
    ar = &((*ar)->next);
  }
}


/*
 * 番号順に記事を並べ変える
 */
void
article_make_order(sbj)
subject_t *sbj;
{
  article_t *temptop;
  register article_t **art,**ars, *sv;
  long art_no;
  register char *from,*wk;
  
  temptop = sbj->art;
  sbj->art = 0;
  
  while ( temptop != 0 ){
    art_no = 0;
    /* 一番大きな記事番号を探す */
    for ( art = &temptop; *art != 0; art = &((*art)->next)){
      if ( art_no < (*art)->numb ) {
	art_no = (*art)->numb;
	ars = art;
      }
    }
    /* キューの先頭につなぐ */
    sv = (*ars)->next;
    (*ars)->next = sbj->art;
    sbj->art = *ars;
    *ars = sv;

    /* インデントを戻す */
    from = sbj->art->from;
    while ( *from == ' ' )
      from++;
    if ( (wk = (char *)malloc(strlen(from)+1)) == NULL )
      memory_error();
    strcpy(wk,from);
    free(sbj->art->from);
    sbj->art->from = wk;
  }
}

/*
 * 投稿者検索
 */
static void
article_search(pt)
char *pt;
{
  register int i;
  register article_t *ar;
  
  if ( kb_search(sea_artname, pt) == QUIT )
    return;
  
  for ( i = 0 ,ar = sb->art; i < top && ar != 0 ; i++,ar=ar->next)
    ;
  for (  ; ar != 0 ; ar = ar->next,i++) {
    if( i != top && sregcmp(ar->from,sea_artname) ) {
      top = i;
      return;
    }
  }
  
  mc_printf("文字列 \"%s\" は、ありません \007\n",sea_artname);
}
/*
 * 投稿者検索（逆順）
 */
static void
article_search_rev(pt)
char *pt;
{
  register int i,tmptop;
  register article_t *ar;
  
  if (kb_search(sea_artname, pt) == QUIT )
    return;
  
  tmptop = -1;
  for ( i = 0 ,ar = sb->art; i < top && ar != 0 ; i++,ar=ar->next) {
    if( sregcmp(ar->from,sea_artname) )
      tmptop = i;
  }
  
  if ( tmptop == -1 ) {
    mc_printf("文字列 \"%s\" は、ありません \007\n",sea_artname);
  } else {
    top = tmptop;
  }
}
void
article_mode_help()
{
  static char *os = (char *)OS;
  static struct help_t help_tbl[] = {
    { "アーティクルモード", 0 },
    { "",0 },
    { "<CR>のみ      :最初の未読記事を選択 ", 0 },
    { "数字<CR>      :指定した番号の記事を選択 ", 0 },
    { "a<CR>         :未読の全記事を選択 ", 0 },
    { "p [数]<CR>    :前画面（もしくは指定した数だけ上）を表示する ", 0 },
    { "n [数]<CR>    :次画面（もしくは指定した数だけ下）を表示する ", 0 },
    { "t<CR>         :長い行を折り曲げて表示する／しない（トグル）", 0 },
    { "P<CR>         :前のサブジェクト ", 0 },
    { "N<CR>         :次のサブジェクト ", 0 },
    { "i<CR>         :現在のグループへ記事をポストする ", 0 },
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
#endif /* ! QUICKWIN && ! WINDOWS */
    { "m<CR>         :メイルを送信する ", 0 },
    { "u<CR>         :このグループはもう読まない ", 0 },
    { "S<CR>         :このグループはやっぱり読む ", 0 },
    { "c<CR>         :未読の記事を全部読んだことにする ", 0 },
    { "l<CR>         :参照順に表示する／しない（トグル） ", 0 },
    { "U [No.]<CR>   :指定した番号の記事を未読にする ", 0 },
    { "/ [文字列]<CR>:文字列による投稿者検索 ", 0 },
    { "\\ [文字列]<CR>:文字列による投稿者逆検索 ", 0 },
#if ! defined(QUICKWIN) && ! defined(WINDOWS)
#ifndef	NO_SHELL
    { "! [cmd]<CR>   :%s コマンドを実行する ",&os },
#endif /* NO_SHELL */
#endif /* ! QUICKWIN && ! WINDOWS */
    { "q<CR>         :サブジェクトモードへ ", 0 },
    { "?<CR>         :このヘルプ ", 0 },
#ifdef WINDOWS
    { "",0 },
    { "投稿者表示の上で左ボタンダブルクリック  :その記事を選択 ",0 },
    { "投稿者表示のないところで左ボタンクリック:最初の未読記事を選択 ",0 },
    { "右ボタンクリック                        :サブジェクトモードへ",0 },
#endif /* WINDOWS */
    { 0,0 }
  };
  
  gn_help(help_tbl);
}
