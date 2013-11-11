#ifndef __TWITCH_X11_STUFF_H__
#define __TWITCH_X11_STUFF_H__
#include <stdint.h>

void InitializeX11(uint16_t x_off, uint16_t y_off, uint16_t *w, uint16_t *h);
void StopX11();

uint32_t *GetX11(uint16_t x, uint16_t y, uint16_t w, uint16_t h);

#endif
