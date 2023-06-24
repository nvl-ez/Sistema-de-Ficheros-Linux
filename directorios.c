// Krishna Jorda Jimenez, Margalida Covas Roig , Nahuel Vazquez
#include "directorios.h"
struct UltimaEntrada CacheEntradas[NCACHE];
int idx_cache=0;

/* --- EXTRAER_CAMINO (nivel 7) -------------------------------------------------------------------
    Dada una cadena de caracteres (camino) que comience por '/'), separa su contenido en dos, y lo guarda en
    el parámetro inicial y el final, y dice si es un directorio o no en  la variable tipo

    Devuelve 0 si va bien y -1 si hay error
    Utilizada por:
        - buscar_entrada()
*/
int extraer_camino(const char *camino, char *inicial, char *final, char *tipo)
{
    // si no comienza con '/'
    if (camino[0] != '/')
    {
        return FALLO;
    }
    // buscamos el siguiente '/'
    char *proxCamino = strchr(camino + 1, '/');
    if (proxCamino != NULL)
    {
        strncpy(inicial, (camino+1), (strlen(camino) - strlen(proxCamino) - 1));
        inicial[strlen(camino) - strlen(proxCamino) - 1] = '\0';
        // guardamos el resto de camino
        strcpy(final, proxCamino);
        // Miramos si es un fichero
        if (final[0] != '/')
        {
            *tipo = (char)'f';
        }
        else
        {
            *tipo = (char)'d';
        }
    }
    //si no hay
    else
    {
        strcpy(inicial, (camino + 1));
        *final= '\0';
        *tipo = (char)'f';
    }
    return EXITO;
}

/* --- BUSCAR_ENTRADA (nivel 7) -------------------------------------------------------------------
    Busca una entrada (correspondiente directorio o fichero de "camino") en el directorio
    correspondiente a "*p_inodo_dir". Si reservar=1, en caso de no encontrar la entrada, la crea,
    con los permisos correspondientes a "permisos".

    Guarda en "*p_inodo" el Nº de inodo a ese fichero/directorio
    y su numero de entrada en "p_entrada".

    Devuelve:
        0 si no hay error
        -1 para fallo general
        Para fallos especificos devuelve los valores especificados en "directorios.h"

    Utilizada por:
        - mi_creat()
        - mi_dir()
        - mi_chmod()
        - mi_stat()
*/
int buscar_entrada(const char *camino_parcial, unsigned int *p_inodo_dir, unsigned int *p_inodo, unsigned int *p_entrada, char reservar, unsigned char permisos){
    struct entrada entradas[NENTRADAS];  // Buffer entradas (de un bloque)

    // CUIDADO! NO PONER SIZEOF, ES STRLEN! :v
    char entrada[strlen(camino_parcial)];  // Entrada que hay que buscar en *p_inodo_dir (inicial)
    char resto[strlen(camino_parcial)];  // Resto de la ruta que pasaremos a buscar_entrada recursivamente (final)
    char tipo;  // Nos dira si entrada es un fichero o un directorio
    struct inodo inodo_dir;

    if (strcmp(camino_parcial, "/") == 0){  // Si el camino la raiz, devolvemos SB.posInodoRaiz
        struct superbloque SB;  // No entiendo
        if (bread(posSB, &SB) == FALLO)
            return FALLO;

        *p_inodo = SB.posInodoRaiz;
        *p_entrada = 0;

        return EXITO;
    }

    if (extraer_camino(camino_parcial, entrada, resto, &tipo) == FALLO){   // Obtenemos tipo, entrada y resto
        return ERROR_CAMINO_INCORRECTO;
    }

#if DEBUGN7
    fprintf(stderr, GRIS"[buscar_entrada()-> inicial: %s, final: %s, reservar: %d]\n"RES, entrada, resto, reservar);
#endif

    if (leer_inodo(*p_inodo_dir, &inodo_dir) == FALLO)  // Leemos el inodo correspondiente a *p_inodo_dir
        return FALLO;

    if ((inodo_dir.permisos&4) != 4)  // Comprobamos permisos
        return ERROR_PERMISO_LECTURA;

    memset(entradas, '\0', BLOCKSIZE);
    int num_entradas = inodo_dir.tamEnBytesLog/sizeof(struct entrada);
    int num_bloques = ((num_entradas-1)/NENTRADAS) +1;
    int idxEnt = 0;

    bool buscar = true;
    for (int idxBL = 0; idxBL < num_bloques && buscar; idxBL++) {  // Recorremos los BL
        // Leemos un bloque entero de entradas
        int leidos = mi_read_f(*p_inodo_dir, entradas, idxBL*BLOCKSIZE, BLOCKSIZE);
        if (leidos == FALLO)
            return FALLO;

        // Comprobamos para cada entrada si es la que buscamos
        for (int i = 0 ; i < NENTRADAS; i++){
            if (!(idxEnt < num_entradas) || strcmp(entradas[i].nombre, entrada) == 0){
                buscar = false;
                break;
            }
            idxEnt++;
        }
    }

    if (idxEnt == num_entradas){  // Si la entrada no existe
        if (!reservar)
            return ERROR_NO_EXISTE_ENTRADA_CONSULTA;

        // reservar=1 y por lo tanto creamos la entrada en inodo_dir

        // Pasaria por ej. si hacemos "touch fichero/fichero2" (y fichero no es un directorio)
        if (inodo_dir.tipo == 'f')
            return ERROR_NO_SE_PUEDE_CREAR_ENTRADA_EN_UN_FICHERO;

        if ((inodo_dir.permisos&2) != 2)
            return ERROR_PERMISO_ESCRITURA;

        strcpy(entradas[0].nombre, entrada);

        int res;
        if (tipo == 'd'){  // Es un directorio
            if (strcmp(resto, "/") == 0){ // Si esta entrada esta al final del camino
                res = reservar_inodo('d', permisos);
                if (res == FALLO)
                    return FALLO;

                entradas[0].ninodo = res;
            }
            else{  // Pasa si no existe una entrada de tipo 'd' pero el camino sigue despues de ella
                return ERROR_NO_EXISTE_DIRECTORIO_INTERMEDIO;
            }
        }
        else{  // Es un fichero
            res = reservar_inodo('f', permisos);
            if (res == FALLO)
                return FALLO;

            entradas[0].ninodo = res;
        }

#if DEBUGN7
            fprintf(stderr, GRIS"[buscar_entrada()-> reservado inodo %d de tipo %c con permisos %d para %s]\n"RES, res, tipo, permisos, entrada);
#endif

        // Guardamos la entrada creada en el directorio *p_inodo_dir
        if (mi_write_f(*p_inodo_dir, &entradas[0], inodo_dir.tamEnBytesLog, sizeof(struct entrada)) == FALLO) {
            liberar_inodo(entradas[0].ninodo);
            return FALLO;
        }

#if DEBUGN7
            fprintf(stderr, GRIS"[buscar_entrada()-> creada entrada [%s,%d]]\n"RES, entradas[0].nombre, entradas[0].ninodo);
#endif
    }

    if (strlen(resto) == 0 || strcmp(resto, "/") == 0){  // Si hemos llegado al final del camino (cortamos la recursividad)

        if (idxEnt < num_entradas) {  // Si hemos encontrado la entrada
            if (reservar)  // Y ademas queriamos reservarla
                return ERROR_ENTRADA_YA_EXISTENTE;

            *p_inodo = entradas[idxEnt%NENTRADAS].ninodo;  // Improtante el %NENTRADAS !!!!!!
            *p_entrada = idxEnt;


        }
        else{  // Si no la hemos encontrado (la hemos reservardo)
            *p_inodo = entradas[0].ninodo;
            *p_entrada = num_entradas; // num_entradas esta desactualizado asi que no hace falta restar 1
        }
        return EXITO;

    }

    // Si no hemos llegado al final del camino
    *p_inodo_dir = entradas[idxEnt%NENTRADAS].ninodo;  // La entrada encontrada se convierte en el nuevo padre donde buscar

    return buscar_entrada(resto, p_inodo_dir, p_inodo, p_entrada, reservar, permisos);
}

/* --- MOSTRAR_ERROR_BUSCAR_ENTRADA (nivel 7) -----------------------------------------------------
    Dado un codigo de error de buscar_entrada(), imprime a stderr el mensaje de error correspondiente
    Utilizada por:
    - mi_cat.c
    - mi_ls.c
    - mi_mkdir.c
    - mi_touch.c
*/
void mostrar_error_buscar_entrada(int error) {
   // fprintf(stderr, "Error: %d\n", error);
   switch (error) {
   case -2: fprintf(stderr, "%sError: Camino incorrecto.\n%s", ROJO, RES); break;
   case -3: fprintf(stderr, "%sError: Permiso denegado de lectura.\n%s", ROJO, RES); break;
   case -4: fprintf(stderr, "%sError: No existe el archivo o el directorio.\n%s", ROJO, RES); break;
   case -5: fprintf(stderr, "%sError: No existe algún directorio intermedio.\n%s", ROJO, RES); break;
   case -6: fprintf(stderr, "%sError: Permiso denegado de escritura.\n%s", ROJO, RES); break;
   case -7: fprintf(stderr, "%sError: El archivo ya existe.\n%s", ROJO, RES); break;
   case -8: fprintf(stderr, "%sError: No es un directorio.\n%s", ROJO, RES); break;
   }
}

/* --- MI_CHMOD (nivel 8) -------------------------------------------------------------------------
    Busca la entrada *camino con buscar_entrada() para obtener el nº de inodo (p_inodo).
    Si la entrada existe se llama a la función correspondiente de ficheros.c pasando el p_inodo
    y los permisos pasados por parametros

    Devuelve 0 si ha ido bien -1 si hay error
    Utilizada por:
        - mi_chmod.c
*/
int mi_chmod(const char *camino, unsigned char permisos){
    unsigned int p_inodo_dir;
    unsigned int p_inodo;
    unsigned int p_entrada;
    int error;
    p_inodo_dir=0;
    p_inodo=0;
    p_entrada=0;
    //variable para el control de los errores
    error=buscar_entrada(camino, &p_inodo_dir, &p_inodo, &p_entrada, 0, permisos);
    if( error<0){
        mostrar_error_buscar_entrada(error);
        return FALLO;
    }
    error= mi_chmod_f(p_inodo, permisos);

    return error;
}

/* --- MI_STAT (nivel 8) --------------------------------------------------------------------------
    Muestra la información del inodo de un fichero o directorio
    Para esto busca la entrada *camino con buscar_entrada() para obtener el p_inodo. Si la entrada
    existe se llama a la función correspondiete de ficheros.c pasándole el p_inodo

    Devuelve el numero de inodo correspondiente si ha ido bien, o -1 si hay error.
    Utilizada por:
    - mi_stat.c
*/
int mi_stat(const char *camino, struct STAT *p_stat){
    unsigned int p_inodo_dir;
    unsigned int p_inodo;
    unsigned int  p_entrada;
    p_inodo_dir=0;
    p_inodo=0;
    p_entrada=0;
    //variable para el control de los errores
    int error;
    error = buscar_entrada(camino, &p_inodo_dir, &p_inodo, &p_entrada, 0, p_stat->permisos);
    mostrar_error_buscar_entrada(error);
    if (error < 0) return FALLO;

    if (mi_stat_f(p_inodo, p_stat) == FALLO)
        return FALLO;

    return p_inodo;
}

/* --- MI_CREAT (nivel 8) -------------------------------------------------------------------------
    Crea un fichero o directorio y su entrada en el directorio padre

    Devuelve 0 si no hay fallo, -1 para fallo general o cualquiera de
    los codigos de fallo que puede devolver buscar_entrada()

    Utilizada por:
        - mi_mkdir.c
*/
int mi_creat(const char *camino, unsigned char permisos) {
    struct superbloque SB;
    mi_waitSem();
    
    
    if (bread(posSB, &SB) == FALLO){
        mi_signalSem();
        return FALLO;
    }

    unsigned int numIn=0;
    unsigned int numEnt=0;

    int ret = buscar_entrada(camino, &SB.posInodoRaiz, &numIn, &numEnt, 1, permisos);
    mi_signalSem();
    return ret;
}

/* --- MI_DIR (nivel 8) ---------------------------------------------------------------------------
    Pone el contenido (para cada entrada, su nombre, y metadatos) del directorio 'camino' en el
    buffer 'buffer'.
    Utilizada por:
    - mi_ls.c
*/
int mi_dir(const char *camino, char *buffer, char tipo, char extended)
{
    struct entrada entradas[NENTRADAS];  // Buffer entradas (de un bloque)
    struct inodo inodo_dir; //Contiene el inodo leido

    //Lectura del superbloqeu para obtener la raiz
    struct superbloque SB; 
    if (bread(posSB, &SB) == FALLO)
        return FALLO;

    char cabecera[TAMFILA*2];//seran 2 filas de cabecera (texto y separador)
    char tmp[40]; //Contendra mas adelante el tiempo qeu se mostrara en el ls y tamaño
    unsigned int total = 0; //contiene la cantidad de entradas
    unsigned int p_inodo;
    unsigned int p_entrada;

    if(extended || tipo == 'f'){
        //se añade la cabecera al buffer del cuerpo
        strcpy(cabecera, "Tipo\tPermisos\tmTime\t\t\tTamaño\tNombre\n----------------------------------------------------------------------\n");
        strcat(buffer, cabecera);
    }   
    //Preparamos el camino para asegurar que le pasamos un directorio a buscar_entrada()
    char nombre[TAMNOMBRE];
    if(tipo == 'f'){ //si se trata de un fichero
        char* offset = strrchr(camino, '/');
        strcpy(nombre, offset+1); //Obtenemos el nombre del archivo
    }

    //buscamos la el inodo que contiene la entrada del camino y verificamos que no hay ningun error
    int err = buscar_entrada(camino, &SB.posInodoRaiz, &p_inodo, &p_entrada, 0, 4);
    if(err) return err;

    if (leer_inodo(p_inodo, &inodo_dir) == FALLO)  // Leemos el inodo correspondiente a p_inodo
        return FALLO;

    if ((inodo_dir.permisos&4) != 4)  // Comprobamos permisos
        return ERROR_PERMISO_LECTURA;

    if(inodo_dir.tipo != tipo){
        return ERROR_CAMINO_INCORRECTO;
    }

    if(tipo == 'd'){
        //Limpiamos el array de entradas
        memset(entradas, '\0', BLOCKSIZE);
        int num_entradas = inodo_dir.tamEnBytesLog/sizeof(struct entrada); //la cantidad de entradas en el inodo
        int num_bloques = ((num_entradas-1)/NENTRADAS) +1; //la cantidad de bloques de entradas

        for (int idxBL = 0; idxBL < num_bloques; idxBL++) {  // Recorremos los BL
            //MEJORA: Leemos un bloque entero de entradas para reducir el nombre de accesos al disco
            int leidos = mi_read_f(p_inodo, entradas, idxBL*BLOCKSIZE, BLOCKSIZE);
            if (leidos == FALLO)
                return FALLO;

            struct inodo inodoDeEntrada;
            for(int i = 0; i < leidos/sizeof(struct entrada); i++){ //Lectura de cada entrada para obtener su inodo
                if (leer_inodo(entradas[i].ninodo, &inodoDeEntrada) == FALLO)
                    return FALLO;

                total++; //incrementa el contador de entradas
                if(extended){ //verifica si se tiene que hacer la version de -l o la simplificada
                    //Se añade el tipo
                    if(inodoDeEntrada.tipo == 'd'){
                        strcat(buffer, AZUL);
                        strcat(buffer, "d");
                    } else{
                        strcat(buffer, LILA);
                        strcat(buffer, "f");
                    }
                    
                    strcat(buffer, "\t");

                    //Se añaden los permisos
                    if (inodoDeEntrada.permisos & 4) strcat(buffer, "r"); else strcat(buffer, "-");
                    if (inodoDeEntrada.permisos & 2) strcat(buffer, "w"); else strcat(buffer, "-");
                    if (inodoDeEntrada.permisos & 1) strcat(buffer, "x"); else strcat(buffer, "-");
                    strcat(buffer, "\t\t");

                    //Se añade el tiempo
                    struct tm *tm; //ver info: struct tm
                    tm = localtime(&inodoDeEntrada.mtime);
                    sprintf(tmp, "%d-%02d-%02d %02d:%02d:%02d", tm->tm_year + 1900, tm->tm_mon + 1, tm->tm_mday, tm->tm_hour, tm->tm_min,  tm->tm_sec);
                    strcat(buffer, tmp);
                    strcat(buffer, "\t");

                    //Se añade el tamaño
                    sprintf(tmp, "%d", inodoDeEntrada.tamEnBytesLog);
                    strcat(buffer, tmp);
                    strcat(buffer, "\t");

                    //Se añade el nombre
                    strcat(buffer, entradas[i].nombre);
                    strcat(buffer, RES);
                    strcat(buffer, "\n");
                } else{
                    //Se cambai el color dependiendo del tipo de entrada
                    if(inodoDeEntrada.tipo == 'd'){
                        strcat(buffer, AZUL);
                    } else{
                        strcat(buffer, LILA);
                    }

                    //Se añade el nombre
                    strcat(buffer, entradas[i].nombre);
                    strcat(buffer, RES);
                    strcat(buffer, "\t");
                }
            }
        }
        
        //se añade un salto de linea si no es la version extendida ya que no se ha añadido previamente
        if(!extended){
            strcat(buffer, "\n");
        }
    } else{ //MEJORA: mostrar datos de un fichero
        total++;
        strcat(buffer, LILA);
        strcat(buffer, "f");
        strcat(buffer, "\t");

        //Se añaden los permisos
        if (inodo_dir.permisos & 4) strcat(buffer, "r"); else strcat(buffer, "-");
        if (inodo_dir.permisos & 2) strcat(buffer, "w"); else strcat(buffer, "-");
        if (inodo_dir.permisos & 1) strcat(buffer, "x"); else strcat(buffer, "-");
        strcat(buffer, "\t\t");

        //Se añade el tiempo
        struct tm *tm; //ver info: struct tm
        tm = localtime(&inodo_dir.mtime);
        sprintf(tmp, "%d-%02d-%02d %02d:%02d:%02d", tm->tm_year + 1900, tm->tm_mon + 1, tm->tm_mday, tm->tm_hour, tm->tm_min,  tm->tm_sec);
        strcat(buffer, tmp);
        strcat(buffer, "\t");

        //Se añade el tamaño
        sprintf(tmp, "%d", inodo_dir.tamEnBytesLog);
        strcat(buffer, tmp);
        strcat(buffer, "\t");

        //Se añade el nombre
        strcat(buffer, nombre);
        strcat(buffer, RES);
        strcat(buffer, "\n");
    }
    return total;
}

/* --- MI_READ (nivel 9) --------------------------------------------------------------------------
    Fuencion que lee los primeros 'nbytes' del fichero indicado por 'camino', a partir del 'offset'
    pasado por parámetro y los copia en el buffer 'buf'.

    Devuelve la cantidad de bytes leidos
    Utilizada por:
    - mi_cat.c
*/
int mi_read(const char *camino, void *buf, unsigned int offset, unsigned int nbytes){
    struct superbloque SB;
    if (bread(posSB, &SB) == FALLO)
        return FALLO;

    struct inodo inodo_dir; //Contiene el inodo leido

    //contiene el n inodo del fichero;
    unsigned int p_inodo = 0;

    unsigned int p_entrada = 0; //inutil, se utiliza pq lo requiere buscar_entrada() pero su valor no se utiliza
    //contiene el codigo de error en caso que ocurra
    int err;

     //Optimizacion de buffer implementada por nahuel vazquez
    char encontrado = 0; //variable para saber si se tiene que almacenar la entrada o no;
    //recorremos el cache comprobando si el camino ya ha sido buscado
    for(int i = 0; i<NCACHE; i++){
        
        if(strcmp(camino, CacheEntradas[i].camino)==0){
#if DEBUGN9
                fprintf(stderr, GRIS"\n[mi_read() → Utilizamos cacheEntradas[%d]: %s]\n"RES, i, camino);
#endif
            p_inodo=CacheEntradas[i].p_inodo;
            encontrado++;
        }
    }

    //Si el camino no estaba en el cache, lo buscamos conbuscar_entrada() y lo añadimos al cache
    if(encontrado==0){ 
        err = buscar_entrada(camino, &SB.posInodoRaiz, &p_inodo, &p_entrada, 0, 4);
        if (err<0){
            mostrar_error_buscar_entrada(err);
            return FALLO;
        }

        struct inodo in;  // Comprobamos que no sea un directorio
        leer_inodo(p_inodo, &in);
        
        //if (in.tipo == 'd'){
        //    fprintf(stderr, ROJO"Error: No se puede escribir en un directorio!"RES);
        //    return FALLO;
        //}
        

        //Creacion del nuevo elemento del cache
        struct UltimaEntrada ue;
        strcpy(ue.camino, camino);
        ue.p_inodo = p_inodo;

        //Operacion de modulo para substituir el elemento mas antiguo
#if DEBUGN9
        fprintf(stderr, GRIS"\n[mi_read() → Reemplazamos cacheEntradas[%d]: %s]\n"RES, idx_cache, camino);
#endif
        CacheEntradas[idx_cache]=ue;
        idx_cache++;
        idx_cache=idx_cache%NCACHE;
    } 

    if (leer_inodo(p_inodo, &inodo_dir) == FALLO)  return FALLO;// Leemos el inodo correspondiente a p_inodo
    if ((inodo_dir.permisos&4) != 4)  // Comprobamos permisos
        return ERROR_PERMISO_LECTURA;

    return mi_read_f(p_inodo, buf, offset, nbytes);

 }

/* --- MI_WRITE (nivel 9) -------------------------------------------------------------------------
    Escribe contenido en un fichero existente. Escribe los primeros 'nbytes' del contenido de 'buf'
    en la posicion 'offset' del fichero 'camino'.

    Devuelve el numero de bytes escritos, -2 si no hay permisos de escritura, y -1 si hay fallo general
    Utilizada por:
    - mi_escribir.c
*/
int mi_write(const char *camino, const void *buf, unsigned int offset, unsigned int nbytes)
{
    int error;
    int bytesEscritos;
    unsigned int p_inodo_dir;
    unsigned int p_inodo;
    unsigned int p_entrada;

    p_inodo_dir = 0;
    p_inodo = 0;
    p_entrada = 0;

    bool encontrado = false;
    for(int i = 0; i<NCACHE; i++){
        if(strcmp(camino, CacheEntradas[i].camino)==0){
#if DEBUGN9
            fprintf(stderr, GRIS"\n[mi_write() → Utilizamos CacheEntradas[%d]: %s)]\n"RES, i, camino);
#endif
            p_inodo=CacheEntradas[i].p_inodo;
            encontrado = true;
        }
    }
    
    if (!encontrado)
    {
        error = buscar_entrada(camino, &p_inodo_dir, &p_inodo, &p_entrada, 0, '6');
        if (error < 0)
        {
            mostrar_error_buscar_entrada(error);
            return FALLO;
        }
        struct inodo in;  // Comprobamos que no sea un directorio
        leer_inodo(p_inodo, &in);
        /*
        if (in.tipo == 'd'){
            fprintf(stderr, ROJO"Error: No se puede escribir en un directorio!"RES);
            return FALLO;
        }
        */

        struct UltimaEntrada ue;
        strcpy(ue.camino, camino);
        ue.p_inodo = p_inodo;
        
        CacheEntradas[idx_cache]=ue;
#if DEBUGN9
        fprintf(stderr, GRIS"\n[mi_write() → Reemplazamos cacheEntradas[%d]: %s]\n"RES, idx_cache, camino);
#endif
        idx_cache = (idx_cache+1)%NCACHE;
    }

    bytesEscritos = mi_write_f(p_inodo, buf, offset, nbytes);

    return bytesEscritos;
}

/* --- MI_LINK ------------------------------------------------------------------------------------
    Crea un enlace fisico (una entrada de directorio 'camino2'), que apunta al inodo correspondiente
    al fichero 'camino1'

    Devuelve 0 si va bien, -1 si hay error general, -2 si camino1 no tiene permiso de lectura, y
    -3 si camino1 no es un fichero
    Utilizada por: 
    - mi_link.c
*/
int mi_link(const char *camino1, const char *camino2) {
    unsigned int p_inodo_dir = 0;
    unsigned int p_inodo1;
    unsigned int p_entrada;
    mi_waitSem();
    
    // Averiguamos el numero de inodo correspondiente a la ruta camino1
    int err = buscar_entrada(camino1, &p_inodo_dir, &p_inodo1, &p_entrada, 0, '\0');
    if (err < 0) {
        mostrar_error_buscar_entrada(err);
        mi_signalSem();
        return FALLO;
    }

    struct inodo inodo1;
    if (leer_inodo(p_inodo1, &inodo1) == FALLO){
        mi_signalSem();
        return FALLO;
    }

    if (!(inodo1.permisos&4)){  // Comprobamos que inodo1 tiene permisos de lectura
        mi_signalSem();
        return -2;
    }

    if (inodo1.tipo != 'f'){
        mi_signalSem();
        return -3;
    }

    unsigned int p_inodo_dir2 = 0;
    unsigned int p_inodo2;  // Lo guardamos para poder liberarlo
    unsigned int p_entrada2;
    err = buscar_entrada(camino2, &p_inodo_dir2, &p_inodo2, &p_entrada2, 1, 6);  // Creamos camino2
    if (err < 0) {
        mostrar_error_buscar_entrada(err);
        mi_signalSem();
        return FALLO;
    }

    struct entrada entrada2;  // Leemos la entrada correspondiente a camino2 en el directorio padre del enlace
    if (mi_read_f(p_inodo_dir2, &entrada2, p_entrada2*sizeof(struct entrada), sizeof(struct entrada)) == FALLO){
        mi_signalSem();
        return FALLO;
    }

    entrada2.ninodo = p_inodo1;  // Asignamos a esta entrada el inodo correspondiente a camino1, creando el enlace

    // Escribimos la entrada modificada
    if (mi_write_f(p_inodo_dir2, &entrada2, p_entrada2*sizeof(struct entrada), sizeof(struct entrada)) == FALLO){
        mi_signalSem();
        return FALLO;
    }


    if (liberar_inodo(p_inodo2) == FALLO){  // Liberamos el inodo que ha reservado buscar_entrada y que ya no es necesario
        mi_signalSem();
        return FALLO;
    }
        

    inodo1.nlinks++;  // Incrementamos la cantidad de enlaces del inodo correspondiente a camino1
    inodo1.ctime = time(NULL);  // Actualizamos el tiempo de la ultima modificacion al inodo
    escribir_inodo(p_inodo1, &inodo1);  // Guardamos el inodo modificado

    mi_signalSem();
    return EXITO;
}

/*--- MI_UNLINK (nivel 10) -------------------------------------------------------------------------
    Borra la entrada del direectorio especificado por *camino y, en caso de que fuera el último enlace
    existente, borra el propio fichero/directoriol

    Devuelve 0 si va bien, -1 si hay error y -2 si se intenta borrar un directorio no vacio
    Utilizada por:
    - mi_rmdir.c
    - mi_rm.c
*/
int mi_unlink(const char *camino){
    int error;
    unsigned int p_inodo_dir;
    unsigned int p_inodo;
    unsigned int p_entrada;
    int numEntradas;
    struct inodo inodo;
    struct entrada entrada;
    mi_waitSem();

    numEntradas=0;
    p_inodo_dir = 0;
    p_inodo = 0;
    p_entrada = 0;
    //obtenemos su numero de entrada
    error = buscar_entrada(camino, &p_inodo_dir, &p_inodo, &p_entrada, 0, '6');
     if (error < 0)
        {
            mostrar_error_buscar_entrada(error);
            mi_signalSem();
            return FALLO;
        }

    if(leer_inodo(p_inodo, &inodo) ==FALLO){
        mi_signalSem();
        return FALLO;
    }
    //si se trata de un directorio y no está vacio, no se puede borrar
    if(inodo.tipo== 'd' && inodo.tamEnBytesLog>0 && inodo.nlinks == 1){
        mi_signalSem();
        return -2;
    }
    
    //leemos el inodo asociado al directorio
     if(leer_inodo(p_inodo_dir, &inodo) == FALLO){
         mi_signalSem();
        return FALLO;
    }
    //obtenemos el numero de entradas que tiene
    numEntradas= inodo.tamEnBytesLog/sizeof(struct entrada);
    
    //si la entrada a eliminar es la ultima
    if(p_entrada== numEntradas-1){
        mi_truncar_f(p_inodo_dir, (numEntradas - 1) * sizeof(struct entrada));
    }
    else{
        //leemos la úlitma entrada
        mi_read_f(p_inodo_dir, &entrada, (numEntradas - 1) * sizeof(struct entrada), sizeof(struct entrada));
		//escribimos la ultima entrada en la posicion de la entrada que queramos eliminar
        mi_write_f(p_inodo_dir, &entrada, p_entrada * sizeof(struct entrada), sizeof(struct entrada));
        //truncamos el inodo
        mi_truncar_f(p_inodo_dir, (numEntradas - 1) * sizeof(struct entrada));
    }
    //leemos el inodo asociado a la entrada eliminada para decrementar el numero de enlaces
    if(leer_inodo(p_inodo, &inodo) == FALLO) {
        mi_signalSem();
        return FALLO;
    }
	inodo.nlinks--;
	if(inodo.nlinks == 0) {

		if (liberar_inodo(p_inodo) == FALLO){
            mi_signalSem();
            return FALLO;
        }
	} else{
        //actualizamos el ctime
		inodo.ctime = time(NULL);
		if (escribir_inodo(p_inodo, &inodo) == FALLO){
            mi_signalSem();
            return FALLO;
        }
	}
	mi_signalSem();
	return EXITO;
}

// --------------------- EXTRA ------------------------------------------------------------------------------------------------------------

/* --- EXTRAER_ULTIMO (extra_krishna) -------------------------------------------------------------
    Funcion auxiliar que extrae la parte final de un camino (el nombre del fichero o directorio que hay que copiar)
*/
char* extraer_ultimo(char* camino) {
    char* ptr = strrchr(camino, '/');  // Encuentra el ultimo '/'
    if (ptr == NULL)
        return NULL;
    
    if (*(ptr+1) == '\0'){ // Si el ultimo '/' esta al final de la string
        *(ptr) = '\0';
        char* ptr2 = strrchr(camino, '/');  // Encontramos el penultimo '/'
        if (ptr2 == NULL)
            return NULL;
        
        *(ptr) = '/';
        camino = ptr2+1;  // Si el ultimo '/' esta al final, guardamos el puntero al antepenultimo +1
        return camino;
    }
    camino = ptr+1;  // Si el ultimo '/' no esta al final, guardamos el puntero al ultimo + 1
    return camino;
}

/* --- MI_CP (extra krishna) ----------------------------------------------------------------------
    Copia un fichero o un directorio (en este caso recursivamente) 'src', en un directorio 'dst'.
    si 'dir' es false, 'src' debe ser un fichero, y viceversa. Se encarga de manipular los caminos
    y llamar a buscar_entrada para obtener los Nº de inodo, mientras que llama a mi_copiar para
    realizar la copia del contenido en si, una vez se saben los numeros de inodo.
    
    Devuelve 0 si todo va bien, -1 si hay fallo, -2 si 'src' no es del tipo que corresponde, y -3
    src no tiene permisos de lectura
*/ 
int mi_cp(char* dst, char* src, bool dir) {
    unsigned int inodoSrc;
    unsigned int entradaSrc;
    unsigned int p_inodo_dir = 0;
    
    // Obtenemos el numero de inodo del fichero o directorio origen
    int err = buscar_entrada(src, &p_inodo_dir, &inodoSrc, &entradaSrc, 0, 0);
    if (err < 0){
        mostrar_error_buscar_entrada(err);
        return FALLO;
    }
    
    char* nombre= extraer_ultimo(src);  // Extraemos el nombre del fichero o directorio a ser copiado
    if (nombre == NULL)
        return FALLO;
    
    char caminoCopia[strlen(nombre)+strlen(dst)];
    strcpy(caminoCopia, dst);
    strcat(caminoCopia, nombre);  // Camino copia es el camino destino + el nombre del fichero o directorio copiado
    
    struct inodo in;
    if (leer_inodo(inodoSrc,  &in) == FALLO)  // Leemos el inodo origen para saber sus permisos
        return FALLO;
    
    if ((in.tipo == 'f' && dir) || (in.tipo == 'd' && !dir))
        return -2;
    
    unsigned int inodoDst;
    unsigned int entradaDst;
    p_inodo_dir = 0;
    
    // Reservamos el inodo para la copia del fichero o directorio y creamos la entrada correspondiente en el directorio padre
    mi_waitSem();
    err = buscar_entrada(caminoCopia, &p_inodo_dir, &inodoDst, &entradaDst, 1, in.permisos);
    mi_signalSem();
    if (err < 0){
        mostrar_error_buscar_entrada(err);
        return FALLO;
    }
    
    mi_waitSem();
    int ret = mi_copiar(inodoDst, inodoSrc);
    
    mi_signalSem();
    return ret;
}

/* --- MI_COPIAR (extra krishna) ------------------------------------------------------------------
    Funcion auxiliar a mi_cp, que se encarga de la copia del contenido de un fichero o directorio
    correspondiente al inodo 'inodoSrc', en el de 'inodoDst'.
    
    Devuelve el numero de bloques copiados, -1 si hay error, y -3 si src no tiene permisos de lectura
*/
int mi_copiar(unsigned int inodoDst, unsigned int inodoSrc){
    struct inodo dst;
    struct inodo src;
    if (leer_inodo(inodoDst, &dst) == FALLO)  // Leemos el inodo destino
        return FALLO;
    
    if (dst.tamEnBytesLog != 0)
        return FALLO;
    
    if (leer_inodo(inodoSrc, &src) == FALLO)  // Leemos el inodo origen
        return FALLO;
    
    if ((src.permisos&4) != 4)
        return -3;

    int cp = copiar_bloques_inodo(&dst, &src);  // Copiamos los bloques del inodo origen en el inodo destino

    if (cp == FALLO)
        return FALLO;
    
    if (escribir_inodo(inodoDst, &dst) == FALLO)  // ¡¡IMPORTANTE!! ESCRIBIR PARA ACTUALIZAR EL TAMENBYTESLOG PARA MI_READ_F
        return FALLO;
    
    if (dst.tipo == 'd') {  // Si estamos copiando un directorio, recorremos sus entradas y las copiamos recursivamente
        
        struct entrada entradas[NENTRADAS];
        int ultimoBL = dst.tamEnBytesLog / BLOCKSIZE;
        int nEntradas = dst.tamEnBytesLog / sizeof(struct entrada);
        int idxEnt = 0;
        if (dst.tamEnBytesLog % BLOCKSIZE == 0)
            ultimoBL--;
        
        for (int bl = 0; bl <= ultimoBL; bl++) {  // Recorremos los bloques del directorio
            
            // Leemos un bloque de entradas
            int leidos = mi_read_f(inodoDst, entradas, bl*BLOCKSIZE, BLOCKSIZE);
            if (leidos == FALLO)
                return FALLO;
            
            // Recorremos las entradas, reservando un inodo para cada entrada y llamando a esta funcion recursivamente
            // para copiar el contenido. Guardamos el nuevo Nº de inodo en cada entrada.
            for (int i = 0; i < NENTRADAS && idxEnt < nEntradas; idxEnt++, i++) {
                //printf(ROJO"Entrada: [\"%s\", %d]\n"RES, entradas[i].nombre, entradas[i].ninodo);
                struct inodo inEnt;  // Leemos el inodo de la entrada para saber el tipo y permisos
                if (leer_inodo(entradas[i].ninodo, &inEnt) == FALLO)
                    return FALLO;
                
                unsigned int copiaEntrada;  // Reservamos un inodo para la copia de la entrada correspondiente
                copiaEntrada = reservar_inodo(inEnt.tipo, inEnt.permisos);
                if (copiaEntrada == FALLO)
                    return FALLO;
                
                mi_copiar(copiaEntrada, entradas[i].ninodo);  // Copiamos el contenido del inodo original al inodo copia
                entradas[i].ninodo = copiaEntrada;  // Asignamos el inodo copia a la entrada
            }
            
            // Sobreescribimos el bloque de entradas modificado
            if (mi_write_f(inodoDst, &entradas, bl*BLOCKSIZE, BLOCKSIZE) == FALLO)
                return FALLO;
        }
    }

    dst.mtime = time(NULL);
    dst.ctime = time(NULL);
    src.atime = time(NULL);
    src.ctime = time(NULL);

    if (escribir_inodo(inodoDst, &dst) == FALLO) // Escribimos el inodo destino actualizado
        return FALLO;

    if (escribir_inodo(inodoSrc, &src) == FALLO)  // Escribimos el inodo origen actualizado
        return FALLO;

    return cp;
}

/* --- MI_MV (extra krishna) ----------------------------------------------------------------------
    Mueve el fichero o directorio correspondiente a la ruta 'src' a la ruta 'dst'. Es decir, elimina
    la entrada 'src' y crea una nueva en 'dst', con el mismo numero de inodo.
    
    Devuelve 0 si va bien, -1 si hay fallo, -2 si src no tiene permisos de lectura
*/
int mi_mv(char* dst, char* src) {
    // No podemos utilizar mi_link ya que solo permite crear enlaces a ficheros
    unsigned int p_src = 0;
    unsigned int inodoSrc;
    unsigned int entradaSrc;
    
    // Buscamos la entrada origen, para averiguar el Nº de inodo del fichero o directorio a mover
    int err = buscar_entrada(src, &p_src, &inodoSrc, &entradaSrc, 0, 0);
    if (err < 0){
        mostrar_error_buscar_entrada(err);
        return FALLO;
    }
    
    struct inodo inodo;  // Leemos el inodo, para saber si hay permisos de lectura e incrementar nlinks despues
    if (leer_inodo(inodoSrc, &inodo) == FALLO)
        return FALLO;
    
    if ((inodo.permisos&4) != 4)
        return -2;
        
    unsigned int p_dst = 0;
    unsigned int inodoDst;
    unsigned int entradaDst;

    char* nombre = extraer_ultimo(src);  // Guardamos el nombre del fichero o directorio
    if (nombre == NULL) {
        return FALLO;
    }
    if (nombre[strlen(nombre)-1] == '/')
        nombre[strlen(nombre)-1] = '\0';  // Quitamos el '/' al final del nombre si es un directorio
    
    char caminoEntero[strlen(dst)+strlen(nombre)];
    strcpy(caminoEntero, dst);
    strcat(caminoEntero, nombre);
    
    // Llamamos a buscar_entrada para crear la entrada en destino
    mi_waitSem();
    err = buscar_entrada(caminoEntero, &p_dst, &inodoDst, &entradaDst, 1, 1);  // Los permisos dan igual, ya que liberaremos
    mi_signalSem();
    if (err < 0) {
        mostrar_error_buscar_entrada(err);
        return FALLO;
    }
    
    if (liberar_inodo(inodoDst) == FALLO)  // Liberamos el inodo que nos reserva buscar_entrada (ya que usaremos el del origen)
        return FALLO;
    
    struct entrada entDst; // Creamos la entrada destino (extraer_ultimo nos evita un acceso a disco)
    strcpy(entDst.nombre, nombre);
    entDst.ninodo = inodoSrc;
    
    // Sobreescribimos la entrada actualizada con el numero de inodo original
    if (mi_write_f(p_dst, &entDst, entradaDst*sizeof(struct entrada), sizeof(struct entrada)) == FALLO)
        return FALLO;
    
    inodo.nlinks++;  // Incrementamos nlinks para poder eliminar la entrada original con mi_unlink
    inodo.ctime = time(NULL);
    
    if (escribir_inodo(inodoSrc, &inodo) == FALLO)
        return FALLO;
    
    if (mi_unlink(src) < 0)
        return FALLO;
    
    return EXITO;
}
