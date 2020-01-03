Cannonball-C
============

See README.md for original version.

Cannonball-C is a conversion of the Cannonball source code to ANSI C. 
The code is intended as a base for less powerful platforms that have 
non-existent or poor C++ compilers, like old consoles, computers, etc.
    
Codebase is based on the source at https://github.com/djyt/cannonball taken 
early September 2015. 

Credits & Support
-----------------

The conversion to C was done by djcc over at the Reassembler forums 
(http://reassembler.game-host.org/). Lantus360 (Modern Vintage Gamer)
did the initial Amiga port of the SDL codebase.

This fork by Henryk Richter is dedicated to a native AmigaOS/68k version 
and does no longer use SDL.
    
Conversion and Porting Notes
----------------------------
    
    - All code converted from C++ to C
    - Removed STL
    - Removed Boost (which disables Cannonboard support)
    - Removed directx, (which disables force feedback)
    - Removed SDL
    - Added Picasso96 drawing layer
    - replaced SDL functions for input/timing/sound by Amiga native versions
    - re-enabled tile (mainly clouds) rendering
    - rewrote lots of internal routines for higher performance,
      partially in 68k asm code
    - due to the multiple rendering layers and the multitude of 
      sprites, this game requires very fast 68k CPU (MC68060 or Vampire)
    - Vampire-specific: Scanline on/off feature, some minor AMMX optimizations

Known Issues
----------------

    - not all config options are working
    - music is only available in game
    - no sound FX supported
             
   
    
