// Krishna Jorda Jimenez
#include "directorios.h"

/*
    Programa que llama a la funcion mi_mv de la capa de ficheros y permite mover un fichero o un
    directorio a un directorio. 
    
    Sintaxis: ./mi_mv <disco> </origen/nombre> </destino/>
*/
int main(int argc, char *argv[]){
    if (argc != 4){
        fprintf(stderr, "Error: Sintaxis: ./mi_mv <disco> </origen/nombre> </destino/>\n");
        return FALLO;
    }
    
    // Comprobamos que la ruta acabe con "/"
    char* ptr = strrchr(argv[3], '/');
    if (ptr != NULL && *(ptr+1) != '\0'){
        fprintf(stderr, ROJO"Error: El destino tiene que ser un directorio.\n"RES);
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
    
    int err = mi_mv(argv[3], argv[2]);
    switch(err) {
        case FALLO: {
            fprintf(stderr,ROJO"Se ha producido un error durante la operacion de mover.\n"RES);
            break;
        }
        case -2: {
            fprintf(stderr, ROJO"Error: %s no tiene permisos de lectura\n"RES, argv[2]);
            break;
        }
    }
    
    if (bumount() == FALLO)
        return FALLO;
    
    if (err < 0)
        return FALLO;
    else
        return EXITO;
}
