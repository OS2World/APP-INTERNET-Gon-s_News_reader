/*
 * @(#)referenc.c	1.40 Jul.9,1997
 *
 * ������������ޤ��󡣤�������������Ū�ʳ��λ��ѡ����ۤ����¤��ߤ��ޤ���
 * Copyright (C) yamasita@omronsoft.co.jp
 */

#if defined(WINDOWS)
#	include	<windows.h>
#endif	/* WINDOWS */

#include	<stdio.h>

#ifdef	HAS_STRING_H
#	include	<string.h>
#endif
#ifdef	HAS_STDLIB_H
#	include	<stdlib.h>
#endif

#if defined(WATTCP)
#	include <tcp.h>
#endif
#ifdef PCNFS
#	include "pcnfs.h"
#endif

#include	"nntp.h"
#include	"gn.h"

extern void mc_puts(),mc_printf();
extern void hit_return();
extern int put_server();
extern int get_server();
extern void ng_name_copy();
extern int last_art_check();

#if defined(WINDOWS)
extern int mc_MessageBox();
extern void set_hourglass(),reset_cursor();
#endif	/* WINDOWS */

int
reference_mode()
{
  extern int show_article_id();
  
  register char *m_id;
#ifdef	WINDOWS
  static char *msg ="�Ǹ���ɤޤ줿�����˸������Ϥ���ޤ��� ";
#else	/* WINDOWS */
  static char *msg ="\n�Ǹ���ɤޤ줿�����˸������Ϥ���ޤ��� \n\007";
#endif	/* WINDOWS */
  
  if (last_art_check() == ERROR ){
#ifdef	WINDOWS
    mc_MessageBox(hWnd,"�������ɤޤ�Ƥ��ޤ��� ","",MB_OK);
#else  /* WINDOWS */
    mc_puts("\n�������ɤޤ�Ƥ��ޤ��� \n\007");
    hit_return();
#endif /* WINDOWS */
    return(CONT);
  }
  if (last_art.references == 0 ){
#ifdef	WINDOWS
    mc_MessageBox(hWnd,msg,"",MB_OK);
#else  /* WINDOWS */
    mc_puts(msg);
    hit_return();
#endif /* WINDOWS */
    return(CONT);
  }
  
  /* �������Υ�å������ɣĤ���� */
  m_id = strrchr(last_art.references,'>');
  if ( m_id == 0 ) {
#ifdef	WINDOWS
    mc_MessageBox(hWnd,msg,"",MB_OK);
#else  /* WINDOWS */
    mc_puts(msg);
    hit_return();
#endif /* WINDOWS */
    return(CONT);
  }
  m_id[1] = 0;
  while ( m_id >= last_art.references){
    if ( *m_id == '<' )
      break;
    m_id--;
  }
  if ( *m_id != '<' ) {
#ifdef	WINDOWS
    mc_MessageBox(hWnd,msg,"",MB_OK);
#else  /* WINDOWS */
    mc_puts(msg);
    hit_return();
#endif /* WINDOWS */
    return(CONT);
  }
  
  return(show_article_id(m_id));
}

/*
 * ��å������ɣĤˤ�뵭����ɽ��
 */

int
show_article_id(m_id)
register char *m_id;	/* ��å������ɣ� */
{
  extern int show_article_common();
  
  char resp[NNTP_BUFSIZE+1];
  register int m_len;
  FILE *history_fp;
  char history_file[PATH_LEN];
  register char *pt;
  
  if ( gn.with_gnspool == ON && gn.use_history == OFF ) {
#ifdef	WINDOWS
    mc_MessageBox(hWnd,"���ݡ��Ȥ��Ƥ��ޤ��� ","",MB_OK);
#else  /* WINDOWS */
    mc_puts("\n���ݡ��Ȥ��Ƥ��ޤ��� \n");
    hit_return();
#endif /* WINDOWS */
    return(CONT);
  }
  if ( gn.local_spool == ON ) {
    m_len = strlen(m_id);

    strcpy(history_file,gn.newslib);
    if ( history_file[strlen(history_file)-1] != SLASH_CHAR)
      strcat(history_file,SLASH_STR);
    strcat(history_file,"history");
    if ( (history_fp = fopen(history_file,"r")) == NULL ) {
#ifdef	WINDOWS
      mc_MessageBox(hWnd,"�ɤ߹��ߤ˼��Ԥ��ޤ���",history_file,MB_OK);
#else  /* WINDOWS */
      mc_printf("\n %s ���ɤ߹��ߤ˼��Ԥ��ޤ���\n",history_file);
      hit_return();
#endif /* WINDOWS */
      return(CONT);
    }

#ifdef	WINDOWS
    set_hourglass();
#endif /* WINDOWS */
    while ( fgets(resp,NNTP_BUFSIZE,history_fp) != NULL ) {
      if ( resp[1] != m_id[1] )
	continue;
      if (strncmp(resp,m_id,m_len) == 0 ) {
	fclose(history_fp);
	if ( (pt = strrchr(resp,'\t')) == NULL ) {
#ifdef	WINDOWS
	  set_hourglass();
	  mc_MessageBox(hWnd,"�۾�Ǥ�",history_file,MB_OK);
#else  /* WINDOWS */
	  mc_printf("\n %s ���۾�Ǥ�\n",history_file);
	  hit_return();
#endif /* WINDOWS */
	  return(CONT);
	}
	pt++;
	ng_name_copy(resp,pt);
#ifdef	DOSFS
	if ((pt = strchr(resp,'/')) != NULL )
	  *pt = SLASH_CHAR;
#endif
	if ((pt = strchr(resp,'\n')) != NULL )
	  *pt = 0;
	if ((pt = strchr(resp,' ')) != NULL )
	  *pt = 0;		/* cross post */
	while ((pt = strchr(resp,'.')) != NULL )
	  *pt = SLASH_CHAR;
					
	strcpy(history_file,gn.newsspool);
	strcat(history_file,SLASH_STR);
	strcat(history_file,resp);

	if ( (history_fp = fopen(history_file,"r")) == NULL ) {
#ifdef	WINDOWS
	  set_hourglass();
	  mc_MessageBox(hWnd,"����ε�����¸�ߤ��ޤ��� ","",MB_OK);
#else  /* WINDOWS */
	  mc_puts("\n����ε�����¸�ߤ��ޤ��� \n\007");
	  hit_return();
#endif /* WINDOWS */
	  return(CONT);
	}
	return(show_article_common((newsgroup_t *)0,0L,history_fp,history_file));
      }
    }
    fclose(history_fp);
#ifdef	WINDOWS
    set_hourglass();
    mc_MessageBox(hWnd,"����ε�����¸�ߤ��ޤ��� ","",MB_OK);
#else  /* WINDOWS */
    mc_puts("\n����ε�����¸�ߤ��ޤ��� \n\007");
    hit_return();
#endif /* WINDOWS */
    return(CONT);
  }
  
#if ! defined(NO_NET)
#ifdef	WINDOWS
  set_hourglass();
#endif	/* WINDOWS */
  sprintf(resp,"ARTICLE %s",m_id);
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
  case ERR_NOART:
#ifdef	WINDOWS
    reset_cursor();
    mc_MessageBox(hWnd,"����ε�����¸�ߤ��ޤ��� ","",MB_OK);
#else  /* WINDOWS */
    mc_puts("\n����ε�����¸�ߤ��ޤ��� \n\007");
    hit_return();
#endif /* WINDOWS */
    return(CONT);
  default:
#ifdef	WINDOWS
    reset_cursor();
#endif /* WINDOWS */
    return(CONNECTION_CLOSED);
  }
  
  return(show_article_common((newsgroup_t *)0,0L,(FILE *)0,0));
#endif
}
