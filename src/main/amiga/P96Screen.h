/*
  file: P96Screen.h

  author: Henryk Richter <henryk.richter@gmx.net>

  The routines in here assume that the calling program is able to handle different
  color formats. Hence, no internal conversion takes place. As a consequence, when
  a screen of specified mode could not be opened, try another you support.


*/
#ifndef _INC_P96SCREEN_H_
#define _INC_P96SCREEN_H_

#include <exec/lists.h>
#include <exec/libraries.h>
#include <libraries/Picasso96.h>

#ifdef __cplusplus
#define PROTOHEADER extern "C"
#else
#define PROTOHEADER
#endif

#define P96SC_OK	1	/* all fine */
#define P96SC_EP96	-1	/* P96 not found */
#define P96SC_EDIM	-2	/* dimension not available (not even close, nothing) */
#define P96SC_EMODE	-3	/* depth unavailable */
#define P96SC_QUIT	2	/* Quit "window close" */

#define P96SC_ALIGN	32	/* change to anything 2^n , if necessary */
#define P96SC_DOUBLESIZE 0x2000000 /* doublesize buffers for bitmaps ? */
#define P96SC_8MORELINES 0x1000000 /* allocate memory for 8 more lines */
#define P96SC_SILENT     0x0800000 /* don't complain if screenmode is unavailable */
#define P96SC_NUMBUF_MSK 0xffff	   /* you won't need more than 64k buffers, right? */

/* handler struct (read only) */
struct P96SC_Handle {
	APTR		screen;
	struct Window*	window;
	ULONG		width;		/* actual width */
	ULONG		height;		/* actual height */
	ULONG		BytesPerRow;	/* offset (in bytes) from one row to the next */
	struct  MinList buffers;	/* pointers to buffers */
	/* PRIVATE */
	APTR		screenbitmap;	/* old memory base */
};

/* bitmap struct */
struct P96SC_Bitmap {
	struct	Node	n;
	APTR		ptr;		/* pixel array to be used */
	/* private, for dealloc */
	APTR		allocptr;
	ULONG		allocsize;
};

/*
  Open P96 screen of specified dimensions (at least), with specified mode and the
  given number of display buffers
*/
PROTOHEADER struct P96SC_Handle*P96SC_OpenScreen( ULONG width, ULONG height, ULONG mode, ULONG numbuffers );

/* close Screen and free bitmaps */
PROTOHEADER int P96SC_CloseScreen( struct P96SC_Handle*han );

/* show specific buffer */
PROTOHEADER int P96SC_ShowBuffer( struct P96SC_Handle*han, struct P96SC_Bitmap *buf );

PROTOHEADER void P96SC_Cleanup( void );

PROTOHEADER int  P96SC_HandleMessages( struct P96SC_Handle*han );

/* optional: identify RTG board from RTG base (you haven't seen this) */
#ifdef PEEK_RTG

#define MAXNBOARDS 10

/* can't ship boardinfo in GPL code... */
/* #include "boardinfo.h" */ 

/* skip stuff below if the real boardinfo is loaded */
#ifndef boardinfo_H

/* minimal boardinfo compatibility definition */
struct BoardInfo {
 UBYTE *RegisterBase;
 UBYTE *MemoryBase;
 UBYTE *MemoryIOBase;
 ULONG  MemorySize;
 char  *BoardName;   /* 20 */
 char   VBIName[32]; /* 32 */
 char   undefined_in_here[1418-20-32];
 ULONG  ChipData[16];
 ULONG  CardData[16];
};

#define MAXNBOARDS 10

/* partial reconstruction of RTG base (incomplete and highly volatile analysis, I hazard) */
struct RTGBase {
        struct Library      LibNode;
        UBYTE               Flag_Byte;
        UBYTE               Board_Count;

        struct Library      *SysBase;
        struct Library      *DosBase;
        struct Library      *GfxBase;
        struct Library      *IntuitionBase;
        struct Library      *IFFParseBase;
        struct Library      *UtilityBase;

        ULONG               Seglist;

  	struct BoardInfo    *Boards[MAXNBOARDS];

        UBYTE                Struct1[48];
        APTR                 Table1[MAXNBOARDS];
        APTR                 Table2[MAXNBOARDS];

        APTR                 MemoryPtr;
        APTR                 Table3[128];
        struct Library       *LayersBase;

        APTR                 Table4[16];
        APTR                 LibTask;
        ULONG                RTGFlags;
        struct Library       *IconBase;
};

#endif /* boardinfo_H */
#endif /* PEEK_RTG */


#endif /* _INC_P96SCREEN_H_ */
