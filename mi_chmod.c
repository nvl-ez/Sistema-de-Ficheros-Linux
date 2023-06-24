// Krishna Jorda Jimenez, Margalida Covas Roig , Nahuel Vazquez
#include "directorios.h"

/*
    Programa que cambia los permisos de un fichero o directorio, llamando a la función mi_chmod() de la capa de directorios.
    
   - argv[1]=nombre disco, argv[2]=permisos, argv[3]= ruta
*/
int main(int argc, char *argv[])
{
    // Verificar cantidad de argumentos
    if (argc != 4)
    {
        fprintf(stderr, "Sintaxis: ./mi_chmod <disco> <permisos> </ruta>\n");
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

    // Leemos permisos
    char* endp;  // Puntero al primer caracter invalido
    char** endpp = &endp;  // Pasaremos ese puntero por referencia
    int permisos = strtol(argv[2], endpp, 10);  // Llamamos a strtol para parsear los permisos ("man strtol" si no se entiende)
    if ((*endpp != NULL && **endpp != '\0') || permisos < 0 || permisos > 7){
        fprintf(stderr, ROJO"Error: Permisos <<%d>> invalidos (entero 0-7)\n"RES, permisos);
        return FALLO;
    }

    char *camino = argv[3];

    // montamos el disco
    if (bmount(argv[1]) == FALLO)
    {
        fprintf(stderr, "Error al montar el disco.\n");
    }

    // llamamos la función mi_chmod()
    int error = mi_chmod(camino, permisos);
    if (error < 0)
    {
        fprintf(stderr, "Error al cambiar los permisos de %s\n", camino);
        return FALLO;
    }

    // desmontamos el disco
    bumount();

    return EXITO;
}
