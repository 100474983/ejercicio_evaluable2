#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include <strings.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <arpa/inet.h>
#include <signal.h>

#include "claves.h"
#include "lines.h"

enum {
    OP_SET_VALUE    = 1,
    OP_GET_VALUE    = 2,
    OP_MODIFY_VALUE = 3,
    OP_DELETE_KEY   = 4,
    OP_EXIST        = 5,
    OP_DESTROY      = 6
};

/* ── Helpers ─────────────────────────────────────────────────────────── */

static int send_int32(int sd, int32_t value) {
    int32_t net_value = htonl(value);
    return sendMessage(sd, (char *)&net_value, sizeof(net_value));
}

static int recv_int32(int sd, int32_t *value) {
    int32_t net_value;
    if (recvMessage(sd, (char *)&net_value, sizeof(net_value)) < 0)
        return -1;
    *value = ntohl(net_value);
    return 0;
}

static int send_float_array(int sd, const float *values, int n_values) {
    for (int i = 0; i < n_values; i++) {
        uint32_t bits;
        memcpy(&bits, &values[i], sizeof(bits));
        bits = htonl(bits);
        if (sendMessage(sd, (char *)&bits, sizeof(bits)) < 0)
            return -1;
    }
    return 0;
}

static int recv_float_array(int sd, float *values, int n_values) {
    for (int i = 0; i < n_values; i++) {
        uint32_t bits;
        if (recvMessage(sd, (char *)&bits, sizeof(bits)) < 0)
            return -1;
        bits = ntohl(bits);
        memcpy(&values[i], &bits, sizeof(bits));
    }
    return 0;
}

static int send_fixed_string(int sd, const char *value) {
    char buffer[256];
    memset(buffer, 0, sizeof(buffer));
    if (value != NULL)
        strncpy(buffer, value, sizeof(buffer) - 1);
    return sendMessage(sd, buffer, sizeof(buffer));
}

static int recv_fixed_string(int sd, char *value) {
    if (recvMessage(sd, value, 256) < 0)
        return -1;
    value[255] = '\0';
    return 0;
}

/* ── Validación del argumento puerto ─────────────────────────────────── */

static int parse_port_arg(const char *arg, int *port_out) {
    char *endptr = NULL;
    if (arg == NULL || *arg == '\0' || port_out == NULL)
        return -1;
    long parsed = strtol(arg, &endptr, 10);
    if (endptr == arg || *endptr != '\0' || parsed < 1 || parsed > 65535)
        return -1;
    *port_out = (int)parsed;
    return 0;
}

/* ── Procesamiento de una petición de cliente ────────────────────────── */

static int handle_client(int sc) {
    int32_t op;
    char key[256], value1[256];
    int32_t n_value2, x, y, z;
    float v_value2[32];
    struct Paquete p;
    int result;

    if (recv_int32(sc, &op) < 0)
        return -1;

    switch (op) {

        case OP_SET_VALUE:
            if (recv_fixed_string(sc, key)          < 0 ||
                recv_fixed_string(sc, value1)        < 0 ||
                recv_int32(sc, &n_value2)            < 0 ||
                recv_float_array(sc, v_value2, (int)n_value2) < 0 ||  /* solo N floats */
                recv_int32(sc, &x)                   < 0 ||
                recv_int32(sc, &y)                   < 0 ||
                recv_int32(sc, &z)                   < 0)
                return -1;
            p.x = (int)x; p.y = (int)y; p.z = (int)z;
            result = set_value(key, value1, (int)n_value2, v_value2, p);
            return send_int32(sc, (int32_t)result);

        case OP_GET_VALUE: {
            if (recv_fixed_string(sc, key) < 0)
                return -1;
            int n_local = 0;
            result = get_value(key, value1, &n_local, v_value2, &p);
            if (send_int32(sc, (int32_t)result) < 0)
                return -1;
            if (result == 0) {
                if (send_fixed_string(sc, value1)              < 0 ||
                    send_int32(sc, (int32_t)n_local)           < 0 ||
                    send_float_array(sc, v_value2, n_local)    < 0 ||  /* solo N floats */
                    send_int32(sc, (int32_t)p.x)               < 0 ||
                    send_int32(sc, (int32_t)p.y)               < 0 ||
                    send_int32(sc, (int32_t)p.z)               < 0)
                    return -1;
            }
            return 0;
        }

        case OP_MODIFY_VALUE:
            if (recv_fixed_string(sc, key)          < 0 ||
                recv_fixed_string(sc, value1)        < 0 ||
                recv_int32(sc, &n_value2)            < 0 ||
                recv_float_array(sc, v_value2, (int)n_value2) < 0 ||  /* solo N floats */
                recv_int32(sc, &x)                   < 0 ||
                recv_int32(sc, &y)                   < 0 ||
                recv_int32(sc, &z)                   < 0)
                return -1;
            p.x = (int)x; p.y = (int)y; p.z = (int)z;
            result = modify_value(key, value1, (int)n_value2, v_value2, p);
            return send_int32(sc, (int32_t)result);

        case OP_DELETE_KEY:
            if (recv_fixed_string(sc, key) < 0)
                return -1;
            result = delete_key(key);
            return send_int32(sc, (int32_t)result);

        case OP_EXIST:
            if (recv_fixed_string(sc, key) < 0)
                return -1;
            result = exist(key);
            return send_int32(sc, (int32_t)result);

        case OP_DESTROY:
            result = destroy();
            return send_int32(sc, (int32_t)result);

        default:
            return send_int32(sc, -1);
    }
}

/* ── SIGCHLD: evitar procesos zombie ─────────────────────────────────── */
static void sigchld_handler(int sig) {
    (void)sig;
    while (waitpid(-1, NULL, WNOHANG) > 0);
}

/* ── main ────────────────────────────────────────────────────────────── */

int main(int argc, char *argv[]) {

    int sd, sc;
    int server_port = 0;
    int val;
    struct sockaddr_in server_addr, client_addr;
    socklen_t size;

    if (argc != 2 || parse_port_arg(argv[1], &server_port) < 0) {
        printf("Uso: %s <puerto>\n", argv[0]);
        return -1;
    }

    /* Evitar procesos zombie al usar fork */
    signal(SIGCHLD, sigchld_handler);

    sd = socket(AF_INET, SOCK_STREAM, 0);
    if (sd < 0) {
        perror("[SERVIDOR] Error en socket");
        return -1;
    }

    val = 1;
    setsockopt(sd, SOL_SOCKET, SO_REUSEADDR, (char *)&val, sizeof(int));

    bzero((char *)&server_addr, sizeof(server_addr));
    server_addr.sin_family      = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port        = htons((uint16_t)server_port);

    if (bind(sd, (const struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("[SERVIDOR] Error en bind");
        return -1;
    }

    if (listen(sd, SOMAXCONN) < 0) {
        perror("[SERVIDOR] Error en listen");
        return -1;
    }

    printf("[SERVIDOR] Escuchando en puerto %d\n", server_port);

    size = sizeof(client_addr);

    while (1) {

        sc = accept(sd, (struct sockaddr *)&client_addr, &size);
        if (sc < 0) {
            perror("[SERVIDOR] Error en accept");
            continue;
        }

        printf("[SERVIDOR] Conexion de %s:%d\n",
               inet_ntoa(client_addr.sin_addr),
               ntohs(client_addr.sin_port));

        /* ── Concurrencia con fork ── */
        pid_t pid = fork();

        if (pid < 0) {
            perror("[SERVIDOR] Error en fork");
            close(sc);
            continue;
        }

        if (pid == 0) {
            /* Proceso hijo: atiende al cliente */
            close(sd);   /* el hijo no necesita el socket servidor */
            if (handle_client(sc) < 0)
                printf("[HIJO] Error procesando peticion\n");
            close(sc);
            exit(0);
        }

        /* Proceso padre: cierra su copia del socket cliente y sigue */
        close(sc);
    }

    close(sd);
    return 0;
}