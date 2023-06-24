// Krishna Jorda Jimenez, Margalida Covas Roig , Nahuel Vazquez
#include "simulacion.h"

/*FUNCIÓN ENTERRADOR*/
static int acabados = 0;
void reaper()
{
    pid_t ended;
    signal(SIGCHLD, reaper);
    while ((ended = waitpid(-1, NULL, WNOHANG)) > 0)
    {
        acabados++;
    }
}

int main(int argc, char const *argv[])
{
    signal(SIGCHLD, reaper);

    // Comprobamos la sintaxis del comando
    if (argc != 2)
    {
        fprintf(stderr, "Argumentos Invalidos: ./simulación <disco> \n");
        return FALLO;
    }

    // Montamos el dispositivo
    if (bmount(argv[1]) == FALLO)
    {
        fprintf(stderr, ROJO"Error al montar el disco.\n"RES);
    }

    // Creamos el directorio de simulación
    time_t t = time(NULL);
    struct tm tm = *localtime(&t);
    char *tiempo = malloc(40);
    sprintf(tiempo, "%d%02d%02d%02d%02d%02d", tm.tm_year + 1900,
            tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);

    char *camino = malloc(40);
    strcat(strcpy(camino, "/simul_"), tiempo);
    strcat(camino, "/");
    fprintf(stderr, "Directorio de simulación:%s \n", camino);
    
    if ((mi_creat(camino, 6)) < 0)
    {
        bumount();
        fprintf(stderr, ROJO"Error al crear el directorio: %s\n"RES, camino);
        return -1;
    }

    for (int i = 1; i <= NUMPROCESOS;i++)
    {
        //fprintf(stderr, GRIS"[padre -> ejecutando bucle iteracion %d]\n"RES, i);
        pid_t pid = fork();
        // Si es el hijo
        if (pid == 0)
        {
            int escrituras = 0;
            //fprintf(stderr, GRIS"[proceso hijo %d creado]\n"RES, i);
            // montamos el dispositivo
            if (bmount(argv[1]) == FALLO)
            {
                fprintf(stderr, ROJO"Error al montar el disco.\n"RES);
            }
            
            // creamos el directorio del proceso añadiendo el PID al nombre
            char nombre[100];
            char buf[80];
            strcpy(buf, camino);
            sprintf(nombre, "%sproceso_%d/", buf, getpid());
            
            //fprintf(stderr, "[hijo %d -> Nombre del directorio: '%s']\n", i, nombre);
            int err = mi_creat(nombre, 6);
            if (err < 0)
            {
                bumount();
                fprintf(stderr, ROJO"Error creación directorio '%s'. Error: %d\n"RES, nombre, err);
                fflush(stderr);
                exit(0);
            }
            //fprintf(stderr, "[hijo %d -> Directorio: %s creado]\n", i, nombre);

            char nomfichero[110];
            sprintf(nomfichero, "%sprueba.dat", nombre);
            //printf("nomFichero: %s\n", nomfichero);
            err = mi_creat(nomfichero, 6);
            if (err != 0)
            {
                bumount();
                fprintf(stderr, ROJO"Error creación fichero %s (Error: %d)\n"RES, nomfichero, err);
                fflush(stderr);
                exit(0);
            }
            //fprintf(stderr, "Fichero del proceso %i '%s' creado\n", i, nomfichero);

            
            // inicializamos la semilla de numeros aleatrorios
            srand(time(NULL) + getpid());
            for (int j = 1; j <= NUMESCRITURAS; j++)
            {
                struct REGISTRO registro;

                registro.fecha = time(NULL);
                registro.pid = getpid();
                registro.nEscritura = j;
                registro.nRegistro = rand() % REGMAX;
                // Escribimos el registro
                if (mi_write(nomfichero, &registro, registro.nRegistro * sizeof(struct REGISTRO), sizeof(struct REGISTRO)) < 0)
                {
                    fprintf(stderr, "Fallo en la escritura en %s\n", camino);
                    return FALLO;
                }
                //fprintf(stderr, "[hijo %d → Escritura %i en %s. pid: %d]\n", i, j, buf, registro.pid);
                escrituras++;
                usleep(50000);
            }
            fprintf(stderr, GRIS"[Proceso %d: Completadas %d escrituras en %s]\n"RES, i, escrituras, nomfichero);
            bumount();
            //fprintf(stderr, GRIS"[proceso %d -> desmontado disco y saliendo]\n"RES, i);
            //fflush(stderr);
            return 0;
        }
        usleep(150000);
    }
    while (acabados < NUMPROCESOS)
    {
        pause();
    }
    if (bumount() < 0)
    {
        fprintf(stderr, "Error al desmontar el dispositivo\n");
        return EXITO;
    }
    return 0;
}
