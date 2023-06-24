// Krishna Jorda Jimenez, Margalida Covas Roig , Nahuel Vazquez
#include "ficheros.h"

/*
    Sintaxis: truncar <nombre_dispositivo> <ninodo> <nbytes>
    Trunca el fichero correspondiente al inodo 'ninodo' a 'nbytes' de longitud
*/
int main(int argc, char *argv[]){
    if (argc != 4){
        fprintf(stderr, "Sintaxis: truncar <nombre_dispositivo> <ninodo> <nbytes>\n");
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

    // Guardamos los argumentos de entrada
    char* nombre_dispositivo = argv[1];
    int ninodo = atoi(argv[2]);
    int nbytes = atoi(argv[3]);

    if (bmount(nombre_dispositivo) == FALLO){  // Montamos el dispositivo virtual
        fprintf(stderr, ROJO"Error al montar el dispositivo virtual \"%s\"\n"RES, nombre_dispositivo);
        return FALLO;
    }

    struct STAT stat;  // Creamos un struct de tipo STAT para leer los metadatos del inodo

    if (nbytes == 0){  // Si truncamos desde el byte 0 liberamos todo el inodo
        if (liberar_inodo(ninodo) == FALLO){
            fprintf(stderr, ROJO"Error al liberar el inodo %d\n"RES, ninodo);
            return FALLO;
        }
    }
    else if (mi_truncar_f(ninodo, nbytes) == FALLO){  // Si no llamamos a mi_truncar_f
        fprintf(stderr, ROJO"Error al truncar el inodo %d a %d bytes\n"RES, ninodo, nbytes);
        return FALLO;
    }

    if (mi_stat_f(ninodo, &stat) == FALLO){
        fprintf(stderr, ROJO"Error al leer los datos estadisticos del inodo\n"RES);
        return FALLO;
    }
    struct tm *ts;
    char atime[80];
    char mtime[80];
    char ctime[80];
    ts = localtime(&stat.atime);
    strftime(atime, sizeof(atime), "%a %Y-%m-%d %H:%M:%S", ts);
    ts = localtime(&stat.mtime);
    strftime(mtime, sizeof(mtime), "%a %Y-%m-%d %H:%M:%S", ts);
    ts = localtime(&stat.ctime);
    strftime(ctime, sizeof(ctime), "%a %Y-%m-%d %H:%M:%S", ts);

    //DATOS INODO
    printf("\nDATOS INODO %d:\n", ninodo);
    printf("tipo=%c\n", stat.tipo);
    printf("permisos=%d\n", stat.permisos);
    printf("atime: %s\n", atime);
    printf("ctime: %s\n", ctime);
    printf("mtime: %s\n", mtime);
    printf("nLinks= %d\n", stat.nlinks);
    printf("tamEnBytesLog= %d\n", stat.tamEnBytesLog);
    printf("numBloquesOcupados= %d\n", stat.numBloquesOcupados);

    if (bumount() == FALLO){  // Desmontamos el disco
        fprintf(stderr, ROJO"Error al desmontar el dispositivo virtual\n"RES);
        return FALLO;
    }
}
