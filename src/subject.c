/*
 * @(#)subject.c	1.40 Mar.26,1998
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
extern void mc_puts(),mc_printf();
extern int get_1_header_file();
extern void hit_return();
extern int kb_search();
extern int sregcmp();
extern int put_server();
extern int get_server();
extern int get_1_header();
extern void kanji_convert();
extern int group_command();
extern void article_make_thread();
extern void article_make_order();
extern int show_article();
extern void str_cut();
extern void memory_error();
extern void gn_help();
extern void add_esc();
extern char *Fgets();
extern int last_art_check();

#if defined(WINDOWS)
extern int mc_MessageBox();
extern int setmenu();
extern void fast_mc_puts_start(),fast_mc_puts_end();
extern void set_hourglass(),reset_cursor();
#endif	/* WINDOWS */

#ifdef	MIME
extern void MIME_strHeaderDecode();
#endif

#ifdef USE_JNAMES
extern char *jnFetch();
#endif

static newsgroup_t *ng;
static int top, start;
static long sb_s,sb_e;	/*  ���֥������ȥ��塼��ͭ�뵭���ϰ� */

static char sea_subname[80] = "";

static int exit_group();
static void erase_subject_queue(),newsrc_check(),
	subject_search(), subject_search_rev();
static subject_catchup(),
	enter_group(),goto_article();
static kill_list_t *alloc_kill_queue();
static subject_t *alloc_subject_queue(), *join_subject_queue();
static article_t *alloc_article_queue();
static void free_subject_queue(),free_article_queue();

static long	tmp_first,tmp_last;

static kill_list_t	*kill_list = (kill_list_t *)0;

#ifndef	WINDOWS
int
subject_mode(group)
register newsgroup_t *group;
{
  void subject_mode_list();
  int subject_mode_command();
  
  register char *pt;
  char key_buf[KEY_BUF_LEN];
  int rtcode;
  
  ng = group;
  
  while (1) {
    top = 0;
    sb_s = ng->read+1;
    sb_e = ng->last;
    if ( sb_s > sb_e )		/* ���������ɤ�Ǥ��� */
      sb_s = ng->first;		/* ���ס����κǽ�ε������� */

    switch ( enter_group(&sb_s,&sb_e) ) {
    case CONNECTION_CLOSED:
      return(CONNECTION_CLOSED);
    case ERROR:
      return(ERROR);
    case QUIT:
      return(CONT);
    }

    if ( ng->subj == 0 ) {	/* ���ס����˵������ʤ� */
      if ( exit_group() == ERROR )
	return(ERROR);
      return(CONT);
    }
    while(1) {
      /* ����ɽ�� */
      subject_mode_list();
			
      /* ���ޥ������ */
      memset(key_buf,0,KEY_BUF_LEN);
      redraw_func = subject_mode_list;
      while ( FGETS(key_buf,KEY_BUF_LEN,stdin) == NULL ){
	clearerr(stdin);
	subject_mode_list();
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
      rtcode = subject_mode_command(pt);
      if ( rtcode == CONNECTION_CLOSED )
	return(CONNECTION_CLOSED);
      if ( rtcode == ERROR )
	return(ERROR);
      if ( rtcode == QUIT )	/* l ���ޥ�ɤǡ�q ������ */
	return(CONT);

      if ( rtcode == END ) {
	if ( exit_group() == ERROR )
	  return(ERROR);
	return(CONT);
      }
      if ( rtcode == NEXT )	/* ���Υ��롼�� */
	break;
    }
  }
}
#else	/* WINDOWS */
subject_mode(group)
register newsgroup_t *group;
{
  if ( group != 0 )
    ng = group;
  top = 0;
  sb_s = ng->read+1;
  sb_e = ng->last;
  if ( sb_s > sb_e )		/* ���������ɤ�Ǥ��� */
    sb_s = ng->first;	/* ���ס����κǽ�ε������� */
  
  switch ( enter_group(&sb_s,&sb_e) ) {
  case CONNECTION_CLOSED:
    return(CONNECTION_CLOSED);
  case ERROR:
    return(ERROR);
  case QUIT:
    return(QUIT);
  }
  
  if ( ng->subj == 0 ) {	/* ���ס����˵������ʤ� */
    if ( exit_group() == ERROR )
      return(ERROR);
    return(QUIT);
  }
  
  return(CONT);
}
set_subjectmode_menu()
{
  static gnpopupmenu_t subjectmode_file_menu[] = {
#ifndef	NO_SAVE
    { "�Ǹ���ɤ��������ե��������¸����(\036S)",
	IDM_SAVE,		AFTER_READ|SEPARATOR_NEXT },
#endif /* NO_SAVE */
    { "���Υ˥塼�����롼�פ�(\036P)",
	IDM_SUBPREVGRP,		0 },
    { "���Υ˥塼�����롼�פ�(\036N)",
	IDM_SUBNEXTGRP,		0 },
    { "�˥塼�����롼�ץ⡼�ɤ�(\036Q)",
	IDM_SUBQUIT,			0 },
    { 0,0,0 }
  };
  
  static gnpopupmenu_t subjectmode_edit_menu[] = {
    { "���֥��������ֹ桢��å������ɣĤλ���...",
	IDM_SUBCOMMAND,		SEPARATOR_NEXT },

    { "���֥������Ȥθ���(\036/)...",
	IDM_SUBSEARCH,		0 },
    { "���֥������Ȥεո���(\036\\)...",
	IDM_SUBREVSEARCH,		SEPARATOR_NEXT },

    { "���Υ��롼�פϤ⤦�ɤޤʤ�(\036U)",
	IDM_SUBUNSUBSCRIBE,	0 },
    { "���Υ��롼�פϤ�äѤ��ɤ�(\036S)",
	IDM_SUBSUBSCRIBE,		SEPARATOR_NEXT },

    { "̤�ɤε����������ɤ�����Ȥˤ���(\036C)",
	IDM_SUBCATCHUP,		0 },
    { 0,0,0 }
  };
  
  static gnpopupmenu_t subjectmode_view_menu[] = {
    { "�����̤�ɽ������(\036P)",
	IDM_SUBPREV,			0 },
    { "�����̤�ɽ������(\036N)",
	IDM_SUBNEXT,			0 },
    { "",			/* programable */
	IDM_SUBTRUNC,			SEPARATOR_NEXT },

    { "�����ε����Υ��֥�������ɽ������(\036L)",
	IDM_SUBLIST,			SEPARATOR_NEXT },

    { "�Ǹ���ɤ�������θ�������ɽ������(\036R)",
	IDM_REFERENCE,		AFTER_READ|SEPARATOR_NEXT },

    { "̤�ɤ������������򤹤�(\036A)",
	IDM_SUBALL,			0 },
    { 0,0,0 }
  };
  
  static gnpopupmenu_t subjectmode_send_menu[] = {
    { "�Ǹ���ɤ�������˥ե���������Ф�(\036F)",
	IDM_FOLLOW,			AFTER_READ },
    { "�Ǹ���ɤ�������˥�ץ饤�ᥤ���Ф�(\036R)",
	IDM_REPLY,			AFTER_READ },
    { "�Ǹ���ɤ�������򥭥�󥻥뤹��(\036C)",
	IDM_CANCEL,			AFTER_READ|SEPARATOR_NEXT },

    { "������ݥ��Ȥ���(\036I)...",
	IDM_POST,				0 },
    { "�ᥤ�����������(\036M)...",
	IDM_MAIL,				0 },
    { 0,0,0 }
  };
  
  static gnpopupmenu_t subjectmode_help_menu[] = {
    { "���ޥ�ɰ�����ɽ��(\036?)...",
	IDM_SUBHELP, 			0 },
    { "�С���������(\036A)...",
	IDM_ABOUT, 			0 },
    { 0,0,0 }
  };
  
  static gnmenu_t subjectmode_menu[] = {
    { "�ե�����(\036F)",	subjectmode_file_menu },
    { "�Խ�(\036E)",		subjectmode_edit_menu },
    { "ɽ��(\036V)",		subjectmode_view_menu },
    { "ȯ��(\036S)",		subjectmode_send_menu },
    { "�إ��(\036H)",		subjectmode_help_menu },
    { 0,0 }
  };
  
  static char
    *disp_trunc    = "Ĺ���Ԥ��ޤ�ʤ���ɽ������(\036T)",
    *disp_nottrunc = "Ĺ���Ԥ��ޤ�ʤ���ɽ�����ʤ�(\036T)";
  
  gnpopupmenu_t *gnpm;
  
  for ( gnpm = subjectmode_view_menu ; gnpm->item != 0 ; gnpm++ ) {
    if ( gnpm->idm != IDM_SUBTRUNC )
      continue;
    if ( gn.show_truncate == ON )
      gnpm->item = disp_nottrunc;
    else
      gnpm->item = disp_trunc;
    break;
  }
  
  return(setmenu(subjectmode_menu));
}
subjectmode_menu_exec(wParam)
WPARAM wParam;
{
  void win_subject_mode_command();
  void subject_mode_list();
  
  register char *pt;
  char key_buf[2];
  
  pt = key_buf;
  key_buf[1] = 0;
  
  switch ( wParam ) {
  case IDM_SUBCOMMAND:
    if ( (pt = kb_input("���֥������Ȥ��ֹ�����Ϥ��Ʋ�����")) == NULL )
      return(0);
    break;
  case IDM_SUBPREV:
    *pt = 'p';
    break;
  case IDM_SUBNEXT:
    *pt = 'n';
    break;
  case IDM_SUBTRUNC:
    *pt = 't';
    break;
  case IDM_SUBALL:
    *pt = 'a';
    break;
  case IDM_SUBUNSUBSCRIBE:
    *pt = 'u';
    break;
  case IDM_SUBSUBSCRIBE:
    *pt = 'S';
    break;
  case IDM_SUBCATCHUP:
    *pt = 'c';
    break;
  case IDM_SUBLIST:
    *pt = 'l';
    break;
  case IDM_SUBSEARCH:
    *pt = '/';
    break;
  case IDM_SUBREVSEARCH:
    *pt = '\\';
    break;
  case IDM_SUBPREVGRP:
    *pt = 'P';
    break;
  case IDM_SUBNEXTGRP:
    *pt = 'N';
    break;
  case IDM_SUBQUIT:
    *pt = 'q';
    break;
  case IDM_SUBHELP:
    *pt = '?';
    break;
  default:
    return(0);
  }
  
  win_subject_mode_command(pt);
  
  if ( wParam == IDM_SUBTRUNC )
    set_subjectmode_menu();
  
  return(0);
}
void
win_subject_mode_command(pt)
char *pt;
{
  extern int subject_mode_command();
  
  switch ( subject_mode_command(pt) ) {
  case CONT:
    subject_mode_list();
    return;
  case SELECT:			/* article mode */
    return;
  case END:
    if ( exit_group() == ERROR )
      return;
    PostMessage(hWnd, WM_GROUPMODE,0 , 0L);
    return;
  case NEXT:			/* ���Υ��롼�� */
    return;
  case QUIT:			/* l ���ޥ�ɤǡ�q ������ */
    return;
  case CONNECTION_CLOSED:
    return;
  case ERROR:
    return;
  }
  return;
}
#endif	/* WINDOWS */

/*
 * ���֥������ȥ⡼�ɰ���ɽ��
 */
void
subject_mode_list()
{
  register int i,count;
  register subject_t *sb;
  register char *pt;
  char buf[NNTP_BUFSIZE+1];
#ifdef	WINDOWS
  char buf2[NNTP_BUFSIZE+1];
#endif	/* WINDOWS */
  int	sb_width;
  
  start = -1;
#ifndef WINDOWS
#ifdef QUICKWIN
  mc_puts("\n");
#else
  if ( gn.cls == ON )
    mc_puts(CLS_STRING);
  else
    mc_puts("\n");
#endif

  if ( gn.show_indent > 0 )
    mc_printf("%*c", gn.show_indent, ' ');
  mc_printf("���֥������ȥ⡼�ɡ�%s��\n\n",ng->name);
#else	/* WINDOWS */
  sprintf(buf,"���֥������ȥ⡼�� %s",ng->name);
  kanji_convert(internal_kanji_code,buf,
		gn.display_kanji_code,buf2);
  SetWindowText(hWnd,buf2);
  
  fast_mc_puts_start();
#endif	/* WINDOWS */
  
  if ( gn.show_indent > 0 )
    mc_printf("%*c", gn.show_indent, ' ');
  mc_puts("�ֹ� ������ ���֥�������\n");
  
  while(1){
    for ( i = 0 ,sb = ng->subj; i < top && sb != 0 ; i++,sb=sb->next)
      ;
    count = 0;
    start = -1;
    for ( i = top  ; sb != 0 ; i++,sb = sb->next) {
      sb_width = gn.columns;
      if ( gn.show_indent > 0 ) {
	mc_printf("%*c", gn.show_indent, ' ');
	sb_width -= gn.show_indent;
      }

      mc_printf("%4d %5ld  ",i,sb->numb);
      sb_width -= 4 + 1 + 5 + 2;

      if ( gn.disp_re == ON && (sb->flag & NOTREONLY) == 0 ) {
	mc_printf("Re: ");
	sb_width -= 4;
      }
      if ( gn.show_truncate == ON ) {
	mc_puts(sb->name);
	mc_printf("\n");
      } else {
	strcpy(buf,sb->name);
	if ( (pt = strchr(buf,'\n')) != NULL )
	  *pt = 0;
	str_cut(buf,sb_width);
	mc_printf("%s\n",buf);
      }
      if ( start == -1 && sb->numb != 0 )
	start = i;
      if ( ++count >= EFFECT_LINES )
	break;
    }
		
    if ( count == 0 ){
      if ( top == 0 )
	break;
      top -= EFFECT_LINES;
      if ( top < 0 )
	top = 0;
      continue;
    }
    break;
  }
  if ( gn.show_indent > 0 )
    mc_printf("%*c", gn.show_indent, ' ');
  mc_puts("���֥������Ȥ��ֹ�ޤ��ϥ��ޥ�ɡ�help=?�ˡ�");
#ifndef	WINDOWS
  fflush(stdout);
#else	/* WINDOWS */
  fast_mc_puts_end();
  
  count = 0;
  for ( sb = ng->subj ; sb != 0 ; sb=sb->next)
    count++;
  
  SetScrollRange(hWnd,SB_VERT,0,count,FALSE);
  SetScrollPos(hWnd,SB_VERT,top,TRUE);
#endif	/* WINDOWS */
}

/*
 * ���֥������ȥ⡼�ɥ��ޥ��
 */
int
subject_mode_command(pt)
register char *pt;
{
  extern int post_mode();
  extern int reference_mode();
  extern int followup_mode();
  extern int cancel_mode();
  extern void shell_mode();
  extern int show_article_id();
  extern int subject_all_article();
  extern void subject_mode_help();
  extern void kill_mode();

  register newsgroup_t *nng,*png;
  register subject_t *sb;
  register int arg;
  
  /* �����ǻϤޤ�Τϡ����֥��������ֹ� */
  if ( '0' <= *pt && *pt <= '9' ) {
    return(goto_article(ng->subj,atoi(pt)));
  }
  
  if ( *pt == 0 ) {		/* CR�Τߡ��ǽ�Υ��֥������� */
    if ( start == -1 ) {
      register int i;
      for ( i = 0, sb = ng->subj ; sb != 0 ; i++,sb = sb->next) {
	if ( sb->numb != 0 ) {
	  start = i;
	  top = i;
	  break;
	}
      }
      if ( sb == 0 ) {
#ifndef	WINDOWS
	mc_puts("\n̤�ɤε����Ϥ���ޤ��� \n");
#endif /* WINDOWS */
	return(END);
      }
    }
    return(goto_article(ng->subj,start));
  }
  
  if ( pt[1] == 0 ) {		/* ��ʸ�������Υ��ޥ�� */
    switch ( *pt ) {
    case 'a':			/* ������ */
#ifndef	WINDOWS
      if ( subject_all_article() == CONNECTION_CLOSED )
	return(CONNECTION_CLOSED);
      return(CONT);
#else  /* WINDOWS */
      PostMessage(hWnd, WM_SUBJECTALL,0 , 0L);
      return(SELECT);
#endif /* WINDOWS */
			
    case 't':
      gn.autolist = 1;
      gn.show_truncate ^= ON;
      return(CONT);
			
    case 'P':			/* ���Υ��롼�� */
      png = 0;
      for ( nng = newsrc ; nng != ng ; nng = nng->next) {
	if ( (nng->flag & (UNSUBSCRIBE | NOARTICLE)) == 0 )
	  png = nng;
      }
      if ( png == 0 ) {
	mc_puts("\n���Υ˥塼�����롼�פ�¸�ߤ��ޤ��� \n");
	return(END);
      }
      mc_printf("\n���Υ˥塼�����롼�פ� %s �Ǥ�\n",png->name);
      if ( exit_group() == ERROR )
	return(ERROR);
      ng = png;
      switch (group_command(ng->name)) {
      case CONNECTION_CLOSED:
	return(CONNECTION_CLOSED);
      case CONT:
#ifdef	WINDOWS
	PostMessage(hWnd, WM_ENTERSUBJECTMODE,0 , (long)ng);
#endif /* WINDOWS */
	return(NEXT);
      }
      return(CONT);
			
    case 'N':			/* ���Υ��롼�� */
      nng = ng->next;
      while ( nng != 0 && (nng->flag & (UNSUBSCRIBE | NOARTICLE)))
	nng = nng->next;
      if ( nng == 0 ) {
	mc_puts("\n���Υ˥塼�����롼�פ�¸�ߤ��ޤ��� \n");
	return(END);
      }
      mc_printf("\n���Υ˥塼�����롼�פ� %s �Ǥ�\n",nng->name);
      if ( exit_group() == ERROR )
	return(ERROR);
      ng = nng;
      switch (group_command(ng->name)) {
      case CONNECTION_CLOSED:
	return(CONNECTION_CLOSED);
      case CONT:
#ifdef	WINDOWS
	PostMessage(hWnd, WM_ENTERSUBJECTMODE,0 , (long)ng);
#endif /* WINDOWS */
	return(NEXT);
      }
      return(CONT);
			
    case 'l':			/* �����롼�פ�ɽ�� */
      sb_s = ng->first;
      if ( exit_group() == ERROR )
	return(ERROR);
      return(enter_group(&sb_s,&sb_e));
			
    case 'i':			/* �ݥ��� */
      return(post_mode(ng->name));
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
    case 'u':			/* �⤦�ɤޤʤ� */
#ifndef	WINDOWS
      pt = kb_input("�⤦�ꥹ�ȥ��åפ��ޤ���(y:λ��/����¾:���)");
      if ( *pt == 'y' || *pt == 'Y' ){
	ng->flag |= UNSUBSCRIBE;
	change = 1;
	return(END);
      }
#else  /* WINDOWS */
      if ( mc_MessageBox(hWnd,"�⤦�ꥹ�ȥ��åפ��ޤ��� ",
			 "",MB_OKCANCEL) == IDOK ){
	ng->flag |= UNSUBSCRIBE;
	change = 1;
	return(END);
      }
#endif /* WINDOWS */
      return(CONT);
    case 'S':			/* ��äѤ��ɤ� */
      if ( ng->flag & UNSUBSCRIBE ) {
	ng->flag &= ~UNSUBSCRIBE;
	change = 1;
      }
      return(CONT);
    case 'K':
      kill_mode();		/* KILL */
      return(CONT);

    case 'q':			/* ��λ���˥塼�����롼�ץ⡼�ɤ� */
      return(END);
    case '?':			/* help */
      subject_mode_help();
      return(CONT);
    }
  }
  
  /* ������Ȥ륳�ޥ�� */
  switch ( *pt ) {
  case 'c':			/* �ɤ�����Ȥˤ��� */
    return(subject_catchup(pt));
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
#ifndef	NO_SAVE
  case 's':			/* ������ */
    save_mode(pt);
    return(CONT);
#endif /* NO_SAVE */
		
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
		
  case '/':			/* ���֥������Ȥ�ʸ���󸡺� */
    subject_search(pt);
    return(CONT);
		
  case '\\':			/* ���֥������Ȥ�ʸ���󸡺��ʵս��*/
    subject_search_rev(pt);
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
subject_catchup(pt)
register char *pt;
{
  extern int get_area();
  
  register subject_t *sb;
  register article_t *ar;
  long catch_s,catch_e;
  
  pt++;
  while (*pt == ' ' || *pt == '\t')
    pt++;
  if ( *pt == 0 ) {		/* �����ɤ�����Ȥˤ��� */
    for ( sb = ng->subj; sb != 0 ; sb=sb->next) {
      for ( ar = sb->art; ar != 0 ; ar=ar->next){
	if ( ar->flag == 0 ){	/* ̤�� */
	  ar->flag = 1;
	  change = 1;
	  sb->numb--;
	}
      }
    }
    return(END);
  }
  
  if ( get_area(pt,&catch_s,&catch_e) != CONT )
    return(CONT);
  
  if ( catch_s == -1 ) {
    sb = ng->subj;
  } else {
    for ( sb = ng->subj ; sb != 0 && catch_s != 0 ; sb=sb->next) {
      catch_s--;
      catch_e--;
    }
  }
  if ( catch_e <= -1 ){
    for ( ; sb != 0 ; sb=sb->next) {
      for ( ar = sb->art; ar != 0 ; ar=ar->next){
	if ( ar->flag == 0 ){	/* ̤�� */
	  ar->flag = 1;
	  change = 1;
	  sb->numb--;
	}
      }
    }
  } else {
    for ( ; sb != 0 && catch_e >= 0 ; sb=sb->next) {
      for ( ar = sb->art; ar != 0 ; ar=ar->next){
	if ( ar->flag == 0 ){	/* ̤�� */
	  ar->flag = 1;
	  change = 1;
	  sb->numb--;
	}
      }
      catch_e--;
    }
  }
  return(CONT);
}
/*
 * ������̤�ɵ�����ɽ������
 */
#define	CHECK_COUNT	10
#ifndef	WINDOWS
int
subject_all_article()
{
  extern int show_article_common();
  
#if ! defined(NO_NET)
  char resp[NNTP_BUFSIZE+1];
#endif
  register char *article_file;
  register subject_t *sb;
  register article_t *ar;
  register int count = 0;
  char *key_buf;
  FILE *fp;
  
  for ( sb = ng->subj ; sb != 0 ; sb = sb->next) {
    if ( sb->numb == 0 )
      continue;

    if ( disp_mode == THREADED && (sb->flag & THREADED) == 0 ){
      article_make_thread(sb);
      sb->flag |= THREADED;
    } else if (disp_mode != THREADED && (sb->flag & THREADED) != 0 ){
      article_make_order(sb);
      sb->flag &= ~THREADED;
    }

    for ( ar = sb->art; ar != 0 ; ar=ar->next) {
      if ( ar->flag )
	continue;

      if ( gn.local_spool ) {
	if ( gn.power_save == COPY_TO_TMP )
	  article_file = expand_filename(article_filename(ar->numb),gn.tmpdir);
	else
	  article_file = expand_filename(article_filename(ar->numb),ngdir);
	if ( article_file == 0 ) {
#if ! defined(WINDOWS)
	  mc_printf("�ե�����̾��Ÿ���˼��Ԥ��ޤ���(%s)\n",
		    article_filename(ar->numb));
#else  /* WINDOWS */
	  mc_MessageBox(hWnd,"�ե�����̾��Ÿ���˼��Ԥ��ޤ���",
			article_filename(ar->numb),MB_OK);
#endif /* WINDOWS */
	  continue;
	}

	if ( (fp = fopen(article_file,"r")) == NULL )
	  continue;
#if ! defined(NO_NET)
      } else {
#ifdef	WINDOWS
	set_hourglass();
#endif /* WINDOWS */
	sprintf(resp,"ARTICLE %ld",ar->numb);
	if ( put_server(resp) ) {
#ifdef	WINDOWS
	  reset_cursor();
#endif /* WINDOWS */
	  return(CONNECTION_CLOSED);
	}
	if ( get_server(resp,NNTP_BUFSIZE)) {
#ifdef	WINDOWS
	  reset_cursor();
#endif /* WINDOWS */
	  return(CONNECTION_CLOSED);
	}
	switch ( atoi(resp) ) {
	case OK_ARTICLE:
	  break;
	case ERR_NOARTIG:
	  continue;
	default:
#ifdef	WINDOWS
	  reset_cursor();
#endif /* WINDOWS */
	  return(CONNECTION_CLOSED);
	}
#endif
      }

      switch ( show_article_common(ng,ar->numb,fp,article_file) ) {
      case CONNECTION_CLOSED:
	return(CONNECTION_CLOSED);
      case FILE_WRITE_ERROR:
	mc_puts("�ƥ�ݥ��ե�����κ����˼��Ԥ��ޤ���\n");
	return(CONT);
      case ARTICLE_NOT_FOUND:
	return(CONT);
      case CONT:
      default:
	break;
      }

      /* ̤�ɤʤ���ɤ� */
      if ( ar->flag == 0 ) {
	ar->flag = 1;
	sb->numb--;
      }

      if ( ++count >= CHECK_COUNT && ar->next != 0 && sb->next != 0) {
	key_buf = kb_input("³���ޤ����������:n��");
	while( *key_buf == ' ' || *key_buf == '\t' )
	  key_buf++;
	if ( *key_buf == 'n' || *key_buf == 'N' )
	  return(CONT);
	count = 0;
      }
    }
  }
  return(CONT);
}
#else	/* WINDOWS */
subject_all_article()
{
  extern int show_article();
  
  static int count = 0;
  register subject_t *sb;
  register article_t *ar;
  
  ar = 0;
  for ( sb = ng->subj ; sb != 0 ; sb = sb->next) {
    if ( sb->numb == 0 )
      continue;

    if ( disp_mode == THREADED && (sb->flag & THREADED) == 0 ){
      article_make_thread(sb);
      sb->flag |= THREADED;
    } else if (disp_mode != THREADED && (sb->flag & THREADED) != 0 ){
      article_make_order(sb);
      sb->flag &= ~THREADED;
    }

    for (ar = sb->art ; ar != 0 ; ar=ar->next) {
      if ( ar->flag == 0)
	break;
    }
    if ( ar != 0 )
      break;
  }
  
  if ( ar == 0 ) {
    count = 0;
    PostMessage(hWnd, WM_SUBJECTMODE,0 , 0L);
    /* PostMessage(hWnd, WM_GROUPMODE,0 , 0L); */
    return(END);
  }
  
  if ( ++count >= CHECK_COUNT && ar->next != 0) {
    if ( mc_MessageBox(hWnd,"³���ޤ�����",
		       "",MB_OKCANCEL) == IDCANCEL ){
      count = 0;
      PostMessage(hWnd, WM_SUBJECTMODE,0 , 0L);
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
    mc_puts("�ƥ�ݥ��ե�����κ����˼��Ԥ��ޤ���\n");
    return(END);
  }
  
  return(CONT);
}
#endif	/* WINDOWS */


/*
 * ���֥������ȥ��塼�κ���
 */
static int
enter_group(s,e)
long *s,*e;
{
  extern int get_area();
  extern int get_1_header();
  extern void set_readed();
  extern char *jfrom();
  
  char resp[NNTP_BUFSIZE+1],resp2[NNTP_BUFSIZE+1];
  char buf[NNTP_BUFSIZE+1];
  register char *pt;
  long l;
  register int x;
  register subject_t *sb;
  register article_t *ar;
  register char *reference;	/* References: �إå��κǸ�� M-ID */
  long st,ed;
  register int ng_control = 0,junk_show_groups = 0,control_show_control = 0;
#if defined USE_JNAMES
  char tmp[NNTP_BUFSIZE+1];
#endif
  FILE *fp;
  FILE *fpt;
  int join_flag;
  register kill_list_t *kq;
  
  if ( ng->read < ng->first - 1 )
    ng->read = ng->first - 1;
  
  if ( *s < ng->first )
    *s = ng->first;
  
  if ( (*e - *s + 1) > gn.select_limit && gn.select_limit > 0) {
    while ( 1 ){
#ifndef	WINDOWS
      if ( gn.select_number > 0 )
	sprintf(resp,"�ɤ��ϰϤ���ꤷ�Ʋ������ʥǥե���ȡ��Ť������� %d ),q:�ɤޤʤ���",gn.select_number);
      else
	sprintf(resp,"�ɤ��ϰϤ���ꤷ�Ʋ������ʥǥե���ȡ�����(%ld-%ld),q:�ɤޤʤ���",*s,*e);

      pt = kb_input(resp);
      if ( *pt == 'q' )
	return(QUIT);
#else  /* WINDOWS */
      if ( gn.select_number > 0 )
	sprintf(resp,"�ɤ��ϰϤ���ꤷ�Ʋ������ʥǥե���ȡ��Ť������� %d��",gn.select_number);
      else
	sprintf(resp,"�ɤ��ϰϤ���ꤷ�Ʋ������ʥǥե���ȡ�����(%ld-%ld)��",*s,*e);

      if ( (pt = kb_input(resp)) == NULL )
	return(QUIT);
#endif /* WINDOWS */
      if ( *pt == 0 ) {
	if ( gn.select_number > 0 ) {
	  ed = *s + gn.select_number - 1;
	  if ( ng->last > ed )
	    *e = ed;
	}
	break;
      }

      if ( *pt == '#' ) {
	pt++;
	ed = *s + atol(pt) - 1;
	if ( ng->last > ed )
	  *e = ed;
	break;
      }
			  
      if ( get_area(pt, &st, &ed) != CONT )
	continue;
      if ( st == -1 )
	st = *s;
      if ( ed == -1 )
	ed = *e;
      if ( st < ng->first || ng ->last < st )
	continue;
      if ( ed < ng->first || ng->last < ed || ed < st )
	continue;
      *s = st;
      *e = ed;
      break;
    }
  }
  if ( strncmp(ng->name,"control",7) == 0 ) {
    ng_control = 1;
    if ( (gn.kill_control & CONTROL_SHOW_CONTROL) != 0)
      control_show_control = 1;
  }
  if ( strcmp(ng->name,"junk") == 0 &&
      (gn.kill_control & JUNK_SHOW_GROUPS) != 0) {
    junk_show_groups = 1;
  }
  
  tmp_first = *s;		/* tmpdir �ˤ��뵭�� */
  tmp_last = *e;
  
#ifdef	WINDOWS
  set_hourglass();
#endif	/* WINDOWS */
  
  x = 0;
  for ( l = *s ; l <= *e ; l++) {
    if ( ++x > 5 ){
      mc_printf(".");
      fflush(stdout);
      x = 0;
#ifdef	QUICKWIN
      _wyield();
#endif
    }
    join_flag = OFF;

    /* �����Υإå����׵� */
    if ( gn.local_spool ) {
      if ( (pt = expand_filename(article_filename(l),ngdir)) == 0 ) {
#if ! defined(WINDOWS)
	mc_printf("�ե�����̾��Ÿ���˼��Ԥ��ޤ���(%s)\n",
		  article_filename(l));
#else  /* WINDOWS */
	mc_MessageBox(hWnd,"�ե�����̾��Ÿ���˼��Ԥ��ޤ���",
		      article_filename(l),MB_OK);
#endif /* WINDOWS */
	continue;
      }

      if ( (fp = fopen(pt,"r")) == NULL ) {
	set_readed(ng,l);
	continue;
      }
#if ! defined(NO_NET)
    } else {
      sprintf(resp,"HEAD %ld",l);
      if ( put_server(resp) ) {
#ifdef	WINDOWS
	reset_cursor();
#endif /* WINDOWS */
	return(CONNECTION_CLOSED);
      }
      if ( get_server(resp,NNTP_BUFSIZE)) {
#ifdef	WINDOWS
	reset_cursor();
#endif /* WINDOWS */
	return(CONNECTION_CLOSED);
      }
			
      if (atoi(resp) != OK_HEAD ) { /* ����󥻥뤵�줿 */
	set_readed(ng,l);
	continue;
      }
#endif
    }

    sb = 0;
    if ( (ar = alloc_article_queue()) == NULL )
      return(ERROR);
    ar->next = 0;
    ar->from = 0;
    ar->reference = 0;
    ar->message_id = 0;
    ar->flag = 0;
    ar->lines = -1;
    ar->numb = l;		/* �����ֹ� */
    if ( ar->numb <= ng->read )	/* ���� */
      ar->flag = 1;
		
    resp[0] = 0;
    if ( gn.local_spool ) {
      get_1_header_file(buf,resp,fp);
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
		
    while (1) {
      if ( gn.local_spool ) {
	if (resp[0] == 0 ) {
	  fclose(fp);
	  break;
	}
	get_1_header_file(buf,resp,fp);
	if (buf[0] == 0 ) {
	  fclose(fp);
	  break;
	}
#if ! defined(NO_NET)
      } else  {
	if (resp[0] == '.' && resp[1] == 0)
	  break;
	if ( get_1_header(buf,resp) == CONNECTION_CLOSED ) {
#ifdef	WINDOWS
	  reset_cursor();
#endif /* WINDOWS */
	  return(CONNECTION_CLOSED);
	}
	if (buf[0] == '.' && buf[1] == 0)
	  break;
#endif
      }

      switch (buf[0]) {
      case 'F':
	/* From: �إå� */
	if (strncmp("From: ",buf,6) == 0 ){
#ifdef	MIME
	  if ( (gn.mime_head & MIME_DECODE) != 0 ) {
	    MIME_strHeaderDecode(buf,resp2,sizeof(resp2));
	    strcpy(buf,resp2);
	  }
#endif
#if defined USE_JNAMES
	  kanji_spooltoint(&buf[6],tmp);
	  strcpy(resp2, jfrom(tmp));
#else
	  kanji_spooltoint(&buf[6],resp2);
#endif
	  if ( (ar->from = (char *)malloc(strlen(resp2)+1)) == NULL ) {
	    memory_error();
	    return(ERROR);
	  }
	  strcpy(ar->from,resp2);
	}
	break;
      case 'L':
	/* Lines: �إå��ʤʤ����Ȥ⤢���*/
	if (strncmp("Lines: ",buf,7) == 0 ){
	  ar->lines = atoi(&buf[7]);
	}
	break;
      case 'M':
	/* Message-ID: �إå� */
	if ( strncmp(buf,"Message-ID: ",12) == 0 ) {
	  if ( (ar->message_id = (char *)malloc(strlen(buf)+1-12)) == NULL ) {
	    memory_error();
	    return(ERROR);
	  }
	  strcpy(ar->message_id,&buf[12]);
	  if ( (pt = strrchr(ar->message_id,'>')) != NULL )
	    *(++pt) = 0;
	}
	break;
      case 'C':
	/* Control: �إå� */
	if (gn.kill_control != 0 && ng_control != 0 &&
	    strncmp(buf,"Control: ",9) == 0 ) {
	  pt = &buf[9];
	  if ( ar->flag == 0 ) {
	    while ( *pt == ' ')
	      pt++;
	    /*                                                            12345678901 */
	    if (((gn.kill_control & KILL_CANCEL     ) != 0 && strncmp(pt,"cancel",6) == 0) ||
		((gn.kill_control & KILL_IHAVE      ) != 0 && strncmp(pt,"ihave",5) == 0) ||
		((gn.kill_control & KILL_SENDME     ) != 0 && strncmp(pt,"sendme",6) == 0) ||
		((gn.kill_control & KILL_NEWGROUP   ) != 0 && strncmp(pt,"newgroup",8) == 0) ||
		((gn.kill_control & KILL_RMGROUP    ) != 0 && strncmp(pt,"rmgroup",7) == 0) ||
		((gn.kill_control & KILL_CHECKGROUPS) != 0 && strncmp(pt,"checkgroups",11) == 0) ||
		((gn.kill_control & KILL_SENDSYS    ) != 0 && strncmp(pt,"sendsys",7) == 0) ||
		((gn.kill_control & KILL_VERSION    ) != 0 && strncmp(pt,"version",7) == 0) ) {
	      ar->flag = 1;
	      if ( sb != 0 )
		sb->numb--;
	    }
	  }
	  if ( control_show_control != 0 ) {
	    if ( join_flag != ON ) {
	      if ( (sb = join_subject_queue(ar,pt,NOTREONLY)) == NULL ) {
		memory_error();
		return(ERROR);
	      }
	      join_flag = ON;
	    }
	  }
	}
	break;
      case 'S':
	/* Subject: �إå� */
	if (junk_show_groups == 0 && control_show_control == 0 &&
	    strncmp("Subject: ",buf,9) == 0 ){

	  kanji_convert(gn.spool_kanji_code,buf,JIS,resp2); /* spool->JIS */
#ifdef	MIME
	  if ( (gn.mime_head & MIME_DECODE) != 0 ) {
	    MIME_strHeaderDecode(resp2,buf,sizeof(buf));
	    strcpy(resp2,buf);
	  } 
#endif
	  add_esc(resp2,buf);	/* ESC ���䴰 */
	  kanji_convert(JIS,buf,internal_kanji_code,resp2); /* JIS->intnl. */

	  if ( join_flag != ON ) {
	    pt = &resp2[9];
	    while ( *pt == ' ' ) /* Subject: ��³�� ' ' */
	      pt++;
	    if ( strncmp(pt,"Re:",3) == 0 ) {
	      pt += 3;
	      while ( *pt == ' ' ) /* Re: ��³�� ' ' */
		pt++;
	      if ( strncmp(pt,"Re:",3) == 0 ) {	/* Re: Re: X-< */
		pt += 3;
		while ( *pt == ' ' ) /* Re: ��³�� ' ' */
		  pt++;
	      }
	      if ( gn.disp_re == ON ) {
		if ( (sb = join_subject_queue(ar,pt,0)) == NULL ) {
		  memory_error();
		  return(ERROR);
		}
	      } else {
		if ( (sb = join_subject_queue(ar,pt,NOTREONLY)) == NULL ) {
		  memory_error();
		  return(ERROR);
		}
	      }
	    } else {
	      if ( (sb = join_subject_queue(ar,pt,NOTREONLY)) == NULL ) {
		memory_error();
		return(ERROR);
	      }
	    }
	    join_flag = ON;
	  }
	}
	break;
      case 'N':
	/* Newsgroups: �إå� (junk �λ�������*/
	if (junk_show_groups != 0 && control_show_control == 0 &&
	    strncmp("Newsgroups: ",buf,12) == 0 ){
	  kanji_spooltoint(buf,resp2);
	  if ( join_flag != ON ) {
	    if ( (sb = join_subject_queue(ar,&resp2[12],NOTREONLY)) == NULL ) {
	      memory_error();
	      return(ERROR);
	    }
	    join_flag = ON;
	  }
	}
	break;
      case 'R':
	/* References: �إå��ʤʤ����Ȥ⤢���*/
	if ( strncmp(buf,"References: ",12) == 0 ) {
	  if ( (reference = (char *)malloc(strlen(buf)+1-12)) == NULL ) {
	    memory_error();
	    return(ERROR);
	  }
	  strcpy(reference,&buf[12]);

	  if ( (pt = strrchr(reference,'<')) != NULL ) {
	    if ( (ar->reference = (char *)malloc(strlen(pt)+1)) == NULL ) {
	      memory_error();
	      return(ERROR);
	    }
	    strcpy(ar->reference,pt);
	  }
	  if ( (pt = strrchr(reference,'>')) != NULL )
	    *(++pt) = 0;

	  free(reference);
	}
	break;
      }
    }
    /* �ɤΥ��֥������ȥ��塼�ˤ�°���ʤ������ϡ����ɤȤ��� X-< */
    if ( join_flag == OFF ) {
      if ( ng_control != 0 && (gn.kill_control & KILL_NOCONTROL) != 0 ) {
	ar->flag = 1;
	if ( sb != 0 )
	  sb->numb--;
      }
      if ( control_show_control != 0 ) {
	if ( (sb = join_subject_queue(ar,"No Control Header",NOTREONLY)) == NULL ) {
	  memory_error();
	  return(ERROR);
	}
      } else {
	sprintf(buf,"%ld",l);
	if ( ng->newsrc == 0 ) {
	  if ( (ng->newsrc = malloc(strlen(buf)+1)) == NULL ) {
	    memory_error();
	    return(ERROR);
	  }
	  strcpy(ng->newsrc,buf);
	} else {
	  if ( (pt = malloc(strlen(ng->newsrc)+1+strlen(buf)+1)) == NULL ) {
	    memory_error();
	    return(ERROR);
	  }
	  strcpy(pt,ng->newsrc);
	  strcat(pt,",");
	  strcat(pt,buf);
	  free(ng->newsrc);
	  ng->newsrc = pt;
	}
	free_article_queue(ar);
      }
    }
    if ( gn.local_spool == ON ) {
      if ( gn.power_save != NO ) {
	if ( (pt = expand_filename(article_filename(l),ngdir)) == 0 ) {
#if ! defined(WINDOWS)
	  mc_printf("�ե�����̾��Ÿ���˼��Ԥ��ޤ���(%s)\n",
		    article_filename(l));
#else  /* WINDOWS */
	  mc_MessageBox(hWnd,"�ե�����̾��Ÿ���˼��Ԥ��ޤ���",
			article_filename(l),MB_OK);
#endif /* WINDOWS */
	  continue;
	}

	if ( (fp = fopen(pt,"r")) == NULL )
	  continue;

	if ( gn.power_save == COPY_TO_TMP ) {
	  if ( (pt = expand_filename(article_filename(l),gn.tmpdir)) == 0 ) {
#if ! defined(WINDOWS)
	    mc_printf("�ե�����̾��Ÿ���˼��Ԥ��ޤ���(%s)\n",
		      article_filename(l));
#else  /* WINDOWS */
	    mc_MessageBox(hWnd,"�ե�����̾��Ÿ���˼��Ԥ��ޤ���",
			  article_filename(l),MB_OK);
#endif /* WINDOWS */
	    continue;
	  }

	  if ( (fpt = fopen(pt,"w")) == NULL )
	    continue;
	  while( fgets(resp,NNTP_BUFSIZE,fp) != NULL )
	    fputs(resp,fpt);
	  fclose(fpt);
	} else {			/* COPY_TO_CACHE */
	  while( fgets(resp,NNTP_BUFSIZE,fp) != NULL )
	    ;
	}
	fclose(fp);
      }
    }
    /* kill */
    for ( kq = kill_list ; kq != 0 ; kq = kq->next) {
      if (strcmp(kq->from,ar->from) == 0 &&
	  strcmp(kq->subject,sb->name) == 0 ) {
	if ( ar->flag == 0 ) {
	  ar->flag = 1;
	  sb->numb--;
	}
      }
    }
  }
  newsrc_check();	/* GNUS format �� .newsrc �Υ����å� */
  
#ifdef	WINDOWS
  reset_cursor();
#endif	/* WINDOWS */
  
  return(CONT);
}

#if ! defined(NO_NET)
/*
 * �إå�����
 */
int
get_1_header(buf,resp)
char *resp;	/* ����ΰ� */
char *buf;		/* ��� */
{
  register int buf_len,resp_len;
  
  strcpy(buf,resp);
  buf_len = strlen(buf);
  
  while (1) {
    if ( get_server(resp,NNTP_BUFSIZE))
      return(CONNECTION_CLOSED);
    if ( resp[0] == 0 || resp[0] == '.' ||
	('A' <= resp[0] && resp[0] <= 'Z') )
      break;
    resp_len = strlen(resp);
    if ( buf_len + 1 + resp_len > NNTP_BUFSIZE ) {
      /* NNTP_BUFSIZE �ʾ�ϼΤƤ� X-< */
      while (1) {
	if ( get_server(resp,NNTP_BUFSIZE))
	  return(CONNECTION_CLOSED);
	if ( resp[0] != ' ' || resp[0] != TAB )
	  break;
      }
      return(CONT);
    }
    buf_len = buf_len + 1 + resp_len;
    strcat(buf,"\n");
    strcat(buf,resp);
  }
  return(CONT);
}
#endif	/* ! NO_NET */

static subject_t *
join_subject_queue(ar,pt,notreonly)
register article_t *ar;
register char *pt;
int notreonly;
{
  register subject_t **sb;
  register article_t *al;
  register char *pts,*ptd;

  for ( sb = &(ng->subj) ; *sb != 0 ;sb = &((*sb)->next)){

    if ( ((*sb)->name)[0] != pt[0] )
      continue;

    if ( strcmp((*sb)->name,pt) == 0 ){	/* ���Ǥˤ��륵�֥������� */
      if ( ar->flag == 0 )
	(*sb)->numb ++;
      (*sb)->flag |= notreonly;
      for (al = (*sb)->art;al->next != 0 ;al = al->next)
	;
      al->next = ar;
      return(*sb);
    }

    /* �ۥ磻�ȥ��ڡ��������Ԥ����ΰ㤤�ʤ饪����˸��� */
    pts = (*sb)->name;
    ptd = pt;
    while (1) {
      while (*pts == ' ' || *pts == '\t' || *pts == '\n' || *pts == '\r')
	pts++;
      while (*ptd == ' ' || *ptd == '\t' || *ptd == '\n' || *ptd == '\r')
	ptd++;

      if ( *pts == 0 && *ptd == 0 ) { /* �Ǹ�ޤǰ��� */
	if ( ar->flag == 0 )
	  (*sb)->numb ++;
	(*sb)->flag |= notreonly;
	for (al = (*sb)->art;al->next != 0 ;al = al->next)
	  ;
	al->next = ar;
	return(*sb);
      }

      if ( *pts != *ptd )	/* �㤦 */
	break;

      pts++;			/* �� */
      ptd++;
    }
  }
  
  /* ���ƤΥ��֥������� */
  if ( ((*sb) = alloc_subject_queue()) == NULL ) {
    return(NULL);
  }
  (*sb)->next = 0;
  (*sb)->flag = notreonly;
  if ( ar->flag == 0 )
    (*sb)->numb = 1;
  else
    (*sb)->numb = 0;
  if ( ((*sb)->name = (char *)malloc(strlen(pt)+1)) == NULL ) {
    memory_error();
    return(NULL);
  }

  /* sb->name �ˤϲ��Ԥ�����ʤ���tab �⡡space ���Ѵ����� */
  pts = pt;
  ptd = (*sb)->name;
  while (*pts != 0) {
    if (*pts == '\n' || *pts == '\r') {
      pts++;
      continue;
    }
    if (*pts == '\t' ) {
      *ptd++ = ' ';
      pts++;
      continue;
    }
    *ptd++ = *pts++;
  }
  *ptd = 0;

  (*sb)->art = ar;
  return(*sb);
}

/*
 * �ϰϤβ���
 */
int
get_area(pt,st,ed)
register char *pt;
long *st;	/* �����ֹ桧��ά������ -1 */
long *ed;	/* ��λ�ֹ桧��ά������ -1 */
{
  *st = -1;
  *ed = -1;
  
  while ( *pt == ' ' || *pt == '\t' )
    pt++;
  
  if ( *pt == '-' ) {		/* -999 */
    pt++;
    while ( *pt == ' ' || *pt == '\t' )
      pt++;
    if ( *pt < '0' || '9' < *pt )
      return(ERROR);
    *ed = atol(pt);
    return(CONT);
  }
  
  if ( *pt < '0' || '9' < *pt )
    return(ERROR);
  
  *st = atol(pt);
  while( '0' <= *pt && *pt <= '9' )
    pt++;
  while ( *pt == ' ' || *pt == '\t' )
    pt++;
  if ( *pt == 0 ) {		/* 9999 */
    *ed = *st;
    return(CONT);
  }
  
  if ( *pt != '-' )
    return(ERROR);
  pt++;
  while ( *pt == ' ' || *pt == '\t' )
    pt++;
  if ( *pt == 0 )	/* 9999- */
    return(CONT);
  
  if ( *pt < '0' || '9' < *pt )
    return(ERROR);
  
  *ed = atol(pt);
  
  if ( *ed < *st )
    return(ERROR);
  
  return(CONT);
}

/*
 * ���֥������ȥꥹ�Ȥ��˴�
 */
static void
erase_subject_queue()
{
  register subject_t *sb;
  register article_t *al;
  
  /* ���֥������ȥ��塼 */
  if ( ng->subj == 0)
    return;
  for ( sb = ng->subj ; sb != 0 ; sb = sb->next ){
    for (al = sb->art ; al != 0 ; al = al->next ) {
      if (al->from != 0) {
	free(al->from);		/* ��Ƽ� */
	al->from = 0;
      }
      if (al->message_id != 0) {
	free(al->message_id);
	al->message_id = 0;
      }
      if (al->reference != 0) {
	free(al->reference);
	al->reference = 0;
      }
    }
    free_article_queue(sb->art);	/* �������塼 */

    free(sb->name);			/* ���֥������� */
    sb->name = 0;
  }
  free_subject_queue(ng->subj);		/* ���֥������ȥ��塼 */
  ng->subj = 0;
}

/*
 * GNUS format �� .newsrc �Υ����å�
 */
static void
newsrc_check()
{
  register char *pt;
  register long st,ed;
  register subject_t *sb;
  register article_t *ar;
  
  if ( (pt = ng->newsrc) == 0 )
    return;
  
  while (*pt != 0 && *pt != '\n' ){
    if ( *pt < '0' || '9' < *pt  )
      return;

    st = atol(pt);		/* �ǽ���ֹ� */
    while ( '0' <= *pt && *pt <= '9' )
      pt++;
    switch ( *pt ) {
    case ',':			/* �ֹ��Ĥ��� */
    case '\n':			/* �ֹ��Ĥ��� */
    case 0:			/* �ֹ��Ĥ��� */
      for ( sb = ng->subj ; sb != 0 ;sb = sb->next) {
	for (ar = sb->art ; ar != 0 ;ar = ar->next) {
	  if ( ar->numb != st )
	    continue;
	  if ( ar->flag == 0 ) {
	    ar->flag = 1;
	    sb->numb--;
	  }
	  break;
	}
	if ( ar != 0 )
	  break;
      }
      if ( *pt == 0 || *pt == '\n' )
	return;
      pt++;
      break;
    case '-':			/* 111-222 */
      pt++;
      if ( *pt < '0' || '9' < *pt )
	return;
      ed = atol(pt);		/* ������ֹ� */
      if ( ed < st )
	return;
      while ( '0' <= *pt && *pt <= '9' )
	pt++;
      for ( sb = ng->subj ; sb != 0 ;sb = sb->next) {
	for (ar = sb->art ; ar != 0 ;ar = ar->next) {
	  if ( ar->numb < st || ed < ar->numb )
	    continue;
	  if ( ar->flag == 0 ) {
	    ar->flag = 1;
	    sb->numb--;
	  }
	}
      }
      if ( *pt++ != ',' )
	return;
      break;
    default:
      return;
    }
  }
}

/*
 * �����ƥ�����⡼�ɤ�
 */
static int
goto_article(sb,n)
register subject_t *sb;
register int n;
{
  extern int article_mode();
  
  for (  ; n > 0 && sb != 0 ; n--,sb = sb->next)
    ;
  
  if ( sb == 0 || n != 0 ){
#ifdef	WINDOWS
    mc_MessageBox(hWnd,"���ꤵ�줿�ֹ�Υ��֥������Ȥ�¸�ߤ��ޤ��� ",
		  "",MB_OK);
#else  /* WINDOWS */
    mc_puts("\n���ꤵ�줿�ֹ�Υ��֥������Ȥ�¸�ߤ��ޤ��� \n");
    hit_return();
#endif /* WINDOWS */
    return(0);
  }
#ifdef	WINDOWS
  PostMessage(hWnd, WM_ENTERARTICLEMODE,0 , (long)sb);
  return(SELECT);
#else	/* WINDOWS */
  return(article_mode(ng,sb));
#endif	/* WINDOWS */
}

/*
 * �˥塼�����롼�פ�ȴ����
 */
static int
exit_group()
{
  register char *readed;
  long l;
  register subject_t *sb;
  register article_t *ar;
  char newsrc_line[NEWSRC_LEN];
  register char *pt;
  long st,ed,base;
  char *newsrc_save = 0;
  
  if ( gn.local_spool == ON && gn.power_save == COPY_TO_TMP ) {
    for ( l = tmp_first; l <= tmp_last; l++) {
      if ( (pt = expand_filename(article_filename(l),gn.tmpdir)) == 0 ) {
#if ! defined(WINDOWS)
	mc_printf("�ե�����̾��Ÿ���˼��Ԥ��ޤ���(%s)\n",
		  article_filename(l));
#else  /* WINDOWS */
	mc_MessageBox(hWnd,"�ե�����̾��Ÿ���˼��Ԥ��ޤ���",
		      article_filename(l),MB_OK);
#endif /* WINDOWS */
	continue;
      }
      unlink(pt);
    }
  }

  if ( ng->last < ng->first )	/* �����������ʤ���� */
    return(CONT);
    
  base = ng->first;
  
#if defined(MSDOS) && (defined(MSC) || defined(__TURBOC__))
  if ( (readed = (char *)malloc(sizeof(char)*(short)(ng->last-ng->first+1))) == NULL ) {
    memory_error();
    return(ERROR);
  }
  memset(readed,0,(short)(ng->last-ng->first+1));
#else
  if ( (readed = (char *)malloc(sizeof(char)*(ng->last-ng->first+1))) == NULL ) {
    memory_error();
    return(ERROR);
  }
  memset(readed,0,(ng->last-ng->first+1));
#endif
  
  for ( l = ng->first; l <= ng->read ; l++)
    readed[l-base] = 1;
  
  /* newsrc �Ԥ�����ɥꥹ�Ⱥ��� */
  if ( ng->newsrc ) {
    newsrc_save = ng->newsrc;
    ng->newsrc = 0;
    pt = newsrc_save;
    while (*pt != 0 ) {
      st = atol(pt);
      while( '0' <= *pt && *pt <= '9' )
	pt++;
      if ( *pt == 0 ){
	if ( ng->first <= st && st <= ng->last)
	  readed[st-base] = 1;
	break;
      }
      if ( *pt == ',' ){
	pt++;
	if ( ng->first <= st && st <= ng->last)
	  readed[st-base] = 1;
	continue;
      }
      if ( *pt == '-' ){
	pt++;
	ed = atol(pt);
	while( '0' <= *pt && *pt <= '9' )
	  pt++;
	if ( *pt == ',' )
	  pt++;
	for ( ; st <= ed ; st++) {
	  if ( ng->first <= st && st <= ng->last)
	    readed[st-base] = 1;
	}
      }
    }
  }
  
  /* �Ƶ����ξ��󤫤���ɥꥹ�Ȥ���� */
  for ( sb = ng->subj ; sb != 0 ;sb = sb->next) {
    for (ar = sb->art ; ar != 0 ;ar = ar->next)
      readed[ar->numb - base] = ar->flag;
  }
  
  /* �ǽ�Ϣ³���ɵ����ֹ� */
  st = ng->first - 1;
  for ( l = ng->first; l <= ng->last && readed[l-base] == 1; l++)
    st = l;
  
  if ( ng->read != st ) {
    ng->read = st;
    change = 1;
  }
  
  /* GNUS format �� newsrc �Ԥκ��� */
  pt = newsrc_line;
  *pt = 0;
  ng->numb = 0;
  for ( ; l <= ng->last ; l++ ) {
    if ( readed[l-base] != 1 ) {
      ng->numb++;
      continue;
    }
    st = ed = l;
    for ( ; l <= ng->last; l++ ) {
      if ( readed[l-base] != 1 ) {
	ng->numb++;
	break;
      }
      ed = l;
    }
    if ( st == ed ) {
      sprintf(pt,"%ld,",st);
    } else {
      sprintf(pt,"%ld-%ld,",st,ed);
    }
    pt = &newsrc_line[strlen(newsrc_line)];
  }
  if ( strlen(newsrc_line) != 0 ){
    pt--;
    *pt = 0;
    if (newsrc_save == 0 ||
	(newsrc_save != 0 && strcmp(newsrc_save,newsrc_line) != 0) ) {
      change = 1;
    }
    if ( (ng->newsrc = (char *)malloc(strlen(newsrc_line)+1)) == NULL ) {
      memory_error();
      return(ERROR);
    }
    strcpy(ng->newsrc,newsrc_line);
  } else {
    if (newsrc_save != 0)
      change = 1;
  }
  
  if ( ng->numb <= 0 )	/* ̤�ɵ����ʤ� */
    ng->flag |= NOARTICLE;
  
  erase_subject_queue();
  if (newsrc_save)
    free(newsrc_save);
  free(readed);
  return(CONT);
}

void
kill_mode()
{
  register kill_list_t **kq;

  /* ���뤹�뵭�����ɤޤ�Ƥ��뤳�Ȥγ�ǧ */
  if ( last_art_check() == ERROR ){
#ifdef	WINDOWS
    mc_MessageBox(hWnd,"���뤹�뵭�����ɤޤ�Ƥ��ޤ��� ","",MB_OK);
#else  /* WINDOWS */
    mc_puts("\n���뤹�뵭�����ɤޤ�Ƥ��ޤ��� \n\007");
    hit_return();
#endif /* WINDOWS */
    return;
  }

  /* kill queue �κǸ��õ�� */
  for ( kq = &(kill_list) ; *kq != 0 ;kq = &((*kq)->next))
    ;

  /* kill queue �γ��� */
  if ( (*kq = alloc_kill_queue()) == NULL )
    return;

  (*kq)->next = 0;


  /* ��Ƽ� */
  if ( ((*kq)->from = (char *)malloc(strlen(last_art.from)+1)) == NULL ) {
    memory_error();
    return;
  }
  strcpy((*kq)->from,last_art.from);

  /* ���֥������� */
  if ( ((*kq)->subject = (char *)malloc(strlen(last_art.subject)+1)) == NULL ) {
    memory_error();
    return;
  }
  strcpy((*kq)->subject,last_art.subject);
}
/*----------------------------------------------------------------------*/
#define	KILL_ALLOC_SIZE 10
kill_list_t *kill_free_queue = (kill_list_t *)0;

/* ���륭�塼�γ��� */
static kill_list_t *
alloc_kill_queue()
{
  register kill_list_t *p;
  register int i;
  
  /* �⤷�ե꡼���塼�����ݤ���Ƥ��ʤ���� malloc */
  if ( kill_free_queue == (kill_list_t *)0) {
    if ( (kill_free_queue =
	  (kill_list_t *)malloc(sizeof(kill_list_t)*KILL_ALLOC_SIZE)) == NULL ) {
      memory_error();
      return(NULL);
    }
    for ( i = 0 ; i < KILL_ALLOC_SIZE-1 ; i++ )
      kill_free_queue[i].next = &kill_free_queue[i+1];
    kill_free_queue[KILL_ALLOC_SIZE-1].next = 0;
  }
  
  p = kill_free_queue;
  kill_free_queue = p->next;
  return(p);
}
/*----------------------------------------------------------------------*/
#define	SUBJECT_ALLOC_SIZE 128
static subject_t *subject_free_queue = (subject_t *)0;

/* ���֥������ȥ��塼�γ��� */
static subject_t *
alloc_subject_queue()
{
  register subject_t *p;
  register int i;
  
  /* �⤷�ե꡼���塼�����ݤ���Ƥ��ʤ���� malloc */
  if ( subject_free_queue == (subject_t *)0) {
    if ( (subject_free_queue =
	  (subject_t *)malloc(sizeof(subject_t)*SUBJECT_ALLOC_SIZE)) == NULL ) {
      memory_error();
      return(NULL);
    }
    for ( i = 0 ; i < SUBJECT_ALLOC_SIZE-1 ; i++ )
      subject_free_queue[i].next = &subject_free_queue[i+1];
    subject_free_queue[SUBJECT_ALLOC_SIZE-1].next = 0;
  }
  
  p = subject_free_queue;
  subject_free_queue = p->next;
  return(p);
}

/* ���֥������ȥ��塼���ֵ� */
static void
free_subject_queue(sb)
register subject_t *sb;
{
  register subject_t *p;
  
  p = sb;
  
  while ( p->next )
    p = p->next;
  p->next = subject_free_queue;
  
  subject_free_queue = sb;	/* �ե꡼���塼������ */
}

/*----------------------------------------------------------------------*/
#define	ARTICLE_ALLOC_SIZE 128
static article_t *article_free_queue = (article_t *)0;

/* �������塼�γ��� */
static article_t *
alloc_article_queue()
{
  register article_t *p;
  register int i;
  
  /* �⤷�ե꡼���塼�����ݤ���Ƥ��ʤ���� malloc */
  if ( article_free_queue == (article_t *)0) {
    if ( (article_free_queue =
	  (article_t *)malloc(sizeof(article_t)*ARTICLE_ALLOC_SIZE)) == NULL ) {
      memory_error();
      return(NULL);
    }
    for ( i = 0 ; i < ARTICLE_ALLOC_SIZE-1 ; i++ )
      article_free_queue[i].next = &article_free_queue[i+1];
    article_free_queue[ARTICLE_ALLOC_SIZE-1].next = 0;
  }
  
  p = article_free_queue;
  article_free_queue = p->next;
  return(p);
}

/* �������塼���ֵ� */
static void
free_article_queue(art)
register article_t *art;
{
  register article_t *p;
  
  p = art;
  
  while ( p->next )
    p = p->next;
  p->next = article_free_queue;
  
  article_free_queue = art;	/* �ե꡼���塼������ */
}
/*
 * ���֥������Ȥ�ʸ���󸡺�
 */
static void
subject_search(pt)
char *pt;
{
  register int i;
  register subject_t *sb;
  
  if (kb_search(sea_subname, pt) == QUIT )
    return;
  
  for( i = 0 ,sb = ng->subj; i < top && sb != 0 ; i++,sb=sb->next )
    ;
  for ( i = top  ; sb != 0 ; i++,sb = sb->next) {
    if( i != top && sregcmp(sb->name,sea_subname) ) {
      top = i;
      return;
    }
  }
  mc_printf("ʸ���� \"%s\" �ϡ�����ޤ��� \007\n",sea_subname);
}
/*
 * ���֥������Ȥ�ʸ���󸡺��ʵս��
 */
static void
subject_search_rev(pt)
char *pt;
{
  register int i,tmptop;
  register subject_t *sb;
  
  if (kb_search(sea_subname, pt) == QUIT )
    return;
  
  tmptop = -1;
  for( i = 0 ,sb = ng->subj; i < top && sb != 0 ; i++,sb=sb->next ) {
    if( sregcmp(sb->name,sea_subname) )
      tmptop = i;
  }
  
  if ( tmptop == -1 ) {
    mc_printf("ʸ���� \"%s\" �ϡ�����ޤ��� \007\n",sea_subname);
  } else {
    top = tmptop;
  }
}
#ifdef USE_JNAMES
char *
jfrom(from)
char *from;
{
  char *jname;
  char *name;
  static char tmp[256];
  
  strcpy(tmp, from);
  name = strtok(from, " ");
  
  if(NULL != (jname = jnFetch("familyname", name)))
    sprintf(tmp, "%s ��%s��", jname, name);
  return tmp;
}
#endif /* USE_JNAMES */

void
subject_mode_help()
{
  static char *os = (char *)OS;
  static struct help_t help_tbl[] = {
    { "���֥������ȥ⡼��", 0 },
    { "",0 },
    { "<CR>�Τ�      :�ǽ��̤�ɤΤ��륵�֥������Ȥ����� ", 0 },
    { "����<CR>      :���ꤷ���ֹ�Υ��֥������Ȥ����� ", 0 },
    { "a<CR>         :̤�ɤ������������� ", 0 },
    { "p [��]<CR>    :�����̡ʤ⤷���ϻ��ꤷ����������ˤ�ɽ������ ", 0 },
    { "n [��]<CR>    :�����̡ʤ⤷���ϻ��ꤷ�����������ˤ�ɽ������ ", 0 },
    { "t<CR>         :Ĺ���Ԥ��ޤ�ʤ���ɽ�����롿���ʤ��ʥȥ����", 0 },
    { "P<CR>         :���Υ˥塼�����롼�� ", 0 },
    { "N<CR>         :���Υ˥塼�����롼�� ", 0 },
    { "i<CR>         :���ߤΥ��롼�פص�����ݥ��Ȥ��� ", 0 },
    { "�ɣ�<CR>      :���ꤷ����å������ɣĤε�����ɽ������ ", 0 },
    { "R<CR>         :�Ǹ���ɤ�������θ�������ɽ������ ", 0 },
    { "f<CR>         :�Ǹ���ɤ�������˥ե���������Ф� ", 0 },
    { "r<CR>         :�Ǹ���ɤ�������˥�ץ饤�ᥤ���Ф� ", 0 },
    { "C<CR>         :�Ǹ���ɤ�������򥭥�󥻥뤹�� ", 0 },
#ifndef	NO_SAVE
    { "s [file]<CR>  :�Ǹ���ɤ��������ե��������¸���� ", 0 },
#endif /* NO_SAVE */
#if ! defined(QUICKWIN) && ! defined(WINDOWS)
    { "| [cmd]<CR>   :�Ǹ���ɤ�������򥳥ޥ�ɤ��Ϥ� ", 0 },
#endif /* ! QUICKWIN */
    { "u<CR>         :���Υ��롼�פϤ⤦�ɤޤʤ� ", 0 },
    { "S<CR>         :���Υ��롼�פϤ�äѤ��ɤ� ", 0 },
    { "c [�ϰ�]<CR>  :̤�ɤε����������ɤ�����Ȥˤ��� ", 0 },
    { "l<CR>         :�����ε�����ɽ������ ", 0 },
#if ! defined(QUICKWIN) && ! defined(WINDOWS)
#ifndef	NO_PIPE
    { "| [cmd]<CR>   :�Ǹ���ɤ�������򥳥ޥ�ɤ��Ϥ� ", 0 },
#endif /* NO_PIPE */
#endif /* ! QUICKWIN && ! WINDOWS  */
    { "m<CR>         :�ᥤ����������� ", 0 },
    { "/ [ʸ����]<CR>:ʸ����ˤ�륵�֥������ȸ��� ", 0 },
    { "\\ [ʸ����]<CR>:ʸ����ˤ�륵�֥������ȵո��� ", 0 },
#if ! defined(QUICKWIN) && ! defined(WINDOWS)
#ifndef	NO_SHELL
    { "! [cmd]<CR>   :%s ���ޥ�ɤ�¹Ԥ��� ", &os },
#endif /* NO_SHELL */
#endif /* ! QUICKWIN */
    { "q<CR>         :�˥塼�����롼�ץ⡼�ɤ� ", 0 },
    { "?<CR>         :���Υإ�� ", 0 },
#ifdef WINDOWS
    { "",0 },
    { "���֥�������ɽ���ξ�Ǻ��ܥ�����֥륯��å�  :���Υ��֥������Ȥ����� ",0 },
    { "���֥�������ɽ���Τʤ��Ȥ���Ǻ��ܥ��󥯥�å�:�ǽ�Υ��֥������Ȥ����� ",0 },
    { "���ܥ��󥯥�å�                              :�˥塼�����롼�ץ⡼�ɤ�",0 },
#endif /* WINDOWS */
    { 0,0 }
  };
  
  gn_help(help_tbl);
}
