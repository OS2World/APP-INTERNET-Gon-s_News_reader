/*
 * @(#)gncom.c	1.40 Aug.13,1997
 *
 * 著作権は放棄しません。ただし、営利目的以外の使用／配布に制限は設けません。
 * Copyright (C) yamasita@omronsoft.co.jp
 */

#if defined(WINDOWS)
#	include	<windows.h>
#	include	"wingn.h"
#endif	/* WINDOWS */

#include	<stdio.h>
#include	<sys/types.h>
#include	<sys/stat.h>
#include	<time.h>

#ifdef	UNIX
#	include <fcntl.h>
#	include <errno.h>
#	include <sys/signal.h>
extern int errno;
#endif /* UNIX */

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

#ifdef OS2
#	define INCL_VIO            /* for VioGetMode */
#	include <os2.h>
#endif

#if defined(WATTCP)
#	include <tcp.h>
#endif
#ifdef PCNFS
#	include "pcnfs.h"
#endif


#ifdef	uniosu
char *getenv();
#endif

#include	"nntp.h"
#include	"gn.h"

extern char *kb_input();
extern void mc_puts(),mc_printf();
extern void kanji_convert();
extern char *expand_filename();

#if defined(WINDOWS)
extern int mc_MessageBox();
static HCURSOR hSaveCursor = NULL;	/* カーソルのハンドル */
#endif	/* WINDOWS */

static void newsrc_gnus_count(),newsrc_kill_article();

void
memory_error()
{
#ifndef	WINDOWS
  mc_puts("メモリ不足で実行できません \n");
  exit(1);
#else	/* WINDOWS */
  mc_MessageBox(hWnd,"メモリ不足で実行できません ","",MB_OK);
  PostQuitMessage(0);
#endif	/* WINDOWS */
}

/*
 * ファイル名の展開
 */

char *
expand_filename(name,default_dir)
char *name;		/* 解析する名前 */
char *default_dir;	/* / から始まらない時のデフォルトディレクトリ */
{
  char buf1[PATH_LEN],buf2[PATH_LEN],buf3[PATH_LEN];
  char *pt;
  char *path;
  char *mae,*ato,*naka;

  /* 先行するホワイトスペースを削除 */
  pt = name;
  while ( *pt == ' ' || *pt == '\t' )
    pt++;
  
  /* 後に続くホワイトスペース、ニューラインを削除 */
  ato = &pt[strlen(pt)-1];
  while ( *ato == ' ' || *ato == '\t' || *ato == '\n' )
    *ato-- = 0;
  
  if ( (mae = malloc(strlen(pt)+1)) == NULL ) {
    memory_error();
    return(0);
  }
  strcpy(mae,pt);
  pt = mae;
  
  while( 1 ) {
    /* 変数の展開 */
    while ( (naka = strchr(pt,'$')) != 0 ){
      ato = naka + 1;
      while(('A' <= *ato && *ato <= 'Z') || ('a' <= *ato && *ato <= 'z') ||
	    ('0' <= *ato && *ato <= '9') || *ato == '_' )
	ato++;
      strcpy(buf3,ato);		/* $ より右 */
      *ato = 0;
      strcpy(buf2,naka);	/* $何とか */
      *naka = 0;
      strcpy(buf1,pt);		/* $ より左 */
      free(pt);
      if ( (naka = getenv(&buf2[1]))== 0 ) {
#if ! defined(WINDOWS)
	mc_printf("環境変数の展開に失敗しました(%s)\n",buf2);
#else  /* WINDOWS */
	mc_MessageBox(hWnd,"環境変数の展開に失敗しました",
		      buf2,MB_OK);
#endif /* WINDOWS */
	return(0);
      }
      if ( (pt = malloc(strlen(buf1)+strlen(naka)+strlen(buf3)+3)) == NULL) {
	memory_error();
	return(0);
      }
      strcpy(pt,buf1);
      strcat(pt,SLASH_STR);
      strcat(pt,naka);
      strcat(pt,SLASH_STR);
      strcat(pt,buf3);
      continue;
    }

    /* ~/ の展開 */
    if ( *pt == '~' ) {
      if ( pt[1] == SLASH_CHAR ){
	if ( (path = malloc(strlen(pt)+strlen(gn.home)+1)) == NULL ) {
	  memory_error();
	  return(0);
	}
	strcpy(path,gn.home);
	strcat(path,SLASH_STR);
	strcat(path,&pt[1]);
	free(pt);
	pt = path;
	continue;
      }
    }

    /* // を / に変換する */
#ifdef	apollo
    for ( mae = &pt[1] ; *mae ; mae++)
#else
      for ( mae = pt ; *mae ; mae++)
#endif
	while( 1 ) {
	  if ( mae[0] == SLASH_CHAR  && mae[1] == SLASH_CHAR )
	    strcpy(&mae[0],&mae[1]);
	  else
	    break;
	}

    /* １文字目による判断 */
#if defined(MSDOS) || defined(OS2)
    for ( mae = pt ; *mae ; mae++){
      if ( mae[0] == '.' && strlen( mae ) > 4 )
	mae[0] = SLASH_CHAR;
    }
    if ( *pt != SLASH_CHAR && *(pt+1) != ':' ) {
      if ( (path = malloc(strlen(pt)+strlen(default_dir)+4)) == NULL ) {
	memory_error();
	return(0);
      }
      if ( *default_dir != SLASH_CHAR && *(default_dir+1) != ':' ){
	strcpy(path,SLASH_STR);
	strcat(path,default_dir);
      } else
	strcpy(path,default_dir);
      strcat(path,SLASH_STR);
      strcat(path,pt);
      free(pt);
      pt = path;
      continue;
    }
#else
    if ( *pt != SLASH_CHAR ) {
      if ( (path = malloc(strlen(pt)+strlen(default_dir)+2)) == NULL) {
	memory_error();
	return(0);
      }
      strcpy(path,default_dir);
      strcat(path,SLASH_STR);
      strcat(path,pt);
      free(pt);
      pt = path;
      continue;
    }
#endif
    break;
  }
  
#ifdef DOSFS
  /* / -> \ */
  while ( (path = strchr(pt,'/')) != NULL )
    *path = SLASH_CHAR;
#endif
  
  return(pt);
}

/*
 * ニュースグループ名のコピー
 * dosfs の場合は
 * . と . の間が LEN 文字を越える場合は LEN 文字で切り詰める
 */

void
ng_name_copy(dest,src)
register char *dest,*src;
{
  register int count;
  register char *ptdest,*pt;

  if ( gn.dosfs == OFF ) {
    strcpy(dest,src);
    return;
  }

  ptdest = dest;

  count = 0;
  while (1) {
    if ( count >= BASENAME_LEN ) {
      count = 0;
      while ( *src != 0 && *src != '.' )
	src++;
    }
    if ( *src == 0 ) {
      *dest = 0;
      break;
    }
    if ( *src == '.' ) {
      *dest++ = *src++;
      count = 0;
      continue;
    }
    *dest++ = *src++;
    count++;
  }

  /* + -> ` */
  while ( (pt = strchr(ptdest,'+')) != NULL )
    *pt = '`';
}

void
ng_directory(dir,ng_name)
register char *dir;		/* そのニュースグループのディレクトリ */
register char *ng_name;		/* ニュースグループ名 */
{
  char ng_dir[PATH_LEN];
  register char *pt;

  ng_name_copy(ng_dir,ng_name);

  /* ニュースグループ名の . -> / */
  pt = ng_dir;
  while( (pt = strchr(pt,'.')) != NULL )
    *pt = SLASH_CHAR;

  strcpy(dir,gn.newsspool);
  if ( dir[strlen(dir)-1] != SLASH_CHAR)
    strcat(dir,SLASH_STR);
  strcat(dir,ng_dir);
}
/*
 * ファイルから一つのヘッダを得る
 * resp[0] が 0 なら、ヘッダの終り
 */
int
get_1_header_file(buf,resp,fp)
char *buf;		/* 結果 */
char *resp;	/* 作業領域 */
FILE *fp;
{
  register int buf_len,resp_len;
  char *pt;
  
  strcpy(buf,resp);	/* 前の解析の残り */
  buf_len = strlen(buf);
  
  while (1) {
    resp[0] = 0;
    if ( fgets(resp,NNTP_BUFSIZE,fp) == NULL )
      return(CONT);
    if ( (pt = strchr(resp,'\n')) != NULL)
      *pt = 0;
    if ( (pt = strchr(resp,'\r')) != NULL)
      *pt = 0;

    if ( resp[0] == 0 || resp[0] == '.' ||
	('A' <= resp[0] && resp[0] <= 'Z') ||
	('a' <= resp[0] && resp[0] <= 'z') )
      break;
    resp_len = strlen(resp);
    if ( buf_len + 1 + resp_len > NNTP_BUFSIZE ) {
      /* NNTP_BUFSIZE 以上は捨てる X-< */
      while (1) {
	resp[0] = 0;
	if ( fgets(resp,NNTP_BUFSIZE,fp) == NULL )
	  return(CONT);
	if ( (pt = strchr(resp,'\n')) != NULL)
	  *pt = 0;
	if ( (pt = strchr(resp,'\r')) != NULL)
	  *pt = 0;
	if ( resp[0] != ' ' && resp[0] != TAB )
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


/*
 * ニュースグループのチェック
 */
void
check_group(check_only)
int check_only;
{
  extern void check_group_sub();
  
  char resp[NNTP_BUFSIZE+1];
#if ! defined(WINDOWS)
  char *key_p;
#endif
  register newsgroup_t **ng;
  register int large_group = 0;
  
  if ( newsrc == 0 )
    return;
  
  ng = &newsrc;
  while(*ng != 0) {
    if ( ((*ng)->flag & ACTIVE) != ACTIVE ) {
      /* アクティブでないグループは、キューから外す */
      mc_printf("\n%s はアクティブではありません \n",(*ng)->name);
      *ng = (*ng)->next;
      change = 1;
      continue;
    }
    check_group_sub(*ng);
    /* 未読の記事が多い時はチェック */
    if (((*ng)->flag & UNSUBSCRIBE) != UNSUBSCRIBE &&
	(*ng)->numb > gn.article_limit && gn.article_limit > 0 ) {
      large_group = 1;
    }
    ng = &((*ng)->next);
  }
  
  /* 未読の記事が多い時は確認入力 */
  if ( check_only == 0 && large_group == 1 ) {
#if ! defined(WINDOWS)
    sprintf(resp,
	    "\n未読の記事が%dより多くなっているニュースグループがあります\n%d だけ残して消しますか？(y:消す/その他:全部残す)",
	    gn.article_limit,gn.article_leave);

    key_p = kb_input(resp);
    if ( *key_p == 'y' || *key_p == 'Y' )
      newsrc_kill_article();
#else  /* WINDOWS */
    sprintf(resp,
	    "未読の記事が%dより多くなっているニュースグループがあります %d だけ残して消します ",
	    gn.article_limit,gn.article_leave);
    switch (mc_MessageBox(hWnd,resp,"",MB_OKCANCEL)) {
    case IDOK:
      newsrc_kill_article();
      break;
    }
#endif /* WINDOWS */
  }
}
void
check_group_sub(ng)
register newsgroup_t *ng;
{
  if ( ng->read < ng->first )
    ng->read = ng->first - 1;
  if ( ng->last < ng->read )
    ng->read = ng->last;
  
  /* 未読記事数 */
  if ( ng->first < ng->read + 1)
    ng->numb = ng->last - ng->read;
  else
    ng->numb = ng->last - ng->first + 1;
  
  /* GNUS フォーマット時の未読記事数 */
  if ( ng->newsrc )
    newsrc_gnus_count(ng);
  
  if ( ng->numb < 0 )	/* 異常な newsrc の場合のみ */
    ng->numb = 0;
  
  if ( ng->numb == 0 )	/* 未読記事なし */
    ng->flag |= NOARTICLE;
  else
    ng->flag &= ~NOARTICLE;
}
static void
newsrc_gnus_count(ng)
register newsgroup_t *ng;
{
  register char *pt,*wk;
  register long st,ed;
  
  pt = ng->newsrc;
  while ( '0' <= *pt && *pt <= '9' ) {
    st = atol(pt);		/* 範囲開始番号 */
    while ( '0' <= *pt && *pt <= '9' )
      pt++;

    switch ( *pt ) {
    case '-':			/* 999-999 */
      pt++;
      ed = atol(pt);		/* 範囲終了番号 */
      while ( '0' <= *pt && *pt <= '9' )
	pt++;
      if ( *pt == ',' )
	pt++;

      /* 終了番号が既読の場合は捨てる */
      if ( ed <= ng->read ){
	if ( strlen(pt) != 0 ) {
	  wk = malloc(strlen(pt)+1);
	  strcpy(wk,pt);
	  free(ng->newsrc);
	  ng->newsrc = wk;
	  pt = wk;
	} else {
	  free(ng->newsrc);
	  ng->newsrc = 0;
	  return;
	}
	break;
      }
      /* 開始番号が存在しない場合（異常）は捨てる */
      if ( ng->last < st ){
	if ( strlen(pt) != 0 ) {
	  wk = malloc(strlen(pt)+1);
	  strcpy(wk,pt);
	  free(ng->newsrc);
	  ng->newsrc = wk;
	  pt = wk;
	} else {
	  free(ng->newsrc);
	  ng->newsrc = 0;
	  return;
	}
	break;
      }
      /* 開始は既読で終了は未読 */
      if ( st <= ng->read && ng->read < ed && ed <= ng->last ) {
	ng->numb -= ed - ng->read;
	ng->read = ed;
	if ( strlen(pt) != 0 ) {
	  wk = malloc(strlen(pt)+1);
	  strcpy(wk,pt);
	  free(ng->newsrc);
	  ng->newsrc = wk;
	  pt = wk;
	} else {
	  free(ng->newsrc);
	  ng->newsrc = 0;
	  return;
	}
	break;
      }
      /* 開始は未読で終了は存在しない場合（異常）は捨てる */
      if ( ng->read < st && st <= ng->last && ng->last < ed) {
	if ( strlen(pt) != 0 ) {
	  wk = malloc(strlen(pt)+1);
	  strcpy(wk,pt);
	  free(ng->newsrc);
	  ng->newsrc = wk;
	  pt = wk;
	} else {
	  free(ng->newsrc);
	  ng->newsrc = 0;
	  return;
	}
	break;
      }

      /* 正常な範囲 */
      if (ng->read < st && st <= ng->last &&
	  ng->read < ed && ed <= ng->last) {
	ng->numb -= ( ed - st + 1);
      }
      break;

    default:			/* 999 */
      if ( *pt == ',' )
	pt++;

      if ( st <= ng->read || st < ng->first ){
	if ( strlen(pt) != 0 ) {
	  wk = malloc(strlen(pt)+1);
	  strcpy(wk,pt);
	  free(ng->newsrc);
	  ng->newsrc = wk;
	  pt = wk;
	} else {
	  free(ng->newsrc);
	  ng->newsrc = 0;
	  return;
	}
	break;
      }
      if (ng->read < st && st <= ng->last)
	ng->numb--;
      break;
    }
  }
}

/*
 * 基準以上の記事があれば捨てる
 */

static void
newsrc_kill_article()
{
  register newsgroup_t *ng;
  
  for ( ng = newsrc; ng != 0 ; ng=ng->next ) {
    /* 未購読は触らない */
    if ( (ng->flag & UNSUBSCRIBE) == UNSUBSCRIBE )
      continue;

    if ( ng->numb <= gn.article_limit )
      continue;    /* 基準以下 */

    if ( ng->read >= ng->last - gn.article_leave )
      continue;    /* 残す数より後まで読んでいる */

    ng->read = ng->last - gn.article_leave;
    ng->numb = gn.article_leave;
    change = 1;
  }
}

/*
 * signature の付加
 */

void
add_signature(fp)
FILE *fp;
{
  FILE *rp;
  char resp[NNTP_BUFSIZE+1];
  char resp2[NNTP_BUFSIZE+1];
  register char *pt;
  int signature_code = -1;
  
  if ( ( rp = fopen(gn.signature,"r") ) == NULL )
    return;		/* ファイルなし */
  
  /* .signature は gn.file_kanji_code に関わらず JIS の場合がある */
  while ( fgets(resp,NNTP_BUFSIZE,rp) != NULL ){
    for ( pt = resp; *pt != '\n' && *pt != 0 ; pt++) {
      if ( ' ' <= *pt && *pt < DEL )
	continue;
      if ( *pt == ESC ) {
	if ((pt[1] == '$' && (pt[2] == '@' || pt[2] == 'B' )) ||
	    (pt[1] == '(' && (pt[2] == 'B' || pt[2] == 'H' || pt[2] == 'J' ))){
	  signature_code = JIS;
	  break;
	}
      }
      /* 本当は、ここで EUC or SJIS の判定がしたい */
    }
    if ( signature_code != -1 )
      break;
  }
  if ( signature_code == -1 )
    signature_code = gn.file_kanji_code;
  fclose(rp);
  
  fprintf(fp,"---\n");
  
  if ( ( rp = fopen(gn.signature,"r") ) != NULL ){
    while ( fgets(resp,NNTP_BUFSIZE,rp) != NULL ){
      kanji_convert(signature_code,resp,gn.process_kanji_code,resp2);
      fprintf(fp,resp2);
    }
    fclose(rp);
  }
}

/*
 * 指定した名前のニュースグループキューを返す
 */
newsgroup_t *
search_group(name)
register char *name;
{
  register newsgroup_t *ng;
  
  for ( ng = newsrc ; ng != 0 ; ng=ng->next) {
    if ( ng->name[0] == name[0] &&  strcmp(ng->name,name) == 0)
      return(ng);
  }
  return(0);
}

/*
 * 記事番号のファイル名
 */
char *
article_filename(numb)
long numb;
{
  static char filename[20];
  
  if ( gn.dosfs == ON && numb > 99999999L )
    sprintf(filename,"%ld.%03ld",numb/1000, numb%1000);
  else
    sprintf(filename,"%ld",numb);
  
  return(filename);
}

/*
 * ng の num 番の記事を既読とする
 */

void
set_readed(ng,num)
newsgroup_t *ng;
long num;
{
  register char *pt;
  register char *readed,*readp;
  char newsrc_line[NEWSRC_LEN];
  long old_readed;
  long st,ed;
  long l;
  register subject_t *ss;
  register article_t *aa;
  
  if ( (ng->flag & UNSUBSCRIBE ) != 0 )	/* 未購読 */
    return;
  
  if ( ng->read >= num )	/* すでに読んでいる */
    return;
  
  if ( ng->newsrc == 0 ){
    /* 連続 */
    if ( num == ng->read + 1 ){	/* 既読の記事の次 */
      ng->read = num;
    } else {
      sprintf(newsrc_line,"%ld",num);
      ng->newsrc = (char *)malloc(strlen(newsrc_line)+1);
      strcpy(ng->newsrc,newsrc_line);
    }
    ng->numb--;
    if (ng->numb == 0)
      ng->flag |= NOARTICLE;
    return;
  }
  /* 連続していない */
#if defined(MSDOS) && (defined(MSC) || defined(__TURBOC__))
  readed = (char *)malloc(sizeof(char)*(short)(ng->last - ng->read));
  memset(readed,0,(short)(ng->last-ng->read));
#else
  readed = (char *)malloc(sizeof(char)*(ng->last - ng->read));
  memset(readed,0,(ng->last-ng->read));
#endif
  old_readed = ng->read;
  
  /* 既読リスト作成 */
  pt = ng->newsrc;
  while (*pt != 0 ) {
    st = atol(pt);
    while( '0' <= *pt && *pt <= '9' )
      pt++;
    if ( *pt == 0 ){
      if ( (st-(old_readed+1)) >= 0 )
	readed[st-(old_readed+1)] = 1;
      break;
    }
    if ( *pt == ',' ){
      pt++;
      if ( (st-(old_readed+1)) >= 0 )
	readed[st-(old_readed+1)] = 1;
      continue;
    }
    if ( *pt == '-' ){
      pt++;
      ed = atol(pt);
      while( '0' <= *pt && *pt <= '9' )
	pt++;
      if ( *pt == ',' )
	pt++;
      for ( ; st <= ed ; st++)
	if ( (st-(old_readed+1)) >= 0 )
	  readed[st-(old_readed+1)] = 1;
    }
  }
  /* 既読リストから ng->read の更新 */
  if ( readed[num-(old_readed+1)] == 0 ) {
    readed[num-(old_readed+1)] = 1;
    ng->numb--;
    readp = readed;
    for ( l = ng->read +1;
	 l <= ng->last && *readp == 1; l++,readp++) {
      ng->read = l;
    }
    free(ng->newsrc);
    ng->newsrc = 0;
    if ( l <= ng->last ) {
      /* GNUS format の newsrc 行の作成 */
      pt = newsrc_line;
      *pt = 0;
      for ( ; l <= ng->last ; l++ ) {
	if ( readed[l-(old_readed+1)] != 1 )
	  continue;
	st = ed = l;
	for ( ; l <= ng->last ; l++ ) {
	  if ( readed[l-(old_readed+1)] != 1 )
	    break;
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
	ng->newsrc = (char *)
	  malloc(strlen(newsrc_line)+1);
	strcpy(ng->newsrc,newsrc_line);
      }
    } else {
      ng->flag |= NOARTICLE;
    }
  }
  free(readed);
  
  if ( ng->subj != 0 ) {	/* サブジェクトキューがある場合 */
    for ( ss = ng->subj ; ss != 0 ;ss = ss->next) {
      for (aa = ss->art ; aa != 0 ;aa = aa->next) {
	if ( aa->numb != num )
	  continue;
	if ( aa->flag != 0 )	/* すでに読んでいる */
	  break;
	aa->flag = 1;
	ss->numb--;
	break ;
      }
      if ( aa != 0 && aa->numb == num )
	break;
    }
  }
}
/*
 * ng の num 番の記事を未読とする
 */

void
set_unreaded(ng,num)
newsgroup_t *ng;
long num;
{
  register char *readed;
  long l;
  char newsrc_line[NEWSRC_LEN];
  register char *pt;
  long st,ed,base;
  char *newsrc_save = 0;
  
  if ( (ng->flag & UNSUBSCRIBE ) != 0 )	/* 未購読 */
    return;
  
  base = ng->first;
  
#if defined(MSDOS) && (defined(MSC) || defined(__TURBOC__))
  readed = (char *)malloc(sizeof(char)*(short)(ng->last-ng->first+1));
  memset(readed,0,(short)(ng->last-ng->first+1));
#else
  readed = (char *)malloc(sizeof(char)*(ng->last-ng->first+1));
  memset(readed,0,(ng->last-ng->first+1));
#endif
  
  for ( l = ng->first; l <= ng->read ; l++)
    readed[l-base] = 1;
  
  /* newsrc 行から既読リスト作成 */
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
  
  /* num を未読に */
  readed[num - base] = 0;
  
  /* 最終連続既読記事番号 */
  st = ng->first - 1;
  for ( l = ng->first; l <= ng->last && readed[l-base] == 1; l++)
    st = l;
  
  if ( ng->read != st ) {
    ng->read = st;
    change = 1;
  }
  
  /* GNUS format の newsrc 行の作成 */
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
    ng->newsrc = (char *)malloc(strlen(newsrc_line)+1);
    strcpy(ng->newsrc,newsrc_line);
  } else {
    if (newsrc_save != 0)
      change = 1;
  }
  
  if ( ng->numb <= 0 )	/* 未読記事なし */
    ng->flag |= NOARTICLE;
  else
    ng->flag &= (~NOARTICLE);
  
  
  if (newsrc_save)
    free(newsrc_save);
  free(readed);
}
#if !defined(MSDOS) && !defined(OS2)
#define LOCKNAME ".gn-lck-"
#define PIDSIZE  10
static char *lck;

static void
UnLinkl()
{
  if (unlink(lck) != 0){
    mc_puts("すでに存在するロックファイルを消去できません \n");
    perror(lck);
    exit(1);
  }
}

static int
chk_lck()
{
  int fd, gpid;
  char spid[PIDSIZE+1];
  
  if ((fd = open(lck, O_RDONLY)) == -1){
    if (errno == ENOENT)
      return 1 ;		/* ロックファイルはない */
    UnLinkl();
    return 1;			/* ロックファイルの消去には成功 */
  }
  if (read(fd, spid, PIDSIZE+1) != (PIDSIZE+1)){
    close(fd);			/* フォーマットの異なるロックファイル */
    UnLinkl();
    return 1;			/* ロックファイルの消去には成功 */
  }
  close(fd);
  gpid = atoi(spid);
  if (kill(gpid, 0) == 0)
    return 0;       /* gn はすでに起動されている */
  UnLinkl();		/* 起動されていないのでロックファイル消去 */
  return 1;
}

int
lock_gn(server)
char *server;
{
  int lckfd;
  char spid[PIDSIZE+1], *ltmp;
  
  ltmp = malloc(strlen(server)+strlen(LOCKNAME)+1);
  strcpy(ltmp, LOCKNAME);
  strcat(ltmp, server);
  if ( (lck = expand_filename(ltmp, gn.home)) == 0 ) {
    mc_printf("ファイル名の展開に失敗しました(%s)\n",ltmp);
    exit(1);
  }
  free(ltmp);
  
  if (!chk_lck())
    return 1;
  if ((lckfd = creat(lck, 0444)) < 0){
    mc_puts("ロックファイルが作れません \n");
    perror(lck);
    exit(1);
  }
  sprintf(spid, "%*d\n", PIDSIZE, getpid());	/* HDB format */
  if (write(lckfd, spid, PIDSIZE+1) != (PIDSIZE+1)){
    mc_puts("ロックファイル書き込みエラー\n");
    perror(lck);
    exit(1);
  }
  close(lckfd);
  return 0;
}

void
Exit(n)
int n;
{
  if (unlink(lck) != 0){
    if (errno != ENOENT){
      mc_puts("ロックファイルを消去できません \n");
      perror(lck);
    }
  }
  exit(n);
}
#endif	/* MSDOS */

int
STRNCASECMP(str1,str2,len)
unsigned char *str1,*str2;
int len;
{
  unsigned char c1,c2;

  for ( ; len > 0 ; len--,str1++,str2++) {

    if (*str1 == *str2)
      continue;

    if ( 'A' <= *str1 && *str1 <= 'Z' )
      c1 = *str1 - 'A' + 'a';
    else
      c1 = *str1;

    if ( 'A' <= *str2 && *str2 <= 'Z' )
      c2 = *str2 - 'A' + 'a';
    else
      c2 = *str2;

    if ( c1 == c2 )
      continue;

    return(-1);
  }
  return(0);
}

#ifdef	WINDOWS

BOOL __export CALLBACK
About(hDlg, message, wParam, lParam)
HWND hDlg;
unsigned message;
WORD wParam;
LONG lParam;
{
  switch (message) {
  case WM_INITDIALOG:		/* 初期化 */
    SetDlgItemText(hDlg, IDC_VERSION, (LPSTR)gn_version);
    SetDlgItemText(hDlg, IDC_DATE, (LPSTR)gn_date);
    return(TRUE);

  case WM_COMMAND:
    if (wParam == IDOK ||	/* [OK]ボタン */
	wParam == IDCANCEL) {	/* [閉じる] */
      EndDialog(hDlg, TRUE);
      return(TRUE);
    }
    break;
  }
  return(FALSE);
}

/*
 * 砂時計カーソルにする
 */
void
set_hourglass()
{
  if ( hSaveCursor == NULL )
    hSaveCursor = SetCursor(hHourGlass);
  else
    SetCursor(hHourGlass);
  UpdateWindow(hWnd);
}
/*
 * 砂時計カーソルから戻す
 */
void
reset_cursor()
{
  if ( hSaveCursor == NULL )
    return;
  SetCursor(hSaveCursor);	/* 元のカーソル */
  UpdateWindow (hWnd);
}
#endif	/* WINDOWS */

#ifdef DOSFS

#if defined( OS2 )
GetDisk()
{
#ifdef OS2_16BIT
    USHORT DriveNumber;
    USHORT rc;
#else
    ULONG  DriveNumber;
    APIRET rc;
#endif

    ULONG LogicalDriveMap;

#ifdef OS2_16BIT
    rc = DosQCurDisk(&DriveNumber, &LogicalDriveMap);
#else
    rc = DosQueryCurrentDisk(&DriveNumber, &LogicalDriveMap);
#endif
    return (int)DriveNumber-1;
}

SetDisk(disk)
int disk;
{
#ifdef OS2_16BIT
    USHORT DriveNumber = disk+1;
    USHORT rc;
#else
    ULONG  DriveNumber = disk+1L;
    APIRET rc;
#endif

    if (disk == GetDisk()) return 0;

#ifdef OS2_16BIT
    rc = DosSelectDisk(DriveNumber);
#else
    rc = DosSetDefaultDisk(DriveNumber);
#endif

    return (int)rc;
}

VIOMODEINFO *GetScreenInfo()
{
  static VIOMODEINFO ModeData;
  static int initialized = 0;
  
  if (!initialized)
    {
      initialized = 1;

      ModeData.cb = sizeof(ModeData);
      VioGetMode(&ModeData, 0);
    }
  return &ModeData;
}

#else 	/* OS2 */
#if defined (HUMAN68K)
#include <unistd.h>
GetDisk()
{
  return (getdrive() - 1);
}

SetDisk(disk)
int disk;
{
  int ordisk;
  if (disk == (ordisk=GetDisk())) return 0;
  
  chdrive(disk + 1);
  if (ordisk == GetDisk()) return 1;	/* Can't change disk drive */
  return 0;
}
#else	/* HUMAN68K */
#if defined(MSC)
GetDisk()
{
  return(_getdrive() -1);
}

SetDisk(disk)
int disk;
{
  if ( _chdrive(disk+1) == 0 )
    return(0);
  return(1);		/* Can't change disk drive */
}
#else	/* MSC */
#include <dos.h>
GetDisk()
{
  union REGS reg;

  reg.h.ah = 0x19;
  return(intdos(&reg, &reg) & 0x00ff);
}

SetDisk(disk)
int disk;
{
  union REGS reg;
  int ordisk;
  
  if (disk == (ordisk=GetDisk())) return 0;
  
  reg.h.ah = 0x0e;
  reg.h.dl = disk;
  intdos(&reg, &reg);
  if (ordisk == GetDisk()) return 1;	/* Can't change disk drive */
  return 0;
}
#endif	/* MSC */


#endif	/* HUMAN68K */
#endif	/* OS2 */
#endif	/* MSDOS */
