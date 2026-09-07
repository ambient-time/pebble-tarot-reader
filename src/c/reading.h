#pragma once
#include <stdint.h>
#include <stdbool.h>

typedef struct {
  uint32_t magic, seed;
  uint16_t revealed;
  uint8_t count, selected, reversals, reserved;
  uint8_t cards[10], reversed[10], x[10], y[10];
} Reading;

bool reading_valid(const Reading *r);
void reading_draw(Reading *r, uint8_t count, uint32_t seed, bool reversals);
