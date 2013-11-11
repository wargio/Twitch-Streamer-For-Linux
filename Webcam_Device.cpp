#include "Webcam_Device.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <linux/videodev2.h>

#define CLEAR(x) memset(&(x), 0, sizeof(x))
#define CLAMP(x) ( (x)>=0xff ? 0xff : ( (x) <= 0x00 ? 0x00 : ((uint8_t)x) ) )

struct buffer {
        void   *start;
        size_t length;
};

typedef void (ImgConvCallback)(uint8_t *rgb, uint8_t *src);

uint16_t width;
uint16_t height;

struct v4l2_format              format;
struct v4l2_buffer              buf;
struct v4l2_requestbuffers      req;
enum v4l2_buf_type              type;
fd_set                          fds;
struct timeval                  tv;
int                             r, fd = -1;
unsigned int                    i, n_buffers;
char                            out_name[256];
FILE                            *fout;
struct buffer                   *buffers;
uint32_t			*fb;

ImgConvCallback			*CB = NULL;

int device_exist(const char *filename){
  struct stat   buffer;   
  return (stat (filename, &buffer) == 0);
}

static void xioctl(int fh, int request, void *arg){
        int r;
        do {
                r = ioctl(fh, request, arg);
        } while (r == -1 && ((errno == EINTR) || (errno == EAGAIN)));

        if (r == -1) {
                fprintf(stderr, "error %d, %s\n", errno, strerror(errno));
                exit(EXIT_FAILURE);
        }
}


static inline uint32_t YUVtoRGB(float y, float u, float v) {
    int r,g,b;
 
    r = 1.0f*y + (1.402f*v);
    g = 1.0f*y - (0.344f*u - 0.714f*v);
    b = 1.0f*y + (1.772f*u);
    r = r>255? 255 : r<0 ? 0 : r;
    g = g>255? 255 : g<0 ? 0 : g;
    b = b>255? 255 : b<0 ? 0 : b;
    return (r<<16) | (g<<8) | b;
}

static inline uint32_t YUV2toRGB(int y,int u,int v){
	int r,g,b;
	v -= 128;
	u -= 128;
	// Conversion
	r = y + u;
	g = y-u/2-v/8;
	b = y+v;
	// Clamp to 0..1
	if (r < 0) r = 0;
	if (g < 0) g = 0;
	if (b < 0) b = 0;
	if (r > 255) r = 255;
	if (g > 255) g = 255;
	if (b > 255) b = 255;
	return r << 16 | g << 8 | b;
}

static inline void YUV422P(uint8_t *rgb, uint8_t *yuv){
	uint32_t yy, uu, vv, ug_plus_vg, ub, vr;
	uint32_t r,g,b;
	//rgb += width*(height-1)*3;
	uint32_t* pixels = (uint32_t*) rgb;
	uint32_t total = width*height;
	total /= 2;
	while (total--) {
		yy = yuv[0] << 8;
		uu = yuv[1] - 128;
		vv = yuv[3] - 128;
		ug_plus_vg = uu * 88 + vv * 183;
		ub = uu * 454;
		vr = vv * 359;
		r = (yy + vr) >> 8;
		g = (yy - ug_plus_vg) >> 8;
		b = (yy + ub) >> 8;
		rgb[1] = r < 0 ? 0 : (r > 255 ? 255 : (uint8_t)r);
		rgb[2] = g < 0 ? 0 : (g > 255 ? 255 : (uint8_t)g);
		rgb[3] = b < 0 ? 0 : (b > 255 ? 255 : (uint8_t)b);
		yy = yuv[2] << 8;
		r = (yy + vr) >> 8;
		g = (yy - ug_plus_vg) >> 8;
		b = (yy + ub) >> 8;
		rgb[5] = r < 0 ? 0 : (r > 255 ? 255 : (uint8_t)r);
		rgb[6] = g < 0 ? 0 : (g > 255 ? 255 : (uint8_t)g);
		rgb[7] = b < 0 ? 0 : (b > 255 ? 255 : (uint8_t)b);
		yuv += 4;
		rgb += 8;
	}
}

static inline void YUV2(uint8_t *rgb, uint8_t *yuv){
	uint32_t  pixel1, pixel2;
	uint32_t *buf = (uint32_t*)rgb;
	int y1,y2,u,v;
	for(uint16_t i=0;i<height; i++){
		for(uint16_t j=0;j<width; j += 2){
			// Extract yuv components
			y1 = yuv[0];
			v  = yuv[1];
			y2 = yuv[2];
			u  = yuv[3];
			// yuv to rgb
			buf[i* width + j] = YUV2toRGB(y1,u,v);
			buf[i* width + j + 1] = YUV2toRGB(y2,u,v);
			yuv += 4;
		}
	}
}


void InitializeV4L(const char* device, uint16_t w, uint16_t h, int *check_changes){
	printf("Loading Video4Linux Module\n");

	if(!device_exist(device)){
		printf("Webcam %s do not exist. Webcam is disable!\n",device);
		return;
	}

	fd = open(device, O_RDWR | O_NONBLOCK, 0);
        if (fd < 0) {
                perror("Cannot open device");
                exit(EXIT_FAILURE);
        }

        CLEAR(format);
        format.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        format.fmt.pix.width       = w;
        format.fmt.pix.height      = h;
        format.fmt.pix.pixelformat = V4L2_PIX_FMT_RGB24;
        format.fmt.pix.field       = V4L2_FIELD_INTERLACED;
	width  = format.fmt.pix.width;
	height = format.fmt.pix.height;
        xioctl(fd, VIDIOC_S_FMT, &format);
	if ((format.fmt.pix.width != w) || (format.fmt.pix.height != h)){
	        printf("Warning: driver is sending image at %dx%d\n", format.fmt.pix.width, format.fmt.pix.height);
		*check_changes = 1;
		width  = format.fmt.pix.width;
		height = format.fmt.pix.height;
	}else
		*check_changes = 0;
        switch(format.fmt.pix.pixelformat){
		case V4L2_PIX_FMT_RGB24:
			printf("Format V4L2_PIX_FMT_RGB24 %dx%d\n", width, height);
			CB = NULL;
			break;

		case V4L2_PIX_FMT_PJPG:
			printf("Format V4L2_PIX_FMT_PJPG %dx%d\n", width, height);
			printf("Not supported, sorry. Webcam disable\n");
	                fd = -1;
			CB = NULL;
			break;

		case V4L2_PIX_FMT_YUV422P:
			printf("Format V4L2_PIX_FMT_YUV422P %dx%d\n", width, height);
			CB = YUV422P;
			break;

		case V4L2_PIX_FMT_YUYV:
			printf("Format V4L2_PIX_FMT_YUYV %dx%d\n", width, height);
			CB = YUV2;
			break;
		default:
			CB = NULL;
	                printf("Unkown format. Can't proceed. Webcam disable 0x%x\n",format.fmt.pix.pixelformat);
	                fd = -1;
			break;
        }
	
	if(fd >= 0){
		fb = (uint32_t*) malloc(width*height*sizeof(uint32_t));
		CLEAR(req);
		req.count = 2;
		req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		req.memory = V4L2_MEMORY_MMAP;
		xioctl(fd, VIDIOC_REQBUFS, &req);

		buffers = (struct buffer*) calloc(req.count, sizeof(*buffers));
		for (n_buffers = 0; n_buffers < req.count; ++n_buffers) {
		        CLEAR(buf);

		        buf.type        = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		        buf.memory      = V4L2_MEMORY_MMAP;
		        buf.index       = n_buffers;

		        xioctl(fd, VIDIOC_QUERYBUF, &buf);

		        buffers[n_buffers].length = buf.length;
		        buffers[n_buffers].start = mmap(NULL, buf.length,
		                      PROT_READ | PROT_WRITE, MAP_SHARED,
		                      fd, buf.m.offset);

		        if (MAP_FAILED == buffers[n_buffers].start) {
				printf("MMAP failed!!\n");
		                fd = -1;
		        }
		}
		if(fd >= 0){
			for (i = 0; i < n_buffers; ++i) {
				CLEAR(buf);
				buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
				buf.memory = V4L2_MEMORY_MMAP;
				buf.index = i;
				xioctl(fd, VIDIOC_QBUF, &buf);
			}
			type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
			xioctl(fd, VIDIOC_STREAMON, &type);
		}
	}
	printf("Video4Linux Module Running\n");
}

void StopV4L(){
	if (fd < 0)
		return;
        type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        xioctl(fd, VIDIOC_STREAMOFF, &type);
        for (i = 0; i < n_buffers; ++i)
                munmap(buffers[i].start, buffers[i].length);
        close(fd);
	free(fb);
	printf("Closing Video4Linux Module\n");
}

void GetWebcamInfo(uint16_t *w, uint16_t *h){
	if(w && h){
		*w = format.fmt.pix.width;
		*h = format.fmt.pix.height;
	}
}

uint32_t* GetWebcam(){
	if(fd < 0)
		return NULL;
	do {
		FD_ZERO(&fds);
		FD_SET(fd, &fds);
		/* Timeout. */
		tv.tv_sec = 2;
		tv.tv_usec = 0;
		r = select(fd + 1, &fds, NULL, NULL, &tv);
	}while((r == -1 && (errno = EINTR)));
	if (r == -1) {
		printf("select failed!!\n");
		fd = -1;
		return NULL;
	}
	CLEAR(buf);
	buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	buf.memory = V4L2_MEMORY_MMAP;
	xioctl(fd, VIDIOC_DQBUF, &buf);
	if(CB != NULL)
		CB((uint8_t *)fb,(uint8_t *)buffers[buf.index].start);
	else
		memcpy(fb,buffers[buf.index].start,buf.bytesused);
	xioctl(fd, VIDIOC_QBUF, &buf);

	return fb;
}


