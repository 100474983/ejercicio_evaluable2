#include <arpa/inet.h>
#include <errno.h>
#include <netinet/in.h>
#include <netdb.h>
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

/*Creamos un enum para las operaciones que puede realizar el proxy*/
enum {
    OP_SET_VALUE = 1,
    OP_GET_VALUE = 2,
    OP_MODIFY_VALUE = 3,
    OP_DELETE_KEY = 4,
    OP_EXIST = 5,
    OP_DESTROY = 6
};

/* Función para enviar un entero de 32 bits a través del socket con las funciones de envío */
static int send_int32(int sd, int32_t value) {
    // Convertimos a orden de bytes de red 
    int32_t net_value = htonl(value); 
    return sendMessage(sd, (char *)&net_value, sizeof(net_value));
}

/* Función para recibir un entero de 32 bits a través del socket con las funciones de recepción */
static int recv_int32(int sd, int32_t *value) {
    int32_t net_value;
    if (recvMessage(sd, (char *)&net_value, sizeof(net_value)) < 0) {
        return -1;
    }
    // Convertimos de orden de bytes de red a host 
    *value = ntohl(net_value); 
    return 0;
}

/* Función para enviar un array de floats a través del socket */
static int send_float_array(int sd, const float *values, int n_values) {
    for (int i = 0; i < n_values; i++) {
        // Usamos uint32_t para manipular los bits del float
        uint32_t bits; 
        // Copiamos los bits del float al uint32_t
        memcpy(&bits, &values[i], sizeof(bits)); 
        bits = htonl(bits);
        if (sendMessage(sd, (char *)&bits, sizeof(bits)) < 0) {
            return -1;
        }
    }
    return 0;
}

/* Función para recibir un array de floats a través del socket */
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

/* Función para enviar una cadena de caracteres de longitud fija a través del socket */
static int send_fixed_string(int sd, const char *value) {
    char buffer[256];
    // Inicializamos el buffer con ceros
    memset(buffer, 0, sizeof(buffer)); 
    if (value != NULL) {
        // Copiamos la cadena al buffer asegurándonos de no exceder el tamaño
        strncpy(buffer, value, sizeof(buffer) - 1); 
    }
    return sendMessage(sd, buffer, sizeof(buffer));
}

/* Función para recibir una cadena de caracteres de longitud fija a través del socket */
static int recv_fixed_string(int sd, char *value) {
    if (recvMessage(sd, value, 256) < 0) {
        return -1;
    }
    value[255] = '\0';
    return 0;
}

/* Función para leer el puerto del servidor desde una variable de entorno.
Si la variable de entorno no está definida o está vacía se utiliza un puerto por defecto */
static int read_server_port(void) {
    const char *port_env = getenv("PORT_TUPLAS"); 
    if (port_env == NULL || *port_env == '\0') {
        return DEFAULT_SERVER_PORT; 
    }

    // Puntero para almacenar el final de la cadena analizada
    char *endptr = NULL; 
     // Convertimos el puerto a un número entero
    long parsed = strtol(port_env, &endptr, 10);
    if (endptr == port_env || *endptr != '\0' || parsed < 1 || parsed > 65535) {
        return DEFAULT_SERVER_PORT; 
    }
    return (int)parsed;
}

/* Función para conectar al servidor */
static int connect_server(void) {
    int sd = -1;
    struct sockaddr_in server_addr; 
    // Lectura de la variable de entorno IP_TUPLAS para obtener la dirección IP del servidor
    const char *server_ip = getenv("IP_TUPLAS"); 
    // Obtención del puerto del servidor 
    int server_port = read_server_port(); 

    if (server_ip == NULL || *server_ip == '\0') {
        // Se utiliza la IP por defecto si la var de entorno no está definida o está vacía
        server_ip = DEFAULT_SERVER_IP; 
    }

    // Socket para la conexión TCP
    sd = socket(AF_INET, SOCK_STREAM, 0);
    if (sd < 0) {
        return -1;
    }

    // Configuración de la dirección del servidor
    memset(&server_addr, 0, sizeof(server_addr)); 
    server_addr.sin_family = AF_INET; 
    // Conversión del puerto a orden de bytes de red
    server_addr.sin_port = htons((uint16_t)server_port); 
    // Conversión de la dirección IP del servidor de texto a formato binario
    if (inet_pton(AF_INET, server_ip, &server_addr.sin_addr) != 1) {

        /* Si la dirección IP no es válida, intentamos resolverla como un nombre de host 
        Convertimos el nombre de host a una estructura hostent*/
        struct hostent *he = gethostbyname(server_ip); 
        // Si la resolución falla, cerramos el socket y devolvemos -1 para indicar un error
        if (he == NULL) {
            close(sd);
            return -1;
        }
        // Copiamos la dirección IP resuelta al campo sin_addr de la estructura server_addr 
        memcpy(&server_addr.sin_addr, he->h_addr_list[0], he->h_length);
    }

    // Nos conectamos al servidor usando el socket creado y la dirección configurada
    if (connect(sd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        close(sd);
        return -1;
    }

    return sd;
}
/* FUNCIONES DE LA API PARA LLAMADAS AL SERVIDOR */

int destroy(void) {
    // Abrimos una conexión TCP con el servidor
    int sd = connect_server(); 
    int32_t result; 

    if (sd < 0) {
        return -1;
    }
    /* Enviamos la operación OP_DESTROY al servidor y esperamos la respuesta con el resultado de la operación
       Si falla el envío o la recepción, cerramos el socket y retornamos -1 para indicar un error */
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

    // Validamos que los parámetros no sean nulos antes de intentar conectarnos al servidor
    if (key == NULL || value1 == NULL || V_value2 == NULL) {
        return -1;
    }

    // Abrimos una conexión TCP con el servidor 
    sd = connect_server();
    if (sd < 0) {
        return -1;
    }

    /* Enviamos la operación OP_SET_VALUE al servidor junto con los parámetros necesarios para establecer el valor.
       Si falla el envío o la recepción, cerramos el socket y retornamos -1 para indicar un error */
    if (send_int32(sd, OP_SET_VALUE) < 0 ||
        send_fixed_string(sd, key) < 0 ||
        send_fixed_string(sd, value1) < 0 ||
        send_int32(sd, N_value2) < 0 ||
        send_float_array(sd, V_value2, N_value2) < 0 ||
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
    int32_t x, y, z; /

    if (key == NULL || value1 == NULL || N_value2 == NULL || V_value2 == NULL || value3 == NULL) {
        return -1;
    }

    sd = connect_server();
    if (sd < 0) {
        return -1;
    }

    /* Enviamos la operación OP_GET_VALUE al servidor junto con la clave para obtener el valor asociado.
       Luego esperamos la respuesta del servidor con el resultado de la operación
       Si el resultado es diferente de 0, significa que hubo un error al obtener el valor */
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

    /* Si el resultado es 0, recibimos los datos. Primero recibimos value1 y n_recv,
       y luego usamos n_recv para saber cuántos floats leer. */
    if (recv_fixed_string(sd, value1) < 0 ||
        recv_int32(sd, &n_recv) < 0) {
        close(sd);
        return -1;
    }
    if (recv_float_array(sd, V_value2, (int)n_recv) < 0 ||
        recv_int32(sd, &x) < 0 ||
        recv_int32(sd, &y) < 0 ||
        recv_int32(sd, &z) < 0) {
        close(sd);
        return -1;
    }
    // Asignamos los valores recibidos a las variables correspondientes, incluyendo la estructura Paquete 
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
    /* Comprobamos que todas las operaciones, tanto las de envío como las de recepción, se realicen correctamente.
       Si alguna falla, cerramos el socket y retornamos -1 para indicar un error */
    if (send_int32(sd, OP_MODIFY_VALUE) < 0 ||
        send_fixed_string(sd, key) < 0 ||
        send_fixed_string(sd, value1) < 0 ||
        send_int32(sd, N_value2) < 0 ||
        send_float_array(sd, V_value2, N_value2) < 0 ||
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
    // Si la operación de envío o recepción falla, cerramos el socket y retornamos -1 para indicar un error
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