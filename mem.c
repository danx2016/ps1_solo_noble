#include <malloc.h>

#include "mem.h"

static uint8_t mem_buffer[MEM_HEAP_SIZE];

void mem_init()
{
    InitHeap3((uint32_t*) mem_buffer, MEM_HEAP_SIZE);
}

void* mem_alloc(uint32_t size)
{
    return malloc3(size);
}

void mem_free(void* ptr)
{
    free3(ptr);
}