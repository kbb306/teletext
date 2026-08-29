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

int main (int argc, char* argv[]) {
  FILE* ifp;
  Teletext* teletext;
  unsigned char page[MAX_LINES][MAX_LENGTH];
  char* endptr;
  long port;
  long pageNumber;

  ifp = NULL;

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

    if (VBIT2_capture_page(argv[2], (int)port, (int)pageNumber, page) != 0) {
      return EXIT_FAILURE;
    }

    ifp = tmpfile();
    if (ifp == NULL) {
      ON_ERROR("Unable to create temporary page file.\n");
    }

    if (fwrite(page, 1, MAX_LINES * MAX_LENGTH, ifp) !=
        (size_t)(MAX_LINES * MAX_LENGTH)) {
      fclose(ifp);
      ON_ERROR("Unable to write captured teletext page.\n");
    }
    rewind(ifp);
  }
  else if (argc == 2) {
    ifp = fopen(argv[1], "rb");
    if (ifp == NULL) {
      ON_ERROR("Teletext file not found.\n");
    }
  }
  else {
    usage(argv[0]);
    return EXIT_FAILURE;
  }

  teletext = TLT_init(ifp);
  fclose(ifp);

  /* Render teletext with SDL */
  TLT_SDL_render(teletext);
  TLT_free(&teletext);

  return 0;
}
