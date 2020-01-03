/* P96Screen.c

   purpose: P96 Video Out Module

   author:  Henryk Richter <henryk.richter@gmx.net>

   directives:
    PEEK_RTG - if defined, then the code tries to identify
    the vampiregfx driver, and extract the DAC mode in that
    case to switch scanlines on/off
    SCANLINEFLAG - reference external flag for scanline on/off

*/
#define USE_AMMX

#include	<proto/exec.h>
#include	<proto/dos.h>
#include	<proto/intuition.h>
#include	<proto/timer.h>
#include	<proto/graphics.h>
#include	<proto/picasso96.h>

#include	<intuition/intuitionbase.h>
#include	<intuition/screens.h>
#if defined(__GNUC__)
#include	<clib/macros.h>
#endif

#include	<proto/vampire.h>
#include	<vampire/vampire.h>

#include	<math.h>
#include	<stdio.h>
#include	<stdlib.h>

#define PEEK_RTG
#include	"P96Screen.h"
#include	"apolloammxenable.h"

#define CARDDATA_DACMODE 2

/* disable, if undesired */
#define SCANLINEFLAG Render_scanlines

char	screen_title[] = "Cannonball";
LONG	Array[] = { 0, 0, 0 };
WORD	Pens[] = {~0};

BYTE   *cursor = (0);
struct Library	*P96Base = (0);
struct RTGBase *RTGBase = (0);
struct Library *VampireBase = (0);
ULONG  *DACptr = (0); /* pointer to DAC mode */
ULONG  VampFlag = 0;
int	haveammx=0;
#ifdef USE_AMMX
unsigned char Apollo_AMMX2on = 0;
#endif

#ifdef SCANLINEFLAG 
extern int Render_scanlines;
#endif


#if 0
struct TagItem	WindowTags[] = {
	WA_Width, 200,
	WA_Height, 300,
	WA_MinWidth, 100,
	WA_MinHeight, 100,
	WA_MaxWidth, -1,
	WA_MaxHeight, -1,
	WA_SimpleRefresh, TRUE,
	WA_RMBTrap, TRUE,
	WA_Activate, TRUE,
	WA_CloseGadget, TRUE,
	WA_DepthGadget, TRUE,
	WA_DragBar, TRUE,
	WA_SizeGadget, TRUE,
	WA_SizeBBottom, TRUE,
	WA_GimmeZeroZero, TRUE,
	WA_ScreenTitle, (ULONG)screen_title,
	WA_IDCMP, IDCMP_RAWKEY|IDCMP_CLOSEWINDOW,
	TAG_END
};
#endif

char noAMMXStr[] = "AMMX not detected.\n\nThis program requires a Vampire accelerator\nwith Gold2 core or newer.";
char noAMMX2Str[] = "AMMX2 instructions not detected.\n\nThis program requires a Vampire accelerator\nwith Gold2.7 core or newer.";
char noScreenStr[] = "Cannot find suitable screen mode.\nA resolution of 320x240 (or higher) in 16 Bit is required.\nUse Picasso96 Screenmode tool to set up a 16 Bit mode with at least 320x240 pixels.";
char noP96Str[] = "Cannot open Picasso96API.library.\n\nThis software requires a Picasso96 installation,\nwith active 16 Bit screen modes.";
char vamgfxStr[] = "vampiregfx";
#define vamgfxStrLEN 10

/* No AMMX requester */
struct EasyStruct ESnoAMMX = {
      sizeof (struct EasyStruct),
      0,
      screen_title,
      0,
      "OK",
};

#define	random(x)	(rand()/(RAND_MAX/x))
#define min(_a_,_b_) ( (_a_) > (_b_) ) ? _b_ : _a_
#define max(_a_,_b_) ( (_a_) > (_b_) ) ? _a_ : _b_

PROTOHEADER struct P96SC_Handle*P96SC_OpenScreen( ULONG width, ULONG height, ULONG mode, ULONG numbuffers )
{
	struct P96SC_Handle*ret = NULL;
	struct P96SC_Bitmap*bm;
	struct Screen	*sc;
	ULONG  did;
	ULONG fl=0;

	if( !P96Base )
	{
		P96Base=OpenLibrary("Picasso96API.library",2);
		if( !P96Base )
		{
			ESnoAMMX.es_TextFormat = noP96Str;
			EasyRequest( 0, &ESnoAMMX, &fl, 0 );
			return	ret; 
		}
	}
	if( !RTGBase )
	{
		RTGBase = (struct RTGBase*)OpenLibrary("Picasso96/rtg.library",0);
	}
	if( !cursor )
	 cursor = AllocMem(16,MEMF_CHIP|MEMF_CLEAR);
	
	/* detect vampire board based on vampire.resource (Gold3+) */
	VampireBase = OpenResource("vampire.resource");
/*	if( VampireBase )
	 printf("Running on Vampire, direct draw enabled\n");*/

#ifdef USE_AMMX
	if( VampireBase )
	{
		if( !Apollo_AMMX2on )
		{
			if( VampireBase->lib_Version >= 45 )
				if( VRES_ERROR != V_EnableAMMX( V_AMMX_V2 ) )
					Apollo_AMMX2on = 1;
		}
	}
#if 0
	haveammx = Apollo_EnableAMMX( );
	if( !haveammx )
	{
		ESnoAMMX.es_TextFormat = noAMMXStr;
		EasyRequest( 0, &ESnoAMMX, &fl, 0 );
		//printf("AMMX not available, exiting\n");
		return	ret;
	}

/*	{ ULONG i;
          i = Apollo_AMMX2Available( APOLLO_AMMX2F_PMULA );
	  printf("got %d on %d\n",i,APOLLO_AMMX2F_PMULA);
	}*/
	if( APOLLO_AMMX2F_PMULA != Apollo_AMMX2Available( APOLLO_AMMX2F_PMULA ) )
	{
		ESnoAMMX.es_TextFormat = noAMMX2Str;
		EasyRequest( 0, &ESnoAMMX, &fl, 0 );
		return ret;
	}
#endif

#endif
	/* */
	{
	 int found = 0;
	 char*nam;
	 ULONG w,h;
	 ULONG w0 = width;
	 ULONG h0 = height;

	 while( (!found) && (w0 <= (2*width) ))
	 {
		did = p96BestModeIDTags( P96BIDTAG_NominalWidth, w0,
	        	                 P96BIDTAG_NominalHeight, h0,
					 P96BIDTAG_FormatsAllowed, mode,
	                	        /* P96BIDTAG_Depth, depth,
	                	         P96BIDTAG_FormatsForbidden, (RGBFF_R5G5B5|RGBFF_R5G5B5PC|RGBFF_B5G5R5PC),*/
	                	         TAG_DONE);
		w = p96GetModeIDAttr( did, P96IDA_WIDTH );
		h = p96GetModeIDAttr( did, P96IDA_HEIGHT );
		nam=(char*)p96GetModeIDAttr( did, P96IDA_BOARDNAME );
		if( w >= width )
		{
			if( h >= height )
				found = 1;
		}
		if( !found )
		{
			w0 += 100;
			h0 += 100;
		}

	 }
	 if( !found )
	 {
	 	if( !(numbuffers & P96SC_SILENT) )
		{
			ESnoAMMX.es_TextFormat = noScreenStr;
			EasyRequest( 0, &ESnoAMMX, &fl, 0 );
		}
		//printf("Cannot find Screenmode for %d x %d\n",width,height);
		return NULL;
	 }
	 //fprintf(stderr,"Got Screenmode %d x %d\n",w,h);
	 if( h > height )
	 	height = h;

	 /* check if our mode is a vampiregfx mode */
	 VampFlag = 0;
	 if( nam )
	 {
	 	if( !Stricmp( vamgfxStr, nam ) )
		 	VampFlag = 1;
/*	 	for( w=0 ; w<vamgfxStrLEN ; w++ )
			if( vamgfxStr[w] != nam[w] )
				VampFlag = 0;*/
	 }
	 //Printf("Vamp Flag is %ld\n",VampFlag);
	}

	if(sc = p96OpenScreenTags(
					P96SA_Width, width,
					P96SA_Height, height,
					P96SA_DisplayID, did,
					/*P96SA_Depth, 16,*/
					P96SA_AutoScroll, TRUE,
					P96SA_Pens, (ULONG)Pens,
					P96SA_Title, (ULONG)screen_title,
					TAG_DONE,0))
	{
		//fprintf(stderr,"screen open\n");

		ret = AllocVec( sizeof( struct P96SC_Handle ), MEMF_FAST|MEMF_CLEAR );
		if( !ret )
		{
			p96CloseScreen( sc );
			return ret;
		}
		NewList( (struct List*)&ret->buffers );
		ret->screen = sc;
		ret->width  = width;
		ret->height = height;

		//fprintf(stderr,"have handle\n");

		/* get DAC mode pointer on Vamp (vampiregfx driver checked above) */
		DACptr = (0); /* pointer to DAC mode */
		if( (VampFlag) && (RTGBase) ) 
		{
			int n = (int)RTGBase->Board_Count; /* find Vampiregfx board */
			struct BoardInfo *bo;
			
			while( --n >= 0 )
			{
			 bo = RTGBase->Boards[n];
			 if( bo )
			 {
				if( !Stricmp( bo->BoardName, vamgfxStr ) )
				{
		 			DACptr = &bo->CardData[CARDDATA_DACMODE];
		 			break;
		 		}
			 }
			}
		}
		//Printf("DAC Ptr is %ld\n",(ULONG)DACptr );

		/* obtain screen Bytes per row and memory base pointer */
		{
			struct RenderInfo ri;
			LONG bl;

			if( (bl = p96LockBitMap( sc->RastPort.BitMap, (APTR)&ri, sizeof(ri) ) ))
			{
				ret->screenbitmap = ri.Memory;
				ret->BytesPerRow = ri.BytesPerRow; 
				p96UnlockBitMap( sc->RastPort.BitMap, bl );
				//fprintf(stderr,"have bitmap\n");
			}
		}

		ret->window = OpenWindowTags(NULL, 
		                             WA_CustomScreen, (ULONG)sc,
					     WA_Title, (ULONG)screen_title,
					     WA_Left, 0,
					     WA_Top, 0,
					     WA_Width, ret->width,
					     WA_Height, ret->height,
					     WA_Activate, TRUE,
					     WA_CloseGadget, FALSE,
					     WA_Borderless, TRUE,
					     WA_SimpleRefresh, TRUE,
					     WA_RMBTrap, TRUE,
					     WA_Backdrop, TRUE,
					     WA_IDCMP, IDCMP_RAWKEY|IDCMP_CLOSEWINDOW,
					     TAG_DONE );
			//fprintf(stderr,"have window\n");

		{ /* TODO: separate function */
			int i,err,sz;
			ULONG aln;


		sz = height*ret->BytesPerRow + sizeof( struct P96SC_Bitmap ) + P96SC_ALIGN;
			if( numbuffers & P96SC_DOUBLESIZE )
				sz<<=1;
			if( numbuffers & P96SC_8MORELINES )
				sz += (ret->BytesPerRow<<3);
			numbuffers &= P96SC_NUMBUF_MSK; /* you won't need more than 64k buffers, right? */

			for( i=0,err=0 ; i < numbuffers ; i++ )
			{
				bm = AllocMem( sz, MEMF_FAST|MEMF_CLEAR );
				if( bm )
				{
					bm->allocptr = bm;
					bm->allocsize= sz;
					aln = ( (ULONG)bm + sizeof( struct P96SC_Bitmap ) + (P96SC_ALIGN-1) ) & ~(P96SC_ALIGN-1);
					bm->ptr = (APTR)aln;
					AddTail( (struct List*)&ret->buffers, &bm->n );
				}
				else err = 1;
			}

			if( cursor )
				SetPointer( ret->window, (UWORD*)cursor, 1,1,0,0);

			if( err )
			{
				P96SC_CloseScreen( ret );
				ret = NULL;
			}
		}
	}

	return	ret;
}



/* close Screen and free bitmaps */
PROTOHEADER int P96SC_CloseScreen( struct P96SC_Handle*han )
{
	struct P96SC_Bitmap*bm;

	if( !han )
		return	P96SC_EP96;

	if( han->window )
	{
		Forbid();
		{
			struct Message	*msg;
			while( (msg = GetMsg(han->window->UserPort))) 
				ReplyMsg(msg);
		}
		Permit();
		CloseWindow( han->window );
	}

	if( han->screen )
		p96CloseScreen( han->screen );

	while( ( bm = (struct P96SC_Bitmap*)RemHead( (struct List*)&han->buffers ) ) )
	{
		FreeMem( bm->allocptr, bm->allocsize );
	}

	FreeVec( han );

	return	P96SC_OK;
}



/* show specific buffer */
/* Hack: if vampire.resource is found, bang hardware registers directly when
         screen is in front */
PROTOHEADER int P96SC_ShowBuffer( struct P96SC_Handle*han, struct P96SC_Bitmap *buf )
{
	struct  Screen	*sc;
	struct  RenderInfo ri;
	ULONG	bl;
	int     direct = 0;

	if( !han )
		return P96SC_EP96;

	sc = han->screen;

	/* only on Vampire, only when own screen is in front
	   (check haveammx, too because Vampire.resource doesn't ship yet with current cores...)
	*/
	if( VampireBase || haveammx  )
	{
		if( IntuitionBase->FirstScreen == sc )
			direct = 1;
	}	

	if( direct )
	{
		*(volatile unsigned long*)0xDFF1ec = (unsigned long)buf->ptr;

#ifdef SCANLINEFLAG
		if( DACptr )
		{
			if( SCANLINEFLAG )
				*(volatile unsigned short*)0xDFF1F4 = *DACptr | (1<<10);
			else
				*(volatile unsigned short*)0xDFF1F4 = *DACptr & ~(1<<10);
		}
#endif
		/* printf("d"); */
	}
	else
	{
		/* printf("i"); */
		
		if( (bl = p96LockBitMap( sc->RastPort.BitMap, (APTR)&ri, sizeof(ri) ) ))
		{
				memcpy( ri.Memory, buf->ptr, han->BytesPerRow * han->height );

				p96UnlockBitMap( sc->RastPort.BitMap, bl );
		}
	}

	return	P96SC_OK;
}

PROTOHEADER void P96SC_Cleanup( )
{
	if( cursor )
		FreeMem(cursor,16);
		
	if( RTGBase )
		CloseLibrary( (struct Library*)RTGBase );
	RTGBase = NULL;

	if( P96Base )
		CloseLibrary( P96Base );
	P96Base = NULL;
}

/*
  returns 
*/
unsigned char keys[256];

PROTOHEADER int  P96SC_HandleMessages( struct P96SC_Handle*han )
{
	struct IntuiMessage	*imsg;
	int terminate = P96SC_OK;
	int code;

	while(imsg=(struct IntuiMessage *)GetMsg(han->window->UserPort))
	{
		if( imsg->Class==IDCMP_CLOSEWINDOW )
			terminate = P96SC_QUIT;
		if( imsg->Class==IDCMP_RAWKEY )
		{
			code = imsg->Code;

			if( code&IECODE_UP_PREFIX )
			{
				code &= ~IECODE_UP_PREFIX;
				keys[ code & 0xff ] = 0;
			}
			else
			{	
				keys[ code & 0xff ] = 32;
			}
		}
		ReplyMsg((struct Message *)imsg);
	}
	if(0){ /* test: timer for keydown */
		int i;
		for(i=0 ; i < 255 ; i++ )
		{
			if( keys[i] ) 
				keys[i]--;
		}
	}

	return terminate;
}




/* fill 16 bit pic with crap */
PROTOHEADER void FillPic( unsigned char *ptr, long width, long height, long bytesperrow )
{
 long i,j;
 unsigned short *p;
 long k = (long)ptr;

 for( j=0 ; j < height ; j++ )
 {
 	p = (unsigned short*)ptr;
	for(i = 0 ; i < width ; i++ )
	{
		p[i] = (short)(i +  k);
	}
	ptr += bytesperrow;
 }

}

#if 0 
const ULONG modelist[] = { RGBFF_R5G6B5,RGBFF_R5G6B5PC,RGBFF_R5G5B5,RGBFF_R5G5B5PC, 0 };

int main(void)
{
 struct P96SC_Handle*scr;
 LONG modeidx;
 struct P96SC_Bitmap*bm;
 unsigned char *pix;

 modeidx = -1;
 scr = 0;
 while( !scr )
 {
	modeidx++;
	if( !modelist[modeidx] )
		break;
 	scr = P96SC_OpenScreen( 320, 200, modelist[modeidx] , 3|P96SC_SILENT );
 }

 if( scr ) /* we only get a screen handle when the requested buffers are available */
 {
	struct IntuiMessage	*imsg;
	int terminate = 0;

	while( terminate == 0 )
	{
		Delay(1);

		/* do something with buffers */
		bm = (struct P96SC_Bitmap*)RemHead( (struct List*)&scr->buffers );
		pix = bm->ptr;
		FillPic( pix, scr->width, scr->height, scr->BytesPerRow );
	
		P96SC_ShowBuffer( scr, bm );
		/* evil */
		//memcpy( scr->screenbitmap, pix, scr->height * scr->BytesPerRow );

		AddTail( (struct List*)&scr->buffers, &bm->n );

		while(imsg=(struct IntuiMessage *)GetMsg(scr->window->UserPort))
		{
			if( (imsg->Class==IDCMP_CLOSEWINDOW) ||
			    ((imsg->Class==IDCMP_RAWKEY) && ((imsg->Code==0x40) || (imsg->Code==0x45))))
			{
				// Close window, press SPACE bar or ESCAPE key to end program
				terminate=1;
			}
			ReplyMsg((struct Message *)imsg);
		}
	}

 	P96SC_CloseScreen(scr);
 }

 P96SC_Cleanup();


#if 0
	if(P96Base=OpenLibrary("Picasso96API.library",2)){
		struct RDArgs	*rda;
		struct Screen	*sc;

		LONG	Width = 640, Height = 480, Depth = 8;
		int	i;
		
		if(sc = p96OpenScreenTags(
										P96SA_Width, Width,
										P96SA_Height, Height,
										P96SA_Depth, Depth,
										P96SA_AutoScroll, TRUE,
										P96SA_Pens, (ULONG)Pens,
										P96SA_Title, (ULONG)screen_title,
										TAG_DONE)){

			struct Window	*wdf, *wdp;
			WORD	Dimensions[4];

			Dimensions[0] = 0;
			Dimensions[1] = sc->BarHeight+1;
			Dimensions[2] = sc->Width;
			Dimensions[3] = sc->Height-sc->BarHeight-1;

			wdp = OpenWindowTags(NULL,	WA_CustomScreen, (ULONG)sc,
												WA_Title, (ULONG)"WritePixel",
												WA_Left,(sc->Width/2-200)/2 + sc->Width/2,
												WA_Top,(sc->Height-sc->BarHeight-300)/2,
												WA_Zoom, (ULONG)&Dimensions,
												TAG_MORE, (ULONG)WindowTags);

			wdf = OpenWindowTags(NULL,	WA_CustomScreen, (ULONG)sc,
												WA_Title, (ULONG)"FillRect",
												WA_Left,(sc->Width/2-200)/2,
												WA_Top,(sc->Height-sc->BarHeight-300)/2,
												WA_Zoom, (ULONG)&Dimensions,
												TAG_MORE, (ULONG)WindowTags);

			if(wdf && wdp){
				struct RastPort	*rpf = wdf->RPort;
				struct RastPort	*rpp = wdp->RPort;
				BOOL	terminate = FALSE;
				ULONG	signals = ((1<<wdf->UserPort->mp_SigBit) | (1<<wdp->UserPort->mp_SigBit));
				RGBFTYPE	format = (RGBFTYPE)p96GetBitMapAttr(sc->RastPort.BitMap, P96BMA_RGBFORMAT);

				srand((unsigned int)wdf + (unsigned int)wdp);

				do{
					WORD x1, y1, x2, y2, x3, y3;

					x1 = random(wdf->Width);
					y1 = random(wdf->Height);
					x2 = random(wdf->Width);
					y2 = random(wdf->Height);
					x3 = random(wdp->Width);
					y3 = random(wdp->Height);

					if(format == RGBFB_CLUT){
						SetAPen(rpf, random(255));
						RectFill(rpf, min(x1,x2), min(y1,y2), max(x1,x2), max(y1,y2));

						SetAPen(rpp, random(255));
						WritePixel(rpp, x3, y3);
					}else{
						p96RectFill(rpf, min(x1,x2), min(y1,y2), max(x1,x2), max(y1,y2),
										((random(255))<<16)|((random(255))<<8)|(random(255)));
						p96WritePixel(rpp, x3, y3,
										((random(255))<<16)|((random(255))<<8)|(random(255)));
					}
					if(SetSignal(0L, signals) & signals){
						struct IntuiMessage	*imsg;

						while(imsg=(struct IntuiMessage *)GetMsg(wdf->UserPort)){
							if((imsg->Class==IDCMP_CLOSEWINDOW) ||
								((imsg->Class==IDCMP_RAWKEY) && ((imsg->Code==0x40) || (imsg->Code==0x45)))){
								// Close window, press SPACE bar or ESCAPE key to end program
								terminate=TRUE;
							}
							ReplyMsg((struct Message *)imsg);
						}

						while(imsg=(struct IntuiMessage *)GetMsg(wdp->UserPort)){
							if((imsg->Class==IDCMP_CLOSEWINDOW) ||
								((imsg->Class==IDCMP_RAWKEY) && ((imsg->Code==0x40) || (imsg->Code==0x45)))){
								// Close window, press SPACE bar or ESCAPE key to end program
								terminate=TRUE;
							}
							ReplyMsg((struct Message *)imsg);
						}
					}
				}while(!terminate);

				Forbid();
				{
					struct Message	*msg;
					while(msg = GetMsg(wdf->UserPort)) ReplyMsg(msg);
					while(msg = GetMsg(wdp->UserPort)) ReplyMsg(msg);
				}
				Permit();

			}
			if(wdf) CloseWindow(wdf);
			if(wdp) CloseWindow(wdp);

			p96CloseScreen(sc);
		}
		CloseLibrary(P96Base);
	}
	return 0;
#endif
}
#endif
