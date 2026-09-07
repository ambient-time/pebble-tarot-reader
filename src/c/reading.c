#include "reading.h"
#include <string.h>

static uint32_t next(uint32_t *state) {
  uint32_t x = *state; x ^= x << 13; x ^= x >> 17; x ^= x << 5;
  return *state = x;
}
static uint32_t bounded(uint32_t *state, uint32_t n) {
  // Xorshift emits 1..UINT32_MAX. Reject the incomplete final bucket.
  uint32_t x, limit = UINT32_MAX - UINT32_MAX % n;
  do { x = next(state); } while (x > limit);
  return (x - 1) % n;
}
bool reading_valid(const Reading *r) {
  if (r->magic != 0x54415231 || !r->seed || r->reversals > 1 ||
      !(r->count == 1 || r->count == 3 || r->count == 5 || r->count == 6 || r->count == 7 || r->count == 10) ||
      r->selected >= r->count || r->revealed >= (1u << r->count)) return false;
  for (int i = 0; i < r->count; i++) {
    if (r->cards[i] >= 78 || r->reversed[i] > 1 || (!r->reversals && r->reversed[i]) ||
        r->x[i] > 100 || r->y[i] > 100) return false;
    for (int j = 0; j < i; j++) if (r->cards[i] == r->cards[j]) return false;
  }
  return true;
}
void reading_draw(Reading *r, uint8_t count, uint32_t seed, bool reversals) {
  memset(r, 0, sizeof(*r));
  if (!(count == 1 || count == 3 || count == 5 || count == 6 || count == 7 || count == 10)) count = 1;
  r->magic = 0x54415231; r->seed = seed ? seed : 0x6d2b79f5;
  r->count = count; r->reversals = reversals;
  uint32_t state = r->seed;
  uint8_t deck[78]; for (int i = 0; i < 78; i++) deck[i] = i;
  for (int i = 77; i > 0; i--) {
    int j = bounded(&state, i+1); uint8_t t = deck[i]; deck[i] = deck[j]; deck[j] = t;
  }
  for (int i = 0; i < count; i++) {
    r->cards[i] = deck[i];
    // The supplied Celtic table uses 28%; its open cast uses 50%.
    r->reversed[i] = reversals && bounded(&state, 100) < (count == 10 ? 28 : 50);
    int best = -1;
    for (int trial = 0; trial < 72; trial++) {
      int x = 21 + bounded(&state, 59), y = 15 + bounded(&state, 53), nearest = 100000;
      for (int j = 0; j < i; j++) {
        int dx = (x-r->x[j])*5, dy = (y-r->y[j])*4;
        int distance = dx*dx + dy*dy; if (distance < nearest) nearest = distance;
      }
      if (nearest > best) { best = nearest; r->x[i] = x; r->y[i] = y; }
    }
  }
}
