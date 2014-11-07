/*
 * @(#)kanji.c	1.36 May.30,1997
 *
 * 著作権は放棄しません。ただし、営利目的以外の使用／配布に制限は設けません。
 * Copyright (C) yamasita@omronsoft.co.jp
 */

#define	HANKAKU2ZENKAKU

#if ! defined(CONFIGUR)
#ifdef WINDOWS
#	include <windows.h>
#	include	<mbctype.h>
#endif

#include	<stdio.h>

#ifdef	HAS_STRING_H
#	include	<string.h>
#endif

#if defined(WATTCP)
#	include <tcp.h>
#endif
#ifdef PCNFS
#	include "pcnfs.h"
#endif

#include	"nntp.h"
#include	"gn.h"

#endif		/* ! CONFIGUR */

static void euctojis(),euctosjis();
static void sjistoeuc(),sjistojis();
static void jistoeuc(),jistosjis();

static void check_kana();
static void check_kana_jis();
static void check_kana_euc();
static void check_kana_sjis();

#if defined(WINDOWS) && ! defined(CONFIGUR)
static void	scroll_up();

static HDC		hdc;
static int	fast_mc_puts_flag = OFF;
#endif

/*
 * メッセージの表示
 */
#if defined(WINDOWS) && ! defined(CONFIGUR)
int
mc_MessageBox(hwndParent,lpszText,lpszTitle,fuStyle)
HWND hwndParent;
LPCSTR lpszText,lpszTitle;
UINT fuStyle;
{
  extern void kanji_convert();
  
  char text[256],title[256];
  
  if ( lpszTitle == NULL ) {
    if ( gn.display_kanji_code == internal_kanji_code )
      strcpy(text,lpszText);
    else
      kanji_convert(internal_kanji_code,lpszText,
		    gn.display_kanji_code,text);
    return(MessageBox(hwndParent,text,NULL,fuStyle));
  }
  
  
  if ( gn.display_kanji_code == internal_kanji_code ){
    strcpy(text,lpszText);
    strcpy(title,lpszTitle);
  } else {
    kanji_convert(internal_kanji_code,lpszText,gn.display_kanji_code,text);
    kanji_convert(internal_kanji_code,lpszTitle,gn.display_kanji_code,title);
  }
  
  return(MessageBox(hwndParent,text,title,fuStyle));
}
#endif	/* WINDOWS */
void
mc_printf(format,arg1,arg2,arg3,arg4)
char *format,*arg1,*arg2,*arg3,*arg4;
{
  extern void mc_puts();
  char buf[NNTP_BUFSIZE+1];
  
  sprintf(buf,format,arg1,arg2,arg3,arg4);
  mc_puts(buf);
}

#if defined(WINDOWS) && ! defined(CONFIGUR)
void
fast_mc_puts_start()
{
  ScrollWindow(hWnd, 0, -wheight, NULL, NULL);
  memset(display_buf,' ',gn.lines*gn.columns);
  xpos = ypos = 0;
  
  HideCaret(hWnd);
  hdc = GetDC(hWnd);
  SelectObject(hdc, GetStockObject(SYSTEM_FIXED_FONT));
  
  fast_mc_puts_flag = ON;
}
void
fast_mc_puts_end()
{
  ShowCaret(hWnd);
  ReleaseDC(hWnd, hdc);
  SetCaretPos(xpos * xsiz, ypos * ysiz);
  
  fast_mc_puts_flag = OFF;
}
void
mc_puts(sp)
char *sp;	/* internal_kanji_code のメッセージ */
{
  extern void kanji_convert();
  
  static int	need_nl = OFF;
  char buf[NNTP_BUFSIZE+1];
  register char *bp;
  
  if ( gn.display_kanji_code == internal_kanji_code ) {
    bp = sp;
  } else {
    kanji_convert(internal_kanji_code,sp,gn.display_kanji_code,buf);
    bp = buf;
  }
  
  if ( fast_mc_puts_flag == OFF ) {
    HideCaret(hWnd);
    hdc = GetDC(hWnd);
    SelectObject(hdc, GetStockObject(SYSTEM_FIXED_FONT));
  }
  
  while( *bp != 0 ) {
    if ( *bp == NL ) {
      need_nl = OFF;
      ypos++;
      xpos = 0;
      if (ypos >= gn.lines) {
	scroll_up();
	ypos--;
      }
      bp++;
      continue;
    }

    if ( *bp == BEL ) {
      MessageBeep(MB_OK);
      bp++;
      continue;
    }

    if ( *bp == TAB ) {
      need_nl = OFF;
      display_buf[xpos+ypos*gn.columns] = ' ';
      if ( fast_mc_puts_flag == OFF )
	TextOut(hdc, xpos * xsiz, ypos * ysiz, " ", 1);
      xpos++;
      while ( (xpos % 8) != 0 ) {
	display_buf[xpos+ypos*gn.columns] = ' ';
	if ( fast_mc_puts_flag == OFF )
	  TextOut(hdc, xpos * xsiz, ypos * ysiz, " ", 1);
	xpos++;
      }
      bp++;
      continue;
    }

    if ( need_nl != OFF ) {	/* 最終カラムで、カーソルが止まっている */
      need_nl = OFF;
      ypos++;
      xpos = 0;
      if (ypos >= gn.lines) {
	scroll_up();
	ypos--;
      }
    }

    if ( _ismbblead(*bp) ) {	/* 漢字１バイト目 */
      if ( xpos >= gn.columns - 1 ) {
	need_nl = OFF;
	ypos++;
	xpos = 0;
	if (ypos >= gn.lines) {
	  scroll_up();
	  ypos--;
	}
      }
      display_buf[xpos+ypos*gn.columns] = *bp;
      display_buf[xpos+ypos*gn.columns+1] = bp[1];
      if ( fast_mc_puts_flag == OFF )
	TextOut(hdc, xpos * xsiz, ypos * ysiz, bp, 2);
      bp +=2;
      xpos += 2;
    } else {
      display_buf[xpos+ypos*gn.columns] = *bp;
      if ( fast_mc_puts_flag == OFF )
	TextOut(hdc, xpos * xsiz, ypos * ysiz, bp, 1);
      bp++;
      xpos++;
    }
    if ( xpos >= gn.columns ) {
      xpos = gn.columns -1;
      need_nl = ON;
    }
  }
  if ( fast_mc_puts_flag == OFF ) {
    ShowCaret(hWnd);
    ReleaseDC(hWnd, hdc);
    UpdateWindow(hWnd);
    SetCaretPos(xpos * xsiz, ypos * ysiz);
  }
}
static void
scroll_up()
{
  memcpy(&display_buf[0], &display_buf[gn.columns],(gn.lines-1)*gn.columns);
  memset(&display_buf[(gn.lines-1)*gn.columns],' ',gn.columns);
  if ( fast_mc_puts_flag == OFF )
    ScrollWindow(hWnd, 0, -ysiz, NULL, NULL);
}
#else	/* WINDOWS */
void
mc_puts(sp)
char *sp;	/* internal_kanji_code のメッセージ */
{
  extern void kanji_convert();
  
  char dest[NNTP_BUFSIZE+1];
  
  if ( gn.display_kanji_code == internal_kanji_code ){
    printf("%s",sp);
  } else {
    kanji_convert(internal_kanji_code,sp,gn.display_kanji_code,dest);
    printf("%s",dest);
  }
}
#endif	/* WINDOWS */
/*
 * 漢字変換
 */
void
kanji_convert(s_code,sp,d_code,dp)
int s_code;				/* 入力漢字コード */
register unsigned char *sp;		/* 入力文字列 */
int d_code;				/* 出力漢字コード */
register unsigned char *dp;		/* 出力文字列 */
{
  if ( s_code == d_code ) {
#ifdef HANKAKU2ZENKAKU
    check_kana(sp,dp,s_code);
#else
    strcpy(dp,sp);
#endif
  } else {
    switch (s_code) {
    case EUC:
      switch (d_code){
      case SJIS:
	euctosjis(sp,dp);
	break;
      default:			/* JIS */
	euctojis(sp,dp);
	break;
      }
      break;
    case SJIS:
      switch (d_code){
      case EUC:
	sjistoeuc(sp,dp);
	break;
      default:			/* JIS */
	sjistojis(sp,dp);
	break;
      }
      break;
    default:			/* JIS */
      switch (d_code){
      case SJIS:
	jistosjis(sp,dp);
	break;
      default:
	jistoeuc(sp,dp);
	break;
      }
      break;
    }
  }
}
#ifdef HANKAKU2ZENKAKU
/* 半角->全角変換テーブル */
static unsigned short hanzen_euc[] = {
  /* a1 - a7      "。","「","」","、","・","ヲ","ァ", */
  0xa1a3, 0xa1d6, 0xa1d7, 0xa1a2, 0xa1a6, 0xa5f2, 0xa5a1, 
  /* a8 - af "ィ","ゥ","ェ","ォ","ャ","ュ","ョ","ッ", */
  0xa5a3, 0xa5a5, 0xa5a7, 0xa5a9, 0xa5e3, 0xa5e5, 0xa5e7, 0xa5c3,
  /* b0 - b7 "ー","ア","イ","ウ","エ","オ","カ","キ", */
  0xa1bc, 0xa5a2, 0xa5a4, 0xa5a6, 0xa5a8, 0xa5aa, 0xa5ab, 0xa5ad,
  /* b8 - bf "ク","ケ","コ","サ","シ","ス","セ","ソ", */
  0xa5af, 0xa5b1, 0xa5b3, 0xa5b5, 0xa5b7, 0xa5b9, 0xa5bb, 0xa5bd,
  /* c0 - c7 "タ","チ","ツ","テ","ト","ナ","ニ","ヌ", */
  0xa5bf, 0xa5c1, 0xa5c4, 0xa5c6, 0xa5c8, 0xa5ca, 0xa5cb, 0xa5cc, 
  /* c8 - cf "ネ","ノ","ハ","ヒ","フ","ヘ","ホ","マ", */
  0xa5cd, 0xa5ce, 0xa5cf, 0xa5d2, 0xa5d5, 0xa5d8, 0xa5db, 0xa5de, 
  /* d0 - d7 "ミ","ム","メ","モ","ヤ","ユ","ヨ","ラ", */
  0xa5df, 0xa5e0, 0xa5e1, 0xa5e2, 0xa5e4, 0xa5e6, 0xa5e8, 0xa5e9, 
  /* d8 - df "リ","ル","レ","ロ","ワ","ン","゛","゜" */
  0xa5ea, 0xa5eb, 0xa5ec, 0xa5ed, 0xa5ef, 0xa5f3, 0xa1ab, 0xa1ac
};
static unsigned short hanzen_sjis[] = {
  /* a1 - a7      "。","「","」","、","・","ヲ","ァ", */
  0x8142, 0x8175, 0x8176, 0x8141, 0x8145, 0x8392, 0x8340, 
  /* a8 - af "ィ","ゥ","ェ","ォ","ャ","ュ","ョ","ッ", */
  0x8342, 0x8344, 0x8346, 0x8348, 0x8383, 0x8385, 0x8387, 0x8362,
  /* b0 - b7 "ー","ア","イ","ウ","エ","オ","カ","キ", */
  0x815b, 0x8341, 0x8343, 0x8345, 0x8347, 0x8349, 0x834a, 0x834c,
  /* b8 - bf "ク","ケ","コ","サ","シ","ス","セ","ソ", */
  0x834e, 0x8350, 0x8352, 0x8354, 0x8356, 0x8358, 0x835a, 0x835c,
  /* c0 - c7 "タ","チ","ツ","テ","ト","ナ","ニ","ヌ", */
  0x835e, 0x8360, 0x8363, 0x8365, 0x8367, 0x8369, 0x836a, 0x836b,
  /* c8 - cf "ネ","ノ","ハ","ヒ","フ","ヘ","ホ","マ", */
  0x836c, 0x836d, 0x836e, 0x8371, 0x8374, 0x8377, 0x837a, 0x837d,
  /* d0 - d7 "ミ","ム","メ","モ","ヤ","ユ","ヨ","ラ", */
  0x837e, 0x8380, 0x8381, 0x8382, 0x8384, 0x8386, 0x8388, 0x8389,
  /* d8 - df "リ","ル","レ","ロ","ワ","ン","゛","゜" */
  0x838a, 0x838b, 0x838c, 0x838d, 0x838f, 0x8393, 0x814a, 0x814b,
};

#define	KANA_KA		(0xb6 & 0x7f)	/* カ */
#define	KANA_TO		(0xc4 & 0x7f)	/* ト */
#define	KANA_HA		(0xca & 0x7f)	/* ハ */
#define	KANA_HO		(0xce & 0x7f)	/* ホ */

#define	IS_DAKU(x)	((KANA_KA <= (int)(x & 0x7f) && (int)(x & 0x7f) <= KANA_TO) || (KANA_HA <= (int)(x & 0x7f) && (int)(x & 0x7f) <= KANA_HO))
#define	IS_HANDAKU(x)	(KANA_HA <= (int)(x & 0x7f) && (int)(x & 0x7f) <= KANA_HO)

#define	DAKUTEN8	0xde	/* ゛ */
#define	DAKUTEN7	(DAKUTEN8 & 0x7f)
#define	HANDAKUTEN8	0xdf	/* ゜ */
#define	HANDAKUTEN7	(HANDAKUTEN8 & 0x7f)
#endif /* HANKAKU2ZENKAKU */

/*
 * EUC -> JIS 変換
 */
static void
euctojis(sp,dp)
register unsigned char *sp;		/* 入力 EUC 文字列 */
register unsigned char *dp;		/* 出力 JIS 文字列 */
{
  int kanji_flag = 0;
  int kana_flag = 0;
  unsigned short kana;

  while( *sp != 0 ) {
    /* ASCII */
    if ( *sp <= DEL ){
      if ( kanji_flag == 1 || kana_flag == 1 ){
	*dp++ = ESC;
	*dp++ = '(';
	*dp++ = 'B';		/* *dp++ = 'J'; */
	kanji_flag = 0;
	kana_flag = 0;
      }
      *dp++ = *sp++;
      continue;
    }
    /* 半角カナ */
#ifdef HANKAKU2ZENKAKU
    if ( *sp == SS2 ) {
      if ( sp[1] < 0xa1 && 0xdf < sp[1] ) {
	sp++;
	continue;		/* 違法なコード */
      }

      if ( kanji_flag == 0 ){
	*dp++ = ESC;
	*dp++ = '$';
	*dp++ = 'B';
	kanji_flag = 1;
	kana_flag = 0;
      }

      sp++;
      kana = hanzen_euc[(*sp & 0x7f) - 0x21];
      if ( sp[1] == SS2 ) {
	if ( sp[2] == DAKUTEN8 ) {	/* 濁点 */
	  if ( IS_DAKU(*sp) ) {
	    kana++;
	    sp += 2;
	  }
	}
	if ( sp[1] == HANDAKUTEN8 ) {	/* 半濁点 */
	  if ( IS_HANDAKU(*sp) ) {
	    kana +=2;
	    sp += 2;
	  }
	}
      }
      sp++;

      *dp++ = (unsigned char)((int)(kana & 0x7f00) >> 8);
      *dp++ = (unsigned char)(kana & 0x007f);
      continue;
    }
#else  /* HANKAKU2ZENKAKU */
    if ( *sp == SS2 ) {
      if ( kana_flag == 0 ){
	*dp++ = ESC;
	*dp++ = '(';
	*dp++ = 'I';
	kana_flag = 1;
	kanji_flag = 0;
      }
      sp++;
      *dp++ = *sp++ & 0x7f;
      continue;
    }
#endif /* HANKAKU2ZENKAKU */

    /* 漢字 */
    if ( kanji_flag == 0 ){
      *dp++ = ESC;
      *dp++ = '$';
      *dp++ = 'B';
      kanji_flag = 1;
      kana_flag = 0;
    }
    *dp++ = *sp++ & 0x7f;
    *dp++ = *sp++ & 0x7f;
  }
  if ( kanji_flag == 1  || kana_flag == 1 ){
    *dp++ = ESC;
    *dp++ = '(';
    *dp++ = 'B';		/* *dp++ = 'J'; */
  }
  *dp = 0;
}

/*
 * JIS -> EUC 変換
 */
static void
jistoeuc(sp,dp)
register unsigned char *sp;		/* 入力 JIS 文字列 */
register unsigned char *dp;		/* 出力 EUC 文字列 */
{
  int kanji_flag = 0;
  int kana_flag = 0;
  unsigned short kana;
  
  while( *sp != 0 ) {
    /* エスケープシーケンスの判定 */
    if ( *sp == ESC ) {
      if ( sp[1] == '$' && ( sp[2] == '@' || sp[2] == 'B' ) ) {
	/* 漢字開始 */
	kanji_flag = 1;
	kana_flag = 0;
	sp += 3;
	continue;
      }
      if ( sp[1] == '(' && sp[2] == 'I' ) {
	/* 半角カナ開始 */
	kana_flag = 1;
	kanji_flag = 0;
	sp += 3;
	continue;
      }
      if ( sp[1] == '(' &&
	  (sp[2] == 'B' || sp[2] == 'H' || sp[2] == 'J' ) ) {
	/* ASCII 開始 */
	kanji_flag = 0;
	kana_flag = 0;
	sp += 3;
	continue;
      }
    }
    if ( *sp == 0x0e )	{	/* ^N */
      kana_flag = 1;
      kanji_flag = 0;
      sp += 1;
      continue;
    }
    if ( *sp == 0x0f )	{	/* ^O */
      kanji_flag = 0;
      kana_flag = 0;
      sp += 1;
      continue;
    }

    /* 漢字 */
    if ( kanji_flag == 1 ) {
      *dp++ = *sp++ | 0x80;
      *dp++ = *sp++ | 0x80;
      continue;
    }
    /* 半角カナ */
#ifdef HANKAKU2ZENKAKU
    if ( kana_flag == 1 && (*sp & 0x7f) >= 0x21 && (*sp & 0x7f) < 0x60 ) {
      kana = hanzen_euc[(*sp & 0x7f) - 0x21];
      if ( sp[1] == DAKUTEN7 ) {	/* 濁点 */
	if ( IS_DAKU(*sp) ) {
	  kana++;
	  sp++;
	}
      }
      if ( sp[1] == HANDAKUTEN7 ) {	/* 半濁点 */
	if ( IS_HANDAKU(*sp) ) {
	  kana +=2;
	  sp++;
	}
      }
      sp++;
      *dp++ = (unsigned char)((int)(kana & 0xff00) >> 8);
      *dp++ = (unsigned char)(kana & 0x00ff);
      continue;
    }
#else  /* HANKAKU2ZENKAKU */
    if ( kana_flag == 1 ) {
      *dp++ = SS2;
      *dp++ = *sp++ | 0x80;
      continue;
    }
#endif /* HANKAKU2ZENKAKU */
    /* ASCII */
    *dp++ = *sp++;
  }
  *dp = 0;
}

/*
 * SJIS -> JIS 変換
 */
static void
sjistojis(sp,dp)
register unsigned char *sp;		/* 入力 SJIS 文字列 */
register unsigned char *dp;		/* 出力 JIS 文字列 */
{
  int kanji_flag = 0;
  int kana_flag = 0;
  register int jupper,jlower,supper,slower;
  unsigned short kana;
  
  while( *sp != 0 ) {
    /* ASCII */
    if ( *sp <= DEL ){
      if ( kanji_flag == 1 ){
	*dp++ = ESC;
	*dp++ = '(';
	*dp++ = 'B';		/* *dp++ = 'J'; */
	kanji_flag = 0;
	kana_flag = 0;
      }
      *dp++ = *sp++;
      continue;
    }
    /* 半角カナ */
#ifdef HANKAKU2ZENKAKU
    if ( 0xa0 <= *sp && *sp <= 0xdf ){
      if ( kanji_flag == 0 ){
	*dp++ = ESC;
	*dp++ = '$';
	*dp++ = 'B';
	kanji_flag = 1;
	kana_flag = 0;
      }
      kana = hanzen_euc[(*sp & 0x7f) - 0x21];
      if ( sp[1] == DAKUTEN8 ) {	/* 濁点 */
	if ( IS_DAKU(*sp) ) {
	  kana++;
	  sp++;
	}
      }
      if ( sp[1] == HANDAKUTEN8 ) {	/* 半濁点 */
	if ( IS_HANDAKU(*sp) ) {
	  kana +=2;
	  sp++;
	}
      }
      sp++;
      *dp++ = (unsigned char)((int)(kana & 0x7f00) >> 8);
      *dp++ = (unsigned char)(kana & 0x007f);
      continue;
    }
#else  /* HANKAKU2ZENKAKU */
    if ( 0xa0 <= *sp && *sp <= 0xdf ){
      if ( kana_flag == 0 ){
	*dp++ = ESC;
	*dp++ = '(';
	*dp++ = 'I';
	kana_flag = 1;
	kanji_flag = 0;
      }
      *dp++ = *sp++ & 0x7f;
      continue;
    }
#endif /* HANKAKU2ZENKAKU */
    /* 漢字 */
    if ( kanji_flag == 0 ){
      *dp++ = ESC;
      *dp++ = '$';
      *dp++ = 'B';
      kanji_flag = 1;
      kana_flag = 0;
    }

    supper = *sp++;
    slower = *sp++;

    if(slower <= 0x9E){
      if( supper <= 0x9F )
	jupper = (supper-0x71) * 2 + 1;
      else
	jupper = (supper-0xB1) * 2 + 1;
      jlower = slower - 0x1F;
      if ( slower >= 0x80 )
	jlower--;
    } else {
      if( supper <= 0x9F )
	jupper = (supper-0x70) * 2;
      else
	jupper = (supper-0xB0) * 2;
      jlower = slower - 0x7E;
    }
    *dp++ = jupper;
    *dp++ = jlower;
  }
  if ( kanji_flag == 1 || kana_flag == 1 ){
    *dp++ = ESC;
    *dp++ = '(';
    *dp++ = 'B';		/* *dp++ = 'J'; */
  }
  *dp = 0;
}

/*
 * JIS -> SJIS 変換
 */
static void
jistosjis(sp,dp)
register unsigned char *sp;		/* 入力 JIS 文字列 */
register unsigned char *dp;		/* 出力 SJIS 文字列 */
{
  int kanji_flag = 0;
  int kana_flag = 0;
  register int jupper,jlower,supper,slower;
  unsigned short kana;
  
  while( *sp != 0 ) {
    /* エスケープシーケンスの判定 */
    if ( *sp == ESC ) {
      if ( sp[1] == '$' && ( sp[2] == '@' || sp[2] == 'B' ) ) {
	kanji_flag = 1;
	kana_flag = 0;
	sp += 3;
	continue;
      }
      if ( sp[1] == '(' && sp[2] == 'I' ) {
	kana_flag = 1;
	kanji_flag = 0;
	sp += 3;
	continue;
      }
      if ( sp[1] == '(' &&
	  (sp[2] == 'B' || sp[2] == 'H' || sp[2] == 'J' ) ) {
	kanji_flag = 0;
	kana_flag = 0;
	sp += 3;
	continue;
      }
    }
    if ( *sp == 0x0e )	{	/* ^N */
      kana_flag = 1;
      kanji_flag = 0;
      sp += 1;
      continue;
    }
    if ( *sp == 0x0f )	{	/* ^O */
      kanji_flag = 0;
      kana_flag = 0;
      sp += 1;
      continue;
    }

    /* 漢字 */
    if ( kanji_flag == 1 ) {
      jupper = *sp++;
      jlower = *sp++;
      
      if( jupper <= 0x5E )
	supper = (jupper-1) / 2 + 0x71;
      else
	supper = (jupper-1) / 2 + 0xB1;
      
      if( jupper & 1 ){
	slower = jlower + 0x1F;
	if( slower >= 0x7F )
	  slower++;
      } else {
	slower = jlower + 0x7E;
      }
      *dp++ = supper;
      *dp++ = slower;
      continue;
    }
    
    /* 半角カナ */
#ifdef HANKAKU2ZENKAKU
    if ( kana_flag == 1 && (*sp & 0x7f) >= 0x21 && (*sp & 0x7f) < 0x60 ) {
      kana = hanzen_sjis[(*sp & 0x7f) - 0x21];
      if ( sp[1] == DAKUTEN7 ) {	/* 濁点 */
	if ( IS_DAKU(*sp) ) {
	  kana++;
	  sp++;
	}
      }
      if ( sp[1] == HANDAKUTEN7 ) {	/* 半濁点 */
	if ( IS_HANDAKU(*sp) ) {
	  kana +=2;
	  sp++;
	}
      }
      sp++;
      *dp++ = (unsigned char)((int)(kana & 0xff00) >> 8);
      *dp++ = (unsigned char)(kana & 0x00ff);
      continue;
    }
#else  /* HANKAKU2ZENKAKU */
    if ( kana_flag == 1 ) {
      *dp++ = *sp++ | 0x80;
      continue;
    }
#endif /* HANKAKU2ZENKAKU */
    /* ASCII */
    *dp++ = *sp++;
  }
  *dp = 0;
}

/*
 * EUC -> SJIS 変換
 */
static void
euctosjis(sp,dp)
register unsigned char *sp;		/* 入力 EUC 文字列 */
register unsigned char *dp;		/* 出力 SJIS 文字列 */
{
  register int jupper,jlower,supper,slower;
  unsigned short kana;
  
  while( *sp != 0 ) {
    /* ASCII */
    if ( *sp <= DEL ){
      *dp++ = *sp++;
      continue;
    }
    /* 半角カナ */
#ifdef HANKAKU2ZENKAKU
    if ( *sp == SS2 ) {
      sp++;
      kana = hanzen_sjis[(*sp & 0x7f) - 0x21];
      if ( sp[1] == SS2 ) {
	if ( sp[2] == DAKUTEN8 ) {	/* 濁点 */
	  if ( IS_DAKU(*sp) ) {
	    kana++;
	    sp += 2;
	  }
	}
	if ( sp[1] == HANDAKUTEN8 ) {	/* 半濁点 */
	  if ( IS_HANDAKU(*sp) ) {
	    kana +=2;
	    sp += 2;
	  }
	}
      }
      sp++;

      *dp++ = (unsigned char)((int)(kana & 0xff00) >> 8);
      *dp++ = (unsigned char)(kana & 0x00ff);
      continue;
    }
#else  /* HANKAKU2ZENKAKU */
    if ( *sp == SS2 ) {
      sp++;
      *dp++ = *sp++;
      continue;
    }
#endif /* HANKAKU2ZENKAKU */
    /* 漢字 */
    jupper = *sp++ & 0x7f;
    jlower = *sp++ & 0x7f;

    if( jupper <= 0x5E )
      supper = (jupper-1) / 2 + 0x71;
    else
      supper = (jupper-1) / 2 + 0xB1;

    if( jupper & 1 ){
      slower = jlower + 0x1F;
      if( slower >= 0x7F )
	slower++;
    } else {
      slower = jlower + 0x7E;
    }
    *dp++ = supper;
    *dp++ = slower;
  }
  *dp = 0;
}

/*
 * SJIS -> EUC 変換
 */
static void
sjistoeuc(sp,dp)
register unsigned char *sp;		/* 入力 SJIS 文字列 */
register unsigned char *dp;		/* 出力 euc 文字列 */
{
  register int jupper,jlower,supper,slower;
  unsigned short kana;
  
  while( *sp != 0 ) {
    /* ASCII */
    if ( *sp <= DEL ){
      *dp++ = *sp++;
      continue;
    }
    /* 半角カナ */
#ifdef HANKAKU2ZENKAKU
    if ( 0xa0 <= *sp && *sp <= 0xdf ){
      kana = hanzen_euc[(*sp++ & 0x7f) - 0x21];
      if ( sp[1] == DAKUTEN8 ) {	/* 濁点 */
	if ( IS_DAKU(*sp) ) {
	  kana++;
	  sp++;
	}
      }
      if ( sp[1] == HANDAKUTEN8 ) {	/* 半濁点 */
	if ( IS_HANDAKU(*sp) ) {
	  kana +=2;
	  sp++;
	}
      }
      sp++;
      *dp++ = (unsigned char)((int)(kana & 0x7f00) >> 8);
      *dp++ = (unsigned char)(kana & 0x007f);
      continue;
    }
#else  /* HANKAKU2ZENKAKU */
    if ( 0xa0 <= *sp && *sp <= 0xdf ){
      *dp++ = SS2;
      *dp++ = *sp++;
      continue;
    }
#endif /* HANKAKU2ZENKAKU */
    /* 漢字 */
    supper = *sp++;
    slower = *sp++;
    if(slower <= 0x9E){
      if( supper <= 0x9F )
	jupper = (supper-0x71) * 2 + 1;
      else
	jupper = (supper-0xB1) * 2 + 1;
      jlower = slower - 0x1F;
      if ( slower >= 0x80 )
	jlower--;
    } else {
      if( supper <= 0x9F )
	jupper = (supper-0x70) * 2;
      else
	jupper = (supper-0xB0) * 2;
      jlower = slower - 0x7E;
    }
    *dp++ = jupper | 0x80;
    *dp++ = jlower | 0x80;
  }
  *dp = 0;
}

/*
 * ESC が落ちていれば付加する
 */
void
add_esc(s_string,d_string)
register char *s_string;	/* 入力 JIS 文字列（ESC が落ちているかも） */
register char *d_string;	/* 出力 JIS 文字列 */
{
  register char *sp;		/* 入力 JIS 文字列（ESC が落ちているかも） */
  register char *dp;		/* 出力 JIS 文字列 */
  register char *pt;

  sp = s_string;
  dp = d_string;

  /* もし一つでも ESC があれば、ESC 落ちではない */
  for ( pt = sp ; *pt != 0 ; pt++ ) {
    if ( *pt == ESC ) {
      strcpy(dp,sp);
      return;
    }
  }

  /* ESC 落ちかどうかを調べる */
  while ( *sp != 0 ) {

    if ( *sp != '$' || ( sp[1] != '@' && sp[1] != 'B' ) ) {
      /* 漢字のコードセット指定ではない */
      *dp++ = *sp++;
      continue;
    }

    /* 漢字のコードセット指定かも知れない */
    /* 後ろをずっと調べる */
    for ( pt = &sp[1] ; *pt != 0 ; pt++ ) {
      if ( *pt != '$' && *pt != '(' )
	continue;

      if ( *pt == '$' ) {
	if ( pt[1] != '@' && pt[1] != 'B' )
	  continue;

	if ( pt[1] == sp[1] ) {	/* 同じシーケンスがあらわれることはない */
	  strcpy(d_string,s_string);
	  return;
	}
      }
      if ( *pt == '(' ) {
	if ( pt[1] != 'B' && pt[1] != 'H' && pt[1] != 'J' && pt[1] != 'I' )
	  continue;
      }
      /* 多分 ESC 落ちだろう */
      *dp++ = ESC;
      while ( sp < pt )
	*dp++ = *sp++;
      *dp++ = ESC;
      *dp++ = *sp++;
      /* *dp++ = *sp++; break してから for ループの外で*/
      break;
    }
    *dp++ = *sp++;
  }
  *dp = 0;
}


/*
 * len に収まるように文字列を切る
 */
void
str_cut(buf,len)
unsigned char *buf;
int len;
{
  register unsigned char *pt;
  
  pt = buf;
  if ( internal_kanji_code == EUC ) {
    while (len > 0) {
      if ( *pt == 0 )
	return;
      if ( (*pt & 0x80) != 0 ) {
	if ( len == 1 ) {
	  *pt = 0;
	  return;
	}
	len--;
	pt++;
      }
      pt++;
      len--;
    }
  } else {
    while (len > 0) {
      if ( *pt == 0 )
	return;
      if ( (0x80 <= *pt && *pt <= 0x9f) || (0xe0 <= *pt && *pt <= 0xff) ){
	if ( len == 1 ) {
	  *pt = 0;
	  return;
	}
	len--;
	pt++;
      }
      pt++;
      len--;
    }
  }
  *pt = 0;
}

#ifdef HANKAKU2ZENKAKU
/*
 * 半角カナから全角カナへの変換
 */

static void
check_kana(sp,dp,code)
unsigned char *sp;	/* 入力文字列 */
unsigned char *dp;	/* 出力文字列 */
int code;
{
  switch (code) {
  case JIS:
    check_kana_jis(sp,dp);
    return;
  case EUC:
    check_kana_euc(sp,dp);
    return;
  case SJIS:
    check_kana_sjis(sp,dp);
    return;
  }
}
/*
 * 半角カナから全角カナへの変換(JIS)
 */

static void
check_kana_jis(sp,dp)
unsigned char *sp;	/* 入力文字列 */
unsigned char *dp;	/* 出力文字列 */
{
  int kanji_flag = 0;
  int kana_flag = 0;
  unsigned short kana;
  static unsigned char kanji_in_char = 0;
  static unsigned char ascii_in_char = 0;
  
  while( *sp != 0 ) {
    /* エスケープシーケンスの判定 */
    if ( *sp == ESC ) {
      if ( sp[1] == '$' && ( sp[2] == '@' || sp[2] == 'B' ) ) {
	/* 漢字開始 */
	kanji_flag = 1;
	kana_flag = 0;

	kanji_in_char = sp[2];

	*dp++ = *sp++;
	*dp++ = *sp++;
	*dp++ = *sp++;

	continue;
      }
      if ( sp[1] == '(' && sp[2] == 'I' ) {
	/* 半角カナ開始 */
	kana_flag = 1;
	kanji_flag = 0;

	sp += 3;

	*dp++ = ESC;
	*dp++ = '$';
	if (kanji_in_char == 0)
	  *dp++ = 'B';
	else
	  *dp++ = kanji_in_char;

	continue;
      }
      if ( sp[1] == '(' &&
	  (sp[2] == 'B' || sp[2] == 'H' || sp[2] == 'J' ) ) {
	/* ASCII 開始 */
	kanji_flag = 0;
	kana_flag = 0;

	ascii_in_char = sp[2];

	*dp++ = *sp++;
	*dp++ = *sp++;
	*dp++ = *sp++;

	continue;
      }
    }
    if ( *sp == 0x0e )	{	/* ^N */
      /* 半角カナ開始 */
      kana_flag = 1;
      kanji_flag = 0;

      sp++;

      *dp++ = ESC;
      *dp++ = '$';
      if (kanji_in_char == 0)
	*dp++ = 'B';
      else
	*dp++ = kanji_in_char;

      continue;
    }
    if ( *sp == 0x0f )	{	/* ^O */
      /* ASCII 開始 */
      kanji_flag = 0;
      kana_flag = 0;

      sp++;

      *dp++ = ESC;
      *dp++ = '(';
      if (ascii_in_char == 0)
	*dp++ = 'B';
      else
	*dp++ = ascii_in_char;

      continue;
    }

    /* 漢字 */
    if ( kanji_flag == 1 ) {
      *dp++ = *sp++;
      *dp++ = *sp++;
      continue;
    }

    /* 半角カナ */
    if ( kana_flag == 1 && (*sp & 0x7f) >= 0x21 && (*sp & 0x7f) < 0x60 ) {
      kana = hanzen_euc[(*sp & 0x7f) - 0x21];
      if ( sp[1] == DAKUTEN7 ) {	/* 濁点 */
	if ( IS_DAKU(*sp) ) {
	  kana++;
	  sp++;
	}
      }
      if ( sp[1] == HANDAKUTEN7 ) {	/* 半濁点 */
	if ( IS_HANDAKU(*sp) ) {
	  kana +=2;
	  sp++;
	}
      }
      sp++;
      *dp++ = (unsigned char)((int)(kana & 0x7f00) >> 8);
      *dp++ = (unsigned char)(kana & 0x007f);
      continue;
    }

    /* ASCII */
    *dp++ = *sp++;
  }
  if ( kanji_flag == 1 || kana_flag == 1 ){
    *dp++ = ESC;
    *dp++ = '(';
    if (ascii_in_char == 0)
      *dp++ = 'B';
    else
      *dp++ = ascii_in_char;
  }
  *dp = 0;
}

/*
 * 半角カナから全角カナへの変換(EUC)
 */

static void
check_kana_euc(sp,dp)
unsigned char *sp;	/* 入力文字列 */
unsigned char *dp;	/* 出力文字列 */
{
  unsigned short kana;

  while( *sp != 0 ) {
    /* ASCII */
    if ( *sp <= DEL ){
      *dp++ = *sp++;
      continue;
    }

    /* 半角カナ */
    if ( *sp == SS2 ) {
      if ( sp[1] < 0xa1 && 0xdf < sp[1] ) {
	sp++;
	continue;		/* 違法なコード */
      }

      sp++;
      kana = hanzen_sjis[(*sp & 0x7f) - 0x21];
      if ( sp[1] == SS2 ) {
	if ( sp[2] == DAKUTEN8 ) {	/* 濁点 */
	  if ( IS_DAKU(*sp) ) {
	    kana++;
	    sp += 2;
	  }
	}
	if ( sp[1] == HANDAKUTEN8 ) {	/* 半濁点 */
	  if ( IS_HANDAKU(*sp) ) {
	    kana +=2;
	    sp += 2;
	  }
	}
      }
      sp++;
      kana = hanzen_euc[((*sp++) & 0x7f) - 0x21];
      *dp++ = (unsigned char)((int)(kana & 0xff00) >> 8);
      *dp++ = (unsigned char)(kana & 0x00ff);
      continue;
    }

    /* 漢字 */
    *dp++ = *sp++;
    *dp++ = *sp++;
  }
  *dp = 0;
}

/*
 * 半角カナから全角カナへの変換(S-JIS)
 */

static void
check_kana_sjis(sp,dp)
unsigned char *sp;	/* 入力文字列 */
unsigned char *dp;	/* 出力文字列 */
{
  unsigned short kana;
  
  while( *sp != 0 ) {
    /* ASCII */
    if ( *sp <= DEL ){
      *dp++ = *sp++;
      continue;
    }

    /* 半角カナ */
    if ( 0xa0 <= *sp && *sp <= 0xdf ){
      kana = hanzen_sjis[(*sp & 0x7f) - 0x21];
      if ( sp[1] == DAKUTEN8 ) {	/* 濁点 */
	if ( IS_DAKU(*sp) ) {
	  kana++;
	  sp++;
	}
      }
      if ( sp[1] == HANDAKUTEN8 ) {	/* 半濁点 */
	if ( IS_HANDAKU(*sp) ) {
	  kana +=2;
	  sp++;
	}
      }
      sp++;
      *dp++ = (unsigned char)((int)(kana & 0xff00) >> 8);
      *dp++ = (unsigned char)(kana & 0x00ff);
      continue;
    }

    /* 漢字 */
    *dp++ = *sp++;
    *dp++ = *sp++;
  }
  *dp = 0;
}
#endif /* HANKAKU2ZENKAKU */
