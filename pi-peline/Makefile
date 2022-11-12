OPTFLAGS   = -Wall -O2
SDL_LIBS   = `sdl-config --static-libs`
SDL_CFLAGS = `sdl-config --cflags`
CFLAGS     = $(OPTFLAGS)  $(SDL_CFLAGS)
LDFLAGS    = $(SDL_LIBS) 

CSRC       = yay.c vision.c plan.c
TARGET     = pi
OBJ        = $(CSRC:.c=.o)

all: $(TARGET)

%.o: %.c
	$(CC) $(CFLAGS)  -c -o $@ $<

$(TARGET): $(OBJ)
	$(CC) -o $@ $(OBJ) $(LDFLAGS)

clean:
	rm $(OBJ) $(TARGET)

