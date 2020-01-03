#ifndef _INC_SRC_MAIN_UTILS_H
#define _INC_SRC_MAIN_UTILS_H
/***************************************************************************
    General Helper Functions

    Copyright Chris White.
    See license.txt for more details.
***************************************************************************/

#include "stdint.h"

const char* Utils_int_to_string(int i);
const char* Utils_char_to_string(char c);
const char* Utils_int_to_hex_string(int i);
uint32_t Utils_from_hex_string(const char* string);

const char* getScoresFilename();
#endif
