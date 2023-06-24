// Krishna Jorda Jimenez, Margalida Covas Roig , Nahuel Vazquez
#include "ficheros.h"
#define GRIS "\x1b[94m"
#define RES "\x1b[0m"

int main(int argc, char* argv[]){
     //Verificar cantidad de argumentos
    if(argc != 3){
        fprintf(stderr, "Argumentos Invalidos: ./leer.c <nombre> <ninodo>\n");
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
        fprintf(stderr, ROJO"Error: %d (bmount): %s\n"RES, errno, strerror(errno));
        return FALLO;
    }

    //Variables
    int ninodo = atoi(argv[2]);
    int offset = 0;
    int leidos = 1;
    int tambuffer= 1500;  // Puede ser cualquier valor
    struct STAT stat;
    unsigned char buffer_texto[tambuffer];
    int totalLeidos = 0;

    while(leidos>0){  // Mientras mi_read_f no devuelva 0 seguimos leyendo
        memset(buffer_texto, 0, tambuffer);  // Limpiamos el buffer antes de leer

        leidos = mi_read_f(ninodo, buffer_texto, offset, tambuffer);
        if (leidos == FALLO){
            fprintf(stderr, ROJO"Error al leer del fichero\n"RES);
            return FALLO;
        }
        else if (leidos == -2){
            fprintf(stderr, ROJO"Error: No hay permisos de lectura\n"RES);
            return FALLO;
        }

        totalLeidos += leidos;
        write(1, buffer_texto, leidos);  // Escribimos el contenido del buffer al stdout

        offset += tambuffer;  // Avanzamos para leer de mas adelante
    }

    if (leidos != FALLO){
        fprintf(stderr ,"\nNº de bytes leídos : %d\n", totalLeidos);
        mi_stat_f(ninodo, &stat);
        fprintf(stderr ,"Tamaño en bytes lógicos: %d\n", stat.tamEnBytesLog);
    }

    if(bumount()==FALLO){
        return FALLO;
    }

    return EXITO;
}
