#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <pulse/simple.h>
#include <pulse/error.h>
#include "Pulse.h"

#define BUFSIZE 1024

static const pa_sample_spec ss = {
	.format = PA_SAMPLE_S16LE,
	.rate = 44100,
	.channels = 2
};

uint8_t pbuf[BUFFER_SIZE];
pa_simple *s = NULL;
int error2;

void InitializePulse(){
	printf("Loading PulseAudio Module\n");

	/* Create the recording stream */
	if (!(s = pa_simple_new(NULL, "Twitch Streamer Linux - Audio Rec", PA_STREAM_RECORD, NULL, "Twitch Streamer Linux - Audio Rec", &ss, NULL, NULL, &error2))) {
		fprintf(stderr, __FILE__": pa_simple_new() failed: %s\n", pa_strerror(error2));
		s = NULL;
		return;
	}
	printf("PulseAudio Module Running\n");
}

void StopPulse(){
	if (s)
		pa_simple_free(s);
	printf("Closing PulseAudio Module\n");
}

/* A simple routine calling UNIX write() in a loop */
static ssize_t loop_write(int fd, const void*data, size_t size) {
	ssize_t ret = 0;
	while (size > 0) {
		ssize_t r;
		if ((r = write(fd, data, size)) < 0)
			return r;
		if (r == 0)
			break;
		ret += r;
		data = (const uint8_t*) data + r;
		size -= (size_t) r;
	}
	return ret;
}

uint8_t *AudioLoop(){
	/* Record some data ... */
	if (pa_simple_read(s, pbuf, sizeof(pbuf), &error2) < 0) {
		fprintf(stderr, __FILE__": pa_simple_read() failed: %s\n", pa_strerror(error2));
		StopPulse();
	}
	return (uint8_t *)pbuf;
	/* And write it to STDOUT */
/*
	if (loop_write(STDOUT_FILENO, pbuf, sizeof(pbuf)) != sizeof(pbuf)) {
		fprintf(stderr, __FILE__": write() failed: %s\n", strerror2(errno));
		StopPulse();
	}
*/
}

