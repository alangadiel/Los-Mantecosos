/*
 * Service.h
 *
 *  Created on: 9/6/2017
 *      Author: utnso
 */
#ifndef SERVICE_H_
#define SERVICE_H_

#include <parser/metadata_program.h>
#include "SocketsL.h"

//Tipos de ExitCode
#define FINALIZACIONNORMAL 0
#define NOSEPUDIERONRESERVARRECURSOS -1
#define ACCEDERAARCHIVOINEXISTENTE -2
#define LEERARCHIVOSINPERMISOS -3
#define ESCRIBIRARCHIVOSINPERMISOS -4
#define EXCEPCIONDEMEMORIA -5
#define DESCONEXIONDECONSOLA -6
#define DESCONECTADODESDECOMANDOCONSOLA -7
#define SOLICITUDMASGRANDEQUETAMANIOPAGINA -8
#define NOSEPUEDENASIGNARMASPAGINAS -9
#define STACKOVERFLOW -10
#define ERRORSINDEFINIR -20

#define INDEX_NUEVOS 0
#define INDEX_LISTOS 1
#define INDEX_EJECUTANDO 2
#define INDEX_BLOQUEADOS 3
#define INDEX_FINALIZADOS 4

#define NUEVOS "NUEVOS"
#define LISTOS "LISTOS"
#define EJECUTANDO "EJECUTANDO"
#define BLOQUEADOS "BLOQUEADOS"
#define FINALIZADOS "FINALIZADOS"

/*
typedef struct {
	FILE * file;
	BloqueControlProceso pcb;
} PeticionTamanioStack;

*/

typedef struct {
	uint32_t pid;
	uint32_t nroPagina; 		//Numero de pagina DEL PROCESO
	int32_t espacioDisponible;
} PaginaDelProceso;

extern t_list* ArchivosGlobales;
extern t_list* ArchivosProcesos;
extern t_list* PIDsPorSocketConsola;
extern t_list* CPUsConectadas;
extern t_list* Semaforos;
extern t_list* VariablesCompartidas;
extern t_list* PaginasPorProceso;
extern t_queue* Nuevos;
extern t_queue* Finalizados;
extern t_queue* Bloqueados;
extern t_queue* Ejecutando;
extern t_queue* Listos;
/*
extern t_list* Estados;
extern t_list* EstadosConProgramasFinalizables;
*/

extern bool end;
extern int pidAFinalizar;
extern uint32_t TamanioPagina;

extern int socketConMemoria;
extern int socketConFS;
int ultimoPID;
//Variables archivo de configuracion
extern char* IP_PROG;
extern int PUERTO_PROG;
extern int PUERTO_CPU;
extern char* IP_MEMORIA;
extern int PUERTO_MEMORIA;
extern char* IP_FS;
extern int PUERTO_FS;

extern int QUANTUM;
extern uint32_t QUANTUM_SLEEP;
extern char* ALGORITMO;
extern int GRADO_MULTIPROG;
extern char** SEM_IDS;
extern char** SEM_INIT;
extern char** SHARED_VARS;
extern uint32_t STACK_SIZE;
extern bool planificacion_detenida;

extern sem_t semDispacherListos;
extern sem_t semDispatcherCpus;
extern pthread_mutex_t mutexQueueNuevos;
extern pthread_mutex_t mutexQueueListos;
extern pthread_mutex_t mutexQueueEjecutando;
extern pthread_mutex_t mutexQueueBloqueados;
extern pthread_mutex_t mutexQueueFinalizados;
extern pthread_mutex_t mutexFinalizarPrograma;
extern pthread_mutex_t mutexQueuesProcesos;
extern pthread_mutex_t mutexCPUsConectadas;
extern pthread_mutex_t mutexSemaforos;
extern pthread_mutex_t mutexVariablesCompartidas;
extern pthread_mutex_t mutexPaginasPorProceso;
extern pthread_mutex_t mutexConsolaFD1;

void CrearListasEstados();
void CrearNuevoProceso(BloqueControlProceso* pcb,int* ultimoPid,t_queue* nuevos);
void LiberarVariablesYListas();
void CrearListasYSemaforos();
void obtenerError(int exitCode);
//void Evento_ListosRemove();
void Evento_ListosAdd();//para el dispacher

#endif /* SERVICE_H_ */
