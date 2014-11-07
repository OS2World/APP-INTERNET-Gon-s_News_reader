/*
 * @(#)client.c	1.40 May.2,1998
 *
 * 著作権は放棄しません。ただし、営利目的以外の使用／配布に制限は設けません。
 * Copyright (C) yamasita@omronsoft.co.jp
 */
#if ! defined(NO_NET)
#ifndef lint
static char	*rcsid = "@(#)$Header: clientlib.c,v 1.16 91/01/12 15:12:37 sob Exp $";
#endif

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
/*
 * NNTP client routines.
 */

/*
 * Include configuration parameters only if we're made in the nntp tree.
 */

#if defined(WINDOWS)
#	include	<windows.h>
extern int mc_MessageBox();
#	ifdef WIN32
#		include <io.h>		/* for dup() */
#	endif
#endif	/* WINDOWS */

#ifdef NNTPSRC
#	include "../common/conf.h"
#endif /* NNTPSRC */

#include <stdio.h>
#include <ctype.h>
#ifndef SLIMTCP
#ifndef MVUX
#	include <sys/types.h>
#else  /* MVUX */
#	include <sys/vs_tcp_types.h>
typedef char * caddr_t;
#endif  /* MVUX */
#endif  /* SLIMTCP */

#ifdef	HAS_STRING_H
#	include	<string.h>
#endif
#ifdef	HAS_STDLIB_H
#	include	<stdlib.h>
#endif
#ifdef	HAS_UNISTD_H
#	include	<unistd.h>
#endif

#ifdef OS2
#     ifndef __EMX__
#             include <types.h>
#     endif   /* !__EMX__ */
#endif /* OS2 */

#ifdef	HAS_NETDB_H
#	include	<netdb.h>
#endif
#ifdef	HAS_SYS_SOCKET_H
#	include	<sys/socket.h>
#endif
#ifdef	HAS_NETINET_IN_H
#	include	<netinet/in.h>
#endif
#ifdef	HAS_ARPA_INET_H
#	include	<arpa/inet.h>
#endif

#ifdef TLI
#	include	<fcntl.h>
#	include	<tiuser.h>
#	include	<stropts.h>
#	include	<sys/socket.h>
#	ifdef WIN_TCP
#		include	<sys/in.h>
#	else
#		include	<netinet/in.h>
#	endif
#	define	IPPORT_NNTP	((unsigned short) 119)
#	include <netdb.h>	/* All TLI implementations may not have this */
#endif /* !TLI */

#ifdef INETBIOS
#	include	"inetbios.h"
extern int htons();
extern int socket();
extern int connect();
extern int send();
extern int recv();
extern int soclose();
#endif /* INETBIOS */

#ifdef PATHWAY
#	ifdef INTERNAL
#		include <socket/types.h>
#		include <socket/fcntl.h>
#		include <socket/socket.h>
#		include <socket/time.h>
#		include <socket/in.h>
#		include <socket/errno.h>
#		include <socket/netdb.h>
#	else /* INTERNAL */
#		include <sys/types.h>
#		include <fcntl.h>
#		include <sys/socket.h>
#		include <sys/time.h>
#		include <netinet/in.h>
#		include <sys/errno.h>
#		include <netdb.h>
#	endif /* INTERNAL */
#endif /* PATHWAY */


#ifdef WATTCP
#	include <tcp.h>
#endif /* defined(WATTCP) */

#if defined(WINSOCK)
#	include	<winsock.h>
	/* Novell lwp168 & Core Internet-Connect */
#	define	BUG_GETSERVBYNAME
#endif /* defined(winsock) */

#ifdef SLIMTCP
#	include <socket/sys/types.h>
#	include <socket/fcntl.h>
#	include <socket/sys/socket.h>
#	include <socket/sys/time.h>
#	include <socket/netinet/in.h>
#	include <socket/netdb.h>
#endif /* defined(SLIMTCP) */

#ifdef PCTCP4
#	include <sys/types.h>
#	include <fcntl.h>
#	include <sys/socket.h>
#	include <sys/time.h>
#	include <netinet/in.h>
#	include <netdb.h>
#endif /* defined(PCTCP4) */

#ifdef ESPX
#	include <network.h>
#	include <socket.h>
#endif /* defined(ESPX) */

#if defined(PCNFS)
#	include "pcnfs.h"
#	include <sys/socket.h>
#	include <netinet/in.h>
#	include <netdb.h>
#endif /* PCNFS */

#ifndef EXCELAN
#endif /* !EXCELAN */

#if defined(USG) || defined(MSDOS)
#	define	index	strchr
#	ifndef	NEWWATTCP
#		define        bcopy(a,b,c)   memcpy(b,a,c)
#		define        bzero(a,b)     memset(a,'\0',b)
#	endif /* NEWWATTCP */
#endif /* USG */


#ifdef EXCELAN
# define	IPPORT_NNTP	((unsigned short) 119)
#if __STDC__
int connect(int, struct sockaddr *);
unsigned short htons(unsigned short);
unsigned long rhost(char **);
int rresvport( int );
int socket( int, struct sockproto *, struct sockaddr_in *, int );
#endif
#endif

#ifdef DECNET
#	include <netdnet/dn.h>
#	include <netdnet/dnetdb.h>
#endif /* DECNET */

extern void mc_puts(),mc_printf();
static int put_server_1line();

#include "nntp.h"
#include "gn.h"

#ifdef WATTCP
#ifdef	NEWWATTCP
#	define SOCK_WAIT_ESTABLISHED(a,b,c,d)	sock_wait_established((sock_type *)a, b,c,d)
#	define SOCK_PUTS(a,b)	sock_puts((sock_type *)a,b)
#	define SOCK_FLUSH(a)	sock_flush((sock_type *)a)
#	define SOCK_WAIT_INPUT(a,b,c,d)	sock_wait_input((sock_type *)a,b,c,d)
#	define SOCK_FASTREAD(a,b,c)	sock_fastread((sock_type *)a,b,c)
#	define SOCK_CLOSE(a)	sock_close((sock_type *)a)
#	define SOCK_WAIT_CLOSED(a,b,c,d)	sock_wait_closed((sock_type *)a,b,c,d)

#else	/* NEWWATTCP */
#	define SOCK_WAIT_ESTABLISHED(a,b,c,d)	sock_wait_established(a,b,c,d)
#	define SOCK_PUTS(a,b)	sock_puts(a,b)
#	define SOCK_FLUSH(a)	sock_flush(a)
#	define SOCK_WAIT_INPUT(a,b,c,d)	sock_wait_input(a,b,c,d)
#	define SOCK_FASTREAD(a,b,c)	sock_fastread(a,b,c)
#	define SOCK_CLOSE(a)	sock_close(a)
#	define SOCK_WAIT_CLOSED(a,b,c,d)	sock_wait_closed(a,b,c,d)
#endif	/* NEWWATTCP */

#endif /* WATTCP */

#ifdef ESPX
#	define inet_addr(a)	a2n_ipaddr(a)
#endif /* ESPX */

#if 0
/*
 * getserverbyfile	Get the name of a server from a named file.
 *			Handle white space and comments.
 *			Use NNTPSERVER environment variable if set.
 *
 *	Parameters:	"file" is the name of the file to read.
 *
 *	Returns:	Pointer to static data area containing the
 *			first non-ws/comment line in the file.
 *			NULL on error (or lack of entry in file).
 *
 *	Side effects:	None.
 */

char *
getserverbyfile(file)
char	*file;
{
  register FILE	*fp;
  register char	*cp;
  static char	buf[256];
  char		*index();
  char		*getenv();
  char		*strcpy();
  
  if ((cp = getenv("NNTPSERVER")) != NULL) {
    (void) strcpy(buf, cp);
    return (buf);
  }
  
  if (file == NULL)
    return (NULL);
  
  fp = fopen(file, "r");
  if (fp == NULL)
    return (NULL);
  
  while (fgets(buf, sizeof (buf), fp) != NULL) {
    if (*buf == '\n' || *buf == '#')
      continue;
    cp = index(buf, '\n');
    if (cp)
      *cp = '\0';
    (void) fclose(fp);
    return (buf);
  }
  
  (void) fclose(fp);
  return (NULL);			 /* No entry */
}
#endif

/*
 * server_init  Get a connection to the remote news server.
 *
 *	Parameters:	"machine" is the machine to connect to.
 *
 *	Returns:	-1 on error
 *			server's initial response code on success.
 *
 *	Side effects:	Connects to server.
 *			"ser_rd_fp" and "ser_wr_fp" are fp's
 *			for reading and writing to server.
 */
int
server_init_com(machine,com_ser_rd_fp,com_ser_wr_fp, name, proto,line)
char	*machine;
socket_t *com_ser_rd_fp;
socket_t *com_ser_wr_fp;
char *name, *proto;
char *line;
{
  extern int get_tcp_socket();
  extern int get_server_com();
  
  int sockt_rd;
#if defined(SOCKTYPE_FILE)
  int sockt_wr;
#endif
  char	*index();

#if defined( DECNET )
  char	*cp;

  cp = index(machine, ':');
  if (cp && cp[1] == ':') {
    *cp = '\0';
    sockt_rd = get_dnet_socket(machine, name, proto);
  } else
    sockt_rd = get_tcp_socket(machine, name, proto);
#else /* defined( DECNET ) */
#if defined( WATTCP )
  sockt_rd = get_tcp_socket(machine, name, proto,com_ser_wr_fp);
#else /* defined( WATTCP ) */
  sockt_rd = get_tcp_socket(machine, name, proto);
#endif /* defined( WATTCP ) */
#endif /* defined( DECNET ) */
  
  if (sockt_rd < 0)
    return (-1);
  
  /*
   * Now we'll make file pointers (i.e., buffered I/O) out of
	 * the socket file descriptor.  Note that we can't just
	 * open a fp for reading and writing -- we have to open
	 * up two separate fp's, one for reading, one for writing.
	 */

#if	defined(SOCKTYPE_INT)
	*com_ser_rd_fp = *com_ser_wr_fp = sockt_rd;
#else
#if ! defined( WATTCP )
	if ((*com_ser_rd_fp = fdopen(sockt_rd, "r")) == NULL) {
		perror("server_init: fdopen #1");
		return (-1);
	}

#ifdef WIN32
	sockt_wr = _dup(sockt_rd);
#else
	sockt_wr = dup(sockt_rd);
#endif
#ifdef TLI
	if (t_sync(sockt_rd) < 0){	/* Sync up new fd with TLI */
    		t_error("server_init: t_sync");
		*com_ser_rd_fp = NULL;		/* from above */
		return (-1);
	}
#endif
	if ((*com_ser_wr_fp = fdopen(sockt_wr, "w")) == NULL) {
		perror("server_init: fdopen #2");
		*com_ser_rd_fp = NULL;		/* from above */
		return (-1);
	}
#endif /* ! defined(WATTCP) */
#endif /* defined(SOCKTYPE_INT) */

	/* Now get the server's signon message */
  
  (void) get_server_com(line, NNTP_BUFSIZE, com_ser_rd_fp);
  return (atoi(line));
}


/*
 * get_tcp_socket -- get us a socket connected to the news server.
 *
 *	Parameters:	"machine" is the machine the server is running on.
 *
 *	Returns:	Socket connected to the news server if
 *			all is ok, else -1 on error.
 *
 *	Side effects:	Connects to server.
 *
 *	Errors:		Printed via perror.
 */

#ifdef	WATTCP

#if defined(NO_DNS) /* copy from inetbios.[ch] */

#ifndef	NEWWATTCP
struct	hostent {
	char	*h_name;	/* official name of host */
	char	**h_aliases;	/* alias list */
	int h_addrtype;	/* host address type	*/
	int h_length;	/* length of address	*/
	unsigned long	h_addr;		/* host address	*/
};
static struct hostent *
gethostbyname( name )
char	*name;
{
  static	struct	hostent host_p;
  static	char	buff[128], *alias_p[8];
  FILE	*fp;
  char	*ep, pathname[64], **cp;
  int n;
  
  if ( ( ep = getenv( "HOSTS" ) )== NULL ) {
    mc_puts( "環境変数 HOSTS が見つかりません.\n\r" );
    return NULL;
  }
  strcpy( pathname, ep );
  strcat( pathname, "\\HOSTS" );
  if ( ( fp = fopen( pathname, "r" ) )== NULL ) {
    mc_puts( "HOSTS ファイルが見つかりません.\n\r" );
    return NULL;
  }
  while ( 1 ) {
    fgets( buff, 127, fp );
    if ( feof( fp ) )	break;
    if ( ferror( fp ) )	break;
    if ( *buff == '#' )	continue;
    /* host_p.h_addrtype = PF_INET;	*/
    host_p.h_length   = 4;
    host_p.h_addr     = inet_addr( buff );
    strtok( buff, "\t " );	/* dumy read ip address	*/
    host_p.h_name     = strtok( NULL, "\t \n\r" );
    n = 0;
    while( alias_p[n] = strtok( NULL, "\t \n\r" ) ) {
      if ( *alias_p[n] == '#' ) {
	*alias_p[n] = 0;
	break;
      } else {
	++n;
      }
    }
    host_p.h_aliases = alias_p;
    if ( strcmp( name, host_p.h_name ) == 0 )
      break;
    for ( cp = host_p.h_aliases ; *cp ; cp++ )
      if ( strcmp( name, *cp ) == 0 )
	break;
    if ( *cp )			/* FIX yamasita@omronsoft.co.jp	*/
      break;
    host_p.h_addr = 0;
  }
  fclose( fp );
  if ( host_p.h_addr ) return ( &host_p );
  else		     return NULL;
}

#else	/* NEWWATTCP */
/* wattcp の inet_addr() は int を <<24 していて正常に動作しない */
static unsigned long
inet_addr( addr )
char	*addr;
{
  char	work[16];
  long adrs;
  
  strncpy( work, addr, 15 );
  adrs = atol( strtok( work, "." ) )<<24;
  adrs += atol( strtok( NULL, "." ) )<<16;
  adrs += atol( strtok( NULL, "." ) )<<8;
  adrs += atol( strtok( NULL, "\t " ) );
  return(adrs);
}
static struct hostent *
gethostbyname( name )
char	*name;
{
  static	struct	hostent host_p;
  static	char	buff[128], *alias_p[8];
  FILE	*fp;
  char	*ep, pathname[64], **cp;
  int n;
  u_long	ul;
  
  if ( ( ep = getenv( "HOSTS" ) )== NULL ) {
    mc_puts( "環境変数 HOSTS が見つかりません.\n\r" );
    return NULL;
  }
  strcpy( pathname, ep );
  strcat( pathname, "\\HOSTS" );
  if ( ( fp = fopen( pathname, "r" ) )== NULL ) {
    mc_puts( "HOSTS ファイルが見つかりません.\n\r" );
    return NULL;
  }
  while ( 1 ) {
    fgets( buff, 127, fp );
    if ( feof( fp ) )	break;
    if ( ferror( fp ) )	break;
    if ( *buff == '#' )	continue;
    host_p.h_addrtype = AF_INET;
    host_p.h_length   = 4;
    host_p.h_addr     = (char *)malloc(sizeof(struct in_addr));
    ul = inet_addr(buff);
    movmem(&ul,host_p.h_addr,4);

    strtok( buff, "\t " );	/* dumy read ip address	*/
    host_p.h_name     = strtok( NULL, "\t \n\r" );
    n = 0;
    while( alias_p[n] = strtok( NULL, "\t \n\r" ) ) {
      if ( *alias_p[n] == '#' ) {
	*alias_p[n] = 0;
	break;
      } else {
	++n;
      }
    }
    host_p.h_aliases = alias_p;
    if ( strcmp( name, host_p.h_name ) == 0 )
      break;
    for ( cp = host_p.h_aliases ; *cp ; cp++ )
      if ( strcmp( name, *cp ) == 0 )
	break;
    if ( *cp )			/* FIX yamasita@omronsoft.co.jp	*/
      break;
    host_p.h_addr = 0;
  }
  fclose( fp );
  if ( host_p.h_addr ) return ( &host_p );
  else		     return NULL;
}
#endif
#endif	/* NO_DNS */

get_tcp_socket(machine, name, proto,s)
char	*machine;	/* remote host */
char *name, *proto;
tcp_Socket *s;
{
  struct services_t {
    char *service;
    word port_no;
  };
  static struct services_t services[] = {
    { "ftp",	21 },
    { "smtp",	25 },
    { "pop",	109 },
    { "pop3",	110 },
    { "nntp",	119 },
    { 0,0 },
  };
  static int init_flag = OFF;
  longword host;
  word port;
  int status;
  struct services_t *svc;
  
  if ( init_flag == OFF ) {
    init_flag = ON;
    sock_init();
  }
#if defined(NO_DNS)
  if ( isdigit(*machine) ) {
    host = inet_addr(machine);
  } else {
    struct	hostent *gethostbyname(), *hp;
    hp = gethostbyname(machine);
    if (hp == NULL) {
      fprintf(stderr, "%s: Unknown host.\n", machine);
      return (-1);
    }
#ifdef	NEWWATTCP
    host = *(unsigned long*)hp->h_addr;
#else  /* NEWWATTCP */
    host = hp->h_addr;
#endif /* NEWWATTCP */
  }
#else	/* name server */
  if ((host = resolve( machine )) == 0) {
    fprintf(stderr,"Could not resolve host '%s'\n", machine );
    return(-1);
  }
#endif
  for ( svc = services ; svc != 0 ; svc++ ) {
    if ( strcmp(name,svc->service) == 0 ) {
      port = svc->port_no;
      break;
    }
  }
  
  if ( gn.nntpport != 0 && strcmp(name,"nntp") == 0 )
    port = gn.nntpport;		/* gnrc で指定された NNTP port */
  else if ( gn.smtpport != 0 && strcmp(name,"smtp") == 0 )
    port = gn.smtpport;		/* gnrc で指定された SMTP port */
  else if ( gn.pop3port != 0 && strcmp(name,"pop3") == 0 )
    port = gn.pop3port;		/* gnrc で指定された POP3 port */

  if (!tcp_open( s, 0, host, port, NULL )) {
    puts("Sorry, unable to connect to that machine right now!");
    return(-1);
  }
  
  SOCK_WAIT_ESTABLISHED(s, sock_delay, NULL, &status);
  
 sock_err:
  switch (status) {
  case 1 :			/* foreign host closed */
    return(-1);
  case -1:			/* timeout */
    printf("ERROR: sock_wait_established() %s\n", sockerr(s));
    return(-1);
  }
  
  return(0);
}
#else	/* WATTCP */
int
get_tcp_socket(machine, name, proto)
char	*machine;	/* remote host */
char *name, *proto;
{
  int	s;
  struct	sockaddr_in sin;
#ifdef TLI
  char 	*t_alloc();
  struct	t_call	*callptr;
  /*
   * Create a TCP transport endpoint.
   */
  if ((s = t_open("/dev/tcp", O_RDWR, (struct t_info*) 0)) < 0){
    t_error("t_open: can't t_open /dev/tcp");
    return(-1);
  }
  if(t_bind(s, (struct t_bind *)0, (struct t_bind *)0) < 0){
    t_error("t_bind");
    t_close(s);
    return(-1);
  }
  bzero((char *) &sin, sizeof(sin));
  sin.sin_family = AF_INET;
  if ( gn.nntpport != 0 )
    sin.sin_port = htons((unsigned short)gn.nntpport);
  else
    sin.sin_port = htons(IPPORT_NNTP);
  if (!isdigit(*machine) ||
      (long)(sin.sin_addr.s_addr = inet_addr(machine)) == -1){
    struct	hostent *gethostbyname(), *hp;
    if((hp = gethostbyname(machine)) == NULL){
      fprintf(stderr,"gethostbyname: %s: host unknown\n",
	      machine);
      t_close(s);
      return(-1);
    }
    bcopy(hp->h_addr, (char *) &sin.sin_addr, hp->h_length);
  }
  /*
   * Allocate a t_call structure and initialize it.
   * Let t_alloc() initialize the addr structure of the t_call structure.
   */
  if ((callptr = (struct t_call *) t_alloc(s,T_CALL,T_ADDR)) == NULL){
    t_error("t_alloc");
    t_close(s);
    return(-1);
  }
  
  callptr->addr.maxlen = sizeof(sin);
  callptr->addr.len = sizeof(sin);
  callptr->addr.buf = (char *) &sin;
  callptr->opt.len = 0;			/* no options */
  callptr->udata.len = 0;			/* no user data with connect */
  
  /*
   * Connect to the server.
   */
  if (t_connect(s, callptr, (struct t_call *) 0) < 0) {
    t_error("t_connect");
    t_close(s);
    return(-1);
  }
  
  /*
   * Now replace the timod module with the tirdwr module so that
   * standard read() and write() system calls can be used on the
   * descriptor.
   */
  
  if (ioctl(s,  I_POP,  (char *) 0) < 0){
    perror("I_POP(timod)");
    t_close(s);
    return(-1);
  }
  
  if (ioctl(s,  I_PUSH, "tirdwr") < 0){
    perror("I_PUSH(tirdwr)");
    t_close(s);
    return(-1);
  }
  
#else /* !TLI */
#ifndef EXCELAN
#if defined(WINSOCK)
#ifndef	BUG_GETSERVBYNAME
  struct servent *sp;
#endif
  struct hostent *hp;
#else	/* WINSOCK */
  struct	servent *getservbyname(), *sp;
  struct	hostent *gethostbyname(), *hp;
#endif	/* WINSOCK */
#ifdef h_addr
  int	x = 0;
  register char **cp;
  static char *alist[1];
#endif /* h_addr */
#ifdef	INETBIOS
  int	x;
#endif	/* INETBIOS */
  
#if defined(WINSOCK)
  unsigned long PASCAL FAR inet_addr ();
#else	/* WINSOCK */
#	ifndef ESPX
#	ifndef WIN32
#	ifndef linux
  unsigned long inet_addr();
#	endif /* (!linux) */
#	endif /* (!WIN32) */
#	endif /* (!ESPX) */
#endif	/* WINSOCK */
  static struct hostent def;
  static struct in_addr defaddr;
  static char namebuf[ 256 ];
  
#ifndef	BUG_GETSERVBYNAME
  if ((sp = getservbyname(name, proto)) ==  NULL) {
#ifndef	WINDOWS
    fprintf(stderr, "%s/%s: Unknown service.\n",name, proto);
#else  /* WINDOWS */
    sprintf(namebuf, "%s/%s: Unknown service.%d",
	    name, proto,WSAGetLastError());
    mc_MessageBox(hWnd,namebuf,"",MB_OK);
#endif /* WINDOWS */
    return (-1);
  }
#endif
  /* If not a raw ip address, try nameserver */
  if (!isdigit(*machine) ||
      (long)(defaddr.s_addr = inet_addr(machine)) == -1)
    hp = gethostbyname(machine);
  else {
    /* Raw ip address, fake  */
    (void) strcpy(namebuf, machine);
    def.h_name = namebuf;
#ifdef h_addr
    def.h_addr_list = alist;
#endif
#ifdef INETBIOS
    def.h_addr = defaddr.s_addr;
#else
    def.h_addr = (char *)&defaddr;
#endif
    def.h_length = sizeof(struct in_addr);
    def.h_addrtype = AF_INET;
    def.h_aliases = 0;
    hp = &def;
  }
#ifdef	LWP
  if (hp == NULL) {
    if((defaddr.s_addr = rhost(&machine)) != -1) {
      (void) strcpy(namebuf, machine);
      def.h_name = namebuf;
#ifdef h_addr
      def.h_addr_list = alist;
#endif
      def.h_addr = (char *)&defaddr;
      def.h_length = sizeof(struct in_addr);
      def.h_addrtype = AF_INET;
      def.h_aliases = 0;
      hp = &def;
    } else {
      fprintf(stderr, "%s: Unknown host.\n", machine);
      return (-1);
    }
  }
#else	/* LWP */
  if (hp == NULL) {
#ifndef	WINDOWS
    fprintf(stderr, "%s: Unknown host.\n", machine);
#else  /* WINDOWS */
    sprintf(namebuf, "%s: Unknown host.", machine);
    mc_MessageBox(hWnd,namebuf,"",MB_OK);
#endif /* WINDOWS */
    return (-1);
  }
#endif	/* LWP */
  
  bzero((char *) &sin, sizeof(sin));
#if defined(PCNFS) ||  defined(WINSOCK)
  sin.sin_family = AF_INET;
#else
  sin.sin_family = hp->h_addrtype;
#endif
#if defined(INETBIOS)
  if ( gn.nntpport != 0 && strcmp(name,"nntp") == 0 )
    sin.sin_port = htons((unsigned short)gn.nntpport);
  else if ( gn.smtpport != 0 && strcmp(name,"smtp") == 0 )
    sin.sin_port = htons((unsigned short)gn.smtpport);
  else if ( gn.pop3port != 0 && strcmp(name,"pop3") == 0 )
    sin.sin_port = htons((unsigned short)gn.pop3port);
  else
    sin.sin_port = htons(sp->s_port);
#else	/* INETBIOS */
#if defined(WINSOCK)
#ifndef	BUG_GETSERVBYNAME
  if ( gn.nntpport != 0 && strcmp(name,"nntp") == 0 )
    sin.sin_port = htons((unsigned short)gn.nntpport);
  else if ( gn.smtpport != 0 && strcmp(name,"smtp") == 0 )
    sin.sin_port = htons((unsigned short)gn.smtpport);
  else if ( gn.pop3port != 0 && strcmp(name,"pop3") == 0 )
    sin.sin_port = htons((unsigned short)gn.pop3port);
  else
    sin.sin_port = htons((sp->s_port)>>8);
#else	/* BUG_GETSERVBYNAME */
  {
    struct services_t {
      char *service;
      short port_no;
    };
    static struct services_t services[] = {
      { "ftp",	21 },
      { "smtp",	25 },
      { "pop",	109 },
      { "pop3",	110 },
      { "nntp",	119 },
      { 0,0 },
    };
    struct services_t *svc;

    if ( gn.nntpport != 0 && strcmp(name,"nntp") == 0 ) {
      sin.sin_port = htons((unsigned short)gn.nntpport);
    } else if ( gn.smtpport != 0 && strcmp(name,"smtp") == 0 ) {
      sin.sin_port = htons((unsigned short)gn.smtpport);
    } else if ( gn.pop3port != 0 && strcmp(name,"pop3") == 0 ) {
      sin.sin_port = htons((unsigned short)gn.pop3port);
    } else {
      for ( svc = services ; svc != 0 ; svc++ ) {
	if ( strcmp(name,svc->service) == 0 ) {
	  sin.sin_port = htons(svc->port_no);
	  break;
	}
      }
    }
  }
#endif	/* BUG_GETSERVBYNAME */
#else	/* WINSOCK */
  if ( gn.nntpport != 0 && strcmp(name,"nntp") == 0 )
    sin.sin_port = htons((unsigned short)gn.nntpport);
  else if ( gn.smtpport != 0 && strcmp(name,"smtp") == 0 )
    sin.sin_port = htons((unsigned short)gn.smtpport);
  else if ( gn.pop3port != 0 && strcmp(name,"pop3") == 0 )
    sin.sin_port = htons((unsigned short)gn.pop3port);
  else
    sin.sin_port = sp->s_port;
#endif	/* WINSOCK */
#endif	/* INETBIOS */
#else /* EXCELAN */
  bzero((char *) &sin, sizeof(sin));
  sin.sin_family = AF_INET;
#endif /* EXCELAN */
  
  /*
   * The following is kinda gross.  The name server under 4.3
   * returns a list of addresses, each of which should be tried
   * in turn if the previous one fails.  However, 4.2 hostent
   * structure doesn't have this list of addresses.
   * Under 4.3, h_addr is a #define to h_addr_list[0].
   * We use this to figure out whether to include the NS specific
   * code...
   */
  
#ifdef	h_addr
#ifdef ESPX
  s = socket(AF_INET, SOCK_STREAM, 0);
  sin.sin_addr.s_addr = *(long *) hp->h_addr;
  if (connect(s, (char *) &sin, sizeof(sin)) < 0) {
    perror("connect");
    (void) close_s(s);
    return (-1);
  }
#else
  
  /* get a socket and initiate connection -- use multiple addresses */
  
  for (cp = hp->h_addr_list; cp && *cp; cp++) {
#if defined(WINSOCK)
    s = socket(PF_INET, SOCK_STREAM, 0);
#else
    s = socket(hp->h_addrtype, SOCK_STREAM, 0);
#endif
    if (s < 0) {
#ifdef	WINDOWS
      mc_MessageBox(hWnd,"ソケットの作成に失敗しました","",MB_OK);
#else  /* WINDOWS */
      perror("socket");
#endif /* WINDOWS */
      return (-1);
    }
    bcopy(*cp, (char *)&sin.sin_addr, hp->h_length);

    if (x < 0) {
#ifndef	WINDOWS
      fprintf(stderr, "trying %s\n", inet_ntoa(sin.sin_addr));
#else  /* WINDOWS */
      mc_printf(stderr, "trying %s\n", inet_ntoa(sin.sin_addr));
#endif /* WINDOWS */
    }
    x = connect(s, (struct sockaddr *)&sin, sizeof (sin));
    if (x == 0)
      break;
#if defined(WINSOCK)
#ifndef	WINDOWS
    fprintf(stderr, "connection to %s: ", inet_ntoa(sin.sin_addr));
    fprintf(stderr,"%d\n",WSAGetLastError());
#else  /* WINDOWS */
    sprintf(namebuf, "connection to %s: %d",
	    inet_ntoa(sin.sin_addr),WSAGetLastError());
    mc_MessageBox(hWnd,namebuf,"",MB_OK);
#endif /* WINDOWS */
    (void) closesocket(s);
#else
    fprintf(stderr, "connection to %s: ", inet_ntoa(sin.sin_addr));
    perror("");
    (void) close(s);
#endif
  }
  if (x < 0) {
#ifndef	WINDOWS
    fprintf(stderr, "giving up...\n");
#else  /* WINDOWS */
    sprintf(namebuf, "giving up...");
    mc_MessageBox(hWnd,namebuf,"",MB_OK);
#endif
    return (-1);
  }
#endif /* !ESPX */
#else	/* no name server */
#ifdef EXCELAN
  if ((s = socket(SOCK_STREAM,(struct sockproto *)NULL,&sin,SO_KEEPALIVE)) < 0)
    {
      /* Get the socket */
      perror("socket");
      return (-1);
    }
  bzero((char *) &sin, sizeof(sin));
  sin.sin_family = AF_INET;
  if ( gn.nntpport != 0 )
    sin.sin_port = htons((unsigned short)gn.nntpport);
  else
    sin.sin_port = htons(IPPORT_NNTP);
  /* set up addr for the connect */
  
  if ((sin.sin_addr.s_addr = rhost(&machine)) == -1) {
    fprintf(stderr, "%s: Unknown host.\n", machine);
    return (-1);
  }
  /* And then connect */
  
  if (connect(s, (struct sockaddr *)&sin) < 0) {
    perror("connect");
    (void) close(s);
    return (-1);
  }
#else /* not EXCELAN */
  if ((s = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
    perror("socket");
    return (-1);
  }
  
  /* And then connect */
  
#ifdef INETBIOS
  sin.sin_addr.s_addr = hp->h_addr;
#else
  bcopy(hp->h_addr, (char *) &sin.sin_addr, hp->h_length);
#endif
#ifdef	INETBIOS
  if ((x=connect(s, (struct sockaddr_in *) &sin, sizeof(sin))) < 0) {
    printf("connect return %d\n",x);
    return (-1);
  }
#else
  if (connect(s, (struct sockaddr *) &sin, sizeof(sin)) < 0) {
    perror("connect");
    (void) close(s);
    return (-1);
  }
#endif
  
#endif /* !EXCELAN */
#endif /* !h_addr */
#endif /* !TLI */
  return (s);
}
#endif /* !WATTCP */

#ifdef DECNET
/*
 * get_dnet_socket -- get us a socket connected to the news server.
 *
 *	Parameters:	"machine" is the machine the server is running on.
 *
 *	Returns:	Socket connected to the news server if
 *			all is ok, else -1 on error.
 *
 *	Side effects:	Connects to server.
 *
 *	Errors:		Printed via nerror.
 */

get_dnet_socket(machine)
char	*machine;
{
  int	s, area, node;
  struct	sockaddr_dn sdn;
  struct	nodeent *getnodebyname(), *np;
  
  bzero((char *) &sdn, sizeof(sdn));
  
  switch (s = sscanf( machine, "%d%*[.]%d", &area, &node )) {
  case 1:
    node = area;
    area = 0;
  case 2:
    node += area*1024;
    sdn.sdn_add.a_len = 2;
    sdn.sdn_family = AF_DECnet;
    sdn.sdn_add.a_addr[0] = node % 256;
    sdn.sdn_add.a_addr[1] = node / 256;
    break;
  default:
    if ((np = getnodebyname(machine)) == NULL) {
      fprintf(stderr,
	      "%s: Unknown host.\n", machine);
      return (-1);
    } else {
      bcopy(np->n_addr,
	    (char *) sdn.sdn_add.a_addr,
	    np->n_length);
      sdn.sdn_add.a_len = np->n_length;
      sdn.sdn_family = np->n_addrtype;
    }
    break;
  }
  sdn.sdn_objnum = 0;
  sdn.sdn_flags = 0;
  sdn.sdn_objnamel = strlen("NNTP");
  bcopy("NNTP", &sdn.sdn_objname[0], sdn.sdn_objnamel);
  
  if ((s = socket(AF_DECnet, SOCK_STREAM, 0)) < 0) {
    nerror("socket");
    return (-1);
  }
  
  /* And then connect */
  
  if (connect(s, (struct sockaddr *) &sdn, sizeof(sdn)) < 0) {
    nerror("connect");
    close(s);
    return (-1);
  }
  
  return (s);
}
#endif



/*
 * handle_server_response
 *
 *	Print some informative messages based on the server's initial
 *	response code.  This is here so inews, rn, etc. can share
 *	the code.
 *
 *	Parameters:	"response" is the response code which the
 *			server sent us, presumably from "server_init",
 *			above.
 *			"nntpserver" is the news server we got the
 *			response code from.
 *
 *	Returns:	-1 if the error is fatal (and we should exit).
 *			0 otherwise.
 *
 *	Side effects:	None.
 */
#if 0
handle_server_response(response, nntpserver)
int	response;
char	*nntpserver;
{
  switch (response) {
  case OK_NOPOST:		/* fall through */
    printf(
	   "NOTE: This machine does not have permission to post articles.\n");
    printf(
	   "      Please don't waste your time trying.\n\n");
  case OK_CANPOST:
    return (0);
    break;

  case ERR_ACCESS:

    printf(
	   "This machine does not have permission to use the %s news server.\n",
	   nntpserver);
    return (-1);
    break;

  default:
    printf("Unexpected response code from %s news server: %d\n",
	   nntpserver, response);
    return (-1);
    break;
  }
  /*NOTREACHED*/
}
#endif

/*
 * put_server -- send a line of text to the server, terminating it
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

int
put_server_com(string,com_ser_wr_fp)
char *string;
socket_t *com_ser_wr_fp;
{
  register char *pt,*pe;
  register int ret;

  pt = pe = string;

  while ( (pe = strchr(pt,'\n')) != NULL ) {
    *pe = '\0';
    if ( (ret = put_server_1line(pt,com_ser_wr_fp) ) != CONT )
      return(ret);
    pe++;
    pt = pe;
  }
  if ( (ret = put_server_1line(pt,com_ser_wr_fp) ) != CONT )
    return(ret);
}
static int
put_server_1line(string,com_ser_wr_fp)
char *string;
socket_t *com_ser_wr_fp;
{
#if	defined(INETBIOS) || defined(PATHWAY) || defined(SLIMTCP) || defined(PCTCP4) || defined(LWP) || defined(OS2)
  char cp[NNTP_BUFSIZE + 8];
  
  strcpy( cp, string );
  strcat( cp, "\r\n" );
  send( *com_ser_wr_fp, cp, strlen( cp ), 0 );
#else /* defined(INETBIOS || PATHWAY || SLIMTCP || PCTCP4 || LWP || OS2 ) */
#if defined( WATTCP )
  char cp[NNTP_BUFSIZE + 8];
  
  strcpy( cp, string );
  strcat( cp, "\r\n" );
  SOCK_PUTS(com_ser_wr_fp, (byte *)cp);
  SOCK_FLUSH(com_ser_wr_fp);
#else	/* WATTCP */
#if defined(WINSOCK)
  char cp[NNTP_BUFSIZE + 8];
  int ret;
  
  strcpy( cp, string );
  strcat( cp, "\r\n" );
  while (1) {
    if ( send(*com_ser_wr_fp,cp,strlen(cp),0) != SOCKET_ERROR)
      break;
    if ( (ret = WSAGetLastError()) == WSAEWOULDBLOCK )
      continue;
#ifndef	WINDOWS
    fprintf(stderr,"send %d\n",ret);
#else  /* WINDOWS */
    mc_printf("send %d\n",ret);
#endif /* WINDOWS */
    return(ret);
  }
#else	/* WINSOCK */
#if defined(ESPX)
  char cp[NNTP_STRLEN + 8];
  
  strcpy( cp, string );
  strcat( cp, "\r\n" );
  write_s( *com_ser_wr_fp, cp, strlen( cp ));
#else
  fprintf(*com_ser_wr_fp, "%s\r\n", string);
  (void) fflush(*com_ser_wr_fp);
#endif	/* defined(ESPX) */
#endif	/* defined(WINSOCK) */
#endif	/* defined(WATTCP) */
#endif  /* defined(INETBIOS || PATHWAY || SLIMTCP || PCTCP4 || LWP || OS2 ) */
#ifdef	COM_DEBUG
  mc_printf("< :%s:\n",string);
#endif /* COM_DEBUG */
  return(CONT);
}


/*
 * get_server -- get a line of text from the server.  Strips
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
int
get_server_com(string, size, com_ser_rd_fp)
char	*string;
int size;
socket_t *com_ser_rd_fp;
{
  register char *cp;
  char	*index();
  
#if defined(INETBIOS)
  int ret;
  if ( ( ret = recvs( *com_ser_rd_fp, string, size, 0 ) ) < 0 ) {
    printf( "recvs Return code %d\n", ret );
    return (-1);
  }
#else  /* defined( INETBIOS ) */
#if defined(OS2) || defined(PATHWAY) || defined(SLIMTCP) || defined(PCTCP4) || defined(LWP) || defined(WINSOCK)
#if defined(WINSOCK)
#	define	RECVERROR	0
#else
#	define	RECVERROR	-1
#endif
#define BUFLEN 1024
  int	ret;
  int	i = 0;
  char    c;
  static int     chars_in_buffer = 0;    /* # of characters in buffer */
  static char    buffer[BUFLEN];
  static char   *bp = buffer;
  
  while (i < size) {
    if (!chars_in_buffer) {
      /* If buffer is empty, get data from server at a time */
      ret = recv( *com_ser_rd_fp, buffer, BUFLEN, 0 );
      if ( ret <= RECVERROR ) {
	mc_printf( "recv Return code %d\n", ret );
	return (-1);
      }
      chars_in_buffer = ret;
      bp = buffer;
    }
    string[i++] = c = *bp;
    bp++;
    chars_in_buffer--;
    if (c == '\n') break;
  }
  
  string[i] = '\0';
#else /* defined(OS2 || PATHWAY || SLIMTCP || PCTCP4 || LWP) */
#if defined( WATTCP )
#	define BUFLEN 1024
#	define RETRY 2
  
  int status;
  int	i = 0;
  char    c;
  static int     chars_in_buffer = 0;    /* # of characters in buffer */
  static char    buffer[BUFLEN];
  static char   *bp = buffer;
  int retry = 0;
  
 sock_retry:
  while (i < size) {
    if ( chars_in_buffer == 0) {
      /* If buffer is empty, get data from server at a time */
      SOCK_WAIT_INPUT(com_ser_rd_fp,sock_delay, NULL, &status );
      chars_in_buffer = SOCK_FASTREAD(com_ser_rd_fp, buffer, BUFLEN );
      bp = buffer;
    }
    string[i++] = c = *bp;
    bp++;
    chars_in_buffer--;
    if (c == '\n')
      break;
  }
  
  string[i] = '\0';
#else /* defined(WATTCP) */
#if defined(ESPX)
  int ret;
  if ( ( ret = recvline( *com_ser_rd_fp, string, size) ) < 0 ) {
    printf( "recvline Return code %d\n", ret );
    return (-1);
  }
#else
  if (fgets(string, size, *com_ser_rd_fp) == NULL)
    return (-1);
#endif /* defined(ESPX) */
#endif /* defined(WATTCP) */
#endif /* defined(OS2 || PATHWAY || SLIMTCP || PCTCP4 || LWP) */
#endif /* defined(INETBIOS) */
  
  if ((cp = index(string, '\r')) != NULL)
    *cp = '\0';
  else if ((cp = index(string, '\n')) != NULL)
    *cp = '\0';
#if COM_DEBUG
  mc_printf("> :%s:\n",string);
#endif /* COM_DEBUG */
  return (0);
  
#if defined( WATTCP )
 sock_err:
  switch (status) {
  case 1 :			/* foreign host closed */
    return (-1);
  case -1:			/* timeout */
    printf("ERROR: sock_wait_input() %s\n", sockerr(com_ser_rd_fp));
    if ( ++retry < RETRY && sockerr(com_ser_rd_fp) == 0 )
      goto sock_retry;
    return (-1);
  }
  return(0);
#endif
}


/*
 * close_server -- close the connection to the server, after sending
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
close_server_com(com_ser_rd_fp,com_ser_wr_fp)
socket_t *com_ser_rd_fp;
socket_t *com_ser_wr_fp;
{
  char	ser_line[256];
#ifdef	WATTCP
  int status;
#endif
  
#if	!defined(PATHWAY) && !defined(SLIMTCP) && !defined(PCTCP4) && !defined(LWP) && !defined(WINSOCK)
#ifndef	WATTCP
#if	defined(INETBIOS) || defined(OS2) || defined(ESPX)
  if (*com_ser_wr_fp == 0 || *com_ser_rd_fp == 0 )
    return;
#else
  if (*com_ser_wr_fp == NULL || *com_ser_rd_fp == NULL)
    return;
#endif
#endif	/* !WATTCP */
#endif	/* !PATHWAY !LWP */
  
  put_server_com("QUIT", com_ser_wr_fp);
  (void) get_server_com(ser_line, sizeof(ser_line), com_ser_rd_fp);
  
#if	defined(INETBIOS) || defined(PATHWAY) || defined(SLIMTCP) || defined(PCTCP4) || defined(LWP) || defined(OS2)
#if	defined(__EMX__)
  close( *com_ser_rd_fp );
#else
  soclose( *com_ser_rd_fp );
#endif
#else /* defined(INETBIOS || PATHWAY || SLIMTCP || PCTCP4 || LWP || OS2) */
#if defined( WATTCP )
  SOCK_CLOSE(com_ser_rd_fp);
  SOCK_WAIT_CLOSED(com_ser_rd_fp, sock_delay, NULL, &status);
 sock_err:
  switch (status) {
  case 1 :			/* foreign host closed */
    break;
  case -1:			/* timeout */
    printf("ERROR: sock_wait_close() %s\n", sockerr(com_ser_rd_fp));
    break;
  }
#else /* defined( WATTCP ) */
#if defined(WINSOCK)
  closesocket(*com_ser_rd_fp);
#else
#if defined(ESPX)
  close_s(*com_ser_wr_fp);
#else
  (void) fclose(*com_ser_wr_fp);
  (void) fclose(*com_ser_rd_fp);
#endif /* defined(ESPX) */
#endif /* defined(WINSOCK) */
#endif /* defined(WATTCP) */
#endif /*  defined(INETBIOS || PATHWAY || SLIMTCP || PCTCP4 || LWP || OS2) */
}

#if defined(LWP)
int
find_inet()			/* TCPIP.EXE 組み込み確認 */
{
  return(loaded());
}
#endif	/* LWP */

#if defined(PCTCP4)
int
find_inet()			/* TCP/IP 組み込み確認	*/
{
  return(1);
}
int
soclose( s )
int	s;
{
  return ( close ( s ) );
}
#endif	/* PCTCP4 */

#if defined(PATHWAY) || defined(SLIMTCP)
static int	winit = 0;

int
find_inet()			/* TCP/IP 組み込み確認	*/
{
  if ( winit++ ) return(1);
  if ( win_init() ) {
    printf( "TCP/IP初期化エラー\n" );
    return(0);
  }
  
  if ( socket_init() ) {
    printf( "socketライブラリ初期化エラー\n" );
    return(0);
  }
  return(1);
}

int
soclose( s )
int	s;
{
  return ( sock_close ( s ) );
}
#endif /* defined(PATHWAY || SLIMTCP) */

#if defined(PATHWAY) || defined(SLIMTCP) || defined(PCTCP4)
unsigned long
inet_addr( addr )
char	*addr;
{
  char	adrs[8], work[16];
  
  strncpy( work, addr, 15 );
  adrs[0] = adrs[1] = adrs[2] = adrs[3] = 0;
  adrs[0] = (char)atoi( strtok( work, "." ) );
  adrs[1] = (char)atoi( strtok( NULL, "." ) );
  adrs[2] = (char)atoi( strtok( NULL, "." ) );
  adrs[3] = (char)atoi( strtok( NULL, "\t " ) );
  return *(u_long *)adrs;
}
#endif	/* defined(PATHWAY || SLIMTCP || PCTCP4) */

#endif	/* ! NO_NET */
