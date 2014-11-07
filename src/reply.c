/*
 * @(#)reply.c	1.40 Jul.9,1997
 *
 * 著作権は放棄しません。ただし、営利目的以外の使用／配布に制限は設けません。
 * Copyright (C) yamasita@omronsoft.co.jp
 */

#if defined(WINDOWS)
#	include	<windows.h>
#	ifndef WIN32
#		include	<toolhelp.h>
#	endif
#	include	"wingn.h"
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

#if defined(WATTCP)
#	include <tcp.h>
#endif
#ifdef PCNFS
#	include "pcnfs.h"
#endif

#include	"nntp.h"
#include	"gn.h"

extern char *kb_input();
extern void add_signature();
extern int mail_file();
extern int get_1_header_file();
extern void mc_puts(),mc_printf();
extern void hit_return();
extern void kanji_convert();
extern int last_art_check();

#if defined(WINDOWS)
extern int mc_MessageBox();
static HTASK exeTask;
#ifndef	WIN32
static LPFNNOTIFYCALLBACK NprocInst;
#endif	/* !WIN32 */
#endif	/* WINDOWS */

static reply_write_file();
static mail_write_file();
static char tmp_file_f[PATH_LEN];

#ifdef	WINDOWS
static char mail_to[NNTP_BUFSIZE+1];
static char mail_subject[NNTP_BUFSIZE+1];
#endif	/* WINDOWS */

void
reply_mode()
{
#ifdef WINDOWS
#ifdef WIN32
  STARTUPINFO		siStartInfo = {0};
  PROCESS_INFORMATION	piProcInfo;
#else	/* WIN32 */
  BOOL __export CALLBACK _loadds NotifyHandler_reply();
  int x;
#endif	/* WIN32 */
#endif	/* WINDOWS */
#ifndef	QUICKWIN
  char resp[NNTP_BUFSIZE+1];
#endif
#ifndef WINDOWS
  char *pt;
#endif	/* WINDOWS */
  
  if ( last_art_check() == ERROR ){
#ifdef	WINDOWS
    mc_MessageBox(hWnd,"リプライする記事が読まれていません ","",MB_OK);
#else  /* WINDOWS */
    mc_puts("\nリプライする記事が読まれていません \n\007");
    hit_return();
#endif /* WINDOWS */
    return;
  }
  
  /* 編集用のファイルを作成する */
  if ( (reply_write_file(tmp_file_f)) != CONT )
    return;
  
#if defined(WINDOWS)
  sprintf(resp,"%s %s.",gn.editor,tmp_file_f);
#ifdef WIN32
  siStartInfo.cb = sizeof(STARTUPINFO);

  if (CreateProcess(NULL, resp, NULL, NULL, FALSE,
                    0, NULL, NULL, &siStartInfo, &piProcInfo))
  {
    ShowWindow(hWnd,SW_MINIMIZE);
    WaitForSingleObject(piProcInfo.hProcess, INFINITE);
    /* Process has terminated */
    PostMessage(hWnd, WM_REPLYCONF, 0, 0L);
  } else {
    /* Process could not be started */
    sprintf(resp,"エディタの起動に失敗しました");
    mc_MessageBox(hWnd,resp,"",MB_ICONSTOP);
  }
#else	/* WIN32 */
  x = WinExec(resp,SW_SHOW);
  if ( x < 32 ){
    sprintf(resp,"エディタの起動に失敗しました %d",x);
    mc_MessageBox(hWnd,resp,"",MB_ICONSTOP);
  } else {
    exeTask = GetWindowTask(GetActiveWindow());
    NprocInst = (LPFNNOTIFYCALLBACK)
      MakeProcInstance(NotifyHandler_reply, hInst);
    NotifyRegister(NULL, NprocInst, NF_NORMAL);
    
    ShowWindow(hWnd,SW_MINIMIZE);
  }
#endif	/* WIN32 */
  return;
#else	/* WINDOWS */
  while (1) {
    /* エディタ起動 */
#if defined(QUICKWIN)
    mc_printf("%s を、テキストエディタで編集して下さい\n",
	      tmp_file_f);
    kb_input("編集が完了したならリターンキーを押して下さい");
#else  /* QUICKWIN */
    sprintf(resp,"%s %s",gn.editor, tmp_file_f);
    system(resp);
#endif /* QUICKWIN */

    while (1) {
      /* 確認入力 */
      pt = kb_input("本当にリプライしますか？(y:する/e:もう一度編集/n:中止)");
      if (*pt == 'e' || *pt == 'E' ||
	  *pt == 'y' || *pt == 'Y' ||
	  *pt == 'n' || *pt == 'N')
	break;
    }
    switch(*pt) {
    case 'e':
    case 'E':			/* もう一回編集 */
      continue;
    case 'y':
    case 'Y':			/* リプライする */
      mail_file(tmp_file_f, gn.process_kanji_code);
      break;
    default:
      break;
    }
    unlink(tmp_file_f);
    return;
  }
#endif	/* WINDOWS */
}
#ifdef WINDOWS
#ifndef	WIN32
BOOL __export CALLBACK _loadds
NotifyHandler_reply(WORD wID, DWORD dwData)
{
  if (wID == NFY_EXITTASK ) {
    if(GetCurrentTask() == exeTask){
      PostMessage(hWnd, WM_REPLYCONF,0 , 0L);
      return(TRUE);
    }
  }
  return(FALSE);
}
#endif	/* !WIN32 */
void
reply_confirm()
{
  BOOL __export CALLBACK reply_conf();
  char resp[NNTP_BUFSIZE+1];
#ifdef WIN32
  STARTUPINFO		siStartInfo = {0};
  PROCESS_INFORMATION	piProcInfo;
#else	/* WIN32 */
  int x;
  
  FreeProcInstance(NprocInst);
  NotifyUnRegister(NULL);
#endif	/* !WIN32 */
  ShowWindow(hWnd,SW_RESTORE);
  
  switch(DialogBox(hInst,	/* 現在のインスタンス */
		   "IDD_SEND",	/* 使用するリソース */
		   hWnd,	/* 親ウィンドウのハンドル */
		   reply_conf)){
  case 'e':
    sprintf(resp,"%s %s.",gn.editor,tmp_file_f);
#ifdef WIN32
    siStartInfo.cb = sizeof(STARTUPINFO);

    if (CreateProcess(NULL, resp, NULL, NULL, FALSE,
                      0, NULL, NULL, &siStartInfo, &piProcInfo))
    {
      ShowWindow(hWnd,SW_MINIMIZE);
      WaitForSingleObject(piProcInfo.hProcess, INFINITE);
      /* Process has terminated */
      PostMessage(hWnd, WM_REPLYCONF, 0, 0L);
    } else {
      /* Process could not be started */
      sprintf(resp,"エディタの起動に失敗しました");
      mc_MessageBox(hWnd,resp,"",MB_ICONSTOP);
    }
#else	/* !WIN32 */
    x = WinExec(resp,SW_SHOW);
    if ( x < 32 ){
      sprintf(resp,"エディタの起動に失敗しました %d",x);
      mc_MessageBox(hWnd,resp,"",MB_ICONSTOP);
    } else {
      exeTask = GetWindowTask(GetActiveWindow());
      NprocInst = (LPFNNOTIFYCALLBACK)
	MakeProcInstance(NotifyHandler_reply, hInst);
      NotifyRegister(NULL, NprocInst, NF_NORMAL);
      ShowWindow(hWnd,SW_MINIMIZE);
    }
#endif	/* WIN32 */
    return;
  case 'y':
    mail_file(tmp_file_f, gn.process_kanji_code);
    break;
    
  default:
    break;
  }
  unlink(tmp_file_f);
}
BOOL __export CALLBACK
reply_conf(hDlg, message, wParam, lParam)
HWND hDlg;                       /* ダイアログボックスウィンドウのハンドル */
unsigned message;                /* メッセージのタイプ                     */
WORD wParam;                     /* メッセージ固有の情報                   */
LONG lParam;
{
  char sendtext[256];
  
  switch (message) {
  case WM_INITDIALOG:
    kanji_convert(internal_kanji_code,"本当にリプライしますか？",
		  gn.display_kanji_code, sendtext);
    SetDlgItemText(hDlg, IDC_SENDTEXT, sendtext);
    kanji_convert(internal_kanji_code,"する(\036Y)",
		  gn.display_kanji_code, sendtext);
    SetDlgItemText(hDlg, IDOK, sendtext);
    kanji_convert(internal_kanji_code,"しない(\036N)",
		  gn.display_kanji_code, sendtext);
    SetDlgItemText(hDlg, IDCANCEL, sendtext);
    kanji_convert(internal_kanji_code,"もう一度編集(\036E)",
		  gn.display_kanji_code, sendtext);
    SetDlgItemText(hDlg, IDRETRY, sendtext);
    return TRUE;

  case WM_COMMAND:
    switch (wParam) {
    case IDOK:
      EndDialog(hDlg, 'y');
      return(TRUE);

    case IDCANCEL:
      EndDialog(hDlg, 'n');
      return(TRUE);

    case IDRETRY:
      EndDialog(hDlg, 'e');
      return(TRUE);
    }
    break;
  }
  return(FALSE);
}
#endif	/* WINDOWS */

/*
 * リプライ用のテンポラリファイルの作成
 */

static int
reply_write_file(tmp_file_f)
char *tmp_file_f;
{
  FILE *fp,*rp;
  char resp[NNTP_BUFSIZE+1];
  char buf[NNTP_BUFSIZE+1];
  char *pt;
  int x = 0;
  char *from = 0;
  char *to = 0;
  char *date = 0;
  char *subject = 0;
  char *message_id = 0;
  
  /* 元記事ファイル */
  if ( ( rp = fopen(last_art.tmp_file,"r") ) == NULL ){
#ifdef	WINDOWS
    mc_MessageBox(hWnd,"テンポラリファイルのオープンに失敗しました",
		  last_art.tmp_file,MB_OK);
#else  /* WINDOWS */
    perror(last_art.tmp_file);
#endif /* WINDOWS */
    return(-1);
  }
  resp[0] = 0;
  get_1_header_file(buf,resp,rp);
  
  /* ヘッダの解析 */
  while(1) {
    if ( resp[0] == 0 )		/* ヘッダの終り */
      break;
    get_1_header_file(buf,resp,rp);
    if ( buf[0] == '\n' )	/* ヘッダの終り */
      break;

    switch(buf[0]) {
    case 'F':
      if ( strncmp(buf,"From: ",6) == 0 ) {
	if ( from != 0 )
	  free(from);
	from = (char *)malloc(strlen(&buf[6])+1);
	strcpy(from,&buf[6]);
	if ( to == 0 ) {
	  to = (char *)malloc(strlen(from)+1);
	  strcpy(to,from);
	}
	if ( (pt = strchr(from,'(')) != NULL ){
	  *pt = 0;
	  pt--;
	  while ( *pt == ' ' || *pt == '\t' )
	    *pt-- = 0;
	}
      }
      break;
    case 'D':
      if ( strncmp(buf,"Date: ",6) == 0 ) {
	if ( date == 0 ) {
	  date = (char *)malloc(strlen(&buf[6])+1);
	  strcpy(date,&buf[6]);
	}
      }
      break;
    case 'R':
      if ( strncmp(buf,"Reply-To: ",10) == 0 ) {
	if ( to != 0 )
	  free(to);
	to = (char *)malloc(strlen(&buf[10])+1);
	strcpy(to,&buf[10]);
      }
      break;
    case 'S':
      if ( strncmp(buf,"Subject: ",9) == 0 ) {
	pt = &buf[9];
	while ( *pt == ' ' )
	  pt++;
	subject = (char *)malloc(strlen(pt)+4+1);
	if ( strncmp(pt,"Re:",3) == 0 ) {
	  pt += 3;
	  while ( *pt == ' ' )
	    pt++;
	}
	strcpy(subject,"Re: ");
	strcat(subject,pt);
      }
      break;
    case 'M':
      if ( strncmp(buf,"Message-ID: ",12) == 0 ) {
	message_id = (char *)malloc(strlen(&buf[12])+1);
	strcpy(message_id,&buf[12]);
      }
      break;
    default:
      break;
    }
  }
  
  /* エディット用ファイル */
  strcpy(tmp_file_f,gn.tmpdir);
  strcat(tmp_file_f,"gnXXXXXX");
  mktemp(&tmp_file_f[0]);
  if ( ( fp = fopen(tmp_file_f,"w") ) == NULL ){
#ifdef	WINDOWS
    mc_MessageBox(hWnd,"テンポラリファイルのオープンに失敗しました",
		  tmp_file_f,MB_OK);
#else  /* WINDOWS */
    perror(tmp_file_f);
#endif /* WINDOWS */
    return(-1);
  }
  
  /* ヘッダ部の作成 */
  fprintf(fp,"To: ");
  if ( to != 0 )
    fprintf(fp,"%s",to);
  fprintf(fp,"\n");
  
  fprintf(fp,"Subject: ");
  if ( subject != 0 )
    fprintf(fp,"%s",subject);
  fprintf(fp,"\n");
  
  if ( from != 0 && date != 0 ) {
    if ( (pt = strchr(from,'\n' )) != NULL )
      *pt = 0;
    fprintf(fp,"In-Reply-To: %s's message of %s",from,date);
  }
  fprintf(fp,"\n");
  
  if ( gn.cc2me == CCTOME )
    fprintf(fp,"Cc: %s\n", gn.usrname);
  if ( gn.cc2me == BCCTOME )
    fprintf(fp,"Bcc: %s\n", gn.usrname);
  
  fprintf(fp,"\n");	/* ヘッダの終り */
  
  if ( gn.rn_like == 1 )
    fprintf(fp,"\nIn article %s\n\t%s writes:\n\n",message_id,from);
  
  if (from)
    free(from);
  if (date)
    free(date);
  if (to)
    free(to);
  if (subject)
    free(subject);
  if (message_id)
    free(message_id);
  
  /* 元記事ファイル */
  fclose(rp);
  if ( ( rp = fopen(last_art.tmp_file,"r") ) == NULL ){
#ifdef	WINDOWS
    mc_MessageBox(hWnd,"テンポラリファイルのオープンに失敗しました",
		  last_art.tmp_file,MB_OK);
#else  /* WINDOWS */
    perror(last_art.tmp_file);
#endif /* WINDOWS */
    return(-1);
  }
  
  /* 記事部の作成 */
  while(1) {
    if ( ++x > 20 ){
      mc_printf(".");
      fflush(stdout);
      x = 0;
#ifdef	QUICKWIN
      _wyield();
#endif
    }
    if ( fgets(resp,NNTP_BUFSIZE,rp) == NULL )
      break;
    if ( (pt = strchr(resp,'\n')) != NULL )
      *pt = 0;
    fprintf(fp,"> %s\n",resp);
  }
  fclose(rp);
  
  /* signature の付加 */
  add_signature(fp);
  
  fclose(fp);
  return(CONT);
}
void
mail_mode()
{
#ifdef WINDOWS
#ifdef WIN32
  STARTUPINFO		siStartInfo = {0};
  PROCESS_INFORMATION	piProcInfo;
#else	/* WIN32 */
  BOOL __export CALLBACK _loadds NotifyHandler_mail();
  int x;
#endif	/* WIN32 */
#endif	/* WINDOWS */
#ifndef	QUICKWIN
  char resp[NNTP_BUFSIZE+1];
#endif
#ifndef WINDOWS
  char *pt;
#endif	/* WINDOWS */
  
  /* 編集用のファイルを作成する */
  if ( (mail_write_file(tmp_file_f)) != CONT )
    return;
  
#if defined(WINDOWS)
  sprintf(resp,"%s %s.",gn.editor,tmp_file_f);
#ifdef WIN32
  siStartInfo.cb = sizeof(STARTUPINFO);

  if (CreateProcess(NULL, resp, NULL, NULL, FALSE,
                    0, NULL, NULL, &siStartInfo, &piProcInfo))
  {
    ShowWindow(hWnd,SW_MINIMIZE);
    WaitForSingleObject(piProcInfo.hProcess, INFINITE);
    /* Process has terminated */
    PostMessage(hWnd, WM_MAILCONF, 0, 0L);
  } else {
    /* Process could not be started */
    sprintf(resp,"エディタの起動に失敗しました");
    mc_MessageBox(hWnd,resp,"",MB_ICONSTOP);
  }
#else	/* !WIN32 */
  x = WinExec(resp,SW_SHOW);
  if ( x < 32 ){
    sprintf(resp,"エディタの起動に失敗しました %d",x);
    mc_MessageBox(hWnd,resp,"",MB_ICONSTOP);
  } else {
    exeTask = GetWindowTask(GetActiveWindow());
    NprocInst = (LPFNNOTIFYCALLBACK)
      MakeProcInstance(NotifyHandler_mail, hInst);
    NotifyRegister(NULL, NprocInst, NF_NORMAL);
    
    ShowWindow(hWnd,SW_MINIMIZE);
  }
#endif	/* WIN32 */
  return;
#else	/* WINDOWS */
  while (1) {
    /* エディタ起動 */
#if defined(QUICKWIN)
    mc_printf("%s を、テキストエディタで編集して下さい\n",
	      tmp_file_f);
    kb_input("編集が完了したならリターンキーを押して下さい");
#else  /* QUICKWIN */
    sprintf(resp,"%s %s",gn.editor, tmp_file_f);
    system(resp);
#endif /* QUICKWIN */

    while (1) {
      /* 確認入力 */
      pt = kb_input("本当にメイルしますか？(y:する/e:もう一度編集/n:中止)");
      if (*pt == 'e' || *pt == 'E' ||
	  *pt == 'y' || *pt == 'Y' ||
	  *pt == 'n' || *pt == 'N')
	break;
    }
    switch(*pt) {
    case 'e':
    case 'E':			/* もう一回編集 */
      continue;
    case 'y':
    case 'Y':			/* リプライする */
      mail_file(tmp_file_f, gn.process_kanji_code);
      break;
    default:
      break;
    }
    unlink(tmp_file_f);
    return;
  }
#endif	/* WINDOWS */
}
#ifdef WINDOWS
#ifndef	WIN32
BOOL __export CALLBACK _loadds
NotifyHandler_mail(WORD wID, DWORD dwData)
{
  if (wID == NFY_EXITTASK ) {
    if(GetCurrentTask() == exeTask){
      PostMessage(hWnd, WM_MAILCONF,0 , 0L);
      return(TRUE);
    }
  }
  return(FALSE);
}
#endif	/* WIN32 */
void
mail_confirm()
{
  BOOL __export CALLBACK mail_conf();
  char resp[NNTP_BUFSIZE+1];
#ifdef WIN32
  STARTUPINFO		siStartInfo = {0};
  PROCESS_INFORMATION	piProcInfo;
#else	/* !WIN32 */
  int x;
  
  FreeProcInstance(NprocInst);
  NotifyUnRegister(NULL);
#endif	/* WIN32 */
  ShowWindow(hWnd,SW_RESTORE);
  
  switch(DialogBox(hInst,	/* 現在のインスタンス */
		   "IDD_SEND",	/* 使用するリソース */
		   hWnd,	/* 親ウィンドウのハンドル */
		   mail_conf)){
  case 'e':
    sprintf(resp,"%s %s.",gn.editor,tmp_file_f);
#ifdef WIN32
    siStartInfo.cb = sizeof(STARTUPINFO);

    if (CreateProcess(NULL, resp, NULL, NULL, FALSE,
                      0, NULL, NULL, &siStartInfo, &piProcInfo))
    {
      ShowWindow(hWnd,SW_MINIMIZE);
      WaitForSingleObject(piProcInfo.hProcess, INFINITE);
      /* Process has terminated */
      PostMessage(hWnd, WM_MAILCONF, 0, 0L);
    } else {
      /* Process could not be started */
      sprintf(resp,"エディタの起動に失敗しました");
      mc_MessageBox(hWnd,resp,"",MB_ICONSTOP);
    }
#else	/* !WIN32 */
    x = WinExec(resp,SW_SHOW);
    if ( x < 32 ){
      sprintf(resp,"エディタの起動に失敗しました %d",x);
      mc_MessageBox(hWnd,resp,"",MB_ICONSTOP);
    } else {
      exeTask = GetWindowTask(GetActiveWindow());
      NprocInst = (LPFNNOTIFYCALLBACK)
	MakeProcInstance(NotifyHandler_mail, hInst);
      NotifyRegister(NULL, NprocInst, NF_NORMAL);
      ShowWindow(hWnd,SW_MINIMIZE);
    }
#endif	/* WIN32 */
    return;
  case 'y':
    mail_file(tmp_file_f, gn.process_kanji_code);
    break;
    
  default:
    break;
  }
  unlink(tmp_file_f);
}
BOOL __export CALLBACK
mail_conf(hDlg, message, wParam, lParam)
HWND hDlg;                       /* ダイアログボックスウィンドウのハンドル */
unsigned message;                /* メッセージのタイプ                     */
WORD wParam;                     /* メッセージ固有の情報                   */
LONG lParam;
{
  char sendtext[256];
  
  switch (message) {
  case WM_INITDIALOG:
    kanji_convert(internal_kanji_code,"本当にメイルしますか？",
		  gn.display_kanji_code, sendtext);
    SetDlgItemText(hDlg, IDC_SENDTEXT, sendtext);
    kanji_convert(internal_kanji_code,"する(\036Y)",
		  gn.display_kanji_code, sendtext);
    SetDlgItemText(hDlg, IDOK, sendtext);
    kanji_convert(internal_kanji_code,"しない(\036N)",
		  gn.display_kanji_code, sendtext);
    SetDlgItemText(hDlg, IDCANCEL, sendtext);
    kanji_convert(internal_kanji_code,"もう一度編集(\036E)",
		  gn.display_kanji_code, sendtext);
    SetDlgItemText(hDlg, IDRETRY, sendtext);
    return TRUE;

  case WM_COMMAND:
    switch (wParam) {
    case IDOK:
      EndDialog(hDlg, 'y');
      return(TRUE);

    case IDCANCEL:
      EndDialog(hDlg, 'n');
      return(TRUE);

    case IDRETRY:
      EndDialog(hDlg, 'e');
      return(TRUE);
    }
    break;
  }
  return(FALSE);
}
#endif	/* WINDOWS */
/*
 * 編集用のファイルの作成
 */

static int
mail_write_file(tmp_file_f)
char *tmp_file_f;
{
#ifdef	WINDOWS
  BOOL __export CALLBACK mail_dialog();
#endif	/* WINDOWS */
  FILE *fp;
#ifndef	WINDOWS
  register char *pt;
#endif	/* WINDOWS */
  
  /* エディット用ファイル */
  strcpy(tmp_file_f,gn.tmpdir);
  strcat(tmp_file_f,"gnXXXXXX");
  mktemp(&tmp_file_f[0]);
  if ( ( fp = fopen(tmp_file_f,"w") ) == NULL ){
#ifdef	WINDOWS
    mc_MessageBox(hWnd,"テンポラリファイルのオープンに失敗しました",
		  tmp_file_f,MB_OK);
#else  /* WINDOWS */
    perror(tmp_file_f);
#endif /* WINDOWS */
    return(-1);
  }
  
  /* ヘッダ部の作成 */
#ifndef	WINDOWS
  
  /* To: */
  while (1) {
    pt = kb_input("宛先を入力して下さい");
    if ( strlen(pt) != 0 )
      break;
    pt = kb_input("中止しますか？(y:中止/その他:継続)");
    if ( *pt == 'y' || *pt == 'Y'){
      fclose(fp);
      unlink(tmp_file_f);
      return (1);
    }
  }
  fprintf(fp,"To: %s\n",pt);
  
  /* Subject: */
  while (1) {
    pt = kb_input("サブジェクトを入力して下さい（半角英数字のみ）");
    if ( strlen(pt) != 0 )
      break;
    pt = kb_input("中止しますか？(y:中止/その他:継続)");
    if ( *pt == 'y' || *pt == 'Y'){
      fclose(fp);
      unlink(tmp_file_f);
      return (1);
    }
  }
  fprintf(fp,"Subject: %s\n",pt);
  
  /* X-Nsubject: */
  if ( gn.nsubject != 0 ){
    pt = kb_input("日本語サブジェクトを入力して下さい（省略可）");
    if ( strlen(pt) != 0 )
      fprintf(fp,"X-Nsubject: %s\n",pt);
  }
  
#else	/* WINDOWS */
  if (DialogBox(hInst,		/* 現在のインスタンス */
		"IDD_MAIL",	/* 使用するリソース */
		hWnd,		/* 親ウィンドウのハンドル */
		mail_dialog) == IDCANCEL ) {
    fclose(fp);
    unlink(tmp_file_f);
    return (1);
  }
  fprintf(fp,"To: %s\n",mail_to);
  fprintf(fp,"Subject: %s\n",mail_subject);
  
#endif	/* WINDOWS */
  if ( gn.cc2me == CCTOME )
    fprintf(fp,"Cc: %s\n", gn.usrname);
  if ( gn.cc2me == BCCTOME )
    fprintf(fp,"Bcc: %s\n", gn.usrname);
  
  fprintf(fp,"\n\n");
  
  /* signature の付加 */
  add_signature(fp);
  
  fclose(fp);
  return(0);
}
#ifdef	WINDOWS
BOOL __export CALLBACK
mail_dialog(hDlg, message, wParam, lParam)
HWND hDlg;                       /* ダイアログボックスウィンドウのハンドル */
unsigned message;                /* メッセージのタイプ                     */
WORD wParam;                     /* メッセージ固有の情報                   */
LONG lParam;
{
  char buf[128];
  
  switch (message) {
  case WM_INITDIALOG:
    kanji_convert(internal_kanji_code,"宛先のメイルアドレス",
		  gn.display_kanji_code, buf);
    SetDlgItemText(hDlg, IDC_MAILTOTEXT, buf);

    kanji_convert(internal_kanji_code,"サブジェクト",
		  gn.display_kanji_code, buf);
    SetDlgItemText(hDlg, IDC_MAILSUBJECTTEXT, buf);

    return TRUE;

  case WM_COMMAND:
    switch (wParam) {
    case IDOK:
      GetDlgItemText(hDlg, IDC_MAILTO, mail_to,NNTP_BUFSIZE);
      GetDlgItemText(hDlg, IDC_MAILSUBJECT, mail_subject,NNTP_BUFSIZE);
      EndDialog(hDlg, IDOK);
      return(TRUE);

    case IDCANCEL:
      EndDialog(hDlg, IDCANCEL);
      return(TRUE);
    }
    break;
  }
  return(FALSE);
}
#endif	/* WINDOWS */
