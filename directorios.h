// Krishna Jorda Jimenez, Margalida Covas Roig , Nahuel Vazquez
#include "ficheros.h"

#define DEBUGN7 0  //leer_sf.c
#define DEBUGN8 0
#define DEBUGN9 0

#define ERROR_CAMINO_INCORRECTO -2
#define ERROR_PERMISO_LECTURA -3
#define ERROR_NO_EXISTE_ENTRADA_CONSULTA -4
#define ERROR_NO_EXISTE_DIRECTORIO_INTERMEDIO -5
#define ERROR_PERMISO_ESCRITURA -6
#define ERROR_ENTRADA_YA_EXISTENTE -7
#define ERROR_NO_SE_PUEDE_CREAR_ENTRADA_EN_UN_FICHERO -8

#define TAMFILA 100
#define TAMBUFFER (TAMFILA*1000)
#define TAMBUFFERCAT (BLOCKSIZE*4)
#define TAMNOMBRE 60 //tamaño del nombre de directorio o fichero, en Ext2 = 256
#define NCACHE 3
#define PROFUNDIDAD 32 //profundidad máxima del árbol de directorios

struct UltimaEntrada{
  char camino [TAMNOMBRE*PROFUNDIDAD];
  int p_inodo;
};

struct entrada {     // sizeof(struct entrada) = 64
  char nombre[TAMNOMBRE];
  unsigned int ninodo;
};

#define NENTRADAS (BLOCKSIZE/sizeof(struct entrada))

int extraer_camino(const char *camino, char *inicial, char *final, char *tipo);
int buscar_entrada(const char *camino_parcial, unsigned int *p_inodo_dir, unsigned int *p_inodo, unsigned int *p_entrada, char reservar, unsigned char permisos);
void mostrar_error_buscar_entrada(int error);
int mi_chmod(const char *camino, unsigned char permisos);
int mi_stat(const char *camino, struct STAT *p_stat);
int mi_creat(const char *camino, unsigned char permisos);
int mi_dir(const char *camino, char *buffer, char tipo, char extended);
int mi_read(const char *camino, void *buf, unsigned int offset, unsigned int nbytes);
int mi_write(const char *camino, const void *buf, unsigned int offset, unsigned int nbytes);
int mi_link(const char *camino1, const char *camino2);
int mi_unlink(const char *camino);

int mi_copiar(unsigned int inodoDst, unsigned int inodoSrc);
int mi_cp(char* dst, char* src, bool dir);
char* extraer_ultimo(char* camino);
int mi_mv(char* dst, char* src);
