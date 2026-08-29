#ifndef NEILLSDL_H_
#define NEILLSDL_H_

#include <SDL.h>
#include "token.h"

#define WWIDTH (MAX_LENGTH * FNTWIDTH)
#define WHEIGHT (MAX_LINES * FNTHEIGHT)

/* Font stuff */
typedef unsigned short fntrow;
#define FNTWIDTH (sizeof(fntrow)*8)
#define FNTHEIGHT 18
#define FNTCHARS 96
#define FNT1STCHAR 32

/* All info required for the SDL 1.2 surface and event loop. */
struct SDL_Simplewin {
   int finished;
   SDL_Surface *screen;
   Uint32 drawColour;
};
typedef struct SDL_Simplewin SDL_Simplewin;

void Neill_SDL_Init(SDL_Simplewin *sw);
void Neill_SDL_Events(SDL_Simplewin *sw);
void Neill_SDL_SetDrawColour(SDL_Simplewin *sw, Uint8 r, Uint8 g, Uint8 b);
void Neill_SDL_DrawPoint(SDL_Simplewin *sw, int x, int y);
void Neill_SDL_FillRect(SDL_Simplewin *sw, SDL_Rect *rect);
void Neill_SDL_DrawChar(SDL_Simplewin *sw,
   fntrow fontdata[FNTCHARS][FNTHEIGHT],
   unsigned char chr, int ox, int oy);
void Neill_SDL_DrawString(SDL_Simplewin *sw,
   fntrow fontdata[FNTCHARS][FNTHEIGHT],
   char *str, int ox, int oy);
void Neill_SDL_ReadFont(fntrow fontdata[FNTCHARS][FNTHEIGHT], char *fname);
void Neill_SDL_UpdateScreen(SDL_Simplewin *sw);

#endif /* NEILLSDL_H_ */
