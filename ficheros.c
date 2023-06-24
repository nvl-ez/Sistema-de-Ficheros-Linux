// Krishna Jorda Jimenez, Margalida Covas Roig , Nahuel Vazquez
#include "ficheros.h"

/* --- MI_WRITE_F (nivel 5) -----------------------------------------------------------------------
    Escribe el contenido de buf_original (un buffer de tamaño nbytes) en el fichero correspondiente con
    el numero de inodo ninodo. Empezara a escribir el buffer en la posicion logica de ese fichero offset
    (en bytes). 
    
    Devuelve la cantidad de bytes escritos, -1 si hay fallo general y -2 si no hay permisos de escritura
    Utilizada por:
        - Ejecutable escribir.c
*/

int mi_write_f(unsigned int ninodo, const void *buf_original, unsigned int offset, unsigned int nbytes){
    mi_waitSem();
    struct inodo inodo;
    if (leer_inodo(ninodo, &inodo) == FALLO){
        mi_signalSem();
        return FALLO;
    }

    if ((inodo.permisos & 2) != 2 ){  // w=1? (rwx & 010 = 0w0, si 0w0 == 2 -> w=1, si 0w0 != 2 -> w=0)
        mi_signalSem();
        return -2;
        //return FALLO;
    }

    // Calculamos el primer y ultimo bloque logico que hay que modificar dado offset y nbytes
    int primerBL = offset / BLOCKSIZE;
    int ultimoBL = (offset + nbytes - 1) / BLOCKSIZE;

    // Calculamos en que posicion de esos bloques hay que empezar o dejar de escribir
    int desp1 = offset % BLOCKSIZE;
    int desp2 = (offset + nbytes - 1) % BLOCKSIZE;

    int bescritos = 0;

    if (primerBL == ultimoBL){  // CASO 1: El buffer cabe en un solo bloque ------------------------------
        // Obtenemos el Nº de bloque fisico correspondiente a primerBL y lo reservamos si es necesario
        int nbfisico = traducir_bloque_inodo(&inodo, primerBL, 1);

        char buf_bloque[BLOCKSIZE];
        if (bread(nbfisico, buf_bloque) == FALLO){  // Leemos el bloque fisico correspondiente al primer BL
            mi_signalSem();
            return FALLO;
        }

        memcpy(buf_bloque + desp1, buf_original, nbytes);  // Copiamos los datos empezando en offset

        // Escribimos el primer bloque modificado
        if (bwrite(nbfisico, buf_bloque) == FALLO){
            mi_signalSem();
            return FALLO;
        }

        bescritos += nbytes;
    }
    else{  // -------------------- CASO 2: El buffer necesita varios bloques -----------------------------
        char buf_bloque[BLOCKSIZE];

        // Fase 1: Primer bloque logico
        int nbfisico = traducir_bloque_inodo(&inodo, primerBL, 1);
        if (bread(nbfisico, buf_bloque) == FALLO){  // Leemos el primer bloque
            mi_signalSem();
            return FALLO;
        }

        memcpy(buf_bloque+desp1, buf_original, BLOCKSIZE-desp1);  // Sobreescribimos desde desp1 hasta el final

        // Escribimos el primer bloque modificado
        if (bwrite(nbfisico, buf_bloque) == FALLO){
            mi_signalSem();
            return FALLO;
        }

        bescritos += BLOCKSIZE-desp1;

        // Fase 2: Bloques logicos intermedios
        for (int bl = primerBL+1; bl < ultimoBL; bl++){  // Recorremos todos los bloques intermedios
            nbfisico = traducir_bloque_inodo(&inodo, bl, 1);

            // Escribimos los bytes correspondientes directamente
            if (bwrite(nbfisico, buf_original + (BLOCKSIZE - desp1) + (bl - primerBL - 1) * BLOCKSIZE) ==FALLO){
                mi_signalSem();
                return FALLO;
            }

            bescritos += BLOCKSIZE;
        }

        // Fase 3: Ultimo bloque logico
        nbfisico = traducir_bloque_inodo(&inodo, ultimoBL, 1);
        if (bread(nbfisico, buf_bloque) == FALLO){
            mi_signalSem();
            return FALLO;
        }

        memcpy(buf_bloque, buf_original+(nbytes-desp2-1), desp2+1); // Sobreescribimos hasta desp2 (desp2 + 1 bytes)

        // Escribimos el ultimo bloque logico modificado
        if (bwrite(nbfisico, buf_bloque)==FALLO){
            mi_signalSem();
            return FALLO;
        }

        bescritos += desp2+1;
    }

    // Actualizamos el tamaño del fichero, el ctime y el mtime
    if (offset+bescritos >= inodo.tamEnBytesLog){
        inodo.tamEnBytesLog = offset+bescritos;
        inodo.ctime = time(NULL);
    }
    inodo.mtime = time(NULL);

    if (escribir_inodo(ninodo, &inodo) == FALLO){  // Guardamos el inodo modificado
        mi_signalSem();
        return FALLO;
    }

    mi_signalSem();
    return bescritos;
} 

/* --- MI_READ_F (nivel 5) ------------------------------------------------------------------------
    Lee informacion del fichero correspondiente al numero de inodo pasado como parametreo y la guarda
    en el buffer buf_original pasado por parametro. Empieza a leer en la posicion offset y lee nbytes
    (ambos valores pasados por parametro). 
 
    Devuelve la cantidad de bytes leidos, -1 si hay fallo general y -2 si no hay permisos de lectura
    Utilizada por:
    - Ejecutable leer
*/
int mi_read_f(unsigned int ninodo, void *buf_original, unsigned int offset, unsigned int nbytes){

    // Leemos el inodo y actualizamos el atime
    struct inodo inodo;
    mi_waitSem();
    if (leer_inodo(ninodo, &inodo)==FALLO){
        mi_signalSem();
        return FALLO;
    }

    inodo.atime = time(NULL);

    if (escribir_inodo(ninodo, &inodo)==FALLO){
        mi_signalSem();
        return FALLO;
    }
    mi_signalSem();

    if ((inodo.permisos&4) !=4) // r=1? rwx&100 = r00. si r00 = 4 -> r=1, si no r = 0
        return -2;
        //return FALLO;

    if (offset >= inodo.tamEnBytesLog)  // Quiere empezar a leer mas alla del EOF
        return 0;  // Hemos leido 0 bytes

    if (offset+nbytes >= inodo.tamEnBytesLog)  // Si quiere leer mas alla del EOF
        nbytes = inodo.tamEnBytesLog - offset;  // Leemos solo desde offset a EOF

    // Calculamos el primer y ultimo bloque logico que hay que modificar dado offset y nbytes
    int primerBL = offset / BLOCKSIZE;
    int ultimoBL = (offset + nbytes - 1) / BLOCKSIZE;

    // Calculamos en que posicion de esos bloques hay que empezar o dejar de leer
    int desp1 = offset % BLOCKSIZE;
    int desp2 = (offset + nbytes - 1) % BLOCKSIZE;

    char buf_bloque[BLOCKSIZE];
    int bleidos = 0;

    if (primerBL==ultimoBL){ // CASO 1: LEEMOS DE UN SOLO BLOQUE ---------------------------------------------
        int nbfisico = traducir_bloque_inodo(&inodo, primerBL, 0);  // Averiguamos cual es el BF
        if (nbfisico != FALLO){
            if (bread(nbfisico, buf_bloque) == FALLO)  // Leemos el BF correspondiente al BL
                return FALLO;

            memcpy(buf_original, buf_bloque+desp1, nbytes);  // Copiamos desde desp1 hasta desp1+nbytes-1
        }
        bleidos += nbytes;
    }
    else{  // ----------------- CASO 2: TENEMOS QUE LEER VARIOS BLOQUES --------------------------------------
        // Fase 1: Primer bloque logico
        int nbfisico = traducir_bloque_inodo(&inodo, primerBL, 0);
        if (nbfisico!=FALLO){
            // Leemos el primer bloque logico

            if (bread(nbfisico, buf_bloque) == FALLO)
                return FALLO;

            memcpy(buf_original, buf_bloque+desp1, BLOCKSIZE-desp1);
        }
        bleidos += BLOCKSIZE-desp1;

        // Fase 2: Bloques logicos intermedios
        for (int bl = primerBL+1; bl < ultimoBL; bl++){
            nbfisico = traducir_bloque_inodo(&inodo, bl, 0);
            if (nbfisico!=FALLO){
                if (bread(nbfisico, buf_bloque) == FALLO)
                    return FALLO;

                memcpy(buf_original + (BLOCKSIZE-desp1)+(bl-primerBL-1)*BLOCKSIZE, buf_bloque, BLOCKSIZE);
            }
            bleidos += BLOCKSIZE;
        }

        // Fase 3: Ultimo bloque logico
        nbfisico = traducir_bloque_inodo(&inodo, ultimoBL,0);
        if (nbfisico!=FALLO){
            if (bread(nbfisico, buf_bloque) == FALLO)
                return FALLO;

            memcpy(buf_original+(nbytes-desp2-1), buf_bloque, desp2+1);
        }

        bleidos += desp2+1;
    }

    return bleidos;
} 

/* --- MI_STAT_F (nivel 5) ------------------------------------------------------------------------
    Devuelve la metainformación de un fichero/directorio (correspondiente al nº de inodo pasado como argumento)
    en el struc STAT tambien pasado por referencia
    Utilizada por:
    - Ejecutable escribir
    - Ejecutable leer
    - Ejecutable truncar
*/
int mi_stat_f(unsigned int ninodo, struct STAT *p_stat){
    //Obtenemos el inodo que se quiere leer
    struct inodo buf_indo;
    if(leer_inodo(ninodo, &buf_indo) == EXITO){
        //Pasamos los parametros del inodo leido al struc STAT
        p_stat->tipo = buf_indo.tipo;
        p_stat->permisos = buf_indo.permisos;
        p_stat->atime = buf_indo.atime;
        p_stat->mtime = buf_indo.mtime;
        p_stat->ctime = buf_indo.ctime;
        p_stat->nlinks = buf_indo.nlinks;
        p_stat->tamEnBytesLog = buf_indo.tamEnBytesLog;
        p_stat->numBloquesOcupados = buf_indo.numBloquesOcupados;
        return EXITO;
    }
    return FALLO;
}

/* --- MI_CHMOD_F (nivel 5) -----------------------------------------------------------------------
    Cambia los permisos de un fichero/directorio (correspondiente al nº de inodo pasado como argumento, ninodo)
    con el valor que indique el argumento permisos.

    Devuelve 0 si ha ido bien o -1 si hay error general, y -2 si el inodo estaba libre
    Utilizada por:
    - Ejecutable permitir
*/
int mi_chmod_f(unsigned int ninodo, unsigned char permisos){
    mi_waitSem();
    
    //Obtenemos el inodo
    struct inodo buf_indo;
    if(leer_inodo(ninodo, &buf_indo) == EXITO){
        if (buf_indo.tipo == 'l'){
            mi_signalSem();
            return -2;
        }

        //Modificacion de los permisos
        buf_indo.permisos = permisos;

        //Modificacion del ctime
        buf_indo.ctime = time(NULL);

        //Sobreescribir el inodo
        if(escribir_inodo(ninodo, &buf_indo) == EXITO){
            mi_signalSem();
            return EXITO;
        }
        mi_signalSem();
        return FALLO;
    }
    mi_signalSem();
    return FALLO;
}

/* --- MI_TRUNCAR_F (nivel 6) ---------------------------------------------------------------------
    Trunca un fichero / directorio correspondiente a 'ninodo' (pasado por argumento), para que ocupe
    'nbytes' bytes (pasado por argumento), liberando los bloques necesarios.
 
    Devuelve la cantidad de bloques liberados, o -1 si hay fallo.
    Utilizada por:
    - Ejecutable truncar
*/
int mi_truncar_f(unsigned int ninodo, unsigned int nbytes){
    mi_waitSem();

    struct inodo inodo;
    if (leer_inodo(ninodo, &inodo) == FALLO){
        mi_signalSem();
        return FALLO;
    }

    if ((inodo.permisos&4) != 4){
        mi_signalSem();
        return FALLO;
    }

    if (nbytes >= inodo.tamEnBytesLog){
        mi_signalSem();
        return FALLO;
    }

    int primerBL;
    if (nbytes % BLOCKSIZE == 0)
        primerBL = nbytes / BLOCKSIZE;
    else
        primerBL = (nbytes/BLOCKSIZE) + 1;

    int liberados = liberar_bloques_inodo(primerBL, &inodo);
    if (liberados == FALLO){
        mi_signalSem();
        return FALLO;
    }

    inodo.mtime = time(NULL);
    inodo.ctime = time(NULL);
    inodo.tamEnBytesLog = nbytes;
    inodo.numBloquesOcupados = inodo.numBloquesOcupados - liberados;
    if (escribir_inodo(ninodo, &inodo) == FALLO){
        mi_signalSem();
        return FALLO;
    }

    mi_signalSem();
    return liberados;
}

