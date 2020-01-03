/*
  ammxtilerender.h

  author: Henryk Richter <henryk.richter@gmx.net>

  purpose: Interface to rendering routines

*/
#ifndef _INC_M68KROADRENDER_H
#define _INC_M68KROADRENDER_H

#include "asminterface.h"
#include "hwvideo/hwtiles.h"

ASM SAVEDS void HWTiles_render8x8_tile_mask_loresammx( ASMR(a2) hwtilerenderarg *Rnd ASMREG(a2) );
ASM SAVEDS void HWTiles_render8x8_tile_mask_loresclipammx( ASMR(a2) hwtilerenderarg *Rnd ASMREG(a2) );

#endif /* _INC_M68KROADRENDER_H */
