// Krishna Jorda Jimenez, Margalida Covas Roig , Nahuel Vazquez
#include "directorios.h"
#include <signal.h>
#include <sys/wait.h>
#define GRIS "\x1b[94m"
#define RES "\x1b[0m"
#define ROJO "\x1b[31m"
#define AZUL "\x1b[34m"
#define LILA "\x1b[35m"

#define REGMAX 500000
#define NUMPROCESOS 100
#define NUMESCRITURAS 50

struct REGISTRO { //sizeof(struct REGISTRO): 24 bytes
   time_t fecha; //Precisión segundos
   pid_t pid; //PID del proceso que lo ha creado
   int nEscritura; //Entero con el número de escritura, de 1 a 50 (orden por tiempo)
   int nRegistro; //Entero con el número del registro dentro del fichero (orden por posición)
};
