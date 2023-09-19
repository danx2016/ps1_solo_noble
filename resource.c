#include <stddef.h>
#include <libetc.h>
#include <libcd.h>

#include "resource.h"
#include "mem.h"
    

void resource_init()
{
    CdInit();
}

void* resource_load(uint8_t* res_file_name)
{
    CdlFILE file;
    uint8_t result;
    if (CdSearchFile(&file, res_file_name))
    {
        uint32_t sectors_count = (file.size + 2047) / 2048;
        void* res_buffer = mem_alloc(sectors_count * 2048);
        CdControl(CdlSetloc, (uint8_t*) &file.pos, &result);
        CdRead(sectors_count, (uint32_t*) res_buffer, CdlModeSpeed);
        CdReadSync(0, &result);
        return res_buffer; 
    }
    return NULL;
}

static void load_texture(uint32_t *tim, TIM_IMAGE *tparam)
{
    OpenTIM(tim);                                   
    ReadTIM(tparam);
    //printf("rect %d %d %d %d\n", tparam->prect->x, tparam->prect->y, tparam->prect->w, tparam->prect->h);                                
    LoadImage(tparam->prect, tparam->paddr);        
    DrawSync(0);                                    
    if (tparam->mode & 0x8){ // check 4th bit       
        LoadImage(tparam->crect, tparam->caddr);    
        DrawSync(0);                                
    }
}

TIM_IMAGE* resource_load_tim_image(uint8_t* res_tim_img_file_name)
{
    TIM_IMAGE *tparam = mem_alloc(sizeof(TIM_IMAGE));
    void* tim = resource_load(res_tim_img_file_name);
    load_texture(tim, tparam);
    return tparam;
}

// level = 1~9
void resource_load_level(uint8_t* res_level_file_name, size_t grid_cols, size_t grid_rows, uint8_t *grid)
{
    uint8_t* level_data = resource_load(res_level_file_name);
    uint32_t level_data_index = 0;
    for (int i = 0; i < grid_cols * grid_rows; i++)
    {
        uint8_t c = 0;
        while (c != '0' && c != '1' && c != '2')
        {
            c = level_data[level_data_index++];
        }
        grid[i] = c - '0';
    }
    mem_free(level_data);
}