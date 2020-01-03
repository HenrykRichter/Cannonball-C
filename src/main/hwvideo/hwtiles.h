#ifndef _INC_SRC_MAIN_HWVIDEO_HWTILES_H
#define _INC_SRC_MAIN_HWVIDEO_HWTILES_H

#include "stdint.h"
#include "romloader.h"

enum
{
    HWTILES_LEFT,
    HWTILES_RIGHT,
    HWTILES_CENTRE,
};

typedef struct {
    uint16_t *buf;
    uint32_t *tileptr;
    int16_t  x;
    int16_t  y;
    uint16_t stride;
    uint16_t nTilePalette;
    uint16_t nColourDepth; 
    uint16_t nMaskColour;
    uint16_t nPaletteOffset;
} hwtilerenderarg;


extern uint8_t HWTiles_text_ram[0x1000]; // Text RAM
extern uint8_t HWTiles_tile_ram[0x10000]; // Tile RAM

void HWTiles_Create(void);
void HWTiles_Destroy(void);

void HWTiles_init(uint8_t* src_tiles, const Boolean hires);
void HWTiles_patch_tiles(RomLoader* patch);
void HWTiles_restore_tiles();
void HWTiles_set_x_clamp(const uint16_t);
void HWTiles_update_tile_values();
void HWTiles_render_tile_layer(uint16_t*, uint8_t, uint8_t);
void HWTiles_render_text_layer(uint16_t*, uint8_t);
void HWTiles_render_all_tiles(uint16_t*);
#endif
