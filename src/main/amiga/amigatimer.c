#include "amiga/amigatimer.h"
#include <proto/exec.h>
#include <proto/timer.h>

/***************************************************************************
    Amiga timer.device based Timer.

    (C)2019 Henryk Richter

    See license.txt for more details.
***************************************************************************/
void Timer_destroy(Timer *timer )
{
 if( timer->initflag != 0xDEADBEEF )
 	return;
 timer->initflag = 0;

 CloseDevice(&timer->treq.tr_node);

 if( timer->port )
 	DeleteMsgPort(timer->port);
 timer->port = (0);

 /* printf("destroyed timer %lx\n",(ULONG)timer); */
}

void Timer_init(Timer* timer, ULONG granularity)
{
    ULONG unit;

    /* printf("init timer %lx\n",(ULONG)timer); */

    if( timer->initflag == 0xDEADBEEF )
    {
	Timer_destroy( timer );
    }

    //Initialize the variables
    timer->startTime.tv_secs = 0;
    timer->startTime.tv_micro = 0;
    timer->pauseTime.tv_secs = 0;
    timer->pauseTime.tv_micro = 0;

    timer->paused = FALSE;
    timer->started = FALSE;

    timer->port = CreateMsgPort();
    if( timer->port )
    {
    	unit = UNIT_MICROHZ;
    	if( granularity == TIMER_COARSE )
		unit = UNIT_VBLANK;

    	timer->treq.tr_node.io_Message.mn_Node.ln_Type=NT_REPLYMSG;
   	timer->treq.tr_node.io_Message.mn_ReplyPort=timer->port;
	if (!(OpenDevice(TIMERNAME,unit,&timer->treq.tr_node,0)))
	{
	   	timer->initflag = 0xDEADBEEF;
	}
	else	
	{
		DeleteMsgPort( timer->port );
		timer->port = (0);
	}
    }
}

void Timer_start(Timer* timer)
{
    struct Library *TimerBase = (struct Library*)timer->treq.tr_node.io_Device;

    //Start the timer
    timer->started = TRUE;

    //Unpause the timer
    timer->paused = FALSE;

    //Get the current clock time
    GetSysTime( &timer->startTime );
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
    struct Library *TimerBase = (struct Library*)timer->treq.tr_node.io_Device;

    //If the timer is running and isn't already paused
    if( ( timer->started == TRUE ) && ( timer->paused == FALSE ) )
    {
        struct timeval curtime;

        //Pause the timer
        timer->paused = TRUE;

        //Calculate the paused ticks
	GetSysTime( &timer->pauseTime );
	SubTime( &timer->pauseTime, &timer->startTime );
    }
}

void Timer_unpause(Timer* timer)
{
    struct Library *TimerBase = (struct Library*)timer->treq.tr_node.io_Device;

    //If the timer is paused
    if( timer->paused == TRUE )
    {
        //Unpause the timer
        timer->paused = FALSE;

        //Reset the starting ticks
	GetSysTime( &timer->startTime );
	SubTime( &timer->startTime, &timer->pauseTime );

        //Reset the paused ticks
        timer->pauseTime.tv_secs = 0;
	timer->pauseTime.tv_micro= 0;
    }
}

int Timer_get_ticks(Timer* timer)
{
    struct Library *TimerBase = (struct Library*)timer->treq.tr_node.io_Device;

    //If the timer is running
    if( timer->started == TRUE )
    {
        //If the timer is paused
        if( timer->paused == TRUE )
        {
            //Return the number of ticks when the timer was paused
		return ((timer->pauseTime.tv_secs*1000) + (timer->pauseTime.tv_micro/1000));
        }
        else
        {
            //Return the current time minus the start time
	    struct timeval curtime;
	    GetSysTime( &curtime );
	    SubTime( &curtime, &timer->startTime );
	    return ((curtime.tv_secs*1000)+(curtime.tv_micro/1000));
        }
    }

    //If the timer isn't running
    return 0;
}

/* delay in milliseconds */
void Timer_delay( Timer* timer, ULONG delay )
{
   long secs,micro;

   secs = 0;
   if( delay > 1000 ) /* >1s ? */
   {
	secs = delay / 1000;
	delay= delay % 1000;
   }
   micro = delay*1000; /* ms to usecs */

   timer->treq.tr_node.io_Command=TR_ADDREQUEST;
   timer->treq.tr_time.tv_secs  =secs;
   timer->treq.tr_time.tv_micro=micro;
   DoIO( &timer->treq.tr_node );
   /* if needed, timer->treq.tr_time will contain current system time */
}


Boolean Timer_is_started(Timer* timer)
{
    return timer->started;
}

Boolean Timer_is_paused(Timer* timer)
{
    return timer->paused;
}


