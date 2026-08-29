#include "vbit2.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <netdb.h>
#include <netinet/in.h>
#include <unistd.h>

/* Teletext Hamming 8/4 codewords, indexed by decoded nibble. */
static const unsigned char hamming84[16] = {
   0x15, 0x02, 0x49, 0x5e,
   0x64, 0x73, 0x38, 0x2f,
   0xd0, 0xc7, 0x8c, 0x9b,
   0xa1, 0xb6, 0xfd, 0xea
};

static int hamming84_decode(unsigned char value)
{
   int i;

   for (i = 0; i < 16; i++) {
      if (hamming84[i] == value) {
         return i;
      }
   }

   return -1;
}

static int connect_server(const char *hostname, int port)
{
   struct hostent *host;
   struct sockaddr_in address;
   int sock;

   host = gethostbyname(hostname);
   if (host == NULL || host->h_addr_list[0] == NULL) {
      fprintf(stderr, "Unable to resolve VBIT2 host %s\n", hostname);
      return -1;
   }

   sock = socket(AF_INET, SOCK_STREAM, 0);
   if (sock < 0) {
      perror("Unable to create VBIT2 socket");
      return -1;
   }

   memset(&address, 0, sizeof(address));
   address.sin_family = AF_INET;
   address.sin_port = htons((unsigned short)port);
   memcpy(&address.sin_addr, host->h_addr_list[0],
      (size_t)host->h_length);

   if (connect(sock, (struct sockaddr *)&address, sizeof(address)) < 0) {
      perror("Unable to connect to VBIT2 packet server");
      close(sock);
      return -1;
   }

   return sock;
}

static int packet_address(const unsigned char *packet, int *mag, int *row)
{
   int a;
   int b;

   a = hamming84_decode(packet[0]);
   b = hamming84_decode(packet[1]);
   if (a < 0 || b < 0) {
      return -1;
   }

   *mag = a & 0x07;
   *row = ((a >> 3) & 0x01) | (b << 1);
   return 0;
}

static int header_page_number(const unsigned char *packet)
{
   int units;
   int tens;

   units = hamming84_decode(packet[2]);
   tens = hamming84_decode(packet[3]);
   if (units < 0 || tens < 0) {
      return -1;
   }

   return (tens << 4) | units;
}

static void finish_page(VBIT2_Client *client)
{
   memcpy(client->completed, client->working,
      (size_t)(MAX_LINES * MAX_LENGTH));
   client->pageReady = 1;
}

static void start_page(VBIT2_Client *client, const unsigned char *packet)
{
   int i;

   memset(client->working, 0x20, (size_t)(MAX_LINES * MAX_LENGTH));

   /*
    * Packet 0 contains eight bytes of page addressing/control data followed
    * by 32 display characters.  Those characters occupy columns 8-39.
    */
   for (i = 0; i < 32; i++) {
      client->working[0][i + 8] = packet[i + 10] & 0x7f;
   }

   client->collecting = 1;
}

static void process_packet(VBIT2_Client *client,
   const unsigned char *packet)
{
   int mag;
   int row;
   int packetPage;
   int i;

   if (packet_address(packet, &mag, &row) != 0) {
      return;
   }

   if (row == 0 && mag == client->targetMagazine) {
      packetPage = header_page_number(packet);
      if (packetPage < 0) {
         return;
      }

      /*
       * A new header in the selected magazine terminates the page currently
       * being collected.  This also handles a repeating target page and
       * subpage carousels: finish the old transmission, then begin the new
       * one if the header is for our target page.
       */
      if (client->collecting) {
         finish_page(client);
         client->collecting = 0;
      }

      if (packetPage == client->targetPage) {
         start_page(client, packet);
      }

      return;
   }

   if (client->collecting && mag == client->targetMagazine &&
       row > 0 && row < MAX_LINES) {
      for (i = 0; i < MAX_LENGTH; i++) {
         client->working[row][i] = packet[i + 2] & 0x7f;
      }
   }
}

static void process_frame(VBIT2_Client *client)
{
   int p;
   const unsigned char *packet;

   for (p = 0; p < VBIT2_PACKETS_PER_FRAME; p++) {
      packet = client->frame + (p * VBIT2_PACKET_SIZE);
      process_packet(client, packet);
   }
}

int VBIT2_set_page(VBIT2_Client *client, int pageNumber)
{
   int magazine;

   magazine = (pageNumber >> 8) & 0x0f;
   if (magazine < 1 || magazine > 8 ||
       (pageNumber & 0xff) == 0xff) {
      return -1;
   }

   client->targetPageNumber = pageNumber;
   client->targetPage = pageNumber & 0xff;
   client->targetMagazine = magazine == 8 ? 0 : magazine;
   client->collecting = 0;
   client->pageReady = 0;

   memset(client->working, 0x20, (size_t)(MAX_LINES * MAX_LENGTH));
   memset(client->completed, 0x20, (size_t)(MAX_LINES * MAX_LENGTH));

   return 0;
}

int VBIT2_open(VBIT2_Client *client, const char *host, int port,
   int pageNumber)
{
   memset(client, 0, sizeof(*client));
   client->sock = -1;

   if (VBIT2_set_page(client, pageNumber) != 0) {
      fprintf(stderr, "Invalid teletext page %03X\n",
         (unsigned int)pageNumber);
      return -1;
   }

   client->sock = connect_server(host, port);
   if (client->sock < 0) {
      return -1;
   }

   return 0;
}

int VBIT2_poll(VBIT2_Client *client,
   unsigned char page[MAX_LINES][MAX_LENGTH], int timeoutMs)
{
   fd_set readfds;
   struct timeval timeout;
   int ready;
   int received;

   if (client->sock < 0) {
      return -1;
   }

   if (client->pageReady) {
      memcpy(page, client->completed,
         (size_t)(MAX_LINES * MAX_LENGTH));
      client->pageReady = 0;
      return 1;
   }

   FD_ZERO(&readfds);
   FD_SET(client->sock, &readfds);

   timeout.tv_sec = timeoutMs / 1000;
   timeout.tv_usec = (timeoutMs % 1000) * 1000;

   ready = select(client->sock + 1, &readfds, NULL, NULL, &timeout);
   if (ready < 0) {
      if (errno == EINTR) {
         return 0;
      }
      perror("VBIT2 select");
      return -1;
   }

   if (ready == 0) {
      return 0;
   }

   received = recv(client->sock,
      (char *)(client->frame + client->frameBytes),
      VBIT2_FRAME_SIZE - client->frameBytes, 0);

   if (received == 0) {
      fprintf(stderr, "VBIT2 packet server disconnected.\n");
      return -1;
   }

   if (received < 0) {
      if (errno == EINTR || errno == EAGAIN) {
         return 0;
      }
      perror("VBIT2 recv");
      return -1;
   }

   client->frameBytes += received;

   if (client->frameBytes == VBIT2_FRAME_SIZE) {
      process_frame(client);
      client->frameBytes = 0;

      if (client->pageReady) {
         memcpy(page, client->completed,
            (size_t)(MAX_LINES * MAX_LENGTH));
         client->pageReady = 0;
         return 1;
      }
   }

   return 0;
}

void VBIT2_close(VBIT2_Client *client)
{
   if (client->sock >= 0) {
      close(client->sock);
      client->sock = -1;
   }
}

int VBIT2_capture_page(const char *host, int port, int pageNumber,
   unsigned char page[MAX_LINES][MAX_LENGTH])
{
   VBIT2_Client client;
   int status;

   if (VBIT2_open(&client, host, port, pageNumber) != 0) {
      return -1;
   }

   fprintf(stderr, "Waiting for teletext page %03X from %s:%d...\n",
      (unsigned int)pageNumber, host, port);

   for (;;) {
      status = VBIT2_poll(&client, page, 1000);
      if (status > 0) {
         VBIT2_close(&client);
         return 0;
      }
      if (status < 0) {
         VBIT2_close(&client);
         return -1;
      }
   }
}
