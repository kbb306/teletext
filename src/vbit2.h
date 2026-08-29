#ifndef VBIT2_H_
#define VBIT2_H_

#include "token.h"

/*
 * Capture one Level 1 teletext page from a VBIT2 TCP packet server.
 * pageNumber is the usual three-digit hexadecimal teletext number
 * (for example 0x100 or 0x8FF).
 */
int VBIT2_capture_page(const char *host, int port, int pageNumber,
   unsigned char page[MAX_LINES][MAX_LENGTH]);

#endif /* VBIT2_H_ */
