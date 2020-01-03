/***************************************************************************
    Front End Menu System.

    This file is part of Cannonball. 
    Copyright Chris White.
    See license.txt for more details.
***************************************************************************/
#ifndef _INC_SRC_MAIN_FRONTEND_MENU_H
#define _INC_SRC_MAIN_FRONTEND_MENU_H

#include "stdint.h"
#include "cannonboard/interface.h"


void Menu_init();
void Menu_tick(Packet* packet);
#endif
