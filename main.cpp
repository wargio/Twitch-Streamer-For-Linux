#include "SDL_Window.h"
#include <stdlib.h>

int main(int argc, char** argv){
	InitializeSDL();
	while(Flip()){
	}
	StopSDL();

	return 0;
}


