#pragma once
#include <ozone.h>
#include <stdint.h>

uint32_t rgba(uint8_t r, uint8_t g, uint8_t b, uint8_t a);
uint32_t rgb(uint8_t r, uint8_t g, uint8_t b);
uint64_t complete_color(uint32_t fg, uint32_t bg);