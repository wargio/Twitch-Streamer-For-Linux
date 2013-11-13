#ifndef __TWITCH_LIBAV_STUFF_H__
#define __TWITCH_LIBAV_STUFF_H__

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <libavutil/mathematics.h>
}
#include "Pulse.h"

// Settings here: http://help.twitch.tv/customer/portal/articles/1253460-broadcast-requirements
#define AUDIO_CODEC_NAME	"libmp3lame" // MP3 Lib
#define AUDIO_SAMPLING_RATE	(44100)      // Sampling frequency 44.1 KHz (MP3)
#define AUDIO_BIT_RATE		(128*1000)   // 128 kbps (MP3)

#define VIDEO_CODEC_NAME	"libx264"    // x264 Lib
#define VIDEO_BIT_RATE		(500000)     // 500 kbps
#define VIDEO_FRAME_RATE	(1)         // 24

void InitializeLibAV(uint16_t Width, uint16_t Height, const char* url, uint32_t *ScrBuffer);
void StopLibAV();
void MuxingLoop();

#endif

