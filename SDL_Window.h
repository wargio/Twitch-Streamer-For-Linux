#ifndef __TWITCH_SDL_MODULE_H__
#define __TWITCH_SDL_MODULE_H__
#include <stdint.h>
#include "X11_Device.h"
#include "Webcam_Device.h"
#include "Pulse.h"

void InitializeSDL();
void StopSDL();
int  Flip();

#endif
