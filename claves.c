#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include "claves.h"

/* Creamos la estructura Nodo para la lista enlazada */
typedef struct Nodo {
    char key[256];
    char value1[256];
    int  N_value2;
    float V_value2[32];
    struct Paquete value3;
    struct Nodo *siguiente;
} Nodo;

/* Inicializamos el puntero al primer elemento de la lista y el mutex que usaremos para evitar condiciones de carrera */
static Nodo *cabeza = NULL;
static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

/* Esta función busca un nodo por clave*/
static Nodo *buscar_nodo(const char *key) {
    Nodo *actual = cabeza;
    while (actual != NULL) {
        if (strcmp(actual->key, key) == 0)
            return actual;
        actual = actual->siguiente;
    }
    return NULL;
}
int destroy(void) {
    /* Adquirimos el mutex para evitar condiciones de carrera */
    pthread_mutex_lock(&mutex); 

    Nodo *actual = cabeza;
    while (actual != NULL) {
        Nodo *siguiente = actual->siguiente;
        /* En cada iteración se libera la memoria del nodo actual */
        free(actual);  
        actual = siguiente;
    }
    cabeza = NULL;
    /* Una vez terminado liberamos el mutex*/
    pthread_mutex_unlock(&mutex); 
    return 0;
}

int set_value(char *key, char *value1, int N_value2, float *V_value2, struct Paquete value3) {
    /* Hacemos algunas comprobaciones iniciales */
    if (key == NULL || value1 == NULL || V_value2 == NULL)
        return -1;
    if (N_value2 < 1 || N_value2 > 32)
        return -1;
    if (strlen(key) > 255 || strlen(value1) > 255)
        return -1;

    pthread_mutex_lock(&mutex); 

    /* No permitimos claves duplicadas */
    if (buscar_nodo(key) != NULL) {
        pthread_mutex_unlock(&mutex); 
        return -1;
    }

    /* Creamos el nuevo nodo con una asignación de memoria dinámica */
    Nodo *nuevo = (Nodo *)malloc(sizeof(Nodo));
    if (nuevo == NULL) {
        pthread_mutex_unlock(&mutex);
        return -1;
    }

    strncpy(nuevo->key, key, 255); nuevo->key[255] = '\0';
    strncpy(nuevo->value1, value1, 255); nuevo->value1[255] = '\0';
    nuevo->N_value2 = N_value2;
    /* Inicializamos todo el array antes de copiar */
    memset(nuevo->V_value2, 0, 32 * sizeof(float));  
    memcpy(nuevo->V_value2, V_value2, N_value2 * sizeof(float));
    nuevo->value3 = value3;
    nuevo->siguiente = cabeza;
    cabeza = nuevo;

    pthread_mutex_unlock(&mutex);
    return 0;
}

/* Con esta función se obtienen los valores asociados a una clave*/
int get_value(char *key, char *value1, int *N_value2, float *V_value2, struct Paquete *value3) {
    if (key == NULL || value1 == NULL || N_value2 == NULL || V_value2 == NULL || value3 == NULL)
        return -1;

    pthread_mutex_lock(&mutex);

    Nodo *nodo = buscar_nodo(key);
    if (nodo == NULL) {
        pthread_mutex_unlock(&mutex);
        return -1;
    }

    strncpy(value1, nodo->value1, 255); value1[255] = '\0';
    *N_value2 = nodo->N_value2;
    memcpy(V_value2, nodo->V_value2, nodo->N_value2 * sizeof(float));
    *value3 = nodo->value3;

    pthread_mutex_unlock(&mutex);
    return 0;
}

/* Con esta función se modifican los valores asociados a una clave existente */
int modify_value(char *key, char *value1, int N_value2, float *V_value2, struct Paquete value3) {
    if (key == NULL || value1 == NULL || V_value2 == NULL)
        return -1;
    if (N_value2 < 1 || N_value2 > 32)
        return -1;
    if (strlen(key) > 255 || strlen(value1) > 255)
        return -1;

    pthread_mutex_lock(&mutex);

    Nodo *nodo = buscar_nodo(key);
    if (nodo == NULL) {
        pthread_mutex_unlock(&mutex);
        return -1;
    }

    strncpy(nodo->value1, value1, 255); nodo->value1[255] = '\0';
    nodo->N_value2 = N_value2;
    memcpy(nodo->V_value2, V_value2, N_value2 * sizeof(float));
    nodo->value3 = value3;

    pthread_mutex_unlock(&mutex);
    return 0;
}

/* Con esta función se elimina una clave existente */
int delete_key(char *key) {
    if (key == NULL)
        return -1;

    pthread_mutex_lock(&mutex);

    Nodo *actual   = cabeza;
    Nodo *anterior = NULL;

    while (actual != NULL) {
        if (strcmp(actual->key, key) == 0) {
            if (anterior == NULL)
                cabeza = actual->siguiente;
            else
                anterior->siguiente = actual->siguiente;
            free(actual);
            pthread_mutex_unlock(&mutex);
            return 0;
        }
        anterior = actual;
        actual   = actual->siguiente;
    }

    pthread_mutex_unlock(&mutex);
    /* En caso de no encontrar la clave */
    return -1; 
}

/* Con esta función verificamos si una clave existe */
int exist(char *key) {
    if (key == NULL)
        return -1;

    pthread_mutex_lock(&mutex);
    int resultado = (buscar_nodo(key) != NULL) ? 1 : 0;
    pthread_mutex_unlock(&mutex);

    return resultado;
}