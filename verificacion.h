// Krishna Jorda Jimenez, Margalida Covas Roig , Nahuel Vazquez
#include "simulacion.h"
#define CANTIDAD 256

struct INFORMACION {
  int pid;
  unsigned int nEscrituras; //validadas 
  struct REGISTRO PrimeraEscritura;
  struct REGISTRO UltimaEscritura;
  struct REGISTRO MenorPosicion;
  struct REGISTRO MayorPosicion;
};

void escribirInfo(const struct INFORMACION *info, int proceso, char* fic);
