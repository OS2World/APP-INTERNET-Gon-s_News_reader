/*
 * @(#)key.c	1.40 May.2,1998
 *
 * 著作権は放棄しません。ただし、営利目的以外の使用／配布に制限は設けません。
 * Copyright (C) yamasita@omronsoft.co.jp
 *     + inouet@strl.nhk.or.jp  Aug.19.1993
 */


#if defined(WINDOWS)
#	include	<windows.h>
#	include	"wingn.h"
#endif	/* WINDOWS */

#include	<stdio.h>
#include	<ctype.h>

#ifdef	SVR4
#	include	<sys/termios.h>
#	define SYSV_TERMIO
#endif

#ifdef	HAS_STRING_H
#	include	<string.h>
#endif
#ifdef	HAS_STDLIB_H
#	include	<stdlib.h>
#endif
#ifdef	HAS_SYS_IOCTL_H
#	include	<sys/ioctl.h>
#endif
#ifdef	HAS_UNISTD_H
#	include	<unistd.h>
#endif
#ifdef	HAS_SYS_TERMIO_H
#	if !defined(AIX) && (!defined(sparc) || defined(SVR4))
#		include	<sys/termio.h>
#		define SYSV_TERMIO
#	endif
#endif
#ifdef	HAS_CONIO_H
#	include	<conio.h>
#endif

#ifdef        __FreeBSD__
#	include      <sgtty.h>
#endif        /* __FreeBSD__ */

#if defined(WATTCP)
#	include <tcp.h>
#endif


#ifdef	UNIX
extern void Exit();
#	define	exit(n) Exit(n)
#endif /* UNIX */

#include	"nntp.h"
#include	"gn.h"

extern void mc_puts(),mc_printf();
extern void kanji_convert();
extern char *Fgets();

static void kb_input_msg();
static char key_buf[KEY_BUF_LEN];

#if ! defined(WINDOWS)
/*
 * キーボード入力
 */
static char *message_save;
static void
kb_input_msg()
{
  mc_puts(message_save);
  mc_puts("：");
  fflush(stdout);
}
char *
kb_input(message)
char *message;
{
  register char *pt;
  
  message_save = message;
  kb_input_msg();
  redraw_func = kb_input_msg;
  while ( FGETS(key_buf,KEY_BUF_LEN,stdin) == NULL ) {
    clearerr(stdin);
    mc_printf("\n");
    kb_input_msg();
  }
  redraw_func = (void (*)())NULL;
  
  /* 後ろのホワイトスペースを消す */
  pt = &key_buf[strlen(key_buf)-1];
  while( pt >= key_buf && ( *pt == ' ' || *pt == '\t' || *pt == '\n' ) )
    *pt-- = 0;
  
  /* 前のホワイトスペースを消す */
  pt = key_buf;
  while(*pt == ' ' || *pt == '\t' )
    pt++;
  return(pt);
}

void
hit_return()
{
#if defined(QUICKWIN)
  kb_input("\nリターンキーを押して下さい");
#else
  if ( gn.use_space == OFF ) {
    kb_input("\nリターンキーを押して下さい");
  } else {
    mc_puts("\nリターンキーもしくはスペースキーを押して下さい：");
    fflush(stdout);
    while ( Fgets(key_buf,KEY_BUF_LEN,stdin) == NULL )
      ;
  }
#endif
  mc_printf("\n");
}
#else	/* WINDOWS */
char *
kb_input(message)
char *message;
{
  BOOL __export CALLBACK kb_input_dialog();
  
  kanji_convert(internal_kanji_code,message,
		gn.display_kanji_code,key_buf);
  
  if ( DialogBox(hInst,		/* 現在のインスタンス */
		 "IDD_KBINPUT",	/* 使用するリソース */
		 hWnd,		/* 親ウィンドウのハンドル */
		 kb_input_dialog) == FALSE ){
    return(NULL);
  }
  return(key_buf);
}
BOOL __export CALLBACK
kb_input_dialog(hDlg, message, wParam, lParam)
HWND hDlg;                       /* ダイアログボックスウィンドウのハンドル */
unsigned message;                /* メッセージのタイプ                     */
WORD wParam;                     /* メッセージ固有の情報                   */
LONG lParam;
{
  switch (message) {
  case WM_INITDIALOG:
    SetDlgItemText(hDlg, IDC_KBINPUT, (LPSTR)key_buf);
    return TRUE;

  case WM_COMMAND:
    switch (wParam) {
    case IDOK:
      EndDialog(hDlg, TRUE);
      GetDlgItemText(hDlg, IDC_KBTEXT, (LPSTR)key_buf, KEY_BUF_LEN);

      return(TRUE);

    case IDCANCEL:
      EndDialog(hDlg, FALSE);
      return(TRUE);
    }
    break;
  }
  return FALSE;
}
#endif	/* WINDOWS */


#if ! defined(WINDOWS) && ! defined(QUICKWIN)
char *
Fgets(s, n, stream)
char *s;
int n;
FILE *stream;
{
  int c;
  char *fp;
#if !defined(MSDOS) && !defined(OS2)
#ifdef	SYSV_TERMIO
  struct termio sgo, sgn;
#else
  struct sgttyb sgo, sgn;
#endif
#endif
  
  if (stream != stdin){
    fputs("Fgets: Bad stream\n",stdout);
    exit(1);
  }

  n--;
#if !defined(MSDOS) && !defined(OS2)
#ifdef	SYSV_TERMIO
  ioctl(fileno(stdin), TCGETA, &sgo);
  sgn = sgo;
  sgn.c_iflag &=~(/* INLCR |*/ ICRNL  | IUCLC | ISTRIP | IXON | IXOFF);
  sgn.c_lflag &=~(ISIG | ICANON | XCASE | ECHO | ECHOE | ECHOK | ECHONL);
  sgn.c_oflag = OPOST;
  /*	sgn.c_cc[VINTR] = CDEL;	*/
  /*	sgn.c_cc[VQUIT] = CDEL;	*/
  /*	sgn.c_cc[VERASE] = CDEL;	*/
  /*	sgn.c_cc[VKILL] = CDEL;	*/
  sgn.c_cc[VEOF] = 1;
  sgn.c_cc[VMIN] = 1;
  sgn.c_cc[VEOL] = 0;
  sgn.c_cc[VTIME] = 0;
#ifndef	linux
  sgn.c_cc[VSWTCH] = CDEL;
#endif
  if (ioctl (fileno(stdin), TCSETAW, &sgn) < 0){
    perror("Fgets: ioctl");
    exit(0);
  }
#else	/* SYSV_TERMIO */
  ioctl(0, TIOCGETP, &sgo);
  sgn = sgo;
  sgn.sg_flags &= ~ECHO;
  sgn.sg_flags |= RAW;
  if (ioctl(0, TIOCSETP, &sgn) < 0){
    perror("Fgets: ioctl");
    exit(0);
  }
#endif	/* SYSV_TERMIO */
#endif	/* MSDOS */
  
  
  fp = (char*)s;
  do{
#if defined(MSDOS) || defined(OS2)
    c = getch() & 0x00ff;
    if ((c == '\b')||(c == DEL))
#else  /* MSDOS */
      c = getc(stream) & 0x00ff;
#ifdef	SYSV_TERMIO
    if ((c == sgo.c_cc[VERASE])||(c == DEL))
#else  /* SYSV_TERMIO */
      if ((c == sgo.sg_erase)||(c == DEL))
#endif /* SYSV_TERMIO */
#endif /* MSDOS */
	{
	  if (fp != (char*)s) {
	    printf("\b \b");
	    if (iscntrl(*(fp-1)))
	      printf("\b \b");
	    fflush(stdout);
	    fp--; n++;
	    *fp = '\0';
	  }
	} else {
	  if (c == '\n')
	    c = '\r';		/* for Openwindows cmdtool */
	  switch(c){
	  case EOF:
	    break;
	  case ' ':
	    if (fp == (char*)s)
	      c='\r';
	  default:
	    *fp = c;
	    if (iscntrl(c) && (c != '\r')){
	      putchar('^'); putchar(c+'A'-1);
	    }else
	      putchar(c);
	    fflush(stdout);
	    fp++;
	    *fp = '\0';
	    n--;
	    break;
	  }
	}
  }while((c != '\r')||(c == EOF)||(n == 0));
  if (*(fp-1) == '\r')
    *(fp-1) = '\n';
  putchar('\n');
  fflush(stdout);
#if !defined(MSDOS) && !defined(OS2)
#ifdef	SYSV_TERMIO
  if (ioctl (fileno(stdin), TCSETAW, &sgo) < 0){
    perror("Fgets: ioctl2");
    exit(0);
  }
#else
  if (ioctl(0, TIOCSETP, &sgo) < 0){
    perror("Fgets: ioctl2");
    exit(0);
  }
#endif
#endif
  if ((c == EOF)&&(fp == (char*)s))
    return NULL;
  else
    return s;
}
#endif	/* ! WINDOWS && !QUICKWIN */
