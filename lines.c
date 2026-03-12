#include <unistd.h>
#include <errno.h>
#include "lines.h"

int sendMessage(int socket, char *buffer, int len)
{
    int r;
    int l = len;

    do {
        r = write(socket, buffer, l);
        l = l - r;
        buffer = buffer + r;
    } while ((l > 0) && (r >= 0));

    if (r < 0)
        return -1;
    else
        return 0;
}

int recvMessage(int socket, char *buffer, int len)
{
    int r;
    int l = len;

    do {
        r = read(socket, buffer, l);
        l = l - r;
        buffer = buffer + r;
    } while ((l > 0) && (r >= 0));

    if (r < 0)
        return -1;
    else
        return 0;
}