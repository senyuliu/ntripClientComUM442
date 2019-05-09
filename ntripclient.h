#ifndef _NTRIPCLIENT_H
#define _NTRIPCLIENT_H

typedef int sockettype;

typedef struct{
  char dst_host[20];
  char dst_port[10];
  char dst_mount[20];
  char dst_user[20];
  char dst_password[20];
} ntrip_client_t;

extern sockettype sockfd_ntripc;

int  ntripclient_connect(ntrip_client_t *ntrip_cli);
int  ntripclient_receive(char *buff, int nbytes);
int  ntripclient_send(char *buff, int nbytes);
void ntripclient_close(void);

#endif  //_NTRIPCLIENT_H
