#ifndef MEMORIA_H_
#define MEMORIA_H_

#include "SocketsL.h"
#include "Interface.h"
#include "ThreadsMemoria.h"

#define DATOS ((uint32_t*)paquete.Payload)
#define TablaDePagina ((RegistroTablaPaginacion*)BloquePrincipal)

//Variables del Archivo de Config
extern int PUERTO;
extern uint32_t MARCOS;
extern uint32_t MARCO_SIZE;
extern uint32_t ENTRADAS_CACHE;
extern uint32_t CACHE_X_PROC;
extern char* REEMPLAZO_CACHE;
extern unsigned int RETARDO_MEMORIA;
extern char* IP;

typedef struct {
	uint32_t Frame;
	uint32_t PID;
	uint32_t Pag;
} RegistroTablaPaginacion;

typedef struct {
	uint32_t PID;
	uint32_t Pag;
	uint32_t Contenido; //El frame correspondiente en la memoria principal.
} entradaCache;

extern t_list* tablaCache;
extern void* BloquePrincipal;
extern void* ContenidoMemoria;
extern int tamanioTotalBytesMemoria;
extern int tamEstructurasAdm;
extern int cantPagAsignadas;
extern int socketABuscar;
extern t_list* tablaCache;
extern t_list* listaHilos;
extern bool end;

//semaforos
extern pthread_mutex_t mutexTablaCache;
extern pthread_mutex_t mutexTablaPagina;
extern pthread_mutex_t mutexContenidoMemoria;

#endif /* MEMORIA_H_ */
