#include "globals.h"
#include "romloader.h"
#include "hwvideo/hwtiles.h"
#include "frontend/config.h"

#include <string.h>
#ifdef _AMIGA_
#include "amiga/asminterface.h"
#include "amiga/apolloammxenable.h"
#include "amiga/ammxtilerender.h"
#endif

/***************************************************************************
    Video Emulation: OutRun Tilemap Hardware.
    Based on MAME source code.

    Copyright Aaron Giles.
    All rights reserved.
***************************************************************************/

/*******************************************************************************************
 *
 *  System 16B-style tilemaps
 *
 *  16 total pages
 *  Column/rowscroll enabled via bits in text layer
 *  Alternate tilemap support
 *
 *  Tile format:
 *      Bits               Usage
 *      p------- --------  Tile priority versus sprites
 *      -??----- --------  Unknown
 *      ---ccccc cc------  Tile color palette
 *      ---nnnnn nnnnnnnn  Tile index
 *
 *  Text format:
 *      Bits               Usage
 *      p------- --------  Tile priority versus sprites
 *      -???---- --------  Unknown
 *      ----ccc- --------  Tile color palette
 *      -------n nnnnnnnn  Tile index
 *
 *  Alternate tile format:
 *      Bits               Usage
 *      p------- --------  Tile priority versus sprites
 *      -??----- --------  Unknown
 *      ----cccc ccc-----  Tile color palette
 *      ---nnnnn nnnnnnnn  Tile index
 *
 *  Alternate text format:
 *      Bits               Usage
 *      p------- --------  Tile priority versus sprites
 *      -???---- --------  Unknown
 *      -----ccc --------  Tile color palette
 *      -------- nnnnnnnn  Tile index
 *
 *  Text RAM:
 *      Offset   Bits               Usage
 *      E80      aaaabbbb ccccdddd  Foreground tilemap page select
 *      E82      aaaabbbb ccccdddd  Background tilemap page select
 *      E84      aaaabbbb ccccdddd  Alternate foreground tilemap page select
 *      E86      aaaabbbb ccccdddd  Alternate background tilemap page select
 *      E90      c------- --------  Foreground tilemap column scroll enable
 *               -------v vvvvvvvv  Foreground tilemap vertical scroll
 *      E92      c------- --------  Background tilemap column scroll enable
 *               -------v vvvvvvvv  Background tilemap vertical scroll
 *      E94      -------v vvvvvvvv  Alternate foreground tilemap vertical scroll
 *      E96      -------v vvvvvvvv  Alternate background tilemap vertical scroll
 *      E98      r------- --------  Foreground tilemap row scroll enable
 *               ------hh hhhhhhhh  Foreground tilemap horizontal scroll
 *      E9A      r------- --------  Background tilemap row scroll enable
 *               ------hh hhhhhhhh  Background tilemap horizontal scroll
 *      E9C      ------hh hhhhhhhh  Alternate foreground tilemap horizontal scroll
 *      E9E      ------hh hhhhhhhh  Alternate background tilemap horizontal scroll
 *      F16-F3F  -------- vvvvvvvv  Foreground tilemap per-16-pixel-column vertical scroll
 *      F56-F7F  -------- vvvvvvvv  Background tilemap per-16-pixel-column vertical scroll
 *      F80-FB7  a------- --------  Foreground tilemap per-8-pixel-row alternate tilemap enable
 *               -------h hhhhhhhh  Foreground tilemap per-8-pixel-row horizontal scroll
 *      FC0-FF7  a------- --------  Background tilemap per-8-pixel-row alternate tilemap enable
 *               -------h hhhhhhhh  Background tilemap per-8-pixel-row horizontal scroll
 *
 *******************************************************************************************/

uint8_t HWTiles_text_ram[0x1000]; // Text RAM
uint8_t HWTiles_tile_ram[0x10000]; // Tile RAM

int16_t HWTiles_x_clamp;
    
// S16 Width, ignoring widescreen related scaling.
int16_t HWTiles_s16_width_noscale;

#define TILES_LENGTH 0x10000
uint32_t HWTiles_tiles[TILES_LENGTH];        // Converted tiles
uint32_t HWTiles_tiles_backup[TILES_LENGTH]; // Converted tiles (backup without patch)

uint16_t HWTiles_page[4];
uint16_t HWTiles_scroll_x[4];
uint16_t HWTiles_scroll_y[4];

uint8_t HWTiles_tile_banks[2] = { 0, 1 };

static const uint16_t NUM_TILES = 0x2000; // Length of graphic rom / 24
static const uint16_t TILEMAP_COLOUR_OFFSET = 0x1c00;
   
#ifndef _AMIGA_
#define SAVEDS
#define ASMR(_a_)
#define ASMREG(_a_)
#define ASM
#endif

ASM SAVEDS void (*HWTiles_render8x8_tile_maskargs)(ASMR(a2) hwtilerenderarg*args ASMREG(a2) );
ASM SAVEDS void (*HWTiles_render8x8_tile_mask_clipargs)(ASMR(a2) hwtilerenderarg*args ASMREG(a2) );
ASM SAVEDS void HWTiles_render8x8_tile_mask_loresargs(ASMR(a2) hwtilerenderarg*args ASMREG(a2) );
ASM SAVEDS void HWTiles_render8x8_tile_mask_clip_loresargs(ASMR(a2) hwtilerenderarg*args ASMREG(a2) );
ASM SAVEDS void HWTiles_render8x8_tile_mask_hiresargs(ASMR(a2) hwtilerenderarg*args ASMREG(a2) );
ASM SAVEDS void HWTiles_render8x8_tile_mask_clip_hiresargs(ASMR(a2) hwtilerenderarg*args ASMREG(a2) );

#if 0
void (*HWTiles_render8x8_tile_mask)(
    uint16_t *buf,
    uint16_t nTileNumber, 
    uint16_t StartX, 
    uint16_t StartY, 
    uint16_t nTilePalette, 
    uint16_t nColourDepth, 
    uint16_t nMaskColour, 
    uint16_t nPaletteOffset); 
void (*HWTiles_render8x8_tile_mask_clip)(
    uint16_t *buf,
    uint16_t nTileNumber, 
    int16_t StartX, 
    int16_t StartY, 
    uint16_t nTilePalette, 
    uint16_t nColourDepth, 
    uint16_t nMaskColour, 
    uint16_t nPaletteOffset); 
#endif

#if 0       
void HWTiles_render8x8_tile_mask_lores(
    uint16_t *buf,
    uint16_t nTileNumber, 
    uint16_t StartX, 
    uint16_t StartY, 
    uint16_t nTilePalette, 
    uint16_t nColourDepth, 
    uint16_t nMaskColour, 
    uint16_t nPaletteOffset); 

void HWTiles_render8x8_tile_mask_clip_lores(
    uint16_t *buf,
    uint16_t nTileNumber, 
    int16_t StartX, 
    int16_t StartY, 
    uint16_t nTilePalette, 
    uint16_t nColourDepth, 
    uint16_t nMaskColour, 
    uint16_t nPaletteOffset);
#endif

void HWTiles_render8x8_tile_mask_hires(
    uint16_t *buf,
    uint32_t *tileptr,
    uint16_t StartX, 
    uint16_t StartY, 
    uint16_t nTilePalette, 
    uint16_t nColourDepth, 
    uint16_t nMaskColour, 
    uint16_t nPaletteOffset); 
        
void HWTiles_render8x8_tile_mask_clip_hires(
    uint16_t *buf,
    uint32_t *tileptr,
    int16_t StartX, 
    int16_t StartY, 
    uint16_t nTilePalette, 
    uint16_t nColourDepth, 
    uint16_t nMaskColour, 
    uint16_t nPaletteOffset);
        
 void HWTiles_set_pixel_x4(uint16_t *buf, uint32_t data);

void HWTiles_Create(void)
{
    int i;
    for (i = 0; i < 2; i++)
        HWTiles_tile_banks[i] = i;

    HWTiles_set_x_clamp(HWTILES_CENTRE);
}

void HWTiles_Destroy(void)
{

}

// Convert S16 tiles to a more useable format
void HWTiles_init(uint8_t* src_tiles, const Boolean hires)
{
    int i;
    int ii;
    if (src_tiles)
    {
        for (i = 0; i < TILES_LENGTH; i++)
        {
            uint8_t p0 = src_tiles[i];
            uint8_t p1 = src_tiles[i + 0x10000];
            uint8_t p2 = src_tiles[i + 0x20000];

            uint32_t val = 0;

            for (ii = 0; ii < 8; ii++) 
            {
                uint8_t bit = 7 - ii;
                uint8_t pix = ((((p0 >> bit)) & 1) | (((p1 >> bit) << 1) & 2) | (((p2 >> bit) << 2) & 4));
                val = (val << 4) | pix;
            }
            HWTiles_tiles[i] = val; // Store converted value
        }
        memcpy(HWTiles_tiles_backup, HWTiles_tiles, TILES_LENGTH * sizeof(uint32_t));
    }
    
    if (hires)
    {
        HWTiles_s16_width_noscale = Config_s16_width >> 1;
	HWTiles_render8x8_tile_maskargs = &HWTiles_render8x8_tile_mask_hiresargs;
	HWTiles_render8x8_tile_mask_clipargs=&HWTiles_render8x8_tile_mask_clip_hiresargs;
    }
    else
    {
        HWTiles_s16_width_noscale = Config_s16_width;
#ifdef _AMIGA_
	if( Apollo_AMMX2on )
	{
		HWTiles_render8x8_tile_maskargs = &HWTiles_render8x8_tile_mask_loresammx;
		//HWTiles_render8x8_tile_mask_clipargs=&HWTiles_render8x8_tile_mask_loresclipammx;
	        HWTiles_render8x8_tile_mask_clipargs=&HWTiles_render8x8_tile_mask_clip_loresargs;
	}	
	else
#endif
	{
		HWTiles_render8x8_tile_maskargs  = &HWTiles_render8x8_tile_mask_loresargs;
	        HWTiles_render8x8_tile_mask_clipargs=&HWTiles_render8x8_tile_mask_clip_loresargs;
	}
    }
}

void HWTiles_patch_tiles(RomLoader* patch)
{
    uint32_t i;
    memcpy(HWTiles_tiles_backup, HWTiles_tiles, TILES_LENGTH * sizeof(uint32_t));

    for (i = 0; i < patch->length;)
    {
        uint32_t tile_index =         RomLoader_read16IncP(patch, &i) << 3;
        HWTiles_tiles[tile_index++] = RomLoader_read32IncP(patch, &i);
        HWTiles_tiles[tile_index++] = RomLoader_read32IncP(patch, &i);
        HWTiles_tiles[tile_index++] = RomLoader_read32IncP(patch, &i);
        HWTiles_tiles[tile_index++] = RomLoader_read32IncP(patch, &i);
        HWTiles_tiles[tile_index++] = RomLoader_read32IncP(patch, &i);
        HWTiles_tiles[tile_index++] = RomLoader_read32IncP(patch, &i);
        HWTiles_tiles[tile_index++] = RomLoader_read32IncP(patch, &i);
        HWTiles_tiles[tile_index++] = RomLoader_read32IncP(patch, &i);
    }
}

void HWTiles_restore_tiles()
{
    memcpy(HWTiles_tiles, HWTiles_tiles_backup, TILES_LENGTH * sizeof(uint32_t));
}

// Set Tilemap X Clamp
//
// This is used for the widescreen mode, in order to clamp the tilemap to
// a location of the screen. 
//
// In-Game we must clamp right to avoid page scrolling issues.
//
// The clamp will always be 192 for the non-widescreen mode.
void HWTiles_set_x_clamp(const uint16_t props)
{
    if (props == HWTILES_LEFT)
    {
        HWTiles_x_clamp = 192;
    }
    else if (props == HWTILES_RIGHT)
    {
        HWTiles_x_clamp = (512 - HWTiles_s16_width_noscale); 
    }
    else if (props == HWTILES_CENTRE)
    {
        HWTiles_x_clamp = 192 - Config_s16_x_off;
    }
}

void HWTiles_update_tile_values()
{
    int i;
    for (i = 0; i < 4; i++)
    {
        HWTiles_page[i] = ((HWTiles_text_ram[0xe80 + (i * 2) + 0] << 8) | HWTiles_text_ram[0xe80 + (i * 2) + 1]);

        HWTiles_scroll_x[i] = ((HWTiles_text_ram[0xe98 + (i * 2) + 0] << 8) | HWTiles_text_ram[0xe98 + (i * 2) + 1]);
        HWTiles_scroll_y[i] = ((HWTiles_text_ram[0xe90 + (i * 2) + 0] << 8) | HWTiles_text_ram[0xe90 + (i * 2) + 1]);
    }
}

#if 0
// A quick and dirty debug function to display the contents of tile memory.
void HWTiles_render_all_tiles(uint16_t* buf)
{
    uint32_t Code = 0, Colour = 5, x, y;
    for (y = 0; y < 224; y += 8) 
    {
        for (x = 0; x < 320; x += 8) 
        {
            HWTiles_render8x8_tile_mask(buf, Code, x, y, Colour, 3, 0, TILEMAP_COLOUR_OFFSET);
            Code++;
        }
    }
}
#endif

void HWTiles_render_tile_layer(uint16_t* buf, uint8_t page_index, uint8_t priority_draw)
{
    int my, mx,mxmin,mxmax;
    int16_t Colour, x, y, Priority = 0,yflag;

    uint32_t ActPageA,ActPage1,ActPageB; //ActPage
    uint16_t EffPage = HWTiles_page[page_index];
    uint16_t xScroll = HWTiles_scroll_x[page_index];
    uint16_t yScroll = HWTiles_scroll_y[page_index];
    uint32_t TileIndex;
    uint16_t Data;
    int16_t  xcache[128];
#define X_INVALID -32768
#define X_CLIPPED 4096
    hwtilerenderarg args;

    // Need to support this at each row/column
    if ((xScroll & 0x8000) != 0)
        xScroll = (HWTiles_text_ram[0xf80 + (0x40 * page_index) + 0] << 8) | HWTiles_text_ram[0xf80 + (0x40 * page_index) + 1];
    if ((yScroll & 0x8000) != 0)
        yScroll = (HWTiles_text_ram[0xf16 + (0x40 * page_index) + 0] << 8) | HWTiles_text_ram[0xf16 + (0x40 * page_index) + 1];

    args.buf = buf;
    args.nColourDepth = 3;
    args.nMaskColour = 0;
    args.stride = Config_s16_width; 

    /* perform x decisions out of main loop */
    for(mx = 0,mxmin=-1,mxmax=-1; mx < 128; mx++) 
    {
                x = 8 * mx;
                // We take into account the internal screen resolution here
                // to account for widescreen mode.
                x -= (HWTiles_x_clamp - xScroll) & 0x3ff;
                if (x < -HWTiles_x_clamp)
                    x += 1024;
		if( (x <= -8) || (x>=HWTiles_s16_width_noscale) )
		   xcache[mx] = X_INVALID;
		else
		{
		 if( mxmin < 0 )
		  mxmin = mx;
		 mxmax  = mx;
		 if( (x >=0) && (x < (HWTiles_s16_width_noscale - 8)) )
		  xcache[mx] = x;        /* noclip */
		 else
		  xcache[mx] = x - X_CLIPPED; /* clip case */
		}
    }

    for (my = 0; my < 64; my++) 
    {
	y = my<<3;
	y -= yScroll & 0x1ff;
	if (y < -288)
		y += 512;

	if( (y < -7) || (y>=S16_HEIGHT) )
		continue;
	yflag = 0;
	if( (y >=0) && (y <= (S16_HEIGHT - 8)) )
		yflag = 1;

    	ActPage1 = EffPage;
	if(my >= 32)
		ActPage1 = EffPage>>8;

	ActPageA = ((ActPage1      & 0xF )<<12)+((my<<7)&0xfff);
	ActPageB = (((ActPage1>>4) & 0xF )<<12)+((my<<7)&0xfff);

	args.y = y;

        for (mx = mxmin; mx <= mxmax; mx++) 
        {
	    if( xcache[mx] == X_INVALID )
	    	continue;

	    if( mx>=64 )
	    	ActPageA = ActPageB;

            TileIndex = ActPageA + (0x7f&(mx<<1));  //TileIndex = 64 * 32 * 2 * ActPage + ((2 * 64 * my) & 0xfff) + ((2 * mx) & 0x7f);
            Data = (HWTiles_tile_ram[TileIndex + 0] << 8) | HWTiles_tile_ram[TileIndex + 1];

            Priority = (Data >> 15) & 1;

            if (Priority == priority_draw) 
            {
	    	uint16_t ColourOff;
                uint32_t Code = Data & 0x1fff;
                Code = HWTiles_tile_banks[Code / 0x1000] * 0x1000 + Code % 0x1000;
                Code &= (NUM_TILES - 1);

                if(Code == 0)continue;

                Colour = (Data >> 6) & 0x7f;
		ColourOff = ((Colour&0x60)<<3)|TILEMAP_COLOUR_OFFSET;

		x = xcache[mx];

		args.tileptr = HWTiles_tiles + (Code<< 3);
		args.x = x;
		args.nTilePalette = Colour;
		args.nPaletteOffset = ColourOff;

                if( (x >=0) && (yflag) ) // implicitly: if (x >=0 && x < (HWTiles_s16_width_noscale - 8) && y >=0 && y <= (S16_HEIGHT - 8))
			HWTiles_render8x8_tile_maskargs( &args );
                else 	// implicitly: else if (x > -8 && x < HWTiles_s16_width_noscale && y > -8 && y < S16_HEIGHT)
                {
                	if( x < -32 )
                		x+=X_CLIPPED;
                	args.x = x;
			HWTiles_render8x8_tile_mask_clipargs( &args );
		}
            } // end priority check
        }
    } // end for loop
}

void HWTiles_render_text_layer(uint16_t* buf, uint8_t priority_draw)
{
    uint16_t mx, my, Code, Colour, Priority, TileIndex = 0;
    int16_t x,y;
    hwtilerenderarg args;

    args.buf = buf;
    args.nColourDepth = 3;
    args.nMaskColour = 0;
    args.stride = Config_s16_width; 
    args.nPaletteOffset = TILEMAP_COLOUR_OFFSET;

    for (my = 0; my < 32; my++) 
    {
	y = 8 * my;
	args.y = y;

	TileIndex += 48;
        for (mx = 24; mx < 64; mx++) 
        {
	    if( HWTiles_text_ram[TileIndex + 0] >= 0x80 ) /* Top bit = Priority */
	    {
		Code = (HWTiles_text_ram[TileIndex + 0] << 8) | HWTiles_text_ram[TileIndex + 1];
                Colour = (Code >> 9) & 0x07;
                Code &= 0x1ff;
                Code += HWTiles_tile_banks[0] * 0x1000;
                Code &= (NUM_TILES - 1);

                if (Code != 0) 
                {
                    x = 8 * mx;
                    x -= 192;

		    args.tileptr = HWTiles_tiles + (Code<< 3);
		    args.x = x;
		    args.nTilePalette = Colour;

                    HWTiles_render8x8_tile_maskargs(&args);//HWTiles_render8x8_tile_mask(buf, Code, x + Config_s16_x_off, y, Colour, 3, 0, TILEMAP_COLOUR_OFFSET);
                }
            }
            TileIndex += 2;
        }
    }
}


// ------------------------------------------------------------------------------------------------
// Additional routines for Hi-Res Mode.
// Note that the tilemaps are displayed at the same resolution, we just want everything to be
// proportional.
// ------------------------------------------------------------------------------------------------
void HWTiles_render8x8_tile_mask_hires(
    uint16_t *buf,
    uint32_t *tileptr,
    uint16_t StartX, 
    uint16_t StartY, 
    uint16_t nTilePalette, 
    uint16_t nColourDepth, 
    uint16_t nMaskColour, 
    uint16_t nPaletteOffset) 
{
    int y;
    uint32_t nPalette = (nTilePalette << nColourDepth) | nMaskColour;
    uint32_t* pTileData = tileptr;//HWTiles_tiles + (nTileNumber << 3);
    buf += ((StartY << 1) * Config_s16_width) + (StartX << 1);
    
    for (y = 0; y < 8; y++) 
    {
        uint32_t p0 = *pTileData;

        if (p0 != nMaskColour) 
        {
            uint32_t c7 = p0 & 0xf;
            uint32_t c6 = (p0 >> 4) & 0xf;
            uint32_t c5 = (p0 >> 8) & 0xf;
            uint32_t c4 = (p0 >> 12) & 0xf;
            uint32_t c3 = (p0 >> 16) & 0xf;
            uint32_t c2 = (p0 >> 20) & 0xf;
            uint32_t c1 = (p0 >> 24) & 0xf;
            uint32_t c0 = (p0 >> 28);

            if (c0) HWTiles_set_pixel_x4(&buf[0],  nPalette + c0);
            if (c1) HWTiles_set_pixel_x4(&buf[2],  nPalette + c1);
            if (c2) HWTiles_set_pixel_x4(&buf[4],  nPalette + c2);
            if (c3) HWTiles_set_pixel_x4(&buf[6],  nPalette + c3);
            if (c4) HWTiles_set_pixel_x4(&buf[8],  nPalette + c4);
            if (c5) HWTiles_set_pixel_x4(&buf[10], nPalette + c5);
            if (c6) HWTiles_set_pixel_x4(&buf[12], nPalette + c6);
            if (c7) HWTiles_set_pixel_x4(&buf[14], nPalette + c7);
        }
        buf += (Config_s16_width << 1);
        pTileData++;
    }
}

void HWTiles_render8x8_tile_mask_clip_hires(
    uint16_t *buf,
    uint32_t *tileptr,
    int16_t StartX, 
    int16_t StartY, 
    uint16_t nTilePalette, 
    uint16_t nColourDepth, 
    uint16_t nMaskColour, 
    uint16_t nPaletteOffset) 
{
    int y;
    uint32_t nPalette = (nTilePalette << nColourDepth) | nMaskColour;
    uint32_t* pTileData = tileptr;//HWTiles_tiles + (nTileNumber << 3);
    buf += ((StartY << 1) * Config_s16_width) + (StartX << 1);

    for (y = 0; y < 8; y++) 
    {
        if ((StartY + y) >= 0 && (StartY + y) < S16_HEIGHT) 
        {
            uint32_t p0 = *pTileData;

            if (p0 != nMaskColour) 
            {
                uint32_t c7 = p0 & 0xf;
                uint32_t c6 = (p0 >> 4) & 0xf;
                uint32_t c5 = (p0 >> 8) & 0xf;
                uint32_t c4 = (p0 >> 12) & 0xf;
                uint32_t c3 = (p0 >> 16) & 0xf;
                uint32_t c2 = (p0 >> 20) & 0xf;
                uint32_t c1 = (p0 >> 24) & 0xf;
                uint32_t c0 = (p0 >> 28);

                if (c0 && 0 + StartX >= 0 && 0 + StartX < HWTiles_s16_width_noscale) HWTiles_set_pixel_x4(&buf[0],  nPalette + c0);
                if (c1 && 1 + StartX >= 0 && 1 + StartX < HWTiles_s16_width_noscale) HWTiles_set_pixel_x4(&buf[2],  nPalette + c1);
                if (c2 && 2 + StartX >= 0 && 2 + StartX < HWTiles_s16_width_noscale) HWTiles_set_pixel_x4(&buf[4],  nPalette + c2);
                if (c3 && 3 + StartX >= 0 && 3 + StartX < HWTiles_s16_width_noscale) HWTiles_set_pixel_x4(&buf[6],  nPalette + c3);
                if (c4 && 4 + StartX >= 0 && 4 + StartX < HWTiles_s16_width_noscale) HWTiles_set_pixel_x4(&buf[8],  nPalette + c4);
                if (c5 && 5 + StartX >= 0 && 5 + StartX < HWTiles_s16_width_noscale) HWTiles_set_pixel_x4(&buf[10], nPalette + c5);
                if (c6 && 6 + StartX >= 0 && 6 + StartX < HWTiles_s16_width_noscale) HWTiles_set_pixel_x4(&buf[12], nPalette + c6);
                if (c7 && 7 + StartX >= 0 && 7 + StartX < HWTiles_s16_width_noscale) HWTiles_set_pixel_x4(&buf[14], nPalette + c7);
            }
        }
        buf += (Config_s16_width << 1);
        pTileData++;
    }
}


ASM SAVEDS void HWTiles_render8x8_tile_mask_loresargs(ASMR(a2) hwtilerenderarg*args ASMREG(a2) )
{
    int y;
    uint16_t nMaskColour = args->nMaskColour;
    uint16_t stride = args->stride;
    uint32_t nPalette = (args->nTilePalette << args->nColourDepth) | nMaskColour;
//    uint32_t* pTileData = HWTiles_tiles + (args->nTileNumber << 3);
    uint16_t *buf = args->buf + (args->y * stride) + args->x;
    uint32_t* pTileData =  args->tileptr;

    for (y = 0; y < 8; y++) 
    {
        uint32_t p0 = *pTileData;

        if (p0 != nMaskColour) 
        {
            uint32_t c7 = p0 & 0xf;
            uint32_t c6 = (p0 >> 4) & 0xf;
            uint32_t c5 = (p0 >> 8) & 0xf;
            uint32_t c4 = (p0 >> 12) & 0xf;
            uint32_t c3 = (p0 >> 16) & 0xf;
            uint32_t c2 = (p0 >> 20) & 0xf;
            uint32_t c1 = (p0 >> 24) & 0xf;
            uint32_t c0 = (p0 >> 28);

            if (c0) buf[0] = nPalette + c0;
            if (c1) buf[1] = nPalette + c1;
            if (c2) buf[2] = nPalette + c2;
            if (c3) buf[3] = nPalette + c3;
            if (c4) buf[4] = nPalette + c4;
            if (c5) buf[5] = nPalette + c5;
            if (c6) buf[6] = nPalette + c6;
            if (c7) buf[7] = nPalette + c7;
        }
        buf += stride;
        pTileData++;
    }
}


ASM SAVEDS void HWTiles_render8x8_tile_mask_clip_loresargs(ASMR(a2) hwtilerenderarg*args ASMREG(a2) )
{
#if 1
    int y;
    uint16_t nMaskColour = args->nMaskColour;
    uint16_t stride = args->stride;
    uint32_t nPalette = (args->nTilePalette << args->nColourDepth) | nMaskColour;
//    uint32_t* pTileData = HWTiles_tiles + (args->nTileNumber << 3);
    uint16_t *buf = args->buf + (args->y * stride) + args->x;
    int StartY = args->y;
    int StartX = args->x;
    uint32_t* pTileData =  args->tileptr;


    for (y = 0; y < 8; y++) 
    {
        if ((StartY + y) >= 0 && (StartY + y) < S16_HEIGHT) 
        {
            uint32_t p0 = *pTileData;

            if (p0 != nMaskColour) 
            {
                uint32_t c7 = p0 & 0xf;
                uint32_t c6 = (p0 >> 4) & 0xf;
                uint32_t c5 = (p0 >> 8) & 0xf;
                uint32_t c4 = (p0 >> 12) & 0xf;
                uint32_t c3 = (p0 >> 16) & 0xf;
                uint32_t c2 = (p0 >> 20) & 0xf;
                uint32_t c1 = (p0 >> 24) & 0xf;
                uint32_t c0 = (p0 >> 28);

                if (c0 && 0 + StartX >= 0 && 0 + StartX < stride) buf[0] = nPalette + c0;
                if (c1 && 1 + StartX >= 0 && 1 + StartX < stride) buf[1] = nPalette + c1;
                if (c2 && 2 + StartX >= 0 && 2 + StartX < stride) buf[2] = nPalette + c2;
                if (c3 && 3 + StartX >= 0 && 3 + StartX < stride) buf[3] = nPalette + c3;
                if (c4 && 4 + StartX >= 0 && 4 + StartX < stride) buf[4] = nPalette + c4;
                if (c5 && 5 + StartX >= 0 && 5 + StartX < stride) buf[5] = nPalette + c5;
                if (c6 && 6 + StartX >= 0 && 6 + StartX < stride) buf[6] = nPalette + c6;
                if (c7 && 7 + StartX >= 0 && 7 + StartX < stride) buf[7] = nPalette + c7;
            }
        }
        buf += stride;
        pTileData++;
    }
#else
 HWTiles_render8x8_tile_mask_clip_lores( args->buf, 
                                    args->nTileNumber, 
				    args->x, 
				    args->y, 
				    args->nTilePalette, 
				    args->nColourDepth, 
				    args->nMaskColour, 
				    args->nPaletteOffset);
#endif
}

ASM SAVEDS void HWTiles_render8x8_tile_mask_hiresargs(ASMR(a2) hwtilerenderarg*args ASMREG(a2) )
{
 HWTiles_render8x8_tile_mask_hires( args->buf, 
                                    args->tileptr, 
				    args->x, 
				    args->y, 
				    args->nTilePalette, 
				    args->nColourDepth, 
				    args->nMaskColour, 
				    args->nPaletteOffset);
}

ASM SAVEDS void HWTiles_render8x8_tile_mask_clip_hiresargs(ASMR(a2) hwtilerenderarg*args ASMREG(a2) )
{
 HWTiles_render8x8_tile_mask_clip_hires( args->buf, 
                                    args->tileptr, 
				    args->x, 
				    args->y, 
				    args->nTilePalette, 
				    args->nColourDepth, 
				    args->nMaskColour, 
				    args->nPaletteOffset);
}



// Hires Mode: Set 4 pixels instead of one.
void HWTiles_set_pixel_x4(uint16_t *buf, uint32_t data)
{
    buf[0] = buf[1] = buf[0  + Config_s16_width] = buf[1 + Config_s16_width] = data;
}
