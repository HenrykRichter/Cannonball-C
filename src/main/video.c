/***************************************************************************
    Video Rendering. 
    
    - Renders the System 16 Video Layers
    - Handles Reads and Writes to these layers from the main game code
    - Interfaces with platform specific rendering code

    Copyright Chris White.
    See license.txt for more details.
***************************************************************************/

#include "Video.h"
#include "setup.h"
#include "globals.h"

#ifdef WITH_OPENGL
#include "sdl/rendergl.h"
#else
#include "sdl/rendersw.h"
#endif
#ifndef NULL
#define NULL (0)
#endif
    
uint16_t *Video_pixels = NULL;
Boolean Video_enabled;



    
uint8_t palette[S16_PALETTE_ENTRIES * 2]; // 2 Bytes Per Palette Entry
void Video_refresh_palette(uint32_t);


void Video_Create(void)
{
    HWTiles_Create();
}

void Video_Destroy(void)
{
    HWTiles_Destroy();
    if (Video_pixels) free(Video_pixels);
    Render_disable();
}

int Video_init(video_settings_t* settings)
{
    if (!Video_set_video_mode(settings))
        return 0;

    // Internal pixel array. The size of this is always constant
    if (Video_pixels) free(Video_pixels);
    Video_pixels = (uint16_t*)malloc(Config_s16_width * Config_s16_height * sizeof(uint16_t));

    // Convert S16 tiles to a more useable format
    HWTiles_init(Roms_tiles.rom, Config_video.hires != 0);
    
    Video_clear_tile_ram();
    Video_clear_text_ram();
    if (Roms_tiles.rom)
    {
        free(Roms_tiles.rom);
        Roms_tiles.rom = NULL;
    }

    // Convert S16 sprites
    HWSprites_init(Roms_sprites.rom);
    if (Roms_sprites.rom)
    {
        free(Roms_sprites.rom);
        Roms_sprites.rom = NULL;
    }

    // Convert S16 Road Stuff
    HWRoad_init(Roms_road.rom, Config_video.hires != 0);
    if (Roms_road.rom)
    {
        free(Roms_road.rom);
        Roms_road.rom = NULL;
    }

    Video_enabled = TRUE;
    return 1;
}

void Video_disable()
{
    Render_disable();
}

// ------------------------------------------------------------------------------------------------
// Configure video settings from config file
// ------------------------------------------------------------------------------------------------

int Video_set_video_mode(video_settings_t* settings)
{
    if (settings->widescreen)
    {
        Config_s16_width  = S16_WIDTH_WIDE;
        Config_s16_x_off = (S16_WIDTH_WIDE - S16_WIDTH) / 2;
    }
    else
    {
        Config_s16_width = S16_WIDTH;
        Config_s16_x_off = 0;
    }


    Config_s16_height = S16_HEIGHT;

    // Internal video buffer is doubled in hi-res mode.
    if (settings->hires)
    {
        Config_s16_width  <<= 1;
        Config_s16_height <<= 1;
    }

    if (settings->scanlines < 0) settings->scanlines = 0;
    else if (settings->scanlines > 100) settings->scanlines = 100;

    if (settings->scale < 1)
        settings->scale = 1;

    Render_init(Config_s16_width, Config_s16_height, settings->scale, settings->mode, settings->scanlines);

    return 1;
}

void Video_draw_frame()
{
    
    int i;
 

    if (!Video_enabled)
    {
        // Fill with black Video_pixels
        for (i = 0; i < Config_s16_width * Config_s16_height; i++)
            Video_pixels[i] = 0;
    }
    else
    {
        // OutRun Hardware Video Emulation
        HWTiles_update_tile_values();
        HWRoad_render_background(Video_pixels);
 
        if (Config_video.detailLevel == 2)        
        {
            HWTiles_render_tile_layer(Video_pixels, 1, 0);      // background layer
            HWTiles_render_tile_layer(Video_pixels, 0, 0);      // foreground layer
        }
        
        
        HWRoad_render_foreground(Video_pixels);
        HWSprites_render(8);
        HWTiles_render_text_layer(Video_pixels, 1);
     }

    Render_draw_frame(Video_pixels);
    Render_finalize_frame();
}

// ---------------------------------------------------------------------------
// Text Handling Code
// ---------------------------------------------------------------------------

void Video_clear_text_ram()
{
    uint32_t i;
    for (i = 0; i <= 0xFFF; i++)
        HWTiles_text_ram[i] = 0;
}

void Video_write_text8(uint32_t addr, const uint8_t data)
{
    HWTiles_text_ram[addr & 0xFFF] = data;
}

void Video_write_text16IncP(uint32_t* addr, const uint16_t data)
{
    HWTiles_text_ram[*addr & 0xFFF] = (data >> 8) & 0xFF;
    HWTiles_text_ram[(*addr+1) & 0xFFF] = data & 0xFF;

    *addr += 2;
}

void Video_write_text16(uint32_t addr, const uint16_t data)
{
    HWTiles_text_ram[addr & 0xFFF] = (data >> 8) & 0xFF;
    HWTiles_text_ram[(addr+1) & 0xFFF] = data & 0xFF;
}

void Video_write_text32IncP(uint32_t* addr, const uint32_t data)
{
    HWTiles_text_ram[*addr & 0xFFF] = (data >> 24) & 0xFF;
    HWTiles_text_ram[(*addr+1) & 0xFFF] = (data >> 16) & 0xFF;
    HWTiles_text_ram[(*addr+2) & 0xFFF] = (data >> 8) & 0xFF;
    HWTiles_text_ram[(*addr+3) & 0xFFF] = data & 0xFF;

    *addr += 4;
}

void Video_write_text32(uint32_t addr, const uint32_t data)
{
    HWTiles_text_ram[addr & 0xFFF] = (data >> 24) & 0xFF;
    HWTiles_text_ram[(addr+1) & 0xFFF] = (data >> 16) & 0xFF;
    HWTiles_text_ram[(addr+2) & 0xFFF] = (data >> 8) & 0xFF;
    HWTiles_text_ram[(addr+3) & 0xFFF] = data & 0xFF;
}

uint8_t Video_read_text8(uint32_t addr)
{
    return HWTiles_text_ram[addr & 0xFFF];
}

// ---------------------------------------------------------------------------
// Tile Handling Code
// ---------------------------------------------------------------------------

void Video_clear_tile_ram()
{
    uint32_t i;
    for (i = 0; i <= 0xFFFF; i++)
        HWTiles_tile_ram[i] = 0;
}

void Video_write_tile8(uint32_t addr, const uint8_t data)
{
    HWTiles_tile_ram[addr & 0xFFFF] = data;
} 

void Video_write_tile16IncP(uint32_t* addr, const uint16_t data)
{
    HWTiles_tile_ram[*addr & 0xFFFF] = (data >> 8) & 0xFF;
    HWTiles_tile_ram[(*addr+1) & 0xFFFF] = data & 0xFF;

    *addr += 2;
}

void Video_write_tile16(uint32_t addr, const uint16_t data)
{
    HWTiles_tile_ram[addr & 0xFFFF] = (data >> 8) & 0xFF;
    HWTiles_tile_ram[(addr+1) & 0xFFFF] = data & 0xFF;
}   

void Video_write_tile32IncP(uint32_t* addr, const uint32_t data)
{
    HWTiles_tile_ram[*addr & 0xFFFF] = (data >> 24) & 0xFF;
    HWTiles_tile_ram[(*addr+1) & 0xFFFF] = (data >> 16) & 0xFF;
    HWTiles_tile_ram[(*addr+2) & 0xFFFF] = (data >> 8) & 0xFF;
    HWTiles_tile_ram[(*addr+3) & 0xFFFF] = data & 0xFF;

    *addr += 4;
}

void Video_write_tile32(uint32_t addr, const uint32_t data)
{
    HWTiles_tile_ram[addr & 0xFFFF] = (data >> 24) & 0xFF;
    HWTiles_tile_ram[(addr+1) & 0xFFFF] = (data >> 16) & 0xFF;
    HWTiles_tile_ram[(addr+2) & 0xFFFF] = (data >> 8) & 0xFF;
    HWTiles_tile_ram[(addr+3) & 0xFFFF] = data & 0xFF;
}

uint8_t Video_read_tile8(uint32_t addr)
{
    return HWTiles_tile_ram[addr & 0xFFFF];
}


// ---------------------------------------------------------------------------
// Sprite Handling Code
// ---------------------------------------------------------------------------

void Video_write_sprite16(uint32_t* addr, const uint16_t data)
{
    HWSprites_write(*addr & 0xfff, data);
    *addr += 2;
}

// ---------------------------------------------------------------------------
// Palette Handling Code
// ---------------------------------------------------------------------------

void Video_write_pal8(uint32_t* palAddr, const uint8_t data)
{
    palette[*palAddr & 0x1fff] = data;
    Video_refresh_palette(*palAddr & 0x1fff);
    *palAddr += 1;
}

void Video_write_pal16(uint32_t* palAddr, const uint16_t data)
{    
    uint32_t adr = *palAddr & 0x1fff;
    palette[adr]   = (data >> 8) & 0xFF;
    palette[adr+1] = data & 0xFF;
    Video_refresh_palette(adr);
    *palAddr += 2;
}

void Video_write_pal32IncP(uint32_t* palAddr, const uint32_t data)
{    
    uint32_t adr = *palAddr & 0x1fff;

    palette[adr]   = (data >> 24) & 0xFF;
    palette[adr+1] = (data >> 16) & 0xFF;
    palette[adr+2] = (data >> 8) & 0xFF;
    palette[adr+3] = data & 0xFF;

    Video_refresh_palette(adr);
    Video_refresh_palette(adr+2);

    *palAddr += 4;
}

void Video_write_pal32(uint32_t adr, const uint32_t data)
{    
    adr &= 0x1fff;

    palette[adr]   = (data >> 24) & 0xFF;
    palette[adr+1] = (data >> 16) & 0xFF;
    palette[adr+2] = (data >> 8) & 0xFF;
    palette[adr+3] = data & 0xFF;
    Video_refresh_palette(adr);
    Video_refresh_palette(adr+2);
}

uint8_t Video_read_pal8(uint32_t palAddr)
{
    return palette[palAddr & 0x1fff];
}

uint16_t Video_read_pal16(uint32_t palAddr)
{
    uint32_t adr = palAddr & 0x1fff;
    return (palette[adr] << 8) | palette[adr+1];
}

uint16_t Video_read_pal16IncP(uint32_t* palAddr)
{
    uint32_t adr = *palAddr & 0x1fff;
    *palAddr += 2;
    return (palette[adr] << 8)| palette[adr+1];
}

uint32_t Video_read_pal32(uint32_t* palAddr)
{
    uint32_t adr = *palAddr & 0x1fff;
    *palAddr += 4;
    return (palette[adr] << 24) | (palette[adr+1] << 16) | (palette[adr+2] << 8) | palette[adr+3];
}

// Convert internal System 16 RRRR GGGG BBBB format palette to Video_renderer output format
void Video_refresh_palette(uint32_t palAddr)
{
    uint32_t a,r,g,b;

    palAddr &= ~1;
    a = (palette[palAddr] << 8) | palette[palAddr + 1];
    r = (a & 0x000f) << 1; // r rrr0
    g = (a & 0x00f0) >> 3; // g ggg0
    b = (a & 0x0f00) >> 7; // b bbb0
     

    Render_convert_palette(palAddr, r, g, b);
}
