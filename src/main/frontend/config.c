/***************************************************************************
    XML Configuration File Handling.

    Load Settings.
    Load & Save Hi-Scores.

    Copyright Chris White.
    See license.txt for more details.
***************************************************************************/


#include "main.h"
#include "config.h"
#include "globals.h"
#include "setup.h"
#include "utils.h"
#include "xmlutils.h"
#include "engine/ohiscore.h"
#include "engine/audio/OSoundInt.h"


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
    XMLDoc doc;

    Config_init();

    return; /* FIXME: could not get XML stuff to work (crashes) */

    if (!XMLDoc_init(&doc) || !XMLDoc_parse_file_DOM(filename, &doc))
    {
        fprintf(stderr, "Error: Couldn't load %s\n", filename);
        return;
    }

#ifdef _AMIGA_
    fprintf(stderr,"after config read\n");
#if 1 
    Config_video.fps        = GetXMLDocValueInt(&doc, "/video/fps",                0); // Default is 60 fps
    Config_video.widescreen = GetXMLDocValueInt(&doc, "/video/widescreen",         0); // Enable Widescreen Mode
    Config_video.hires      = GetXMLDocValueInt(&doc, "/video/hires",              0); // Hi-Resolution Mode
    Config_video.scanlines  = GetXMLDocValueInt(&doc, "/video/scanlines",          0); // Scanlines

    Config_set_fps(Config_video.fps);
    Config_video.fps_count  = GetXMLDocValueInt(&doc, "/video/fps_counter",        0); // FPS Counter
#endif

#else
    // ------------------------------------------------------------------------
    // Menu Settings
    // ------------------------------------------------------------------------

    Config_menu.enabled           = GetXMLDocValueInt(&doc, "/menu/enabled",   1);
    Config_menu.road_scroll_speed = GetXMLDocValueInt(&doc, "/menu/roadspeed", 50);

    // ------------------------------------------------------------------------
    // Video Settings
    // ------------------------------------------------------------------------
   
    Config_video.mode       = GetXMLDocValueInt(&doc, "/video/mode",               0); // Video Mode: Default is Windowed 
    Config_video.scale      = GetXMLDocValueInt(&doc, "/video/window/scale",       2); // Video Scale: Default is 2x    
    Config_video.scanlines  = GetXMLDocValueInt(&doc, "/video/scanlines",          0); // Scanlines
    Config_video.fps        = GetXMLDocValueInt(&doc, "/video/fps",                2); // Default is 60 fps
    Config_video.fps_count  = GetXMLDocValueInt(&doc, "/video/fps_counter",        0); // FPS Counter
    Config_video.widescreen = GetXMLDocValueInt(&doc, "/video/widescreen",         1); // Enable Widescreen Mode
    Config_video.hires      = GetXMLDocValueInt(&doc, "/video/hires",              0); // Hi-Resolution Mode
    Config_video.filtering  = GetXMLDocValueInt(&doc, "/video/filtering",          0); // Open GL Filtering Mode
          
    Config_set_fps(Config_video.fps);

    // ------------------------------------------------------------------------
    // Sound Settings
    // ------------------------------------------------------------------------
    Config_sound.enabled     = GetXMLDocValueInt(&doc, "/sound/enable",      1);
    Config_sound.advertise   = GetXMLDocValueInt(&doc, "/sound/advertise",   1);
    Config_sound.preview     = GetXMLDocValueInt(&doc, "/sound/preview",     1);
    Config_sound.fix_samples = GetXMLDocValueInt(&doc, "/sound/fix_samples", 1);


    // Custom Music
    for (i = 0; i < 4; i++)
    {
        char basexmltag[64] = "/sound/custom_music/track";
        char childtag[64];
        char defaultTitle[128] = "TRACK ";
        char defaultFilename[128] = "track";

        strcat(basexmltag, Utils_int_to_string(i+1));
        strcat(basexmltag, "/");

        Config_sound.custom_music[i].enabled  = GetXmlDocAttributeInt(&doc, basexmltag, "enabled", 0);

        strcpy(childtag, basexmltag); strcat(childtag, "title");
        strcat(defaultTitle, Utils_int_to_string(i + 1));
        strcpy(Config_sound.custom_music[i].title, GetXMLDocValueString(&doc, childtag, defaultTitle));
            
        strcpy(childtag, basexmltag); strcat(childtag, "filename");
        strcat(defaultFilename, Utils_int_to_string(i + 1));

        strcpy(Config_sound.custom_music[i].filename, GetXMLDocValueString(&doc, childtag, defaultFilename));
    }

    // ------------------------------------------------------------------------
    // CannonBoard Settings
    // ------------------------------------------------------------------------
    Config_cannonboard.enabled = GetXmlDocAttributeInt(&doc, "/cannonboard", "enabled", 0);
    strcpy(Config_cannonboard.port, GetXMLDocValueString(&doc, "/cannonboard/port", "COM6"));
    Config_cannonboard.baud    = GetXMLDocValueInt(&doc, "/cannonboard/baud", 57600);
    Config_cannonboard.debug   = GetXMLDocValueInt(&doc, "/cannonboard/debug", 0);
    Config_cannonboard.cabinet = GetXMLDocValueInt(&doc, "/cannonboard/cabinet", 0);


    // ------------------------------------------------------------------------
    // Controls
    // ------------------------------------------------------------------------
    Config_controls.gear          = GetXMLDocValueInt(&doc, "/controls/gear", 0);
    Config_controls.steer_speed   = GetXMLDocValueInt(&doc, "/controls/steerspeed", 3);
    Config_controls.pedal_speed   = GetXMLDocValueInt(&doc, "/controls/pedalspeed", 4);
    Config_controls.keyconfig[0]  = GetXMLDocValueInt(&doc, "/controls/keyconfig/up",    273);
    Config_controls.keyconfig[1]  = GetXMLDocValueInt(&doc, "/controls/keyconfig/down",  274);
    Config_controls.keyconfig[2]  = GetXMLDocValueInt(&doc, "/controls/keyconfig/left",  276);
    Config_controls.keyconfig[3]  = GetXMLDocValueInt(&doc, "/controls/keyconfig/right", 275);
    Config_controls.keyconfig[4]  = GetXMLDocValueInt(&doc, "/controls/keyconfig/acc",   122);
    Config_controls.keyconfig[5]  = GetXMLDocValueInt(&doc, "/controls/keyconfig/brake", 120);
    Config_controls.keyconfig[6]  = GetXMLDocValueInt(&doc, "/controls/keyconfig/gear1", 32);
    Config_controls.keyconfig[7]  = GetXMLDocValueInt(&doc, "/controls/keyconfig/gear2", 32);
    Config_controls.keyconfig[8]  = GetXMLDocValueInt(&doc, "/controls/keyconfig/start", 49);
    Config_controls.keyconfig[9]  = GetXMLDocValueInt(&doc, "/controls/keyconfig/coin",  53);
    Config_controls.keyconfig[10] = GetXMLDocValueInt(&doc, "/controls/keyconfig/menu",  286);
    Config_controls.keyconfig[11] = GetXMLDocValueInt(&doc, "/controls/keyconfig/view",  304);
    Config_controls.padconfig[0]  = GetXMLDocValueInt(&doc, "/controls/padconfig/acc",   0);
    Config_controls.padconfig[1]  = GetXMLDocValueInt(&doc, "/controls/padconfig/brake", 1);
    Config_controls.padconfig[2]  = GetXMLDocValueInt(&doc, "/controls/padconfig/gear1", 2);
    Config_controls.padconfig[3]  = GetXMLDocValueInt(&doc, "/controls/padconfig/gear2", 2);
    Config_controls.padconfig[4]  = GetXMLDocValueInt(&doc, "/controls/padconfig/start", 3);
    Config_controls.padconfig[5]  = GetXMLDocValueInt(&doc, "/controls/padconfig/coin",  4);
    Config_controls.padconfig[6]  = GetXMLDocValueInt(&doc, "/controls/padconfig/menu",  5);
    Config_controls.padconfig[7]  = GetXMLDocValueInt(&doc, "/controls/padconfig/view",  6);

    Config_controls.analog        = GetXmlDocAttributeInt(&doc, "/controls/analog", "enabled", 0);
    Config_controls.pad_id        = GetXMLDocValueInt(&doc, "/controls/pad_id", 0);
    Config_controls.axis[0]       = GetXMLDocValueInt(&doc, "/controls/analog/axis/wheel", 0);
    Config_controls.axis[1]       = GetXMLDocValueInt(&doc, "/controls/analog/axis/accel", 2);
    Config_controls.axis[2]       = GetXMLDocValueInt(&doc, "/controls/analog/axis/brake", 3);
    Config_controls.asettings[0]  = GetXMLDocValueInt(&doc, "/controls/analog/wheel/zone", 75);
    Config_controls.asettings[1]  = GetXMLDocValueInt(&doc, "/controls/analog/wheel/dead", 0);
    Config_controls.asettings[2]  = GetXMLDocValueInt(&doc, "/controls/analog/pedals/dead", 0);
    
    Config_controls.haptic        = GetXmlDocAttributeInt(&doc, "/controls/analog/haptic", "enabled", 0);
    Config_controls.max_force     = GetXMLDocValueInt(&doc, "/controls/analog/haptic/max_force", 9000);
    Config_controls.min_force     = GetXMLDocValueInt(&doc, "/controls/analog/haptic/min_force", 8500);
    Config_controls.force_duration= GetXMLDocValueInt(&doc, "/controls/analog/haptic/force_duration", 20);

    // ------------------------------------------------------------------------
    // Config_engine Settings
    // ------------------------------------------------------------------------

    Config_engine.dip_time      = GetXMLDocValueInt(&doc, "/engine/time",    0);
    Config_engine.dip_traffic   = GetXMLDocValueInt(&doc, "/engine/traffic", 1);
    
    Config_engine.freeze_timer    = Config_engine.dip_time == 4;
    Config_engine.disable_traffic = Config_engine.dip_traffic == 4;
    Config_engine.dip_time    &= 3;
    Config_engine.dip_traffic &= 3;

    Config_engine.freeplay      = GetXMLDocValueInt(&doc, "/engine/freeplay",        0) != 0;
    Config_engine.jap           = GetXMLDocValueInt(&doc, "/engine/japanese_tracks", 0);
    Config_engine.prototype     = GetXMLDocValueInt(&doc, "/engine/prototype",       0);
    
    // Additional Level Objects
    Config_engine.level_objects   = GetXMLDocValueInt(&doc, "/engine/levelobjects", 1);
    Config_engine.randomgen       = GetXMLDocValueInt(&doc, "/engine/randomgen",    1);
    Config_engine.fix_bugs_backup = 
    Config_engine.fix_bugs        = GetXMLDocValueInt(&doc, "/engine/fix_bugs",     1) != 0;
    Config_engine.fix_timer       = GetXMLDocValueInt(&doc, "/engine/fix_timer",    0) != 0;
    Config_engine.layout_debug    = GetXMLDocValueInt(&doc, "/engine/layout_debug", 0) != 0;
    Config_engine.new_attract     = GetXMLDocValueInt(&doc, "/engine/new_attract", 1) != 0;

    // ------------------------------------------------------------------------
    // Time Trial Mode
    // ------------------------------------------------------------------------

    Config_ttrial.laps    = GetXMLDocValueInt(&doc, "/time_trial/laps",    5);
    Config_ttrial.traffic = GetXMLDocValueInt(&doc, "/time_trial/traffic", 3);
    Config_cont_traffic   = GetXMLDocValueInt(&doc, "/continuous/traffic", 3);
#endif
    XMLDoc_free(&doc);
}

Boolean Config_save(const char* filename)
{
    XMLDoc saveDoc;
    XMLNode* videoNode,* windowNode,* soundNode,* controlsNode,* keyconfigNode,* padconfigNode,*engineNode,*timeTrialNode,*continuousNode;
    XMLDoc_init(&saveDoc);
    AddXmlHeader(&saveDoc);

    videoNode = AddXmlFatherNode(&saveDoc, "video");
    AddNodeInt(&saveDoc, videoNode, "mode",             Config_video.mode);
    windowNode = AddNode(&saveDoc, videoNode, "window");
    AddNodeInt(&saveDoc, windowNode, "scale",           Config_video.scale);
    AddNodeInt(&saveDoc, videoNode, "scanlines",        Config_video.scanlines);
    AddNodeInt(&saveDoc, videoNode, "fps",              Config_video.fps);
    AddNodeInt(&saveDoc, videoNode, "widescreen",       Config_video.widescreen);
    AddNodeInt(&saveDoc, videoNode, "hires",            Config_video.hires);

    soundNode = AddXmlFatherNode(&saveDoc, "sound");
    AddNodeInt(&saveDoc, soundNode, "enable",           Config_sound.enabled);
    AddNodeInt(&saveDoc, soundNode, "advertise",        Config_sound.advertise);
    AddNodeInt(&saveDoc, soundNode, "preview",          Config_sound.preview);
    AddNodeInt(&saveDoc, soundNode, "fix_samples",      Config_sound.fix_samples);

    controlsNode = AddXmlFatherNode(&saveDoc, "controls");
    AddNodeInt(&saveDoc, controlsNode, "gear",            Config_controls.gear);
    AddNodeInt(&saveDoc, controlsNode, "steerspeed",      Config_controls.steer_speed);
    AddNodeInt(&saveDoc, controlsNode, "pedalspeed",      Config_controls.pedal_speed);

    keyconfigNode = AddNode(&saveDoc, controlsNode, "keyconfig");
    AddNodeInt(&saveDoc, keyconfigNode, "up",    Config_controls.keyconfig[0]);
    AddNodeInt(&saveDoc, keyconfigNode, "down",  Config_controls.keyconfig[1]);
    AddNodeInt(&saveDoc, keyconfigNode, "left",  Config_controls.keyconfig[2]);
    AddNodeInt(&saveDoc, keyconfigNode, "right", Config_controls.keyconfig[3]);
    AddNodeInt(&saveDoc, keyconfigNode, "acc",   Config_controls.keyconfig[4]);
    AddNodeInt(&saveDoc, keyconfigNode, "brake", Config_controls.keyconfig[5]);
    AddNodeInt(&saveDoc, keyconfigNode, "gear1", Config_controls.keyconfig[6]);
    AddNodeInt(&saveDoc, keyconfigNode, "gear2", Config_controls.keyconfig[7]);
    AddNodeInt(&saveDoc, keyconfigNode, "start", Config_controls.keyconfig[8]);
    AddNodeInt(&saveDoc, keyconfigNode, "coin",  Config_controls.keyconfig[9]);
    AddNodeInt(&saveDoc, keyconfigNode, "menu",  Config_controls.keyconfig[10]);
    AddNodeInt(&saveDoc, keyconfigNode, "view",  Config_controls.keyconfig[11]);

    padconfigNode = AddNode(&saveDoc, controlsNode, "padconfig");
    AddNodeInt(&saveDoc, padconfigNode, "acc",   Config_controls.padconfig[0]);
    AddNodeInt(&saveDoc, padconfigNode, "brake", Config_controls.padconfig[1]);
    AddNodeInt(&saveDoc, padconfigNode, "gear1", Config_controls.padconfig[2]);
    AddNodeInt(&saveDoc, padconfigNode, "gear2", Config_controls.padconfig[3]);
    AddNodeInt(&saveDoc, padconfigNode, "start", Config_controls.padconfig[4]);
    AddNodeInt(&saveDoc, padconfigNode, "coin",  Config_controls.padconfig[5]);
    AddNodeInt(&saveDoc, padconfigNode, "menu",  Config_controls.padconfig[6]);
    AddNodeInt(&saveDoc, padconfigNode, "view",  Config_controls.padconfig[7]);

    AddNodeAttributeInt(&saveDoc, controlsNode, "analog", "enabled", Config_controls.analog);

    engineNode = AddXmlFatherNode(&saveDoc, "engine");
    AddNodeInt(&saveDoc, engineNode, "time",            Config_engine.freeze_timer ? 4 : Config_engine.dip_time);
    AddNodeInt(&saveDoc, engineNode, "traffic",         Config_engine.disable_traffic ? 4 : Config_engine.dip_traffic);
    AddNodeInt(&saveDoc, engineNode, "japanese_tracks", Config_engine.jap);
    AddNodeInt(&saveDoc, engineNode, "prototype",       Config_engine.prototype);
    AddNodeInt(&saveDoc, engineNode, "levelobjects",    Config_engine.level_objects);
    AddNodeInt(&saveDoc, engineNode, "new_attract",     Config_engine.new_attract);

    timeTrialNode = AddXmlFatherNode(&saveDoc, "time_trial");
    AddNodeInt(&saveDoc, timeTrialNode, "laps",    Config_ttrial.laps);
    AddNodeInt(&saveDoc, timeTrialNode, "traffic", Config_ttrial.traffic);
    
    continuousNode = AddXmlFatherNode(&saveDoc, "continuous");
    AddNodeInt(&saveDoc, continuousNode, "traffic", Config_cont_traffic);

    {
    FILE* file = fopen(filename, "w");

     if (!file)
     {
        fprintf(stderr, "Error: can't open %s for save\n", filename);
        return FALSE;
     }

     XMLDoc_print(&saveDoc, file, "\n", "\t", FALSE, 0, 4);
     fclose(file);
    }
    XMLDoc_free(&saveDoc);

    return TRUE;
}


void Config_load_scores(const char* filename)
{
    int i;
    XMLDoc doc;
    if (!XMLDoc_init(&doc) || !XMLDoc_parse_file_DOM(filename, &doc))
    {
        fprintf(stderr, "Error: problem loading %s for loading\n", filename);
        return;
    }
  
    // Game Scores
    for (i = 0; i < HISCORE_NUM_SCORES; i++)
    {
        score_entry* e = &OHiScore_scores[i];
        char basexmltag[64] = "/score";
        char childtag[64];

        strcat(basexmltag, Utils_int_to_string(i));
        strcat(basexmltag, "/");

   
        strcpy(childtag, basexmltag); strcat(childtag, "score");
        e->score = Utils_from_hex_string(GetXMLDocValueString(&doc, childtag, "0"));

        strcpy(childtag, basexmltag); strcat(childtag, "initial1");
        e->initial1 = GetXMLDocValueString(&doc, childtag, ".")[0];

        strcpy(childtag, basexmltag); strcat(childtag, "initial2");
        e->initial2 = GetXMLDocValueString(&doc, childtag, ".")[0];

        strcpy(childtag, basexmltag); strcat(childtag, "initial3");
        e->initial3 = GetXMLDocValueString(&doc, childtag, ".")[0];

        strcpy(childtag, basexmltag); strcat(childtag, "maptiles");
        e->maptiles = Utils_from_hex_string(GetXMLDocValueString(&doc, childtag, "20202020"));

        strcpy(childtag, basexmltag); strcat(childtag, "time");
        e->time = Utils_from_hex_string(GetXMLDocValueString(&doc, childtag, "0")); 

        if (e->initial1 == '.') e->initial1 = 0x20;
        if (e->initial2 == '.') e->initial2 = 0x20;
        if (e->initial3 == '.') e->initial3 = 0x20;
    }

    XMLDoc_free(&doc);
}

void Config_save_scores(const char* filename)
{
    int i;
    XMLDoc saveDoc;
    XMLDoc_init(&saveDoc);
    AddXmlHeader(&saveDoc);

    for (i = 0; i < HISCORE_NUM_SCORES; i++)
    {
        score_entry* e = &OHiScore_scores[i];
        char basexmltag[64] = "score";
        strcat(basexmltag, Utils_int_to_string(i));

        AddXmlFatherNode(&saveDoc, basexmltag);
        AddXmlChildNodeRootString(&saveDoc, "score", Utils_int_to_hex_string(e->score));
        AddXmlChildNodeRootString(&saveDoc, "initial1", e->initial1 == 0x20 ? "." : Utils_char_to_string(e->initial1));
        AddXmlChildNodeRootString(&saveDoc, "initial2", e->initial2 == 0x20 ? "." : Utils_char_to_string(e->initial2));
        AddXmlChildNodeRootString(&saveDoc, "initial3", e->initial3 == 0x20 ? "." : Utils_char_to_string(e->initial3));
        AddXmlChildNodeRootString(&saveDoc, "maptiles", Utils_int_to_hex_string(e->maptiles));
        AddXmlChildNodeRootString(&saveDoc, "time", Utils_int_to_hex_string(e->time));
    }

    {
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
}

void Config_load_tiletrial_scores()
{
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
}

void Config_save_tiletrial_scores()
{
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
