#include "LibAV.h"

#include <cstdio>
#include <cstdlib>
//http://www.roxlu.com/2013/010/x264-encoder
short error = 0;

AVFormatContext	*Muxer = NULL; //oc
AVOutputFormat	*MuxFormat = NULL;

/* VIDEO */
AVCodec 	*VideoCodec = NULL;
AVStream 	*VideoStream = NULL;
double VideoTime;
AVFrame *VFrame = NULL;
AVPicture dst_picture;
AVPacket VideoPKT = { 0 };
static int frame_count = 0;

/* AUDIO */
AVCodec 	*AudioCodec = NULL;
AVStream 	*AudioStream = NULL;
AVPacket AudioPKT = { 0 };
AVFrame *AFrame = NULL;
double AudioTime;
static int got_packet = 0;

int fail = 0;
uint8_t *ScrBuf;

void InitializeLibAV(uint16_t Width, uint16_t Height, const char* url, uint32_t *ScrBuffer){
	printf("Loading LibAV Module\n");
	avcodec_register_all();
	av_register_all();
	
        avformat_network_init();
	AVOutputFormat  *OFormat = NULL;
	AVCodecContext	*AudioContext = NULL;
	AVCodecContext	*VideoContext = NULL;
	ScrBuf = (uint8_t*)ScrBuffer;

	Muxer = avformat_alloc_context();
	OFormat = av_guess_format("flv", NULL, NULL);
	if(OFormat == NULL){
		printf("OFormat = av_guess_format('flv', NULL, NULL); FAIL!\n");
		fail = 1;
		return;
	}
	Muxer->oformat = OFormat;
	snprintf(Muxer->filename, sizeof(Muxer->filename), "%s", url);

	avio_open(&Muxer->pb, url, AVIO_FLAG_WRITE);
/* VIDEO Setup */
	VideoCodec = avcodec_find_encoder(AV_CODEC_ID_H264);
	if(VideoCodec == NULL){
		printf("VideoCodec = avcodec_find_encoder(AV_CODEC_ID_H264); FAIL!\n");
		fail = 1;
		return;
	}

	VideoStream = avformat_new_stream(Muxer, VideoCodec);
	if(VideoStream == NULL){
		printf("VideoStream = avformat_new_stream(Muxer, VideoCodec); FAIL!\n");
		fail = 1;
		return;
	}

	VideoStream->id = Muxer->nb_streams-1;
	VideoContext = VideoStream->codec;  // c
	VideoContext->bit_rate = VIDEO_BIT_RATE;
	/* Resolution must be a multiple of two. */
	VideoContext->width    = (Width%2==0)  ? Width  : Width+1;
	VideoContext->height   = (Height%2==0) ? Height : Height+1;
	VideoContext->time_base.den = VIDEO_FRAME_RATE;
	VideoContext->time_base.num = 1;
	VideoContext->gop_size = 12; /* emit one intra frame every twelve frames at most */
	VideoContext->pix_fmt = AV_PIX_FMT_YUV420P;
	VideoContext->sample_rate = 1;
	if (Muxer->oformat->flags & AVFMT_GLOBALHEADER)
		VideoContext->flags |= CODEC_FLAG_GLOBAL_HEADER;
	printf("VIDEO Setup\n");
/* AUDIO Setup */	
	AudioCodec = avcodec_find_encoder(AV_CODEC_ID_MP3); // AV_CODEC_ID_AAC AV_CODEC_ID_MP3
	if(AudioCodec == NULL){
		printf("AudioCodec = avcodec_find_encoder(AV_CODEC_ID_MP3); FAIL!\n");
		fail = 1;
		return;
	}
	AudioStream = avformat_new_stream(Muxer, AudioCodec);
	if(AudioStream == NULL){
		printf("AudioStream = avformat_new_stream(Muxer, AudioCodec); FAIL!\n");
		fail = 1;
		return;
	}
	AudioStream->id = Muxer->nb_streams-1;

	AudioContext = AudioStream->codec;
	AudioContext->sample_fmt  = AV_SAMPLE_FMT_S16P;
	AudioContext->bit_rate    = AUDIO_BIT_RATE;
	AudioContext->sample_rate = AUDIO_SAMPLING_RATE;
	AudioContext->channels    = 2;
	
	if (Muxer->oformat->flags & AVFMT_GLOBALHEADER)
		AudioContext->flags |= CODEC_FLAG_GLOBAL_HEADER;
	printf("AUDIO Setup\n");
	av_dump_format(Muxer, 0, url, 1);
/* Opening VIDEO */	
	if(avcodec_open2(VideoContext, VideoCodec, NULL) != 0){
		printf("avcodec_open2(VideoContext, VideoCodec, NULL); FAIL!\n");
		fail = 1;
		return;
	}
	if((VFrame = avcodec_alloc_frame())== NULL){
		printf("VFrame = avcodec_alloc_frame() FAIL!\n");
		fail = 1;
		return;
	}
	avpicture_alloc(&dst_picture, VideoContext->pix_fmt,
			VideoContext->width, VideoContext->height);
	VFrame = (AVFrame*)&dst_picture;
	av_init_packet(&VideoPKT);
	VFrame->pts = AV_NOPTS_VALUE;

/* Opening AUDIO */
	if(avcodec_open2(AudioContext, AudioCodec, NULL) != 0){
		printf("avcodec_open2(AudioContext, AudioCodec, NULL); FAIL!\n");
		fail = 1;
		return;
	}
	if((AFrame = avcodec_alloc_frame())== NULL){
		printf("AFrame = avcodec_alloc_frame() FAIL!\n");
		fail = 1;
		return;
	}
	AFrame->nb_samples = 1;
	av_init_packet(&AudioPKT);

//	av_dump_format(oc, 0, filename, 1); //??
	avformat_write_header(Muxer, NULL);
	printf("LibAV Module Running\n");
}

void StopLibAV(){
	if(Muxer!=NULL){
		av_write_trailer(Muxer);
		avio_close(Muxer->pb);
	}
	if(AFrame!=NULL)
		av_free(&AFrame);
	if(AudioStream->codec!=NULL)
		avcodec_close(AudioStream->codec);
	if(VideoStream->codec!=NULL)
		avcodec_close(VideoStream->codec);
	av_free(dst_picture.data[0]);
	if(VFrame!=NULL)
		av_free(VFrame);
	if(Muxer!=NULL)
		avformat_free_context(Muxer);
	avformat_network_deinit();
	printf("Closing LibAV Module\n");
}

void RBGtoYUV420p(uint8_t *rgb, int width, int height){
    unsigned char *y, *u, *v;
    unsigned char *r, *g, *b;
    int i, loop;

    b = rgb;
    g = b + 1;
    r = g + 1;
    y = dst_picture.data[0];
    u = dst_picture.data[1];
    v = dst_picture.data[2];
//    memset(u, 0, width * height / 4);
  //  memset(v, 0, width * height / 4);

    for (loop = 0; loop < height; loop++) {
        for (i = 0; i < width; i += 2) {
            *y++ = (9796 ** r + 19235 ** g + 3736 ** b) >> 15;
            *u += ((-4784 ** r - 9437 ** g + 14221 ** b) >> 17) + 32;
            *v += ((20218 ** r - 16941 ** g - 3277 ** b) >> 17) + 32;
            r += 3;
            g += 3;
            b += 3;
            *y++ = (9796 ** r + 19235 ** g + 3736 ** b) >> 15;
            *u += ((-4784 ** r - 9437 ** g + 14221 ** b) >> 17) + 32;
            *v += ((20218 ** r - 16941 ** g - 3277 ** b) >> 17) + 32;
            r += 3;
            g += 3;
            b += 3;
            u++;
            v++;
        }

        if ((loop & 1) == 0) {
            u -= width / 2;
            v -= width / 2;
        }
    }
}

void MuxingLoop(){
	if(fail)
		return;
	printf("Muxing1\n");
 	AudioTime = AudioStream ? AudioStream->pts.val * av_q2d(AudioStream->time_base) : 0.0;
	AudioTime = VideoStream ? VideoStream->pts.val * av_q2d(VideoStream->time_base) : 0.0;

		/* AUDIO */
	avcodec_fill_audio_frame(AFrame, 2, AV_SAMPLE_FMT_S16, AudioLoop(), BUFFER_SIZE, 0);
	avcodec_encode_audio2(AudioStream->codec, &AudioPKT, AFrame, &got_packet);
	if(got_packet){
		AudioPKT.stream_index = AudioStream->index;
		av_interleaved_write_frame(Muxer, &AudioPKT);
	}

		/* VIDEO */
	RBGtoYUV420p(ScrBuf, VideoStream->codec->width, VideoStream->codec->height);

	avcodec_encode_video2(VideoStream->codec, &VideoPKT, VFrame, &got_packet);
	if(got_packet){
		VideoPKT.stream_index = VideoStream->index;
		if (VideoPKT.dts != AV_NOPTS_VALUE)
			VideoPKT.dts = av_rescale_q(VideoPKT.dts, VideoStream->codec->time_base, VideoStream->time_base);

		av_interleaved_write_frame(Muxer, &VideoPKT);
		VFrame->pts += VideoPKT.dts;
	}
//	av_write_trailer(Muxer);
	printf("Muxing2\n");
}

