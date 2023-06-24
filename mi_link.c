// Krishna Jorda Jimenez, Margalida Covas Roig , Nahuel Vazquez
#include "directorios.h"
/*
    Programa que crea un enlace a un fichero
    Sintaxis: ./mi_link <disco> </ruta_fichero_original> </ruta_enlace>
*/
int main(int argc, char *argv[]){
    if (argc != 4){
        fprintf(stderr,"Sintaxis: ./mi_link disco </ruta_fichero_original> </ruta_enlace>\n");
        return FALLO;
    }

    if (*(strrchr(argv[2], '/')+1) == '\0') { // Comprobamos que la ruta sea a un fichero
        fprintf(stderr, ROJO"Error: El fichero original no puede ser un directorio, la ruta no puede acabar con \"/\"\n"RES);
        return FALLO;
    }

    if (*(strrchr(argv[3], '/')+1) == '\0') { // Comprobamos que la ruta sea a un fichero
        fprintf(stderr, ROJO"Error: El enlace no puede ser un directorio, la ruta no puede acabar con \"/\"\n"RES);
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

    int err = mi_link(argv[2], argv[3]);

    if (err == FALLO) {
        fprintf(stderr, ROJO"Error al crear el enlace %s\n"RES, argv[3]);
    }
    else if (err == -2){
        fprintf(stderr, ROJO"Error: %s no tiene permiso de lectura\n"RES, argv[2]);
    }
    else if (err == -3) {
        fprintf(stderr, ROJO"Error: %s no es un fichero\n"RES, argv[2]);
    }

    if (bumount()==FALLO)
        return FALLO;

    return EXITO;
}
