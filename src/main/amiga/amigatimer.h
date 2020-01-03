#ifndef _INC_SRC_MAIN_SDL_TIMER_H
#define _INC_SRC_MAIN_SDL_TIMER_H

#include "stdint.h"
#include <exec/devices.h>
#include <devices/timer.h>

typedef struct
{
    //The clock time when the timer started
    struct timeval startTime;

    //The ticks stored when the timer was paused
    struct timeval pauseTime;

    //The timer status
    Boolean paused;
    Boolean started;

    struct MsgPort *port;
    struct timerequest treq;
    ULONG  initflag;
} Timer;

#define TIMER_COARSE 1
#define TIMER_MICRO  2
#define TIMER_NANO   3


//The various clock actions
void Timer_init(Timer* timer,ULONG granularity);
void Timer_destroy( Timer *timer );

void Timer_start(Timer* timer);
void Timer_stop(Timer* timer);
void Timer_pause(Timer* timer);
void Timer_unpause(Timer* timer);

void Timer_delay(Timer *timer, ULONG delay );


//Gets the timer's time
int Timer_get_ticks(Timer* timer);

//Checks the status of the timer
Boolean Timer_is_started(Timer* timer);
Boolean Timer_is_paused(Timer* timer);
#endif
