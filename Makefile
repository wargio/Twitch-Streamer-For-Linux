CXXFLAGS	= -fPIC -O2 `sdl-config --cflags` -D__STDC_CONSTANT_MACROS 
CXX		= g++
LDLIBS  	= -lavutil -lavformat -lavcodec -lswscale -lz -lm `sdl-config --libs` -lX11 -lXext -lXmu
FILES		= main SDL_Window X11_Device Webcam_Device

OBJS		= $(addsuffix .o, $(FILES))

all: $(OBJS)
	$(CXX) -o Twitch-Streamer-Linux $(OBJS) $(LDLIBS) -ggdb

$(OBJS): %.o: %.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $< -ggdb

clean:
	-rm -f $(OBJS) *~ Twitch-Streamer-Linux


