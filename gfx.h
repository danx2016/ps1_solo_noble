#ifndef _GFX_H_
#define _GFX_H_

#include <stdint.h>

extern uint32_t gfx_screen_width;
extern uint32_t gfx_screen_height;

extern void gfx_init();
extern void gfx_swap_buffers();

#define TILESET_SIZE 32

#define MAX_TILESETS 2

#define TILESET_LAYER_MARBLE 0
#define TILESET_LAYER_STONES 2

#define TILESET_ID_MARBLE 0
#define TILESET_ID_STONES 1

#define STONE_BLACK 0
#define STONE_RED   1
#define STONE_WHITE 2

extern void gfx_add_tileset(uint8_t layer, uint8_t tileset_id, TIM_IMAGE *tileset);
extern void gfx_set_tileset(uint8_t tileset_id);
extern void gfx_draw_tile(uint8_t tile_id, int16_t x, int16_t y);
extern void gfx_draw_selected_tile(int16_t x, int16_t y, uint8_t r, uint8_t g, uint8_t b);

extern void gfx_set_title(TIM_IMAGE *title);
extern void gfx_draw_title(int16_t x, int16_t y);

extern void gfx_set_font(TIM_IMAGE *font, uint8_t cols, uint8_t rows, uint8_t width, uint8_t height, uint8_t* chars);
extern void gfx_draw_text(uint8_t* text, int16_t x, int16_t y);

#endif /* _GFX_H_ */