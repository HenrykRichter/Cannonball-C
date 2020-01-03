#include "Video.h"
#include "hwvideo/hwsprites.h"
#include "globals.h"
#include "frontend/config.h"

/***************************************************************************
    Video Emulation: OutRun Sprite Rendering Hardware.
    Based on MAME source code.

    Copyright Aaron Giles.
    All rights reserved.
***************************************************************************/

/*******************************************************************************************
*  Out Run/X-Board-style sprites
*
*      Offs  Bits               Usage
*       +0   e------- --------  Signify end of sprite list
*       +0   -h-h---- --------  Hide this sprite if either bit is set
*       +0   ----bbb- --------  Sprite bank
*       +0   -------t tttttttt  Top scanline of sprite + 256
*       +2   oooooooo oooooooo  Offset within selected sprite bank
*       +4   ppppppp- --------  Signed 7-bit pitch value between scanlines
*       +4   -------x xxxxxxxx  X position of sprite (position $BE is screen position 0)
*       +6   -s------ --------  Enable shadows
*       +6   --pp---- --------  Sprite priority, relative to tilemaps
*       +6   ------vv vvvvvvvv  Vertical zoom factor (0x200 = full size, 0x100 = half size, 0x300 = 2x size)
*       +8   y------- --------  Render from top-to-bottom (1) or bottom-to-top (0) on screen
*       +8   -f------ --------  Horizontal flip: read the data backwards if set
*       +8   --x----- --------  Render from left-to-right (1) or right-to-left (0) on screen
*       +8   ------hh hhhhhhhh  Horizontal zoom factor (0x200 = full size, 0x100 = half size, 0x300 = 2x size)
*       +E   dddddddd dddddddd  Scratch space for current address
*
*  Out Run only:
*       +A   hhhhhhhh --------  Height in scanlines - 1
*       +A   -------- -ccccccc  Sprite color palette
*
*  X-Board only:
*       +A   ----hhhh hhhhhhhh  Height in scanlines - 1
*       +C   -------- cccccccc  Sprite color palette
*
*  Final bitmap format:
*
*            -s------ --------  Shadow control
*            --pp---- --------  Sprite priority
*            ----cccc cccc----  Sprite color palette
*            -------- ----llll  4-bit pixel data
*
 *******************************************************************************************/

#ifdef _AMIGA_
//extern uint16_t *palette;
//#define GET_PALETTE16(_a_) palette[ (_a_)>>1 ]
extern uint8_t *palette;
#define GET_PALETTE16(_a_) (palette[ (_a_) ]<<8)
#else
#define GET_PALETTE16(_a_) Video_read_pal16(_a_)
#endif

/* Sprite row rendering cache (H. Richter)
   syntax:
    0x80      = reload 4 bytes from sprite ROM to pixel cache
    0x81      = end marker
    0x00-0x1F = shift value
   semantics:
    if( ROW_Cache[pos] & 0x80 )
    {
     if( pix_cache & 0xf  == 0xf ) // last set of pixels had 0xf in second-to-last pixel?
      break;
     if( ROW_Cache[pos] == 0x81 )
      break;
     pix_cache = sprite_rom[++addr];
     pos++;
    }
    else
    {
     shift=ROW_Cache[pos++];
    }
*/
int8_t ROW_Cache[800+128];
int32_t ROW_Cache_x1;
#define ROWC_Reload 0x80
#define ROWC_End 0x81


// Clip values.
uint16_t HWSprites_x1, HWSprites_x2;

// 128 sprites, 16 bytes each (0x400)
#define SPRITE_RAM_SIZE (128 * 8)
#define SPRITES_LENGTH (0x100000 >> 2)
#define COLOR_BASE 0x800

uint32_t sprites[SPRITES_LENGTH]; // Converted sprites
    
// Two halves of RAM
uint16_t ram[SPRITE_RAM_SIZE];
uint16_t ramBuff[SPRITE_RAM_SIZE];

void HWSprites_init(const uint8_t* src_sprites)
{
    uint32_t i;
    HWSprites_reset();

    if (src_sprites)
    {
        // Convert S16 tiles to a more useable format
        const uint8_t *spr = src_sprites;

        for (i = 0; i < SPRITES_LENGTH; i++)
        {
            uint8_t d3 = *spr++;
            uint8_t d2 = *spr++;
            uint8_t d1 = *spr++;
            uint8_t d0 = *spr++;

            sprites[i] = (d0 << 24) | (d1 << 16) | (d2 << 8) | d3;
        }
    }
}

void HWSprites_reset()
{
    uint16_t i;
    
    // Clear Sprite RAM buffers
    for (i = 0; i < SPRITE_RAM_SIZE; i++)
    {
        ram[i] = 0;
        ramBuff[i] = 0;
    }
}

// Clip areas of the screen in wide-screen mode
void HWSprites_set_x_clip(Boolean on)
{
    // Clip to central 320 width window.
    if (on)
    {
        HWSprites_x1 = Config_s16_x_off;
        HWSprites_x2 = HWSprites_x1 + S16_WIDTH;

        if (Config_video.hires)
        {
            HWSprites_x1 <<= 1;
            HWSprites_x2 <<= 1;
        }
    }
    // Allow full wide-screen.
    else
    {
        HWSprites_x1 = 0;
        HWSprites_x2 = Config_s16_width;
    }
}


uint8_t HWSprites_read(const uint16_t adr)
{
    uint16_t a = adr >> 1;
    if ((adr & 1) == 1)
        return ram[a] & 0xff;
    else
        return ram[a] >> 8;
}

void HWSprites_write(const uint16_t adr, const uint16_t data)
{
    ram[adr >> 1] = data;
}

// Copy back buffer to main ram, ready for blit
void HWSprites_swap()
{
    uint16_t i;
    uint16_t *src = (uint16_t *)ram;
    uint16_t *dst = (uint16_t *)ramBuff;

    // swap the halves of the road RAM
    for (i = 0; i < SPRITE_RAM_SIZE; i++)
    {
        uint16_t temp = *src;
        *src++ = *dst;
        *dst++ = temp;
    }
}

#if 1
#define SEP_SHADOW
#define HWSprites_draw_pixel()                                                                                  \
{                                                                                                     \
    if (x >= HWSprites_x1 && x < HWSprites_x2 && pix != 0 && pix != 15)                                                   \
    {                                                                                                 \
            pPixel[x] = (pix | color);                                                                \
    }                                                                                                 \
}
#define HWSprites_draw_pixelS()                                                                                  \
{                                                                                                     \
    if (x >= HWSprites_x1 && x < HWSprites_x2 && pix != 0 && pix != 15)                                                   \
    {                                                                                                 \
        if (pix == 0xa)                                                                     \
        {                                                                                             \
            pPixel[x] &= 0xfff;                                                                       \
            pPixel[x] += ((S16_PALETTE_ENTRIES * 2) - ((GET_PALETTE16(pPixel[x]) & 0x8000) >> 3)); \
        }                                                                                             \
        else                                                                                          \
        {                                                                                             \
            pPixel[x] = (pix | color);                                                                \
        }                                                                                             \
    }                                                                                                 \
}
#else
#define HWSprites_draw_pixel()                                                                                  \
{                                                                                                     \
    if (x >= HWSprites_x1 && x < HWSprites_x2 && pix != 0 && pix != 15)                                                   \
    {                                                                                                 \
        if (shadow && pix == 0xa)                                                                     \
        {                                                                                             \
            pPixel[x] &= 0xfff;                                                                       \
            pPixel[x] += ((S16_PALETTE_ENTRIES * 2) - ((GET_PALETTE16(pPixel[x]) & 0x8000) >> 3)); \
        }                                                                                             \
        else                                                                                          \
        {                                                                                             \
            pPixel[x] = (pix | color);                                                                \
        }                                                                                             \
    }                                                                                                 \
}
#endif
void HWSprites_render(const uint8_t priority)
{
    uint16_t data;
    const uint32_t numbanks = SPRITES_LENGTH / 0x10000;

    for (data = 0; data < SPRITE_RAM_SIZE; data += 8) 
    {
	uint32_t sprpri; 
	int16_t hide; 
        int32_t height;
        int16_t bank;   
        int32_t top;    
        uint32_t addr;  
        int32_t pitch;  
        int32_t xpos;   
        uint8_t shadow; 
        int32_t vzoom;  
        int32_t ydelta; 
        int32_t flip;   
        int32_t xdelta; 
        int32_t hzoom;  
        int32_t color;  
        int32_t x, y, ytarget, yacc = 0, pix;
	const uint32_t* spritedata;
	int32_t ramaddr;

        // stop when we hit the end of sprite list
        if ((ramBuff[data+0] & 0x8000) != 0) break;

        sprpri  = 1 << ((ramBuff[data+3] >> 12) & 3);
        if (sprpri != priority) continue;

        // if hidden, or top greater than/equal to bottom, or invalid bank, punt
        hide    = (ramBuff[data+0] & 0x5000);
        height  = (ramBuff[data+5] >> 8) + 1;       
        if (hide != 0 || height < 2) continue;
        
        bank    = (ramBuff[data+0] >> 9) & 7;
        top     = (ramBuff[data+0] & 0x1ff) - 0x100;
        addr    = ramBuff[data+1];
        pitch  = ((ramBuff[data+2] >> 1) | ((ramBuff[data+4] & 0x1000) << 3)) >> 8;
        xpos    =  ramBuff[data+6]; // moved from original structure to accomodate widescreen
        shadow  = (ramBuff[data+3] >> 14) & 1;
        vzoom    = ramBuff[data+3] & 0x7ff;
        ydelta = ((ramBuff[data+4] & 0x8000) != 0) ? 1 : -1;
        flip   = (~ramBuff[data+4] >> 14) & 1;
        xdelta = ((ramBuff[data+4] & 0x2000) != 0) ? 1 : -1;
        hzoom    = ramBuff[data+4] & 0x7ff;     
        color   = COLOR_BASE + ((ramBuff[data+5] & 0x7f) << 4);
            
        // adjust X coordinate
        // note: the threshhold below is a guess. If it is too high, rachero will draw garbage
        // If it is too low, smgp won't draw the bottom part of the road
        if (xpos < 0x80 && xdelta < 0)
            xpos += 0x200;
        xpos -= 0xbe;

        // initialize the end address to the start address
        ramBuff[data+7] = addr;

        // clamp to within the memory region size
        if (numbanks)
            bank %= numbanks;

        spritedata = sprites + 0x10000 * bank;

        // clamp to a maximum of 8x (not 100% confirmed)
        if (vzoom < 0x40) vzoom = 0x40;
        if (hzoom < 0x40) hzoom = 0x40;


        // loop from top to bottom
        ytarget = top + ydelta * height;

        // Adjust for widescreen mode
        xpos += Config_s16_x_off;

        // Adjust for hi-res mode
        if (Config_video.hires)
        {
            xpos <<= 1;
            top <<= 1;
            ytarget <<= 1;
            hzoom >>= 1;
            vzoom >>= 1;
        }

#define DO_HWSPRITE_PIXEL_FW(_shift_,_add_) \
if( xacc<0x200 )\
{\
 pix = (pixels >> _shift_ ) & 0xf;\
 if( (pix==0)||(pix==15) )\
 {\
  do {x+=_add_;xacc+=hzoom; } while( xacc<0x200 ); \
 }\
 else\
 {\
  pix |= color;\
  do\
  {\
   if (x >= HWSprites_x1 && x < HWSprites_x2) pPixel[x] = pix;\
   x+=_add_;xacc+=hzoom;\
  }\
  while( xacc<0x200 );\
 }\
}\
xacc-=0x200;

#define DO_HWSPRITE_PIXEL_FWS(_shift_,_add_) \
if( xacc<0x200 )\
{\
 pix = (pixels >> _shift_ ) & 0xf;\
 if( (pix==0)||(pix==15) )\
 {\
  do {x+=_add_;xacc+=hzoom; } while( xacc<0x200 ); \
 }\
 else\
 {\
  if( pix == 0xa )\
  {\
   do \
   { \
    if(x >= HWSprites_x1 && x < HWSprites_x2) \
    {\
     pPixel[x] &=0xfff;\
     pPixel[x] += ((S16_PALETTE_ENTRIES * 2) - ((GET_PALETTE16(pPixel[x]) & 0x8000) >> 3));\
    }\
    x+=_add_;xacc+=hzoom;\
   }\
   while( xacc<0x200 );\
  } \
  else \
  { \
   pix |= color; \
   do \
   { \
    if (x >= HWSprites_x1 && x < HWSprites_x2) pPixel[x] = pix;\
    x+=_add_;xacc+=hzoom;\
   }\
   while( xacc<0x200 );\
  }\
 }\
}\
xacc-=0x200;

	/* compute ROW cache */
	{
	  int16_t group;
	  uint32_t cpos = 0;
	  int32_t  xacc = 0;
	  uint32_t reloads = 0;
	  uint32_t xmax;

	  if( xdelta > 0 )
	  {
	 	xmax = (Config_s16_width > HWSprites_x2) ? HWSprites_x2 : Config_s16_width;
	 	for (x = xpos; (x < xmax) && (reloads<128) ; )
	 	{
	 	 ROW_Cache[cpos++] = ROWC_Reload;
	 	 reloads++;
	 	 for( group = 0x20 ; group > 0 ; group -= 4 )
	 	 {
	 	 	while( xacc < 0x200 )
	 	 	{
		 	   if( (x >= HWSprites_x1) && (x < HWSprites_x2) ) /* skip pixel if out of cliprect */
		 	   {
		 	   	if( flip )
		 	   		ROW_Cache[cpos++] = 32-group;
		 	   	else
		    			ROW_Cache[cpos++] = group-4;
		 	   } 
		 	   x++;
		 	   xacc += hzoom;
		 	}
		 	xacc -= 0x200;
		 }
	 	}
 	        ROW_Cache_x1 = (xpos < HWSprites_x1) ? HWSprites_x1 : xpos;
	  }
	  else /* xdelta > 0 */
	  {
	 	for (x = xpos; (x >= 0) && (reloads<128) ; )
	 	{
	 	 ROW_Cache[cpos++] = ROWC_Reload;
	 	 reloads++;
	 	 for( group = 0x20 ; group > 0 ; group -= 4 )
	 	 {
	 	 	while( xacc < 0x200 )
	 	 	{
		 	   if (x >= HWSprites_x1 && x < HWSprites_x2 ) /* skip pixel if out of cliprect */
		 	   {
		 	   	if( flip )
		 	   		ROW_Cache[cpos++] = 32-group;
				else
					ROW_Cache[cpos++] = group-4;
		 	   }
		 	   x--;
		 	   xacc += hzoom;
		 	}
		 	xacc -= 0x200;
		 }
		}
 	        ROW_Cache_x1 = (xpos > HWSprites_x2) ? HWSprites_x2 : xpos;
	  }    /* xdelta > 0 */
	 
	  ROW_Cache[cpos++] = ROWC_End;
	}

	 if( shadow )
	 {
		int8_t   *rowc;
		uint32_t cmpend  = 0xf0;
		int32_t  ramstep = 1;
		
		if( flip )
		{
			cmpend = 0x0f000000;
			ramstep= -1;
		}

		for (y = top; y != ytarget; y += ydelta)
		{
		
		    // skip drawing if not within the cliprect
		    if (y >= 0 && y < Config_s16_height)
		    {
			uint16_t* pPixel = &Video_pixels[y * Config_s16_width];
			int32_t xacc = 0;

			rowc = ROW_Cache;
#if 1
			    // start at the word before because we preincrement below
			    ramaddr = addr - ramstep; // ramaddr = (addr - 1);

			    if( xdelta > 0 )
			    {
				uint32_t pixels = 0;
				int8_t cmd;
				x = ROW_Cache_x1;
	
				while( 1 )
				{
					cmd = *rowc++;
					if( cmd >= 0 )
					{
						 pix = (pixels >> (int)cmd ) & 0xf;
						 if( (pix != 0) && (pix!=0xf) )
						 {
						  if( pix != 0xa )
						  {
						   pPixel[x]=pix | color;
						  }
						  else
						  {
						   pPixel[x] += ((S16_PALETTE_ENTRIES * 2) - ((GET_PALETTE16(pPixel[x]) & 0x8000) >> 3));
						  }
						 }
						 x++;
					}
					else
					{
						ramaddr += ramstep;
						
						if( cmd == (int8_t)ROWC_End )
							break;
						// stop if the second-to-last pixel in the group was 0xf
						if( (pixels & cmpend) == cmpend )
							break;
						/* currently, the only "other" command is "reload" */
						pixels = spritedata[ramaddr];
					}
				}
			    }
			    else
			    {
				uint32_t pixels = 0;
				int8_t cmd;
				x = ROW_Cache_x1;
	
				while( 1 )
				{
					cmd = *rowc++;
					if( cmd >= 0 )
					{
						 pix = (pixels >> (int)cmd ) & 0xf;
						 if( (pix != 0) && (pix!=0xf) )
						 {
						  if( pix != 0xa )
						  {
						   pPixel[x]=pix | color;
						  }
						  else
						  {
						   pPixel[x] += ((S16_PALETTE_ENTRIES * 2) - ((GET_PALETTE16(pPixel[x]) & 0x8000) >> 3));
						  }
						 }
						 x--;
					}
					else
					{
						ramaddr += ramstep;
						if( cmd == (int8_t)ROWC_End )
							break;
						// stop if the second-to-last pixel in the group was 0xf
						if( (pixels & cmpend) == cmpend )
							break;
						/* currently, the only "other" command is "reload" */
						pixels = spritedata[ramaddr];
					}
				}
			    }
#else
			// non-flipped case
			if (flip == 0)
			{
			    // start at the word before because we preincrement below
			    ramaddr = (addr - 1);
			    if( xdelta > 0 )
			    {
				    for (x = xpos; x < Config_s16_width ; )
				    {
					uint32_t pixels = spritedata[++ramaddr]; // Add to base sprite data the vzoom value

					// draw 8 pixels
					DO_HWSPRITE_PIXEL_FWS(28,1);//while (xacc < 0x200) { HWSprites_draw_pixel(); x++; xacc += hzoom; } xacc -= 0x200;
					DO_HWSPRITE_PIXEL_FWS(24,1);//while (xacc < 0x200) { HWSprites_draw_pixel(); x++; xacc += hzoom; } xacc -= 0x200;
					DO_HWSPRITE_PIXEL_FWS(20,1);//while (xacc < 0x200) { HWSprites_draw_pixel(); x++; xacc += hzoom; } xacc -= 0x200;
					DO_HWSPRITE_PIXEL_FWS(16,1);//while (xacc < 0x200) { HWSprites_draw_pixel(); x++; xacc += hzoom; } xacc -= 0x200;
					DO_HWSPRITE_PIXEL_FWS(12,1);//while (xacc < 0x200) { HWSprites_draw_pixel(); x++; xacc += hzoom; } xacc -= 0x200;
					DO_HWSPRITE_PIXEL_FWS(8,1);//while (xacc < 0x200) { HWSprites_draw_pixel(); x++; xacc += hzoom; } xacc -= 0x200;
					DO_HWSPRITE_PIXEL_FWS(4,1);//while (xacc < 0x200) { HWSprites_draw_pixel(); x++; xacc += hzoom; } xacc -= 0x200;
					DO_HWSPRITE_PIXEL_FWS(0,1);//while (xacc < 0x200) { HWSprites_draw_pixel(); x++; xacc += hzoom; } xacc -= 0x200;

					// stop if the second-to-last pixel in the group was 0xf
					if ((pixels & 0x000000f0) == 0x000000f0)
					    break;
				    }
			    }
			    else
			    {
				    for (x = xpos; x >= 0; )
				    {
					uint32_t pixels = spritedata[++ramaddr]; // Add to base sprite data the vzoom value

					// draw 8 pixels
					DO_HWSPRITE_PIXEL_FWS(28,-1);//while (xacc < 0x200) { HWSprites_draw_pixel(); x--; xacc += hzoom; } xacc -= 0x200;
					DO_HWSPRITE_PIXEL_FWS(24,-1);//while (xacc < 0x200) { HWSprites_draw_pixel(); x--; xacc += hzoom; } xacc -= 0x200;
					DO_HWSPRITE_PIXEL_FWS(20,-1);//while (xacc < 0x200) { HWSprites_draw_pixel(); x--; xacc += hzoom; } xacc -= 0x200;
					DO_HWSPRITE_PIXEL_FWS(16,-1);//while (xacc < 0x200) { HWSprites_draw_pixel(); x--; xacc += hzoom; } xacc -= 0x200;
					DO_HWSPRITE_PIXEL_FWS(12,-1);//while (xacc < 0x200) { HWSprites_draw_pixel(); x--; xacc += hzoom; } xacc -= 0x200;
					DO_HWSPRITE_PIXEL_FWS(8,-1);//while (xacc < 0x200) { HWSprites_draw_pixel(); x--; xacc += hzoom; } xacc -= 0x200;
					DO_HWSPRITE_PIXEL_FWS(4,-1);//while (xacc < 0x200) { HWSprites_draw_pixel(); x--; xacc += hzoom; } xacc -= 0x200;
					DO_HWSPRITE_PIXEL_FWS(0,-1);//while (xacc < 0x200) { HWSprites_draw_pixel(); x--; xacc += hzoom; } xacc -= 0x200;

					// stop if the second-to-last pixel in the group was 0xf
					if ((pixels & 0x000000f0) == 0x000000f0)
					    break;
				    }
			    }
			}
			// flipped case
			else
			{
#if 1
			    // start at the word before because we preincrement below
			    ramaddr = (addr + 1);
			    if( xdelta > 0 )
			    {
				    for (x = xpos; x < Config_s16_width ; )
				    {
					uint32_t pixels = spritedata[--ramaddr]; // Add to base sprite data the vzoom value

					// draw 8 pixels
					DO_HWSPRITE_PIXEL_FWS(0,1);//while (xacc < 0x200) { HWSprites_draw_pixel(); x++; xacc += hzoom; } xacc -= 0x200;
					DO_HWSPRITE_PIXEL_FWS(4,1);//while (xacc < 0x200) { HWSprites_draw_pixel(); x++; xacc += hzoom; } xacc -= 0x200;
					DO_HWSPRITE_PIXEL_FWS(8,1);//while (xacc < 0x200) { HWSprites_draw_pixel(); x++; xacc += hzoom; } xacc -= 0x200;
					DO_HWSPRITE_PIXEL_FWS(12,1);//while (xacc < 0x200) { HWSprites_draw_pixel(); x++; xacc += hzoom; } xacc -= 0x200;
					DO_HWSPRITE_PIXEL_FWS(16,1);//while (xacc < 0x200) { HWSprites_draw_pixel(); x++; xacc += hzoom; } xacc -= 0x200;
					DO_HWSPRITE_PIXEL_FWS(20,1);//while (xacc < 0x200) { HWSprites_draw_pixel(); x++; xacc += hzoom; } xacc -= 0x200;
					DO_HWSPRITE_PIXEL_FWS(24,1);//while (xacc < 0x200) { HWSprites_draw_pixel(); x++; xacc += hzoom; } xacc -= 0x200;
					DO_HWSPRITE_PIXEL_FWS(28,1);//while (xacc < 0x200) { HWSprites_draw_pixel(); x++; xacc += hzoom; } xacc -= 0x200;

					// stop if the second-to-last pixel in the group was 0xf
					if ((pixels & 0x0f000000) == 0x0f000000)
					    break;
				    }
			    }
			    else
			    {
				    for (x = xpos; x >= 0; )
				    {
					uint32_t pixels = spritedata[--ramaddr]; // Add to base sprite data the vzoom value

					// draw 8 pixels
					DO_HWSPRITE_PIXEL_FWS(0,-1);//while (xacc < 0x200) { HWSprites_draw_pixel(); x--; xacc += hzoom; } xacc -= 0x200;
					DO_HWSPRITE_PIXEL_FWS(4,-1);//while (xacc < 0x200) { HWSprites_draw_pixel(); x--; xacc += hzoom; } xacc -= 0x200;
					DO_HWSPRITE_PIXEL_FWS(8,-1);//while (xacc < 0x200) { HWSprites_draw_pixel(); x--; xacc += hzoom; } xacc -= 0x200;
					DO_HWSPRITE_PIXEL_FWS(12,-1);//while (xacc < 0x200) { HWSprites_draw_pixel(); x--; xacc += hzoom; } xacc -= 0x200;
					DO_HWSPRITE_PIXEL_FWS(16,-1);//while (xacc < 0x200) { HWSprites_draw_pixel(); x--; xacc += hzoom; } xacc -= 0x200;
					DO_HWSPRITE_PIXEL_FWS(20,-1);//while (xacc < 0x200) { HWSprites_draw_pixel(); x--; xacc += hzoom; } xacc -= 0x200;
					DO_HWSPRITE_PIXEL_FWS(24,-1);//while (xacc < 0x200) { HWSprites_draw_pixel(); x--; xacc += hzoom; } xacc -= 0x200;
					DO_HWSPRITE_PIXEL_FWS(28,-1);//while (xacc < 0x200) { HWSprites_draw_pixel(); x--; xacc += hzoom; } xacc -= 0x200;

					// stop if the second-to-last pixel in the group was 0xf
					if ((pixels & 0x0f000000) == 0x0f000000)
					    break;
				    }
			    }

#else
			    // start at the word after because we predecrement below
			    ramaddr = (addr + 1);

			    for (x = xpos; (xdelta > 0 && x < Config_s16_width) || (xdelta < 0 && x >= 0); )
			    {
				uint32_t pixels = spritedata[--ramaddr];

				// draw four pixels
				pix = (pixels >>  0) & 0xf; while (xacc < 0x200) { HWSprites_draw_pixelS(); x += xdelta; xacc += hzoom; } xacc -= 0x200;
				pix = (pixels >>  4) & 0xf; while (xacc < 0x200) { HWSprites_draw_pixelS(); x += xdelta; xacc += hzoom; } xacc -= 0x200;
				pix = (pixels >>  8) & 0xf; while (xacc < 0x200) { HWSprites_draw_pixelS(); x += xdelta; xacc += hzoom; } xacc -= 0x200;
				pix = (pixels >> 12) & 0xf; while (xacc < 0x200) { HWSprites_draw_pixelS(); x += xdelta; xacc += hzoom; } xacc -= 0x200;
				pix = (pixels >> 16) & 0xf; while (xacc < 0x200) { HWSprites_draw_pixelS(); x += xdelta; xacc += hzoom; } xacc -= 0x200;
				pix = (pixels >> 20) & 0xf; while (xacc < 0x200) { HWSprites_draw_pixelS(); x += xdelta; xacc += hzoom; } xacc -= 0x200;
				pix = (pixels >> 24) & 0xf; while (xacc < 0x200) { HWSprites_draw_pixelS(); x += xdelta; xacc += hzoom; } xacc -= 0x200;
				pix = (pixels >> 28) & 0xf; while (xacc < 0x200) { HWSprites_draw_pixelS(); x += xdelta; xacc += hzoom; } xacc -= 0x200;

				// stop if the second-to-last pixel in the group was 0xf
				if ((pixels & 0x0f000000) == 0x0f000000)
				    break;
			    }
#endif
			}
#endif
		    }
		    // accumulate zoom factors; if we carry into the high bit, skip an extra row
		    yacc += vzoom; 
		    addr += pitch * (yacc >> 9);
		    yacc &= 0x1ff;
		}
		ramBuff[data+7] = ramaddr;
	 }
	 else /* shadow */
	 {
		int8_t   *rowc;
		uint32_t cmpend  = 0xf0;
		int32_t  ramstep = 1;
		
		if( flip )
		{
			cmpend = 0x0f000000;
			ramstep= -1;
		}
	
		for (y = top; y != ytarget; y += ydelta)
		{
		    // skip drawing if not within the cliprect
		    if (y >= 0 && y < Config_s16_height)
		    {
			uint16_t* pPixel = &Video_pixels[y * Config_s16_width];
			int32_t xacc = 0;

			rowc = ROW_Cache;

			// non-flipped case
			if(1)//if (flip == 0)
			{
			    // start at the word before because we preincrement below
			    ramaddr = addr - ramstep; // ramaddr = (addr - 1);
			    if( xdelta > 0 )
			    {
#if 1
				uint32_t pixels = 0;
				int8_t cmd;
				x = ROW_Cache_x1;
	
				while( 1 )
				{
					cmd = *rowc++;
					if( cmd >= 0 )
					{
						 pix = (pixels >> (int)cmd ) & 0xf;
						 if( (pix != 0) && (pix!=0xf) )
						  pPixel[x]=pix | color;
						 x++;
					}
					else
					{
						ramaddr += ramstep;
						
						if( cmd == (int8_t)ROWC_End )
							break;
						// stop if the second-to-last pixel in the group was 0xf
						if( (pixels & cmpend) == cmpend )
							break;
						/* currently, the only "other" command is "reload" */
						pixels = spritedata[ramaddr];
					}
				}
#else
				    for (x = xpos; x < Config_s16_width ; )
				    {
					uint32_t pixels = spritedata[++ramaddr]; // Add to base sprite data the vzoom value

					// draw 8 pixels
					DO_HWSPRITE_PIXEL_FW(28,1);//while (xacc < 0x200) { HWSprites_draw_pixel(); x++; xacc += hzoom; } xacc -= 0x200;
					DO_HWSPRITE_PIXEL_FW(24,1);//while (xacc < 0x200) { HWSprites_draw_pixel(); x++; xacc += hzoom; } xacc -= 0x200;
					DO_HWSPRITE_PIXEL_FW(20,1);//while (xacc < 0x200) { HWSprites_draw_pixel(); x++; xacc += hzoom; } xacc -= 0x200;
					DO_HWSPRITE_PIXEL_FW(16,1);//while (xacc < 0x200) { HWSprites_draw_pixel(); x++; xacc += hzoom; } xacc -= 0x200;
					DO_HWSPRITE_PIXEL_FW(12,1);//while (xacc < 0x200) { HWSprites_draw_pixel(); x++; xacc += hzoom; } xacc -= 0x200;
					DO_HWSPRITE_PIXEL_FW(8,1);//while (xacc < 0x200) { HWSprites_draw_pixel(); x++; xacc += hzoom; } xacc -= 0x200;
					DO_HWSPRITE_PIXEL_FW(4,1);//while (xacc < 0x200) { HWSprites_draw_pixel(); x++; xacc += hzoom; } xacc -= 0x200;
					DO_HWSPRITE_PIXEL_FW(0,1);//while (xacc < 0x200) { HWSprites_draw_pixel(); x++; xacc += hzoom; } xacc -= 0x200;

					// stop if the second-to-last pixel in the group was 0xf
					if ((pixels & 0x000000f0) == 0x000000f0)
					    break;
				    }
#endif
			    }
			    else
			    {
#if 1
				uint32_t pixels = 0;
				int8_t cmd;
				x = ROW_Cache_x1;
	
				while( 1 )
				{
					cmd = *rowc++;
					if( cmd >= 0 )
					{
						 pix = (pixels >> (int)cmd ) & 0xf;
						 if( (pix != 0) && (pix!=0xf) )
						  pPixel[x]=pix | color;
						 x--;
					}
					else
					{
						ramaddr += ramstep;
						if( cmd == (int8_t)ROWC_End )
							break;
						// stop if the second-to-last pixel in the group was 0xf
						if( (pixels & cmpend) == cmpend )
							break;
						/* currently, the only "other" command is "reload" */
						pixels = spritedata[ramaddr];
					}
				}
#else
				    for (x = xpos; x >= 0; )
				    {
					uint32_t pixels = spritedata[++ramaddr]; // Add to base sprite data the vzoom value

					// draw 8 pixels
					DO_HWSPRITE_PIXEL_FW(28,-1);//while (xacc < 0x200) { HWSprites_draw_pixel(); x--; xacc += hzoom; } xacc -= 0x200;
					DO_HWSPRITE_PIXEL_FW(24,-1);//while (xacc < 0x200) { HWSprites_draw_pixel(); x--; xacc += hzoom; } xacc -= 0x200;
					DO_HWSPRITE_PIXEL_FW(20,-1);//while (xacc < 0x200) { HWSprites_draw_pixel(); x--; xacc += hzoom; } xacc -= 0x200;
					DO_HWSPRITE_PIXEL_FW(16,-1);//while (xacc < 0x200) { HWSprites_draw_pixel(); x--; xacc += hzoom; } xacc -= 0x200;
					DO_HWSPRITE_PIXEL_FW(12,-1);//while (xacc < 0x200) { HWSprites_draw_pixel(); x--; xacc += hzoom; } xacc -= 0x200;
					DO_HWSPRITE_PIXEL_FW(8,-1);//while (xacc < 0x200) { HWSprites_draw_pixel(); x--; xacc += hzoom; } xacc -= 0x200;
					DO_HWSPRITE_PIXEL_FW(4,-1);//while (xacc < 0x200) { HWSprites_draw_pixel(); x--; xacc += hzoom; } xacc -= 0x200;
					DO_HWSPRITE_PIXEL_FW(0,-1);//while (xacc < 0x200) { HWSprites_draw_pixel(); x--; xacc += hzoom; } xacc -= 0x200;

					// stop if the second-to-last pixel in the group was 0xf
					if ((pixels & 0x000000f0) == 0x000000f0)
					    break;
				    }
#endif
			    }
			}
			// flipped case
			else
			{
			    // start at the word after because we predecrement below
			    ramaddr = (addr + 1);
			    if( xdelta > 0 )
			    {
				    for (x = xpos; x < Config_s16_width ; )
				    {
					uint32_t pixels = spritedata[--ramaddr]; // Add to base sprite data the vzoom value

					// draw 8 pixels
					DO_HWSPRITE_PIXEL_FW(0,1);//while (xacc < 0x200) { HWSprites_draw_pixel(); x++; xacc += hzoom; } xacc -= 0x200;
					DO_HWSPRITE_PIXEL_FW(4,1);//while (xacc < 0x200) { HWSprites_draw_pixel(); x++; xacc += hzoom; } xacc -= 0x200;
					DO_HWSPRITE_PIXEL_FW(8,1);//while (xacc < 0x200) { HWSprites_draw_pixel(); x++; xacc += hzoom; } xacc -= 0x200;
					DO_HWSPRITE_PIXEL_FW(12,1);//while (xacc < 0x200) { HWSprites_draw_pixel(); x++; xacc += hzoom; } xacc -= 0x200;
					DO_HWSPRITE_PIXEL_FW(16,1);//while (xacc < 0x200) { HWSprites_draw_pixel(); x++; xacc += hzoom; } xacc -= 0x200;
					DO_HWSPRITE_PIXEL_FW(20,1);//while (xacc < 0x200) { HWSprites_draw_pixel(); x++; xacc += hzoom; } xacc -= 0x200;
					DO_HWSPRITE_PIXEL_FW(24,1);//while (xacc < 0x200) { HWSprites_draw_pixel(); x++; xacc += hzoom; } xacc -= 0x200;
					DO_HWSPRITE_PIXEL_FW(28,1);//while (xacc < 0x200) { HWSprites_draw_pixel(); x++; xacc += hzoom; } xacc -= 0x200;

					// stop if the second-to-last pixel in the group was 0xf
					if ((pixels & 0x0f000000) == 0x0f000000)
					    break;
				    }
			    }
			    else
			    {
				    for (x = xpos; x >= 0; )
				    {
					uint32_t pixels = spritedata[--ramaddr]; // Add to base sprite data the vzoom value

					// draw 8 pixels
					DO_HWSPRITE_PIXEL_FW(0,-1);//while (xacc < 0x200) { HWSprites_draw_pixel(); x--; xacc += hzoom; } xacc -= 0x200;
					DO_HWSPRITE_PIXEL_FW(4,-1);//while (xacc < 0x200) { HWSprites_draw_pixel(); x--; xacc += hzoom; } xacc -= 0x200;
					DO_HWSPRITE_PIXEL_FW(8,-1);//while (xacc < 0x200) { HWSprites_draw_pixel(); x--; xacc += hzoom; } xacc -= 0x200;
					DO_HWSPRITE_PIXEL_FW(12,-1);//while (xacc < 0x200) { HWSprites_draw_pixel(); x--; xacc += hzoom; } xacc -= 0x200;
					DO_HWSPRITE_PIXEL_FW(16,-1);//while (xacc < 0x200) { HWSprites_draw_pixel(); x--; xacc += hzoom; } xacc -= 0x200;
					DO_HWSPRITE_PIXEL_FW(20,-1);//while (xacc < 0x200) { HWSprites_draw_pixel(); x--; xacc += hzoom; } xacc -= 0x200;
					DO_HWSPRITE_PIXEL_FW(24,-1);//while (xacc < 0x200) { HWSprites_draw_pixel(); x--; xacc += hzoom; } xacc -= 0x200;
					DO_HWSPRITE_PIXEL_FW(28,-1);//while (xacc < 0x200) { HWSprites_draw_pixel(); x--; xacc += hzoom; } xacc -= 0x200;

					// stop if the second-to-last pixel in the group was 0xf
					if ((pixels & 0x0f000000) == 0x0f000000)
					    break;
				    }
			    }
			}
		    }
		    // accumulate zoom factors; if we carry into the high bit, skip an extra row
		    yacc += vzoom; 
		    addr += pitch * (yacc >> 9);
		    yacc &= 0x1ff;
		}
		ramBuff[data+7] = ramaddr;
	 }
	}
}
