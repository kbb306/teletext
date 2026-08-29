CFLAGS = -Wall -Wextra -Werror -Wfloat-equal -pedantic -ansi
SDL_CONFIG ?= /usr/local/bin/sdl-config
SDL_CFLAGS = `$(SDL_CONFIG) --cflags`
SDL_LIBS = `$(SDL_CONFIG) --libs`
INCS = teletext.h token.h tokenizer.h decoder.h translator.h SDLrenderer.h neillsdl.h rgb_colors.h

all: teletext

teletext:	main.c teletext.c tokenizer.c decoder.c translator.c SDLrenderer.c neillsdl.c $(INCS)
	$(CC) main.c teletext.c tokenizer.c decoder.c translator.c SDLrenderer.c neillsdl.c -o teletext -g -lm -O3 $(CFLAGS) $(SDL_CFLAGS) $(SDL_LIBS)

run: all
	./teletext samples/test.m7

lfc: teletext
	./teletext samples/lfc.m7

panda: teletext
	./teletext samples/panda.m7

clean:
	rm -f teletext

.PHONY: clean run all lfc panda
