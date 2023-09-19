#ifndef _RESOURCE_H
#define _RESOURCE_H

#include <stdint.h>
#include <libgpu.h>

extern void resource_init();
extern void* resource_load(uint8_t* res_file_name);
extern TIM_IMAGE* resource_load_tim_image(uint8_t* res_tim_img_file_name);
extern void resource_load_level(uint8_t* res_level_file_name, size_t grid_cols, size_t grid_rows, uint8_t *grid);

#endif /* _RESOURCE_LOADER_H */