CC=gcc -lm
CFLAGS=-c -g -Wall -std=gnu99
LDFLAGS=-pthread

SOURCES=bloques.c mi_mkfs.c ficheros_basico.c leer_sf.c ficheros.c escribir.c leer.c permitir.c truncar.c directorios.c mi_chmod.c mi_stat.c mi_mkdir.c mi_touch.c mi_ls.c mi_escribir.c mi_cat.c  mi_link.c mi_cp.c mi_cp_f.c mi_rm.c mi_rmdir.c mi_mv.c semaforo_mutex_posix.c simulacion.c verificacion.c #mi_escribir_varios_difdirs.c mi_escribir_varios.c
LIBRARIES=bloques.o ficheros_basico.o ficheros.o directorios.o semaforo_mutex_posix.o
INCLUDES=bloques.h ficheros_basico.h ficheros.h directorios.h semaforo_mutex_posix.h simulacion.h verificacion.h
PROGRAMS=mi_mkfs leer_sf escribir leer permitir truncar mi_chmod mi_stat mi_mkdir mi_touch mi_ls mi_escribir mi_cat mi_link mi_cp mi_cp_f mi_rm mi_rmdir mi_mv simulacion verificacion# mi_escribir_varios_difdirs mi_escribir_varios
OBJS=$(SOURCES:.c=.o)

all: $(OBJS) $(PROGRAMS)

$(PROGRAMS): $(LIBRARIES) $(INCLUDES)
	$(CC) $(LDFLAGS) $(LIBRARIES) $@.o -o $@

%.o: %.c $(INCLUDES)
	$(CC) $(CFLAGS) -o $@ -c $<

.PHONY: clean
clean:
	rm -rf *.o *~ $(PROGRAMS) disco* ext*
