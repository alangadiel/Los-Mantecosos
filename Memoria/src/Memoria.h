#ifndef MEMORIA_H_
#define MEMORIA_H_

#include "SocketsL.h"
#include "Threads.h"
#include "Interface.h"

#define IP_MEMORIA 127.0.0.1
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
	pthread_t hilo;
	int socket;
} structHilo;

typedef struct {
	uint32_t Frame;
	uint32_t PID;
	uint32_t Pag;
} RegistroTablaPaginacion;

typedef struct {
	uint32_t PID;
	uint32_t Pag;
	void* contenido;
} tabla_Cache;

extern void* BloquePrincipal;
extern void* ContenidoMemoria;
extern int tamanioTotalBytesMemoria;
extern int tamEstructurasAdm;
extern int cantPagAsignadas;
extern int socketABuscar;
extern t_list* listaHilos;
extern char end;

#endif /* MEMORIA_H_ */
