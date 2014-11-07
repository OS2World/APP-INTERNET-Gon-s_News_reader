/*
 * @(#)follow.c	1.40 Jul.9,1997
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
#include	<time.h>

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
extern void reply_mode();
extern int post_file();
extern void mc_puts(),mc_printf();
extern void hit_return();
extern void kanji_convert();
extern int last_art_check();

#ifdef	USE_JNAMES
char *jnFetch();
#endif

#if defined(WINDOWS)
extern int mc_MessageBox();
static HTASK exeTask;
#ifndef	WIN32
static LPFNNOTIFYCALLBACK NprocInst;
#endif	/* !WIN32 */
#endif	/* WINDOWS */

static follow_write_file();
static char tmp_file_f[PATH_LEN];

int
followup_mode()
{
#ifdef WINDOWS
#ifndef	WIN32
  BOOL __export CALLBACK _loadds NotifyHandler_follow();
  int x;
#endif	/* !WIN32 */
#endif	/* WINDOWS */
#ifndef	QUICKWIN
  char resp[NNTP_BUFSIZE+1];
#endif
#ifndef WINDOWS
  char *pt;
#endif	/* WINDOWS */
#ifdef WIN32
  STARTUPINFO		siStartInfo = {0};
  PROCESS_INFORMATION	piProcInfo;
#endif	/* WIN32 */
  
  if ( last_art_check() == ERROR ){
#ifdef	WINDOWS
    mc_MessageBox(hWnd,"フォローする記事が読まれていません ","",MB_OK);
#else  /* WINDOWS */
    mc_puts("\nフォローする記事が読まれていません \n\007");
    hit_return();
#endif /* WINDOWS */
    return(CONT);
  }
  
  /* 編集用のファイルを作成する */
  if ( follow_write_file(tmp_file_f) != 0 )
    return(CONT);
  
#ifdef WINDOWS
  sprintf(resp,"%s %s.",gn.editor,tmp_file_f);
#ifdef WIN32
  siStartInfo.cb = sizeof(STARTUPINFO);

  if (CreateProcess(NULL, resp, NULL, NULL, FALSE,
                    0, NULL, NULL, &siStartInfo, &piProcInfo))
  {
    ShowWindow(hWnd,SW_MINIMIZE);
    WaitForSingleObject(piProcInfo.hProcess, INFINITE);
    /* Process has terminated */
    PostMessage(hWnd, WM_FOLLOWCONF, 0, 0L);
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
      MakeProcInstance(NotifyHandler_follow, hInst);
    NotifyRegister(NULL, NprocInst, NF_NORMAL);
    
    ShowWindow(hWnd,SW_MINIMIZE);
  }
#endif	/* WIN32 */
  return(CONT);
#else	/* WINDOWS */
  while (1) {
    /* 編集 */
#ifdef QUICKWIN
    mc_printf("%s を、テキストエディタで編集して下さい\n",
	      tmp_file_f);
    kb_input("編集が完了したならリターンキーを押して下さい");
#else  /* QUICKWIN */
    sprintf(resp,"%s %s",gn.editor,tmp_file_f);
    system(resp);
#endif /* QUICKWIN */

    while (1) {
      /* 確認 */
      pt = kb_input("本当にフォローしますか？(y:する/e:もう一度編集/n:中止)");
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
    case 'Y':			/* ポストする */
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
NotifyHandler_follow(WORD wID, DWORD dwData)
{
  if (wID == NFY_EXITTASK ) {
    if(GetCurrentTask() == exeTask){
      PostMessage(hWnd, WM_FOLLOWCONF,0 , 0L);
      return(TRUE);
    }
  }
  return(FALSE);
}
#endif	/* !WIN32 */
void
follow_confirm()
{
  BOOL __export CALLBACK follow_conf();
  char resp[NNTP_BUFSIZE+1];
#ifdef WIN32
  STARTUPINFO		siStartInfo = {0};
  PROCESS_INFORMATION	piProcInfo;
#else	/* WIN32 */
  int x;
#endif	/* !WIN32 */
  
#ifndef	WIN32
  FreeProcInstance(NprocInst);
  NotifyUnRegister(NULL);
#endif	/* !WIN32 */
  ShowWindow(hWnd,SW_RESTORE);
  
  switch(DialogBox(hInst,	/* 現在のインスタンス */
		   "IDD_SEND",	/* 使用するリソース */
		   hWnd,	/* 親ウィンドウのハンドル */
		   follow_conf)){
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
      PostMessage(hWnd, WM_FOLLOWCONF, 0, 0L);
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
	MakeProcInstance(NotifyHandler_follow, hInst);
      NotifyRegister(NULL, NprocInst, NF_NORMAL);
      ShowWindow(hWnd,SW_MINIMIZE);
    }
    return;
#endif	/* WIN32 */
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
follow_conf(hDlg, message, wParam, lParam)
HWND hDlg;                       /* ダイアログボックスウィンドウのハンドル */
unsigned message;                /* メッセージのタイプ                     */
WORD wParam;                     /* メッセージ固有の情報                   */
LONG lParam;
{
  char sendtext[256];
  
  switch (message) {
  case WM_INITDIALOG:
    kanji_convert(internal_kanji_code,"本当にフォローしますか？",
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

static int
follow_write_file(tmp_file_f)
char *tmp_file_f;
{
  extern int STRNCASECMP();
#ifdef	WINDOWS
  static char file_read_error[] =
    "ファイルの読み込みに失敗しました";
#else	/* WINDOWS */
  static char file_read_error[] =
    "\nファイルの読み込みに失敗しました\n\007";
#endif	/* WINDOWS */
  FILE *fp,*rp;
  char resp[NNTP_BUFSIZE+1];
#ifdef	USE_JNAMES
  char resp2[NNTP_BUFSIZE+1];
#endif	/* USE_JNAMES */
  char *key_buf;
  char *pt;
  int x = 0;
  char *from = 0;
  char *date = 0;
  char *newsgroups = 0;
  char *subject = 0;
  char *x_nsubject = 0;
  char *message_id = 0;
  char *references = 0;
  char *distribution = 0;
  char *followup_to = 0;
#ifdef	USE_JNAMES
  char *tmp;
#endif
  
  /* 元記事ファイル */
  if ( ( rp = fopen(last_art.tmp_file,"r") ) == NULL ){
#ifdef	WINDOWS
    mc_MessageBox(hWnd,file_read_error,"",MB_OK);
#else  /* WINDOWS */
    perror(last_art.tmp_file);
#endif /* WINDOWS */
    return(-1);
  }
  resp[0] = 0;
  
  /* ヘッダの解析 */
  while(1) {
    if ( resp[0] == 0 ){
      if ( fgets(resp,NNTP_BUFSIZE,rp) == NULL ) {
#ifdef	WINDOWS
	mc_MessageBox(hWnd,file_read_error,"",MB_OK);
#else  /* WINDOWS */
	mc_puts(file_read_error);
	perror(resp);
	hit_return();
#endif /* WINDOWS */
	return(-1);
      }
    }

    if ( resp[0] == '\n' )	/* ヘッダの終り */
      break;

    switch (resp[0]){
    case 'F':
      if ( strncmp(resp,"From: ",6) == 0 ) {
	from = (char *)malloc(strlen(&resp[6])+1);
	strcpy(from,&resp[6]);
	while(1) {
	  if ( fgets(resp,NNTP_BUFSIZE,rp) == NULL ) {
#ifdef	WINDOWS
	    mc_MessageBox(hWnd,file_read_error,"",MB_OK);
#else  /* WINDOWS */
	    mc_puts(file_read_error);
	    hit_return();
#endif /* WINDOWS */
	    free(from);
	    return(-1);
	  }
	  if ( ('A' <= resp[0] && resp[0] <= 'Z') || resp[0] == '\n')
	    break;
	  pt = (char *)malloc(strlen(from)+strlen(resp)+1);
	  strcpy(pt,from);
	  strcat(pt,resp);
	  free(from);
	  from = pt;
	}
	if ( (pt = strchr(from,'(')) != NULL ){
	  *pt = 0;
	  pt--;
	  while ( *pt == ' ' || *pt == '\t' )
	    *pt-- = 0;
	}
	continue;
      }
      if ( STRNCASECMP(resp,"Followup-To: ",13) == 0 ) {
	followup_to = (char *)malloc(strlen(&resp[13])+1);
	strcpy(followup_to,&resp[13]);
	while(1) {
	  if ( fgets(resp,NNTP_BUFSIZE,rp) == NULL ) {
#ifdef	WINDOWS
	    mc_MessageBox(hWnd,file_read_error,"",MB_OK);
#else  /* WINDOWS */
	    mc_puts(file_read_error);
	    hit_return();
#endif /* WINDOWS */
	    free(followup_to);
	    return(-1);
	  }
	  if ( ('A' <= resp[0] && resp[0] <= 'Z') || resp[0] == '\n')
	    break;
	  pt = (char *)malloc(strlen(followup_to)+strlen(resp)+1);
	  strcpy(pt,followup_to);
	  strcat(pt,resp);
	  free(followup_to);
	  followup_to = pt;
	}
				
	if (strcmp(followup_to,"poster\n") == 0) {
	  free(followup_to);	/* フォローする場合でも不要 */
	  followup_to = 0;
					
	  key_buf = kb_input("\nFollowup-To: poster の指定がされています\nリプライに切替えますか？(n:無視してフォロー/その他:リプライ)");
					
	  /* 無視してフォロー */
	  if ( key_buf[0] == 'n' || key_buf[0] == 'N' )
	    continue;
					
	  /* リプライ */
	  if (from)
	    free(from);
	  if (date)
	    free(date);
	  if (newsgroups)
	    free(newsgroups);
	  if (subject)
	    free(subject);
	  if (x_nsubject)
	    free(x_nsubject);
	  if (message_id)
	    free(message_id);
	  if (references)
	    free(references);
	  if (distribution)
	    free(distribution);
					
	  fclose(rp);
					
	  reply_mode();
	  return(-1);
	}
	continue;
      }
      resp[0] = 0;
      break;
    case 'D':
      if ( strncmp(resp,"Date: ",6) == 0 ) {
	date = (char *)malloc(strlen(&resp[6])+1);
	strcpy(date,&resp[6]);
	while(1) {
	  if ( fgets(resp,NNTP_BUFSIZE,rp) == NULL ) {
#ifdef	WINDOWS
	    mc_MessageBox(hWnd,file_read_error,"",MB_OK);
#else  /* WINDOWS */
	    mc_puts(file_read_error);
	    hit_return();
#endif /* WINDOWS */
	    free(date);
	    return(-1);
	  }
	  if ( ('A' <= resp[0] && resp[0] <= 'Z') || resp[0] == '\n')
	    break;
	  pt = (char *)malloc(strlen(date)+strlen(resp)+1);
	  strcpy(pt,date);
	  strcat(pt,resp);
	  free(date);
	  date = pt;
	}
	continue;
      }
      if ( strncmp(resp,"Distribution: ",14) == 0 ) {
	distribution = (char *)malloc(strlen(&resp[14])+1);
	strcpy(distribution,&resp[14]);
	while(1) {
	  if ( fgets(resp,NNTP_BUFSIZE,rp) == NULL ) {
#ifdef	WINDOWS
	    mc_MessageBox(hWnd,file_read_error,"",MB_OK);
#else  /* WINDOWS */
	    mc_puts(file_read_error);
	    hit_return();
#endif /* WINDOWS */
	    free(distribution);
	    return(-1);
	  }
	  if ( ('A' <= resp[0] && resp[0] <= 'Z') || resp[0] == '\n')
	    break;
	  pt = (char *)malloc(strlen(distribution)+strlen(resp)+1);
	  strcpy(pt,distribution);
	  strcat(pt,resp);
	  free(distribution);
	  distribution = pt;
	}
	continue;
      }
      resp[0] = 0;
      break;
    case 'N':
      if ( strncmp(resp,"Newsgroups: ",12) == 0 ) {
	newsgroups = (char *)malloc(strlen(&resp[12])+1);
	strcpy(newsgroups,&resp[12]);
	while(1) {
	  if ( fgets(resp,NNTP_BUFSIZE,rp) == NULL ) {
#ifdef	WINDOWS
	    mc_MessageBox(hWnd,file_read_error,"",MB_OK);
#else  /* WINDOWS */
	    mc_puts(file_read_error);
	    hit_return();
#endif /* WINDOWS */
	    free(newsgroups);
	    return(-1);
	  }
	  if ( ('A' <= resp[0] && resp[0] <= 'Z') || resp[0] == '\n')
	    break;
	  pt = (char *)malloc(strlen(newsgroups)+strlen(resp)+1);
	  strcpy(pt,newsgroups);
	  strcat(pt,resp);
	  free(newsgroups);
	  newsgroups = pt;
	}
	continue;
      }
      resp[0] = 0;
      break;
    case 'S':
      if ( strncmp(resp,"Subject: ",9) == 0 ) {
	pt = &resp[9];
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
	while(1) {
	  if ( fgets(resp,NNTP_BUFSIZE,rp) == NULL ) {
#ifdef	WINDOWS
	    mc_MessageBox(hWnd,file_read_error,"",MB_OK);
#else  /* WINDOWS */
	    mc_puts(file_read_error);
	    hit_return();
#endif /* WINDOWS */
	    free(subject);
	    return(-1);
	  }
	  if ( ('A' <= resp[0] && resp[0] <= 'Z') || resp[0] == '\n')
	    break;
	  pt = (char *)malloc(strlen(subject)+strlen(resp)+1);
	  strcpy(pt,subject);
	  strcat(pt,resp);
	  free(subject);
	  subject = pt;
	}
	continue;
      }
      resp[0] = 0;
      break;
    case 'X':
      if ( gn.nsubject == 1 && strncmp(resp,"X-Nsubject: ",12) == 0 ) {
	pt = &resp[12];
	while ( *pt == ' ' )
	  pt++;
	x_nsubject = (char *)malloc(strlen(pt)+4+1);
	if ( strncmp(pt,"Re:",3) == 0 ) {
	  pt += 3;
	  while ( *pt == ' ' )
	    pt++;
	}
	strcpy(x_nsubject,"Re: ");
	strcat(x_nsubject,pt);
	while(1) {
	  if ( fgets(resp,NNTP_BUFSIZE,rp) == NULL ) {
#ifdef	WINDOWS
	    mc_MessageBox(hWnd,file_read_error,"",MB_OK);
#else  /* WINDOWS */
	    mc_puts(file_read_error);
	    hit_return();
#endif /* WINDOWS */
	    free(x_nsubject);
	    return(-1);
	  }
	  if ( ('A' <= resp[0] && resp[0] <= 'Z') || resp[0] == '\n')
	    break;
	  pt = (char *)malloc(strlen(x_nsubject)+strlen(resp)+1);
	  strcpy(pt,x_nsubject);
	  strcat(pt,resp);
	  free(x_nsubject);
	  x_nsubject = pt;
	}
	continue;
      }
      resp[0] = 0;
      break;
    case 'M':
      if ( strncmp(resp,"Message-ID: ",12) == 0 ) {
	message_id = (char *)malloc(strlen(&resp[12])+1);
	strcpy(message_id,&resp[12]);
	while(1) {
	  if ( fgets(resp,NNTP_BUFSIZE,rp) == NULL ) {
#ifdef	WINDOWS
	    mc_MessageBox(hWnd,file_read_error,"",MB_OK);
#else  /* WINDOWS */
	    mc_puts(file_read_error);
	    hit_return();
#endif /* WINDOWS */
	    free(message_id);
	    return(-1);
	  }
	  if ( ('A' <= resp[0] && resp[0] <= 'Z') || resp[0] == '\n')
	    break;
	  pt = (char *)malloc(strlen(message_id)+strlen(resp)+1);
	  strcpy(pt,message_id);
	  strcat(pt,resp);
	  free(message_id);
	  message_id = pt;
	}
	continue;
      }
      resp[0] = 0;
      break;
    case 'R':
      if ( strncmp(resp,"References: ",12) == 0 ) {
	references = (char *)malloc(strlen(&resp[12])+1);
	strcpy(references,&resp[12]);
	while(1) {
	  if ( fgets(resp,NNTP_BUFSIZE,rp) == NULL ) {
#ifdef	WINDOWS
	    mc_MessageBox(hWnd,file_read_error,"",MB_OK);
#else  /* WINDOWS */
	    mc_puts(file_read_error);
	    hit_return();
#endif /* WINDOWS */
	    free(references);
	    return(-1);
	  }
	  if ( ('A' <= resp[0] && resp[0] <= 'Z') || resp[0] == '\n')
	    break;
	  pt = (char *)malloc(strlen(references)+strlen(resp)+1);
	  strcpy(pt,references);
	  strcat(pt,resp);
	  free(references);
	  references = pt;
	}
	continue;
      }
      resp[0] = 0;
      break;
    default:
      resp[0] = 0;
      break;
    }
  }
  
  /* エディット用ファイル */
  strcpy(tmp_file_f,gn.tmpdir);
  strcat(tmp_file_f,"gnXXXXXX");
  mktemp(&tmp_file_f[0]);
  if ( ( fp = fopen(tmp_file_f,"w") ) == NULL ){
#ifdef	WINDOWS
    mc_MessageBox(hWnd,"テンポラリファイルの作成に失敗しました","",MB_OK);
#else  /* WINDOWS */
    perror(tmp_file_f);
#endif /* WINDOWS */
    return(-1);
  }
  
  /* ヘッダ部の作成 */
  fprintf(fp,"Subject: ");
  if ( subject != 0 )
    fprintf(fp,"%s",subject);
  else
    fprintf(fp,"\n");
  
  fprintf(fp,"Newsgroups: ");
  if ( followup_to != 0 ){
    fprintf(fp,"%s",followup_to);
    strcpy(resp,followup_to);
  } else {
    if ( newsgroups != 0 ) {
      fprintf(fp,"%s",newsgroups);
      strcpy(resp,newsgroups);
    } else {
      fprintf(fp,"\n");
      resp[0] = 0;
    }
  }
  
  fprintf(fp,"References: ");
  if ( references != 0 ) {
    pt = strrchr(references,'\n');
    if ( pt != 0 )
      *pt = 0;
    fprintf(fp,"%s ",references);

    pt = references;
    while (1){
      if ( (pt = strchr(pt,'<')) == NULL )
	break;
      if ( (pt = strchr(pt,'>')) != NULL )
	continue;
#ifdef	WINDOWS
      mc_MessageBox(hWnd,"References: ヘッダがつぶれています。編集時に修正して下さい","",MB_OK);
#else  /* WINDOWS */
      mc_puts("References: ヘッダがつぶれています。編集時に修正して下さい\n\007");
      hit_return();
#endif /* WINDOWS */
      break;
    }
  }
  if ( message_id != 0 )
    fprintf(fp,"%s",message_id);
  else
    fprintf(fp,"\n");
  
  if ( from != 0 && date != 0 ) {
    if ( (pt = strchr(from,'\n')) != NULL )
      *pt = 0;
    fprintf(fp,"In-Reply-To: %s's message of %s",from,date);
  }
  
  if ( distribution != 0 ) {
    fprintf(fp,"Distribution: ");
    fprintf(fp,"%s",distribution);
  }
  
  if ( x_nsubject != 0 )
    fprintf(fp,"X-Nsubject: %s",x_nsubject);
  
#ifdef	USE_JNAMES
  message_id[strlen(message_id)-1] = 0;	/* message_id の改行を削除。 */
  if(NULL != (tmp = jnFetch("familyname", from)))
    sprintf(resp, "\n\n%sの記事で、\n%sさんは書きました。\n\n",
	    message_id,tmp);
  else
    sprintf(resp, "\n\n%sの記事で、\n%sさんは書きました。\n\n",
	    message_id, from);
  kanji_convert(internal_kanji_code,resp,gn.process_kanji_code,resp2);
  fprintf(fp, "%s",resp2);
#else
  if ( ! gn.rn_like )
    fprintf(fp,"\n\nFrom article %s\tby %s\n\n",message_id,from);
  else
    fprintf(fp,"\n\nIn article %s\t%s writes:\n\n",message_id,from);
#endif
  
  if (from)
    free(from);
  if (date)
    free(date);
  if (newsgroups)
    free(newsgroups);
  if (subject)
    free(subject);
  if (x_nsubject)
    free(x_nsubject);
  if (message_id)
    free(message_id);
  if (references)
    free(references);
  if (distribution)
    free(distribution);
  if (followup_to)
    free(followup_to);
  
  /* 元記事ファイル */
  fclose(rp);
  if ( ( rp = fopen(last_art.tmp_file,"r") ) == NULL ){
#ifdef	WINDOWS
    mc_MessageBox(hWnd,"テンポラリファイルの読込みに失敗しました","",MB_OK);
#else  /* WINDOWS */
    perror(last_art.tmp_file);
#endif /* WINDOWS */
    return(-1);
  }
  
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
    fprintf(fp,"> %s",resp);
  }
  fclose(rp);
  
  /* signature の付加 */
  add_signature(fp);
  
  fclose(fp);
  return(CONT);
}
