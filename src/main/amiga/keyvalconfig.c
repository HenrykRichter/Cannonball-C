/*
  file:   keyvalconfig.c

  configuration file parsing, taglist output

  author: Henryk Richter <henryk.richter@gmx.net>

  Input is a section oriented key-value config file,
  comments are expected only at start of the line(s),
  either with # or %
   [GLOBAL]
   I2CADDRESS=$98
   INIT=011800
   # valid VOLTAGET,CURRENT,TEMP,FAN,PRESSURE 
   TYPE=VOLTAGE
   # hex string (prefix 0x or $ optional), no whitespaces(!)
   READPRE=0x06
   READBYTES=2
   NUMBITS=10
   BITOFFSET=2

  
*/

#include <dos/dos.h>
#include <exec/memory.h>
#include <proto/dos.h>
#include <proto/utility.h>
#include <proto/exec.h>
#define _I2CCLASS_INTERNAL /* triggers use of internal library bases */
#include "keyvalconfig.h"          /* loads */

//#undef  _I2CCLASS_INTERNAL /* we don't want the i2cclass_sensor.h internals here */
//#include "i2cclass_sensor.h"
//#include "debug.h"

/* debug */
#if 0
#ifdef _M68881
#error "FPU?"
#endif
#ifdef _FFP
#error "FFP?"
#endif
#endif

/* disable Debug macro if debug.h is missing */
#ifndef D
#define D(x)
#endif

/* 1k buffer, half of it refilled */
#define FBUFSIZE   1024
#define FBUFMARGIN 256

/* parser states */
#define PARS_IDLE       0
#define PARS_DISCARDEOL 1
#define PARS_ASSIGNFIND 2
#define PARS_ARG        3
#define PARS_SECTION    4

/* default allocated tags per device (>1), 
   more chunks of this size, if necesssary */
#define DEVICE_NTAGS 42


/* configuration key mapping */
/* KARG_HEXSTRING,KARG_KEYTYPE,KARG_STRING,KARG_FIX1616,KARG_KEYCUSTOM */
struct keytag confkeytag[] = {
 { CVAR_VIDMODE,	"VIDMODE",       7, KARG_INT },   
 { CVAR_VIDSCALE,	"VIDSCALE",      8, KARG_INT },
 { CVAR_VIDSCANLINES,   "VIDSCANLINES", 12, KARG_INT },
 { CVAR_VIDFPS,         "VIDFPS",        6, KARG_INT },
 { CVAR_VIDWIDE,        "VIDWIDESCREEN",13, KARG_INT },
 { CVAR_VIDHIRES,       "VIDHIRES",      8, KARG_INT },
 { CVAR_SNDENABLE,      "SNDENABLE",     9, KARG_INT },
 { CVAR_SNDADV,         "SNDADVERTISE", 12, KARG_INT },
 { CVAR_SNDPREVIEW,     "SNDPREVIEW",   10, KARG_INT },
 { CVAR_SNDFIXSAMPLES,  "SNDFIXSAMPLES",13, KARG_INT },
 { CVAR_SNDMODS,        "SNDMODS",       7, KARG_INT },
 { CVAR_ENGTIME,        "ENGINETIME",   10, KARG_INT },
 { CVAR_ENGTRAFFIC,     "ENGINETRAFFIC",13, KARG_INT },
 { CVAR_ENGJAPTRACKS,   "ENGINEJAPAN",  11, KARG_INT },
 { CVAR_ENGPROTOTYPE,   "ENGINEPROTO",  11, KARG_INT },
 { CVAR_ENGLEVELOBJS,   "ENGINELEVELOB",13, KARG_INT },
 { CVAR_ENGNEWATTRACT,  "ENGINENEWATT", 12, KARG_INT },
 { CVAR_TTRLAPS,        "TTRLAPS",       7, KARG_INT },
 { CVAR_TTRTRAFFIC,     "TTRTRAFFIC",   10, KARG_INT },
 { CVAR_CONGEAR,        "CONGEAR",       7, KARG_INT },
 { CVAR_CONSTEERSPEED,  "CONSTEERSPEED",13, KARG_INT },
 { CVAR_CONPEDALSPEED,  "CONPEDALSPEED",13, KARG_INT },
 { CVAR_CONKEYUP,       "CONKEYUP",      8, KARG_INT },
 { CVAR_CONKEYDOWN,     "CONKEYDOWN",   10, KARG_INT },
 { CVAR_CONKEYLEFT,     "CONKEYLEFT",   10, KARG_INT },
 { CVAR_CONKEYRIGHT,    "CONKEYRIGHT",  11, KARG_INT },
 { CVAR_CONKEYACC,      "CONKEYACC",     9, KARG_INT },
 { CVAR_CONKEBRAKE,     "CONKEYBRAKE",  11, KARG_INT },
 { CVAR_CONKEYGEAR1,    "CONKEYGEAR1",  11, KARG_INT },
 { CVAR_CONKEYGEAR2,    "CONKEYGEAR2",  11, KARG_INT },
 { CVAR_CONKEYSTART,    "CONKEYSTART",  11, KARG_INT },
 { CVAR_CONKEYCOIN,     "CONKEYCOIN",   10, KARG_INT },
 { CVAR_CONKEYMENU,     "CONKEYMENU",   10, KARG_INT },
 { CVAR_CONKEYVIEW,     "CONKEYVIEW",   10, KARG_INT },
 { CVAR_CONPADACC,      "CONPADACC",     9, KARG_INT },
 { CVAR_CONPADBRAKE,    "CONPADBRAKE",  11, KARG_INT },
 { CVAR_CONPADGEAR1,    "CONPADGEAR1",  11, KARG_INT },
 { CVAR_CONPADGEAR2,    "CONPADGEAR2",  11, KARG_INT },
 { CVAR_CONPADSTART,    "CONPADSTART",  11, KARG_INT },
 { CVAR_CONPADCOIN,     "CONPADCOIN",   10, KARG_INT },
 { CVAR_CONPADMENU,     "CONPADMENU",   10, KARG_INT },
 { CVAR_CONPADVIEW,     "CONPADVIEW",   10, KARG_INT },
 /* */
 { CVAR_HI0  , "HISCORE00", 9  , KARG_HEXSTRING },
 { CVAR_HI1  , "HISCORE01", 9  , KARG_HEXSTRING },
 { CVAR_HI2  , "HISCORE02", 9  , KARG_HEXSTRING },
 { CVAR_HI3  , "HISCORE03", 9  , KARG_HEXSTRING },
 { CVAR_HI4  , "HISCORE04", 9  , KARG_HEXSTRING },
 { CVAR_HI5  , "HISCORE05", 9  , KARG_HEXSTRING },
 { CVAR_HI6  , "HISCORE06", 9  , KARG_HEXSTRING },
 { CVAR_HI7  , "HISCORE07", 9  , KARG_HEXSTRING },
 { CVAR_HI8  , "HISCORE08", 9  , KARG_HEXSTRING },
 { CVAR_HI9  , "HISCORE09", 9  , KARG_HEXSTRING },
 { CVAR_HI10 , "HISCORE10", 9  , KARG_HEXSTRING },
 { CVAR_HI11 , "HISCORE11", 9  , KARG_HEXSTRING },
 { CVAR_HI12 , "HISCORE12", 9  , KARG_HEXSTRING },
 { CVAR_HI13 , "HISCORE13", 9  , KARG_HEXSTRING },
 { CVAR_HI14 , "HISCORE14", 9  , KARG_HEXSTRING },
 { CVAR_HI15 , "HISCORE15", 9  , KARG_HEXSTRING },
 { CVAR_HI16 , "HISCORE16", 9  , KARG_HEXSTRING },
 { CVAR_HI17 , "HISCORE17", 9  , KARG_HEXSTRING },
 { CVAR_HI18 , "HISCORE18", 9  , KARG_HEXSTRING },
 { CVAR_HI19 , "HISCORE19", 9  , KARG_HEXSTRING },
 { TAG_DONE, (0), 0, KARG_INVALID }
};

#if 0
/* key names for types */
struct keytag conftypetag[] = {
 { I2C_VOLTAGE , "VOLTAGE", 7, KARG_INT },
 { I2C_CURRENT , "CURRENT", 7, KARG_INT },
 { TAG_DONE, (0), 0, KARG_INVALID }
};
/* key names for Custom */
struct keytag confcustomtag[] = {
 { I2SENC_BMP280T, "BMP280T",7, KARG_INT },
 { I2SENC_BMP280P, "BMP280P",7, KARG_INT },
 { TAG_DONE, (0), 0, KARG_INVALID }
};
#endif

struct keyvalue {
        char *key;
        int   keylen;
        char *value;
        int   valuelen;
};

struct confparse {
	struct keyvalue kv;

	/* global taglist: branches to sections by I2C_DEVENTRY */
	struct TagItem *retlist;
	LONG   nTags;
	LONG   TagCount;

	/* local taglist: each DEVENTRY is a section in config */
	struct TagItem *section_cur;
	LONG   section_tagcount;
	LONG   section_ntags;

	BPTR   iFile;
	struct FileInfoBlock fib;
	BYTE  *fbuf;
	LONG   fbytes;
	void  *mempool;

	/* parser data */
	LONG  idx;
	LONG  pos;
	LONG  mode;
	LONG  off;
	LONG  totalpos;
};




ASM void *AllocVecPooled( BASEDEF
                          ASMR(a0) void *pool ASMREG(a0),
			  ASMR(d0) LONG len   ASMREG(d0) )
{
	LONG *ret = AllocPooled( pool, len+4 );
	if( ret )
		*ret++ = len+4;

	return (void*)ret;
}

ASM void FreeVecPooled(   BASEDEF
                          ASMR(a0) void *pool ASMREG(a0), 
                          ASMR(a1) void *ptr  ASMREG(a1))
{
	LONG *p  = (LONG*)ptr;
	LONG len = *(--p);
	FreePooled( pool, p, len );
}

#ifdef USE_MATH
ASM void PrintFloat(BASEDEF 
                    ASMR(d0) FRACNUM val ASMREG(d0) )
{
 LONG l = FIX(val);        /* IEEESPFix(val); */

 Printf((STRPTR)"%ld.",l);

 val = FSUB( val, FLT(l)  ); /* IEEESPSub( val, IEEESPFlt( l   ) ); */
 val = FMUL( val, 10000.f ); /* IEEESPMul( val, IEEESPFlt(10000) ); */
 l = FIX(val); /* IEEESPFix(val); */

 Printf( (STRPTR)"%04ld ",l );
}
#endif


/*
  skip leading whitespaces by advancing string pointer
*/
BYTE *conf_delwhitespace( BYTE *str )
{
	while( *str != 0 )
	{
		if( (*str != 0x20) &&
		    (*str != 0x09)   )
		    	break;
		str++;
	}
	return str;
}



/*
  hex string mapped to binary with single-byte length header

  ti_Data pointer format: LEN.B bin.b bin.b ....
*/
ASM LONG conf_setsectionHEX(    BASEDEF
                                ASMR(a0) struct confparse *cfp ASMREG(a0), 
                                ASMR(d0) LONG tagid ASMREG(d0), 
			        ASMR(a1) BYTE *str ASMREG(a1) )
{
	LONG  len = 0;
	LONG  val = 0;
	LONG  digit = 0;
	BYTE *dat,*dat2;

	if( !conf_checksection(BASEARG cfp) ) /* make sure there is room for this tag */
		return 0;


	/* we accept hex with $ and 0x designation as well as blank sequences */
	if( str[0] == '$' )
	{
		str++;
	}
	if( (str[0] == '0') && (str[1] == 'x') )
	{
		str+=2;
	}


	while( str[len++] != 0 ){}
	

	if( !(dat = AllocVecPooled(BASEARG cfp->mempool, (len>>1)+2 ) ))
		return 0;

	dat2 = dat;

	*dat++ = (BYTE)(len>>1);

	while( 1 )
	{
			BYTE sub = 0;

			if( (*str >= 'A') && ( *str <= 'F' ) )
				sub = 'A'-10;
			if( (*str >= 'a') && ( *str <= 'f' ) )
				sub = 'a'-10;
			if( (*str >= '0') && ( *str <= '9' ) )
				sub = '0';
			if( !sub )
				break;
			val = (val<<4) + (*str++ - sub);
			if( !(digit ^= 1) )
			{
				*dat++ = val;
				D(("%02lx ",val));
				val = 0;
			}
	}
	D(("\n"));

	cfp->section_cur[cfp->section_tagcount].ti_Tag  = tagid;
	cfp->section_cur[cfp->section_tagcount].ti_Data = (ULONG)dat2;
	cfp->section_tagcount++;
	return 1;
}


/*
  decimal 16.16 fractional integer conversion
*/
ASM LONG conf_setsectionFIX1616(BASEDEF
                                ASMR(a0) struct confparse *cfp ASMREG(a0), 
                                ASMR(d0) LONG tagid ASMREG(d0), 
			        ASMR(a1) BYTE *str ASMREG(a1) )
{
/* TODO: once tested, merge with USE_MATH path */
#ifdef USE_MATHLIB
	FRACNUM ten = 10.f;
	FRACNUM opoint1 = 0.1f;
	LONG    len = 0;
	FRACNUM val = 0.f;
	FRACNUM frac= 0.1f;
	FRACNUM neg = 1.f;

	if( !conf_checksection(BASEARG cfp) ) /* make sure there is room for this tag */
		return 0;

	if( str[0] == '-' )
	{
		neg = -1.;
		str++;
	}

	while( (str[len] >= '0') && (str[len]<='9' ) )
	{
		val = FMUL( val, ten );	                /* val *= 10.; */
		val = FADD( val, FLT( str[len]-'0' ) ); /* val += (float)((LONG)(str[len]-'0')); */
		len++;
	}
	if( str[len] == '.' )
	{
		len++;
		while( (str[len] >= '0') && (str[len]<='9' ) )
		{
			val = FADD( val, FMUL( frac, FLT( str[len]-'0' ))); /* val  += frac * (float)((LONG)(str[len]-'0')); */
			frac= FMUL( frac, opoint1 );                        /* frac *= 0.1; */
			len++;
		}
	}
	val = FMUL( val, neg ); /* val *= neg; */
	D(("floatval "));
#ifdef DEBUG
	PrintFloat(BASEARG val);
#endif
	D(("\n"));
	cfp->section_cur[cfp->section_tagcount].ti_Tag  = tagid;
	{
	 FRACNUM *f = (FRACNUM*)&cfp->section_cur[cfp->section_tagcount].ti_Data;
	 *f = val;
	}
	cfp->section_tagcount++;

#else  /* USE_MATHLIB */

#ifdef USE_MATH
	LONG  len = 0;
	float val = 0.;
	float frac= 0.1;
	float neg = 1.;

	if( !conf_checksection(BASEARG cfp) ) /* make sure there is room for this tag */
		return 0;

	if( str[0] == '-' )
	{
		neg = -1.;
		str++;
	}

	while( (str[len] >= '0') && (str[len]<='9' ) )
	{
		val *= 10.;
		val += (float)((LONG)(str[len]-'0'));
		len++;
	}
	if( str[len] == '.' )
	{
		len++;
		while( (str[len] >= '0') && (str[len]<='9' ) )
		{
			val  += frac * (float)((LONG)(str[len]-'0'));
			frac *= 0.1;
			len++;
		}
	}
	val *= neg;
	D(("floatval "));
#ifdef DEBUG
	PrintFloat(BASEARG val);
#endif
	D(("\n"));
	cfp->section_cur[cfp->section_tagcount].ti_Tag  = tagid;
	{
	 float *f = (float*)&cfp->section_cur[cfp->section_tagcount].ti_Data;
	 *f = val;
	}
	cfp->section_tagcount++;

#else /* USE_MATH */
	LONG  len = 0;
	LONG  highval = 0;
	LONG  val = 0;
	LONG  neg = 1;
	LONG  komma = -1024;

	if( !conf_checksection(BASEARG cfp) ) /* make sure there is room for this tag */
		return 0;

	if( str[0] == '-' )
	{
		neg = -1;
		str++;
	}

	while( (str[len] >= '0') && (str[len]<='9' ) )
	{
		val *= 10;
		komma++;
		val += (LONG)(str[len]-'0');
		len++;
		if( str[len] == '.' )
		{
			highval = val<<16;
			val = 0;
			komma = 0;
			len++;
		}
	}

	if( komma > 0 )
	{
		val <<= 16;
		while(komma--)
			val/=10;
		val += highval;
	}
	else
	{
		val <<= 16;
	}
	val *= neg;
	D(("fixval %ld (%lx)\n",val,val));
	cfp->section_cur[cfp->section_tagcount].ti_Tag  = tagid;
	cfp->section_cur[cfp->section_tagcount].ti_Data = (ULONG)val;
	cfp->section_tagcount++;
#endif /* USE_MATH */
#endif /* USE_MATHLIB */
	return 1;
}


/*
  decimal and hexadecimal integer conversion
*/
ASM LONG conf_setsectionInt(    BASEDEF
                                ASMR(a0) struct confparse *cfp ASMREG(a0), 
                                ASMR(d0) LONG tagid ASMREG(d0), 
			        ASMR(a1) BYTE *str ASMREG(a1) )
{
	LONG  len = 0;
	LONG  val = 0;
	LONG  neg = 0;
	LONG  hexmode = 0;

	if( !conf_checksection(BASEARG cfp) ) /* make sure there is room for this tag */
		return 0;

	if( str[0] == '-' )
	{
		neg = -1;
		str++;
	}
	if( str[0] == '$' )
	{
		str++;
		hexmode = 1;
	}
	if( (str[0] == '0') && (str[1] == 'x') )
	{
		str+=2;
		hexmode = 1;
	}

	if( hexmode )
	{
		while( 1 )
		{
			BYTE sub = 0;

			if( (*str >= 'A') && ( *str <= 'F' ) )
				sub = 'A'-10;
			if( (*str >= 'a') && ( *str <= 'f' ) )
				sub = 'a'-10;
			if( (*str >= '0') && ( *str <= '9' ) )
				sub = '0';
			if( !sub )
				break;
			val = (val<<4) + (*str++ - sub);
		}
	}
	else
	{
		while( (str[len] >= '0') && (str[len]<='9' ) )
		{
			val  = (val<<3)+(val<<1);//val *= 10;
			val += (LONG)(str[len]-'0');
			len++;
		}
	}
	val = (val^neg)-neg; /* if( neg == 0xffffffff ) val = -1-val-(-1), i.e. 0-val  */
	D(("intval %ld (%lx)\n",val,val));

	cfp->section_cur[cfp->section_tagcount].ti_Tag  = tagid;
	cfp->section_cur[cfp->section_tagcount].ti_Data = (ULONG)val;
	cfp->section_tagcount++;
	return 1;
}

ASM LONG conf_setsectionString( BASEDEF
                                ASMR(a0) struct confparse *cfp ASMREG(a0), 
                                ASMR(d0) LONG tagid ASMREG(d0), 
			        ASMR(a1) BYTE *str ASMREG(a1) )
{
	LONG len = 0;
	BYTE *dat;

	if( !conf_checksection(BASEARG cfp) ) /* make sure there is room for this tag */
		return 0;

	while( str[len++] != 0 ){}

	if( !(dat = AllocVecPooled(BASEARG cfp->mempool, len ) ))
		return 0;

	CopyMem( str, dat, len );

	cfp->section_cur[cfp->section_tagcount].ti_Tag  = tagid;
	cfp->section_cur[cfp->section_tagcount].ti_Data = (ULONG)dat;
	cfp->section_tagcount++;
	return 1;
}

/*
  TYPE=BLAH, where BLAH is mapped into a TAG
*/
ASM LONG conf_setsectionKey(    BASEDEF
                                ASMR(a0) struct confparse *cfp ASMREG(a0), 
                                ASMR(d0) LONG tagid ASMREG(d0), 
			        ASMR(a1) BYTE *str            ASMREG(a1),
      			        ASMR(a2) struct keytag * keys ASMREG(a2)
 )
{
	LONG i;

	if( !conf_checksection(BASEARG cfp) ) /* make sure there is room for this tag */
		return 0;

	for( i=0 ; keys[i].argtype != KARG_INVALID  ; i++ )
	{
		if( !Strnicmp( (STRPTR)keys[i].key, 
		               (STRPTR)str,
			       keys[i].keychars ))
			goto found;
	}
	return 0; /* not found */
found:
	D(("Type %ld, String %s\n",keys[i].tag,(ULONG)keys[i].key));

	cfp->section_cur[cfp->section_tagcount].ti_Tag  = tagid;
	cfp->section_cur[cfp->section_tagcount].ti_Data = keys[i].tag;
	cfp->section_tagcount++;
	return 1;
}


/*
  make room for one more tag in current section`s list
*/
ASM LONG conf_checksection( BASEDEF
                            ASMR(a0) struct confparse *cfp ASMREG(a0) )
{
	if(!cfp->section_cur)
	{
		D(("No Current Section!\n"));
		return 0;
	}
	if( (cfp->section_tagcount+1) >= cfp->section_ntags )
	{
		cfp->section_cur[cfp->section_tagcount].ti_Tag = TAG_MORE;
		cfp->section_cur[cfp->section_tagcount].ti_Data = (ULONG)
		     AllocVecPooled(BASEARG cfp->mempool, sizeof(struct TagItem)*DEVICE_NTAGS);
		cfp->section_ntags = DEVICE_NTAGS;

		if( !cfp->section_cur[cfp->section_tagcount].ti_Data )
			return 0;

		cfp->section_cur = (struct TagItem*)cfp->section_cur[cfp->section_tagcount].ti_Data;
		cfp->section_cur[cfp->section_tagcount].ti_Tag = TAG_MORE;
		cfp->section_tagcount = 0;
	}
	return 1;
}



ASM LONG conf_add_global(   BASEDEF
                            ASMR(a0) struct confparse *cfp  ASMREG(a0), 
			    ASMR(d0) ULONG tag              ASMREG(d0), 
			    ASMR(d1) ULONG tagdata          ASMREG(d1) )
{
	if( cfp->TagCount >= cfp->nTags )
	{
		/* we've read something, prepare tags */
		if( !conf_grow(BASEARG &cfp->retlist, &cfp->nTags ) )
			goto err;
	}

	cfp->retlist[cfp->TagCount].ti_Tag  = tag;
	cfp->retlist[cfp->TagCount].ti_Data = tagdata;
	cfp->TagCount++;

	return 1;
err:
	return 0;
}



ASM LONG conf_add_device( BASEDEF
                          ASMR(a0) struct confparse *cfp  ASMREG(a0)  )
{
	D(("device section header %s\n",(ULONG)&cfp->fbuf[cfp->pos-cfp->off]));


	/* init section for new tags */
	if(!(cfp->section_cur = (struct TagItem*)AllocVecPooled(BASEARG
	                        cfp->mempool, 
	                        sizeof(struct TagItem)*DEVICE_NTAGS)))
		goto err;
	cfp->section_ntags = DEVICE_NTAGS;
	cfp->section_tagcount = 0;
	/* requires initialized section */
	if( !conf_setsectionString(BASEARG cfp, I2SEN_DEVICE, &cfp->fbuf[cfp->pos-cfp->off] ))
		goto err;

#if 1
	return conf_add_global(BASEARG cfp, I2C_DEVENTRY, (ULONG)cfp->section_cur );
#else

	/* update global list */
	cfp->retlist[cfp->TagCount].ti_Tag  = I2C_DEVENTRY;
	cfp->retlist[cfp->TagCount].ti_Data = (ULONG)cfp->section_cur;
	cfp->TagCount++;
	return 1;
#endif
err:
	return 0;
}



/*
  map key=val to TAG+val
*/
ASM void conf_mapkeyarg(BASEDEF
                        ASMR(a0) struct confparse *cfp ASMREG(a0) )
{
	int i;
	for( i=0 ; confkeytag[i].argtype != KARG_INVALID  ; i++ )
	{
		if( !Strnicmp( (STRPTR)confkeytag[i].key, 
		               (STRPTR)cfp->kv.key,
			       confkeytag[i].keychars ))
			goto found;
	}
	D(("arg >%s< not found\n",(ULONG)cfp->kv.key));
	/* if( confkeytag[i].argtype == KARG_INVALID ) */
		return; /* ignore this keyword */
found:
	{
	 BYTE *str = conf_delwhitespace( (BYTE*)cfp->kv.value );

	 switch( confkeytag[i].argtype )
	 {
		case KARG_STRING:
			D(("SetString >%s< for >%s<\n", (ULONG)str,(ULONG)cfp->kv.key));
			conf_setsectionString(BASEARG cfp, confkeytag[i].tag, str );
			break;
		case KARG_INT:
			D(("SetInt >%s< for >%s<\n", (ULONG)str,(ULONG)cfp->kv.key));
			conf_setsectionInt(BASEARG cfp, confkeytag[i].tag, str );
			break;
		case KARG_FIX1616:
			D(("SetFix >%s< for >%s<\n", (ULONG)str,(ULONG)cfp->kv.key));
			conf_setsectionFIX1616(BASEARG  cfp, confkeytag[i].tag, str );
			break;
		case KARG_HEXSTRING:
			D(("SetHex >%s< for >%s<\n", (ULONG)str,(ULONG)cfp->kv.key));
			conf_setsectionHEX(BASEARG cfp, confkeytag[i].tag, str );
			break;
#if 0
		case KARG_KEYTYPE:
			D(("SetKey >%s< for >%s<\n", (ULONG)str,(ULONG)cfp->kv.key));
			conf_setsectionKey(BASEARG cfp, confkeytag[i].tag, str, conftypetag );
			break;
		case KARG_KEYCUSTOM:
			D(("SetCustom >%s< for >%s<\n", (ULONG)str,(ULONG)cfp->kv.key));
			conf_setsectionKey(BASEARG cfp, confkeytag[i].tag, str, confcustomtag );
			break;
#endif
		default:
			D(("unknown arg type\n"));
			break;
	 }
	}

}



ASM void trim_confbuf(BASEDEF
                      ASMR(a0) struct confparse *cfp ASMREG(a0) )
{
	int i;
	BYTE *d = cfp->fbuf;

	/* D(("reload at %ld pos %ld fbytes %ld\n",cfp->totalpos,cfp->pos,cfp->fbytes)); */

	/* mem move */
	for( i=FBUFSIZE ; i < 2*FBUFSIZE ; i++ )
	{
		*d++ = cfp->fbuf[i];
	}
	
	/* adjust position */
	cfp->pos    -= FBUFSIZE;
	cfp->fbytes -= FBUFSIZE;

	/* top off buffer */
	i = Read( cfp->iFile, cfp->fbuf+cfp->fbytes, FBUFSIZE*2-cfp->fbytes);

	if( i > 0 )
		cfp->fbytes += i;

	/*D(("read %ld\n",i));*/
}



/* 
  the tagitem structures returned here are two levels, first level
  is the device list, whose ti_Data points to the taglist describing
  sensors for each device
*/
ASM struct TagItem *config_parse( BASEDEF
                                  ASMR(a0) BYTE *path ASMREG(a0),
                                  ASMR(a1) struct TagItem *oldconf ASMREG(a1) )
{
	struct confparse cf;

	MemClear( &cf, sizeof(struct confparse) );

	if( oldconf )
	{
	 struct TagItem *tmp;
	 LONG count = 0;

	 tmp = oldconf;
	 while( tmp->ti_Tag != TAG_DONE )
	 {
	 	count++;
	 	if( tmp->ti_Tag == I2SEN_MEMHANDLE )
	 	{
	 		cf.mempool = (void*)tmp->ti_Data;
	 		D(("mempool %lx\n",tmp->ti_Data));
	 	}
	 	if( tmp->ti_Tag == TAG_MORE )
	 		tmp = (struct TagItem*)tmp->ti_Data;
		else	tmp++;
	 }
	 cf.retlist  = oldconf;
	 cf.TagCount = count;
	 cf.nTags    = count+1; /* we don`t know better right now, FreeVec to the rescue :-) */
	 D(("tags in last cfg %lx\n (w/o TAG_DONE)",count));
	}
	
	if( !cf.mempool )
		cf.mempool = CreatePool( MEMF_PUBLIC|MEMF_CLEAR,2048,1536 );

	cf.iFile = Open( (STRPTR)path, MODE_OLDFILE );
	if( !cf.iFile )
		goto err;
	ExamineFH( cf.iFile, &cf.fib );

	cf.fbuf = (BYTE*)AllocMem( FBUFSIZE*2+4, MEMF_PUBLIC );
	if( !cf.fbuf )
		goto err;

	if( !(cf.fbytes = Read( cf.iFile, cf.fbuf, FBUFSIZE*2 ) ) )
		goto err;
	cf.fbuf[cf.fbytes]   = 0xa; /* make sure we've got newline at end */
	cf.fbuf[cf.fbytes+1] = 0x0; /* that's why the +4 */

	/* we've read something, prepare tags */
	if( !conf_grow(BASEARG &cf.retlist, &cf.nTags ) )
		goto err;

	/* main loop: parse line by line */
	while( cf.totalpos < cf.fib.fib_Size )
	{
		if( cf.pos >= FBUFSIZE+FBUFMARGIN ) /* trim buffer ? */ 
			trim_confbuf(BASEARG &cf );

                if( cf.mode == PARS_ASSIGNFIND)  /* find "=" mode */
                {
			cf.totalpos++;
                        if( cf.fbuf[cf.pos] != 0xa ) /* invalid ? */
	                        cf.off++;
                        cf.pos++;
                        if( cf.fbuf[cf.pos] == '=' )
                        {
                                cf.mode = PARS_ARG;
				cf.kv.keylen = cf.off;
				cf.kv.key    = (char*)&cf.fbuf[cf.pos-cf.off];
				cf.totalpos++;
                                cf.pos++;
                                cf.off=0;
                        }
                        if( cf.fbuf[cf.pos] == 0xa ) /* invalid ? */
                        {
                                cf.mode = PARS_IDLE;
				cf.pos++;
				cf.totalpos++;
                                continue;
                        }
                }
                if( (cf.mode == PARS_SECTION) ||    /* find end of section header */ 
		    (cf.mode == PARS_DISCARDEOL) || /* discard to EOL mode */
		    (cf.mode == PARS_ARG) )         /* find end of section header */
                {
			cf.totalpos++;
                        cf.off++;
                        cf.pos++;
			if( cf.fbuf[cf.pos] == ']' )
			{
				/* new device */
				cf.fbuf[cf.pos] = 0; /* 0-terminate string */
				if( !conf_add_device(BASEARG &cf ) )
					goto err;
			}

                        if( cf.fbuf[cf.pos] != 0xa )
                                continue;

                        if( cf.mode == PARS_ARG )
                        {
				cf.kv.valuelen = cf.off;
				cf.kv.value    = (char*)&cf.fbuf[cf.pos-cf.off];

				cf.kv.value[cf.kv.valuelen] = 0;
				cf.kv.key[cf.kv.keylen] = 0;  /* 0-terminate strings */
				conf_mapkeyarg(BASEARG &cf );
#if 0
				keyvals[cf.idx].value[keyvals[cf.idx].valuelen] = 0; /* 0-terminate string */
				keyvals[cf.idx].key[keyvals[cf.idx].keylen] = 0;

				D(("key %s val %s idx %ld\n",(ULONG)keyvals[cf.idx].key,(ULONG)keyvals[cf.idx].value,cf.idx));

                                cf.idx++;
                                if( cf.idx >= 512 )
                                        break;
#endif
                        }
			cf.pos++;
			cf.totalpos++;
                        cf.mode = PARS_IDLE; /* revert to keyword mode */
                        continue;
                }

		if( cf.fbuf[cf.pos] == '[' ) /* section header */
		{
			cf.mode = PARS_SECTION;
			cf.off  = -1;
			continue;
		}

                if( cf.fbuf[cf.pos] == '%' ) /* comment: discard to EOL */
                {
                        cf.mode = PARS_DISCARDEOL;
                        continue;
                }
                if( cf.fbuf[cf.pos] == '#' ) /* comment: discard to EOL */
                {
                        cf.mode = PARS_DISCARDEOL;
                        continue;
                }

                if( cf.mode == PARS_IDLE )
                {
                        /* mode = 0: begin keyword scan */
                        cf.off=0;
                        cf.mode=PARS_ASSIGNFIND;
                        continue;

                }

	}

	/* remember memory pool */
	if( !FindTagItem(I2SEN_MEMHANDLE, cf.retlist ) )
	{
	 conf_add_global(BASEARG &cf, I2SEN_MEMHANDLE, (ULONG)cf.mempool );
	}
	conf_add_global(BASEARG &cf, TAG_DONE, 0UL );

err:
	if( (!cf.retlist) && (cf.mempool) )
		 DeletePool( cf.mempool );
	if( cf.iFile )
		Close( cf.iFile );
	if( cf.fbuf )
		FreeMem( cf.fbuf, FBUFSIZE*2+4 );

	return cf.retlist;
}



ASM void config_free( BASEDEF
                      ASMR(a0) struct TagItem *devlist ASMREG(a0)  )
{
	struct TagItem *cur;

	if( !devlist )
		return;

	cur = FindTagItem( I2SEN_MEMHANDLE, devlist );

	if( cur )
		DeletePool( (void*)cur->ti_Data );

	/*D(("Mem Handle %lx\n",(ULONG)cur));*/

	/* TODO: put this in pool, too */
	FreeVec( devlist );
}

ASM LONG config_stringfortag( BASEPROTO
                              ASMR(d0) ULONG   tag  ASMREG(d0),
                              ASMR(a0) char** strp ASMREG(a0) )
{
	LONG len = 0;
	int i;

	*strp = (0); /* not found */

	for( i=0 ; confkeytag[i].argtype != KARG_INVALID  ; i++ )
	{
		if( confkeytag[i].tag != tag )
			continue;
		len   = confkeytag[i].keychars;
		*strp = confkeytag[i].key;
		break;
	}

	return len;
}



/* re-allocate TagItem array to hold more data            */
/* output new size in nTags, return 1 if successful       */
/* in case of error, return (0) but don't touch "curlist" */
#define CONF_GROW_SPEED 16
ASM LONG conf_grow( BASEDEF
                    ASMR(a0) struct TagItem **curlist ASMREG(a0), 
		    ASMR(a1) LONG *nTags              ASMREG(a1))
{
	struct TagItem *retlist;

	/* TODO: put this in pool */
	retlist = AllocVec( (*nTags+CONF_GROW_SPEED)*sizeof(struct TagItem), MEMF_PUBLIC|MEMF_CLEAR );
	if( !retlist )
		return 0;

	if(*curlist )
	{
	 CopyMem( *curlist, retlist, *nTags*sizeof(struct TagItem) );
	 FreeVec( *curlist );
	}

	*curlist = retlist;
	*nTags  += CONF_GROW_SPEED;
	return 1;
}



/* 
  this is just bzero()
*/
void MemClear( void *ptr, LONG bytes )
{
	UBYTE *p = (UBYTE*)ptr;

	while( bytes-- )
		*p++ = 0;
}



