#ifndef __TWITCH_SDL_STUFF_H__
#define __TWITCH_SDL_STUFF_H__
#include <stdint.h>
#include "X11_Device.h"
#include "Webcam_Device.h"

void InitializeSDL();
void StopSDL();
int  Flip();

#endif
