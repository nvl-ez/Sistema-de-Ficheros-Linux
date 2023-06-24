// Krishna Jorda Jimenez, Margalida Covas Roig , Nahuel Vazquez
#include "directorios.h"

/*
    Programa que crea un fichero en la ruta especificada (no podra acabar con "/")
    Sintaxis: ./mi_touch <disco> <permisos> </ruta>
*/

int main (int argc, char* argv[]) {
    if (argc != 4){
        fprintf(stderr, "Sintaxis: ./mi_touch <disco> <permisos> </ruta>\n");
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

    char* endp;  // Puntero al primer caracter invalido
    char** endpp = &endp;  // Pasaremos ese puntero por referencia
    int permisos = strtol(argv[2], endpp, 10);  // Llamamos a strtol para parsear los permisos ("man strtol" si no se entiende)
    if ((*endpp != NULL && **endpp != '\0') || permisos < 0 || permisos > 7){
        fprintf(stderr, ROJO"Error: Permisos <<%d>> invalidos (entero 0-7)\n"RES, permisos);
        return FALLO;
    }

    // Comprobamos que la ruta no acabe con "/"
    char* ptr = strrchr(argv[3], '/');
    if (ptr != NULL && *(ptr+1) == '\0'){
        fprintf(stderr, ROJO"Error: La ruta no puede acabar con \"/\"\n"RES);
        return FALLO;
    }

    if (bmount(argv[1]) == FALLO){  // Montamos el disco
        fprintf(stderr, ROJO"Error al montar el disco\n"RES);
        return FALLO;
    }

    int err = mi_creat(argv[3], atoi(argv[2]));  // Creamos el fichero

    if (err) {  // Comprobamos si hay algun error
        mostrar_error_buscar_entrada(err);
        return FALLO;
    }

    if (bumount() == FALLO){  // Desmontamos el disco
        fprintf(stderr, ROJO"Error al desmontar el disco\n"RES);
        return FALLO;
    }

    return EXITO;
}
