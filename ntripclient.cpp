#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <string.h>
#include <errno.h>

#include "ntripclient.h"

#define INVALID_SOCKET	-1

#ifndef MSG_DONTWAIT
#define MSG_DONTWAIT	0 /* prevent compiler errors */
#endif

#define MAXDATASIZE	1024        /* max number of bytes we can get at once */

/* The string, which is send as agent in HTTP request */
#define NTRIPCLIENT_AGENTSTRING	"NTRIP NtripClientPOSIX"

static char ntripclient_revisionstr[] = "$Revision: 1.51 $";

static const char encodingTable [64] = {
  'A','B','C','D','E','F','G','H','I','J','K','L','M','N','O','P',
  'Q','R','S','T','U','V','W','X','Y','Z','a','b','c','d','e','f',
  'g','h','i','j','k','l','m','n','o','p','q','r','s','t','u','v',
  'w','x','y','z','0','1','2','3','4','5','6','7','8','9','+','/'
};

sockettype       sockfd_ntripc = INVALID_SOCKET;



static int encode(char *buf, int size, const char *user, const char *pwd)
{
  unsigned char inbuf[3];
  char *out = buf;
  int i, sep = 0, fill = 0, bytes = 0;

  while(*user || *pwd)
  {
    i = 0;
    while(i < 3 && *user) inbuf[i++] = *(user++);
    if(i < 3 && !sep)	 { inbuf[i++] = ':'; ++sep; }
    while(i < 3 && *pwd)  inbuf[i++] = *(pwd++);
    while(i < 3)         { inbuf[i++] = 0; ++fill; }
    if(out-buf < size-1)
      *(out++) = encodingTable[(inbuf [0] & 0xFC) >> 2];
    if(out-buf < size-1)
      *(out++) = encodingTable[((inbuf [0] & 0x03) << 4)
               | ((inbuf [1] & 0xF0) >> 4)];
    if(out-buf < size-1)
    {
      if(fill == 2)
        *(out++) = '=';
      else
        *(out++) = encodingTable[((inbuf [1] & 0x0F) << 2)
                 | ((inbuf [2] & 0xC0) >> 6)];
    }
    if(out-buf < size-1)
    {
      if(fill >= 1)
        *(out++) = '=';
      else
        *(out++) = encodingTable[inbuf [2] & 0x3F];
    }
    bytes += 4;
  }
  if(out-buf < size)
    *out = 0;
  return bytes;
}


int ntripclient_connect(ntrip_client_t *ntrip_cli)
{
  struct sockaddr_in their_addr; /* connector's address information */
  struct hostent *he;
  //struct servent *se;

  const char *nmeahead = "$GPGGA";
  char buf[MAXDATASIZE];

  char *b;
  long i;

  printf("Ntrip client conneting ...\n");
  memset(&their_addr, 0, sizeof(struct sockaddr_in));
/* 
  if(i = strtol(p_dev->ntrip.client.dst_port, &b, 10))
  {
    their_addr.sin_port = htons(i);
  }
  else
  {
    zlog_error(o, "Can't resolve port %s", p_dev->ntrip.client.dst_port);
    return -1;
  }
*/
  i = strtol(ntrip_cli->dst_port, &b, 10);
  their_addr.sin_port = htons(i);

  if(!(he = gethostbyname(ntrip_cli->dst_host)))
  {
    printf("Server name lookup failed for %s (%s)\n", ntrip_cli->dst_host, strerror(errno));
    return -1;
  }
  
  if((sockfd_ntripc = socket(AF_INET, SOCK_STREAM, 0)) == INVALID_SOCKET)
  {
    printf("Create socket error (%s)\n", strerror(errno));
    return -1;
  }
  else
  {
    their_addr.sin_family = AF_INET;
    their_addr.sin_addr = *((struct in_addr *)he->h_addr);
  }

  if(connect(sockfd_ntripc, (struct sockaddr *)&their_addr, sizeof(struct sockaddr)) == -1)
  {
    printf("Connect error (%s)\n", strerror(errno));
    return -1;
  }

  memset(buf, 0, sizeof(buf));
  i=snprintf(buf, MAXDATASIZE-40, /* leave some space for login */
             "GET /%s HTTP/1.0\r\n"
             "Host: %s\r\n"
             "User-Agent: %s/%s\r\n"
             "%s%s%s"
             "Connection: close%s",
             ntrip_cli->dst_mount, 
             ntrip_cli->dst_host,
             NTRIPCLIENT_AGENTSTRING, ntripclient_revisionstr,
             nmeahead ? "Ntrip-GGA: " : "", nmeahead ? nmeahead : "", nmeahead ? "\r\n" : "",
             (*(ntrip_cli->dst_user) || *(ntrip_cli->dst_password)) ? "\r\nAuthorization: Basic " : "");  
  
  if(i > MAXDATASIZE-40 || i < 0)
  {
    printf("Requested data too long\n");
    return -1;
  }
  else
  {
    i += encode(buf+i, MAXDATASIZE-i-4, ntrip_cli->dst_user, ntrip_cli->dst_password);
    if(i > MAXDATASIZE-4)
    {
      printf("Username and/or password too long\n");
      return -1;
    }
    else
    {
      buf[i++] = '\r';
      buf[i++] = '\n';
      buf[i++] = '\r';
      buf[i++] = '\n';
    }  
    //here add nmea 
  }

  //print for debug
  printf("Ntrip client head:\n%s\n", buf);
/*
  int t;
  printf("\nNtrip client head:\n");
  for(t=0; t<i; t++)
  {
    printf("%c", buf[t]);
  }
*/
  if(send(sockfd_ntripc, buf, (size_t)i, 0) != i)
  {
    printf("Send error (%s)\n", strerror(errno));
    return -1;
  }
  else
  {
    int numbytes;
    //int k = 0;

    while((numbytes = recv(sockfd_ntripc, buf, MAXDATASIZE-1, 0)) > 0)
    {
      buf[numbytes] = 0; /* latest end mark for strstr */
      printf("Destination caster response: %s\n", buf);
/*  
      printf("Destination caster response:");
      for(k = 0; k < numbytes && buf[k] != '\n' && buf[k] != '\r'; k++)
      {
        printf("%c", isprint(buf[k]) ? buf[k] : '.');
      }
      printf("\n");
*/      
      if(strstr(buf, "ICY 200 OK"))
      {
        printf("Ntrip client connected!\n");
        return 1;
      }
      else
      {
        printf("Could not get the requested data");
        return -1;
      }
    }
  }

  return -1;  
}


void ntripclient_close(void)
{
  if(sockfd_ntripc != INVALID_SOCKET)
  {
    close(sockfd_ntripc);
    sockfd_ntripc = INVALID_SOCKET;
    printf("Ntripclient closed!\n");
  }
}


int ntripclient_receive(char *buff, int nbytes)
{
  int rbytes = 0;
  //int i;

  rbytes = recv(sockfd_ntripc, buff, nbytes, MSG_DONTWAIT);
  if(rbytes <= 0)
  {
    printf("Recv error (%s)", strerror(errno));
  }
/*
  if(rbytes > 0)
  {
    printf("[%d bytes]:", rbytes);
    for(i=0; i<rbytes; i++)
      printf("%c", *(rbuf+i));
    printf("\n");
  }
*/

  return rbytes;
}


int ntripclient_send(char *buff, int nbytes)
{
  if((buff == NULL) || (nbytes <= 0))
  {
    printf("Input parameter(s) invalid!\n");
    return 0;
  }
  else
  {
    int n = 0;
    int sbytes = 0;
/*
    int i;
    for(i=0; i<nbytes; i++)
    {
      printf("%c", *(buff+i));
    }
    printf("\n");
*/
    do{
      n = send(sockfd_ntripc, buff+sbytes, (size_t)(nbytes-sbytes), MSG_DONTWAIT);
      if(n < 0)
      {
        printf("Send error (%d: %s)\n", errno, strerror(errno));
        return -1;
      }

      sbytes += n;
    }while(sbytes < nbytes);

    return sbytes;
  }
}

