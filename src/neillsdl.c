#include "neillsdl.h"

/* Set up a simple SDL 1.2 software surface. */
void Neill_SDL_Init(SDL_Simplewin *sw)
{
   if (SDL_Init(SDL_INIT_VIDEO) != 0) {
      fprintf(stderr, "\nUnable to initialize SDL: %s\n", SDL_GetError());
      SDL_Quit();
      exit(1);
   }

   sw->finished = 0;
   sw->screen = SDL_SetVideoMode(WWIDTH, WHEIGHT, 32, SDL_SWSURFACE);
   if (sw->screen == NULL) {
      fprintf(stderr, "\nUnable to initialize SDL video mode: %s\n",
         SDL_GetError());
      SDL_Quit();
      exit(1);
   }

   SDL_WM_SetCaption("Teletext", "Teletext");

   sw->drawColour = SDL_MapRGB(sw->screen->format, 0, 0, 0);
   if (SDL_FillRect(sw->screen, NULL, sw->drawColour) != 0) {
      fprintf(stderr, "\nUnable to clear SDL surface: %s\n", SDL_GetError());
      SDL_Quit();
      exit(1);
   }
}

/* Present the completed surface.  The SDL1.2-SIXEL driver handles output. */
void Neill_SDL_UpdateScreen(SDL_Simplewin *sw)
{
   if (SDL_Flip(sw->screen) != 0) {
      fprintf(stderr, "\nUnable to update SDL surface: %s\n", SDL_GetError());
   }
}

/* Gobble all events and stop on quit, mouse button, or key press. */
void Neill_SDL_Events(SDL_Simplewin *sw)
{
   SDL_Event event;

   while (SDL_PollEvent(&event)) {
      switch (event.type) {
         case SDL_QUIT:
         case SDL_MOUSEBUTTONDOWN:
         case SDL_KEYDOWN:
            sw->finished = 1;
            break;
         default:
            break;
      }
   }
}

/* Store the current drawing colour in the surface's native pixel format. */
void Neill_SDL_SetDrawColour(SDL_Simplewin *sw, Uint8 r, Uint8 g, Uint8 b)
{
   sw->drawColour = SDL_MapRGB(sw->screen->format, r, g, b);
}

/* SDL 1.2 has no renderer point primitive; fill a one-pixel rectangle. */
void Neill_SDL_DrawPoint(SDL_Simplewin *sw, int x, int y)
{
   SDL_Rect point;

   if (x < 0 || y < 0 || x >= sw->screen->w || y >= sw->screen->h) {
      return;
   }

   point.x = x;
   point.y = y;
   point.w = 1;
   point.h = 1;
   SDL_FillRect(sw->screen, &point, sw->drawColour);
}

void Neill_SDL_FillRect(SDL_Simplewin *sw, SDL_Rect *rect)
{
   SDL_FillRect(sw->screen, rect, sw->drawColour);
}

void Neill_SDL_DrawString(SDL_Simplewin *sw,
   fntrow fontdata[FNTCHARS][FNTHEIGHT],
   char *str, int ox, int oy)
{
   int i;
   unsigned char chr;

   i = 0;
   do {
      chr = str[i++];
      Neill_SDL_DrawChar(sw, fontdata, chr, ox+i*FNTWIDTH, oy);
   } while (str[i]);
}

void Neill_SDL_DrawChar(SDL_Simplewin *sw,
   fntrow fontdata[FNTCHARS][FNTHEIGHT],
   unsigned char chr, int ox, int oy)
{
   unsigned x;
   unsigned y;

   for (y = 0; y < FNTHEIGHT; y++) {
      for (x = 0; x < FNTWIDTH; x++) {
         if (fontdata[chr-FNT1STCHAR][y] >>
            (FNTWIDTH - 1 - x) & 1) {
            Neill_SDL_SetDrawColour(sw, 255, 255, 255);
         }
         else {
            Neill_SDL_SetDrawColour(sw, 0, 0, 0);
         }
         Neill_SDL_DrawPoint(sw, x + ox, y + oy);
      }
   }
}

void Neill_SDL_ReadFont(fntrow fontdata[FNTCHARS][FNTHEIGHT], char *fname)
{
   FILE *fp;
   size_t itms;

   fp = fopen(fname, "rb");
   if (!fp) {
      fprintf(stderr, "Can't open Font file %s\n", fname);
      exit(1);
   }

   itms = fread(fontdata, sizeof(fntrow), FNTCHARS*FNTHEIGHT, fp);
   if (itms != FNTCHARS*FNTHEIGHT) {
      fprintf(stderr, "Can't read all Font file %s (%d) \n",
         fname, (int)itms);
      fclose(fp);
      exit(1);
   }

   fclose(fp);
}
