// Krishna Jorda Jimenez, Margalida Covas Roig , Nahuel Vazquez
#include "directorios.h"

/*
    Recibe por los argumentos especificados en la consola, el nombre del dispositivo en la
    posicion 2 del array y la cantidad de bloques de este en la 3ra posicion

    Sintaxis: ./mi_mkfs <nombre> <tamaño>
*/
int main(int argc, char *argv[]){
    //Verificar cantidad de argumentos
    if(argc != 3){
        fprintf(stderr, "Argumentos Invalidos: ./mi_mkfs <nombre> <tamaño>\n");
        return FALLO;
    }
    
    //Verifica que se pueda crear el disco y que la cantidad de bloques es corracta
    if(atoi(argv[2]) <= 0 ){
        fprintf(stderr, "Syntaxis: ./mi_mkfs <nombre> <tamaño>\n");
        return FALLO;
    }

    //Crea el buf de 0s para limpiar el bloque
    unsigned char buf[BLOCKSIZE];
    memset(buf, 0, BLOCKSIZE);

    if (bmount(argv[1]) == FALLO){
        fprintf(stderr, ROJO "Error: mi_mkfs -> bmount()\n" RES);
        exit(FALLO);
    }
    //Limpia la cantidad de bloques especificada
    for(int i = 0; i<atoi(argv[2]); i++){
        if (bwrite(i, buf) == FALLO){
            fprintf(stderr, "Error al limpiar los bloques para crear el disco\n");
            return FALLO;
        }
    }

    initSB(atoi(argv[2]), atoi(argv[2])/4);
    if (initMB() == FALLO){
        fprintf(stderr, "Error en initMB()\n");
        return FALLO;
    }
    if (initAI() == FALLO){
        fprintf(stderr, "Error en initAI()\n");
        return FALLO;
    }

    //Crear el directorio raiz.
    reservar_inodo('d', 7);


    bumount();

    return EXITO;
}
