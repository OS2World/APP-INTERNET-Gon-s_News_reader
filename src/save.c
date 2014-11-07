/*
 * @(#)save.c	1.40 Jan.7,1998
 *
 * ������������ޤ��󡣤�������������Ū�ʳ��λ��ѡ����ۤ����¤��ߤ��ޤ���
 * Copyright (C) yamasita@omronsoft.co.jp
 */

#ifndef	NO_SAVE

#ifdef WINDOWS
#	include <windows.h>
#endif

#include	<stdio.h>
#include	<sys/types.h>
#include	<sys/stat.h>
#include	<errno.h>
#include	<time.h>

#ifdef	HAS_STRING_H
#	include	<string.h>
#endif
#ifdef	HAS_STDLIB_H
#	include	<stdlib.h>
#endif
#ifdef	HAS_DIRECT_H
#	include	<direct.h>
#endif
#ifdef	HAS_DIR_H
#	include <dir.h>
#endif

#ifdef MSC
#	ifndef	MSC9
extern int _near _cdecl volatile errno;
#	endif
#endif
#ifdef	UNIX
extern int errno;
#endif /* UNIX */


#if defined(WATTCP)
#	include <tcp.h>
#endif
#ifdef PCNFS
#	include "pcnfs.h"
#endif

#include	"nntp.h"
#include	"gn.h"

extern char *kb_input();
extern char *expand_filename();
extern void mc_puts(),mc_printf();
extern void hit_return();
extern void kanji_convert();
extern int last_art_check();

#if defined(WINDOWS)
extern int mc_MessageBox();
#endif	/* WINDOWS */

static jan1();

void
save_mode(file)
char *file;
{
  FILE *rp,*wp;
  char resp[NNTP_BUFSIZE+1],resp2[NNTP_BUFSIZE+1],*pt,*key_buf;
  struct stat statbuf;
  struct tm tm;
  static char *month[] = {
    "Jan","Feb","Mar","Apr","May","Jun",
    "Jul","Aug","Sep","Oct","Nov","Dec"
  };
  static char mon[] = { 0,
			  31, 29, 31, 30, 31, 30,
			  31, 31, 30, 31, 30, 31,
			};
  int i,day;
  
  /* �������ɤޤ�Ƥ��뤳�Ȥγ�ǧ */
  if ( last_art_check() == ERROR ){
#ifdef	WINDOWS
    mc_MessageBox(hWnd,"��¸���뵭�����ɤޤ�Ƥ��ޤ��� ","",MB_OK);
#else  /* WINDOWS */
    mc_puts("\n��¸���뵭�����ɤޤ�Ƥ��ޤ��� \n\007");
    hit_return();
#endif /* WINDOWS */
    return;
  }
  
  /* �ե�����̾�γ��� */
  file++;
  while( *file == ' ' || *file == '\t' )
    file++;
  if ( *file != 0 ) {		/* ���ޥ�ɥ饤��ǻ��� */
    if ( (pt = strchr(file,'\n')) != 0 )
      *pt = 0;
    if ( (pt = expand_filename(file,gn.savedir)) == 0 ) {
#ifdef	WINDOWS
      mc_MessageBox(hWnd,"�ե�����̾��Ÿ���˼��Ԥ��ޤ���",file,MB_OK);
#else  /* WINDOWS */
      mc_printf("�ե�����̾��Ÿ���˼��Ԥ��ޤ���(%s)\n",file);
      hit_return();
#endif /* WINDOWS */
      return;
    }
    strcpy(resp,pt);
    free(pt);
  } else {
#if defined(MSDOS) || defined(OS2)
    char *pp;
    
    sprintf(resp,"��¸����ե�����̾�����Ϥ��Ʋ������ʥǥե���� ");
    pt = &resp[strlen(resp)];
    for ( pp = gn.savedir ; *pp != 0 ; pp++ ) {
      if (*pp == SLASH_CHAR){
	*pt++ = SLASH_CHAR;
      } else {
	*pt++ = *pp;
      }
    }
    for ( pp = last_art.newsgroups ; *pp != 0 ; pp++ ) {
      if (*pp == '.'){
	*pt++ = SLASH_CHAR;
      } else {
	*pt++ = *pp;
      }
    }
    *pt = 0;
    strcat(resp,"��");
#else  /* MSDOS || OS2 */
    sprintf(resp,
	    "��¸����ե�����̾�����Ϥ��Ʋ������ʥǥե���� %s%s��",
	    gn.savedir,last_art.newsgroups);
#endif /* MSDOS || OS2 */
    
    if ( (key_buf = kb_input(resp)) == NULL )
      return;
    
    if ( (pt = strchr(key_buf,'\n')) != 0 )
      *pt = 0;
    if ( strlen(key_buf) == 0 ){
#if defined(MSDOS) || defined(OS2)
      strcpy(resp,last_art.newsgroups);
      for ( pt = resp ; *pt != 0 ; pt++) {
	if ( *pt == '.' )
	  *pt = SLASH_CHAR;
      }
      pt = expand_filename(resp,gn.savedir);
#else
      pt = expand_filename(last_art.newsgroups,gn.savedir);
#endif
    } else {
      pt = expand_filename(key_buf,gn.savedir);
    }
    if ( pt == 0 ) {
#ifdef	WINDOWS
      mc_MessageBox(hWnd,"�ե�����̾��Ÿ���˼��Ԥ��ޤ���","",MB_OK);
#else  /* WINDOWS */
      mc_puts("�ե�����̾��Ÿ���˼��Ԥ��ޤ���\n");
      hit_return();
#endif /* WINDOWS */
      return;
    }
    strcpy(resp,pt);
    free(pt);
  }
  
  /* �ǥ��쥯�ȥ�Ϥ��롩 */
  strcpy(resp2,resp);
  if ( (pt = strrchr(resp2,SLASH_CHAR)) != NULL ){ /* dos= a:file */
    *(++pt) = 0;		/* terminator */
#if defined(MSDOS) || defined(OS2)
    pt = resp2;
    if (('A' <= *pt && *pt <= 'Z' || 'a' <= *pt && *pt <= 'z') &&
	pt[1] == ':' && pt[2] == SLASH_CHAR ) {
      pt = &resp2[3];
    } else {
      pt = &resp2[1];
    }
#else
    pt = &resp2[1];
#endif
    while ( pt != 0 ){
      while ( *pt != SLASH_CHAR ) {
	if ( *pt == 0 )
	  goto mkdir_end;
	pt++;
      }
      *pt = 0;
      if ( stat(resp2,&statbuf) != 0) {
	switch(errno) {
	case ENOENT:
	  mc_printf("�ǥ��쥯�ȥ� %s ������ޤ��󡣺������ޤ�\n",
		    resp2);
	  if ( MKDIR(resp2) != 0 ) { /* boooom */
#ifdef	WINDOWS
	    mc_MessageBox(hWnd,"�������뤳�Ȥ��Ǥ��ޤ��� ",
			  resp2,MB_OK);
#else  /* WINDOWS */
	    mc_printf("%s ��������뤳�Ȥ��Ǥ��ޤ��� \n",resp2);
	    hit_return();
#endif /* WINDOWS */
	    return;
	  }
	  break;
	default:
#ifdef	WINDOWS
	  mc_MessageBox(hWnd,resp2,NULL,MB_OK);
#else  /* WINDOWS */
	  perror(resp2);
	  hit_return();
#endif /* WINDOWS */
	  return;
	}
      } else {
	if ( ( statbuf.st_mode & S_IFMT) != S_IFDIR ) {
#ifdef	WINDOWS
	  mc_MessageBox(hWnd,"�̾�ǥ��쥯�ȥ�ǤϤ���ޤ��� ",
			resp2,MB_OK);
#else  /* WINDOWS */
	  mc_printf("%s ���̾�ǥ��쥯�ȥ�ǤϤ���ޤ��� \n",resp2);
	  hit_return();
#endif /* WINDOWS */
	  return;
	}
      }
      *pt++ = SLASH_CHAR;
    }
  }
 mkdir_end:
  
  /* �������ɲä�Ƚ�� */
  if ( stat(resp,&statbuf) ) {
    switch(errno) {
    case ENOENT:
      mc_printf("%s ��������ޤ�\n",resp);
      break;
    default:
#ifdef	WINDOWS
      mc_MessageBox(hWnd,resp,"",MB_OK);
#else  /* WINDOWS */
      perror(resp);
      hit_return();
#endif /* WINDOWS */
      return;
    }
  } else {
    if ( statbuf.st_mode & S_IFMT & (~S_IFREG) ) {
#ifdef	WINDOWS
      mc_MessageBox(hWnd,"�̾�Υե�����ˤ������ϤǤ��ޤ��� ",
		    "",MB_OK);
#else  /* WINDOWS */
      mc_puts("�̾�Υե�����ˤ������ϤǤ��ޤ��� \n");
      hit_return();
#endif /* WINDOWS */
      return;
    }
    mc_printf("%s ���ɲä��ޤ�\n",resp);
  }
  
  /* ��¸�ե�����Υ����ץ� */
  if ( ( wp = fopen(resp,"a") ) == NULL ){
#ifdef	WINDOWS
    mc_MessageBox(hWnd,resp,"",MB_OK);
#else  /* WINDOWS */
    perror(resp);
    hit_return();
#endif /* WINDOWS */
    return;
  }
  
  /* �������ե�����Υ����ץ� */
  if ( ( rp = fopen(last_art.tmp_file,"r") ) == NULL ){
#ifdef	WINDOWS
    mc_MessageBox(hWnd,last_art.tmp_file,"",MB_OK);
#else  /* WINDOWS */
    perror(last_art.tmp_file);
    hit_return();
#endif /* WINDOWS */
    fclose(wp);
    return;
  }
  
  /* unix mail �إå� */
  
  tm.tm_isdst = 0;
  pt = last_art.date;
  while ( *pt == ' ' )
    pt++;
  
  /*      27 May 91 07:38:41 GMT   -> Mon May 27 07:38:41 1991 */
  /* Wed, 22 Apr 1992 06:39:12 GMT -> Wed Apr 22 06:39:12 1992 */
  
  while (('A' <= *pt && *pt <= 'Z') ||
	 ('a' <= *pt && *pt <= 'z') ||
	 (',' == *pt || *pt == ' ') ) /* Cnews �ե����ޥå� */
    pt++;
  
  /* �� */
  tm.tm_mday = atoi(pt);
  while ( '0' <= *pt && *pt <= '9' )
    pt++;
  while ( *pt == ' ' )
    pt++;
  /* �� */
  for ( i = 0 ; i < 12 ; i++ ){
    if (strncmp(pt,month[i],3) == 0 ){
      tm.tm_mon = i;		/* 0 origin */
      break;
    }
  }
  while ( *pt != ' ' )
    pt++;
  while ( *pt == ' ' )
    pt++;
  /* ǯ */
  tm.tm_year = atoi(pt);
  if ( tm.tm_year >= 1900 )
    tm.tm_year -= 1900;		/* 2000 ǯ�ʸ�ϡ� */
  
  while ( '0' <= *pt && *pt <= '9' )
    pt++;
  while ( *pt == ' ' )
    pt++;
  /* �� */
  tm.tm_hour = atoi(pt);
  while ( '0' <= *pt && *pt <= '9' )
    pt++;
  pt++;		/* : */
  /* ʬ */
  tm.tm_min = atoi(pt);
  while ( '0' <= *pt && *pt <= '9' )
    pt++;
  pt++;		/* : */
  /* �� */
  tm.tm_sec = atoi(pt);
  while ( '0' <= *pt && *pt <= '9' )
    pt++;
  /* ���� */
  day = jan1(tm.tm_year);
  mon[2] = 29;
  switch((jan1(tm.tm_year+1)+7-day)%7) {
  case 1:
    mon[2] = 28;
    break;
  default:
    break;
  }
  for(i=1; i<=tm.tm_mon; i++)
    day += mon[i];
  day += tm.tm_mday -1;
  tm.tm_wday = day%7;
  
  kanji_convert(internal_kanji_code,last_art.from,gn.file_kanji_code,resp);
  if ( (pt = strchr(resp,'(')) != NULL )
    *pt = 0;
  if ( (pt = strchr(resp,'<')) != NULL ){
    pt++;
    strcpy(resp,pt);
    if ( (pt = strchr(resp,'>')) != NULL )
      *pt = 0;
  }
  fprintf(wp,"From %s %s",resp, asctime(&tm));
  
  /* �ɲ� */
  if ( gn.process_kanji_code == gn.file_kanji_code ) {
    while ( fgets(resp,NNTP_BUFSIZE,rp) != NULL ) {
      if ( strncmp(resp,"From ",5) == 0 )
	fprintf(wp,">%s",resp);
      else
	fprintf(wp,"%s",resp);
    }
  } else {
    while ( fgets(resp,NNTP_BUFSIZE,rp) != NULL ) {
      kanji_tofile(resp,resp2);
      if ( strncmp(resp2,"From ",5) == 0 )
	fprintf(wp,">%s",resp2);
      else
	fprintf(wp,"%s",resp2);
    }
  }
  fprintf(wp,"\n");
  
  fclose(rp);
  fclose(wp);
}

/* ���������������� �ʣ�����������*/
static int
jan1(year)
register int year;
{
  year += 1900;
  return( (4+year+(year+3)/4 - (year-1701)/100 + (year-1601)/400 + 3)%7);
}
#endif /* NO_SAVE */
