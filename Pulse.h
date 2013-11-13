#ifndef __TWITCH_PULSE_MODULE_H__
#define __TWITCH_PULSE_MODULE_H__

#define BUFFER_SIZE	(524288)   // 512k = 512*1024

void InitializePulse();
void StopPulse();
uint8_t *AudioLoop();

#endif


