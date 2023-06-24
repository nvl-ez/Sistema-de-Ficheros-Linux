// Krishna Jorda Jimenez, Margalida Covas Roig , Nahuel Vazquez
#include "ficheros.h"

/*
    Cambia los permisos de un inodo
    Sintaxis: ./permitir <nombre_dispositivo> <ninodo> <permisos>
*/
int main(int argc, char *argv[]){
    //Verificar cantidad de argumentos
    if(argc != 4){
        fprintf(stderr, "Argumentos Invalidos: ./permitir <nombre_dispositivo> <ninodo> <permisos>\n");
        return FALLO;
    }

    FILE *file;  // Comprobamos que el disco existe antes de hacer el bmount ni nada
    if ((file = fopen(argv[1], "r"))){
        fclose(file);
    }
    else{
        fprintf(stderr, ROJO"Error: El disco \"%s\" no existe.\n"RES, argv[1]);
        return FALLO;
    }
    
    // Convertimos ninodo
    char* endp;  // Puntero al primer caracter invalido
    char** endpp = &endp;  // Pasaremos ese puntero por referencia
    int ninodo = strtol(argv[2], endpp, 10);  // Llamamos a strtol para parsear los permisos ("man strtol" si no se entiende)
    if ((*endpp != NULL && **endpp != '\0') || ninodo < 0){
        fprintf(stderr, ROJO"Error: ninodo <<%d>> invalido (entero > 0)\n"RES, ninodo);
        return FALLO;
    }

    // Convertimos permisos
    int permisos = strtol(argv[3], endpp, 10);  // Llamamos a strtol para parsear los permisos ("man strtol" si no se entiende)
    if ((*endpp != NULL && **endpp != '\0') || permisos < 0 || permisos > 7){
        fprintf(stderr, ROJO"Error: Permisos <<%d>> invalidos (entero 0-7)\n"RES, permisos);
        return FALLO;
    }

    bmount(argv[1]);

    // Cambio de permisos
    int err = mi_chmod_f(ninodo, permisos);

    if (err == -1){
        fprintf(stderr, ROJO"Error al cambiar los permisos del inodo %d\n"RES, ninodo);
        return FALLO;
    }
    if (err == -2) {
        fprintf(stderr, ROJO"Error: El inodo numero %d no esta reservado.\n"RES, ninodo);
        return FALLO;
    }

    fprintf(stdout, "Cambiamos permisos del inodo %d a %d\n", ninodo, permisos);
    fflush(stdout);

    //desmontar
    bumount();

    return EXITO;
}
