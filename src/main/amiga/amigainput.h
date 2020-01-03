/***************************************************************************
    amiga Input Handling.

    Henryk iIchter

***************************************************************************/
#ifndef _INC_A_INPUT_H
#define _INC_A_INPUT_H

#include "stdint.h"

enum presses
{
    INPUT_LEFT  = 0,
    INPUT_RIGHT = 1,
    INPUT_UP    = 2,
    INPUT_DOWN  = 3,
    INPUT_ACCEL = 4,
    INPUT_BRAKE = 5,
    INPUT_GEAR1 = 6,
    INPUT_GEAR2 = 7,
    INPUT_START = 8,
    INPUT_COIN  = 9,
    INPUT_VIEWPOINT = 10,        
    INPUT_PAUSE = 11,
    INPUT_STEP  = 12,
    INPUT_TIMER = 13,
    INPUT_MENU = 14,     
};

extern Boolean Input_keys[15];
extern Boolean Input_keys_old[15];

// Has gamepad been found?
extern const Boolean Input_gamepad;

// Use analog controls (constant 0 for Amiga port)
extern const int Input_analog;

// Latch last key press for redefines
extern int Input_key_press;

// Latch last joystick button press for redefines
extern int16_t Input_joy_button;

// Analog Controls
extern const int Input_a_wheel;
extern const int Input_a_accel;
extern const int Input_a_brake;

void Input_init(int, int*, int*, int, int*, int*);
void Input_close();
void Input_frame_done();
void aInput_handle_key_up(int);
void aInput_handle_key_down(int);
void aInput_handle_joy_axis(int); /* handles joystick axes and buttons */

Boolean Input_is_pressed(enum presses p);
Boolean Input_is_pressed_clear(enum presses p);
Boolean Input_has_pressed(enum presses p);

#endif
