CC         = gcc
OPTFLAGS   = -Wall -O2
SDL_LIBS   = `sdl2-config --static-libs`
SDL_CFLAGS = `sdl2-config --cflags`
CFLAGS     = $(OPTFLAGS)  $(SDL_CFLAGS)
LDFLAGS    = $(SDL_LIBS) 

PLAYSRC    = display.c vision.c plan.c replay.c
PLAYOBJ    = $(PLAYSRC:.c=.o)

all: play

%.o: %.c %.h
	$(CC) $(CFLAGS)  -c -o $@ $<

play: $(PLAYOBJ)
	$(CC) -o $@ $(PLAYOBJ) $(LDFLAGS)

clean:
	rm -f $(PLAYOBJ) play

