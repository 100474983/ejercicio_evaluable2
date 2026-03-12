#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include "claves.h"

/* =========================================================
 * Nodo de la lista enlazada
 * ========================================================= */
typedef struct Nodo {
    char key[256];
    char value1[256];
    int  N_value2;
    float V_value2[32];
    struct Paquete value3;
    struct Nodo *siguiente;
} Nodo;

/* =========================================================
 * Cabeza de la lista y mutex para atomicidad
 * ========================================================= */
static Nodo *cabeza = NULL;
static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

/* =========================================================
 * Función auxiliar interna: busca un nodo por clave.
 * IMPORTANTE: llamar siempre con el mutex ya adquirido.
 * ========================================================= */
static Nodo *buscar_nodo(const char *key) {
    Nodo *actual = cabeza;
    while (actual != NULL) {
        if (strcmp(actual->key, key) == 0)
            return actual;
        actual = actual->siguiente;
    }
    return NULL;
}

/* =========================================================
 * destroy: elimina todas las tuplas almacenadas.
 * ========================================================= */
int destroy(void) {
    pthread_mutex_lock(&mutex);

    Nodo *actual = cabeza;
    while (actual != NULL) {
        Nodo *siguiente = actual->siguiente;
        free(actual);
        actual = siguiente;
    }
    cabeza = NULL;

    pthread_mutex_unlock(&mutex);
    return 0;
}

/* =========================================================
 * set_value: inserta una nueva tupla.
 * Error si la clave ya existe o N_value2 está fuera de rango.
 * ========================================================= */
int set_value(char *key, char *value1, int N_value2, float *V_value2, struct Paquete value3) {
    /* Validaciones previas */
    if (key == NULL || value1 == NULL || V_value2 == NULL)
        return -1;
    if (N_value2 < 1 || N_value2 > 32)
        return -1;
    if (strlen(key) > 255 || strlen(value1) > 255)
        return -1;

    pthread_mutex_lock(&mutex);

    /* Comprobar que la clave no existe ya */
    if (buscar_nodo(key) != NULL) {
        pthread_mutex_unlock(&mutex);
        return -1;
    }

    /* Crear nuevo nodo */
    Nodo *nuevo = (Nodo *)malloc(sizeof(Nodo));
    if (nuevo == NULL) {
        pthread_mutex_unlock(&mutex);
        return -1;
    }

    strncpy(nuevo->key,    key,    255); nuevo->key[255]    = '\0';
    strncpy(nuevo->value1, value1, 255); nuevo->value1[255] = '\0';
    nuevo->N_value2 = N_value2;
    memset(nuevo->V_value2, 0, 32 * sizeof(float));
    memcpy(nuevo->V_value2, V_value2, N_value2 * sizeof(float));
    nuevo->value3 = value3;
    nuevo->siguiente = cabeza;
    cabeza = nuevo;

    pthread_mutex_unlock(&mutex);
    return 0;
}

/* =========================================================
 * get_value: obtiene los valores asociados a una clave.
 * ========================================================= */
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

/* =========================================================
 * modify_value: modifica los valores de una clave existente.
 * ========================================================= */
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

/* =========================================================
 * delete_key: elimina la tupla con la clave indicada.
 * ========================================================= */
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
    return -1; /* clave no encontrada */
}

/* =========================================================
 * exist: comprueba si existe una tupla con la clave dada.
 * ========================================================= */
int exist(char *key) {
    if (key == NULL)
        return -1;

    pthread_mutex_lock(&mutex);
    int resultado = (buscar_nodo(key) != NULL) ? 1 : 0;
    pthread_mutex_unlock(&mutex);

    return resultado;
}