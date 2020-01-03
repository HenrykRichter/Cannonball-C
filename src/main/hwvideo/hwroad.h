#ifndef _INC_SRC_MAIN_HWVIDEO_HWROAD_H
#define _INC_SRC_MAIN_HWVIDEO_HWROAD_H
#include "stdint.h"


#define ROAD_RAM_SIZE 0x1000
#define HWRoad_rom_size 0x8000

void HWRoad_init(const uint8_t*, const Boolean hires);
void HWRoad_write16(uint32_t adr, const uint16_t data);
void HWRoad_write16IncP(uint32_t* adr, const uint16_t data);
void HWRoad_write32(uint32_t* adr, const uint32_t data);
uint16_t HWRoad_read_road_control();
void HWRoad_write_road_control(const uint8_t);

extern void (*HWRoad_render_background)(uint16_t*);
extern void (*HWRoad_render_foreground)(uint16_t*);

typedef struct
{
   uint8_t  *src0;
   uint8_t  *src1;
   int32_t  hpos0;
   int32_t  hpos1;
   uint16_t width;
   uint16_t *dest;
   uint16_t *colortable;
} RoadRender;

typedef struct
{
  uint16_t *pix;        /* output pixel data */
  uint16_t *ram;	/* road RAM     */
  uint16_t coloff;      /* color offset */
  uint16_t xs;          /* width  */
  uint16_t ys;          /* height */
  uint8_t  ctl;         /* HWRoad_road_control */

} FrameRoadBG;

#endif

