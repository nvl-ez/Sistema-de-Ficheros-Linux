// Krishna Jorda Jimenez, Margalida Covas Roig , Nahuel Vazquez
#include "directorios.h"

/*
    Programa que permite escribir texto en un fichero a partir de un offset determinado (nivel 9)
    Sintaxis: ./mi_escribir <disco> </ruta_fichero> <texto> <offset>
*/
int main(int argc, char *argv[]){
    if (argc != 5){
        fprintf(stderr, "Sintaxis: ./mi_escribir <disco> </ruta_fichero> <texto> <offset>\n");
        return FALLO;
    }

    if (*(strrchr(argv[2], '/')+1) == '\0') { // Comprobamos que la ruta sea a un fichero
        fprintf(stderr, ROJO"Error: No se puede escribir en un directorio.\n"RES);
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

    if (bmount(argv[1])==FALLO)
        return FALLO;

    // Leemos el offset con strtol
    char* endp;  // Puntero al primer caracter invalido
    char** endpp = &endp;  // Pasaremos ese puntero por referencia
    int offset = strtol(argv[4], endpp, 10);  // Llamamos a strtol para parsear los permisos ("man strtol" si no se entiende)
    if ((*endpp != NULL && **endpp != '\0') || offset < 0){
        fprintf(stderr, ROJO"Error: Offset invalido.)\n"RES);
        return FALLO;
    }

    int bescritos = mi_write(argv[2], argv[3], offset, strlen(argv[3]));

    printf("Longitud texto: %ld\n", strlen(argv[3]));
    if (bescritos == -1){
        fprintf(stderr, ROJO"Error: Error al escribir al archivo\n"RES);
    }
    if (bescritos == -2) {
        fprintf(stderr, ROJO"Error: No hay permisos de escritura\n"RES);
    }

    if (bescritos < 0)
        bescritos = 0;

    printf("Bytes escritos: %d\n", bescritos);

    if (bumount(argv[1])==FALLO)
        return FALLO;

    return bescritos;
}
