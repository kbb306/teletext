CFLAGS = -Wall -Wextra -Werror -Wfloat-equal -pedantic -ansi
SDL_CONFIG ?= /usr/local/bin/sdl-config
SDL_CFLAGS = `$(SDL_CONFIG) --cflags`
SDL_LIBS = `$(SDL_CONFIG) --libs`

PREFIX ?= /usr/local
BINDIR ?= $(PREFIX)/bin
DATADIR ?= $(PREFIX)/share/teletext
DESTDIR ?=

CPPFLAGS = -DTELETEXT_DATADIR=\"$(DATADIR)\"
INCS = teletext.h token.h tokenizer.h decoder.h translator.h SDLrenderer.h neillsdl.h rgb_colors.h vbit2.h

all: teletext

teletext:	main.c teletext.c tokenizer.c decoder.c translator.c SDLrenderer.c neillsdl.c vbit2.c $(INCS)
	$(CC) $(CPPFLAGS) main.c teletext.c tokenizer.c decoder.c translator.c SDLrenderer.c neillsdl.c vbit2.c -o teletext -g -lm -O3 $(CFLAGS) $(SDL_CFLAGS) $(SDL_LIBS)

run: all
	./teletext samples/test.m7

lfc: teletext
	./teletext samples/lfc.m7

panda: teletext
	./teletext samples/panda.m7

install: teletext
	install -d "$(DESTDIR)$(BINDIR)"
	install -m 755 teletext "$(DESTDIR)$(BINDIR)/teletext"
	install -d "$(DESTDIR)$(DATADIR)"
	install -m 644 m7fixed.fnt "$(DESTDIR)$(DATADIR)/m7fixed.fnt"

uninstall:
	rm -f "$(DESTDIR)$(BINDIR)/teletext"
	rm -f "$(DESTDIR)$(DATADIR)/m7fixed.fnt"
	rmdir "$(DESTDIR)$(DATADIR)" 2>/dev/null || true

clean:
	rm -f teletext

.PHONY: clean run all lfc panda install uninstall
