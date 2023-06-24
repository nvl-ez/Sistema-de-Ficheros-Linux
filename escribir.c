// Krishna Jorda Jimenez, Margalida Covas Roig , Nahuel Vazquez
#include "ficheros.h"
#define NOFFSETS 5

// argv[1]=nombre disco, argv[2]=texto a escribir, argv[3]= diferentes_inodos
int main(int argc, char *argv[]){
    // Bloques logicos 8, 204, 30004, 400004, y 468750 respectivamente
    int offsets[] = {9000, 209000, 30725000, 409605000, 480000000};  // NOFFSETS = 5

    if (argc < 4){  // Alertamos si la sintaxis no es correcta
        fprintf(stderr, "Sintaxis: ./escribir <nombre_dispositivo> <\"$(cat fichero)\"> <diferentes_inodos>\n");
        fprintf(stderr, "Offsets: ");
        for (int i = 0; i < 5; i++) {fprintf(stderr, "%d, ", offsets[i]);}
        fprintf(stderr, "\nSi diferentes_inodos=1 se reserva un inodo distinto para cada offset\n");

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

    printf("longitud texto: %d\n\n", (int) strlen(argv[2]));

    int ninodo;  // Numero del inodo reservado
    for (int i = 0; i < NOFFSETS; i++){
        if (i==0 || atoi(argv[3])){  // Si es el primer offset o queremos inodos diferentes para cada offset
            // Reserva un inodo de tipo fichero con permisos de escritura y lectura
            ninodo = reservar_inodo('f', 6);
        }

        printf("Num inodo reservado: %d\n", ninodo);
        printf("Offset: %d\n", offsets[i]);
        int bescritos = mi_write_f(ninodo, argv[2], offsets[i], strlen(argv[2]));
        if (bescritos == FALLO){
            fprintf(stderr, ROJO"Error al escribir en el fichero\n"RES);
            return FALLO;
        }

        printf("Bytes escritos: %d\n", bescritos);

        struct STAT info;
        mi_stat_f(ninodo, &info);
        printf("tamEnBytesLog=%d\n", info.tamEnBytesLog);
        printf("numBloquesOcupados=%d\n\n", info.numBloquesOcupados);

        // Test
        /*
        printf("-------PRUEBA----------\n");
        char buffer[30];
        memset(buffer, 0, 20);
        mi_read_f(ninodo, buffer, offsets[i], 20);
        write(1, buffer, 20);
        printf("\n---------------------\n\n\n");
        */
    }

    if (bumount(argv[1])==FALLO)
        return FALLO;

    return EXITO;
}
