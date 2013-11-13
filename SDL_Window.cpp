#include "SDL_Window.h"
#include <SDL.h>
#include <SDL_thread.h>

SDL_Surface	*screen   = NULL;
SDL_Event	 event;

uint16_t FBWidth = 1280;
uint16_t FBHeight = 720;

uint16_t X11Width  = 720;
uint16_t X11Height = 720;
uint16_t X11XOffset = 0;
uint16_t X11YOffset = 0;

uint16_t WebCamWidth  = 180;
uint16_t WebCamHeight = 120;
uint16_t X11XOffset_web = 0;
uint16_t X11YOffset_web = 0;
int      sw_rescale = 0;
uint8_t  *WOBuffer = NULL; //WebcamOverlayBuffer

void InitializeSDL(const char* twitch_key){
	if(SDL_Init(SDL_INIT_EVERYTHING)) {
		fprintf(stderr, "Could not initialize SDL - %s\n", SDL_GetError());
		exit(1);
	}
	SDL_WM_SetCaption("Twitch Streamer Linux","Twitch Streamer Linux");
	screen = SDL_SetVideoMode(FBWidth, FBHeight, 32, SDL_HWSURFACE);

	InitializeX11(X11XOffset, X11YOffset, &X11Width, &X11Height);

	//software_rescale checks if it's needed a rescale for the webcam overlay
	InitializeV4L("/dev/video1", WebCamWidth, WebCamHeight, &sw_rescale);

	/* just to check if the webcam has accepted the configs, if not, software rescale*/
	if(sw_rescale){
		WOBuffer = (uint8_t*) malloc(WebCamWidth*WebCamHeight*sizeof(uint32_t));
		printf("Software Rescale for Webcam Enabled!\n");
	}
	InitializePulse();
	InitializeLibAV(FBWidth, FBHeight, twitch_key, (uint32_t*)screen->pixels);
}

void StopSDL(){
	if(sw_rescale)
		free(WOBuffer);
	StopLibAV();
	StopPulse();
	StopV4L();
	StopX11();
	SDL_FreeSurface(screen);
	SDL_Quit();
}

static inline void ScaleLine(uint32_t *Target2, uint32_t *Source2, uint32_t SrcWidth, uint32_t TgtWidth){
 //Thanks to: http://www.compuphase.com/graphic/scale.htm
	int NumPixels = TgtWidth;
	int IntPart = SrcWidth / TgtWidth;
	int FractPart = SrcWidth % TgtWidth;
	int E = 0;

	while (NumPixels-- > 0) {
		*Target2++ = *Source2;
		Source2 += IntPart;
		E += FractPart;
		if (E >= (int)TgtWidth) {
			E -= TgtWidth;
			Source2++;
		} /* if */
	} /* while */
}


static inline void DrawWebcam(){
	uint32_t *Target;
	uint32_t *Source;
	if(sw_rescale){
		uint16_t Web_w, Web_h;
		GetWebcamInfo(&Web_w, &Web_h); //Gets the original Output Width/Height

		Source = GetWebcam();
		Target = (uint32_t *)WOBuffer;


		int NumPixels = WebCamHeight;
		int IntPart = (Web_h / WebCamHeight) * Web_w;
		int FractPart = Web_h % WebCamHeight;
		int E = 0;
		uint32_t *PrevSource = NULL;

		while (NumPixels-- > 0) {
			if (Source == PrevSource) {
				memcpy(Target, Target-WebCamWidth, WebCamWidth*sizeof(*Target));
			} else {
				ScaleLine(Target, Source, Web_w, WebCamWidth);
				PrevSource = Source;
			}
			Target += WebCamWidth;
			Source += IntPart;
			E += FractPart;
			if (E >= (int)WebCamHeight) {
				E -= WebCamHeight;
				Source += Web_w;
			}
		}
		Target = (uint32_t *)screen->pixels;
		Source = (uint32_t *)WOBuffer;
	}else{
		Target = (uint32_t *)screen->pixels;
		Source = GetWebcam();
		if(Source == NULL)
			return;
	}

	uint32_t n,width,height;

	Target += X11YOffset_web*FBWidth+X11XOffset_web;

	height = WebCamHeight;
	if((X11YOffset_web+height) >= FBHeight)
		height = FBHeight;

	width = WebCamWidth;
	if((X11XOffset_web+width) >= FBWidth)
		width = FBWidth;

	for(n=0;n < height;n++){
		memcpy(Target, Source, width*sizeof(uint32_t));
		Source+=WebCamWidth;
		Target+=FBWidth;
	}
}

static inline void ResizeBuffer(){
	uint32_t *Target = (uint32_t *)screen->pixels;
	uint32_t *Source = GetX11(X11XOffset, X11YOffset, &X11Width, &X11Height);
	if(Source == NULL)
		return;

	if(FBWidth == X11Width && FBHeight == X11Height){
		memcpy(screen->pixels, Source, FBHeight*FBWidth*sizeof(uint32_t));
		return;
	}
	int NumPixels = FBHeight;
	int IntPart = (X11Height / FBHeight) * X11Width;
	int FractPart = X11Height % FBHeight;
	int E = 0;
	uint32_t *PrevSource = NULL;

	while (NumPixels-- > 0) {
		if (Source == PrevSource) {
			memcpy(Target, Target-FBWidth, FBWidth*sizeof(*Target));
		} else {
			ScaleLine(Target, Source, X11Width, FBWidth);
			PrevSource = Source;
		}
		Target += FBWidth;
		Source += IntPart;
		E += FractPart;
		if (E >= (int)FBHeight) {
			E -= FBHeight;
			Source += X11Width;
		}
	}
}

int Flip(){
	while(SDL_PollEvent(&event)){
		if(event.type == SDL_QUIT){
			return 0;
		}
	}
	ResizeBuffer();
	DrawWebcam();
	AudioLoop();
	MuxingLoop();

	SDL_Flip(screen);
	return 1;
}



