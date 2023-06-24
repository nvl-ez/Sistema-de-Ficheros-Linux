// Krishna Jorda Jimenez, Margalida Covas Roig , Nahuel Vazquez
#include "directorios.h"
/*
    Programa que que muestra TODO el contenido de un fichero. (nivel 9)
    Sintaxis: ./mi_cat <disco> </ruta_fichero>
*/

int main(int argc, char* argv[]){
     //Verificar cantidad de argumentos
    if(argc != 3){
        fprintf(stderr, "Argumentos Invalidos: ./mi_cat <disco> </ruta_fichero>\n");
        return FALLO;
    }

    if (*(strrchr(argv[2], '/')+1) == '\0') { // Comprobamos que la ruta sea a un fichero
        fprintf(stderr, ROJO"Error: No se puede hacer cat de un directorio."RES);
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
    int offset = 0;
    int leidos = 1;
    unsigned char buffer_texto[TAMBUFFERCAT];
    int totalLeidos = 0;

    while(leidos>0){  // Mientras mi_read no devuelva 0 seguimos leyendo
        memset(buffer_texto, 0, TAMBUFFERCAT);  // Limpiamos el buffer antes de leer

        leidos = mi_read(argv[2], buffer_texto, offset, TAMBUFFERCAT);
        if (leidos < 0){
            mostrar_error_buscar_entrada(leidos);
            return FALLO;
        }

        totalLeidos += leidos;
        write(1, buffer_texto, leidos);  // Escribimos el contenido del buffer al stdout

        offset += TAMBUFFERCAT;  // Avanzamos para leer de mas adelante
    }
    fprintf(stderr, "\nBytes leidos: %d\n", totalLeidos);

    if(bumount()==FALLO){
        return FALLO;
    }

    return EXITO;
}
