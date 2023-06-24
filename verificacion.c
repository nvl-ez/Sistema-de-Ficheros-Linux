// Krishna Jorda Jimenez, Margalida Covas Roig , Nahuel Vazquez
#include "verificacion.h"

int main(int argc, char const *argv[]){
    if (argc != 3){  // Comprobamos sintaxis
        fprintf(stderr, "Argumentos Invalidos: ./simulación <disco> <dir_sim>\n");
        return FALLO;
    }

    const char* dir_sim = argv[2];

    if (bmount(argv[1]) == FALLO)  // Montamos el disco
        return FALLO;

    // Calculamos el numero de entradas del directorio padre
    struct STAT stat;
    mi_stat(dir_sim, &stat);
    int numEntradas = stat.tamEnBytesLog / sizeof(struct entrada);

    if (numEntradas != NUMPROCESOS){  // Debe haber un directorio por cada proceso
        fprintf(stderr, ROJO"Num directorios no conincide con num procesos\n"RES);
        bumount();
        return FALLO;
    }

    // Creamos el fichero del informe
    char fichero[100];
    sprintf(fichero, "%s%s", dir_sim, "informe.txt");
    if (mi_creat(fichero, 6) < 0){
        bumount(argv[1]);
        return FALLO;
    }

    // --------------------------------------------------------------

    struct entrada entradas[numEntradas];  // Buffer de entradas
    int a = mi_read(dir_sim, &entradas, 0, numEntradas*sizeof(struct entrada));  // Leemos todas las entradas de golpe
    if (a < 0){
        //printf("AAAAAAA\n");
        mostrar_error_buscar_entrada(a);
        return FALLO;
    }

    for (int i = 0; i < numEntradas; i++){  // Recorremos los directorios de cada proceso
        struct entrada entrada = entradas[i];

        // Obtenemos el pid del proceso a partir del nombre de su entrada de directorio
        int pid = atoi(strchr(entrada.nombre, '_') + 1);

        // Obtenemos el nombre del fichero prueba.dat correspondiente al proceso actual
        char caminoFic[50];
        strcpy(caminoFic, dir_sim);
        strcat(caminoFic, entrada.nombre);
        strcat(caminoFic, "/prueba.dat");

        // Inicializamos el struct de informacion
        struct INFORMACION info;
        info.pid = pid;
        info.nEscrituras = 0;

        bool primera = true;

        int cant_registros_buffer = 256;
        struct REGISTRO registros[cant_registros_buffer];  // Buffer de registros

        int leidos = 1;
        int offset = 0;  // Leemos un bloque de escrituras hasta que no queden
        //printf("caminoFic: %s\n", caminoFic);

        while (leidos > 0){

            // Leemos escrituras al buffer "escrituras"
            memset(registros, 0, sizeof(registros));
            // caminoFic es el fichero prueba.dat correspondiente al proceso
            leidos = mi_read(caminoFic, registros, offset, sizeof(registros));
            if (leidos < 0){
                //printf("BBBBB\n");
                mostrar_error_buscar_entrada(leidos);  // Da "Permiso denegado de lectura"
                return FALLO;
            }

            // Para cada escritura del buffer
            for (int j = 0; j < (leidos/sizeof(struct REGISTRO));j++){
                /*
                if (registros[j].pid != 0){
                    printf("registros[j].pid: %d, j=%d, max=%ld\n", registros[j].pid, j, leidos/sizeof(struct REGISTRO));
                    printf("nEscritura: %d\n", registros[j].nEscritura);
                    printf("nRegistro: %d\n", registros[j].nRegistro);
                }*/

                if (registros[j].pid == pid){  // Si la escritura/registro es valida

                    if (primera){  // Si es la primera validada, es la de menor posicion
                        info.MenorPosicion = registros[j];
                        info.PrimeraEscritura = registros[j];
                        info.UltimaEscritura = registros[j];
                        primera = false;
                    }
                    else{
                        if (registros[j].nEscritura < info.PrimeraEscritura.nEscritura)  // Actualizamos la primera escritura
                            info.PrimeraEscritura = registros[j];

                        else if (registros[j].nEscritura > info.UltimaEscritura.nEscritura) // Actualizamos la ultima escritura
                            info.UltimaEscritura = registros[j];
                    }

                    info.nEscrituras++;
                    info.MayorPosicion = registros[j];  // La ultima validada, sera la de mayor posicion
                }
            }

            offset += sizeof(registros);
        }
        fprintf(stderr, GRIS"[%d) %d Escrituras validadas en %s]\n"RES, i+1, info.nEscrituras, caminoFic);
        escribirInfo(&info, i, fichero);
    }

    bumount();
    return EXITO;
}

void escribirInfo(const struct INFORMACION *info, int proceso, char* fic){
    char m[512];  // Guarda el mensaje para este proceso
     memset(m, 0, 512);

    //Formateamos la fecha
    char tMenorPos[24];
    char tMayorPos[24];
    char t1Escritura[24];
    char tNEscritura[24];
    struct tm *tm;

    tm = localtime(&info->MenorPosicion.fecha);
    strftime(tMenorPos, sizeof(tMenorPos), "%a %d-%m-%Y %H:%M:%S", tm);
    tm = localtime(&info->MayorPosicion.fecha);
    strftime(tMayorPos, sizeof(tMayorPos), "%a %d-%m-%Y %H:%M:%S", tm);
    tm = localtime(&info->PrimeraEscritura.fecha);
    strftime(t1Escritura, sizeof(t1Escritura), "%a %d-%m-%Y %H:%M:%S", tm);
    tm = localtime(&info->UltimaEscritura.fecha);
    strftime(tNEscritura, sizeof(tNEscritura), "%a %d-%m-%Y %H:%M:%S", tm);

    sprintf(m, "PID: %d\n", info->pid);
    sprintf(m+strlen(m), "Numero de escrituras: %d\n", info->nEscrituras);
    sprintf(m+strlen(m), "Primera Escritura\t%d\t%d\t%s\n", info->PrimeraEscritura.nEscritura, info->PrimeraEscritura.nRegistro, t1Escritura);
    sprintf(m+strlen(m), "Ultima Escritura\t%d\t%d\t%s\n", info->UltimaEscritura.nEscritura, info->UltimaEscritura.nRegistro, tNEscritura);
    sprintf(m+strlen(m), "Menor Posicion\t\t%d\t%d\t%s\n", info->MenorPosicion.nEscritura, info->MenorPosicion.nRegistro, tMenorPos);
    sprintf(m+strlen(m), "Mayor Posicion\t\t%d\t%d\t%s\n\n", info->MayorPosicion.nEscritura, info->MayorPosicion.nRegistro, tMayorPos);

    int error = mi_write(fic, &m, proceso*512, 512);
    if (error < 0){
        //printf("CCCCC\n");
        mostrar_error_buscar_entrada(error);
        bumount();
    }
}










