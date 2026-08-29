#include "vbit2.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <unistd.h>

#define VBIT2_PACKET_SIZE 42
#define VBIT2_PACKETS_PER_FRAME 32
#define VBIT2_FRAME_SIZE (VBIT2_PACKET_SIZE * VBIT2_PACKETS_PER_FRAME)

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

static int recv_exact(int sock, unsigned char *buffer, int length)
{
   int got;
   int n;

   got = 0;
   while (got < length) {
      n = recv(sock, (char *)(buffer + got), length - got, 0);
      if (n <= 0) {
         return -1;
      }
      got += n;
   }

   return 0;
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

int VBIT2_capture_page(const char *host, int port, int pageNumber,
   unsigned char page[MAX_LINES][MAX_LENGTH])
{
   unsigned char frame[VBIT2_FRAME_SIZE];
   unsigned char *packet;
   int sock;
   int targetMagazine;
   int targetPage;
   int mag;
   int row;
   int packetPage;
   int collecting;
   int i;
   int p;

   targetMagazine = (pageNumber >> 8) & 0x0f;
   targetPage = pageNumber & 0xff;

   if (targetMagazine < 1 || targetMagazine > 8) {
      fprintf(stderr, "Invalid teletext page %03X\n", pageNumber);
      return -1;
   }

   /* MRAG represents magazine 8 as zero. */
   if (targetMagazine == 8) {
      targetMagazine = 0;
   }

   sock = connect_server(host, port);
   if (sock < 0) {
      return -1;
   }

   memset(page, 0x20, MAX_LINES * MAX_LENGTH);
   collecting = 0;

   fprintf(stderr, "Waiting for teletext page %03X from %s:%d...\n",
      pageNumber, host, port);

   for (;;) {
      if (recv_exact(sock, frame, VBIT2_FRAME_SIZE) != 0) {
         fprintf(stderr, "VBIT2 packet server disconnected.\n");
         close(sock);
         return -1;
      }

      for (p = 0; p < VBIT2_PACKETS_PER_FRAME; p++) {
         packet = frame + (p * VBIT2_PACKET_SIZE);

         if (packet_address(packet, &mag, &row) != 0) {
            continue;
         }

         if (row == 0 && mag == targetMagazine) {
            packetPage = header_page_number(packet);
            if (packetPage < 0) {
               continue;
            }

            /*
             * Any subsequent header in the selected magazine marks the end
             * of the page transmission we were collecting.
             */
            if (collecting) {
               close(sock);
               return 0;
            }

            if (packetPage == targetPage) {
               memset(page, 0x20, MAX_LINES * MAX_LENGTH);

               /*
                * A packet-0 header has eight addressing/control bytes before
                * the 32 display characters.  Put those 32 characters at
                * screen columns 8-39.
                */
               for (i = 0; i < 32; i++) {
                  page[0][i + 8] = packet[i + 10] & 0x7f;
               }

               collecting = 1;
            }

            continue;
         }

         if (collecting && mag == targetMagazine &&
             row > 0 && row < MAX_LINES) {
            for (i = 0; i < MAX_LENGTH; i++) {
               page[row][i] = packet[i + 2] & 0x7f;
            }
         }
      }
   }
}
