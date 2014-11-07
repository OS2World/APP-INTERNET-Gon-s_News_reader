/*
 * @(#)cancel.c	1.40 Jul.9,1997
 *
 * ������������ޤ��󡣤�������������Ū�ʳ��λ��ѡ����ۤ����¤��ߤ��ޤ���
 * Copyright (C) yamasita@omronsoft.co.jp
 */

#if defined(WINDOWS)
#	include	<windows.h>
#endif	/* WINDOWS */

#include	<stdio.h>
#include	<sys/types.h>
#include	<sys/stat.h>

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
#ifdef	HAS_DIRECT_H
#	include	<direct.h>
#endif
#ifdef	HAS_DIR_H
#	include <dir.h>
#endif

#ifdef PCNFS
#	include "pcnfs.h"
#endif
#if defined(WATTCP)
#	include <tcp.h>
#endif

#include	"nntp.h"
#include	"gn.h"

extern char *kb_input();
extern void mc_puts(),mc_printf();
extern void hit_return();
extern void kanji_convert();
extern int put_server();
extern int get_server();
extern int last_art_check();

#if defined(WINDOWS)
extern int mc_MessageBox();
#endif	/* WINDOWS */

#ifdef	MIME
extern void MIME_strHeaderEncode();
#endif

int
cancel_mode()
{
  FILE *rp;
  char resp[NNTP_BUFSIZE+1];
#ifdef	MIME
  char resp2[NNTP_BUFSIZE+1];
#endif
  char *pt;
  int x = 0;
  char *from = 0;
  char *newsgroups = 0;
  char *message_id = 0;
  char *distribution = 0;
#if !defined(USE_PUT_SERVER)
  extern FILE *ser_wr_fp;
#endif
  struct stat stat_buf;
  
  if ( last_art_check() == ERROR ){
#ifndef	WINDOWS
    mc_puts("\n����󥻥뤹�뵭�����ɤޤ�Ƥ��ޤ��� \007\n");
    hit_return();
#else  /* WINDOWS */
    mc_MessageBox(hWnd,"����󥻥뤹�뵭�����ɤޤ�Ƥ��ޤ��� ","",MB_OK);
#endif /* WINDOWS */
    return(CONT);
  }
  
#ifndef	WINDOWS
  pt = kb_input("�����˥���󥻥뤷�ޤ�����(y:����/����¾:���ʤ�)");
  
  if ( *pt != 'y' && *pt != 'Y' )
    return(CONT);
#else	/* WINDOWS */
  if ( mc_MessageBox(hWnd,"�����˥���󥻥뤷�ޤ�����",
		     "",MB_OKCANCEL) == IDCANCEL)
    return(CONT);
#endif	/* WINDOWS */
  
  /* �������ե����� */
  if ( ( rp = fopen(last_art.tmp_file,"r") ) == NULL ){
#ifdef	WINDOWS
    mc_MessageBox(hWnd,"�ե�����Υ����ץ�˼��Ԥ��ޤ���",
		  last_art.tmp_file,MB_OK);
#else  /* WINDOWS */
    perror(last_art.tmp_file);
#endif /* WINDOWS */
    return(CONT);
  }
  
  /* �إå��β��� */
  while(1) {
    if ( ++x > 20 ){
      mc_printf(".");
      fflush(stdout);
      x = 0;
#ifdef	QUICKWIN
      _wyield();
#endif
    }
    if ( fgets(resp,NNTP_BUFSIZE,rp) == NULL ) {
#ifdef	WINDOWS
      mc_MessageBox(hWnd,"�ե�������ɤ߹��ߤ˼��Ԥ��ޤ���","",MB_OK);
#else  /* WINDOWS */
      mc_puts("\n�ե�������ɤ߹��ߤ˼��Ԥ��ޤ���\n\007");
      hit_return();
#endif /* WINDOWS */
      fclose(rp);
      return(CONT);
    }
    if ( (pt = strchr(resp,'\n')) != NULL )
      *pt = 0;
    if ( resp[0] == 0 )		/* �إå��ν��� */
      break;

    if ( strncmp(resp,"From: ",6) == 0 ) {
      if ( (pt = strchr(&resp[6],' ')) != 0 )
	*pt = 0;
      from = (char *)malloc(strlen(&resp[6])+1);
      strcpy(from,&resp[6]);
      continue;
    }
    if ( strncmp(resp,"Newsgroups: ",12) == 0 ) {
      newsgroups = (char *)malloc(strlen(&resp[12])+1);
      strcpy(newsgroups,&resp[12]);
      continue;
    }
    if ( strncmp(resp,"Message-ID: ",12) == 0 ) {
      message_id = (char *)malloc(strlen(&resp[12])+1);
      strcpy(message_id,&resp[12]);
      continue;
    }
    if ( strncmp(resp,"Distribution: ",14) == 0 ) {
      distribution = (char *)malloc(strlen(&resp[14])+1);
      strcpy(distribution,&resp[14]);
      continue;
    }
  }
  fclose(rp);
  
  if ( from == 0 || newsgroups == 0 || message_id == 0 ) {
#ifdef	WINDOWS
    mc_MessageBox(hWnd,"����󥻥�Ǥ��ޤ��� ","",MB_OK);
#else  /* WINDOWS */
    mc_puts("\n����󥻥�Ǥ��ޤ��� \n\007");
    hit_return();
#endif /* WINDOWS */
    if (from)
      free(from);
    if (newsgroups)
      free(newsgroups);
    if (message_id)
      free(message_id);
    if (distribution)
      free(distribution);
    return(CONT);
  }
  
  if ( gn.genericfrom != 0 ) {
    sprintf(resp, "%s@%s", gn.usrname, gn.domainname);
  } else {
    sprintf(resp, "%s@%s.%s", gn.usrname, gn.hostname, gn.domainname);
  }
  if ( strcmp(from,resp) != 0 ) {
#ifdef	WINDOWS
    mc_MessageBox(hWnd,"¾�ͤε����ϡ�����󥻥�Ǥ��ޤ��� ","",MB_OK);
#else  /* WINDOWS */
    mc_puts("\n¾�ͤε����ϡ�����󥻥�Ǥ��ޤ��� \n\007");
    hit_return();
#endif /* WINDOWS */
    if (from)
      free(from);
    if (newsgroups)
      free(newsgroups);
    if (message_id)
      free(message_id);
    if (distribution)
      free(distribution);
    return(CONT);
  }
  
  if ( prog == GN && gn.with_gnspool ) {
    strcpy(resp,gn.newsspool);
    strcat(resp,SLASH_STR);
    strcat(resp,"news.out");
    if ( stat(resp,&stat_buf) != 0 ) {
      if ( MKDIR(resp) != 0 ) {
	mc_printf("%s �κ����˼��Ԥ��ޤ���\n",resp);
	return(-1);
      }
    }
    strcat(resp,SLASH_STR);
    strcat(resp,"gnXXXXXX");
    mktemp(&resp[0]);
    if ( ( rp = fopen(resp,"w") ) == NULL ){
#ifdef	WINDOWS
      mc_MessageBox(hWnd,"�ƥ�ݥ��ե�����Υ����ץ�˼��Ԥ��ޤ���",
		    resp,MB_OK);
#else  /* WINDOWS */
      perror(resp);
#endif /* WINDOWS */
      return(-1);
    }
    fprintf(rp,"Newsgroups: %s\n",newsgroups);
    fprintf(rp, "Subject: cancel %s\n", message_id);
    fprintf(rp, "Control: cancel %s\n", message_id);
    if ( distribution != 0 )
      fprintf(rp, "Distribution: %s\n", distribution);
    fprintf(rp, "\n");

    if ( fclose(rp) == EOF ) {
#ifdef	WINDOWS
      mc_MessageBox(hWnd,"�ƥ�ݥ��ե�����κ����˼��Ԥ��ޤ���","",
		    MB_OK);
#else  /* WINDOWS */
      perror(resp);
#endif /* WINDOWS */
      return(-1);
    }

    if (from)
      free(from);
    if (newsgroups)
      free(newsgroups);
    if (message_id)
      free(message_id);
    if (distribution)
      free(distribution);
    return(CONT);
  }
#if ! defined(NO_NET)
  if ( gn.local_spool ) {
#ifndef	MSDOS
#ifdef	USG
    putenv("HOME=");
#else
    unsetenv("HOME");
#endif
    if ( ( rp = popen(gn.postproc,"w") ) == NULL ) {
#ifdef	WINDOWS
      mc_MessageBox(hWnd,"�ݥ��ȤǤ��ޤ��� ","",MB_OK);
#else  /* WINDOWS */
      mc_puts("\n�ݥ��ȤǤ��ޤ��� \n\007");
      perror(gn.postproc);
      hit_return();
#endif /* WINDOWS */
      return(CONT);
    }
#endif /* MSDOS */
  } else {
    if ( put_server("POST") )
      return(CONNECTION_CLOSED);
    if ( get_server(resp,NNTP_BUFSIZE) )
      return(CONNECTION_CLOSED);
    switch ( atoi(resp) ) {
    case CONT_POST:
      break;
    case ERR_NOPOST:
#ifdef	WINDOWS
      mc_MessageBox(hWnd,resp,"����󥻥�Ǥ��ޤ��� ",MB_OK);
#else  /* WINDOWS */
      mc_puts("\n����󥻥�Ǥ��ޤ��� \n\007");
      mc_printf("%s\n",resp);
      hit_return();
#endif /* WINDOWS */
      if (from)
	free(from);
      if (newsgroups)
	free(newsgroups);
      if (message_id)
	free(message_id);
      if (distribution)
	free(distribution);
      return(CONT);
    default:
      return(CONNECTION_CLOSED);
    }
  }
  
  /* �إå���ʬ */
  sprintf(resp, "Path: %s", gn.usrname);
  if ( gn.local_spool )
    fprintf(rp,"%s\n",resp);
  else
    PUT_SERVER(resp);
  if ( gn.genericfrom != 0 ) {
    sprintf(resp, "From: %s@%s (%s)", gn.usrname, gn.domainname, gn.fullname);
  } else {
    sprintf(resp, "From: %s@%s.%s (%s)",
	    gn.usrname, gn.hostname, gn.domainname, gn.fullname);
  }
#ifdef	MIME
  if ( (gn.mime_head & MIME_ENCODE) != 0 ) {
    kanji_convert(internal_kanji_code,resp,JIS,resp2);
    MIME_strHeaderEncode(resp2,resp,sizeof(resp));
  }
#endif
  if ( gn.local_spool )
    fprintf(rp,"%s\n",resp);
  else
    PUT_SERVER(resp);
  sprintf(resp, "Newsgroups: %s",newsgroups);
  if ( gn.local_spool )
    fprintf(rp,"%s\n",resp);
  else
    PUT_SERVER(resp);
  sprintf(resp, "Subject: cancel %s", message_id);
  if ( gn.local_spool )
    fprintf(rp,"%s\n",resp);
  else
    PUT_SERVER(resp);
  sprintf(resp, "Control: cancel %s", message_id);
  if ( gn.local_spool )
    fprintf(rp,"%s\n",resp);
  else
    PUT_SERVER(resp);
  if ( distribution != 0 ) {
    sprintf(resp, "Distribution: %s", distribution);
    if ( gn.local_spool )
      fprintf(rp,"%s\n",resp);
    else
      PUT_SERVER(resp);
  }
  sprintf(resp, "Organization: %s", gn.organization);
#ifdef	MIME
  if ( (gn.mime_head & MIME_ENCODE) != 0 ) {
    kanji_convert(internal_kanji_code,resp,JIS,resp2);
    MIME_strHeaderEncode(resp2,resp,sizeof(resp));
  }
#endif
  if ( gn.local_spool )
    fprintf(rp,"%s\n",resp);
  else
    PUT_SERVER(resp);
  
  /* INN �Ǥϡ��إå��������ȥ���󥻥�Ǥ��ʤ� */
  if ( gn.local_spool )
    fprintf(rp,"\n");
  else
    PUT_SERVER("\n");
  
  if (from)
    free(from);
  if (newsgroups)
    free(newsgroups);
  if (message_id)
    free(message_id);
  if (distribution)
    free(distribution);
  
  if ( gn.local_spool ) {
#ifndef	MSDOS
    pclose(rp);
#endif /* MSDOS */
  } else {
    PUT_SERVER( "." );
    
#if ! defined(USE_PUT_SERVER)
    (void) fflush(ser_wr_fp);
#endif
    
    if ( get_server(resp,NNTP_BUFSIZE))
      return(CONNECTION_CLOSED);
    if (atoi(resp) != OK_POSTED ) {
#ifdef	WINDOWS
      mc_MessageBox(hWnd,resp,"����󥻥�Ǥ��ޤ���Ǥ���",MB_OK);
#else  /* WINDOWS */
      mc_puts("\n����󥻥�Ǥ��ޤ���Ǥ���\n\007");
      mc_printf("%s\n",resp);
      hit_return();
#endif /* WINDOWS */
    }
  }
  return(CONT);
#endif	/* ! NO_NET */
}
