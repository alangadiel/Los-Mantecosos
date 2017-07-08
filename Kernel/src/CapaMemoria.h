#ifndef CAPAMEMORIA_H_
#define CAPAMEMORIA_H_

#include "Service.h"
#include "ThreadsKernel.h"

typedef struct {
	char* nombreVariableGlobal;
	uint32_t valorVariableGlobal;
} VariableCompartida;

typedef struct {
	char* nombreSemaforo;
	uint32_t valorSemaforo;
	t_list* listaDeProcesos;
} Semaforo;

uint32_t ActualizarMetadata(uint32_t PID, uint32_t nroPagina, uint32_t cantAReservar,int32_t *tipoError);
uint32_t SolicitarHeap(uint32_t PID,uint32_t cantAReservar,int32_t *tipoError);
void SolicitudLiberacionDeBloque(uint32_t pid,uint32_t punteroALiberar,int32_t *tipoError);
int RecorrerHastaEncontrarUnMetadataUsed(void* datosPagina);

#endif /* CAPAMEMORIA_H_ */
