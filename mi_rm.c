// Krishna Jorda Jimenez, Margalida Covas Roig , Nahuel Vazquez
#include "directorios.h"
/*
    Programa que borra un fichero o directorio, llamando a la función mi_unlink() de la capa de directorios.
    Sintaxis: ./mi_rm <disco> </ruta>
*/

int main(int argc, char* argv[]){
     //Verificar cantidad de argumentos
    if(argc != 3){
        fprintf(stderr, "Argumentos Invalidos: ./mi_rm <disco< </ruta>\n");
        return FALLO;
    }
    
    // Comprobamos que la ruta no acabe con "/"
    char* ptr = strrchr(argv[2], '/');
    if (ptr != NULL && *(ptr+1) == '\0'){
        fprintf(stderr, ROJO"Error: La ruta no puede acabar con \"/\"\n"RES);
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

    if (bmount(argv[1]) == FALLO) {
        fprintf(stderr, ROJO"Error al montar el disco\n");
        return FALLO;
    }

    int err = mi_unlink(argv[2]);
    if(err == FALLO){
        fprintf(stderr, ROJO"Error al eliminar fichero o directorio: %s"RES, argv[2]);
    }

    if(bumount()==FALLO){
        return FALLO;
    }

    return EXITO;
}
