// Krishna Jorda Jimenez, Margalida Covas Roig , Nahuel Vazquez
#include "ficheros_basico.h"

/* --- INITMB (nivel 2) ---------------------------------------------------------------------------
    Inicializa el mapa de bits, no recibe parametros

    Devuelve 0 si ha ido bien o -1 si hay error.
    Utilizada por:
        - Ejecutable mi_mkfs.c
     
*/
int initMB(){
    struct superbloque SB;
    if (bread(posSB, &SB) == FALLO)
        return FALLO;

    //obtenemos el tamaño en bits de los metadatos
    int metaSize = tamSB+tamMB(SB.totBloques)+tamAI(SB.totInodos);
    int fullBlocks = (metaSize/BLOCKSIZE)/8;
    int partialMeta = metaSize-(fullBlocks*8*BLOCKSIZE);

    int blockOffset = 0;

    //si los metadatos requieren mas de un bloque, se llenaran primero los bloques que esten completos
    if(fullBlocks){
        //Crea el buf de 1s para llenar el bloque
            unsigned char bufferMB[BLOCKSIZE];
            memset(bufferMB, 255, BLOCKSIZE);

            //llena la cantidad de bloques especificada
            for(; blockOffset<fullBlocks; blockOffset++){
                if (bwrite(SB.posPrimerBloqueMB+blockOffset, bufferMB) == FALLO)
                    return FALLO;
            }
    }
    //se rellenan los bits sobrantes que no han cabido en los bloques completos
    if(partialMeta){
        //Cantidad de bytes llenos de 1s
        int metaBytes = partialMeta/8;
        //cantidad de bits a 1 en el ultimo byte
        int metaBits = partialMeta%8;
        //el valor en decimal de los bits en el ultimo byte
        int bitValue = 0;

        //rellena los bytes enteros de 1s
        unsigned char bufferMB[BLOCKSIZE];
        memset(bufferMB, 0, BLOCKSIZE);
        memset(bufferMB, 255, metaBytes);

        for(int i = 7; i>7-metaBits; i--){
            bitValue += power(2, i);
        }
        bufferMB[metaBytes] = bitValue;
        
        if (bwrite(SB.posPrimerBloqueMB+blockOffset, bufferMB) == FALLO)
            return FALLO;
    }

    SB.cantBloquesLibres -= metaSize;
    
    if (bwrite(posSB, &SB) == FALLO)
        return FALLO;

    return EXITO;
}

/* --- INITSB (nivel 2) ---------------------------------------------------------------------------
    Inicializa el superbloque pasando por parametro la cantidad de bloques y la cantidad de inodos

    Devuelve 0 si ha ido bien o -1 si hay error.
    Utilizada por:
        - Ejecutable mi_mkfs.c
*/
int initSB(unsigned int nbloques, unsigned int ninodos){
    struct superbloque SB;

    SB.posPrimerBloqueMB = posSB + tamSB;
    SB.posUltimoBloqueMB = SB.posPrimerBloqueMB + tamMB(nbloques) - 1;
    SB.posPrimerBloqueAI = SB.posUltimoBloqueMB + 1;
    SB.posUltimoBloqueAI = SB.posPrimerBloqueAI + tamAI(ninodos) - 1;
    SB.posPrimerBloqueDatos = SB.posUltimoBloqueAI + 1;
    SB.posUltimoBloqueDatos = nbloques - 1;
    SB.posInodoRaiz = 0;
    SB.posPrimerInodoLibre = 0;
    SB.cantBloquesLibres = nbloques;
    SB.cantInodosLibres = ninodos;
    SB.totBloques = nbloques;
    SB.totInodos = ninodos;

    if (bwrite(posSB, &SB) == FALLO)
        return FALLO;
    else
        return EXITO;
}

/* --- TAMMB (nivel 2) ----------------------------------------------------------------------------
    Recibe el numero de bloques como argumento
    Calcula el tamaño en bloques que ocupara el mapa de bits

    Devuelve el el tamaño en bloques que ocupara el mapa de bits
    Utilizada por:
        - initMB()
        - initSB()
*/
int tamMB(unsigned int nbloques){
    int bloques = (nbloques / 8);

    if (nbloques % 8 != 0)
        bloques++;

    if (bloques % BLOCKSIZE != 0){
        bloques = bloques / BLOCKSIZE;
        bloques++;
    }
    else{
        bloques = bloques / BLOCKSIZE;
    }

    return bloques;
}

/* --- TAMAI (nivel 2) ----------------------------------------------------------------------------
    Recibe el numero de indos como argumento (nbloques/4 en nuestro caso)

    Devuelve el tamaño en bloques del array de inodos
    Utilizada por:
        - initMB()
        - initSB()
*/
int tamAI(unsigned int ninodos){
    int bloques = (ninodos*INODOSIZE)/BLOCKSIZE;
    if ((ninodos*INODOSIZE) % BLOCKSIZE != 0)
        bloques++;

    return bloques;
}

/* --- INITAI (nivel 2) ---------------------------------------------------------------------------
    Inicializa el array de inodos

    Devuelve 0 si ha ido bien o -1 si hay error.
    Utilizada por:
        - Ejecutable mi_mkfs.c
*/
int initAI(){
    struct superbloque SB;
    if (bread(posSB, &SB) == FALLO)
        return FALLO;

    struct inodo inodos[BLOCKSIZE/INODOSIZE];
    int contInodos = 1;

    for (int i = SB.posPrimerBloqueAI; i <= SB.posUltimoBloqueAI; i++){ // Para cada bloque del AI
        if (bread(i, inodos) == -1)
            return FALLO;

        for (int j = 0; j < (BLOCKSIZE / INODOSIZE); j++){  // Para cada inodo de un bloque
            inodos[j].tipo = 'l'; // Marcar indodo como libre
            
            if (contInodos < SB.totInodos){
                inodos[j].punterosDirectos[0] = contInodos;
                contInodos++;
            }
            else{
                inodos[j].punterosDirectos[0] = UINT_MAX;
                break;
            }
        }

        if (bwrite(i, inodos) == FALLO)
            return FALLO;
    }
    
    return EXITO;
}

/* --- ESCRIBIR_BIT (nivel 3) ---------------------------------------------------------------------
    Escribe el valor indicado por el parámetro bit: 0 (libre) 
    ó 1 (ocupado) en un determinado bit del MB que representa el bloque nbloque.

    Devuelve -1 si ha habido error y el numero de bytes escritos si ha ido bien
    Utilizada por:
        - reservar_bloque()
        - liberar_bloque()
*/
int escribir_bit(unsigned int nbloque, unsigned int bit){
    //Leer superbloque
    struct superbloque SB;
    if (bread(posSB, &SB) == FALLO)
        return FALLO;

    //Obtenemos que byte y que bit dentro de este hay que modificar
    int posbyte = nbloque/8;
    int posbit = nbloque%8;

    //Obtenemos el bloque de MB que contiene el bit
    int nbloqueMB = posbyte/BLOCKSIZE;

    //Posicion absoluta del bloque
    int nbloqueabs = SB.posPrimerBloqueMB + nbloqueMB;

    //Leemos el bloque 
    unsigned char bufferMB[BLOCKSIZE];
    bread(nbloqueabs, bufferMB);

    //modificar el byte
    posbyte = posbyte % BLOCKSIZE;
    unsigned char mascara = 128; //10000000
    mascara >>= posbit;

    if(bit == 1){
        bufferMB[posbyte] |= mascara;
    } else if(bit == 0){
        bufferMB[posbyte] &= ~mascara;
    } else{
        fprintf(stderr, "Error: valor incorrecto para un bit \"%d\"", bit);
        fflush(stderr);
        return FALLO;
    }

    return bwrite(nbloqueabs, bufferMB);
}

/* --- LEER_BIT (nivel 3) -------------------------------------------------------------------------
    Lee un determinado bit del MB
    
    Devuelve el valor del bit leído.
    Utilizada por:
        - Ejecutable leer_sf.c (debug)
*/
char leer_bit(unsigned int nbloque){
    //Leer superbloque
    struct superbloque SB;
    if (bread(posSB, &SB) == FALLO)
        return FALLO;

    //Obtenemos que byte y que bit dentro de este hay que leer
    int posbyte = nbloque/8;
    int posbit = nbloque%8;

    //Obtenemos el bloque de MB que contiene el bit
    int nbloqueMB = posbyte/BLOCKSIZE;

    //Posicion absoluta del bloque
    int nbloqueabs = SB.posPrimerBloqueMB + nbloqueMB;

#if DEBUGN3
    fprintf(stderr, GRIS"[leer_bit(%d)-> posbyte: %d, posbit: %d, nbloqueMB: %d, nbloqueabs: %d]\n"RES, nbloque, posbyte, posbit, nbloqueMB, nbloqueabs);
#endif
    //Leemos el bloque 
    unsigned char bufferMB[BLOCKSIZE];
    bread(nbloqueabs, bufferMB);

    //Leer el bit
    posbyte = posbyte % BLOCKSIZE;
    unsigned char mascara = 128; // 10000000
    mascara >>= posbit;          // desplazamiento de bits a la derecha, los que indique posbit
    mascara &= bufferMB[posbyte]; // operador AND para bits
    mascara >>= (7 - posbit);     // desplazamiento de bits a la derecha  para dejar el 0 o 1 en el extremo derecho y leerlo en decimal

    return mascara;
}

/* --- ESCRIBIR_INODO (nivel 3) -------------------------------------------------------------------
    Sobreescribe el inodo en la posicion ninodo del array de inodos con el contenido
    del buffer inodo. 
    
    Devuelve -1 si hay fallo y 0 si ha ido bien.
    Utilizada por:
        - liberar_inodo()
        - reservar_inodo()
        - mi_write_f()
        - mi_read_f()
        - mi_chmod_f()
        - mi_truncar_f()
*/
int escribir_inodo(unsigned int ninodo, struct inodo *inodo){
    struct superbloque SB;              // Leemos el superbloque para saber cual es el primer bloque del AI
    if (bread(posSB, &SB) == -1)
        return FALLO;

    int bloque = ninodo / (BLOCKSIZE/INODOSIZE);    // Calculamos el Nº de bloque que contiene el inodo a escribir
    bloque += SB.posPrimerBloqueAI;                 // Calculamos la posicion absoluta de ese bloque

    struct inodo inodos[BLOCKSIZE/INODOSIZE];
    if (bread(bloque, &inodos) == -1)               // Leemos el bloque que contiene el inodo a escribir
        return FALLO;

    inodos[ninodo%(BLOCKSIZE/INODOSIZE)] = *inodo;

    if (bwrite(bloque, &inodos) == -1)
        return FALLO;

    return EXITO;
}

/* --- LEER_INODO (nivel 3) -----------------------------------------------------------------------
    Lee el inodo en la posicion ninodo de el array de inodos y lo guarda en el buffer inodo.
    
    Devuelve -1 si hay error y 0 si ha ido bien.
    Utilizada por:
        - reservar_inodo()
        - liberar_inodo()
        - mi_write_f()
        - mi_read_f()
        - mi_stat_f()
        - mi_chmod_f()
        - mi_truncar_f()
        - Ejecutable leer_sf.c (debug)
*/
int leer_inodo(unsigned int ninodo, struct inodo *inodo){
    struct superbloque SB;
    if (bread(posSB, &SB) == -1)            // Leemos el superbloque para saber cual es el primer bloque del AI
        return FALLO;

    int bloque = ninodo/(BLOCKSIZE/INODOSIZE);  // Calculamos el Nº de bloque que contiene el inodo a leer
    bloque += SB.posPrimerBloqueAI;             // Calculamos la posicion absoluta de ese bloque

    struct inodo inodos[BLOCKSIZE/INODOSIZE];
    if (bread(bloque, &inodos) == -1)   // Leemos el bloque que contiene el inodo a leer
        return FALLO;

    *inodo = inodos[ninodo%(BLOCKSIZE/INODOSIZE)];

    return EXITO;
}

/* --- RESERVAR_INODO (nivel 3) -------------------------------------------------------------------
    Reserva el primer inodo libre de la lista y lo inicializa, actualiza SB.primerInodoLibre para que apunte
    al siguiente y decrementa SB.cantInodosLibres. 
    
    Devuelve el indice del inodo reservado en el array
    de inodos o -1 si hay algun error.
    Utilizada por:
        - Ejecutable escribir.c
        - Ejecutable mi_mkfs.c
*/
int reservar_inodo(unsigned char tipo, unsigned char permisos){
    struct superbloque SB;
    if (bread(posSB, &SB) == -1)            // Leemos el superbloque para saber cual es el primer inodo libre
        return FALLO;
    
    if (SB.posPrimerInodoLibre == UINT_MAX)
        return FALLO;
    
    int posReservado = SB.posPrimerInodoLibre;  // Guardamos para retornar al final

    struct inodo in;
    if (leer_inodo(SB.posPrimerInodoLibre, &in) == -1)  // Leemos el primer inodo libre
        return FALLO;
    
    SB.posPrimerInodoLibre = in.punterosDirectos[0];    // Hacemos que la lista empieze en el siguiente libre
    SB.cantInodosLibres--;
    bwrite(posSB, &SB);         // Actualizamos el superbloque

    // Inicializamos el inodo reservado
    in.tipo = tipo;
    in.permisos = permisos;
    in.nlinks = 1;
    in.tamEnBytesLog = 0;
    in.atime = time(NULL);
    in.mtime = time(NULL);
    in.ctime = time(NULL);
    in.numBloquesOcupados = 0;
    memset(&in.punterosDirectos[0], 0, sizeof(unsigned int)*12);
    memset(&in.punterosIndirectos[0], 0, sizeof(unsigned int)*3);

    escribir_inodo(posReservado, &in);

    return posReservado;
}

/* --- RESERVAR_BLOQUE (nivel 3) ------------------------------------------------------------------
    Encuentra el primer bloque libre, consultando el MB (primer bit a 0)
    y lo pone a 1 (lo ocupa)

    Devuelve el numero de bloque que se ha reservado
    Utilizada por:
        - traducir_bloque_inodo()
        - Ejecutable leer_sf.c (debug)
*/

int reservar_bloque()
{
    struct superbloque SB;
    // leemos SB
    if (bread(posSB, &SB) == FALLO)
    {
        return FALLO;
    }

    // miramos si hay bloques libres
    if (SB.cantBloquesLibres == 0)
    {
        return FALLO;
    }

    unsigned char bufferAux[BLOCKSIZE];
    unsigned char bufferMB[BLOCKSIZE];
    unsigned int nbloqueabs = SB.posPrimerBloqueMB;
    // variable que nos indica si hemos encontrado un bloque libre
    int encontradoLibre = 0;
    // variable para saber si es igual el bloque que hemos leido y el bufferAux
    int igual = 0;

    // llenamos de unos el bufferAux
    if (memset(bufferAux, 255, BLOCKSIZE) == NULL)
    {
        return FALLO;
    }
    // RECORRIDO PARA ENCONTRAR EL PRIMER BLOQUE LIBRE
    for (; nbloqueabs <= SB.posUltimoBloqueMB && encontradoLibre == 0; nbloqueabs++)
    {
        if (bread(nbloqueabs, bufferMB) == FALLO)
        {
            return FALLO;
        }
        // si lo que nos ha devuelto almenos hay un bit que esta a 0 tendremos un valor mayor o menor a 0
        igual = memcmp(bufferMB, bufferAux, BLOCKSIZE);

        // si hay un bloque a 0
        if (igual != 0)
        {
            encontradoLibre = 1;
        }
    }
    nbloqueabs--;

    // RECORRIDO PARA ENCONTRAR EL BYTE QUE CONIENE ALGÚN 0
    int posByte = 0;
    // augmentamos la posción mientras todos sean 1
    while (bufferMB[posByte] == 255)
    {
        posByte++;
    }

    // RECORRIDO PARA ENCONTRAR EL BIT 0
    unsigned char mascara = 128;
    int posBit = 0;
    // un solo & porque miramos a nivel de bits
    while (bufferMB[posByte] & mascara)
    {
        // desplazamiento a la izquierda de bits
        bufferMB[posByte] <<= 1;
        posBit++;
    }

    // Determinamos que numero de bloque fisico podemos reservar
    int nbloque = ((nbloqueabs - SB.posPrimerBloqueMB) * BLOCKSIZE + posByte) * 8 + posBit;
    // Indicamos que el bloque esta reservado
    if (escribir_bit(nbloque, 1) == FALLO)
    {
        return FALLO;
    }
    // Decrementamos la num bloques libres
    SB.cantBloquesLibres--;

    // LIMPIAMOS EL BLOQUE EN LA ZONA DE DATOS
    // Rellenos el buffer de 0
    if (memset(bufferAux, 0, BLOCKSIZE) == NULL)
    {
        return FALLO;
    }
    // Escribimos el bloque
    if (bwrite(nbloque, &bufferAux) == FALLO)
    {
        return FALLO;
    }
    // Salvamos el superbloque
    if (bwrite(posSB, &SB) == FALLO)
    {
        return FALLO;
    }

    return nbloque;
}

/* --- LIBERAR_BLOQUE (nivel 3) -------------------------------------------------------------------
    Libera un bloque determinado (pone el bit del MB correspondiente al nbloque)

    Devuelve el numero de bloque liberado
    Utilizada por:
        - liberar_bloque_inodo()
        - liberar_bloque_puntero()
        - Ejecutable leer_sf.c (debug)
*/
int liberar_bloque(unsigned int nbloque)
{
    if (nbloque == 0){
        fprintf(stderr, ROJO"ERROR: NO SE PUEDE LIBERAR EL SB!!\n"RES);
    }

    // Ponemos a 0 el bit del MB
    if (escribir_bit(nbloque, 0) == FALLO)
    {
        return FALLO;
    }
    struct superbloque SB;
    // Leemos el supeBloque
    if (bread(posSB, &SB) == FALLO)
    {
        return FALLO;
    }
    // Incrementamos la cantidad de bloques libres del SB
    SB.cantBloquesLibres++;

    // Salvamos SB
    if (bwrite(posSB, &SB) == FALLO)
    {
        return FALLO;
    }
    return nbloque;
}

/* --- OBTENER_NRANGOBL (nivel 4) -----------------------------------------------------------------
    Devuelve el rango de punteros en el que se encuentra el bloque logico pasado
    por parametro: Directos=0, Indirectos0=1, Indirectos1=2, Indirectos2=3. Ademas
    obtiene la direccion del puntero correspondiente a cada rango del inodo (pasado
    por parametro) y la guarda en el 'puntero' pasado por parametro
    Utilizada por:
        - traducir_bloque_inodo()
        - liberar_bloques_inodo()
*/
int obtener_nRangoBL(struct inodo *inodo, unsigned int nblogico, unsigned int *ptr)
{
    if (nblogico < DIRECTOS)
    {
        *ptr = inodo->punterosDirectos[nblogico];
        return 0;
    }
    else if (nblogico < INDIRECTOS0)
    {
        *ptr = inodo->punterosIndirectos[0];
        return 1;
    }
    else if (nblogico < INDIRECTOS1)
    {
        *ptr = inodo->punterosIndirectos[1];
        return 2;
    }
    else if (nblogico < INDIRECTOS2)
    {
        *ptr = inodo->punterosIndirectos[2];
        return 3;
    }
    else
    {
        return FALLO;
    }
}

/* --- OBTENER_INDICE (nivel 4) -------------------------------------------------------------------
    Recibe por parametro un Nº de bloque logico. 
    Devuelve el indice del puntero a ese bloque logico (nivel_punteros=1) o, alternativamente, 
    del puntero al bloque de punteros que apunta a ese bloque logico (nivel_punteros=2) o 
    del puntero que apunta al bloque de punteros que apunta al bloque de punteros que apunta a 
    ese bloque logico (nivel_punteros=3).
    Utilizada por:
        - traducir_bloque_inodo()
*/
int obtener_indice(unsigned int nblogico, int nivel_punteros)
{
    if(nblogico < DIRECTOS){
        return nblogico;
    }
    else if(nblogico < INDIRECTOS0){
        return nblogico - DIRECTOS;
    }
    else if(nblogico < INDIRECTOS1){
        if(nivel_punteros == 2){
            return (nblogico - INDIRECTOS0) / NPUNTEROS;
        }
        else if(nivel_punteros == 1){
            return (nblogico - INDIRECTOS0) % NPUNTEROS;
        }
    }
    else if(nblogico < INDIRECTOS2){
        if(nivel_punteros == 3){
            return (nblogico - INDIRECTOS1) / (NPUNTEROS * NPUNTEROS);
        }
        else if(nivel_punteros == 2){
            return ((nblogico - INDIRECTOS1) % (NPUNTEROS * NPUNTEROS)) / NPUNTEROS;
        }
        else if(nivel_punteros == 1){
            return ((nblogico - INDIRECTOS1) % (NPUNTEROS * NPUNTEROS)) % NPUNTEROS;
        }
    }
    return FALLO;
}

/*
    Funcion que calcula la potencia de un numero sobre otro recibiendo por parametro la base y el exponente
    Utilizada por:
        - initMB()
        - liberar_bloques_inodo()
        - liberar_bloque_puntero()
*/
int power(int base, int exponente){
    if (exponente == 0)
        return 1;

    if (exponente%2 != 0)
        return base*power(base, exponente-1);
    else
        return power(base*base, exponente/2);
}

/* --- TRADUCIR_BLOQUE_INODO (nivel 4) ------------------------------------------------------------
    Obtiene (devuelve) el Nº de bloque fisico correspondiente a bloque logico (pasado
    por parametro) de un inodo (pasado por parametro). Si reservar=1 se reservara dicho
    bloque fisico en caso de que no exista.
    Utilizada por:
        - mi_write_f()
        - mi_read_f()
*/
int traducir_bloque_inodo(struct inodo *inodo, unsigned int nblogico, unsigned char reservar){
    unsigned int ptr, ptr_ant;  // ptr es un puntero al bloque de nivel nivel_punteros
    int nRangoBL, nivel_punteros;  // El nivel es 0 si ptr apunta al bloque de datos, 1 si apunta a un bloque de
    // punteros que apunta al bloque de datos, 2 si apunta a un bloque de punteros que apunta a otro de nivel 1, etc

    int indice;  // El indice es la posicion dentro del bloque de punteros anterior que contiene
                 // el puntero a el bloque del nivel actual
    unsigned int buffer[NPUNTEROS];

    ptr = 0, ptr_ant = 0;

    // ptr empieza siendo el puntero al bloque de punteros (o datos) que cuelga del inodo
    nRangoBL = obtener_nRangoBL(inodo, nblogico, &ptr);  // 0=Directos, 1=Ind0, 2=Ind1, 3=Ind2.
    if (nRangoBL==FALLO) return FALLO;

    nivel_punteros = nRangoBL;  // Empezamos con el nivel mas alto (que cuelga directamente del inodo)

    // Para cada nivel de punteros indirectos (decrementamos nivel_punteros hasta llegar al nivel 1,
    while (nivel_punteros > 0){  // que es el ultimo y apunta a bloques de datos)
        if (ptr == 0){  // Si el bloque de punteros del nivel correspondiente no esta reservado
            if (reservar == 0){
                return FALLO;
            }
            else{  // Si ademas reservar = 1
                ptr = reservar_bloque();  // Reservamos un bloque de punteros (si podemos)
                if (ptr == FALLO){
                    return FALLO;
                }
                inodo->numBloquesOcupados++;
                if (ptr==FALLO) return FALLO;

                inodo->ctime = time(NULL);

                // Si estamos en el nivel que cuelga directamente del inodo asignamos el bloque reservado
                if (nivel_punteros == nRangoBL){  // a el puntero indirectos correspondiente del inodo
#if DEBUGN4 || DEBUGN5 || DEBUGN6
                        fprintf(stderr, GRIS"[traducir_bloque_inodo()-> punterosIndirectos[%d]=%d (reservado BF %d para punteros_nivel%d)]\n"RES, nRangoBL-1, ptr, ptr, nivel_punteros);
#endif
                    inodo->punterosIndirectos[nRangoBL - 1] = ptr;
                }
                else{  // Si estamos en un nivel mas bajo
#if DEBUGN4 || DEBUGN5 || DEBUGN6
                        fprintf(stderr, GRIS"[traducir_bloque_inodo()-> punteros_nivel%d[%d]=%d (reservado BF %d para punteros_nivel%d)]\n"RES, nivel_punteros+1, indice, ptr, ptr, nivel_punteros);
#endif
                    // Buffer contiene el contenido del bloque de punteros padre
                    buffer[indice] = ptr;  // Guardamos el puntero a el bloque recien reservado en la posicion
                                           // correspondiente del bloque de punteros anterior (el que apunta a este)
                    if (bwrite(ptr_ant, buffer)==FALLO)  // Actualizamos el bloque de punteros anterior
                        return FALLO;
                }
                // Dado que no vamos a leer ningun bloque porque no estaba reservado, ponemos el buffer todo a 0s
                memset(buffer, 0, BLOCKSIZE);
            }
        }
        else {  // Si el bloque de punteros correspondiente para este nivel ya existia, lo leemos
            if (bread(ptr, buffer) == FALLO)
                return FALLO;
        }
        // Obtenemos el indice del puntero a el siguiente bloque de punteros (o datos) dentro del
        // bloque de punteros correspondiente al nivel actual
        indice = obtener_indice(nblogico, nivel_punteros);
        if (indice==FALLO) return FALLO;

        ptr_ant = ptr;  // Guardamos el puntero a este bloque de punteros
        ptr = buffer[indice];  // Obtenemos el puntero a el siguiente bloque de punteros (si no esta reservado sera 0)
        nivel_punteros--;  // Por lo tanto, pasamos a estar un nivel mas cerca del bloque de datos (un nivel menos)
    }

    // nivel_punteros = 0, ptr->bloque de datos
    if (ptr == 0){  // Si el bloque de datos no esta reservado
        if (reservar == 0){
            return FALLO;
        }
        ptr = reservar_bloque();  // Lo reservamos (si podemos)
        if (ptr == FALLO){
            return FALLO;
        }
        inodo->numBloquesOcupados++;
        if (ptr==FALLO) return FALLO;

        inodo->ctime = time(NULL);
        if (nRangoBL == 0){  // Si el bloque de datos cuelga directamente del inodo
#if DEBUGN4 || DEBUGN5 || DEBUGN6
                fprintf(stderr, GRIS"[traducir_bloque_inodo()-> punterosDirectos[%d]=%d (reservado BF %d para BL %d)]\n"RES, nblogico, ptr, ptr, nblogico);
#endif
            inodo->punterosDirectos[nblogico] = ptr;  // Guardamos el puntero en la posicion correspondiente del inodo
        }
        else{  // Si el bloque de datos cuelga de un bloque de punteros
#if DEBUGN4 || DEBUGN5 || DEBUGN6
            fprintf(stderr, GRIS"[traducir_bloque_inodo()-> punteros_nivel1[%d]=%d (reservado BF %d para BL %d)]\n"RES, indice, ptr, ptr, nblogico);
#endif
            buffer[indice] = ptr;    // Guardamos el puntero a el nuevo blque de datos reservado en el indice correspondiente
                                     // del bloque de punteros de nivel 1 correspondiente

            if (bwrite(ptr_ant, buffer) == FALLO)  // Escribimos ese bloque de punteros actualizado
                return FALLO;
        }
    }
    // Si el bloque de datos ya esta reservado no hay que hacer nada

    return ptr;  // Devolvemos el puntero a el bloque de datos
}

/* --- LIBERAR_BLOQUES_INODO (nivel 6) ------------------------------------------------------------
    Libera los bloques de un inodo (pasado por referencia) a partir del bloque primerBL

    Devuelve la cantidad de bloques liberados (-1 si hay error)
    Utilizada por:
        - liberar_inodo()
        - mi_truncar_f()
*/
int liberar_bloques_inodo(unsigned int primerBL, struct inodo *inodo){
    int liberados = 0;
    if (inodo->tamEnBytesLog == 0)  // Si el fichero esta vacio no liberamos nada
        return 0;

    int ultimoBL;  // Calculamos el ultimo bloque logico
    if (inodo->tamEnBytesLog % BLOCKSIZE == 0)
        ultimoBL = (inodo->tamEnBytesLog/BLOCKSIZE) - 1;
    else
        ultimoBL = inodo->tamEnBytesLog/BLOCKSIZE;

#if DEBUGN6
    fprintf(stderr, GRIS"[liberar_bloques_inodo()-> primerBL: %d, ultimoBL: %d]\n"RES, primerBL, ultimoBL);
#endif
    unsigned int p; // p es inutil pero necesario para obtener_nRangoBL
    // Ultimo/primer (inclusive) rango de punteros del inodo donde hay que liberar BL
    // Directos=0, Indirectos0=1, Indirectos1=2, Indirectos2=3
    int ultimoRango = obtener_nRangoBL(inodo, ultimoBL, &p);
    int primerRango = obtener_nRangoBL(inodo, primerBL, &p);

    // Liberamos los bloques necesarios de los primeros 12 BL (punteros diectos)
    for (int i = primerBL; i < 12 && i <= ultimoBL; i++){
        if (inodo->punterosDirectos[i] != 0){  // Si el puntero apunta a un bloque fisico
            if (liberar_bloque(inodo->punterosDirectos[i]) == FALLO) {  // Lo liberamos
                return FALLO;
            } 
#if DEBUGN6
            fprintf(stderr, GRIS"[liberar_bloques_inodo()-> Liberado BF %d (BL %d)]\n"RES, inodo->punterosDirectos[i], i);
#endif
            liberados++;
            inodo->punterosDirectos[i] = 0;  // ¡¡IMPORTANTE!! Limpiar el puntero!!!!
        }
    }
#if DEBUGN6
    fprintf(stderr, "\n");
#endif
    int blinicial = 12;  // blinicial es un argumento crucial de liberar_bloque_punteros que nos permite saber a que BL
    // corresponden los punteros del bloque de punteros (y por lo tanto solo liberarlos si son mayores que primerBL).
    // blinicial es el BL mas bajo accesible desde el bloque de punteros correspondiente
    int breads = 0;
    int bwrites = 0;

    // Recorremos los indices de punteros indirectos (desde el primero) hasta el ultimo que contiene BL a liberar
    // Empezamos desde el 0 aunque no haya BL que liberar para incrementar blinicial
    for (int idxInd = 0; idxInd < ultimoRango; idxInd++){
        // primerRango-1 es el primer indice de indirectos donde hay BL a liberar
        // AAAAAAAAAAAAAAAAAAAAAAAAAAA  ¡¡¡¡¡¡¡¡¡¡¡¡¡¡¡¡IMPORTANTE!!!!!!!!!!!!    COMPOBAR QUE NO APUNTE A 0
        if (idxInd >= primerRango-1  && inodo->punterosIndirectos[idxInd] != 0){  // Si idxInd contiene BL a liberar
            int lib = liberar_bloque_punteros(inodo->punterosIndirectos[idxInd], idxInd+1, blinicial, primerBL, &breads, &bwrites);
            // El nivel del BP que cuelga de punterosIndirectos[idxInd] es idxInd+1
            if (lib == FALLO)
                return FALLO;
            
            if (blinicial >= primerBL)  // IMPORTANTE TAMBIEN
                inodo->punterosIndirectos[idxInd] = 0;

            liberados += lib;
        }
        blinicial += power(NPUNTEROS, (idxInd+1));  // Pasamos los bloques logicos que hemos recorrido
        // Que seran: NPUNTEROS para BP de nivel 1, NPUNTEROS^2 para BP de nivel 2, etc
    }
#if DEBUGN6
    fprintf(stderr, GRIS"[liberar_bloques_inodo()-> liberados: %d, breads: %d, bwrites: %d]\n"RES, liberados, breads, bwrites);
#endif
    return liberados;
}

/*
    Funcion auxiliar a liberar_bloques_inodo. Libera todo todos los BL >= 'primerBL' que esten contenidos en
    el bloque de punteros correspondiente al BF 'nbloque'. 'nivel' nos indica el nivel del bloque de punteros.
    'breads' y 'bwrites' cuentan el numero de accesos a disco. 'blinicial' nos permite saber que BL es cada BF.
 
    Devuelve el numero de bloques liberados o -1 si hay fallo.
    Utilizada por:
        - liberar_bloques_inodo()
*/
int liberar_bloque_punteros(int nbloque, int nivel, int blinicial, int primerBL, int* breads, int* bwrites){
#if DEBUGN6
    for (int d = 0; d < (3-nivel); d++) {fprintf(stderr, GRIS"|\t");}
    fprintf(stderr, GRIS"[liberar_bloque_punteros()-> nivel: %d, BL: %d-%d]\n"RES, nivel, blinicial, blinicial+power(NPUNTEROS, nivel)-1);
#endif
    unsigned int buffer[NPUNTEROS];  // Guarda el bloque de punteros
    int liberados = 0;
    int blactual = blinicial;  // blactual es el BL mas bajo accesible desde cada puntero del BP.
    // blinicial es el BL mas bajo accesible desde buffer[0] y por lo tanto desde todo el BP

    (*breads)++;
    if (bread(nbloque, buffer) == FALLO)  // Leemos los punteros
        return FALLO;

    bool modificado = false;  // Nos dira si hay que hacer bwrite del bloque de punteros al final

    // Recorremos todos los punteros del bloque de punteros -------------------------------------------------------------
    for (int i = 0; i < NPUNTEROS; i++){

        // Si el puntero no es null y 'contiene' algun BL a liberar (sera el caso si la diferencia entre el BL mas bajo
        // accesible desde el puntero y el primerBL a liberar es menor que la cantidad de BL que abarca el puntero)
        if (buffer[i] != 0 && primerBL-blactual < power(NPUNTEROS, nivel-1)){  // Entonces habra que liberar el puntero

            if (nivel > 1){  // CASO 1: Si el padre es un bloque de punteros de nivel > 1 (apunta a otro bloque de punteros)
                int lib = liberar_bloque_punteros(buffer[i], nivel-1, blactual, primerBL, breads, bwrites);  // Lo liberamos
                if (lib == FALLO)
                    return FALLO;

                liberados += lib;
                if (blactual >= primerBL){  // Si todos los BL accesibles desde buffer[i] son >= primerBL
                    buffer[i] = 0;  // No hay BL menores que primerBL en el puntero y por lo tanto se ha liberado completamente
                    modificado = true;
                }
            }
            else{  // CASO 2: Si el padre es un bloque de punteros de nivel 1 (apunta a datos)
                if (liberar_bloque(buffer[i]) == FALLO) // Liberamos el bloque de datos
                    return FALLO;
#if DEBUGN6
                for (int d = 0; d < (4-nivel); d++) {fprintf(stderr, GRIS"|\t"RES);}
                fprintf(stderr, GRIS"[liberado BF %d de datos (BL: %d)]\n"RES, buffer[i], blactual);
#endif
                liberados++;
                modificado = true;
                buffer[i] = 0;
            }
        }

        blactual += power(NPUNTEROS, nivel-1); // Al pasar un puntero de nivel 'nivel-1' hemos pasado todos estos BL
        // (los BL que abarca un bloque de punteros son NPUNTEROS^nivel)
    }

    // Finalmente liberamos el bloque de punteros padre si no contiene ningun BL a preservar (que sea menor que primerBL)
    if (blinicial >= primerBL){
        if (liberar_bloque(nbloque)==FALLO)
            return FALLO;

        liberados++;
#if DEBUGN6
        for (int d = 0; d < (4-nivel); d++) {fprintf(stderr, GRIS"|\t"RES);}
        fprintf(stderr, GRIS"[liberado BF %d de punteros nivel %d (BL %d-%d)]\n"RES, nbloque, nivel, blinicial, blactual-1);
#endif
    }
    else if (modificado){  // Si no liberamos este bloque de punteros y lo hemos modificado, guardamos las modificaciones
#if DEBUGN6
        for (int d = 0; d < (4-nivel); d++) {fprintf(stderr, GRIS"|\t"RES);}
        fprintf(stderr, GRIS"[actualizado BF %d de punteros nivel %d (BL %d-%d)]\n"RES, nbloque, nivel, blinicial, blactual-1);
#endif
        (*bwrites)++;
        if (bwrite(nbloque, buffer)==FALLO)
            return FALLO;
    }

#if DEBUGN6
    for (int d = 0; d < (3-nivel); d++) {fprintf(stderr, GRIS"|\t");}
    fprintf(stderr, "\n");
#endif

    return liberados;
}

/* --- LIBERAR_INODO (nivel 6) --------------------------------------------------------------------
    Libera inodo, que este pasará a la cabeza de la lista de inodos libres, y también se
    recorrerá la estructura de enlaces del inodo para liberar todos aquellos bloques de 
    datos que se estaba ocupando, más todos aquellos bloques indices

    Devuelve el numero de inodo liberado o -1 si hay error
    Utilizada por:
        - Ejecutable truncar.c
*/
int liberar_inodo(unsigned int ninodo){
    struct inodo inodo;

    //Leemos el inodo
    if(leer_inodo(ninodo,&inodo)== FALLO){
        return FALLO;
    }

    int bloquesLiberados= liberar_bloques_inodo(0,&inodo);
    inodo.numBloquesOcupados-= bloquesLiberados;

    //Debería quedar 0 después de llamar a la función liberar_bloques_inodo
    if (inodo.numBloquesOcupados != 0){
        return FALLO;
    }
    //Marcamos el inodo com tipo libre
    inodo.tipo= 'l';
    inodo.tamEnBytesLog=0;

    //ACTUALIZAMOS LA LISTA ENLAZADA DE INODOS LIBRES
    //Leemosn el superbloque
    struct superbloque SB;
    if (bread(posSB, &SB) == FALLO){
        return FALLO;
    }

    //Actualizamos la lista enlaxada de inodos libres
    int auxInodo =SB.posPrimerInodoLibre;
    SB.posPrimerInodoLibre= ninodo;
    inodo.punterosDirectos[0]= auxInodo;

    //Incrementamos la cantidad de inodos libres
    SB.cantInodosLibres++;

    //Escribimos el superbloque en el dispositivo virtual
    if(bwrite(posSB,&SB) == FALLO){
        return FALLO;
    }


    //Actualizamos el ctime
    inodo.ctime= time(NULL);

    //Escribimos el inodo actualizado en el dispositivo virutal
    if(escribir_inodo(ninodo, &inodo) ==FALLO){
        return FALLO;
    }

    return ninodo;
}

// --------------------- EXTRA ------------------------------------------------------------------------------------------------------------

/* --- COPIAR_BLOQUES_INODO (extra krishna) -------------------------------------------------------
    Recorre todos los bloques del inodo src, y los copia, creando la misma estructura en el inodo dst.
    El inodo dst deberia estar vacio (tamEnBytesLog = 0)

    Devuelve la cantidad de bloques copiados (-1 si ha error)
*/
int copiar_bloques_inodo(struct inodo *dst, struct inodo *src){
    int copiados = 0;

    // Copiamos los bloques de punterosDirectos
    for (int i = 0; i < 12; i++) {
        if (src->punterosDirectos[i] == 0)
            continue;

        int cp = clonar_bloque(&dst->punterosDirectos[i], src->punterosDirectos[i], 0);
        if (cp == FALLO)
            return FALLO;
        
        copiados += cp;
    }

    for (int i = 0; i < 3; i++) {  // Copiamos los bloques de punterosIndirectos
        if (src->punterosIndirectos[i] == 0)
            continue;

        int cp = clonar_bloque(&dst->punterosIndirectos[i], src->punterosIndirectos[i], i+1);
        if (cp == FALLO)
            return FALLO;

        copiados += cp;
    }

    dst->tamEnBytesLog = src->tamEnBytesLog;
    dst->numBloquesOcupados = copiados;
    return copiados;
}

/* --- CLONAR_BLOQUE (extra krishna) --------------------------------------------------------------
    Funcion auxiliar a copiar_bloques_inodo. Crea una copia de un bloque de datos o de punteros 'src'.
    Y la guarda en *'dst'. Si es de punteros, lo hace recursivamente, copiando los bloques hijo.

    Devuelve el numero de bloques copiados o -1 si hay error.
*/
int clonar_bloque(unsigned int* dst, unsigned int src, int nivel) {
    *dst = reservar_bloque();
    if (*dst == FALLO)
        return FALLO;

    if (nivel == 0) {  // Si nivel = 0, src es un bloque de datos
        char buffer[BLOCKSIZE];
        if (bread(src, buffer) == FALLO)
            return FALLO;

        if (bwrite(*dst, buffer) == FALLO)
            return FALLO;

        return 1;
    }

    unsigned int BPsrc[NPUNTEROS];  // Si nivel > 0, es un bloque de punteros
    unsigned int BPdst[NPUNTEROS];
    int copiados = 0;

    if (bread(src, BPsrc) == FALLO)  // Leemos el BP original
        return FALLO;

    for (int i = 0; i < NPUNTEROS; i++) {  // Recorremos los punteros del BP
        if (BPsrc[i] == 0){
            BPdst[i] = 0;
            continue;
        }

        // Si el puntero i no es 0 en el bloque original, clonamos el bloque al que apunta
        // Y lo asignamos al puntero i del bloque copia
        int cp = clonar_bloque(&BPdst[i], BPsrc[i], nivel-1);
        if (cp == FALLO)
            return FALLO;

        copiados += cp;
    }

    // Finalmente, escribimos el contenido en el bloque de punteros copia que hemos reservado
    if (bwrite(*dst, BPdst) == FALLO)
        return FALLO;

    return copiados+1;
}


