#include "SDL_Window.h"
#include <stdlib.h>

int main(int argc, char** argv){
	if(argc!=2){
		printf("usage: %s rtmp://live.twitch.tv/app/live_xxxxxxxx_yyyyyyyyyyyyyyyyyyyyyyyyyyyyyy\n",argv[0]);
		return 1;
	}
	InitializeSDL(argv[1]);
	while(Flip()){
	}
	StopSDL();

	return 0;
}


