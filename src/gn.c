/*
 * @(#)gn.c	1.40 Sep.19,1997
 *
 * ������������ޤ��󡣤�������������Ū�ʳ��λ��ѡ����ۤ����¤��ߤ��ޤ���
 * Copyright (C) yamasita@omronsoft.co.jp
 */

#if defined(WINDOWS)
#	include	<windows.h>
#	include	"wingn.h"
#endif	/* WINDOWS */

#include	<stdio.h>
#include	<signal.h>

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
#ifdef WINSOCK
#	include <winsock.h>
#endif


#include	"nntp.h"
#include	"gn.h"

extern char *kb_input();
extern void save_newsrc();
extern void mc_puts(),mc_printf();
extern void memory_error();
extern int last_art_check();

#ifdef	USE_JNAMES
int jnOpen();
int jnClose();
#endif

int prog = GN;
int newsrc_flag;		/* newsrc ���ɤ߹�����飰���ʤ��ä���ݣ� */

#ifdef __TURBOC__
extern unsigned _stklen = 10240;
#endif

extern int gn_init();
extern int get_newsrc();
extern int open_server();
extern int get_active();
extern int get_newsgroups();
extern void check_group();
extern void close_server();
extern int group_mode();

#if defined(WINDOWS)
extern int mc_MessageBox();
#endif	/* WINDOWS */

#if !defined(MSDOS) && !defined(OS2)
extern void Exit();
#endif

#ifndef	WINDOWS
static void disp_group();

int
main(argc,argv)
int argc;
char *argv[];
{
  int check_only;
  
  /* ��ư���ץ����β��� */
  check_only = gn_init(argc,argv);

#if ! defined(NO_NET)
  if ( gn.local_spool == OFF ) {
    /* �˥塼�������ФȤ���³  */
    printf(gn.nntpserver);
    mc_puts(" ����³���Ƥ��ޤ�\n");
    open_server();
  }
#endif
#ifdef	USE_JNAMES
  jnOpen(gn.jnames_file, NULL);
#endif
  
  /* �����ƥ��֥ե�����μ��� */
  switch ( get_active() ) {
  case CONNECTION_CLOSED:
    mc_puts("\n�˥塼�������ФȤ���³���ڤ�Ƥ��ޤ�\n");
    return(1);
  case ERROR:
    return(1);
  }
  
  /* .newsrc �ե�������ɤ߹��� */
  if ( (newsrc_flag = get_newsrc()) == ERROR)
    return(1);
  
  /* �˥塼�����롼�פΥ����å� */
  check_group(check_only);
  
  
  /* �ƥ��롼�פ������μ��� */
  if ( gn.description == ON ) {
    switch ( get_newsgroups() ) {
    case CONNECTION_CLOSED:
      mc_puts("\n�˥塼�������ФȤ���³���ڤ�Ƥ��ޤ�\n");
      return(1);
    case ERROR:
      return(1);
    }
  }
  
#if ! defined(QUICKWIN) && ! defined(WINDOWS)
  /* �����ʥ���� */
  signal(SIGINT,SIG_IGN);
#endif
  
  /* ̤�ɵ����Υ����å������ξ�� */
  if ( check_only ) {
    /* �˥塼�����롼�פ�ɽ�� */
    disp_group();
#if ! defined(NO_NET)
    if ( gn.local_spool == 0 )
      close_server();		/* �����ФȤ���³���ڤ� */
#endif
    /* ��λ */
    return(0);
  }
  
  /* �˥塼�����롼�ץ⡼�� */
  switch ( group_mode()) {
  case CONNECTION_CLOSED:
    mc_puts("\n�˥塼�������ФȤ���³���ڤ�ޤ���\n");
    if ( change ) {
      char *pt;
      pt = kb_input("���ɾ������¸���ޤ�����(n:���ʤ�/����¾:����)");
      if ( *pt != 'n' && *pt != 'N' )
	save_newsrc();
    }
    break;
  case END:
    if ( change )
      save_newsrc();		/* .newsrc �򹹿� */
#if ! defined(NO_NET)
    if ( gn.local_spool == OFF )
      close_server();		/* �����ФȤ���³���ڤ� */
#endif
    break;
  case QUIT:
#if ! defined(NO_NET)
    if ( gn.local_spool == OFF )
      close_server();		/* �����ФȤ���³���ڤ� */
#endif
    break;
  }
  
  /* �Ǹ���ɤ������������мΤƤ� */
  if ( last_art_check() != ERROR && make_article_tmpfile == ON )
    unlink(last_art.tmp_file);
  
#ifdef	USE_JNAMES
  jnClose();
#endif
#ifdef	QUICKWIN
  _wsetexit(_WINEXITNOPERSIST);
#endif
#if !defined(MSDOS) && !defined(OS2)
  Exit(0);
#endif
  return(0);
}
/*
 * ̤�ɵ����Υ����å������ξ���ɽ��
 */
static void
disp_group()
{
  register newsgroup_t *ng;
  register int col;
  
  printf("\n");
  for ( ng = newsrc; ng != 0 ; ng=ng->next) {
    if ( ng->flag & (UNSUBSCRIBE | NOARTICLE)) /* ���ɡ������ʤ� */
      continue;
    if ( ng->last <= ng->read ){ /* �����������ʤ� */
      ng->flag |= NOARTICLE;
      ng->numb = 0;
      continue;
    }
    printf("%4ld %s",ng->numb,ng->name);
    if ( gn.description == 1 && ng->desc != 0) {
      col = 5 + strlen(ng->name) + 1;
      printf(" ");
      if ( col < COLPOS ) {
	printf("%*s",COLPOS-col,"");
	col = COLPOS;
      }
      printf("%.*s",gn.columns-col,ng->desc);
    }
    printf("\n");

  }
}
#endif	/* WINDOWS */

/*
 * ��ư���ץ����β���(gn)
 */
int
read_option(argc,argv,check_only)
int argc;
char *argv[];
int	*check_only;
{
  extern int getopt();
  
  extern char *optarg;
  
  int c;
  char *optopt;
#ifdef	WINDOWS
  char buf[128];
#endif	/* WINDOWS */
  int l_flag,s_flag;
  
#if defined(MSDOS) || defined(OS2)
#ifdef	WINDOWS
  optopt = "h:d:gns";
#else	/* WINDOWS */
  optopt = "h:d:gnsV";
#endif	/* WINDOWS */
#else	/* DOS */
  optopt = "h:d:gnlsV";
#endif	/* MSDOS || OS2 */


  l_flag = s_flag = OFF;
  
  /* ��ư���ץ����β��� */
  
  while ((c = getopt(argc, argv, optopt)) != EOF) {
    switch (c) {
    case 'h':			/* �˥塼�������Ф���ꤹ�� */
      if ( gn.nntpserver != 0 )
	free(gn.nntpserver);
      if ( (gn.nntpserver = malloc(strlen(optarg)+1)) == NULL ) {
	memory_error();
	return(ERROR);
      }
      strcpy(gn.nntpserver,optarg);
      break;

    case 'd':			/* �ɥᥤ��̾����ꤹ�� */
      if ( gn.domainname != 0 )
	free(gn.domainname);
      if ( (gn.domainname = malloc(strlen(optarg)+1)) == NULL ) {
	memory_error();
	return(ERROR);
      }
      strcpy(gn.domainname,optarg);
      break;

#ifndef	WINDOWS
    case 'V':
      printf("%s Version %s %s.  Copyright (C) yamasita@omronsoft.co.jp\n",
	     argv[0],gn_version,gn_date);
      break;
#endif /* WINDOWS */

    case 'g':			/* upper compatible */
      gn.gnus_format = 1;
      break;

    case 'n':
      *check_only = QUIT;
      break;

    case 'l':
      if ( s_flag == ON ) {
#ifndef	WINDOWS
	mc_puts("-l ���ץ����� -s ���ץ�����Ʊ���ˤϻȤ��ޤ��� \n");
	exit(5);
#else  /* WINDOWS */
	mc_MessageBox(hWnd,"-l ���ץ����� -s ���ץ�����Ʊ���ˤϻȤ��ޤ��� ","",MB_OK);
	return(ERROR);
#endif /* WINDOWS */
      }
      l_flag = ON;
      gn.local_spool = ON;
      gn.use_history = ON;
      break;

    case 's':
      if ( l_flag == ON ) {
#ifndef	WINDOWS
	mc_puts("-s ���ץ����� -l ���ץ�����Ʊ���ˤϻȤ��ޤ��� \n");
	exit(5);
#else  /* WINDOWS */
	mc_MessageBox(hWnd,"-s ���ץ����� -l ���ץ�����Ʊ���ˤϻȤ��ޤ��� ","",MB_OK);
	return(ERROR);
#endif /* WINDOWS */
      }
      s_flag = ON;
      gn.with_gnspool = ON;
      gn.local_spool = ON;
      break;

    default:
#ifndef	WINDOWS
      mc_printf("%s [-h nntpserver] [-d domainname][-nlsV]\n",argv[0]);

      mc_puts("   -h nntpserver �˥塼�������Ф���ꤹ�� \n");
      mc_puts("   -d domainname �ɥᥤ��̾����ꤹ�� \n");
      mc_puts("   -n            ̤�ɤε�������ɽ�����ƽ�λ \n");
      mc_puts("   -l            �����륹�ס���⡼�� \n");
      mc_puts("   -s            gnspool ���Ȥ߹�碌�ƻ��Ѥ��� \n");
      mc_puts("   -V            �С�������ɽ�� \n");
      exit(5);
#else  /* WINDOWS */
      sprintf(buf,"%c: unknown option",c);
      mc_MessageBox(hWnd,buf,"",MB_OK);
      return(ERROR);
#endif /* WINDOWS */
    }
  }
  return(CONT);
}
