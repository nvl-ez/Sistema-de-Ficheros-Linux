// Krishna Jorda Jimenez, Margalida Covas Roig , Nahuel Vazquez
#include "bloques.h"
#include "semaforo_mutex_posix.h"
static sem_t *mutex;
static unsigned int inside_sc = 0;
static int descriptor=0;

/*
    Abre o crea un fichero y devuelve el descriptor de este
    Utilizada por:
    - Ejecutable escribir
    - Ejecutable leer_sf 
    - Ejecutable leer
    - Ejecutable mi_mkfs
    - Ejecutable permitir
    - Ejecutable truncar
*/
int bmount(const char *camino)
{
    if (descriptor > 0) {
       close(descriptor);
    }

  
    //umask(000);
    //Variables
    descriptor = open(camino, O_RDWR | O_CREAT, 0666);
    if (descriptor==-1){
        perror("Error");
    }

    if (!mutex)  {  // Inicializamos el semaforo para todo el sistema de ficheros
        mutex = initSem();
        if (mutex == SEM_FAILED)
            return -1;
    }

    return descriptor;
}

/*
    Cierra el enlace con el archivo y devuelve el estado del cierre
    Utilizada por:
    - Ejecutable escribir
    - Ejecutable leer_sf 
    - Ejecutable leer
    - Ejecutable mi_mkfs
    - Ejecutable permitir
    - Ejecutable truncar
*/

int bumount()
{
    descriptor = close(descriptor);
    if (descriptor==FALLO){
        perror("Error");
    }
    deleteSem();
    return descriptor;
}

/*
    Escribe un bloque de datos (*buf) en el bloque fisico cuya posicion es nbloque

    Devuelve -1 si ha habido error y el numero de bytes escritos si ha ido bien
    Utilizada por:
    - initMB()
    - initSB()
    - initAI()
    - escribir_bit()
    - escribir_inodo()
    - reservar_inodo()
    - reservar_bloque()
    - liberar_bloque()
    - traducir_bloque_inodo()
    - liberar_bloque_punteros()
    - liberar_inodo()
    - mi_write_f()
    - Ejecutable mi_mkfs
*/
int bwrite(unsigned int nbloque, const void *buf){

    // Posicionamos el puntero asociado con el descriptor de fichero
    int status = lseek(descriptor, nbloque*BLOCKSIZE, SEEK_SET);

    if (status == FALLO){
        fprintf(stderr, "bwrite() -> Error %d: %s\n", errno, strerror(errno));
        //fflush(stderr);
        return FALLO;
    }
    
    // Escribimos el contenido del buffer en la posicion correspondiente al nbloque
    status = write(descriptor, buf, BLOCKSIZE);

    if (status == FALLO){
        fprintf(stderr, "bwrite() -> Error %d: %s\n", errno, strerror(errno));
        fflush(stderr);
        return FALLO;
    }
    return status;
}

/*
    Lee un bloque del dispositivo virtual indicado por nbloque.
    Para ello se volcará en un buffer de memoria, apuntado por *buf, los nbytes 
    contenidos a partir de la posición del dispositivo virtual correspondiente a nbloque
 
    Devuelve el numero de bytes leidos o -1 si hay error.
    Utilizada por:
    - initMB()
    - initAI()
    - escribir_bit()
    - leer_bit()
    - escribir_inodo()
    - leer_inodo()
    - reservar_inodo()
    - reservar_bloque()
    - liberar_bloque()
    - traducir_bloque_inodo()
    - liberar_bloque_punteros()
    - liberar_inodo()
    - mi_write_f()
    - mi_read_f()
    - Ejecutable leer_sf()
    - Ejecutable mi_mkfs()
 */
int bread(unsigned int nbloque, void *buf)
{
    // Cálculo del desplazamiento
    int despl = nbloque * BLOCKSIZE;
    // Movemos el puntero
    if (lseek(descriptor, despl, SEEK_SET) != -1)
    {
        // Leemos el bloque
        size_t numBytes = read(descriptor, buf, BLOCKSIZE);

        // Error al leer
        if (numBytes < 0)
        {
            fprintf(stderr, "bread() -> Error %d: %s\n", errno, strerror(errno));
            //fflush(stderr);
            return FALLO;
        }
        return numBytes;
    }
    else
    {
        fprintf(stderr, "bread() -> Error %d: %s\n", errno, strerror(errno));
        //fflush(stderr);
        return FALLO;
    }
}

/* --- MI_WAITSEM (nivel 11) ----------------------------------------------------------------------
    Llama a la funcion de semaforo_mutex_posix.c para realizar un wait.
*/
void mi_waitSem() {
    if (!inside_sc){
        waitSem(mutex);
    }
    inside_sc++;
}

/* --- MI_SIGNALSEM (nivel 11) ----------------------------------------------------------------------
    Llama a la funcion de semaforo_mutex_posix.c para realizar un signal.
*/
void mi_signalSem() {
    inside_sc--;
    if (!inside_sc){
        signalSem(mutex);
    }
}
