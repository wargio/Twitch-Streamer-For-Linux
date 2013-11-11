#ifndef __TWITCH_WEBCAM_STUFF_H__
#define __TWITCH_WEBCAM_STUFF_H__
#include <stdint.h>

void InitializeV4L(const char* device, uint16_t w, uint16_t h, int *check_changes);
void StopV4L();
void GetWebcamInfo(uint16_t *w, uint16_t *h);
uint32_t* GetWebcam();

#endif
