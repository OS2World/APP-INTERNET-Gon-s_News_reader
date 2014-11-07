/*
 * @(#)post.c	1.40 Jun.26,1997
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
#include	<time.h>

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
extern void hit_return();
extern char *kb_input();
extern void add_signature();
extern int post_file();
extern void kanji_convert();

#if defined(WINDOWS)
extern int mc_MessageBox();
static char postnewsgroups[NNTP_BUFSIZE+1];
static char postsubject[NNTP_BUFSIZE+1];
static char postfollowupto[NNTP_BUFSIZE+1];
static char postdistribution[NNTP_BUFSIZE+1];
static char postnsubject[NNTP_BUFSIZE+1];
static HTASK exeTask;
#ifndef	WIN32
static LPFNNOTIFYCALLBACK NprocInst;
#endif	/* !WIN32 */
#endif	/* WINDOWS */

static post_write_file(),check_group_name();
static char tmp_file_f[PATH_LEN];

int
post_mode(group)
char *group;
{
#ifdef WINDOWS
#ifdef WIN32
  STARTUPINFO		siStartInfo = {0};
  PROCESS_INFORMATION	piProcInfo;
#else	/* !WIN32 */
  BOOL __export CALLBACK _loadds NotifyHandler_post();
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
  if ( post_write_file(group,tmp_file_f) != 0 )
    return(CONT);
  
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
    PostMessage(hWnd, WM_POSTCONF, 0, 0L);
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
      MakeProcInstance(NotifyHandler_post, hInst);
    NotifyRegister(NULL, NprocInst, NF_NORMAL);
    
    ShowWindow(hWnd,SW_MINIMIZE);
  }
#endif	/* WIN32 */
  return(CONT);
#else	/* WINDOWS */
  while (1) {
    /* �Խ� */
#ifdef QUICKWIN
    mc_printf("%s �򡢥ƥ����ȥ��ǥ������Խ����Ʋ�����\n",tmp_file_f);
    kb_input("�Խ�����λ�����ʤ�꥿���󥭡��򲡤��Ʋ�����");
#else  /* QUICKWIN */
    sprintf(resp,"%s %s",gn.editor,tmp_file_f);
    system(resp);
#endif /* QUICKWIN */

    while (1) {
      /* ��ǧ */
      pt = kb_input("�����˥ݥ��Ȥ��ޤ�����(y:����/e:�⤦�����Խ�/n:���)");
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
    case 'Y':			/* �ݥ��Ȥ��� */
      if ( post_file(tmp_file_f, gn.process_kanji_code) == CONNECTION_CLOSED)
	return(CONNECTION_CLOSED);
      break;
    default:
      break;
    }
    unlink(tmp_file_f);
    return(CONT);
  }
#endif	/* WINDOWS */
}
#ifdef WINDOWS
#ifndef	WIN32
BOOL __export CALLBACK _loadds
NotifyHandler_post(WORD wID, DWORD dwData)
{
  if (wID == NFY_EXITTASK ) {
    if(GetCurrentTask() == exeTask){
      PostMessage(hWnd, WM_POSTCONF,0 , 0L);
      return(TRUE);
    }
  }
  return(FALSE);
}
#endif	/* !WIN32 */
void
post_confirm()
{
  BOOL __export CALLBACK post_conf();
  char resp[NNTP_BUFSIZE+1];
#ifdef WIN32
  STARTUPINFO		siStartInfo = {0};
  PROCESS_INFORMATION	piProcInfo;
#else	/* !WIN32 */
  int x;
  
  FreeProcInstance(NprocInst);
  NotifyUnRegister(NULL);
#endif	/* !WIN32 */
  ShowWindow(hWnd,SW_RESTORE);
  
  switch(DialogBox(hInst,	/* ���ߤΥ��󥹥��� */
		   "IDD_SEND",	/* ���Ѥ���꥽���� */
		   hWnd,	/* �ƥ�����ɥ��Υϥ�ɥ� */
		   post_conf)){
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
      PostMessage(hWnd, WM_POSTCONF, 0, 0L);
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
	MakeProcInstance(NotifyHandler_post, hInst);
      NotifyRegister(NULL, NprocInst, NF_NORMAL);
      ShowWindow(hWnd,SW_MINIMIZE);
    }
#endif	/* !WIN32 */
    return;
  case 'y':
    if ( post_file(tmp_file_f, gn.process_kanji_code) == CONNECTION_CLOSED)
      return;
    break;
    
  default:
    break;
  }
  unlink(tmp_file_f);
}
BOOL __export CALLBACK
post_conf(hDlg, message, wParam, lParam)
HWND hDlg;                       /* ���������ܥå���������ɥ��Υϥ�ɥ� */
unsigned message;                /* ��å������Υ�����                     */
WORD wParam;                     /* ��å�������ͭ�ξ���                   */
LONG lParam;
{
  char sendtext[256];
  
  switch (message) {
  case WM_INITDIALOG:
    kanji_convert(internal_kanji_code,"�����˥ݥ��Ȥ��ޤ�����",
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
    return(TRUE);

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
post_write_file(group,tmp_file_f)
char *group;
char *tmp_file_f;
{
#ifdef	WINDOWS
  BOOL __export CALLBACK post_dialog();
  char *dialog_name;
#endif	/* WINDOWS */
  FILE *fp;
#ifndef	WINDOWS
  char newsgroups[NNTP_BUFSIZE+1];
  char followup_to[NNTP_BUFSIZE+1];
  char resp[NNTP_BUFSIZE+1];
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
  
  /* Newsgroups: */
  if ( group == NULL ) {
    while (1) {
      group = kb_input("�˥塼�����롼�פ����Ϥ��Ʋ�����");
      if ( strlen(group) == 0 ){
	pt = kb_input("��ߤ��ޤ�����(y:���/����¾:��³)");
	if ( *pt == 'y' || *pt == 'Y'){
	  fclose(fp);
	  unlink(tmp_file_f);
	  return (1);
	} else
	  continue;
      }
      if ( check_group_name(group) == 0 )
	break;
    }
    strcpy(newsgroups,group);
    fprintf(fp,"Newsgroups: %s\n",newsgroups);
  } else {
    strcpy(newsgroups,group);
    group = &newsgroups[0];
    fprintf(fp,"Newsgroups: %s\n",group);
  }
  
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
  
  /* Followup-To: */
  while (1) {
    pt = kb_input("�ե������åפ��륰�롼�פ���ꤷ�Ʋ�����\n	�ʥǥե�����裱�˥塼�����롼�ס�");
    if ( strlen(pt) <= 0 ) {
      if ( strchr(newsgroups,',') == NULL )
	break;			/* �����ݥ��ȤǤʤ����ϤĤ��ʤ� */
      strcpy(followup_to,newsgroups);
      if ( (pt = strchr(followup_to,',')) != NULL )
	*pt = 0;
      pt = followup_to;
    }
    if ( check_group_name(pt) == 0 ) {
      fprintf(fp,"Followup-To: %s\n",pt);
      break;
    }
  }
  
  /* Distribution: */
  sprintf(resp,"�����ϰϤ����Ϥ��Ʋ������ʥǥե����:%s �����ۤ�����ϰϡ�",newsgroups);
  pt = kb_input(resp);
  if ( strlen(pt) != 0 )
    fprintf(fp,"Distribution: %s\n",pt);
  
#else	/* WINDOWS */
  if ( gn.nsubject != 0 )
    dialog_name = "IDD_POSTN";
  else
    dialog_name = "IDD_POST";
  
  if (DialogBox(hInst,		/* ���ߤΥ��󥹥��� */
		dialog_name,	/* ���Ѥ���꥽���� */
		hWnd,		/* �ƥ�����ɥ��Υϥ�ɥ� */
		post_dialog) == IDCANCEL ) {
    fclose(fp);
    unlink(tmp_file_f);
    return (1);
  }
  fprintf(fp,"Newsgroups: %s\n",postnewsgroups);
  fprintf(fp,"Subject: %s\n",postsubject);
  fprintf(fp,"Followup-To: %s\n",postfollowupto);
  fprintf(fp,"Distribution: %s\n",postdistribution);
  if ( gn.nsubject != 0 )
    fprintf(fp,"X-Nsubject: %s\n",postnsubject);
#endif	/* WINDOWS */
  fprintf(fp,"\n\n");
  
  /* signature ���ղ� */
  
  add_signature(fp);
  
  fclose(fp);
  return(0);
}
#ifdef	WINDOWS
BOOL __export CALLBACK
post_dialog(hDlg, message, wParam, lParam)
HWND hDlg;                       /* ���������ܥå���������ɥ��Υϥ�ɥ� */
unsigned message;                /* ��å������Υ�����                     */
WORD wParam;                     /* ��å�������ͭ�ξ���                   */
LONG lParam;
{
  char buf[128];
  
  switch (message) {
  case WM_INITDIALOG:
    kanji_convert(internal_kanji_code,
		  "�ݥ��Ȥ���˥塼�����롼��",
		  gn.display_kanji_code,buf);
    SetDlgItemText(hDlg, IDC_POSTNEWSGROUPSTEXT, (LPSTR)buf);
    
    kanji_convert(internal_kanji_code,
		  "���֥������ȡ�Ⱦ�ѱѿ����Τߡ�",
		  gn.display_kanji_code,buf);
    SetDlgItemText(hDlg, IDC_POSTSUBJECTTEXT, (LPSTR)buf);
    
    kanji_convert(internal_kanji_code,
		  "�ե������åפ��륰�롼�סʥǥե���ȡ��裱���롼�ס�",
		  gn.display_kanji_code,buf);
    SetDlgItemText(hDlg, IDC_POSTFOLLOWUPTOTEXT, (LPSTR)buf);
    
    kanji_convert(internal_kanji_code,
		  "�����ϰϡʥǥե���ȡ����¤ʤ���",
		  gn.display_kanji_code,buf);
    SetDlgItemText(hDlg, IDC_POSTDISTRIBUTIONTEXT, (LPSTR)buf);
    
    if ( gn.nsubject != 0 ){
      kanji_convert(internal_kanji_code,
		    "���ܸ쥵�֥������ȡʾ�ά�ġ�",
		    gn.display_kanji_code,buf);
      SetDlgItemText(hDlg, IDC_POSTNSUBJECTTEXT, (LPSTR)buf);
    }
    return TRUE;
    
  case WM_COMMAND:
    switch (wParam) {
    case IDOK:
      GetDlgItemText(hDlg,IDC_POSTNEWSGROUPS,postnewsgroups,NNTP_BUFSIZE);
      GetDlgItemText(hDlg,IDC_POSTSUBJECT,postsubject,NNTP_BUFSIZE);
      GetDlgItemText(hDlg,IDC_POSTFOLLOWUPTO,postfollowupto,NNTP_BUFSIZE);
      GetDlgItemText(hDlg,IDC_POSTDISTRIBUTION,postdistribution,NNTP_BUFSIZE);
      if ( gn.nsubject != 0 )
	GetDlgItemText(hDlg,IDC_POSTNSUBJECT,postnsubject,NNTP_BUFSIZE);
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

static int
check_group_name(ng_name)
char *ng_name;
{
  extern	newsgroup_t *search_group();
  
  char name[NNTP_BUFSIZE+1];
  register char *pt1, *pt2, tmp;
  
  pt2 = ng_name;
  while ( *pt2 == ' ' || *pt2 == '\t' )
    pt2++;
  
  if ( strchr(pt2,' ') != 0 || strchr(pt2,'\t') != 0 ) {
    mc_puts("�˥塼�����롼�פ˥ۥ磻�ȥ��ڡ����ϻȤ��ޤ��� \n");
    return(-1);
  }
  
  strcpy(name,pt2);
  
  while ( 1 ) {
    pt1 = pt2;
    while (*pt2 != 0 && *pt2 != ' ' && *pt2 != '\t' &&
	   *pt2 != '\n' && *pt2 != ',' ){
      pt2++;
    }
    tmp = *pt2;
    *pt2 = 0;

    if ( search_group(pt1) == (newsgroup_t *)0 ) {
      mc_puts("���ꤵ�줿�˥塼�����롼�פ�¸�ߤ��ޤ��� \n");
      return(-1);
    }
    *pt2 = tmp;
    while ( *pt2 == ' ' || *pt2 == '\t' )
      pt2++;
    if ( *pt2 == 0 || *pt2 == '\n' )
      break;

    if ( *pt2 != ',' ){
      mc_puts("�˥塼�����롼�פλ�����ˡ����äƤ��ޤ�\n");
      return(-1);
    }
    pt2++;
    while ( *pt2 == ' ' || *pt2 == '\t' )
      pt2++;
  }
  return(0);
}
