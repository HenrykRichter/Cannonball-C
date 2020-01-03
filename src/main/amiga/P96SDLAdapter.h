/*
  P96SDLAdapter.h

  minimal definitions for dummy SDL types to be used with 194x game
  caution: this is not an SDL replacement
  
  also: this file _as is_ won't work with multi-file environments,
  a proper .c would be required

  CAUTION: define P96ADAPTER_MAIN _ONCE_ in the main file (!)

*/
#ifndef _INC_P96SDLAdapter_H_
#define _INC_P96SDLAdapter_H_

typedef unsigned char SDL_keysym;
typedef int SDL_JoyAxisEvent;
typedef int SDL_JoyButtonEvent;

typedef unsigned char  Uint8;
typedef unsigned short Uint16;
typedef unsigned long  Uint32;

struct _SDL_Rect {
	Uint32 x;
	Uint32 y;
	Uint32 w;
	Uint32 h;
};
typedef struct _SDL_Rect SDL_Rect;

struct _SDL_PixelFormat {
	Uint32	Rshift;
	Uint32	Gshift;
	Uint32	Bshift;
	Uint32	Rmask;
	Uint32	Gmask;
	Uint32	Bmask;
};
typedef struct _SDL_PixelFormat SDL_PixelFormat;

struct _SDL_Surface {
	long	w;
	long	h;
	SDL_PixelFormat *format;
	long		pitch;
	unsigned char  *pixels;
};
typedef struct _SDL_Surface SDL_Surface;

struct _SDL_VideoInfo {
        Uint32 hw_available :1; /**< Flag: Can you create hardware surfaces? */
        Uint32 wm_available :1; /**< Flag: Can you talk to a window manager? */
        Uint32 UnusedBits1  :6;
        Uint32 UnusedBits2  :1;
        Uint32 blit_hw      :1; /**< Flag: Accelerated blits HW --> HW */
        Uint32 blit_hw_CC   :1; /**< Flag: Accelerated blits with Colorkey */
        Uint32 blit_hw_A    :1; /**< Flag: Accelerated blits with Alpha */
        Uint32 blit_sw      :1; /**< Flag: Accelerated blits SW --> HW */
        Uint32 blit_sw_CC   :1; /**< Flag: Accelerated blits with Colorkey */
        Uint32 blit_sw_A    :1; /**< Flag: Accelerated blits with Alpha */
        Uint32 blit_fill    :1; /**< Flag: Accelerated color fill */
        Uint32 UnusedBits3  :16;
        Uint32 video_mem;       /**< The total amount of video memory (in K) */
        SDL_PixelFormat *vfmt;  /**< Value: The format of the video surface */
        int    current_w;       /**< Value: The current video mode width */
        int    current_h;       /**< Value: The current video mode height */
};
typedef struct _SDL_VideoInfo SDL_VideoInfo;


#define SDLK_ESCAPE	0x45
#define SDLK_SPACE	0x40
#define SDLK_1		0x01
#define SDLK_2		0x02
#define SDLK_3		0x03
#define SDLK_4		0x04
#define SDLK_5		0x05
#define SDLK_6		0x06
#define SDLK_7		0x07
#define SDLK_8		0x08
#define SDLK_9		0x09
#define SDLK_0		0x0a
#define SDLK_KP0	0x0f

#define SDLK_q		0x10
#define SDLK_w		0x11
#define SDLK_e		0x12
#define SDLK_r		0x13
#define SDLK_t		0x14
#define SDLK_y		0x15
#define SDLK_u		0x16
#define SDLK_i		0x17
#define SDLK_o		0x18
#define SDLK_p		0x19
#define	SDLK_KP1	0x1d
#define SDLK_KP2	0x1e
#define	SDLK_KP3	0x1f

#define SDLK_a		0x20
#define SDLK_s		0x21
#define SDLK_d		0x22
#define SDLK_f		0x23
#define SDLK_g		0x24
#define SDLK_h		0x25
#define SDLK_j		0x26
#define SDLK_k		0x27
#define SDLK_l		0x28
#define	SDLK_KP4	0x2d
#define	SDLK_KP5	0x2e
#define	SDLK_KP6	0x2f
#define SDLK_z		0x31
#define SDLK_x		0x32
#define SDLK_c		0x33
#define SDLK_v		0x34
#define SDLK_b		0x35
#define SDLK_n		0x36
#define SDLK_m		0x37
#define	SDLK_KP7	0x3d
#define	SDLK_KP8	0x3e
#define	SDLK_KP9	0x3f

#define SDLK_RETURN	0x44
#define SDLK_RIGHT	0x4e
#define SDLK_LEFT	0x4f
#define SDLK_DOWN	0x4d
#define SDLK_UP		0x4c
#define SDLK_LSHIFT	0x60
#define SDLK_RSHIFT	0x61
#define SDLK_LALT	0x64	/* LALT = RALT */
#define SDLK_RALT	0x64
#define SDLK_RCTRL	0x63	/* there is no RCTRL on Amiga */
#define SDLK_LCTRL	0x63	


#define SDL_SWSURFACE 1
#define SDL_HWSURFACE 0
#define SDL_INIT_VIDEO 1
#define SDL_INIT_TIMER 2
#define SDL_INIT_JOYSTICK 4



#define SDL_DOUBLEBUF 0 /* FIXME: make it meaningful */
#define SDL_FULLSCREEN 0 /* FIXME: make it meaningful */

#define SDL_HAT_UP	1
#define SDL_HAT_RIGHT	2
#define SDL_HAT_DOWN	4
#define SDL_HAT_LEFT	8

#ifdef P96ADAPTER_MAIN 
#include <proto/timer.h>
#include <proto/exec.h>

struct Device* TimerBase = (0);
static struct timerequest timereq;
struct timeval tvbase,tv;

void _SDL_OpenTimer()
{
	if( TimerBase )
		return;
	memset( &timereq, 0, sizeof( struct timerequest ) );
	timereq.tr_node.io_Message.mn_ReplyPort = CreateMsgPort();
	OpenDevice("timer.device", UNIT_MICROHZ, (struct IORequest *)&timereq, 0);
	TimerBase = timereq.tr_node.io_Device;
	if( !TimerBase )
	{
		if( timereq.tr_node.io_Message.mn_ReplyPort != 0 )
			DeleteMsgPort( timereq.tr_node.io_Message.mn_ReplyPort );
		timereq.tr_node.io_Message.mn_ReplyPort = 0;
	}
}

void _SDL_CloseTimer()
{
	if( TimerBase )
	{
		if( timereq.tr_node.io_Message.mn_ReplyPort != 0 )
			DeleteMsgPort( timereq.tr_node.io_Message.mn_ReplyPort );
		CloseDevice( (struct IORequest *)&timereq );
	}

	TimerBase = (struct Device*)(0);
	timereq.tr_node.io_Message.mn_ReplyPort = 0;
}

unsigned long SDL_GetTicks( )
{
	unsigned long ticks;

	if( !TimerBase )
	{
		_SDL_OpenTimer();
		if( TimerBase )
			GetSysTime( &tvbase );
		return	0;
	}

	GetSysTime( &tv );
	if( tvbase.tv_micro > tv.tv_micro )
	{
		tv.tv_secs--;
		tv.tv_micro += 1000000;
	}
	ticks = ((tv.tv_secs - tvbase.tv_secs) * 1000) + ((tv.tv_micro - tvbase.tv_micro)/1000);

	return ticks;
}

void SDL_Delay( long msecs )
{
	if( !TimerBase )
	{
		_SDL_OpenTimer();
		if( !TimerBase )
			return;
	}

	timereq.tr_node.io_Command = TR_ADDREQUEST;
	timereq.tr_time.tv_secs  = msecs/1000;
	timereq.tr_time.tv_micro = (msecs % 1000)*1000;

	DoIO((struct IORequest *)&timereq );
}

int SDL_JoystickGetButton( int joy, int flag )
{
    unsigned char regB;
    int ret = 0;

    regB=*(volatile unsigned char *)0xBFE001;
    if( joy == 1 )
    {
    	if ((regB & 0x40) ==0)
	{
		ret = 1;
	}
    }
    else
    if( joy == 0)
    {
    	if ((regB & 0x80) ==0)
	{
		ret = 1;
	}
    }

    return ret;
}

int SDL_NumJoysticks( )
{
	return 1;
}
int SDL_JoystickOpen( )
{
	return 1;
}
int SDL_JoystickClose( )
{
	return 1;
}



//int SDL_Init( int which );
//void SDL_Quit( void );
char errstr[] = "SDL adapter error";
char *SDL_GetError( void )
{
	return errstr;
}

int SDL_MUSTLOCK( SDL_Surface *s )
{
	return 0;
}
int SDL_LockSurface( SDL_Surface *s )
{
	return 0;
}
int SDL_UnlockSurface( SDL_Surface *s )
{
	return 0;
}
SDL_Surface *SDL_DisplayFormat( SDL_Surface *s)
{
	return (0);
}

extern const SDL_PixelFormat pixelfmts[];
extern long modeidx;
SDL_Surface *SDL_CreateRGBSurface(int type, int w, int h, int depth, int rm, int gm, int bm, int flags )
{
	SDL_Surface *ret;
	unsigned long d;

	ret = (SDL_Surface*)malloc( sizeof(SDL_Surface) + 32 + w*h*(depth>>3) );
	if( !ret )
		return ret;
	
	ret->w = w;
	ret->h = h;
	ret->pitch = w * (depth>>3);
//	ret->format = (SDL_PixelFormat*)&pixelfmts[modeidx];
	d = (unsigned long)(ret+1);

	ret->pixels = (unsigned char*)( (d+31) & ~31 );

	return ret;
}
void SDL_FreeSurface( SDL_Surface *s )
{
	if( s ) 
		free( s );
}

SDL_Surface *SDL_LoadBMP(const char *path )
{
	return	0;
}
#else /* P96ADAPTER_MAIN */
unsigned long SDL_GetTicks( );
void SDL_Delay( long msecs );
int SDL_JoystickGetButton( int joy, int flag );
char *SDL_GetError( void );
int SDL_MUSTLOCK( SDL_Surface *s );
int SDL_LockSurface( SDL_Surface *s );
int SDL_UnlockSurface( SDL_Surface *s );
SDL_Surface *SDL_DisplayFormat( SDL_Surface *s);
SDL_Surface *SDL_CreateRGBSurface(int type, int w, int h, int depth, int rm, int gm, int bm, int flags );
void SDL_FreeSurface( SDL_Surface *s );
SDL_Surface *SDL_LoadBMP(const char *path );
#endif


#endif /* _INC_P96SDLAdapter_H_ */

