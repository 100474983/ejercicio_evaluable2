#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "claves.h"

#define NUM_HILOS 20

/*
Realizamos un test para probar la concurrencia del servidor en el manejo de varias peticiones simultaneas de clientes.
Para ello creamos varios hilos que realizan operaciones sobre claves diferentes a la vez.*/

// Función de trabajo para cada hilo, que realiza varias operaciones sobre la clave correspondiente a su ID
void *worker(void *arg) {
    int id = *(int *)arg;

    // Creamos la clave específica para este hilo
    char key[50];
    sprintf(key, "clave_%d", id);

    char value1[] = "test";
    float v[32] = {1.1, 2.2};
    struct Paquete p = {id, id, id};

    // set
    if (set_value(key, value1, 2, v, p) == 0)
        printf("[HILO %d] set OK\n", id);

    // get
    char v1_out[256];
    int n;
    float v_out[32];
    struct Paquete p_out;

    if (get_value(key, v1_out, &n, v_out, &p_out) == 0)
        printf("[HILO %d] get OK\n", id);

    // exist
    int ex = exist(key);
    printf("[HILO %d] exist = %d\n", id, ex);

    // delete
    if (delete_key(key) == 0)
        printf("[HILO %d] delete OK\n", id);

    return NULL;
}

int main() {
    // Creamos un array de hilos y un array de IDs para pasar a cada hilo
    pthread_t hilos[NUM_HILOS];
    int ids[NUM_HILOS];

    // Antes de iniciar los hilos limpiamos el servidor de posibles datos residuales de pruebas anteriores
    destroy();  

    // Creamos NUM_HILOS hilos que ejecutan la función worker, pasando su ID como argumento
    for (int i = 0; i < NUM_HILOS; i++) {
        ids[i] = i;
        pthread_create(&hilos[i], NULL, worker, &ids[i]);
    }

    // Esperamos a que todos los hilos terminen su ejecución
    for (int i = 0; i < NUM_HILOS; i++) {
        pthread_join(hilos[i], NULL);
    }

    printf("\nTEST CONCURRENCIA FINALIZADO\n");
    return 0;
}