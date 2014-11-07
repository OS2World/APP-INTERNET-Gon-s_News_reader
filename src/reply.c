/*
 * @(#)reply.c	1.40 Jul.9,1997
 *
 * ������������ޤ��󡣤�������������Ū�ʳ��λ��ѡ����ۤ����¤��ߤ��ޤ���
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
    mc_MessageBox(hWnd,"��ץ饤���뵭�����ɤޤ�Ƥ��ޤ��� ","",MB_OK);
#else  /* WINDOWS */
    mc_puts("\n��ץ饤���뵭�����ɤޤ�Ƥ��ޤ��� \n\007");
    hit_return();
#endif /* WINDOWS */
    return;
  }
  
  /* �Խ��ѤΥե������������� */
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
    sprintf(resp,"���ǥ����ε�ư�˼��Ԥ��ޤ���");
    mc_MessageBox(hWnd,resp,"",MB_ICONSTOP);
  }
#else	/* WIN32 */
  x = WinExec(resp,SW_SHOW);
  if ( x < 32 ){
    sprintf(resp,"���ǥ����ε�ư�˼��Ԥ��ޤ��� %d",x);
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
    /* ���ǥ�����ư */
#if defined(QUICKWIN)
    mc_printf("%s �򡢥ƥ����ȥ��ǥ������Խ����Ʋ�����\n",
	      tmp_file_f);
    kb_input("�Խ�����λ�����ʤ�꥿���󥭡��򲡤��Ʋ�����");
#else  /* QUICKWIN */
    sprintf(resp,"%s %s",gn.editor, tmp_file_f);
    system(resp);
#endif /* QUICKWIN */

    while (1) {
      /* ��ǧ���� */
      pt = kb_input("�����˥�ץ饤���ޤ�����(y:����/e:�⤦�����Խ�/n:���)");
      if (*pt == 'e' || *pt == 'E' ||
	  *pt == 'y' || *pt == 'Y' ||
	  *pt == 'n' || *pt == 'N')
	break;
    }
    switch(*pt) {
    case 'e':
    case 'E':			/* �⤦����Խ� */
      continue;
    case 'y':
    case 'Y':			/* ��ץ饤���� */
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
  
  switch(DialogBox(hInst,	/* ���ߤΥ��󥹥��� */
		   "IDD_SEND",	/* ���Ѥ���꥽���� */
		   hWnd,	/* �ƥ�����ɥ��Υϥ�ɥ� */
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
      sprintf(resp,"���ǥ����ε�ư�˼��Ԥ��ޤ���");
      mc_MessageBox(hWnd,resp,"",MB_ICONSTOP);
    }
#else	/* !WIN32 */
    x = WinExec(resp,SW_SHOW);
    if ( x < 32 ){
      sprintf(resp,"���ǥ����ε�ư�˼��Ԥ��ޤ��� %d",x);
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
HWND hDlg;                       /* ���������ܥå���������ɥ��Υϥ�ɥ� */
unsigned message;                /* ��å������Υ�����                     */
WORD wParam;                     /* ��å�������ͭ�ξ���                   */
LONG lParam;
{
  char sendtext[256];
  
  switch (message) {
  case WM_INITDIALOG:
    kanji_convert(internal_kanji_code,"�����˥�ץ饤���ޤ�����",
		  gn.display_kanji_code, sendtext);
    SetDlgItemText(hDlg, IDC_SENDTEXT, sendtext);
    kanji_convert(internal_kanji_code,"����(\036Y)",
		  gn.display_kanji_code, sendtext);
    SetDlgItemText(hDlg, IDOK, sendtext);
    kanji_convert(internal_kanji_code,"���ʤ�(\036N)",
		  gn.display_kanji_code, sendtext);
    SetDlgItemText(hDlg, IDCANCEL, sendtext);
    kanji_convert(internal_kanji_code,"�⤦�����Խ�(\036E)",
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
 * ��ץ饤�ѤΥƥ�ݥ��ե�����κ���
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
  
  /* �������ե����� */
  if ( ( rp = fopen(last_art.tmp_file,"r") ) == NULL ){
#ifdef	WINDOWS
    mc_MessageBox(hWnd,"�ƥ�ݥ��ե�����Υ����ץ�˼��Ԥ��ޤ���",
		  last_art.tmp_file,MB_OK);
#else  /* WINDOWS */
    perror(last_art.tmp_file);
#endif /* WINDOWS */
    return(-1);
  }
  resp[0] = 0;
  get_1_header_file(buf,resp,rp);
  
  /* �إå��β��� */
  while(1) {
    if ( resp[0] == 0 )		/* �إå��ν��� */
      break;
    get_1_header_file(buf,resp,rp);
    if ( buf[0] == '\n' )	/* �إå��ν��� */
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
  
  /* ���ǥ��å��ѥե����� */
  strcpy(tmp_file_f,gn.tmpdir);
  strcat(tmp_file_f,"gnXXXXXX");
  mktemp(&tmp_file_f[0]);
  if ( ( fp = fopen(tmp_file_f,"w") ) == NULL ){
#ifdef	WINDOWS
    mc_MessageBox(hWnd,"�ƥ�ݥ��ե�����Υ����ץ�˼��Ԥ��ޤ���",
		  tmp_file_f,MB_OK);
#else  /* WINDOWS */
    perror(tmp_file_f);
#endif /* WINDOWS */
    return(-1);
  }
  
  /* �إå����κ��� */
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
  
  fprintf(fp,"\n");	/* �إå��ν��� */
  
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
  
  /* �������ե����� */
  fclose(rp);
  if ( ( rp = fopen(last_art.tmp_file,"r") ) == NULL ){
#ifdef	WINDOWS
    mc_MessageBox(hWnd,"�ƥ�ݥ��ե�����Υ����ץ�˼��Ԥ��ޤ���",
		  last_art.tmp_file,MB_OK);
#else  /* WINDOWS */
    perror(last_art.tmp_file);
#endif /* WINDOWS */
    return(-1);
  }
  
  /* �������κ��� */
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
  
  /* signature ���ղ� */
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
  
  /* �Խ��ѤΥե������������� */
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
    sprintf(resp,"���ǥ����ε�ư�˼��Ԥ��ޤ���");
    mc_MessageBox(hWnd,resp,"",MB_ICONSTOP);
  }
#else	/* !WIN32 */
  x = WinExec(resp,SW_SHOW);
  if ( x < 32 ){
    sprintf(resp,"���ǥ����ε�ư�˼��Ԥ��ޤ��� %d",x);
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
    /* ���ǥ�����ư */
#if defined(QUICKWIN)
    mc_printf("%s �򡢥ƥ����ȥ��ǥ������Խ����Ʋ�����\n",
	      tmp_file_f);
    kb_input("�Խ�����λ�����ʤ�꥿���󥭡��򲡤��Ʋ�����");
#else  /* QUICKWIN */
    sprintf(resp,"%s %s",gn.editor, tmp_file_f);
    system(resp);
#endif /* QUICKWIN */

    while (1) {
      /* ��ǧ���� */
      pt = kb_input("�����˥ᥤ�뤷�ޤ�����(y:����/e:�⤦�����Խ�/n:���)");
      if (*pt == 'e' || *pt == 'E' ||
	  *pt == 'y' || *pt == 'Y' ||
	  *pt == 'n' || *pt == 'N')
	break;
    }
    switch(*pt) {
    case 'e':
    case 'E':			/* �⤦����Խ� */
      continue;
    case 'y':
    case 'Y':			/* ��ץ饤���� */
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
  
  switch(DialogBox(hInst,	/* ���ߤΥ��󥹥��� */
		   "IDD_SEND",	/* ���Ѥ���꥽���� */
		   hWnd,	/* �ƥ�����ɥ��Υϥ�ɥ� */
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
      sprintf(resp,"���ǥ����ε�ư�˼��Ԥ��ޤ���");
      mc_MessageBox(hWnd,resp,"",MB_ICONSTOP);
    }
#else	/* !WIN32 */
    x = WinExec(resp,SW_SHOW);
    if ( x < 32 ){
      sprintf(resp,"���ǥ����ε�ư�˼��Ԥ��ޤ��� %d",x);
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
HWND hDlg;                       /* ���������ܥå���������ɥ��Υϥ�ɥ� */
unsigned message;                /* ��å������Υ�����                     */
WORD wParam;                     /* ��å�������ͭ�ξ���                   */
LONG lParam;
{
  char sendtext[256];
  
  switch (message) {
  case WM_INITDIALOG:
    kanji_convert(internal_kanji_code,"�����˥ᥤ�뤷�ޤ�����",
		  gn.display_kanji_code, sendtext);
    SetDlgItemText(hDlg, IDC_SENDTEXT, sendtext);
    kanji_convert(internal_kanji_code,"����(\036Y)",
		  gn.display_kanji_code, sendtext);
    SetDlgItemText(hDlg, IDOK, sendtext);
    kanji_convert(internal_kanji_code,"���ʤ�(\036N)",
		  gn.display_kanji_code, sendtext);
    SetDlgItemText(hDlg, IDCANCEL, sendtext);
    kanji_convert(internal_kanji_code,"�⤦�����Խ�(\036E)",
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
 * �Խ��ѤΥե�����κ���
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
  
  /* ���ǥ��å��ѥե����� */
  strcpy(tmp_file_f,gn.tmpdir);
  strcat(tmp_file_f,"gnXXXXXX");
  mktemp(&tmp_file_f[0]);
  if ( ( fp = fopen(tmp_file_f,"w") ) == NULL ){
#ifdef	WINDOWS
    mc_MessageBox(hWnd,"�ƥ�ݥ��ե�����Υ����ץ�˼��Ԥ��ޤ���",
		  tmp_file_f,MB_OK);
#else  /* WINDOWS */
    perror(tmp_file_f);
#endif /* WINDOWS */
    return(-1);
  }
  
  /* �إå����κ��� */
#ifndef	WINDOWS
  
  /* To: */
  while (1) {
    pt = kb_input("��������Ϥ��Ʋ�����");
    if ( strlen(pt) != 0 )
      break;
    pt = kb_input("��ߤ��ޤ�����(y:���/����¾:��³)");
    if ( *pt == 'y' || *pt == 'Y'){
      fclose(fp);
      unlink(tmp_file_f);
      return (1);
    }
  }
  fprintf(fp,"To: %s\n",pt);
  
  /* Subject: */
  while (1) {
    pt = kb_input("���֥������Ȥ����Ϥ��Ʋ�������Ⱦ�ѱѿ����Τߡ�");
    if ( strlen(pt) != 0 )
      break;
    pt = kb_input("��ߤ��ޤ�����(y:���/����¾:��³)");
    if ( *pt == 'y' || *pt == 'Y'){
      fclose(fp);
      unlink(tmp_file_f);
      return (1);
    }
  }
  fprintf(fp,"Subject: %s\n",pt);
  
  /* X-Nsubject: */
  if ( gn.nsubject != 0 ){
    pt = kb_input("���ܸ쥵�֥������Ȥ����Ϥ��Ʋ������ʾ�ά�ġ�");
    if ( strlen(pt) != 0 )
      fprintf(fp,"X-Nsubject: %s\n",pt);
  }
  
#else	/* WINDOWS */
  if (DialogBox(hInst,		/* ���ߤΥ��󥹥��� */
		"IDD_MAIL",	/* ���Ѥ���꥽���� */
		hWnd,		/* �ƥ�����ɥ��Υϥ�ɥ� */
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
  
  /* signature ���ղ� */
  add_signature(fp);
  
  fclose(fp);
  return(0);
}
#ifdef	WINDOWS
BOOL __export CALLBACK
mail_dialog(hDlg, message, wParam, lParam)
HWND hDlg;                       /* ���������ܥå���������ɥ��Υϥ�ɥ� */
unsigned message;                /* ��å������Υ�����                     */
WORD wParam;                     /* ��å�������ͭ�ξ���                   */
LONG lParam;
{
  char buf[128];
  
  switch (message) {
  case WM_INITDIALOG:
    kanji_convert(internal_kanji_code,"����Υᥤ�륢�ɥ쥹",
		  gn.display_kanji_code, buf);
    SetDlgItemText(hDlg, IDC_MAILTOTEXT, buf);

    kanji_convert(internal_kanji_code,"���֥�������",
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
