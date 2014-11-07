/*
 * @(#)configur.c	1.40 Sep.10,1997
 *
 * 著作権は放棄しません。ただし、営利目的以外の使用／配布に制限は設けません。
 * Copyright (C) yamasita@omronsoft.co.jp
 */

#ifdef WINDOWS
#	include <windows.h>
#endif

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>

#ifdef	HAS_STRING_H
#	include	<string.h>
#endif
#ifdef	HAS_STDLIB_H
#	include	<stdlib.h>
#endif
#ifdef	HAS_IO_H
#	include	<io.h>
#endif
#ifdef	HAS_DIRECT_H
#	include	<direct.h>
#endif
#ifdef	HAS_CONIO_H
#	include <conio.h>
#endif

#ifdef	UNIX
extern char *strchr();
extern int unlink();
#endif /* UNIX */


#ifdef OS2
#       define INCL_DOSFILEMGR     /* for DosQCurDisk and DosSelectDisk */
#       include <os2.h>
#endif

#define	CONFIGUR
#include	"nntp.h"
#include	"gn.h"

gn_t gn;			/* カスタマイズ */
int internal_kanji_code;	/* gn 内部の漢字コード */

#include	"kanji.c"		/* boom */

#define	LEN		256

#define	ANS_YES		1
#define	ANS_NO		0
#define	ANS_QUIT	-1

#define	NUMERIC		1
#define	KANJI		2
#define	DIR			4
#define	CHECKMASK	0xff
#define	READONLY	0x100

#define	CONT	0
#define	SKIP	1

#if defined(MSDOS) || defined(OS2)
#define	SLASH_STR	"\\"
#endif

int pre_check_smtpserver();

#if !defined(MSDOS) && !defined(OS2)
#define	INCLUDE_DIR	"/usr/include/"
char *header_tbl[] = {
  "string.h",
  "stdlib.h",
  "malloc.h",
  "unistd.h",
  "netdb.h",
  "sys/ioctl.h",
  "sys/termio.h",
  "sys/socket.h",
  "netinet/in.h",
  "arpa/inet.h",
  0,
};
char	headers[1024];
#endif /* MSDOS */

struct config_t {
  char *keyword;
  char value[LEN];
  int	must;
  int	check;
  int (*pre_func)();
  char *cf1;
  char *cf2;
  char *cf3;
  char *guide;
};
struct config_t config_tbl[] = {
  /* keyword		value	must	check	pre_func	cf1	cf2	cf3 */
  
  /* system */
#ifdef	WATTCP
  { "WATTCP",		"",	YES,	DIR,	0,	"CFLAGS","LIBS",	0,      "directory of Waterloo TCP" },
#endif	/* WATTCP */
#ifdef	PATHWAY
  { "DIR",		"",	YES,	DIR,	0,	"CFLAGS","LIBS",	0,      "directory of socket library" },
#endif	/* PATHWAY */
#ifdef	SLIMTCP
  { "DIR",		"",	YES,	DIR,	0,	"CFLAGS","LIBS",	0,      "directory of socket library" },
#endif	/* SLIMTCP */
#ifdef	PCTCP4
  { "DIR",		"",	YES,	DIR,	0,	"CFLAGS","LIBS",	0,      "directory of socket library" },
#endif	/* SLIMTCP */
#ifdef	LANWP
  { "DIR",		"",	YES,	DIR,	0,	"CFLAGS",	0,	0,      "directory of socket library" },
#endif	/* LANWP */
#ifdef	PCNFS
  { "EXTLIBDIR",	"",	YES,	DIR,	0,	"LIBS",		0,	0,      "directory of socket library" },
#endif	/* PCNFS */
#ifdef	ESPX
  { "ESPX",		"",	YES,	DIR,	0,	"CFLAGS","LIBS",	0,      "directory of ESPX" },
#endif	/* PCNFS */
  
  
  { "BINDIR",		"",	YES,	DIR,	0,	0,	0,	0,      "directory of binaries" },
  { "MANDIR",		"",	YES,	DIR,	0,	0,	0,	0,      "directory of manuals" },
  { "MANEXT",		"",	NO,	NO,	0,	0,	0,	0,      "extention of manuals" },
  
  { "CC",		"",	YES,	NO,	0,	0,	0,	0,      "C compiler" },
  { "OPTCFLAGS",	"",	NO,	NO,	0,	"DEFINES","CFLAGS",	0,      "optional CFLAGS" },
  { "OPTGNOBJS",	"",	NO,	NO,	0,	0,	0,	0,      "optional OBJECTS for gn" },
  { "OPTGNSPOOLOBJS",	"",	NO,	NO,	0,	0,	0,	0,      "optional OBJECTS for gnspool" },
  { "OPTLFLAGS",	"",	NO,	NO,	0,	"LFLAGS",	0,	0,      "optional link flags" },
  { "OPTLIBS",		"",	NO,	NO,	0,	"LIBS",	0,	0,      "optional libraries" },
  
  { "NEWSSPOOL",	"",	YES,	NO,	0,	0,	0,	0,      "directory of articles" },
  { "NEWSLIB",		"",	YES,	NO,	0,	0,	0,	0,      "directory of controls" },
  
  { "MAIL_KANJI_CODE",		"",	YES,	KANJI,	0,	0,	0,	0,      "kanji code of mail (JIS/EUC/SJIS)" },
  { "PROCESS_KANJI_CODE",	"",	YES,	KANJI,	0,	0,	0,	0,      "kanji code of process (JIS/EUC/SJIS)" },
  { "FILE_KANJI_CODE",		"",	YES,	KANJI,	0,	0,	0,	0,      "kanji code of file (JIS/EUC/SJIS)" },
  { "DISPLAY_KANJI_CODE",	"",	YES,	KANJI,	0,	0,	0,	0,      "kanji code of display (JIS/EUC/SJIS)" },
  
  { "LINES",		"",	YES,	NUMERIC,0,	0,	0,	0,      "lines of display" },
  { "COLUMNS",		"",	YES,	NUMERIC,0,	0,	0,	0,      "columns of display" },
  
  /* site */
  
#if !defined(MSDOS) && !defined(OS2)
  { "MAILER",		"",	YES,	NO,		0,	0,	0,	0,      "mailer" },
#endif
  { "NNTPSERVER",	"",	YES,	NO,		0,	0,	0,	0,      "hostname of your NNTP server" },
#if defined(MSDOS) || defined(OS2)
  { "SMTPSERVER",	"",	YES,	NO,		0,	0,	0,	0,      "hostname of your MAIL server" },
#else
  { "SMTPSERVER",	"",	NO,	NO,		pre_check_smtpserver,	0,	0,	0,      "hostname of your MAIL server" },
#endif
  { "DOMAINNAME",	"",	YES,	NO,		0,	0,	0,	0,      "Your domain name" },
  { "GENERICFROM",	"",	YES,	NUMERIC,	0,	0,	0,	0,      "1:username@domainname / 0:username@hostname.domainname" },
  { "ORGANIZATION",	"",	YES,	NO,		0,	0,	0,	0,      "Your organization" },
  
  /* user */
  
  { "NEWSRC",		"",	YES,	NO,		0,	0,	0,	0,      "newsrc file" },
  { "SIGNATURE",	"",	YES,	NO,		0,	0,	0,	0,      "signature file" },
  { "AUTHORCOPY",	"",	YES,	NO,		0,	0,	0,	0,      "authorcopy file" },
  { "SAVEDIR",		"",	YES,	NO,		0,	0,	0,	0,      "save directory" },
#if !defined(MSDOS) && !defined(OS2)
  { "TMPDIR",		"",	YES,	DIR,		0,	0,	0,	0,      "temporary directory" },
#else
  { "TMPDIR",		"",	NO,	NO,		0,	0,	0,	0,      "temporary directory" },
#endif
#if ! defined(WINDOWS) && ! defined(QUICKWIN)
  { "PAGER",			"",	YES,	NO,		0,	0,	0,	0,      "pager" },
  { "RETURN_AFTER_PAGER",	"",	YES,	NUMERIC,	0,	0,	0,	0,      "if your pager is not stop at last page, you must input 1" },
#endif	/* ! WINDOWS && !QUICKWIN */
  { "EDITOR",			"",	YES,	NO,		0,	0,	0,	0,      "editor" },
  
  /* readonly */
  { "DEFINES",	"",	YES,	READONLY,	0,	0,	0,	0,	"" },
  { "CFLAGS",	"",	YES,	READONLY,	0,	0,	0,	0,	"" },
  { "LFLAGS",	"",	YES,	READONLY,	0,	0,	0,	0,	"" },
  { "LIBS",	"",	YES,	READONLY,	0,	0,	0,	0,	"" },
  
  { 0,"",0,0,0 }
};


int
main(argc,argv)
int argc;
char **argv;
{
#if !defined(MSDOS) && !defined(OS2)
  extern void examin_header();
#endif /* MSDOS */
  extern int read_default();
  extern int user_input();
  extern int edit_makefile();
  extern int edit_file();
  extern void set_file_kanji_code();
  extern int save_site_def();
  
  int	read_site_def = YES;
  
  internal_kanji_code = ((unsigned char *)"あ")[0] == 0x82 ? SJIS : EUC;
  gn.file_kanji_code = internal_kanji_code;

  printf("Reading Makefile\n");
  if ( read_default("Makefile") != 0 ) {
    printf("Can't open Makefile\n");
    return(1);
  }
  
  printf("Reading site.def\n");
  if ( read_default("site.def") != 0 ) {
    read_site_def = NO;
  }
  
  if ( user_input(read_site_def,argc) != 0 )
    return(1);
  
#if !defined(MSDOS) && !defined(OS2)
  examin_header();
#endif /* MSDOS */

  if ( edit_makefile() != 0 )
    return(1);
  if ( edit_file("config.sv","config.h","config.hd") != 0 )
    return(1);
  
  set_file_kanji_code();
  
  if ( edit_file("gn.sv","gn.man","gn.md") != 0 )
    return(1);
  if ( edit_file("gnspool.sv","gnspool.man","gnspool.md") != 0 )
    return(1);
  if ( edit_file("gnloops.sv","gnloops.man","gnloops.md") != 0 )
    return(1);
  
  if ( save_site_def() != 0 )
    return(1);
  return(0);
  
}

#if !defined(MSDOS) && !defined(OS2)
void
examin_header()
{
  char **hp;
  char buf[512];
  struct stat stat_buf;
  register char *pt;

  strcpy(headers,"");
  for ( hp = &header_tbl[0]; *hp != 0 ; hp++ ) {
    strcpy(buf,INCLUDE_DIR);
    strcat(buf,*hp);
    if (stat(buf,&stat_buf) != 0 || (stat_buf.st_mode & S_IFREG) == 0 )
      continue;
    strcat(headers," -DHAS_");
    strcat(headers,*hp);
  }
  for ( pt = headers ; *pt != 0 ; pt++ ) {
    if ( 'a' <= *pt && *pt <= 'z' ) {
      *pt = *pt - 'a' + 'A';
      continue;
    }
    if ( *pt == '.' || *pt == '/' )
      *pt = '_';
  }
}
#endif /* MSDOS */

/*
 * 指定されたファイルが有れば、そのファイルから configure 情報を読み込む
 */
int
read_default(default_file)
char *default_file;
{
  extern struct config_t *get_key_val();
  
  char buf[LEN];
  char *ptv;
  FILE *fp;
  struct config_t *ctp;
  
  if ( (fp = fopen(default_file,"r")) == NULL )
    return(-1);
  
  while ( fgets(buf,LEN,fp) != NULL ) {
    if ( (ctp = get_key_val(buf,&ptv)) != (struct config_t *)0 )
      strcpy(ctp->value,ptv);
  }
  
  fclose(fp);
  return(0);
}

/*
 * buf で与えられた行が、
 * keyword = value という形式をしているなら、
 * keyword が 予約語かどうか調べ、
 * 予約語であるなら
 * value のポインタを ptv にストアして
 * config_t のポインタを返す
 */
struct config_t *
get_key_val(buf,ptv)
register char *buf,**ptv;
{
  register char *pt,*ptk;
  register struct config_t *ctp;
  
  if ( (pt = strchr(buf,'\n')) != NULL )
    *pt = 0;
  
  pt = buf;
  
  while ( *pt == ' ' || *pt == '\t' )
    pt++;				/* 先行するホワイトスペースを無視 */
  if ( *pt == 0 )			/* ホワイトスペースのみ */
    return((struct config_t *)0);
  if ( *pt == '#' )
    return((struct config_t *)0);
  
  ptk = pt;				/* keyword のはじまり */
  
  while ( *pt != 0 && *pt != ' ' && *pt != '\t' )
    pt++;				/* keyword を skip */
  if ( *pt == 0 )			/* = 以降がない */
    return((struct config_t *)0);
  
  *pt++ = 0;
  
  while ( *pt == ' ' || *pt == '\t' )
    pt++;				/* keyword と = の間のホワイトスペース */
  
  if ( *pt != '=' )		/* = がない */
    return((struct config_t *)0);
  
  pt++;
  while ( *pt == ' ' || *pt == '\t' )
    pt++;				/* = と value の間のホワイトスペース */
  
  *ptv = pt;				/* value のはじまり */
  
  for ( ctp = &config_tbl[0] ; ctp->keyword != 0 ; ctp++ ) {
    if (strcmp(ctp->keyword,ptk) == 0 ) {
      return(ctp);
    }
  }
  return((struct config_t *)0);
}

/*
 * ユーザによる入力
 */
int
user_input(read_site_def,flag)
int	read_site_def,flag;
{
  extern int conf_ct();
  extern int check_ct();
  extern int check_ct1();
  extern void ct_error_message();
  extern struct config_t *ct_lookup();
  
  char key_buf[LEN];
  register char *pt;
  register struct config_t *ctp,*ctcf;
  
  if ( read_site_def == YES ) {	/* site.def を読み込んだ場合 */
    if ( check_ct() == 0 ) {	/* 全部のキーワードに値が入っている */
      switch (conf_ct(flag)) {
      case ANS_YES:
	return(0);
      case ANS_QUIT:
	return(-1);
      }
    }
  }
  
  while(1) {
    for ( ctp = &config_tbl[0] ; ctp->keyword != 0 ; ctp++ ) {
      if ( ctp->check == READONLY )
	continue;

      while (1) {
	if ( ctp->pre_func != 0 ) {
	  if ( ctp->pre_func(ctp) == SKIP )
	    break;
	} else {
	  printf("%s = %s?\n",ctp->keyword,ctp->guide);

	  if ( ctp->cf1 != 0 ) {
	    printf("which will be used as\n");
	    ctcf = ct_lookup(ctp->cf1);
	    printf("	%s = %s\n",ctp->cf1,ctcf->value);
	    if ( ctp->cf2 != 0 ) {
	      ctcf = ct_lookup(ctp->cf2);
	      printf("	%s = %s\n",ctp->cf2,ctcf->value);
	      if ( ctp->cf3 != 0 ) {
		ctcf = ct_lookup(ctp->cf3);
		printf("	%s = %s\n",ctp->cf3,ctcf->value);
	      }
	    }
	  }

	  if ( strlen(ctp->value) != 0 &&
	      check_ct1(ctp,ctp->value) == 0) {
	    printf("(default:%s)\n",ctp->value);
	  }
	}

	while ( fgets(key_buf,LEN,stdin) == NULL )
	  clearerr(stdin);
	if ( (pt = strchr(key_buf,'\n')) != NULL )
	  *pt = 0;

	pt = key_buf;
	if ( *pt == TAB && pt[1] == 0 ) {
	  pt++;
	} else {
	  while ( *pt == ' ' || *pt == '\t' )
	    pt++;		/* 先行するホワイトスペースを無視 */
					
	  if ( strlen(pt) == 0 ) { /* CR のみ */
	    if ( ctp->must == NO ) /* 必須でない */
	      break;
	    if ( strlen(ctp->value) != 0 ) { /* デフォルト値有 */
	      if ( check_ct1(ctp,ctp->value) != 0 ) {
		ct_error_message(ctp,ctp->value);
		putchar(BEL);
		continue;
	      }
	      break;
	    }
	    putchar(BEL);
	    continue;
	  }
	}
	if ( check_ct1(ctp,pt) != 0 ) {
	  ct_error_message(ctp,pt);
	  putchar(BEL);
	  continue;
	}
	strcpy(ctp->value,pt);	/* 新しい値 */
	break;
      }
    }
    switch (conf_ct(0)) {
    case ANS_YES:
      return(0);
    case ANS_QUIT:
      return(-1);
    }
  }
}

/*
 * config_tbl のチェック
 */
int
check_ct()
{
  extern int check_ct1();
  register struct config_t *ctp;
  
  for ( ctp = &config_tbl[0] ; ctp->keyword != 0 ; ctp++ ) {
    if ( ctp->check == READONLY )
      continue;
    if ( check_ct1(ctp,ctp->value) != 0 ) {
      ctp->value[0] = 0;
      return(-1);
    }
  }
  return(0);
}
/*
 * value をチェックする
 */
int
check_ct1(ctp,value)
register struct config_t *ctp;
char *value;
{
  struct stat stat_buf;
  char *pt;
#if defined(DOSFS)
  extern int GetDisk(),SetDisk();
  int  disk = -1;
#endif
  
  if ( ctp->must == NO )		/* 必須でない */
    return(0);
  if ( strlen(value) == 0 )	/* 長さ＝０ */
    return(-1);
  
  switch (ctp->check & CHECKMASK) {
  case NUMERIC:			/* 数字のみ */
    for ( pt = value ; *pt != 0 ; pt++) {
      if ( *pt < '0' || '9' < *pt )
	return(-1);
    }
    break;

  case KANJI:
    if (strcmp(value,"JIS") != 0 &&
	strcmp(value,"EUC") != 0 &&
	strcmp(value,"SJIS") != 0 ) {
      return(-1);
    }
    break;

  case DIR:
#ifdef	DOSFS
    if ( (pt = strchr(value, ':')) != NULL){
      /* tmpdir にディスクが書かれている */
      pt++;
      disk = GetDisk();
      if (SetDisk(toupper(value[0])-'A')){
	printf("Can't use disk : %c\n", value[0]);
	return(-1);
      }
      SetDisk(disk);
    }
    if (disk == -1)
      pt = value;
    if (strcmp(pt, SLASH_STR) != 0 && strcmp(pt, "") !=0){
      if (stat(value,&stat_buf) != 0 || (stat_buf.st_mode & S_IFDIR) == 0 )
	return(-1);
    } else if (strcmp(pt, SLASH_STR) == 0){
      *pt = 0;
    }
#else  /* MSDOS || OS2 */
    if (stat(value,&stat_buf) != 0 || (stat_buf.st_mode & S_IFDIR) == 0 )
      return(-1);
#endif /* MSDOS || OS2 */
    break;
  }
  return(0);
}
void
ct_error_message(ctp,value)
struct config_t *ctp;
char *value;
{
  if ( *value == 0 ) {
    printf("You must set value\n");
    return;
  }
  
  switch(ctp->check) {
  case NUMERIC:			/* 数字のみ */
    printf("Plese input numerical character\n");
    break;
  case KANJI:
    printf("Plese input one of JIS,EUC,SJIS\n");
    break;
  case DIR:
    printf("%s does not exist\n",value);
    break;
  }
  value[0] = 0;
}
/*
 * 確認入力
 */
int
conf_ct(flag)
int flag;
{
  register struct config_t *ctp;
  char key_buf[LEN];
  
  for ( ctp = &config_tbl[0] ; ctp->keyword != 0 ; ctp++ ){
    if ( ctp->check == READONLY )
      continue;
    if ( (ctp+1)->keyword == 0 ||
	strlen(ctp->value) > 18 || strlen((ctp+1)->value) > 18 ) {
      printf("%18s = %-18s\n",ctp->keyword,ctp->value);
    } else {
      printf("%18s = %-18s %18s = %-18s\n",
	     ctp->keyword,ctp->value,(ctp+1)->keyword,(ctp+1)->value);
      ctp++;
    }
  }
  
  if ( flag >= 2 )
    return(ANS_YES);
  
  while (1){
    printf("OK? (y:Yes/n:No/q:quit)\n");
    while ( fgets(key_buf,LEN,stdin) == NULL )
      clearerr(stdin);
    if ( key_buf[0] == 'Y' || key_buf[0] == 'y' )
      return(ANS_YES);
    if ( key_buf[0] == 'N' || key_buf[0] == 'n' )
      return(ANS_NO);
    if ( key_buf[0] == 'Q' || key_buf[0] == 'q' )
      return(ANS_QUIT);
    putchar(BEL);
  }
}

/*
 * keyword が value のテーブルを探す
 */
struct config_t *
ct_lookup(value)
char *value;
{
  register struct config_t *ctp;
  
  for ( ctp = &config_tbl[0] ; ctp->keyword != 0 ; ctp++ ) {
    if (strcmp(ctp->keyword,value) == 0 ) {
      return(ctp);
    }
  }
  return((struct config_t *)0);
}

#if !defined(MSDOS) && !defined(OS2)
/*
 * SMTPSERVER 指定前のチェック
 */
int
pre_check_smtpserver(ctp)
struct config_t *ctp;	/* SMTPSERVER */
{
  register struct config_t *ct;
  
  ct = ct_lookup("MAILER");
  
  if ( strcmp(ct->value,"SMTP") != 0 ) {
    ctp->value[0] = 0;
    ctp->must = NO;
    return(SKIP);
  }
  
  printf("%s = %s?\n",ctp->keyword,ctp->guide);
  if (strlen(ctp->value) != 0 &&
      check_ct1(ctp,ctp->value) == 0) {
    printf("(default:%s)\n",ctp->value);
  }
  
  ctp->must = YES;
  return(CONT);
}
#endif	/* MSDOS */

/****************************************************************************
 * Makefile.sv が有れば、消去し
 * Makefile を Makefile.sv に rename し
 * カスタマイズされた Makefile を作成する
 */
int
edit_makefile()
{
  char buf[LEN],line[LEN];
  FILE *fp,*fps;
  register struct config_t *ctp;
  char *ptv;
  
  if ( access("Makefile.sv",0) == 0 ) {
    printf("remove Makefile.sv\n");
    if ( unlink("Makefile.sv") != 0 ) {
      printf("can't remove Makefile.sv\n");
      return(-1);
    }
  }
  
  printf("rename Makefile to Makefile.sv\n");
#ifdef	NO_RENAME
  if (link("Makefile","Makefile.sv") != 0 ||
      unlink("Makefile") != 0 ) {
    printf("can't rename Makefile to Makefile.sv\n");
    return(-1);
  }
#else
  if ( rename("Makefile","Makefile.sv") != 0 ){
    printf("can't rename Makefile to Makefile.sv\n");
    return(-1);
  }
#endif
  
  printf("creating Makefile\n");
  if ( (fp = fopen("Makefile","w")) == NULL ) {
    printf("can't create Makefile\n");
    return(-1);
  }
  
  if ( (fps = fopen("Makefile.sv","r")) == NULL ) {
    printf("can't read Makefile.sv\n");
    return(-1);
  }
  
  while ( fgets(buf,LEN,fps) != NULL ) {
#if !defined(MSDOS) && !defined(OS2)
    if ( strncmp(buf,"HEADER_DEFINES	=",16) == 0 ) {
      fprintf(fp,"HEADER_DEFINES	=%s\n",headers);
      continue;
    }
#endif /* MSDOS */
    strcpy(line,buf);
    if ( (ctp = get_key_val(buf,&ptv)) != (struct config_t *)0 )
      fprintf(fp,"%s	= %s\n",ctp->keyword,ctp->value);
    else
      fprintf(fp,"%s",line);
  }
  
  if ( fclose(fp) == EOF ) {
    printf("can't Create Makefile\n");
    return(-1);
  }
  fclose(fps);
  return(0);
}

/*
 *
 */
void
set_file_kanji_code()
{
  register struct config_t *ctp;
  
  for ( ctp = &config_tbl[0] ; ctp->keyword != 0 ; ctp++ ) {
    if (strcmp(ctp->keyword,"FILE_KANJI_CODE") == 0 )
      break;
  }
  
  if ( strcmp(ctp->value,"JIS") == 0 )
    gn.file_kanji_code = JIS;
  else if ( strcmp(ctp->value,"SJIS") == 0 )
    gn.file_kanji_code = SJIS;
  else
    gn.file_kanji_code = EUC;
}
/*
 * backup_name が有れば、消去し
 * target_name を backup_name に rename し
 * source_name からカスタマイズされた target_name を作成する
 */
int
edit_file(backup_name,target_name,source_name)
char *backup_name;
char *target_name;
char *source_name;
{
  extern int subst_keyword();
  
  FILE *fp,*fps;
  
  if ( access(backup_name,0) == 0 ) {
    printf("remove %s\n",backup_name);
    if ( unlink(backup_name) != 0 ) {
      printf("can't remove %s\n",backup_name);
      return(-1);
    }
  }
  
  if ( access(target_name,0) == 0 ) {
    printf("rename %s to %s\n",target_name,backup_name);
#ifdef	NO_RENAME
    if (link(target_name,backup_name) != 0 ||
	unlink(target_name) != 0 ) {
      printf("can't rename %s to %s\n",target_name,backup_name);
      return(-1);
    }
#else
    if ( rename(target_name,backup_name) != 0 ){
      printf("can't rename %s to %s\n",target_name,backup_name);
      return(-1);
    }
#endif
  }
  
  
  printf("creating %s\n",target_name);
  if ( (fp = fopen(target_name,"w")) == NULL ) {
    printf("can't create %s\n",target_name);
    return(-1);
  }
  
  if ( (fps = fopen(source_name,"r")) == NULL ) {
    printf("can't read %s\n",source_name);
    return(-1);
  }
  
  if ( subst_keyword(fps,fp) != 0 ) {
    printf("can't Create %s\n",target_name);
    return(-1);
  }
  
  if ( fclose(fp) == EOF ) {
    printf("can't Create %s\n",target_name);
    return(-1);
  }
  fclose(fps);
  return(0);
}
/*
 * fps のファイルの keyword を value に置き変えて、fpd に出力する
 */
int
subst_keyword(fps,fpd)
FILE *fps,*fpd;
{
  char buf[LEN],line[LEN],kw[LEN];
  register char *pt;
  char *bef_kw,*aft_kw;
  register struct config_t *ctp;
  
  while ( fgets(buf,LEN,fps) != NULL ) {
    strcpy(line,buf);
    pt = buf;

    while (1) {
      if ( (pt = strchr(pt,'k')) == NULL ) {
	kanji_convert(internal_kanji_code,line,gn.file_kanji_code,buf);
	fputs(buf,fpd);
	break;
      }
      if ( strncmp(pt,"keyword-",8) != 0 ){
	pt++;
	continue;		/* keyword-XXX でない */
      }

      bef_kw = buf;		/* bef_kw:行頭から keyword- の前まで */
      *pt = 0;
      pt += 8;
      aft_kw = pt;
      pt = kw;
      while(('a' <= *aft_kw && *aft_kw <= 'z') || *aft_kw == '_' ){
	if ( *aft_kw == '_' )
	  *pt++ = *aft_kw++;
	else
	  *pt++ = *aft_kw++ - 'a' + 'A';
      }
      *pt = 0;
      for ( ctp = &config_tbl[0] ; ctp->keyword != 0 ; ctp++ ) {
	if (strcmp(ctp->keyword,kw) == 0 ) {
	  sprintf(kw,"%s%s%s",bef_kw,ctp->value,aft_kw);
	  kanji_convert(internal_kanji_code,kw,gn.file_kanji_code,buf);
	  fputs(buf,fpd);
	  break;
	}
      }
      if ( ctp->keyword == 0 ) {
	kanji_convert(internal_kanji_code,line,gn.file_kanji_code,buf);
	fputs(buf,fpd);
      }
      break;
    }
  }
  return(0);
}
/*
 * configure 情報を site.def に保存する
 */
int
save_site_def()
{
  FILE *fp;
  register struct config_t *ctp;
  
  if ( access("site.sv",0) == 0 ) {
    printf("remove site.sv\n");
    if ( unlink("site.sv") != 0 ) {
      printf("can't remove site.sv\n");
      return(-1);
    }
  }
  
  if ( access("site.def",0) == 0 ) {
    printf("rename site.def to site.sv\n");
#ifdef	NO_RENAME
    if (link("site.def","site.sv") != 0 ||
	unlink("site.def") != 0 ) {
      printf("can't rename site.def to site.sv\n");
      return(-1);
    }
#else
    if ( rename("site.def","site.sv") != 0 ){
      printf("can't rename site.def to site.sv\n");
      return(-1);
    }
#endif
  }
  
  printf("creating site.def\n");
  if ( (fp = fopen("site.def","w")) == NULL ) {
    printf("can't create site.def\n");
    return(-1);
  }
  
  for ( ctp = &config_tbl[0] ; ctp->keyword != 0 ; ctp++ ) {
    if ( ctp->check == READONLY )
      continue;
    fprintf(fp,"%s	=	%s\n",ctp->keyword,ctp->value);
  }
  
  if ( fclose(fp) == EOF ) {
    printf("can't Create site.def\n");
    return(-1);
  }
  return(0);
}

#ifdef DOSFS
#if defined( OS2 )
GetDisk()
{
#ifdef OS2_16BIT
    USHORT DriveNumber;
    ULONG  LogicalDriveMap;
#else
    ULONG  DriveNumber;
    APIRET LogicalDriveMap;
#endif
    USHORT rc;
	
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
    ULONG  DriveNumber = disk+1;
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
#endif	/* DOSFS */
