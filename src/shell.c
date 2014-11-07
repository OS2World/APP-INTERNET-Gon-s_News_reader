/*
 * @(#)shell.c	1.40 Jul.9,1997
 *
 * ������������ޤ��󡣤�������������Ū�ʳ��λ��ѡ����ۤ����¤��ߤ��ޤ���
 * Copyright (C) yamasita@omronsoft.co.jp
 */

#if ! defined(QUICKWIN) && ! defined(WINDOWS)

#include	<stdio.h>
#include	<signal.h>

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

extern char *kb_input();
extern void mc_puts();
extern void hit_return();
extern int last_art_check();

#ifndef	NO_PIPE
void
pipe_mode(command)
register char *command;
{
  register char *pt;
#ifdef MSDOS
  char cmd[128];
#else
  register FILE *rp,*wp;
  char resp[NNTP_BUFSIZE+1];
#endif
  
  /* �������ɤޤ�Ƥ��뤳�Ȥγ�ǧ */
  if ( last_art_check() == ERROR ){
    mc_puts("\n���ޥ�ɤ��Ϥ��������ɤޤ�Ƥ��ޤ��� \n\007");
    hit_return();
    return;
  }
  
  /* ���ޥ�ɤγ��� */
  command++;		/* '|' */
  while( *command == ' ' || *command == '\t' )
    command++;
  if ( *command == 0 )
    command = kb_input("�¹Ԥ��륳�ޥ��̾�����Ϥ��Ʋ�����");
  
  if ( (pt = strchr(command,'\n')) != 0 )
    *pt = 0;
  
#ifdef MSDOS
  strcpy( cmd, command );
  strcat( cmd, " "  );
  strcat( cmd, last_art.tmp_file );
  system( cmd );
#else  /* !MSDOS */
  signal(SIGPIPE,SIG_IGN);
  
  /* ���ޥ�ɼ¹� */
  if ( ( wp = popen(command,"w") ) == NULL ){
    signal(SIGPIPE,SIG_DFL);
    perror(command);
    hit_return();
    return;
  }
  
  /* �������ե�����Υ����ץ� */
  if ( ( rp = fopen(last_art.tmp_file,"r") ) == NULL ){
    signal(SIGPIPE,SIG_DFL);
    perror(last_art.tmp_file);
    hit_return();
    fclose(wp);
    return;
  }
  
  /* �ɲ� */
  while ( fgets(resp,NNTP_BUFSIZE,rp) != NULL )
    fprintf(wp,"%s",resp);
  
  fclose(rp);
  pclose(wp);
  signal(SIGPIPE,SIG_DFL);
#endif
  hit_return();
}
#endif /* NO_PIPE */
#ifndef	NO_SHELL
void
shell_mode(command)
register char *command;
{
  register char *pt;
  
  /* ���ޥ�ɤγ��� */
  command++;		/* ! */
  while( *command == ' ' || *command == '\t' )
    command++;
  if ( *command == 0 )
    command = kb_input("�¹Ԥ��륳�ޥ��̾�����Ϥ��Ʋ�����");
  
  if ( (pt = strchr(command,'\n')) != 0 )
    *pt = 0;
  
  /* ���ޥ�ɼ¹� */
  system(command);
  hit_return();
}
#endif /* NO_SHELL */
#endif /* ! defined(QUICKWIN) && ! defined(WINDOWS) */
