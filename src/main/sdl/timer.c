#include "sdl/timer.h"

/***************************************************************************
    SDL Based Timer.
    
    Will need to be replaced if SDL library is replaced.

    Copyright Chris White.
    See license.txt for more details.
***************************************************************************/

void Timer_init(Timer* timer,Uint32 granularity)
{
    //Initialize the variables
    timer->startTicks = 0;
    timer->pausedTicks = 0;
    timer->paused = FALSE;
    timer->started = FALSE;
}

void Timer_destroy(Timer*timer)
{}


void Timer_start(Timer* timer)
{
    //Start the timer
    timer->started = TRUE;

    //Unpause the timer
    timer->paused = FALSE;

    //Get the current clock time
    timer->startTicks = SDL_GetTicks();
}

void Timer_stop(Timer* timer)
{
    //Stop the timer
    timer->started = FALSE;

    //Unpause the timer
    timer->paused = FALSE;
}



void Timer_pause(Timer* timer)
{
    //If the timer is running and isn't already paused
    if( ( timer->started == TRUE ) && ( timer->paused == FALSE ) )
    {
        //Pause the timer
        timer->paused = TRUE;

        //Calculate the paused ticks
        timer->pausedTicks = SDL_GetTicks() - timer->startTicks;
    }
}

void Timer_unpause(Timer* timer)
{
    //If the timer is paused
    if( timer->paused == TRUE )
    {
        //Unpause the timer
        timer->paused = FALSE;

        //Reset the starting ticks
        timer->startTicks = SDL_GetTicks() - timer->pausedTicks;

        //Reset the paused ticks
        timer->pausedTicks = 0;
    }
}

int Timer_get_ticks(Timer* timer)
{
    //If the timer is running
    if( timer->started == TRUE )
    {
        //If the timer is paused
        if( timer->paused == TRUE )
        {
            //Return the number of ticks when the timer was paused
            return timer->pausedTicks;
        }
        else
        {
            //Return the current time minus the start time
            return SDL_GetTicks() - timer->startTicks;
        }
    }

    //If the timer isn't running
    return 0;
}

void Timer_delay( Timer *timer, Uint32 delay_ms )
{
	SDL_Delay(delay_ms);
}

Boolean Timer_is_started(Timer* timer)
{
    return timer->started;
}

Boolean Timer_is_paused(Timer* timer)
{
    return timer->paused;
}
