#ifndef SYSTEM_H
#define SYSTEM_H
#include <stddef.h>
#include <stdint.h>

#include "real.h"

#ifdef RISCV_CONSOLE

typedef enum {
    INPUT_BUTTON_4 = 128,
    INPUT_BUTTON_3 = 64,
    INPUT_BUTTON_2 = 32,
    INPUT_BUTTON_1 = 16,
    INPUT_DIRECTION_RIGHT = 8,
    INPUT_DIRECTION_DOWN = 4,
    INPUT_DIRECTION_UP = 2,
    INPUT_DIRECTION_LEFT = 1,
} Controls;

// library functions
Real clock();
int32_t abs(int32_t);
void *malloc(size_t size);
int32_t srand(int32_t seed);

// syscalls
uint32_t printf(const char *, ...);
uint32_t memcpy(void *src, void *dest, uint32_t size);
uint32_t set_mode(uint32_t mode);
uint32_t get_controller();
uint32_t get_pixel_bg_data(uint32_t index);
uint32_t set_pixel_bg_controls(uint32_t index, uint32_t controls);
uint32_t get_bg_palette(uint32_t index);
uint32_t set_video_callback(void (*callback)(void *), void *arg);
uint32_t set_timer_callback(void (*callback)());
#else
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#endif

#endif
