/*
  m68kRoadRender.h

  author: Henryk Richter <henryk.richter@gmx.net>

  purpose: Interface to rendering routines

*/
#ifndef _INC_M68KROADRENDER_H
#define _INC_M68KROADRENDER_H

#include "asminterface.h"

ASM SAVEDS void m68k_RoadRenderLine0( ASMR(a2) void *Rnd ASMREG(a2) );
ASM SAVEDS void m68k_RoadRenderLine1( ASMR(a2) void *Rnd ASMREG(a2) );
ASM SAVEDS void m68k_RoadRenderLine2( ASMR(a2) void *Rnd ASMREG(a2) );
ASM SAVEDS void m68k_RoadRenderLine3( ASMR(a2) void *Rnd ASMREG(a2) );

ASM SAVEDS void m68k_RoadRenderBG( ASMR(a0) void *Rnd ASMREG(a0) );

#endif /* _INC_M68KROADRENDER_H */
