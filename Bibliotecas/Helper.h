
#ifndef HELPER_
#define HELPER_
#define LAMBDA(c_) ({ c_ _;}) //Para funciones lamda

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <unistd.h>
#include <commons/temporal.h>
#include <commons/log.h>
#include <commons/config.h>
#include <commons/collections/list.h>
#include <commons/collections/dictionary.h>
#include <commons/collections/queue.h>
#include <commons/string.h>
#include <commons/txt.h>
#include <commons/bitarray.h>
#include <pthread.h>
#include <fcntl.h>
#include <time.h>
#include <string.h>
#include <stdint.h>
#include <sys/types.h>
#include <limits.h>
#include <sys/stat.h>
#include <dirent.h>
#include <parser/parser.h>
#include <ctype.h>
#include <semaphore.h>
#include <assert.h>
#include <errno.h>
#include <signal.h>

typedef struct {
	uint32_t NumeroDePagina;
	uint32_t Offset; //Desplazamiento
	uint32_t Tamanio;
}__attribute__((packed)) PosicionDeMemoria;

typedef struct {
	t_nombre_variable ID; //el nombre de la variable
	PosicionDeMemoria Posicion;
}__attribute__((packed)) Variable; //de AnSISOP

typedef struct {
	t_list* Argumentos; //lista de Variable
	t_list* Variables; //Desplazamiento, lista de Variable
	uint32_t DireccionDeRetorno;
	PosicionDeMemoria PosVariableDeRetorno;
}__attribute__((packed)) regIndiceStack;
typedef struct {
	t_puntero_instruccion start;
	t_size offset;
} __attribute__((packed)) RegIndiceCodigo;
typedef struct {
	t_list* IndiceDeCodigo;    //Cada elemento seria un array de 2 ints
	t_dictionary* IndiceDeEtiquetas;
	t_list* IndiceDelStack; //lista de IndiceStack
	char* etiquetas;
	t_size etiquetas_size;
	int32_t ExitCode;
	uint32_t PID;
	uint32_t ProgramCounter;
	uint32_t PaginasDeCodigo;
	uint32_t cantTotalVariables;
	uint32_t cantidad_de_etiquetas;
	uint32_t cantidad_de_funciones;
	uint32_t cantidadDeRafagasAEjecutar; //esto se manda en send de kernel
	uint32_t cantidadDeRafagasEjecutadasHistorica; //esto se hace en cpu
	uint32_t cantidadDeRafagasEjecutadas; //esto se manda en send de cpu
	uint32_t cantidadSyscallEjecutadas;
	uint32_t cantidadAccionesAlocar;
	uint32_t cantidadAccionesLiberar;
	uint32_t cantBytesAlocados;
	uint32_t cantBytesLiberados;
}__attribute__((packed)) BloqueControlProceso;

typedef struct {
	//CUANDO MODFIQUEN EL PCB MODIFIQUEN ESTE IGUAL, PERO SIN LOS PUNTEROS
	t_size etiquetas_size;
	int32_t ExitCode;
	uint32_t PID;
	uint32_t ProgramCounter;
	uint32_t PaginasDeCodigo;
	uint32_t cantidad_de_etiquetas;
	uint32_t cantidad_de_funciones;
	uint32_t cantidadDeRafagasAEjecutar; //esto se manda en send de kernel
	uint32_t cantidadDeRafagasEjecutadasHistorica; //esto se hace en cpu
	uint32_t cantidadDeRafagasEjecutadas; //esto se manda en send de cpu
	uint32_t cantidadSyscallEjecutadas;
	uint32_t cantidadAccionesAlocar;
	uint32_t cantTotalVariables;
	uint32_t cantidadAccionesLiberar;
	uint32_t cantBytesAlocados;
	uint32_t cantBytesLiberados;
}__attribute__((packed)) IntsDelPCB;

typedef struct {
	int socketCPU;
	bool isFree;
	uint32_t pid;
} __attribute__((packed))DatosCPU;

typedef struct {
	uint32_t* Atributos;
	char* buffer;
}__attribute__((packed)) BufferArchivo;

char* getWord(char* string, int pos);
char* integer_to_string(int x);
char* obtenerTiempoString(time_t t);
int GetTamanioArchivo(FILE * f);
bool list_contains(t_list* list, void* item);
void imprimirArchivoConfiguracion();

void pcb_Create(BloqueControlProceso* pecebe, uint32_t pid_actual);
void pcb_Destroy(BloqueControlProceso* pecebe);

#endif /* HELPER_*/
