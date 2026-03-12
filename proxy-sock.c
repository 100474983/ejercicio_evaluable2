#include <arpa/inet.h>
#include <errno.h>
#include <netinet/in.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#include "claves.h"
#include "lines.h"

#define DEFAULT_SERVER_IP "127.0.0.1"
#define DEFAULT_SERVER_PORT 4200

enum {
    OP_SET_VALUE = 1,
    OP_GET_VALUE = 2,
    OP_MODIFY_VALUE = 3,
    OP_DELETE_KEY = 4,
    OP_EXIST = 5,
    OP_DESTROY = 6
};

static int send_int32(int sd, int32_t value) {
    int32_t net_value = htonl(value);
    return sendMessage(sd, (char *)&net_value, sizeof(net_value));
}

static int recv_int32(int sd, int32_t *value) {
    int32_t net_value;
    if (recvMessage(sd, (char *)&net_value, sizeof(net_value)) < 0) {
        return -1;
    }
    *value = ntohl(net_value);
    return 0;
}

static int send_float_array(int sd, const float *values, int n_values) {
    for (int i = 0; i < n_values; i++) {
        uint32_t bits;
        memcpy(&bits, &values[i], sizeof(bits));
        bits = htonl(bits);
        if (sendMessage(sd, (char *)&bits, sizeof(bits)) < 0) {
            return -1;
        }
    }
    return 0;
}

static int recv_float_array(int sd, float *values, int n_values) {
    for (int i = 0; i < n_values; i++) {
        uint32_t bits;
        if (recvMessage(sd, (char *)&bits, sizeof(bits)) < 0) {
            return -1;
        }
        bits = ntohl(bits);
        memcpy(&values[i], &bits, sizeof(bits));
    }
    return 0;
}

static int send_fixed_string(int sd, const char *value) {
    char buffer[256];
    memset(buffer, 0, sizeof(buffer));
    if (value != NULL) {
        strncpy(buffer, value, sizeof(buffer) - 1);
    }
    return sendMessage(sd, buffer, sizeof(buffer));
}

static int recv_fixed_string(int sd, char *value) {
    if (recvMessage(sd, value, 256) < 0) {
        return -1;
    }
    value[255] = '\0';
    return 0;
}

static int read_server_port(void) {
    const char *port_env = getenv("PORT_TUPLAS");
    if (port_env == NULL || *port_env == '\0') {
        return DEFAULT_SERVER_PORT;
    }

    char *endptr = NULL;
    long parsed = strtol(port_env, &endptr, 10);
    if (endptr == port_env || *endptr != '\0' || parsed < 1 || parsed > 65535) {
        return DEFAULT_SERVER_PORT;
    }
    return (int)parsed;
}

static int connect_server(void) {
    int sd = -1;
    struct sockaddr_in server_addr;
    const char *server_ip = getenv("IP_TUPLAS");
    int server_port = read_server_port();

    if (server_ip == NULL || *server_ip == '\0') {
        server_ip = DEFAULT_SERVER_IP;
    }

    sd = socket(AF_INET, SOCK_STREAM, 0);
    if (sd < 0) {
        return -1;
    }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons((uint16_t)server_port);
    if (inet_pton(AF_INET, server_ip, &server_addr.sin_addr) != 1) {
        close(sd);
        return -1;
    }

    if (connect(sd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        close(sd);
        return -1;
    }

    return sd;
}

int destroy(void) {
    int sd = connect_server();
    int32_t result;

    if (sd < 0) {
        return -1;
    }
    if (send_int32(sd, OP_DESTROY) < 0 || recv_int32(sd, &result) < 0) {
        close(sd);
        return -1;
    }

    close(sd);
    return (int)result;
}

int set_value(char *key, char *value1, int N_value2, float *V_value2, struct Paquete value3) {
    int sd = -1;
    int32_t result;

    if (key == NULL || value1 == NULL || V_value2 == NULL) {
        return -1;
    }

    sd = connect_server();
    if (sd < 0) {
        return -1;
    }

    if (send_int32(sd, OP_SET_VALUE) < 0 ||
        send_fixed_string(sd, key) < 0 ||
        send_fixed_string(sd, value1) < 0 ||
        send_int32(sd, N_value2) < 0 ||
        send_float_array(sd, V_value2, 32) < 0 ||
        send_int32(sd, value3.x) < 0 ||
        send_int32(sd, value3.y) < 0 ||
        send_int32(sd, value3.z) < 0 ||
        recv_int32(sd, &result) < 0) {
        close(sd);
        return -1;
    }

    close(sd);
    return (int)result;
}

int get_value(char *key, char *value1, int *N_value2, float *V_value2, struct Paquete *value3) {
    int sd = -1;
    int32_t result;
    int32_t n_recv;
    int32_t x, y, z;

    if (key == NULL || value1 == NULL || N_value2 == NULL || V_value2 == NULL || value3 == NULL) {
        return -1;
    }

    sd = connect_server();
    if (sd < 0) {
        return -1;
    }

    if (send_int32(sd, OP_GET_VALUE) < 0 ||
        send_fixed_string(sd, key) < 0 ||
        recv_int32(sd, &result) < 0) {
        close(sd);
        return -1;
    }

    if (result != 0) {
        close(sd);
        return (int)result;
    }

    if (recv_fixed_string(sd, value1) < 0 ||
        recv_int32(sd, &n_recv) < 0 ||
        recv_float_array(sd, V_value2, 32) < 0 ||
        recv_int32(sd, &x) < 0 ||
        recv_int32(sd, &y) < 0 ||
        recv_int32(sd, &z) < 0) {
        close(sd);
        return -1;
    }

    *N_value2 = (int)n_recv;
    value3->x = (int)x;
    value3->y = (int)y;
    value3->z = (int)z;

    close(sd);
    return (int)result;
}

int modify_value(char *key, char *value1, int N_value2, float *V_value2, struct Paquete value3) {
    int sd = -1;
    int32_t result;

    if (key == NULL || value1 == NULL || V_value2 == NULL) {
        return -1;
    }

    sd = connect_server();
    if (sd < 0) {
        return -1;
    }

    if (send_int32(sd, OP_MODIFY_VALUE) < 0 ||
        send_fixed_string(sd, key) < 0 ||
        send_fixed_string(sd, value1) < 0 ||
        send_int32(sd, N_value2) < 0 ||
        send_float_array(sd, V_value2, 32) < 0 ||
        send_int32(sd, value3.x) < 0 ||
        send_int32(sd, value3.y) < 0 ||
        send_int32(sd, value3.z) < 0 ||
        recv_int32(sd, &result) < 0) {
        close(sd);
        return -1;
    }

    close(sd);
    return (int)result;
}

int delete_key(char *key) {
    int sd = -1;
    int32_t result;

    if (key == NULL) {
        return -1;
    }

    sd = connect_server();
    if (sd < 0) {
        return -1;
    }

    if (send_int32(sd, OP_DELETE_KEY) < 0 ||
        send_fixed_string(sd, key) < 0 ||
        recv_int32(sd, &result) < 0) {
        close(sd);
        return -1;
    }

    close(sd);
    return (int)result;
}

int exist(char *key) {
    int sd = -1;
    int32_t result;

    if (key == NULL) {
        return -1;
    }

    sd = connect_server();
    if (sd < 0) {
        return -1;
    }

    if (send_int32(sd, OP_EXIST) < 0 ||
        send_fixed_string(sd, key) < 0 ||
        recv_int32(sd, &result) < 0) {
        close(sd);
        return -1;
    }

    close(sd);
    return (int)result;
}
