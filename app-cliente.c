#include <stdio.h>
#include <string.h>
#include "claves.h"

int main() {

    printf("---- INICIO PRUEBA ----\n");

    /* 1. destroy inicial */
    if (destroy() == 0)
        printf("Destroy correcto\n");
    else
        printf("Error en destroy\n");

    /* 2. Insertar una clave */
    char key[] = "clave1";
    char value1[] = "hola mundo";
    int N_value2 = 3;
    float V_value2[32] = {1.1, 2.2, 3.3};
    struct Paquete p = {10, 20, 30};

    if (set_value(key, value1, N_value2, V_value2, p) == 0)
        printf("set_value correcto\n");
    else
        printf("Error en set_value\n");

    /* 3. Comprobar existencia */
    int ex = exist(key);
    printf("exist devuelve: %d (esperado 1)\n", ex);

    /* 4. Obtener valores */
    char value1_out[256];
    int N_value2_out;
    float V_value2_out[32];
    struct Paquete p_out;

    if (get_value(key, value1_out, &N_value2_out, V_value2_out, &p_out) == 0) {
        printf("get_value correcto\n");
        printf("value1: %s\n", value1_out);
        printf("N_value2: %d\n", N_value2_out);
        printf("V_value2: ");
        for (int i = 0; i < N_value2_out; i++)
            printf("%.2f ", V_value2_out[i]);
        printf("\n");
        printf("Paquete: %d %d %d\n", p_out.x, p_out.y, p_out.z);
    } else {
        printf("Error en get_value\n");
    }

    /* 5. Modificar valores */
    char new_value1[] = "nuevo texto";
    float new_V_value2[32] = {9.9, 8.8};
    struct Paquete new_p = {100, 200, 300};

    if (modify_value(key, new_value1, 2, new_V_value2, new_p) == 0)
        printf("modify_value correcto\n");
    else
        printf("Error en modify_value\n");

    /* 6. Obtener de nuevo para comprobar */
    if (get_value(key, value1_out, &N_value2_out, V_value2_out, &p_out) == 0) {
        printf("get_value tras modify correcto\n");
        printf("value1: %s\n", value1_out);
        printf("N_value2: %d\n", N_value2_out);
        printf("V_value2: ");
        for (int i = 0; i < N_value2_out; i++)
            printf("%.2f ", V_value2_out[i]);
        printf("\n");
        printf("Paquete: %d %d %d\n", p_out.x, p_out.y, p_out.z);
    }

    /* 7. Borrar clave */
    if (delete_key(key) == 0)
        printf("delete_key correcto\n");
    else
        printf("Error en delete_key\n");

    /* 8. Comprobar que ya no existe */
    ex = exist(key);
    printf("exist tras delete devuelve: %d (esperado 0)\n", ex);

    /* ---- PRUEBAS DE ERROR ---- */
    printf("\n---- PRUEBAS DE ERROR ----\n");

    /* Clave duplicada */
    if (set_value("clave2", value1, 3, V_value2, p) == 0)
        printf("set_value clave2 -> correcto\n");
    if (set_value("clave2", value1, 3, V_value2, p) == -1)
        printf("set_value duplicada -> -1 correcto\n");

    /* N fuera de rango */
    if (set_value("clave3", value1, 0, V_value2, p) == -1)
        printf("set_value N=0 -> -1 correcto\n");
    if (set_value("clave3", value1, 33, V_value2, p) == -1)
        printf("set_value N=33 -> -1 correcto\n");

    /* get sobre clave inexistente */
    if (get_value("no_existe", value1_out, &N_value2_out, V_value2_out, &p_out) == -1)
        printf("get_value no_existe -> -1 correcto\n");

    /* delete sobre clave inexistente */
    if (delete_key("no_existe") == -1)
        printf("delete_key no_existe -> -1 correcto\n");

    printf("\n---- FIN PRUEBA ----\n");

    return 0;
}