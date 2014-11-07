
/*	inetbios.c	InetBIOS support libraly	Ver 0.35
 *
 *	Copyleft(c)	1992,1993 sawada@murai-electric.co.jp
 *	Permit anyone to use, modify, redistribute this software.
 *	last modified at  5 July 1993 by sawada@murai-electric.co.jp
 */

#if ! defined(NO_NET)
#if defined(MSDOS) && defined(INETBIOS)

#include	<stdio.h>
#include	<stdlib.h>
#include	<string.h>
#include	<dos.h>
#include	"inetbios.h"

/* for gn */
#include	"nntp.h"
#include	"gn.h"
#define	puts(x)	mc_puts(x)
extern void mc_puts();
/* */

#ifdef peek
# undef peek
#endif

extern	int	debug;

int		find_inet( void );
int		htons( int );
int		ntohs( int );
int		socket( int, int, int );
int		connect( int, struct sockaddr_in *, int );
int		gpeek( int );
int		listen( int, int );
int		accept( int, struct sockaddr_in *, int * );
int		bind( int, struct sockaddr_in *, int );
int		_recv( int, char *, int, int );
int		recv( int, char *, int, int );
int		recvfrom( int, char *, int, int, struct sockaddr_in *, int * );
int		send( int, char *, int, int );
int		sendto( int, char *, int, int, struct sockaddr_in *, int * );
int		shutdown( int, int );
int		soclose( int );
int		sallclose( void );
int		gethostname( char *, int );
unsigned long	inet_addr( char * );
struct hostent	*gethostbyname( char * );
struct servent	*getservbyname( char *, char * );

#define	MAXCONNECTION	8
#ifdef GNUDOS
#define	TCP_BUFF_SIZE	32768			/* 受信バッファーサイズ	*/
#else
#define	TCP_BUFF_SIZE	(size_t)32768		/* 受信バッファーサイズ	*/
#endif

static	struct	TCB	{			/* Inet Bios 制御構造体	*/
			char	reserved[16];		/* 予約領域	*/
			unsigned char	command;	/* コマンド	*/
			unsigned char	result;		/* 処理結果	*/
			void		(* async)();	/* 非同期処理	*/
			unsigned char	complete;	/* 完了フラグ	*/
			unsigned char	adapter;	/* アダプタ番号	*/
			unsigned char	service;	/* サービスタイプ */
			unsigned char	padding;	/* 未使用	*/
			int		flags;		/* オプション	*/
			int		endpoint;	/* 端末識別子	*/
			int		connection;	/* 接続識別子	*/
			char *		prmbuff;	/* 第１buff番地 */
			unsigned int	prmlen;		/* 第１buff長	*/
			char *		scndbuff;	/* 第２buff番地 */
			unsigned int	scndlen;	/* 第２buff長	*/
			int		locleng;	/* アドレス長	*/
			u_short		locport;	/* ポート番号	*/
			u_long		locaddr;	/* IPアドレス	*/
			char		locaddrs[8];	/* IPアドレス	*/
			int		remleng;	/* アドレス長	*/
			u_short		remport;	/* ポート番号	*/
			u_long		remaddr;	/* IPアドレス	*/
			char		remaddrs[8];	/* IPアドレス	*/
			char		useflag;	/* 使用中フラグ	*/
#ifdef GNUDOS
			short		selector;	/* セレクタ	*/
			char *		address;	/* ポインタ	*/
			short		segment;	/* セグメント	*/
			char *		buffers;	/* ポインタ	*/
#endif
				} TCB[ MAXCONNECTION ];

#define	INETB_OPEN	1
#define	INETB_CLOSE	2
#define	INETB_CALL	3
#define	INETB_LISTEN	4
#define	INETB_HANGUP	5
#define	INETB_SEND	6
#define	INETB_CAPACITY	7
#define	INETB_RECV	8
#define	INETB_PEEK	9
#define	INETB_NO_WAIT	0x80

#define	SRV_DG		0
#define	SRV_STRM	5

static	int		InetVect = 0x7E;
static	int		loaded = 0;
static	char		*TCB_buff[MAXCONNECTION];
static	long		TCB_fpos[MAXCONNECTION];
#ifdef GNUDOS
static	long		TCB_fsiz[MAXCONNECTION];
#endif

#ifdef GNUDOS
char *
conventional_malloc( socket_num )
short	socket_num;
{
union	REGS	reg;
	char	*address;

	reg.x.ax = 0x0000FF48;
	reg.x.bx = ( TCP_BUFF_SIZE + 80 ) / 16;
	intdos( &reg, &reg );
	if ( reg.x.ax == 0 )	return 0;
	TCB[socket_num].selector = reg.x.ax;
	address	= (char *)( reg.x.bx + 0xE0000000 );
	TCB[socket_num].address = address;
	memset( address, 0, TCP_BUFF_SIZE + 80 );
	TCB[socket_num].segment = ( (int)address & 0x000FFFF0 ) >> 4;
	TCB[socket_num].buffers = address + 80;
	return address;
}

int
conventional_free( socket_num )
short	socket_num;
{
	short	selector, result;

	if ( selector = TCB[socket_num].selector ) {
asm volatile( "push %%es; movw %2,%%es; int $0x21; pop %%es"
		: "=a"(result) : "a"((short)0xFF49), "r"(selector) );
	    if ( result == 0 ) {
		TCB[socket_num].selector = 0;
		TCB[socket_num].address  = 0;
		TCB[socket_num].segment  = 0;
		TCB[socket_num].buffers  = 0;
	    }
	    return (int)result;
	}
	return 0;
}
#endif

int
find_inet()			/* InetBIOS 組み込み確認	*/
{
	char 	*cp, *ep;
#ifdef GNUDOS
	int	ad;
#endif

	if ( (ep = getenv( "INETVECT" )) != NULL )
	    sscanf( ep, "%x", &InetVect );
#ifdef GNUDOS
	ad = *(int *)( ( InetVect * 4 ) + 0xE0000000 );
	cp = (char *)( ( ( ad >> 12 ) & 0x000FFFF0 )
					+ ( ad & 0x0000FFFF ) | 0xE0000000 );
#else
	cp = *(char **)(InetVect * 4L);
#endif
#ifdef GNUDOS
	while( (int)cp & 0x000FFFFF ) {		/* Next table address	*/
#else
	while( cp ) {				/* Next table address	*/
#endif
	    if ( *( cp + 10 ) == 'A' && *( cp + 11 ) == 'S' &&
		  *( cp + 12 ) == 'C' && *( cp + 13 ) == '!' &&
		    *( cp + 14 ) == 'I' && *( cp + 15 ) == 'N' &&
		      *( cp + 16 ) == 'E' && *( cp + 17 ) == 'T' ) {
		loaded = 1;
		return 1;				/* Found	*/
	    }
#ifdef GNUDOS
	    ad = *(int *)( cp + 6 );
	    cp = (char *)( ( ( ad >> 12 ) & 0x000FFFF0 )
					| ( ad & 0x0000FFFF ) | 0xE0000000 );
#else
	    cp = *(char **)( cp + 6 );
#endif
	}
	return 0;					/* Not found	*/
}

int
htons( n )			/* CPU -> Network ordr		*/
int	n;
{
	return ( (( n & 0xFF00 ) >> 8 ) | (( n & 0x00FF ) << 8 ) ) ;
}

int
ntohs( n )			/* Network ordr -> CPU		*/
int	n;
{
	return ( (( n & 0xFF00 ) >> 8 ) | (( n & 0x00FF ) << 8 ) ) ;
}

int
tcb_print( tcb )
struct TCB	*tcb;
{
	unsigned char	*cp, adr[8];

	switch( tcb->command ) {
	    case INETB_OPEN:		cp = "OPEN";		break;
	    case INETB_CLOSE:		cp = "CLOSE";		break;
	    case INETB_CALL:		cp = "CALL";		break;
	    case INETB_LISTEN:		cp = "LISTEN";		break;
	    case INETB_HANGUP:		cp = "HANGUP";		break;
	    case INETB_SEND:		cp = "SEND";		break;
	    case INETB_CAPACITY:	cp = "CAPACITY";	break;
	    case INETB_RECV:		cp = "RECV";		break;
	    case INETB_PEEK:		cp = "PEEK";		break;
	}
	printf( "Command %-8s  Service %-3s  eid %d  cid %d  Result %d  ", cp,
				(tcb->service == SRV_STRM ? "TCP" : "UDP"),
				tcb->endpoint, tcb->connection, tcb->result );
	printf( "Length %d\n", tcb->prmlen );
	*(long *)adr = tcb->locaddr;
	printf( "LocADDR %d.%d.%d.%d  Port %d    ",
			adr[0], adr[1],	adr[2], adr[3], ntohs( tcb->locport ));
	*(long *)adr = tcb->remaddr;
	printf( "RemADDR %d.%d.%d.%d  Port %d\n",
			adr[0], adr[1],	adr[2], adr[3], ntohs( tcb->remport ));
	printf( "Data %-70.70s\n", tcb->prmbuff );
	return 0;
}

int
ib_request( tcb )		/* Inet BIOS コール		*/
struct TCB	*tcb;
{
union	REGS	reg;
struct	SREGS	sreg;

#ifdef GNUDOS
	short	selector, result;
	char	*s, *d;
	short	n;

#ifdef DEBUGGING
	if ( debug & 32768 ) {
	    printf( "Inet Call\n" );	tcb_print( tcb );
	}
#endif
	*( tcb->address + 16 ) = tcb->command;
	*( tcb->address + 23 ) = tcb->adapter;
	*( tcb->address + 24 ) = tcb->service;
	*(unsigned short *)( tcb->address + 26 ) = tcb->flags;
	*(unsigned short *)( tcb->address + 28 ) = tcb->endpoint;
	*(unsigned short *)( tcb->address + 30 ) = tcb->connection;
	*(unsigned short *)( tcb->address + 32 ) = 80;
	*(unsigned short *)( tcb->address + 34 ) = tcb->segment;
	*(unsigned short *)( tcb->address + 36 ) = tcb->prmlen;
	if ( tcb->command == INETB_SEND ) {
	    memcpy( tcb->buffers, tcb->prmbuff, tcb->prmlen );		/* */
	}
	*(unsigned short *)( tcb->address + 44 ) = tcb->locleng;
	*(unsigned short *)( tcb->address + 46 ) = tcb->locport;
	*(unsigned int   *)( tcb->address + 48 ) = tcb->locaddr;
	*(unsigned short *)( tcb->address + 60 ) = tcb->remleng;
	*(unsigned short *)( tcb->address + 62 ) = tcb->remport;
	*(unsigned int   *)( tcb->address + 64 ) = tcb->remaddr;
	selector = tcb->selector;

asm( "push %%esi; push %%edi; push %%es; movw %1,%%es; int $0x7f; pop %%es; pop %%edi; pop %%esi"
					: :"b"((short)0), "r"(selector));

	tcb->remaddr	= *(unsigned int   *)( tcb->address + 64 );
	tcb->remport	= *(unsigned short *)( tcb->address + 62 );
	tcb->remleng	= *(unsigned short *)( tcb->address + 60 );
	tcb->locaddr	= *(unsigned int   *)( tcb->address + 48 );
	tcb->locport	= *(unsigned short *)( tcb->address + 46 );
	tcb->locleng	= *(unsigned short *)( tcb->address + 44 );
	tcb->prmlen	= *(unsigned short *)( tcb->address + 36 );
	if ( tcb->command == INETB_RECV ) {
	    memcpy( tcb->prmbuff, tcb->buffers, tcb->prmlen );		/* */
	}
	tcb->connection = *(unsigned short *)( tcb->address + 30 );
	tcb->endpoint   = *(unsigned short *)( tcb->address + 28 );
	tcb->flags      = *(unsigned short *)( tcb->address + 26 );
	tcb->complete   = *( tcb->address + 22 );
	tcb->result     = *( tcb->address + 17 );
#else
	char	*p;

	p = (char *)tcb->reserved;
	sreg.es  = FP_SEG( p );
	reg.x.bx = FP_OFF( p );
	int86x( InetVect + 1, &reg, &reg, &sreg );
#endif
#ifdef DEBUGGING
	if ( debug & 32768 ) {
	    printf( "Inet Return\n" );	tcb_print( tcb );
	}
#endif
	return (int)tcb->result ;
}

int
socket( domain, type, protocol )
int	domain, type, protocol;
{
static	int	endflag = 0;
	int	n;

	if ( domain != PF_INET )	return -2;	/* PF_INET onry	*/
	if ( loaded == 0 )
	    if ( find_inet() == 0 )
		return -9;			/* InetBIOS not loaded	*/
	for ( n=0 ; n<MAXCONNECTION ; n++ ) {
	    if ( TCB[ n ].useflag == 0 )
		break;
	}
	if ( n == MAXCONNECTION )	return -3;
	switch( type ) {
	    case SOCK_STREAM:	TCB[ n ].service = SRV_STRM;	break;
	    case SOCK_DGRAM:	TCB[ n ].service = SRV_DG;	break;
	    default:			return -4;
	}
	if ( endflag == 0 ) {
#if defined(__TURBOC__) || defined(GNUDOS)
	    if ( atexit( (void (*)())sallclose ) != 0 )	/* 終了関数登録	*/
#else
	    if ( onexit( sallclose ) == NULL )		/* 終了関数登録	*/
#endif
					return -5;
	    endflag = 1;
	}							/* */
#ifdef GNUDOS
	if ( TCB[ n ].selector == 0 )
	    if ( conventional_malloc( n ) == 0 )
					return -6;
#endif
	TCB[ n ].useflag = 1;
	return ( n + 1 );
}

int
connect( s, name, namelen )
int	s;
struct sockaddr_in *name;
int	namelen;
{
	int	n;

	if ( name->sin_family != PF_INET ) return -2;	/* PF_INET onry	*/
	n = s - 1;
	if ( n >= MAXCONNECTION )	return -3;
	if ( TCB[ n ].useflag == 0 )	return -8;
	if ( TCB[ n ].endpoint == 0 ) {
	    TCB[ n ].command = INETB_OPEN;		/* ポートオープン */
	    if ( ib_request( &TCB[ n ] ) )	return -1;
	}
	TCB[ n ].command  = INETB_CALL;			/* ポート接続要求 */
	TCB[ n ].remleng  = 8;
	TCB[ n ].remport  = name->sin_port;
	TCB[ n ].remaddr  = name->sin_addr.s_addr;
	if ( ib_request( &TCB[ n ] ) )	return -1;	/* Connect error */
	return TCB[ n ].connection;
}

int
gpeek( s )
int	s;
{
	int	n;

	n = s - 1;
	if ( n >= MAXCONNECTION )	return -3;
	if ( TCB[ n ].useflag == 0 )	return -8;
	TCB[ n ].command = INETB_PEEK;			/* PEEK command */
	if ( ib_request( &TCB[ n ] ) )	return -1;	/* Peek error	*/
	if ( TCB[ n ].result == 58 )	return 0;	/* EShutdown	*/
	return TCB[ n ].prmlen;
}

int
listen( s, backlog )
int	s, backlog;
{
	int	n;

	n = s - 1;
	if ( n >= MAXCONNECTION )	return -3;
	if ( TCB[ n ].useflag == 0 )	return -8;
	if ( backlog > 5 )		return -5;
	if ( TCB[ n ].service != SRV_STRM )	return -4;
	TCB[ n ].command = INETB_LISTEN;		/* LISN command */
	if ( ib_request( &TCB[ n ] ) )	return -1;	/* Lisn error	*/
	if ( TCB[ n ].result == 58 )	return 0;	/* EShutdown	*/
	return TCB[ n ].result;
}

int
accept( s, name, namelen )
int	s;
struct sockaddr_in *name;
int	*namelen;
{
	int	n;

	n = s - 1;
	if ( n >= MAXCONNECTION )	return -3;
	if ( TCB[ n ].useflag == 0 )	return -8;
	name->sin_port = TCB[ n ].remport;
	name->sin_addr.s_addr = TCB[ n ].remaddr;
	*namelen = 8;
#ifdef DEBUGGING
	if ( debug & 32768 ) {
	    printf( "Inet Accept\n" );	tcb_print( &TCB[n] );	getch();
	}
#endif
	return s;
}

int
bind( s, name, namelen )
int	s;
struct sockaddr_in *name;
int	namelen;
{
	int	n;
#if 0	/* yamasita */
	unsigned long	address;
#endif

#ifdef DEBUGGING
	if ( debug & 32768 )
	    printf( "Inet Bind\n" );
#endif
	n = s - 1;
	if ( n >= MAXCONNECTION )	return -3;
	if ( TCB[ n ].useflag == 0 )	return -8;
	TCB[ n ].locport = name->sin_port;
	TCB[ n ].locaddr = name->sin_addr.s_addr;
	TCB[ n ].command = INETB_OPEN;		/* ポートオープン要求	*/
	if ( ib_request( &TCB[ n ] ) )	return -1;	/* Open error	*/
	return 0;
}

int
_recv( s, buf, len, flags )
int	s;
char	*buf;
int	len, flags;
{
	int	n;

	n = s - 1;
	if ( n >= MAXCONNECTION )	return -3;
	if ( TCB[ n ].useflag == 0 )	return -8;
	TCB[ n ].command = INETB_RECV;			/* RECV command */
	TCB[ n ].flags   = flags;
	TCB[ n ].prmlen  = len;
	TCB[ n ].prmbuff = buf;
	ib_request( &TCB[ n ] );
	if ( TCB[ n ].result ) {
	    if ( TCB[ n ].result == 58 )	return 0;  /* EShutdown	*/
	    return -1;					/* Recv error	*/
	}
	return TCB[ n ].prmlen;
}

int
recv( s, buf, len, flags )
int	s;
char	*buf;
int	len, flags;
{
#ifdef GNUDOS
	int	n;
	unsigned int l;
	char	*cp, *mp;

	n = s - 1;
	if ( n >= MAXCONNECTION )	return -3;
	while( 1 ) {
	    l = gpeek( s );
	    if ( l > 0 ) {
		if ( ( mp = malloc( TCP_BUFF_SIZE ) )== NULL )	return -2;
		if ( ( l = _recv( s, mp, TCP_BUFF_SIZE, 0 ) ) < 0 )  return l;
		if ( TCB_buff[ n ] != NULL ) {
		    TCB_buff[ n ] = realloc( TCB_buff[n], TCB_fsiz[n] + l );
		    memcpy( TCB_buff[n]+TCB_fsiz[n], mp, l );
		    TCB_fsiz[ n ] = TCB_fsiz[ n ] + l;
		    free( mp );
		} else {
		    TCB_buff[ n ] = realloc( mp, l );
		    TCB_fpos[ n ] = 0;
		    TCB_fsiz[ n ] = l;
		}
	    }
	    if ( TCB_buff[ n ] != NULL )  {
		cp = TCB_buff[ n ] + TCB_fpos[ n ];
		for( l=0 ; l<len && TCB_fpos[n]+l<TCB_fsiz[n] ; l++, cp++ ) {
		    if (( buf[ l ] = *cp )== '\n')
			break;
		}
		if ( *cp == '\n' ) {
		    buf[ ++l ] = 0;
		    if ( TCB_fpos[ n ] + l == TCB_fsiz[ n ] ) {
			free( TCB_buff[ n ] );
			TCB_buff[ n ] = NULL;
			TCB_fpos[ n ] = 0;
			TCB_fsiz[ n ] = 0;
		    } else {
			TCB_fpos[ n ] = TCB_fpos[ n ] + l;
		    }
		    return (int)strlen( buf );
		}
	    }
	}
#else
	FILE	*fp;
	int	n;
	unsigned int l;
	char	*cp, *mp;

	n = s - 1;
	if ( n >= MAXCONNECTION )	return -3;
	while( 1 ) {
	    l = gpeek( s );
	    if ( TCB_buff[ n ] == NULL ) {
		if ( ( cp = malloc( 64 + 1 ) )== NULL )	return -2;
		*cp = 0;
#if 0	/* for gn Feb.10,'94 yamasita@omronsoft.co.jp */
		if ( (mp = getenv( "TMP" )) != NULL )
		    strcpy( cp, mp );
#else
		mp = gn.tmpdir;
	    strcpy( cp, mp );
#endif		
		strcat( cp, "\\$$$inet.$$$"  );
		cp[ strlen( cp ) - 2 ] = '0' + n;
		mp = cp;
		while( *mp ) {
		    if ( mp[0] == '\\' && mp[1] == '\\' )
			strcpy( mp, mp + 1 );
		    ++mp;
		}
		TCB_buff[ n ] = cp;
	    }
	    if ( l > 0 ) {
		if ( ( mp = malloc( TCP_BUFF_SIZE ) )== NULL )	return -2;
		if ( ( l = _recv( s, mp, TCP_BUFF_SIZE, 0 ) ) < 0 )  return l;
		if ( ( fp = fopen( TCB_buff[ n ], "ab" ) )== NULL ) return -4;
		fwrite( mp, l, 1, fp );
		fclose( fp );
		free( mp );
	    }
	    if ( ( fp = fopen( TCB_buff[ n ], "rb" ) )!= NULL )  {
		fseek( fp, TCB_fpos[ n ], SEEK_SET );
		*buf = 0;
		fgets( buf, len, fp );
		if ( strchr( buf, '\n' ) != NULL )  {
		    TCB_fpos[ n ] = ftell( fp );
		    fgetc( fp );			/* dummy read \n */
		    if ( feof( fp ) ) {
			fclose( fp );
			unlink( TCB_buff[ n ] );
			free( TCB_buff[ n ] );
			TCB_buff[ n ] = NULL;
			TCB_fpos[ n ] = 0;
		    } else {
			fclose( fp );
		    }
		    return (int)strlen( buf );
		}
		fclose( fp );
	    }
	}
#endif
}

int
recvfrom( s, buf, len, flags, name, namelen )
int	s;
char	*buf;
int	len, flags;
struct sockaddr_in *name;
int	*namelen;
{
	int	ret;

#ifdef DEBUGGING
	if ( debug & 32768 )
	    printf( "Recvfrom %d Bytes buffer\n", len );
#endif
	if ( TCB[ s - 1 ].endpoint == 0 ) {
	    if ( *namelen < sizeof( struct sockaddr_in ) )
		return -5;
	    if ( ( ret = connect( s, name, *namelen ) ) < 0 )
		return ret;
	}
	return _recv( s, buf, len, flags );
}

int
send( s, buf, len, flags )
int	s;
char	*buf;
int	len, flags;
{
	int	n;

	n = s - 1;
	if ( n >= MAXCONNECTION )	return -3;
	if ( TCB[ n ].useflag == 0 )	return -8;
	TCB[ n ].command  = INETB_SEND;			/* SEND command */
	TCB[ n ].flags    = flags;
	TCB[ n ].prmlen   = len;
	TCB[ n ].prmbuff  = (char *)buf;
	if ( ib_request( &TCB[ n ] ) )	return -1;	/* Send error	*/
	return TCB[ n ].prmlen;
}

int
sendto( s, buf, len, flags, name, namelen )
int	s;
char	*buf;
int	len, flags;
struct sockaddr_in *name;
int	*namelen;
{
	int	ret;

#ifdef DEBUGGING
	if ( debug & 32768 )
	    printf( "Send to %d Bytes\n", len );
#endif
	if ( TCB[ s - 1 ].endpoint == 0 ) {
	    if ( *namelen < sizeof( struct sockaddr_in ) )
		return -5;
	    if ( ( ret = connect( s, name, *namelen ) ) < 0 )
		return ret;
	}
	return send( s, buf, len, flags );
}

int
shutdown( s, how )
int	s, how;
{
	int	n;

	n = s - 1;
	if ( n >= MAXCONNECTION )	return -3;
	if ( TCB[ n ].useflag == 0 )	return -8;
	if ( TCB[ n ].service == SRV_STRM && TCB[ n ].connection ) {
	    TCB[ n ].command  = INETB_HANGUP;		/* H_UP command */
	    TCB[ n ].prmlen   = 0;
	    TCB[ n ].prmbuff  = 0;
	    if ( ib_request( &TCB[ n ] ) )	return -1; /* Hangup error */
	}
	if ( TCB[ n ].useflag > 0 ) {		/* djperl interface	*/
	    TCB[ n ].useflag = -1;
	    return soclose( s );
	}
	return 0;				/* gn     interface	*/
}

int
soclose( s )
int	s;
{
	int	n;

	n = s - 1;
	if ( n >= MAXCONNECTION )	return -3;
	if ( TCB[ n ].useflag == 0 )	return -8;
	if ( TCB[ n ].useflag > 0 ) {
	    TCB[ n ].useflag = -1;
	    if ( shutdown( s, 0 ) )	return -1;	/* SHUTDOWN	*/
	}
	TCB[ n ].command = INETB_CLOSE;			/* CLOSE command */
	if ( ib_request( &TCB[ n ] ) )	return -1;	/* Close error	*/
	memset( (char *)&TCB[ n ], 0, sizeof( struct TCB ) );
	if ( TCB_buff[ n ] != NULL ) {
	    unlink( TCB_buff[ n ] );
	    free( TCB_buff[ n ] );
	    TCB_buff[ n ] = NULL;
	    TCB_fpos[ n ] = 0;
	}
#ifdef GNUDOS
	conventional_free( n );
#endif
	return 0;
}

int
sallclose()
{
	int	n;

	for ( n=0 ; n<MAXCONNECTION ; n++ ) {
	    if ( TCB[ n ].useflag != 0 ) {
		TCB[ n ].useflag = -1 ;
		shutdown( n + 1, 3 );
		soclose( n + 1 );
	    }
	}
	return 0;
}

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

int
gethostname( name, namelen )
char	*name;
int	namelen;
{
	char	*ep;

	if ( (ep = getenv( "HOST" )) != NULL ) {
	    strncpy( name, ep, namelen );
	    return 0;
	}
	puts("INETBIOS.C: 環境変数 HOST が見つかりません.\n\r");
	return -1;
}

struct hostent *
gethostbyname( name )
char	*name;
{
static	struct	hostent host_p;
static	char	buff[128], *alias_p[8];
	FILE	*fp;
	char	*ep, pathname[64], **cp;
	int	n;

	if ( ( ep = getenv( "INETBIOS" ) )== NULL ) {
	    puts( "INETBIOS.C: 環境変数 INETBIOS が見つかりません.\n\r" );
	    return NULL;
	}
	strcpy( pathname, ep );
	strcat( pathname, "\\HOSTS" );
	if ( ( fp = fopen( pathname, "r" ) )== NULL ) {
	    puts( "INETBIOS.C: HOSTS ファイルが見つかりません.\n\r" );
	    return NULL;
	}
	while ( 1 ) {
	    fgets( buff, 127, fp );
	    if ( feof( fp ) )	break;
	    if ( ferror( fp ) )	break;
	    if ( *buff == '#' )	continue;
	    host_p.h_addrtype = PF_INET;
	    host_p.h_length   = 4;
	    host_p.h_addr     = inet_addr( buff );
	    strtok( buff, "\t " );		/* dumy read ip address	*/
	    host_p.h_name     = strtok( NULL, "\t \n\r" );
	    n = 0;
	    while( (alias_p[n] = strtok( NULL, "\t \n\r" )) != NULL ) {
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
#if defined(DEBUG) || defined(DEBUGGING)
#ifdef DEBUGGING
	if ( debug & 32768 ) {
#endif /* DEBUGGING */
	printf("gethostbyname: %s %u.%u.%u.%u\n", host_p.h_name,
		*(((unsigned char *)&host_p.h_addr)+0),
			*(((unsigned char *)&host_p.h_addr)+1),
				*(((unsigned char *)&host_p.h_addr)+2),
				       *(((unsigned char *)&host_p.h_addr)+3));
#ifdef DEBUGGING
	}
#endif /* DEBUGGING */
#endif /* DEBUG || DEBUGGING */
	if ( host_p.h_addr ) return ( &host_p );
	else		     return NULL;
}

struct servent *
getservbyname( name, protcol )
char	*name, *protcol;
{
static	struct	servent serv_p;
static	char	buff[128], *alias_p[8];
	FILE	*fp;
	char	*ep, pathname[64], **cp, *tp;
	int	n;

	if ( ( ep = getenv( "INETBIOS" ) )== NULL ) {
	    puts( "INETBIOS.C: 環境変数 INETBIOS が見つかりません.\n\r" );
	    return NULL;
	}
	strcpy( pathname, ep );
	strcat( pathname, "\\SERVICES" );
	if ( ( fp = fopen( pathname, "r" ) )== NULL ) {
	    puts( "INETBIOS.C: SERVICES ファイルが見つかりません.\n\r" );
	    return NULL;
	}
	while ( 1 ) {
	    fgets( buff, 127, fp );
	    if ( feof( fp ) )	break;
	    if ( ferror( fp ) )	break;
	    n = 0;
	    serv_p.s_name  = strtok( buff, "\t \n\r" );
	    if ( (tp = strtok( NULL, "/" )) != NULL )
		serv_p.s_port  = atoi( tp );
	    serv_p.s_proto = strtok( NULL, "\t \n\r" );
	    while( (alias_p[n] = strtok( NULL, "\t \n\r" )) != NULL ) {
		if ( *alias_p[n] == '#' ) {
		    *alias_p[n] = 0;
		    break;
		} else {
		    ++n;
		}
	    }
	    serv_p.s_aliases = alias_p;
	    if ( strcmp( name, serv_p.s_name ) == 0 )
		goto gotname;
	    for ( cp = serv_p.s_aliases ; *cp ; cp++ )
		if ( strcmp( name, *cp ) == 0 )
		    goto gotname;
	    continue;
gotname:
	    if ( protcol == 0 || strcmp( serv_p.s_proto, protcol ) == 0 ) {
		fclose( fp );
#if defined(DEBUG) || defined(DEBUGGING)
#ifdef DEBUGGING
		if ( debug & 32768 ) {
#endif /* DEBUGGING */
		    printf("getservbyname: %s %s %u\n", serv_p.s_name,
					serv_p.s_proto, serv_p.s_port );
#ifdef DEBUGGING
		}
#endif /* DEBUGGING */
#endif /* DEBUG || DEBUGGING */
		return ( &serv_p );
	    }
	}
	fclose( fp );
	return NULL;
}

#endif	/* MSDOS && INETBIOS */
#endif	/* ! NO_NET */
