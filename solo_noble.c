#include <memory.h>

#include <libetc.h>
#include <libgpu.h>

#include "solo_noble.h"
#include "gfx.h"
#include "input.h"
#include "mem.h"
#include "resource.h"
#include "audio.h"

#define CELL_ID_BLOCKED 2

#define GRID_COLS 9
#define GRID_ROWS 7

uint8_t grid_template[GRID_ROWS][GRID_COLS];
uint8_t grid[GRID_ROWS][GRID_COLS];

int8_t grid_selected_col;
int8_t grid_selected_row;

uint8_t selection_color_intensity[64];

uint32_t frame_count;
uint32_t frame_count2;

#define GAME_STATE_ID_TITLE         1
#define GAME_STATE_ID_CREDITS       2
#define GAME_STATE_ID_PLAYING       3
#define GAME_STATE_ID_LEVEL_CLEARED 4

uint8_t game_state_id = GAME_STATE_ID_TITLE;

#define PLAYING_STATE_ID_SELECT_SRC 1
#define PLAYING_STATE_ID_SELECT_DST 2

uint8_t playing_state_id = PLAYING_STATE_ID_SELECT_SRC;
int8_t src_col;
int8_t src_row;

#define GAME_LAST_LEVEL 2
uint8_t current_level;

static void load_current_level()
{
    uint8_t* level_file_name = "\\ASSETS\\LEVELS\\LEVEL_.TXT;1";
    level_file_name[20] = current_level + '0';
    resource_load_level(level_file_name, GRID_COLS, GRID_ROWS, (uint8_t*) grid_template);
    uint32_t grid_ptr = (uint32_t) grid_template;
}

static void reset_current_level()
{
    grid_selected_col = 4;
    grid_selected_row = 3;
    frame_count = 0;
    frame_count2 = 0;
    playing_state_id = PLAYING_STATE_ID_SELECT_SRC;
    src_col = 0;
    src_row = 0;
    memcpy(grid, grid_template, GRID_COLS * GRID_ROWS);
}

static void load_all_assets()
{
    TIM_IMAGE *tileset_marble = resource_load_tim_image("\\ASSETS\\IMAGES\\MARBLE.TIM;1");
    TIM_IMAGE *tileset_stones = resource_load_tim_image("\\ASSETS\\IMAGES\\STONES.TIM;1");

    gfx_add_tileset(TILESET_LAYER_MARBLE, TILESET_ID_MARBLE, tileset_marble);
    gfx_add_tileset(TILESET_LAYER_STONES, TILESET_ID_STONES, tileset_stones);

    TIM_IMAGE *title = resource_load_tim_image("\\ASSETS\\IMAGES\\TITLE.TIM;1");
    gfx_set_title(title);

    TIM_IMAGE *font = resource_load_tim_image("\\ASSETS\\IMAGES\\FONT.TIM;1");
    uint8_t *char_set = " !\"#$%&'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_`abcdefghijklmnopqrstuvwxyz{|}~ ";
    gfx_set_font(font, 16, 6, 8, 12, char_set);

    uint8_t *sound_drop = resource_load("\\ASSETS\\SOUNDS\\DROP.VAG;1");
    uint8_t *sound_move = resource_load("\\ASSETS\\SOUNDS\\MOVE.VAG;1");
    uint8_t *sound_select = resource_load("\\ASSETS\\SOUNDS\\SELECT.VAG;1");
    uint8_t *sound_start = resource_load("\\ASSETS\\SOUNDS\\START.VAG;1");
    uint8_t *sound_error = resource_load("\\ASSETS\\SOUNDS\\ERROR.VAG;1");

    audio_add_sound(SOUND_ID_DROP, sound_drop);
    audio_add_sound(SOUND_ID_MOVE, sound_move);
    audio_add_sound(SOUND_ID_SELECT, sound_select);
    audio_add_sound(SOUND_ID_START, sound_start);
    audio_add_sound(SOUND_ID_ERROR, sound_error);

    // once all sounds have been transferred to SPU RAM, 
    // they can be freed from the main RAM
    mem_free(sound_drop);
    mem_free(sound_move);
    mem_free(sound_select);
    mem_free(sound_start);
    mem_free(sound_error);
}

void init()
{
    mem_init();
    resource_init();
    gfx_init();
    audio_init();
    input_init();
    load_all_assets();
    
    // pre-calculate selection color intensity
    for (int c = 0; c < 64; c++)
    {
        selection_color_intensity[c] = (uint8_t) ((csin(c << 6) >> 5) + 128) >> 1;
    }

    reset_current_level();
    audio_play_cdda_music(MUSIC_TRACK_TITLE, true);
}


static uint8_t get_grid_cell_id(int8_t col, int8_t row)
{
    if (col < 0 || row < 0 || col > GRID_COLS - 1 || row > GRID_ROWS - 1)
    {
        return 2;
    }
    return grid[row][col];
}

static bool check_stone_src(int8_t col, int8_t row)
{
    bool cond_local = get_grid_cell_id(col, row) == 1;
    bool cond_up = get_grid_cell_id(col, row - 1) == 1 && get_grid_cell_id(col, row - 2) == 0;
    bool cond_down = get_grid_cell_id(col, row + 1) == 1 && get_grid_cell_id(col, row + 2) == 0;
    bool cond_left = get_grid_cell_id(col - 1, row) == 1 && get_grid_cell_id(col - 2, row) == 0;
    bool cond_right = get_grid_cell_id(col + 1, row) == 1 && get_grid_cell_id(col + 2, row) == 0;
    return cond_local && (cond_up || cond_down || cond_left || cond_right);
}

static bool check_stone_dst(int8_t col, int8_t row)
{
    bool cond_local = get_grid_cell_id(col, row) == 0;
    bool cond_up = get_grid_cell_id(col, row - 1) == 1 && get_grid_cell_id(col, row - 2) == 1 && col == src_col && row - 2 == src_row;
    bool cond_down = get_grid_cell_id(col, row + 1) == 1 && get_grid_cell_id(col, row + 2) == 1 && col == src_col && row + 2 == src_row;
    bool cond_left = get_grid_cell_id(col - 1, row) == 1 && get_grid_cell_id(col - 2, row) == 1 && col - 2 == src_col && row == src_row;
    bool cond_right = get_grid_cell_id(col + 1, row) == 1 && get_grid_cell_id(col + 2, row) == 1 && col + 2== src_col && row == src_row;
    return cond_local && (cond_up || cond_down || cond_left || cond_right);
}

static bool can_move(int8_t dc, int8_t dr)
{
    return get_grid_cell_id(grid_selected_col + dc, grid_selected_row + dr) != CELL_ID_BLOCKED;
}

static bool commit_move()
{
    int8_t dst_col = grid_selected_col;
    int8_t dst_row = grid_selected_row;
    int8_t remove_col = src_col + (dst_col - src_col) / 2;
    int8_t remove_row = src_row + (dst_row - src_row) / 2;
    grid[src_row][src_col] = 0;
    grid[remove_row][remove_col] = 0;
    grid[dst_row][dst_col] = 1;
    playing_state_id = PLAYING_STATE_ID_SELECT_SRC;
    src_col = 0;
    src_row = 0;
    audio_play_sound(SOUND_ID_DROP);
}

static bool check_level_cleared()
{
    uint32_t stones_count = 0;
    for (int8_t r = 0; r < GRID_ROWS; r++)
    {
        for (int8_t c = 0; c < GRID_COLS; c++)
        {
            if (get_grid_cell_id(c, r) == 1)
            {
                stones_count++;
            }
        }
    }
    return stones_count == 1;
}

static void handle_input_playing()
{
    if (input_is_action_just_pressed(0, SELECT))
    {
        frame_count = 0;
        game_state_id = GAME_STATE_ID_TITLE;
        audio_play_cdda_music(MUSIC_TRACK_TITLE, true);
        return;
    }
    if (input_is_action_just_pressed(0, START))
    {
        reset_current_level();
    }

    if (input_is_action_just_pressed(0, UP) && can_move(0, -1))
    {
        grid_selected_row--;
        frame_count = 0;
        audio_play_sound(SOUND_ID_MOVE);
    }
    if (input_is_action_just_pressed(0, DOWN) && can_move(0, 1))
    {
        grid_selected_row++;
        frame_count = 0;
        audio_play_sound(SOUND_ID_MOVE);
    }
    if (input_is_action_just_pressed(0, LEFT) && can_move(-1, 0))
    {
        grid_selected_col--;
        frame_count = 0;
        audio_play_sound(SOUND_ID_MOVE);
    }
    if (input_is_action_just_pressed(0, RIGHT) && can_move(1, 0))
    {
        grid_selected_col++;
        frame_count = 0;
        audio_play_sound(SOUND_ID_MOVE);
    }

    if (input_is_action_just_pressed(0, CONFIRM) && check_stone_src(grid_selected_col, grid_selected_row))
    {
        src_col = grid_selected_col;
        src_row = grid_selected_row;
        playing_state_id = PLAYING_STATE_ID_SELECT_DST;
        audio_play_sound(SOUND_ID_SELECT);
    }
    else if (input_is_action_just_pressed(0, CONFIRM) && check_stone_dst(grid_selected_col, grid_selected_row))
    {
        commit_move();
    }
    else if (input_is_action_just_pressed(0, CONFIRM))
    {
        audio_play_sound(SOUND_ID_ERROR);
    }
}

void fixed_update()
{
    input_update();
    switch (game_state_id)
    {
        case GAME_STATE_ID_TITLE:
            if (input_is_action_just_pressed(0, START))
            {
                current_level = 1;
                load_current_level();
                reset_current_level();
                audio_play_cdda_music(MUSIC_TRACK_PLAYING, true);
                game_state_id = GAME_STATE_ID_PLAYING;
                audio_play_sound(SOUND_ID_START);
                VSync(60);
            }
            break;

        case GAME_STATE_ID_CREDITS:
            if (input_is_action_just_pressed(0, ANY))
            {
                frame_count = 0;
                game_state_id = GAME_STATE_ID_TITLE;
            }
            break;

        case GAME_STATE_ID_PLAYING:
            handle_input_playing();
            break;

        case GAME_STATE_ID_LEVEL_CLEARED:
            if (frame_count < 360)
            {
                return;
            }
            if (current_level == GAME_LAST_LEVEL && input_is_action_just_pressed(0, ANY))
            {
                frame_count = 0;
                game_state_id = GAME_STATE_ID_TITLE;
                audio_play_cdda_music(MUSIC_TRACK_TITLE, true);
            }
            else if (input_is_action_just_pressed(0, ANY))
            {
                current_level++;
                load_current_level();
                reset_current_level();
                game_state_id = GAME_STATE_ID_PLAYING;
                audio_play_cdda_music(MUSIC_TRACK_PLAYING, true);
            }
            break;
    }
}

static void render_board()
{
    gfx_set_tileset(TILESET_ID_MARBLE);
    for (int8_t r = 0; r < 9; r++)
    {
        for (int8_t c = 0; c < 11; c++)
        {
            uint8_t tile_id = get_grid_cell_id(c - 1, r - 1) == 2 ? 4 : 0;
            if (game_state_id != GAME_STATE_ID_PLAYING)
            {
                tile_id = 4;
            }
            gfx_draw_tile(tile_id, c * 32 - 16, r * 32 - 16);

        }
    }
}

static void render_stones()
{
    gfx_set_tileset(TILESET_ID_STONES);
    for (int r = 0; r < 7; r++)
    {
        for (int c = 0; c < 9; c++)
        {
            if (grid[r][c] == 1)
            {
                if (playing_state_id == PLAYING_STATE_ID_SELECT_DST && 
                    c == src_col && r == src_row && 
                    (frame_count2 % 24) < 12)
                {
                    gfx_draw_tile(STONE_RED, (c + 1) * 32 - 16, (r + 1) * 32 - 16);
                }
                else {
                    gfx_draw_tile(STONE_BLACK, (c + 1) * 32 - 16, (r + 1) * 32 - 16);
                }
            }

        }
    }
}

static void render_selected_tile()
{
    uint8_t c = selection_color_intensity[frame_count % 64];
    int16_t sc = (grid_selected_col + 1) * TILESET_SIZE - 16;
    int16_t sr = (grid_selected_row + 1) * TILESET_SIZE - 16;

    bool check_src = check_stone_src(grid_selected_col, grid_selected_row);
    bool check_dst_state = playing_state_id == PLAYING_STATE_ID_SELECT_DST;
    bool check_dst = check_stone_dst(grid_selected_col, grid_selected_row);

    if (check_src || (check_dst_state && check_dst))
    {
        gfx_draw_selected_tile(sc, sr, 0, c, 0);
    }
    else {
        gfx_draw_selected_tile(sc, sr, c, 0, 0);
    }
}

static void render_title()
{
    gfx_draw_title(32, 72);
    if ((frame_count2 % 24) < 12) // blink
    {
        gfx_draw_text("- PRESS START BUTTON -", 72, 150);
    }
    gfx_draw_text("(C)danx2016 2023", 96, 212);

    // after some time, show credits
    if (frame_count > 600)
    {
        game_state_id = GAME_STATE_ID_CREDITS;
        frame_count = 0;
    }
}

static void render_credits()
{
    uint16_t credits_y = 32;
    gfx_draw_text("- CREDITS -", 116, credits_y); credits_y += 12;
    
    gfx_draw_text("Programming:", 8, credits_y); credits_y += 12;
    gfx_draw_text(" by danx2016", 8, credits_y); credits_y += 24;

    gfx_draw_text("Graphics:", 8, credits_y); credits_y += 12;
    gfx_draw_text(" Boardgame Tiles by Lanea Zimmerman", 8, credits_y); credits_y += 12;
    gfx_draw_text(" Bitmap Font     by Clint Bellanger", 8, credits_y); credits_y += 24;

    gfx_draw_text("Musics:", 8, credits_y);  credits_y += 12;
    gfx_draw_text(" \"Rise of Spirit\" by Alexandr Zhelanov", 8, credits_y);  credits_y += 12;
    gfx_draw_text(" soundcloud.com/alexandr-zhelanov", 8, credits_y);  credits_y += 24;

    gfx_draw_text(" \"Soliloquy\" by Matthew Pablo", 8, credits_y);  credits_y += 12;
    gfx_draw_text(" \"Lively Meadow", 8, credits_y);  credits_y += 12;
    gfx_draw_text("    (Victory Fanfare and Song)\"", 8, credits_y);  credits_y += 12;
    gfx_draw_text("    by Matthew Pablo", 8, credits_y);  credits_y += 12;
    gfx_draw_text(" www.matthewpablo.com", 8, credits_y); 

    // after some time, return to title
    if (frame_count > 600)
    {
        game_state_id = GAME_STATE_ID_TITLE;
        frame_count = 0;
    }    
}

static void render_playing()
{
    render_stones();
    render_selected_tile();
    gfx_draw_text("START ->RESET", 212, 220); 
    gfx_draw_text("SELECT->TITLE", 212, 232); 
    uint8_t *level_str = "LEVEL:_";
    level_str[6] = current_level + '0'; 
    gfx_draw_text(level_str, 260, 16); 
}

static void render_level_cleared()
{
    gfx_draw_text("CONGRATULATIONS!", 96, 108); 
    gfx_draw_text("LEVEL CLEARED :)", 96, 132); 
}

void render()
{
    render_board();
    switch (game_state_id)
    {
        case GAME_STATE_ID_TITLE:
            render_title();
            break;

        case GAME_STATE_ID_CREDITS:
            render_credits();
            break;

        case GAME_STATE_ID_PLAYING:
            render_playing();
            break;

        case GAME_STATE_ID_LEVEL_CLEARED:
            render_level_cleared();
            break;
    }
    gfx_draw_text(GetVideoMode() == MODE_NTSC ? "NTSC" : "PAL", 8, 16); 
}

int solo_noble_main()
{
    init();
    
    while (true)
    {
        fixed_update();
        render();

        gfx_swap_buffers();

        // check level cleared
        if (check_level_cleared() && game_state_id == GAME_STATE_ID_PLAYING) 
        {
            frame_count = 0;
            game_state_id = GAME_STATE_ID_LEVEL_CLEARED;
            audio_play_cdda_music(MUSIC_TRACK_VICTORY, false);
            VSync(30);
        }

        frame_count++;
        frame_count2++;
    }

    return 0;
};