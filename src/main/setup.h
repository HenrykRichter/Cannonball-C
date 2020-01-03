// This file is AUTO GENERATED by CMake.
#ifndef _INC_SDL_SETUP_H
#define _INC_SDL_SETUP_H

#ifndef NOSDL
const static char* FILENAME_CONFIG = "config.xml";
const static char* FILENAME_SCORES = "hiscores.xml";
const static char* FILENAME_SCORES_JAPAN = "hiscores_jap.xml";
const static char* FILENAME_TTRIAL = "hiscores_timetrial.xml";
const static char* FILENAME_TTRIAL_JAPAN = "hiscores_timetrial_jap.xml";
const static char* FILENAME_CONT   = "hiscores_continuous.xml";
const static char* FILENAME_CONT_JAPAN   = "hiscores_continuous_jap.xml";
#include "SDL/SDL_video.h"
const static int SDL_FLAGS = SDL_HWSURFACE | SDL_DOUBLEBUF;
#else /* NOSDL */
const static char* FILENAME_CONFIG = "config.conf";
const static char* FILENAME_SCORES = "hiscores.conf";
const static char* FILENAME_SCORES_JAPAN = "hiscores_jap.conf";
const static char* FILENAME_TTRIAL = "hiscores_timetrial.conf";
const static char* FILENAME_TTRIAL_JAPAN = "hiscores_timetrial_jap.conf";
const static char* FILENAME_CONT   = "hiscores_continuous.conf";
const static char* FILENAME_CONT_JAPAN   = "hiscores_continuous_jap.conf";
#endif /* NOSDL */
#endif    
