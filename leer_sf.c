// Krishna Jorda Jimenez, Margalida Covas Roig , Nahuel Vazquez
#include "directorios.h"
#include <stdbool.h>
#include <limits.h>

/*
    Se muestra el camino, si se tiene que reservar y en caso de error, llama a la funcion
    mostrar_error_buscar_entrada() para mostrar la causa de este.
    Utilizada por:
    - main()
*/
void mostrar_buscar_entrada(char *camino, char reservar){
    unsigned int p_inodo_dir = 0;
    unsigned int p_inodo = 0;
    unsigned int p_entrada = 0;
    int error;
    printf("\ncamino: %s, reservar: %d\n", camino, reservar);
    if ((error = buscar_entrada(camino, &p_inodo_dir, &p_inodo, &p_entrada, reservar, 6)) < 0) {
        mostrar_error_buscar_entrada(error);
    }
    printf("**********************************************************************\n");
    return;
}


/*
    Programa para determinar la información almacenada en el superbloque, en el mapa de bits o en el array de inodos
    Recibe por los argumentos especificados en la consola, el nombre del dispositivo en la posicion 2 del array
    y la cantidad de bloques de este en la 3ra posicion ???
*/
int main(int argc, char* argv[]){
    if (argc != 2){
        printf("Error de sintaxis. Uso: ./leer_sf <nombre>\n");
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

    printf("DATOS DEL SUPERBLOQUE\n");

    if(bmount(argv[1]) == FALLO){
        printf("Error al montar el disco\n");
        return FALLO;
    }

    struct superbloque SB;
    if (bread(posSB, &SB) == FALLO){
        printf("Error al leer el superbloque\n");
        return FALLO;
    }

    printf("posPrimerBloqueMB = %u\n", SB.posPrimerBloqueMB);
    printf("posUltimoBloqueMB = %u\n", SB.posUltimoBloqueMB);
    printf("posPrimerBloqueAI = %u\n", SB.posPrimerBloqueAI);
    printf("posUltimoBloqueAI = %u\n", SB.posUltimoBloqueAI);
    printf("posPrimerBloqueDatos = %u\n", SB.posPrimerBloqueDatos);
    printf("posUltimoBloqueDatos = %u\n", SB.posUltimoBloqueDatos);
    printf("posInodoRaiz = %u\n", SB.posInodoRaiz);
    printf("posPrimerInodoLibre = %u\n", SB.posPrimerInodoLibre);
    printf("cantBloquesLibres = %u\n", SB.cantBloquesLibres);
    printf("cantInodosLibres %u\n", SB.cantInodosLibres);
    printf("totBloques = %u\n", SB.totBloques);
    printf("totInodos = %u\n\n", SB.totInodos);

#if DEBUGN2
    printf("sizeof struct superbloque = %lu\n", sizeof(struct superbloque));
    printf("sizeof struct inodo = %lu\n\n", sizeof(struct inodo));
    printf("RECORRIDO DE LA LISTA ENLAZADA DE INODOS LIBRES\n");

    struct inodo AI[(SB.posUltimoBloqueAI-SB.posPrimerBloqueAI+1)*(BLOCKSIZE/INODOSIZE)];

    for (unsigned int i = SB.posPrimerBloqueAI; i <= SB.posUltimoBloqueAI; i++){   // Leemos todo el array de inodos a RAM
        bread(i, &AI[(i-SB.posPrimerBloqueAI)*(BLOCKSIZE/INODOSIZE)]);
    }

    unsigned int pos = SB.posPrimerInodoLibre;
    while(pos != UINT_MAX){ // Recorremos la lista de inodos libres
        printf("%u ", pos);
        pos = AI[pos].punterosDirectos[0];
    }
    printf("%u\n", pos);
#endif

#if DEBUGN3
    printf("RESERVAMOS UN BLOQUE Y LUEGO LO LIBERAMOS\n");
    int bloqueReservado = reservar_bloque();
    bread(posSB, &SB);
    printf("Se ha reservado el bloque físico nº %i que era el 1º libre indicado por el MB.\n", bloqueReservado);
    printf("SB.cantBloquesLibres: %i\n", SB.cantBloquesLibres);
    liberar_bloque(bloqueReservado);
    bread(posSB, &SB);
    printf("Liberamos ese bloque, y después SB.cantBloquesLibres: %i\n\n", SB.cantBloquesLibres);
    printf("MAPA DE BITS CON BLOQUES DE METADATOS OCUPADOS\n");
    int bit = leer_bit(posSB);
    printf("leer_bit(%i) = %i\n", posSB, bit);
    bit = leer_bit(SB.posPrimerBloqueMB);
    printf("leer_bit(%i) = %i\n", SB.posPrimerBloqueMB, bit);
    bit = leer_bit(SB.posUltimoBloqueMB);
    printf("leer_bit(%i) = %i\n", SB.posUltimoBloqueMB, bit);
    bit = leer_bit(SB.posPrimerBloqueAI);
    printf("leer_bit(%i) = %i\n", SB.posPrimerBloqueAI, bit);
    bit = leer_bit(SB.posUltimoBloqueAI);
    printf("leer_bit(%i) = %i\n", SB.posUltimoBloqueAI, bit);
    bit = leer_bit(SB.posPrimerBloqueDatos);
    printf("leer_bit(%i) = %i\n", SB.posPrimerBloqueDatos, bit);
    bit = leer_bit(SB.posUltimoBloqueDatos);
    printf("leer_bit(%i) = %i\n", SB.posUltimoBloqueDatos, bit);

    printf("\nDATOS DEL DIRECTORIO RAIZ\n");
    struct tm *ts;
    char atime[80];
    char mtime[80];
    char ctime[80];
    struct inodo inodo;
    int ninodo = 0;
    leer_inodo(ninodo, &inodo);
    ts = localtime(&inodo.atime);
    strftime(atime, sizeof(atime), "%a %Y-%m-%d %H:%M:%S", ts);
    ts = localtime(&inodo.mtime);
    strftime(mtime, sizeof(mtime), "%a %Y-%m-%d %H:%M:%S", ts);
    ts = localtime(&inodo.ctime);
    strftime(ctime, sizeof(ctime), "%a %Y-%m-%d %H:%M:%S", ts);
    printf("tipo: %c\n", inodo.tipo);
    printf("permisos: %i\n", inodo.permisos);
    printf("ID: %d \nATIME: %s \nMTIME: %s \nCTIME: %s\n", ninodo, atime, mtime, ctime);
    printf("nlinks: %i\n", inodo.nlinks);
    printf("tamEnBytesLog: %i\n", inodo.tamEnBytesLog);
    printf("numBloquesOcupados: %i\n", inodo.numBloquesOcupados);
#endif

#if DEBUGN4
    int idx = reservar_inodo('f', 6);
    struct inodo inodo;
    leer_inodo(idx, &inodo);
    int bl[] = {8, 204, 30004, 400004, 468750};

    for (int i = 0; i < 5; i++) {
        traducir_bloque_inodo(&inodo, bl[i], 1);
    }

    struct tm *ts;
    char atime[80];
    char mtime[80];
    char ctime[80];
    ts = localtime(&inodo.atime);
    strftime(atime, sizeof(atime), "%a %Y-%m-%d %H:%M:%S", ts);
    ts = localtime(&inodo.mtime);
    strftime(mtime, sizeof(mtime), "%a %Y-%m-%d %H:%M:%S", ts);
    ts = localtime(&inodo.ctime);
    strftime(ctime, sizeof(ctime), "%a %Y-%m-%d %H:%M:%S", ts);
    printf("\nDATOS DEL INODO RESERVADO %d\n", idx);
    printf("tipo: %c\n", inodo.tipo);
    printf("permisos: %i\n", inodo.permisos);
    printf("ID: %d \nATIME: %s \nMTIME: %s \nCTIME: %s\n", idx, atime, mtime, ctime);
    printf("nlinks: %i\n", inodo.nlinks);
    printf("tamEnBytesLog: %i\n", inodo.tamEnBytesLog);
    printf("numBloquesOcupados: %i\n", inodo.numBloquesOcupados);

    if (bread(posSB, &SB) == FALLO){
        printf("Error al leer el superbloque\n");
        return FALLO;
    }
    printf("SB.posPrimerInodoLibre = %u\n", SB.posPrimerInodoLibre);
#endif

#if DEBUGN7
      //Mostrar creación directorios y errores

    //mostrar_buscar_entrada("/pruebas/pruebasFic", 1); //ERROR_CAMINO_INCORRECTO
    //mostrar_buscar_entrada("/pruebas/pruebas/abcde", 1); //ERROR_CAMINO_INCORRECTO


    mostrar_buscar_entrada("pruebas/", 1); //ERROR_CAMINO_INCORRECTO
    mostrar_buscar_entrada("/pruebas/", 0); //ERROR_NO_EXISTE_ENTRADA_CONSULTA
    mostrar_buscar_entrada("/pruebas/docs/", 1); //ERROR_NO_EXISTE_DIRECTORIO_INTERMEDIO
    mostrar_buscar_entrada("/pruebas/", 1); // creamos /pruebas/
    mostrar_buscar_entrada("/pruebas/docs/", 1); //creamos /pruebas/docs/
    mostrar_buscar_entrada("/pruebas/docs/doc1", 1); //creamos /pruebas/docs/doc1
    mostrar_buscar_entrada("/pruebas/docs/doc1/doc11", 1);
    //ERROR_NO_SE_PUEDE_CREAR_ENTRADA_EN_UN_FICHERO
    mostrar_buscar_entrada("/pruebas/", 1); //ERROR_ENTRADA_YA_EXISTENTE
    mostrar_buscar_entrada("/pruebas/docs/doc1", 0); //consultamos /pruebas/docs/doc1
    mostrar_buscar_entrada("/pruebas/docs/doc1", 1); //creamos /pruebas/docs/doc1
    mostrar_buscar_entrada("/pruebas/casos/", 1); //creamos /pruebas/casos/
    mostrar_buscar_entrada("/pruebas/docs/doc2", 1); //creamos /pruebas/docs/doc2

#endif

    if(bumount()==FALLO){
        return FALLO;
    }

    return EXITO;
}
