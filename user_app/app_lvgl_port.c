#include "platform.h"
#include "lvgl.h"

#define AMOLED_MAX_WIDTH  470
#define AMOLED_MAX_HEIGHT 460
uint8_t _buf[2048];

void amoled_flush_area(const lv_area_t * area, lv_color_t * color_p)
{
    if (!area || !color_p) return;

    uint16_t width  = area->x2 - area->x1 + 1;
    uint16_t height = area->y2 - area->y1 + 1;


    if (area->x2 >= AMOLED_MAX_WIDTH)  width  = AMOLED_MAX_WIDTH  - area->x1;
    if (area->y2 >= AMOLED_MAX_HEIGHT) height = AMOLED_MAX_HEIGHT - area->y1;

    for (int y = 0; y < height; y++) {
        lv_color_t * color_line = color_p + y * width;

        for (int x = 0; x < width; x++) {
            _buf[2 * x]     = color_line[x].full >> 8;
            _buf[2 * x + 1] = color_line[x].full & 0xFF;
        }

        amoled_set_window(area->x1, area->y1 + y, area->x1 + width - 1, area->y1 + y);
        amoled_select();
        amoled_send_color(0x2c, _buf, width * 2);
        amoled_unselect();
    }
}
