/*
 * @(#)auth.c	1.40 Jul.24,1997
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


#ifdef AUTHENTICATION
#if ! defined(NO_NET)
#if defined(MSDOS) || defined(OS2)

#ifdef WINDOWS
#	include <windows.h>
#	include "wingn.h"
#endif

#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <ctype.h>

#ifdef	HAS_CONIO_H
#	include <conio.h>
#endif
#ifdef	QUICKWIN
#	include <io.h>
#	include <graph.h>
#endif

#if defined(WATTCP)
#	include <tcp.h>
#endif
#ifdef PCNFS
#	include "pcnfs.h"
#endif

#include	"nntp.h"
#include	"gn.h"

#if defined(WATTCP)
static socket_t	pser_rd_fp;
#	define	pser_wr_fp pser_rd_fp
#else	/* WATTCP */
static socket_t	pser_rd_fp = 0;
static socket_t	pser_wr_fp = 0;
#endif	/* WATTCP */

#ifdef	AUTH_POP2
#	define	POP	"pop"
#else
#	define	POP	"pop3"
#endif

extern void mc_puts(),mc_printf();
extern void kanji_convert();

#if defined(WINDOWS)
extern int mc_MessageBox();
extern void set_hourglass(),reset_cursor();
#endif	/* WINDOWS */

static auth_server_init(),auth_get_server();
static int auth_put_server();
static void auth_close_server();
static void auth_input_pass();
int auth_check_pass();

/*
 * auth_server_init  Get a connection to the remote ftp/pop server.
 *
 *	Parameters:	"machine" is the machine to connect to.
 *
 *	Returns:	-1 on error
 *			0 on success.
 *
 *	Side effects:	Connects to server.
 *			"pser_rd_fp" and "pser_wr_fp" are fp's
 *			for reading and writing to server.
 */
static
auth_server_init(machine)
char *machine;
{
  extern int server_init_com();
  char resp[NNTP_BUFSIZE+1];
  
#ifdef	AUTH_FTP
  return(server_init_com(machine, &pser_rd_fp, &pser_wr_fp,
			 "ftp", "tcp", resp));
#else
  return(server_init_com(machine, &pser_rd_fp, &pser_wr_fp,
			 POP, "tcp", resp));
#endif
}


/*
 * auth_put_server -- send a line of text to the server, terminating it
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
auth_put_server(string)
char *string;
{
  extern int put_server_com();
  
  int	ret;
#ifdef	WINDOWS
  char buf[128];
#endif	/* WINDOWS */
  
  if ( (ret = put_server_com(string,&pser_wr_fp)) == CONT )
    return(CONT);
  
#ifdef	WINDOWS
  reset_cursor();
  
  sprintf(buf,"サーバとの接続が切れました:%d",ret);
  mc_MessageBox(hWnd,buf,"",MB_OK);
#else	/* WINDOWS */
  mc_printf("サーバとの接続が切れました:%d\n",ret);
#endif	/* WINDOWS */
  return(ERROR);
}

/*
 * auth_get_server -- get a line of text from the server.  Strips
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
static
auth_get_server(string, size)
char *string;
int size;
{
  extern int get_server_com();
  
  return(get_server_com(string, size, &pser_rd_fp));
}


/*
 * auth_close_server -- close the connection to the server, after sending
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

static void
auth_close_server()
{
  extern void close_server_com();
  
  close_server_com(&pser_rd_fp,&pser_wr_fp);
}


#define	LEN	1024
char pass[LEN];

#ifndef	WINDOWS
auth(usr)
char *usr;
{
  if (gn.passwd != 0 )
    strcpy(pass,gn.passwd);
  else
    auth_input_pass();
  
  return(auth_check_pass());
}
static void
auth_input_pass()
{
  char resp[LEN];
  char *pt;
  
#ifdef	QUICKWIN
  int handle, status;
  FILE *wp;
  struct _wsizeinfo wsize;
  
  status = _setvideomode( _MAXRESMODE ); 
  handle = _wggetactive( );              
  status = _wgclose( handle );           
  
  handle = _wgopen( "passwd" );              
  status = _wgsetactive( handle );       
  status = _setvideomode( _MAXRESMODE ); 
  
  sprintf(resp,"ユーザ %s のパスワードを入力して下さい：",gn.usrname);
  kanji_convert(internal_kanji_code,resp,gn.display_kanji_code,pass);
  _outtext(pass);
  
  pt = pass;
  *pt = 0;
  while (1) {
    *pt = (char)_inchar();
    if ( *pt == '\n' || *pt == '\r' )
      break;
    pt++;
  }
  *pt = 0;
  
  status = _wgclose( handle );           
  
  wsize._version = _QWINVER;
  wsize._type = _WINSIZEMAX;	/* 最大 */
  
  if ( (wp = _fwopen( NULL, &wsize, "w" )) == NULL ) {
    gn.lines = 24;
    gn.columns = 80;
  } else {
    if ( _wgetsize( _WINFRAMEHAND, _WINCURRREQ, &wsize ) != -1 ) {
      gn.columns = wsize._w;
      gn.lines = wsize._h;
    }
    _wclose( _fileno( wp ), _WINNOPERSIST );
  }
#else	/* QUICKWIN */
  sprintf(resp,"ユーザ %s のパスワードを入力して下さい：",gn.usrname);
  mc_puts(resp);
  fflush(stdout);
  pt = pass;
  *pt = 0;
  while (1) {
    *pt = getch();
    if ( *pt == '\n' || *pt == '\r' )
      break;
    pt++;
  }
  *pt = 0;
  printf("\n");
#endif	/* QUICKWIN */
  
#ifdef	DEBUG
  printf("passwd = %s\n",pass);
#endif
}

#else	/* WINDOWS */

BOOL __export CALLBACK
get_passwd(hDlg, message, wParam, lParam)
HWND hDlg;                       /* ダイアログボックスウィンドウのハンドル */
unsigned message;                /* メッセージのタイプ                     */
WORD wParam;                     /* メッセージ固有の情報                   */
LONG lParam;
{
  char buf[LEN];
  
  switch (message) {
  case WM_INITDIALOG:
    sprintf(buf,"ユーザ %s のパスワードを入力して下さい",gn.usrname);
    kanji_convert(internal_kanji_code,buf,gn.display_kanji_code,pass);
    SetDlgItemText(hDlg, IDC_PASSWDTEXT, (LPSTR)pass);
    return TRUE;

  case WM_COMMAND:
    if (wParam == IDOK) {
      GetDlgItemText(hDlg, IDC_PASSWD, (LPSTR)pass, 255);
      EndDialog(hDlg, TRUE);
      SendMessage(hWnd, WM_CHECKPASS,0 , 0L);
      return TRUE;
    }
    break;
  }
  return FALSE;
}
void
auth_set_passwd()
{
  strcpy(pass,gn.passwd);
}
#endif	/* WINDOWS */

auth_check_pass()
{
  char resp[LEN];
  
  mc_puts("パスワードを照合しています\n");
  
#ifdef	WINDOWS
  set_hourglass();
#endif	/* WINDOWS */
  
#ifdef	AUTH_FTP
  if ( auth_server_init(gn.authserver) != 220 ) {
#ifdef	WINDOWS
    reset_cursor();
    mc_MessageBox(hWnd,"ftp サーバと接続できませんでした","",MB_OK);
#else  /* WINDOWS */
    mc_puts("ftp サーバと接続できませんでした\n");
#endif /* WINDOWS */
    return(-1);
  }
#else
#ifdef	AUTH_POP2
  if ( auth_server_init(gn.authserver) != 0 ) {
#ifdef	WINDOWS
    reset_cursor();
    mc_MessageBox(hWnd,"pop サーバと接続できませんでした","",MB_OK);
#else  /* WINDOWS */
    mc_puts("pop サーバと接続できませんでした\n");
#endif /* WINDOWS */
    return(-1);
  }
#else
  if ( auth_server_init(gn.authserver) != 0 ) {
#ifdef	WINDOWS
    reset_cursor();
    mc_MessageBox(hWnd,"pop3 サーバと接続できませんでした","",MB_OK);
#else  /* WINDOWS */
    mc_puts("pop3 サーバと接続できませんでした\n");
#endif /* WINDOWS */
    return(-1);
  }
#endif
#endif
  
  sprintf(resp,"user %s",gn.usrname);
  if ( auth_put_server(resp) != CONT )
    return(-1);
  auth_get_server(resp,LEN);
  
#ifdef	AUTH_FTP
  if ( atoi(resp) != 331 ) {
#ifdef	WINDOWS
    reset_cursor();
    mc_MessageBox(hWnd,"ユーザが見つかりません","",MB_OK);
#else  /* WINDOWS */
    mc_puts("ユーザが見つかりません \n"); /* ??? */
#endif /* WINDOWS */
    auth_close_server();
    return(-1);
  }
#else
  if ( resp[0] != '+' ) {
#ifdef	WINDOWS
    reset_cursor();
    mc_MessageBox(hWnd,"ユーザが見つかりません ","",MB_OK);
#else  /* WINDOWS */
    mc_puts("ユーザが見つかりません \n"); /* ??? */
#endif /* WINDOWS */
    auth_close_server();
    return(-1);
  }
#endif
  
  sprintf(resp,"pass %s",pass);
  if ( auth_put_server(resp) != CONT )
    return(-1);
  auth_get_server(resp,LEN);
  
#ifdef	AUTH_FTP
  if ( atoi(resp) != 230 ) {
#ifdef	WINDOWS
    reset_cursor();
    mc_MessageBox(hWnd,"パスワードが違います","",MB_OK);
#else  /* WINDOWS */
    mc_puts("パスワードが違います\n");
#endif /* WINDOWS */
    auth_close_server();
    return(-1);
  }
#else
  if ( resp[0] != '+' ) {
#ifdef	WINDOWS
    reset_cursor();
    mc_MessageBox(hWnd,"パスワードが違います","",MB_OK);
#else  /* WINDOWS */
    mc_puts("パスワードが違います\n");
#endif /* WINDOWS */
    auth_close_server();
    return(-1);
  }
#endif
  
  auth_close_server();
  
#ifdef	WINDOWS
  reset_cursor();
#endif	/* WINDOWS */
  
  return(0);
}
#endif	/* MSDOS || OS2 */
#endif	/* ! NO_NET */
#endif	/* AUTHENTICATION */
