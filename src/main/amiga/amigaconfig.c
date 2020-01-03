/***************************************************************************
    Key-Value Configuration File Handling.

    Load Settings.
    Load & Save Hi-Scores.

    Copyright Henryk Richter (cloned and modified from Chris White's code)
    See license.txt for more details.
***************************************************************************/

#include "main.h"
#include "frontend/config.h"
#include "globals.h"
#include "setup.h"
#include "utils.h"
#include "engine/ohiscore.h"
#include "engine/audio/OSoundInt.h"
#include "keyvalconfig.h"
#include <stdio.h>
#include <proto/utility.h>

menu_settings_t        Config_menu;
video_settings_t       Config_video;
sound_settings_t       Config_sound;
controls_settings_t    Config_controls;
engine_settings_t      Config_engine;
ttrial_settings_t      Config_ttrial;
cannonboard_settings_t Config_cannonboard;
uint16_t Config_s16_width;
uint16_t Config_s16_height;
uint16_t Config_s16_x_off;
int Config_fps;
int Config_tick_fps;
int Config_cont_traffic;

void Config_init()
{
    
    // ------------------------------------------------------------------------
    // Menu Settings
    // ------------------------------------------------------------------------

    Config_menu.enabled           = 0;
    Config_menu.road_scroll_speed = 50;

    // ------------------------------------------------------------------------
    // Video Settings
    // ------------------------------------------------------------------------

    Config_video.mode       = 1;
    Config_video.scale      = 0;
    Config_video.scanlines  = 0;
    Config_video.fps        = 0;
    Config_video.fps_count  = 1;
    Config_video.widescreen = 0;
    Config_video.hires      = 0;
    Config_video.filtering  = 0;
    Config_video.detailLevel =2; 

    Config_set_fps(Config_video.fps);

    // ------------------------------------------------------------------------
    // Sound Settings
    // ------------------------------------------------------------------------
    Config_sound.enabled     = 0;
    Config_sound.advertise   = 0;
    Config_sound.preview     = 1;
    Config_sound.fix_samples = 0;
    Config_sound.amiga_mods  = 0;

    Config_controls.gear          = 0;
    Config_controls.steer_speed   = 3;
    Config_controls.pedal_speed   = 4;
    Config_controls.keyconfig[0]  = 273;
    Config_controls.keyconfig[1]  = 274;
    Config_controls.keyconfig[2]  = 276;
    Config_controls.keyconfig[3]  = 275;
    Config_controls.keyconfig[4]  = 122;
    Config_controls.keyconfig[5]  = 120;
    Config_controls.keyconfig[6]  = 32;
    Config_controls.keyconfig[7]  = 32;
    Config_controls.keyconfig[8]  = 49;
    Config_controls.keyconfig[9]  = 53;
    Config_controls.keyconfig[10] = 286;
    Config_controls.keyconfig[11] = 304;
    Config_controls.padconfig[0]  = 0;
    Config_controls.padconfig[1]  = 1;
    Config_controls.padconfig[2]  = 2;
    Config_controls.padconfig[3]  = 2;
    Config_controls.padconfig[4]  = 3;
    Config_controls.padconfig[5]  = 4;
    Config_controls.padconfig[6]  = 5;
    Config_controls.padconfig[7]  = 6;
    Config_controls.analog        = 0;
    Config_controls.pad_id        = 0;
    Config_controls.axis[0]       = 0;
    Config_controls.axis[1]       = 2;
    Config_controls.axis[2]       = 3;
    Config_controls.asettings[0]  = 75;
    Config_controls.asettings[1]  = 0;
    Config_controls.asettings[2]  = 0;
    
 

    // ------------------------------------------------------------------------
    // CannonBoard Settings
    // ------------------------------------------------------------------------
    Config_cannonboard.enabled = 0;
    
    Config_cannonboard.baud    = 1212;
    Config_cannonboard.debug   = 0;
    Config_cannonboard.cabinet = 0;

 

    // ------------------------------------------------------------------------
    // Engine Settings
    // ------------------------------------------------------------------------

    Config_engine.dip_time      = 0;
    Config_engine.dip_traffic   = 1;
    
    Config_engine.freeze_timer    = Config_engine.dip_time == 4;
    Config_engine.disable_traffic = Config_engine.dip_traffic == 4;
    Config_engine.dip_time    &= 3;
    Config_engine.dip_traffic &= 3;

    Config_engine.freeplay      = 0;
    Config_engine.jap           = 0;
    Config_engine.prototype     = 0;
    
    // Additional Level Objects
    Config_engine.level_objects   = 0;
    Config_engine.randomgen       = 0;
    Config_engine.fix_bugs_backup = 0;
    Config_engine.fix_bugs        = 0;
    Config_engine.fix_timer       = 0;
    Config_engine.layout_debug    = 0;
    Config_engine.new_attract     = 0;

    // ------------------------------------------------------------------------
    // Time Trial Mode
    // ------------------------------------------------------------------------

    Config_ttrial.laps    = 5;
    Config_ttrial.traffic = 3;

    Config_cont_traffic   = 3;

}

void Config_load(const char* filename)
{
    int i;
    struct TagItem *conf,*curdev,*tmp,*curtag;

    Config_init();

    conf = config_parse( (char*)filename ,(0));
    if( !conf )
    {
    	fprintf(stderr, "Error: Couldn't load %s\n", filename);
    	return;
    }
   
    while( (curdev = NextTagItem(&conf)) )
    {
	if( curdev->ti_Tag == CVAR_SECTIONHEADER )
	{
		curdev  = (struct TagItem*)curdev->ti_Data;
		while( (curtag = NextTagItem(&curdev)) )
		{
//			if( (ULONG)curtag->ti_Data > 200 )
//				printf("%lx dat %ld\n",(ULONG)curtag->ti_Tag,(ULONG)curtag->ti_Data);
			switch( curtag->ti_Tag )
			{
				case CVAR_VIDMODE:      Config_video.mode      = curtag->ti_Data;  break; 
				case CVAR_VIDSCALE:	Config_video.scale     = curtag->ti_Data;  break;   
				case CVAR_VIDSCANLINES: Config_video.scanlines = curtag->ti_Data;  break; 
				case CVAR_VIDFPS:       Config_video.fps        = curtag->ti_Data; break; 
				case CVAR_VIDWIDE:      Config_video.widescreen = curtag->ti_Data; break; 
				case CVAR_VIDHIRES:     Config_video.hires      = curtag->ti_Data; break; 
				case CVAR_SNDENABLE:     Config_sound.enabled     = curtag->ti_Data; break;
				case CVAR_SNDADV:        Config_sound.advertise   = curtag->ti_Data; break;
				case CVAR_SNDPREVIEW:    Config_sound.preview     = curtag->ti_Data; break;
				case CVAR_SNDFIXSAMPLES: Config_sound.fix_samples = curtag->ti_Data; break;
				case CVAR_SNDMODS:       Config_sound.amiga_mods  = curtag->ti_Data; break;
				case CVAR_ENGTIME:       Config_engine.dip_time   = curtag->ti_Data&3;
				                         Config_engine.freeze_timer = (curtag->ti_Data==4); break;
				case CVAR_ENGTRAFFIC:    Config_engine.dip_traffic = curtag->ti_Data&3;
							 Config_engine.disable_traffic = (curtag->ti_Data==4);break;
				case CVAR_ENGJAPTRACKS:  Config_engine.jap           = curtag->ti_Data; break;
				case CVAR_ENGPROTOTYPE:  Config_engine.prototype     = curtag->ti_Data; break;
				case CVAR_ENGLEVELOBJS:  Config_engine.level_objects = curtag->ti_Data; break;
				case CVAR_ENGNEWATTRACT: Config_engine.new_attract   = curtag->ti_Data; break;
				case CVAR_TTRLAPS:       Config_ttrial.laps     = curtag->ti_Data; break;
				case CVAR_TTRTRAFFIC:    Config_ttrial.traffic  = curtag->ti_Data; break;
				case CVAR_CONGEAR:       Config_controls.gear    = curtag->ti_Data; break;
				case CVAR_CONSTEERSPEED: Config_controls.steer_speed = curtag->ti_Data; break;
				case CVAR_CONPEDALSPEED: Config_controls.pedal_speed = curtag->ti_Data; break;
				case CVAR_CONKEYUP:      Config_controls.keyconfig[0]= curtag->ti_Data; break;
				case CVAR_CONKEYDOWN:    Config_controls.keyconfig[1]= curtag->ti_Data; break;
				case CVAR_CONKEYLEFT:    Config_controls.keyconfig[2]= curtag->ti_Data; break;  
				case CVAR_CONKEYRIGHT:   Config_controls.keyconfig[3]= curtag->ti_Data; break;
				case CVAR_CONKEYACC:     Config_controls.keyconfig[4]= curtag->ti_Data; break;
				case CVAR_CONKEBRAKE:    Config_controls.keyconfig[5]= curtag->ti_Data; break;
				case CVAR_CONKEYGEAR1:   Config_controls.keyconfig[6]= curtag->ti_Data; break;
				case CVAR_CONKEYGEAR2:   Config_controls.keyconfig[7]= curtag->ti_Data; break;
				case CVAR_CONKEYSTART:   Config_controls.keyconfig[8]= curtag->ti_Data; break;
				case CVAR_CONKEYCOIN:    Config_controls.keyconfig[9]= curtag->ti_Data; break;
				case CVAR_CONKEYMENU:    Config_controls.keyconfig[10]= curtag->ti_Data; break;
				case CVAR_CONKEYVIEW:    Config_controls.keyconfig[11]= curtag->ti_Data; break;
				case CVAR_CONPADACC:     Config_controls.padconfig[0]= curtag->ti_Data; break;
				case CVAR_CONPADBRAKE:   Config_controls.padconfig[1]= curtag->ti_Data; break;
				case CVAR_CONPADGEAR1:   Config_controls.padconfig[2]= curtag->ti_Data; break;
				case CVAR_CONPADGEAR2:   Config_controls.padconfig[3]= curtag->ti_Data; break;
				case CVAR_CONPADSTART:   Config_controls.padconfig[4]= curtag->ti_Data; break;
				case CVAR_CONPADCOIN:    Config_controls.padconfig[5]= curtag->ti_Data; break;
				case CVAR_CONPADMENU:    Config_controls.padconfig[6]= curtag->ti_Data; break;
				case CVAR_CONPADVIEW:    Config_controls.padconfig[7]= curtag->ti_Data; break;
				case I2SEN_DEVICE: break;
				default:
					fprintf(stderr,"unhandled config tag %ld %lx\n",curtag->ti_Tag,curtag->ti_Tag);
					break;
			}
		}
	}
    }

    config_free( conf );
    return; 
}

Boolean config_save_string( FILE *f, char *str )
{
	int len = strlen( str );

	if( len != fwrite( str, 1, len, f ) )
		return FALSE;

	return TRUE;
}

Boolean config_save_int_bytag( FILE *f, ULONG tag, LONG val )
{
	char *name;

	if( !config_stringfortag( tag, &name ) ) /* tag found ? */
		return FALSE;

	fprintf(f,"%s = %d\n",name,val);

	return TRUE;
}

Boolean config_save_hex_bytag( FILE *f, ULONG tag, unsigned char *str, LONG length )
{
	char *name;

	if( !config_stringfortag( tag, &name ) ) /* tag found ? */
		return FALSE;

	fprintf(f,"%s = 0x",name);

	while( length-- )
	{
		fprintf(f,"%02lx",(ULONG)*str++);
	}

	fprintf(f,"\n");

	return TRUE;
}


Boolean Config_save(const char* filename)
{
  Boolean ret = FALSE;
  FILE *sfile = fopen( filename, "wb" );

  if( !sfile )
	return ret;

  do
  {
	if( !config_save_string(     sfile, "# Cannonball-C-Amiga configuration file\n# \n[GLOBAL]\n" ) ) break;

	if( !config_save_string(     sfile, "# video\n" )) break;
	if( !config_save_int_bytag(  sfile, CVAR_VIDMODE     , Config_video.mode      ))  break;
	if( !config_save_int_bytag(  sfile, CVAR_VIDSCALE    , Config_video.scale     ))  break;
	if( !config_save_int_bytag(  sfile, CVAR_VIDSCANLINES, Config_video.scanlines ))  break;
	if( !config_save_int_bytag(  sfile, CVAR_VIDFPS      , Config_video.fps        )) break;
	if( !config_save_int_bytag(  sfile, CVAR_VIDWIDE     , Config_video.widescreen )) break;
	if( !config_save_int_bytag(  sfile, CVAR_VIDHIRES    , Config_video.hires      )) break;

	if( !config_save_string(     sfile, "# audio\n" )) break;
	if( !config_save_int_bytag(  sfile, CVAR_SNDENABLE    , Config_sound.enabled     )) break;
	if( !config_save_int_bytag(  sfile, CVAR_SNDADV       , Config_sound.advertise   )) break;
	if( !config_save_int_bytag(  sfile, CVAR_SNDPREVIEW   , Config_sound.preview     )) break;
	if( !config_save_int_bytag(  sfile, CVAR_SNDFIXSAMPLES, Config_sound.fix_samples )) break;
	if( !config_save_int_bytag(  sfile, CVAR_SNDMODS,       Config_sound.amiga_mods  )) break;

	if( !config_save_string(     sfile, "# engine\n" )) break;
	if( !config_save_int_bytag(  sfile, CVAR_ENGTIME      , Config_engine.freeze_timer ? 4 : Config_engine.dip_time       )) break;
	if( !config_save_int_bytag(  sfile, CVAR_ENGTRAFFIC   , Config_engine.disable_traffic ? 4 : Config_engine.dip_traffic )) break;
	if( !config_save_int_bytag(  sfile, CVAR_ENGJAPTRACKS , Config_engine.jap                                             )) break;
	if( !config_save_int_bytag(  sfile, CVAR_ENGPROTOTYPE , Config_engine.prototype                                       )) break;
	if( !config_save_int_bytag(  sfile, CVAR_ENGLEVELOBJS , Config_engine.level_objects                                   )) break;
	if( !config_save_int_bytag(  sfile, CVAR_ENGNEWATTRACT, Config_engine.new_attract                                     )) break;

	if( !config_save_string(     sfile, "# time trial settings\n" )) break;
	if( !config_save_int_bytag(  sfile, CVAR_TTRLAPS   ,  Config_ttrial.laps     )) break;
	if( !config_save_int_bytag(  sfile, CVAR_TTRTRAFFIC,  Config_ttrial.traffic  )) break;

	if( !config_save_string(     sfile, "# controls\n" )) break;
	if( !config_save_int_bytag(  sfile, CVAR_CONGEAR      , Config_controls.gear    )) break;
	if( !config_save_int_bytag(  sfile, CVAR_CONSTEERSPEED, Config_controls.steer_speed )) break;
	if( !config_save_int_bytag(  sfile, CVAR_CONPEDALSPEED, Config_controls.pedal_speed )) break;

	if( !config_save_string(     sfile, "# keyboard configuration options\n")) break;
	if( !config_save_int_bytag(  sfile, CVAR_CONKEYUP   , Config_controls.keyconfig[0])) break;
	if( !config_save_int_bytag(  sfile, CVAR_CONKEYDOWN , Config_controls.keyconfig[1])) break;
	if( !config_save_int_bytag(  sfile, CVAR_CONKEYLEFT , Config_controls.keyconfig[2])) break;
	if( !config_save_int_bytag(  sfile, CVAR_CONKEYRIGHT, Config_controls.keyconfig[3])) break;
	if( !config_save_int_bytag(  sfile, CVAR_CONKEYACC  , Config_controls.keyconfig[4])) break;
	if( !config_save_int_bytag(  sfile, CVAR_CONKEBRAKE , Config_controls.keyconfig[5])) break;
	if( !config_save_int_bytag(  sfile, CVAR_CONKEYGEAR1, Config_controls.keyconfig[6])) break;
	if( !config_save_int_bytag(  sfile, CVAR_CONKEYGEAR2, Config_controls.keyconfig[7])) break;
	if( !config_save_int_bytag(  sfile, CVAR_CONKEYSTART, Config_controls.keyconfig[8])) break;
	if( !config_save_int_bytag(  sfile, CVAR_CONKEYCOIN , Config_controls.keyconfig[9])) break;
	if( !config_save_int_bytag(  sfile, CVAR_CONKEYMENU , Config_controls.keyconfig[10])) break;
	if( !config_save_int_bytag(  sfile, CVAR_CONKEYVIEW , Config_controls.keyconfig[11])) break;

	if( !config_save_string(     sfile, "# pad/joystick configuration options\n")) break;
	if( !config_save_int_bytag(  sfile, CVAR_CONPADACC  ,Config_controls.padconfig[0])) break;
	if( !config_save_int_bytag(  sfile, CVAR_CONPADBRAKE,Config_controls.padconfig[1])) break;
	if( !config_save_int_bytag(  sfile, CVAR_CONPADGEAR1,Config_controls.padconfig[2])) break;
	if( !config_save_int_bytag(  sfile, CVAR_CONPADGEAR2,Config_controls.padconfig[3])) break;
	if( !config_save_int_bytag(  sfile, CVAR_CONPADSTART,Config_controls.padconfig[4])) break;
	if( !config_save_int_bytag(  sfile, CVAR_CONPADCOIN ,Config_controls.padconfig[5])) break;
	if( !config_save_int_bytag(  sfile, CVAR_CONPADMENU ,Config_controls.padconfig[6])) break;
	if( !config_save_int_bytag(  sfile, CVAR_CONPADVIEW ,Config_controls.padconfig[7])) break;

	/* TODO continuous mode traffic */
        /* Config_cont_traffic */

	ret = TRUE;
  }
  while(0);

  fclose(sfile);
  return ret;
}


void Config_load_scores(const char* filename)
{
    ULONG i;
    struct TagItem *conf,*curdev,*tmp,*curtag;
    score_entry* e;


    conf = config_parse( (char*)filename ,(0));
    if( !conf )
    {
    	fprintf(stderr, "Error: Couldn't load %s\n", filename);
    	return;
    }
   
    while( (curdev = NextTagItem(&conf)) )
    {
	if( curdev->ti_Tag == CVAR_SECTIONHEADER )
	{
		curdev  = (struct TagItem*)curdev->ti_Data;
		while( (curtag = NextTagItem(&curdev)) )
		{
			if( (curtag->ti_Tag>= CVAR_HI0) && (curtag->ti_Tag<=CVAR_HI19))
			{
				i = curtag->ti_Tag-CVAR_HI0;
				e = &OHiScore_scores[i];
				CopyMem( 1+curtag->ti_Data ,e,sizeof(score_entry) );
				/* printf("score %ld %c%c%c %ld\n",e->score,e->initial1,e->initial2,e->initial3,e->time); */
			}
		}
	}
    }

    config_free( conf );
}

void Config_save_scores(const char* filename)
{
  FILE *sfile = fopen( filename, "wb" );
  int i,num;
  ULONG tg;

  if( !sfile )
	return;

  num = (HISCORE_NUM_SCORES <= (CVAR_HI19-CVAR_HI0) ) ? HISCORE_NUM_SCORES : (CVAR_HI19-CVAR_HI0);

  config_save_string(     sfile, "# Cannonball-C-Amiga hiscores file\n# \n[SCORES]\n" ); 

  tg = CVAR_HI0;
  for (i = 0; i < num; i++,tg++)
  {
        score_entry* e = &OHiScore_scores[i];

	config_save_hex_bytag( sfile, tg, (unsigned char*)e, sizeof( score_entry ) );
  }

  fclose( sfile );
  return;
}

void Config_load_tiletrial_scores()
{
#if 0
    int i;
    const char* filename = Config_engine.jap ? FILENAME_TTRIAL_JAPAN : FILENAME_TTRIAL;

    // Counter value that represents 1m 15s 0ms
    const uint16_t COUNTER_1M_15 = 0x11D0;

    XMLDoc doc;
    if (!XMLDoc_init(&doc) || !XMLDoc_parse_file_DOM(filename, &doc))
    {
        for (i = 0; i < 15; i++)
            Config_ttrial.best_times[i] = COUNTER_1M_15;

        fprintf(stderr, "Error: problem loading %s for loading\n", filename);

        return;
    }

    {
     char basexmltag[64] = "/time_trial/score";
     char childtag[64];

     // Time Trial Scores
     for (i = 0; i < 15; i++)
     {
        strcpy(childtag, basexmltag); strcat(childtag, Utils_int_to_string(i));
        Config_ttrial.best_times[i] = GetXMLDocValueInt(&doc, childtag, COUNTER_1M_15);
     }
    }
#endif
}

void Config_save_tiletrial_scores()
{
#if 0
    int i;
    XMLDoc saveDoc;
    char scoreXmlTag[64];

    XMLDoc_init(&saveDoc);
    AddXmlHeader(&saveDoc);
    AddXmlFatherNode(&saveDoc, "time_trial");


    for (i = 0; i < 15; i++)
    {
        strcpy(scoreXmlTag, "score");
        strcat(scoreXmlTag, Utils_int_to_string(i));

        AddXmlChildNodeRootString(&saveDoc, scoreXmlTag, Utils_int_to_string(Config_ttrial.best_times[i]));
    }

    {
     const char* filename = Config_engine.jap ? FILENAME_TTRIAL_JAPAN : FILENAME_TTRIAL;

     FILE* file = fopen(filename, "w");

     if (!file)
     {
        fprintf(stderr, "Error: can't open %s for save\n", filename);
        return;
     }

     XMLDoc_print(&saveDoc, file, "\n", "\t", FALSE, 0, 4);
     fclose(file);
    }
    XMLDoc_free(&saveDoc);
#endif
}

Boolean Config_clear_scores()
{
    int clear = 0;

    // Init Default Hiscores
    OHiScore_init_def_scores();

    // Remove XML files if they exist
    clear += remove(FILENAME_SCORES);
    clear += remove(FILENAME_SCORES_JAPAN);
    clear += remove(FILENAME_TTRIAL);
    clear += remove(FILENAME_TTRIAL_JAPAN);
    clear += remove(FILENAME_CONT);
    clear += remove(FILENAME_CONT_JAPAN);

    // remove returns 0 on success
    return clear == 6;
}

void Config_set_fps(int fps)
{
    Config_video.fps = fps;
    // Set core FPS to 30fps or 60fps
    Config_fps = Config_video.fps == 0 ? 30 : 60;
    
    // Original game ticks sprites at 30fps but background scroll at 60fps
    Config_tick_fps  = Config_video.fps < 2 ? 30 : 60;

    cannonball_frame_ms = 1000.0 / Config_fps;

    #ifdef COMPILE_SOUND_CODE
    if (Config_sound.enabled)
        Audio_stop_audio();
    OSoundInt_init();
    if (Config_sound.enabled)
        Audio_start_audio();
    #endif
}
