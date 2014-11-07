/*
 * @(#)newsrc.c	1.36 Mar.27,1997
 *
 * ������������ޤ��󡣤�������������Ū�ʳ��λ��ѡ����ۤ����¤��ߤ��ޤ���
 * Copyright (C) yamasita@omronsoft.co.jp
 */

#if defined(WINDOWS)
#	include	<windows.h>
#endif	/* WINDOWS */

#include	<stdio.h>

#ifdef	HAS_STRING_H
#	include	<string.h>
#endif
#ifdef	HAS_STDLIB_H
#	include	<stdlib.h>
#endif
#ifdef	HAS_MALLOC_H
#	include	<malloc.h>
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

extern void mc_puts(),mc_printf();
extern void hit_return();
extern void memory_error();

static int newsrc_make_gnus();
static void bad_newsrc_line();

/*
 * .newsrc ���ɤ߹���
 */
int
get_newsrc()
{
  extern void add_new_newsgroups();

  FILE *fp;
  newsgroup_t *newsrc_new;	/* �˥塼�����롼�ץꥹ�� */
  register newsgroup_t **ng;
  register newsgroup_t **ng_new;
  register newsgroup_t *ng_p,*ng_tmp;
  char resp[NEWSRC_LEN+1];
  register char *pt;
  char rc[NEWSRC_LEN+1];
  register int line = 0;
  int ng_name_len;
  int flag;
  char rcfile_tmp[PATH_LEN+1];

  if ( strncmp(gn.rcfile,gn.home,strlen(gn.home)) == 0 ) {
    strcpy(rcfile_tmp,"~/");
    strcat(rcfile_tmp,&gn.rcfile[strlen(gn.home)+1]);
  } else {
    strcpy(rcfile_tmp,gn.rcfile);
  }

  /* .newsrc �Υ����ץ� */
  if ( (fp = fopen(gn.rcfile,"r")) == NULL ) {
    mc_printf("\n%s ���ߤĤ���ޤ��� \n",rcfile_tmp);
    change = ON;

    /* �����Ȥ��ʤ���� control/junk �� unsub �ˤ������*/
    if ( gn.ng_sort == NG_SORT_NO ) {
      for ( ng_p = newsrc; ng_p != 0 ; ng_p = ng_p->next) {
	if ((ng_p->name[0] == 'c' && strncmp(ng_p->name,"control",7) == 0 )||
	    (ng_p->name[0] == 'j' && strncmp(ng_p->name,"junk",4) == 0 )) {
	  ng_p->flag |= UNSUBSCRIBE;
	}
      }
      return(CONT);
    }

    /* �����Ȥ��� */
    mc_printf("�˥塼�����롼�פ򥽡��Ȥ��Ƥ��ޤ� \n");
    ng_p = newsrc;
    newsrc = 0;
    while ( ng_p != 0 ) {
      if ((ng_p->name[0] == 'c' && strncmp(ng_p->name,"control",7) == 0 )||
	  (ng_p->name[0] == 'j' && strncmp(ng_p->name,"junk",4) == 0 )) {
	ng_p->flag |= UNSUBSCRIBE;
      }
      for ( ng = &newsrc; *ng != 0; ng = &((*ng)->next)) {
	if ((*ng)->name[0] > ng_p->name[0] ||
	    strcmp((*ng)->name,ng_p->name) > 0 )
	  break;
      }
      ng_tmp = *ng;
      *ng = ng_p;
      ng_p = ng_p->next;
      (*ng)->next = ng_tmp;
    }
    return(CONT);
  }

  newsrc_new = newsrc;
  newsrc = 0;
  ng = &newsrc;

  mc_printf("%s ������ɾ�����ɤ߹���Ǥ��ޤ�\n",rcfile_tmp);

  while( 1 ) {
    line++;
    /* �����ɤ߹��� */
    if ( fgets(resp,NEWSRC_LEN,fp) == NULL ) {
      fclose(fp);
      break;
    }
    if ( (pt = strchr(resp,'\r')) != NULL )
      *pt = '\n';

    strcpy(rc,resp);

    /* UNSUBSCRIBE ��Ƚ�� */
    if ( (pt = strchr(resp,':') ) != NULL ) {
      flag = 0;
    } else if ( (pt = strchr(resp,'!') ) != NULL ) {
      flag = UNSUBSCRIBE;
    } else {
      bad_newsrc_line(line,rc,rcfile_tmp);
      continue;
    }

    /* �˥塼�����롼��̾ */
    *pt = 0;
    /* active ���椫��õ�� */
    ng_name_len = strlen(resp);
    for ( ng_new = &newsrc_new; *ng_new != 0 ; ng_new = &((*ng_new)->next)) {
      if ( (*ng_new)->numb != ng_name_len )
	continue;
      if ((*ng_new)->name[0] == resp[0] && strcmp((*ng_new)->name,resp)== 0 )
	break;
    }
    if ( *ng_new == 0 ) {
      mc_printf("\n%s �ϥ����ƥ��֤ǤϤ���ޤ��� \n",resp);
      change = ON;
      continue;
    }
    *ng = *ng_new;
    *ng_new = (*ng_new)->next;

    (*ng)->next = 0;
    (*ng)->flag |= flag;
    (*ng)->numb = 0;

    /* ���ڡ��������ɬ�� */
    pt++;
    if ( *pt != ' ' ) {
      if ( *pt == '\n' )
	(*ng)->read = 0;	/* ���ɤε������ʤ� */
      else
	bad_newsrc_line(line,rc,rcfile_tmp);
    } else {
      /* ���� */
      pt++;
      if ( *pt == 's') {	/* for WinVN */
	pt++;
	while( '0' <= *pt && *pt <= '9' )
	  pt++;
	pt++;
      }
      if ( *pt < '0' || '9' < *pt ) {
	/* ���ɤε������ʤ� */
	(*ng)->read = 0;
      } else {
	/* �ǽ�δ��ɤε����ֹ� */
	(*ng)->read = atol(pt);
	if ( (*ng)->read != 1) { /* GNUS format */
	  (*ng)->read = 0;
	  if ( newsrc_make_gnus(*ng,pt) == ERROR )
	    return(ERROR);
	} else {
	  while( '0' <= *pt && *pt <= '9' )
	    pt++;
	  switch ( *pt ) {
	  case '-':
	    pt++;
	    if ( *pt < '0' || '9' < *pt ){
	      bad_newsrc_line(line,rc,rcfile_tmp);
	      break;
	    }
	    /* 1-9999 format */
	    (*ng)->read = atol(pt);
	    while( '0' <= *pt && *pt <= '9' )
	      pt++;
	    switch ( *pt ) {
	    case ',':		/* GNUS format */
	      pt++;
	      if ( newsrc_make_gnus(*ng,pt) == ERROR )
		return(ERROR);
	      break;
	    case '\n':		/* gn format */
	      break;
	    default:
	      bad_newsrc_line(line,rc,rcfile_tmp);
	      break;
	    }
	    break;
	  case ',':		/* GNUS format */
	    pt++;
	    if ( newsrc_make_gnus(*ng,pt) == ERROR )
	      return(ERROR);
	    break;
	  case '\n':		/* �ǽ�ε����ֹ椷���ʤ� */
	    break;
	  default:
	    bad_newsrc_line(line,rc,rcfile_tmp);
	    break;
	  }
	}
      }
    }
    /* ���Ԥ� NEWSRC_LEN ʸ���ʾ�ξ�硢�ɤ����Ф� */
    while ( strchr(pt,'\n') == NULL ) {
      if ( fgets(resp,NEWSRC_LEN,fp) == NULL ) {
	fclose(fp);
	return(CONT);
      }
      pt = resp;
    }

    (*ng)->subj = 0;
    ng = &((*ng)->next);
  }

  add_new_newsgroups(newsrc_new);
  return(CONT);
}
/*
 * �˥塼�����롼�פ��ɲ�
 */
#define	C_GROUP	20
void
add_new_newsgroups(newsrc_new)
newsgroup_t *newsrc_new;	/* �ɲä���˥塼�����롼�ץꥹ�� */
{
  register int c_group = 0;
  register newsgroup_t **ng;
  register newsgroup_t *ng_p,*ng_tmp;
  int ng_name_len;
  register char *pt;
  
  if ( newsrc_new == 0 )
    return;
  
  for ( ng_p = newsrc_new; ng_p != 0 ; ng_p = ng_p->next) {
    c_group++;
    if ( c_group == C_GROUP ) {
      mc_printf("���̤Υ˥塼�����롼�פ��������줿�ΤǤ���ʾ��ɽ�����ޤ��� \n");
    } else if ( c_group < C_GROUP ) {
      mc_printf("�˥塼�����롼�� %s ������ޤ���\n",ng_p->name);
    }
    /* �������줿���롼�פ���� control/junk ������� unsub  */
    if ((ng_p->name[0] == 'c' && strncmp(ng_p->name,"control",7) == 0 )||
	(ng_p->name[0] == 'j' && strncmp(ng_p->name,"junk",4) == 0 )) {
      ng_p->flag |= UNSUBSCRIBE;
    }
  }
  
  switch ( gn.ng_sort ) {
  case NG_SORT_NO:		/* �����Ȥ��ʤ� */
    for ( ng = &newsrc; *ng != 0; ng = &((*ng)->next))
      ;
    (*ng) = newsrc_new;		/* �Ǹ���ɲ� */
    break;
  case NG_SORT_ALL:
    ng_p = newsrc_new;
    while ( ng_p != 0 ) {
      for ( ng = &newsrc; *ng != 0; ng = &((*ng)->next)) {
	if ((*ng)->name[0] > ng_p->name[0] ||
	    strcmp((*ng)->name,ng_p->name) > 0 )
	  break;
      }
      ng_tmp = *ng;
      *ng = ng_p;
      ng_p = ng_p->next;
      (*ng)->next = ng_tmp;
    }
    break;
  case NG_SORT_CATEGORY:
    ng_p = newsrc_new;
    while ( ng_p != 0 ) {
      ng_name_len = 0;
      pt = ng_p->name;
      while ( *pt != '.' && *pt != 0 ) {
	ng_name_len++;
	pt ++;
      }
      /* Ʊ��Υ��ƥ����õ�� */
      for ( ng = &newsrc; *ng != 0; ng = &((*ng)->next)) {
	if ((*ng)->name[0] != ng_p->name[0] )
	  continue;		/* ���ƥ��꤬�㤦 */
	if ( strncmp((*ng)->name,ng_p->name,ng_name_len) == 0 )
	  break;		/* Ʊ��Υ��ƥ��꤬���Ĥ��ä� */
      }

      /* Ʊ��Υ��ƥ�������������֤�õ�� */
      for ( ; *ng != 0; ng = &((*ng)->next)) {
	if ((*ng)->name[0] != ng_p->name[0] )
	  break;;		/* ���ƥ��꤬�㤦 */
	if ( strncmp((*ng)->name,ng_p->name,ng_name_len) != 0 )
	  break;		/* ���ƥ��꤬�㤦 */
	if ((*ng)->name[0] > ng_p->name[0] ||
	    strcmp((*ng)->name,ng_p->name) > 0 )
	  break;
      }

      ng_tmp = *ng;
      *ng = ng_p;
      ng_p = ng_p->next;
      (*ng)->next = ng_tmp;
    }
  }
}

/*
 * GNUS �ե����ޥåȹԤ���¸
 */
static int
newsrc_make_gnus(ng,pt)
register newsgroup_t *ng;
register char *pt;
{
  register char *pt2;
  register long st,ed,base;
  
  /* GNUS �ե����ޥåȤ� newsrc ���������Υ����å� */
  pt2 = pt;
  base = ng->read;
  while ( '0' <= *pt2 && *pt2 <= '9' ) {
    st = atol(pt2);		/* �ϰϳ����ֹ� */
    if ( st <= base ) {		/* ���ʲ� */
      ng->newsrc = 0;
      return(CONT);
    }
    while ( '0' <= *pt2 && *pt2 <= '9' )
      pt2++;

    switch ( *pt2 ) {
    case '-':			/* 999-999 */
      pt2++;
      ed = atol(pt2);		/* �ϰϽ�λ�ֹ� */
      while ( '0' <= *pt2 && *pt2 <= '9' )
	pt2++;
      if ( *pt2 == ',' )
	pt2++;
      if ( st >= ed ) {		/* �ϰϳ��Ϥ������礭�� */
	ng->newsrc = 0;
	return(CONT);
      }
      base = ed;
      break;
    default:			/* 999 */
      if ( *pt2 == ',' )
	pt2++;
      base = st;
      break;
    }
  }
  
  
  /* .newsrc �ιԤ���¸ */
  if ( (ng->newsrc = (char *)malloc(strlen(pt)+1)) == NULL ) {
    memory_error();
    return(ERROR);
  }
  strcpy(ng->newsrc,pt);
  if ( (pt2 = strchr(ng->newsrc,'\n')) != NULL )
    *pt2 = 0;
  if ( gn.gnus_format == 0 ) {
    mc_puts("GNUS format �Ȥ��ư����ޤ�\n");
    gn.gnus_format = 1;
  }
  return(CONT);
}

/*
 * �����Ԥηٹ�
 */
static void
bad_newsrc_line(line,resp,newsrcfile)
int line;			/* �� */
char *resp;		/* ���� */
char *newsrcfile;	/* newsrc �Υե�����̾ */
{
  mc_printf("%s �Υե����ޥåȤ˸�꤬����ޤ�\n",gn.rcfile);
  mc_printf("Line %d:%s\n",line,resp);
}

/*
 * .newsrc �Υ�����
 */
void
save_newsrc()
{
  FILE *fp;
  register newsgroup_t *ng;
  register char *pt;
  char rcfile_tmp[PATH_LEN+1];
  
  if ( strncmp(gn.rcfile,gn.home,strlen(gn.home)) == 0 ) {
    strcpy(rcfile_tmp,"~/");
    strcat(rcfile_tmp,&gn.rcfile[strlen(gn.home)+1]);
  } else {
    strcpy(rcfile_tmp,gn.rcfile);
  }
  mc_printf("\n���ɾ���� %s ����¸���ޤ�\n",rcfile_tmp);
  
  /* .newsrc �Υ����ץ� */
  if ( (fp = fopen(gn.rcfile,"w")) == NULL ) {
    mc_printf("\n %s �ι����˼��Ԥ��ޤ���\n",rcfile_tmp);
    return;
  }
  
  for ( ng = newsrc ; ng != 0 ; ng=ng->next) {
    fprintf(fp,"%s",ng->name);	/* ̾�� */
    if ( ng->flag & UNSUBSCRIBE ) { /* ���ɤʤ� */
      fprintf(fp,"! ");
      if ( ng->read != 0 && ng->read <= ng->first ) {
	fprintf(fp,"\n");
	continue;
      }
    } else {
      fprintf(fp,": ");
    }
    if ( (ng->first == 0 || ng->read <= 0) && ng->newsrc == 0 ){
      fprintf(fp,"\n");
      continue;
    }
    if ( ng->read >= 1 ) {
      fprintf(fp,"1");
      if ( ng->read != 1)
	fprintf(fp,"-%ld",ng->read);
    }

    /* GNUS format �� .newsrc */
    if ( gn.gnus_format == 0 || (pt = ng->newsrc) == 0 ) {
      fprintf(fp,"\n");
      continue;
    }
    if ( ng->first == 0 || ng->read <= 0 )
      fprintf(fp,"%s\n",pt);
    else
      fprintf(fp,",%s\n",pt);
  }
  fclose(fp);
}
