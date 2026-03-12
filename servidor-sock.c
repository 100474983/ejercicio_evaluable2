#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include <strings.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#include "claves.h"
#include "lines.h"

enum {
    OP_SET_VALUE = 1,
    OP_GET_VALUE = 2,
    OP_MODIFY_VALUE = 3,
    OP_DELETE_KEY = 4,
    OP_EXIST = 5,
    OP_DESTROY = 6
};

static int parse_port_arg(const char *arg, int *port_out) {
    char *endptr = NULL;
    long parsed;

    if (arg == NULL || *arg == '\0' || port_out == NULL) {
        return -1;
    }

    parsed = strtol(arg, &endptr, 10);
    if (endptr == arg || *endptr != '\0' || parsed < 1 || parsed > 65535) {
        return -1;
    }

    *port_out = (int)parsed;
    return 0;
}

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

static int handle_client(int sc) {
    int32_t op;

    char key[256];
    char value1[256];
    int32_t n_value2;
    float v_value2[32];
    int32_t x, y, z;
    struct Paquete p;
    int result;

    if (recv_int32(sc, &op) < 0) {
        return -1;
    }

    switch (op) {
        case OP_SET_VALUE:
            if (recv_fixed_string(sc, key) < 0 ||
                recv_fixed_string(sc, value1) < 0 ||
                recv_int32(sc, &n_value2) < 0 ||
                recv_float_array(sc, v_value2, 32) < 0 ||
                recv_int32(sc, &x) < 0 ||
                recv_int32(sc, &y) < 0 ||
                recv_int32(sc, &z) < 0) {
                return -1;
            }
            p.x = (int)x;
            p.y = (int)y;
            p.z = (int)z;
            result = set_value(key, value1, (int)n_value2, v_value2, p);
            if (send_int32(sc, result) < 0) {
                return -1;
            }
            break;

        case OP_GET_VALUE:
            if (recv_fixed_string(sc, key) < 0) {
                return -1;
            }
            {
                int n_local = 0;
                result = get_value(key, value1, &n_local, v_value2, &p);
                n_value2 = n_local;
            }
            if (send_int32(sc, result) < 0) {
                return -1;
            }
            if (result == 0) {
                if (send_fixed_string(sc, value1) < 0 ||
                    send_int32(sc, n_value2) < 0 ||
                    send_float_array(sc, v_value2, 32) < 0 ||
                    send_int32(sc, p.x) < 0 ||
                    send_int32(sc, p.y) < 0 ||
                    send_int32(sc, p.z) < 0) {
                    return -1;
                }
            }
            break;

        case OP_MODIFY_VALUE:
            if (recv_fixed_string(sc, key) < 0 ||
                recv_fixed_string(sc, value1) < 0 ||
                recv_int32(sc, &n_value2) < 0 ||
                recv_float_array(sc, v_value2, 32) < 0 ||
                recv_int32(sc, &x) < 0 ||
                recv_int32(sc, &y) < 0 ||
                recv_int32(sc, &z) < 0) {
                return -1;
            }
            p.x = (int)x;
            p.y = (int)y;
            p.z = (int)z;
            result = modify_value(key, value1, (int)n_value2, v_value2, p);
            if (send_int32(sc, result) < 0) {
                return -1;
            }
            break;

        case OP_DELETE_KEY:
            if (recv_fixed_string(sc, key) < 0) {
                return -1;
            }
            result = delete_key(key);
            if (send_int32(sc, result) < 0) {
                return -1;
            }
            break;

        case OP_EXIST:
            if (recv_fixed_string(sc, key) < 0) {
                return -1;
            }
            result = exist(key);
            if (send_int32(sc, result) < 0) {
                return -1;
            }
            break;

        case OP_DESTROY:
            result = destroy();
            if (send_int32(sc, result) < 0) {
                return -1;
            }
            break;

        default:
            if (send_int32(sc, -1) < 0) {
                return -1;
            }
            break;
    }

    return 0;
}

int main(int argc, char *argv[]) {

    struct sockaddr_in server_addr, client_addr;
    socklen_t size;
    int sd, sc;
    int server_port = 0;
    int val;
    int err;

    if (argc != 2 || parse_port_arg(argv[1], &server_port) < 0) {
        printf("Uso: %s <puerto>\n", argv[0]);
        printf("El puerto debe ser un entero entre 1 y 65535.\n");
        return -1;
    }

    if ((sd = socket(AF_INET, SOCK_STREAM, 0)) < 0){
        printf("SERVER: Error en socket\n");
        return -1;
    }

    val = 1;
    setsockopt(sd, SOL_SOCKET, SO_REUSEADDR, (char *)&val, sizeof(int));

    bzero((char *)&server_addr, sizeof(server_addr));

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons((uint16_t)server_port);

    err = bind(sd, (const struct sockaddr *)&server_addr, sizeof(server_addr));
    if (err == -1){
        printf("Error en bind\n");
        return -1;
    }

    err = listen(sd, SOMAXCONN);
    if (err == -1){
        printf("Error en listen\n");
        return -1;
    }

    size = sizeof(client_addr);

    while(1){

        printf("Esperando conexion...\n");

        sc = accept(sd, (struct sockaddr *)&client_addr, &size);

        if (sc == -1){
            printf("Error en accept\n");
            return -1;
        }

        printf("Conexion aceptada de %s:%d\n",
            inet_ntoa(client_addr.sin_addr),
            ntohs(client_addr.sin_port));

        if (handle_client(sc) == -1) {
            printf("Error procesando peticion de cliente\n");
        }

        close(sc);
    }

    close(sd);
    return 0;
}
