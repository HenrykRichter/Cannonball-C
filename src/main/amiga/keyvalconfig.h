/*
  file:   config.h

  configuration file parsing, taglist output

  author: Henryk Richter <henryk.richter@gmx.net>

*/
#ifndef _INC_CONFIG_H
#define _INC_CONFIG_H

#include <utility/tagitem.h>

#if 0
#ifndef _INC_I2CCLASS_H
#include <libraries/i2cclass.h> /* will load utility/tagitem.h */
#endif
#endif

#ifndef _INC_ASMINTERFACE_H
#include "asminterface.h"
#endif

/* glue definitions: I've copied the code from my I2CSensors config parser */
#define I2SEN_BASE       TAG_USER+0x012c0000
#define I2C_DEVENTRY     (I2SEN_BASE+0x81) /* this points to a taglist            */
#define CVAR_SECTIONHEADER I2C_DEVENTRY
#define I2SEN_MEMHANDLE  (I2SEN_BASE+0x80)
#define I2SEN_CUSTOMBASE (I2SEN_MEMHANDLE+1) /* internal: designation for custom routines */
#define I2SEN_DEVICE     (I2SEN_BASE+1)    /* name string */

/* TODO: move out for more flexibility */
#define CONFVAR_BASETAG	TAG_USER+0x0cba1100
#define CVAR_VIDMODE	   (CONFVAR_BASETAG+1)
#define CVAR_VIDSCALE	   (CONFVAR_BASETAG+2)
#define CVAR_VIDSCANLINES  (CONFVAR_BASETAG+3)
#define CVAR_VIDFPS        (CONFVAR_BASETAG+4)
#define CVAR_VIDWIDE       (CONFVAR_BASETAG+5)
#define CVAR_VIDHIRES      (CONFVAR_BASETAG+6)

#define CVAR_SNDENABLE     (CONFVAR_BASETAG+10)
#define CVAR_SNDADV        (CONFVAR_BASETAG+11)
#define CVAR_SNDPREVIEW    (CONFVAR_BASETAG+12)
#define CVAR_SNDFIXSAMPLES (CONFVAR_BASETAG+13)
#define CVAR_SNDMODS       (CONFVAR_BASETAG+14)

#define CVAR_ENGTIME       (CONFVAR_BASETAG+20)
#define CVAR_ENGTRAFFIC    (CONFVAR_BASETAG+21)
#define CVAR_ENGJAPTRACKS  (CONFVAR_BASETAG+22)
#define CVAR_ENGPROTOTYPE  (CONFVAR_BASETAG+23)
#define CVAR_ENGLEVELOBJS  (CONFVAR_BASETAG+24)
#define CVAR_ENGNEWATTRACT (CONFVAR_BASETAG+25)

#define CVAR_TTRLAPS       (CONFVAR_BASETAG+30)
#define CVAR_TTRTRAFFIC    (CONFVAR_BASETAG+31)

#define CVAR_CONGEAR       (CONFVAR_BASETAG+40)
#define CVAR_CONSTEERSPEED (CONFVAR_BASETAG+41)
#define CVAR_CONPEDALSPEED (CONFVAR_BASETAG+42)
#define CVAR_CONKEYUP      (CONFVAR_BASETAG+43)
#define CVAR_CONKEYDOWN    (CONFVAR_BASETAG+44)
#define CVAR_CONKEYLEFT    (CONFVAR_BASETAG+45)
#define CVAR_CONKEYRIGHT   (CONFVAR_BASETAG+46)
#define CVAR_CONKEYACC     (CONFVAR_BASETAG+47)
#define CVAR_CONKEBRAKE    (CONFVAR_BASETAG+48)
#define CVAR_CONKEYGEAR1   (CONFVAR_BASETAG+49)
#define CVAR_CONKEYGEAR2   (CONFVAR_BASETAG+50)
#define CVAR_CONKEYSTART   (CONFVAR_BASETAG+51)
#define CVAR_CONKEYCOIN    (CONFVAR_BASETAG+52)
#define CVAR_CONKEYMENU    (CONFVAR_BASETAG+53)
#define CVAR_CONKEYVIEW    (CONFVAR_BASETAG+54)

#define CVAR_CONPADACC     (CONFVAR_BASETAG+55)
#define CVAR_CONPADBRAKE   (CONFVAR_BASETAG+56)
#define CVAR_CONPADGEAR1   (CONFVAR_BASETAG+57)
#define CVAR_CONPADGEAR2   (CONFVAR_BASETAG+58)
#define CVAR_CONPADSTART   (CONFVAR_BASETAG+59)
#define CVAR_CONPADCOIN    (CONFVAR_BASETAG+60)
#define CVAR_CONPADMENU    (CONFVAR_BASETAG+61)
#define CVAR_CONPADVIEW    (CONFVAR_BASETAG+62)
/* END TAG DEFINITIONS FOR CONFIG STRUCT */

/* keep these together */
#define CVAR_HI0           (CONFVAR_BASETAG+70)
#define CVAR_HI1           (CONFVAR_BASETAG+71)
#define CVAR_HI2           (CONFVAR_BASETAG+72)
#define CVAR_HI3           (CONFVAR_BASETAG+73)
#define CVAR_HI4           (CONFVAR_BASETAG+74)
#define CVAR_HI5           (CONFVAR_BASETAG+75)
#define CVAR_HI6           (CONFVAR_BASETAG+76)
#define CVAR_HI7           (CONFVAR_BASETAG+77)
#define CVAR_HI8           (CONFVAR_BASETAG+78)
#define CVAR_HI9           (CONFVAR_BASETAG+79)
#define CVAR_HI10          (CONFVAR_BASETAG+80)
#define CVAR_HI11          (CONFVAR_BASETAG+81)
#define CVAR_HI12          (CONFVAR_BASETAG+82)
#define CVAR_HI13          (CONFVAR_BASETAG+83)
#define CVAR_HI14          (CONFVAR_BASETAG+84)
#define CVAR_HI15          (CONFVAR_BASETAG+85)
#define CVAR_HI16          (CONFVAR_BASETAG+86)
#define CVAR_HI17          (CONFVAR_BASETAG+87)
#define CVAR_HI18          (CONFVAR_BASETAG+88)
#define CVAR_HI19          (CONFVAR_BASETAG+89)
#define CVAR_HIMIN	CVAR_HI0
#define CVAR_HIMAX	CVAR_HI19
/* #define CVAR_HI20          (CONFVAR_BASETAG+90) */























/*-----------------------------------------------------------------*/
/*--------------- swap FIX1616 by float when defined --------------*/
//#define  USE_MATH
//#include "mathwrap.h"
#ifndef FRACNUM
#include <math.h>
#define FRACNUM float
#endif

/*-----------------------------------------------------------------*/
/* some fiddly defines to keep the parser code relatively indendent
   of the i2c_sensorbase stuff, if so desired...
   
   This was added to keep library bases in a well known structure,
   whose contents are referenced for library calls

   If you want to get rid of it, see that the Lib bases of
   SysBase,Utilitybase and DOSBase are global and keep the
   defines below empty
*/
#if 1
/* plain arguments for standalone code */
#define BASENAME 
#define BASETYPE 
#define BASEARG  
#define BASEPROTO
#define BASEDEF  
#define BASEPTR  
#else
/* used for library code, where system library bases are redirected to internal 
   fields of this library's base */
#define BASENAME  i2c_sensorbase
#define BASETYPE  struct i2c_sensorbase 
#define BASEARG   BASENAME,
#define BASEPROTO ASMR(a6) BASETYPE* ASMREG(a6),
#define BASEDEF   ASMR(a6) BASETYPE*BASENAME ASMREG(a6),
#define BASEPTR  BASETYPE *BASENAME
#endif

/*-----------------------------------------------------------------*/
#if 0
#ifndef I2C_SENSORBASE_DECL
struct  i2c_sensorbase; /* make library base struct known */
#define I2C_SENSORBASE_DECL
#endif
#endif

struct confparse;


/*----------------- keyword to tag item mapping -------------------*/
struct keytag {
	ULONG	tag;	 /* tag value to apply                     */
	char	*key;	 /* key to search for                      */
	LONG    keychars;/* avoid strlen() for each cmp            */ 
	ULONG	argtype; /* argument type to set in tag data       */
};

#define KARG_INVALID   0
#define KARG_STRING    1 /* String                                 */
#define KARG_INT       2 /* Integer                                */
#define KARG_FIX1616   3 /* 16.16 fractional integer               */
#define KARG_KEYCUSTOM 4 /* String parsed for custom keyword       */
#define KARG_KEYTYPE   5 /* String that's parsed for type keyword  */
#define KARG_HEXSTRING 6 /* hex string                             */


/*-----------------  public functions -----------------------------*/

ASM struct TagItem*config_parse(BASEPROTO 
                                ASMR(a0) BYTE *          ASMREG(a0),
                                ASMR(a1) struct TagItem* ASMREG(a1)
                               );

ASM void config_free(           BASEPROTO
                                ASMR(a0) struct TagItem * ASMREG(a0)
                               );

ASM LONG config_stringfortag( BASEPROTO
                              ASMR(d0) ULONG  ASMREG(d0),
                              ASMR(a0) char** ASMREG(a0) );


/*-----------------  private functions ----------------------------*/

ASM void *AllocVecPooled(       BASEPROTO
                                ASMR(a0) void * ASMREG(a0), 
                                ASMR(d0) LONG ASMREG(d0) 
                               );
ASM void FreeVecPooled(         BASEPROTO
                                ASMR(a0) void * ASMREG(a0), 
                                ASMR(a1) void * ASMREG(a1)
                               );
ASM LONG conf_checksection(     BASEPROTO
                                ASMR(a0) struct confparse * ASMREG(a0)
                               );
ASM LONG conf_grow(             BASEPROTO
                                ASMR(a0) struct TagItem **  ASMREG(a0), 
                                ASMR(a1) LONG *             ASMREG(a1)
                               );
ASM LONG conf_add_global(       BASEPROTO
                                ASMR(a0) struct confparse * ASMREG(a0), 
			        ASMR(d0) ULONG              ASMREG(d0), 
			        ASMR(d1) ULONG              ASMREG(d1)
			       );
ASM LONG conf_add_device(       BASEPROTO
                                ASMR(a0) struct confparse * ASMREG(a0)
                               );
ASM void conf_mapkeyarg(        BASEPROTO
                                ASMR(a0) struct confparse * ASMREG(a0)
                               );
ASM void trim_confbuf(          BASEPROTO
                                ASMR(a0) struct confparse * ASMREG(a0)
                               );

ASM LONG conf_setsectionString( BASEPROTO
                                ASMR(a0) struct confparse * ASMREG(a0), 
                                ASMR(d0) LONG               ASMREG(d0), 
			        ASMR(a1) BYTE *             ASMREG(a1) 
			       );
ASM LONG conf_setsectionHEX(    BASEPROTO
                                ASMR(a0) struct confparse * ASMREG(a0), 
                                ASMR(d0) LONG               ASMREG(d0), 
			        ASMR(a1) BYTE *             ASMREG(a1) 
			       );
ASM LONG conf_setsectionInt(    BASEPROTO
                                ASMR(a0) struct confparse * ASMREG(a0), 
                                ASMR(d0) LONG               ASMREG(d0), 
			        ASMR(a1) BYTE *             ASMREG(a1) 
			       );
ASM LONG conf_setsectionKey(    BASEPROTO
                                ASMR(a0) struct confparse * ASMREG(a0),
                                ASMR(d0) LONG               ASMREG(d0), 
			        ASMR(a1) BYTE *             ASMREG(a1),
			        ASMR(a2) struct keytag *    ASMREG(a2)
			       );
BYTE *conf_delwhitespace( BYTE *str );

#if  defined(USE_MATH) || defined(USE_MATHLIB)
/* semi public :-) */
ASM void PrintFloat(            BASEPROTO
                                ASMR(d0) FRACNUM ASMREG(d0) 
                               );
#endif

void MemClear(                  void *, LONG
                               );


#endif /* _INC_CONFIG_H */
