/***************************************************************************
    Amiga Software Video Rendering.  
    
    Known Bugs:
    - Scanlines work only on Vampire (soon :-)

    Copyright Henryk Richter 
    See license.txt for more details.
***************************************************************************/

#include "frontend/config.h"

#include "../globals.h"
#include "../setup.h"

//#include "amiga/P96SDLAdapter.h"
#include "amiga/P96Screen.h"
#include <stdio.h>

uint32_t my_min(uint32_t a, uint32_t b) { return a < b ? a : b; }

struct P96SC_Handle*screen = (0);

// Palette Lookup
uint32_t Render_rgb[S16_PALETTE_ENTRIES * 3];    // Extended to hold shadow/hilight colours

uint16_t *Render_screen_pixels;

uint16_t Render_byteswap;

// Original Screen Width & Height
uint16_t Render_orig_width, Render_orig_height;

// --------------------------------------------------------------------------------------------
// Screen setup properties. Example below: 
// ________________________
// |  |                |  | <- screen size      (e.g. 1280 x 720)
// |  |                |  |
// |  |                |<-|--- destination size (e.g. 1027 x 720) to maintain aspect ratio
// |  |                |  | 
// |  |                |  |    source size      (e.g. 320  x 224) System 16 proportions
// |__|________________|__|
//
// --------------------------------------------------------------------------------------------

// Source texture / pixel array that we are going to manipulate
int Render_src_width, Render_src_height;

// Destination window width and height
int Render_dst_width, Render_dst_height;

int Render_topleft; /* centering on screen */

// Screen width and height 
//int Render_scn_width, Render_scn_height;
// Full-Screen, Stretch, Window
int Render_video_mode;

// Scanline density. 0 = Off, 1 = Full
int Render_scanlines;

// Offsets (for full-screen mode, where x/y resolution isn't a multiple of the original height)
//uint32_t Render_screen_xoff, Render_screen_yoff;

// SDL Pixel Format Codes. These differ between platforms.
uint8_t  Render_Rshift, Render_Gshift, Render_Bshift;
uint32_t Render_Rmask, Render_Gmask, Render_Bmask;

struct P96SC_Bitmap *curbuf;

// Scanline pixels
//uint16_t* Render_scan_pixels = NULL;

// Scale the screen
//int Render_scale_factor;

void Render_Quit( )
{
	if( screen )
	{
	   if( curbuf )
		AddTail( (struct List*)&screen->buffers, &curbuf->n );
	   P96SC_CloseScreen( screen );
	   screen = (0);
	   curbuf = (0);
	   P96SC_Cleanup();
	}
}

Boolean Render_init(int src_width, int src_height, 
                    int scale,
                    int video_mode,
                    int scanlines)
{
    Render_src_width  = src_width;
    Render_src_height = src_height;
    Render_video_mode = video_mode;
    Render_scanlines  = scanlines;
    Render_topleft = 0;

    if( screen )
    {
    	if( curbuf )
		AddTail( (struct List*)&screen->buffers, &curbuf->n );
    	P96SC_CloseScreen( screen );
    }
    curbuf = (0);

    Render_byteswap   = 0;
    screen = P96SC_OpenScreen( src_width, src_height, RGBFF_R5G6B5|(1<<31), 2|P96SC_SILENT );
    if( !screen )
    {
	Render_byteswap = 1;
	screen = P96SC_OpenScreen( src_width, src_height, RGBFF_R5G6B5PC, 2 );
	if( !screen )
	{
		fprintf(stderr,"Error: Cannot open 16 Bit screen using Picasso96\n");
		return FALSE;
	}
	fprintf(stderr,"Opened screen with requested mode %ld (16 Bit PC)\n",(ULONG)RGBFF_R5G6B5PC); 
    }
    else
	fprintf(stderr,"Opened screen with requested mode %ld (16 Bit native)\n",(ULONG)RGBFF_R5G6B5); 

    fprintf(stderr,"requested size %ld x %ld\n",src_width,src_height);
    Render_dst_width = src_width;
    Render_dst_height= src_height;

    Render_Rshift = 11;
    Render_Gshift = 5;
    Render_Bshift = 0;
    Render_Rmask  = 0xf800;
    Render_Gmask  = 0x07e0;
    Render_Bmask  = 0x001f;

    // Doubled intermediate pixel array for scanlines
//    if (scanlines)

    return TRUE;
}

void Render_disable()
{

}

Boolean Render_start_frame()
{
	if( !screen )
		return FALSE;
	if( !curbuf )
		curbuf = (struct P96SC_Bitmap*)RemHead( (struct List*)&screen->buffers );
	return TRUE;
}

Boolean Render_finalize_frame()
{
	if( !screen )
		return FALSE;
	if( curbuf )
	{
		P96SC_ShowBuffer( screen, curbuf );
		AddTail( (struct List*)&screen->buffers, &curbuf->n );
	}
	curbuf = (struct P96SC_Bitmap*)RemHead( (struct List*)&screen->buffers );
	return TRUE;
}

void Render_draw_frame(uint16_t* pixels)
{
    int i,j,add,addp;
    uint16_t* spix;
    //= Render_screen_pixels;

    if( !curbuf ) /* this happens at least once, called without Render_start_frame() */
    {
//	fprintf(stderr,"error! no curbuf for Render_draw_frame\n");
	return;
    }

//    fprintf(stderr,"render size %ld x %ld\n",src_width,src_height);

    spix = (uint16_t*)curbuf->ptr + Render_topleft;

    add  = (screen->BytesPerRow>>1)-(Render_dst_width&0xfffffff0); /* in uint16 units */
    addp = Render_dst_width&15;
    // Lookup real RGB value from rgb array for backbuffer
    // 320*224 = 71680 = 0x1180
    for (j = 0; j < Render_dst_height ; j++ )
    {
     for (i = 0; i < (Render_dst_width>>4) ; i++)
     {
        *(spix++) = Render_rgb[*(pixels++) & ((S16_PALETTE_ENTRIES * 3) - 1)];
        *(spix++) = Render_rgb[*(pixels++) & ((S16_PALETTE_ENTRIES * 3) - 1)];
        *(spix++) = Render_rgb[*(pixels++) & ((S16_PALETTE_ENTRIES * 3) - 1)];
        *(spix++) = Render_rgb[*(pixels++) & ((S16_PALETTE_ENTRIES * 3) - 1)];
        *(spix++) = Render_rgb[*(pixels++) & ((S16_PALETTE_ENTRIES * 3) - 1)];
        *(spix++) = Render_rgb[*(pixels++) & ((S16_PALETTE_ENTRIES * 3) - 1)];
        *(spix++) = Render_rgb[*(pixels++) & ((S16_PALETTE_ENTRIES * 3) - 1)];
        *(spix++) = Render_rgb[*(pixels++) & ((S16_PALETTE_ENTRIES * 3) - 1)];
        *(spix++) = Render_rgb[*(pixels++) & ((S16_PALETTE_ENTRIES * 3) - 1)];
        *(spix++) = Render_rgb[*(pixels++) & ((S16_PALETTE_ENTRIES * 3) - 1)];
        *(spix++) = Render_rgb[*(pixels++) & ((S16_PALETTE_ENTRIES * 3) - 1)];
        *(spix++) = Render_rgb[*(pixels++) & ((S16_PALETTE_ENTRIES * 3) - 1)];
        *(spix++) = Render_rgb[*(pixels++) & ((S16_PALETTE_ENTRIES * 3) - 1)];
        *(spix++) = Render_rgb[*(pixels++) & ((S16_PALETTE_ENTRIES * 3) - 1)];
        *(spix++) = Render_rgb[*(pixels++) & ((S16_PALETTE_ENTRIES * 3) - 1)];
        *(spix++) = Render_rgb[*(pixels++) & ((S16_PALETTE_ENTRIES * 3) - 1)];
     }
     pixels+=addp;
     spix+=add;
    }
}

 
#define CURRENT_RGB() (r << Render_Rshift) | (g << Render_Gshift) | (b << Render_Bshift);

void Render_convert_palette(uint32_t adr, uint32_t r, uint32_t g, uint32_t b)
{
    USHORT currgb;

    adr >>= 1;

    g<<=1; /* quick fix for 16 bit screens */

    currgb = CURRENT_RGB();

    if( Render_byteswap )
    	currgb = (0xff&(currgb>>8)) | (currgb<<8);

    Render_rgb[adr] = currgb;
      
    // Create shadow / highlight colours at end of RGB array
    // The resultant values are the same as MAME
    r = r * 202 / 256;
    g = g * 202 / 256;
    b = b * 202 / 256;

    currgb = CURRENT_RGB();

    if( Render_byteswap )
    	currgb = (0xff&(currgb>>8)) | (currgb<<8);

    Render_rgb[adr + S16_PALETTE_ENTRIES] =
    Render_rgb[adr + (S16_PALETTE_ENTRIES * 2)] = currgb;
}
