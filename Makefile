CXXFLAGS	= -fPIC -O2 `sdl-config --cflags` -D__STDC_CONSTANT_MACROS 
CXX		= g++
LDLIBS  	= -lm `sdl-config --libs` -lX11 -lXext -lXmu -lpulse-simple -lpulse  -lavformat -lavutil -lavcodec -lavfilter -lavformat -lavutil -lavcodec -lavfilter -lpthread -lrtmp -lx264 -lmp3lame -lz -lm
FILES		= main SDL_Window X11_Device Webcam_Device Pulse LibAV

OBJS		= $(addsuffix .o, $(FILES))

all: $(OBJS)
	$(CXX) -o Twitch-Streamer-Linux $(OBJS) $(LDLIBS) -ggdb 

$(OBJS): %.o: %.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $< -ggdb

clean:
	-rm -f $(OBJS) *~ Twitch-Streamer-Linux


