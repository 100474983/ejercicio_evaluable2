#ifndef LINES_H
#define LINES_H

int sendMessage(int socket, char *buffer, int len);
int recvMessage(int socket, char *buffer, int len);

#endif