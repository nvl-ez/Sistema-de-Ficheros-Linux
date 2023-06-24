// Krishna Jorda Jimenez
#include "directorios.h"
/*
    Programa que llama a la funcion mi_cp de la capa de directorios, y permite copiar un fichero
    o directorio en un directorio. Si el origen es un directorio con entradas, se copiaran
    recursivamente todos los ficheros y directorios hijo.
    
    Sintaxis: ./mi_cp_f <disco> </origen/nombre> </destino/>
*/
int main(int argc, char *argv[]){
    if (argc != 4){
        fprintf(stderr,"Sintaxis: ./mi_cp <disco> </orgien/nombre> </destino/>\n");
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
    
    if (bmount(argv[1])==FALLO)
        return FALLO;
    
    int err = mi_cp(argv[3], argv[2], false);
    switch(err) {
        case FALLO:
            fprintf(stderr,ROJO"Se ha producido un error durante la operacion de copiado.\n"RES);
            break;
        case -2:
            fprintf(stderr, ROJO"Error: %s no es un fichero. Usa mi_cp para copiar directorios.\n"RES, argv[2]);
            break;
        case -3:
            fprintf(stderr, ROJO"Error: %s no tiene permisos de lectura."RES, argv[2]);
            break;
    }
    
    if (bumount() == FALLO)
        return FALLO;
    
    if (err < 0)
        return FALLO;
    else
        return EXITO;
}
