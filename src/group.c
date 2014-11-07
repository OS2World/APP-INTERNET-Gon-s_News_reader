/*
 * @(#)group.c	1.40 May.2,1998
 *
 * ������������ޤ��󡣤�������������Ū�ʳ��λ��ѡ����ۤ����¤��ߤ��ޤ���
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
    /* ����ɽ�� */
    group_mode_list();

    /* ���ޥ������ */
    memset(key_buf,0,KEY_BUF_LEN);
    redraw_func = group_mode_list;
    while ( FGETS(key_buf,KEY_BUF_LEN,stdin) == NULL ){
      clearerr(stdin);
      group_mode_list();
    }
    redraw_func = (void (*)())NULL;

    /* ���Υۥ磻�ȥ��ڡ�����ä� */
    pt = &key_buf[strlen(key_buf)-1];
    while( pt >= key_buf && ( *pt == ' ' || *pt == '\t' || *pt == '\n' ) )
      *pt-- = 0;
		
    /* ���Υۥ磻�ȥ��ڡ�����ä� */
    pt = key_buf;
    while(*pt == ' ' || *pt == '\t' )
      pt++;
		
    /* ���ޥ�ɲ��� */
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
    { "�Ǹ���ɤ��������ե��������¸����(\036S)",
	IDM_SAVE,		AFTER_READ|SEPARATOR_NEXT },
#endif /* NO_SAVE */
    { "���ɾ������¸����(\036W)",
	IDM_GRPWRITENEWSRC,	AFTER_READ },
    { "��λ(\036Q)",
	IDM_GRPEND,			0 },
    { "���ɾ���򹹿�������λ(\036X)",
	IDM_GRPQUIT,			0 },
    { 0,0,0 }
  };
  
  static gnpopupmenu_t groupmode_edit_menu[] = {
    { "���롼�ס���å������ɣĤλ���...",
	IDM_GRPCOMMAND,		SEPARATOR_NEXT },

    { "�˥塼�����롼��̾�θ���(\036/)...",
	IDM_GRPSEARCH,		0 },
    { "�˥塼�����롼��̾�εո���(\036\\)...",
	IDM_GRPREVSEARCH,		SEPARATOR_NEXT },

    { "���ꤷ�����롼�פϤ⤦�ɤޤʤ�(\036U)...",
	IDM_GRPUNSUBSCRIBE,	0 },
    { "���ꤷ�����롼�פϤ�äѤ��ɤ�(\036S)...",
	IDM_GRPSUBSCRIBE,		SEPARATOR_NEXT },

    { "���ꤷ�����롼�פε����������ɤ�����Ȥˤ���(\036C)...",
	IDM_GRPCATCHUP,		SEPARATOR_NEXT },

    { "����������������å�����(\036H)",
	IDM_GRPHASH,			0 },
    { 0,0,0 }
  };
  
  static gnpopupmenu_t groupmode_view_menu[] = {
    { "�����̤�ɽ������(\036P)",
	IDM_GRPPREV,			0 },
    { "�����̤�ɽ������(\036N)",
	IDM_GRPNEXT,			0 },
    { "",			/* programable */
	IDM_GRPLIST,			SEPARATOR_NEXT },

    { "�Ǹ���ɤ�������θ�������ɽ������(\036R)",
	IDM_REFERENCE, 		AFTER_READ },
    { 0,0,0 }
  };
  
  static gnpopupmenu_t groupmode_send_menu[] = {
    { "�Ǹ���ɤ�������˥ե���������Ф�(\036F)",
	IDM_FOLLOW, 			AFTER_READ },
    { "�Ǹ���ɤ�������˥�ץ饤�ᥤ���Ф�(\036R)",
	IDM_REPLY, 			AFTER_READ },
    { "�Ǹ���ɤ�������򥭥�󥻥뤹��(\036C)",
	IDM_CANCEL,			AFTER_READ|SEPARATOR_NEXT },

    { "������ݥ��Ȥ���(\036I)...",
	IDM_POST,				0 },
    { "�ᥤ�����������(\036M)...",
	IDM_MAIL, 			0 },
    { 0,0,0 }
  };
  
  static gnpopupmenu_t groupmode_help_menu[] = {
    { "���ޥ�ɰ�����ɽ��(\036?)...",
	IDM_GRPHELP, 			0 },
    { "�С���������(\036A)...",
	IDM_ABOUT, 			0 },
    { 0,0,0 }
  };
  
  static gnmenu_t groupmode_menu[] = {
    { "�ե�����(\036F)",	groupmode_file_menu },
    { "�Խ�(\036E)",		groupmode_edit_menu },
    { "ɽ��(\036V)",		groupmode_view_menu },
    { "ȯ��(\036S)",		groupmode_send_menu },
    { "�إ��(\036H)",		groupmode_help_menu },
    { 0,0 }
  };
  
  static char
    *disp_all    = "�����롼�פ�ɽ������(\036L)",
    *disp_notall = "̤�ɤε��������륰�롼�פΤߤ�ɽ������(\036L)";
  
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
  
  /* ���ˤ����˥塼��ä� */
  while ( (hmenupopup = GetSubMenu(hmenu,0)) != NULL ) {
    if ( DeleteMenu(hmenu,0,MF_BYPOSITION) == 0 )
      return(-1);
  }
  
  /* ��������˥塼������ */
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
    if ((pt = kb_input("�˥塼�����롼�פ��ֹ桢�ޤ���̾�������Ϥ��Ʋ�����")) == NULL )
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
 * �˥塼�����롼�ץ⡼�ɰ���ɽ��
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

  mc_puts("�˥塼�����롼�ץ⡼��\n\n");
#else	/* WINDOWS */
  
  kanji_convert(internal_kanji_code,"�˥塼�����롼�ץ⡼��",
		gn.display_kanji_code,buf);
  SetWindowText(hWnd,buf);
  
  fast_mc_puts_start();
#endif	/* WINDOWS */
  
  mc_puts("  �ֹ� ������ ���롼��̾\n");
  
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
	if ( ng->flag & (UNSUBSCRIBE | NOARTICLE)) /* ���ɡ������ʤ� */
	  continue;
	if ( ng->numb <= 0 ) {	/* �����������ʤ� */
	  ng->flag |= NOARTICLE;
	  continue;
	}
      } else {
	if ( ng->last < ng->read ) { /* .newsrc �۾� */
	  ng->read = ng->last;
	  ng->numb = 0;
	}
      }
      if ( start == -1 )
	start = i;		/* �ǽ�ε��� */
      if ( ng->flag & UNSUBSCRIBE ) /* ���ɤʤ� */
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
	mc_puts("\n̤�ɤε���������ޤ��� \n\007");
      } else {
	top -= EFFECT_LINES;
	if ( top < 0 )
	  top = 0;
	continue;
      }
    }
    break;
  }
  mc_puts("�˥塼�����롼�פ��ֹ桿̾���ޤ��ϥ��ޥ�ɡ�help=?�ˡ�");
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
 * �˥塼�����롼�ץ⡼�ɥ��ޥ��
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
  
  /* �����ǻϤޤ�Τϡ��˥塼�����롼�פ��ֹ� */
  if ( '0' <= *pt && *pt <= '9' )	/* �������ֹ�Υ��롼�� */
    return(goto_group(atoi(pt)));
  
  /* �˥塼�����롼�פ�̾������ */
  if ( strlen(pt) >= 2 && pt[1] != ' ' && pt[0] != '<') {
    if ( (ng = search_group(pt)) == (newsgroup_t*)0 ) {
      mc_puts("\n����Υ��롼�פ��ߤĤ���ޤ��� \n\007");
#ifndef	WINDOWS
      if ( group_lookup(pt) == QUIT )
	return(CONT);
      pt = kb_input("�˥塼�����롼�פ��ֹ�����Ϥ��Ʋ�����");
      while(*pt == ' ' || *pt == '\t' )
	pt++;
      if ( '0' <= *pt && *pt <= '9' ) /* �������ֹ�Υ��롼�� */
	return(goto_group(atoi(pt)));
#endif /* WINDOWS */
      return(CONT);
    } else {
      return(goto_ng(ng));
    }
  }
  
  if ( pt[1] == 0 ) {		/* ��ʸ�������Υ��ޥ�� */
    switch ( *pt ) {
    case 0:			/* CR�Τߡ��ǽ�Υ��롼�� */
      if ( start == -1 ){
#ifndef	WINDOWS
	mc_puts("\n̤�ɤε���������ޤ��� \n\007");
#endif /* WINDOWS */
	return(CONT);
      }
      return(goto_group(start));
    case 'h':			/* �ϥå��� */
      for ( ng = newsrc; ng != 0 ; ng=ng->next)
	ng->flag &= ~(NOARTICLE|ACTIVE);
      /* �����ƥ��֥ե�����μ��� */
      switch ( re_get_active() ) {
      case CONNECTION_CLOSED:
	return(CONNECTION_CLOSED);
      case ERROR:
	return(QUIT);
      }
      check_group(0);
      top = 0;
      return(CONT);
    case 'i':			/* �ݥ��� */
      return(post_mode(NULL));
    case 'R':			/* reference */
      return(reference_mode());
    case 'f':			/* �ե��� */
      return(followup_mode());
    case 'r':			/* ��ץ饤 */
      reply_mode();
      return(CONT);
    case 'm':			/* �ᥤ�� */
      mail_mode();
      return(CONT);
    case 'C':			/* ����󥻥� */
      return(cancel_mode());
    case 'K':
      kill_mode();		/* KILL */
      return(CONT);
    case 'w':			/* .newsrc ����¸����³ */
      if ( change ) {
	save_newsrc();		/* .newsrc ����¸ */
	change = 0;
      }
      return(CONT);
    case 'q':			/* ��λ */
      return(END);
    case 'x':			/* newsrc �򹹿�������λ */
      return(QUIT);
    case '?':			/* help */
      group_mode_help();
      return(CONT);
    }
  }
  
  /* ������Ȥ륳�ޥ�� */
  switch ( *pt ) {
  case 'p':			/* ������ */
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
    
  case 'n':			/* ������ */
    pt++;
    while (*pt == ' ')
      pt++;
    arg = atoi(pt);
    if ( arg <= 0 )
      top += EFFECT_LINES;
    else
      top += arg;
    
    return(CONT);
    
  case 'l':			/* �����롼��ɽ�� */
    group_list_mode(pt);
    return(CONT);

#ifndef	NO_SAVE
  case 's':			/* ������ */
    save_mode(pt);
    return(CONT);
#endif /* NO_SAVE */

  case 'u':			/* �⤦�ɤޤʤ� */
    group_unsubscribe(pt);
    return(CONT);
    
  case 'S':			/* ��äѤ��ɤ� */
    group_subscribe(pt);
    return(CONT);
    
  case 'c':			/* �����ɤ�����Ȥˤ��� */
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
    
  case '/':			/* �˥塼�����롼�פ�ʸ���󸡺� */
    group_search(pt);
    return(CONT);
    
  case '\\':			/* �˥塼�����롼�פ�ʸ���󸡺��ʵս��*/
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
      continue;			/* ���ɤʤ� */
    if ( i == num )
      return(goto_ng(ng));
    i++;
  }
#ifdef	WINDOWS
  mc_MessageBox(hWnd,"���ꤵ�줿�ֹ�Υ��롼�פ�¸�ߤ��ޤ��� ","",MB_OK);
#else	/* Windows */
  mc_puts("\n���ꤵ�줿�ֹ�Υ��롼�פ�¸�ߤ��ޤ��� \n");
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
 * ���ɤ��ʤ�
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
    if ( (pt = kb_input("���ɤ��ʤ�����˥塼�����롼�פ��ֹ���ϰϡˡ��ޤ���̾������ꤷ�Ʋ�����")) == NULL )
      return;
    while( *pt == ' ' || *pt == '\t' )
      pt++;
  }
  
  /* �ΣǤ��ֹ����  */
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
	    (ng->flag & (UNSUBSCRIBE|NOARTICLE)) ) /* ���ɤʤ� */
	  continue;
	ed++;
      }
    }
    
    /* �ǽ�ΣΣǤ�õ�� */
    for ( ng = newsrc,l = 0 ; ng != 0 ; ng = ng->next) {
      if ( (ng->flag & NOLIST) != 0 )
	continue;
      if ( all_flag == 0 &&
	  (ng->flag & (UNSUBSCRIBE|NOARTICLE)) ) /* ���ɤʤ� */
	continue;
      if ( l == st )
	break;
      l++;
    }
    if ( l != st || ng == 0){
#ifdef	WINDOWS
      mc_MessageBox(hWnd,"���ꤵ�줿�ֹ�Υ��롼�פ�¸�ߤ��ޤ��� ","",MB_OK);
#else  /* WINDOWS */
      mc_puts("\n���ꤵ�줿�ֹ�Υ��롼�פ�¸�ߤ��ޤ��� \n");
      hit_return();
#endif /* WINDOWS */
      return;
    }
    
    /* ��ǧ���� */
    for ( l = st ; l <= ed ; l++) {
      if ( (ng->flag & NOLIST) != 0 )
	continue;
      if ( all_flag != 0 ||
	  ( ng->flag & (UNSUBSCRIBE|NOARTICLE)) == 0 ){
#ifndef	WINDOWS
	sprintf(resp,
		"%s �Ϥ⤦�ꥹ�ȥ��åפ��ޤ���(y:λ��/a:����/����¾:���)",
		ng->name);
	key_buf = kb_input(resp);
	c = *key_buf;
#else  /* WINDOWS */
	sprintf(group_name,
		"%s �Ϥ⤦�ꥹ�ȥ��åפ��ޤ��� ",ng->name);
	c = DialogBox(hInst,	/* ���ߤΥ��󥹥��� */
		      "IDD_GROUP_UNSUBSCRIBE_CONF", /* �꥽���� */
		      hWnd,	/* �ƥ�����ɥ��Υϥ�ɥ� */
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
	    (ng->flag & (UNSUBSCRIBE|NOARTICLE)) ) /* ���ɤʤ� */
	  continue;
	break;
      }
      if ( ng == 0 )
	break;
    }
    return;
  }
  
  /* �ΣǤ�̾������ */
  if ( (wk = strchr(pt,'\n')) != 0 )
    *wk = 0;
  
  if ( (ng = search_group(pt)) == (newsgroup_t*)0 ) {
#ifdef	WINDOWS
    mc_MessageBox(hWnd,"����Υ��롼�פ��ߤĤ���ޤ��� ","",MB_OK);
#else  /* WINDOWS */
    mc_puts("\n����Υ��롼�פ��ߤĤ���ޤ��� \n\007");
    hit_return();
#endif /* WINDOWS */
  } else {
    if ( ( ng->flag & UNSUBSCRIBE) == 0 ){
#ifndef	WINDOWS
      sprintf(resp,
	      "%s �Ϥ⤦�ꥹ�ȥ��åפ��ޤ���(y:λ��/����¾:���)",
	      ng->name);
      key_buf = kb_input(resp);
      if ( *key_buf == 'y' || *key_buf == 'Y' ){
	ng->flag |= UNSUBSCRIBE;
	change = 1;
      }
#else  /* WINDOWS */
      sprintf(resp,
	      "%s �Ϥ⤦�ꥹ�ȥ��åפ��ޤ��� ",
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
HWND hDlg;                       /* ���������ܥå���������ɥ��Υϥ�ɥ� */
unsigned message;                /* ��å������Υ�����                     */
WORD wParam;                     /* ��å�������ͭ�ξ���                   */
LONG lParam;
{
  char sendtext[255];
  
  switch (message) {
  case WM_INITDIALOG:
    kanji_convert(internal_kanji_code,group_name,
		  gn.display_kanji_code,sendtext);
    SetDlgItemText(hDlg, IDC_UNSUBSCRIBE,(LPSTR)sendtext);
    kanji_convert(internal_kanji_code,"�Ϥ�(\036Y)",
		  gn.display_kanji_code, sendtext);
    SetDlgItemText(hDlg, IDOK, sendtext);
    kanji_convert(internal_kanji_code,"������(\036N)",
		  gn.display_kanji_code, sendtext);
    SetDlgItemText(hDlg, IDCANCEL, sendtext);
    kanji_convert(internal_kanji_code,"����(\036A)",
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
 * ���ɤ���
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
    if ((pt = kb_input("���ɤ���˥塼�����롼�פ��ֹ���ϰϡˡ��ޤ���̾������ꤷ�Ʋ�����")) == NULL)
      return;
    while( *pt == ' ' || *pt == '\t' )
      pt++;
  }
  
  /* �ΣǤ��ֹ����  */
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
    
    /* �ǽ�ΣΣǤ�õ�� */
    for ( ng = newsrc,l = 0 ; ng != 0 ; ng = ng->next) {
      if ( (ng->flag & NOLIST) != 0 )
	continue;
      if ( all_flag == 0 &&
	  (ng->flag & (UNSUBSCRIBE|NOARTICLE)) ) /* ���ɤʤ� */
	continue;
      if ( l == st )
	break;
      l++;
    }
    if ( l != st || ng == 0){
#ifdef	WINDOWS
      mc_MessageBox(hWnd,"���ꤵ�줿�ֹ�Υ��롼�פ�¸�ߤ��ޤ��� ","",MB_OK);
#else  /* WINDOWS */
      mc_puts("\n���ꤵ�줿�ֹ�Υ��롼�פ�¸�ߤ��ޤ��� \n");
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
  
  /* �ΣǤ�̾������ */
  if ( (wk = strchr(pt,'\n')) != 0 )
    *wk = 0;
  
  if ( (ng = search_group(pt)) == (newsgroup_t*)0 ) {
#ifdef	WINDOWS
    mc_MessageBox(hWnd,"����Υ��롼�פ��ߤĤ���ޤ��� ","",MB_OK);
#else  /* WINDOWS */
    mc_puts("\n����Υ��롼�פ��ߤĤ���ޤ��� \n\007");
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
 * �����ɤ�����Ȥˤ���
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
    if ((pt = kb_input("�˥塼�����롼�פ��ֹ桢�ޤ���̾������ꤷ�Ʋ�����")) == NULL)
      return;
    while( *pt == ' ' || *pt == '\t' )
      pt++;
  }
  
  /* �ΣǤ��ֹ����  */
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
	    (ng->flag & (UNSUBSCRIBE|NOARTICLE)) ) /* ���ɤʤ� */
	  continue;
	ed++;
      }
    }
    
    /* �ǽ�ΣΣǤ�õ�� */
    for ( ng = newsrc,l = 0 ; ng != 0 ; ng = ng->next) {
      if ( (ng->flag & NOLIST) != 0 )
	continue;
      
      if ( all_flag == 0 &&
	  (ng->flag & (UNSUBSCRIBE|NOARTICLE)) ) /* ���ɤʤ� */
	continue;
      if ( l == st )
	break;
      l++;
    }
    if ( l != st || ng == 0){
#ifdef	WINDOWS
      mc_MessageBox(hWnd,"���ꤵ�줿�ֹ�Υ��롼�פ�¸�ߤ��ޤ��� ","",MB_OK);
#else  /* WINDOWS */
      mc_puts("\n���ꤵ�줿�ֹ�Υ��롼�פ�¸�ߤ��ޤ��� \n");
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
	    (ng->flag & (UNSUBSCRIBE|NOARTICLE)) ) /* ���ɤʤ� */
	  continue;
	break;
      }
      if ( ng == 0 )
	break;
    }
    return;
  }
  
  /* �ΣǤ�̾������ */
  if ( (wk = strchr(resp,'\n')) != 0 )
    *wk = 0;
  
  if ( (ng = search_group(pt)) == (newsgroup_t*)0 ) {
#ifdef	WINDOWS
    mc_MessageBox(hWnd,"����Υ��롼�פ��ߤĤ���ޤ��� ","",MB_OK);
#else  /* WINDOWS */
    mc_puts("\n����Υ��롼�פ��ߤĤ���ޤ��� \n\007");
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
 * �˥塼�����롼�פ�ʸ���󸡺�
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
      if ( ng->flag & (UNSUBSCRIBE | NOARTICLE)) /* ���ɡ������ʤ� */
	continue;
      if ( ng->numb <= 0 ) {	/* �����������ʤ� */
	ng->flag |= NOARTICLE;
	continue;
      }
    } else {
      if ( ng->last < ng->read ) { /* .newsrc �۾� */
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
  mc_printf("ʸ���� \"%s\" �ϡ�����ޤ��� \007\n",sea_grpname);
}
/*
 * �˥塼�����롼�פ�ʸ���󸡺��ʵս��
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
      if ( ng->flag & (UNSUBSCRIBE | NOARTICLE)) /* ���ɡ������ʤ� */
	continue;
      if ( ng->numb <= 0 ) {	/* �����������ʤ� */
	ng->flag |= NOARTICLE;
	continue;
      }
    } else {
      if ( ng->last < ng->read ) { /* .newsrc �۾� */
	ng->read = ng->last;
	ng->numb = 0;
      }
    }
    if( i != top && sregcmp(ng->name,sea_grpname) )
      tmptop = i;
    i++;
  }
  
  if ( tmptop == -1 ) {
    mc_printf("ʸ���� \"%s\" �ϡ�����ޤ��� \007\n",sea_grpname);
  } else {
    top = tmptop;
  }
}

/*
 * ����ʸ���������
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
      if ( (pt = kb_input("��������ʸ�������ꤷ�Ʋ�����")) == NULL )
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
 * ���Ƥ���˥塼�����롼��̾��õ��
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
  int list_count;		/* �ꥹ�Ȥ���Կ� */
  
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
      continue;			/* ���ɤʤ� */
    numb++;
    if ( (para = fstrcmp(ng->name,name)) <= 0)
      continue;			/* �������פ��ʤ� */
    
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
    if ( list[i].ng->flag & UNSUBSCRIBE ) /* ���ɤʤ� */
      mc_printf("U");
    else
      mc_printf(" ");
    mc_printf(" %4d  %4ld  ",list[i].numb,list[i].ng->numb);
    mc_puts(list[i].ng->name);
    mc_printf("\n");
  }
  return(CONT);
}

#define	MAXPOINT	30	/* �����˰��פ����������� */
#define SEARCH_CHAR	5	/* �����õ��ʸ���� */
#define	MINPOINT	10	/* ���줿������������� */
#define	MINPOINTDEC	1	/* ���줿�������������� */

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
  
  point -= strlen(a);	/* ʸ�����κ������ʤ����˹�������Ϳ������ */
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
    { "�˥塼�����롼�ץ⡼��",0 },
    { "",0 },
    { "<CR>�Τ�      :�ǽ�Υ��롼�פ����� ", 0 },
    { "����<CR>      :���ꤷ���ֹ�Υ��롼�פ����� ", 0 },
    { "̾��<CR>      :���ꤷ��̾���Υ��롼�פ����� ", 0 },
    { "p [��]<CR>    :�����̡ʤ⤷���ϻ��ꤷ����������ˤ�ɽ������ ", 0 },
    { "n [��]<CR>    :�����̡ʤ⤷���ϻ��ꤷ�����������ˤ�ɽ������ ", 0 },
    { "h<CR>         :����������������å����� ", 0 },
    { "l [NG]<CR>    :ɽ���⡼�ɤ��ڤ��ؤ��� ", 0 },
    { "i<CR>         :������ݥ��Ȥ��� ", 0 },
    { "�ɣ�<CR>      :���ꤷ����å������ɣĤε�����ɽ������ ", 0 },
    { "R<CR>         :�Ǹ���ɤ�������θ�������ɽ������ ", 0 },
    { "f<CR>         :�Ǹ���ɤ�������˥ե���������Ф� ", 0 },
    { "r<CR>         :�Ǹ���ɤ�������˥�ץ饤�ᥤ���Ф� ", 0 },
    { "C<CR>         :�Ǹ���ɤ�������򥭥�󥻥뤹�� ", 0 },
#ifndef	NO_SAVE
    { "s [file]<CR>  :�Ǹ���ɤ��������ե��������¸���� ", 0 },
#endif /* NO_SAVE */
#if ! defined(QUICKWIN) && ! defined(WINDOWS)
#ifndef	NO_PIPE
    { "| [cmd]<CR>   :�Ǹ���ɤ�������򥳥ޥ�ɤ��Ϥ� ", 0 },
#endif /* NO_PIPE */
#endif /* ! QUICKWIN && ! WINDOWS  */
    { "m<CR>         :�ᥤ����������� ", 0 },
    { "u [NG]<CR>    :���ꤷ�����롼�פϤ⤦�ɤޤʤ� ", 0 },
    { "S [NG]<CR>    :���ꤷ�����롼�פϤ�äѤ��ɤ� ", 0 },
    { "c [NG]<CR>    :���ꤷ�����롼�פε����������ɤ�����Ȥˤ��� ", 0 },
    { "/ [ʸ����]<CR>:ʸ����ˤ��˥塼�����롼��̾���� ", 0 },
    { "\\ [ʸ����]<CR>:ʸ����ˤ��˥塼�����롼��̾�ո��� ", 0 },
#if ! defined(QUICKWIN) && ! defined(WINDOWS)
#ifndef	NO_SHELL
    { "! [cmd]<CR>   :%s ���ޥ�ɤ�¹Ԥ��� ",&os},
#endif /* NO_SHELL */
#endif /* ! QUICKWIN && ! WINDOWS */
    { "w<CR>         :���ɾ���� %s ����¸���� ",&rcfile_tmp_p},
    { "q<CR>         :��λ ", 0 },
    { "x<CR>         :%s �򹹿�������λ ",&rcfile_tmp_p},
    { "?<CR>         :���Υإ�� ", 0 },
#ifdef WINDOWS
    { "",0 },
    { "���롼��ɽ���ξ�Ǻ��ܥ�����֥륯��å�  :���Υ��롼�פ����� ",0 },
    { "���롼��ɽ���Τʤ��Ȥ���Ǻ��ܥ��󥯥�å�:�ǽ�Υ��롼�פ����� ",0 },
    { "���ܥ��󥯥�å�                          :��λ",0 },
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
    mc_MessageBox(hWnd,"�ƥ�ݥ��ե�����κ����˼��Ԥ��ޤ���",
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
    mc_MessageBox(hWnd,"�ƥ�ݥ��ե�����κ����˼��Ԥ��ޤ���",
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
