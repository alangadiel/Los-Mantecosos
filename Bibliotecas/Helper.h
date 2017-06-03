#ifndef HELPER_
#define HELPER_
#define LAMBDA(c_) ({ c_ _;}) //Para funciones lamda

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <unistd.h>
#include <commons/log.h>
#include <commons/collections/list.h>
#include <commons/string.h>
#include <commons/txt.h>
#include <pthread.h>
#include <fcntl.h>
#include <time.h>
#include <string.h>
#include <stdint.h>
#include <sys/types.h>
#include <limits.h>

typedef struct {
	uint32_t PID;
	uint32_t IndiceStack;
	uint32_t ProgramCounter;
	uint32_t IndiceDeCodigo[2];
	uint32_t PaginasDeCodigo;
	int ExitCode;
}__attribute__((packed)) BloqueControlProceso;


typedef struct {
	uint32_t* Atributos;
	char* buffer;

}__attribute__((packed)) BufferArchivo;

char* getWord(char* string, int pos);
char* integer_to_string(int x);
char* obtenerTiempoString(time_t t);
int GetTamanioArchivo(FILE * f);
bool list_contains(t_list* list, void* item);

#endif /* HELPER_*/
