/***************************************************************************
    Software Video Rendering.  
    
    Copyright Chris White.
    See license.txt for more details.
***************************************************************************/
#ifndef _INC_SDL_MAIN_SDL_RENDERSW_H
#define _INC_SDL_MAIN_SDL_RENDERSW_H

#include "../stdint.h"


Boolean Render_init(int src_width, int src_height, int scale, int video_mode, int scanlines);
void Render_Quit();
void Render_disable();
Boolean Render_start_frame();
Boolean Render_finalize_frame();
void Render_draw_frame(uint16_t* pixels);
void Render_convert_palette(uint32_t adr, uint32_t r, uint32_t g, uint32_t b);
#endif
