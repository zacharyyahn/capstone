OPTFLAGS   = -Wall -O2
SDL_LIBS   = `sdl-config --static-libs`
SDL_CFLAGS = `sdl-config --cflags`
CFLAGS     = $(OPTFLAGS)  $(SDL_CFLAGS)
LDFLAGS    = $(SDL_LIBS) 

CSRC       = yay.c fir.c
TARGET     = yay
OBJ        = $(CSRC:.c=.o)

all: fir

%.o: %.c
	$(CC) $(CFLAGS)  -c -o $@ $<

$(TARGET): $(OBJ)
	$(CC) -o $@ $(OBJ) $(LDFLAGS)

fir: $(OBJ)
	$(CC) -o fir $(OBJ) $(LDFLAGS)

clean:
	rm $(OBJ) $(TARGET)

