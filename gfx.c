#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <libetc.h>
#include <libgpu.h>

#include "gfx.h"

uint32_t gfx_screen_width;
uint32_t gfx_screen_height;
uint32_t gfx_screen_offset_y;

static DISPENV disp[2];
static DRAWENV draw[2];
static uint8_t db;

static TIM_IMAGE* tilesets[MAX_TILESETS];
static uint8_t tileset_layers[MAX_TILESETS];
static DR_TPAGE tileset_tpages[MAX_TILESETS];
static uint32_t current_tileset_cols;
static uint32_t current_tileset_rows;
static uint8_t current_tileset_id;

static TIM_IMAGE* title_image;
static DR_TPAGE title_tpage;

static TIM_IMAGE *font_img;
static uint8_t font_cols;
static uint8_t font_rows;
static uint8_t font_width;
static uint8_t font_height;
static uint8_t* font_chars;
static DR_TPAGE font_tpage;
static SVECTOR font_coords[256] = { 0 };

#define ORDERING_TABLE_SIZE (2 * MAX_TILESETS + 5) // tileset + title + font
static uint32_t ordering_table[ORDERING_TABLE_SIZE];

#define OT_FONT_TEXT (ORDERING_TABLE_SIZE - 1)
#define OT_FONT_TPAGE (ORDERING_TABLE_SIZE - 2)
#define OT_TITLE_IMAGE (ORDERING_TABLE_SIZE - 3)
#define OT_TITLE_TPAGE (ORDERING_TABLE_SIZE - 4)

#define PRIMITIVES_BUFFER_SIZE 32768
static uint8_t primitives_buffer[PRIMITIVES_BUFFER_SIZE];
static uint8_t *next_primitive;

// kindly provided by @spicyjpeg
static bool is_gpu_in_pal_mode(void) {
    return (*((uint32_t *) 0x1f801814) >> 20) & 1;
}

static void gfx_draw_begin()
{
    ClearOTag(ordering_table, ORDERING_TABLE_SIZE);
    next_primitive = primitives_buffer;

    for (uint8_t tileset_id = 0; tileset_id < MAX_TILESETS; tileset_id++)
    {
        DR_TPAGE* tpage = &tileset_tpages[tileset_id];
        AddPrim(&ordering_table[tileset_layers[tileset_id]], tpage);
    }
    addPrim(&ordering_table[OT_TITLE_TPAGE], &title_tpage);
    addPrim(&ordering_table[OT_FONT_TPAGE], &font_tpage);
}    

// set the video resolution according to 
// the current gpu pal or ntsc video mode
void gfx_init()
{
    // get the current video mode
    bool is_video_mode_pal = is_gpu_in_pal_mode();
    ResetGraph(0);

    // video mode pal
    if (is_video_mode_pal)    
    {
        gfx_screen_width = 320;
        gfx_screen_height = 256;
        gfx_screen_offset_y = 0;
        SetVideoMode(MODE_PAL);
    }
    // video mode ntsc
    else 
    {
        gfx_screen_width = 320;
        gfx_screen_height = 240;
        gfx_screen_offset_y = 8;
        SetVideoMode(MODE_NTSC); 
    }
    
    SetDefDispEnv(&disp[0], 0, gfx_screen_offset_y, gfx_screen_width, gfx_screen_height);
    SetDefDispEnv(&disp[1], 0, 256 + gfx_screen_offset_y, gfx_screen_width, gfx_screen_height);
    SetDefDrawEnv(&draw[0], 0, 256, gfx_screen_width, 256);
    SetDefDrawEnv(&draw[1], 0, 0, gfx_screen_width, 256);
    
    // to effectively change the display vertical resolution
    disp[0].screen.h = gfx_screen_height;
    disp[1].screen.h = gfx_screen_height;

    draw[0].isbg = 1;
    draw[1].isbg = 1;
    setRGB0(&draw[0], 32, 32, 32);
    setRGB0(&draw[1], 32, 32, 32);

    SetDispMask(1);

    // clear entire vram
    RECT rect = { 0, 0, 1024, 512 };
    ClearImage(&rect, 0, 0, 0);
    DrawSync(0);

    gfx_draw_begin();
}

void gfx_swap_buffers()
{
    DrawOTag2(&ordering_table[0]);
    DrawSync(0);
    VSync(0);
    db = !db;
    PutDispEnv(&disp[db]);
    PutDrawEnv(&draw[db]);

    gfx_draw_begin();
}

void gfx_add_tileset(uint8_t layer, uint8_t tileset_id, TIM_IMAGE *tileset)
{
    tilesets[tileset_id] = tileset;
    tileset_layers[tileset_id] = layer;
    // precreate tpage
    DR_TPAGE* tpage = &tileset_tpages[tileset_id];
    uint16_t tp = getTPage(tileset->mode & 3, 1, tileset->prect->x, tileset->prect->y);
    setDrawTPage(tpage, 0, 0, tp);
}

void gfx_set_tileset(uint8_t tileset_id)
{
    current_tileset_id = tileset_id;
    TIM_IMAGE *tileset = tilesets[tileset_id];
    current_tileset_cols = tileset->prect->w / TILESET_SIZE;
    current_tileset_rows = tileset->prect->h / TILESET_SIZE;
}

void gfx_draw_tile(uint8_t tile_id, int16_t x, int16_t y)
{
    SPRT *sprt = (SPRT*) next_primitive;
    setSprt(sprt);
    setXY0(sprt, x, y);
    setWH(sprt, TILESET_SIZE, TILESET_SIZE);
    uint8_t u = (tile_id % current_tileset_cols) * TILESET_SIZE;
    uint8_t v = (tile_id / current_tileset_cols) * TILESET_SIZE;
    setUV0(sprt, u, v);
    setRGB0(sprt, 128, 128, 128);
    addPrim(&ordering_table[tileset_layers[current_tileset_id] + 1], sprt);
    next_primitive += sizeof(SPRT);
}

void gfx_draw_selected_tile(int16_t x, int16_t y, uint8_t r, uint8_t g, uint8_t b)
{
    TILE *tile = (TILE*) next_primitive;
    setTile(tile);
    setXY0(tile, x, y);
    setWH(tile, TILESET_SIZE, TILESET_SIZE);
    setRGB0(tile, r, g, b);
    setSemiTrans(tile, 1);
    addPrim(&ordering_table[ORDERING_TABLE_SIZE - 1], tile);
    next_primitive += sizeof(TILE);
}


void gfx_set_title(TIM_IMAGE *title)
{
    title_image = title;
    // precreate tpage
    uint16_t tp = getTPage(title->mode & 3, 1, title->prect->x, title->prect->y);
    setDrawTPage(&title_tpage, 0, 0, tp);
}

void gfx_draw_title(int16_t x, int16_t y)
{
    SPRT *sprt = (SPRT*) next_primitive;
    setSprt(sprt);
    setXY0(sprt, x, y);
    setWH(sprt, title_image->prect->w, title_image->prect->h);
    setUV0(sprt, 0, 0);
    setRGB0(sprt, 128, 128, 128);
    addPrim(&ordering_table[OT_TITLE_IMAGE], sprt);
    next_primitive += sizeof(SPRT);
}

void gfx_set_font(TIM_IMAGE *font, uint8_t cols, uint8_t rows, uint8_t width, uint8_t height, uint8_t* chars)
{
    font_img = font;
    font_cols = cols;
    font_rows = rows;
    font_width = width;
    font_height = height;
    font_chars = chars;
    // font coordinates
    uint32_t chars_count = cols * rows;
    int16_t rx = 0;
    int16_t ry = 0;
    for (uint32_t i = 0; i < chars_count; i++)
    {
        uint8_t char_index = chars[i];
        font_coords[char_index].vx = rx;
        font_coords[char_index].vy = ry;
        rx += font_width;
        if (rx > (cols - 1) * font_width)
        {
            rx = 0;
            ry += font_height;
        }
    }
    // precreate tpage
    uint16_t tp = getTPage(font->mode & 3, 1, font->prect->x, font->prect->y);
    setDrawTPage(&font_tpage, 0, 0, tp);
}

void gfx_draw_text(uint8_t* text, int16_t x, int16_t y)
{
    uint32_t char_index = 0;
    uint8_t current_char;
    while (current_char = text[char_index])
    {
        SVECTOR* font_coord = &font_coords[current_char];
        SPRT *sprt = (SPRT*) next_primitive;
        setSprt(sprt);
        setXY0(sprt, x, y);
        setUV0(sprt, font_coord->vx, font_coord->vy);
        setWH(sprt, font_width, font_height);
        setRGB0(sprt, 128, 128, 128);
        addPrim(&ordering_table[OT_FONT_TEXT], sprt);
        next_primitive += sizeof(SPRT);
        x += font_width;
        char_index++;
    } 
}
