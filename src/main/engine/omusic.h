/***************************************************************************
    Music Selection Screen.

    This is a combination of a tilemap and overlayed sprites.

    Copyright Chris White.
    See license.txt for more details.
***************************************************************************/
#ifndef _INC_SRC_MAIN_ENGINE_OMUSIC_H
#define _INC_SRC_MAIN_ENGINE_OMUSIC_H

#include "outrun.h"


// Music Track Selected By Player
extern uint8_t OMusic_music_selected;

Boolean OMusic_load_widescreen_map();
void OMusic_enable();
void OMusic_disable();
void OMusic_tick();
void OMusic_blit();
void OMusic_check_start();

#endif


