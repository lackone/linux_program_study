#ifndef UDP_MSG_H
#define UDP_MSG_H

#define LOGIN 1
#define CHAT 2
#define QUIT 3

typedef struct _MSG {
   int type;
   char name[32];
   char text[1024];
} MSG;

typedef struct _Client {
   sockaddr_in addr;
   char name[32];
} Client;

#endif //UDP_MSG_H
