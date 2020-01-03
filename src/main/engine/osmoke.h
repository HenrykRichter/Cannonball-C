/***************************************************************************
    Smoke & Spray Control.
    
    Animate the smoke and spray below the Ferrari's wheels.

    Copyright Chris White.
    See license.txt for more details.
***************************************************************************/
#ifndef _INC_SRC_MAIN_ENGINE_OSMOKE_H
#define _INC_SRC_MAIN_ENGINE_OSMOKE_H

#include "outrun.h"


// Load smoke sprites for next level?
extern int8_t OSmoke_load_smoke_data;

void OSmoke_init();
void OSmoke_setup_smoke_sprite(Boolean);
void OSmoke_draw_ferrari_smoke(oentry*);
void OSmoke_draw(oentry*);
#endif
