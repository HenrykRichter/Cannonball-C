/***************************************************************************
    Amiga Input Handling.

    Populates keys array with user Input_
    If porting to a non-SDL platform, you would need to replace this class.

    Copyright Henryk Richter, Chris White.
    See license.txt for more details.
***************************************************************************/

#include <stdlib.h>
#include <stdio.h>

#include "amiga/amigainput.h"
#include "engine/ohud.h"
#include "frontend/config.h"
#include <SDL/SDL_keysym.h>


#define AJOY_LEFT	1
#define AJOY_RIGHT	2
#define AJOY_UP		4
#define AJOY_DOWN	8
#define AJOY_FIR0	16
#define AJOY_FIR1	32
#define AJOY_FIR2	64


Boolean Input_keys[15];
Boolean Input_keys_old[15];

// Has gamepad been found? (on Amiga, we always assume so)
const Boolean Input_gamepad = 1;
// Use analog controls (nope, no support right now)
const int Input_analog = 0;
// Analog Controls
const int Input_a_wheel = 0;
const int Input_a_accel = 0;
const int Input_a_brake = 0;

// Latch last key press for redefines
int Input_key_press;

// Latch last joystick button press for redefines
int16_t Input_joy_button;

unsigned long aInput_JoyState = 0;

// Configurations for keyboard and joypad
int* Input_pad_config;
int* Input_key_config;
int* Input_axis;

int Input_wheel_zone;
int Input_wheel_dead;
int Input_pedals_dead;

/* Mapping from Amiga RAWKEY to SDL symbols (reports US keyboard) */
short amigakey2SDL[128] = {
 /*  0 */ SDLK_QUOTE,
 /*  1 */ SDLK_1,
 /*  2 */ SDLK_2,
 /*  3 */ SDLK_3,
 /*  4 */ SDLK_4,
 /*  5 */ SDLK_5,
 /*  6 */ SDLK_6,
 /*  7 */ SDLK_7,
 /*  8 */ SDLK_8,
 /*  9 */ SDLK_9,
 /* 10 */ SDLK_0,
 /* 11 */ SDLK_MINUS,
 /* 12 */ SDLK_EQUALS,
 /* 13 */ SDLK_BACKSLASH,
 /* 14 */ SDLK_UNKNOWN,
 /* 15 */ SDLK_KP0,
 /*$10 */ SDLK_q,
 /*$11 */ SDLK_w,
 /*$12 */ SDLK_e,
 /*$13 */ SDLK_r,
 /*$14 */ SDLK_t,
 /*$15 */ SDLK_y,
 /*$16 */ SDLK_u,
 /*$17 */ SDLK_i,
 /*$18 */ SDLK_o,
 /*$19 */ SDLK_p,
 /*$1a */ SDLK_LEFTBRACKET,
 /*$1b */ SDLK_RIGHTBRACKET,
 /*$1c */ SDLK_UNKNOWN,
 /*$1d */ SDLK_KP1,
 /*$1e */ SDLK_KP2,
 /*$1f */ SDLK_KP3,
 /*$20 */ SDLK_a,
 /*$21 */ SDLK_s,
 /*$22 */ SDLK_d,
 /*$23 */ SDLK_f,
 /*$24 */ SDLK_g,
 /*$25 */ SDLK_h,
 /*$26 */ SDLK_j,
 /*$27 */ SDLK_k,
 /*$28 */ SDLK_l,
 /*$29 */ SDLK_SEMICOLON,
 /*$2a */ SDLK_BACKQUOTE,
 /*$2b */ SDLK_HASH,
 /* 44 */ SDLK_UNKNOWN,
 /* 45 */ SDLK_KP4,
 /* 46 */ SDLK_KP5,
 /* 47 */ SDLK_KP6,
 /*$30 */ SDLK_LESS,
 /*$31 */ SDLK_z,
 /*$32 */ SDLK_x,
 /*$33 */ SDLK_c,
 /*$34 */ SDLK_v,
 /*$35 */ SDLK_b,
 /*$36 */ SDLK_n,
 /*$37 */ SDLK_m,
 /*$38 */ SDLK_COMMA,
 /*$39 */ SDLK_PERIOD,
 /*$3a */ SDLK_SLASH,
 /*$3b */ SDLK_UNKNOWN,
 /*$3c */ SDLK_KP_PERIOD,
 /*$3d */ SDLK_KP7,
 /*$3e */ SDLK_KP8,
 /*$3f */ SDLK_KP9,
 /*$40 */ SDLK_SPACE,
 /*$41 */ SDLK_BACKSPACE,
 /*$42 */ SDLK_TAB,
 /*$43 */ SDLK_KP_ENTER,
 /*$44 */ SDLK_RETURN,
 /*$45 */ SDLK_ESCAPE,
 /*$46 */ SDLK_DELETE,
 /* 71 */ SDLK_UNKNOWN,
 /* 72 */ SDLK_UNKNOWN,
 /* 73 */ SDLK_UNKNOWN,
 /*$4a */ SDLK_KP_MINUS,
 /* 75 */ SDLK_UNKNOWN,
 /* 76 */ SDLK_UP,
 /* 77 */ SDLK_DOWN,
 /* 78 */ SDLK_RIGHT,
 /* 79 */ SDLK_LEFT,
 /*$50 */ SDLK_F1,
 /* 81 */ SDLK_F2,
 /* 82 */ SDLK_F3,
 /* 83 */ SDLK_F4,
 /* 84 */ SDLK_F5,
 /* 85 */ SDLK_F6,
 /* 86 */ SDLK_F7,
 /* 87 */ SDLK_F8,
 /* 88 */ SDLK_F9,
 /* 89 */ SDLK_F10,
 /*$5a */ SDLK_LEFTBRACKET,  /* actually KP_LEFTBRACKET */
 /*$5b */ SDLK_RIGHTBRACKET, /* actually KP_RIGHTBRACKET */
 /*$5c */ SDLK_KP_DIVIDE,
 /*$5d */ SDLK_KP_MULTIPLY,
 /*$5e */ SDLK_KP_PLUS,
 /*$5f */ SDLK_HELP,
 /*$60 */ SDLK_LSHIFT, 
 /*$61 */ SDLK_RSHIFT, 
 /*$62 */ SDLK_CAPSLOCK,   
 /*$63 */ SDLK_LCTRL, /* nb: no RCTRL on Amiga */
 /*$64 */ SDLK_LALT,
 /*$65 */ SDLK_LMETA, /* or better SUPER ? */
 /*$66 */ SDLK_RMETA,
 /*$67 */ SDLK_RALT,
 /*104 */ SDLK_UNKNOWN,
 /*105 */ SDLK_UNKNOWN,
 /*106 */ SDLK_UNKNOWN,
 /*107 */ SDLK_UNKNOWN,
 /*108 */ SDLK_UNKNOWN,
 /*109 */ SDLK_UNKNOWN,
 /*110 */ SDLK_UNKNOWN,
 /*111 */ SDLK_UNKNOWN,
 /*112 */ SDLK_UNKNOWN,
 /*113 */ SDLK_UNKNOWN,
 /*114 */ SDLK_UNKNOWN,
 /*115 */ SDLK_UNKNOWN,
 /*116 */ SDLK_UNKNOWN,
 /*117 */ SDLK_UNKNOWN,
 /*118 */ SDLK_UNKNOWN,
 /*119 */ SDLK_UNKNOWN,
 /*120 */ SDLK_UNKNOWN,
 /*121 */ SDLK_UNKNOWN,
 /*122 */ SDLK_UNKNOWN,
 /*123 */ SDLK_UNKNOWN,
 /*124 */ SDLK_UNKNOWN,
 /*125 */ SDLK_UNKNOWN,
 /*126 */ SDLK_UNKNOWN,
 /*127 */ SDLK_UNKNOWN,
};

void Input_handle_key(const int, const Boolean);
void Input_handle_joy(const uint8_t, const Boolean);

void Input_init(int pad_id, int* key_config, int* pad_config, int analog, int* axis, int* analog_settings)
{
    Input_key_config  = key_config;
    Input_pad_config  = pad_config;
//    Input_analog      = analog;
    Input_axis        = axis;
//    Input_wheel_zone  = analog_settings[0];
//    Input_wheel_dead  = analog_settings[1];
//    Input_pedals_dead = analog_settings[2];

//    Input_gamepad   = 1;

    aInput_JoyState = 0; /* no joystick buttons pressed */

//    Input_a_wheel = CENTRE;
}

void Input_close()
{
}

// Detect whether a key press change has occurred
Boolean Input_has_pressed(enum presses p)
{
    return Input_keys[p] && !Input_keys_old[p];
}

// Detect whether key is still pressed
Boolean Input_is_pressed(enum presses p)
{
    return Input_keys[p];
}

// Detect whether pressed and clear the press
Boolean Input_is_pressed_clear(enum presses p)
{
    Boolean pressed = Input_keys[p];
    Input_keys[p&0x7f] = FALSE;
    return pressed;
}

// Denote that a frame has been done by copying key presses into previous array
void Input_frame_done()
{
    memcpy(&Input_keys_old, &Input_keys, sizeof(Input_keys));
}

void aInput_handle_key_down( int sym )
{
    Input_key_press = amigakey2SDL[sym&0x7f];
    Input_handle_key(Input_key_press, TRUE);
}

void aInput_handle_key_up( int sym )
{
    Input_handle_key( amigakey2SDL[sym&0x7f], FALSE);
}

void Input_handle_key(const int key, const Boolean is_pressed)
{
    // Redefinable Key Input
    /* fprintf(stderr,"%c ",key); */
    if (key == Input_key_config[0])
        Input_keys[INPUT_UP] = is_pressed;

    else if (key == Input_key_config[1])
        Input_keys[INPUT_DOWN] = is_pressed;

    else if (key == Input_key_config[2])
        Input_keys[INPUT_LEFT] = is_pressed;

    else if (key == Input_key_config[3])
        Input_keys[INPUT_RIGHT] = is_pressed;

    if (key == Input_key_config[4])
        Input_keys[INPUT_ACCEL] = is_pressed;

    if (key == Input_key_config[5])
        Input_keys[INPUT_BRAKE] = is_pressed;

    if (key == Input_key_config[6])
        Input_keys[INPUT_GEAR1] = is_pressed;

    if (key == Input_key_config[7])
        Input_keys[INPUT_GEAR2] = is_pressed;

    if (key == Input_key_config[8])
        Input_keys[INPUT_START] = is_pressed;

    if (key == Input_key_config[9])
        Input_keys[INPUT_COIN] = is_pressed;

    if (key == Input_key_config[10])
        Input_keys[INPUT_MENU] = is_pressed;

    if (key == Input_key_config[11])
        Input_keys[INPUT_VIEWPOINT] = is_pressed;

    // Function keys are not redefinable
    switch (key)
    {
        case SDLK_F1:
            Input_keys[INPUT_PAUSE] = is_pressed;
            break;

        case SDLK_F2:
            Input_keys[INPUT_STEP] = is_pressed;
            break;

        case SDLK_F3:
            Input_keys[INPUT_TIMER] = is_pressed;
            break;

        case SDLK_F5:
            Input_keys[INPUT_MENU] = is_pressed;
            break;
            
        case SDLK_F9:
            Config_video.detailLevel++;
            if (Config_video.detailLevel > 2)
                Config_video.detailLevel = 0;
            
            
            
            break;
    }
}

void aInput_handle_joy_axis( int value )
{
 /* note: input is ignored */
 unsigned long joy = 0,ojoy;
 /* first joystick only */
 unsigned short regW = *(volatile unsigned short*)0xDFF016;
 unsigned char  regB=*(volatile unsigned char *)0xBFE001;

 if ((regB & 0x80) == 0) /* fire0 */
 	joy |= AJOY_FIR0;
 if( (regW & 0x4000) == 0 ) /* fire1 */
 	joy |= AJOY_FIR1;
 if( (regW & 0x1000) == 0 ) /* fire2 */
 	joy |= AJOY_FIR2;

 regW = *(volatile unsigned short*)0xDFF00C;
 if ((regW & 0x0002) !=0){
                joy = joy | AJOY_RIGHT;
 }
 if ((regW & 0x0200) !=0){
                joy = joy | AJOY_LEFT;
 }
 if ((((regW >>1) ^ regW) & 0x0100) !=0){
		joy |= AJOY_UP;
 }
 if((((regW >>1) ^ regW) & 0x0001) !=0){
                 joy = joy | AJOY_DOWN;
 }

 /* react on changes only */
 ojoy = aInput_JoyState ^ joy;
 if( ojoy )
 {
  aInput_JoyState = joy; /* remember new state */
 
  if( ojoy & AJOY_LEFT )
	Input_keys[INPUT_LEFT]  = (joy&AJOY_LEFT) ? TRUE:FALSE;
  if( ojoy & AJOY_RIGHT )
	Input_keys[INPUT_RIGHT] = (joy&AJOY_RIGHT) ? TRUE:FALSE;
  if( ojoy & AJOY_UP )
	Input_keys[INPUT_UP]    = (joy&AJOY_UP)    ? TRUE:FALSE;
  if( ojoy & AJOY_DOWN )
	Input_keys[INPUT_DOWN]  = (joy&AJOY_DOWN)  ? TRUE:FALSE;
  if( ojoy & AJOY_FIR0 )
  	Input_handle_joy( 0, (joy&AJOY_FIR0)  ? TRUE:FALSE );
  if( ojoy & AJOY_FIR1 )
  	Input_handle_joy( 1, (joy&AJOY_FIR1)  ? TRUE:FALSE );
  if( ojoy & AJOY_FIR2 )
  	Input_handle_joy( 2, (joy&AJOY_FIR2)  ? TRUE:FALSE );
#if 1
  /* optional: use up/down for shifting and brake */
  if( ojoy & AJOY_UP )
  {
        Input_keys[INPUT_GEAR1] = (joy&AJOY_UP)    ? TRUE:FALSE;
  }
  if( ojoy & AJOY_DOWN )
  {
        Input_keys[INPUT_BRAKE] = (joy&AJOY_DOWN)    ? TRUE:FALSE;
  }
#endif
 }
}


void Input_handle_joy(const uint8_t button, const Boolean is_pressed)
{
    if (button == Input_pad_config[0])
        Input_keys[INPUT_ACCEL] = is_pressed;

    if (button == Input_pad_config[1])
        Input_keys[INPUT_BRAKE] = is_pressed;

    if (button == Input_pad_config[2])
        Input_keys[INPUT_GEAR1] = is_pressed;

    if (button == Input_pad_config[3])
        Input_keys[INPUT_GEAR2] = is_pressed;

    if (button == Input_pad_config[4])
        Input_keys[INPUT_START] = is_pressed;

    if (button == Input_pad_config[5])
        Input_keys[INPUT_COIN] = is_pressed;

    if (button == Input_pad_config[6])
        Input_keys[INPUT_MENU] = is_pressed;

    if (button == Input_pad_config[7])
        Input_keys[INPUT_VIEWPOINT] = is_pressed;
}
