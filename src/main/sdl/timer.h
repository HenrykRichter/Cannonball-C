#ifndef _INC_SRC_MAIN_SDL_TIMER_H
#define _INC_SRC_MAIN_SDL_TIMER_H

#include "stdint.h"
#include <SDL/SDL.h>

typedef struct
{
    //The clock time when the timer started
    int startTicks;

    //The ticks stored when the timer was paused
    int pausedTicks;

    //The timer status
    Boolean paused;
    Boolean started;
} Timer;

#define TIMER_COARSE 1
#define TIMER_MICRO  2
#define TIMER_NANO   3

//The various clock actions
void Timer_init(Timer* timer,Uint32 granularity);
void Timer_destroy(Timer*timer);

void Timer_start(Timer* timer);
void Timer_stop(Timer* timer);
void Timer_pause(Timer* timer);
void Timer_unpause(Timer* timer);
void Timer_delay(Timer*timer,Uint32 ms); 
//Gets the timer's time
int Timer_get_ticks(Timer* timer);

//Checks the status of the timer
Boolean Timer_is_started(Timer* timer);
Boolean Timer_is_paused(Timer* timer);
#endif
