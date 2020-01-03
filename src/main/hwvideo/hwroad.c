#include "hwvideo/hwroad.h"
#include "globals.h"
#include "frontend/config.h"

#include <string.h>

#ifdef	_AMIGA_ASM_
#include "amiga/m68kRoadRender.h"
#endif

/***************************************************************************
    Video Emulation: OutRun Road Rendering Hardware.
    Based on MAME source code.

    Copyright Aaron Giles.
    All rights reserved.
***************************************************************************/

/*******************************************************************************************
 *
 *  Out Run/X-Board-style road chip
 *
 *  Road control register:
 *      Bits               Usage
 *      -------- -----d--  (X-board only) Direct scanline mode (1) or indirect mode (0)
 *      -------- ------pp  Road enable/priorities:
 *                            0 = road 0 only visible
 *                            1 = both roads visible, road 0 has priority
 *                            2 = both roads visible, road 1 has priority
 *                            3 = road 1 only visible
 *
 *  Road RAM:
 *      Offset   Bits               Usage
 *      000-1FF  ----s--- --------  Road 0: Solid fill (1) or ROM fill
 *               -------- -ccccccc  Road 0: Solid color (if solid fill)
 *               -------i iiiiiiii  Road 0: Index for other tables (if in indirect mode)
 *               -------r rrrrrrr-  Road 0: Road ROM line select
 *      200-3FF  ----s--- --------  Road 1: Solid fill (1) or ROM fill
 *               -------- -ccccccc  Road 1: Solid color (if solid fill)
 *               -------i iiiiiiii  Road 1: Index for other tables (if in indirect mode)
 *               -------r rrrrrrr-  Road 1: Road ROM line select
 *      400-7FF  ----hhhh hhhhhhhh  Road 0: horizontal scroll
 *      800-BFF  ----hhhh hhhhhhhh  Road 1: horizontal scroll
 *      C00-FFF  ----bbbb --------  Background color index
 *               -------- s-------  Road 1: stripe color index
 *               -------- -a------  Road 1: pixel value 2 color index
 *               -------- --b-----  Road 1: pixel value 1 color index
 *               -------- ---c----  Road 1: pixel value 0 color index
 *               -------- ----s---  Road 0: stripe color index
 *               -------- -----a--  Road 0: pixel value 2 color index
 *               -------- ------b-  Road 0: pixel value 1 color index
 *               -------- -------c  Road 0: pixel value 0 color index
 *
 *  Logic:
 *      First, the scanline is used to index into the tables at 000-1FF/200-3FF
 *          - if solid fill, the background is filled with the specified color index
 *          - otherwise, the remaining tables are used
 *
 *      If indirect mode is selected, the index is taken from the low 9 bits of the
 *          table value from 000-1FF/200-3FF
 *      If direct scanline mode is selected, the index is set equal to the scanline
 *          for road 0, or the scanline + 256 for road 1
 *
 *      The horizontal scroll value is looked up using the index in the tables at
 *          400-7FF/800-BFF
 *
 *      The color information is looked up using the index in the table at C00-FFF. Note
 *          that the same table is used for both roads.
 *
 *
 *  Out Run road priorities are controlled by a PAL that maps as indicated below.
 *  This was used to generate the priority_map. It is assumed that X-board is the
 *  same, though this logic is locked inside a Sega custom.
 *
 *  RRC0 =  CENTA & (RDA == 3) & !RRC2
 *      | CENTB & (RDB == 3) & RRC2
 *      | (RDA == 1) & !RRC2
 *      | (RDB == 1) & RRC2
 *
 *  RRC1 =  CENTA & (RDA == 3) & !RRC2
 *      | CENTB & (RDB == 3) & RRC2
 *      | (RDA == 2) & !RRC2
 *      | (RDB == 2) & RRC2
 *
 *  RRC2 = !/HSYNC & IIQ
 *      | (CTRL == 3)
 *      | !CENTA & (RDA == 3) & !CENTB & (RDB == 3) & (CTRL == 2)
 *      | CENTB & (RDB == 3) & (CTRL == 2)
 *      | !CENTA & (RDA == 3) & !M2 & (CTRL == 2)
 *      | !CENTA & (RDA == 3) & !M3 & (CTRL == 2)
 *      | !M0 & (RDB == 0) & (CTRL == 2)
 *      | !M1 & (RDB == 0) & (CTRL == 2)
 *      | !CENTA & (RDA == 3) & CENTB & (RDB == 3) & (CTRL == 1)
 *      | !M0 & CENTB & (RDB == 3) & (CTRL == 1)
 *      | !M1 & CENTB & (RDB == 3) & (CTRL == 1)
 *      | !CENTA & M0 & (RDB == 0) & (CTRL == 1)
 *      | !CENTA & M1 & (RDB == 0) & (CTRL == 1)
 *      | !CENTA & (RDA == 3) & (RDB == 1) & (CTRL == 1)
 *      | !CENTA & (RDA == 3) & (RDB == 2) & (CTRL == 1)
 *
 *  RRC3 =  VA11 & VB11
 *      | VA11 & (CTRL == 0)
 *      | (CTRL == 3) & VB11
 *
 *  RRC4 =  !CENTA & (RDA == 3) & !CENTB & (RDB == 3)
 *      | VA11 & VB11
 *      | VA11 & (CTRL == 0)
 *      | (CTRL == 3) & VB11
 *      | !CENTB & (RDB == 3) & (CTRL == 3)
 *      | !CENTA & (RDA == 3) & (CTRL == 0)
 *
 *******************************************************************************************/

uint8_t HWRoad_road_control;
//uint16_t HWRoad_color_offset1;
//uint16_t HWRoad_color_offset2;
//uint16_t HWRoad_color_offset3;
//int32_t HWRoad_x_offset;
#define HWRoad_x_offset 0
#define HWRoad_color_offset1 0x400
#define HWRoad_color_offset2 0x420
#define HWRoad_color_offset3 0x780
 
// Decoded road graphics
uint8_t HWRoad_roads[0x40200];

// Two halves of RAM
uint16_t HWRoad_ram[ROAD_RAM_SIZE / 2];
uint16_t HWRoad_ramBuff[ROAD_RAM_SIZE / 2];

void HWRoad_decode_road(const uint8_t*);
void HWRoad_render_background_lores(uint16_t*);
void HWRoad_render_foreground_lores(uint16_t*);
void HWRoad_render_background_hires(uint16_t*);
void HWRoad_render_foreground_hires(uint16_t*);

void (*HWRoad_render_background)(uint16_t*);
void (*HWRoad_render_foreground)(uint16_t*);

// Convert road to a more useable format
void HWRoad_init(const uint8_t* src_road, const Boolean hires)
{
    HWRoad_road_control = 0;
#if 0
    HWRoad_color_offset1 = 0x400;
    HWRoad_color_offset2 = 0x420;
    HWRoad_color_offset3 = 0x780;
    HWRoad_x_offset = 0;
#endif

    if (src_road)
        HWRoad_decode_road(src_road);
    
    if (hires)
    {
        HWRoad_render_background = &HWRoad_render_background_hires;
        HWRoad_render_foreground = &HWRoad_render_foreground_hires;
    }
    else
    {
        HWRoad_render_background = &HWRoad_render_background_lores;
        HWRoad_render_foreground = &HWRoad_render_foreground_lores;   
    }
}

/*
    There are TWO (identical) roads we need to decode.
    Each of these roads is represented using a 512x256 map.
    See: http://www.extentofthejam.com/pseudo/
      
    512 x 256 x 2bpp map 
    0x8000 bytes of data. 
    2 Bits Per Pixel.
       
    Per Road:
    Bit 0 of each pixel is stored at offset 0x0000 - 0x3FFF
    Bit 1 of each pixel is stored at offset 0x4000 - 0x7FFF

    This means: 80 bytes per X Row [2 x 0x40 Bytes from the two separate locations]

    Decoded Format:  
    0 = Road Colour
    1 = Road Inner Stripe
    2 = Road Outer Stripe
    3 = Road Exterior
    7 = Central Stripe
*/

void HWRoad_decode_road(const uint8_t* src_road)
{
    int y,x,i;
    for (y = 0; y < 256 * 2; y++) 
    {
        const int src = ((y & 0xff) * 0x40 + (y >> 8) * 0x8000) % HWRoad_rom_size; // tempGfx
        const int dst = y * 512; // System16Roads

        // loop over columns
        for (x = 0; x < 512; x++) 
        {
            HWRoad_roads[dst + x] = (((src_road[src + (x / 8)] >> (~x & 7)) & 1) << 0) | (((src_road[src + (x / 8 + 0x4000)] >> (~x & 7)) & 1) << 1);

            // pre-mark road data in the "stripe" area with a high bit
            if (x >= 256 - 8 && x < 256 && HWRoad_roads[dst + x] == 3)
                HWRoad_roads[dst + x] |= 4;
        }
    }

    // set up a dummy road in the last entry
    for (i = 0; i < 512; i++) 
    {
        HWRoad_roads[256 * 2 * 512 + i] = 3;
    }
}

// Writes go to RAM, but we read from the RAM Buffer.
void HWRoad_write16(uint32_t adr, const uint16_t data)
{
    HWRoad_ram[(adr >> 1) & 0x7FF] = data;
}

void HWRoad_write16IncP(uint32_t* adr, const uint16_t data)
{
    uint32_t a = *adr;
    HWRoad_ram[(a >> 1) & 0x7FF] = data;
    *adr += 2;
}

void HWRoad_write32(uint32_t* adr, const uint32_t data)
{
    uint32_t a = *adr;
    HWRoad_ram[(a >> 1) & 0x7FF] = data >> 16;
    HWRoad_ram[((a >> 1) + 1) & 0x7FF] = data & 0xFFFF;
    *adr += 4;
}

uint16_t HWRoad_read_road_control()
{
    uint16_t i;
    uint32_t *src = (uint32_t *)HWRoad_ram;
    uint32_t *dst = (uint32_t *)HWRoad_ramBuff;

    // swap the halves of the road RAM
    for (i = 0; i < ROAD_RAM_SIZE/4; i++)
    {
        uint32_t temp = *src;
        *src++ = *dst;
        *dst++ = temp;
    }

    return 0xffff;
}

void HWRoad_write_road_control(const uint8_t road_control)
{
    HWRoad_road_control = road_control;
}

// ------------------------------------------------------------------------------------------------
// Road Rendering: Lores Version
// ------------------------------------------------------------------------------------------------

// Background: Look for solid fill scanlines
void HWRoad_render_background_lores(uint16_t* pixels)
{
#ifdef	_AMIGA_ASM_
  FrameRoadBG bg;
  
  bg.ram    = HWRoad_ramBuff;
  bg.ctl    = HWRoad_road_control;
  bg.coloff = HWRoad_color_offset3;
  bg.ys     = S16_HEIGHT;
  bg.xs     = Config_s16_width;
  bg.pix    = pixels;

  m68k_RoadRenderBG( &bg );

#else
    int x, y;
    uint16_t* roadram = HWRoad_ramBuff;
    
    for (y = 0; y < S16_HEIGHT; y++) 
    {
        int data0 = roadram[0x000 + y];
        int data1 = roadram[0x100 + y];

        int color = -1;

        // based on the info->control, we can figure out which sky to draw
        switch (HWRoad_road_control & 3) 
        {
            case 0:
                if (data0 & 0x800)
                    color = data0 & 0x7f;
                break;

            case 1:
                if (data0 & 0x800)
                    color = data0 & 0x7f;
                else if (data1 & 0x800)
                    color = data1 & 0x7f;
                break;

            case 2:
                if (data1 & 0x800)
                    color = data1 & 0x7f;
                else if (data0 & 0x800)
                    color = data0 & 0x7f;
                break;

            case 3:
                if (data1 & 0x800)
                    color = data1 & 0x7f;
                break;
        }

        // fill the scanline with color
        if (color != -1) 
        {
            uint16_t* pPixel = pixels + (y * Config_s16_width);
            color |= HWRoad_color_offset3;
            
            for (x = 0; x < Config_s16_width; x++)
                *(pPixel)++ = color;
        }
    }
#endif
}

// Foreground: Render From ROM
void HWRoad_render_foreground_lores(uint16_t* pixels)
{
    int x, y;
    uint16_t* roadram = HWRoad_ramBuff;
    uint16_t color_table[32];
    static const uint8_t priority_map[2][8] =
    {
            { 0x80,0x81,0x81,0x87,0,0,0,0x00 },
            { 0x81,0x81,0x81,0x8f,0,0,0,0x80 }
    };
#ifdef	_AMIGA_ASM_
    RoadRender RoadRnd;
 
    RoadRnd.width = Config_s16_width;
    RoadRnd.colortable = color_table;
#endif

    for (y = 0; y < S16_HEIGHT; y++) 
    {
        const uint32_t data0 = roadram[0x000 + y];
        const uint32_t data1 = roadram[0x100 + y];
        int32_t hpos0, hpos1, color0, color1;
        uint8_t *src0, *src1;
        int32_t bgcolor; // 8 bits
	uint16_t* pPixel;
	int32_t control;
	uint16_t s16_x;

        // if both roads are low priority, skip
        if (((data0 & 0x800) != 0) && ((data1 & 0x800) != 0))
            continue;

        pPixel = pixels + (y * Config_s16_width);
        control = HWRoad_road_control & 3;


        // get road 0 data
        src0   = ((data0 & 0x800) != 0) ? HWRoad_roads + 256 * 2 * 512 : (HWRoad_roads + (0x000 + ((data0 >> 1) & 0xff)) * 512);
        hpos0  = roadram[0x200 + (((HWRoad_road_control & 4) != 0) ? y : (data0 & 0x1ff))] & 0xfff;
        color0 = roadram[0x600 + (((HWRoad_road_control & 4) != 0) ? y : (data0 & 0x1ff))];

        // get road 1 data
        src1   = ((data1 & 0x800) != 0) ? HWRoad_roads + 256 * 2 * 512 : (HWRoad_roads + (0x100 + ((data1 >> 1) & 0xff)) * 512);
        hpos1  = roadram[0x400 + (((HWRoad_road_control & 4) != 0) ? (0x100 + y) : (data1 & 0x1ff))] & 0xfff;
        color1 = roadram[0x600 + (((HWRoad_road_control & 4) != 0) ? (0x100 + y) : (data1 & 0x1ff))];

        // determine the 5 colors for road 0
        color_table[0x00] = HWRoad_color_offset1 ^ 0x00 ^ ((color0 >> 0) & 1);
        color_table[0x01] = HWRoad_color_offset1 ^ 0x02 ^ ((color0 >> 1) & 1);
        color_table[0x02] = HWRoad_color_offset1 ^ 0x04 ^ ((color0 >> 2) & 1);
        bgcolor = (color0 >> 8) & 0xf;
        color_table[0x03] = ((data0 & 0x200) != 0) ? color_table[0x00] : (HWRoad_color_offset2 ^ 0x00 ^ bgcolor);
        color_table[0x07] = HWRoad_color_offset1 ^ 0x06 ^ ((color0 >> 3) & 1);

        // determine the 5 colors for road 1
        color_table[0x10] = HWRoad_color_offset1 ^ 0x08 ^ ((color1 >> 4) & 1);
        color_table[0x11] = HWRoad_color_offset1 ^ 0x0a ^ ((color1 >> 5) & 1);
        color_table[0x12] = HWRoad_color_offset1 ^ 0x0c ^ ((color1 >> 6) & 1);
        bgcolor = (color1 >> 8) & 0xf;
        color_table[0x13] = ((data1 & 0x200) != 0) ? color_table[0x10] : (HWRoad_color_offset2 ^ 0x10 ^ bgcolor);
        color_table[0x17] = HWRoad_color_offset1 ^ 0x0e ^ ((color1 >> 7) & 1);

        // Shift road dependent on whether we are in widescreen mode or not
        s16_x = 0x5f8 + Config_s16_x_off;

        // draw the road
        switch (control) 
        {
            case 0:
                if (data0 & 0x800)
                    continue;
                hpos0 = (hpos0 - (s16_x + HWRoad_x_offset)) & 0xfff;
#ifdef	_AMIGA_ASM_
		RoadRnd.hpos0 = hpos0;
		/*RoadRnd.hpos1 = hpos1;*/
		RoadRnd.src0  = src0;
		/*RoadRnd.src1  = src1;*/
		RoadRnd.dest  = pPixel;
		m68k_RoadRenderLine0( &RoadRnd );
#else	    
                for (x = 0; x < Config_s16_width; x++) 
                {
                    int pix0 = (hpos0 < 0x200) ? src0[hpos0] : 3;
                    pPixel[x] = color_table[0x00 + pix0];
                    hpos0 = (hpos0 + 1) & 0xfff;
                }
#endif
                break;
            case 1:
                hpos0 = (hpos0 - (s16_x + HWRoad_x_offset)) & 0xfff;
                hpos1 = (hpos1 - (s16_x + HWRoad_x_offset)) & 0xfff;
#ifdef	_AMIGA_ASM_
		RoadRnd.hpos0 = hpos0;
		RoadRnd.hpos1 = hpos1;
		RoadRnd.src0  = src0;
		RoadRnd.src1  = src1;
		RoadRnd.dest  = pPixel;
		m68k_RoadRenderLine1( &RoadRnd );
#else
                for (x = 0; x < Config_s16_width; x++) 
                {
                    int pix0 = (hpos0 < 0x200) ? src0[hpos0] : 3;
                    int pix1 = (hpos1 < 0x200) ? src1[hpos1] : 3;
                    if (((priority_map[0][pix0] >> pix1) & 1) != 0)
                        pPixel[x] = color_table[0x10 + pix1];
                    else
                        pPixel[x] = color_table[0x00 + pix0];

                    hpos0 = (hpos0 + 1) & 0xfff;
                    hpos1 = (hpos1 + 1) & 0xfff;
                }
#endif
                break;

            case 2:
                hpos0 = (hpos0 - (s16_x + HWRoad_x_offset)) & 0xfff;
                hpos1 = (hpos1 - (s16_x + HWRoad_x_offset)) & 0xfff;
#ifdef	_AMIGA_ASM_
		RoadRnd.hpos0 = hpos0;
		RoadRnd.hpos1 = hpos1;
		RoadRnd.src0  = src0;
		RoadRnd.src1  = src1;
		RoadRnd.dest  = pPixel;
		m68k_RoadRenderLine2( &RoadRnd );
#else
                for (x = 0; x < Config_s16_width; x++) 
                {
                    int pix0 = (hpos0 < 0x200) ? src0[hpos0] : 3;
                    int pix1 = (hpos1 < 0x200) ? src1[hpos1] : 3;
                    if (((priority_map[1][pix0] >> pix1) & 1) != 0)
                        pPixel[x] = color_table[0x10 + pix1];
                    else
                        pPixel[x] = color_table[0x00 + pix0];

                    hpos0 = (hpos0 + 1) & 0xfff;
                    hpos1 = (hpos1 + 1) & 0xfff;
                }
#endif
                break;

            case 3:
                if (data1 & 0x800)
                    continue;
                hpos1 = (hpos1 - (s16_x + HWRoad_x_offset)) & 0xfff;
#ifdef	_AMIGA_ASM_
		RoadRnd.hpos1 = hpos1;
		RoadRnd.src1  = src1;
		RoadRnd.dest  = pPixel;
		m68k_RoadRenderLine3( &RoadRnd );
#else	
                for (x = 0; x < Config_s16_width; x++) 
                {
                    int pix1 = (hpos1 < 0x200) ? src1[hpos1] : 3;
                    pPixel[x] = color_table[0x10 + pix1];
                    hpos1 = (hpos1 + 1) & 0xfff;
                }
#endif
                break;
            } // end switch
    } // end for
}

// ------------------------------------------------------------------------------------------------
// High Resolution (Double Resolution) Road Rendering
// ------------------------------------------------------------------------------------------------
void HWRoad_render_background_hires(uint16_t* pixels)
{
    int x, y;
    uint16_t* roadram = HWRoad_ramBuff;

    for (y = 0; y < Config_s16_height; y += 2) 
    {
        int data0 = roadram[0x000 + (y >> 1)];
        int data1 = roadram[0x100 + (y >> 1)];

        int color = -1;

        // based on the info->control, we can figure out which sky to draw
        switch (HWRoad_road_control & 3) 
        {
            case 0:
                if (data0 & 0x800)
                    color = data0 & 0x7f;
                break;

            case 1:
                if (data0 & 0x800)
                    color = data0 & 0x7f;
                else if (data1 & 0x800)
                    color = data1 & 0x7f;
                break;

            case 2:
                if (data1 & 0x800)
                    color = data1 & 0x7f;
                else if (data0 & 0x800)
                    color = data0 & 0x7f;
                break;

            case 3:
                if (data1 & 0x800)
                    color = data1 & 0x7f;
                break;
        }

        // fill the scanline with color
        if (color != -1) 
        {
            uint16_t* pPixel = pixels + (y * Config_s16_width);
            color |= HWRoad_color_offset3;
            
            for (x = 0; x < Config_s16_width; x++)
                *(pPixel)++ = color;
        }

        // Hi-Res Mode: Copy extra line of background
        memcpy(pixels + ((y+1) * Config_s16_width), pixels + (y * Config_s16_width), sizeof(uint16_t) * Config_s16_width);
    }
}

// ------------------------------------------------------------------------------------------------
// Render Road Foreground - High Resolution Version
// Interpolates previous scanline with next.
// ------------------------------------------------------------------------------------------------
void HWRoad_render_foreground_hires(uint16_t* pixels)
{
    int x, y, yy;
    uint16_t* roadram = HWRoad_ramBuff;
    
    uint16_t color_table[32];
    int32_t color0, color1;
    int32_t bgcolor; // 8 bits

    for (y = 0; y < Config_s16_height; y++) 
    {
	uint32_t data0; 
	uint32_t data1;
	uint8_t *src0,*src1;
	int32_t hpos0,hpos1;

        static const uint8_t priority_map[2][8] =
        {
            { 0x80,0x81,0x81,0x87,0,0,0,0x00 },
            { 0x81,0x81,0x81,0x8f,0,0,0,0x80 }
        };

	yy = y >> 1;

        data0 = roadram[0x000 + yy];
        data1 = roadram[0x100 + yy];

        // if both roads are low priority, skip
        if (((data0 & 0x800) != 0) && ((data1 & 0x800) != 0))
        {
            y++; 
            continue;
        }

        src0 = NULL, *src1 = NULL;

        // get road 0 data
        hpos0  = roadram[0x200 + (((HWRoad_road_control & 4) != 0) ? yy : (data0 & 0x1ff))] & 0xfff;

        // get road 1 data       
        hpos1  = roadram[0x400 + (((HWRoad_road_control & 4) != 0) ? (0x100 + yy) : (data1 & 0x1ff))] & 0xfff;
        
        // ----------------------------------------------------------------------------------------
        // Interpolate Scanlines when in hi-resolution mode.
        // ----------------------------------------------------------------------------------------
        if (y & 1 && yy < S16_HEIGHT - 1)
        {
            uint32_t data0_next = roadram[0x000 + yy + 1];
            uint32_t data1_next = roadram[0x100 + yy + 1];

            int32_t  hpos0_next = roadram[0x200 + (((HWRoad_road_control & 4) != 0) ? yy + 1 : (data0_next & 0x1ff))] & 0xfff;
            int32_t  hpos1_next = roadram[0x400 + (((HWRoad_road_control & 4) != 0) ? yy + 1 : (data1_next & 0x1ff))] & 0xfff;
	    int32_t  diff;

            // Interpolate road 1 position
            if (((data0 & 0x800) == 0) && (data0_next & 0x800) == 0)
            {
                data0      = (data0      >> 1) & 0xFF;
                data0_next = (data0_next >> 1) & 0xFF;
                diff = (data0 + ((data0_next - data0) >> 1)) & 0xFF;
                src0 = (HWRoad_roads + (0x000 + diff) * 512);
                hpos0 = (hpos0 + ((hpos0_next - hpos0) >> 1)) & 0xFFF;
            }
            // Interpolate road 2 source position
            if (((data1 & 0x800) == 0) && (data1_next & 0x800) == 0)
            {
                data1      = (data1      >> 1) & 0xFF;
                data1_next = (data1_next >> 1) & 0xFF;
                diff = (data1 + ((data1_next - data1) >> 1)) & 0xFF;
                src1 = (HWRoad_roads + (0x100 + diff) * 512);
                hpos1 = (hpos1 + ((hpos1_next - hpos1) >> 1)) & 0xFFF;
            }     
        }
        // ----------------------------------------------------------------------------------------
        // Recalculate for non-interpolated scanlines
        // ----------------------------------------------------------------------------------------
        else
        {            
            color0 = roadram[0x600 + (((HWRoad_road_control & 4) != 0) ? yy :           (data0 & 0x1ff))];
            color1 = roadram[0x600 + (((HWRoad_road_control & 4) != 0) ? (0x100 + yy) : (data1 & 0x1ff))];
        
            // determine the 5 colors for road 0
            color_table[0x00] = HWRoad_color_offset1 ^ 0x00 ^ ((color0 >> 0) & 1);
            color_table[0x01] = HWRoad_color_offset1 ^ 0x02 ^ ((color0 >> 1) & 1);
            color_table[0x02] = HWRoad_color_offset1 ^ 0x04 ^ ((color0 >> 2) & 1);
            bgcolor = (color0 >> 8) & 0xf;
            color_table[0x03] = ((data0 & 0x200) != 0) ? color_table[0x00] : (HWRoad_color_offset2 ^ 0x00 ^ bgcolor);
            color_table[0x07] = HWRoad_color_offset1 ^ 0x06 ^ ((color0 >> 3) & 1);

            // determine the 5 colors for road 1
            color_table[0x10] = HWRoad_color_offset1 ^ 0x08 ^ ((color1 >> 4) & 1);
            color_table[0x11] = HWRoad_color_offset1 ^ 0x0a ^ ((color1 >> 5) & 1);
            color_table[0x12] = HWRoad_color_offset1 ^ 0x0c ^ ((color1 >> 6) & 1);
            bgcolor = (color1 >> 8) & 0xf;
            color_table[0x13] = ((data1 & 0x200) != 0) ? color_table[0x10] : (HWRoad_color_offset2 ^ 0x10 ^ bgcolor);
            color_table[0x17] = HWRoad_color_offset1 ^ 0x0e ^ ((color1 >> 7) & 1);        
        }
        
        if (src0 == NULL)
            src0 = ((data0 & 0x800) != 0) ? HWRoad_roads + 256 * 2 * 512 : (HWRoad_roads + (0x000 + ((data0 >> 1) & 0xff)) * 512);
        if (src1 == NULL)
            src1 = ((data1 & 0x800) != 0) ? HWRoad_roads + 256 * 2 * 512 : (HWRoad_roads + (0x100 + ((data1 >> 1) & 0xff)) * 512);

	{
        // Shift road dependent on whether we are in widescreen mode or not
        uint16_t s16_x = 0x5f8 + Config_s16_x_off;
        uint16_t* const pPixel = pixels + (y * Config_s16_width);

        // draw the road
        switch (HWRoad_road_control & 3)
        {
            case 0:
                if (data0 & 0x800)
                    continue;
                hpos0 = (hpos0 - (s16_x + HWRoad_x_offset)) & 0xfff;
                for (x = 0; x < Config_s16_width; x++) 
                {
                    int pix0 = (hpos0 < 0x200) ? src0[hpos0] : 3;
                    pPixel[x] = color_table[0x00 + pix0];
                    if (x & 1)
                        hpos0 = (hpos0 + 1) & 0xfff;
                }
                break;

            case 1:
                hpos0 = (hpos0 - (s16_x + HWRoad_x_offset)) & 0xfff;
                hpos1 = (hpos1 - (s16_x + HWRoad_x_offset)) & 0xfff;
                for (x = 0; x < Config_s16_width; x++) 
                {
                    int pix0 = (hpos0 < 0x200) ? src0[hpos0] : 3;
                    int pix1 = (hpos1 < 0x200) ? src1[hpos1] : 3;
                    if (((priority_map[0][pix0] >> pix1) & 1) != 0)
                        pPixel[x] = color_table[0x10 + pix1];
                    else
                        pPixel[x] = color_table[0x00 + pix0];

                    if (x & 1)
                    {
                        hpos0 = (hpos0 + 1) & 0xfff;
                        hpos1 = (hpos1 + 1) & 0xfff;
                    }
                }
                break;

            case 2:
                hpos0 = (hpos0 - (s16_x + HWRoad_x_offset)) & 0xfff;
                hpos1 = (hpos1 - (s16_x + HWRoad_x_offset)) & 0xfff;
                for (x = 0; x < Config_s16_width; x++) 
                {
                    int pix0 = (hpos0 < 0x200) ? src0[hpos0] : 3;
                    int pix1 = (hpos1 < 0x200) ? src1[hpos1] : 3;
                    if (((priority_map[1][pix0] >> pix1) & 1) != 0)
                        pPixel[x] = color_table[0x10 + pix1];
                    else
                        pPixel[x] = color_table[0x00 + pix0];
                      
                    if (x & 1)
                    {
                        hpos0 = (hpos0 + 1) & 0xfff;
                        hpos1 = (hpos1 + 1) & 0xfff;
                    }
                }
                break;

            case 3:
                if (data1 & 0x800)
                    continue;
                hpos1 = (hpos1 - (s16_x + HWRoad_x_offset)) & 0xfff;
                for (x = 0; x < Config_s16_width; x++) 
                {
                    int pix1 = (hpos1 < 0x200) ? src1[hpos1] : 3;
                    pPixel[x] = color_table[0x10 + pix1];                   
                    if (x & 1)
                        hpos1 = (hpos1 + 1) & 0xfff;
                }
                break;
            } // end switch
     }
    } // end for
}

