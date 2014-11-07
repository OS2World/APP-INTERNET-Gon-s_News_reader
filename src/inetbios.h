/*	inetbios.h	InetBIOS support libraly	Ver 0.33
 *
 *	Copyleft(c)	1992 sawada@murai-electric.co.jp
 *	Permit anyone to use, modify, redistribute this software.
 *	last modified at  1 July 1993 by sawada@murai-electric.co.jp
 */

#ifndef GNUDOS
typedef	unsigned char	u_char;
typedef	unsigned short	u_short;
typedef	unsigned int	u_int;
typedef	unsigned long	u_long;
#endif
typedef	unsigned short	ushort;		/* sys III compat */
/*
 * Internet address (a structure for historical reasons)
 */
struct in_addr {
	u_long s_addr;
};

/*
 * Socket address, internet style.
 */
struct sockaddr_in {
	short	sin_family;
	u_short	sin_port;
	struct	in_addr sin_addr;
	char	sin_zero[8];
};

/*
 * Types
 */
#define	SOCK_STREAM	1		/* stream socket */
#define	SOCK_DGRAM	2		/* datagram socket */

/*
 * Address families.
 */
#define	AF_UNSPEC	0		/* unspecified */
#define	AF_INET		2		/* internetwork: UDP, TCP, etc. */

/*
 * Protocol families, same as address families for now.
 */
#define	PF_UNSPEC	AF_UNSPEC
#define	PF_INET		AF_INET

struct	hostent {
	char	*h_name;	/* official name of host */
	char	**h_aliases;	/* alias list */
	int	h_addrtype;	/* host address type	*/
	int	h_length;	/* length of address	*/
	u_long	h_addr;		/* host address	*/
};

struct	servent {
	char	*s_name;	/* official service name */
	char	**s_aliases;	/* alias list */
	int	s_port;		/* port # */
	char	*s_proto;	/* protocol to use */
};

#define	recvs(s,buf,len,flags)	recv(s,buf,len,flags)
