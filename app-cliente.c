#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "claves.h"

int main() {

    const char *ip_tuplas = getenv("IP_TUPLAS");
    const char *port_tuplas = getenv("PORT_TUPLAS");

    printf("---- INICIO PRUEBA ----\n");
    printf("IP_TUPLAS = %s\n", (ip_tuplas != NULL) ? ip_tuplas : "(no definida)");
    printf("PORT_TUPLAS = %s\n", (port_tuplas != NULL) ? port_tuplas : "(no definida)");

    // Destroy inicial para limpiar cualquier estado previo
    if (destroy() == 0)
        printf("Destroy correcto\n");
    else
        printf("Error en destroy\n");

    // Insertar una clave con valores asociados 
    char key[] = "clave1";
    char value1[] = "hola mundo";
    int N_value2 = 3;
    float V_value2[32] = {1.1, 2.2, 3.3};
    struct Paquete p = {10, 20, 30};

    if (set_value(key, value1, N_value2, V_value2, p) == 0)
        printf("set_value correcto\n");
    else
        printf("Error en set_value\n");

    // Comprobar existencia de una clave
    int ex = exist(key);
    printf("exist devuelve: %d (esperado 1)\n", ex);

    // Obtener valores asociados a una clave
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

    // 5. Modificar valores asociados a una clave
    char new_value1[] = "nuevo texto";
    float new_V_value2[32] = {9.9, 8.8};
    struct Paquete new_p = {100, 200, 300};

    if (modify_value(key, new_value1, 2, new_V_value2, new_p) == 0)
        printf("modify_value correcto\n");
    else
        printf("Error en modify_value\n");

    // Obtenemos de nuevo los valores asociados a la clave para verificar que se han modificado correctamente
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

    // Borrar claves y comprobar que se han eliminado correctamente
    if (delete_key(key) == 0)
        printf("delete_key correcto\n");
    else
        printf("Error en delete_key\n");

    // Comprobar que ya no existe
    ex = exist(key);
    printf("exist tras delete devuelve: %d (esperado 0)\n", ex);

    /* ---- PRUEBAS DE ERROR ---- */
    printf("\n---- PRUEBAS DE ERROR ----\n");

    // Clave duplicada 
    if (set_value("clave2", value1, 3, V_value2, p) == 0)
        printf("set_value clave2 -> correcto\n");
    if (set_value("clave2", value1, 3, V_value2, p) == -1)
        printf("set_value duplicada -> -1 correcto\n");

    // N fuera de rango 
    if (set_value("clave3", value1, 0, V_value2, p) == -1)
        printf("set_value N=0 -> -1 correcto\n");
    if (set_value("clave3", value1, 33, V_value2, p) == -1)
        printf("set_value N=33 -> -1 correcto\n");

    // get sobre clave inexistente
    if (get_value("no_existe", value1_out, &N_value2_out, V_value2_out, &p_out) == -1)
        printf("get_value no_existe -> -1 correcto\n");

    // delete sobre clave inexistente
    if (delete_key("no_existe") == -1)
        printf("delete_key no_existe -> -1 correcto\n");

    /* ---- PRUEBA ESPECÍFICA DE CONEXIÓN POR DNS/NOMBRE ---- */
    printf("\n---- PRUEBA DNS / NOMBRE DE HOST ----\n");

    if (ip_tuplas != NULL && strcmp(ip_tuplas, "127.0.0.1") != 0) {
        char dns_key[] = "clave_dns";
        char dns_value1[] = "prueba dns";
        int dns_n = 2;
        float dns_v[32] = {4.4, 5.5};
        struct Paquete dns_p = {40, 50, 60};

        if (delete_key(dns_key) == 0 || delete_key(dns_key) == -1) {
            printf("Clave DNS eliminada (si existía)\n");
        }

        if (set_value(dns_key, dns_value1, dns_n, dns_v, dns_p) == 0)
            printf("set_value con nombre de host correcto\n");
        else
            printf("Error en set_value con nombre de host\n");

        ex = exist(dns_key);
        printf("exist(clave_dns) = %d (esperado 1)\n", ex);

        if (get_value(dns_key, value1_out, &N_value2_out, V_value2_out, &p_out) == 0) {
            printf("get_value con nombre de host correcto\n");
            printf("value1: %s\n", value1_out);
            printf("N_value2: %d\n", N_value2_out);
            printf("V_value2: ");
            for (int i = 0; i < N_value2_out; i++)
                printf("%.2f ", V_value2_out[i]);
            printf("\n");
            printf("Paquete: %d %d %d\n", p_out.x, p_out.y, p_out.z);
        } else {
            printf("Error en get_value con nombre de host\n");
        }

        if (delete_key(dns_key) == 0)
            printf("delete_key con nombre de host correcto\n");
        else
            printf("Error en delete_key con nombre de host\n");

        ex = exist(dns_key);
        printf("exist(clave_dns) tras delete = %d (esperado 0)\n", ex);
    } else {
        printf("Esta ejecución no usa nombre de host.\n");
        printf("Para probar DNS, ejecuta con IP_TUPLAS=localhost\n");
    }

    printf("\n---- FIN PRUEBA ----\n");

    return 0;
}