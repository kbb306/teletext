/*
 * Written by Evan Lalopoulos <evan.lalopoulos.2017@my.bristol.ac.uk>
 * Copyright (C) 2018 - All rights reserved.
 * Unauthorized copying of this file is strictly prohibited.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "teletext.h"
#include "vbit2.h"

static void usage(const char *program)
{
  fprintf(stderr, "Usage:\n");
  fprintf(stderr, "  %s page.m7\n", program);
  fprintf(stderr, "  %s --vbit2 host port page\n", program);
  fprintf(stderr, "Example: %s --vbit2 127.0.0.1 19761 100\n", program);
}

static Teletext* teletext_from_page(
  unsigned char page[MAX_LINES][MAX_LENGTH])
{
  FILE* ifp;
  Teletext* teletext;

  ifp = tmpfile();
  if (ifp == NULL) {
    fprintf(stderr, "Unable to create temporary page file.\n");
    return NULL;
  }

  if (fwrite(page, 1, MAX_LINES * MAX_LENGTH, ifp) !=
      (size_t)(MAX_LINES * MAX_LENGTH)) {
    fprintf(stderr, "Unable to write captured teletext page.\n");
    fclose(ifp);
    return NULL;
  }

  rewind(ifp);
  teletext = TLT_init(ifp);
  fclose(ifp);

  return teletext;
}

static int key_to_digit(SDLKey key)
{
  if (key >= SDLK_0 && key <= SDLK_9) {
    return key - SDLK_0;
  }

  if (key >= SDLK_KP0 && key <= SDLK_KP9) {
    return key - SDLK_KP0;
  }

  return -1;
}

static void terminal_beep(void)
{
  fputc('\a', stdout);
  fflush(stdout);
}

static void set_page_caption(int pageNumber)
{
  char caption[64];

  sprintf(caption, "Teletext - %03X", (unsigned int)pageNumber);
  SDL_WM_SetCaption(caption, caption);
}

static void set_waiting_caption(int pageNumber)
{
  char caption[64];

  sprintf(caption, "Teletext - waiting for %03X",
    (unsigned int)pageNumber);
  SDL_WM_SetCaption(caption, caption);
}

static void set_entry_caption(int *digits, int count)
{
  char caption[64];

  if (count == 1) {
    sprintf(caption, "Teletext - %d__", digits[0]);
  }
  else if (count == 2) {
    sprintf(caption, "Teletext - %d%d_", digits[0], digits[1]);
  }
  else {
    sprintf(caption, "Teletext");
  }

  SDL_WM_SetCaption(caption, caption);
}

static void set_no_fastext_caption(int pageNumber, const char *name)
{
  char caption[80];

  sprintf(caption, "Teletext - %03X has no %s link",
    (unsigned int)pageNumber, name);
  SDL_WM_SetCaption(caption, caption);
}

static int fastext_key(SDLKey key, const char **name)
{
  switch (key) {
    case SDLK_r:
      *name = "red";
      return 0;
    case SDLK_g:
      *name = "green";
      return 1;
    case SDLK_y:
      *name = "yellow";
      return 2;
    case SDLK_b:
      *name = "blue";
      return 3;
    case SDLK_i:
      *name = "index";
      return 5;
    default:
      break;
  }

  return -1;
}

static int run_vbit2_viewer(const char *host, int port, int initialPage)
{
  VBIT2_Client client;
  SDL_Simplewin sw;
  fntrow fontdata[FNTCHARS][FNTHEIGHT];
  unsigned char page[MAX_LINES][MAX_LENGTH];
  unsigned char lastPage[MAX_LINES][MAX_LENGTH];
  Teletext* teletext;
  SDL_Event event;
  SDLKey key;
  int digits[3];
  int digit;
  int digitCount;
  int fastextLink;
  int fastextPage;
  const char *fastextName;
  int requestedPage;
  int currentPage;
  int haveLastPage;
  int running;
  int status;
  int result;

  if (VBIT2_open(&client, host, port, initialPage) != 0) {
    return EXIT_FAILURE;
  }

  TLT_SDL_init(&sw, fontdata);
  currentPage = initialPage;
  digitCount = 0;
  haveLastPage = 0;
  running = 1;
  result = EXIT_SUCCESS;

  set_waiting_caption(currentPage);

  while (running) {
    while (SDL_PollEvent(&event)) {
      if (event.type == SDL_QUIT) {
        running = 0;
      }
      else if (event.type == SDL_KEYDOWN) {
        key = event.key.keysym.sym;

        if (key == SDLK_ESCAPE || key == SDLK_q) {
          running = 0;
        }
        else if ((fastextLink = fastext_key(key, &fastextName)) >= 0) {
          digitCount = 0;

          if (VBIT2_get_fastext(&client, fastextLink, &fastextPage)) {
            if (VBIT2_set_page(&client, fastextPage) == 0) {
              currentPage = fastextPage;
              haveLastPage = 0;
              set_waiting_caption(currentPage);
            }
          }
          else {
            terminal_beep();
            set_no_fastext_caption(currentPage, fastextName);
          }
        }
        else if (key == SDLK_BACKSPACE) {
          if (digitCount > 0) {
            digitCount--;
            if (digitCount > 0) {
              set_entry_caption(digits, digitCount);
            }
            else {
              set_page_caption(currentPage);
            }
          }
        }
        else {
          digit = key_to_digit(key);
          if (digit >= 0) {
            /*
             * Normal television Teletext page entry is three digits and
             * magazine numbers run from 1 through 8.
             */
            if (digitCount == 0 && (digit < 1 || digit > 8)) {
              terminal_beep();
              continue;
            }

            digits[digitCount++] = digit;

            if (digitCount < 3) {
              set_entry_caption(digits, digitCount);
            }
            else {
              requestedPage = (digits[0] << 8) |
                              (digits[1] << 4) |
                               digits[2];

              if (VBIT2_set_page(&client, requestedPage) == 0) {
                currentPage = requestedPage;
                haveLastPage = 0;
                set_waiting_caption(currentPage);
              }

              digitCount = 0;
            }
          }
        }
      }
    }

    if (!running) {
      break;
    }

    /*
     * Keep consuming the VBIT2 service while also returning to SDL often
     * enough for terminal keyboard input to remain responsive.
     */
    status = VBIT2_poll(&client, page, 40);
    if (status < 0) {
      result = EXIT_FAILURE;
      break;
    }

    if (status > 0) {
      /*
       * VBIT2 repeats pages continuously.  Only repaint when the selected
       * page actually changes; this gives live clocks and subpages without
       * retransmitting identical SIXEL frames.
       */
      if (!haveLastPage ||
          memcmp(page, lastPage, (size_t)(MAX_LINES * MAX_LENGTH)) != 0) {
        teletext = teletext_from_page(page);
        if (teletext == NULL) {
          result = EXIT_FAILURE;
          break;
        }

        TLT_SDL_render_page(&sw, teletext, fontdata);
        TLT_free(&teletext);

        memcpy(lastPage, page, (size_t)(MAX_LINES * MAX_LENGTH));
        haveLastPage = 1;
        if (digitCount == 0) {
          set_page_caption(currentPage);
        }
      }
    }
  }

  VBIT2_close(&client);
  SDL_Quit();

  return result;
}

int main (int argc, char* argv[]) {
  FILE* ifp;
  Teletext* teletext;
  char* endptr;
  long port;
  long pageNumber;

  if (argc == 5 && strcmp(argv[1], "--vbit2") == 0) {
    endptr = NULL;
    port = strtol(argv[3], &endptr, 10);
    if (endptr == argv[3] || *endptr != '\0' || port < 1 || port > 65535) {
      fprintf(stderr, "Invalid VBIT2 port: %s\n", argv[3]);
      return EXIT_FAILURE;
    }

    endptr = NULL;
    pageNumber = strtol(argv[4], &endptr, 16);
    if (endptr == argv[4] || *endptr != '\0' ||
        pageNumber < 0x100 || pageNumber > 0x8fe ||
        (pageNumber & 0xff) == 0xff) {
      fprintf(stderr, "Invalid teletext page: %s\n", argv[4]);
      return EXIT_FAILURE;
    }

    return run_vbit2_viewer(argv[2], (int)port, (int)pageNumber);
  }

  if (argc == 2) {
    ifp = fopen(argv[1], "rb");
    if (ifp == NULL) {
      ON_ERROR("Teletext file not found.\n");
    }

    teletext = TLT_init(ifp);
    fclose(ifp);

    TLT_SDL_render(teletext);
    TLT_free(&teletext);

    return EXIT_SUCCESS;
  }

  usage(argv[0]);
  return EXIT_FAILURE;
}
