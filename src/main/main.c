/***************************************************************************
    Cannonball Main Entry Point.
    
    Copyright Chris White.
    See license.txt for more details.
***************************************************************************/

// SDL Library
#if 1 /* #ifndef NOSDL */
#include <SDL/SDL.h>
#pragma comment(lib, "SDLmain.lib") // Replace main with SDL_main
#pragma comment(lib, "SDL.lib")
#pragma comment(lib, "glu32.lib")
// SDL Specific Code
#endif

#ifdef _AMIGA_
#ifdef NOCAMD
#include "amiga/Sound.h"
extern void *mod;
#ifdef NOSDL
#include "amiga/amigainput.h"
#include "amiga/amigatimer.h"
#include <intuition/intuition.h>
//#define P96ADAPTER_MAIN
#include "amiga/P96Screen.h"
extern struct P96SC_Handle*screen;
/*#include "amiga/P96SDLAdapter.h"*/
#endif
#endif
#endif

#include <stdio.h>
#ifndef NOSDL
#include "sdl/timer.h"
#include "sdl/input.h"
#endif
#include "Video.h"

#include "romloader.h"
#include "trackloader.h"
#include "stdint.h"
#include "main.h"
#include "setup.h"

#include "frontend/config.h"
#include "frontend/menu.h"

#include "cannonboard/interface.h"
#include "engine/oinputs.h"
#include "engine/ooutputs.h"
#include "engine/omusic.h"


// Initialize Shared Variables
int    cannonball_state       = STATE_BOOT;
double cannonball_frame_ms    = 0;
int    cannonball_frame       = 0;
Boolean   cannonball_tick_frame  = TRUE;
int    cannonball_fps_counter = 0;


extern  int kprintf (char *fmt, ... );

int kprintf (char *fmt, ... )
{
    return 0;
}

static void quit_func(int code)
{
#ifdef COMPILE_SOUND_CODE
    Audio_stop_audio();
#endif
#ifndef NOCAMD
    I_CAMD_StopSong();
    I_CAMD_ShutdownMusic();
#else
    SND_StopModule();
    SND_EjectModule(mod);
    mod = NULL;
#endif
    Input_close();
    Render_Quit();
#ifndef NOSDL
    SDL_Quit();
#endif
    exit(code);
}

static void process_events(void)
{
#ifdef NOSDL
  struct IntuiMessage     *imsg;
  int terminate = P96SC_OK;
  int code;

  if( !screen )
	return;

  while(imsg=(struct IntuiMessage *)GetMsg(screen->window->UserPort))
  {
	if( imsg->Class==IDCMP_CLOSEWINDOW )
                cannonball_state = STATE_QUIT;
	else
	{
		if( imsg->Class==IDCMP_RAWKEY )
		{
			code = imsg->Code;
			if( code&IECODE_UP_PREFIX )
			{
				code &= ~IECODE_UP_PREFIX;
				aInput_handle_key_up( code );
			}
			else
			{
				if( code == 69 ) /* ESCAPE */
					cannonball_state = STATE_QUIT;
				aInput_handle_key_down( code );
			}
		}
	}
	ReplyMsg( imsg );
  }
  aInput_handle_joy_axis( 0 ); /* Amiga code handles joystick ports internally */
#else
    SDL_Event event;

    // Grab all events from the queue.
    while(SDL_PollEvent(&event))
    {
        switch(event.type)
        {
            case SDL_KEYDOWN:
                // Handle key presses.
                if (event.key.keysym.sym == SDLK_ESCAPE)
                    cannonball_state = STATE_QUIT;
                else
                    Input_handle_key_down(&event.key.keysym);
                break;

            case SDL_KEYUP:
                Input_handle_key_up(&event.key.keysym);
                break;

            case SDL_JOYAXISMOTION:
                Input_handle_joy_axis(&event.jaxis);
                break;

            case SDL_JOYBUTTONDOWN:
                Input_handle_joy_down(&event.jbutton);
                break;

            case SDL_JOYBUTTONUP:
                Input_handle_joy_up(&event.jbutton);
                break;

            case SDL_QUIT:
                // Handle quit requests (like Ctrl-c).
                cannonball_state = STATE_QUIT;
                break;
        }
    }
#endif
}

// Pause Engine
Boolean pause_engine;

static void tick()
{
    Packet *packet;

    cannonball_frame++;

#if 0
	{
		//Printf("frame %d hires %d\n",frame,Config_video.hires);
		Printf("hiresT %ld (cbf %ld) (state %ld)\n",Config_video.hires,cannonball_frame,(long)cannonball_state);
		if( Config_video.hires == 0 )
		{
			cannonball_state = STATE_QUIT;
			return;
		}
		//break;
	}
#endif

#ifdef CANNONBOARD
    packet = Config_cannonboard.enabled ? Interface_get_packet() : NULL;
#else
    packet = 0;
#endif
    // Non standard FPS.
    // Determine whether to tick the current frame.
    if (Config_fps == 30)
        cannonball_tick_frame = 1;
    else if (Config_fps == 60)
        cannonball_tick_frame = cannonball_frame & 1;
    else if (Config_fps == 120)
        cannonball_tick_frame = (cannonball_frame & 3) == 1;


    process_events();


    if (cannonball_tick_frame)
        OInputs_tick(packet); // Do Controls
    if( Config_fps )		// this is actually a dummy compare forcing gcc2.95/m68k to emit different calling code
	OInputs_do_gear();        // Digital Gear


    switch (cannonball_state)
    {
        case STATE_GAME:
        {
		//printf("sG\n");
            if (Input_has_pressed(INPUT_TIMER))
                Outrun_freeze_timer = !Outrun_freeze_timer;

            if (Input_has_pressed(INPUT_PAUSE))
                pause_engine = !pause_engine;

            if (Input_has_pressed(INPUT_MENU))
                cannonball_state = STATE_INIT_MENU;

            if (!pause_engine || Input_has_pressed(INPUT_STEP))
            {
                Outrun_tick(packet, cannonball_tick_frame);
                Input_frame_done(); // Denote keys read

                #ifdef COMPILE_SOUND_CODE
                // Tick audio program code
                OSoundInt_tick();
                // Tick SDL Audio
                Audio_tick();
                #endif
            }
            else
            {                
                Input_frame_done(); // Denote keys read
            }
        }
        break;

        case STATE_INIT_GAME:
		//printf("sI\n");

            if (Config_engine.jap && !Roms_load_japanese_roms())
            {
                cannonball_state = STATE_QUIT;
            }
            else
            {
                pause_engine = FALSE;
                Outrun_init();
                cannonball_state = STATE_GAME;
            }
            break;

        case STATE_MENU:
        {
		//printf("sM\n");
            Menu_tick(packet);
            Input_frame_done();
            #ifdef COMPILE_SOUND_CODE
            // Tick audio program code
            OSoundInt_tick();
            // Tick SDL Audio
            Audio_tick();
            #endif
        }
        break;

        case STATE_INIT_MENU:
		//printf("sIM\n");
            OInputs_init();
            OOutputs_init();
            Menu_init();
            cannonball_state = STATE_MENU;
            break;
	default:
		break;
    }

    // Draw SDL Video
    Video_draw_frame();  
}

static void main_loop()
{
    // FPS Counter (If Enabled)
    Timer fps_count;
    Timer frame_time;
    int frame = 0;
    int t;
    double deltatime  = 0;
    int deltaintegral = 0;

    Timer_init(&fps_count,TIMER_COARSE);
    Timer_start(&fps_count);

    // General Frame Timing
    Timer_init(&frame_time,TIMER_MICRO);

    //printf("hiresE %ld confsz %ld\n",Config_video.hires,sizeof(Config_video));

    while (cannonball_state != STATE_QUIT)
    {
        Timer_start(&frame_time);
        tick();


        #ifdef COMPILE_SOUND_CODE
        deltatime += (cannonball_frame_ms * Audio_adjust_speed());
        #else
        deltatime += cannonball_frame_ms;
        #endif
        deltaintegral  = (int) deltatime;
        t = Timer_get_ticks(&frame_time);

        // Cap Frame Rate: Sleep Remaining Frame Time
       
       // Cap Frame Rate: Sleep Remaining Frame Time
        if (t < deltatime)
        {
            Timer_delay( &frame_time, (Uint32) (deltatime - t) );
        }
        
        deltatime -= deltaintegral;

        if (Config_video.fps_count)
        {
            frame++;
            // One second has elapsed
            if (Timer_get_ticks(&fps_count) >= 1000)
            {
                cannonball_fps_counter = frame;
                frame       = 0;
                Timer_start(&fps_count);
            }
        }
    }

    Timer_destroy(&frame_time);
    Timer_destroy(&fps_count);
    quit_func(0);
}

int main(int argc, char* argv[])
{
    Boolean loaded = FALSE;

#ifndef NOSDL
    // Initialize timer and video systems
    if( SDL_Init( SDL_INIT_TIMER | SDL_INIT_VIDEO | SDL_INIT_JOYSTICK) == -1 ) 
    { 
		fprintf(stderr, "SDL Initialization failed: %d\n", SDL_GetError());
        return 1; 
    }
#endif

    TrackLoader_Create();


    // Load LayOut File
    if (argc == 3 && strcmp(argv[1], "-file") == 0)
    {
        if (TrackLoader_set_layout_track(argv[2]))
        {
            loaded = Roms_load_revb_roms();
        }
    }
    // Load Roms Only
    else
    {
        loaded = Roms_load_revb_roms();
    }
    
    //TrackLoader_set_layout_track("d:/temp.bin");
    //loaded = Roms_load_revb_roms();

    if (loaded)
    {
        // Load XML Config
        Config_load(FILENAME_CONFIG);
#ifndef NOCAMD
        I_CAMD_InitMusic();
#endif
        // Load fixed PCM ROM based on config
        if (Config_sound.fix_samples)
            Roms_load_pcm_rom(TRUE);

        // Load patched widescreen tilemaps
        if (!OMusic_load_widescreen_map())
        {
            fprintf(stderr, "Unable to load widescreen tilemaps.\n");
        }

#ifndef NOSDL
        //Set the window caption 
        SDL_WM_SetCaption( "Cannonball", NULL ); 
#endif

        Video_Create();

        // Initialize SDL Video
	//printf("hiresA %ld\n",Config_video.hires);
        if (!Video_init(&Config_video))
            quit_func(1);
	//printf("hiresB %ld confsz %ld\n",Config_video.hires,sizeof(Config_video));

#ifdef COMPILE_SOUND_CODE
        Audio_init();
#endif
        cannonball_state = Config_menu.enabled ? STATE_INIT_MENU : STATE_INIT_GAME;

	//printf("hiresC %ld confsz %ld state %ld\n",Config_video.hires,sizeof(Config_video),(unsigned long)cannonball_state);

        // Initalize controls
        Input_init(Config_controls.pad_id,
                   Config_controls.keyconfig, Config_controls.padconfig, 
                   Config_controls.analog,    Config_controls.axis, Config_controls.asettings);

	//printf("hiresD %ld confsz %ld state %ld\n",Config_video.hires,sizeof(Config_video),(unsigned long)cannonball_state);

        main_loop();  // Loop until we quit the app
    }
    else
    {
        quit_func(1);
    }

    // Never Reached
    return 0;
}
