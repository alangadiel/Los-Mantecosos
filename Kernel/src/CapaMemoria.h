#ifndef CAPAMEMORIA_H_
#define CAPAMEMORIA_H_

#include "Service.h"

typedef struct {
 uint32_t size;
 bool isFree;
}__attribute__((packed)) HeapMetadata;

// hay que leerlo de la memoria o guardarlo aca?
typedef struct {
	char* nombreVariableGlobal;
	uint32_t valorVariableGlobal;
} variableGlobal;

typedef struct {
	char* nombreSemaforo;
	uint32_t valorSemaforo;
} semaforo;


uint32_t ActualizarMetadata(uint32_t PID, uint32_t nroPagina, uint32_t cantAReservar, int socketFD);
uint32_t SolicitarHeap(uint32_t PID,uint32_t cantAReservar,int socket);
void SolicitudLiberacionDeBloque(int socketFD,uint32_t pid,PosicionDeMemoria pos);
int RecorrerHastaEncontrarUnMetadataUsed(void* datosPagina);

#endif /* CAPAMEMORIA_H_ */
