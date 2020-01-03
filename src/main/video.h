/***************************************************************************
    Video Rendering. 
    
    - Renders the System 16 Video Layers
    - Handles Reads and Writes to these layers from the main game code
    - Interfaces with platform specific rendering code

    Copyright Chris White.
    See license.txt for more details.
***************************************************************************/
#ifndef _INC_SRC_MAIN_VIDEO_H
#define _INC_SRC_MAIN_VIDEO_H


#include "stdint.h"
#include "globals.h"
#include "roms.h"
#include "frontend/config.h"
#include "hwvideo/hwtiles.h"
#include "hwvideo/hwsprites.h"
#include "hwvideo/hwroad.h"

extern uint16_t *Video_pixels;

extern Boolean Video_enabled;

void Video_Create();
void Video_Destroy();
    
int Video_init(video_settings_t* settings);
void Video_disable();
int Video_set_video_mode(video_settings_t* settings);
void Video_draw_frame();

void Video_clear_text_ram();
void Video_write_text8(uint32_t, const uint8_t);
void Video_write_text16IncP(uint32_t*, const uint16_t);
void Video_write_text16(uint32_t, const uint16_t);
void Video_write_text32IncP(uint32_t*, const uint32_t);
void Video_write_text32(uint32_t, const uint32_t);
uint8_t Video_read_text8(uint32_t);

void Video_clear_tile_ram();    
void Video_write_tile8(uint32_t, const uint8_t);
void Video_write_tile16IncP(uint32_t*, const uint16_t);
void Video_write_tile16(uint32_t, const uint16_t);
void Video_write_tile32IncP(uint32_t*, const uint32_t);
void Video_write_tile32(uint32_t, const uint32_t);
uint8_t Video_read_tile8(uint32_t);

void Video_write_sprite16(uint32_t*, const uint16_t);

void Video_write_pal8(uint32_t*, const uint8_t);
void Video_write_pal16(uint32_t*, const uint16_t);
void Video_write_pal32IncP(uint32_t*, const uint32_t);
void Video_write_pal32(uint32_t, const uint32_t);
uint8_t Video_read_pal8(uint32_t);
uint16_t Video_read_pal16IncP(uint32_t*);
uint16_t Video_read_pal16(uint32_t);
uint32_t Video_read_pal32(uint32_t*);

#endif
