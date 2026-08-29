#ifndef VBIT2_H_
#define VBIT2_H_

#include "token.h"

#define VBIT2_PACKET_SIZE 42
#define VBIT2_PACKETS_PER_FRAME 32
#define VBIT2_FRAME_SIZE (VBIT2_PACKET_SIZE * VBIT2_PACKETS_PER_FRAME)

typedef struct VBIT2_Client {
   int sock;
   int targetPageNumber;
   int targetMagazine;
   int targetPage;
   int collecting;
   int pageReady;
   int frameBytes;
   unsigned char frame[VBIT2_FRAME_SIZE];
   unsigned char working[MAX_LINES][MAX_LENGTH];
   unsigned char completed[MAX_LINES][MAX_LENGTH];
   int workingFastext[6];
   int completedFastext[6];
   int workingFastextValid[6];
   int completedFastextValid[6];
} VBIT2_Client;

/* Open one persistent connection to a VBIT2 TCP packet server. */
int VBIT2_open(VBIT2_Client *client, const char *host, int port,
   int pageNumber);

/* Change the page being collected without reconnecting. */
int VBIT2_set_page(VBIT2_Client *client, int pageNumber);

/*
 * Process network data for up to timeoutMs milliseconds.
 * Returns 1 when a complete selected page is copied to page,
 * 0 when no complete page is ready, and -1 if the connection is lost.
 */
int VBIT2_poll(VBIT2_Client *client,
   unsigned char page[MAX_LINES][MAX_LENGTH], int timeoutMs);

/*
 * Return one of the six FLOF/Fastext links attached to the most recently
 * completed selected page.  Link order is red, green, yellow, cyan,
 * unused, index.  Returns 1 for a usable link and 0 when absent/null.
 */
int VBIT2_get_fastext(VBIT2_Client *client, int link, int *pageNumber);

void VBIT2_close(VBIT2_Client *client);

/* Legacy one-shot helper retained for simple callers. */
int VBIT2_capture_page(const char *host, int port, int pageNumber,
   unsigned char page[MAX_LINES][MAX_LENGTH]);

#endif /* VBIT2_H_ */
