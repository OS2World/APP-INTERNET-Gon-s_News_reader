/*
 * @(#)smtp.c	1.40 May.13,1998
 *
 * 著作権は放棄しません。ただし、営利目的以外の使用／配布に制限は設けません。
 * Copyright (C) yamasita@omronsoft.co.jp
 */

/*
 * This software is Copyright 1991 by Stan Barber. 
 *
 * Permission is hereby granted to copy, reproduce, redistribute or otherwise
 * use this software as long as: there is no monetary profit gained
 * specifically from the use or reproduction or this software, it is not
 * sold, rented, traded or otherwise marketed, and this copyright notice is
 * included prominently in any copy made. 
 *
 * The author make no claims as to the fitness or correctness of this software
 * for any use whatsoever, and it is provided as is. Any use of this software
 * is at the user's own risk. 
 *
 */

#ifdef WINDOWS
#	include <windows.h>
#endif

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <ctype.h>


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
#ifdef	HAS_DIRECT_H
#	include	<direct.h>
#endif
#ifdef	HAS_DIR_H
#	include <dir.h>
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
extern void kanji_convert();
extern void hit_return();
extern int get_1_header_file();

#if defined(WINDOWS)
extern int mc_MessageBox();
extern void set_hourglass(),reset_cursor();
#endif	/* WINDOWS */

#ifdef	MIME
extern void MIME_strHeaderEncode();
#endif

static int mail_file_with_gnspool();
#if ! defined(NO_NET)
static int smtp_server_init(),smtp_get_server();
static int smtp_put_server();
extern void smtp_close_server();
static int mail_file_by_smtp(),mail_file_by_mailer();
static int smtp_get_resp();

#if defined(WATTCP)
static socket_t	sser_rd_fp;
#	define	sser_wr_fp sser_rd_fp
#else	/* WATTCP */
static socket_t	sser_rd_fp = 0;
static socket_t	sser_wr_fp = 0;
#endif	/* WATTCP */

/*
 * smtp_server_init  Get a connection to the remote smtp server.
 *
 *	Parameters:	"machine" is the machine to connect to.
 *
 *	Returns:	-1 on error
 *			0 on success.
 *
 *	Side effects:	Connects to server.
 *			"sser_rd_fp" and "sser_wr_fp" are fp's
 *			for reading and writing to server.
 */

static int
smtp_server_init(machine,resp)
char *machine;
char *resp;
{
  extern int server_init_com();
  
  return(server_init_com(machine, &sser_rd_fp, &sser_wr_fp,
			 "smtp", "tcp", resp));
}


/*
 * smtp_put_server -- send a line of text to the server, terminating it
 * with CR and LF, as per ARPA standard.
 *
 *	Parameters:	"string" is the string to be sent to the
 *			server.
 *
 *	Returns:	Nothing.
 *
 *	Side effects:	Talks to the server.
 *
 *	Note:		This routine flushes the buffer each time
 *			it is called.  For large transmissions
 *			(i.e., posting news) don't use it.  Instead,
 *			do the fprintf's yourself, and then a final
 *			fflush.
 */

static int
smtp_put_server(string)
char *string;
{
  extern int put_server_com();
  
  int	ret;
#ifdef	WINDOWS
  char buf[128];
#endif	/* WINDOWS */
  
  if ( (ret = put_server_com(string,&sser_wr_fp)) == CONT )
    return(CONT);
  
#ifdef	WINDOWS
  reset_cursor();
  
  sprintf(buf,"SMTP サーバとの接続が切れました:%d",ret);
  mc_MessageBox(hWnd,buf,"",MB_OK);
#else	/* WINDOWS */
  mc_printf("SMTP サーバとの接続が切れました:%d\n",ret);
#endif	/* WINDOWS */
  return(ERROR);
}

/*
 * smtp_get_server -- get a line of text from the server.  Strips
 * CR's and LF's.
 *
 *	Parameters:	"string" has the buffer space for the
 *			line received.
 *			"size" is the size of the buffer.
 *
 *	Returns:	-1 on error, 0 otherwise.
 *
 *	Side effects:	Talks to server, changes contents of "string".
 */

static int
smtp_get_server(string)
char *string;
{
  extern int get_server_com();
  
  return(get_server_com(string, NNTP_BUFSIZE, &sser_rd_fp));
}


/*
 * smtp_close_server -- close the connection to the server, after sending
 *		the "quit" command.
 *
 *	Parameters:	None.
 *
 *	Returns:	Nothing.
 *
 *	Side effects:	Closes the connection with the server.
 *			You can't use "put_server" or "get_server"
 *			after this routine is called.
 */

void
smtp_close_server()
{
  extern void close_server_com();
  
  close_server_com(&sser_rd_fp,&sser_wr_fp);
}
#endif	/* ! NO_NET */

/*
 * ファイルをメイルとして送る
 */
int
mail_file(tmp_file_f, kanji_code)
char *tmp_file_f;	/* 送るファイル名 */
int kanji_code;
{
  int retcode;
  
#ifdef	WINDOWS
  set_hourglass();
#endif	/* WINDOWS */
  
  if ( prog == GN && gn.with_gnspool ) {
    retcode = mail_file_with_gnspool(tmp_file_f, kanji_code);
#ifdef	WINDOWS
    reset_cursor();
#endif /* WINDOWS */
    return(retcode);
  }
  
#if ! defined(NO_NET)
#if defined(MSDOS) || defined(OS2)
  retcode = mail_file_by_smtp(tmp_file_f, kanji_code);
#ifdef	WINDOWS
  reset_cursor();
#endif	/* WINDOWS */
  return(retcode);
#else
  if ( strcmp(gn.mailer,"SMTP") == 0 )
    return(mail_file_by_smtp(tmp_file_f, kanji_code));
  else
    return(mail_file_by_mailer(tmp_file_f, kanji_code));
#endif	/* MSDOS || OS2 */
#endif	/* NO_NET */
}
static int
mail_file_with_gnspool(tmp_file_f, kanji_code)
char *tmp_file_f;	/* 送るファイル名 */
int kanji_code;
{
  char resp[NNTP_BUFSIZE+1];
  char buf[NNTP_BUFSIZE+1];
  char buf2[NNTP_BUFSIZE+1];
  FILE *fp,*rp;
  struct stat stat_buf;
  
  strcpy(buf,gn.newsspool);
  strcat(buf,SLASH_STR);
  strcat(buf,"mail.out");
  if ( stat(buf,&stat_buf) != 0 ) {
    if ( MKDIR(buf) != 0 ) {
      mc_printf("%s の作成に失敗しました\n",buf);
      return(ERROR);
    }
  }
  strcat(buf,SLASH_STR);
  strcat(buf,"gnXXXXXX");
  mktemp(&buf[0]);
  if ( ( rp = fopen(buf,"w") ) == NULL ){
#ifdef	WINDOWS
    mc_MessageBox(hWnd,"テンポラリファイルのオープンに失敗しました",
		  buf,MB_OK);
#else  /* WINDOWS */
    perror(buf);
#endif /* WINDOWS */
    return(ERROR);
  }
  fp = fopen(tmp_file_f,"r");
  while (fgets(resp,NNTP_BUFSIZE,fp) != NULL ) {
    kanji_convert(kanji_code,resp,gn.mail_kanji_code,buf2);
    fprintf(rp, "%s", buf2);
  }
  fclose(rp);
  fclose(fp);
  return(CONT);
}

#if ! defined(NO_NET)
#define BIT_FROM	1
static int
mail_file_by_smtp(tmp_file_f,kanji_code)
char *tmp_file_f;	/* 送るファイル名 */
int kanji_code;
{
  extern int STRNCASECMP();
  extern int smtp_server_init();
  extern int smtp_get_server();
  extern void smtp_close_server();
  extern int smtp_get_resp();
  
  FILE *fp;
  char resp[NNTP_BUFSIZE+1];
  char resp2[NNTP_BUFSIZE+1];
  char to[NNTP_BUFSIZE+1];
  char next[NNTP_BUFSIZE+1];
  register char *sp,*pt,*np;
  int header=0;			/* BIT_{PATH|FROM|ORGANIZATION} */

  /* SMTP サーバの初期化 */
  if ( prog == GN || (prog == GNSPOOL && init_smtp_server == NO) ) {
    init_smtp_server = YES;
#if ! defined(WINDOWS)
    mc_puts("smtp サーバと接続しています\n");
#endif /* WINDOWS */
    
    if ( smtp_server_init(gn.smtpserver,resp) != 220 ) {
#if ! defined(WINDOWS)
      mc_puts("smtp サーバと接続できませんでした\n");
      hit_return();
#else  /* WINDOWS */
      mc_MessageBox(hWnd," smtp サーバと接続できませんでした ","mail",MB_OK);
#endif /* WINDOWS */
      return(ERROR);
    }
    if  ( resp[3] == '-' ) {
      if ( smtp_get_resp(220) )
	return(ERROR);
    }

    sprintf(resp,"helo %s",gn.hostname);
    if ( smtp_put_server(resp) != CONT )
      return(ERROR);
    if ( smtp_get_resp(250) )
      return(ERROR);
  }
  
  sprintf( resp, "mail from: <%s@%s>", gn.usrname,gn.domainname);
  if ( smtp_put_server( resp ) != CONT )
    return(ERROR);
  if ( smtp_get_resp(250) )
    return(ERROR);



  /* 宛先を探す */
  fp = fopen(tmp_file_f,"r");

  resp[0] = 0;
  get_1_header_file(resp2,resp,fp);

  while (1) {
    get_1_header_file(resp2,resp,fp);
    if (resp2[0] == 0 )
      break;

    if (STRNCASECMP(resp2,"To: ",4) != 0 &&
	STRNCASECMP(resp2,"Cc: ",4) != 0 &&
	STRNCASECMP(resp2,"Bcc: ",5) != 0 ) {
      continue;
    }

    np = strchr(resp2,' ');
    np++;
    while ( *np != 0 ) {
      pt = np;
      while (*pt == ' ' || *pt == '\t' || *pt == ',' || *pt == '\n' )
	pt++;

      /* 宛先を一つだけ抽出 */
      for ( np = pt; *np != ',' && *np != '\n' && *np != 0 ; np++)
	;
      if ( *np == 0 )
	np[1] = 0;
      *np++ = 0;

      if ( strlen(pt) == 0 )
	continue;

      /*  To: Yasunari Gon Yamasita <yamasita@omronsoft.co.jp> */
      if ( (sp = strchr(pt,'<')) != NULL ) {
	pt = &sp[1];
	if ( (sp = strchr(pt,'>')) == NULL ){ 	/* > が閉じていない */
#if ! defined(WINDOWS)
	  mc_puts("\n%s: 宛先が正常ではありません \n\007",pt);
	  hit_return();
#else  /* WINDOWS */
	  mc_MessageBox(hWnd," 宛先が正常ではありません ","mail",MB_OK);
#endif /* WINDOWS */
	  return(ERROR);
	}
	*sp = 0;

	if (strchr(pt,' ') != NULL ) { /* <> 内にホワイトスペースがある */
#if ! defined(WINDOWS)
	  mc_puts("\n%s: 宛先が正常ではありません \n\007",pt);
	  hit_return();
#else  /* WINDOWS */
	  mc_MessageBox(hWnd," 宛先が正常ではありません ","mail",MB_OK);
#endif /* WINDOWS */
	  return(ERROR);
	}
	sprintf( to, "rcpt to: <%s>", pt);
	if ( smtp_put_server( to ) != CONT )
	  return(ERROR);
	if ( smtp_get_resp(250) )
	  return(ERROR);
	continue;
      }

      /* To: yamasita@omronsoft.co.jp (Yasunari Gon Yamasita) */
      if ( (sp = strchr(pt,' ')) != NULL )
	*sp = 0;
      if ( (sp = strchr(pt,'\t')) != NULL )
	*sp = 0;

      sprintf( to, "rcpt to: <%s>", pt);
      if ( smtp_put_server( to ) != CONT )
	return(ERROR);
      if ( smtp_get_resp(250) )
	return(ERROR);
    }
  }
  fclose(fp);

  if ( smtp_put_server("data") != CONT )
    return(ERROR);
  if ( smtp_get_resp(354) )
    return(ERROR);
  
  /* ヘッダ部分 */
  if ( gn.substitute_header == ON ) {
    if ( gn.genericfrom != 0 ) {
      sprintf(resp, "From: %s@%s (%s)",
	      gn.usrname, gn.domainname, gn.fullname);
    } else {
      sprintf(resp, "From: %s@%s.%s (%s)",
	      gn.usrname, gn.hostname, gn.domainname, gn.fullname);
    }
#ifdef	MIME
    if ( (gn.mime_head & MIME_ENCODE) != 0 ) {
      kanji_convert(internal_kanji_code,resp,JIS,resp2);
      MIME_strHeaderEncode(resp2,resp,sizeof(resp));
    } else
#endif
      {
	kanji_convert(internal_kanji_code,resp,gn.mail_kanji_code,resp2);
	strcpy(resp,resp2);
      }
    
    if ( smtp_put_server( resp ) != CONT )
      return(ERROR);
  }

  fp = fopen(tmp_file_f,"r");
  get_1_header_file(resp,next,fp);

  while (1) {
    get_1_header_file(resp,next,fp);

    if (resp[0] == 0 )
      break;

    if ( gn.substitute_header == ON ) {
      if (STRNCASECMP(resp,"From: ",6) == 0 ||
	  STRNCASECMP(resp,"Bcc: ",5) == 0 ) {
	if (next[0] == 0 )
	  break;
	continue;
      }
    }else {
      if (STRNCASECMP(resp,"Bcc: ",5) == 0 ) {
	if (next[0] == 0 )
	  break;
	continue;
      }
      if (STRNCASECMP(resp,"From: ",6) == 0 )
	header |= BIT_FROM;
    }

#ifdef	MIME
    if ( (gn.mime_head & MIME_ENCODE) != 0 ) {
      kanji_convert(kanji_code,resp,JIS,resp2);
      MIME_strHeaderEncode(resp2,resp,sizeof(resp));
    }
#endif
    kanji_convert(kanji_code,resp,gn.mail_kanji_code,resp2);
    if ( smtp_put_server( resp2 ) != CONT )  {
      fclose(fp);
      return(ERROR);
    }
    if (next[0] == 0 )
      break;
  }

  if ( gn.substitute_header == OFF ) {
    if ( (header & BIT_FROM) == 0 ) {
      if ( gn.genericfrom != 0 ) {
	sprintf(resp, "From: %s@%s (%s)",
		gn.usrname, gn.domainname, gn.fullname);
      } else {
	sprintf(resp, "From: %s@%s.%s (%s)",
		gn.usrname, gn.hostname, gn.domainname, gn.fullname);
      }
#ifdef	MIME
      if ( (gn.mime_head & MIME_ENCODE) != 0 ) {
	kanji_convert(internal_kanji_code,resp,JIS,resp2);
	MIME_strHeaderEncode(resp2,resp,sizeof(resp));
      } else
#endif
	{
	  kanji_convert(internal_kanji_code,resp,gn.mail_kanji_code,resp2);
	  strcpy(resp,resp2);
	}

      if ( smtp_put_server( resp ) != CONT )
	return(ERROR);
    }
  }

#ifdef	MIME
  if ( (gn.mime_head & MIME_ENCODE) != 0 ) {
    if ( smtp_put_server("Mime-Version: 1.0") != CONT ) {
      fclose(fp);
      return(ERROR);
    }
    if ( smtp_put_server("Content-Type: text/plain; charset=ISO-2022-JP") != CONT )  {
      fclose(fp);
      return(ERROR);
    }
  }
#endif
  
  /* ヘッダと本文の間の空白行 */
  if ( smtp_put_server( "" ) != CONT ) {
    fclose(fp);
    return(ERROR);
  }
  
  /* 本文 */
  while (fgets(resp,NNTP_BUFSIZE,fp) != NULL ) {
    if ( (sp = strchr(resp,'\n')) != NULL )
      *sp = 0;

    kanji_convert(kanji_code,resp,gn.mail_kanji_code,resp2);
    if ( smtp_put_server( resp2 ) != CONT ) {
      fclose(fp);
      return(ERROR);
    }
  }
  
  fclose(fp);
  if ( smtp_put_server( "." ) != CONT )
    return(ERROR);
  
  if ( smtp_get_resp(250) )
    return(ERROR);

  if ( prog != GNSPOOL )
    smtp_close_server();
  return(CONT);
}
#if !defined(MSDOS) && !defined(OS2)
static int
mail_file_by_mailer(tmp_file_f,kanji_code)
char *tmp_file_f;	/* 送るファイル名 */
int kanji_code;
{
  FILE *fp,*rp;
  char resp[NNTP_BUFSIZE+1];
  char resp2[NNTP_BUFSIZE+1];
  char buf[NNTP_BUFSIZE+1];
  int x = 0;
  
  /* 送るファイル */
  if ( ( fp = fopen(tmp_file_f,"r") ) == NULL ){
    perror(tmp_file_f);
    hit_return();
    return(ERROR);
  }
  
  if ( ( rp = popen(gn.mailer,"w") ) == NULL ) {
    perror(gn.mailer);
    hit_return();
    return(ERROR);
  }
  
  /* ヘッダ部 */
  resp[0] = 0;
  get_1_header_file(buf,resp,fp);
  
  while (1) {
    if ( ++x > 20 ){
      mc_printf(".");
      fflush(stdout);
      x = 0;
    }
    if (resp[0] == 0 )
      break;
    get_1_header_file(buf,resp,fp);
    if (buf[0] == 0 )
      break;

#ifdef	MIME
    if ( (gn.mime_head & MIME_ENCODE) != 0 ) {
      kanji_convert(kanji_code,buf,JIS,resp2);
      MIME_strHeaderEncode(resp2,buf,sizeof(buf));
    }
#endif
    kanji_convert(kanji_code,buf,gn.mail_kanji_code,resp2);
    fprintf(rp, "%s\n", resp2);
  }
#ifdef	MIME
  if ( (gn.mime_head & MIME_ENCODE) != 0 ) {
    fprintf(rp,"Mime-Version: 1.0\n");
    fprintf(rp,"Content-Type: text/plain; charset=ISO-2022-JP\n");
  }
#endif
  
  /* ヘッダと本文の間の空白行 */
  fprintf(rp,"\n");
  
  /* 本文 */
  while (fgets(resp,NNTP_BUFSIZE,fp) != NULL ) {
    if ( ++x > 20 ){
      mc_printf(".");
      fflush(stdout);
      x = 0;
    }
    kanji_convert(kanji_code,resp,gn.mail_kanji_code,resp2);
    fprintf(rp, "%s", resp2);
  }
  
  pclose(rp);
  fclose(fp);
  return(CONT);
}
#endif	/* ! MSDOS */
#endif	/* ! NO_NET */
#if ! defined(NO_NET)
static int
smtp_get_resp(want)
int	want;	/* 必要なレスポンス */
{
  char resp[NNTP_BUFSIZE+1];
  register char *pt;
  
  while (1) {
    if ( smtp_get_server(resp) != 0 ) {
#if ! defined(WINDOWS)
      mc_printf("\nリプライできません \n\007 %s\n",resp);
      hit_return();
#else  /* WINDOWS */
      mc_MessageBox(hWnd,resp,
		    "リプライできません ",MB_OK);
#endif /* WINDOWS */
      smtp_close_server();
      return(ERROR);
    }

    if ( resp[0] <'0' || '9' < resp[0] )
      continue;

    if ( atoi(resp) != want ) {
#if ! defined(WINDOWS)
      mc_printf("\nリプライできません \n\007 %s\n",resp);
      hit_return();
#else  /* WINDOWS */
      mc_MessageBox(hWnd,resp,
		    "リプライできません ",MB_OK);
#endif /* WINDOWS */
      smtp_close_server();
      return(ERROR);
    }

    pt = resp;			/* 行頭のステータス番号をスキップ */
    while ( '0' <= *pt && *pt <= '9' )
      pt++;

    if ( *pt == ' ' )		/* - なら継続 */
      break;
  }
  
  return(CONT);
}
#endif	/* ! NO_NET */
