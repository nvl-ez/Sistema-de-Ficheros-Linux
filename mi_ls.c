// Krishna Jorda Jimenez, Margalida Covas Roig , Nahuel Vazquez
#include "directorios.h"
/*
    Mejoras de mi_ls.c y de la función mi_dir() implementadas por Nahuel Vazquez
    Programa que lista el contenido de un directorio en un disco especifico.
    Sintxis: ./mi_ls <disco> <ruta_directorio> -l (opcional)
*/

int main (int argc, char* argv[]) {
    //verificacion de cantidad de argumentos
    if (argc <3 ||argc>4 ){
        fprintf(stderr, "Sintaxis: ./mi_ls <disco> </ruta_directorio> -l (opcional)\n");
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

    //MEJORA: diferenciar comando normal del comando con el flag
    //verificacion del flag
    char extended = 0;
    if(argc == 4){
        if(argv[3][1]!='l'){
            fprintf(stderr, ROJO"Sintaxis: ./mi_ls <disco> </ruta_directorio> -l (opcional)\n"RES);
            return FALLO;
        } else{
            extended = 1;
        }
    }

    if (bmount(argv[1]) == FALLO){
        fprintf(stderr, ROJO"Error al montar el disco\n"RES);
        return FALLO;
    }
    //Para obtener la cantidad total de inodos
    struct superbloque SB; 
        if (bread(posSB, &SB) == FALLO)
            return FALLO;

    char buffer[TAMFILA*SB.totInodos];

    //Detectamos si es un directorio o un fichero
    char tipo;
    if(argv[2][strlen(argv[2])-1]=='/'){
        tipo = 'd';
    } else{
        tipo = 'f';
    }

    extended = 1;
    //total almacena la cantidad de entradas, y en caso de error, el codigo de este
    int total = mi_dir(argv[2], buffer, tipo, extended);

    //verificamos que no haya ocurrido un error
    if (total<0) {
        mostrar_error_buscar_entrada(total);
        return FALLO;
    }
    //Se muestra por pantalla la info del comando
    if(total){
        printf("Total: %d\n", total);
        printf("%s",buffer);
    }

    //desmontar disco
    if (bumount() == FALLO){
        fprintf(stderr, ROJO"Error al desmontar el disco\n"RES);
        return FALLO;
    }

    return EXITO;
}
