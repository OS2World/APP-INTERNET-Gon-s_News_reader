/*
 * @(#)save.c	1.40 Jan.7,1998
 *
 * 著作権は放棄しません。ただし、営利目的以外の使用／配布に制限は設けません。
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
  
  /* 記事が読まれていることの確認 */
  if ( last_art_check() == ERROR ){
#ifdef	WINDOWS
    mc_MessageBox(hWnd,"保存する記事が読まれていません ","",MB_OK);
#else  /* WINDOWS */
    mc_puts("\n保存する記事が読まれていません \n\007");
    hit_return();
#endif /* WINDOWS */
    return;
  }
  
  /* ファイル名の確定 */
  file++;
  while( *file == ' ' || *file == '\t' )
    file++;
  if ( *file != 0 ) {		/* コマンドラインで指定 */
    if ( (pt = strchr(file,'\n')) != 0 )
      *pt = 0;
    if ( (pt = expand_filename(file,gn.savedir)) == 0 ) {
#ifdef	WINDOWS
      mc_MessageBox(hWnd,"ファイル名の展開に失敗しました",file,MB_OK);
#else  /* WINDOWS */
      mc_printf("ファイル名の展開に失敗しました(%s)\n",file);
      hit_return();
#endif /* WINDOWS */
      return;
    }
    strcpy(resp,pt);
    free(pt);
  } else {
#if defined(MSDOS) || defined(OS2)
    char *pp;
    
    sprintf(resp,"保存するファイル名を入力して下さい（デフォルト ");
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
    strcat(resp,"）");
#else  /* MSDOS || OS2 */
    sprintf(resp,
	    "保存するファイル名を入力して下さい（デフォルト %s%s）",
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
      mc_MessageBox(hWnd,"ファイル名の展開に失敗しました","",MB_OK);
#else  /* WINDOWS */
      mc_puts("ファイル名の展開に失敗しました\n");
      hit_return();
#endif /* WINDOWS */
      return;
    }
    strcpy(resp,pt);
    free(pt);
  }
  
  /* ディレクトリはある？ */
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
	  mc_printf("ディレクトリ %s がありません。作成します\n",
		    resp2);
	  if ( MKDIR(resp2) != 0 ) { /* boooom */
#ifdef	WINDOWS
	    mc_MessageBox(hWnd,"作成することができません ",
			  resp2,MB_OK);
#else  /* WINDOWS */
	    mc_printf("%s を作成することができません \n",resp2);
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
	  mc_MessageBox(hWnd,"通常ディレクトリではありません ",
			resp2,MB_OK);
#else  /* WINDOWS */
	  mc_printf("%s は通常ディレクトリではありません \n",resp2);
	  hit_return();
#endif /* WINDOWS */
	  return;
	}
      }
      *pt++ = SLASH_CHAR;
    }
  }
 mkdir_end:
  
  /* 新規／追加の判断 */
  if ( stat(resp,&statbuf) ) {
    switch(errno) {
    case ENOENT:
      mc_printf("%s を作成します\n",resp);
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
      mc_MessageBox(hWnd,"通常のファイルにしか出力できません ",
		    "",MB_OK);
#else  /* WINDOWS */
      mc_puts("通常のファイルにしか出力できません \n");
      hit_return();
#endif /* WINDOWS */
      return;
    }
    mc_printf("%s に追加します\n",resp);
  }
  
  /* 保存ファイルのオープン */
  if ( ( wp = fopen(resp,"a") ) == NULL ){
#ifdef	WINDOWS
    mc_MessageBox(hWnd,resp,"",MB_OK);
#else  /* WINDOWS */
    perror(resp);
    hit_return();
#endif /* WINDOWS */
    return;
  }
  
  /* 元記事ファイルのオープン */
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
  
  /* unix mail ヘッダ */
  
  tm.tm_isdst = 0;
  pt = last_art.date;
  while ( *pt == ' ' )
    pt++;
  
  /*      27 May 91 07:38:41 GMT   -> Mon May 27 07:38:41 1991 */
  /* Wed, 22 Apr 1992 06:39:12 GMT -> Wed Apr 22 06:39:12 1992 */
  
  while (('A' <= *pt && *pt <= 'Z') ||
	 ('a' <= *pt && *pt <= 'z') ||
	 (',' == *pt || *pt == ' ') ) /* Cnews フォーマット */
    pt++;
  
  /* 日 */
  tm.tm_mday = atoi(pt);
  while ( '0' <= *pt && *pt <= '9' )
    pt++;
  while ( *pt == ' ' )
    pt++;
  /* 月 */
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
  /* 年 */
  tm.tm_year = atoi(pt);
  if ( tm.tm_year >= 1900 )
    tm.tm_year -= 1900;		/* 2000 年以後は？ */
  
  while ( '0' <= *pt && *pt <= '9' )
    pt++;
  while ( *pt == ' ' )
    pt++;
  /* 時 */
  tm.tm_hour = atoi(pt);
  while ( '0' <= *pt && *pt <= '9' )
    pt++;
  pt++;		/* : */
  /* 分 */
  tm.tm_min = atoi(pt);
  while ( '0' <= *pt && *pt <= '9' )
    pt++;
  pt++;		/* : */
  /* 秒 */
  tm.tm_sec = atoi(pt);
  while ( '0' <= *pt && *pt <= '9' )
    pt++;
  /* 曜日 */
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
  
  /* 追加 */
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

/* １月１日の曜日を求める （１９００〜）*/
static int
jan1(year)
register int year;
{
  year += 1900;
  return( (4+year+(year+3)/4 - (year-1701)/100 + (year-1601)/400 + 3)%7);
}
#endif /* NO_SAVE */
