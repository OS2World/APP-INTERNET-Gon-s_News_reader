/*
 * @(#)pager.c	1.40 Aug.21,1997
 *
 * 著作権は放棄しません。ただし、営利目的以外の使用／配布に制限は設けません。
 * Copyright (C) yamasita@omronsoft.co.jp
 */

#if defined(WINDOWS)
#	include	<windows.h>
#	include	"wingn.h"
#endif	/* WINDOWS */

#include <stdio.h>

#include	"nntp.h"
#include	"gn.h"

#ifdef	QUICKWIN
#include	<io.h>

extern void mc_puts();

int
qw_pager(file_name)
char *file_name;
{
  FILE *wp;
  FILE *rp;
  struct _wopeninfo wopen;
  struct _wsizeinfo wsize;
  char resp[NNTP_BUFSIZE];
  FILE *fp;
  int x;
  
  wopen._version = _QWINVER;
  sprintf(resp,"%s:%ld",last_art.newsgroups,last_art.art_numb);
  wopen._title = resp;
  wopen._wbufsize = _WINBUFDEF;
  
  wsize._version = _QWINVER;
  wsize._type = _WINSIZEMAX;
  
  /* ウィンドウのオープン */
  if ( (wp = _fwopen( &wopen, &wsize, "w" )) == NULL ) {
    mc_puts("ページャが実行できません \n");
    return(-1);
  }
  
  if ( (rp = _fdopen(_fileno(wp),"r")) == NULL  ) {
    mc_puts("ページャが実行できません \n");
    return(-1);
  }
  
  if( _wgetsize( _WINFRAMEHAND, _WINCURRREQ, &wsize ) == -1 ) {
    mc_puts("ページャが実行できません \n");
    return(-1);
  }
  gn.lines = wsize._h;
  gn.columns = wsize._w;
  
  if ( (fp = fopen(file_name,"r")) == NULL ) {
    return(-1);
  }
  
  x = 0;
  while (1) {
    if (fgets(resp,NNTP_BUFSIZE,fp) == NULL ) {
      fprintf(wp,"end:");
      fflush(wp);
      fgets(resp,NNTP_BUFSIZE,rp);
      break;
    }
    fprintf(wp,"%s", resp);

    if ( ++x >= gn.lines - 1 ) {
      fprintf(wp,":");
      fflush(wp);
      fgets(resp,NNTP_BUFSIZE,rp);
      if ( resp[0] == 'q' )
	break;
      x = 0;
    }
  }
  fclose(fp);
  
  fclose(rp);
  _wclose( _fileno( wp ), _WINNOPERSIST );
  
  return(0);
}
#endif	/* QUICKWIN */
#ifdef	WINDOWS
extern int mc_MessageBox();
extern int setmenu();
extern void mc_puts();
extern void fast_mc_puts_start(),fast_mc_puts_end();

static FILE *fp;
static long	pager_start,pager_end;	/* 1 orig. */
static int pager_eof;
static int ikc;	/* internal_kanji_code */
void
win_pager(tmp_file,lines)
char *tmp_file;
int lines;
{
  static gnpopupmenu_t pagermode_file_menu[] = {
    { "ページャ終了(\036Q)",
	IDM_PAGQUIT,			0 },
    { 0,0,0 }
  };
  
  static gnpopupmenu_t pagermode_view_menu[] = {
    { "前画面を表示する(\036B)",
	IDM_PAGBACK,			0 },
    { "次画面を表示する(\036 )",
	IDM_PAGCONT,			0 },
    { "１行上を表示する(\036K)",
	IDM_PAGK,				0 },
    { "１行下を表示する(\036J)",
	IDM_PAGONE,			0 },
    { "最後を表示する(\036G)",
	IDM_PAGBOTTOM,		0 },
    { 0,0,0 }
  };
  
  static gnpopupmenu_t pagermode_help_menu[] = {
    { "バージョン情報(\036A)...",
	IDM_ABOUT, 			0 },
    { 0,0,0 }
  };
  
  static gnmenu_t pagermode_menu[] = {
    { "ファイル(\036F)",	pagermode_file_menu },
    { "表示(\036V)",		pagermode_view_menu },
    { "ヘルプ(\036H)",		pagermode_help_menu },
    { 0,0 }
  };
  
  setmenu(pagermode_menu);
  
  fp = fopen(tmp_file,"r");
  pager_start = pager_end = 0L;
  pager_eof = 0;
  
  SetScrollRange(hWnd,SB_VERT,0,lines,FALSE);
  SetScrollPos(hWnd,SB_VERT,0,TRUE);
  
  PostMessage(hWnd, WM_PAGER,0 , 0L);
}
void
win_pager_key(c)
char c;
{
  extern void win_pager_cont(),win_pager_one(),win_pager_back();
  extern void win_pager_k(),win_pager_bottom();
  
  switch ( c ) {
  case ' ':
    if ( ( gn.win_pager_flag & 1 ) != 0 && pager_eof == ON) {
      MessageBeep(MB_OK);
      break;
    }
    win_pager_cont();
    break;
  case '\r':
  case '\n':
  case 'j':
    if ( ( gn.win_pager_flag & 1 ) != 0 && pager_eof == ON) {
      MessageBeep(MB_OK);
      break;
    }
    win_pager_one();
    break;
  case 'b':
    win_pager_back();
    break;
  case 'k':
    win_pager_k();
    break;
  case 'G':
    win_pager_bottom();
    break;
  case 'q':
    PostMessage(hWnd,WM_PAGER_END,0,0L);
    break;
  }
}
void
win_pager_cont()
{
  char resp[NNTP_BUFSIZE+1];
  
  if ( pager_eof == ON ) {
    PostMessage(hWnd, WM_PAGER_END,0 , 0L);
    return;
  }
  
  
  ikc = internal_kanji_code;
  internal_kanji_code = SJIS;
  
  pager_start = pager_end + 1L;
  SetScrollPos(hWnd,SB_VERT,(int)pager_end,TRUE);
  
  fast_mc_puts_start();
  while ( ypos < (gn.lines-1) ) {
    pager_end ++;
    if (fgets(resp,NNTP_BUFSIZE,fp) == NULL ) {
      while ( ypos < (gn.lines-1))
	mc_puts("~\n");
      mc_puts("end:");
      pager_eof = ON;
      fast_mc_puts_end();
      internal_kanji_code = ikc;
      return;
    }
    if ( resp[0] == FF && resp[1] == NL ) {
      mc_puts("^L\n");
      while ( ypos < (gn.lines-1))
	mc_puts("~\n");
      break;
    }
    mc_puts(resp);
  }
  mc_puts(":");
  fast_mc_puts_end();
  internal_kanji_code = ikc;
}
void
win_pager_redraw()
{
  void win_pager_line();
  
  win_pager_line(pager_start);
}
/* b:back */
void
win_pager_back()
{
  void win_pager_line();
  long back_start;
  
  if ( pager_start == 1 ) {
    MessageBeep(MB_OK);
    return;
  }
  
  back_start = pager_start - gn.lines;
  if ( back_start <= 0 )
    back_start = 1;
  
  win_pager_line(back_start);
}
/* k */
void
win_pager_k()
{
  void win_pager_line();
  long back_start;
  
  if ( pager_start == 1 ) {
    MessageBeep(MB_OK);
    return;
  }
  
  back_start = pager_start - 1;
  if ( back_start <= 0 )
    back_start = 1;
  
  win_pager_line(back_start);
}
void
win_pager_line(start_line)
long start_line;	/* 開始行 1 orig. */
{
  char resp[NNTP_BUFSIZE+1];
  
  pager_eof = 0;
  
  rewind(fp);
  
  pager_end = start_line - 1L;
  
  while ( --start_line > 0L )
    fgets(resp,NNTP_BUFSIZE,fp);
  win_pager_cont();
}

/* CR を押した時、１行だけ表示する */
void
win_pager_one()
{
  char resp[NNTP_BUFSIZE+1];
  
  if ( pager_eof == ON ) {
    PostMessage(hWnd, WM_PAGER_END,0 , 0L);
    return;
  }
  pager_start++;
  pager_end++;
  SetScrollPos(hWnd,SB_VERT,(int)pager_start-1,TRUE);
  
  xpos = 0;
  mc_puts(" ");
  xpos = 0;
  if (fgets(resp,NNTP_BUFSIZE,fp) == NULL ) {
    mc_puts("end:");
    pager_eof = ON;
    return;
  }
  
  ikc = internal_kanji_code;
  internal_kanji_code = SJIS;
  mc_puts(resp);
  internal_kanji_code = ikc;
  mc_puts(":");
}
void
win_pager_bottom()
{
  char resp[NNTP_BUFSIZE+1];
  register long l;
  
  rewind(fp);
  
  l = 0;
  while ( fgets(resp,NNTP_BUFSIZE,fp) != NULL )
    l++;
  
  win_pager_line(l-gn.lines+2);
  
  while ( pager_eof != ON )
    win_pager_one();
}
void
win_pager_end()
{
  fclose(fp);
  pager_start = pager_end = 0L;
  pager_eof = 0;
}
#endif	/* WINDOWS */
