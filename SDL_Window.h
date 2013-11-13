#ifndef __TWITCH_SDL_MODULE_H__
#define __TWITCH_SDL_MODULE_H__
#include <stdint.h>
#include "X11_Device.h"
#include "Webcam_Device.h"
#include "Pulse.h"
#include "LibAV.h"

void InitializeSDL(const char* twitch_key);
void StopSDL();
int  Flip();

#endif
