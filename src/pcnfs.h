/* J3100 + Sun PCNFStoolkit
 *   defines
 *   $Header: /home/sk2/nori/CvsRoot/work/gndos/src/j31defs.h,v 1.1.1.1 1993/07/03 01:50:44 nori Exp $
 */

#ifndef _pcnfs_h
#define _pcnfs_h

typedef	unsigned char	u_char;
typedef	unsigned short	u_short;
typedef	unsigned int	u_int;
typedef	unsigned long	u_long;
typedef	unsigned short	ushort;		/* sys III compat */

#ifdef vax
typedef	struct	_physadr { int r[1]; } *physadr;
typedef	struct	label_t	{
	int	val[14];
} label_t;
#endif
#ifdef mc68000
typedef	struct	_physadr { short r[1]; } *physadr;
typedef	struct	label_t	{
	int	val[13];
} label_t;
#endif
typedef	long	daddr_t;
typedef	char *caddr_t;
typedef	u_short	ino_t;
typedef	long	swblk_t;
/* commented out size_t and time_t; redundant with MS C jlm */
/*typedef	unsigned	size_t;*/ /* used to be int, but this breaks MSC 5.1 */


typedef	short	dev_t;
typedef	long	off_t;

#endif
