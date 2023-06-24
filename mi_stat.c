// Krishna Jorda Jimenez, Margalida Covas Roig , Nahuel Vazquez
#include "directorios.h"

/*
    Programa que muestra la información acerca del inodo de un fichero o directorio, 
    llamando a la función mi_stat() de la capa de directorios, que a su vez llamará 
    a mi_stat_f() de la capa de ficheros.
    - argv[1]=nombre disco, argv[2]=ruta

*/

int main(int argc, char *argv[])
{
    // Verificar cantidad de argumentos
    if (argc != 3)
    {
        fprintf(stderr, "Argumentos Invalidos: ./mi_stat <disco> </ruta>\n");
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

    char *camino= argv[2];
    int p_inodo;

    // montamos el disco
    if (bmount(argv[1]) == FALLO)
    {
        fprintf(stderr, "Error al montar el disco.\n");
    }

    struct STAT p_stat;
    p_inodo=mi_stat(camino, &p_stat);
    if(p_inodo<0){
         fprintf(stderr, "Error al leer los metadatos de %s\n", camino);
        return FALLO;
    }

    struct tm *ts;
    char atime[80];
    char mtime[80];
    char ctime[80];

    ts = localtime(&p_stat.atime);
    strftime(atime, sizeof(atime), "%a %Y-%m-%d %H:%M:%S", ts);
    ts = localtime(&p_stat.mtime);
    strftime(mtime, sizeof(mtime), "%a %Y-%m-%d %H:%M:%S", ts);
    ts = localtime(&p_stat.ctime);
    strftime(ctime, sizeof(ctime), "%a %Y-%m-%d %H:%M:%S", ts);

    //Motramos valores
    printf("Nº de inodo: %d\n", p_inodo);
    printf("tipo: %c\n", p_stat.tipo);
    printf("permisos: %d\n", p_stat.permisos);
    printf("atime: %s\n", atime);
    printf("ctime: %s\n", ctime);
    printf("mtime: %s\n", mtime);
    printf("nlinks: %d\n", p_stat.nlinks);
    printf("tamEnBytesLog: %d\n", p_stat.tamEnBytesLog);
    printf("numBloquesOcupados: %d\n", p_stat.numBloquesOcupados);

    bumount();
}
